/* $Id: genworld.cpp 26371 2014-02-23 22:03:08Z frosch $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file genworld.cpp Functions to generate a map. */

#include "stdafx.h"
#include <algorithm>
#include <vector>
#include "landscape.h"
#include "company_func.h"
#include "genworld.h"
#include "gfxinit.h"
#include "window_func.h"
#include "network/network.h"
#include "heightmap.h"
#include "viewport_func.h"
#include "date_func.h"
#include "engine_func.h"
#include "water.h"
#include "video/video_driver.hpp"
#include "tilehighlight_func.h"
#include "saveload/saveload.h"
#include "void_map.h"
#include "town.h"
#include "newgrf.h"
#include "core/random_func.hpp"
#include "core/backup_type.hpp"
#include "progress.h"
#include "error.h"
#include "game/game.hpp"
#include "game/game_instance.hpp"
#include "pathfinder/npf/aystar.h"
#include "string_func.h"

#include "safeguards.h"


void GenerateClearTile();
void GenerateIndustries();
void GenerateObjects();
void GenerateTrees();

void StartupEconomy();
void StartupCompanies();
void StartupDisasters();

void InitializeGame(uint size_x, uint size_y, bool reset_date, bool reset_settings);

/**
 * Please only use this variable in genworld.h and genworld.cpp and
 *  nowhere else. For speed improvements we need it to be global, but
 *  in no way the meaning of it is to use it anywhere else besides
 *  in the genworld.h and genworld.cpp!
 */
GenWorldInfo _gw;

/** Whether we are generating the map or not. */
bool _generating_world;

/**
 * Tells if the world generation is done in a thread or not.
 * @return the 'threaded' status
 */
bool IsGenerateWorldThreaded()
{
	return _gw.threaded && !_gw.quit_thread;
}

/**
 * Clean up the 'mess' of generation. That is, show windows again, reset
 * thread variables, and delete the progress window.
 */
static void CleanupGeneration()
{
	_generating_world = false;

	SetMouseCursorBusy(false);
	/* Show all vital windows again, because we have hidden them */
	if (_gw.threaded && _game_mode != GM_MENU) ShowVitalWindows();
	SetModalProgress(false);
	_gw.proc     = NULL;
	_gw.abortp   = NULL;
	_gw.threaded = false;

	DeleteWindowByClass(WC_MODAL_PROGRESS);
	ShowFirstError();
	MarkWholeScreenDirty();
}



static const uint PUBLIC_ROAD_HASH_SIZE = 8U; ///< The number of bits the hash for river finding should have.

/**
* Simple hash function for public road tiles to be used by AyStar.
* @param tile The tile to hash.
* @param dir The unused direction.
* @return The hash for the tile.
*/
static uint PublicRoad_Hash(uint tile, uint dir)
{
	return GB(TileHash(TileX(tile), TileY(tile)), 0, PUBLIC_ROAD_HASH_SIZE);
}

/* AyStar callback for getting the cost of the current node. */
static int32 PublicRoad_CalculateG(AyStar *aystar, AyStarNode *current, OpenListNode *parent)
{
	return 1;
}

/* AyStar callback for getting the estimated cost to the destination. */
static int32 PublicRoad_CalculateH(AyStar *aystar, AyStarNode *current, OpenListNode *parent)
{
	return DistanceManhattan(*(TileIndex*)aystar->user_target, current->tile);
}

/* Helper function to check if a road along this tile path is possible. */
static bool CanBuildRoadFromTo(TileIndex begin, TileIndex end)
{
	assert(DistanceManhattan(begin, end) == 1);

	int heightBegin;
	int heightEnd;
	Slope slopeBegin = GetTileSlope(begin, &heightBegin);
	Slope slopeEnd = GetTileSlope(end, &heightEnd);

	return 
		/* Slope either is inclined or flat; rivers don't support other slopes. */
		(slopeEnd == SLOPE_FLAT || IsInclinedSlope(slopeEnd)) &&
		/* Slope continues, then it must be lower... or either end must be flat. */
		((slopeEnd == slopeBegin && heightEnd != heightBegin) || slopeEnd == SLOPE_FLAT || slopeBegin == SLOPE_FLAT);
}

/* AyStar callback for getting the neighbouring nodes of the given node. */
static void PublicRoad_GetNeighbours(AyStar *aystar, OpenListNode *current)
{
	TileIndex tile = current->path.node.tile;

	aystar->num_neighbours = 0;
	for (DiagDirection d = DIAGDIR_BEGIN; d < DIAGDIR_END; d++) {
		TileIndex t2 = tile + TileOffsByDiagDir(d);
		if (IsValidTile(t2) && CanBuildRoadFromTo(tile, t2) &&
			(IsTileType(t2, MP_CLEAR) || IsTileType(t2, MP_TREES) || IsTileType(t2, MP_ROAD))) {
			aystar->neighbours[aystar->num_neighbours].tile = t2;
			aystar->neighbours[aystar->num_neighbours].direction = INVALID_TRACKDIR;
			aystar->num_neighbours++;
		}
	}
}

/* AyStar callback for checking whether we reached our destination. */
static int32 PublicRoad_EndNodeCheck(AyStar *aystar, OpenListNode *current)
{
	return current->path.node.tile == *(TileIndex*)aystar->user_target ? AYSTAR_FOUND_END_NODE : AYSTAR_DONE;
}
/* AyStar callback when an route has been found. */
static void PublicRoad_FoundEndNode(AyStar *aystar, OpenListNode *current)
{
	/* Count river length. */
	uint length = 0;

	for (PathNode *path = &current->path; path != NULL; path = path->parent) {
		length++;
	}

	uint cur_pos = 0;
	PathNode* child = nullptr;
	for (PathNode *path = &current->path; path != NULL; path = path->parent, cur_pos++) {
		TileIndex tile = path->node.tile;

		TownID townID = CalcClosestTownFromTile(tile)->index;
		RoadBits roadBits = ROAD_NONE;

		if (child != nullptr) {
			TileIndex tile2 = child->node.tile;			
			roadBits |= DiagDirToRoadBits(DiagdirBetweenTiles(tile, tile2));
		}
		if (path->parent != nullptr) {
			TileIndex tile2 = path->parent->node.tile;
			roadBits |= DiagDirToRoadBits(DiagdirBetweenTiles(tile, tile2));
		}

		if (child != nullptr || path->parent != nullptr) {
			if (GetTileType(tile) == MP_ROAD) {
				SetRoadBits(tile, GetRoadBits(tile, ROADTYPE_ROAD) | roadBits, ROADTYPE_ROAD);
			} else {
				MakeRoadNormal(tile, roadBits, ROADTYPES_ROAD, townID, OWNER_TOWN, OWNER_NONE);
			}
		}

		child = path;
	}
}

/**
* Build the public road network connecting towns using AyStar.
*/
static void GeneratePublicRoads()
{
	Town* town;
	std::vector<TileIndex> towns;
	std::vector<TileIndex> already_connected_towns;
	std::vector<TileIndex> unreachable_towns;

	FOR_ALL_TOWNS(town) {
		towns.push_back(town->xy);
	}

	AyStar finder;
	MemSetT(&finder, 0);
	TileIndex begin;
	TileIndex end;

	begin = *towns.begin();
	already_connected_towns.push_back(begin);
	towns.erase(towns.begin());

	while (!towns.empty()) {
		begin = *towns.begin();

		std::sort(already_connected_towns.begin(),
			already_connected_towns.end(),
			[begin](TileIndex a, TileIndex b) { return DistanceManhattan(begin, a) < DistanceManhattan(begin, b); });

		bool found_connection = false;

		for (auto iter2 = already_connected_towns.begin(); iter2 != already_connected_towns.end(); iter2++) {
			end = *iter2;

			finder.CalculateG = PublicRoad_CalculateG;
			finder.CalculateH = PublicRoad_CalculateH;
			finder.GetNeighbours = PublicRoad_GetNeighbours;
			finder.EndNodeCheck = PublicRoad_EndNodeCheck;
			finder.FoundEndNode = PublicRoad_FoundEndNode;
			finder.user_target = &end;

			finder.Init(PublicRoad_Hash, 1 << PUBLIC_ROAD_HASH_SIZE);

			AyStarNode start;
			start.tile = begin;
			start.direction = INVALID_TRACKDIR;
			finder.AddStartNode(&start, 0);

			if (finder.Main() == AYSTAR_FOUND_END_NODE) {
				already_connected_towns.push_back(end);
				found_connection = true;
				break;
			}
		}

		if (!found_connection) {
			towns.erase(towns.begin());
			unreachable_towns.push_back(begin);
		}

		// Remove already connected towns from the townlist.
		for (auto iter3 = already_connected_towns.begin(); iter3 != already_connected_towns.end(); iter3++) {
			std::remove(towns.begin(), towns.end(), *iter3);
		}
	}
	finder.Free();
}

/**
 * The internal, real, generate function.
 */
static void _GenerateWorld(void *)
{
	/* Make sure everything is done via OWNER_NONE. */
	Backup<CompanyByte> _cur_company(_current_company, OWNER_NONE, FILE_LINE);

	try {
		_generating_world = true;
		_modal_progress_work_mutex->BeginCritical();
		if (_network_dedicated) DEBUG(net, 1, "Generating map, please wait...");
		/* Set the Random() seed to generation_seed so we produce the same map with the same seed */
		if (_settings_game.game_creation.generation_seed == GENERATE_NEW_SEED) _settings_game.game_creation.generation_seed = _settings_newgame.game_creation.generation_seed = InteractiveRandom();
		_random.SetSeed(_settings_game.game_creation.generation_seed);
		SetGeneratingWorldProgress(GWP_MAP_INIT, 2);
		SetObjectToPlace(SPR_CURSOR_ZZZ, PAL_NONE, HT_NONE, WC_MAIN_WINDOW, 0);

		BasePersistentStorageArray::SwitchMode(PSM_ENTER_GAMELOOP);

		IncreaseGeneratingWorldProgress(GWP_MAP_INIT);
		/* Must start economy early because of the costs. */
		StartupEconomy();

		/* Don't generate landscape items when in the scenario editor. */
		if (_gw.mode == GWM_EMPTY) {
			SetGeneratingWorldProgress(GWP_OBJECT, 1);

			/* Make sure the tiles at the north border are void tiles if needed. */
			if (_settings_game.construction.freeform_edges) {
				for (uint row = 0; row < MapSizeY(); row++) MakeVoid(TileXY(0, row));
				for (uint col = 0; col < MapSizeX(); col++) MakeVoid(TileXY(col, 0));
			}

			/* Make the map the height of the setting */
			if (_game_mode != GM_MENU) FlatEmptyWorld(_settings_game.game_creation.se_flat_world_height);

			ConvertGroundTilesIntoWaterTiles();
			IncreaseGeneratingWorldProgress(GWP_OBJECT);
		} else {
			GenerateLandscape(_gw.mode);
			GenerateClearTile();

			/* only generate towns, tree and industries in newgame mode. */
			if (_game_mode != GM_EDITOR) {
				if (!GenerateTowns(_settings_game.economy.town_layout)) {
					_cur_company.Restore();
					HandleGeneratingWorldAbortion();
					return;
				}
				GenerateIndustries();
				GenerateObjects();
				GenerateTrees();
				GeneratePublicRoads();
			}
		}

		/* These are probably pointless when inside the scenario editor. */
		SetGeneratingWorldProgress(GWP_GAME_INIT, 3);
		StartupCompanies();
		IncreaseGeneratingWorldProgress(GWP_GAME_INIT);
		StartupEngines();
		IncreaseGeneratingWorldProgress(GWP_GAME_INIT);
		StartupDisasters();
		_generating_world = false;

		/* No need to run the tile loop in the scenario editor. */
		if (_gw.mode != GWM_EMPTY) {
			uint i;

			SetGeneratingWorldProgress(GWP_RUNTILELOOP, 0x500);
			for (i = 0; i < 0x500; i++) {
				RunTileLoop();
				_tick_counter++;
				IncreaseGeneratingWorldProgress(GWP_RUNTILELOOP);
			}

			if (_game_mode != GM_EDITOR) {
				Game::StartNew();

				if (Game::GetInstance() != NULL) {
					SetGeneratingWorldProgress(GWP_RUNSCRIPT, 2500);
					_generating_world = true;
					for (i = 0; i < 2500; i++) {
						Game::GameLoop();
						IncreaseGeneratingWorldProgress(GWP_RUNSCRIPT);
						if (Game::GetInstance()->IsSleeping()) break;
					}
					_generating_world = false;
				}
			}
		}

		BasePersistentStorageArray::SwitchMode(PSM_LEAVE_GAMELOOP);

		ResetObjectToPlace();
		_cur_company.Trash();
		_current_company = _local_company = _gw.lc;

		SetGeneratingWorldProgress(GWP_GAME_START, 1);
		/* Call any callback */
		if (_gw.proc != NULL) _gw.proc();
		IncreaseGeneratingWorldProgress(GWP_GAME_START);

		CleanupGeneration();
		_modal_progress_work_mutex->EndCritical();

		ShowNewGRFError();

		if (_network_dedicated) DEBUG(net, 1, "Map generated, starting game");
		DEBUG(desync, 1, "new_map: %08x", _settings_game.game_creation.generation_seed);

		if (_debug_desync_level > 0) {
			char name[MAX_PATH];
			seprintf(name, lastof(name), "dmp_cmds_%08x_%08x.sav", _settings_game.game_creation.generation_seed, _date);
			SaveOrLoad(name, SLO_SAVE, DFT_GAME_FILE, AUTOSAVE_DIR, false);
		}
	} catch (...) {
		BasePersistentStorageArray::SwitchMode(PSM_LEAVE_GAMELOOP, true);
		if (_cur_company.IsValid()) _cur_company.Restore();
		_generating_world = false;
		_modal_progress_work_mutex->EndCritical();
		throw;
	}
}

/**
 * Set here the function, if any, that you want to be called when landscape
 * generation is done.
 * @param proc callback procedure
 */
void GenerateWorldSetCallback(GWDoneProc *proc)
{
	_gw.proc = proc;
}

/**
 * Set here the function, if any, that you want to be called when landscape
 * generation is aborted.
 * @param proc callback procedure
 */
void GenerateWorldSetAbortCallback(GWAbortProc *proc)
{
	_gw.abortp = proc;
}

/**
 * This will wait for the thread to finish up his work. It will not continue
 * till the work is done.
 */
void WaitTillGeneratedWorld()
{
	if (_gw.thread == NULL) return;

	_modal_progress_work_mutex->EndCritical();
	_modal_progress_paint_mutex->EndCritical();
	_gw.quit_thread = true;
	_gw.thread->Join();
	delete _gw.thread;
	_gw.thread   = NULL;
	_gw.threaded = false;
	_modal_progress_work_mutex->BeginCritical();
	_modal_progress_paint_mutex->BeginCritical();
}

/**
 * Initializes the abortion process
 */
void AbortGeneratingWorld()
{
	_gw.abort = true;
}

/**
 * Is the generation being aborted?
 * @return the 'aborted' status
 */
bool IsGeneratingWorldAborted()
{
	return _gw.abort;
}

/**
 * Really handle the abortion, i.e. clean up some of the mess
 */
void HandleGeneratingWorldAbortion()
{
	/* Clean up - in SE create an empty map, otherwise, go to intro menu */
	_switch_mode = (_game_mode == GM_EDITOR) ? SM_EDITOR : SM_MENU;

	if (_gw.abortp != NULL) _gw.abortp();

	CleanupGeneration();

	if (_gw.thread != NULL) _gw.thread->Exit();

	SwitchToMode(_switch_mode);
}

/**
 * Generate a world.
 * @param mode The mode of world generation (see GenWorldMode).
 * @param size_x The X-size of the map.
 * @param size_y The Y-size of the map.
 * @param reset_settings Whether to reset the game configuration (used for restart)
 */
void GenerateWorld(GenWorldMode mode, uint size_x, uint size_y, bool reset_settings)
{
	if (HasModalProgress()) return;
	_gw.mode   = mode;
	_gw.size_x = size_x;
	_gw.size_y = size_y;
	SetModalProgress(true);
	_gw.abort  = false;
	_gw.abortp = NULL;
	_gw.lc     = _local_company;
	_gw.quit_thread   = false;
	_gw.threaded      = true;

	/* This disables some commands and stuff */
	SetLocalCompany(COMPANY_SPECTATOR);

	InitializeGame(_gw.size_x, _gw.size_y, true, reset_settings);
	PrepareGenerateWorldProgress();

	/* Load the right landscape stuff, and the NewGRFs! */
	GfxLoadSprites();
	LoadStringWidthTable();

	/* Re-init the windowing system */
	ResetWindowSystem();

	/* Create toolbars */
	SetupColoursAndInitialWindow();
	SetObjectToPlace(SPR_CURSOR_ZZZ, PAL_NONE, HT_NONE, WC_MAIN_WINDOW, 0);

	if (_gw.thread != NULL) {
		_gw.thread->Join();
		delete _gw.thread;
		_gw.thread = NULL;
	}

	if (!VideoDriver::GetInstance()->HasGUI() || !ThreadObject::New(&_GenerateWorld, NULL, &_gw.thread, "ottd:genworld")) {
		DEBUG(misc, 1, "Cannot create genworld thread, reverting to single-threaded mode");
		_gw.threaded = false;
		_modal_progress_work_mutex->EndCritical();
		_GenerateWorld(NULL);
		_modal_progress_work_mutex->BeginCritical();
		return;
	}

	UnshowCriticalError();
	/* Remove any open window */
	DeleteAllNonVitalWindows();
	/* Hide vital windows, because we don't allow to use them */
	HideVitalWindows();

	/* Don't show the dialog if we don't have a thread */
	ShowGenerateWorldProgress();

	/* Centre the view on the map */
	if (FindWindowById(WC_MAIN_WINDOW, 0) != NULL) {
		ScrollMainWindowToTile(TileXY(MapSizeX() / 2, MapSizeY() / 2), true);
	}
}

/* $Id: road.cpp 24900 2013-01-08 22:46:42Z planetmaker $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file road.cpp Generic road related functions. */

#include "stdafx.h"
#include <algorithm>
#include <vector>
#include "rail_map.h"
#include "road_map.h"
#include "water_map.h"
#include "genworld.h"
#include "company_func.h"
#include "company_base.h"
#include "engine_base.h"
#include "date_func.h"
#include "landscape.h"
#include "town.h"
#include "pathfinder/npf/aystar.h"
#include "tunnelbridge.h"
#include "command_func.h"
#include "core/backup_type.hpp"

#include "safeguards.h"

/**
 * Return if the tile is a valid tile for a crossing.
 *
 * @param tile the current tile
 * @param ax the axis of the road over the rail
 * @return true if it is a valid tile
 */
static bool IsPossibleCrossing(const TileIndex tile, Axis ax)
{
	return (IsTileType(tile, MP_RAILWAY) &&
		GetRailTileType(tile) == RAIL_TILE_NORMAL &&
		GetTrackBits(tile) == (ax == AXIS_X ? TRACK_BIT_Y : TRACK_BIT_X) &&
		GetFoundationSlope(tile) == SLOPE_FLAT);
}

/**
 * Clean up unnecessary RoadBits of a planed tile.
 * @param tile current tile
 * @param org_rb planed RoadBits
 * @return optimised RoadBits
 */
RoadBits CleanUpRoadBits(const TileIndex tile, RoadBits org_rb)
{
	if (!IsValidTile(tile)) return ROAD_NONE;
	for (DiagDirection dir = DIAGDIR_BEGIN; dir < DIAGDIR_END; dir++) {
		const TileIndex neighbor_tile = TileAddByDiagDir(tile, dir);

		/* Get the Roadbit pointing to the neighbor_tile */
		const RoadBits target_rb = DiagDirToRoadBits(dir);

		/* If the roadbit is in the current plan */
		if (org_rb & target_rb) {
			bool connective = false;
			const RoadBits mirrored_rb = MirrorRoadBits(target_rb);

			if (IsValidTile(neighbor_tile)) {
				switch (GetTileType(neighbor_tile)) {
					/* Always connective ones */
					case MP_CLEAR: case MP_TREES:
						connective = true;
						break;

					/* The conditionally connective ones */
					case MP_TUNNELBRIDGE:
					case MP_STATION:
					case MP_ROAD:
						if (IsNormalRoadTile(neighbor_tile)) {
							/* Always connective */
							connective = true;
						} else {
							const RoadBits neighbor_rb = GetAnyRoadBits(neighbor_tile, ROADTYPE_ROAD) | GetAnyRoadBits(neighbor_tile, ROADTYPE_TRAM);

							/* Accept only connective tiles */
							connective = (neighbor_rb & mirrored_rb) != ROAD_NONE;
						}
						break;

					case MP_RAILWAY:
						connective = IsPossibleCrossing(neighbor_tile, DiagDirToAxis(dir));
						break;

					case MP_WATER:
						/* Check for real water tile */
						connective = !IsWater(neighbor_tile);
						break;

					/* The definitely not connective ones */
					default: break;
				}
			}

			/* If the neighbor tile is inconnective, remove the planed road connection to it */
			if (!connective) org_rb ^= target_rb;
		}
	}

	return org_rb;
}

/**
 * Finds out, whether given company has all given RoadTypes available
 * @param company ID of company
 * @param rts RoadTypes to test
 * @return true if company has all requested RoadTypes available
 */
bool HasRoadTypesAvail(const CompanyID company, const RoadTypes rts)
{
	RoadTypes avail_roadtypes;

	if (company == OWNER_DEITY || company == OWNER_TOWN || _game_mode == GM_EDITOR || _generating_world) {
		avail_roadtypes = ROADTYPES_ROAD;
	} else {
		Company *c = Company::GetIfValid(company);
		if (c == NULL) return false;
		avail_roadtypes = (RoadTypes)c->avail_roadtypes | ROADTYPES_ROAD; // road is available for always for everybody
	}
	return (rts & ~avail_roadtypes) == 0;
}

/**
 * Validate functions for rail building.
 * @param rt road type to check.
 * @return true if the current company may build the road.
 */
bool ValParamRoadType(const RoadType rt)
{
	return HasRoadTypesAvail(_current_company, RoadTypeToRoadTypes(rt));
}

/**
 * Get the road types the given company can build.
 * @param company the company to get the roadtypes for.
 * @return the road types.
 */
RoadTypes GetCompanyRoadtypes(CompanyID company)
{
	RoadTypes rt = ROADTYPES_NONE;

	Engine *e;
	FOR_ALL_ENGINES_OF_TYPE(e, VEH_ROAD) {
		const EngineInfo *ei = &e->info;

		if (HasBit(ei->climates, _settings_game.game_creation.landscape) &&
				(HasBit(e->company_avail, company) || _date >= e->intro_date + DAYS_IN_YEAR)) {
			SetBit(rt, HasBit(ei->misc_flags, EF_ROAD_TRAM) ? ROADTYPE_TRAM : ROADTYPE_ROAD);
		}
	}

	return rt;
}

/* ========================================================================= */
/*                                PUBLIC ROADS                               */
/* ========================================================================= */

CommandCost CmdBuildTunnel(TileIndex start_tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text = NULL);

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

static const int32 BASE_COST = 100;          // Cost for utilizing an existing road, bridge, or tunnel.
static const int32 COST_FOR_NEW_ROAD = 1000; // Cost for building a new road.
static const int32 COST_FOR_TURN = 100;      // Additional cost if the road makes a turn.
static const int32 COST_FOR_BRIDGE = 1250;   // Cost for building a bridge.
static const int32 COST_FOR_TUNNEL = 1050;   // Cost for building a tunnel.
static const int32 COST_FOR_SLOPE = 250;     // Additional cost if the road heads up or down a slope.

/* AyStar callback for getting the cost of the current node. */
static int32 PublicRoad_CalculateG(AyStar *aystar, AyStarNode *current, OpenListNode *parent)
{
	int32 cost = BASE_COST;

	if (!IsTileType(current->tile, MP_ROAD)) {
		if (!AreTilesAdjacent(parent->path.node.tile, current->tile))
		{
			// We're not adjacent, so we built a tunnel or bridge.
			cost += (DistanceManhattan(parent->path.node.tile, current->tile)) * COST_FOR_NEW_ROAD + 4 * COST_FOR_SLOPE;
		}
		else if (!IsTileFlat(current->tile)) {
			cost += COST_FOR_NEW_ROAD;
			cost += COST_FOR_SLOPE;
		}
		else
		{
			cost += COST_FOR_NEW_ROAD;
		}

		//if (parent->path.parent != nullptr &&
		//	DiagdirBetweenTiles(parent->path.parent->node.tile, parent->path.node.tile) != DiagdirBetweenTiles(parent->path.node.tile, current->tile))
		//{
		//	cost += COST_FOR_TURN;
		//}
	}

	return cost;
}

/* AyStar callback for getting the estimated cost to the destination. */
static int32 PublicRoad_CalculateH(AyStar *aystar, AyStarNode *current, OpenListNode *parent)
{
	return DistanceManhattan(*(TileIndex*)aystar->user_target, current->tile) * BASE_COST;
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

static TileIndex BuildTunnel(PathNode *current, PathNode *previous, bool build_tunnel = false)
{
	if (!build_tunnel) {
		// Check if we are at a current slope.
		if (!IsInclinedSlope(GetTileSlope(current->node.tile))) return INVALID_TILE;

		// Check if it's an upward slope.
		DiagDirection slope_direction = GetInclinedSlopeDirection(GetTileSlope(current->node.tile));

		if (previous == nullptr) return INVALID_TILE;

		DiagDirection road_direction = DiagdirBetweenTiles(previous->node.tile, current->node.tile);

		if (slope_direction != road_direction) return INVALID_TILE;
	}

	Backup<CompanyByte> cur_company(_current_company, OWNER_DEITY, FILE_LINE);
	bool can_build_tunnel = CmdBuildTunnel(current->node.tile, build_tunnel ? DC_EXEC : DC_NONE, ROADTYPES_ROAD | (TRANSPORT_ROAD << 8), 0).Succeeded();
	cur_company.Restore();

	if (!can_build_tunnel) return INVALID_TILE;

	if (!IsTileType(_build_tunnel_endtile, MP_CLEAR) && !IsTileType(_build_tunnel_endtile, MP_TREES)) return INVALID_TILE;

	return _build_tunnel_endtile;
}

/* AyStar callback for getting the neighbouring nodes of the given node. */
static void PublicRoad_GetNeighbours(AyStar *aystar, OpenListNode *current)
{
	TileIndex tile = current->path.node.tile;

	aystar->num_neighbours = 0;

	if (IsTileType(tile, MP_TUNNELBRIDGE))
	{
		aystar->neighbours[aystar->num_neighbours].tile = GetOtherTunnelBridgeEnd(tile);
		aystar->neighbours[aystar->num_neighbours].direction = INVALID_TRACKDIR;
		aystar->num_neighbours++;

		auto direction_to_other_end = DiagdirBetweenTiles(tile, GetOtherTunnelBridgeEnd(tile));
		auto tunnel_direction = GetTunnelBridgeDirection(tile);
		auto reversed_tunnel_direction = ReverseDiagDir(tunnel_direction);

		TileIndex t2 = tile + TileOffsByDiagDir(reversed_tunnel_direction);
		auto slope = GetTileSlope(t2);
		if (IsValidTile(t2) && (slope == SLOPE_FLAT || IsInclinedSlope(slope)) &&
			(IsTileType(t2, MP_CLEAR) || IsTileType(t2, MP_TREES) || IsTileType(t2, MP_ROAD))) {
			aystar->neighbours[aystar->num_neighbours].tile = t2;
			aystar->neighbours[aystar->num_neighbours].direction = INVALID_TRACKDIR;
			aystar->num_neighbours++;
		}
	} else {
		if (current->path.parent != nullptr && !AreTilesAdjacent(current->path.node.tile, current->path.parent->node.tile)) {
			// We went through a potential tunnel, this limits our options to proceed to only forward.
			auto tunnel_direction = DiagdirBetweenTiles(current->path.parent->node.tile, current->path.node.tile);
			
			TileIndex t2 = tile + TileOffsByDiagDir(tunnel_direction);
			if (IsValidTile(t2) && CanBuildRoadFromTo(tile, t2) &&
				(IsTileType(t2, MP_CLEAR) || IsTileType(t2, MP_TREES) || IsTileType(t2, MP_ROAD) || IsTileType(t2, MP_TUNNELBRIDGE))) {
				aystar->neighbours[aystar->num_neighbours].tile = t2;
				aystar->neighbours[aystar->num_neighbours].direction = INVALID_TRACKDIR;
				aystar->num_neighbours++;
			}
		} else {
			for (DiagDirection d = DIAGDIR_BEGIN; d < DIAGDIR_END; d++) {
				TileIndex t2 = tile + TileOffsByDiagDir(d);
				if (IsValidTile(t2) && CanBuildRoadFromTo(tile, t2) &&
					(IsTileType(t2, MP_CLEAR) || IsTileType(t2, MP_TREES) || IsTileType(t2, MP_ROAD) || 
					(IsTileType(t2, MP_TUNNELBRIDGE) && GetTunnelBridgeDirection(t2) == DiagdirBetweenTiles(tile, t2)))) {
					aystar->neighbours[aystar->num_neighbours].tile = t2;
					aystar->neighbours[aystar->num_neighbours].direction = INVALID_TRACKDIR;
					aystar->num_neighbours++;
				}
			}

			auto tunnel_end = BuildTunnel(&current->path, current->path.parent);

			if (tunnel_end != INVALID_TILE) {
				aystar->neighbours[aystar->num_neighbours].tile = tunnel_end;
				aystar->neighbours[aystar->num_neighbours].direction = INVALID_TRACKDIR;
				aystar->num_neighbours++;
			}
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
	PathNode* child = nullptr;

	for (PathNode *path = &current->path; path != NULL; path = path->parent) {
		TileIndex tile = path->node.tile;

		TownID townID = CalcClosestTownFromTile(tile)->index;

		if (IsTileType(tile, MP_TUNNELBRIDGE)) {
			continue;
		} else if (path->parent == nullptr || AreTilesAdjacent(tile, path->parent->node.tile)) {
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
				}
				else {
					MakeRoadNormal(tile, roadBits, ROADTYPES_ROAD, townID, OWNER_TOWN, OWNER_NONE);
				}
			}
		} else {
			bool tile_and_child_adjacent = (child == nullptr || AreTilesAdjacent(tile, child->node.tile));
			bool tile_and_parent_adjacent = (path->parent == nullptr || AreTilesAdjacent(tile, path->parent->node.tile));

			BuildTunnel(path, child, true);
		}

		child = path;
	}
}

/**
* Build the public road network connecting towns using AyStar.
*/
void GeneratePublicRoads()
{
	AyStar finder;
	MemSetT(&finder, 0);

	std::vector<Town*> towns;
	std::vector<Town*> unconnected_towns;
	{
		Town* town;
		FOR_ALL_TOWNS(town) {
			unconnected_towns.push_back(town);
		}
	}

	do
	{
		towns.clear();
		std::for_each(unconnected_towns.begin(), unconnected_towns.end(), [&](auto t) { towns.push_back(t); });
		unconnected_towns.clear();

		std::sort(towns.begin(), towns.end(), [&](auto a, auto b) { return a->cache.population < b->cache.population; });

		Town* begin = *towns.begin();
		towns.erase(towns.begin());

		std::sort(towns.begin(), towns.end(), [&](auto a, auto b) { return DistanceManhattan(begin->xy, a->xy) < DistanceManhattan(begin->xy, b->xy); });

		std::for_each(towns.begin(), towns.end(), [&](auto end) {
			finder.CalculateG = PublicRoad_CalculateG;
			finder.CalculateH = PublicRoad_CalculateH;
			finder.GetNeighbours = PublicRoad_GetNeighbours;
			finder.EndNodeCheck = PublicRoad_EndNodeCheck;
			finder.FoundEndNode = PublicRoad_FoundEndNode;
			finder.user_target = &(end->xy);

			finder.Init(PublicRoad_Hash, 1 << PUBLIC_ROAD_HASH_SIZE);

			AyStarNode start;
			start.tile = begin->xy;
			start.direction = INVALID_TRACKDIR;
			finder.AddStartNode(&start, 0);

			if (finder.Main() != AYSTAR_FOUND_END_NODE)
			{
				unconnected_towns.push_back(end);
			}
		});
	} while (!unconnected_towns.empty());

	finder.Free();
}

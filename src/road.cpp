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
#include <map>
#include <memory>
#include <numeric>
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
#include "core/random_func.hpp"

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

/** Whether to build public roads */
enum PublicRoadsConstruction {
	PRC_NONE,         ///< Generate no public roads
	PRC_WITH_CURVES,  ///< Generate roads with lots of curves
	PRC_AVOID_CURVES, ///< Generate roads avoiding curves if possible
};

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

CommandCost CmdBuildBridge(TileIndex end_tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text = nullptr);
CommandCost CmdBuildTunnel(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text = nullptr);
CommandCost CmdBuildRoad(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text = nullptr);

static std::map<TileIndex, Town*> _town_centers;
static std::vector<Town*> _towns_visited_along_the_way;
static std::map<const Town*, std::shared_ptr<std::vector<Town*>>> _towns_reachable_networks;
static const uint PUBLIC_ROAD_HASH_SIZE = 8U; ///< The number of bits the hash for river finding should have.

static int checked_nodes = 0;
static std::vector<int> _checked_nodes_on_success;
static std::vector<int> _checked_nodes_on_failure;

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

static const int32 BASE_COST = 1;          // Cost for utilizing an existing road, bridge, or tunnel.
static const int32 COST_FOR_NEW_ROAD = 10; // Cost for building a new road.
static const int32 COST_FOR_SLOPE = 5;     // Additional cost if the road heads up or down a slope.

/* AyStar callback for getting the cost of the current node. */
static int32 PublicRoad_CalculateG(AyStar *aystar, AyStarNode *current, OpenListNode *parent)
{
	int32 cost = BASE_COST;

	if (!IsTileType(current->tile, MP_ROAD)) {
		if (!AreTilesAdjacent(parent->path.node.tile, current->tile))
		{
			// We're not adjacent, so we built a tunnel or bridge.
			cost += (DistanceManhattan(parent->path.node.tile, current->tile)) * COST_FOR_NEW_ROAD + 6 * COST_FOR_SLOPE;
		}
		else if (!IsTileFlat(current->tile)) {
			cost += COST_FOR_NEW_ROAD;
			cost += COST_FOR_SLOPE;
		}
		else
		{
			cost += COST_FOR_NEW_ROAD;
		}
	}

	if (_settings_game.game_creation.build_public_roads == PRC_AVOID_CURVES &&
		parent->path.parent != nullptr &&
		DiagdirBetweenTiles(parent->path.parent->node.tile, parent->path.node.tile) != DiagdirBetweenTiles(parent->path.node.tile, current->tile)) {
		cost += 1;
	}

	return cost;
}

/* AyStar callback for getting the estimated cost to the destination. */
static int32 PublicRoad_CalculateH(AyStar *aystar, AyStarNode *current, OpenListNode *parent)
{
	return DistanceManhattan(*(TileIndex*)aystar->user_target, current->tile) * BASE_COST;
}

/* Helper function to check if a tile along a certain direction is going up an inclined slope. */
static bool IsUpwardsSlope(TileIndex tile, DiagDirection road_direction)
{
	auto slope = GetTileSlope(tile);

	if (!IsInclinedSlope(slope)) return false;

	auto slope_direction = GetInclinedSlopeDirection(slope);

	return road_direction == slope_direction;
}

/* Helper function to check if a tile along a certain direction is going down an inclined slope. */
static bool IsDownwardsSlope(TileIndex tile, DiagDirection road_direction)
{
	auto slope = GetTileSlope(tile);

	if (!IsInclinedSlope(slope)) return false;

	auto slope_direction = GetInclinedSlopeDirection(slope);

	return road_direction == ReverseDiagDir(slope_direction);
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

static TileIndex BuildTunnel(PathNode *current, bool build_tunnel = false)
{
	Backup<CompanyByte> cur_company(_current_company, OWNER_DEITY, FILE_LINE);
	bool can_build_tunnel = CmdBuildTunnel(current->node.tile, build_tunnel ? DC_EXEC : DC_NONE, ROADTYPES_ROAD | (TRANSPORT_ROAD << 8), 0).Succeeded();
	cur_company.Restore();

	assert(!build_tunnel || can_build_tunnel);
	assert(!build_tunnel || (IsTileType(current->node.tile, MP_TUNNELBRIDGE) && IsTileType(_build_tunnel_endtile, MP_TUNNELBRIDGE)));

	if (!can_build_tunnel) return INVALID_TILE;
	if (!build_tunnel && !IsTileType(_build_tunnel_endtile, MP_CLEAR) && !IsTileType(_build_tunnel_endtile, MP_TREES)) return INVALID_TILE;

	return _build_tunnel_endtile;
}

static TileIndex BuildBridge(PathNode *current, TileIndex end_tile = INVALID_TILE, bool build_bridge = false)
{
	TileIndex start_tile = current->node.tile;

	DiagDirection direction = ReverseDiagDir(GetInclinedSlopeDirection(GetTileSlope(start_tile)));

	if (!build_bridge) {
		// We are not building yet, so we still need to find the end_tile.
		for (TileIndex tile = start_tile + TileOffsByDiagDir(direction);
			IsValidTile(tile) &&
			(GetTunnelBridgeLength(start_tile, tile) <= _settings_game.construction.max_bridge_length) &&
			(GetTileZ(start_tile) < (GetTileZ(tile) + _settings_game.construction.max_bridge_height)) &&
			(GetTileZ(tile) <= GetTileZ(start_tile));
			tile += TileOffsByDiagDir(direction)) {

			// No supershort bridges and always ending up on a matching upwards slope.
			if (!AreTilesAdjacent(start_tile, tile) && IsUpwardsSlope(tile, direction)) {
				end_tile = tile;
				break;
			}
		}

		if (!IsValidTile(end_tile)) return INVALID_TILE;
		if (!IsTileType(end_tile, MP_CLEAR) && !IsTileType(end_tile, MP_TREES)) return INVALID_TILE;
	}

	assert(!build_bridge || IsValidTile(end_tile));

	std::vector<BridgeType> available_bridge_types;

	for (uint i = 0; i < MAX_BRIDGES; ++i) {
		if (CheckBridgeAvailability((BridgeType)i, GetTunnelBridgeLength(start_tile, end_tile)).Succeeded()) {
			available_bridge_types.push_back((BridgeType)i);
		}
	}

	assert(!build_bridge || !available_bridge_types.empty());
	if (available_bridge_types.empty()) return INVALID_TILE;

	auto bridge_type = available_bridge_types[build_bridge ? RandomRange(available_bridge_types.size()) : 0];

	Backup<CompanyByte> cur_company(_current_company, OWNER_DEITY, FILE_LINE);
	bool can_build_bridge = CmdBuildBridge(end_tile, build_bridge ? DC_EXEC : DC_NONE, start_tile, bridge_type | (ROADTYPES_ROAD << 8) | (TRANSPORT_ROAD << 15)).Succeeded();
	cur_company.Restore();

	assert(!build_bridge || can_build_bridge);
	assert(!build_bridge || (IsTileType(start_tile, MP_TUNNELBRIDGE) && IsTileType(end_tile, MP_TUNNELBRIDGE)));

	if (!can_build_bridge) return INVALID_TILE;

	return end_tile;
}

static TileIndex BuildRiverBridge(PathNode *current, DiagDirection road_direction, TileIndex end_tile = INVALID_TILE, bool build_bridge = false)
{
	TileIndex start_tile = current->node.tile;

	if (!build_bridge) {
		// We are not building yet, so we still need to find the end_tile.
		// We will only build a bridge if we need to cross a river, so first check for that.
		TileIndex tile = start_tile + TileOffsByDiagDir(road_direction);

		if (!IsWaterTile(tile) || !IsRiver(tile)) return INVALID_TILE;

		// Now let's see if we can bridge it. But don't bridge anything more than 4 river tiles. Cities aren't allowed to, so public roads we are not either.
		// Only bridges starting at slopes should be longer ones. The others look like crap when built this way. Players can build them but the map generator
		// should not force that on them. This is just to bridge rivers, not to make long bridges.
		for (;
			IsValidTile(tile) &&
			(GetTunnelBridgeLength(start_tile, tile) <= 5) &&
			(GetTileZ(start_tile) < (GetTileZ(tile) + _settings_game.construction.max_bridge_height)) &&
			(GetTileZ(tile) <= GetTileZ(start_tile));
			tile += TileOffsByDiagDir(road_direction)) {

			if ((IsTileType(tile, MP_CLEAR) || IsTileType(tile, MP_TREES)) &&
				GetTileZ(tile) <= GetTileZ(start_tile) &&
				GetTileSlope(tile) == SLOPE_FLAT) {
				end_tile = tile;
				break;
			}
		}

		if (!IsValidTile(end_tile)) return INVALID_TILE;
		if (!IsTileType(end_tile, MP_CLEAR) && !IsTileType(end_tile, MP_TREES)) return INVALID_TILE;
	}

	assert(!build_bridge || IsValidTile(end_tile));

	std::vector<BridgeType> available_bridge_types;

	for (uint i = 0; i < MAX_BRIDGES; ++i) {
		if (CheckBridgeAvailability((BridgeType)i, GetTunnelBridgeLength(start_tile, end_tile)).Succeeded()) {
			available_bridge_types.push_back((BridgeType)i);
		}
	}

	auto bridge_type = available_bridge_types[build_bridge ? RandomRange(available_bridge_types.size()) : 0];

	Backup<CompanyByte> cur_company(_current_company, OWNER_DEITY, FILE_LINE);
	bool can_build_bridge = CmdBuildBridge(end_tile, build_bridge ? DC_EXEC : DC_NONE, start_tile, bridge_type | (ROADTYPES_ROAD << 8) | (TRANSPORT_ROAD << 15)).Succeeded();
	cur_company.Restore();

	assert(!build_bridge || can_build_bridge);
	assert(!build_bridge || (IsTileType(start_tile, MP_TUNNELBRIDGE) && IsTileType(end_tile, MP_TUNNELBRIDGE)));

	if (!can_build_bridge) return INVALID_TILE;

	return end_tile;
}

static bool IsValidNeighbourOfPreviousTile(TileIndex tile, TileIndex previous_tile)
{
	if (!IsValidTile(tile)) return false;

	if (IsTileType(tile, MP_TUNNELBRIDGE))
	{
		if (GetOtherTunnelBridgeEnd(tile) == previous_tile) return true;

		auto tunnel_direction = GetTunnelBridgeDirection(tile);

		if (previous_tile + TileOffsByDiagDir(tunnel_direction) != tile) return false;
	} else {

		if (!IsTileType(tile, MP_CLEAR) && !IsTileType(tile, MP_TREES) && !IsTileType(tile, MP_ROAD)) return false;

		auto slope = GetTileSlope(tile);

		if (IsInclinedSlope(slope)) {
			auto slope_direction = GetInclinedSlopeDirection(slope);
			auto road_direction = DiagdirBetweenTiles(previous_tile, tile);

			if (slope_direction != road_direction && ReverseDiagDir(slope_direction) != road_direction) return false;
		} else if (slope != SLOPE_FLAT) {
			return false;
		}
	}

	return true;
}

/* AyStar callback for getting the neighbouring nodes of the given node. */
static void PublicRoad_GetNeighbours(AyStar *aystar, OpenListNode *current)
{
	TileIndex tile = current->path.node.tile;

	aystar->num_neighbours = 0;

	// Check if we just went through a tunnel or a bridge.
	if (current->path.parent != nullptr && !AreTilesAdjacent(tile, current->path.parent->node.tile)) {
		auto previous_tile = current->path.parent->node.tile;
		// We went through a tunnel or bridge, this limits our options to proceed to only forward.
		auto tunnel_bridge_direction = DiagdirBetweenTiles(previous_tile, tile);

		TileIndex t2 = tile + TileOffsByDiagDir(tunnel_bridge_direction);
		if (IsValidNeighbourOfPreviousTile(t2, tile)) {
			aystar->neighbours[aystar->num_neighbours].tile = t2;
			aystar->neighbours[aystar->num_neighbours].direction = INVALID_TRACKDIR;
			aystar->num_neighbours++;
		}
	} else {
		// Handle all the regular neighbours and existing tunnels/bridges.
		std::vector<TileIndex> potential_neighbours;

		if (IsTileType(tile, MP_TUNNELBRIDGE)) {
			auto neighbour = GetOtherTunnelBridgeEnd(tile);

			aystar->neighbours[aystar->num_neighbours].tile = neighbour;
			aystar->neighbours[aystar->num_neighbours].direction = INVALID_TRACKDIR;
			aystar->num_neighbours++;

			neighbour = tile + TileOffsByDiagDir(ReverseDiagDir(DiagdirBetweenTiles(tile, neighbour)));

			if (IsValidNeighbourOfPreviousTile(neighbour, tile)) {
				aystar->neighbours[aystar->num_neighbours].tile = neighbour;
				aystar->neighbours[aystar->num_neighbours].direction = INVALID_TRACKDIR;
				aystar->num_neighbours++;
			}
		} else {
			for (DiagDirection d = DIAGDIR_BEGIN; d < DIAGDIR_END; d++) {
				auto neighbour = tile + TileOffsByDiagDir(d);
				if (IsValidNeighbourOfPreviousTile(neighbour, tile)) {
					aystar->neighbours[aystar->num_neighbours].tile = neighbour;
					aystar->neighbours[aystar->num_neighbours].direction = INVALID_TRACKDIR;
					aystar->num_neighbours++;
				}
			}

			// Check if we can turn this into a tunnel or a bridge.
			if (current->path.parent != nullptr) {
				auto road_direction = DiagdirBetweenTiles(current->path.parent->node.tile, tile);
				if (IsUpwardsSlope(tile, road_direction)) {
					auto tunnel_end = BuildTunnel(&current->path);

					if (tunnel_end != INVALID_TILE) {
						assert(IsValidDiagDirection(DiagdirBetweenTiles(tile, tunnel_end)));
						aystar->neighbours[aystar->num_neighbours].tile = tunnel_end;
						aystar->neighbours[aystar->num_neighbours].direction = INVALID_TRACKDIR;
						aystar->num_neighbours++;
					}
				}
				else if (IsDownwardsSlope(tile, road_direction)) {
					auto bridge_end = BuildBridge(&current->path);

					if (bridge_end != INVALID_TILE) {
						assert(IsValidDiagDirection(DiagdirBetweenTiles(tile, bridge_end)));
						aystar->neighbours[aystar->num_neighbours].tile = bridge_end;
						aystar->neighbours[aystar->num_neighbours].direction = INVALID_TRACKDIR;
						aystar->num_neighbours++;
					}
				}
				else if (GetTileSlope(tile) == SLOPE_FLAT)
				{
					// Check if we could bridge a river from a flat tile. Not looking pretty on the map but you gotta do what you gotta do.
					auto bridge_end = BuildRiverBridge(&current->path, DiagdirBetweenTiles(current->path.parent->node.tile, tile));
					assert(bridge_end == INVALID_TILE || GetTileSlope(bridge_end) == SLOPE_FLAT);

					if (bridge_end != INVALID_TILE) {
						assert(IsValidDiagDirection(DiagdirBetweenTiles(tile, bridge_end)));
						aystar->neighbours[aystar->num_neighbours].tile = bridge_end;
						aystar->neighbours[aystar->num_neighbours].direction = INVALID_TRACKDIR;
						aystar->num_neighbours++;
					}

				}
			}
		}
	}
}

/* AyStar callback for checking whether we reached our destination. */
static int32 PublicRoad_EndNodeCheck(AyStar *aystar, OpenListNode *current)
{
	++checked_nodes;

	// Mark towns visited along the way.
	auto search_result = _town_centers.find(current->path.node.tile);

	if (search_result != _town_centers.end()) {
		_towns_visited_along_the_way.push_back(search_result->second);
	};

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
			// Just follow the path; infrastructure is already in place.
			continue;
		} else if (path->parent == nullptr || AreTilesAdjacent(tile, path->parent->node.tile)) {
			RoadBits road_bits = ROAD_NONE;

			if (child != nullptr) {
				TileIndex tile2 = child->node.tile;
				road_bits |= DiagDirToRoadBits(DiagdirBetweenTiles(tile, tile2));
			}
			if (path->parent != nullptr) {
				TileIndex tile2 = path->parent->node.tile;
				road_bits |= DiagDirToRoadBits(DiagdirBetweenTiles(tile, tile2));
			}

			if (child != nullptr || path->parent != nullptr) {
				// Check if we need to build anything.
				// If it is already a road and has the right bits, we are good. Otherwise build the needed ones.
				if (!IsTileType(tile, MP_ROAD) || ((GetRoadBits(tile, ROADTYPE_ROAD) & road_bits) != road_bits))
				{
					Backup<CompanyByte> cur_company(_current_company, OWNER_DEITY, FILE_LINE);
					auto road_built = CmdBuildRoad(tile, DC_EXEC, ROADTYPE_ROAD << 4 | road_bits, 0).Succeeded();
					cur_company.Restore();

					assert(road_built);
				}
			}
		} else {
			// We only get here if we have a parent and we're not adjacent to it. River/Tunnel time!
			DiagDirection road_direction = DiagdirBetweenTiles(tile, path->parent->node.tile);

			auto end_tile = INVALID_TILE;

			auto start_slope = GetTileSlope(tile);

			if (IsUpwardsSlope(tile, road_direction)) {
				end_tile = BuildTunnel(path, true);
				assert(IsValidTile(end_tile) && IsDownwardsSlope(end_tile, road_direction));
			} else if (IsDownwardsSlope(tile, road_direction)) {
				// Provide the function with the end tile, since we already know it, but still check the result.
				end_tile = BuildBridge(path, path->parent->node.tile, true);
				assert(IsValidTile(end_tile) && IsUpwardsSlope(end_tile, road_direction));
			} else {
				// River bridge is the last possibility.
				assert(GetTileSlope(tile) == SLOPE_FLAT);
				end_tile = BuildRiverBridge(path, road_direction, path->parent->node.tile, true);
				assert(IsValidTile(end_tile) && GetTileSlope(end_tile) == SLOPE_FLAT);
			}
		}

		child = path;
	}
}

bool FindPath(AyStar& finder, TileIndex from, TileIndex to)
{
	finder.CalculateG = PublicRoad_CalculateG;
	finder.CalculateH = PublicRoad_CalculateH;
	finder.GetNeighbours = PublicRoad_GetNeighbours;
	finder.EndNodeCheck = PublicRoad_EndNodeCheck;
	finder.FoundEndNode = PublicRoad_FoundEndNode;
	finder.user_target = &(to);

	finder.Init(PublicRoad_Hash, 1 << PUBLIC_ROAD_HASH_SIZE);

	AyStarNode start;
	start.tile = from;
	start.direction = INVALID_TRACKDIR;
	finder.AddStartNode(&start, 0);

	bool found_path = (finder.Main() == AYSTAR_FOUND_END_NODE);

	if (found_path) {
		_checked_nodes_on_success.push_back(checked_nodes);
	} else {
		_checked_nodes_on_failure.push_back(checked_nodes);
	}

	checked_nodes = 0;

	return found_path;
}

/**
* Build the public road network connecting towns using AyStar.
*/
void GeneratePublicRoads()
{
	using namespace std;

	if (_settings_game.game_creation.build_public_roads == PRC_NONE) return;

	vector<Town*> towns;
	{
		Town* town;
		FOR_ALL_TOWNS(town) {
			towns.push_back(town);
			_town_centers.insert(make_pair(town->xy, town));
		}
	}
	
	SetGeneratingWorldProgress(GWP_PUBLIC_ROADS, towns.size());

	// Create a list of networks which also contain a value indicating how many times we failed to connect to them.
	vector<pair<uint, shared_ptr<vector<Town*>>>> town_networks;

	sort(towns.begin(), towns.end(), [&](auto a, auto b) { return a->cache.population < b->cache.population; });
	Town* main_town = *towns.begin();
	towns.erase(towns.begin());

	shared_ptr<vector<Town*>> main_network(new vector<Town*>);
	main_network->push_back(main_town);

	town_networks.push_back(make_pair(0, main_network));
	IncreaseGeneratingWorldProgress(GWP_PUBLIC_ROADS);

	sort(towns.begin(), towns.end(), [&](auto a, auto b) { return DistanceManhattan(main_town->xy, a->xy) < DistanceManhattan(main_town->xy, b->xy); });

	int times_we_found_path_the_easy_way = 0;

	for_each(towns.begin(), towns.end(), [&](auto begin_town) {
		// Check if we can connect to any of the networks.
		_towns_visited_along_the_way.clear();

		auto reachable_network_iter = _towns_reachable_networks.find(begin_town);

		if (reachable_network_iter != _towns_reachable_networks.end()) {
			AyStar finder;
			auto reachable_network = reachable_network_iter->second;

			sort(reachable_network->begin(), reachable_network->end(), [&](auto a, auto b) { return DistanceManhattan(begin_town->xy, a->xy) < DistanceManhattan(begin_town->xy, b->xy); });

			Town* end_town = *reachable_network->begin();

			bool found_path = FindPath(finder, begin_town->xy, end_town->xy);

			assert(found_path);
			++times_we_found_path_the_easy_way;

			for_each(_towns_visited_along_the_way.begin(), _towns_visited_along_the_way.end(), [&](const Town* visited_town) {
				if (visited_town != begin_town) _towns_reachable_networks.insert(make_pair(visited_town, reachable_network_iter->second));
			});
			finder.Free();
		} else {
			// Sort networks by failed connection attempts, so we try the most likely one first.
			sort(town_networks.begin(), town_networks.end(), [&](auto a, auto b) { return a.first < b.first; });

			if (!any_of(town_networks.begin(), town_networks.end(), [&](auto network_pair) {
				AyStar finder;
				auto network = network_pair.second;

				// Try to connect to the town in the network that is closest to us.
				// If we can't connect to that one, we can't connect to any of them since they are all interconnected.
				sort(network->begin(), network->end(), [&](auto a, auto b) { return DistanceManhattan(begin_town->xy, a->xy) < DistanceManhattan(begin_town->xy, b->xy); });
				Town* end_town = *network->begin();

				bool found_path = FindPath(finder, begin_town->xy, end_town->xy);

				if (found_path) {
					for_each(_towns_visited_along_the_way.begin(), _towns_visited_along_the_way.end(), [&](auto visited_town) {
						if (visited_town != begin_town) _towns_reachable_networks.insert(make_pair(visited_town, network));
					});
				}

				// Increase number of failed attempts if necessary.
				network_pair.first += (found_path ? 0 : 1);
				finder.Free();

				return found_path;

			})) {
				// We failed to connect to any network, so we are a separate network. Let future towns try to connect to us.
				shared_ptr<vector<Town*>> new_network(new vector<Town*>);
				new_network->push_back(begin_town);

				// We basically failed to connect to this many towns.
				int towns_already_in_networks = std::accumulate(town_networks.begin(), town_networks.end(), 0, [&](int accumulator, auto network_pair) {
					return accumulator + network_pair.second->size();
				});

				town_networks.push_back(make_pair(towns_already_in_networks, new_network));

				for_each(_towns_visited_along_the_way.begin(), _towns_visited_along_the_way.end(), [&](const Town* visited_town) {
					if (visited_town != begin_town) _towns_reachable_networks.insert(make_pair(visited_town, new_network));
				});
			}
		}

		IncreaseGeneratingWorldProgress(GWP_PUBLIC_ROADS);
	});

	auto checked_nodes_on_success_min = _checked_nodes_on_success.empty() ? 0 : *min_element(_checked_nodes_on_success.begin(), _checked_nodes_on_success.end());
	auto checked_nodes_on_success_max = _checked_nodes_on_success.empty() ? 0 : *max_element(_checked_nodes_on_success.begin(), _checked_nodes_on_success.end());
	auto checked_nodes_on_success_avg = _checked_nodes_on_success.empty() ? 0 : accumulate(_checked_nodes_on_success.begin(), _checked_nodes_on_success.end(), 0) / _checked_nodes_on_success.size();

	auto checked_nodes_on_failure_min = _checked_nodes_on_failure.empty() ? 0 : *min_element(_checked_nodes_on_failure.begin(), _checked_nodes_on_failure.end());
	auto checked_nodes_on_failure_max = _checked_nodes_on_failure.empty() ? 0 : *max_element(_checked_nodes_on_failure.begin(), _checked_nodes_on_failure.end());
	auto checked_nodes_on_failure_avg = _checked_nodes_on_failure.empty() ? 0 : accumulate(_checked_nodes_on_failure.begin(), _checked_nodes_on_failure.end(), 0) / _checked_nodes_on_failure.size();

	DEBUG(misc, 0, "checked_nodes_on_success_min: " + checked_nodes_on_success_min);
	DEBUG(misc, 0, "checked_nodes_on_success_max: " + checked_nodes_on_success_max);
	DEBUG(misc, 0, "checked_nodes_on_success_avg: " + checked_nodes_on_success_avg);
	DEBUG(misc, 0, "checked_nodes_on_failure_min: " + checked_nodes_on_failure_min);
	DEBUG(misc, 0, "checked_nodes_on_failure_max: " + checked_nodes_on_failure_max);
	DEBUG(misc, 0, "checked_nodes_on_failure_avg: " + checked_nodes_on_failure_avg);
	DEBUG(misc, 0, "times_we_found_path_the_easy_way: " + times_we_found_path_the_easy_way);

	
}

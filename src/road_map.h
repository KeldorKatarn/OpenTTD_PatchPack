/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file road_map.h Map accessors for roads. */

#ifndef ROAD_MAP_H
#define ROAD_MAP_H

#include "track_func.h"
#include "depot_type.h"
#include "rail_type.h"
#include "road_func.h"
#include "tile_map.h"


/** The different types of road tiles. */
enum RoadTileType {
	ROAD_TILE_NORMAL,   ///< Normal road
	ROAD_TILE_CROSSING, ///< Level crossing
	ROAD_TILE_DEPOT,    ///< Depot (one entrance)
};

/**
 * Get the type of the road tile.
 * @param t Tile to query.
 * @pre IsTileType(t, MP_ROAD)
 * @return The road tile type.
 */
static inline RoadTileType GetRoadTileType(TileIndex t)
{
	assert(IsTileType(t, MP_ROAD));
	return (RoadTileType)GB(_m[t].m5, 6, 2);
}

/**
 * Return whether a tile is a normal road.
 * @param t Tile to query.
 * @pre IsTileType(t, MP_ROAD)
 * @return True if normal road.
 */
static inline bool IsNormalRoad(TileIndex t)
{
	return GetRoadTileType(t) == ROAD_TILE_NORMAL;
}

/**
 * Return whether a tile is a normal road tile.
 * @param t Tile to query.
 * @return True if normal road tile.
 */
static inline bool IsNormalRoadTile(TileIndex t)
{
	return IsTileType(t, MP_ROAD) && IsNormalRoad(t);
}

/**
 * Return whether a tile is a level crossing.
 * @param t Tile to query.
 * @pre IsTileType(t, MP_ROAD)
 * @return True if level crossing.
 */
static inline bool IsLevelCrossing(TileIndex t)
{
	return GetRoadTileType(t) == ROAD_TILE_CROSSING;
}

/**
 * Return whether a tile is a level crossing tile.
 * @param t Tile to query.
 * @return True if level crossing tile.
 */
static inline bool IsLevelCrossingTile(TileIndex t)
{
	return IsTileType(t, MP_ROAD) && IsLevelCrossing(t);
}

/**
 * Return whether a tile is a road depot.
 * @param t Tile to query.
 * @pre IsTileType(t, MP_ROAD)
 * @return True if road depot.
 */
static inline bool IsRoadDepot(TileIndex t)
{
	return GetRoadTileType(t) == ROAD_TILE_DEPOT;
}

/**
 * Return whether a tile is a road depot tile.
 * @param t Tile to query.
 * @return True if road depot tile.
 */
static inline bool IsRoadDepotTile(TileIndex t)
{
	return IsTileType(t, MP_ROAD) && IsRoadDepot(t);
}

/**
 * Get the present road bits for a specific road type.
 * @param t  The tile to query.
 * @param rt Road type.
 * @pre IsNormalRoad(t)
 * @return The present road bits for the road type.
 */
static inline RoadBits GetRoadBits(TileIndex t, RoadType rt)
{
	assert(IsNormalRoad(t));
	switch (rt) {
		default: NOT_REACHED();
		case ROADTYPE_ROAD: return (RoadBits)GB(_m[t].m5, 0, 4);
		case ROADTYPE_TRAM: return (RoadBits)GB(_m[t].m3, 0, 4);
	}
}

/**
 * Get all RoadBits set on a tile except from the given RoadType
 *
 * @param t The tile from which we want to get the RoadBits
 * @param rt The RoadType which we exclude from the querry
 * @return all set RoadBits of the tile which are not from the given RoadType
 */
static inline RoadBits GetOtherRoadBits(TileIndex t, RoadType rt)
{
	return GetRoadBits(t, rt == ROADTYPE_ROAD ? ROADTYPE_TRAM : ROADTYPE_ROAD);
}

/**
 * Get all set RoadBits on the given tile
 *
 * @param tile The tile from which we want to get the RoadBits
 * @return all set RoadBits of the tile
 */
static inline RoadBits GetAllRoadBits(TileIndex tile)
{
	return GetRoadBits(tile, ROADTYPE_ROAD) | GetRoadBits(tile, ROADTYPE_TRAM);
}

/**
 * Set the present road bits for a specific road type.
 * @param t  The tile to change.
 * @param r  The new road bits.
 * @param rt Road type.
 * @pre IsNormalRoad(t)
 */
static inline void SetRoadBits(TileIndex t, RoadBits r, RoadType rt)
{
	assert(IsNormalRoad(t)); // XXX incomplete
	switch (rt) {
		default: NOT_REACHED();
		case ROADTYPE_ROAD: SB(_m[t].m5, 0, 4, r); break;
		case ROADTYPE_TRAM: SB(_m[t].m3, 0, 4, r); break;
	}
}

/**
 * Get the present road types of a tile.
 * @param t The tile to query.
 * @return Present road types.
 */
static inline RoadTypes GetRoadTypes(TileIndex t)
{
	return (RoadTypes)GB(_me[t].m7, 6, 2);
}

/**
 * Set the present road types of a tile.
 * @param t  The tile to change.
 * @param rt The new road types.
 */
static inline void SetRoadTypes(TileIndex t, RoadTypes rt)
{
	assert(IsTileType(t, MP_ROAD) || IsTileType(t, MP_STATION) || IsTileType(t, MP_TUNNELBRIDGE));
	SB(_me[t].m7, 6, 2, rt);
}

/**
 * Check if a tile has a specific road type.
 * @param t  The tile to check.
 * @param rt Road type to check.
 * @return True if the tile has the specified road type.
 */
static inline bool HasTileRoadType(TileIndex t, RoadType rt)
{
	return HasBit(GetRoadTypes(t), rt);
}

/**
 * Get the owner of a specific road type.
 * @param t  The tile to query.
 * @param rt The road type to get the owner of.
 * @return Owner of the given road type.
 */
static inline Owner GetRoadOwner(TileIndex t, RoadType rt)
{
	assert(IsTileType(t, MP_ROAD) || IsTileType(t, MP_STATION) || IsTileType(t, MP_TUNNELBRIDGE));
	switch (rt) {
		default: NOT_REACHED();
		case ROADTYPE_ROAD: return (Owner)GB(IsNormalRoadTile(t) ? _m[t].m1 : _me[t].m7, 0, 5);
		case ROADTYPE_TRAM: {
			/* Trams don't need OWNER_TOWN, and remapping OWNER_NONE
			 * to OWNER_TOWN makes it use one bit less */
			Owner o = (Owner)GB(_m[t].m3, 4, 4);
			return o == OWNER_TOWN ? OWNER_NONE : o;
		}
	}
}

/**
 * Set the owner of a specific road type.
 * @param t  The tile to change.
 * @param rt The road type to change the owner of.
 * @param o  New owner of the given road type.
 */
static inline void SetRoadOwner(TileIndex t, RoadType rt, Owner o)
{
	switch (rt) {
		default: NOT_REACHED();
		case ROADTYPE_ROAD: SB(IsNormalRoadTile(t) ? _m[t].m1 : _me[t].m7, 0, 5, o); break;
		case ROADTYPE_TRAM: SB(_m[t].m3, 4, 4, o == OWNER_NONE ? OWNER_TOWN : o); break;
	}
}

/**
 * Check if a specific road type is owned by an owner.
 * @param t  The tile to query.
 * @param rt The road type to compare the owner of.
 * @param o  Owner to compare with.
 * @pre HasTileRoadType(t, rt)
 * @return True if the road type is owned by the given owner.
 */
static inline bool IsRoadOwner(TileIndex t, RoadType rt, Owner o)
{
	assert(HasTileRoadType(t, rt));
	return (GetRoadOwner(t, rt) == o);
}

/**
 * Checks if given tile has town owned road
 * @param t tile to check
 * @pre IsTileType(t, MP_ROAD)
 * @return true iff tile has road and the road is owned by a town
 */
static inline bool HasTownOwnedRoad(TileIndex t)
{
	return HasTileRoadType(t, ROADTYPE_ROAD) && IsRoadOwner(t, ROADTYPE_ROAD, OWNER_TOWN);
}

/** Which directions are disallowed ? */
enum DisallowedRoadDirections {
	DRD_NONE,       ///< None of the directions are disallowed
	DRD_SOUTHBOUND, ///< All southbound traffic is disallowed
	DRD_NORTHBOUND, ///< All northbound traffic is disallowed
	DRD_BOTH,       ///< All directions are disallowed
	DRD_END,        ///< Sentinel
};
DECLARE_ENUM_AS_BIT_SET(DisallowedRoadDirections)
/** Helper information for extract tool. */
template <> struct EnumPropsT<DisallowedRoadDirections> : MakeEnumPropsT<DisallowedRoadDirections, byte, DRD_NONE, DRD_END, DRD_END, 2> {};

/**
 * Gets the disallowed directions
 * @param t the tile to get the directions from
 * @return the disallowed directions
 */
static inline DisallowedRoadDirections GetDisallowedRoadDirections(TileIndex t)
{
	assert(IsNormalRoad(t));
	return (DisallowedRoadDirections)GB(_m[t].m5, 4, 2);
}

/**
 * Sets the disallowed directions
 * @param t   the tile to set the directions for
 * @param drd the disallowed directions
 */
static inline void SetDisallowedRoadDirections(TileIndex t, DisallowedRoadDirections drd)
{
	assert(IsNormalRoad(t));
	assert(drd < DRD_END);
	SB(_m[t].m5, 4, 2, drd);
}

/**
 * Sets the catenary bit on a tile
 * @param t the tile to set the catenary bit to
 * @param b the value of the catenary bit
 * @pre IsNormalRoad(t)
 */
static inline void SetCatenary(TileIndex t, bool b)
{
	assert(IsNormalRoad(t));
	SB(_m[t].m1, 7, 1, b ? 1 : 0);
}

/**
* Checks if given tile has catenary bit.
* @param t tile to check
* @return True if tile has catenary bit set
*/
static inline bool HasCatenary(TileIndex t)
{
	return GB(_m[t].m1, 7, 1) != 0;
}

/**
 * Get the road axis of a level crossing.
 * @param t The tile to query.
 * @pre IsLevelCrossing(t)
 * @return The axis of the road.
 */
static inline Axis GetCrossingRoadAxis(TileIndex t)
{
	assert(IsLevelCrossing(t));
	return (Axis)GB(_m[t].m5, 0, 1);
}

/**
 * Get the rail axis of a level crossing.
 * @param t The tile to query.
 * @pre IsLevelCrossing(t)
 * @return The axis of the rail.
 */
static inline Axis GetCrossingRailAxis(TileIndex t)
{
	assert(IsLevelCrossing(t));
	return OtherAxis((Axis)GetCrossingRoadAxis(t));
}

/**
 * Get the road bits of a level crossing.
 * @param tile The tile to query.
 * @return The present road bits.
 */
static inline RoadBits GetCrossingRoadBits(TileIndex tile)
{
	return GetCrossingRoadAxis(tile) == AXIS_X ? ROAD_X : ROAD_Y;
}

/**
 * Get the rail track of a level crossing.
 * @param tile The tile to query.
 * @return The rail track.
 */
static inline Track GetCrossingRailTrack(TileIndex tile)
{
	return AxisToTrack(GetCrossingRailAxis(tile));
}

/**
 * Get the rail track bits of a level crossing.
 * @param tile The tile to query.
 * @return The rail track bits.
 */
static inline TrackBits GetCrossingRailBits(TileIndex tile)
{
	return AxisToTrackBits(GetCrossingRailAxis(tile));
}


/**
 * Get the reservation state of the rail crossing
 * @param t the crossing tile
 * @return reservation state
 * @pre IsLevelCrossingTile(t)
 */
static inline bool HasCrossingReservation(TileIndex t)
{
	assert(IsLevelCrossingTile(t));
	return HasBit(_m[t].m5, 4);
}

/**
 * Set the reservation state of the rail crossing
 * @note Works for both waypoints and rail depots
 * @param t the crossing tile
 * @param b the reservation state
 * @pre IsLevelCrossingTile(t)
 */
static inline void SetCrossingReservation(TileIndex t, bool b)
{
	assert(IsLevelCrossingTile(t));
	SB(_m[t].m5, 4, 1, b ? 1 : 0);
}

/**
 * Get the reserved track bits for a rail crossing
 * @param t the tile
 * @pre IsLevelCrossingTile(t)
 * @return reserved track bits
 */
static inline TrackBits GetCrossingReservationTrackBits(TileIndex t)
{
	return HasCrossingReservation(t) ? GetCrossingRailBits(t) : TRACK_BIT_NONE;
}

/**
 * Check if the level crossing is barred.
 * @param t The tile to query.
 * @pre IsLevelCrossing(t)
 * @return True if the level crossing is barred.
 */
static inline bool IsCrossingBarred(TileIndex t)
{
	assert(IsLevelCrossing(t));
	return HasBit(_m[t].m5, 5);
}

/**
 * Set the bar state of a level crossing.
 * @param t The tile to modify.
 * @param barred True if the crossing should be barred, false otherwise.
 * @pre IsLevelCrossing(t)
 */
static inline void SetCrossingBarred(TileIndex t, bool barred)
{
	assert(IsLevelCrossing(t));
	SB(_m[t].m5, 5, 1, barred ? 1 : 0);
}

/**
 * Unbar a level crossing.
 * @param t The tile to change.
 */
static inline void UnbarCrossing(TileIndex t)
{
	SetCrossingBarred(t, false);
}

/**
 * Bar a level crossing.
 * @param t The tile to change.
 */
static inline void BarCrossing(TileIndex t)
{
	SetCrossingBarred(t, true);
}

/** Check if a road tile has snow/desert. */
#define IsOnDesert IsOnSnow
/**
 * Check if a road tile has snow/desert.
 * @param t The tile to query.
 * @return True if the tile has snow/desert.
 */
static inline bool IsOnSnow(TileIndex t)
{
	return HasBit(_me[t].m7, 5);
}

/** Toggle the snow/desert state of a road tile. */
#define ToggleDesert ToggleSnow
/**
 * Toggle the snow/desert state of a road tile.
 * @param t The tile to change.
 */
static inline void ToggleSnow(TileIndex t)
{
	ToggleBit(_me[t].m7, 5);
}


/** The possible road side decorations. */
enum Roadside {
	ROADSIDE_BARREN           = 0, ///< Road on barren land
	ROADSIDE_GRASS            = 1, ///< Road on grass
	ROADSIDE_PAVED            = 2, ///< Road with paved sidewalks
	ROADSIDE_STREET_LIGHTS    = 3, ///< Road with street lights on paved sidewalks
	ROADSIDE_TREES            = 5, ///< Road with trees on paved sidewalks
	ROADSIDE_GRASS_ROAD_WORKS = 6, ///< Road on grass with road works
	ROADSIDE_PAVED_ROAD_WORKS = 7, ///< Road with sidewalks and road works
};

/**
 * Get the decorations of a road.
 * @param tile The tile to query.
 * @return The road decoration of the tile.
 */
static inline Roadside GetRoadside(TileIndex tile)
{
	return (Roadside)GB(_me[tile].m6, 3, 3);
}

/**
 * Set the decorations of a road.
 * @param tile The tile to change.
 * @param s    The new road decoration of the tile.
 */
static inline void SetRoadside(TileIndex tile, Roadside s)
{
	SB(_me[tile].m6, 3, 3, s);
}

/**
 * Check if a tile has road works.
 * @param t The tile to check.
 * @return True if the tile has road works in progress.
 */
static inline bool HasRoadWorks(TileIndex t)
{
	return GetRoadside(t) >= ROADSIDE_GRASS_ROAD_WORKS;
}

/**
 * Increase the progress counter of road works.
 * @param t The tile to modify.
 * @return True if the road works are in the last stage.
 */
static inline bool IncreaseRoadWorksCounter(TileIndex t)
{
	AB(_me[t].m7, 0, 4, 1);

	return GB(_me[t].m7, 0, 4) == 15;
}

/**
 * Start road works on a tile.
 * @param t The tile to start the work on.
 * @pre !HasRoadWorks(t)
 */
static inline void StartRoadWorks(TileIndex t)
{
	assert(!HasRoadWorks(t));
	/* Remove any trees or lamps in case or roadwork */
	switch (GetRoadside(t)) {
		case ROADSIDE_BARREN:
		case ROADSIDE_GRASS:  SetRoadside(t, ROADSIDE_GRASS_ROAD_WORKS); break;
		default:              SetRoadside(t, ROADSIDE_PAVED_ROAD_WORKS); break;
	}
}

/**
 * Terminate road works on a tile.
 * @param t Tile to stop the road works on.
 * @pre HasRoadWorks(t)
 */
static inline void TerminateRoadWorks(TileIndex t)
{
	assert(HasRoadWorks(t));
	SetRoadside(t, (Roadside)(GetRoadside(t) - ROADSIDE_GRASS_ROAD_WORKS + ROADSIDE_GRASS));
	/* Stop the counter */
	SB(_me[t].m7, 0, 4, 0);
}


/**
 * Get the direction of the exit of a road depot.
 * @param t The tile to query.
 * @return Diagonal direction of the depot exit.
 */
static inline DiagDirection GetRoadDepotDirection(TileIndex t)
{
	assert(IsRoadDepot(t));
	return (DiagDirection)GB(_m[t].m5, 0, 2);
}


RoadBits GetAnyRoadBits(TileIndex tile, RoadType rt, bool straight_tunnel_bridge_entrance = false);


/**
 * Make a normal road tile.
 * @param t    Tile to make a normal road.
 * @param bits Road bits to set for all present road types.
 * @param rot  New present road types.
 * @param town Town ID if the road is a town-owned road.
 * @param road New owner of road.
 * @param tram New owner of tram tracks.
 * @param catenary_flag Then new value for the catenary flag.
 */
static inline void MakeRoadNormal(TileIndex t, RoadBits bits, RoadTypes rot, TownID town, Owner road, Owner tram, bool catenary_flag = false)
{
	SetTileType(t, MP_ROAD);
	SetTileOwner(t, road);
	_m[t].m2 = town;
	_m[t].m3 = (HasBit(rot, ROADTYPE_TRAM) ? bits : 0);
	_m[t].m4 = 0;
	_m[t].m5 = (HasBit(rot, ROADTYPE_ROAD) ? bits : 0) | ROAD_TILE_NORMAL << 6;
	SB(_me[t].m6, 2, 4, 0);
	_me[t].m7 = rot << 6;
	SetRoadOwner(t, ROADTYPE_TRAM, tram);
	SetCatenary(t, catenary_flag);
}

/**
 * Make a level crossing.
 * @param t       Tile to make a level crossing.
 * @param road    New owner of road.
 * @param tram    New owner of tram tracks.
 * @param rail    New owner of the rail track.
 * @param roaddir Axis of the road.
 * @param rat     New rail type.
 * @param rot     New present road types.
 * @param town    Town ID if the road is a town-owned road.
 */
static inline void MakeRoadCrossing(TileIndex t, Owner road, Owner tram, Owner rail, Axis roaddir, RailType rat, RoadTypes rot, uint town)
{
	SetTileType(t, MP_ROAD);
	SetTileOwner(t, rail);
	_m[t].m2 = town;
	_m[t].m3 = rat;
	_m[t].m4 = 0;
	_m[t].m5 = ROAD_TILE_CROSSING << 6 | roaddir;
	SB(_me[t].m6, 2, 4, 0);
	_me[t].m7 = rot << 6 | road;
	SetRoadOwner(t, ROADTYPE_TRAM, tram);
}

/**
 * Make a road depot.
 * @param t     Tile to make a level crossing.
 * @param owner New owner of the depot.
 * @param did   New depot ID.
 * @param dir   Direction of the depot exit.
 * @param rt    Road type of the depot.
 */
static inline void MakeRoadDepot(TileIndex t, Owner owner, DepotID did, DiagDirection dir, RoadType rt)
{
	SetTileType(t, MP_ROAD);
	SetTileOwner(t, owner);
	_m[t].m2 = did;
	_m[t].m3 = 0;
	_m[t].m4 = 0;
	_m[t].m5 = ROAD_TILE_DEPOT << 6 | dir;
	SB(_me[t].m6, 2, 4, 0);
	_me[t].m7 = RoadTypeToRoadTypes(rt) << 6 | owner;
	SetRoadOwner(t, ROADTYPE_TRAM, owner);
}

static inline RoadTypeIdentifier GetRoadTypeRoad(TileIndex t)
{
	return RoadTypeIdentifier(ROADTYPE_ROAD, (RoadSubType)GB(_m[t].m4, 0, 4));
}

static inline RoadTypeIdentifier GetRoadTypeTram(TileIndex t)
{
	return RoadTypeIdentifier(ROADTYPE_TRAM, (RoadSubType)GB(_m[t].m4, 4, 4));
}

struct RoadTypeIdentifiers {
	RoadTypeIdentifier road_identifier = RoadTypeIdentifier(INVALID_ROADTYPE, INVALID_ROADSUBTYPE);
	RoadTypeIdentifier tram_identifier = RoadTypeIdentifier(INVALID_ROADTYPE, INVALID_ROADSUBTYPE);

	/* Creates an INVALID RoadTypeIdentifiers */
	RoadTypeIdentifiers() {}

	/* Extracts the RoadTypeIdentifiers from the given tile
	 * @param tile The tile to read the informations from
	 */
	RoadTypeIdentifiers(TileIndex t)
	{
		assert(IsTileType(t, MP_ROAD) || IsTileType(t, MP_STATION) || IsTileType(t, MP_TUNNELBRIDGE));
		TileType tt = GetTileType(t);

		switch (tt) {
			default: NOT_REACHED();
			case MP_ROAD:
				if (GetRoadBits(t, ROADTYPE_ROAD) != ROAD_NONE) {
					road_identifier = GetRoadTypeRoad(t);
				}

				if (GetRoadBits(t, ROADTYPE_TRAM) != ROAD_NONE) {
					tram_identifier = GetRoadTypeTram(t);
				}
				break;
			case MP_STATION: /* TODO */
			case MP_TUNNELBRIDGE:
				road_identifier = GetRoadTypeRoad(t);
				tram_identifier = GetRoadTypeTram(t);
				break;
		}
	}

	/* Converts a RoadTypeIdentifier into a RoadTypeIdentifiers
	 * @param rtid The RoadTypeIdentifier to convert
	 */
	RoadTypeIdentifiers(RoadTypeIdentifier rtid)
	{
		switch (rtid.basetype) {
			default: NOT_REACHED();
			case ROADTYPE_ROAD: road_identifier = rtid; break;
			case ROADTYPE_TRAM: tram_identifier = rtid; break;
		}
	}

	/* Creates a RoadTypeIdentifiers from two RoadTypeIdentifier
	 * The order of the identifier doesn't matter, but they must be of different
	 * base type (no ROAD + ROAD or TRAM + TRAM)
	 * @param rtid1 The first RoadTypeIdentifier
	 * @param rtid2 The second RoadTypeIdentifier
	 */
	RoadTypeIdentifiers(RoadTypeIdentifier rtid1, RoadTypeIdentifier rtid2)
	{
		assert(rtid1.basetype < ROADTYPE_END);
		assert(rtid2.basetype < ROADTYPE_END);

		/* We can't merge 2 road types of the same base type, not yet */
		assert(rtid1.basetype != rtid2.basetype);

		/* As we are sure about the road types being different we can just check the first one */
		switch (rtid1.basetype) {
			default: NOT_REACHED();
			case ROADTYPE_ROAD:
				road_identifier = rtid1;
				tram_identifier = rtid2;
				break;
			case ROADTYPE_TRAM:
				road_identifier = rtid2;
				tram_identifier = rtid1;
				break;
		}
	}

	/* Create a new RoadTypeIdentifiers merging a new identifier with a previous one
	 * @param rtids a source RoadTypeIdentifiers
	 * @param rtid the new RoadTypeIdentifier
	 */
	RoadTypeIdentifiers(RoadTypeIdentifiers rtids, RoadTypeIdentifier rtid)
	{
		switch (rtid.basetype) {
			default: NOT_REACHED();
			case ROADTYPE_ROAD:
				road_identifier = rtid;
				tram_identifier = rtids.tram_identifier;
				break;
			case ROADTYPE_TRAM:
				road_identifier = rtids.road_identifier;
				tram_identifier = rtid;
				break;
		}
	}

	/* Returns the RoadTypes contained in the RoadTypeIdentifiers
	 * @return RoadTypes flags
	 */
	RoadTypes PresentRoadTypes()
	{
		RoadTypes rot = ROADTYPES_NONE;

		if (road_identifier.IsValid()) {
			rot |= ROADTYPES_ROAD;
		}

		if (tram_identifier.IsValid()) {
			rot |= ROADTYPES_TRAM;
		}

		return rot;
	};

	/* Merge a new road type identifier into the current one
	 * @param rtid The new RoadTypeIdentifier to merge into the current one
	 * @return true on success (current behaviour = always success)
	 */
	bool MergeRoadTypes(RoadTypeIdentifier rtid)
	{
		switch (rtid.basetype) {
			default: NOT_REACHED();
			case ROADTYPE_ROAD: road_identifier = rtid; break;
			case ROADTYPE_TRAM: tram_identifier = rtid; break;
		}

		return true;
	};

	bool HasCatenary()
	{
		if (road_identifier.IsValid() && GetRoadTypeInfo(road_identifier)->flags & ROTFB_CATENARY) {
			return true;
		}

		if (tram_identifier.IsValid() && GetRoadTypeInfo(tram_identifier)->flags & ROTFB_CATENARY) {
			return true;
		}

		return false;
	}
};

/**
 * Combine road types from tile with the new one
 * @param tile The tile to get the present road types from
 * @param rtid The road type identifier to add
 * @return The combined road types
 */
static inline RoadTypeIdentifiers CombineTileRoadTypeIds(TileIndex tile, RoadTypeIdentifier rtid)
{
	/* Extract road types from tile, like "GetRoadTypes(tile)" */
	RoadTypeIdentifiers tile_roadtype_ids = RoadTypeIdentifiers(tile);

	/* Add the new road type preserving the other one (eg. add tram to road), like "roadtypes | RoadTypeToRoadTypes(rt)" */
	return RoadTypeIdentifiers(tile_roadtype_ids, rtid);
}

/**
 * Set the present road types of a tile.
 * @param t  The tile to change.
 * @param rtids The new road types identifiers to set for the tile.
 */
static inline void SetRoadTypes(TileIndex t, RoadTypeIdentifiers rtids)
{
	SetRoadTypes(t, rtids.PresentRoadTypes());

	if (rtids.road_identifier.IsValid()) {
		SB(_m[t].m4, 0, 4, rtids.road_identifier.subtype);
	}

	if (rtids.tram_identifier.IsValid()) {
		SB(_m[t].m4, 4, 4, rtids.tram_identifier.subtype);
	}
}

static inline bool HasRoadTypeRoad(TileIndex t)
{
	return RoadTypeIdentifiers(t).road_identifier.IsValid();
}

static inline bool HasRoadTypeRoad(uint8 rtid)
{
	RoadTypeIdentifier _rtid = RoadTypeIdentifier(rtid);

	return _rtid.IsValid() && _rtid.basetype == ROADTYPE_ROAD;
}

static inline bool HasRoadTypeRoad(RoadTypeIdentifiers rtids)
{
	return rtids.road_identifier.IsValid();
}

static inline bool HasRoadTypeTram(TileIndex t)
{
	return RoadTypeIdentifiers(t).tram_identifier.IsValid();
}

static inline bool HasRoadTypeTram(uint8 rtid)
{
	RoadTypeIdentifier _rtid = RoadTypeIdentifier(rtid);

	return _rtid.IsValid() && _rtid.basetype == ROADTYPE_TRAM;
}

static inline bool HasRoadTypeTram(RoadTypeIdentifiers rtids)
{
	return rtids.tram_identifier.IsValid();
}

/**
 * Make a normal road tile.
 * @param t    Tile to make a normal road.
 * @param bits Road bits to set for all present road types.
 * @param rot  New present road types.
 * @param town Town ID if the road is a town-owned road.
 * @param road New owner of road.
 * @param tram New owner of tram tracks.
 * @param catenary_flag Then new value for the catenary flag.
 */
static inline void MakeRoadNormal(TileIndex t, RoadBits bits, RoadTypeIdentifier rtid, TownID town, Owner road, Owner tram, bool catenary_flag = false)
{
	SetTileType(t, MP_ROAD);
	SetTileOwner(t, road);
	_m[t].m2 = town;
	_m[t].m3 = (rtid.basetype == ROADTYPE_TRAM ? bits : 0);
	SetRoadTypes(t, RoadTypeIdentifiers(rtid));
	_m[t].m5 = (rtid.basetype == ROADTYPE_ROAD ? bits : 0) | ROAD_TILE_NORMAL << 6;
	SB(_me[t].m6, 2, 4, 0);
	_me[t].m7 = RoadTypeToRoadTypes(rtid.basetype) << 6;
	SetRoadOwner(t, ROADTYPE_TRAM, tram);
	SetCatenary(t, catenary_flag);
}

#endif /* ROAD_MAP_H */

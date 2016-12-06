/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file road.h Road specific functions. */

#ifndef ROAD_H
#define ROAD_H

#include "road_type.h"
#include "gfx_type.h"
#include "core/bitmath_func.hpp"
#include "strings_type.h"
#include "date_type.h"
#include "core/enum_type.hpp"
#include "core/smallvec_type.hpp"
#include "newgrf.h"
#include "economy_func.h"

/** Roadtype flags. Starts with RO instead of R because R is used for rails */
enum RoadTypeFlags {
	ROTF_CATENARY = 0,                   ///< Bit number for adding catenary

	ROTFB_NONE = 0,                      ///< All flags cleared.
	ROTFB_CATENARY = 1 << ROTF_CATENARY, ///< Value for drawing a catenary.
};
DECLARE_ENUM_AS_BIT_SET(RoadTypeFlags)

struct SpriteGroup;

/** Sprite groups for a roadtype. */
enum RoadTypeSpriteGroup {
	ROTSG_CURSORS,        ///< Cursor and toolbar icon images
	ROTSG_OVERLAY,        ///< Images for overlaying track
	ROTSG_GROUND,         ///< Main group of ground images
	ROTSG_reserved1,      ///< Placeholder, if we need specific tunnel sprites.
	ROTSG_CATENARY_FRONT, ///< Catenary front
	ROTSG_CATENARY_BACK,  ///< Catenary back
	ROTSG_BRIDGE,         ///< Bridge surface images
	ROTSG_reserved2,      ///< Placeholder, if we need specific level crossing sprites.
	ROTSG_DEPOT,          ///< Depot images
	ROTSG_reserved3,      ///< Placeholder, if we add road fences (for highways).
	ROTSG_ROADSTOP,       ///< Drive-in stop surface
	ROTSG_END,
};

/** List of road type labels. */
typedef SmallVector<RoadTypeLabel, 4> RoadTypeLabelList;

struct RoadtypeInfo {
	/**
	 * struct containing the sprites for the road GUI. @note only sprites referred to
	 * directly in the code are listed
	 */
	struct {
		SpriteID build_x_road;        ///< button for building single rail in X direction
		SpriteID build_y_road;        ///< button for building single rail in Y direction
		SpriteID auto_road;           ///< button for the autoroad construction
		SpriteID build_depot;         ///< button for building depots
		SpriteID build_bus_station;   ///< button for building bus stations
		SpriteID build_truck_station; ///< button for building truck stations
		SpriteID build_tunnel;        ///< button for building a tunnel
		SpriteID convert_road;        ///< button for converting road types
	} gui_sprites;

	struct {
		CursorID road_swne;     ///< Cursor for building rail in X direction
		CursorID road_nwse;     ///< Cursor for building rail in Y direction
		CursorID autoroad;      ///< Cursor for autorail tool
		CursorID depot;         ///< Cursor for building a depot
		CursorID bus_station;   ///< Cursor for building a bus station
		CursorID truck_station; ///< Cursor for building a truck station
		CursorID tunnel;        ///< Cursor for building a tunnel
		SpriteID convert_road;  ///< Cursor for converting road types
	} cursor;                       ///< Cursors associated with the road type.

	struct {
		StringID name;            ///< Name of this rail type. // TODO use
		StringID toolbar_caption; ///< Caption in the construction toolbar GUI for this rail type.
		StringID menu_text;       ///< Name of this rail type in the main toolbar dropdown.
		StringID build_caption;   ///< Caption of the build vehicle GUI for this rail type. // TODO use
		StringID replace_text;    ///< Text used in the autoreplace GUI. // TODO use
		StringID new_engine;      ///< Name of an engine for this type of road in the engine preview GUI. // TODO use

		StringID err_build_road;        ///< Building a normal piece of road
		StringID err_remove_road;       ///< Removing a normal piece of road
		StringID err_depot;             ///< Building a depot
		StringID err_build_station[2];  ///< Building a bus or truck station
		StringID err_remove_station[2]; ///< Removing of a bus or truck station

		StringID picker_title[2];       ///< Title for the station picker for bus or truck stations
		StringID picker_tooltip[2];     ///< Tooltip for the station picker for bus or truck stations
	} strings;                        ///< Strings associated with the rail type.

	/** bitmask to the OTHER roadtypes on which a vehicle of THIS roadtype generates power */
	RoadSubTypes powered_roadtypes; // TODO use

	/**
	 * Multiplier for curve maximum speed advantage
	 */
	byte curve_speed; // TODO use

	/**
	 * Bit mask of road type flags
	 */
	RoadTypeFlags flags; // TODO more flags

	/**
	 * Cost multiplier for building this road type
	 */
	uint16 cost_multiplier; // TODO use

	/**
	 * Cost multiplier for maintenance of this road type
	 */
	uint16 maintenance_multiplier;

	/**
	 * Maximum speed for vehicles travelling on this road type
	 */
	uint16 max_speed; // TODO use

	/**
	 * Unique 32 bit road type identifier
	 */
	RoadTypeLabel label;

	/**
	 * Road type labels this type provides in addition to the main label.
	 */
	RoadTypeLabelList alternate_labels; // TODO use

	/**
	 * Colour on mini-map
	 */
	byte map_colour; // TODO use

	/**
	 * Introduction date.
	 * When #INVALID_DATE or a vehicle using this roadtype gets introduced earlier,
	 * the vehicle's introduction date will be used instead for this roadtype.
	 * The introduction at this date is furthermore limited by the
	 * #introduction_required_types.
	 */
	Date introduction_date; // TODO use

	/**
	 * Bitmask of roadtypes that are required for this roadtype to be introduced
	 * at a given #introduction_date.
	 */
	RoadSubTypes introduction_required_roadtypes; // TODO use

	/**
	 * Bitmask of which other roadtypes are introduced when this roadtype is introduced.
	 */
	RoadSubTypes introduces_roadtypes; // TODO use

	/**
	 * The sorting order of this roadtype for the toolbar dropdown.
	 */
	byte sorting_order;

	/**
	 * NewGRF providing the Action3 for the roadtype. NULL if not available.
	 */
	const GRFFile *grffile[ROTSG_END];

	/**
	 * Sprite groups for resolving sprites
	 */
	const SpriteGroup *group[ROTSG_END];

	inline bool UsesOverlay() const
	{
		return this->group[ROTSG_GROUND] != NULL;
	}
};

struct RoadTypeIdentifier {
	RoadType basetype;
	RoadSubType subtype;

	uint8 Pack() const;
	bool UnpackIfValid(uint32 data);
	static RoadTypeIdentifier Unpack(uint32 data);

	bool IsValid() const
	{
		return (this->basetype == ROADTYPE_ROAD || this->basetype == ROADTYPE_TRAM) && IsInsideMM(this->subtype, ROADSUBTYPE_BEGIN, ROADSUBTYPE_END);
	}

	bool IsRoad() const
	{
		return (this->basetype == ROADTYPE_ROAD) && IsInsideMM(this->subtype, ROADSUBTYPE_BEGIN, ROADSUBTYPE_END);
	}

	bool IsTram() const
	{
		return (this->basetype == ROADTYPE_TRAM) && IsInsideMM(this->subtype, ROADSUBTYPE_BEGIN, ROADSUBTYPE_END);
	}

	RoadTypeIdentifier(RoadType basetype, RoadSubType subtype) : basetype(basetype), subtype(subtype) {}
	RoadTypeIdentifier() : basetype(INVALID_ROADTYPE), subtype(INVALID_ROADSUBTYPE) {}
};

/**
 * Returns a pointer to the Roadtype information for a given roadtype
 * @param roadtype the road type which the information is requested for
 * @return The pointer to the RoadtypeInfo
 */
static inline const RoadtypeInfo *GetRoadTypeInfo(RoadTypeIdentifier rtid)
{
	extern RoadtypeInfo _roadtypes[ROADTYPE_END][ROADSUBTYPE_END];
	assert(rtid.IsValid());
	return &_roadtypes[rtid.basetype][rtid.subtype];
}

/**
 * Checks if an engine of the given RoadType got power on a tile with a given
 * RoadType. This would normally just be an equality check, but for electrified
 * roads (which also support non-electric vehicles).
 * @return Whether the engine got power on this tile.
 * @param  vehicletype The RoadType of the engine we are considering.
 * @param  tiletype   The RoadType of the tile we are considering.
 */
static inline bool HasPowerOnRoad(RoadTypeIdentifier rtid) // TODO
{
	uint8 a = GetRoadTypeInfo(rtid)->powered_roadtypes;
	uint8 b = rtid.basetype;

	return HasBit(a, b);
}


/**
 * Returns the cost of building the specified roadtype.
 * @param rti The roadtype being built.
 * @return The cost multiplier.
 */
static inline Money RoadBuildCost(RoadTypeIdentifier rtid)
{
	assert(rtid.IsValid());
	return (_price[PR_BUILD_ROAD] * GetRoadTypeInfo(rtid)->cost_multiplier) >> 3;
}

/**
 * Returns the 'cost' of clearing the specified railtype.
 * @param railtype The railtype being removed.
 * @return The cost.
 */
static inline Money RoadClearCost(RoadTypeIdentifier rtid)
{
	/* Clearing rail in fact earns money, but if the build cost is set
	 * very low then a loophole exists where money can be made.
	 * In this case we limit the removal earnings to 3/4s of the build
	 * cost.
	 */
	assert(rtid.IsValid());
	return max(_price[PR_CLEAR_ROAD], -RoadBuildCost(rtid) * 3 / 4);
}

RoadTypeIdentifier GetRoadTypeByLabel(RoadTypeLabel label, RoadType subtype, bool allow_alternate_labels = true);

void ResetRoadTypes();
void InitRoadTypes();
RoadTypeIdentifier AllocateRoadType(RoadTypeLabel label, RoadType basetype);

extern RoadTypeIdentifier _sorted_roadtypes[ROADTYPE_END][ROADSUBTYPE_END];
extern uint8 _sorted_roadtypes_size[ROADTYPE_END];

/**
 * Loop header for iterating over roadtypes, sorted by sortorder.
 * @param var Roadtype.
 */
#define FOR_ALL_SORTED_ROADTYPES(var, type) for (uint8 index = 0; index < _sorted_roadtypes_size[type] && (var = _sorted_roadtypes[type][index], true) ; index++)

#endif /* ROAD_H */

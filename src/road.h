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

/** Roadtype flags. Starts with RO instead of R because R is used for rails */
enum RoadTypeFlags {
	ROTF_CATENARY = 7,                   ///< Bit number for adding catenary

	ROTFB_NONE = 0,                      ///< All flags cleared.
	ROTFB_CATENARY = 1 << ROTF_CATENARY, ///< Value for drawing a catenary.
};
DECLARE_ENUM_AS_BIT_SET(RoadTypeFlags)

struct SpriteGroup;

/** Sprite groups for a roadtype. */
enum RoadTypeSpriteGroup {
	ROTSG_CURSORS,     ///< Cursor and toolbar icon images
	ROTSG_OVERLAY,     ///< Images for overlaying track
	ROTSG_GROUND,      ///< Main group of ground images
	ROTSG_TUNNEL,      ///< Main group of ground images for snow or desert
	ROTSG_WIRES,       ///< Catenary wires
	ROTSG_PYLONS,      ///< Catenary pylons
	ROTSG_BRIDGE,      ///< Bridge surface images
	ROTSG_CROSSING,    ///< Level crossing overlay images
	ROTSG_DEPOT,       ///< Depot images
	ROTSG_FENCES,      ///< Fence images
	ROTSG_TUNNEL_PORTAL, ///< Tunnel portal overlay
	ROTSG_END,
};

/** List of road type labels. */
typedef SmallVector<RoadTypeLabel, 4> RoadTypeLabelList;

struct RoadtypeInfo {
	struct {
		StringID name;            ///< Name of this rail type.
		StringID toolbar_caption; ///< Caption in the construction toolbar GUI for this rail type.
		StringID menu_text;       ///< Name of this rail type in the main toolbar dropdown.
		StringID build_caption;   ///< Caption of the build vehicle GUI for this rail type.
		StringID replace_text;    ///< Text used in the autoreplace GUI.
		StringID new_loco;        ///< Name of an engine for this type of rail in the engine preview GUI.
	} strings;                        ///< Strings associated with the rail type.


	/** bitmask to the OTHER roadtypes on which a vehicle of THIS roadtype generates power */
	RoadTypes powered_roadtypes;

	/** bitmask to the OTHER roadtypes on which a vehicle of THIS roadtype can physically travel */
	RoadTypes compatible_roadtypes;

	/**
	 * Bridge offset
	 */
	SpriteID bridge_offset;

	/**
	 * Original roadtype number to use when drawing non-newgrf roadtypes, or when drawing stations.
	 */
	byte fallback_roadtype;

	/**
	 * Multiplier for curve maximum speed advantage
	 */
	byte curve_speed;

	/**
	 * Bit mask of road type flags
	 */
	RoadTypeFlags flags;

	/**
	 * Cost multiplier for building this road type
	 */
	uint16 cost_multiplier;

	/**
	 * Cost multiplier for maintenance of this road type
	 */
	uint16 maintenance_multiplier;

	/**
	 * Maximum speed for vehicles travelling on this road type
	 */
	uint16 max_speed;

	/**
	 * Unique 32 bit road type identifier
	 */
	RoadTypeLabel label;

	/**
	 * Road type labels this type provides in addition to the main label.
	 */
	RoadTypeLabelList alternate_labels;

	/**
	 * Colour on mini-map
	 */
	byte map_colour;

	/**
	 * Introduction date.
	 * When #INVALID_DATE or a vehicle using this roadtype gets introduced earlier,
	 * the vehicle's introduction date will be used instead for this roadtype.
	 * The introduction at this date is furthermore limited by the
	 * #introduction_required_types.
	 */
	Date introduction_date;

	/**
	 * Bitmask of roadtypes that are required for this roadtype to be introduced
	 * at a given #introduction_date.
	 */
	RoadTypes introduction_required_roadtypes;

	/**
	 * Bitmask of which other roadtypes are introduced when this roadtype is introduced.
	 */
	RoadTypes introduces_roadtypes;

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
};

/**
 * Returns a pointer to the Roadtype information for a given roadtype
 * @param roadtype the road type which the information is requested for
 * @return The pointer to the RoadtypeInfo
 */
static inline const RoadtypeInfo *GetRoadTypeInfo(RoadType roadtype)
{
	extern RoadtypeInfo _roadtypes[ROADTYPE_END];
	assert(roadtype < ROADTYPE_END);
	return &_roadtypes[roadtype];
}

/**
 * Checks if an engine of the given RoadType can drive on a tile with a given
 * RoadType. This would normally just be an equality check, but for electrified
 * roads (which also support non-electric vehicles).
 * @return Whether the engine can drive on this tile.
 * @param  vehicletype The RoadType of the engine we are considering.
 * @param  tiletype   The RoadType of the tile we are considering.
 */
static inline bool IsCompatibleRoad(RoadType vehicletype, RoadType tiletype)
{
	return HasBit(GetRoadTypeInfo(vehicletype)->compatible_roadtypes, tiletype);
}

/**
 * Checks if an engine of the given RoadType got power on a tile with a given
 * RoadType. This would normally just be an equality check, but for electrified
 * roads (which also support non-electric vehicles).
 * @return Whether the engine got power on this tile.
 * @param  vehicletype The RoadType of the engine we are considering.
 * @param  tiletype   The RoadType of the tile we are considering.
 */
static inline bool HasPowerOnRoad(RoadType vehicletype, RoadType tiletype)
{
	return HasBit(GetRoadTypeInfo(vehicletype)->powered_roadtypes, tiletype);
}

RoadType GetRoadTypeByLabel(RoadTypeLabel label, bool allow_alternate_labels = true);

void ResetRoadTypes();
void InitRoadTypes();
RoadType AllocateRoadType(RoadTypeLabel label);

extern RoadType _sorted_roadtypes[ROADTYPE_END];
extern uint8 _sorted_roadtypes_size;

/**
 * Loop header for iterating over roadtypes, sorted by sortorder.
 * @param var Roadtype.
 */
#define FOR_ALL_SORTED_ROADTYPES(var) for (uint8 index = 0; index < _sorted_roadtypes_size && (var = _sorted_roadtypes[index], true) ; index++)

#endif /* ROAD_H */

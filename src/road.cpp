/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file road.cpp Generic road related functions. */

#include "stdafx.h"
#include "rail_map.h"
#include "road_map.h"
#include "water_map.h"
#include "genworld.h"
#include "company_func.h"
#include "company_base.h"
#include "engine_base.h"
#include "date_func.h"
#include "landscape.h"
#include "road.h"
#include "road_func.h"

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
							const RoadBits neighbor_rb = GetAnyRoadBits(neighbor_tile, ROADTYPE_ROAD) | GetAnyRoadBits(neighbor_tile, ROADTYPE_TRAM); // TODO

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
 * @param rtid RoadType to test
 * @return true if company has the requested RoadType available
 */
bool HasRoadTypeAvail(const CompanyID company, RoadTypeIdentifier rtid)
{
	if (company == OWNER_DEITY || company == OWNER_TOWN || _game_mode == GM_EDITOR || _generating_world) {
		return rtid.basetype == ROADTYPE_ROAD && rtid.subtype == ROADSUBTYPE_NORMAL; // TODO
	} else {
		Company *c = Company::GetIfValid(company);
		if (c == NULL) return false;
		return HasBit(c->avail_roadtypes[rtid.basetype], rtid.subtype);
	}
}

/**
 * Validate functions for rail building.
 * @param rtid road type to check.
 * @return true if the current company may build the road.
 */
bool ValParamRoadType(RoadTypeIdentifier rtid)
{
	return rtid.IsValid() && HasRoadTypeAvail(_current_company, rtid);
}

/**
 * Add the road types that are to be introduced at the given date.
 * @param rt      Roadtype
 * @param current The currently available roadsubtypes.
 * @param date    The date for the introduction comparisons.
 * @return The road types that should be available when date
 *         introduced road types are taken into account as well.
 */
RoadSubTypes AddDateIntroducedRoadTypes(RoadType rt, RoadSubTypes current, Date date)
{
	RoadSubTypes rts = current;

	for (RoadSubType rst = ROADSUBTYPE_BEGIN; rst != ROADSUBTYPE_END; rst++) {
		const RoadtypeInfo *rti = GetRoadTypeInfo(RoadTypeIdentifier(rt, rst));
		/* Unused road type. */
		if (rti->label == 0) continue;

		/* Not date introduced. */
		if (!IsInsideMM(rti->introduction_date, 0, MAX_DAY)) continue;

		/* Not yet introduced at this date. */
		if (rti->introduction_date > date) continue;

		/* Have we introduced all required roadtypes? */
		RoadSubTypes required = rti->introduction_required_roadtypes;
		if ((rts & required) != required) continue;

		rts |= rti->introduces_roadtypes;
	}

	/* When we added roadtypes we need to run this method again; the added
	 * roadtypes might enable more rail types to become introduced. */
	return rts == current ? rts : AddDateIntroducedRoadTypes(rt, rts, date);
}

/**
 * Get the road (sub) types the given company can build.
 * @param company the company to get the roadtypes for.
 * @param rt the base road type to check
 * @return the available road sub types.
 */
RoadSubTypes GetCompanyRoadtypes(CompanyID company, RoadType rt)
{
	RoadSubTypes rst = ROADSUBTYPES_NONE;

	Engine *e;
	FOR_ALL_ENGINES_OF_TYPE(e, VEH_ROAD) {
		const EngineInfo *ei = &e->info;

		if (HasBit(ei->climates, _settings_game.game_creation.landscape) &&
				(HasBit(e->company_avail, company) || _date >= e->intro_date + DAYS_IN_YEAR) &&
				rt == (HasBit(ei->misc_flags, EF_ROAD_TRAM) ? ROADTYPE_TRAM : ROADTYPE_ROAD)) {
			rst |= GetRoadTypeInfo(e->GetRoadType())->introduces_roadtypes;
		}
	}

	return AddDateIntroducedRoadTypes(rt, rst, _date);
}

/**
 * Get the road type for a given label.
 * @param label the roadtype label.
 * @param allow_alternate_labels Search in the alternate label lists as well.
 * @return the roadtype.
 */
RoadTypeIdentifier GetRoadTypeByLabel(RoadTypeLabel label, RoadType basetype, bool allow_alternate_labels)
{
	RoadTypeIdentifier rtid;

	rtid.basetype = basetype;

	/* Loop through each road type until the label is found */
	for (RoadSubType r = ROADSUBTYPE_BEGIN; r != ROADSUBTYPE_END; r++) {
		rtid.subtype = r;
		const RoadtypeInfo *rti = GetRoadTypeInfo(rtid);
		if (rti->label == label) return rtid;
	}

	if (allow_alternate_labels) {
		/* Test if any road type defines the label as an alternate. */
		for (RoadSubType r = ROADSUBTYPE_BEGIN; r != ROADSUBTYPE_END; r++) {
			rtid.subtype = r;
			const RoadtypeInfo *rti = GetRoadTypeInfo(rtid);
			if (rti->alternate_labels.Contains(label)) return rtid;
		}
	}

	/* No matching label was found, so it is invalid */
	rtid.basetype = INVALID_ROADTYPE;
	rtid.subtype = INVALID_ROADSUBTYPE;
	return rtid;
}

uint8 RoadTypeIdentifier::Pack() const
{
	assert(this->IsValid());

	return this->basetype | (this->subtype << 1);
}

bool RoadTypeIdentifier::UnpackIfValid(uint32 data)
{
	this->basetype = (RoadType)GB(data, 0, 1);
	this->subtype = (RoadSubType)GB(data, 1, 4);

	return this->IsValid();
}

/* static */ RoadTypeIdentifier RoadTypeIdentifier::Unpack(uint32 data)
{
	RoadTypeIdentifier result;
	bool ret = result.UnpackIfValid(data);
	assert(ret);
	return result;
}

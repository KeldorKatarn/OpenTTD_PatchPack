/* $Id: vehiclelist.h 26112 2013-11-25 16:36:11Z rubidium $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file vehiclelist.h Functions and type for generating vehicle lists. */

#ifndef VEHICLELIST_H
#define VEHICLELIST_H

#include "core/smallvec_type.hpp"
#include "vehicle_type.h"
#include "company_type.h"
#include "tile_type.h"

/** Vehicle List type flags */
enum VehicleListType {
	VL_STANDARD,
	VL_SHARED_ORDERS,
	VL_STATION_LIST,
	VL_DEPOT_LIST,
	VL_GROUP_LIST,
	VL_SLOT_LIST,
	VLT_END
};

enum VehicleSortType
{
	VST_NUMBER,
	VST_NAME,
	VST_AGE,
	VST_PROFIT_THIS_YEAR,
	VST_PROFIT_LAST_YEAR,
	VST_PROFIT_LIFETIME,
	VST_CARGO,
	VST_RELIABILITY,
	VST_MAX_SPEED,
	VST_MODEL,
	VST_VALUE,
	VST_LENGTH,
	VST_TIME_TO_LIVE,
	VST_TIMETABLE_DELAY,
};

/** The information about a vehicle list. */
struct VehicleListIdentifier {
	VehicleListType type; ///< The type of vehicle list.
	VehicleType vtype;    ///< The vehicle type associated with this list.
	CompanyID company;    ///< The company associated with this list.
	uint32 index;         ///< A vehicle list type specific index.

	uint32 Pack() const;
	bool UnpackIfValid(uint32 data);
	static VehicleListIdentifier UnPack(uint32 data);

	/**
	 * Create a simple vehicle list.
	 * @param type    List type.
	 * @param vtype   Vehicle type associated with this list.
	 * @param company Company associated with this list.
	 * @param index   Optional type specific index.
	 */
	VehicleListIdentifier(VehicleListType type, VehicleType vtype, CompanyID company, uint index = 0) :
		type(type), vtype(vtype), company(company), index(index) {}

	VehicleListIdentifier() : type(), vtype(), company(), index() {}
};

/** A list of vehicles. */
typedef SmallVector<const Vehicle *, 32> VehicleList;

bool GenerateVehicleSortList(VehicleList *list, const VehicleListIdentifier &identifier);
void BuildDepotVehicleList(VehicleType type, TileIndex tile, VehicleList *engine_list, VehicleList *wagon_list, bool individual_wagons = false);
uint GetUnitNumberDigits(VehicleList &vehicles);

#endif /* VEHICLELIST_H */

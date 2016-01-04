/* $Id: script_stationlist.cpp 23632 2011-12-19 21:05:25Z truebrain $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_stationlist.cpp Implementation of ScriptStationList and friends. */

#include "../../stdafx.h"
#include "script_stationlist.hpp"
#include "script_vehicle.hpp"
#include "../../station_base.h"
#include "../../vehicle_base.h"

ScriptStationList::ScriptStationList(ScriptStation::StationType station_type)
{
	Station *st;
	FOR_ALL_STATIONS(st) {
		if ((st->owner == ScriptObject::GetCompany() || ScriptObject::GetCompany() == OWNER_DEITY) && (st->facilities & station_type) != 0) this->AddItem(st->index);
	}
}

ScriptStationList_Vehicle::ScriptStationList_Vehicle(VehicleID vehicle_id)
{
	if (!ScriptVehicle::IsValidVehicle(vehicle_id)) return;

	Vehicle *v = ::Vehicle::Get(vehicle_id);

	for (Order *o = v->GetFirstOrder(); o != NULL; o = o->next) {
		if (o->IsType(OT_GOTO_STATION)) this->AddItem(o->GetDestination());
	}
}

/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file timetable_cmd.cpp Commands related to time tabling. */

#include "stdafx.h"
#include "command_func.h"
#include "company_func.h"
#include "date_func.h"
#include "window_func.h"
#include "vehicle_base.h"
#include "cmd_helper.h"
#include "settings_type.h"
#include "core/sort_func.hpp"
#include "scope.h"

#include "table/strings.h"

#include "safeguards.h"

/**
 * Change/update a particular timetable entry.
 * @param v            The vehicle to change the timetable of.
 * @param order_number The index of the timetable in the order list.
 * @param val          The new data of the timetable entry.
 * @param mtf          Which part of the timetable entry to change.
 * @param timetabled   If the new value is explicitly timetabled.
 */
static void ChangeTimetable(Vehicle *v, VehicleOrderID order_number, uint16 val, ModifyTimetableFlags mtf, bool timetabled)
{
	Order *order = v->GetOrder(order_number);
	int total_delta = 0;
	int timetable_delta = 0;

	switch (mtf) {
		case MTF_WAIT_TIME:
			if (!order->IsType(OT_CONDITIONAL)) {
				total_delta = val - order->GetWaitTime();
				timetable_delta = (timetabled ? val : 0) - order->GetTimetabledWait();
			}
			order->SetWaitTime(val);
			order->SetWaitTimetabled(timetabled);
			break;

		case MTF_TRAVEL_TIME:
			if (!order->IsType(OT_CONDITIONAL)) {
				total_delta = val - order->GetTravelTime();
				timetable_delta = (timetabled ? val : 0) - order->GetTimetabledTravel();
			}
			if (order->IsType(OT_CONDITIONAL)) assert_msg(val == order->GetTravelTime(), "%u == %u", val, order->GetTravelTime());
			order->SetTravelTime(val);
			order->SetTravelTimetabled(timetabled);
			break;

		case MTF_TRAVEL_SPEED:
			order->SetMaxSpeed(val);
			break;

		default:
			NOT_REACHED();
	}
	v->UpdateTotalDuration(total_delta);
	v->UpdateTimetableDuration(timetable_delta);

	for (v = v->FirstShared(); v != NULL; v = v->NextShared()) {
		if (v->cur_real_order_index == order_number && v->current_order.Equals(*order)) {
			switch (mtf) {
				case MTF_WAIT_TIME:
					v->current_order.SetWaitTime(val);
					v->current_order.SetWaitTimetabled(timetabled);
					break;

				case MTF_TRAVEL_TIME:
					v->current_order.SetTravelTime(val);
					v->current_order.SetTravelTimetabled(timetabled);
					break;

				case MTF_TRAVEL_SPEED:
					v->current_order.SetMaxSpeed(val);
					break;

				default:
					NOT_REACHED();
			}
		}
		SetWindowDirty(WC_VEHICLE_TIMETABLE, v->index);
	}
}

/**
 * Change timetable data of an order.
 * @param tile Not used.
 * @param flags Operation to perform.
 * @param p1 Various bitstuffed elements
 * - p1 = (bit  0-19) - Vehicle with the orders to change.
 * - p1 = (bit 20-27) - Order index to modify.
 * - p1 = (bit 28-29) - Timetable data to change (@see ModifyTimetableFlags)
 * @param p2 The amount of time to wait.
 * - p2 = (bit  0-15) - The data to modify as specified by p1 bits 28-29.
 *                      0 to clear times, UINT16_MAX to clear speed limit.
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdChangeTimetable(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	VehicleID veh = GB(p1, 0, 20);

	Vehicle *v = Vehicle::GetIfValid(veh);
	if (v == NULL || !v->IsPrimaryVehicle()) return CMD_ERROR;

	CommandCost ret = CheckOwnership(v->owner);
	if (ret.Failed()) return ret;

	VehicleOrderID order_number = GB(p1, 20, 8);
	Order *order = v->GetOrder(order_number);
	if (order == NULL || order->IsType(OT_IMPLICIT)) return CMD_ERROR;

	ModifyTimetableFlags mtf = Extract<ModifyTimetableFlags, 28, 2>(p1);
	if (mtf >= MTF_END) return CMD_ERROR;

	int wait_time   = order->GetWaitTime();
	int travel_time = order->GetTravelTime();
	int max_speed   = order->GetMaxSpeed();
	switch (mtf) {
		case MTF_WAIT_TIME:
			wait_time = GB(p2, 0, 16);
			break;

		case MTF_TRAVEL_TIME:
			travel_time = GB(p2, 0, 16);
			break;

		case MTF_TRAVEL_SPEED:
			max_speed = GB(p2, 0, 16);
			if (max_speed == 0) max_speed = UINT16_MAX; // Disable speed limit.
			break;

		default:
			NOT_REACHED();
	}

	if (wait_time != order->GetWaitTime()) {
		switch (order->GetType()) {
			case OT_GOTO_STATION:
				if (order->GetNonStopType() & ONSF_NO_STOP_AT_DESTINATION_STATION) return_cmd_error(STR_ERROR_TIMETABLE_NOT_STOPPING_HERE);
				break;
 
			case OT_GOTO_DEPOT:
				break;

			case OT_CONDITIONAL:
				break;

			default: return_cmd_error(STR_ERROR_TIMETABLE_ONLY_WAIT_AT_STATIONS);
		}
	}

	if (travel_time != order->GetTravelTime() && order->IsType(OT_CONDITIONAL)) return CMD_ERROR;
	if (max_speed != order->GetMaxSpeed() && (order->IsType(OT_CONDITIONAL) || v->type == VEH_AIRCRAFT)) return CMD_ERROR;

	if (flags & DC_EXEC) {
		switch (mtf) {
			case MTF_WAIT_TIME:
				/* Set time if changing the value or confirming an estimated time as timetabled. */
				if (wait_time != order->GetWaitTime() || (wait_time > 0 && !order->IsWaitTimetabled())) {
					ChangeTimetable(v, order_number, wait_time, MTF_WAIT_TIME, wait_time > 0);
				}
				break;

			case MTF_TRAVEL_TIME:
				/* Set time if changing the value or confirming an estimated time as timetabled. */
				if (travel_time != order->GetTravelTime() || (travel_time > 0 && !order->IsTravelTimetabled())) {
					ChangeTimetable(v, order_number, travel_time, MTF_TRAVEL_TIME, travel_time > 0);
				}
				break;

			case MTF_TRAVEL_SPEED:
				if (max_speed != order->GetMaxSpeed()) {
					ChangeTimetable(v, order_number, max_speed, MTF_TRAVEL_SPEED, max_speed != UINT16_MAX);
				}
				break;

			default:
				break;
		}
	}

	return CommandCost();
}
 
/**
 * Change timetable data of all orders of a vehicle.
 * @param tile Not used.
 * @param flags Operation to perform.
 * @param p1 Various bitstuffed elements
 * - p1 = (bit  0-19) - Vehicle with the orders to change.
 * - p1 = (bit 20-27) - unused
 * - p1 = (bit 28-29) - Timetable data to change (@see ModifyTimetableFlags)
 * - p1 = (bit    30) - 0 to set timetable wait/travel time, 1 to clear it
 * @param p2 The amount of time to wait.
 * - p2 = (bit  0-15) - The data to modify as specified by p1 bits 28-29.
 *                      0 to clear times, UINT16_MAX to clear speed limit.
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdBulkChangeTimetable(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	VehicleID veh = GB(p1, 0, 20);

	Vehicle *v = Vehicle::GetIfValid(veh);
	if (v == NULL || !v->IsPrimaryVehicle()) return CMD_ERROR;

	CommandCost ret = CheckOwnership(v->owner);
	if (ret.Failed()) return ret;

	ModifyTimetableFlags mtf = Extract<ModifyTimetableFlags, 28, 2>(p1);
	if (mtf >= MTF_END) return CMD_ERROR;

	if (v->GetNumOrders() == 0) return CMD_ERROR;

	if (flags & DC_EXEC) {
		for (VehicleOrderID order_number = 0; order_number < v->GetNumOrders(); order_number++) {
			Order *order = v->GetOrder(order_number);
			if (order == NULL || order->IsType(OT_IMPLICIT)) continue;

			uint32 new_p1 = p1;
			SB(new_p1, 20, 8, order_number);
			DoCommand(tile, new_p1, p2, flags, CMD_CHANGE_TIMETABLE);
		}
	}

	return CommandCost();
}

/**
 * Clear the lateness counter to make the vehicle on time.
 * @param tile Not used.
 * @param flags Operation to perform.
 * @param p1 Various bitstuffed elements
 * - p1 = (bit  0-19) - Vehicle with the orders to change.
 * @param p2 unused
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdSetVehicleOnTime(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	VehicleID veh = GB(p1, 0, 20);

	Vehicle *v = Vehicle::GetIfValid(veh);
	if (v == NULL || !v->IsPrimaryVehicle() || !v->HasOrdersList()) return CMD_ERROR;

	CommandCost ret = CheckOwnership(v->owner);
	if (ret.Failed()) return ret;

	if (flags & DC_EXEC) {
		v->lateness_counter = 0;
		SetWindowDirty(WC_VEHICLE_TIMETABLE, v->index);
	}

	return CommandCost();
}

/**
 * Order vehicles based on their timetable. The vehicles will be sorted in order
 * they would reach the first station.
 *
 * @param ap First Vehicle pointer.
 * @param bp Second Vehicle pointer.
 * @return Comparison value.
 */
static int CDECL VehicleTimetableSorter(Vehicle * const *ap, Vehicle * const *bp)
{
	const Vehicle *a = *ap;
	const Vehicle *b = *bp;

	VehicleOrderID a_order = a->cur_real_order_index;
	VehicleOrderID b_order = b->cur_real_order_index;
	int j = (int)b_order - (int)a_order;

	/* Are we currently at an ordered station (un)loading? */
	bool a_load = a->current_order.IsType(OT_LOADING) && a->current_order.GetNonStopType() != ONSF_STOP_EVERYWHERE;
	bool b_load = b->current_order.IsType(OT_LOADING) && b->current_order.GetNonStopType() != ONSF_STOP_EVERYWHERE;

	/* If the current order is not loading at the ordered station, decrease the order index by one since we have
	 * not yet arrived at the station (and thus the timetable entry; still in the travelling of the previous one).
	 * Since the ?_order variables are unsigned the -1 will flow under and place the vehicles going to order #0 at
	 * the begin of the list with vehicles arriving at #0. */
	if (!a_load) a_order--;
	if (!b_load) b_order--;

	/* First check the order index that accounted for loading, then just the raw one. */
	int i = (int)b_order - (int)a_order;
	if (i != 0) return i;
	if (j != 0) return j;

	/* Look at the time we spent in this order; the higher, the closer to its destination. */
	i = b->current_order_time - a->current_order_time;
	if (i != 0) return i;

	/* If all else is equal, use some unique index to sort it the same way. */
	return b->unitnumber - a->unitnumber;
}

/**
 * Set the start date of the timetable.
 * @param tile Not used.
 * @param flags Operation to perform.
 * @param p2 Various bitstuffed elements
 * - p2 = (bit 0-19) - Vehicle ID.
 * - p2 = (bit 20)   - Set to 1 to set timetable start for all vehicles sharing this order
 * @param p2 The timetable start date.
 * @param text Not used.
 * @return The error or cost of the operation.
 */
CommandCost CmdSetTimetableStart(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	bool timetable_all = HasBit(p1, 20);
	Vehicle *v = Vehicle::GetIfValid(GB(p1, 0, 20));
	if (v == NULL || !v->IsPrimaryVehicle() || !v->HasOrdersList()) return CMD_ERROR;

	CommandCost ret = CheckOwnership(v->owner);
	if (ret.Failed()) return ret;

	/* Don't let a timetable start more than 15 years into the future or 1 year in the past. */
	Date start_date = (Date)p2;
	if (start_date < 0 || start_date > MAX_DAY) return CMD_ERROR;
	if (start_date - _date > 15 * DAYS_IN_LEAP_YEAR) return CMD_ERROR;
	if (_date - start_date > DAYS_IN_LEAP_YEAR) return CMD_ERROR;
	if (timetable_all && !v->HasCompleteTimetable()) return CMD_ERROR;

	if (flags & DC_EXEC) {
		SmallVector<Vehicle *, 8> vehs;

		if (timetable_all) {
			for (Vehicle *w = v->FirstShared(); w != NULL; w = w->NextShared()) {
				*vehs.Append() = w;
			}
		} else {
			*vehs.Append() = v;
		}

		int total_duration = v->GetTimetableTotalDuration();
		int num_vehs = vehs.Length();

		if (num_vehs >= 2) {
			QSortT(vehs.Begin(), vehs.Length(), &VehicleTimetableSorter);
		}

		int base = vehs.FindIndex(v);

		for (Vehicle **viter = vehs.Begin(); viter != vehs.End(); viter++) {
			int idx = (viter - vehs.Begin()) - base;
			Vehicle *w = *viter;

			w->lateness_counter = 0;
			ClrBit(w->vehicle_flags, VF_TIMETABLE_STARTED);
			/* Do multiplication, then division to reduce rounding errors. */
			w->timetable_start = start_date + idx * total_duration / num_vehs / DAY_TICKS;
			SetWindowDirty(WC_VEHICLE_TIMETABLE, w->index);
		}

	}

	return CommandCost();
}

/**
* Start or stop automatic management of timetables.
* @param tile Not used.
* @param flags Operation to perform.
* @param p1 Vehicle index.
* @param p2 Various bitstuffed elements
* - p2 = (bit 0) - Set to 1 to enable, 0 to disable automation.
* - p2 = (bit 1) - Ctrl was pressed. Used to keep wait times.
* @param text unused
* @return the cost of this operation or an error
*/
CommandCost CmdAutomateTimetable(TileIndex index, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	VehicleID veh = GB(p1, 0, 16);

	Vehicle *v = Vehicle::GetIfValid(veh);
	if (v == NULL || !v->IsPrimaryVehicle()) return CMD_ERROR;

	CommandCost ret = CheckOwnership(v->owner);
	if (ret.Failed()) return ret;

	if (flags & DC_EXEC) {
		for (Vehicle *v2 = v->FirstShared(); v2 != NULL; v2 = v2->NextShared()) {
			if (HasBit(p2, 0)) {
				/* Automated timetable. */
				SetBit(v2->vehicle_flags, VF_AUTOMATE_TIMETABLE);

				if (HasBit(p2, 1)) {
					SetBit(v2->vehicle_flags, VF_AUTOMATE_PRES_WAIT_TIME);
				}

				ClrBit(v2->vehicle_flags, VF_TIMETABLE_STARTED);
				v2->timetable_start = 0;
				v2->lateness_counter = 0;
				v2->current_loading_time = 0;
			}
			else {
				/* De-automate timetable. */
				ClrBit(v2->vehicle_flags, VF_AUTOMATE_TIMETABLE);
				ClrBit(v2->vehicle_flags, VF_AUTOMATE_PRES_WAIT_TIME);
			}
			SetWindowDirty(WC_VEHICLE_TIMETABLE, v2->index);
		}
	}

	return CommandCost();
}

/**
 * Confirm all estimated wait and travel times as timetabled.
 * @param tile Not used.
 * @param flags Operation to perform.
 * @param p1 Vehicle index.
 * @param p2 Unused
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdConfirmAll(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	VehicleID veh = GB(p1, 0, 20);

	Vehicle *v = Vehicle::GetIfValid(veh);
	if (v == nullptr || !v->IsPrimaryVehicle() || !v->HasOrdersList()) return CMD_ERROR;

	CommandCost ret = CheckOwnership(v->owner);
	if (ret.Failed()) return ret;

	if (flags & DC_EXEC) {
		int num_orders = v->GetNumOrders();
		int timetable_delta = 0;

		for (int i = 0; i < num_orders; ++i) {
			Order* order = v->GetOrderAt(i);

			assert(order != nullptr);

			if (!order->IsType(OT_IMPLICIT)) {
				if (order->GetWaitTime() != 0 && !order->IsWaitTimetabled()) {
					timetable_delta += order->GetWaitTime() - order->GetTimetabledWait();
					order->SetWaitTimetabled(true);
				}

				if (order->GetTravelTime() != 0 && !order->IsTravelTimetabled()) {
					timetable_delta += order->GetTravelTime() - order->GetTimetabledTravel();
					order->SetTravelTimetabled(true);
				}
			}
		}

		v->UpdateTimetableDuration(timetable_delta);

		SetWindowDirty(WC_VEHICLE_TIMETABLE, v->index);
	}

	return CommandCost();
}

/**
* Set new separation parameters
* @param tile  Not used.
* @param flags Operation to perform.
* @param p1    Vehicle id.
* @param p2
*   - p2 = (bit 0-2)  - Separation mode (@see TTSepMode)
*   - p2 = (bit 3-31) - Separation parameter (Unused if #TTS_MODE_OFF | #TTS_MODE_AUTO,
*                       Number of vehicles if #TTS_MODE_MAN_N, separation delay in ticks if #TTS_MODE_MAN_T).
* @param text  Not used.
* @return      The error or cost of the operation.
*/
CommandCost CmdReinitSeparation(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	Vehicle *v = Vehicle::GetIfValid(p1);
	if (v == NULL || !v->IsPrimaryVehicle()) return CMD_ERROR;

	CommandCost ret = CheckOwnership(v->owner);
	if (ret.Failed()) return ret;

	if (flags & DC_EXEC) {
		v->SetSepSettings((TTSepMode)GB(p2, 0, 3), GB(p2, 3, 29));
	}

	return CommandCost();
}

/**
 * Update the timetable for the vehicle.
 * @param v The vehicle to update the timetable for.
 * @param traveling Whether we just traveled or waited at a station.
 */
void UpdateVehicleTimetable(Vehicle *v, bool travelling)
{
	if (!travelling) v->current_loading_time++; // +1 because this time is one tick behind
	uint time_taken = v->current_order_time;
	uint time_loading = v->current_loading_time;

	// We are on our way so vehicle separation has finished.
	if (travelling) ClrBit(v->vehicle_flags, VF_SEPARATION_IN_PROGRESS);

	v->current_order_time = 0;
	v->current_loading_time = 0;

	if (v->current_order.IsType(OT_IMPLICIT)) return; // no timetabling of auto orders

	if (v->cur_real_order_index >= v->GetNumOrders()) return;
	Order *real_current_order = v->GetOrder(v->cur_real_order_index);
	Order *real_timetable_order = v->cur_timetable_order_index != INVALID_VEH_ORDER_ID ? v->GetOrder(v->cur_timetable_order_index) : nullptr;

	auto guard = scope_guard([v, travelling]() {
		/* On next call, when updating waiting time, use current order even if travel field of current order isn't being updated */
		if (travelling) v->cur_timetable_order_index = v->cur_real_order_index;
	});

	VehicleOrderID first_manual_order = 0;
	for (Order *o = v->GetFirstOrder(); o != NULL && o->IsType(OT_IMPLICIT); o = o->next) {
		++first_manual_order;
	}

	bool just_started = false;

	/* Start automated timetables at first opportunity */
	if (!HasBit(v->vehicle_flags, VF_TIMETABLE_STARTED) && HasBit(v->vehicle_flags, VF_AUTOMATE_TIMETABLE)) {
		SetBit(v->vehicle_flags, VF_TIMETABLE_STARTED);

		v->lateness_counter = 0;

		for (v = v->FirstShared(); v != NULL; v = v->NextShared()) {
			SetWindowDirty(WC_VEHICLE_TIMETABLE, v->index);
		}
		return;
	}

	/* This vehicle is arriving at the first destination in the timetable. */
	if (v->cur_real_order_index == first_manual_order && travelling) {
		v->trip_history.NewRound();
		/* If the start date hasn't been set, or it was set automatically when
		 * the vehicle last arrived at the first destination, update it to the
		 * current time. Otherwise set the late counter appropriately to when
		 * the vehicle should have arrived. */
		just_started = !HasBit(v->vehicle_flags, VF_TIMETABLE_STARTED);

		if (v->timetable_start != 0) {
			v->lateness_counter = (_date - v->timetable_start) * DAY_TICKS + _date_fract;
			v->timetable_start = 0;
		}

		SetBit(v->vehicle_flags, VF_TIMETABLE_STARTED);
		SetWindowDirty(WC_VEHICLE_TIMETABLE, v->index);
	}

	if (!HasBit(v->vehicle_flags, VF_TIMETABLE_STARTED)) return;
	if (real_timetable_order == nullptr) return;

	bool is_conditional = real_timetable_order->IsType(OT_CONDITIONAL);
	bool remeasure_wait_time = !is_conditional && (!real_current_order->IsWaitTimetabled() ||
		(HasBit(v->vehicle_flags, VF_AUTOMATE_TIMETABLE) && !HasBit(v->vehicle_flags, VF_AUTOMATE_PRES_WAIT_TIME)));

	if (travelling && remeasure_wait_time) {
		/* We just finished traveling and want to remeasure the loading time,
		 * so do not apply any restrictions for the loading to finish. */
		v->current_order.SetWaitTime(0);
	}

	bool travel_field = travelling;
	if (is_conditional) {
		if (travelling) {
			/* conditional orders use the wait field for the jump-taken travel time */
			travel_field = false;
		} else {
			/* doesn't make sense to update wait time for conditional orders */
			return;
		}
	} else {
		assert(real_timetable_order == real_current_order);
	}

	if (just_started) return;

	/* Before modifying waiting times, check whether we want to preserve bigger ones. */
	if (travelling || (time_taken > real_timetable_order->GetWaitTime()) || remeasure_wait_time) {
		/* For trains/aircraft multiple movement cycles are done in one
		 * tick. This makes it possible to leave the station and process
		 * e.g. a depot order in the same tick, causing it to not fill
		 * the timetable entry like is done for road vehicles/ships.
		 * Thus always make sure at least one tick is used between the
		 * processing of different orders when filling the timetable. */
		if (travel_field && !real_timetable_order->IsTravelTimetabled()) {
			ChangeTimetable(v, v->cur_timetable_order_index, max(time_taken, 1U), MTF_TRAVEL_TIME, false);
		} else if (!travel_field && !real_timetable_order->IsWaitTimetabled()) {
			ChangeTimetable(v, v->cur_timetable_order_index, max(time_loading, 1U), MTF_WAIT_TIME, false);
		}
	}

	uint timetabled = travel_field ? real_timetable_order->GetTimetabledTravel() :
			real_timetable_order->GetTimetabledWait();

	/* Update the timetable to gradually shift order times towards the actual travel times. */
	if (timetabled != 0 && HasBit(v->vehicle_flags, VF_AUTOMATE_TIMETABLE) && (travelling || !HasBit(v->vehicle_flags, VF_AUTOMATE_PRES_WAIT_TIME))) {
		int32 new_time;
		if (travelling) {
			new_time = time_taken + _settings_game.order.timetable_auto_travel_buffer;

			assert(_settings_game.order.timetable_auto_travel_rounding != 0);

			if (new_time % _settings_game.order.timetable_auto_travel_rounding != 0) {
				new_time /= _settings_game.order.timetable_auto_travel_rounding;
				new_time += 1;
				new_time *= _settings_game.order.timetable_auto_travel_rounding;
			}
		}
		else {
			new_time = time_loading + _settings_game.order.timetable_auto_load_buffer;

			assert(_settings_game.order.timetable_auto_load_rounding != 0);

			if (new_time % _settings_game.order.timetable_auto_load_rounding != 0) {
				new_time /= _settings_game.order.timetable_auto_load_rounding;
				new_time += 1;
				new_time *= _settings_game.order.timetable_auto_load_rounding;
			}
		}

		if (new_time > (int32)timetabled * 4 && travelling) {
			/* Possible jam, clear time and restart timetable for all vehicles.
			* Otherwise we risk trains blocking 1-lane stations for long times. */
			ChangeTimetable(v, v->cur_timetable_order_index, 0, travel_field ? MTF_TRAVEL_TIME : MTF_WAIT_TIME, true);
			for (Vehicle *v2 = v->FirstShared(); v2 != NULL; v2 = v2->NextShared()) {
				ClrBit(v2->vehicle_flags, VF_TIMETABLE_STARTED);
				SetWindowDirty(WC_VEHICLE_TIMETABLE, v2->index);
			}
			return;
		}
		else if (!travelling || (new_time >= (int32)timetabled / 2)) {
			/* Compute running average, with sign conversion to avoid negative overflow. */
			if (new_time < (int32)timetabled) {
				new_time = ((int32)timetabled * 3 + new_time * 2 + 2) / 5;
			}
			else {
				new_time = ((int32)timetabled * 9 + new_time + 5) / 10;
			}
		}
		else {
			/* new time is less than half old time, set value directly */
		}

		if (new_time < 1) new_time = 1;
		if (new_time != (int32)timetabled) {
			ChangeTimetable(v, v->cur_timetable_order_index, new_time, travel_field ? MTF_TRAVEL_TIME : MTF_WAIT_TIME, true);
		}
	}
	else if (timetabled == 0 && HasBit(v->vehicle_flags, VF_AUTOMATE_TIMETABLE)) {
		/* Add times for orders that are not yet timetabled, even while not autofilling */
		const int32 new_time = travelling ? time_taken : time_loading;

		ChangeTimetable(v, v->cur_timetable_order_index, new_time, travel_field ? MTF_TRAVEL_TIME : MTF_WAIT_TIME, true);
	}

	/* Vehicles will wait at stations if they arrive early even if they are not
	 * timetabled to wait there, so make sure the lateness counter is updated
	 * when this happens. */
	if (timetabled == 0 && (travelling || v->lateness_counter >= 0)) return;

	v->lateness_counter -= (timetabled - time_taken);

	/* When we are more late than this timetabled bit takes we (somewhat expensively)
	 * check how many ticks the (fully filled) timetable has. If a timetable cycle is
	 * shorter than the amount of ticks we are late we reduce the lateness by the
	 * length of a full cycle till lateness is less than the length of a timetable
	 * cycle. When the timetable isn't fully filled the cycle will be INVALID_TICKS. */
	if (v->lateness_counter > (int)timetabled) {
		Ticks cycle = v->GetTimetableTotalDuration();
		if (cycle != INVALID_TICKS && v->lateness_counter > cycle) {
			v->lateness_counter %= cycle;
		}
	}

	for (v = v->FirstShared(); v != NULL; v = v->NextShared()) {
		SetWindowDirty(WC_VEHICLE_TIMETABLE, v->index);
	}
}

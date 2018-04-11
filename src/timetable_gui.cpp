/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file timetable_gui.cpp GUI for time tabling. */

#include "stdafx.h"
#include "command_func.h"
#include "gui.h"
#include "window_gui.h"
#include "window_func.h"
#include "textbuf_gui.h"
#include "strings_func.h"
#include "vehicle_base.h"
#include "string_func.h"
#include "gfx_func.h"
#include "company_func.h"
#include "date_func.h"
#include "date_gui.h"
#include "vehicle_gui.h"
#include "settings_type.h"
#include "viewport_func.h"

#include "widgets/timetable_widget.h"

#include "table/sprites.h"
#include "table/strings.h"
#include "widgets/dropdown_func.h"


/** Entries for mode selection dropdown list. Order must be identical to the one in #TTSepMode */
static const StringID TimetableSeparationDropdownOptions[6] = {
	STR_TTSEPARATION_AUTO,
	STR_TTSEPARATION_OFF,
	STR_TTSEPARATION_MAN_TIME,
	STR_TTSEPARATION_MAN_NUM,
	STR_TTSEPARATION_BUFFERED_AUTO,
	INVALID_STRING_ID,
};

#include "safeguards.h"

/** Container for the arrival/departure dates of a vehicle */
struct TimetableArrivalDeparture {
	Ticks arrival;   ///< The arrival time
	Ticks departure; ///< The departure time
};

/**
 * Set the timetable parameters in the format as described by the setting.
 * @param param1 the first DParam to fill
 * @param param2 the second DParam to fill
 * @param ticks  the number of ticks to 'draw'
 */
void SetTimetableParams(int param1, int param2, Ticks ticks)
{
	SetDParam(param1, STR_TIMETABLE_TICKS);
	SetDParam(param2, ticks);
}

/**
 * Check whether it is possible to determine how long the order takes.
 * @param order the order to check.
 * @param travelling whether we are interested in the travel or the wait part.
 * @return true if the travel/wait time can be used.
 */
static bool CanDetermineTimeTaken(const Order *order, bool travelling)
{
	/* Current order is conditional */
	if (order->IsType(OT_CONDITIONAL) || order->IsType(OT_IMPLICIT)) return false;
	/* No travel time and we have not already finished travelling */
	if (travelling && !order->IsTravelTimetabled()) return false;
	/* No wait time but we are loading at this timetabled station */
	if (!travelling && !order->IsWaitTimetabled() && order->IsType(OT_GOTO_STATION) &&
			!(order->GetNonStopType() & ONSF_NO_STOP_AT_DESTINATION_STATION)) {
		return false;
	}

	return true;
}


/**
 * Fill the table with arrivals and departures
 * @param v Vehicle which must have at least 2 orders.
 * @param start order index to start at
 * @param travelling Are we still in the travelling part of the start order
 * @param table Fill in arrival and departures including intermediate orders
 * @param offset Add this value to result and all arrivals and departures
 */
static void FillTimetableArrivalDepartureTable(const Vehicle *v, VehicleOrderID start, bool travelling, TimetableArrivalDeparture *table, Ticks offset)
{
	assert(table != NULL);
	assert(v->GetNumOrders() >= 2);
	assert(start < v->GetNumOrders());

	Ticks sum = offset;
	VehicleOrderID i = start;
	const Order *order = v->GetOrder(i);

	/* Pre-initialize with unknown time */
	for (int i = 0; i < v->GetNumOrders(); ++i) {
		table[i].arrival = table[i].departure = INVALID_TICKS;
	}

	/* Cyclically loop over all orders until we reach the current one again.
	 * As we may start at the current order, do a post-checking loop */
	do {
		/* Automatic orders don't influence the overall timetable;
		 * they just add some untimetabled entries, but the time till
		 * the next non-implicit order can still be known. */
		if (!order->IsType(OT_IMPLICIT)) {
			if (travelling || i != start) {
				if (!CanDetermineTimeTaken(order, true)) return;
				sum += order->GetTimetabledTravel();
				table[i].arrival = sum;
			}

			if (!CanDetermineTimeTaken(order, false)) return;
			sum += order->GetTimetabledWait();
			table[i].departure = sum;
		}

		++i;
		order = order->next;
		if (i >= v->GetNumOrders()) {
			i = 0;
			assert(order == NULL);
			order = v->GetFirstOrder();
		}
	} while (i != start);

	/* When loading at a scheduled station we still have to treat the
	 * travelling part of the first order. */
	if (!travelling) {
		if (!CanDetermineTimeTaken(order, true)) return;
		sum += order->GetTimetabledTravel();
		table[i].arrival = sum;
	}
}


struct TimetableWindow : Window {
	int sel_index;
	const Vehicle *vehicle;               ///< Vehicle monitored by the window.
	bool show_expected;                   ///< Whether we show expected arrival or scheduled
	uint deparr_time_width;               ///< The width of the departure/arrival time
	uint deparr_abbr_width;               ///< The width of the departure/arrival abbreviation
	Scrollbar *vscroll;
	bool query_is_speed_query;            ///< The currently open query window is a speed query and not a time query
	bool query_is_bulk_query;             ///< The currently open query window applies to all relevant orders.
	TTSepSettings new_sep_settings;       ///< Contains new separation settings.
	VehicleTimetableWidgets query_widget; ///< Required to determinate source of input query

	TimetableWindow(WindowDesc *desc, WindowNumber window_number) :
			Window(desc),
			sel_index(-1),
			vehicle(Vehicle::Get(window_number)),
			show_expected(true)
	{
		this->new_sep_settings = vehicle->GetTimetableSeparationSettings();
		this->CreateNestedTree();
		this->vscroll = this->GetScrollbar(WID_VT_SCROLLBAR);
		this->UpdateSelectionStates();
		this->FinishInitNested(window_number);

		this->owner = this->vehicle->owner;
	}

	~TimetableWindow()
	{
		if (!FocusWindowById(WC_VEHICLE_VIEW, this->window_number)) {
			MarkAllRouteStepsDirty(this->vehicle);
		}
	}

	/**
	 * Build the arrival-departure list for a given vehicle
	 * @param v the vehicle to make the list for
	 * @param table the table to fill
	 * @return if next arrival will be early
	 */
	static bool BuildArrivalDepartureList(const Vehicle *v, TimetableArrivalDeparture *table)
	{
		assert(HasBit(v->vehicle_flags, VF_TIMETABLE_STARTED));

		bool travelling = (!(v->current_order.IsType(OT_LOADING) || v->current_order.IsType(OT_WAITING)) || v->current_order.GetNonStopType() == ONSF_STOP_EVERYWHERE);
		Ticks start_time = GetCurrentTickCount() - v->current_order_time;

		FillTimetableArrivalDepartureTable(v, v->cur_real_order_index % v->GetNumOrders(), travelling, table, start_time);

		return (travelling && v->lateness_counter < 0);
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case WID_VT_ARRIVAL_DEPARTURE_PANEL:
				SetDParamMaxValue(0, MAX_YEAR * DAYS_IN_YEAR, 0, FS_SMALL);
				this->deparr_time_width = GetStringBoundingBox(STR_JUST_DATE_TINY).width;
				this->deparr_abbr_width = max(GetStringBoundingBox(STR_TIMETABLE_ARRIVAL_ABBREVIATION).width, GetStringBoundingBox(STR_TIMETABLE_DEPARTURE_ABBREVIATION).width);
				size->width = WD_FRAMERECT_LEFT + this->deparr_abbr_width + 10 + this->deparr_time_width + WD_FRAMERECT_RIGHT;
				FALLTHROUGH;

			case WID_VT_ARRIVAL_DEPARTURE_SELECTION:
			case WID_VT_TIMETABLE_PANEL:
				resize->height = FONT_HEIGHT_NORMAL;
				size->height = WD_FRAMERECT_TOP + 8 * resize->height + WD_FRAMERECT_BOTTOM;
				break;

			case WID_VT_SUMMARY_PANEL:
				size->height = WD_FRAMERECT_TOP + 2 * FONT_HEIGHT_NORMAL + WD_FRAMERECT_BOTTOM;
				break;
		}
	}

	int GetOrderFromTimetableWndPt(int y, const Vehicle *v)
	{
		int sel = (y - this->GetWidget<NWidgetBase>(WID_VT_TIMETABLE_PANEL)->pos_y - WD_FRAMERECT_TOP) / FONT_HEIGHT_NORMAL;

		if ((uint)sel >= this->vscroll->GetCapacity()) return INVALID_ORDER;

		sel += this->vscroll->GetPosition();

		return (sel < v->GetNumOrders() * 2 && sel >= 0) ? sel : INVALID_ORDER;
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		this->new_sep_settings = vehicle->GetTimetableSeparationSettings();

		switch (data) {
			case VIWD_AUTOREPLACE:
				/* Autoreplace replaced the vehicle */
				this->vehicle = Vehicle::Get(this->window_number);
				break;

			case VIWD_REMOVE_ALL_ORDERS:
				/* Removed / replaced all orders (after deleting / sharing) */
				if (this->sel_index == -1) break;

				this->DeleteChildWindows();
				this->sel_index = -1;
				break;

			case VIWD_MODIFY_ORDERS:
				if (!gui_scope) break;
				this->UpdateSelectionStates();
				this->ReInit();
				break;

			default: {
				if (gui_scope) break; // only do this once; from command scope

				/* Moving an order. If one of these is INVALID_VEH_ORDER_ID, then
				 * the order is being created / removed */
				if (this->sel_index == -1) break;

				VehicleOrderID from = GB(data, 0, 8);
				VehicleOrderID to   = GB(data, 8, 8);

				if (from == to) break; // no need to change anything

				/* if from == INVALID_VEH_ORDER_ID, one order was added; if to == INVALID_VEH_ORDER_ID, one order was removed */
				uint old_num_orders = this->vehicle->GetNumOrders() - (uint)(from == INVALID_VEH_ORDER_ID) + (uint)(to == INVALID_VEH_ORDER_ID);

				VehicleOrderID selected_order = (this->sel_index + 1) / 2;
				if (selected_order == old_num_orders) selected_order = 0; // when last travel time is selected, it belongs to order 0

				bool travel = HasBit(this->sel_index, 0);

				if (from != selected_order) {
					/* Moving from preceding order? */
					selected_order -= (int)(from <= selected_order);
					/* Moving to   preceding order? */
					selected_order += (int)(to   <= selected_order);
				} else {
					/* Now we are modifying the selected order */
					if (to == INVALID_VEH_ORDER_ID) {
						/* Deleting selected order */
						this->DeleteChildWindows();
						this->sel_index = -1;
						break;
					} else {
						/* Moving selected order */
						selected_order = to;
					}
				}

				/* recompute new sel_index */
				this->sel_index = 2 * selected_order - (int)travel;
				/* travel time of first order needs special handling */
				if (this->sel_index == -1) this->sel_index = this->vehicle->GetNumOrders() * 2 - 1;
				break;
			}
		}
	}


	virtual void OnPaint()
	{
		const Vehicle *v = this->vehicle;
		int selected = this->sel_index;

		this->vscroll->SetCount(v->GetNumOrders() * 2);

		if (v->owner == _local_company) {
			bool disable = IsActionDisabled(v, selected);
			bool disable_speed = disable || selected % 2 != 1 || v->type == VEH_AIRCRAFT;

			this->SetWidgetDisabledState(WID_VT_CHANGE_TIME, disable);
			this->SetWidgetDisabledState(WID_VT_CLEAR_TIME, disable);
			this->SetWidgetDisabledState(WID_VT_CHANGE_SPEED, disable_speed);
			this->SetWidgetDisabledState(WID_VT_CLEAR_SPEED, disable_speed);
			this->SetWidgetDisabledState(WID_VT_SHARED_ORDER_LIST, !v->HasSharedOrdersList());

			this->SetWidgetDisabledState(WID_VT_CONFIRM_ALL, !v->HasOrdersList());
			this->SetWidgetDisabledState(WID_VT_RESET_LATENESS, !v->HasOrdersList());
			this->SetWidgetDisabledState(WID_VT_AUTOMATE, !v->HasOrdersList());
		} else {
			this->DisableWidget(WID_VT_CONFIRM_ALL);
			this->DisableWidget(WID_VT_CHANGE_TIME);
			this->DisableWidget(WID_VT_CLEAR_TIME);
			this->DisableWidget(WID_VT_CHANGE_SPEED);
			this->DisableWidget(WID_VT_CLEAR_SPEED);
			this->DisableWidget(WID_VT_RESET_LATENESS);
			this->DisableWidget(WID_VT_AUTOMATE);
			this->DisableWidget(WID_VT_SHARED_ORDER_LIST);
		}

		/* We can only set parameters if we're in one of the manual modes. */
		bool enabled_state = (this->new_sep_settings.mode == TTS_MODE_MAN_N) || (this->new_sep_settings.mode == TTS_MODE_MAN_T);

		this->SetWidgetDisabledState(WID_VT_TTSEP_SET_PARAMETER, !enabled_state);

		this->SetWidgetLoweredState(WID_VT_AUTOMATE, HasBit(v->vehicle_flags, VF_AUTOMATE_TIMETABLE));

		this->DrawWidgets();
	}

	virtual void SetStringParameters(int widget) const
	{
		switch (widget) {
			case WID_VT_CAPTION: SetDParam(0, this->vehicle->index); break;
			case WID_VT_EXPECTED: SetDParam(0, this->show_expected ? STR_TIMETABLE_EXPECTED : STR_TIMETABLE_SCHEDULED); break;
			case WID_VT_TTSEP_MODE_DROPDOWN: SetDParam(0, TimetableSeparationDropdownOptions[this->new_sep_settings.mode]); break;
			case WID_VT_TTSEP_SET_PARAMETER: SetDParam(0, (this->new_sep_settings.mode == TTS_MODE_MAN_N) ? STR_TTSEPARATION_SET_NUM : STR_TTSEPARATION_SET_TIME); break;
		}
	}

	virtual void DrawWidget(const Rect &r, int widget) const
	{
		const Vehicle *v = this->vehicle;
		int selected = this->sel_index;

		switch (widget) {
			case WID_VT_TIMETABLE_PANEL: {
				int y = r.top + WD_FRAMERECT_TOP;
				int i = this->vscroll->GetPosition();
				VehicleOrderID order_id = (i + 1) / 2;
				bool final_order = false;

				bool rtl = _current_text_dir == TD_RTL;
				SetDParamMaxValue(0, v->GetNumOrders(), 2);
				int index_column_width = GetStringBoundingBox(STR_ORDER_INDEX).width + 2 * GetSpriteSize(rtl ? SPR_ARROW_RIGHT : SPR_ARROW_LEFT).width + 3;
				int middle = rtl ? r.right - WD_FRAMERECT_RIGHT - index_column_width : r.left + WD_FRAMERECT_LEFT + index_column_width;

				const Order *order = v->GetOrder(order_id);
				while (order != NULL) {
					/* Don't draw anything if it extends past the end of the window. */
					if (!this->vscroll->IsVisible(i)) break;

					if (i % 2 == 0) {
						DrawOrderString(v, order, order_id, y, i == selected, true, r.left + WD_FRAMERECT_LEFT, middle, r.right - WD_FRAMERECT_RIGHT);

						order_id++;

						if (order_id >= v->GetNumOrders()) {
							order = v->GetOrder(0);
							final_order = true;
						} else {
							order = order->next;
						}
					} else {
						StringID string;
						TextColour colour = (i == selected) ? TC_WHITE : TC_BLACK;
						if (order->IsType(OT_CONDITIONAL)) {
							string = STR_TIMETABLE_NO_TRAVEL;
						} else if (order->IsType(OT_IMPLICIT)) {
							string = STR_TIMETABLE_NOT_TIMETABLEABLE;
							colour = ((i == selected) ? TC_SILVER : TC_GREY) | TC_NO_SHADE;
						} else if (!order->IsTravelTimetabled()) {
							if (order->GetTravelTime() > 0) {
								SetTimetableParams(0, 1, order->GetTravelTime());
								string = order->GetMaxSpeed() != UINT16_MAX ?
										STR_TIMETABLE_TRAVEL_FOR_SPEED_ESTIMATED  :
										STR_TIMETABLE_TRAVEL_FOR_ESTIMATED;
							} else {
								string = order->GetMaxSpeed() != UINT16_MAX ?
										STR_TIMETABLE_TRAVEL_NOT_TIMETABLED_SPEED :
										STR_TIMETABLE_TRAVEL_NOT_TIMETABLED;
							}
						} else {
							SetTimetableParams(0, 1, order->GetTimetabledTravel());
							string = order->GetMaxSpeed() != UINT16_MAX ?
									STR_TIMETABLE_TRAVEL_FOR_SPEED : STR_TIMETABLE_TRAVEL_FOR;
						}
						SetDParam(2, order->GetMaxSpeed());

						DrawString(rtl ? r.left + WD_FRAMERECT_LEFT : middle, rtl ? middle : r.right - WD_FRAMERECT_LEFT, y, string, colour);

						if (final_order) break;
					}

					i++;
					y += FONT_HEIGHT_NORMAL;
				}
				break;
			}

			case WID_VT_ARRIVAL_DEPARTURE_PANEL: {
				/* Arrival and departure times are handled in an all-or-nothing approach,
				 * i.e. are only shown if we can calculate all times.
				 * Excluding order lists with only one order makes some things easier.
				 */
				Ticks total_time = v->GetTimetableDurationIncomplete();
				if (total_time <= 0 || v->GetNumOrders() <= 1 || !HasBit(v->vehicle_flags, VF_TIMETABLE_STARTED)) break;

				TimetableArrivalDeparture *arr_dep = AllocaM(TimetableArrivalDeparture, v->GetNumOrders());
				const VehicleOrderID cur_order = v->cur_real_order_index % v->GetNumOrders();

				VehicleOrderID earlyID = BuildArrivalDepartureList(v, arr_dep) ? cur_order : (VehicleOrderID)INVALID_VEH_ORDER_ID;

				int y = r.top + WD_FRAMERECT_TOP;

				bool show_late = this->show_expected && v->lateness_counter > TICKS_PER_MINUTE;
				Ticks offset = show_late ? 0 : -v->lateness_counter;

				bool rtl = _current_text_dir == TD_RTL;
				int abbr_left = rtl ? r.right - WD_FRAMERECT_RIGHT - this->deparr_abbr_width : r.left + WD_FRAMERECT_LEFT;
				int abbr_right = rtl ? r.right - WD_FRAMERECT_RIGHT : r.left + WD_FRAMERECT_LEFT + this->deparr_abbr_width;
				int time_left = rtl ? r.left + WD_FRAMERECT_LEFT : r.right - WD_FRAMERECT_RIGHT - this->deparr_time_width;
				int time_right = rtl ? r.left + WD_FRAMERECT_LEFT + this->deparr_time_width : r.right - WD_FRAMERECT_RIGHT;

				for (int i = this->vscroll->GetPosition(); i / 2 < v->GetNumOrders(); ++i) { // note: i is also incremented in the loop
					/* Don't draw anything if it extends past the end of the window. */
					if (!this->vscroll->IsVisible(i)) break;

					if (i % 2 == 0) {
						if (arr_dep[i / 2].arrival != INVALID_TICKS) {
							DrawString(abbr_left, abbr_right, y, STR_TIMETABLE_ARRIVAL_ABBREVIATION, i == selected ? TC_WHITE : TC_BLACK);
							if (this->show_expected && i / 2 == earlyID) {
								SetDParam(0, arr_dep[i / 2].arrival);
								DrawString(time_left, time_right, y, STR_JUST_TIME_TINY, TC_GREEN);
							} else {
								SetDParam(0, arr_dep[i / 2].arrival + offset);
								DrawString(time_left, time_right, y, STR_JUST_TIME_TINY, show_late ? TC_RED : i == selected ? TC_WHITE : TC_BLACK);
							}
						}
					}
					else {
						if (arr_dep[i / 2].departure != INVALID_TICKS) {
							DrawString(abbr_left, abbr_right, y, STR_TIMETABLE_DEPARTURE_ABBREVIATION, i == selected ? TC_WHITE : TC_BLACK);
							SetDParam(0, arr_dep[i / 2].departure + offset);
							DrawString(time_left, time_right, y, STR_JUST_TIME_TINY,
								show_late ? TC_RED : i == selected ? TC_WHITE : TC_BLACK);
						}
					}
					y += FONT_HEIGHT_NORMAL;
				}
				break;
			}

			case WID_VT_SUMMARY_PANEL: {
				int y = r.top + WD_FRAMERECT_TOP;

				Ticks total_time = v->GetTimetableDurationIncomplete();
				if (total_time != 0) {
					SetTimetableParams(0, 1, total_time);
					DrawString(r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y, v->HasCompleteTimetable() ? STR_TIMETABLE_TOTAL_TIME : STR_TIMETABLE_TOTAL_TIME_INCOMPLETE);
				}
				y += FONT_HEIGHT_NORMAL;

				if (v->timetable_start != 0) {
					/* We are running towards the first station so we can start the
					 * timetable at the given time. */
					SetDParam(0, STR_JUST_DATE_TINY);
					SetDParam(1, v->timetable_start);
					DrawString(r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y, STR_TIMETABLE_STATUS_START_AT);
				}
				else if (!HasBit(v->vehicle_flags, VF_TIMETABLE_STARTED)) {
					/* We aren't running on a timetable yet, so how can we be "on time"
					 * when we aren't even "on service"/"on duty"? */
					DrawString(r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y, STR_TIMETABLE_STATUS_NOT_STARTED);
				}
				else if (v->lateness_counter == 0) {
					DrawString(r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y, STR_TIMETABLE_STATUS_ON_TIME);
				}
				else {
					SetTimetableParams(0, 1, abs(v->lateness_counter));
					DrawString(r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y, v->lateness_counter < 0 ? STR_TIMETABLE_STATUS_EARLY : STR_TIMETABLE_STATUS_LATE);
				}
				break;
			}

			case WID_VT_TTSEP_PANEL_TEXT: {
				int       y = r.top + WD_FRAMERECT_TOP;     // Represents the current vertical position
				const int left_border = r.left + WD_FRAMERECT_LEFT;   // Represents the left border of the separation display frame
				const int right_border = r.right - WD_FRAMERECT_RIGHT; // Represents the right border of the separation display frame.

				/* If separation is inactive, we can stop here. */
				if (!_settings_game.order.automatic_timetable_separation || !this->vehicle->HasOrdersList())
					break;

				/* If the new mode is OFF... */
				if (this->new_sep_settings.mode == TTS_MODE_OFF) {
					/* ... skip description lines. */
					int offset = GetStringBoundingBox(STR_TTSEPARATION_REQ_TIME_DESC_TICKS).height;

					y = y + GetStringBoundingBox(STR_TTSEPARATION_REQ_NUM_DESC).height + offset;

				}
				else {
					uint64 par;

					// If separation hasn't just been switched off, we need to draw various description lines.
					// The first line is the amount of separation which is either saved in the struct or must
					// be calculated on the fly.
					if (this->new_sep_settings.mode == TTS_MODE_MAN_T ||
						this->new_sep_settings.mode == TTS_MODE_AUTO ||
						this->new_sep_settings.mode == TTS_MODE_BUFFERED_AUTO) {
						par = this->new_sep_settings.sep_ticks;
					}
					else {
						par = this->vehicle->GetTimetableTotalDuration() / max(1u, this->new_sep_settings.num_veh);
					}

					if (this->new_sep_settings.mode == TTS_MODE_MAN_T || 
						(this->vehicle->HasCompleteTimetable() && this->vehicle->IsTimetableSeparationValid())) {
						
						SetDParam(0, par);
						DrawString(left_border, right_border, y, STR_TTSEPARATION_REQ_TIME_DESC_TICKS, TC_BLACK);

						y += GetStringBoundingBox(STR_TTSEPARATION_REQ_TIME_DESC_TICKS).height;
					}
					else {
						y += GetStringBoundingBox(STR_TTSEPARATION_REQ_TIME_DESC_TICKS).height;
					}

					// Print either the chosen amount of vehicles (when in MAN_N mode) or the calculated result...
					// Do not print anything at all if we do not have a valid timetable yet.					
					if (this->new_sep_settings.mode == TTS_MODE_MAN_N ||
						this->new_sep_settings.mode == TTS_MODE_AUTO ||
						this->new_sep_settings.mode == TTS_MODE_BUFFERED_AUTO) {
						par = this->new_sep_settings.num_veh;
					}
					else {
						par = this->vehicle->GetTimetableTotalDuration() / max(1u, this->new_sep_settings.sep_ticks);
					}

					if (this->new_sep_settings.mode == TTS_MODE_MAN_N ||
						(this->vehicle->HasCompleteTimetable() && this->vehicle->IsTimetableSeparationValid())) {
						SetDParam(0, par);
						DrawString(left_border, right_border, y, STR_TTSEPARATION_REQ_NUM_DESC, TC_BLACK);
					}

					y += GetStringBoundingBox(STR_TTSEPARATION_REQ_NUM_DESC).height;
				}

				/* If separation is switched on at all... */				
				if (this->vehicle->IsTimetableSeparationOn()) {
					if (!this->vehicle->HasCompleteTimetable()) {
						SetDParam(0, STR_TTSEPARATION_STATUS_WAITING_FOR_TIMETABLE);
					}
					else {
						/* ... set displayed status to either "Running" or "Initializing" */
						SetDParam(0, (this->vehicle->IsTimetableSeparationValid()) ? STR_TTSEPARATION_STATUS_RUNNING : STR_TTSEPARATION_STATUS_INIT);
					}
				}
				else {
					/* If separation is switched off, show this instead. */
					SetDParam(0, STR_TTSEPARATION_STATUS_OFF);
				}

				/* Print status description. */
				DrawStringMultiLine(left_border, right_border, y, r.bottom - WD_FRAMERECT_BOTTOM, STR_TTSEPARATION_STATUS_DESC);
			}
		}
	}

	static inline uint32 PackTimetableArgs(const Vehicle *v, uint selected, bool speed)
	{
		uint order_number = (selected + 1) / 2;
		ModifyTimetableFlags mtf = (selected % 2 == 1) ? (speed ? MTF_TRAVEL_SPEED : MTF_TRAVEL_TIME) : MTF_WAIT_TIME;

		if (order_number >= v->GetNumOrders()) order_number = 0;

		return v->index | (order_number << 20) | (mtf << 28);
	}

	static inline bool IsActionDisabled(const Vehicle *v, int selected)
	{
		bool disabled = true;
		if (selected != -1) {
			const Order *order = v->GetOrder(((selected + 1) / 2) % v->GetNumOrders());
			if (selected % 2 == 1) {
				disabled = order != NULL && (order->IsType(OT_CONDITIONAL) || order->IsType(OT_IMPLICIT));
			}
			else {
				disabled = order == NULL || ((!(order->IsType(OT_GOTO_STATION) || (order->IsType(OT_GOTO_DEPOT) && !(order->GetDepotActionType() & ODATFB_HALT))) || (order->GetNonStopType() & ONSF_NO_STOP_AT_DESTINATION_STATION)) && !order->IsType(OT_CONDITIONAL));
			}
		}
		return disabled;
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		const Vehicle *v = this->vehicle;

		this->DeleteChildWindows(WC_QUERY_STRING);

		switch (widget) {
			case WID_VT_ORDER_VIEW: // Order view button
				ShowOrdersWindow(v);
				break;

			case WID_VT_TIMETABLE_PANEL: { // Main panel.
				int selected = GetOrderFromTimetableWndPt(pt.y, v);
 
				/* Allow change time by double-clicking order */
				if (click_count == 2) {
					this->sel_index = selected == INVALID_ORDER ? -1 : selected;
					this->OnClick(pt, WID_VT_CHANGE_TIME, click_count);
					return;
				} else {
					this->sel_index = (selected == INVALID_ORDER || selected == this->sel_index) ? -1 : selected;
				}

				this->DeleteChildWindows();
				break;
			}

			case WID_VT_CONFIRM_ALL: { // Confirm all estimated times as timetabled.
				DoCommandP(0, v->index, 0, CMD_CONFIRM_ALL | CMD_MSG(STR_ERROR_CAN_T_TIMETABLE_VEHICLE));
				break;
			}

			case WID_VT_CHANGE_TIME: { // "Wait For" button.
				int selected = this->sel_index;
				VehicleOrderID real = (selected + 1) / 2;

				if (real >= v->GetNumOrders()) real = 0;

				const Order *order = v->GetOrder(real);
				StringID current = STR_EMPTY;

				if (order != NULL) {
					uint time = (selected % 2 == 1) ? order->GetTravelTime() : order->GetWaitTime();

					if (time != 0) {
						SetDParam(0, time);
						current = STR_JUST_INT;
					}
				}

				this->query_widget = WID_VT_CHANGE_TIME;
				this->query_is_speed_query = false;
				this->query_is_bulk_query = _ctrl_pressed;
				ShowQueryString(current, STR_TIMETABLE_CHANGE_TIME, 31, this, CS_NUMERAL, QSF_ACCEPT_UNCHANGED);
				break;
			}

			case WID_VT_CHANGE_SPEED: { // Change max speed button.
				int selected = this->sel_index;
				VehicleOrderID real = (selected + 1) / 2;

				if (real >= v->GetNumOrders()) real = 0;

				StringID current = STR_EMPTY;
				const Order *order = v->GetOrder(real);
				if (order != NULL) {
					if (order->GetMaxSpeed() != UINT16_MAX) {
						SetDParam(0, ConvertKmhishSpeedToDisplaySpeed(order->GetMaxSpeed()));
						current = STR_JUST_INT;
					}
				}

				this->query_widget = WID_VT_CHANGE_SPEED;
				this->query_is_speed_query = true;
				this->query_is_bulk_query = _ctrl_pressed;
				ShowQueryString(current, STR_TIMETABLE_CHANGE_SPEED, 31, this, CS_NUMERAL, QSF_NONE);
				break;
			}

			case WID_VT_CLEAR_TIME: { // Clear waiting time.
				uint32 p1 = PackTimetableArgs(v, this->sel_index, false);
				DoCommandP(0, p1, 0, (_ctrl_pressed ? CMD_BULK_CHANGE_TIMETABLE : CMD_CHANGE_TIMETABLE) | CMD_MSG(STR_ERROR_CAN_T_TIMETABLE_VEHICLE));
				break;
			}

			case WID_VT_CLEAR_SPEED: { // Clear max speed button.
				uint32 p1 = PackTimetableArgs(v, this->sel_index, true);
				DoCommandP(0, p1, UINT16_MAX, (_ctrl_pressed ? CMD_BULK_CHANGE_TIMETABLE : CMD_CHANGE_TIMETABLE) | CMD_MSG(STR_ERROR_CAN_T_TIMETABLE_VEHICLE));
				break;
			}

			case WID_VT_RESET_LATENESS: // Reset the vehicle's late counter.
				DoCommandP(0, v->index, 0, CMD_SET_VEHICLE_ON_TIME | CMD_MSG(STR_ERROR_CAN_T_TIMETABLE_VEHICLE));
				break;

			case WID_VT_AUTOMATE: { // Automate the timetable.
				uint32 p2 = 0;
				if (!HasBit(v->vehicle_flags, VF_AUTOMATE_TIMETABLE)) SetBit(p2, 0);
				if (_ctrl_pressed) SetBit(p2, 1);
				DoCommandP(0, v->index, p2, CMD_AUTOMATE_TIMETABLE | CMD_MSG(STR_ERROR_CAN_T_TIMETABLE_VEHICLE));
				break;
			}

			case WID_VT_EXPECTED:
				this->show_expected = !this->show_expected;
				break;

			case WID_VT_SHARED_ORDER_LIST:
				ShowVehicleListWindow(v);
				break;

			case WID_VT_TTSEP_MODE_DROPDOWN: {
				ShowDropDownMenu(this, TimetableSeparationDropdownOptions, this->new_sep_settings.mode, WID_VT_TTSEP_MODE_DROPDOWN, 0, 0);
				break;
			}

			case WID_VT_TTSEP_SET_PARAMETER: {
				this->query_widget = WID_VT_TTSEP_SET_PARAMETER;
				SetDParam(0, (this->new_sep_settings.mode == TTS_MODE_MAN_N) ? this->new_sep_settings.num_veh : this->new_sep_settings.sep_ticks);
				ShowQueryString(STR_JUST_INT, STR_TIMETABLE_CHANGE_TIME, 31, this, CS_NUMERAL, QSF_NONE);
				break;
			}
		}

		this->SetDirty();
	}

	virtual void OnDropdownSelect(int widget, int index)
	{
		assert(widget == WID_VT_TTSEP_MODE_DROPDOWN);

		this->new_sep_settings = this->vehicle->GetTimetableSeparationSettings();
		this->new_sep_settings.mode = (TTSepMode)index;
		this->vehicle->SetTimetableSeparationSettings(this->new_sep_settings);
		this->InvalidateData();
	}


	virtual void OnQueryTextFinished(char *str)
	{
		if (str == NULL || StrEmpty(str))
			return;

		switch (this->query_widget) {
		case WID_VT_CHANGE_TIME:
		case WID_VT_CHANGE_SPEED: {
			const Vehicle *v = this->vehicle;

			uint32 p1 = PackTimetableArgs(v, this->sel_index, this->query_is_speed_query);

			uint64 val = StrEmpty(str) ? 0 : strtoul(str, NULL, 10);
			if (this->query_is_speed_query) {
				val = ConvertDisplaySpeedToKmhishSpeed(val);
			}

			uint32 p2 = minu(val, UINT16_MAX);

			DoCommandP(0, p1, p2, (this->query_is_bulk_query ? CMD_BULK_CHANGE_TIMETABLE : CMD_CHANGE_TIMETABLE) | CMD_MSG(STR_ERROR_CAN_T_TIMETABLE_VEHICLE));
			break;
		}
		case WID_VT_TTSEP_SET_PARAMETER: {
			int value = atoi(str);

			switch (this->new_sep_settings.mode)
			{
			case TTS_MODE_AUTO:
			case TTS_MODE_BUFFERED_AUTO:
			case TTS_MODE_OFF:
				break;

			case TTS_MODE_MAN_N:
				this->new_sep_settings.num_veh = Clamp(value, 1, 65535);
				break;

			case TTS_MODE_MAN_T:
				this->new_sep_settings.sep_ticks = Clamp(value, 1, 65535);
				break;

			default:
				NOT_REACHED();
				break;
			}

			this->vehicle->SetTimetableSeparationSettings(this->new_sep_settings);
			this->InvalidateData();
			break;
		}
		default:
			NOT_REACHED();
			break;
		}
	}

	virtual void OnResize()
	{
		/* Update the scroll bar */
		this->vscroll->SetCapacityFromWidget(this, WID_VT_TIMETABLE_PANEL, WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM);
	}

	/**
	 * Update the selection state of the arrival/departure data
	 */
	void UpdateSelectionStates()
	{
		this->GetWidget<NWidgetStacked>(WID_VT_ARRIVAL_DEPARTURE_SELECTION)->SetDisplayedPlane(_settings_client.gui.timetable_arrival_departure ? 0 : SZSP_NONE);
		this->GetWidget<NWidgetStacked>(WID_VT_EXPECTED_SELECTION)->SetDisplayedPlane(_settings_client.gui.timetable_arrival_departure ? 0 : 1);
	}

	virtual void OnFocus(Window *previously_focused_window)
	{
		if (HasFocusedVehicleChanged(this->window_number, previously_focused_window)) {
			MarkAllRoutePathsDirty(this->vehicle);
			MarkAllRouteStepsDirty(this->vehicle);
		}
	}

	virtual void OnFocusLost(Window *newly_focused_window)
	{
		if (HasFocusedVehicleChanged(this->window_number, newly_focused_window)) {
			MarkAllRoutePathsDirty(this->vehicle);
			MarkAllRouteStepsDirty(this->vehicle);
		}
	}

	const Vehicle *GetVehicle()
	{
		return this->vehicle;
	}
};

static const NWidgetPart _nested_timetable_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, WID_VT_CAPTION), SetDataTip(STR_TIMETABLE_TITLE, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VT_ORDER_VIEW), SetMinimalSize(61, 14), SetDataTip( STR_TIMETABLE_ORDER_VIEW, STR_TIMETABLE_ORDER_VIEW_TOOLTIP),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, COLOUR_GREY, WID_VT_TIMETABLE_PANEL), SetMinimalSize(388, 82), SetResize(1, 10), SetDataTip(STR_NULL, STR_TIMETABLE_TOOLTIP), SetScrollbar(WID_VT_SCROLLBAR), EndContainer(),
		NWidget(NWID_SELECTION, INVALID_COLOUR, WID_VT_ARRIVAL_DEPARTURE_SELECTION),
			NWidget(WWT_PANEL, COLOUR_GREY, WID_VT_ARRIVAL_DEPARTURE_PANEL), SetMinimalSize(80, 0), SetFill(0, 1), SetDataTip(STR_NULL, STR_TIMETABLE_TOOLTIP), SetScrollbar(WID_VT_SCROLLBAR), EndContainer(),
		EndContainer(),
		NWidget(NWID_VSCROLLBAR, COLOUR_GREY, WID_VT_SCROLLBAR),
		NWidget(WWT_PANEL, COLOUR_GREY),
			NWidget(WWT_FRAME, COLOUR_GREY), SetDataTip(STR_TTSEPARATION_SETTINGS_DESC, STR_NULL), SetPadding(3),
				NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_VT_TTSEP_MODE_DROPDOWN), SetDataTip(STR_JUST_STRING, STR_TIMETABLE_TOOLTIP),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VT_TTSEP_SET_PARAMETER), SetFill(1, 0), SetDataTip(STR_TTSEPARATION_SET_XX, STR_TIMETABLE_TOOLTIP),
				NWidget(WWT_PANEL, COLOUR_GREY, WID_VT_TTSEP_PANEL_TEXT), SetFill(1, 1), SetResize(0, 1), SetMinimalSize(180, 44), EndContainer(),
			EndContainer(),
		EndContainer(),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_GREY, WID_VT_SUMMARY_PANEL), SetMinimalSize(400, 22), SetResize(1, 0), EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_HORIZONTAL, NC_EQUALSIZE),
			NWidget(NWID_VERTICAL, NC_EQUALSIZE),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VT_CHANGE_TIME), SetResize(1, 0), SetFill(1, 1), SetDataTip(STR_TIMETABLE_CHANGE_TIME, STR_TIMETABLE_WAIT_TIME_TOOLTIP),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VT_CLEAR_TIME), SetResize(1, 0), SetFill(1, 1), SetDataTip(STR_TIMETABLE_CLEAR_TIME, STR_TIMETABLE_CLEAR_TIME_TOOLTIP),
			EndContainer(),
			NWidget(NWID_VERTICAL, NC_EQUALSIZE),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VT_CHANGE_SPEED), SetResize(1, 0), SetFill(1, 1), SetDataTip(STR_TIMETABLE_CHANGE_SPEED, STR_TIMETABLE_CHANGE_SPEED_TOOLTIP),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VT_CLEAR_SPEED), SetResize(1, 0), SetFill(1, 1), SetDataTip(STR_TIMETABLE_CLEAR_SPEED, STR_TIMETABLE_CLEAR_SPEED_TOOLTIP),
			EndContainer(),
			NWidget(NWID_VERTICAL, NC_EQUALSIZE),
			NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VT_CONFIRM_ALL), SetResize(1, 0), SetFill(1, 1), SetDataTip(STR_TIMETABLE_CONFIRM_ALL, STR_TIMETABLE_CONFIRM_ALL_TOOLTIP),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VT_RESET_LATENESS), SetResize(1, 0), SetFill(1, 1), SetDataTip(STR_TIMETABLE_RESET_LATENESS, STR_TIMETABLE_RESET_LATENESS_TOOLTIP),
			EndContainer(),
			NWidget(NWID_VERTICAL, NC_EQUALSIZE),
			NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VT_AUTOMATE), SetResize(1, 0), SetFill(1, 1), SetDataTip(STR_TIMETABLE_AUTOMATE, STR_TIMETABLE_AUTOMATE_TOOLTIP),
				NWidget(NWID_SELECTION, INVALID_COLOUR, WID_VT_EXPECTED_SELECTION),
					NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_VT_EXPECTED), SetResize(1, 0), SetFill(1, 1), SetDataTip(STR_BLACK_STRING, STR_TIMETABLE_EXPECTED_TOOLTIP),
					NWidget(WWT_PANEL, COLOUR_GREY), SetResize(1, 0), SetFill(1, 1), EndContainer(),
				EndContainer(),
			EndContainer(),
		EndContainer(),
		NWidget(NWID_VERTICAL, NC_EQUALSIZE),
			NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_VT_SHARED_ORDER_LIST), SetFill(0, 1), SetDataTip(SPR_SHARED_ORDERS_ICON, STR_ORDERS_VEH_WITH_SHARED_ORDERS_LIST_TOOLTIP),
			NWidget(WWT_RESIZEBOX, COLOUR_GREY), SetFill(0, 1),
		EndContainer(),
	EndContainer(),
};

static WindowDesc _timetable_desc(
	WDP_AUTO, "view_vehicle_timetable", 400, 130,
	WC_VEHICLE_TIMETABLE, WC_VEHICLE_VIEW,
	WDF_CONSTRUCTION,
	_nested_timetable_widgets, lengthof(_nested_timetable_widgets)
);

/**
 * Show the timetable for a given vehicle.
 * @param v The vehicle to show the timetable for.
 */
void ShowTimetableWindow(const Vehicle *v)
{
	DeleteWindowById(WC_VEHICLE_DETAILS, v->index, false);
	DeleteWindowById(WC_VEHICLE_ORDERS, v->index, false);
	AllocateWindowDescFront<TimetableWindow>(&_timetable_desc, v->index);
}

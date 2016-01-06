/* $Id: build_vehicle_gui.cpp 23792 2012-01-12 19:23:00Z yexo $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file build_vehicle_gui.cpp GUI for building vehicles. */

#include "stdafx.h"
#include "engine_base.h"
#include "engine_func.h"
#include "station_base.h"
#include "network/network.h"
#include "articulated_vehicles.h"
#include "textbuf_gui.h"
#include "command_func.h"
#include "company_func.h"
#include "vehicle_gui.h"
#include "newgrf_engine.h"
#include "newgrf_text.h"
#include "group.h"
#include "string_func.h"
#include "strings_func.h"
#include "window_func.h"
#include "date_func.h"
#include "vehicle_func.h"
#include "widgets/dropdown_func.h"
#include "engine_gui.h"
#include "cargotype.h"
#include "core/geometry_func.hpp"
#include "autoreplace_func.h"

#include "widgets/build_vehicle_widget.h"

#include "table/strings.h"

#include "tbtr_template_gui_create_virtualtrain.h"

#include "vehicle_gui.h"

static const NWidgetPart _nested_build_vehicle_widgets_train_advanced[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, WID_BV_CAPTION), SetDataTip(STR_WHITE_STRING, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),

	NWidget(NWID_HORIZONTAL),
		/* First half of the window contains locomotives. */
		NWidget(NWID_VERTICAL),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PANEL, COLOUR_GREY), SetFill(1, 0),
					NWidget(WWT_LABEL, COLOUR_GREY, WID_BV_CAPTION_LOCO), SetDataTip(STR_WHITE_STRING, STR_NULL), SetResize(1, 0), SetFill(1, 0),
				EndContainer(),
			EndContainer(),
			NWidget(WWT_PANEL, COLOUR_GREY),
				NWidget(NWID_VERTICAL),
					NWidget(NWID_HORIZONTAL),
						NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_BV_SORT_ASSENDING_DESCENDING_LOCO), SetDataTip(STR_BUTTON_SORT_BY, STR_TOOLTIP_SORT_ORDER), SetFill(1, 0),
						NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_BV_SORT_DROPDOWN_LOCO), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_JUST_STRING, STR_TOOLTIP_SORT_CRITERIA),
					EndContainer(),
					NWidget(NWID_HORIZONTAL),
						NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BV_SHOW_HIDDEN_LOCOS),
						NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_BV_CARGO_FILTER_DROPDOWN_LOCO), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_JUST_STRING, STR_TOOLTIP_FILTER_CRITERIA),
					EndContainer(),
				EndContainer(),
			EndContainer(),
			/* Vehicle list for locomotives. */
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_MATRIX, COLOUR_GREY, WID_BV_LIST_LOCO), SetResize(1, 1), SetFill(1, 0), SetMatrixDataTip(1, 0, STR_NULL), SetScrollbar(WID_BV_SCROLLBAR_LOCO),
				NWidget(NWID_VSCROLLBAR, COLOUR_GREY, WID_BV_SCROLLBAR_LOCO),
			EndContainer(),
			/* Panel with details for locomotives. */
			NWidget(WWT_PANEL, COLOUR_GREY, WID_BV_PANEL_LOCO), SetMinimalSize(240, 122), SetResize(1, 0), EndContainer(),
			/* Build/rename buttons, resize button for locomotives. */
			NWidget(NWID_HORIZONTAL),
				NWidget(NWID_SELECTION, INVALID_COLOUR, WID_BV_BUILD_SEL_LOCO),
					NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_BV_BUILD_LOCO), SetResize(1, 0), SetFill(1, 0),
				EndContainer(),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_BV_SHOW_HIDE_LOCO), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_JUST_STRING, STR_NULL),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_BV_RENAME_LOCO), SetResize(1, 0), SetFill(1, 0),
			EndContainer(),

		EndContainer(),
		/* Second half of the window contains wagons. */
		NWidget(NWID_VERTICAL),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PANEL, COLOUR_GREY), SetFill(1, 0),
					NWidget(WWT_LABEL, COLOUR_GREY, WID_BV_CAPTION_WAGON), SetDataTip(STR_WHITE_STRING, STR_NULL), SetResize(1, 0), SetFill(1, 0),
				EndContainer(),
			EndContainer(),
			NWidget(WWT_PANEL, COLOUR_GREY),
				NWidget(NWID_VERTICAL),
					NWidget(NWID_HORIZONTAL),
						NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_BV_SORT_ASSENDING_DESCENDING_WAGON), SetDataTip(STR_BUTTON_SORT_BY, STR_TOOLTIP_SORT_ORDER), SetFill(1, 0),
						NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_BV_SORT_DROPDOWN_WAGON), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_JUST_STRING, STR_TOOLTIP_SORT_CRITERIA),
					EndContainer(),
					NWidget(NWID_HORIZONTAL),
						NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_BV_SHOW_HIDDEN_WAGONS),
						NWidget(WWT_DROPDOWN, COLOUR_GREY, WID_BV_CARGO_FILTER_DROPDOWN_WAGON), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_JUST_STRING, STR_TOOLTIP_FILTER_CRITERIA),
					EndContainer(),
				EndContainer(),
			EndContainer(),
			/* Vehicle list for wagons. */
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_MATRIX, COLOUR_GREY, WID_BV_LIST_WAGON), SetResize(1, 1), SetFill(1, 0), SetMatrixDataTip(1, 0, STR_NULL), SetScrollbar(WID_BV_SCROLLBAR_WAGON),
				NWidget(NWID_VSCROLLBAR, COLOUR_GREY, WID_BV_SCROLLBAR_WAGON),
			EndContainer(),
			/* Panel with details for wagons. */
			NWidget(WWT_PANEL, COLOUR_GREY, WID_BV_PANEL_WAGON), SetMinimalSize(240, 122), SetResize(1, 0), EndContainer(),
			/* Build/rename buttons, resize button for wagons. */
			NWidget(NWID_HORIZONTAL),
				NWidget(NWID_SELECTION, INVALID_COLOUR, WID_BV_BUILD_SEL_WAGON),
					NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_BV_BUILD_WAGON), SetResize(1, 0), SetFill(1, 0),
				EndContainer(),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_BV_SHOW_HIDE_WAGON), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_JUST_STRING, STR_NULL),
				NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_BV_RENAME_WAGON), SetResize(1, 0), SetFill(1, 0),
				NWidget(WWT_RESIZEBOX, COLOUR_GREY),
			EndContainer(),
		EndContainer(),
		EndContainer(),
};

/** Special cargo filter criteria */
static const CargoID CF_ANY  = CT_NO_REFIT; ///< Show all vehicles independent of carried cargo (i.e. no filtering)
static const CargoID CF_NONE = CT_INVALID;  ///< Show only vehicles which do not carry cargo (e.g. train engines)

static bool _internal_sort_order;           ///< false = descending, true = ascending
static byte _last_sort_criteria[]      = {0, 0, 0, 0};
static bool _last_sort_order[]         = {false, false, false, false};
static CargoID _last_filter_criteria[] = {CF_ANY, CF_ANY, CF_ANY, CF_ANY};

static bool _internal_sort_order_loco;           ///< false = descending, true = ascending
static byte _last_sort_criteria_loco      = 0;
static bool _last_sort_order_loco         = false;
static CargoID _last_filter_criteria_loco = CF_ANY;

static bool _internal_sort_order_wagon;           ///< false = descending, true = ascending
static byte _last_sort_criteria_wagon      = 0;
static bool _last_sort_order_wagon         = false;
static CargoID _last_filter_criteria_wagon = CF_ANY;

/**
 * Determines order of engines by engineID
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineNumberSorter(const EngineID *a, const EngineID *b)
{
	int r = Engine::Get(*a)->list_position - Engine::Get(*b)->list_position;

	return _internal_sort_order ? -r : r;
}

/**
 * Determines order of engines by introduction date
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineIntroDateSorter(const EngineID *a, const EngineID *b)
{
	const int va = Engine::Get(*a)->intro_date;
	const int vb = Engine::Get(*b)->intro_date;
	const int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/**
 * Determines order of engines by name
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineNameSorter(const EngineID *a, const EngineID *b)
{
	static EngineID last_engine[2] = { INVALID_ENGINE, INVALID_ENGINE };
	static char     last_name[2][64] = { "\0", "\0" };

	const EngineID va = *a;
	const EngineID vb = *b;

	if (va != last_engine[0]) {
		last_engine[0] = va;
		SetDParam(0, va);
		GetString(last_name[0], STR_ENGINE_NAME, lastof(last_name[0]));
	}

	if (vb != last_engine[1]) {
		last_engine[1] = vb;
		SetDParam(0, vb);
		GetString(last_name[1], STR_ENGINE_NAME, lastof(last_name[1]));
	}

	int r = strnatcmp(last_name[0], last_name[1]); // Sort by name (natural sorting).

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/**
 * Determines order of engines by reliability
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineReliabilitySorter(const EngineID *a, const EngineID *b)
{
	const int va = Engine::Get(*a)->reliability;
	const int vb = Engine::Get(*b)->reliability;
	const int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/**
 * Determines order of engines by purchase cost
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineCostSorter(const EngineID *a, const EngineID *b)
{
	Money va = Engine::Get(*a)->GetCost();
	Money vb = Engine::Get(*b)->GetCost();
	int r = ClampToI32(va - vb);

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/**
 * Determines order of engines by speed
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineSpeedSorter(const EngineID *a, const EngineID *b)
{
	int va = Engine::Get(*a)->GetDisplayMaxSpeed();
	int vb = Engine::Get(*b)->GetDisplayMaxSpeed();
	int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/**
 * Determines order of engines by power
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EnginePowerSorter(const EngineID *a, const EngineID *b)
{
	int va = Engine::Get(*a)->GetPower();
	int vb = Engine::Get(*b)->GetPower();
	int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/**
 * Determines order of engines by tractive effort
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineTractiveEffortSorter(const EngineID *a, const EngineID *b)
{
	int va = Engine::Get(*a)->GetDisplayMaxTractiveEffort();
	int vb = Engine::Get(*b)->GetDisplayMaxTractiveEffort();
	int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/**
 * Determines order of engines by running costs
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineRunningCostSorter(const EngineID *a, const EngineID *b)
{
	Money va = Engine::Get(*a)->GetRunningCost();
	Money vb = Engine::Get(*b)->GetRunningCost();
	int r = ClampToI32(va - vb);

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/**
 * Determines order of engines by running costs
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EnginePowerVsRunningCostSorter(const EngineID *a, const EngineID *b)
{
	const Engine *e_a = Engine::Get(*a);
	const Engine *e_b = Engine::Get(*b);

	/* Here we are using a few tricks to get the right sort.
	 * We want power/running cost, but since we usually got higher running cost than power and we store the result in an int,
	 * we will actually calculate cunning cost/power (to make it more than 1).
	 * Because of this, the return value have to be reversed as well and we return b - a instead of a - b.
	 * Another thing is that both power and running costs should be doubled for multiheaded engines.
	 * Since it would be multiplying with 2 in both numerator and denominator, it will even themselves out and we skip checking for multiheaded. */
	Money va = (e_a->GetRunningCost()) / max(1U, (uint)e_a->GetPower());
	Money vb = (e_b->GetRunningCost()) / max(1U, (uint)e_b->GetPower());
	int r = ClampToI32(vb - va);

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/* Train sorting functions */

/**
 * Determines order of train engines by capacity
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL TrainEngineCapacitySorter(const EngineID *a, const EngineID *b)
{
	const RailVehicleInfo *rvi_a = RailVehInfo(*a);
	const RailVehicleInfo *rvi_b = RailVehInfo(*b);

	int va = GetTotalCapacityOfArticulatedParts(*a) * (rvi_a->railveh_type == RAILVEH_MULTIHEAD ? 2 : 1);
	int vb = GetTotalCapacityOfArticulatedParts(*b) * (rvi_b->railveh_type == RAILVEH_MULTIHEAD ? 2 : 1);
	int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/**
 * Determines order of train engines by engine / wagon
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL TrainEnginesThenWagonsSorter(const EngineID *a, const EngineID *b)
{
	int val_a = (RailVehInfo(*a)->railveh_type == RAILVEH_WAGON ? 1 : 0);
	int val_b = (RailVehInfo(*b)->railveh_type == RAILVEH_WAGON ? 1 : 0);
	int r = val_a - val_b;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/* Road vehicle sorting functions */

/**
 * Determines order of road vehicles by capacity
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL RoadVehEngineCapacitySorter(const EngineID *a, const EngineID *b)
{
	int va = GetTotalCapacityOfArticulatedParts(*a);
	int vb = GetTotalCapacityOfArticulatedParts(*b);
	int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/* Ship vehicle sorting functions */

/**
 * Determines order of ships by capacity
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL ShipEngineCapacitySorter(const EngineID *a, const EngineID *b)
{
	const Engine *e_a = Engine::Get(*a);
	const Engine *e_b = Engine::Get(*b);

	int va = e_a->GetDisplayDefaultCapacity();
	int vb = e_b->GetDisplayDefaultCapacity();
	int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/* Aircraft sorting functions */

/**
 * Determines order of aircraft by cargo
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL AircraftEngineCargoSorter(const EngineID *a, const EngineID *b)
{
	const Engine *e_a = Engine::Get(*a);
	const Engine *e_b = Engine::Get(*b);

	uint16 mail_a, mail_b;
	int va = e_a->GetDisplayDefaultCapacity(&mail_a);
	int vb = e_b->GetDisplayDefaultCapacity(&mail_b);
	int r = va - vb;

	if (r == 0) {
		/* The planes have the same passenger capacity. Check mail capacity instead */
		r = mail_a - mail_b;

		if (r == 0) {
			/* Use EngineID to sort instead since we want consistent sorting */
			return EngineNumberSorter(a, b);
		}
	}
	return _internal_sort_order ? -r : r;
}

/**
 * Determines order of aircraft by range.
 * @param *a first engine to compare.
 * @param *b second engine to compare.
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal.
 */
static int CDECL AircraftRangeSorter(const EngineID *a, const EngineID *b)
{
	uint16 r_a = Engine::Get(*a)->GetRange();
	uint16 r_b = Engine::Get(*b)->GetRange();

	int r = r_a - r_b;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorter(a, b);
	return _internal_sort_order ? -r : r;
}

/* Locomotive sorting functions. */

/**
 * Determines order of locomotives by engineID
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineNumberSorterLoco(const EngineID *a, const EngineID *b)
{
	int r = Engine::Get(*a)->list_position - Engine::Get(*b)->list_position;

	return _internal_sort_order_loco ? -r : r;
}

/**
 * Determines order of locomotives by introduction date
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineIntroDateSorterLoco(const EngineID *a, const EngineID *b)
{
	const int va = Engine::Get(*a)->intro_date;
	const int vb = Engine::Get(*b)->intro_date;
	const int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterLoco(a, b);
	return _internal_sort_order_loco ? -r : r;
}

/**
 * Determines order of locomotives by name
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineNameSorterLoco(const EngineID *a, const EngineID *b)
{
	static EngineID last_engine[2] = { INVALID_ENGINE, INVALID_ENGINE };
	static char     last_name[2][64] = { "\0", "\0" };

	const EngineID va = *a;
	const EngineID vb = *b;

	if (va != last_engine[0]) {
		last_engine[0] = va;
		SetDParam(0, va);
		GetString(last_name[0], STR_ENGINE_NAME, lastof(last_name[0]));
	}

	if (vb != last_engine[1]) {
		last_engine[1] = vb;
		SetDParam(0, vb);
		GetString(last_name[1], STR_ENGINE_NAME, lastof(last_name[1]));
	}

	int r = strnatcmp(last_name[0], last_name[1]); // Sort by name (natural sorting).

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterLoco(a, b);
	return _internal_sort_order_loco ? -r : r;
}

/**
 * Determines order of locomotives by reliability
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineReliabilitySorterLoco(const EngineID *a, const EngineID *b)
{
	const int va = Engine::Get(*a)->reliability;
	const int vb = Engine::Get(*b)->reliability;
	const int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterLoco(a, b);
	return _internal_sort_order_loco ? -r : r;
}

/**
 * Determines order of locomotives by purchase cost
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineCostSorterLoco(const EngineID *a, const EngineID *b)
{
	Money va = Engine::Get(*a)->GetCost();
	Money vb = Engine::Get(*b)->GetCost();
	int r = ClampToI32(va - vb);

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterLoco(a, b);
	return _internal_sort_order_loco ? -r : r;
}

/**
 * Determines order of locomotives by speed
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineSpeedSorterLoco(const EngineID *a, const EngineID *b)
{
	int va = Engine::Get(*a)->GetDisplayMaxSpeed();
	int vb = Engine::Get(*b)->GetDisplayMaxSpeed();
	int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterLoco(a, b);
	return _internal_sort_order_loco ? -r : r;
}

/**
 * Determines order of locomotives by power
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EnginePowerSorterLoco(const EngineID *a, const EngineID *b)
{
	int va = Engine::Get(*a)->GetPower();
	int vb = Engine::Get(*b)->GetPower();
	int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterLoco(a, b);
	return _internal_sort_order_loco ? -r : r;
}

/**
 * Determines order of locomotives by tractive effort
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineTractiveEffortSorterLoco(const EngineID *a, const EngineID *b)
{
	int va = Engine::Get(*a)->GetDisplayMaxTractiveEffort();
	int vb = Engine::Get(*b)->GetDisplayMaxTractiveEffort();
	int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterLoco(a, b);
	return _internal_sort_order_loco ? -r : r;
}

/**
 * Determines order of locomotives by running costs
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineRunningCostSorterLoco(const EngineID *a, const EngineID *b)
{
	Money va = Engine::Get(*a)->GetRunningCost();
	Money vb = Engine::Get(*b)->GetRunningCost();
	int r = ClampToI32(va - vb);

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterLoco(a, b);
	return _internal_sort_order_loco ? -r : r;
}

/**
 * Determines order of locomotives by running costs
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EnginePowerVsRunningCostSorterLoco(const EngineID *a, const EngineID *b)
{
	const Engine *e_a = Engine::Get(*a);
	const Engine *e_b = Engine::Get(*b);

	/* Here we are using a few tricks to get the right sort.
	 * We want power/running cost, but since we usually got higher running cost than power and we store the result in an int,
	 * we will actually calculate cunning cost/power (to make it more than 1).
	 * Because of this, the return value have to be reversed as well and we return b - a instead of a - b.
	 * Another thing is that both power and running costs should be doubled for multiheaded engines.
	 * Since it would be multiplying with 2 in both numerator and denominator, it will even themselves out and we skip checking for multiheaded. */
	Money va = (e_a->GetRunningCost()) / max(1U, (uint)e_a->GetPower());
	Money vb = (e_b->GetRunningCost()) / max(1U, (uint)e_b->GetPower());
	int r = ClampToI32(vb - va);

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterLoco(a, b);
	return _internal_sort_order_loco ? -r : r;
}

/**
 * Determines order of train locomotives by capacity
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL TrainEngineCapacitySorterLoco(const EngineID *a, const EngineID *b)
{
	const RailVehicleInfo *rvi_a = RailVehInfo(*a);
	const RailVehicleInfo *rvi_b = RailVehInfo(*b);

	int va = GetTotalCapacityOfArticulatedParts(*a) * (rvi_a->railveh_type == RAILVEH_MULTIHEAD ? 2 : 1);
	int vb = GetTotalCapacityOfArticulatedParts(*b) * (rvi_b->railveh_type == RAILVEH_MULTIHEAD ? 2 : 1);
	int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterLoco(a, b);
	return _internal_sort_order_loco ? -r : r;
}

/* Wagon sorting functions. */

/**
 * Determines order of wagons by engineID
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineNumberSorterWagon(const EngineID *a, const EngineID *b)
{
	int r = Engine::Get(*a)->list_position - Engine::Get(*b)->list_position;

	return _internal_sort_order_wagon ? -r : r;
}

/**
 * Determines order of wagons by introduction date
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineIntroDateSorterWagon(const EngineID *a, const EngineID *b)
{
	const int va = Engine::Get(*a)->intro_date;
	const int vb = Engine::Get(*b)->intro_date;
	const int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterWagon(a, b);
	return _internal_sort_order_wagon ? -r : r;
}

/**
 * Determines order of wagons by name
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineNameSorterWagon(const EngineID *a, const EngineID *b)
{
	static EngineID last_engine[2] = { INVALID_ENGINE, INVALID_ENGINE };
	static char     last_name[2][64] = { "\0", "\0" };

	const EngineID va = *a;
	const EngineID vb = *b;

	if (va != last_engine[0]) {
		last_engine[0] = va;
		SetDParam(0, va);
		GetString(last_name[0], STR_ENGINE_NAME, lastof(last_name[0]));
	}

	if (vb != last_engine[1]) {
		last_engine[1] = vb;
		SetDParam(0, vb);
		GetString(last_name[1], STR_ENGINE_NAME, lastof(last_name[1]));
	}

	int r = strnatcmp(last_name[0], last_name[1]); // Sort by name (natural sorting).

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterWagon(a, b);
	return _internal_sort_order_wagon ? -r : r;
}

/**
 * Determines order of wagons by purchase cost
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineCostSorterWagon(const EngineID *a, const EngineID *b)
{
	Money va = Engine::Get(*a)->GetCost();
	Money vb = Engine::Get(*b)->GetCost();
	int r = ClampToI32(va - vb);

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterWagon(a, b);
	return _internal_sort_order_wagon ? -r : r;
}

/**
 * Determines order of wagons by speed
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineSpeedSorterWagon(const EngineID *a, const EngineID *b)
{
	int va = Engine::Get(*a)->GetDisplayMaxSpeed();
	int vb = Engine::Get(*b)->GetDisplayMaxSpeed();
	int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterWagon(a, b);
	return _internal_sort_order_wagon ? -r : r;
}

/**
 * Determines order of wagons by running costs
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL EngineRunningCostSorterWagon(const EngineID *a, const EngineID *b)
{
	Money va = Engine::Get(*a)->GetRunningCost();
	Money vb = Engine::Get(*b)->GetRunningCost();
	int r = ClampToI32(va - vb);

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterWagon(a, b);
	return _internal_sort_order_wagon ? -r : r;
}

/**
 * Determines order of train wagons by capacity
 * @param *a first engine to compare
 * @param *b second engine to compare
 * @return for descending order: returns < 0 if a < b and > 0 for a > b. Vice versa for ascending order and 0 for equal
 */
static int CDECL TrainEngineCapacitySorterWagon(const EngineID *a, const EngineID *b)
{
	const RailVehicleInfo *rvi_a = RailVehInfo(*a);
	const RailVehicleInfo *rvi_b = RailVehInfo(*b);

	int va = GetTotalCapacityOfArticulatedParts(*a) * (rvi_a->railveh_type == RAILVEH_MULTIHEAD ? 2 : 1);
	int vb = GetTotalCapacityOfArticulatedParts(*b) * (rvi_b->railveh_type == RAILVEH_MULTIHEAD ? 2 : 1);
	int r = va - vb;

	/* Use EngineID to sort instead since we want consistent sorting */
	if (r == 0) return EngineNumberSorterWagon(a, b);
	return _internal_sort_order_wagon ? -r : r;
}

static EngList_SortTypeFunction * const _sorter[][11] = {{
	/* Trains */
	&EngineNumberSorter,
	&EngineCostSorter,
	&EngineSpeedSorter,
	&EnginePowerSorter,
	&EngineTractiveEffortSorter,
	&EngineIntroDateSorter,
	&EngineNameSorter,
	&EngineRunningCostSorter,
	&EnginePowerVsRunningCostSorter,
	&EngineReliabilitySorter,
	&TrainEngineCapacitySorter,
}, {
	/* Road vehicles */
	&EngineNumberSorter,
	&EngineCostSorter,
	&EngineSpeedSorter,
	&EnginePowerSorter,
	&EngineTractiveEffortSorter,
	&EngineIntroDateSorter,
	&EngineNameSorter,
	&EngineRunningCostSorter,
	&EnginePowerVsRunningCostSorter,
	&EngineReliabilitySorter,
	&RoadVehEngineCapacitySorter,
}, {
	/* Ships */
	&EngineNumberSorter,
	&EngineCostSorter,
	&EngineSpeedSorter,
	&EngineIntroDateSorter,
	&EngineNameSorter,
	&EngineRunningCostSorter,
	&EngineReliabilitySorter,
	&ShipEngineCapacitySorter,
}, {
	/* Aircraft */
	&EngineNumberSorter,
	&EngineCostSorter,
	&EngineSpeedSorter,
	&EngineIntroDateSorter,
	&EngineNameSorter,
	&EngineRunningCostSorter,
	&EngineReliabilitySorter,
	&AircraftEngineCargoSorter,
	&AircraftRangeSorter,
}};

static EngList_SortTypeFunction * const _sorter_loco[11] = {
	/* Locomotives */
	&EngineNumberSorterLoco,
	&EngineCostSorterLoco,
	&EngineSpeedSorterLoco,
	&EnginePowerSorterLoco,
	&EngineTractiveEffortSorterLoco,
	&EngineIntroDateSorterLoco,
	&EngineNameSorterLoco,
	&EngineRunningCostSorterLoco,
	&EnginePowerVsRunningCostSorterLoco,
	&EngineReliabilitySorter,
	&TrainEngineCapacitySorter
};

static EngList_SortTypeFunction * const _sorter_wagon[7] = {
	/* Wagons */
	&EngineNumberSorterWagon,
	&EngineCostSorterWagon,
	&EngineSpeedSorterWagon,
	&EngineIntroDateSorterWagon,
	&EngineNameSorterWagon,
	&EngineRunningCostSorterWagon,
	&TrainEngineCapacitySorterWagon
};

static const StringID _sort_listing[][12] = {{
	/* Trains */
	STR_SORT_BY_ENGINE_ID,
	STR_SORT_BY_COST,
	STR_SORT_BY_MAX_SPEED,
	STR_SORT_BY_POWER,
	STR_SORT_BY_TRACTIVE_EFFORT,
	STR_SORT_BY_INTRO_DATE,
	STR_SORT_BY_NAME,
	STR_SORT_BY_RUNNING_COST,
	STR_SORT_BY_POWER_VS_RUNNING_COST,
	STR_SORT_BY_RELIABILITY,
	STR_SORT_BY_CARGO_CAPACITY,
	INVALID_STRING_ID
}, {
	/* Road vehicles */
	STR_SORT_BY_ENGINE_ID,
	STR_SORT_BY_COST,
	STR_SORT_BY_MAX_SPEED,
	STR_SORT_BY_POWER,
	STR_SORT_BY_TRACTIVE_EFFORT,
	STR_SORT_BY_INTRO_DATE,
	STR_SORT_BY_NAME,
	STR_SORT_BY_RUNNING_COST,
	STR_SORT_BY_POWER_VS_RUNNING_COST,
	STR_SORT_BY_RELIABILITY,
	STR_SORT_BY_CARGO_CAPACITY,
	INVALID_STRING_ID
}, {
	/* Ships */
	STR_SORT_BY_ENGINE_ID,
	STR_SORT_BY_COST,
	STR_SORT_BY_MAX_SPEED,
	STR_SORT_BY_INTRO_DATE,
	STR_SORT_BY_NAME,
	STR_SORT_BY_RUNNING_COST,
	STR_SORT_BY_RELIABILITY,
	STR_SORT_BY_CARGO_CAPACITY,
	INVALID_STRING_ID
}, {
	/* Aircraft */
	STR_SORT_BY_ENGINE_ID,
	STR_SORT_BY_COST,
	STR_SORT_BY_MAX_SPEED,
	STR_SORT_BY_INTRO_DATE,
	STR_SORT_BY_NAME,
	STR_SORT_BY_RUNNING_COST,
	STR_SORT_BY_RELIABILITY,
	STR_SORT_BY_CARGO_CAPACITY,
	STR_SORT_BY_RANGE,
	INVALID_STRING_ID
}};

static const StringID _sort_listing_loco[12] = {
	/* Locomotives */
	STR_SORT_BY_ENGINE_ID,
	STR_SORT_BY_COST,
	STR_SORT_BY_MAX_SPEED,
	STR_SORT_BY_POWER,
	STR_SORT_BY_TRACTIVE_EFFORT,
	STR_SORT_BY_INTRO_DATE,
	STR_SORT_BY_NAME,
	STR_SORT_BY_RUNNING_COST,
	STR_SORT_BY_POWER_VS_RUNNING_COST,
	STR_SORT_BY_RELIABILITY,
	STR_SORT_BY_CARGO_CAPACITY,
	INVALID_STRING_ID
};

static const StringID _sort_listing_wagon[8] = {
	/* Wagons */
	STR_SORT_BY_ENGINE_ID,
	STR_SORT_BY_COST,
	STR_SORT_BY_MAX_SPEED,
	STR_SORT_BY_INTRO_DATE,
	STR_SORT_BY_NAME,
	STR_SORT_BY_RUNNING_COST,
	STR_SORT_BY_CARGO_CAPACITY,
	INVALID_STRING_ID
};

/** Cargo filter functions */
static bool CDECL CargoFilter(const EngineID *eid, const CargoID cid)
{
	if (cid == CF_ANY) return true;
	uint32 refit_mask = GetUnionOfArticulatedRefitMasks(*eid, true) & _standard_cargo_mask;
	return (cid == CF_NONE ? refit_mask == 0 : HasBit(refit_mask, cid));
}

static GUIEngineList::FilterFunction * const _filter_funcs[] = {
	&CargoFilter,
};

static int DrawCargoCapacityInfo(int left, int right, int y, EngineID engine, bool refittable)
{
	CargoArray cap = GetCapacityOfArticulatedParts(engine);

	for (CargoID c = 0; c < NUM_CARGO; c++) {
		if (cap[c] == 0) continue;

		SetDParam(0, c);
		SetDParam(1, cap[c]);
		SetDParam(2, refittable ? STR_PURCHASE_INFO_REFITTABLE : STR_EMPTY);
		DrawString(left, right, y, STR_PURCHASE_INFO_CAPACITY);
		y += FONT_HEIGHT_NORMAL;

		/* Only show as refittable once */
		refittable = false;
	}

	return y;
}

/* Draw rail wagon specific details */
static int DrawRailWagonPurchaseInfo(int left, int right, int y, EngineID engine_number, const RailVehicleInfo *rvi)
{
	const Engine *e = Engine::Get(engine_number);

	/* Purchase cost */
	SetDParam(0, e->GetCost());
	DrawString(left, right, y, STR_PURCHASE_INFO_COST);
	y += FONT_HEIGHT_NORMAL;

	/* Wagon weight - (including cargo) */
	uint weight = e->GetDisplayWeight();
	SetDParam(0, weight);
	uint cargo_weight = (e->CanCarryCargo() ? CargoSpec::Get(e->GetDefaultCargoType())->weight * GetTotalCapacityOfArticulatedParts(engine_number) / 16 : 0);
	SetDParam(1, cargo_weight + weight);
	DrawString(left, right, y, STR_PURCHASE_INFO_WEIGHT_CWEIGHT);
	y += FONT_HEIGHT_NORMAL;

	/* Wagon speed limit, displayed if above zero */
	if (_settings_game.vehicle.wagon_speed_limits) {
		uint max_speed = e->GetDisplayMaxSpeed();
		if (max_speed > 0) {
			SetDParam(0, max_speed);
			DrawString(left, right, y, STR_PURCHASE_INFO_SPEED);
			y += FONT_HEIGHT_NORMAL;
		}
	}

	/* Running cost */
	if (rvi->running_cost_class != INVALID_PRICE) {
		SetDParam(0, e->GetRunningCost());
		DrawString(left, right, y, STR_PURCHASE_INFO_RUNNINGCOST);
		y += FONT_HEIGHT_NORMAL;
	}

	return y;
}

/* Draw locomotive specific details */
static int DrawRailEnginePurchaseInfo(int left, int right, int y, EngineID engine_number, const RailVehicleInfo *rvi)
{
	const Engine *e = Engine::Get(engine_number);

	/* Purchase Cost - Engine weight */
	SetDParam(0, e->GetCost());
	SetDParam(1, e->GetDisplayWeight());
	DrawString(left, right, y, STR_PURCHASE_INFO_COST_WEIGHT);
	y += FONT_HEIGHT_NORMAL;

	/* Max speed - Engine power */
	SetDParam(0, e->GetDisplayMaxSpeed());
	SetDParam(1, e->GetPower());
	DrawString(left, right, y, STR_PURCHASE_INFO_SPEED_POWER);
	y += FONT_HEIGHT_NORMAL;

	/* Max tractive effort - not applicable if old acceleration or maglev */
	if (_settings_game.vehicle.train_acceleration_model != AM_ORIGINAL && GetRailTypeInfo(rvi->railtype)->acceleration_type != 2) {
		SetDParam(0, e->GetDisplayMaxTractiveEffort());
		DrawString(left, right, y, STR_PURCHASE_INFO_MAX_TE);
		y += FONT_HEIGHT_NORMAL;
	}

	/* Running cost */
	if (rvi->running_cost_class != INVALID_PRICE) {
		SetDParam(0, e->GetRunningCost());
		DrawString(left, right, y, STR_PURCHASE_INFO_RUNNINGCOST);
		y += FONT_HEIGHT_NORMAL;
	}

	/* Powered wagons power - Powered wagons extra weight */
	if (rvi->pow_wag_power != 0) {
		SetDParam(0, rvi->pow_wag_power);
		SetDParam(1, rvi->pow_wag_weight);
		DrawString(left, right, y, STR_PURCHASE_INFO_PWAGPOWER_PWAGWEIGHT);
		y += FONT_HEIGHT_NORMAL;
	}

	return y;
}

/* Draw road vehicle specific details */
static int DrawRoadVehPurchaseInfo(int left, int right, int y, EngineID engine_number)
{
	const Engine *e = Engine::Get(engine_number);

	if (_settings_game.vehicle.roadveh_acceleration_model != AM_ORIGINAL) {
		/* Purchase Cost */
		SetDParam(0, e->GetCost());
		DrawString(left, right, y, STR_PURCHASE_INFO_COST);
		y += FONT_HEIGHT_NORMAL;

		/* Road vehicle weight - (including cargo) */
		int16 weight = e->GetDisplayWeight();
		SetDParam(0, weight);
		uint cargo_weight = (e->CanCarryCargo() ? CargoSpec::Get(e->GetDefaultCargoType())->weight * GetTotalCapacityOfArticulatedParts(engine_number) / 16 : 0);
		SetDParam(1, cargo_weight + weight);
		DrawString(left, right, y, STR_PURCHASE_INFO_WEIGHT_CWEIGHT);
		y += FONT_HEIGHT_NORMAL;

		/* Max speed - Engine power */
		SetDParam(0, e->GetDisplayMaxSpeed());
		SetDParam(1, e->GetPower());
		DrawString(left, right, y, STR_PURCHASE_INFO_SPEED_POWER);
		y += FONT_HEIGHT_NORMAL;

		/* Max tractive effort */
		SetDParam(0, e->GetDisplayMaxTractiveEffort());
		DrawString(left, right, y, STR_PURCHASE_INFO_MAX_TE);
		y += FONT_HEIGHT_NORMAL;
	} else {
		/* Purchase cost - Max speed */
		SetDParam(0, e->GetCost());
		SetDParam(1, e->GetDisplayMaxSpeed());
		DrawString(left, right, y, STR_PURCHASE_INFO_COST_SPEED);
		y += FONT_HEIGHT_NORMAL;
	}

	/* Running cost */
	SetDParam(0, e->GetRunningCost());
	DrawString(left, right, y, STR_PURCHASE_INFO_RUNNINGCOST);
	y += FONT_HEIGHT_NORMAL;

	return y;
}

/* Draw ship specific details */
static int DrawShipPurchaseInfo(int left, int right, int y, EngineID engine_number, bool refittable)
{
	const Engine *e = Engine::Get(engine_number);

	/* Purchase cost - Max speed */
	uint raw_speed = e->GetDisplayMaxSpeed();
	uint ocean_speed = e->u.ship.ApplyWaterClassSpeedFrac(raw_speed, true);
	uint canal_speed = e->u.ship.ApplyWaterClassSpeedFrac(raw_speed, false);

	SetDParam(0, e->GetCost());
	if (ocean_speed == canal_speed) {
		SetDParam(1, ocean_speed);
		DrawString(left, right, y, STR_PURCHASE_INFO_COST_SPEED);
		y += FONT_HEIGHT_NORMAL;
	} else {
		DrawString(left, right, y, STR_PURCHASE_INFO_COST);
		y += FONT_HEIGHT_NORMAL;

		SetDParam(0, ocean_speed);
		DrawString(left, right, y, STR_PURCHASE_INFO_SPEED_OCEAN);
		y += FONT_HEIGHT_NORMAL;

		SetDParam(0, canal_speed);
		DrawString(left, right, y, STR_PURCHASE_INFO_SPEED_CANAL);
		y += FONT_HEIGHT_NORMAL;
	}

	/* Cargo type + capacity */
	SetDParam(0, e->GetDefaultCargoType());
	SetDParam(1, e->GetDisplayDefaultCapacity());
	SetDParam(2, refittable ? STR_PURCHASE_INFO_REFITTABLE : STR_EMPTY);
	DrawString(left, right, y, STR_PURCHASE_INFO_CAPACITY);
	y += FONT_HEIGHT_NORMAL;

	/* Running cost */
	SetDParam(0, e->GetRunningCost());
	DrawString(left, right, y, STR_PURCHASE_INFO_RUNNINGCOST);
	y += FONT_HEIGHT_NORMAL;

	return y;
}

/* Draw aircraft specific details */
static int DrawAircraftPurchaseInfo(int left, int right, int y, EngineID engine_number, bool refittable)
{
	const Engine *e = Engine::Get(engine_number);
	CargoID cargo = e->GetDefaultCargoType();

	/* Purchase cost - Max speed */
	SetDParam(0, e->GetCost());
	SetDParam(1, e->GetDisplayMaxSpeed());
	DrawString(left, right, y, STR_PURCHASE_INFO_COST_SPEED);
	y += FONT_HEIGHT_NORMAL;

	/* Cargo capacity */
	uint16 mail_capacity;
	uint capacity = e->GetDisplayDefaultCapacity(&mail_capacity);
	if (mail_capacity > 0) {
		SetDParam(0, cargo);
		SetDParam(1, capacity);
		SetDParam(2, CT_MAIL);
		SetDParam(3, mail_capacity);
		DrawString(left, right, y, STR_PURCHASE_INFO_AIRCRAFT_CAPACITY);
	} else {
		/* Note, if the default capacity is selected by the refit capacity
		 * callback, then the capacity shown is likely to be incorrect. */
		SetDParam(0, cargo);
		SetDParam(1, capacity);
		SetDParam(2, refittable ? STR_PURCHASE_INFO_REFITTABLE : STR_EMPTY);
		DrawString(left, right, y, STR_PURCHASE_INFO_CAPACITY);
	}
	y += FONT_HEIGHT_NORMAL;

	/* Running cost */
	SetDParam(0, e->GetRunningCost());
	DrawString(left, right, y, STR_PURCHASE_INFO_RUNNINGCOST);
	y += FONT_HEIGHT_NORMAL;

	uint16 range = e->GetRange();
	if (range != 0) {
		SetDParam(0, range);
		DrawString(left, right, y, STR_PURCHASE_INFO_AIRCRAFT_RANGE);
		y += FONT_HEIGHT_NORMAL;
	}

	return y;
}

/**
 * Display additional text from NewGRF in the purchase information window
 * @param left   Left border of text bounding box
 * @param right  Right border of text bounding box
 * @param y      Top border of text bounding box
 * @param engine Engine to query the additional purchase information for
 * @return       Bottom border of text bounding box
 */
static uint ShowAdditionalText(int left, int right, int y, EngineID engine)
{
	uint16 callback = GetVehicleCallback(CBID_VEHICLE_ADDITIONAL_TEXT, 0, 0, engine, NULL);
	if (callback == CALLBACK_FAILED || callback == 0x400) return y;
	const GRFFile *grffile = Engine::Get(engine)->GetGRF();
	if (callback > 0x400) {
		ErrorUnknownCallbackResult(grffile->grfid, CBID_VEHICLE_ADDITIONAL_TEXT, callback);
		return y;
	}

	StartTextRefStackUsage(grffile, 6);
	uint result = DrawStringMultiLine(left, right, y, INT32_MAX, GetGRFStringID(grffile->grfid, 0xD000 + callback), TC_BLACK);
	StopTextRefStackUsage();
	return result;
}

struct BuildVirtualTrainWindow : Window {
	
	Train **virtual_train;						     ///< the virtual train that is currently being created
	bool *noticeParent;

	/* Locomotives and wagons */

	VehicleType vehicle_type;
	bool listview_mode;


	/* Locomotives */

	bool descending_sort_order_loco;
	byte sort_criteria_loco;	
	EngineID sel_engine_loco;
	EngineID rename_engine_loco;
	GUIEngineList eng_list_loco;
	Scrollbar *vscroll_loco;
	byte cargo_filter_criteria_loco;                 ///< Selected cargo filter
	bool show_hidden_locos;                          ///< State of the 'show hidden locomotives' button.
	int details_height_loco;                         ///< Minimal needed height of the details panels (found so far).
	CargoID cargo_filter_loco[NUM_CARGO + 2];        ///< Available cargo filters; CargoID or CF_ANY or CF_NONE
	StringID cargo_filter_texts_loco[NUM_CARGO + 3]; ///< Texts for filter_cargo, terminated by INVALID_STRING_ID


	/* Wagons */

	bool descending_sort_order_wagon;
	byte sort_criteria_wagon;	
	EngineID sel_engine_wagon;
	EngineID rename_engine_wagon;
	GUIEngineList eng_list_wagon;
	Scrollbar *vscroll_wagon;
	byte cargo_filter_criteria_wagon;                 ///< Selected cargo filter
	bool show_hidden_wagons;                          ///< State of the 'show hidden wagons' button.
	int details_height_wagon;                         ///< Minimal needed height of the details panels (found so far).
	CargoID cargo_filter_wagon[NUM_CARGO + 2];        ///< Available cargo filters; CargoID or CF_ANY or CF_NONE
	StringID cargo_filter_texts_wagon[NUM_CARGO + 3]; ///< Texts for filter_cargo, terminated by INVALID_STRING_ID

	BuildVirtualTrainWindow(WindowDesc *desc, Train **vt, bool *notice) : Window(desc)
	{	
		this->vehicle_type = VEH_TRAIN;
		this->window_number = 0;

		this->sel_engine_loco            = INVALID_ENGINE;
		this->sort_criteria_loco         = _last_sort_criteria_loco;
		this->descending_sort_order_loco = _last_sort_order_loco;
		this->show_hidden_wagons         = _engine_sort_show_hidden_wagons;

		this->sel_engine_wagon            = INVALID_ENGINE;
		this->sort_criteria_wagon         = _last_sort_criteria_wagon;
		this->descending_sort_order_wagon = _last_sort_order_wagon;
		this->show_hidden_locos           = _engine_sort_show_hidden_locos;

		this->CreateNestedTree();

		this->vscroll_loco = this->GetScrollbar(WID_BV_SCROLLBAR_LOCO);
		this->vscroll_wagon = this->GetScrollbar(WID_BV_SCROLLBAR_WAGON);

		/* Locomotives */

		NWidgetCore *widget_loco = this->GetWidget<NWidgetCore>(WID_BV_LIST_LOCO);
		widget_loco->tool_tip = STR_BUY_VEHICLE_TRAIN_LIST_TOOLTIP + VEH_TRAIN;

		widget_loco = this->GetWidget<NWidgetCore>(WID_BV_SHOW_HIDE_LOCO);
		widget_loco->tool_tip = STR_BUY_VEHICLE_TRAIN_HIDE_SHOW_TOGGLE_TOOLTIP + VEH_TRAIN;

		widget_loco = this->GetWidget<NWidgetCore>(WID_BV_BUILD_LOCO);
		widget_loco->widget_data = STR_BUY_VEHICLE_TRAIN_BUY_LOCOMOTIVE_BUTTON;
		widget_loco->tool_tip    = STR_BUY_VEHICLE_TRAIN_BUY_LOCOMOTIVE_TOOLTIP;

		widget_loco = this->GetWidget<NWidgetCore>(WID_BV_RENAME_LOCO);
		widget_loco->widget_data = STR_BUY_VEHICLE_TRAIN_RENAME_LOCOMOTIVE_BUTTON;
		widget_loco->tool_tip    = STR_BUY_VEHICLE_TRAIN_RENAME_LOCOMOTIVE_TOOLTIP;

		widget_loco = this->GetWidget<NWidgetCore>(WID_BV_SHOW_HIDDEN_LOCOS);
		widget_loco->widget_data = STR_SHOW_HIDDEN_ENGINES_VEHICLE_TRAIN + VEH_TRAIN;
		widget_loco->tool_tip = STR_SHOW_HIDDEN_ENGINES_VEHICLE_TRAIN_TOOLTIP + VEH_TRAIN;
		widget_loco->SetLowered(this->show_hidden_locos);


		/* Wagons */

		NWidgetCore *widget_wagon = this->GetWidget<NWidgetCore>(WID_BV_LIST_WAGON);
		widget_wagon->tool_tip = STR_BUY_VEHICLE_TRAIN_LIST_TOOLTIP + VEH_TRAIN;

		widget_wagon = this->GetWidget<NWidgetCore>(WID_BV_SHOW_HIDE_WAGON);
		widget_wagon->tool_tip = STR_BUY_VEHICLE_TRAIN_HIDE_SHOW_TOGGLE_TOOLTIP + VEH_TRAIN;

		widget_wagon = this->GetWidget<NWidgetCore>(WID_BV_BUILD_WAGON);
		widget_wagon->widget_data = STR_BUY_VEHICLE_TRAIN_BUY_WAGON_BUTTON;
		widget_wagon->tool_tip    = STR_BUY_VEHICLE_TRAIN_BUY_WAGON_TOOLTIP;

		widget_wagon = this->GetWidget<NWidgetCore>(WID_BV_RENAME_WAGON);
		widget_wagon->widget_data = STR_BUY_VEHICLE_TRAIN_RENAME_WAGON_BUTTON;
		widget_wagon->tool_tip = STR_BUY_VEHICLE_TRAIN_RENAME_WAGON_TOOLTIP;

		widget_wagon = this->GetWidget<NWidgetCore>(WID_BV_SHOW_HIDDEN_WAGONS);
		widget_wagon->widget_data = STR_SHOW_HIDDEN_ENGINES_VEHICLE_TRAIN + VEH_TRAIN;
		widget_wagon->tool_tip = STR_SHOW_HIDDEN_ENGINES_VEHICLE_TRAIN_TOOLTIP + VEH_TRAIN;
		widget_wagon->SetLowered(this->show_hidden_wagons);


		this->details_height_loco = 10 * FONT_HEIGHT_NORMAL + WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM;
		this->details_height_wagon = 10 * FONT_HEIGHT_NORMAL + WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM;

		this->FinishInitNested(VEH_TRAIN);

		this->owner = _local_company;

		this->eng_list_loco.ForceRebuild();
		this->eng_list_wagon.ForceRebuild();
		
		// generate the list, since we need it in the next line
		this->GenerateBuildList();
		
		/* Select the first engine in the list as default when opening the window */
		if (this->eng_list_loco.Length() > 0) this->sel_engine_loco = this->eng_list_loco[0];
		if (this->eng_list_wagon.Length() > 0) this->sel_engine_wagon = this->eng_list_wagon[0];

		this->virtual_train = vt;
		this->noticeParent = notice;
	}

	/** Populate the filter list and set the cargo filter criteria. */
	void SetCargoFilterArray()
	{
		/* Locomotives */

		uint filter_items_loco = 0;

		/* Add item for disabling filtering. */
		this->cargo_filter_loco[filter_items_loco] = CF_ANY;
		this->cargo_filter_texts_loco[filter_items_loco] = STR_PURCHASE_INFO_ALL_TYPES;
		filter_items_loco++;

		/* Add item for vehicles not carrying anything, e.g. train engines. */		
		this->cargo_filter_loco[filter_items_loco] = CF_NONE;
		this->cargo_filter_texts_loco[filter_items_loco] = STR_LAND_AREA_INFORMATION_LOCAL_AUTHORITY_NONE;
		filter_items_loco++;		

		/* Collect available cargo types for filtering. */
		const CargoSpec *cs_loco;
		FOR_ALL_SORTED_STANDARD_CARGOSPECS(cs_loco) {
			this->cargo_filter_loco[filter_items_loco] = cs_loco->Index();
			this->cargo_filter_texts_loco[filter_items_loco] = cs_loco->name;
			filter_items_loco++;
		}

		/* Terminate the filter list. */
		this->cargo_filter_texts_loco[filter_items_loco] = INVALID_STRING_ID;

		/* If not found, the cargo criteria will be set to all cargoes. */
		this->cargo_filter_criteria_loco = 0;

		/* Find the last cargo filter criteria. */
		for (uint i = 0; i < filter_items_loco; i++) {
			if (this->cargo_filter_loco[i] == _last_filter_criteria_loco) {
				this->cargo_filter_criteria_loco = i;
				break;
			}
		}

		this->eng_list_loco.SetFilterFuncs(_filter_funcs);
		this->eng_list_loco.SetFilterState(this->cargo_filter_loco[this->cargo_filter_criteria_loco] != CF_ANY);


		/* Wagons */

		uint filter_items_wagon = 0;

		/* Add item for disabling filtering. */
		this->cargo_filter_wagon[filter_items_wagon] = CF_ANY;
		this->cargo_filter_texts_wagon[filter_items_wagon] = STR_PURCHASE_INFO_ALL_TYPES;
		filter_items_wagon++;

		/* Add item for vehicles not carrying anything, e.g. train engines.
		 * This could also be useful for eyecandy vehicles of other types, but is likely too confusing for joe, */
		
		this->cargo_filter_wagon[filter_items_wagon] = CF_NONE;
		this->cargo_filter_texts_wagon[filter_items_wagon] = STR_LAND_AREA_INFORMATION_LOCAL_AUTHORITY_NONE;
		filter_items_wagon++;
		

		/* Collect available cargo types for filtering. */
		
		const CargoSpec *cs_wagon;

		FOR_ALL_SORTED_STANDARD_CARGOSPECS(cs_wagon) {
			this->cargo_filter_wagon[filter_items_wagon] = cs_wagon->Index();
			this->cargo_filter_texts_wagon[filter_items_wagon] = cs_wagon->name;
			filter_items_wagon++;
		}

		/* Terminate the filter list. */
		this->cargo_filter_texts_wagon[filter_items_wagon] = INVALID_STRING_ID;

		/* If not found, the cargo criteria will be set to all cargoes. */
		this->cargo_filter_criteria_wagon = 0;

		/* Find the last cargo filter criteria. */
		for (uint i = 0; i < filter_items_wagon; i++) {
			if (this->cargo_filter_wagon[i] == _last_filter_criteria_wagon) {
				this->cargo_filter_criteria_wagon = i;
				break;
			}
		}
		this->eng_list_wagon.SetFilterFuncs(_filter_funcs);
		this->eng_list_wagon.SetFilterState(this->cargo_filter_wagon[this->cargo_filter_criteria_wagon] != CF_ANY);
	}

	void OnInit()
	{
		this->SetCargoFilterArray();
	}

	/** Filter the engine list against the currently selected cargo filter */
	void FilterEngineList()
	{
		this->eng_list_loco.Filter(this->cargo_filter_loco[this->cargo_filter_criteria_loco]);
		if (0 == this->eng_list_loco.Length()) { // no engine passed through the filter, invalidate the previously selected engine
			this->sel_engine_loco = INVALID_ENGINE;
		} else if (!this->eng_list_loco.Contains(this->sel_engine_loco)) { // previously selected engine didn't pass the filter, select the first engine of the list
			this->sel_engine_loco = this->eng_list_loco[0];
		}
		this->eng_list_wagon.Filter(this->cargo_filter_wagon[this->cargo_filter_criteria_wagon]);
		if (0 == this->eng_list_wagon.Length()) { // no engine passed through the filter, invalidate the previously selected engine
			this->sel_engine_wagon = INVALID_ENGINE;
		} else if (!this->eng_list_wagon.Contains(this->sel_engine_wagon)) { // previously selected engine didn't pass the filter, select the first engine of the list
			this->sel_engine_wagon = this->eng_list_wagon[0];
		}
	}

	/* Filter a single locomotive */
	bool FilterSingleEngineLoco(EngineID eid)
	{
		CargoID filter_type = this->cargo_filter_loco[this->cargo_filter_criteria_loco];
		return (filter_type == CF_ANY || CargoFilter(&eid, filter_type));
	}

	/* Filter a single wagon */
	bool FilterSingleEngineWagon(EngineID eid)
	{
		CargoID filter_type = this->cargo_filter_wagon[this->cargo_filter_criteria_wagon];
		return (filter_type == CF_ANY || CargoFilter(&eid, filter_type));
	}

	/* Figure out what train EngineIDs to put in the list */
	void GenerateBuildTrainList()
	{
		/* Locomotives */

		EngineID sel_id_loco = INVALID_ENGINE;

		int num_engines_loco = 0;
		int num_wagons_loco  = 0;

		this->eng_list_loco.Clear();

		/* Make list of all available train engines and wagons.
		 * Also check to see if the previously selected engine is still available,
		 * and if not, reset selection to INVALID_ENGINE. This could be the case
		 * when engines become obsolete and are removed */
		const Engine *e_loco;

		FOR_ALL_ENGINES_OF_TYPE(e_loco, VEH_TRAIN) {
			if (!this->show_hidden_locos && e_loco->IsHidden(_local_company)) continue;
			EngineID eid = e_loco->index;
			const RailVehicleInfo *rvi = &e_loco->u.rail;

			if (!IsEngineBuildable(eid, VEH_TRAIN, _local_company)) continue;

			/* Filter now! So num_engines and num_wagons is valid */
			if (!FilterSingleEngineLoco(eid)) continue;			

			if (rvi->railveh_type != RAILVEH_WAGON) {
				num_engines_loco++;
				*this->eng_list_loco.Append() = eid;
			}

			if (eid == this->sel_engine_loco) sel_id_loco = eid;
		}

		this->sel_engine_loco = sel_id_loco;


		/* Wagons */

		EngineID sel_id_wagon = INVALID_ENGINE;

		int num_engines_wagon = 0;
		int num_wagons_wagon  = 0;

		this->eng_list_wagon.Clear();

		/* Make list of all available train engines and wagons.
		 * Also check to see if the previously selected engine is still available,
		 * and if not, reset selection to INVALID_ENGINE. This could be the case
		 * when engines become obsolete and are removed */
		const Engine *e_wagon;

		FOR_ALL_ENGINES_OF_TYPE(e_wagon, VEH_TRAIN) {
			if (!this->show_hidden_wagons && e_wagon->IsHidden(_local_company)) continue;
			EngineID eid = e_wagon->index;
			const RailVehicleInfo *rvi = &e_wagon->u.rail;

			if (!IsEngineBuildable(eid, VEH_TRAIN, _local_company)) continue;

			/* Filter now! So num_engines and num_wagons is valid */
			if (!FilterSingleEngineWagon(eid)) continue;


			if (rvi->railveh_type == RAILVEH_WAGON) {
				*this->eng_list_wagon.Append() = eid;
				num_wagons_wagon++;
			}

			if (eid == this->sel_engine_wagon) sel_id_wagon = eid;
		}

		this->sel_engine_wagon = sel_id_wagon;

		/* Sort locomotives */
		_internal_sort_order_loco = this->descending_sort_order_loco;
		EngList_SortPartial(&this->eng_list_loco, _sorter_loco[this->sort_criteria_loco], 0, num_engines_loco);

		/* Sort wagons */
		_internal_sort_order_wagon = this->descending_sort_order_wagon;
		EngList_SortPartial(&this->eng_list_wagon, _sorter_wagon[this->sort_criteria_wagon], num_engines_wagon, num_wagons_wagon);

	}

	/* Generate the list of vehicles */
	void GenerateBuildList()
	{
		if (!this->eng_list_loco.NeedRebuild() && !this->eng_list_wagon.NeedRebuild()) return;

		this->GenerateBuildTrainList();
		this->eng_list_loco.Compact();
		this->eng_list_loco.RebuildDone();
		this->eng_list_wagon.Compact();
		this->eng_list_wagon.RebuildDone();

	}

	void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {

			/* Locomotives */


			case WID_BV_SORT_ASSENDING_DESCENDING_LOCO: {
				this->descending_sort_order_loco ^= true;
				_last_sort_order_loco = this->descending_sort_order_loco;
				this->eng_list_loco.ForceRebuild();
				this->SetDirty();
				break;
			}

			case WID_BV_SHOW_HIDDEN_LOCOS: {
				this->show_hidden_locos ^= true;
				_engine_sort_show_hidden_locos = this->show_hidden_locos;
				this->eng_list_loco.ForceRebuild();
				this->SetWidgetLoweredState(widget, this->show_hidden_locos);
				this->SetDirty();
				break;
			}

			case WID_BV_LIST_LOCO: {
				uint i = this->vscroll_loco->GetScrolledRowFromWidget(pt.y, this, WID_BV_LIST_LOCO);
				size_t num_items = this->eng_list_loco.Length();
				this->sel_engine_loco = (i < num_items) ? this->eng_list_loco[i] : INVALID_ENGINE;
				this->SetDirty();

				if (_ctrl_pressed) {
					this->OnClick(pt, WID_BV_SHOW_HIDE_LOCO, 1);
				}
				else if (click_count > 1 && !this->listview_mode) {
					this->OnClick(pt, WID_BV_BUILD_LOCO, 1);
				}
				break;
			}

			case WID_BV_SORT_DROPDOWN_LOCO: { // Select sorting criteria dropdown menu
				uint32 hidden_mask = 0;
				/* Disable sorting by tractive effort when the original acceleration model for trains is being used. */
				if (_settings_game.vehicle.train_acceleration_model == AM_ORIGINAL) {
					SetBit(hidden_mask, 4); // tractive effort
				}
				ShowDropDownMenu(this, _sort_listing_loco, this->sort_criteria_loco, WID_BV_SORT_DROPDOWN_LOCO, 0, hidden_mask);
				break;
			}

			case WID_BV_CARGO_FILTER_DROPDOWN_LOCO: { // Select cargo filtering criteria dropdown menu
				ShowDropDownMenu(this, this->cargo_filter_texts_loco, this->cargo_filter_criteria_loco, WID_BV_CARGO_FILTER_DROPDOWN_LOCO, 0, 0);
				break;
			}

			case WID_BV_SHOW_HIDE_LOCO: {
				const Engine *e = (this->sel_engine_loco == INVALID_ENGINE) ? NULL : Engine::GetIfValid(this->sel_engine_loco);
				if (e != NULL) {
					DoCommandP(0, 0, this->sel_engine_loco | (e->IsHidden(_current_company) ? 0 : (1u << 31)), CMD_SET_VEHICLE_VISIBILITY);
				}
				break;
			}

			case WID_BV_BUILD_LOCO: {
				EngineID sel_engine_loco = this->sel_engine_loco;
				if (sel_engine_loco != INVALID_ENGINE) {
					DoCommandP(0, sel_engine_loco, 0, CMD_BUILD_VIRTUAL_RAIL_VEHICLE, CcAddVirtualEngine);
				}
				break;
			}

			case WID_BV_RENAME_LOCO: {
				EngineID sel_eng = this->sel_engine_loco;
				if (sel_eng != INVALID_ENGINE) {
					this->rename_engine_loco = sel_eng;
					this->rename_engine_wagon = INVALID_ENGINE;
					SetDParam(0, sel_eng);
					ShowQueryString(STR_ENGINE_NAME, STR_QUERY_RENAME_TRAIN_TYPE_LOCOMOTIVE_CAPTION + this->vehicle_type, MAX_LENGTH_ENGINE_NAME_CHARS, this, CS_ALPHANUMERAL, QSF_ENABLE_DEFAULT | QSF_LEN_IN_CHARS);
				}
				break;
			}


			/* Wagons */

			case WID_BV_SORT_ASSENDING_DESCENDING_WAGON: {
				this->descending_sort_order_wagon ^= true;
				_last_sort_order_wagon = this->descending_sort_order_wagon;
				this->eng_list_wagon.ForceRebuild();
				this->SetDirty();
				break;
			}

			case WID_BV_SHOW_HIDDEN_WAGONS: {
				this->show_hidden_wagons ^= true;
				_engine_sort_show_hidden_wagons = this->show_hidden_wagons;
				this->eng_list_wagon.ForceRebuild();
				this->SetWidgetLoweredState(widget, this->show_hidden_wagons);
				this->SetDirty();
				break;
			}

			case WID_BV_LIST_WAGON: {
				uint i = this->vscroll_wagon->GetScrolledRowFromWidget(pt.y, this, WID_BV_LIST_WAGON);
				size_t num_items = this->eng_list_wagon.Length();
				this->sel_engine_wagon = (i < num_items) ? this->eng_list_wagon[i] : INVALID_ENGINE;
				this->SetDirty();

				if (_ctrl_pressed) {
					this->OnClick(pt, WID_BV_SHOW_HIDE_WAGON, 1);
				}
				else if (click_count > 1 && !this->listview_mode) {
					this->OnClick(pt, WID_BV_BUILD_WAGON, 1);
				}
				break;
			}

			case WID_BV_SORT_DROPDOWN_WAGON: { // Select sorting criteria dropdown menu
				uint32 hidden_mask = 0;
				/* Disable sorting by maximum speed when wagon speed is disabled. */
				if (!_settings_game.vehicle.wagon_speed_limits) {
					SetBit(hidden_mask, 2); // maximum speed
				}
				ShowDropDownMenu(this, _sort_listing_wagon, this->sort_criteria_wagon, WID_BV_SORT_DROPDOWN_WAGON, 0, hidden_mask);
				break;
			}

			case WID_BV_CARGO_FILTER_DROPDOWN_WAGON: { // Select cargo filtering criteria dropdown menu
				ShowDropDownMenu(this, this->cargo_filter_texts_wagon, this->cargo_filter_criteria_wagon, WID_BV_CARGO_FILTER_DROPDOWN_WAGON, 0, 0);
				break;
			}

			case WID_BV_SHOW_HIDE_WAGON: {
				const Engine *e = (this->sel_engine_wagon == INVALID_ENGINE) ? NULL : Engine::GetIfValid(this->sel_engine_wagon);
				if (e != NULL) {
					DoCommandP(0, 0, this->sel_engine_wagon | (e->IsHidden(_current_company) ? 0 : (1u << 31)), CMD_SET_VEHICLE_VISIBILITY);
				}
				break;
			}

			case WID_BV_BUILD_WAGON: {
				EngineID sel_engine_wagon = this->sel_engine_wagon;
				if (sel_engine_wagon != INVALID_ENGINE) {
					DoCommandP(0, sel_engine_wagon, 0, CMD_BUILD_VIRTUAL_RAIL_VEHICLE, CcAddVirtualEngine);
				}
				break;
			}

			case WID_BV_RENAME_WAGON: {
				EngineID sel_eng = this->sel_engine_wagon;
				if (sel_eng != INVALID_ENGINE) {
					this->rename_engine_loco = INVALID_ENGINE;
					this->rename_engine_wagon = sel_eng;
					SetDParam(0, sel_eng);
					ShowQueryString(STR_ENGINE_NAME, STR_QUERY_RENAME_TRAIN_TYPE_WAGON_CAPTION + this->vehicle_type, MAX_LENGTH_ENGINE_NAME_CHARS, this, CS_ALPHANUMERAL, QSF_ENABLE_DEFAULT | QSF_LEN_IN_CHARS);
				}
				break;
			}
		}
	}

	void AddVirtualEngine(Train *toadd)
	{
		if (*virtual_train == nullptr) {
			*virtual_train = toadd;
		} else {
			VehicleID target = (*(this->virtual_train))->GetLastUnit()->index;
			
			DoCommandP(0, (1<<21) | toadd->index, target, CMD_MOVE_RAIL_VEHICLE);
		}
		*noticeParent = true;
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		if (!gui_scope) return;

		/* When switching to original acceleration model for road vehicles, clear the selected sort criteria if it is not available now. */		
		this->eng_list_loco.ForceRebuild();
		this->eng_list_wagon.ForceRebuild();
	}

	virtual void SetStringParameters(int widget) const
	{
		switch (widget) {
			case WID_BV_CAPTION: {
				if (this->vehicle_type == VEH_TRAIN && !this->listview_mode) {
					const RailtypeInfo *rti = GetRailTypeInfo(RAILTYPE_RAIL);
					SetDParam(0, rti->strings.build_caption);
				} else {
					SetDParam(0, (this->listview_mode ? STR_VEHICLE_LIST_AVAILABLE_TRAINS : STR_BUY_VEHICLE_TRAIN_ALL_CAPTION) + this->vehicle_type);
				}
				break;
			}

			case WID_BV_CAPTION_LOCO: {
				SetDParam(0, STR_BUY_VEHICLE_TRAIN_LOCOMOTIVES);
				break;
			}

			case WID_BV_SHOW_HIDE_LOCO: {
				const Engine *e = (this->sel_engine_loco == INVALID_ENGINE) ? NULL : Engine::GetIfValid(this->sel_engine_loco);
				if (e != NULL && e->IsHidden(_local_company)) {
					SetDParam(0, STR_BUY_VEHICLE_TRAIN_SHOW_TOGGLE_BUTTON + VEH_TRAIN);
				}
				else {
					SetDParam(0, STR_BUY_VEHICLE_TRAIN_HIDE_TOGGLE_BUTTON + VEH_TRAIN);
				}
				break;
			}

			case WID_BV_CAPTION_WAGON: {
				SetDParam(0, STR_BUY_VEHICLE_TRAIN_WAGONS);
				break;
			}

			case WID_BV_SORT_DROPDOWN_LOCO: {
				SetDParam(0, _sort_listing_loco[this->sort_criteria_loco]);
				break;
			}

			case WID_BV_CARGO_FILTER_DROPDOWN_LOCO: {
				SetDParam(0, this->cargo_filter_texts_loco[this->cargo_filter_criteria_loco]);
				break;
			}

			case WID_BV_SORT_DROPDOWN_WAGON: {
				SetDParam(0, _sort_listing_wagon[this->sort_criteria_wagon]);
				break;
			}

			case WID_BV_CARGO_FILTER_DROPDOWN_WAGON: {
				SetDParam(0, this->cargo_filter_texts_wagon[this->cargo_filter_criteria_wagon]);
				break;
			}

			case WID_BV_SHOW_HIDE_WAGON: {
				const Engine *e = (this->sel_engine_wagon == INVALID_ENGINE) ? NULL : Engine::GetIfValid(this->sel_engine_wagon);
				if (e != NULL && e->IsHidden(_local_company)) {
					SetDParam(0, STR_BUY_VEHICLE_TRAIN_SHOW_TOGGLE_BUTTON + VEH_TRAIN);
				}
				else {
					SetDParam(0, STR_BUY_VEHICLE_TRAIN_HIDE_TOGGLE_BUTTON + VEH_TRAIN);
				}
				break;
			}
		}
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case WID_BV_LIST_LOCO: {
				resize->height = GetEngineListHeight(this->vehicle_type);
				size->height = 3 * resize->height;
				break;
			}

			case WID_BV_PANEL_LOCO: {
				size->height = this->details_height_loco;
				break;
			}

			case WID_BV_SORT_ASSENDING_DESCENDING_LOCO: {
				Dimension d = GetStringBoundingBox(this->GetWidget<NWidgetCore>(widget)->widget_data);
				d.width += padding.width + Window::SortButtonWidth() * 2; // Doubled since the string is centred and it also looks better.
				d.height += padding.height;
				*size = maxdim(*size, d);
				break;
			}

			case WID_BV_LIST_WAGON: {
				resize->height = GetEngineListHeight(this->vehicle_type);
				size->height = 3 * resize->height;
				break;
			}

			case WID_BV_PANEL_WAGON: {
				size->height = this->details_height_wagon;
				break;
			}

			case WID_BV_SORT_ASSENDING_DESCENDING_WAGON: {
				Dimension d = GetStringBoundingBox(this->GetWidget<NWidgetCore>(widget)->widget_data);
				d.width += padding.width + Window::SortButtonWidth() * 2; // Doubled since the string is centred and it also looks better.
				d.height += padding.height;
				*size = maxdim(*size, d);
				break;
			}

			case WID_BV_SHOW_HIDE_LOCO: {
				*size = GetStringBoundingBox(STR_BUY_VEHICLE_TRAIN_HIDE_TOGGLE_BUTTON + this->vehicle_type);
				*size = maxdim(*size, GetStringBoundingBox(STR_BUY_VEHICLE_TRAIN_SHOW_TOGGLE_BUTTON + this->vehicle_type));
				size->width += padding.width;
				size->height += padding.height;
				break;
			}

			case WID_BV_SHOW_HIDE_WAGON: {
				*size = GetStringBoundingBox(STR_BUY_VEHICLE_TRAIN_HIDE_TOGGLE_BUTTON + this->vehicle_type);
				*size = maxdim(*size, GetStringBoundingBox(STR_BUY_VEHICLE_TRAIN_SHOW_TOGGLE_BUTTON + this->vehicle_type));
				size->width += padding.width;
				size->height += padding.height;
				break;
			}
		}
	}

	virtual void DrawWidget(const Rect &r, int widget) const
	{
		switch (widget) {
			case WID_BV_LIST_LOCO: {
				DrawEngineList(this->vehicle_type, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, r.top + WD_FRAMERECT_TOP, &this->eng_list_loco, this->vscroll_loco->GetPosition(), min(this->vscroll_loco->GetPosition() + this->vscroll_loco->GetCapacity(), this->eng_list_loco.Length()), this->sel_engine_loco, false, DEFAULT_GROUP);
				break;
			}

			case WID_BV_SORT_ASSENDING_DESCENDING_LOCO: {
				this->DrawSortButtonState(WID_BV_SORT_ASSENDING_DESCENDING_LOCO, this->descending_sort_order_loco ? SBS_DOWN : SBS_UP);
				break;
			}

			case WID_BV_LIST_WAGON: {
				DrawEngineList(this->vehicle_type, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, r.top + WD_FRAMERECT_TOP, &this->eng_list_wagon, this->vscroll_wagon->GetPosition(), min(this->vscroll_wagon->GetPosition() + this->vscroll_wagon->GetCapacity(), this->eng_list_wagon.Length()), this->sel_engine_wagon, false, DEFAULT_GROUP);
				break;
			}

			case WID_BV_SORT_ASSENDING_DESCENDING_WAGON: {
				this->DrawSortButtonState(WID_BV_SORT_ASSENDING_DESCENDING_WAGON, this->descending_sort_order_wagon ? SBS_DOWN : SBS_UP);
				break;
			}
		}
	}

	virtual void OnPaint()
	{
		this->GenerateBuildList();
		this->vscroll_loco->SetCount(this->eng_list_loco.Length());
		this->vscroll_wagon->SetCount(this->eng_list_wagon.Length());

		this->SetWidgetDisabledState(WID_BV_SHOW_HIDE_LOCO, this->sel_engine_loco == INVALID_ENGINE);
		this->SetWidgetDisabledState(WID_BV_SHOW_HIDE_WAGON, this->sel_engine_wagon == INVALID_ENGINE);

		/* disable renaming engines in network games if you are not the server */
		this->SetWidgetDisabledState(WID_BV_RENAME_LOCO, (this->sel_engine_loco == INVALID_ENGINE) || (_networking && !_network_server));
		this->SetWidgetDisabledState(WID_BV_BUILD_LOCO, this->sel_engine_loco == INVALID_ENGINE);

		/* disable renaming engines in network games if you are not the server */
		this->SetWidgetDisabledState(WID_BV_RENAME_WAGON, (this->sel_engine_wagon == INVALID_ENGINE) || (_networking && !_network_server));
		this->SetWidgetDisabledState(WID_BV_BUILD_WAGON, this->sel_engine_wagon == INVALID_ENGINE);

		this->DrawWidgets();

		if (!this->IsShaded()) {
			int needed_height_loco = this->details_height_loco;
			/* Draw details panels. */
			if (this->sel_engine_loco != INVALID_ENGINE) {
				NWidgetBase *nwi = this->GetWidget<NWidgetBase>(WID_BV_PANEL_LOCO);
				int text_end = DrawVehiclePurchaseInfo(nwi->pos_x + WD_FRAMETEXT_LEFT, nwi->pos_x + nwi->current_x - WD_FRAMETEXT_RIGHT,
						nwi->pos_y + WD_FRAMERECT_TOP, this->sel_engine_loco);
				needed_height_loco = max(needed_height_loco, text_end - (int)nwi->pos_y + WD_FRAMERECT_BOTTOM);
			}
			if (needed_height_loco != this->details_height_loco) { // Details window are not high enough, enlarge them.
				int resize = needed_height_loco - this->details_height_loco;
				this->details_height_loco = needed_height_loco;
				this->ReInit(0, resize);
				return;
			}

			int needed_height_wagon = this->details_height_wagon;
			if (this->sel_engine_wagon != INVALID_ENGINE) {
				NWidgetBase *nwi = this->GetWidget<NWidgetBase>(WID_BV_PANEL_WAGON);
				int text_end = DrawVehiclePurchaseInfo(nwi->pos_x + WD_FRAMETEXT_LEFT, nwi->pos_x + nwi->current_x - WD_FRAMETEXT_RIGHT,
						nwi->pos_y + WD_FRAMERECT_TOP, this->sel_engine_wagon);
				needed_height_wagon = max(needed_height_wagon, text_end - (int)nwi->pos_y + WD_FRAMERECT_BOTTOM);
			}
			if (needed_height_wagon != this->details_height_wagon) { // Details window are not high enough, enlarge them.
				int resize = needed_height_wagon - this->details_height_wagon;
				this->details_height_wagon = needed_height_wagon;
				this->ReInit(0, resize);
				return;
			}
		}
	}

	virtual void OnQueryTextFinished(char *str)
	{
		if (str == NULL) return;

		if (this->rename_engine_loco != INVALID_ENGINE)
		{
			DoCommandP(0, this->rename_engine_loco, 0, CMD_RENAME_ENGINE | CMD_MSG(STR_ERROR_CAN_T_RENAME_TRAIN_TYPE + this->vehicle_type), NULL, str);
		}
		else
		{
			DoCommandP(0, this->rename_engine_wagon, 0, CMD_RENAME_ENGINE | CMD_MSG(STR_ERROR_CAN_T_RENAME_TRAIN_TYPE + this->vehicle_type), NULL, str);
		}
	}

	virtual void OnDropdownSelect(int widget, int index)
	{
		switch (widget) {
			case WID_BV_SORT_DROPDOWN_LOCO: {
				if (this->sort_criteria_loco != index) {
					this->sort_criteria_loco = index;
					_last_sort_criteria_loco = this->sort_criteria_loco;
					this->eng_list_loco.ForceRebuild();
				}
				break;
			}

			case WID_BV_CARGO_FILTER_DROPDOWN_LOCO: { // Select a cargo filter criteria
				if (this->cargo_filter_criteria_loco != index) {
					this->cargo_filter_criteria_loco = index;
					_last_filter_criteria_loco = this->cargo_filter_loco[this->cargo_filter_criteria_loco];
					/* deactivate filter if criteria is 'Show All', activate it otherwise */
					this->eng_list_loco.SetFilterState(this->cargo_filter_loco[this->cargo_filter_criteria_loco] != CF_ANY);
					this->eng_list_loco.ForceRebuild();
				}
				break;
			}

			case WID_BV_SORT_DROPDOWN_WAGON: {
				if (this->sort_criteria_wagon != index) {
					this->sort_criteria_wagon = index;
					_last_sort_criteria_wagon = this->sort_criteria_wagon;
					this->eng_list_wagon.ForceRebuild();
				}
				break;
			}

			case WID_BV_CARGO_FILTER_DROPDOWN_WAGON: { // Select a cargo filter criteria
				if (this->cargo_filter_criteria_wagon != index) {
					this->cargo_filter_criteria_wagon = index;
					_last_filter_criteria_wagon = this->cargo_filter_wagon[this->cargo_filter_criteria_wagon];
					/* deactivate filter if criteria is 'Show All', activate it otherwise */
					this->eng_list_wagon.SetFilterState(this->cargo_filter_wagon[this->cargo_filter_criteria_wagon] != CF_ANY);
					this->eng_list_wagon.ForceRebuild();
				}
				break;
			}
		}

		this->SetDirty();
	}

	virtual void OnResize()
	{
		this->vscroll_loco->SetCapacityFromWidget(this, WID_BV_LIST_LOCO);
		this->vscroll_wagon->SetCapacityFromWidget(this, WID_BV_LIST_WAGON);
	}
};

void CcAddVirtualEngine(const CommandCost &result, TileIndex tile, uint32 p1, uint32 p2)
{
	if (result.Failed()) return;

	Window* window = FindWindowById(WC_BUILD_VIRTUAL_TRAIN, 0);
	if (window) {
		Train* train = Train::From(Vehicle::Get(_new_vehicle_id));
		((BuildVirtualTrainWindow*)window)->AddVirtualEngine(train);
		window->InvalidateData();
	}
}

static WindowDesc _build_vehicle_desc_train_advanced(
	WDP_AUTO, "build_vehicle", 480, 268,
	WC_BUILD_VIRTUAL_TRAIN, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_build_vehicle_widgets_train_advanced, lengthof(_nested_build_vehicle_widgets_train_advanced)
);

void ShowBuildVirtualTrainWindow(Train **vt, bool *noticeParent)
{
	assert(IsCompanyBuildableVehicleType(VEH_TRAIN));

	DeleteWindowById(WC_BUILD_VIRTUAL_TRAIN, 0);

	new BuildVirtualTrainWindow(&_build_vehicle_desc_train_advanced, vt, noticeParent);
}

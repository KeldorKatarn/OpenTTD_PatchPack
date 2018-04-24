/** @file triphistory_gui.cpp */

#include "stdafx.h"
#include "triphistory.h"
#include "strings_func.h"
#include "window_func.h"
#include "gfx_func.h"
#include "date_func.h"
#include "vehicle_base.h"
#include "widget_type.h"
#include "window_gui.h"
#include "triphistory.h"
#include "table/strings.h"

/* Names of the widgets. Keep them in the same order as in the widget array */
enum VehicleTripWidgets {
    VTH_CAPTION,
    VTH_LABEL_RECEIVED,
    VTH_LABEL_PROFIT,
    VTH_LABEL_PROFIT_CHANGE,
    VTH_LABEL_TBT,
    VTH_LABEL_TBT_CHANGE,
    VTH_MATRIX_RECEIVED,
    VTH_MATRIX_PROFIT,
    VTH_MATRIX_PROFIT_CHANGE,
    VTH_MATRIX_TBT,
    VTH_MATRIX_TBT_CHANGE,
    VTH_SUMMARY,

};
static const NWidgetPart _vehicle_trip_history_widgets[] = {
    NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, VTH_CAPTION), SetDataTip(STR_TRIP_HISTORY_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_VERTICAL),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, VTH_LABEL_RECEIVED), SetMinimalSize(110, 0), SetMinimalTextLines(1, 2), SetResize(1, 0), SetFill(1, 0),
				SetDataTip(STR_TRIP_HISTORY_RECEIVED_LABEL, STR_TRIP_HISTORY_RECEIVED_LABEL_TIP),
			NWidget(WWT_MATRIX, COLOUR_GREY, VTH_MATRIX_RECEIVED), SetMinimalSize(110, 0), SetMatrixDataTip(1, 10, STR_NULL), SetResize(1, 1),SetFill(1, 0),
		EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, VTH_LABEL_PROFIT), SetMinimalSize(110, 0), SetMinimalTextLines(1, 2), SetResize(1, 0), SetFill(1, 0),
				SetDataTip(STR_TRIP_HISTORY_PROFIT_LABEL, STR_TRIP_HISTORY_PROFIT_LABEL_TIP),
			NWidget(WWT_MATRIX, COLOUR_GREY, VTH_MATRIX_PROFIT), SetMinimalSize(110, 0), SetMatrixDataTip(1, 10, STR_NULL), SetResize(1, 1),SetFill(1, 0),
		EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, VTH_LABEL_PROFIT_CHANGE), SetMinimalSize(50, 0), SetMinimalTextLines(1, 2), SetResize(1, 0),SetFill(1, 0),
				SetDataTip(STR_TRIP_HISTORY_CHANGE_LABEL, STR_TRIP_HISTORY_CHANGE_LABEL_TIP),
			NWidget(WWT_MATRIX, COLOUR_GREY, VTH_MATRIX_PROFIT_CHANGE), SetMinimalSize(50, 0), SetMatrixDataTip(1, 10, STR_NULL), SetResize(1, 1),SetFill(1, 0),
		EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, VTH_LABEL_TBT), SetMinimalSize(110, 0), SetMinimalTextLines(1, 2), SetResize(1, 0),SetFill(1, 0),
				SetDataTip(STR_TRIP_HISTORY_TBT_LABEL, STR_TRIP_HISTORY_TBT_LABEL_TIP),
			NWidget(WWT_MATRIX, COLOUR_GREY, VTH_MATRIX_TBT), SetMinimalSize(110, 0), SetMatrixDataTip(1, 10, STR_NULL), SetResize(1, 1),SetFill(1, 0),
		EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, VTH_LABEL_TBT_CHANGE), SetMinimalSize(50, 0), SetMinimalTextLines(1, 2), SetResize(1, 0),SetFill(1, 0),
				SetDataTip(STR_TRIP_HISTORY_CHANGE_LABEL,    STR_TRIP_HISTORY_CHANGE_LABEL_TIP),
			NWidget(WWT_MATRIX, COLOUR_GREY, VTH_MATRIX_TBT_CHANGE), SetMinimalSize(50, 0), SetMatrixDataTip(1, 10, STR_NULL), SetResize(1, 1),SetFill(1, 0),
		EndContainer(),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_GREY, VTH_SUMMARY), SetMinimalTextLines(3, 2), SetResize(1, 0), SetFill(1, 0), EndContainer(),
};

struct VehicleTripHistoryWindow : Window {

private:
	uint8 valid_rows; // number of rows in trip history

public:
	VehicleTripHistoryWindow(WindowDesc *desc, WindowNumber window_number) :
		Window(desc), valid_rows(0)
	{
		const Vehicle *v = Vehicle::GetIfValid(window_number);

		assert(v != nullptr);

		this->CreateNestedTree();
		
		this->FinishInitNested(window_number);
		this->owner = v->owner;
		InvalidateData();
	}

	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		Vehicle *v = Vehicle::GetIfValid(this->window_number);

		assert(v != nullptr);

		valid_rows = (uint8)v->trip_history.UpdateCalculated();
		this->SetDirty();
	}

	virtual void SetStringParameters(int widget) const
	{
		switch (widget) {
			case VTH_CAPTION: SetDParam(0, this->window_number); break;
		}
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case VTH_SUMMARY: {
				SetDParam(0, UINT64_MAX >> 2);
				SetDParam(1, 100);
				Dimension text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_TOTAL_INCOME);
				size->width = text_dim.width + WD_FRAMERECT_LEFT + WD_FRAMERECT_RIGHT;
				break;
			}

			case VTH_LABEL_RECEIVED: {
				SetDParam(0, INT32_MAX);
				Dimension text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_TIME);
				size->width = text_dim.width + WD_MATRIX_LEFT + WD_MATRIX_RIGHT;
				break;
			}

			case VTH_MATRIX_RECEIVED: {
				SetDParam(0, INT32_MAX);
				Dimension text_dim = GetStringBoundingBox(STR_TRIP_HISTORY_TIME);
				size->width = text_dim.width + WD_MATRIX_LEFT + WD_MATRIX_RIGHT;
			}
			case VTH_MATRIX_PROFIT:
			case VTH_MATRIX_PROFIT_CHANGE:
			case VTH_MATRIX_TBT:
			case VTH_MATRIX_TBT_CHANGE:
				resize->height = FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM;
				size->height = 10 * resize->height;
				break;
		}
	}

	virtual void DrawWidget(const Rect &r, int widget) const
	{
	    const Vehicle *v = Vehicle::Get(this->window_number);
		int y = WD_FRAMERECT_TOP;
		
		switch( widget ) {
		    case VTH_MATRIX_RECEIVED:
			    for (int i = 0; i < valid_rows; i++, y += FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM) {
					if (v->trip_history.entries[i].ticks == 0) continue;
					
					SetDParam(0, v->trip_history.entries[i].ticks);
					DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, i == 0 ? STR_TRIP_HISTORY_TEMPORARY_TIME : STR_TRIP_HISTORY_TIME, TC_BLACK, SA_RIGHT);
			    }
			    break;

			case VTH_MATRIX_PROFIT:
			    for (int i = 0; i < valid_rows; i++, y += FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM) {
					if (v->trip_history.entries[i].ticks == 0) continue;

					SetDParam(0, v->trip_history.entries[i].profit);
					DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, i == 0 ? STR_TRIP_HISTORY_TEMPORARY_PROFIT : STR_TRIP_HISTORY_PROFIT, TC_BLACK, SA_RIGHT);
			    }
			    break;

			case VTH_MATRIX_PROFIT_CHANGE:
			    for (int i = 0; i < valid_rows - 1; i++, y += FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM) {
					if (i == 0) continue;
					if (v->trip_history.entries[i].ticks == 0) continue;
					if (v->trip_history.entries[i + 1].ticks == 0) continue;

					SetDParam(0, v->trip_history.entries[i].profit_change);
					DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y,
						v->trip_history.entries[i].profit_change == 0 ? STR_TRIP_HISTORY_PROFITCHANGEZERO :
						(v->trip_history.entries[i].profit_change >= 0 ?
							STR_TRIP_HISTORY_PROFITCHANGEPOS :
							STR_TRIP_HISTORY_PROFITCHANGENEG), TC_BLACK, SA_RIGHT
					);
			    }
			    break;

			case VTH_MATRIX_TBT:
			    for (int i = 0; i < valid_rows; i++, y += FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM) {
					if (i == 0) continue;
					if (v->trip_history.entries[i].ticks == 0) continue;
					if (v->trip_history.entries[i].time_between_trips == 0) continue;

					SetDParam(0, v->trip_history.entries[i].time_between_trips);
					DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y, STR_TRIP_HISTORY_TBT, TC_BLACK, SA_RIGHT);
			    }
			    break;

			case VTH_MATRIX_TBT_CHANGE:
			    for (int i = 0; i < valid_rows - 1; i++, y += FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM) {
					if (i == 0) continue;
					if (v->trip_history.entries[i].ticks == 0) continue;
					if (v->trip_history.entries[i].time_between_trips == 0) continue;
					if (v->trip_history.entries[i + 1].ticks == 0) continue;
					if (v->trip_history.entries[i + 1].time_between_trips == 0) continue;

					SetDParam(0, v->trip_history.entries[i].time_between_trips_change);
					DrawString(r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, r.top + y,
						v->trip_history.entries[i].time_between_trips_change > 0 ?
							STR_TRIP_HISTORY_TBTCHANGEPOS :
							STR_TRIP_HISTORY_TBTCHANGENEG, TC_BLACK, SA_RIGHT
					);
			    }
			    break;

			case VTH_SUMMARY:
			    SetDParam(0, (valid_rows > 0) ? (valid_rows - 1) : 0);
			    SetDParam(1, v->trip_history.total_profit);
			    SetDParam(2, v->trip_history.avg_profit);
			    DrawString(r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, r.top + WD_FRAMERECT_TOP, STR_TRIP_HISTORY_TOTAL_INCOME, TC_BLACK);
			    SetDParam(0, v->trip_history.avg_time_between_trips);
			    DrawString(r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, r.top + FONT_HEIGHT_NORMAL + WD_FRAMERECT_TOP, STR_TRIP_AVERAGE_TRIP_LENGTH, TC_BLACK);
			    break;
		}
	}
};

static WindowDesc _vehicle_trip_history_desc(
	WDP_AUTO, "trip_history", 380, 191, 
	WC_VEHICLE_TRIP_HISTORY, WC_VEHICLE_DETAILS,
	0,
	_vehicle_trip_history_widgets,
	lengthof(_vehicle_trip_history_widgets)
);

void ShowTripHistoryWindow(const Vehicle *v)
{
	if (!BringWindowToFrontById(WC_VEHICLE_TRIP_HISTORY, v->index)) {
		AllocateWindowDescFront<VehicleTripHistoryWindow>(&_vehicle_trip_history_desc, v->index);
	}
}

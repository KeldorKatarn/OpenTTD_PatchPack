/* $Id$ */

/*
* This file is part of OpenTTD.
* OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
* OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
*/

/** @file plans_gui.cpp The GUI for planning. */

#include "stdafx.h"
#include "plans_func.h"
#include "plans_base.h"
#include "command_func.h"
#include "company_func.h"
#include "company_gui.h"
#include "settings_gui.h"
#include "window_gui.h"
#include "window_func.h"
#include "viewport_func.h"
#include "gfx_func.h"
#include "tilehighlight_func.h"
#include "strings_func.h"
#include "core/pool_func.hpp"
#include "widgets/plans_widget.h"
#include "table/strings.h"
#include "table/sprites.h"

static const NWidgetPart _nested_plans_widgets[] = {
	NWidget(NWID_HORIZONTAL),
	NWidget(WWT_CLOSEBOX, COLOUR_GREY),
	NWidget(WWT_CAPTION, COLOUR_GREY, WID_PLN_CAPTION), SetDataTip(STR_PLANS_CAPTION, STR_NULL),
	NWidget(WWT_SHADEBOX, COLOUR_GREY),
	NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
	NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),

	NWidget(NWID_HORIZONTAL),
	NWidget(WWT_PANEL, COLOUR_GREY),
	NWidget(NWID_HORIZONTAL),
	NWidget(WWT_INSET, COLOUR_GREY, WID_PLN_LIST), SetFill(1, 1), SetPadding(2, 1, 2, 2), SetResize(1, 0), SetScrollbar(WID_PLN_SCROLLBAR), SetDataTip(STR_NULL, STR_PLANS_LIST_TOOLTIP),
	EndContainer(),
	EndContainer(),
	EndContainer(),
	NWidget(NWID_VSCROLLBAR, COLOUR_GREY, WID_PLN_SCROLLBAR),
	EndContainer(),

	NWidget(WWT_PANEL, COLOUR_GREY),
	NWidget(NWID_HORIZONTAL),
	NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_PLN_NEW), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_PLANS_NEW_PLAN, STR_NULL),
	NWidget(WWT_TEXTBTN_2, COLOUR_GREY, WID_PLN_ADD_LINES), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_PLANS_ADD_LINES, STR_PLANS_ADDING_LINES),
	NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_PLN_VISIBILITY), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_PLANS_VISIBILITY_PUBLIC, STR_PLANS_VISIBILITY_TOOLTIP),
	NWidget(NWID_SELECTION, INVALID_COLOUR, WID_PLN_HIDE_ALL_SEL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_PLN_HIDE_ALL), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_PLANS_HIDE_ALL, STR_PLANS_HIDE_ALL_TOOLTIP),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_PLN_SHOW_ALL), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_PLANS_SHOW_ALL, STR_PLANS_SHOW_ALL_TOOLTIP),
	EndContainer(),
	NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_PLN_DELETE), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_PLANS_DELETE, STR_PLANS_DELETE_TOOLTIP),
	NWidget(WWT_RESIZEBOX, COLOUR_GREY),
	EndContainer(),
	EndContainer(),
};

static WindowDesc _plans_desc(
	WDP_AUTO, "plans", 300, 100,
	WC_PLANS, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_plans_widgets, lengthof(_nested_plans_widgets)
	);

struct PlansWindow : Window {
	typedef struct {
		bool is_plan;
		int plan_id;
		int line_id;
	} ListItem;

	Scrollbar *vscroll;
	NWidgetStacked *hide_all_sel;
	std::vector<ListItem> list; ///< The translation table linking panel indices to their related PlanID.
	int selected; ///< What item is currently selected in the panel.
	uint vis_btn_left; ///< left offset of visibility button
	Dimension company_icon_spr_dim; ///< dimensions of company icon

	PlansWindow(WindowDesc *desc) : Window(desc)
	{
		this->CreateNestedTree();
		this->vscroll = this->GetScrollbar(WID_PLN_SCROLLBAR);
		this->hide_all_sel = this->GetWidget<NWidgetStacked>(WID_PLN_HIDE_ALL_SEL);
		this->hide_all_sel->SetDisplayedPlane(0);
		this->FinishInitNested();

		this->selected = INT_MAX;
		RebuildList();
	}

	~PlansWindow()
	{
		this->list.clear();
		_current_plan = NULL;
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
		case WID_PLN_NEW:
			DoCommandP(0, _local_company, 0, CMD_ADD_PLAN, CcAddPlan);
			break;
		case WID_PLN_ADD_LINES:
			if (_current_plan) HandlePlacePushButton(this, widget, SPR_CURSOR_MOUSE, HT_POINT);
			break;
		case WID_PLN_DELETE:
			if (this->selected != INT_MAX) {
				if (this->list[this->selected].is_plan) {
					DoCommandP(0, this->list[this->selected].plan_id, 0, CMD_REMOVE_PLAN);
				}
				else {
					DoCommandP(0, this->list[this->selected].plan_id, this->list[this->selected].line_id, CMD_REMOVE_PLAN_LINE);
				}
			}
			break;
		case WID_PLN_HIDE_ALL: {
			Plan *p;
			FOR_ALL_PLANS(p) {
				if (p->IsListable()) p->SetVisibility(false);
			}
			this->SetWidgetDirty(WID_PLN_LIST);
			break;
		}
			case WID_PLN_SHOW_ALL: {
				Plan *p;
				FOR_ALL_PLANS(p) {
					if (p->IsListable()) p->SetVisibility(true);
				}
				this->SetWidgetDirty(WID_PLN_LIST);
				break;
			}
		case WID_PLN_VISIBILITY:
			if (_current_plan) _current_plan->ToggleVisibilityByAll();
			break;
		case WID_PLN_LIST: {
			int new_selected = this->vscroll->GetScrolledRowFromWidget(pt.y, this, WID_PLN_LIST, WD_FRAMERECT_TOP);
			if (this->selected != INT_MAX) {
				_current_plan->SetFocus(false);
			}
			if (new_selected != INT_MAX) {
				const int btn_left = this->vis_btn_left;
				const int btn_right = btn_left + SETTING_BUTTON_WIDTH;
				if (this->list[new_selected].is_plan) {
					_current_plan = Plan::Get(this->list[new_selected].plan_id);
					_current_plan->SetFocus(true);
					if (pt.x >= btn_left && pt.x < btn_right) _current_plan->ToggleVisibility();
				}
				else {
					_current_plan = Plan::Get(this->list[new_selected].plan_id);
					PlanLine *pl = _current_plan->lines[this->list[new_selected].line_id];
					pl->SetFocus(true);
					if (pt.x >= btn_left && pt.x < btn_right) {
						if (pl->ToggleVisibility()) _current_plan->SetVisibility(true, false);
					}
				}
				if (click_count > 1 && (pt.x < 22 || pt.x >= 41)) {
					_current_plan->show_lines = !_current_plan->show_lines;
					this->InvalidateData(INVALID_PLAN);
				}
			}
			else {
				if (_current_plan) {
					_current_plan->SetFocus(false);
					_current_plan = NULL;
				}
			}
			this->selected = new_selected;
			this->SetDirty();
			break;
		}
		default: break;
		}
	}
 
	bool AllPlansHidden() const
	{
		Plan *p;
		FOR_ALL_PLANS(p) {
			if (p->IsVisible()) return false;
		}
		return true;
	}

	virtual void OnPaint()
	{
		this->SetWidgetDisabledState(WID_PLN_HIDE_ALL, this->vscroll->GetCount() == 0);
		this->SetWidgetDisabledState(WID_PLN_SHOW_ALL, this->vscroll->GetCount() == 0);
		this->hide_all_sel->SetDisplayedPlane(this->vscroll->GetCount() != 0 && this->AllPlansHidden() ? 1 : 0);
		if (_current_plan) {
			this->SetWidgetsDisabledState(_current_plan->owner != _local_company, WID_PLN_ADD_LINES, WID_PLN_VISIBILITY, WID_PLN_DELETE, WIDGET_LIST_END);
			this->GetWidget<NWidgetCore>(WID_PLN_VISIBILITY)->widget_data = _current_plan->visible_by_all ? STR_PLANS_VISIBILITY_PRIVATE : STR_PLANS_VISIBILITY_PUBLIC;
		}
		else {
			this->SetWidgetsDisabledState(true, WID_PLN_ADD_LINES, WID_PLN_VISIBILITY, WID_PLN_DELETE, WIDGET_LIST_END);
		}
		this->DrawWidgets();
	}

	virtual void DrawWidget(const Rect &r, int widget) const
	{
		switch (widget) {
		case WID_PLN_LIST: {
			uint y = r.top + WD_FRAMERECT_TOP; // Offset from top of widget.
			if (this->vscroll->GetCount() == 0) {
				DrawString(r.left + WD_FRAMETEXT_LEFT, r.right - WD_FRAMETEXT_RIGHT, y, STR_STATION_LIST_NONE);
				return;
			}

			bool rtl = _current_text_dir == TD_RTL;
			uint icon_left  = (rtl ? r.right - WD_FRAMERECT_RIGHT - this->company_icon_spr_dim.width : WD_FRAMETEXT_LEFT + r.left);
			uint btn_left   = (rtl ? icon_left - SETTING_BUTTON_WIDTH - 4 : icon_left + this->company_icon_spr_dim.width + 4);
			uint text_left  = (rtl ? r.left + WD_FRAMERECT_LEFT : btn_left + SETTING_BUTTON_WIDTH + 4);
			uint text_right = (rtl ? btn_left - 4 : r.right - WD_FRAMERECT_RIGHT);
			const_cast<PlansWindow*>(this)->vis_btn_left = btn_left;

			for (uint16 i = this->vscroll->GetPosition(); this->vscroll->IsVisible(i) && i < this->vscroll->GetCount(); i++) {
				Plan *p = Plan::Get(list[i].plan_id);

				if (i == this->selected) GfxFillRect(r.left + 1, y, r.right, y + this->resize.step_height, PC_DARK_GREY);

				if (list[i].is_plan) {
					DrawCompanyIcon(p->owner, icon_left, y + (this->resize.step_height - this->company_icon_spr_dim.height) / 2);
					DrawBoolButton(btn_left, y + (this->resize.step_height - SETTING_BUTTON_HEIGHT) / 2, p->visible, true);
					SetDParam(0, list[i].plan_id + 1);
					SetDParam(1, p->lines.size());
					SetDParam(2, p->creation_date);
					DrawString(text_left, text_right, y + (this->resize.step_height - FONT_HEIGHT_NORMAL) / 2, STR_PLANS_LIST_ITEM_PLAN, p->visible_by_all ? TC_LIGHT_BLUE : TC_YELLOW);
				}
				else {
					PlanLine *pl = p->lines[list[i].line_id];
					DrawBoolButton(btn_left, y + (this->resize.step_height - SETTING_BUTTON_HEIGHT) / 2, pl->visible, true);
					SetDParam(0, list[i].line_id + 1);
					SetDParam(1, pl->tiles.size() - 1);
					DrawString(text_left, text_right, y + (this->resize.step_height - FONT_HEIGHT_NORMAL) / 2, STR_PLANS_LIST_ITEM_LINE, TC_WHITE);
				}
				y += this->resize.step_height;
			}
			break;
		}
		}
	}

	virtual void OnResize()
	{
		this->vscroll->SetCapacityFromWidget(this, WID_PLN_LIST, WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM);
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
		case WID_PLN_LIST:
			this->company_icon_spr_dim = GetSpriteSize(SPR_COMPANY_ICON);
			resize->height = max<int>(FONT_HEIGHT_NORMAL, SETTING_BUTTON_HEIGHT);
			size->height = resize->height * 5 + WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM;
			break;
		}
	}

	/** The drawing of a line starts. */
	virtual void OnPlaceObject(Point pt, TileIndex tile)
	{
		/* A player can't add lines to a public plan of another company. */
		if (_current_plan && _current_plan->owner == _local_company) VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_DRAW_PLANLINE);
	}

	/** The drawing of a line is in progress. */
	virtual void OnPlaceDrag(ViewportPlaceMethod select_method, ViewportDragDropSelectionProcess select_proc, Point pt)
	{
		const Point p = GetTileBelowCursor();
		const TileIndex tile = TileVirtXY(p.x, p.y);
		if (_current_plan && tile < MapSize()) {
			_current_plan->StoreTempTile(tile);
			_thd.selstart = _thd.selend;
		}
	}

	/** The drawing of a line ends up normally. */
	virtual void OnPlaceMouseUp(ViewportPlaceMethod select_method, ViewportDragDropSelectionProcess select_proc, Point pt, TileIndex start_tile, TileIndex end_tile)
	{
		if (_current_plan) _current_plan->ValidateNewLine();
	}

	/** The drawing of a line is aborted. */
	virtual void OnPlaceObjectAbort()
	{
		if (_current_plan) {
			_current_plan->temp_line->MarkDirty();
			_current_plan->temp_line->Clear();
		}

		this->RaiseWidget(WID_PLN_ADD_LINES);
		this->SetWidgetDirty(WID_PLN_ADD_LINES);
	}

	void RebuildList()
	{
		int old_focused_plan_id = this->selected == INT_MAX ? INT_MAX : this->list[this->selected].plan_id;

		int sbcnt = 0;
		this->list.clear();
		Plan *p;
		FOR_ALL_PLANS(p) {
			if (!p->IsListable()) continue;

			ListItem li;
			li.is_plan = true;
			li.plan_id = p->index;
			this->list.push_back(li);
			if (old_focused_plan_id == p->index) this->selected = sbcnt;
			sbcnt++;

			if (p->show_lines) {
				const int sz = (int)p->lines.size();
				sbcnt += sz;
				li.is_plan = false;
				for (int i = 0; i < sz; i++) {
					li.line_id = i;
					this->list.push_back(li);
				}
			}
		}

		if (this->selected == INT_MAX) ResetObjectToPlace();

		this->vscroll->SetCount(sbcnt);
	}

	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		if (data != INVALID_PLAN && this->selected != INT_MAX) {
			if (this->list[this->selected].plan_id == data) {
				/* Invalidate the selection if the selected plan has been modified or deleted. */
				this->selected = INT_MAX;

				/* Cancel drawing associated to the deleted plan. */
				ResetObjectToPlace();
			}
		}

		RebuildList();
	}

	void SelectPlan(PlanID plan_index)
	{
		if (this->selected != INT_MAX) {
			if (plan_index == this->list[this->selected].plan_id) return;
			Plan::Get(this->list[this->selected].plan_id)->SetFocus(false);
		}

		if (plan_index == INVALID_PLAN) {
			this->selected = INT_MAX;
			return;
		}
		Plan::Get(plan_index)->SetFocus(true);

		for (size_t i = 0; i < this->list.size(); i++) {
			if (this->list[i].is_plan && this->list[i].plan_id == plan_index) {
				this->selected = (int)i;
				return;
			}
		}
	}
};

/** Show the window to manage plans. */
void ShowPlansWindow()
{
	if (BringWindowToFrontById(WC_PLANS, 0) != NULL) return;
	new PlansWindow(&_plans_desc);
}

/**
* Only the creator of a plan executes this function.
* The other players should not be bothered with these changes.
*/
void CcAddPlan(const CommandCost &result, TileIndex tile, uint32 p1, uint32 p2)
{
	if (result.Failed()) return;

	_current_plan = _new_plan;
	_current_plan->SetVisibility(true);

	Window *w = FindWindowById(WC_PLANS, 0);
	if (w) {
		w->InvalidateData(INVALID_PLAN, false);
		((PlansWindow *)w)->SelectPlan(_current_plan->index);
		if (!w->IsWidgetLowered(WID_PLN_ADD_LINES)) {
			w->SetWidgetDisabledState(WID_PLN_ADD_LINES, false);
			HandlePlacePushButton(w, WID_PLN_ADD_LINES, SPR_CURSOR_MOUSE, HT_POINT);
		}
	}
}
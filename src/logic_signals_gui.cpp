/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file logic_signals_gui.cpp A Window for editing signal programs. */

#include "logic_signals.h"
#include "window_type.h"
#include "window_gui.h"
#include "table/strings.h"
#include "command_func.h"
#include "tilehighlight_func.h"
#include "table/sprites.h"
#include "rail_map.h"
#include "strings_func.h"

/**
 * Definition of widgets
 */
enum ProgramSignalWidgets {
	WID_PROGSIG_OWN_DEFAULT_COLOR_RED,
	WID_PROGSIG_OWN_DEFAULT_COLOR_GREEN,
	WID_PROGSIG_TRIGGER_COLOR_RED,
	WID_PROGSIG_TRIGGER_COLOR_GREEN,
	WID_PROGSIG_OPERATOR_OR,
	WID_PROGSIG_OPERATOR_AND,
	WID_PROGSIG_OPERATOR_NAND,
	WID_PROGSIG_OPERATOR_XOR,
	WID_PROGSIG_LINK_COUNT,
	WID_PROGSIG_ADD_LINK,
	WID_PROGSIG_CLEAR_LINKS
};

/**
 * The Window used for editing signal programs of logic signals.
 */
struct SignalProgramWindow : Window
{
	SignalProgram *program;
	bool add_link_button;

	/**
	 * Constructor
	 */
	SignalProgramWindow(WindowDesc *desc, WindowNumber window_number, SignalProgram *prog) : Window(desc)
	{
		this->program = prog;
		this->add_link_button = false;
		this->InitNested(window_number);
		this->OnInvalidateData();
	}

	/**
	 * Handler which is executed whenever user clicks on the window.
	 */
	virtual void OnClick(Point pt, int widget, int click_count)
	{
		uint32 p1 = 0, p2 = 0;
		bool changed = false;

		SB(p1, 0, 3, program->track);

		switch (widget) {
			case WID_PROGSIG_OWN_DEFAULT_COLOR_RED:
				if (this->program->own_default_state != SIGNAL_STATE_RED) {
					SB(p1, 3, 3, 1);
					SB(p1, 6, 2, SIGNAL_STATE_RED);
					changed = true;
				}
				break;
			case WID_PROGSIG_OWN_DEFAULT_COLOR_GREEN:
				if (this->program->own_default_state != SIGNAL_STATE_GREEN) {
					SB(p1, 3, 3, 1);
					SB(p1, 6, 2, SIGNAL_STATE_GREEN);
					changed = true;
				}
				break;
			case WID_PROGSIG_TRIGGER_COLOR_RED:
				if (this->program->trigger_state != SIGNAL_STATE_RED) {
					SB(p1, 3, 3, 2);
					SB(p1, 6, 2, SIGNAL_STATE_RED);
					changed = true;
				}
				break;
			case WID_PROGSIG_TRIGGER_COLOR_GREEN:
				if (this->program->trigger_state != SIGNAL_STATE_GREEN) {
					SB(p1, 3, 3, 2);
					SB(p1, 6, 2, SIGNAL_STATE_GREEN);
					changed = true;
				}
				break;
			case WID_PROGSIG_OPERATOR_OR:
				if (this->program->signal_op != SIGNAL_OP_OR) {
					SB(p1, 3, 3, 3);
					SB(p1, 6, 2, SIGNAL_OP_OR);
					changed = true;
				}
				break;
			case WID_PROGSIG_OPERATOR_AND:
				if (this->program->signal_op != SIGNAL_OP_AND) {
					SB(p1, 3, 3, 3);
					SB(p1, 6, 2, SIGNAL_OP_AND);
					changed = true;
				}
				break;
			case WID_PROGSIG_OPERATOR_NAND:
				if (this->program->signal_op != SIGNAL_OP_NAND) {
					SB(p1, 3, 3, 3);
					SB(p1, 6, 2, SIGNAL_OP_NAND);
					changed = true;
				}
				break;
			case WID_PROGSIG_OPERATOR_XOR:
				if (this->program->signal_op != SIGNAL_OP_XOR) {
					SB(p1, 3, 3, 3);
					SB(p1, 6, 2, SIGNAL_OP_XOR);
					changed = true;
				}
				break;
			case WID_PROGSIG_ADD_LINK:
				SetWidgetDirty(WID_PROGSIG_ADD_LINK);
				ToggleWidgetLoweredState(WID_PROGSIG_ADD_LINK);
				if (IsWidgetLowered(WID_PROGSIG_ADD_LINK)) {
					SetObjectToPlaceWnd(SPR_CURSOR_TRANSMITTER, PAL_NONE, HT_RECT, this);
				} else {
					ResetObjectToPlace();
				}
				break;
			case WID_PROGSIG_CLEAR_LINKS:
				if (this->program->LinkCount() > 0) {
					SB(p1, 3, 2, 5);
					changed = true;
				}
				break;
		}

		if (changed) DoCommandP(program->tile, p1, p2, CMD_PROGRAM_LOGIC_SIGNAL | CMD_MSG(STR_ERROR_PROGRAM_SIGNAL_HEADER));
	}

	/**
	 * Handler which is executed whenever data has become invalid on the window.
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		if (!gui_scope) return;

		this->SetWidgetLoweredState(WID_PROGSIG_OWN_DEFAULT_COLOR_RED, this->program->own_default_state == SIGNAL_STATE_RED);
		this->SetWidgetLoweredState(WID_PROGSIG_OWN_DEFAULT_COLOR_GREEN, this->program->own_default_state == SIGNAL_STATE_GREEN);
		this->SetWidgetLoweredState(WID_PROGSIG_TRIGGER_COLOR_RED, this->program->trigger_state == SIGNAL_STATE_RED);
		this->SetWidgetLoweredState(WID_PROGSIG_TRIGGER_COLOR_GREEN, this->program->trigger_state == SIGNAL_STATE_GREEN);
		this->SetWidgetLoweredState(WID_PROGSIG_OPERATOR_OR, this->program->signal_op == SIGNAL_OP_OR);
		this->SetWidgetLoweredState(WID_PROGSIG_OPERATOR_AND, this->program->signal_op == SIGNAL_OP_AND);
		this->SetWidgetLoweredState(WID_PROGSIG_OPERATOR_NAND, this->program->signal_op == SIGNAL_OP_NAND);
		this->SetWidgetLoweredState(WID_PROGSIG_OPERATOR_XOR, this->program->signal_op == SIGNAL_OP_XOR);
	}

	/**
	 * Used to set dynamic string parameters to the widget.
	 */
	virtual void SetStringParameters(int widget) const
	{
		switch (widget) {
			case WID_PROGSIG_LINK_COUNT:
				SetDParam(0, program->LinkCount());
				break;
		}
	}

	/**
	 * Executed whenever user tries to link two signals together
	 */
	virtual void OnPlaceObject(Point pt, TileIndex tile)
	{
		uint32 p1 = 0;

		SB(p1, 0, 3, this->program->track);
		SB(p1, 3, 3, 4);

		DoCommandP(this->program->tile, p1, tile, CMD_PROGRAM_LOGIC_SIGNAL | CMD_MSG(STR_ERROR_LINK_SIGNAL_HEADER));
	}

	/**
	 * Executed when user aborts the linking of signals
	 */
	virtual void OnPlaceObjectAbort()
	{
		SetWidgetLoweredState(WID_PROGSIG_ADD_LINK, false);
		SetWidgetDirty(WID_PROGSIG_ADD_LINK);
	}
};

/**
 * Widget structure definition for programmable signals
 */
static const NWidgetPart _nested_program_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY), SetDataTip(STR_PROGSIG_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_GREY),
		NWidget(WWT_DEFSIZEBOX, COLOUR_GREY),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_GREY),
		NWidget(NWID_HORIZONTAL), SetPIP(3, 0, 0),
				NWidget(WWT_TEXT, COLOUR_GREY), SetMinimalSize(200, 14), SetFill(1, 0), SetDataTip(STR_PROGSIG_OWN_DEFAULT_COLOR, STR_PROGSIG_OWN_DEFAULT_COLOR_TOOLTIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_PROGSIG_OWN_DEFAULT_COLOR_RED), SetMinimalSize(80, 14), SetFill(1, 0), SetDataTip(STR_PROGSIG_COLOR_RED, STR_PROGSIG_OWN_DEFAULT_COLOR_TOOLTIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_PROGSIG_OWN_DEFAULT_COLOR_GREEN), SetMinimalSize(80, 14), SetFill(1, 0), SetDataTip(STR_PROGSIG_COLOR_GREEN, STR_PROGSIG_OWN_DEFAULT_COLOR_TOOLTIP),
		EndContainer(),
		NWidget(NWID_HORIZONTAL), SetPIP(3, 0, 0),
				NWidget(WWT_TEXT, COLOUR_GREY), SetMinimalSize(200, 14), SetFill(1, 0), SetDataTip(STR_PROGSIG_TRIGGER_COLOR, STR_PROGSIG_TRIGGER_COLOR_TOOLTIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_PROGSIG_TRIGGER_COLOR_RED), SetMinimalSize(80, 14), SetFill(1, 0), SetDataTip(STR_PROGSIG_COLOR_RED, STR_PROGSIG_TRIGGER_COLOR_TOOLTIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_PROGSIG_TRIGGER_COLOR_GREEN), SetMinimalSize(80, 14), SetFill(1, 0), SetDataTip(STR_PROGSIG_COLOR_GREEN, STR_PROGSIG_TRIGGER_COLOR_TOOLTIP),
		EndContainer(),
		NWidget(NWID_HORIZONTAL), SetPIP(3, 0, 0),
				NWidget(WWT_TEXT, COLOUR_GREY), SetMinimalSize(200, 14), SetFill(1, 0), SetDataTip(STR_PROGSIG_OPERATOR, STR_PROGSIG_OPERATOR_TOOLTIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_PROGSIG_OPERATOR_OR), SetMinimalSize(40, 14), SetFill(1, 0), SetDataTip(STR_PROGSIG_OP_OR, STR_PROGSIG_OPERATOR_TOOLTIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_PROGSIG_OPERATOR_AND), SetMinimalSize(40, 14), SetFill(1, 0), SetDataTip(STR_PROGSIG_OP_AND, STR_PROGSIG_OPERATOR_TOOLTIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_PROGSIG_OPERATOR_XOR), SetMinimalSize(40, 14), SetFill(1, 0), SetDataTip(STR_PROGSIG_OP_XOR, STR_PROGSIG_OPERATOR_TOOLTIP),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_PROGSIG_OPERATOR_NAND), SetMinimalSize(40, 14), SetFill(1, 0), SetDataTip(STR_PROGSIG_OP_NAND, STR_PROGSIG_OPERATOR_TOOLTIP),
		EndContainer(),
		NWidget(NWID_HORIZONTAL), SetPIP(3, 0, 0),
				NWidget(WWT_TEXT, COLOUR_GREY), SetMinimalSize(200, 14), SetFill(1, 0), SetDataTip(STR_PROGSIG_LINKED_SIGNALS, STR_PROGSIG_LINKED_SIGNALS_TOOLTIP),
				NWidget(WWT_TEXT, COLOUR_ORANGE, WID_PROGSIG_LINK_COUNT), SetMinimalSize(160, 14), SetFill(1, 0), SetDataTip(STR_JUST_INT, STR_PROGSIG_LINKED_SIGNALS_TOOLTIP),
		EndContainer(),
	EndContainer(),
	NWidget(NWID_HORIZONTAL, NC_EQUALSIZE),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_PROGSIG_ADD_LINK), SetFill(1, 0), SetDataTip(STR_PROGSIG_ADD_LINK, STR_PROGSIG_ADD_LINK_TOOLTIP),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_PROGSIG_CLEAR_LINKS), SetFill(1, 0), SetDataTip(STR_PROGSIG_CLEAR_LINKS, STR_PROGSIG_CLEAR_LINKS_TOOLTIP),
	EndContainer(),
};

static WindowDesc _signal_program_desc (
	WDP_AUTO, NULL, 0, 0,
	WC_SIGNAL_PROGRAM, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_program_widgets, lengthof(_nested_program_widgets)
);

/**
 * Displays a signal program window.
 * @param program The target signal program which this window modifies.
 */
void ShowSignalProgramWindow(SignalProgram *program)
{
	WindowNumber wnum = GetSignalReference(program->tile, program->track);

	if (BringWindowToFrontById(WC_SIGNAL_PROGRAM, wnum) != NULL) return;

	new SignalProgramWindow(&_signal_program_desc, wnum, program);
}

/* $Id: airport_gui.cpp 21909 2011-01-26 08:14:36Z rubidium $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport_gui.cpp The GUI for airports. */

#include "stdafx.h"
#include "window_gui.h"
#include "window_func.h"
#include "strings_func.h"
#include "company_func.h"
#include "tilehighlight_func.h"
#include "company_base.h"
#include "widgets/dropdown_type.h"
#include "core/geometry_func.hpp"
#include "hotkeys.h"
#include "sprite.h"

#include "table/strings.h"
#include "network/network_type.h"
#include "network/network_func.h"
#include "textbuf_gui.h"
#include "cargotype.h"
#include "console_func.h"

static const uint EXP_LINESPACE  = 2;      ///< Amount of vertical space for a horizontal (sub-)total line.
static const uint EXP_BLOCKSPACE = 10;     ///< Amount of vertical space between two blocks of numbers.
/** Widget number of the airport build window. */
enum CargosToolbarWidgets {
	CTW_BACKGROUND,
	CTW_CARGO_CAPTION,
	CTW_CARGO_HEADER,
	CTW_CARGO_LIST,
	CTW_CARGO_AMOUNT,
	CTW_CARGO_INCOME,
};

static void DrawPrice(Money amount, int left, int right, int top)
{
	SetDParam(0, amount);
	DrawString(left, right, top, STR_FINANCES_POSITIVE_INCOME, TC_FROMSTRING, SA_RIGHT);
}

/** Airport build toolbar window handler. */
struct CargosToolbarWindow : Window {


	CargosToolbarWindow(const WindowDesc *desc, WindowNumber window_number) : Window()
	{
		this->InitNested(desc, window_number);
		//this->cargo_option = CTW_OPTION_CARGO_AMOUNT;
		//this->DisableWidget(CTW_CHOOSE_CARGO_INCOME);
		this->owner = (Owner)this->window_number;
	}

	~CargosToolbarWindow() //~
	{
    //this->owner = (Owner)this->window_number;
	}

  virtual void SetStringParameters(int widget) const
	{
		switch (widget) {
			case CTW_CARGO_CAPTION:
				SetDParam(0, (CompanyID)this->window_number);
				SetDParam(1, (CompanyID)this->window_number);
				break;
		}
	}

  void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
  {
      switch(widget){
        case CTW_CARGO_HEADER:
    			size->width = 288;
          size->height = EXP_BLOCKSPACE + EXP_LINESPACE;
        break;

        case CTW_CARGO_LIST:
        case CTW_CARGO_AMOUNT:
        case CTW_CARGO_INCOME:
          size->width = 96;
          size->height = (_sorted_standard_cargo_specs_size + 3) * (EXP_BLOCKSPACE + EXP_LINESPACE);
        break;
    	}
  }

  	void DrawWidget(const Rect &r, int widget) const
  	{
      int rect_x = (r.left + WD_FRAMERECT_LEFT);
      int y = r.top;
      const Company *c = Company::Get((CompanyID)this->window_number);
      Money sum_cargo_amount = 0;
      Money sum_cargo_income = 0;

  		switch(widget){
          
        case CTW_CARGO_HEADER:
          DrawString(r.left, r.right, y, "Cargo", TC_BLACK, SA_LEFT);
          DrawString(r.left, r.right, y, "Amount", TC_BLACK, SA_CENTER);
          DrawString(r.left, r.right, y, "Income", TC_BLACK, SA_RIGHT);
          y += EXP_BLOCKSPACE + EXP_LINESPACE;
        break;
        
        case CTW_CARGO_LIST:
          y += 5; //top padding
          for (int i = 0; i < _sorted_standard_cargo_specs_size; i++) {
            const CargoSpec *cargos = _sorted_cargo_specs[i];
            //coloured cargo marks
        		GfxFillRect(rect_x, y, rect_x + 8, y + 5, 0);
        		GfxFillRect(rect_x + 1, y + 1, rect_x + 7, y + 4, cargos->legend_colour);
            //cargo name
            SetDParam(0, cargos->name);
            DrawString(r.left + 14, r.right, y, STR_TOOLBAR_CARGO_NAME, TC_FROMSTRING, SA_LEFT);
            //padding
            y += EXP_BLOCKSPACE + EXP_LINESPACE;
          }

          GfxFillRect(rect_x, y, rect_x + 96, y, 0);
          y += EXP_BLOCKSPACE + EXP_LINESPACE;
          DrawString(r.left, r.right, y, "Total", TC_BLACK, SA_LEFT);
        break;
        case CTW_CARGO_AMOUNT:
          y += 5;
          for (int i = 0; i < _sorted_standard_cargo_specs_size; i++) {
            const CargoSpec *cargos = _sorted_cargo_specs[i];
            //cargo amount in pcs
            SetDParam(0,  c->cargo_units[cargos->Index()]);
    		    DrawString(r.left, r.right, y, STR_TOOLBAR_CARGO_UNITS, TC_FROMSTRING, SA_RIGHT);
    		    sum_cargo_amount += c->cargo_units[cargos->Index()];
    		    y += EXP_BLOCKSPACE + EXP_LINESPACE;
          }
          GfxFillRect(rect_x, y, rect_x + 96, y, 0);
          y += EXP_BLOCKSPACE + EXP_LINESPACE;
          SetDParam(0, sum_cargo_amount);
          DrawString(r.left, r.right, y, STR_TOOLBAR_CARGO_UNITS, TC_FROMSTRING, SA_RIGHT);
        break;
        case CTW_CARGO_INCOME:
          y += 5;
          for (int i = 0; i < _sorted_standard_cargo_specs_size; i++) {
            const CargoSpec *cargos = _sorted_cargo_specs[i];
            //cargo income in money
            DrawPrice(Money(c->cargo_income[cargos->Index()]), r.left, r.right, y);
            sum_cargo_income += c->cargo_income[cargos->Index()];
            y += EXP_BLOCKSPACE + EXP_LINESPACE;
          }
          //total
          GfxFillRect(rect_x, y, rect_x + 96, y, 0);
          y += EXP_BLOCKSPACE + EXP_LINESPACE;
          DrawPrice(sum_cargo_income, r.left, r.right, y);
        break;
    	}
  	}

  virtual void OnHundredthTick()
	{
		/* redraw the window every now and then */
		this->ReInit();
		//this->SetDirty();
	}
  
  };

static const NWidgetPart _nested_cargos_toolbar_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY, CTW_CARGO_CAPTION), SetDataTip(STR_TOOLBAR_CARGOS_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_STICKYBOX, COLOUR_GREY),
	EndContainer(),
	NWidget(NWID_SELECTION, INVALID_COLOUR),
		NWidget(WWT_PANEL, COLOUR_GREY),
			NWidget(NWID_HORIZONTAL), SetPadding(WD_FRAMERECT_TOP, WD_FRAMERECT_RIGHT, WD_FRAMERECT_BOTTOM, WD_FRAMERECT_LEFT), SetPIP(0, 9, 0),
			    NWidget(WWT_EMPTY, COLOUR_GREY, CTW_CARGO_HEADER),SetMinimalSize(288, 16),SetFill(0, 0),
			EndContainer(),
		EndContainer(),
	EndContainer(),
	NWidget(NWID_SELECTION, INVALID_COLOUR),
		NWidget(WWT_PANEL, COLOUR_GREY),
			NWidget(NWID_HORIZONTAL), SetPadding(WD_FRAMERECT_TOP, WD_FRAMERECT_RIGHT, WD_FRAMERECT_BOTTOM, WD_FRAMERECT_LEFT), SetPIP(0, 9, 0),
			    NWidget(WWT_EMPTY, COLOUR_GREY, CTW_CARGO_LIST),SetMinimalSize(96, 0),SetFill(0, 0),
			    NWidget(WWT_EMPTY, COLOUR_GREY, CTW_CARGO_AMOUNT),SetMinimalSize(96, 0),SetFill(0, 0),
			    NWidget(WWT_EMPTY, COLOUR_GREY, CTW_CARGO_INCOME),SetMinimalSize(96, 0),SetFill(0, 0),
			EndContainer(),
		EndContainer(),
	EndContainer(),
};

static const WindowDesc _cargos_toolbar_desc(
	WDP_AUTO, 0, 0,
	WC_CARGOS, WC_NONE,
	WDF_UNCLICK_BUTTONS,
	_nested_cargos_toolbar_widgets, lengthof(_nested_cargos_toolbar_widgets)
);

void ShowCompanyCargos(CompanyID company)
{
	if (!Company::IsValidID(company)) return;
	if (BringWindowToFrontById(WC_CARGOS, company)) return;  
	new CargosToolbarWindow(&_cargos_toolbar_desc, company);
}

/* $Id: cargo_table_gui.h 21700 2011-01-03 11:55:08Z  $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file company_gui.h GUI Functions related to companies. */

#ifndef CARGO_TABLE_H
#define CARGO_TABLE_H

#include "company_type.h"
#include "gfx_type.h"

void ShowCompanyCargos(CompanyID company);
//Window ShowCompanyCargos(CompanyID company);
void InvalidateCompanyWindows(const Company *c);
void DeleteCompanyWindows(CompanyID company);

#endif /* CARGO_TABLE_H */

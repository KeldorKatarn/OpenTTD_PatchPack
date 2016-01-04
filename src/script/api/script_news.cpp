/* $Id: script_news.cpp 24982 2013-02-08 20:34:27Z rubidium $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_news.cpp Implementation of ScriptNews. */

#include "../../stdafx.h"
#include "script_news.hpp"
#include "script_error.hpp"
#include "../../command_type.h"
#include "../../string_func.h"

/* static */ bool ScriptNews::Create(NewsType type, Text *text, ScriptCompany::CompanyID company)
{
	CCountedPtr<Text> counter(text);

	EnforcePrecondition(false, text != NULL);
	const char *encoded = text->GetEncodedText();
	EnforcePreconditionEncodedText(false, encoded);
	EnforcePrecondition(false, type == NT_ECONOMY || type == NT_SUBSIDIES || type == NT_GENERAL);
	EnforcePrecondition(false, company == ScriptCompany::COMPANY_INVALID || ScriptCompany::ResolveCompanyID(company) != ScriptCompany::COMPANY_INVALID);

	uint8 c = company;
	if (company == ScriptCompany::COMPANY_INVALID) c = INVALID_COMPANY;

	return ScriptObject::DoCommand(0, type | (NR_NONE << 8) | (c << 16), 0, CMD_CUSTOM_NEWS_ITEM, encoded);
}

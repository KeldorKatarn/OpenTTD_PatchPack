/* $Id: script_news.hpp 24286 2012-05-26 14:16:12Z frosch $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_news.hpp Everything to handle news messages. */

#ifndef SCRIPT_NEWS_HPP
#define SCRIPT_NEWS_HPP

#include "script_company.hpp"
#include "../../news_type.h"

/**
 * Class that handles news messages.
 * @api game
 */
class ScriptNews : public ScriptObject {
public:
	/**
	 * Enumeration for the news types that a script can create news for.
	 */
	enum NewsType {
		/* Arbitrary selection of NewsTypes which might make sense for scripts */
		NT_ACCIDENT          = ::NT_ACCIDENT,         ///< Category accidents.
		NT_COMPANY_INFO      = ::NT_COMPANY_INFO,     ///< Category company info.
		NT_ECONOMY           = ::NT_ECONOMY,          ///< Category economy.
		NT_ADVICE            = ::NT_ADVICE,           ///< Category vehicle advice.
		NT_ACCEPTANCE        = ::NT_ACCEPTANCE,       ///< Category acceptance changes.
		NT_SUBSIDIES         = ::NT_SUBSIDIES,        ///< Category subsidies.
		NT_GENERAL           = ::NT_GENERAL,          ///< Category general.
	};

	/**
	 * Create a news messages for a company.
	 * @param type The type of the news.
	 * @param text The text message to show (can be either a raw string, or a ScriptText object).
	 * @param company The company, or COMPANY_INVALID for all companies.
	 * @return True if the action succeeded.
	 * @pre text != NULL.
	 * @pre company == COMPANY_INVALID || ResolveCompanyID(company) != COMPANY_INVALID.
	 */
	static bool Create(NewsType type, Text *text, ScriptCompany::CompanyID company);
};

#endif /* SCRIPT_NEWS_HPP */

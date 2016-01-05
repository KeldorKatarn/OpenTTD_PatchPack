/* $Id: script_viewport.cpp 23737 2012-01-03 20:37:56Z truebrain $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_viewport.cpp Implementation of ScriptViewport. */

#include "../../stdafx.h"
#include "script_viewport.hpp"
#include "script_game.hpp"
#include "script_map.hpp"
#include "../../viewport_func.h"

#include "../../safeguards.h"

/* static */ void ScriptViewport::ScrollTo(TileIndex tile)
{
	if (ScriptGame::IsMultiplayer()) return;
	if (!ScriptMap::IsValidTile(tile)) return;

	ScrollMainWindowToTile(tile);
}

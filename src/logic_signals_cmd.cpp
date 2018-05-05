/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file logic_signals_cmd.cpp Commands for modifying logic signal programs. */

#include "logic_signals.h"
#include "core/bitmath_func.hpp"
#include "window_func.h"
#include "command_func.h"
#include "rail_map.h"
#include "table/strings.h"
#include "company_func.h"

/**
 * The main command for editing a signal program.
 * @param tile The tile which contains the edited signal.
 * @param flags Internal command handler stuff.
 * @param p1 Bitstuffed items
 * - p1 = (bit 0-2) - The Track part of the signal program tile.
 * - p1 = (bit 3-5) - Subcommand to execute.
 * - p1 = (bit 6-7) - The value to set to the signal program.
 * @param p2 Target SignalReference for linking of two signals
 * @param text Unused
 * @return the cost of this operation (which is free), or an error
 */
CommandCost CmdProgramLogicSignal(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char* text)
{
	const auto track = Extract<Track, 0, 3>(p1);
	const uint32 sub_cmd = GB(p1, 3, 3);
	const uint32 value = GB(p1, 6, 2);

	// Start by checking tile ownership.
	CommandCost ret = CheckTileOwnership(tile);
	if (ret.Failed()) return ret;

	SignalProgram* program = FindSignalProgram(tile, track);

	if (sub_cmd == 1) {
		if (flags & DC_EXEC) {
			program->own_default_state = static_cast<SignalState>(value);
		}
	} else if (sub_cmd == 2) {
		if (flags & DC_EXEC) {
			program->trigger_state = static_cast<SignalState>(value);
		}
	} else if (sub_cmd == 3) {
		if (flags & DC_EXEC) {
			program->signal_op = static_cast<SignalOperator>(value);
		}
	} else if (sub_cmd == 4) {
		const TileIndex target_tile = p2;
		const Track target_track = SignalTrackFromTile(target_tile);

		if (tile == target_tile && track == target_track) {
			return CommandError(STR_ERROR_LINK_SIGNAL_TO_ITSELF);
		}

		if (!IsPlainRailTile(target_tile) || !HasSignalOnTrack(target_tile, target_track)) {
			return CommandError(STR_ERROR_LINK_SIGNAL_NO_SIGNAL);
		}

		if (CheckTileOwnership(target_tile).Failed()) {
			return CommandError(STR_ERROR_OWNED_BY);
		}

		if (flags & DC_EXEC) {
			program->AddLink(target_tile, target_track);
		}
	} else if (sub_cmd == 5) {
		if (flags & DC_EXEC) {
			program->ClearAllLinks();
		}
	}

	if (flags & DC_EXEC) {
		// Invalidate any open windows if something was changed.
		InvalidateWindowData(WC_SIGNAL_PROGRAM, GetSignalReference(tile, track));

		// Re-evaluate the signal state too.
		program->InputChanged(1);
	}

	// No cost, this fun is free :)
	return CommandCost();
}

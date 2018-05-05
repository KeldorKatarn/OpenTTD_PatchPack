/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file logic_signal_program.cpp Implementation of the SignalProgram class functions. */

#include "logic_signals.h"
#include "overlay_cmd.h"
#include "rail_map.h"
#include "viewport_func.h"

#include <algorithm>

/**
 * Return the opposite of the given signal state.
 * @param state The original signal state to revert.
 * @return Green if the given argument is Red and vice versa.
 */
static SignalState OppositeSignalState(SignalState state)
{
	return state == SIGNAL_STATE_RED ? SIGNAL_STATE_GREEN : SIGNAL_STATE_RED;
}

/**
 * The maximum number of signal programs which are evaluated in succession.
 */
static const int MAX_LOGIC_SIGNAL_RECURSIONS = 5;

/**
 * The constructor for creating a new signal program.
 * @param tile The tile where the logic signal for this program is located.
 * @param track The track where the logic signal for this program is located.
 */
SignalProgram::SignalProgram(TileIndex tile, Track track)
{
	this->tile = tile;
	this->track = track;

	// Default to a priority signal: if any of the linked input
	// signals are red, this one goes red.
	this->own_default_state = SIGNAL_STATE_GREEN;
	this->trigger_state = SIGNAL_STATE_RED;
	this->signal_op = SIGNAL_OP_OR;

	this->blocked_by_train = false;
}

/**
 * Add a new signal as input for this signal program.
 * @param tile The tile of the signal to be linked.
 * @param track The track of the signal to be linked.
 */
void SignalProgram::AddLink(TileIndex tile, Track track)
{
	const SignalReference input = GetSignalReference(tile, track);

	if (std::find(this->linked_signals.begin(), this->linked_signals.end(), input) == this->linked_signals.end()) {
		this->linked_signals.push_back(input);
		_signal_link_list.push_back(std::make_pair(input, GetSignalReference(this->tile, this->track)));
	}

	Overlays::Instance()->RefreshLogicSignalOverlay();
}

/**
 * Remove a linked signal from this program. The link must exist.
 * @param tile The tile of the signal to be removed.
 * @param track The track of the signal to be removed.
 */
void SignalProgram::RemoveLink(TileIndex tile, Track track)
{
	// Refresh BEFORE because we need to know what tiles to refresh before we remove the reference to them.
	Overlays::Instance()->RefreshLogicSignalOverlay();

	SignalReference input_to_remove = GetSignalReference(tile, track);
	const auto found_input = std::find(this->linked_signals.begin(), this->linked_signals.end(), input_to_remove);

	assert(found_input != this->linked_signals.end());

	this->linked_signals.erase(found_input);

	const auto link_to_remove = std::make_pair(input_to_remove, GetSignalReference(this->tile, this->track));
	const auto found_link = std::find(_signal_link_list.begin(), _signal_link_list.end(), link_to_remove);

	assert(found_link != _signal_link_list.end());

	_signal_link_list.erase(found_link);
}

/**
 * Remove all links that this signal program has.
 */
void SignalProgram::ClearAllLinks()
{
	// Refresh BEFORE because we need to know what tiles to refresh before we remove the reference to them.
	Overlays::Instance()->RefreshLogicSignalOverlay();
	auto this_signal = GetSignalReference(this->tile, this->track);

	_signal_link_list.erase(std::remove_if(_signal_link_list.begin(), _signal_link_list.end(), [&](std::pair<SignalReference, SignalReference> link)
	{
		return link.second == this_signal;
	}));

	this->linked_signals.clear();
}

/**
* Return all signal input references.
*/
const std::list<SignalReference>& SignalProgram::GetSignalReferences() const
{
	return this->linked_signals;
}

/**
 * The number of signals linked to this signal program.
 */
size_t SignalProgram::LinkCount() const
{
	return this->linked_signals.size();
}

/**
 * This function is run when one of the signals linked to this program has changed.
 * It will (possibly) change the state of the signal, and then recursively change
 * the state of any signal linked to it.
 * @param depth The recursion depth, which starts at 1.
 */
void SignalProgram::InputChanged(int depth)
{
	// If this signal is blocked by a train, we can't do anything.
	if (this->blocked_by_train) {
		return;
	}

	const SignalState new_state = this->Evaluate();

	if (new_state != DetermineSignalState(this->tile, this->track)) {
		SetSignalStateForTrack(this->tile, this->track, new_state);
		MarkTileDirtyByTile(tile);

		// Recursively update any signals that have this one as input.
		if (depth < MAX_LOGIC_SIGNAL_RECURSIONS) {
			SignalStateChanged(this->tile, this->track, depth + 1);
		}
	}
}

/**
 * The main evaluation function which determines the state of the signal linked
 * to this signal program. It should be short, simple and readable.
 */
SignalState SignalProgram::Evaluate()
{
	int trigger_states = 0, not_trigger_states = 0;

	// We need at least one linked signal to evaluate anything.
	if (this->LinkCount() == 0) return this->own_default_state;

	// Loop through all linked signals and count the states.
	for (auto reference : this->linked_signals) {
		const TileIndex target_tile = GetTileFromSignalReference(reference);
		const Track target_track = GetTrackFromSignalReference(reference);
		const SignalState target_state = DetermineSignalState(target_tile, target_track);

		if (target_state == trigger_state) {
			trigger_states++;
		} else {
			not_trigger_states++;
		}
	}

	switch (this->signal_op) {
		case SIGNAL_OP_OR:
			// OR is triggered if we have at least one signal of trigger color.
			if (trigger_states > 0) return OppositeSignalState(this->own_default_state);
			break;

		case SIGNAL_OP_AND:
			// AND is triggered if no signals were of the 'wrong' color.
			if (not_trigger_states == 0) return OppositeSignalState(this->own_default_state);
			break;

		case SIGNAL_OP_NAND:
			// NAND is triggered if we have at least one signal of the 'wrong' color.
			if (not_trigger_states > 0) return OppositeSignalState(this->own_default_state);
			break;

		case SIGNAL_OP_XOR:
			// XOR is triggered if the number of signals in trigger color is uneven.
			if ((trigger_states % 2) > 0) return OppositeSignalState(this->own_default_state);
			break;

		default:
			break;
	}

	// Not triggered, return default color.
	return this->own_default_state;
}

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file logic_signals.cpp Utility functions of the Logic Signals patch. */

#include "logic_signals.h"
#include "overlay_cmd.h"
#include "rail_map.h"
#include "tile_cmd.h"
#include "viewport_func.h"
#include "window_func.h"

SignalProgramList _signal_program_list; //!< List of all signal programs.
SignalLinkList _signal_link_list;       //!< List of all links between signals.

/**
 * Remove all signal programs.
 */
void FreeAllSignalPrograms()
{
	for (auto it = _signal_program_list.begin(); it != _signal_program_list.end(); ++it) {
		SignalProgram* program = it->second;
		delete program;
	}

	_signal_program_list.clear();
	_signal_link_list.clear();
}

/**
 * Determine the color of signals at the given tile/track.
 * 
 * @param tile The tile where the tested signal is located.
 * @param track The track where the tested signal is located.
 * @return Red if either of the two possible signals is red.
 */
SignalState DetermineSignalState(TileIndex tile, Track track)
{
	assert(HasSignalOnTrack(tile, track));

	const uint signal_states = GetSignalStates(tile);
	const byte signal_mask_for_track = SignalOnTrack(track);
	const byte present_signals_on_tile = GetPresentSignals(tile);

	const byte present_signals_on_track = signal_mask_for_track & present_signals_on_tile;
	const byte signal_states_on_track = present_signals_on_track & signal_states;

	// We return red if one of the two possibly present signals is red.
	// Both need to be green for us to accept the tile as green.
	return (signal_states_on_track == present_signals_on_track) ? SIGNAL_STATE_GREEN : SIGNAL_STATE_RED;
}

/**
 * Set the signal state for the given tile and track to the specified state.
 * Both possible signals for the given tile/track are set to the given state.
 * 
 * @param tile Tile where the changed signal is located.
 * @param track Track where the changed signal is located.
 * @param state The new state to set to the signals.
 */
void SetSignalStateForTrack(TileIndex tile, Track track, SignalState state)
{
	const byte present_signals_on_tile = GetPresentSignals(tile);

	if (state == SIGNAL_STATE_GREEN) {
		SetSignalStates(tile, GetSignalStates(tile) | present_signals_on_tile);
	} else {
		SetSignalStates(tile, GetSignalStates(tile) & ~present_signals_on_tile);
	}
}

/**
 * Read a Track from a TileIndex.
 *
 * @param tile The tile where to read the Track from.
 * @return The read Track
 */
Track SignalTrackFromTile(TileIndex tile)
{
	TrackBits trackbits = TrackStatusToTrackBits(GetTileTrackStatus(tile, TRANSPORT_RAIL, 0));

	if (trackbits & TRACK_BIT_VERT) {
		// N-S direction
		trackbits = (_tile_fract_coords.x <= _tile_fract_coords.y) ? TRACK_BIT_RIGHT : TRACK_BIT_LEFT;
	}

	if (trackbits & TRACK_BIT_HORZ) {
		// E-W direction
		trackbits = (_tile_fract_coords.x + _tile_fract_coords.y <= 15) ? TRACK_BIT_UPPER : TRACK_BIT_LOWER;
	}

	return FindFirstTrack(trackbits);
}

/**
 * Combine TileIndex and Track to a SignalReference.
 * @param tile The tile to get a signal reference from
 * @param track The track to get a signal reference from
 * @return The signal reference made up of the given tile and track
 */
SignalReference GetSignalReference(TileIndex tile, Track track)
{
	return uint64(tile) | (uint64(track) << 32);
}

/**
 * Extract a TileIndex from a SignalReference.
 * @param key The signal reference to extract a TileIndex from
 * @return The TileIndex component of the given signal reference
 */
TileIndex GetTileFromSignalReference(SignalReference key)
{
	return GB(key, 0, 32);
}

/**
 * Extract a Track from a SignalReference.
 * @param key The signal reference to extract a Track from
 * @return The Track component of the given signal reference
 */
Track GetTrackFromSignalReference(SignalReference key)
{
	return static_cast<Track>(key >> 32);
}

/**
 * An internal helper function used in searching for a signal program.
 * @param tile The tile to search for
 * @param track The track to search for
 * @return The corresponding signal program, or nullptr if not found.
 */
static SignalProgram* DoFindSignalProgram(TileIndex tile, Track track)
{
	const auto found = _signal_program_list.find(GetSignalReference(tile, track));

	return (found != _signal_program_list.end()) ? found->second : nullptr;
}

/**
 * Finds all links from a signal at the given tile/track.
 * @param tile The tile to search for.
 * @param track The track to search for.
 * @return The corresponding targets where the link points to.
 */
static std::list<SignalReference> FindSignalLinks(TileIndex tile, Track track)
{
	const auto source = GetSignalReference(tile, track);

	std::list<SignalReference> targets;

	for (const auto link : _signal_link_list) {
		if (link.first == source) {
			targets.push_back(link.second);
		}
	}

	return targets;
}

/**
 * Used to find a signal program at the given tile and track.
 * @param tile The tile to search for.
 * @param track The track to search for.
 * @return The signal program if found, or nullptr.
 */
SignalProgram* FindSignalProgram(TileIndex tile, Track track)
{
	SignalProgram* program = DoFindSignalProgram(tile, track);

	assert(program != nullptr);

	return program;
}

/**
 * Remove a signal link at the given tile and track, if it is found.
 * It is perfectly valid to call this function even if the said tile/track has no outgoing link.
 *
 * @param tile The tile to search for
 * @param track The track to search for
 */
void RemoveSignalLink(TileIndex tile, Track track)
{
	auto targets = FindSignalLinks(tile, track);

	for (auto target : targets) {
		SignalProgram* program = FindSignalProgram(GetTileFromSignalReference(target),
												   GetTrackFromSignalReference(target));
		program->RemoveLink(tile, track);

		// Invalidate any windows which have this program open.
		InvalidateWindowData(WC_SIGNAL_PROGRAM, target);
	}
}

/**
 * Create a new signal program at the given tile and track.
 * Used when a new logic signal is created.
 * 
 * @param tile The tile of the logic signal
 * @param track The track of the logic signal
 * @return The newly created signal program
 */
SignalProgram* CreateSignalProgram(TileIndex tile, Track track)
{
	// Existing program for same tile/track should not exist.
	assert(DoFindSignalProgram(tile, track) == nullptr);

	const auto program = new SignalProgram(tile, track);
	_signal_program_list[GetSignalReference(tile, track)] = program;

	return program;
}

/**
 * Delete a signal program at the given tile and track.
 * 
 * @param tile The tile of the logic signal
 * @param track The track of the logic signal
 */
void DeleteSignalProgram(TileIndex tile, Track track)
{
	const SignalReference signal_reference = GetSignalReference(tile, track);

	// Delete any windows which have this program open.
	DeleteWindowById(WC_SIGNAL_PROGRAM, signal_reference, false);

	// Remove the actual program and all links attached to it.
	SignalProgram* program = FindSignalProgram(tile, track);

	Overlays::Instance()->HandleSignalProgramDeletion(program);

	_signal_program_list.erase(signal_reference);
	program->ClearAllLinks();

	delete program;
}

/**
 * Used to create or delete signal programs at the given tile when the signal type changes.
 * 
 * @param tile The tile where the change occurred.
 * @param track The track where the change occurred.
 * @param old_type The old type of the changed signal
 * @param new_type The new type of the changed signal
 */
void SignalTypeChanged(TileIndex tile, Track track, SignalType old_type, SignalType new_type)
{
	if (old_type == SIGTYPE_LOGIC) DeleteSignalProgram(tile, track);
	if (new_type == SIGTYPE_LOGIC) CreateSignalProgram(tile, track);
}

/**
 * Executed whenever signal state has changed by the main program.
 * 
 * @param tile Tile where the change occurred
 * @param track Track where the change occurred
 * @param depth Recursion depth, starts at 1.
 */
void SignalStateChanged(TileIndex tile, Track track, int depth)
{
	auto targets = FindSignalLinks(tile, track);

	for (auto target : targets) {
		SignalProgram* program = FindSignalProgram(GetTileFromSignalReference(target),
												   GetTrackFromSignalReference(target));
		program->InputChanged(depth);
	}
}

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file logic_signals.h Header file for the Logic Signals patch. */

#ifndef LOGIC_SIGNALS_H
#define LOGIC_SIGNALS_H

#include "stdafx.h"
#include "core/smallvec_type.hpp"
#include "tile_type.h"
#include "track_type.h"
#include "signal_type.h"
#include <map>

/**
 * Operators which logic signals can use when evaluating inputs.
 */
enum SignalOperator {
	SIGNAL_OP_OR,
	SIGNAL_OP_AND,
	SIGNAL_OP_NAND,
	SIGNAL_OP_XOR,
	SIGNAL_OP_END
};
/** Helper for saveload stuff. */
template <> struct EnumPropsT<SignalOperator> : MakeEnumPropsT<SignalOperator, byte, SIGNAL_OP_OR, SIGNAL_OP_END, SIGNAL_OP_END, 2> {};
typedef TinyEnumT<SignalOperator> SignalOperatorByte;

/**
 * The definition of a signal program reference.
 *
 * The first 22 bits (0 to 21) is the TileIndex and 3 bits 22 to 24 is the Track.
 * This will stop working if maps larger than 2048 x 2048 are used.
 */
typedef uint32 SignalReference;

/**
 * A simple container used to store multiple signal references.
 */
typedef SmallVector<SignalReference, 4> SignalReferenceList;

/**
 * The main class of a signal program.
 */
class SignalProgram {
private:
	SignalReferenceList linked_signals;

public:
	TileIndex tile;
	TrackByte track;

	SignalStateByte own_default_state;
	SignalStateByte trigger_state;
	SignalOperatorByte signal_op;

	bool blocked_by_train;

	SignalProgram();
	SignalProgram(TileIndex t, Track tr);
	void AddLink(TileIndex tile, Track track, bool remove_first);
	void RemoveLink(TileIndex tile, Track track);
	void ClearAllLinks();
	uint LinkCount();
	void InputChanged(int depth);
	SignalState Evaluate();
};

// Global variables
typedef std::map<SignalReference, SignalProgram*> SignalProgramList;
typedef std::map<SignalReference, SignalReference> SignalLinkList;
extern SignalProgramList _signal_program_list;
extern SignalLinkList _signal_link_list;

// Global functions in logic_signals.cpp
extern void FreeAllSignalPrograms();
extern inline SignalState DetermineSignalState(TileIndex tile, Track track);
extern inline void SetSignalStateForTrack(TileIndex tile, Track track, SignalState state);
extern inline Track SignalTrackFromTile(TileIndex tile);
extern inline SignalReference GetSignalReference(TileIndex tile, Track track);
extern inline TileIndex GetTileFromSignalReference(SignalReference key);
extern inline Track GetTrackFromSignalReference(SignalReference key);
extern SignalProgram * FindSignalProgram(TileIndex tile, Track track);
extern void RemoveSignalLink(TileIndex tile, Track track);
extern SignalProgram * CreateSignalProgram(TileIndex tile, Track track);
extern void DeleteSignalProgram(TileIndex tile, Track track);
extern inline void SignalTypeChanged(TileIndex tile, Track track, SignalType old_type, SignalType new_type);
extern inline void SignalStateChanged(TileIndex tile, Track track, int depth);

// Global functions in logic_signals_gui.cpp
extern void ShowSignalProgramWindow(SignalProgram *program);

#endif /* LOGIC_SIGNALS_H */

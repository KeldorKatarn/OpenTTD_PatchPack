/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file logic_signals_sl.cpp Implementation of saving and loading of signal programs. */

#include "../logic_signals.h"
#include "saveload.h"

struct TempStorage {
	SignalReference from;
	SignalReference to;
};

static const SaveLoad _signal_program_desc[] = {
	SLE_CONDVAR(SignalProgram, tile, SLE_UINT32, SL_PATCH_PACK_1_5, SL_MAX_VERSION),
	SLE_CONDVAR(SignalProgram, track, SLE_UINT8, SL_PATCH_PACK_1_5, SL_MAX_VERSION),
	SLE_CONDVAR(SignalProgram, own_default_state, SLE_UINT8, SL_PATCH_PACK_1_5, SL_MAX_VERSION),
	SLE_CONDVAR(SignalProgram, trigger_state, SLE_UINT8, SL_PATCH_PACK_1_5, SL_MAX_VERSION),
	SLE_CONDVAR(SignalProgram, signal_op, SLE_UINT8, SL_PATCH_PACK_1_5, SL_MAX_VERSION),
	SLE_CONDVAR(SignalProgram, blocked_by_train, SLE_UINT8, SL_PATCH_PACK_1_5, SL_MAX_VERSION),
	SLE_END()
};

static const SaveLoad _signal_link_desc[] = {
	SLE_CONDVAR(TempStorage, from, SLE_UINT32, SL_PATCH_PACK_1_5, SL_MAX_VERSION),
	SLE_CONDVAR(TempStorage, to, SLE_UINT32, SL_PATCH_PACK_1_5, SL_MAX_VERSION),
	SLE_END()
};

/**
 * Save signal programs.
 */
static void Save_SPRG()
{
	int index = 0;

	for (SignalProgramList::iterator it = _signal_program_list.begin(); it != _signal_program_list.end(); it++) {
		SlSetArrayIndex(index++);
		SlObject(it->second, _signal_program_desc);
	}
}

/**
 * Load signal programs.
 */
static void Load_SPRG()
{
	int index;

	while ((index = SlIterateArray()) != -1) {
		SignalProgram *program = new SignalProgram();
		SlObject(program, _signal_program_desc);
		_signal_program_list[GetSignalReference(program->tile, program->track)] = program;
	}
}

/**
 * Save signal links.
 */
static void Save_SLNK()
{
	TempStorage storage;
	int index = 0;

	for (SignalLinkList::iterator it = _signal_link_list.begin(); it != _signal_link_list.end(); it++) {
		SlSetArrayIndex(index++);
		storage.from = it->first;
		storage.to = it->second;
		SlObject(&storage, _signal_link_desc);
	}
}

/**
 * Load signal links.
 */
static void Load_SLNK()
{
	TempStorage storage;
	int index;

	while ((index = SlIterateArray()) != -1) {
		SlObject(&storage, _signal_link_desc);
		_signal_link_list[storage.from] = storage.to;
		SignalProgram *program = FindSignalProgram(GetTileFromSignalReference(storage.to), GetTrackFromSignalReference(storage.to));
		program->AddLink(GetTileFromSignalReference(storage.from), GetTrackFromSignalReference(storage.from), false);
	}
}

extern const ChunkHandler _logic_signal_handlers[] = {
	{  'SPRG', Save_SPRG, Load_SPRG, NULL, NULL, CH_ARRAY },
	{  'SLNK', Save_SLNK, Load_SLNK, NULL, NULL, CH_ARRAY | CH_LAST }
};

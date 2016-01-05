/* $Id: progress.h 22820 2011-08-24 12:18:53Z rubidium $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file progress.h Functions related to modal progress. */

#ifndef PROGRESS_H
#define PROGRESS_H

#include "thread/thread.h"

static const uint MODAL_PROGRESS_REDRAW_TIMEOUT = 200; ///< Timeout between redraws

/**
 * Check if we are currently in a modal progress state.
 * @return Are we in the modal state?
 */
static inline bool HasModalProgress()
{
	extern bool _in_modal_progress;
	return _in_modal_progress;
}

bool IsFirstModalProgressLoop();
void SetModalProgress(bool state);

extern class ThreadMutex *_modal_progress_work_mutex;
extern class ThreadMutex *_modal_progress_paint_mutex;

#endif /* PROGRESS_H */

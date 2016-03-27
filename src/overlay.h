/* $Id$ */

/** @file overlay.h Functions related to overlays. */

#ifndef OVERLAY_H
#define OVERLAY_H

#include "stdafx.h"
#include "openttd.h"
#include "core/bitmath_func.hpp"
#include "gfx_func.h"

/**
 * Transparency option bits: which position in _transparency_opt stands for which transparency.
 * If you change the order, change the order of the ShowTransparencyToolbar() stuff in transparency_gui.cpp too.
 * If you add or remove an option don't forget to change the transparency 'hot keys' in main_gui.cpp.
 */
enum OverlayOption {
	OO_COVERAGES = 0,  ///< coverage
	OO_END,
};

typedef uint OverlayOptionBits; ///< overlay option bits
extern OverlayOptionBits _overlay_opt;
extern OverlayOptionBits _overlay_lock;

/**
 * Check if the overlay option bit is set
 * and if we aren't in the game menu (there's no overlay)
 *
 * @param to the structure which overlay option is ask for
 */
static inline bool IsOverlaySet(OverlayOption to)
{
	return (HasBit(_overlay_opt, to) && _game_mode != GM_MENU);
}

/**
 * Toggle the overlay option bit
 *
 * @param to the overlay option to be toggled
 */
static inline void ToggleOverlay(OverlayOption to)
{
	ToggleBit(_overlay_opt, to);
}

/**
 * Toggle the overlay lock bit
 *
 * @param to the overlay option to be locked or unlocked
 */
static inline void ToggleOverlayLock(OverlayOption to)
{
	ToggleBit(_overlay_lock, to);
}

/** Set or clear all non-locked overlay options */
static inline void ResetRestoreAllOverlays()
{
	/* if none of the non-locked options are set */
	if ((_overlay_opt & ~_overlay_lock) == 0) {
		/* set all non-locked options */
		_overlay_opt |= GB(~_overlay_lock, 0, OO_END);
	} else {
		/* clear all non-locked options */
		_overlay_opt &= _overlay_lock;
	}

	MarkWholeScreenDirty();
}

#endif /* OVERLAY_H */
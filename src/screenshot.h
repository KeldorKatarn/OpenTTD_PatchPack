/* $Id: screenshot.h 24805 2012-12-09 16:53:01Z frosch $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file screenshot.h Functions to make screenshots. */

#ifndef SCREENSHOT_H
#define SCREENSHOT_H

void InitializeScreenshotFormats();

const char *GetCurrentScreenshotExtension();

/** Type of requested screenshot */
enum ScreenshotType {
	SC_VIEWPORT,         ///< Screenshot of viewport.
	SC_CRASHLOG,         ///< Raw screenshot from blitter buffer.
	SC_ZOOMEDIN,         ///< Fully zoomed in screenshot of the visible area.
	SC_DEFAULTZOOM,      ///< Zoomed to default zoom level screenshot of the visible area.
	SC_WORLD,            ///< World screenshot.
	SC_HEIGHTMAP,        ///< Heightmap of the world.
	SC_MINIMAP,          ///< Flat screenshot of the minimap showing the routes.
	SC_MINI_HEIGHTMAP,   ///< Flat screenshot of the minimap showing only topography.
	SC_MINI_INDUSTRYMAP, ///< Flat screenshot of the minimap showing color coded industries.
	SC_MINI_OWNERMAP,    ///< Flat screenshot of the minimap showing routes colored by owner.
};

class SmallMapWindow;

void SetupScreenshotViewport(ScreenshotType t, struct ViewPort *vp);
bool MakeHeightmapScreenshot(const char *filename);
bool MakeSmallMapScreenshot(unsigned int width, unsigned int height, SmallMapWindow *window);
bool MakeScreenshot(ScreenshotType t, const char *name);

extern char _screenshot_format_name[8];
extern uint _num_screenshot_formats;
extern uint _cur_screenshot_format;
extern char _full_screenshot_name[MAX_PATH];

#endif /* SCREENSHOT_H */

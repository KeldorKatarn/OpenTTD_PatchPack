/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file smallmap_colours.h Colours used by smallmap. */

#ifndef SMALLMAP_COLOURS_H
#define SMALLMAP_COLOURS_H

#include "core/endian_func.hpp"

static const uint8 PC_ROUGH_LAND      = 0x52; ///< Dark green palette colour for rough land.
static const uint8 PC_GRASS_LAND      = 0x54; ///< Dark green palette colour for grass land.
static const uint8 PC_BARE_LAND       = 0x37; ///< Brown palette colour for bare land.
static const uint8 PC_FIELDS          = 0x25; ///< Light brown palette colour for fields.
static const uint8 PC_TREES           = 0x57; ///< Green palette colour for trees.
static const uint8 PC_WATER           = 0xCA; ///< Dark blue palette colour for water.

#define MKCOLOUR(x)         TO_LE32X(x)

#define MKCOLOUR_XXXX(x)    (MKCOLOUR(0x01010101) * (uint)(x))
#define MKCOLOUR_X0X0(x)    (MKCOLOUR(0x01000100) * (uint)(x))
#define MKCOLOUR_0X0X(x)    (MKCOLOUR(0x00010001) * (uint)(x))
#define MKCOLOUR_0XX0(x)    (MKCOLOUR(0x00010100) * (uint)(x))
#define MKCOLOUR_X00X(x)    (MKCOLOUR(0x01000001) * (uint)(x))

#define MKCOLOUR_XYXY(x, y) (MKCOLOUR_X0X0(x) | MKCOLOUR_0X0X(y))
#define MKCOLOUR_XYYX(x, y) (MKCOLOUR_X00X(x) | MKCOLOUR_0XX0(y))

#define MKCOLOUR_0000       MKCOLOUR_XXXX(0x00)
#define MKCOLOUR_0FF0       MKCOLOUR_0XX0(0xFF)
#define MKCOLOUR_F00F       MKCOLOUR_X00X(0xFF)
#define MKCOLOUR_FFFF       MKCOLOUR_XXXX(0xFF)

/** Height map colours for the green colour scheme, ordered by height. */
static const uint32 _green_map_heights[] = {
	MKCOLOUR_XXXX(0x5A),
	MKCOLOUR_XYXY(0x5A, 0x5B),
	MKCOLOUR_XXXX(0x5B),
	MKCOLOUR_XYXY(0x5B, 0x5C),
	MKCOLOUR_XXXX(0x5C),
	MKCOLOUR_XYXY(0x5C, 0x5D),
	MKCOLOUR_XXXX(0x5D),
	MKCOLOUR_XYXY(0x5D, 0x5E),
	MKCOLOUR_XXXX(0x5E),
	MKCOLOUR_XYXY(0x5E, 0x5F),
	MKCOLOUR_XXXX(0x5F),
	MKCOLOUR_XYXY(0x5F, 0x1F),
	MKCOLOUR_XXXX(0x1F),
	MKCOLOUR_XYXY(0x1F, 0x27),
	MKCOLOUR_XXXX(0x27),
	MKCOLOUR_XXXX(0x27),
};
assert_compile(lengthof(_green_map_heights) == MAX_TILE_HEIGHT + 1);

/** Darkened height map colours for the green colour scheme, ordered by height. */
static const uint32 _green_map_heights_dark[] = {
	MKCOLOUR_XXXX(0x59),
	MKCOLOUR_XYXY(0x59, 0x5A),
	MKCOLOUR_XXXX(0x5A),
	MKCOLOUR_XYXY(0x5A, 0x5B),
	MKCOLOUR_XXXX(0x5B),
	MKCOLOUR_XYXY(0x5B, 0x5C),
	MKCOLOUR_XXXX(0x5C),
	MKCOLOUR_XYXY(0x5C, 0x5D),
	MKCOLOUR_XXXX(0x5D),
	MKCOLOUR_XYXY(0x5D, 0x5E),
	MKCOLOUR_XXXX(0x5E),
	MKCOLOUR_XYXY(0x5E, 0x5F),
	MKCOLOUR_XXXX(0x5F),
	MKCOLOUR_XYXY(0x5F, 0x1F),
	MKCOLOUR_XXXX(0x1F),
	MKCOLOUR_XYXY(0x1F, 0x27),
};
assert_compile(lengthof(_green_map_heights_dark) == lengthof(_green_map_heights));

/** Lightened height map colours for the green colour scheme, ordered by height. */
static const uint32 _green_map_heights_light[] = {
	MKCOLOUR_XXXX(0x5B),
	MKCOLOUR_XYXY(0x5B, 0x5C),
	MKCOLOUR_XXXX(0x5C),
	MKCOLOUR_XYXY(0x5C, 0x5D),
	MKCOLOUR_XXXX(0x5D),
	MKCOLOUR_XYXY(0x5D, 0x5E),
	MKCOLOUR_XXXX(0x5E),
	MKCOLOUR_XYXY(0x5E, 0x5F),
	MKCOLOUR_XXXX(0x5F),
	MKCOLOUR_XYXY(0x5F, 0x1F),
	MKCOLOUR_XXXX(0x1F),
	MKCOLOUR_XYXY(0x1F, 0x27),
	MKCOLOUR_XXXX(0x27),
	MKCOLOUR_XYXY(0x27, 0x45),
	MKCOLOUR_XXXX(0x45),
	MKCOLOUR_XXXX(0x45),
};
assert_compile(lengthof(_green_map_heights_light) == lengthof(_green_map_heights));

/** Height map colours for the dark green colour scheme, ordered by height. */
static const uint32 _dark_green_map_heights[] = {
	MKCOLOUR_XXXX(0x60),
	MKCOLOUR_XYXY(0x60, 0x61),
	MKCOLOUR_XXXX(0x61),
	MKCOLOUR_XYXY(0x61, 0x62),
	MKCOLOUR_XXXX(0x62),
	MKCOLOUR_XYXY(0x62, 0x63),
	MKCOLOUR_XXXX(0x63),
	MKCOLOUR_XYXY(0x63, 0x64),
	MKCOLOUR_XXXX(0x64),
	MKCOLOUR_XYXY(0x64, 0x65),
	MKCOLOUR_XXXX(0x65),
	MKCOLOUR_XYXY(0x65, 0x66),
	MKCOLOUR_XXXX(0x66),
	MKCOLOUR_XYXY(0x66, 0x67),
	MKCOLOUR_XXXX(0x67),
	MKCOLOUR_XXXX(0x67),
};
assert_compile(lengthof(_dark_green_map_heights) == MAX_TILE_HEIGHT + 1);

/** Darkened height map colours for the dark green colour scheme, ordered by height. */
static const uint32 _dark_green_map_heights_dark[] = {
	MKCOLOUR_XXXX(0x46),
	MKCOLOUR_XYXY(0x46, 0x60),
	MKCOLOUR_XXXX(0x60),
	MKCOLOUR_XYXY(0x60, 0x61),
	MKCOLOUR_XXXX(0x61),
	MKCOLOUR_XYXY(0x61, 0x62),
	MKCOLOUR_XXXX(0x62),
	MKCOLOUR_XYXY(0x62, 0x63),
	MKCOLOUR_XXXX(0x63),
	MKCOLOUR_XYXY(0x63, 0x64),
	MKCOLOUR_XXXX(0x64),
	MKCOLOUR_XYXY(0x64, 0x65),
	MKCOLOUR_XXXX(0x65),
	MKCOLOUR_XYXY(0x65, 0x66),
	MKCOLOUR_XXXX(0x66),
	MKCOLOUR_XYXY(0x66, 0x67),
};
assert_compile(lengthof(_dark_green_map_heights_dark) == lengthof(_dark_green_map_heights));

/** Lightened height map colours for the dark green colour scheme, ordered by height. */
static const uint32 _dark_green_map_heights_light[] = {
	MKCOLOUR_XXXX(0x61),
	MKCOLOUR_XYXY(0x61, 0x62),
	MKCOLOUR_XXXX(0x62),
	MKCOLOUR_XYXY(0x62, 0x63),
	MKCOLOUR_XXXX(0x63),
	MKCOLOUR_XYXY(0x63, 0x64),
	MKCOLOUR_XXXX(0x64),
	MKCOLOUR_XYXY(0x64, 0x65),
	MKCOLOUR_XXXX(0x65),
	MKCOLOUR_XYXY(0x65, 0x66),
	MKCOLOUR_XXXX(0x66),
	MKCOLOUR_XYXY(0x66, 0x67),
	MKCOLOUR_XXXX(0x67),
	MKCOLOUR_XYXY(0x67, 0x0F),
	MKCOLOUR_XXXX(0x0F),
	MKCOLOUR_XXXX(0x0F),
};
assert_compile(lengthof(_dark_green_map_heights_light) == lengthof(_dark_green_map_heights));

/**
 * /// Colour Coding for Stuck Counter
 */
static const uint32 _stuck_counter_colours[] = {
	MKCOLOUR(0xD0D0D0D0),
	MKCOLOUR(0xCECECECE),
	MKCOLOUR(0xBFBFBFBF),
	MKCOLOUR(0xBDBDBDBD),
	MKCOLOUR(0xBABABABA),
	MKCOLOUR(0xB8B8B8B8),
	MKCOLOUR(0xB6B6B6B6),
	MKCOLOUR(0xB4B4B4B4),
};
assert_compile(lengthof(_stuck_counter_colours) == 8);

/** Height map colours for the violet colour scheme, ordered by height. */
static const uint32 _violet_map_heights[] = {
	MKCOLOUR_XXXX(0x80),
	MKCOLOUR_XYXY(0x80, 0x81),
	MKCOLOUR_XXXX(0x81),
	MKCOLOUR_XYXY(0x81, 0x82),
	MKCOLOUR_XXXX(0x82),
	MKCOLOUR_XYXY(0x82, 0x83),
	MKCOLOUR_XXXX(0x83),
	MKCOLOUR_XYXY(0x83, 0x84),
	MKCOLOUR_XXXX(0x84),
	MKCOLOUR_XYXY(0x84, 0x85),
	MKCOLOUR_XXXX(0x85),
	MKCOLOUR_XYXY(0x85, 0x86),
	MKCOLOUR_XXXX(0x86),
	MKCOLOUR_XYXY(0x86, 0x87),
	MKCOLOUR_XXXX(0x87),
	MKCOLOUR_XXXX(0x87),
};
assert_compile(lengthof(_violet_map_heights) == MAX_TILE_HEIGHT + 1);

/** Darkened height map colours for the violet colour scheme, ordered by height. */
static const uint32 _violet_map_heights_dark[] = {
	MKCOLOUR_XXXX(0x46),
	MKCOLOUR_XYXY(0x46, 0x80),
	MKCOLOUR_XXXX(0x80),
	MKCOLOUR_XYXY(0x80, 0x81),
	MKCOLOUR_XXXX(0x81),
	MKCOLOUR_XYXY(0x81, 0x82),
	MKCOLOUR_XXXX(0x82),
	MKCOLOUR_XYXY(0x82, 0x83),
	MKCOLOUR_XXXX(0x83),
	MKCOLOUR_XYXY(0x83, 0x84),
	MKCOLOUR_XXXX(0x84),
	MKCOLOUR_XYXY(0x84, 0x85),
	MKCOLOUR_XXXX(0x85),
	MKCOLOUR_XYXY(0x85, 0x86),
	MKCOLOUR_XXXX(0x86),
	MKCOLOUR_XYXY(0x86, 0x87),
};
assert_compile(lengthof(_violet_map_heights_dark) == lengthof(_violet_map_heights));

/** Lightened height map colours for the violet colour scheme, ordered by height. */
static const uint32 _violet_map_heights_light[] = {
	MKCOLOUR_XXXX(0x81),
	MKCOLOUR_XYXY(0x81, 0x82),
	MKCOLOUR_XXXX(0x82),
	MKCOLOUR_XYXY(0x82, 0x83),
	MKCOLOUR_XXXX(0x83),
	MKCOLOUR_XYXY(0x83, 0x84),
	MKCOLOUR_XXXX(0x84),
	MKCOLOUR_XYXY(0x84, 0x85),
	MKCOLOUR_XXXX(0x85),
	MKCOLOUR_XYXY(0x85, 0x86),
	MKCOLOUR_XXXX(0x86),
	MKCOLOUR_XYXY(0x86, 0x87),
	MKCOLOUR_XXXX(0x87),
	MKCOLOUR_XYXY(0x87, 0x0F),
	MKCOLOUR_XXXX(0x0F),
	MKCOLOUR_XXXX(0x0F),
};
assert_compile(lengthof(_violet_map_heights_light) == lengthof(_violet_map_heights));

/** Colour scheme of the smallmap. */
struct SmallMapColourScheme {
	const uint32 *height_colours; ///< Colour of each level in a heightmap.
	uint32 default_colour;   ///< Default colour of the land.
};

/** Available colour schemes for height maps. */
static const SmallMapColourScheme _heightmap_schemes[] = {
	{_green_map_heights,            MKCOLOUR_XXXX(0x5B)}, ///< Green colour scheme.
	{_dark_green_map_heights,       MKCOLOUR_XXXX(0x62)}, ///< Dark green colour scheme.
	{_violet_map_heights,           MKCOLOUR_XXXX(0x82)}, ///< Violet colour scheme.
};

static const SmallMapColourScheme _heightmap_schemes_dark[] = {
	{_green_map_heights_dark,       MKCOLOUR_XXXX(0x5A)}, ///< Green colour scheme.
	{_dark_green_map_heights_dark,  MKCOLOUR_XXXX(0x61)}, ///< Dark green colour scheme.
	{_violet_map_heights_dark,      MKCOLOUR_XXXX(0x81)}, ///< Violet colour scheme.
};

static const SmallMapColourScheme _heightmap_schemes_light[] = {
	{_green_map_heights_light,      MKCOLOUR_XXXX(0x5C)}, ///< Green colour scheme.
	{_dark_green_map_heights_light, MKCOLOUR_XXXX(0x63)}, ///< Dark green colour scheme.
	{_violet_map_heights_light,     MKCOLOUR_XXXX(0x83)}, ///< Violet colour scheme.
};


struct AndOr {
	uint32 mor;
	uint32 mand;
};

static inline uint32 ApplyMask(uint32 colour, const AndOr *mask)
{
	return (colour & mask->mand) | mask->mor;
}

/** Colour masks for "Contour" and "Routes" modes. */
static const AndOr _smallmap_contours_andor[] = {
	{MKCOLOUR_0000               , MKCOLOUR_FFFF}, // MP_CLEAR
	{MKCOLOUR_0XX0(PC_GREY      ), MKCOLOUR_F00F}, // MP_RAILWAY
	{MKCOLOUR_0XX0(PC_BLACK     ), MKCOLOUR_F00F}, // MP_ROAD
	{MKCOLOUR_0XX0(PC_DARK_RED  ), MKCOLOUR_F00F}, // MP_HOUSE
	{MKCOLOUR_0000               , MKCOLOUR_FFFF}, // MP_TREES
	{MKCOLOUR_XXXX(PC_LIGHT_BLUE), MKCOLOUR_0000}, // MP_STATION
	{MKCOLOUR_XXXX(PC_WATER     ), MKCOLOUR_0000}, // MP_WATER
	{MKCOLOUR_0000               , MKCOLOUR_FFFF}, // MP_VOID
	{MKCOLOUR_XXXX(PC_DARK_RED  ), MKCOLOUR_0000}, // MP_INDUSTRY
	{MKCOLOUR_0000               , MKCOLOUR_FFFF}, // MP_TUNNELBRIDGE
	{MKCOLOUR_0XX0(PC_DARK_RED  ), MKCOLOUR_F00F}, // MP_OBJECT
	{MKCOLOUR_0XX0(PC_GREY      ), MKCOLOUR_F00F},
};

/** Colour masks for "Vehicles", "Industry", and "Vegetation" modes. */
static const AndOr _smallmap_vehicles_andor[] = {
	{MKCOLOUR_0000               , MKCOLOUR_FFFF}, // MP_CLEAR
	{MKCOLOUR_0XX0(PC_BLACK     ), MKCOLOUR_F00F}, // MP_RAILWAY
	{MKCOLOUR_0XX0(PC_BLACK     ), MKCOLOUR_F00F}, // MP_ROAD
	{MKCOLOUR_0XX0(PC_DARK_RED  ), MKCOLOUR_F00F}, // MP_HOUSE
	{MKCOLOUR_0000               , MKCOLOUR_FFFF}, // MP_TREES
	{MKCOLOUR_0XX0(PC_BLACK     ), MKCOLOUR_F00F}, // MP_STATION
	{MKCOLOUR_XXXX(PC_WATER     ), MKCOLOUR_0000}, // MP_WATER
	{MKCOLOUR_0000               , MKCOLOUR_FFFF}, // MP_VOID
	{MKCOLOUR_XXXX(PC_DARK_RED  ), MKCOLOUR_0000}, // MP_INDUSTRY
	{MKCOLOUR_0000               , MKCOLOUR_FFFF}, // MP_TUNNELBRIDGE
	{MKCOLOUR_0XX0(PC_DARK_RED  ), MKCOLOUR_F00F}, // MP_OBJECT
	{MKCOLOUR_0XX0(PC_BLACK     ), MKCOLOUR_F00F},
};

static const uint32 _vegetation_clear_bits[] = {
	MKCOLOUR_XXXX(PC_GRASS_LAND), ///< full grass
	MKCOLOUR_XXXX(PC_ROUGH_LAND), ///< rough land
	MKCOLOUR_XXXX(PC_GREY),       ///< rocks
	MKCOLOUR_XXXX(PC_FIELDS),     ///< fields
	MKCOLOUR_XXXX(PC_LIGHT_BLUE), ///< snow
	MKCOLOUR_XXXX(PC_ORANGE),     ///< desert
	MKCOLOUR_XXXX(PC_GRASS_LAND), ///< unused
	MKCOLOUR_XXXX(PC_GRASS_LAND), ///< unused
};

/* Lighten colour */
#define LTCOLOUR(colour) ((colour) + 1)
#define LTCOLOURS(colours) ((colours) + 0x01010101)
/* Darken colour */
#define DKCOLOUR(colour) ((colour) - 1)
#define DKCOLOURS(colours) ((colours) - 0x01010101)
/* Darken variable colour (for green player, colour-1 gives some blue) */
#define DKCOLOUR_VAR(colour) (((colour) != 0xCE) ? DKCOLOUR(colour) : 0x55)

#endif /* SMALLMAP_COLOURS_H */

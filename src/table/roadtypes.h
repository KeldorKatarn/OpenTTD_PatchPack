/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file roadtypes.h
 * All the roadtype-specific information is stored here.
 */

#ifndef ROADTYPES_H
#define ROADTYPES_H

/**
 * Global Roadtype definition
 */
static const RoadtypeInfo _original_roadtypes[] = {
	/** Road */
	{
		{
			STR_ROAD_NAME_ROAD,
			STR_ROAD_TOOLBAR_ROAD_CONSTRUCTION_CAPTION,
			STR_ROAD_MENU_ROAD_CONSTRUCTION,
			STR_BUY_VEHICLE_ROAD_VEHICLE_CAPTION,
			STR_REPLACE_VEHICLE_ROAD_VEHICLE,
			STR_ENGINE_PREVIEW_ROAD_VEHICLE,
		},

		/* Powered roadtypes */
		ROADTYPES_ROAD | ROADTYPES_TRAM,

		/* Compatible roadtypes */
		ROADTYPES_ROAD,

		/* bridge offset */
		0,

		/* fallback_roadtype */
		0,

		/* curve speed advantage (multiplier) */
		0,

		/* flags */
		ROTFB_NONE,

		/* cost multiplier */
		8,

		/* maintenance cost multiplier */
		8,

		/* acceleration type */
		0,

		/* max speed */
		0,

		/* road type label */
		'ROAD',

		/* alternate labels */
		RoadTypeLabelList(),

		/* map colour */
		0x0A,

		/* introduction date */
		INVALID_DATE,

		/* roadtypes required for this to be introduced */
		ROADTYPES_NONE,

		/* introduction road types */
		ROADTYPES_ROAD,

		/* sort order */
		0 << 4 | 7,

		{ NULL },
		{ NULL },
	},
	/** Tram */
	{
		{
			STR_ROAD_NAME_TRAM,
			STR_ROAD_TOOLBAR_TRAM_CONSTRUCTION_CAPTION,
			STR_ROAD_MENU_TRAM_CONSTRUCTION,
			STR_BUY_VEHICLE_ROAD_VEHICLE_CAPTION,
			STR_REPLACE_VEHICLE_ROAD_VEHICLE,
			STR_ENGINE_PREVIEW_ROAD_VEHICLE,
		},

		/* Powered roadtypes */
		ROADTYPES_ROAD | ROADTYPES_TRAM,

		/* Compatible roadtypes */
		ROADTYPES_TRAM,

		/* bridge offset */
		0,

		/* fallback_roadtype */
		0,

		/* curve speed advantage (multiplier) */
		0,

		/* flags */
		ROTFB_NONE,

		/* cost multiplier */
		8,

		/* maintenance cost multiplier */
		8,

		/* acceleration type */
		0,

		/* max speed */
		0,

		/* road type label */
		'TRAM',

		/* alternate labels */
		RoadTypeLabelList(),

		/* map colour */
		0x0A,

		/* introduction date */
		INVALID_DATE,

		/* roadtypes required for this to be introduced */
		ROADTYPES_NONE,

		/* introduction road types */
		ROADTYPES_TRAM,

		/* sort order */
		1 << 4 | 7,

		{ NULL },
		{ NULL },
	},
};

#endif /* ROADTYPES_H */

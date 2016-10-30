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
			{
					0, 0x546, 0x545, 0x53B, 0x544, 0x534, 0x53E, 0x539,
				0x543, 0x53C, 0x535, 0x538, 0x53D, 0x537, 0x53A, 0x536
			},
			SPR_ROAD_SLOPE_START,
			SPR_ONEWAY_BASE,
			SPR_EXCAVATION_X,
			SPR_EXCAVATION_Y
		},

		/* GUI sprites */
		{
			SPR_IMG_ROAD_X_DIR,
			SPR_IMG_ROAD_Y_DIR,
			SPR_IMG_AUTOROAD,
			SPR_IMG_ROAD_DEPOT,
			SPR_IMG_BUS_STATION,
			SPR_IMG_TRUCK_BAY,
			SPR_IMG_ROAD_TUNNEL
		},

		{
			SPR_CURSOR_ROAD_NESW,
			SPR_CURSOR_ROAD_NWSE,
			SPR_CURSOR_AUTOROAD,
			SPR_CURSOR_ROAD_DEPOT,
			SPR_CURSOR_BUS_STATION,
			SPR_CURSOR_TRUCK_STATION,
			SPR_CURSOR_TUNNEL_RAIL
		},

		/* strings */
		{
			STR_ROAD_NAME_ROAD,
			STR_ROAD_TOOLBAR_ROAD_CONSTRUCTION_CAPTION,
			STR_ROAD_MENU_ROAD_CONSTRUCTION,
			STR_BUY_VEHICLE_ROAD_VEHICLE_CAPTION,
			STR_REPLACE_VEHICLE_ROAD_VEHICLE,
			STR_ENGINE_PREVIEW_ROAD_VEHICLE,

			STR_ERROR_CAN_T_BUILD_ROAD_HERE,
			STR_ERROR_CAN_T_REMOVE_ROAD_FROM,
			STR_ERROR_CAN_T_BUILD_ROAD_DEPOT,
			{ STR_ERROR_CAN_T_BUILD_BUS_STATION,         STR_ERROR_CAN_T_BUILD_TRUCK_STATION },
			{ STR_ERROR_CAN_T_REMOVE_BUS_STATION,        STR_ERROR_CAN_T_REMOVE_TRUCK_STATION },
			{ STR_STATION_BUILD_BUS_ORIENTATION,         STR_STATION_BUILD_TRUCK_ORIENTATION },
			{ STR_STATION_BUILD_BUS_ORIENTATION_TOOLTIP, STR_STATION_BUILD_TRUCK_ORIENTATION_TOOLTIP },
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
		2,

		/* maintenance cost multiplier */
		2,

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
};

static const RoadtypeInfo _original_tramtypes[] = {
	/** Tram */
	{
		{
			{
					0, 0x546, 0x545, 0x53B, 0x544, 0x534, 0x53E, 0x539,
				0x543, 0x53C, 0x535, 0x538, 0x53D, 0x537, 0x53A, 0x536
			},
			SPR_TRAMWAY_SLOPED_OFFSET,
			0,
			0,
			0
		},

		/* GUI sprites */
		{
			SPR_IMG_TRAMWAY_X_DIR,
			SPR_IMG_TRAMWAY_Y_DIR,
			SPR_IMG_AUTOTRAM,
			SPR_IMG_ROAD_DEPOT,
			SPR_IMG_BUS_STATION,
			SPR_IMG_TRUCK_BAY,
			SPR_IMG_ROAD_TUNNEL
		},

		{
			SPR_CURSOR_TRAMWAY_NESW,
			SPR_CURSOR_TRAMWAY_NWSE,
			SPR_CURSOR_AUTOTRAM,
			SPR_CURSOR_ROAD_DEPOT,
			SPR_CURSOR_BUS_STATION,
			SPR_CURSOR_TRUCK_STATION,
			SPR_CURSOR_TUNNEL_RAIL
		},

		/* strings */
		{
			STR_ROAD_NAME_TRAM,
			STR_ROAD_TOOLBAR_TRAM_CONSTRUCTION_CAPTION,
			STR_ROAD_MENU_TRAM_CONSTRUCTION,
			STR_BUY_VEHICLE_ROAD_VEHICLE_CAPTION,
			STR_REPLACE_VEHICLE_ROAD_VEHICLE,
			STR_ENGINE_PREVIEW_ROAD_VEHICLE,

			STR_ERROR_CAN_T_BUILD_TRAMWAY_HERE,
			STR_ERROR_CAN_T_REMOVE_TRAMWAY_FROM,
			STR_ERROR_CAN_T_BUILD_TRAM_DEPOT,
			{ STR_ERROR_CAN_T_BUILD_PASSENGER_TRAM_STATION,         STR_ERROR_CAN_T_BUILD_CARGO_TRAM_STATION },
			{ STR_ERROR_CAN_T_REMOVE_PASSENGER_TRAM_STATION,        STR_ERROR_CAN_T_REMOVE_CARGO_TRAM_STATION },
			{ STR_STATION_BUILD_PASSENGER_TRAM_ORIENTATION,         STR_STATION_BUILD_CARGO_TRAM_ORIENTATION },
			{ STR_STATION_BUILD_PASSENGER_TRAM_ORIENTATION_TOOLTIP, STR_STATION_BUILD_CARGO_TRAM_ORIENTATION_TOOLTIP },
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
		2,

		/* maintenance cost multiplier */
		2,

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

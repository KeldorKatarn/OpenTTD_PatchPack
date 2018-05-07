
 /* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file zoning_cmd.cpp */

#include "stdafx.h"

#include "company_func.h"
#include "gfx_func.h"
#include "industry.h"
#include "map_func.h"
#include "openttd.h"
#include "station_base.h"
#include "station_type.h"
#include "town.h"
#include "town_map.h"
#include "tracerestrict.h"
#include "viewport_func.h"
#include "zoning.h"

#include "table/sprites.h"

#include "3rdparty/cpp-btree/btree_set.h"

Zoning _zoning;
static const SpriteID ZONING_INVALID_SPRITE_ID = UINT_MAX;

static btree::btree_set<uint32> _zoning_cache_inner;
static btree::btree_set<uint32> _zoning_cache_outer;

//! Enumeration of multi-part foundations.
enum FoundationPart
{
	FOUNDATION_PART_NONE = 0xFF,
	//!< Neither foundation nor groundsprite drawn yet.
	FOUNDATION_PART_NORMAL = 0,
	//!< First part (normal foundation or no foundation)
	FOUNDATION_PART_HALFTILE = 1,
	//!< Second part (halftile foundation)
	FOUNDATION_PART_END
};

void DrawTileSelectionRect(const TileInfo* ti, PaletteID pal);
void DrawSelectionSprite(SpriteID image, PaletteID pal, const TileInfo* ti, int z_offset, FoundationPart foundation_part);

//! Detect whether this area is within the acceptance of any station.
//! 
//! @param area the area to search by
//! @param owner the owner of the stations which we need to match again
//! @param facility_mask one or more facilities in the mask must be present for a station to be used
//! @return true if a station is found
bool IsAreaWithinAcceptanceZoneOfStation(TileArea area, Owner owner, StationFacility facility_mask)
{
	const int catchment = _settings_game.station.station_spread + (_settings_game.station.modified_catchment ? MAX_CATCHMENT : CA_UNMODIFIED);

	StationFinder station_finder(TileArea(TileXY(TileX(area.tile) - (catchment / 2), TileY(area.tile) - (catchment / 2)),
										  TileX(area.tile) + area.w + catchment, TileY(area.tile) + area.h + catchment));

	for (auto iter = station_finder.GetStations()->Begin(); iter != station_finder.GetStations()->End(); ++iter) {
		const Station* station = *iter;

		if (station->owner != owner || !(station->facilities & facility_mask)) continue;

		const Rect rect = station->GetCatchmentRect();

		return TileArea(TileXY(rect.left, rect.top), TileXY(rect.right, rect.bottom)).Intersects(area);
	}

	return false;
}

//! Detect whether this tile is within the acceptance of any station.
//! 
//! @param tile the tile to search by
//! @param owner the owner of the stations
//! @param facility_mask one or more facilities in the mask must be present for a station to be used
//! @return true if a station is found
bool IsTileWithinAcceptanceZoneOfStation(TileIndex tile, Owner owner, StationFacility facility_mask)
{
	const int catchment = _settings_game.station.station_spread + (_settings_game.station.modified_catchment ? MAX_CATCHMENT : CA_UNMODIFIED);

	StationFinder station_finder(TileArea(TileXY(TileX(tile) - (catchment / 2), TileY(tile) - (catchment / 2)),
										  catchment, catchment));

	for (auto iter = station_finder.GetStations()->Begin(); iter != station_finder.GetStations()->End(); ++iter) {
		const Station* station = *iter;

		if (station->owner != owner || !(station->facilities & facility_mask)) continue;

		const Rect rect = station->GetCatchmentRect();

		if (uint(rect.left) <= TileX(tile) && TileX(tile) <= uint(rect.right)
			&& uint(rect.top) <= TileY(tile) && TileY(tile) <= uint(rect.bottom)) {
			return true;
		}
	}

	return false;
}

//! Check whether the player can build in tile.
//! 
//! @param tile the tile to check
//! @param owner the company to check for
//! @return red if they cannot
SpriteID TileZoneCheckBuildEvaluation(TileIndex tile, Owner owner)
{
	// Let's first check for the obvious things you cannot build on.
	switch (GetTileType(tile)) {
		case MP_INDUSTRY:
		case MP_OBJECT:
		case MP_STATION:
		case MP_HOUSE:
		case MP_TUNNELBRIDGE:
			return SPR_ZONING_INNER_HIGHLIGHT_RED;

			// There are only two things you can own (or some else
			// can own) that you can still build on. i.e. roads and
			// railways.
			// TODO
			// Add something more intelligent, check what tool the
			// user is currently using (and if none, assume some
			// standards), then check it against if owned by some-
			// one else (e.g. railway on someone else's road).
			// While that being said, it should also check if it
			// is not possible to build railway/road on someone
			// else's/your own road/railway (e.g. the railway track
			// is curved or a cross).
		case MP_ROAD:
		case MP_RAILWAY:
			if (GetTileOwner(tile) != owner) {
				return SPR_ZONING_INNER_HIGHLIGHT_RED;
			} else {
				return ZONING_INVALID_SPRITE_ID;
			}

		default:
			return ZONING_INVALID_SPRITE_ID;
	}
}

//! Check the opinion of the local authority in the tile.
//! 
//! @param tile the tile to check
//! @param owner the company to check for
//! @return black if no opinion, orange if bad,
//!         light blue if good or invalid if no town
SpriteID TileZoneCheckOpinionEvaluation(TileIndex tile, Owner owner)
{
	int opinion = 0; // 0: No town, 1: No opinion, 2: Bad, 3: Good
	Town* town = ClosestTownFromTile(tile, _settings_game.economy.dist_local_authority);

	if (town != nullptr) {
		if (HasBit(town->have_ratings, owner)) {
			opinion = (town->ratings[owner] > 0) ? 3 : 2;
		} else {
			opinion = 1;
		}
	}

	switch (opinion) {
		case 1:
			return SPR_ZONING_INNER_HIGHLIGHT_BLACK; // No opinion
		case 2:
			return SPR_ZONING_INNER_HIGHLIGHT_ORANGE; // Bad
		case 3:
			return SPR_ZONING_INNER_HIGHLIGHT_LIGHT_BLUE; // Good
		default:
			return ZONING_INVALID_SPRITE_ID; // No town
	}
}

//! Detect whether the tile is within the catchment zone of a station.
//! 
//! @param tile the tile to check
//! @param owner the company owning the stations
//! @return black if within, light blue if only in acceptance zone
//!         and nothing if no nearby station.
SpriteID TileZoneCheckStationCatchmentEvaluation(TileIndex tile, Owner owner)
{
	// Never on a station.
	if (IsTileType(tile, MP_STATION)) {
		return ZONING_INVALID_SPRITE_ID;
	}

	// For provided goods
	StationFinder stations(TileArea(tile, 1, 1));

	for (auto iter = stations.GetStations()->Begin(); iter != stations.GetStations()->End(); ++iter) {
		const Station* station = *iter;
		if (station->owner == owner) {
			return SPR_ZONING_INNER_HIGHLIGHT_BLACK;
		}
	}

	// For accepted goods
	if (IsTileWithinAcceptanceZoneOfStation(tile, owner, ~FACIL_NONE)) {
		return SPR_ZONING_INNER_HIGHLIGHT_LIGHT_BLUE;
	}

	return ZONING_INVALID_SPRITE_ID;
}

//! Detect whether a building is unserved by a station of owner.
//! 
//! @param tile the tile to check
//! @param owner the company to check for
//! @return red if unserved, orange if only accepting, nothing if served or not a building
SpriteID TileZoneCheckUnservedBuildingsEvaluation(TileIndex tile, Owner owner)
{
	if (!IsTileType(tile, MP_HOUSE)) {
		return ZONING_INVALID_SPRITE_ID;
	}

	CargoArray dat;

	memset(&dat, 0, sizeof(dat));
	AddAcceptedCargo(tile, dat, nullptr);

	if (dat[CT_MAIL] + dat[CT_PASSENGERS] == 0) {
		// Nothing is accepted, so now test if cargo is produced
		AddProducedCargo(tile, dat);

		if (dat[CT_MAIL] + dat[CT_PASSENGERS] == 0) {
			// Total is still 0, so give up
			return ZONING_INVALID_SPRITE_ID;
		}
	}

	StationFinder stations(TileArea(tile, 1, 1));

	for (auto iter = stations.GetStations()->Begin(); iter != stations.GetStations()->End(); ++iter) {
		const Station* station = *iter;
		if (station->owner == owner) {
			return ZONING_INVALID_SPRITE_ID;
		}
	}

	// For accepted goods
	if (IsTileWithinAcceptanceZoneOfStation(tile, owner, ~FACIL_NONE)) {
		return SPR_ZONING_INNER_HIGHLIGHT_ORANGE;
	}

	return SPR_ZONING_INNER_HIGHLIGHT_RED;
}

//! Detect whether an industry is unserved by a station of owner.
//! 
//! @param tile the tile to check
//! @param owner the company to check for
//! @return red if unserved, orange if only accepting, nothing if served or not a building
SpriteID TileZoneCheckUnservedIndustriesEvaluation(TileIndex tile, Owner owner)
{
	if (IsTileType(tile, MP_INDUSTRY)) {
		Industry* industry = Industry::GetByTile(tile);
		StationFinder stations(industry->location);

		for (auto iter = stations.GetStations()->Begin(); iter != stations.GetStations()->End(); ++iter) {
			const Station* station = *iter;
			if (station->owner == owner && station->facilities & (~FACIL_BUS_STOP)) {
				return ZONING_INVALID_SPRITE_ID;
			}
		}

		// For accepted goods
		if (IsAreaWithinAcceptanceZoneOfStation(industry->location, owner, ~FACIL_BUS_STOP)) {
			return SPR_ZONING_INNER_HIGHLIGHT_ORANGE;
		}

		return SPR_ZONING_INNER_HIGHLIGHT_RED;
	}

	return ZONING_INVALID_SPRITE_ID;
}

//! Detect whether a tile is a restricted signal tile
//! 
//! @param tile the tile to check
//! @param owner the company to check for
//! @return red if a restricted signal, nothing otherwise
SpriteID TileZoneCheckTraceRestrictEvaluation(TileIndex tile, Owner owner)
{
	if (IsTileType(tile, MP_RAILWAY) && HasSignals(tile) && IsRestrictedSignal(tile)) {
		return SPR_ZONING_INNER_HIGHLIGHT_RED;
	}

	return ZONING_INVALID_SPRITE_ID;
}

//! General evaluation function; calls all the other functions depending on
//! evaluation mode.
//! 
//! @param tile Tile to be evaluated.
//! @param owner The current player
//! @param evaluation_mode The current evaluation mode.
//! @return The colour returned by the evaluation functions (none if no evaluation_mode).
SpriteID TileZoningSpriteEvaluation(TileIndex tile, Owner owner, ZoningEvaluationMode evaluation_mode)
{
	switch (evaluation_mode) {
		case ZEM_CAN_BUILD:
			return TileZoneCheckBuildEvaluation(tile, owner);
		case ZEM_AUTHORITY:
			return TileZoneCheckOpinionEvaluation(tile, owner);
		case ZEM_STA_CATCH:
			return TileZoneCheckStationCatchmentEvaluation(tile, owner);
		case ZEM_BUL_UNSER:
			return TileZoneCheckUnservedBuildingsEvaluation(tile, owner);
		case ZEM_IND_UNSER:
			return TileZoneCheckUnservedIndustriesEvaluation(tile, owner);
		case ZEM_TRACERESTRICT:
			return TileZoneCheckTraceRestrictEvaluation(tile, owner);
		default:
			return ZONING_INVALID_SPRITE_ID;
	}
}

inline SpriteID TileZoningSpriteEvaluationCached(TileIndex tile, Owner owner, ZoningEvaluationMode ev_mode, bool is_inner)
{
	if (ev_mode == ZEM_BUL_UNSER && !IsTileType(tile, MP_HOUSE)) return ZONING_INVALID_SPRITE_ID;
	if (ev_mode == ZEM_IND_UNSER && !IsTileType(tile, MP_INDUSTRY)) return ZONING_INVALID_SPRITE_ID;

	if (ev_mode >= ZEM_STA_CATCH && ev_mode <= ZEM_IND_UNSER) {
		// Cacheable
		btree::btree_set<uint32>& cache = is_inner ? _zoning_cache_inner : _zoning_cache_outer;
		const auto lower_bound = cache.lower_bound(tile << 3);

		if (lower_bound != cache.end() && *lower_bound >> 3 == tile) {
			switch (*lower_bound & 7) {
				case 0:
					return ZONING_INVALID_SPRITE_ID;
				case 1:
					return SPR_ZONING_INNER_HIGHLIGHT_RED;
				case 2:
					return SPR_ZONING_INNER_HIGHLIGHT_ORANGE;
				case 3:
					return SPR_ZONING_INNER_HIGHLIGHT_BLACK;
				case 4:
					return SPR_ZONING_INNER_HIGHLIGHT_LIGHT_BLUE;
				default: NOT_REACHED();
			}
		}

		const SpriteID sprite = TileZoningSpriteEvaluation(tile, owner, ev_mode);
		uint val = tile << 3;

		switch (sprite) {
			case ZONING_INVALID_SPRITE_ID:
				val |= 0;
				break;
			case SPR_ZONING_INNER_HIGHLIGHT_RED:
				val |= 1;
				break;
			case SPR_ZONING_INNER_HIGHLIGHT_ORANGE:
				val |= 2;
				break;
			case SPR_ZONING_INNER_HIGHLIGHT_BLACK:
				val |= 3;
				break;
			case SPR_ZONING_INNER_HIGHLIGHT_LIGHT_BLUE:
				val |= 4;
				break;
			default: NOT_REACHED();
		}

		cache.insert(lower_bound, val);

		return sprite;
	}

	return TileZoningSpriteEvaluation(tile, owner, ev_mode);
}

//! Draw the the zoning on the tile.
//! 
//! @param tile_info the tile to draw on.
void DrawTileZoning(const TileInfo* tile_info)
{
	if (IsTileType(tile_info->tile, MP_VOID) || _game_mode != GM_NORMAL) {
		return;
	}

	if (_zoning.outer != ZEM_NOTHING) {
		const auto colour = TileZoningSpriteEvaluationCached(tile_info->tile, _local_company, _zoning.outer, false);

		if (colour != ZONING_INVALID_SPRITE_ID) {
			DrawTileSelectionRect(tile_info, colour);
		}
	}

	if (_zoning.inner != ZEM_NOTHING) {
		const auto colour = TileZoningSpriteEvaluationCached(tile_info->tile, _local_company, _zoning.inner, true);

		if (colour != ZONING_INVALID_SPRITE_ID) {
			auto sprite = SPR_ZONING_INNER_HIGHLIGHT_BASE;

			if (IsHalftileSlope(tile_info->tileh)) {
				DrawSelectionSprite(sprite, colour, tile_info, 7 + TILE_HEIGHT, FOUNDATION_PART_HALFTILE);
			} else {
				sprite += SlopeToSpriteOffset(tile_info->tileh);
			}

			DrawSelectionSprite(sprite, colour, tile_info, 7, FOUNDATION_PART_NORMAL);
		}
	}
}

static uint GetZoningModeDependantStationCoverageRadius(const Station* station, ZoningEvaluationMode evaluation_mode)
{
	switch (evaluation_mode) {
		case ZEM_STA_CATCH:
			return station->GetCatchmentRadius();
		case ZEM_BUL_UNSER:
			return station->GetCatchmentRadius();
		case ZEM_IND_UNSER:
			// This is to wholly update industries partially within the region
			return station->GetCatchmentRadius() + 10;
		default:
			return 0;
	}
}

//! Mark dirty the coverage area around a station if the current zoning mode depends on station coverage
//! 
//! @param station The station to use
//! @param mask The zoning mode mask
void ZoningMarkDirtyStationCoverageArea(const Station* station, ZoningModeMask mask)
{
	if (station->rect.IsEmpty()) return;

	const uint outer_radius = mask & ZMM_OUTER ? GetZoningModeDependantStationCoverageRadius(station, _zoning.outer) : 0;
	const uint inner_radius = mask & ZMM_INNER ? GetZoningModeDependantStationCoverageRadius(station, _zoning.inner) : 0;
	const uint radius = max<uint>(outer_radius, inner_radius);

	if (radius > 0) {
		Rect rect = station->GetCatchmentRectUsingRadius(radius);
		for (int y = rect.top; y <= rect.bottom; y++) {
			for (int x = rect.left; x <= rect.right; x++) {
				MarkTileDirtyByTile(TileXY(x, y));
			}
		}
		const auto invalidate_cache_rect = [&](btree::btree_set<uint32>& cache) {
			for (int y = rect.top; y <= rect.bottom; y++) {
				const auto lower_bound = cache.lower_bound(TileXY(rect.left, y) << 3);
				auto end_iter = lower_bound;
				const uint end = TileXY(rect.right, y) << 3;
				while (end_iter != cache.end() && *end_iter < end) ++end_iter;
				cache.erase(lower_bound, end_iter);
			}
		};

		if (outer_radius) invalidate_cache_rect(_zoning_cache_outer);
		if (inner_radius) invalidate_cache_rect(_zoning_cache_inner);
	}
}

void ClearZoningCaches()
{
	_zoning_cache_inner.clear();
	_zoning_cache_outer.clear();
}

void SetZoningMode(bool inner, ZoningEvaluationMode mode)
{
	ZoningEvaluationMode& current_mode = inner ? _zoning.inner : _zoning.outer;
	btree::btree_set<uint32>& cache = inner ? _zoning_cache_inner : _zoning_cache_outer;

	if (current_mode == mode) return;

	current_mode = mode;
	cache.clear();
	MarkWholeScreenDirty();
}

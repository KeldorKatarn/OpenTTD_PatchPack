/* $Id$ */

/** @file overlay_cmd.cpp Handling of overlays. */

#include "stdafx.h"
#include "tile_type.h"
#include "tile_cmd.h"
#include "overlay.h"
#include "station_func.h"
#include "viewport_func.h"
#include "overlay_cmd.h"

Overlays* Overlays::instance = NULL;

Overlays* Overlays::Instance() 
{
	if (instance == NULL)
		instance = new Overlays();
	return instance;
};

void Overlays::AddStation(const Station* st)
{
	this->catchmentOverlay.insert(st);
};

void Overlays::RemoveStation(const Station* st) 
{
	this->catchmentOverlay.erase(st);
};

void Overlays::ToggleStation(const Station* st) 
{
	if(this->HasStation(st)) {
		this->RemoveStation(st);
	} else {
		this->AddStation(st);
	}
};

void Overlays::HandleSignalProgramDeletion(const SignalProgram* program)
{
	if (this->logic_signal_program == program) {
		RefreshLogicSignalOverlay();
		this->logic_signal_program = nullptr;
	}
};

void Overlays::SetLogicSignalOverlay(const SignalProgram* program)
{
	// Old program input tiles
	RefreshLogicSignalOverlay();

	this->logic_signal_program = program;

	// New program input tiles
	RefreshLogicSignalOverlay();
};

void Overlays::ClearLogicSignalOverlay()
{
	SetLogicSignalOverlay(nullptr);
};

void Overlays::RefreshLogicSignalOverlay() const
{
	if (this->logic_signal_program != nullptr) {
		auto signal_references = this->logic_signal_program->GetSignalReferences();

		for (auto reference : signal_references) {
			MarkTileDirtyByTile(GetTileFromSignalReference(reference));
		}
	}
}

void Overlays::Clear() 
{
	this->catchmentOverlay.clear();

	// Old program input tiles
	RefreshLogicSignalOverlay();

	this->logic_signal_program = nullptr;
};

bool Overlays::IsTileLogicSignalInput(const TileInfo* ti)
{
	if (this->logic_signal_program == nullptr) return false;

	auto signal_references = this->logic_signal_program->GetSignalReferences();

	return std::any_of(signal_references.begin(), signal_references.end(), [&](auto signal_reference) {
		return GetTileFromSignalReference(signal_reference) == ti->tile;
	});
};

bool Overlays::IsTileInCatchmentArea(const TileInfo* ti, CatchmentType type) 
{
	for(std::set<const Station *>::iterator iter = catchmentOverlay.begin();iter != catchmentOverlay.end();) {
		const Station *st = *iter;
		if( st->IsTileInCatchmentArea(ti, type))
			return true;
		iter++;
	}
	return false;
};

bool Overlays::HasStation(const Station* st) 
{
	return (this->catchmentOverlay.find(st) != this->catchmentOverlay.end());
};

Overlays::~Overlays() 
{
	this->catchmentOverlay.clear();
};
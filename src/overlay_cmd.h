/* $Id$ */

/** @file overlay_cmd.h Functions related to overlays. */

#ifndef OVERLAY_CMD_H
#define OVERLAY_CMD_H

#include "tile_type.h"
#include "tile_cmd.h"
#include "station_base.h"
#include "logic_signals.h"
#include <set>

class Overlays {
	
	std::set<const Station *> catchmentOverlay;
	const SignalProgram* logic_signal_program;

protected:
	static Overlays* instance;
	
public:
	static Overlays* Instance();

	void AddStation(const Station* st);

	void RemoveStation(const Station *st);

	void ToggleStation(const Station* st);

	void SetLogicSignalOverlay(const SignalProgram* program);

	void ClearLogicSignalOverlay();

	void RefreshLogicSignalOverlay() const;

	void HandleSignalProgramDeletion(const SignalProgram* program);
	
	void Clear();

	bool IsTileLogicSignalInput(const TileInfo* ti);

	bool IsTileInCatchmentArea(const TileInfo* ti, CatchmentType type);

	bool HasStation(const Station* st);

	virtual ~Overlays();
};

#endif // OVERLAY_CMD_H
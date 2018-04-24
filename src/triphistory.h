/** @file triphistory.h */

#ifndef TRIPHISTORY_H
#define TRIPHISTORY_H

#include "economy_type.h"
#include "date_type.h"
#include <cmath>

// entries to save
static const int NUM_TRIP_HISTORY_ENTRIES = 10;

struct TripHistoryEntry {
	Money profit; // Saved
	Ticks ticks; // Saved
	int32 profit_change; // Calculated
	Ticks time_between_trips; // Calculated
	int32 time_between_trips_change; // Calculated

	TripHistoryEntry() : profit(0), ticks(0), profit_change(0), time_between_trips(0), time_between_trips_change(0) { };
};

/** Structure to hold data for each vehicle */
struct TripHistory {
	// a lot of saveload stuff for std::deque. So...
	TripHistoryEntry entries[NUM_TRIP_HISTORY_ENTRIES];

	Money total_profit;
	Money avg_profit;
	Ticks avg_time_between_trips;

	TripHistory() :
		total_profit(0),
		avg_time_between_trips(0),
		avg_profit(0) { }


	void AddValue(Money profit, Ticks ticks);
	int32 FindPercentChange(float current_value, float previous_value);
	void NewRound();
	int32 UpdateCalculated(bool update_entries = false);
};

#endif /* TRIPHISTORY_H */

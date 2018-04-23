/** @file triphistory.h */

#ifndef TRIPHISTORY_H
#define TRIPHISTORY_H

#include <deque>
#include "window_gui.h"
#include "strings_type.h"
#include "economy_type.h"
#include "date_type.h"

// entries to save
#define TRIP_LENGTH 10

static inline int TripHistoryRound(float x)
{
	return int(x > 0.0 ? x + 0.5 : x - 0.5);
}

struct TripHistoryEntry {
	Money profit; // Saved
	Date date; // Saved
	int32 profit_change; // Calculated
	Date TBT; // Calculated
	int32 TBT_change; // Calculated

	TripHistoryEntry() : profit(0), date(0), profit_change(0), TBT(0), TBT_change(0) { };
};

/** Structure to hold data for each vehicle */
struct TripHistory {
	// a lot of saveload stuff for std::deque. So...
	TripHistoryEntry t[TRIP_LENGTH];

	Money total_profit;
	int32 avg_daylength;
	int32 total_change;
	Money profit_per_day;

	TripHistory() :
		total_profit(0),
		avg_daylength(0),
		total_change(0),
		profit_per_day(0) { }

	void NewRound();

	void AddValue(Money mvalue, Date dvalue);


	/**
	 * Init info for GUI
	 *
	 * @return size_t number of valid rows
	 */
	size_t UpdateCalculated();

	int32 FindPercentChange(Money v1, Money v2) {
		float temp;

		if (v1 > v2) {
			temp = v1 - v2;
			return TripHistoryRound((float)temp * 100 / (float)v1);
		}

		if (v2 > v1) {
			temp = v1 - v2;
			return TripHistoryRound((float)temp * 100 / (float)v2);
		}

		return 0;
	}
};

#endif /* TRIPHISTORY_H */

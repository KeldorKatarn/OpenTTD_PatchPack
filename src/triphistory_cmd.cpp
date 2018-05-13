/** @file triphistory_cmd.cpp */

#include "stdafx.h"
#include "triphistory.h"
#include "table/strings.h"
#include <numeric>

void TripHistory::AddValue(Money profit, Ticks ticks)
{
	if (ticks > 0) {
		this->entries[0].profit += profit;
		this->entries[0].ticks = ticks;
	}
}

void TripHistory::NewRound()
{
	if (this->entries[1].ticks != 0) {
		this->entries[0].time_between_trips = this->entries[0].ticks - this->entries[1].ticks;
	}

	if (this->entries[1].profit != 0) {
		this->entries[0].profit_change = FindPercentChange(this->entries[0].profit, this->entries[1].profit);
	}

	if (this->entries[1].time_between_trips != 0) {
		this->entries[0].time_between_trips_change =
			FindPercentChange(this->entries[0].time_between_trips, this->entries[1].time_between_trips);
	}

	std::rotate(std::begin(this->entries), std::end(this->entries) - 1, std::end(this->entries));

	this->entries[0].profit = 0;
	this->entries[0].ticks = this->entries[1].ticks;
}

/**
* Update info for GUI
*
* @return Number of valid rows
*/
int32 TripHistory::UpdateCalculated(bool update_entries)
{
	if (update_entries) {
		for (auto i = 0; i < lengthof(this->entries) - 1; ++i) {
			if (this->entries[i + 1].ticks != 0) {
				this->entries[i].time_between_trips = this->entries[i].ticks - this->entries[i + 1].ticks;
			}

			if (this->entries[i + 1].profit != 0) {
				this->entries[i].profit_change = FindPercentChange(this->entries[i].profit, this->entries[i + 1].profit);
			}

			if (this->entries[i + 1].time_between_trips != 0) {
				this->entries[i].time_between_trips_change =
					FindPercentChange(this->entries[i].time_between_trips, this->entries[i + 1].time_between_trips);
			}
		}
	}

	// We ignore the first entry since it's still ongoing and would mess up the averages.
	this->total_profit = std::accumulate(std::begin(this->entries) + 1, std::end(this->entries), (Money)0,
		[](auto sum, auto entry) { return sum + entry.profit; });

	auto total_time_between_trips = std::accumulate(std::begin(this->entries) + 1, std::end(this->entries), 0,
		[](auto sum, auto entry) { return sum + entry.time_between_trips; });

	auto valid_entries = std::count_if(std::begin(this->entries), std::end(this->entries),
		[](TripHistoryEntry entry) { return entry.ticks != 0; });

	auto valid_tbt_entries = std::count_if(std::begin(this->entries) + 1, std::end(this->entries),
		[](TripHistoryEntry entry) { return entry.time_between_trips != 0; });

	this->avg_profit = (valid_entries > 1) ? (total_profit / Money(valid_entries - 1)) : Money(0);
	this->avg_time_between_trips = (valid_tbt_entries > 0) ? (total_time_between_trips / valid_tbt_entries) : 0;

	return (int32)valid_entries;
}

int32 TripHistory::FindPercentChange(float current_value, float previous_value)
{
	return std::round(((current_value - previous_value) * 100.0f) / (float)previous_value);
}

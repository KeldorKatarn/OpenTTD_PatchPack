/* $Id: timetable.h 18809 2010-01-15 16:41:15Z rubidium $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file timetable.h Functions related to time tabling. */

#ifndef TIMETABLE_H
#define TIMETABLE_H

#include "date_type.h"
#include "vehicle_type.h"

void ShowTimetableWindow(const Vehicle *v);
void UpdateVehicleTimetable(Vehicle *v, bool travelling);
void SetTimetableParams(Ticks ticks, int param1 = 0);

#endif /* TIMETABLE_H */

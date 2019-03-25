/*
 * Copyright (c) 2018-2019  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "qapi_types.h"

#include "qurt_thread.h"
#include "qurt_timer.h"

#include "qapi_status.h"
#include "qapi_reset.h"

#include "xsmain.h"

void qca4020_delay(uint32_t delayMS) {
	uint32_t	attr;
	uint32_t	curSignals;
	qurt_time_t	qtime;

	qtime = qurt_timer_convert_time_to_ticks(delayMS, QURT_TIME_MSEC);

	attr = QURT_SIGNAL_ATTR_WAIT_ANY | QURT_SIGNAL_ATTR_CLEAR_MASK;
	qurt_signal_wait_timed(&gMainSignal, kSIG_WAKE_MAIN, attr, &curSignals, qtime);
}

uint32_t qca4020_milliseconds() {
	qurt_time_t ticks_now;

	ticks_now = qurt_timer_get_ticks();
	return qurt_timer_convert_ticks_to_time(ticks_now, QURT_TIME_MSEC);
}

void qca4020_watchdog() {
	qapi_System_WDTCount_Reset();
}


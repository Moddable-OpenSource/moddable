/*
 * Copyright (c) 2016-20 Moddable Tech, Inc.
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

#include "xsmc.h"
#include "xsHost.h"
#include "nrf_sdh.h"
#include "nrf_timer.h"

#define TIMER NRF_TIMER4
#define CHANNEL NRF_TIMER_CC_CHANNEL0

void xs_microseconds_start(xsMachine *the)
{
	if (nrf52_softdevice_enabled()) {
		uint32_t running = 0;
		sd_clock_hfclk_request();
		while (0 == running)
			sd_clock_hfclk_is_running(&running);
	}
	else {
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
		NRF_CLOCK->TASKS_HFCLKSTART = 1;
		while (0 == NRF_CLOCK->EVENTS_HFCLKSTARTED) {}
	}
	TIMER->TASKS_STOP = 1;
	TIMER->TASKS_CLEAR = 1;
	TIMER->MODE = NRF_TIMER_MODE_TIMER;
	TIMER->PRESCALER = NRF_TIMER_FREQ_1MHz;
	TIMER->BITMODE = NRF_TIMER_BIT_WIDTH_32;
	TIMER->TASKS_START = 1;
}

void xs_microseconds_stop(xsMachine *the)
{
	TIMER->TASKS_CAPTURE[CHANNEL] = 1;
	xsmcSetInteger(xsResult, TIMER->CC[CHANNEL]);
	TIMER->TASKS_STOP = 1;
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	if (nrf52_softdevice_enabled())
		sd_clock_hfclk_release();
	else
		NRF_CLOCK->TASKS_HFCLKSTOP = 1;
}

void xs_microseconds_get(xsMachine *the)
{
	TIMER->TASKS_CAPTURE[CHANNEL] = 1;
	xsmcSetInteger(xsResult, TIMER->CC[CHANNEL]);
}

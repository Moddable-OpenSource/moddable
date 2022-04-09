/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
#include "xsPlatform.h"

#include "nrf_delay.h"

#include "task.h"

void nrf52_delay(uint32_t delayMS) {

	nrf_delay_us(delayMS * 1000);
}

uint32_t nrf52_milliseconds() {
	return ((uint64_t)xTaskGetTickCount() * (uint64_t)1000) >> 10;
}

/* https://devzone.nordicsemi.com/f/nordic-q-a/38551/three-errors-found-from-sdk15-2-0 */
uint32_t app_timer_cnt_get(void)
{
    if (__get_IPSR() != 0)
    {
        // inside ISR
        return (uint32_t)xTaskGetTickCountFromISR();
    }
    else
    {
        return (uint32_t)xTaskGetTickCount();
    }
} 

uint32_t app_timer_cnt_diff_compute(uint32_t ticks_to, uint32_t ticks_from)
{
	return (ticks_to - ticks_from);
}

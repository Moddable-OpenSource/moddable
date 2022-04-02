/*
 * Copyright (c) 2020  Moddable Tech, Inc.
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

#if ESP32

#include "esp_timer.h"

void xs_time_microseconds(xsMachine *the)
{
	xsNumberValue microseconds = (xsNumberValue)esp_timer_get_time();
	xsmcSetNumber(xsResult, microseconds);
}

#elif defined(__ets__)

void xs_time_microseconds(xsMachine *the)
{
	modTimeVal tv;
	modGetTimeOfDay(&tv, NULL);
	xsmcSetNumber(xsResult, ((xsNumberValue)tv.tv_sec * 1000000.0) + (xsNumberValue)tv.tv_usec);
}

#elif mxMacOSX

void xs_time_microseconds(xsMachine *the)
{
	CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
	xsmcSetNumber(xsResult, now * 1000000.0);
}

#elif defined(PICO_BOARD)

#include "time.h"

void xs_time_microseconds(xsMachine *the)
{
	uint64_t now = time_us_64();
	xsmcSetNumber(xsResult, now);
}

#else
	#error microseconds unsupported
#endif


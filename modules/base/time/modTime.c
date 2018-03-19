/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
#if _RENESAS_SYNERGY_
	#include "xssynergy.h"
#elif gecko
	#include "xsgecko.h"
#else
	#include "xsesp.h"
#endif

void xs_time_set(xsMachine *the)
{
	uint32_t seconds = xsmcToInteger(xsArg(0));
	modSetTime(seconds);
}

void xs_time_timezone_get(xsMachine *the)
{
	xsmcSetInteger(xsResult, modGetTimeZone());
}

void xs_time_timezone_set(xsMachine *the)
{
	int32_t seconds = xsmcToInteger(xsArg(0));
	modSetTimeZone(seconds);
}

void xs_time_dst_get(xsMachine *the)
{
	xsmcSetInteger(xsResult, modGetDaylightSavingsOffset());
}

void xs_time_dst_set(xsMachine *the)
{
	int32_t seconds = xsmcToInteger(xsArg(0));
	modSetDaylightSavingsOffset(seconds);
}

void xs_time_ticks(xsMachine *the)
{
	xsmcSetInteger(xsResult, modMilliseconds());
}

/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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


#include "xs.h"

void xs_time_set(xsMachine *the)
{
}

void xs_time_timezone_get(xsMachine *the)
{
}

void xs_time_timezone_set(xsMachine *the)
{
}

void xs_time_dst_get(xsMachine *the)
{
}

void xs_time_dst_set(xsMachine *the)
{
}

void xs_time_ticks(xsMachine *the)
{
    c_timeval tv;
	c_gettimeofday(&tv, NULL);
	xsResult = xsNumber((uint32_t)(uint64_t)(((double)(tv.tv_sec) * 1000.0) + ((double)(tv.tv_usec) / 1000.0)));
}

void xs_time_delta(xsMachine *the)
{
	xsNumberValue start, end;
	
	start = xsToNumber(xsArg(0));
	if (xsToNumber(xsArgc) > 1)
		end = xsToNumber(xsArg(1));
	else {
		c_timeval tv;
		c_gettimeofday(&tv, NULL);
		end = ((double)(tv.tv_sec) * 1000.0) + ((double)(tv.tv_usec / 1000));
	}
	xsResult = xsNumber(end - start);
}

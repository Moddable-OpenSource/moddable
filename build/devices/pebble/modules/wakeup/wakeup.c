/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
#include "mc.xs.h"      // for xsID_ values

#include "applib/app_wakeup.h"

void xs_wakeup_cancel(xsMachine *the)
{
	if (xsmcArgc > 0)
		app_wakeup_cancel(xsmcToInteger(xsArg(0)));
	else
		app_wakeup_cancel_all();
}

void xs_wakeup_query(xsMachine *the)
{
	WakeupId id = xsmcToInteger(xsArg(0));
	time_t time;
	bool scheduled = app_wakeup_query(id, &time);

	xsResult = xsmcNewObject();
	xsSlot tmp;
	xsmcSetNumber(tmp, time * 1000.0);		// seconds to JavaScript milliseconds
	xsmcSet(xsResult, xsID_time, tmp);

	xsmcSetBoolean(tmp, scheduled);
	xsmcSet(xsResult, xsID_scheduled, tmp);
}

void xs_wakeup_schedule(xsMachine *the)
{
	time_t time = xsmcToNumber(xsArg(0)) / 1000.0;		// JavaScript milliseconds to seconds
	int32_t cookie = xsmcToInteger(xsArg(1));
	bool notify_if_missed = xsmcToBoolean(xsArg(2));	
	WakeupId id = app_wakeup_schedule(time, cookie, notify_if_missed);
	if (id < 0)
		xsUnknownError("app_wakeup_schedule failed %d", id);
	xsmcSetInteger(xsResult, id);
}

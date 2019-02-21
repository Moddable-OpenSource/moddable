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
#include "inttypes.h"

#include "modTimer.h"

typedef struct modTimerScriptRecord modTimerScriptRecord;
typedef modTimerScriptRecord *modTimerScript;

struct modTimerScriptRecord {
	xsMachine *the;
	xsSlot slot;
};

static void xs_timer_callback(modTimer timer, void *refcon, int refconSize)
{
	modTimerScript ts = refcon;

	xsBeginHost(ts->the);
		xsCallFunction1(ts->slot, xsGlobal, xsInteger(modTimerGetID(timer)));

		if (0 == modTimerGetSecondInterval(timer))
			xsForget(ts->slot);

	xsEndHost(ts->the);
}

void xs_timer_add(xsMachine *the)
{
	int argc = xsmcArgc;
	int interval = 0, repeat = 0;
	modTimer timer;
	modTimerScriptRecord ts;

	if (argc > 1) {
		interval = xsmcToInteger(xsArg(1));
		if (argc > 2)
			repeat = xsmcToInteger(xsArg(2));
	}

	ts.the = the;
	ts.slot = xsArg(0);
	timer = modTimerAdd(interval, repeat, xs_timer_callback, &ts, sizeof(ts));
	xsRemember(((modTimerScript)modTimerGetRefcon(timer))->slot);

	xsmcSetInteger(xsResult, modTimerGetID(timer));
}

void xs_timer_repeat(xsMachine *the)
{
	int interval = xsmcToInteger(xsArg(1));
	modTimer timer;
	modTimerScriptRecord ts;

	ts.the = the;
	ts.slot = xsArg(0);
	timer = modTimerAdd(interval, interval, xs_timer_callback, &ts, sizeof(ts));
	xsRemember(((modTimerScript)modTimerGetRefcon(timer))->slot);

	xsmcSetInteger(xsResult, modTimerGetID(timer));
}

void xs_timer_schedule(xsMachine *the)
{
	int interval = xsmcToInteger(xsArg(1)), repeat = 0;
	int id = xsmcToInteger(xsArg(0));
	modTimer timer = modTimerFind((short)id);
	if (NULL == timer)
		xsUnknownError("invalid timer id");

	if (xsmcArgc > 2)
		repeat = xsmcToInteger(xsArg(2));

	modTimerReschedule(timer, interval, repeat);
}

void xs_timer_clear(xsMachine *the)
{
	modTimerScript ts;
	int id = xsmcToInteger(xsArg(0));
	modTimer timer = modTimerFind((short)id);
	if (NULL == timer)
		xsUnknownError("invalid timer id");

	ts = (modTimerScript)modTimerGetRefcon(timer);
	xsForget(ts->slot);

	modTimerRemove(timer);
}

void xs_timer_delay(xsMachine *the)
{
	modTimerDelayMS(xsmcToInteger(xsArg(0)));
}

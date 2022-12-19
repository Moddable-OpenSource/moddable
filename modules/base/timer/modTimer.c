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

#include "modTimer.h"
#include "xsHost.h"

static void modTimerMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void modTimerDelete(void *data);

static const xsHostHooks modTimerHooks ICACHE_RODATA_ATTR = {
	modTimerDelete,
	modTimerMark,
	NULL
};
typedef struct modTimerScriptRecord modTimerScriptRecord;
typedef modTimerScriptRecord *modTimerScript;

struct modTimerScriptRecord {
	xsMachine *the;
	xsSlot *callback;
	xsSlot self;
};

void modTimerMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	modTimerScript ts = (modTimerScript)modTimerGetRefcon((modTimer)it);

	if (ts->callback)
		(*markRoot)(the, ts->callback);
}

void modTimerDelete(void *data)
{
	if (data)
		modTimerRemove((modTimer)data);
}

static void xs_timer_callback(modTimer timer, void *refcon, int refconSize)
{
	modTimerScript ts = refcon;
	xsMachine *the = ts->the;

	xsBeginHost(the);
		xsCallFunction1(xsReference(ts->callback), xsGlobal, xsAccess(ts->self));
	xsEndHost(the);

	if (ts->callback && (0 == modTimerGetSecondInterval(timer))) {
		xsForget(ts->self);
		xsmcSetHostData(ts->self, NULL);
		xsmcSetHostDestructor(ts->self, NULL);
	}
}

static void createTimer(xsMachine *the, int interval, int repeat)
{
	modTimer timer;
	modTimerScriptRecord ts;

	ts.the = the;
	ts.callback = xsToReference(xsArg(0));
	ts.self = xsNewHostObject(NULL);
	timer = modTimerAdd(interval, repeat, xs_timer_callback, &ts, sizeof(ts));
	if (!timer)
		xsUnknownError("add failed");
	xsRemember(((modTimerScript)modTimerGetRefcon(timer))->self);

	xsmcSetHostData(ts.self, timer);
	xsSetHostHooks(ts.self, &modTimerHooks);
	xsResult = ts.self;
}

void xs_timer_set(xsMachine *the)
{
	int argc = xsmcArgc;
	int interval = 0, repeat = 0;

	if (argc > 1) {
		interval = xsmcToInteger(xsArg(1));
		if (argc > 2)
			repeat = xsmcToInteger(xsArg(2));
	}

	createTimer(the, interval, repeat);
}

void xs_timer_repeat(xsMachine *the)
{
	int interval = xsmcToInteger(xsArg(1));

	createTimer(the, interval, interval);
}

void xs_timer_schedule(xsMachine *the)
{
	int argc = xsmcArgc;
	modTimer timer = xsmcGetHostDataValidate(xsArg(0), (void *)&modTimerHooks);

	if (1 == argc)
		modTimerUnschedule(timer);
	else {
		int interval = xsmcToInteger(xsArg(1));
		int repeat = (argc > 2) ? xsmcToInteger(xsArg(2)) : 0;
		modTimerReschedule(timer, interval, repeat);
	}
}

void xs_timer_clear(xsMachine *the)
{
	modTimerScript ts;
	modTimer timer;

	if (xsReferenceType != xsmcTypeOf(xsArg(0)))
		return;
	
	timer = xsmcGetHostDataValidate(xsArg(0), (void *)&modTimerHooks);
	ts = (modTimerScript)modTimerGetRefcon(timer);
	xsForget(ts->self);
	ts->callback = NULL;
	modTimerRemove(timer);
	xsmcSetHostData(xsArg(0), NULL);
	xsmcSetHostDestructor(xsArg(0), NULL);
}

void xs_timer_delay(xsMachine *the)
{
	modTimerDelayMS(xsmcToInteger(xsArg(0)));
}

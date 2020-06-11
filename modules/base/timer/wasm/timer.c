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

#include "mc.xs.h"
#include <emscripten.h>

typedef struct {
	xsMachine *the;
	xsSlot slot;
	xsSlot callback;
	xsIntegerValue interval;
	xsIntegerValue repeat;
	xsIntegerValue count;
} ModTimerRecord, *ModTimer;

static void ModTimerCallback(void* data);
static ModTimer ModTimerCreate(xsMachine* the);
static void ModTimerDelete(void* it);

void ModTimerCallback(void* data)
{
	ModTimer self = data;
	self->count--;
	if (self->count) {
		xsBeginHost(self->the);
		xsVars(2);
		xsVar(0) = xsAccess(self->callback);
		xsVar(1) = xsAccess(self->slot);
		xsCallFunction1(xsVar(0), xsGlobal, xsVar(1));
		xsEndHost(self->the);
		if (self->repeat) {
			self->count++;
			emscripten_async_call(ModTimerCallback, self, self->repeat);
		}
	}
	else {
		c_free(self);
	}
}

ModTimer ModTimerCreate(xsMachine* the)
{
	ModTimer self = c_malloc(sizeof(ModTimerRecord));
	xsResult = xsNewHostObject(ModTimerDelete);
	self->the = the;
	self->slot = xsResult;
	self->callback = xsArg(0);
	xsRemember(self->slot);
	xsRemember(self->callback);
	xsSetHostData(xsResult, self);
	self->count = 1;
	return self;
}

void ModTimerDelete(void* it)
{
	ModTimer self = (ModTimer)it;
	if (self) { // delete machine
		self->count--;
		if (self->count == 0) {
			c_free(self);
		}
	}
}

void xs_timer_set(xsMachine *the)
{
	int argc = xsToInteger(xsArgc);
	ModTimer self = ModTimerCreate(the);
	self->interval = (argc > 1) ? xsToInteger(xsArg(1)) : 0;
	self->repeat = (argc > 2) ? xsToInteger(xsArg(2)) : 0;
	self->count++;
	emscripten_async_call(ModTimerCallback, self, self->interval);
}

void xs_timer_repeat(xsMachine *the)
{
	ModTimer self = ModTimerCreate(the);
	self->interval = self->repeat = xsToInteger(xsArg(1));
	self->count++;
	emscripten_async_call(ModTimerCallback, self, self->interval);
}

void xs_timer_schedule(xsMachine *the)
{
	int argc = xsToInteger(xsArgc);
	ModTimer self;
	xs_timer_clear(the);
	xsArg(0) = xsResult;
	self = ModTimerCreate(the);
	self->interval = xsToInteger(xsArg(1));
	self->repeat = (argc > 2) ? xsToInteger(xsArg(2)) : 0;
	self->count++;
	emscripten_async_call(ModTimerCallback, self, self->interval);
}

void xs_timer_clear(xsMachine *the)
{
	ModTimer self = (ModTimer)xsGetHostData(xsArg(0));
	if (NULL == self)
		xsUnknownError("invalid timer");
	xsResult = xsAccess(self->callback);
	xsForget(self->callback);
	xsForget(self->slot);
	xsSetHostData(xsArg(0), NULL);
	self->count--;
	if (self->count == 0) {
		c_free(self);
	}
}

void xs_timer_delay(xsMachine *the)
{
	emscripten_sleep(xsToInteger(xsArg(0)));
}


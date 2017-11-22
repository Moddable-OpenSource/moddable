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

typedef struct {
	xsMachine *the;
	xsSlot slot;
	xsSlot callback;
	xsIntegerValue interval;
	xsIntegerValue repeat;
	guint g_timer;
} ModTimerRecord, *ModTimer;

static gboolean ModTimerCallback(gpointer data);
static ModTimer ModTimerCreate(xsMachine* the);
static void ModTimerDelete(void* it);
static void ModTimerRemove(ModTimer self);

gboolean ModTimerCallback(gpointer data)
{
	ModTimer self = data;
	xsBeginHost(self->the);
	xsVars(2);
	xsVar(0) = xsAccess(self->callback);
	xsVar(1) = xsAccess(self->slot);
	xsCallFunction1(xsVar(0), xsGlobal, xsVar(1));
	xsEndHost(self->the);
	if (self->repeat) {
		if (self->interval != self->repeat) {
			self->interval = self->repeat;
			self->g_timer = g_timeout_add(self->interval, ModTimerCallback, self);
			return G_SOURCE_REMOVE;
		}
		return G_SOURCE_CONTINUE;
	}
	self->g_timer = 0;
	return G_SOURCE_REMOVE;
}

ModTimer ModTimerCreate(xsMachine* the)
{
	ModTimer self = c_malloc(sizeof(ModTimerRecord));
	xsResult = xsNewHostObject(ModTimerDelete);
	xsSetHostData(xsResult, self);
	self->the = the;
	self->slot = xsResult;
	self->callback = xsArg(0);
	xsRemember(self->slot);
	xsRemember(self->callback);
	return self;
}

void ModTimerDelete(void* it)
{
	if (it) {
		ModTimerRemove(it);
		c_free(it);
	}
}

void ModTimerRemove(ModTimer self)
{
	guint g_timer = self->g_timer;
	if (g_timer) {
		g_source_remove(g_timer);
		self->g_timer = 0;
	}
}

void xs_timer_set(xsMachine *the)
{
	int argc = xsToInteger(xsArgc);
	ModTimer self = ModTimerCreate(the);
	self->interval = (argc > 1) ? xsToInteger(xsArg(1)) : 0;
	self->repeat = (argc > 2) ? xsToInteger(xsArg(2)) : 0;
	self->g_timer = g_timeout_add(self->interval, ModTimerCallback, self);
}

void xs_timer_repeat(xsMachine *the)
{
	ModTimer self = ModTimerCreate(the);
	self->interval = self->repeat = xsToInteger(xsArg(1));
	self->g_timer = g_timeout_add(self->interval, ModTimerCallback, self);
}

void xs_timer_schedule(xsMachine *the)
{
	int argc = xsToInteger(xsArgc);
	ModTimer self = (ModTimer)xsGetHostData(xsArg(0));
	ModTimerRemove(self);
	self->interval = xsToInteger(xsArg(1));
	self->repeat = (argc > 2) ? xsToInteger(xsArg(2)) : 0;
	self->g_timer = g_timeout_add(self->interval, ModTimerCallback, self);
}

void xs_timer_clear(xsMachine *the)
{
	ModTimer self = (ModTimer)xsGetHostData(xsArg(0));
	xsForget(self->callback);
	xsForget(self->slot);
	ModTimerDelete(self);
	xsSetHostData(xsArg(0), NULL);
}

void xs_timer_delay(xsMachine *the)
{
	usleep(xsToInteger(xsArg(0)) * 1000);
}


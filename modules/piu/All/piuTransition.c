/*
 * Copyright (c) 2016-2024  Moddable Tech, Inc.
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

#include "piuAll.h"

static void PiuTransitionBegin(PiuTransition* self);
static void PiuTransitionIdle(void* it, PiuInterval interval);
static void PiuTransitionEnd(PiuTransition* self);
static void PiuTransitionMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuTransitionStep(PiuTransition* self, double fraction);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuTransitionDispatchRecord = {
	"Transition",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	PiuTransitionIdle,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

const xsHostHooks ICACHE_FLASH_ATTR PiuTransitionHooks = {
	PiuTransitionDelete,
	PiuTransitionMark,
	NULL
};

void PiuTransitionBegin(PiuTransition* self)
{
	xsBeginHost((*self)->the);
	xsVars(3);
	xsVar(0) = xsReference((*self)->reference);
	xsVar(1) = xsGet(xsVar(0), xsID_onBegin);
	xsVar(2) = xsGet(xsVar(0), xsID_parameters);
	(void)xsCall2(xsVar(1), xsID_apply, xsVar(0), xsVar(2));
	(void)xsCall1(xsVar(0), xsID_onStep, xsNumber(0));
	xsEndHost((*self)->the);
}

void PiuTransitionComplete(PiuTransition* self, PiuContainer* container)
{
	PiuTransitionStep(self, 1);
	PiuApplicationStopContent((*self)->application, self);
	PiuTransitionEnd(self);
	(*self)->time = -1;
	(*self)->container = NULL;
	(*self)->application = NULL;
	self = (*self)->next;
	while (self) {
		PiuTransitionBegin(self);
		PiuTransitionStep(self, 1);
		PiuTransitionEnd(self);
		self = (*self)->next;
	}
	(*container)->transition = NULL;
	PiuBehaviorOnTransitionEnded(container);
}

void PiuTransitionCreate(xsMachine* the)
{
	PiuTransition* self;
	xsSetHostChunk(xsThis, NULL, sizeof(PiuTransitionRecord));
	self = PIU(Transition, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuTransitionHooks);
	(*self)->dispatch = (PiuDispatch)&PiuTransitionDispatchRecord;
	(*self)->duration = xsToNumber(xsArg(0));
	(*self)->interval = 1;
	(*self)->time = -1;
	xsResult = xsThis;
}

void PiuTransitionDelete(void* it)
{
}

void PiuTransitionEnd(PiuTransition* self)
{
	xsBeginHost((*self)->the);
	xsVars(3);
	xsVar(0) = xsReference((*self)->reference);
	xsVar(1) = xsGet(xsVar(0), xsID_onEnd);
	xsVar(2) = xsGet(xsVar(0), xsID_parameters);
	(void)xsCall2(xsVar(1), xsID_apply, xsVar(0), xsVar(2));
	xsDelete(xsVar(0), xsID_parameters);
	xsEndHost((*self)->the);
}

void PiuTransitionIdle(void* it, PiuInterval interval)
{
    PiuTransition* self = it;
    double duration = (*self)->duration;
    double time = (*self)->time;
	if (time < 0)
    	time = 0;
    else {
    	time += interval;
		if (time > duration)
			time = duration;
    }
	(*self)->time = time;
	PiuTransitionStep(self, time / duration);
	if (time == duration) {
		PiuContainer* container = (*self)->container;
		PiuTransition* transition = (*container)->transition = (*self)->next;
		PiuApplicationStopContent((*self)->application, self);
		PiuTransitionEnd(self);
		(*self)->time = -1;
		(*self)->container = NULL;
		(*self)->application = NULL;
		if (transition) {
			PiuTransitionBegin(transition);
			PiuApplicationStartContent((*transition)->application, transition);
		}
		else
			PiuBehaviorOnTransitionEnded(container);
	}
}

void PiuTransitionMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuTransition self = it;
	PiuMarkHandle(the, self->next);
	PiuMarkHandle(the, self->container);
}

void PiuTransitionRun(PiuTransition* self, PiuContainer* container)
{
	PiuTransition* transition = (*container)->transition;
	(*self)->application = (*container)->application;
	(*self)->container = container;
	if (transition) {
		while ((*transition)->next)
			transition = (*transition)->next;
		(*transition)->next = self;
	}
	else {
		(*container)->transition = self;
		PiuBehaviorOnTransitionBeginning(container);
		PiuTransitionBegin(self);
		PiuApplicationStartContent((*self)->application, self);
	}
}

void PiuTransitionStep(PiuTransition* self, double fraction)
{
	xsBeginHost((*self)->the);
	xsVars(1);
	xsVar(0) = xsReference((*self)->reference);
	(void)xsCall1(xsVar(0), xsID_onStep, xsNumber(fraction));
	xsEndHost((*self)->the);
}

void PiuTransition_get_duration(xsMachine *the)
{
	PiuTransition* self = PIU(Transition, xsThis);
	xsResult = xsNumber((*self)->duration);
}

void PiuTransition_set_duration(xsMachine *the)
{
	PiuTransition* self = PIU(Transition, xsThis);
	xsNumberValue duration = xsToNumber(xsArg(0));
	if (duration < 0)
		duration = 0;
	(*self)->duration = duration;
}

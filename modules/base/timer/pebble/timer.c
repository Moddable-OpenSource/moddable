/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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

 /*
 
	to do:

		is it possible to use a repeating timer to minimize time allocations?
 
 */

#include "xsmc.h"
#include "modTimer.h"
#include "xsHost.h"
#include "moddableAppState.h"

#include "modInstrumentation.h"

#include "kernel/util/sleep.h"

typedef struct modTimerRecord modTimerRecord;
typedef modTimerRecord *modTimer;

#define kTimerFlagFire (1 << 0)
#define kTimerFlagUnscheduled (1 << 1)

struct modTimerRecord {
	struct modTimerRecord  *next;

	uint32_t			triggerTime;			// ms
	uint32_t			repeatInterval;			// ms
	uint16_t			refconSize;
	uint8_t				flags;
	int8_t				useCount;
	modTimerCallback 	cb;
#if MOD_TASKS
	uintptr_t			task;
#endif
	char 				refcon[];
};

static void modTimerEventedExecute(void *);

static void modTimersScheduleNext(void)
{
	ModdablePebbleAppState state = (ModdablePebbleAppState)app_state_get_js_memory_api_context();
	state->eventedTimer = evented_timer_register_or_reschedule(state->eventedTimer,
		(uint32_t)modTimersNext(), modTimerEventedExecute, C_NULL);
}

void modTimerEventedExecute(void *)
{
	setModdableAppState(eventedTimer, EVENTED_TIMER_INVALID_ID);
	modTimersExecute();
	modTimersScheduleNext();
}

void modTimersExecute(void)
{
	ModdablePebbleAppState state = (ModdablePebbleAppState)app_state_get_js_memory_api_context();

	modCriticalSectionDeclare;
	uint32_t now = modMilliseconds();
	modTimer walker;
#if MOD_TASKS
	uintptr_t task = modTaskGetCurrent();
#endif

	// determine who is firing this time (timers added during this call are ineligible)
	modCriticalSectionBegin();
	for (walker = state->timers; NULL != walker; walker = walker->next) {
		if (walker->flags & (kTimerFlagUnscheduled | kTimerFlagFire))
			continue;
		int32_t delta = walker->triggerTime - now;
		if (delta <= 0)
			walker->flags |= kTimerFlagFire;
	}

	// service eligible callbacks. then reschedule (repeating) or remove (one shot)
	for (walker = state->timers; NULL != walker; ) {
		if (!(walker->flags & kTimerFlagFire)
#if MOD_TASKS
			|| (task != walker->task)
#endif
			) {
			walker = walker->next;
			continue;
		}

		walker->flags &= ~kTimerFlagFire;
		walker->triggerTime += walker->repeatInterval;		// non-drifting... OK?

		walker->useCount++;
		modCriticalSectionEnd();
		(walker->cb)(walker, walker->refcon, walker->refconSize);
		modCriticalSectionBegin();
		walker->useCount--;

		if ((walker->useCount <= 0) || (0 == walker->repeatInterval)) {
			modCriticalSectionEnd();
			modTimerRemove(walker);
			modCriticalSectionBegin();
			walker = state->timers;
			continue;
		}

		walker = walker->next;
	}

	modCriticalSectionEnd();
}

int modTimersNext(void)
{
	modCriticalSectionDeclare;
	int next = 60 * 60 * 1000;		// an hour
	uint32_t now = modMilliseconds();
	modTimer walker;
#if MOD_TASKS
	uintptr_t task = modTaskGetCurrent();
#endif

	modCriticalSectionBegin();

	for (walker = getModdableAppState(timers); NULL != walker; walker = walker->next) {
		if ((walker->flags & kTimerFlagUnscheduled)
#if MOD_TASKS
			|| (task != walker->task)
#endif
			)
			continue;

		int32_t delta = walker->triggerTime - now;
		if (delta < next) {
			if (delta <= 0) {
				modCriticalSectionEnd();
				return 0;
			}
			next = delta;
		}
	}

	modCriticalSectionEnd();

	return next;
}

modTimer modTimerAdd(int firstInterval, int secondInterval, modTimerCallback cb, void *refcon, int refconSize)
{
	ModdablePebbleAppState state = (ModdablePebbleAppState)app_state_get_js_memory_api_context();
	modCriticalSectionDeclare;
	modTimer timer;

	timer = c_malloc(sizeof(modTimerRecord) + refconSize);
	if (!timer) return NULL;

	timer->next = NULL;
	timer->triggerTime = modMilliseconds() + firstInterval;
	timer->repeatInterval = secondInterval;
	timer->flags = 0;
	timer->useCount = 1;
	timer->cb = cb;
#if MOD_TASKS
	timer->task = modTaskGetCurrent();
#endif
	timer->refconSize = refconSize;
	c_memmove(timer->refcon, refcon, refconSize);

	modCriticalSectionBegin();

	if (!state->timers)
		state->timers = timer;
	else {
		modTimer walker;

		for (walker = state->timers; walker->next; walker = walker->next)
			;
		walker->next = timer;
	}

	modCriticalSectionEnd();

	modInstrumentationAdjust(Timers, +1);

	modTimersScheduleNext();

	return timer;
}

void modTimerReschedule(modTimer timer, int firstInterval, int secondInterval)
{
	modCriticalSectionDeclare;
	modCriticalSectionBegin();
	timer->triggerTime = modMilliseconds() + firstInterval;
	timer->repeatInterval = secondInterval;
	timer->flags &= ~kTimerFlagUnscheduled;
	modCriticalSectionEnd();

	modTimersScheduleNext();
}

void modTimerUnschedule(modTimer timer)
{
	modCriticalSectionDeclare;
	modCriticalSectionBegin();
	timer->flags |= kTimerFlagUnscheduled;
	timer->flags &= ~kTimerFlagFire;	
	timer->repeatInterval = -1;
	modCriticalSectionEnd();

	modTimersScheduleNext();
}

int modTimerGetSecondInterval(modTimer timer)
{
	return timer->repeatInterval;
}

void *modTimerGetRefcon(modTimer timer)
{
	return timer->refcon;
}

void modTimerRemove(modTimer timer)
{
	modCriticalSectionDeclare;
	ModdablePebbleAppState state = (ModdablePebbleAppState)app_state_get_js_memory_api_context();
	modTimer walker, prev = NULL;

	modCriticalSectionBegin();

	for (walker = state->timers; NULL != walker; prev = walker, walker = walker->next) {
		if (timer == walker) {
			timer->flags = kTimerFlagUnscheduled;

			timer->useCount--;
			if (timer->useCount <= 0) {
				if (NULL == prev)
					state->timers = walker->next;
				else
					prev->next = walker->next;
				c_free(timer);
				modInstrumentationAdjust(Timers, -1);
			}
			break;
		}
	}

	modCriticalSectionEnd();

	if (state->timers)
		modTimersScheduleNext();
	else {
		evented_timer_cancel(state->eventedTimer);
		state->eventedTimer = EVENTED_TIMER_INVALID_ID;
	}
}

void modTimerDelayMS(uint32_t ms)
{
	modDelayMilliseconds(ms);
}

void modTimerExit(void)
{
	ModdablePebbleAppState state = (ModdablePebbleAppState)app_state_get_js_memory_api_context();

	while (state->timers) {
		modTimer next = ((modTimer)state->timers)->next;
		c_free(state->timers);
		state->timers = next;
	}
	evented_timer_cancel(state->eventedTimer);
	state->eventedTimer = EVENTED_TIMER_INVALID_ID;
}

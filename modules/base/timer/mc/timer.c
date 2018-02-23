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
#include "modTimer.h"

#if ESP32 || __ets__
	#include "xsesp.h"
#elif __ZEPHYR__
	#include "modTimer.h"
	#include "xsPlatform.h"
#elif defined(linux)
	#include "xslinux.h"
#elif defined(gecko)
	#include "xsgecko.h"
	#include "xsPlatform.h"
#elif defined(apollo)
	#include "xsapollo.h"
	#include "xsPlatform.h"
#else	// synergy
	#include <sys/types.h>
	#include "xssynergy.h"
	#include "xsPlatform.h"
	#include <string.h>
#endif

#include "modInstrumentation.h"

typedef struct modTimerRecord modTimerRecord;
typedef modTimerRecord *modTimer;

#define kTimerFlagFire (1)

struct modTimerRecord {
	struct modTimerRecord  *next;

	txS4 triggerTime;				// ms
	txS4 repeatInterval;			// ms
	txU2 refconSize;
	txS1 flags;
	txS1 useCount;
	modTimerCallback cb;
#if MOD_TASKS
	uintptr_t task;
#endif
	char refcon[1];
};

static modTimer gTimers = NULL;

void modTimersExecute(void)
{
	int now = modMilliseconds();
	modTimer walker;
#if MOD_TASKS
	uintptr_t task = modTaskGetCurrent();
#endif

	// determine who is firing this time (timers added during this call are ineligible)
	modCriticalSectionBegin();
	for (walker = gTimers; NULL != walker; walker = walker->next)
		walker->flags |= (walker->triggerTime <= now) ? kTimerFlagFire : 0;

	// service eligible callbacks. then reschedule (repeating) or remove (one shot)
	for (walker = gTimers; NULL != walker; ) {
		if (!(walker->flags & kTimerFlagFire) || !walker->cb
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
			walker = gTimers;
			continue;
		}

		walker = walker->next;
	}

	modCriticalSectionEnd();
}

int modTimersNext(void)
{
	int next = 60 * 60 * 1000;		// an hour
	int now = modMilliseconds();
	modTimer walker;
#if MOD_TASKS
	uintptr_t task = modTaskGetCurrent();
#endif

	modCriticalSectionBegin();

	for (walker = gTimers; NULL != walker; walker = walker->next) {
		int delta;

		if (!walker->cb
#if MOD_TASKS
			|| (task != walker->task)
#endif
			)
			continue;

		delta = walker->triggerTime - now;
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
	modTimer timer;

	timer = c_malloc(sizeof(modTimerRecord) + refconSize - 1);
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

	if (!gTimers)
		gTimers = timer;
	else {
		modTimer walker;

		for (walker = gTimers; walker->next; walker = walker->next)
			;
		walker->next = timer;
	}

	modCriticalSectionEnd();

	modInstrumentationAdjust(Timers, +1);

	return timer;
}

void modTimerReschedule(modTimer timer, int firstInterval, int secondInterval)
{
	modCriticalSectionBegin();
	timer->triggerTime = modMilliseconds() + firstInterval;
	timer->repeatInterval = secondInterval;
	modCriticalSectionEnd();
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
	modTimer walker, prev = NULL;

	modCriticalSectionBegin();

	for (walker = gTimers; NULL != walker; prev = walker, walker = walker->next) {
		if (timer == walker) {
			timer->cb = NULL;

			timer->useCount--;
			if (timer->useCount <= 0) {
				if (NULL == prev)
					gTimers = walker->next;
				else
					prev->next = walker->next;
				c_free(timer);
				modInstrumentationAdjust(Timers, -1);
			}
			break;
		}
	}

	modCriticalSectionEnd();
}

void modTimerDelayMS(uint32_t ms)
{
	modDelayMilliseconds(ms);
}

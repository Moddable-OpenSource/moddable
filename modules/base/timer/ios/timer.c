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
#include "modInstrumentation.h"

#define modCriticalSectionBegin()
#define modCriticalSectionEnd()

typedef struct modTimerRecord modTimerRecord;
typedef modTimerRecord *modTimer;

struct modTimerRecord {
	struct modTimerRecord  *next;

	CFRunLoopTimerRef cfTimer;
	int16_t id;
	int8_t useCount;
	modTimerCallback cb;
	uint32_t refconSize;
	char refcon[1];
};

static modTimer gTimers = NULL;
static txS2 gTimerID = 1;		//@@ could id share with other libraries that need unique ID?

static void modTimerExcecuteOne(CFRunLoopTimerRef cfTimer, void *info)
{
	modTimer timer = info;

	timer->useCount++;
	(timer->cb)(timer, timer->refcon, timer->refconSize);
	timer->useCount--;

	if ((timer->useCount <= 0) || (0 == modTimerGetSecondInterval(timer)))
		modTimerRemove(timer);
}

modTimer modTimerAdd(int firstInterval, int secondInterval, modTimerCallback cb, void *refcon, int refconSize)
{
	modTimer timer;
	CFRunLoopTimerContext context = {0};

	timer = c_malloc(sizeof(modTimerRecord) + refconSize - 1);
	if (!timer) return NULL;

	timer->next = NULL;
	timer->id = ++gTimerID;
	timer->useCount = 1;
	timer->cb = cb;
	timer->refconSize = refconSize;
	c_memmove(timer->refcon, refcon, refconSize);

	context.info = timer;
	timer->cfTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + (firstInterval / 1000.0),
					secondInterval / 1000.0, 0, 0, modTimerExcecuteOne, &context);
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer->cfTimer, kCFRunLoopCommonModes);

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
	CFRunLoopTimerContext context = {0};

	CFRunLoopTimerInvalidate(timer->cfTimer);

	context.info = timer;
	timer->cfTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + (firstInterval / 1000.0),
					secondInterval / 1000.0, 0, 0, modTimerExcecuteOne, &context);
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer->cfTimer, kCFRunLoopCommonModes);
}

uint16_t modTimerGetID(modTimer timer)
{
	return timer->id;
}

int modTimerGetSecondInterval(modTimer timer)
{
	return CFRunLoopTimerGetInterval(timer->cfTimer) * 1000;
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
			timer->id = 0;		// can't be found again by script
			timer->cb = NULL;

			timer->useCount--;
			if (timer->useCount <= 0) {
				if (NULL == prev)
					gTimers = walker->next;
				else
					prev->next = walker->next;
				CFRunLoopTimerInvalidate(timer->cfTimer);
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
	usleep(ms * 1000);
}

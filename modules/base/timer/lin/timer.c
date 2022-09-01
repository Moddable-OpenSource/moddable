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
#include "modInstrumentation.h"

#define modCriticalSectionBegin()
#define modCriticalSectionEnd()

typedef struct modTimerRecord modTimerRecord;
typedef modTimerRecord *modTimer;

struct modTimerRecord {
	struct modTimerRecord *next;
	guint g_timer;
	int16_t	id;
	int8_t	useCount;
	int32_t	first;
	int32_t	interval;
	modTimerCallback cb;
	uint32_t refconSize;
	char refcon[1];
};

static modTimer gTimers = NULL;
static txS2 gTimerID = 1;

static gboolean modTimerExecuteOne(gpointer data);
void modTimerRemove(modTimer timer);

gboolean modTimerExecuteOne(gpointer data)
{
	modTimer timer = data;

	if (0 == timer->id) {
		modTimerRemove(timer);
		return G_SOURCE_REMOVE;
	}

	timer->useCount++;
	(timer->cb)(timer, timer->refcon, timer->refconSize);
	timer->useCount--;

	if ((timer->useCount <= 0) || (0 == timer->interval)) {
		modTimerRemove(timer);
		return G_SOURCE_REMOVE;
	}

	if (timer->interval) {
		if (timer->first != timer->interval) {
			timer->g_timer = g_timeout_add(timer->interval, modTimerExecuteOne, timer);
			timer->first = timer->interval;
			return G_SOURCE_REMOVE;
		}
		return G_SOURCE_CONTINUE;
	}
	timer->g_timer = 0;
	return G_SOURCE_REMOVE;
}

modTimer modTimerAdd(int firstInterval, int secondInterval, modTimerCallback cb, void *refcon, int refconSize)
{
	modTimer timer;

	timer = c_malloc(sizeof(modTimerRecord) + refconSize - 1);
	if (!timer) return NULL;

	timer->next = NULL;
	timer->id = ++gTimerID;
	timer->useCount = 1;
	timer->cb = cb;
	timer->refconSize = refconSize;
	timer->first = firstInterval;
	timer->interval = secondInterval;
	c_memmove(timer->refcon, refcon, refconSize);

	if (firstInterval == 0 && secondInterval == 0)
		timer->g_timer = g_timeout_add_full(G_PRIORITY_HIGH, firstInterval, modTimerExecuteOne, timer, NULL);
	else
		timer->g_timer = g_timeout_add(firstInterval, modTimerExecuteOne, timer);

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
	if (timer->g_timer)
		g_source_remove(timer->g_timer);

	timer->first = firstInterval;
	timer->interval = secondInterval;

	if (firstInterval == 0 && secondInterval == 0)
		timer->g_timer = g_timeout_add_full(G_PRIORITY_HIGH, firstInterval, modTimerExecuteOne, timer, NULL);
	else
		timer->g_timer = g_timeout_add( firstInterval, modTimerExecuteOne, timer);

}

void modTimerUnschedule(modTimer timer)
{
	if (!timer || !timer->id)
		return;

	if (timer->g_timer)
		g_source_remove(timer->g_timer);
	timer->g_timer = 0;
}

uint16_t modTimerGetID(modTimer timer)
{
	return timer->id;
}

int modTimerGetSecondInterval(modTimer timer)
{
	return timer->g_timer ? timer->interval : -1;
}

void *modTimerGetRefcon(modTimer timer)
{
	return (void*)timer->refcon;
}

void modTimerRemove(modTimer timer)
{
	modTimer walker, prev = NULL;

	if (NULL == timer)
		return;

	if (timer->g_timer) {
		g_source_remove(timer->g_timer);
		timer->g_timer = 0;
	}

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


/*
 * Copyright (c) 2016-2023 Moddable Tech, Inc.
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
#include "mmsystem.h"

typedef struct modTimerRecord modTimerRecord;
typedef modTimerRecord *modTimer;

struct modTimerRecord {
	struct modTimerRecord  *next;

	UINT_PTR idEvent;
	int16_t id;
	int firstInterval;
	int secondInterval;
	int8_t useCount;
	uint8_t rescheduled;
	uint8_t repeating;
	modTimerCallback cb;
	HWND window;
	uint32_t refconSize;
	char refcon[1];
};

static modTimer gTimers = NULL;
static txS2 gTimerID = 1;		//@@ could id share with other libraries that need unique ID?
static BOOLEAN initializedCriticalSection = FALSE;
static CRITICAL_SECTION gCS;

static void modTimerExecuteOne(modTimer timer);
static modTimer modTimerFindNative(UINT_PTR idEvent);
void modTimerWindowCallback(LPARAM timer);
static void destroyTimer(modTimer timer);
static void createTimerWindow(modTimer timer);
LRESULT CALLBACK modTimerWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

static VOID CALLBACK TimerProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
	modTimer timer;

	timer = modTimerFindNative(uTimerID);
	if (timer)
		SendMessage(timer->window, WM_MODTIMER, 0, (LPARAM)timer);
}

static void modCriticalSectionBegin()
{
	EnterCriticalSection(&gCS);
}

static void modCriticalSectionEnd()
{
	LeaveCriticalSection(&gCS);
}

void modTimerWindowCallback(LPARAM t) {
	modTimer timer = (modTimer)t;
	modTimerExecuteOne(timer);
}

static void modTimerExecuteOne(modTimer timer)
{
	timer->rescheduled = 0;
	timer->useCount++;
	(timer->cb)(timer, timer->refcon, timer->refconSize);
	timer->useCount--;

	if ((timer->useCount <= 0) || (0 == modTimerGetSecondInterval(timer)))
		modTimerRemove(timer);
	else if (!(timer->repeating) && timer->secondInterval > 0 && !(timer->rescheduled)) {
		timer->repeating = true;
		timer->idEvent = timeSetEvent(timer->secondInterval, 1, TimerProc, timer->id, TIME_PERIODIC);
	}
}

static modTimer modTimerFindNative(UINT_PTR idEvent)
{
	modTimer walker;

	modCriticalSectionBegin();

	for (walker = gTimers; NULL != walker; walker = walker->next)
		if (idEvent == walker->idEvent)
			break;

	modCriticalSectionEnd();

	return walker;
}

modTimer modTimerAdd(int firstInterval, int secondInterval, modTimerCallback cb, void *refcon, int refconSize)
{
	modTimer timer;	

	if (!initializedCriticalSection) {
		InitializeCriticalSection(&gCS);
		initializedCriticalSection = TRUE;
	}

	timer = c_calloc(1, sizeof(modTimerRecord) + refconSize - 1);
	if (!timer) return NULL;

	timer->next = NULL;
	timer->id = gTimerID++;
	timer->firstInterval = firstInterval;
	timer->secondInterval = secondInterval;
	timer->useCount = 1;
	timer->repeating = 0;
	timer->cb = cb;
	createTimerWindow(timer);
	timer->refconSize = refconSize;
	c_memmove(timer->refcon, refcon, refconSize);

	if (firstInterval == 0)
		firstInterval = 1;
	timer->idEvent = timeSetEvent(firstInterval, 1, TimerProc, timer->id, TIME_ONESHOT);
	if (!(timer->idEvent)) {
		destroyTimer(timer);
		return NULL;
	} 

	timeBeginPeriod(1);

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
	timer->firstInterval = firstInterval;
	timer->secondInterval = secondInterval;

	if (timer->idEvent)
		timeKillEvent(timer->idEvent);
	
	if (firstInterval == 0)
		firstInterval = 1;
		
	timer->idEvent = timeSetEvent(firstInterval, 1, TimerProc, timer->id, TIME_ONESHOT);
	timer->rescheduled = 1;
	timer->repeating = 0;
}

void modTimerUnschedule(modTimer timer)
{
	timeKillEvent(timer->idEvent);
	timer->idEvent = 0;
	timer->rescheduled = 1;
	timer->repeating = 0;
}

uint16_t modTimerGetID(modTimer timer)
{
	return timer->id;
}

int modTimerGetSecondInterval(modTimer timer)
{
	if (timer->idEvent) {
		return timer->secondInterval;
	} else {
		return -1;
	}
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
			timeEndPeriod(1);

			timer->useCount--;
			if (timer->useCount <= 0) {
				if (NULL == prev)
					gTimers = walker->next;
				else
					prev->next = walker->next;
				if (timer->idEvent)
					timeKillEvent(timer->idEvent);
				destroyTimer(timer);
				modInstrumentationAdjust(Timers, -1);
			}
			break;
		}
	}

	modCriticalSectionEnd();
}

void modTimerDelayMS(uint32_t ms)
{
	timeBeginPeriod(1);
	Sleep(ms);
	timeEndPeriod(1);
}

LRESULT CALLBACK modTimerWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_MODTIMER: {
			modTimerWindowCallback(lParam);
			return 0;
		} break;
		default: 
			return DefWindowProc(window, message, wParam, lParam);
	}	
}

static void createTimerWindow(modTimer timer)
{
	WNDCLASSEX wcex = {0};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.cbWndExtra = sizeof(modTimer);
	wcex.lpfnWndProc = modTimerWindowProc;
	wcex.lpszClassName = "modTimerWindowClass";
	RegisterClassEx(&wcex);
	timer->window = CreateWindowEx(0, wcex.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
	SetWindowLongPtr(timer->window, 0, (LONG_PTR)timer);
}

static void destroyTimer(modTimer timer)
{
	if (timer->window)
		DestroyWindow(timer->window);
	c_free(timer);
}

/*
 * Copyright (c) 2016-2026 Moddable Tech, Inc.
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

	HANDLE hTimer;
	HWND window;
	uint32_t id;
	int firstInterval;
	int secondInterval;
	int8_t useCount;
	modTimerCallback cb;
	uint32_t refconSize;
	char refcon[];
};

typedef struct modThreadTimers modThreadTimers;
struct modThreadTimers {
	HWND window;
	modTimer head;
	uint32_t nextId;
};

static DWORD gFlsIndex = FLS_OUT_OF_INDEXES;
static INIT_ONCE gInitOnce = INIT_ONCE_STATIC_INIT;
static const char kTimerWindowClass[] = "modTimerWindowClass";

static modThreadTimers *getThreadTimers(BOOL create);
static modTimer findById(modThreadTimers *t, uint32_t id);
static void modTimerExecuteOne(modTimer timer);
static VOID CALLBACK timerQueueCallback(PVOID param, BOOLEAN timerOrWait);
static VOID CALLBACK threadCleanup(PVOID p);
static BOOL CALLBACK doOnce(PINIT_ONCE once, PVOID param, PVOID *ctx);
LRESULT CALLBACK modTimerWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

static BOOL CALLBACK doOnce(PINIT_ONCE once, PVOID param, PVOID *ctx)
{
	WNDCLASSEX wcex = {0};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = modTimerWindowProc;
	wcex.lpszClassName = kTimerWindowClass;
	RegisterClassEx(&wcex);
	gFlsIndex = FlsAlloc(threadCleanup);
	return TRUE;
}

static modThreadTimers *getThreadTimers(BOOL create)
{
	InitOnceExecuteOnce(&gInitOnce, doOnce, NULL, NULL);
	if (FLS_OUT_OF_INDEXES == gFlsIndex)
		return NULL;

	modThreadTimers *mtt = (modThreadTimers *)FlsGetValue(gFlsIndex);
	if (mtt || !create)
		return mtt;

	mtt = c_calloc(1, sizeof(modThreadTimers));
	if (!mtt) return NULL;
	mtt->nextId = 1;
	mtt->window = CreateWindowEx(0, kTimerWindowClass, NULL, 0, 0, 0, 0, 0,
		HWND_MESSAGE, NULL, NULL, NULL);
	if (!mtt->window) {
		c_free(mtt);
		return NULL;
	}
	SetWindowLongPtr(mtt->window, GWLP_USERDATA, (LONG_PTR)mtt);
	FlsSetValue(gFlsIndex, mtt);
	return mtt;
}

static modTimer findById(modThreadTimers *mtt, uint32_t id)
{
	if (0 == id) return NULL;
	for (modTimer w = mtt->head; w; w = w->next)
		if (w->id == id) return w;
	return NULL;
}

static VOID CALLBACK timerQueueCallback(PVOID param, BOOLEAN timerOrWait)
{
	modTimer timer = (modTimer)param;
	PostMessage(timer->window, WM_MODTIMER, (WPARAM)timer->id, 0);
}

LRESULT CALLBACK modTimerWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (WM_MODTIMER == message) {
		modThreadTimers *mtt = (modThreadTimers *)GetWindowLongPtr(window, GWLP_USERDATA);
		if (mtt) {
			modTimer timer = findById(mtt, (uint32_t)wParam);
			if (timer)
				modTimerExecuteOne(timer);
		}
		return 0;
	}
	return DefWindowProc(window, message, wParam, lParam);
}

static void modTimerExecuteOne(modTimer timer)
{
	timer->useCount++;
	(timer->cb)(timer, timer->refcon, timer->refconSize);
	timer->useCount--;

	if ((timer->useCount <= 0) || (0 == modTimerGetSecondInterval(timer)))
		modTimerRemove(timer);
}

modTimer modTimerAdd(int firstInterval, int secondInterval, modTimerCallback cb, void *refcon, int refconSize)
{
	modThreadTimers *mtt = getThreadTimers(TRUE);
	if (!mtt) return NULL;

	modTimer timer = c_calloc(1, sizeof(modTimerRecord) + refconSize);
	if (!timer) return NULL;

	timer->next = NULL;
	timer->id = mtt->nextId++;
	if (0 == mtt->nextId) mtt->nextId = 1;	// reserve 0 as "removed"
	timer->firstInterval = firstInterval;
	timer->secondInterval = secondInterval;
	timer->useCount = 1;
	timer->cb = cb;
	timer->window = mtt->window;
	timer->refconSize = refconSize;
	c_memmove(timer->refcon, refcon, refconSize);

	timeBeginPeriod(1);

	if (!CreateTimerQueueTimer(&timer->hTimer, NULL, timerQueueCallback, timer,
			(DWORD)(firstInterval < 0 ? 0 : firstInterval),
			(DWORD)(secondInterval > 0 ? secondInterval : 0),
			WT_EXECUTEINTIMERTHREAD)) {
		timeEndPeriod(1);
		c_free(timer);
		return NULL;
	}

	timer->next = mtt->head;
	mtt->head = timer;

	modInstrumentationAdjust(Timers, +1);

	return timer;
}

void modTimerReschedule(modTimer timer, int firstInterval, int secondInterval)
{
	timer->firstInterval = firstInterval;
	timer->secondInterval = secondInterval;

	if (timer->hTimer) {
		DeleteTimerQueueTimer(NULL, timer->hTimer, INVALID_HANDLE_VALUE);
		timer->hTimer = NULL;
	}

	CreateTimerQueueTimer(&timer->hTimer, NULL, timerQueueCallback, timer,
		(DWORD)(firstInterval < 0 ? 0 : firstInterval),
		(DWORD)(secondInterval > 0 ? secondInterval : 0),
		WT_EXECUTEINTIMERTHREAD);
}

void modTimerUnschedule(modTimer timer)
{
	if (timer->hTimer) {
		DeleteTimerQueueTimer(NULL, timer->hTimer, INVALID_HANDLE_VALUE);
		timer->hTimer = NULL;
	}
}

int modTimerGetSecondInterval(modTimer timer)
{
	return timer->hTimer ? timer->secondInterval : -1;
}

void *modTimerGetRefcon(modTimer timer)
{
	return timer->refcon;
}

void modTimerRemove(modTimer timer)
{
	modThreadTimers *mtt = getThreadTimers(FALSE);
	if (!mtt) return;

	modTimer walker, prev = NULL;
	for (walker = mtt->head; NULL != walker; prev = walker, walker = walker->next) {
		if (timer == walker) {
			timer->id = 0;		// invalidates any queued WM_MODTIMER
			timer->cb = NULL;

			if (timer->hTimer) {
				DeleteTimerQueueTimer(NULL, timer->hTimer, INVALID_HANDLE_VALUE);
				timer->hTimer = NULL;
			}

			timer->useCount--;
			if (timer->useCount <= 0) {
				if (NULL == prev)
					mtt->head = walker->next;
				else
					prev->next = walker->next;
				timeEndPeriod(1);
				c_free(timer);
				modInstrumentationAdjust(Timers, -1);
			}
			break;
		}
	}
}

void modTimerDelayMS(uint32_t ms)
{
	timeBeginPeriod(1);
	Sleep(ms);
	timeEndPeriod(1);
}

static VOID CALLBACK threadCleanup(PVOID p)
{
	modThreadTimers *mtt = (modThreadTimers *)p;
	if (!mtt) return;

	modTimer timer, next;
	for (timer = mtt->head; timer; timer = next) {
		next = timer->next;
		if (timer->hTimer)
			DeleteTimerQueueTimer(NULL, timer->hTimer, INVALID_HANDLE_VALUE);
		timeEndPeriod(1);
		c_free(timer);
//		modInstrumentationAdjust(Timers, -1);
	}

	if (mtt->window)
		DestroyWindow(mtt->window);
	c_free(mtt);
}

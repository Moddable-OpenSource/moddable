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

 #include "xsmc.h"

 #include "modTimer.h"
 #include "modInstrumentation.h"
 
#include "xsHost.h"
#include "services/common/evented_timer.h"
#include "kernel/util/sleep.h"

 typedef struct modTimerRecord modTimerRecord;
 typedef modTimerRecord *modTimer;
 
 struct modTimerRecord { 
	 EventedTimerID eventedTimer;
	 int8_t useCount;
	 int8_t checkRepeat;
	 modTimerCallback cb;
	 int secondInterval;
	 uint32_t refconSize;
	 char refcon[];
 };

 static void modTimerExcecuteOne(void* data)
 {
	 modTimer timer = data;
 
	 timer->useCount++;
	 (timer->cb)(timer, timer->refcon, timer->refconSize);
	 timer->useCount--;
 
	 if ((timer->useCount <= 0) || (0 == timer->secondInterval))
		 modTimerRemove(timer);
	else if (timer->checkRepeat) {
		timer->checkRepeat = 0;
		evented_timer_reschedule(timer->eventedTimer, timer->secondInterval);
	}	
 }
 
 modTimer modTimerAdd(int firstInterval, int secondInterval, modTimerCallback cb, void *refcon, int refconSize)
 {
	 modTimer timer = c_malloc(sizeof(modTimerRecord) + refconSize);
	 if (!timer) return C_NULL;
 
	if (firstInterval < 20)		//@@ hack-around. PebbleOS calls the timer many many times if this number is much smaller than 20
		firstInterval = 20;

	 timer->useCount = 1;
	 timer->cb = cb;
	 timer->refconSize = refconSize;
	 c_memmove(timer->refcon, refcon, refconSize);
 
	 timer->secondInterval = secondInterval;
	 timer->checkRepeat = 1;
	 timer->eventedTimer = evented_timer_register(firstInterval, true, modTimerExcecuteOne, timer);
 
	 modInstrumentationAdjust(Timers, +1);
 
	 return timer;
 }
 
 void modTimerReschedule(modTimer timer, int firstInterval, int secondInterval)
 {
	timer->secondInterval = secondInterval;
	timer->checkRepeat = 1;
	if (EVENTED_TIMER_INVALID_ID == timer->eventedTimer)
		timer->eventedTimer = evented_timer_register(firstInterval, true, modTimerExcecuteOne, timer);
	else
		evented_timer_reschedule(timer->eventedTimer, firstInterval);
 }
 
 void modTimerUnschedule(modTimer timer)
 {
	evented_timer_cancel(timer->eventedTimer);
	timer->eventedTimer = EVENTED_TIMER_INVALID_ID;
 }
 
 //@@ remove me
 uint16_t modTimerGetID(modTimer timer)
 {
	 return 0;
 }
 
 int modTimerGetSecondInterval(modTimer timer)
 {
	 return timer->secondInterval;
 }
 
 void *modTimerGetRefcon(modTimer timer)
 {
	 return timer->refcon;
 }
 
 void modTimerRemove(modTimer timer)
 {
	 timer->cb = NULL;
	 evented_timer_cancel(timer->eventedTimer);
	 timer->eventedTimer = EVENTED_TIMER_INVALID_ID;
 
	 timer->useCount--;
	 if (timer->useCount > 0)
	 	return;

	c_free(timer);
	modInstrumentationAdjust(Timers, -1);
 }
 
 void modTimerDelayMS(uint32_t ms)
 {
	psleep(ms * 1000);
 }
 
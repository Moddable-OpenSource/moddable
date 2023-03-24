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


#ifndef __MODTIMER_H__
#define __MODTIMER_H__

#include "inttypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct modTimerRecord modTimerRecord;
typedef struct modTimerRecord *modTimer;

typedef void (*modTimerCallback)(modTimer timer, void *refcon, int refconSize);
extern modTimer modTimerAdd(int firstInterval, int secondInterval, modTimerCallback cb, void *refcon, int refconSize);
extern void modTimerReschedule(modTimer timer, int firstInterval, int secondInterval);
extern uint16_t modTimerGetID(modTimer timer);
extern int modTimerGetSecondInterval(modTimer timer);
extern void *modTimerGetRefcon(modTimer timer);
extern void modTimerRemove(modTimer timer);
extern void modTimerUnschedule(modTimer timer);

extern void modTimerDelayMS(uint32_t ms);

#ifdef __cplusplus
	}
#endif

#endif

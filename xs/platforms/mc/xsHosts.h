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

#ifndef __XSHOSTS_H__
#define __XSHOSTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef mxInstrument
	#include "modTimer.h"
#endif

extern xsMachine *modCloneMachine(uint32_t allocation, uint32_t stack, uint32_t slotCount, uint32_t keyCount, const char *name);
extern void modRunMachineSetup(xsMachine *the);

extern char *modGetModAtom(xsMachine *the, uint32_t atomTypeIn, int *atomSizeOut);

extern void *modInstallMods(/* txPreparation */ void *preparation, uint8_t *status);

#ifdef mxInstrument
	extern void modInstrumentMachineBegin(xsMachine *the, modTimerCallback instrumentationCallback, int count, char **names, char **units);
	extern void modInstrumentMachineEnd(xsMachine *the);
	extern void modInstrumentMachineReset(xsMachine *the);

	extern int32_t modInstrumentationSlotHeapSize(xsMachine *the);
	extern int32_t modInstrumentationChunkHeapSize(xsMachine *the);
	extern int32_t modInstrumentationKeysUsed(xsMachine *the);
	extern int32_t modInstrumentationGarbageCollectionCount(xsMachine *the);
	extern int32_t modInstrumentationModulesLoaded(xsMachine *the);
	extern int32_t modInstrumentationStackRemain(xsMachine *the);
#endif

#ifdef __cplusplus
}
#endif

#endif // __XSHOSTS_H__

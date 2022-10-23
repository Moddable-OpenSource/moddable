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


#ifndef __modInstrumentation_h__
#define __modInstrumentation_h__

#if MODINSTRUMENTATION

#include "stdint.h"

enum {
	// Counters

	/* Poco */
	kModInstrumentationPixelsDrawn = 1,
	kModInstrumentationFramesDrawn,

	/* network */
	kModInstrumentationNetworkBytesRead,
	kModInstrumentationNetworkBytesWritten,
	kModInstrumentationNetworkSockets,

	/* timers */
	kModInstrumentationTimers,

	/* files */
	kModInstrumentationFiles,

	/* Poco */
	kModInstrumentationPocoDisplayListUsed,

	/* Piu */
	kModInstrumentationPiuCommandListUsed,

#if ESP32
	/* SPI flash */
	kModInstrumentationSPIFlashErases,
#endif
#if PICO_BUILD || defined(__ets__) || ESP32
	/* turns */
	kModInstrumentationTurns,
#endif

	// Callbacks

	/* system */
	kModInstrumentationSystemFreeMemory,

#if ESP32
	/* CPU utilization */
	kModInstrumentationCPU0,
	#if kTargetCPUCount > 1
		kModInstrumentationCPU1,
	#endif
#endif

	/* XS */
	kModInstrumentationSlotHeapSize,
	kModInstrumentationChunkHeapSize,
	kModInstrumentationKeysUsed,
	kModInstrumentationGarbageCollectionCount,
	kModInstrumentationModulesLoaded,
	kModInstrumentationStackRemain,

	kModInstrumentationCallbacksBegin = kModInstrumentationSystemFreeMemory,
	kModInstrumentationCallbacksEnd = kModInstrumentationStackRemain,

	kModInstrumentationLast = kModInstrumentationCallbacksEnd
};

typedef int32_t (*ModInstrumentationGetter)(void *the);

#ifdef __cplusplus 
extern "C" {
#endif

void modInstrumentationInit(void);

void modInstrumentationSetCallback_(uint8_t what, ModInstrumentationGetter getter);

void modInstrumentationSet_(uint8_t what, int32_t value);
void modInstrumentationMin_(uint8_t what, int32_t value);
void modInstrumentationMax_(uint8_t what, int32_t value);
void modInstrumentationAdjust_(uint8_t what, int32_t value);
int32_t modInstrumentationGet_(void *the, uint8_t what);

#ifdef __cplusplus 
};
#endif

#define modInstrumentationSetCallback(what, getter) modInstrumentationSetCallback_(kModInstrumentation##what, getter)
#define modInstrumentationSet(what, value) modInstrumentationSet_(kModInstrumentation##what, value)
#define modInstrumentationMin(what, value) modInstrumentationMin_(kModInstrumentation##what, value)
#define modInstrumentationMax(what, value) modInstrumentationMax_(kModInstrumentation##what, value)
#define modInstrumentationAdjust(what, value) modInstrumentationAdjust_(kModInstrumentation##what, value)
#define modInstrumentationGet(the, what) modInstrumentationGet_(the, kModInstrumentation##what)

#else

#define modInstrumentationInit(void)

#define modInstrumentationSetCallback(what, getter)
#define modInstrumentationSet(what, value)
#define modInstrumentationAdjust(what, value)
#define modInstrumentationGet(the, what)

#endif

#endif

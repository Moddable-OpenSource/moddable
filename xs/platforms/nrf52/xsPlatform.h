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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

#ifndef __XSPLATFORM__
#define __XSPLATFORM__

#include <stdint.h>
#if __has_include(<stdbool.h>)
	#include <stdbool.h>
#endif

#include "sdk_config.h"
#include "nrf.h"
#include "nrf_error.h"
#include "nrf_gpio.h"
#include "nrf_drv_wdt.h"


#define mxRegExp 1
//#define mxReport 1
#define mxNoFunctionLength 1
#define mxNoFunctionName 1
#define mxHostFunctionPrimitive 1
//#define mxDebug 1
#define mxFewGlobalsTable 1
#ifdef mxDebug
	#define mxNoConsole 1
#endif
#define mxMisalignedSettersCrash 1		//@@@ ?
#define mxUseFreeRTOSTasks	1
// #define mxUseGCCAtomics		1		// undefine

#ifndef __XS6PLATFORMMINIMAL__

#define mxExport extern
#ifndef mxImport
#define mxImport
#endif

#define mxBigEndian 0
#define mxLittleEndian 1

#define mxiOS 0
#define mxLinux 0
#define mxMacOSX 0
#define mxWindows 0
		
#define XS_FUNCTION_NORETURN __attribute__((noreturn))
#define XS_FUNCTION_ANALYZER_NORETURN

#ifndef true
	#define true 1
	#define false 0
#endif

typedef int8_t txS1;
typedef uint8_t txU1;
typedef int16_t txS2;
typedef uint16_t txU2;
typedef int32_t txS4;
typedef uint32_t txU4;
typedef int64_t txS8;
typedef uint64_t txU8;

typedef int txSocket;
#define mxNoSocket NULL

#include "xsHost.h"

#ifndef true
    #define true 1
    #define false 0
#endif

#define mxVolatile(type, name, value) type name = value; type *name ## Address __attribute__((unused)) = &name

#define mxGetKeySlotID(SLOT) (SLOT)->ID
#define mxGetKeySlotKind(SLOT) (SLOT)->kind

extern void fx_putc(void *refcon, char c);

#define mxTableMinLength (1)

struct DebugFragmentRecord {
	struct DebugFragmentRecord *next;
	uint16_t count;
	uint8_t binary;
	uint8_t pad;
	uint8_t bytes[1];
};
typedef struct DebugFragmentRecord DebugFragmentRecord;
typedef struct DebugFragmentRecord *DebugFragment;

/* machine */

#define kDebugReaderCount (8)

#ifdef mxDebug
	#define mxMachineDebug \
		txSocket connection; \
		void* readers[kDebugReaderCount]; \
		uint16_t readerOffset;	\
		uint8_t inPrintf; \
    	uint8_t debugNotifyOutstanding; \
    	uint8_t DEBUG_LOOP; \
		uint8_t debugConnectionVerified; \
		uint8_t	wsState; \
		uint8_t	wsFin; \
		uint16_t wsLength; \
		uint16_t wsSendStart; \
		uint8_t	wsMask[4]; \
		uint8_t	*wsCmd; \
		uint8_t	*wsCmdPtr; \
		DebugFragment debugFragments;
#else
	#define mxMachineDebug
#endif

#ifdef mxInstrument
	#define mxMachineInstrument \
		void *instrumentationTimer; \
		void *instrumentationCallback;
#else
	#define mxMachineInstrument
#endif

#define mxMachinePlatform \
	void* host; \
	uint8_t *heap; \
	uint8_t *heap_ptr; \
	uint8_t *heap_pend; \
	void *msgQueue; \
	void *dbgQueue; \
	void *queues; \
	void *task; \
	void *waiterCondition; \
	void *waiterData; \
	void *waiterLink; \
	mxMachineDebug \
	mxMachineInstrument

#endif /* __XS6PLATFORMMINIMAL__ */

int ESP_getc(void);
void ESP_putc(int c);

#include "nrf_delay.h"

extern void nrf52_delay(uint32_t ms);
#define nrf52_delay_us	nrf_delay_us
extern uint32_t nrf52_milliseconds();
extern uint32_t nrf52_memory_remaining();

#define delay(x)            nrf52_delay(x)

#endif /* __XSPLATFORM__ */

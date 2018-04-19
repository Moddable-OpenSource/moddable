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

#include "stdint.h"

#if ESP32
	#define ICACHE_FLASH_ATTR __attribute__((section(".rodata.flash")))
	#define ICACHE_RAM_ATTR __attribute__((section(".iram1.mod")))
#endif

#define mxRegExp 1
//#define mxReport 1
#define mxNoFunctionLength 1
#define mxNoFunctionName 1
#define mxHostFunctionPrimitive 1
#define mxFewGlobalsTable 1
#ifdef mxDebug
	#define mxNoConsole 1
#endif
#define mxMisalignedSettersCrash 1

#ifndef __XS6PLATFORMMINIMAL__

#define mxExport extern
#define mxImport

#define mxBigEndian 0
#define mxLittleEndian 1

#define mxiOS 0
#define mxLinux 0
#define mxMacOSX 0
#define mxWindows 0

#define XS_FUNCTION_NORETURN __attribute__((noreturn))
#define XS_FUNCTION_ANALYZER_NORETURN

typedef int8_t txS1;
typedef uint8_t txU1;
typedef int16_t txS2;
typedef uint16_t txU2;
typedef int32_t txS4;
typedef uint32_t txU4;

#if ESP32
	typedef uint8_t BOOL;
#endif

//@@typedef int txSocket;
typedef void *txSocket;
#define mxNoSocket NULL

#define LWIP_POSIX_SOCKETS_IO_NAMES 0

#include <lwip/netdb.h>
#include <lwip/inet.h>

#include "xsesp.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PATH_MAX
	#undef PATH_MAX
#endif
#define PATH_MAX (256)

#define mxGetKeySlotID(SLOT) (SLOT)->ID
#define mxGetKeySlotKind(SLOT) (SLOT)->kind

extern void fx_putc(void *refcon, char c);

#define mxMapSetLength (1)

struct DebugFragmentRecord {
	struct DebugFragmentRecord *next;
	uint8_t count;
	uint8_t bytes[1];
};
typedef struct DebugFragmentRecord DebugFragmentRecord;
typedef struct DebugFragmentRecord *DebugFragment;

/* machine */
#if ESP32
	#define mxMachinePlatform \
		void* host; \
		txSocket connection; \
		void* reader; \
		txBoolean debugOnReceive; \
		txBoolean pendingSendBytes; \
		txBoolean inPrintf; \
		txBoolean debugNotifyOutstanding; \
		DebugFragment debugFragments; \
		uint8_t *heap; \
		uint8_t *heap_ptr; \
		uint8_t *heap_pend; \
		void *msgQueue; \
		void *task;
#else
	#define mxMachinePlatform \
		void* host; \
		txSocket connection; \
		void* reader; \
		txBoolean debugOnReceive; \
		txBoolean pendingSendBytes; \
		txBoolean inPrintf; \
		txBoolean debugNotifyOutstanding; \
		DebugFragment debugFragments; \
		uint8_t *heap; \
		uint8_t *heap_ptr; \
		uint8_t *heap_pend;
#endif

#ifdef __cplusplus
}
#endif

#endif /* __XS6PLATFORMMINIMAL__ */

#if ESP32
    void delay(uint32_t ms);
#endif

#endif /* __XSPLATFORM__ */

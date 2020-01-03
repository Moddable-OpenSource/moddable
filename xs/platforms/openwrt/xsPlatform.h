/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
#include "stdbool.h"

#define mxRegExp 1
//#define mxReport 1
#define mxNoFunctionLength 1
#define mxNoFunctionName 1
#define mxHostFunctionPrimitive 1
#define mxFewGlobalsTable 1
//#define mxNoConsole 1
#define mxMisalignedSettersCrash 1

#ifndef __XS6PLATFORMMINIMAL__

#undef mxExport
#undef mxImport
#define mxExport extern
#define mxImport

#if defined(__GNUC__) 
	#if defined(__BYTE_ORDER__)
		#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
			#define mxLittleEndian 0
			#define mxBigEndian 1
		#else
			#define mxLittleEndian 1
			#define mxBigEndian 0
		#endif
	#elif defined(__i386__) || defined(i386) || defined(intel) || defined(arm) || defined(__arm__) || defined(__k8__) || defined(__x86_64__)
		#define mxLittleEndian 1
		#define mxBigEndian 0
	#elif defined(mips) || defined(_mips)
		#define mxLittleEndian 1
		#define mxBigEndian 0
	#else
		#error unknown compiler 
	#endif
#else
	#error unknown compiler
#endif

#define mxiOS 0
#define mxMacOSX 0
#define mxWindows 0

#define XS_FUNCTION_NORETURN __attribute__((noreturn))
#define XS_FUNCTION_ANALYZER_NORETURN

typedef signed char txS1;
typedef unsigned char txU1;
typedef short txS2;
typedef unsigned short txU2;
#if __LP64__
typedef int txS4;
typedef unsigned int txU4;
#else
typedef long txS4;
typedef unsigned long txU4;
#endif
typedef int64_t txS8;
typedef uint64_t txU8;

typedef int txSocket;
#define mxNoSocket -1

#include "xsHost.h"

#ifdef __cplusplus
extern "C" {
#endif

#define mxGetKeySlotID(SLOT) (SLOT)->ID
#define mxGetKeySlotKind(SLOT) (SLOT)->kind

struct DebugFragmentRecord {
	struct DebugFragmentRecord *next;
	uint8_t count;
	uint8_t bytes[1];
};
typedef struct DebugFragmentRecord DebugFragmentRecord;
typedef struct DebugFragmentRecord *DebugFragment;

/* MACHINE */

#define mxMachinePlatform \
	void* host; \
	txSocket connection; \
	uint8_t *heap; \
	uint8_t *heap_ptr; \
	uint8_t *heap_pend;
#ifdef __cplusplus
}
#endif

#endif /* __XS6PLATFORMMINIMAL__ */

#endif /* __XSPLATFORM__ */



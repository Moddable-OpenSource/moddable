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

#define mxBigEndian 0
#define mxLittleEndian 1

#define mxiOS 0
#define mxLinux 0
#define mxMacOSX 0
#define mxWasm 0
#define mxWindows 0

#include "xsHost.h"

#ifndef XSPLATFORM	
	/* for xsc and xsid on Linux, macOS or Windows */
	#if defined(_MSC_VER)
		#if defined(_M_IX86) || defined(_M_X64)
			#undef mxWindows
			#define mxWindows 1
			#ifdef mxDynamicLink
				#define mxExport extern __declspec( dllexport )
				#define mxImport extern __declspec( dllimport )
			#endif
		#else 
			#error unknown Microsoft compiler
		#endif
	#elif defined(__GNUC__) 
		#if defined(__i386__) || defined(i386) || defined(intel) || defined(arm) || defined(__arm__) || defined(__k8__) || defined(__x86_64__) || defined(__aarch64__)
			#if defined(__linux__) || defined(linux)
				#undef mxLinux
				#define mxLinux 1
			#else
				#undef mxMacOSX
				#define mxMacOSX 1
			#endif
			#ifdef mxDynamicLink
				#define mxExport extern __attribute__ ((visibility("default")))
				#define mxImport extern __attribute__ ((visibility("default")))
			#endif
			#define XS_FUNCTION_NORETURN __attribute__((noreturn))
		#endif
	#else 
		#error unknown compiler
	#endif
	#if mxWindows
		#define _USE_MATH_DEFINES
		#define WIN32_LEAN_AND_MEAN
		#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#endif
	#include <ctype.h>
	#include <float.h>
	#include <math.h>
	#include <setjmp.h>
	#include <stdarg.h>
	#include <stdint.h>
	#include <stdbool.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>
	#if mxWindows
		#include <winsock2.h>
	#else 
		#include <arpa/inet.h>
		#include <pthread.h>
		#include <unistd.h>
		#define mxUseGCCAtomics 1
		#define mxUsePOSIXThreads 1
	#endif
	#define mxMachinePlatform \
		void* waiterCondition; \
		void* waiterData; \
		txMachine* waiterLink;
	#define mxUseDefaultMachinePlatform 1
	#define mxUseDefaultBuildKeys 1
	#define mxUseDefaultChunkAllocation 1
	#define mxUseDefaultSlotAllocation 1
	#define mxUseDefaultFindModule 1
	#define mxUseDefaultLoadModule 1
	#define mxUseDefaultParseScript 1
	#define mxUseDefaultQueuePromiseJobs 1
	#define mxUseDefaultSharedChunks 1
	#define mxUseDefaultAbort 1
	#define mxUseDefaultDebug 1
#else
	#include XSPLATFORM
#endif

#ifndef ICACHE_FLASH_ATTR
	#define ICACHE_FLASH_ATTR
#endif
#ifndef ICACHE_RODATA_ATTR
	#define ICACHE_RODATA_ATTR
#endif
#ifndef ICACHE_RAM_ATTR
	#define ICACHE_RAM_ATTR
#endif
#ifndef ICACHE_XS6RO_ATTR
	#define ICACHE_XS6RO_ATTR
#endif
#ifndef ICACHE_XS6RO2_ATTR
	#define ICACHE_XS6RO2_ATTR
#endif
#ifndef ICACHE_XS6STRING_ATTR
	#define ICACHE_XS6STRING_ATTR
#endif

#ifndef XS_FUNCTION_NORETURN
	#ifdef __GNUC__
		#define XS_FUNCTION_NORETURN __attribute__((noreturn))
	#else
		#define XS_FUNCTION_NORETURN
	#endif
#endif

#ifndef mxExport
	#define mxExport extern
#endif
#ifndef mxImport
	#define mxImport extern
#endif

#ifndef txS1
	typedef int8_t txS1;
#endif
#ifndef txU1
	typedef uint8_t txU1;
#endif
#ifndef txS2
	typedef int16_t txS2;
#endif
#ifndef txU2
	typedef uint16_t txU2;
#endif
#ifndef txS4
	typedef int32_t txS4;
#endif
#ifndef txU4
	typedef uint32_t txU4;
#endif
#ifndef txS8
	typedef int64_t txS8;
#endif
#ifndef txU8
	typedef uint64_t txU8;
#endif

#ifndef c_jmp_buf
	typedef jmp_buf c_jmp_buf;
#endif
#ifndef c_longjmp
	#define c_longjmp longjmp
#endif
#ifndef c_setjmp
	#define c_setjmp setjmp
#endif

#ifndef c_va_list
	typedef va_list c_va_list;
#endif
#ifndef c_va_arg
	#define c_va_arg va_arg
#endif
#ifndef c_va_end
	#define c_va_end va_end
#endif
#ifndef c_va_start
	#define c_va_start va_start
#endif

#ifndef c_calloc
	#define c_calloc calloc
#endif
#ifndef c_exit
	#define c_exit exit
#endif
#ifndef c_free
	#define c_free free
#endif
#ifndef c_malloc
	#define c_malloc malloc
#endif
#ifndef c_free_uint32
	#define c_free_uint32 free
#endif
#ifndef c_malloc_uint32
	#define c_malloc_uint32 malloc
#endif
#ifndef c_qsort
	#define c_qsort qsort
#endif
#ifndef c_realloc
	#define c_realloc realloc
#endif
#ifndef c_strtod
	#define c_strtod strtod
#endif
#ifndef c_strtol
	#define c_strtol strtol
#endif
#ifndef c_strtoul
	#define c_strtoul strtoul
#endif

#ifndef C_EOF
	#define C_EOF EOF
#endif
#ifndef C_NULL
	#define C_NULL NULL
#endif
#ifndef c_vprintf
	#define c_vprintf vprintf
#endif
#ifndef c_printf
	#define c_printf printf
#endif
#ifndef c_vsnprintf
	#define c_vsnprintf vsnprintf
#endif
#ifndef c_snprintf
	#define c_snprintf snprintf
#endif
#ifndef c_vfprintf
	#define c_vfprintf vfprintf
#endif
#ifndef c_fprintf
	#define c_fprintf fprintf
#endif

#ifndef c_signal
	#ifdef EMSCRIPTEN
		#define c_signal(signum, handler)
	#else
		#define c_signal signal
	#endif
#endif

/* DATE */

#ifndef c_time_t
	typedef time_t c_time_t;
#endif 
#ifndef c_tm
	typedef struct tm c_tm;
#endif 
#ifndef c_localtime
	#define c_localtime localtime
#endif
#ifndef c_mktime
	#define c_mktime mktime
#endif
#ifndef c_gettimeofday
	#if mxWindows
		#include <sys/types.h>
		#include <sys/timeb.h>
		typedef struct timeval c_timeval;
		struct c_timezone {
			int tz_minuteswest;
			int tz_dsttime;
		};
		#ifdef __cplusplus
		extern "C" {
		#endif
			extern int c_gettimeofday(c_timeval *tp, struct c_timezone *tzp);
		#ifdef __cplusplus
		}
		#endif
	#else
		#include <sys/time.h>
		typedef struct timeval c_timeval;
		#define c_timezone timezone 
		#define c_gettimeofday gettimeofday
	#endif
#endif

/* MATH */

#ifndef C_DBL_MAX
	#define C_DBL_MAX DBL_MAX
#endif
#ifndef C_DBL_MIN
	#define C_DBL_MIN (double)5e-324
#endif
#ifndef C_EPSILON
	#define C_EPSILON (double)2.2204460492503130808472633361816e-16
#endif
#ifndef C_FP_INFINITE
	#define C_FP_INFINITE FP_INFINITE
#endif
#ifndef C_FP_NAN
	#define C_FP_NAN FP_NAN
#endif
#ifndef C_FP_NORMAL
	#define C_FP_NORMAL FP_NORMAL
#endif
#ifndef C_FP_SUBNORMAL
	#define C_FP_SUBNORMAL FP_SUBNORMAL
#endif
#ifndef C_FP_ZERO
	#define C_FP_ZERO FP_ZERO
#endif
#ifndef C_INFINITY
	#define C_INFINITY (double)INFINITY
#endif
#ifndef C_M_E
	#define C_M_E M_E
#endif
#ifndef C_M_LN10
	#define C_M_LN10 M_LN10
#endif
#ifndef C_M_LN2
	#define C_M_LN2 M_LN2
#endif
#ifndef C_M_LOG10E
	#define C_M_LOG10E M_LOG10E
#endif
#ifndef C_M_LOG2E
	#define C_M_LOG2E M_LOG2E
#endif
#ifndef C_M_PI
	#define C_M_PI M_PI
#endif
#ifndef C_M_SQRT1_2
	#define C_M_SQRT1_2 M_SQRT1_2
#endif
#ifndef C_M_SQRT2
	#define C_M_SQRT2 M_SQRT2
#endif
#ifndef C_MAX_SAFE_INTEGER
	#define C_MAX_SAFE_INTEGER (double)9007199254740991
#endif
#ifndef C_MIN_SAFE_INTEGER
	#define C_MIN_SAFE_INTEGER (double)-9007199254740991
#endif
#ifndef C_NAN
	#define C_NAN NAN
#endif
#ifndef C_RAND_MAX
	#define C_RAND_MAX RAND_MAX
#endif
#ifndef c_acos
	#define c_acos acos
#endif
#ifndef c_acosh
	#define c_acosh acosh
#endif
#ifndef c_asin
	#define c_asin asin
#endif
#ifndef c_asinh
	#define c_asinh asinh
#endif
#ifndef c_atan
	#define c_atan atan
#endif
#ifndef c_atanh
	#define c_atanh atanh
#endif
	#define c_atan2 atan2
#ifndef c_cbrt
	#define c_cbrt cbrt
#endif
#ifndef c_ceil
	#define c_ceil ceil
#endif
#ifndef c_cos
	#define c_cos cos
#endif
#ifndef c_cosh
	#define c_cosh cosh
#endif
#ifndef c_exp
	#define c_exp exp
#endif
	#define c_expm1 expm1
#ifndef c_fabs
	#define c_fabs fabs
#endif
#ifndef c_floor
	#define c_floor floor
#endif
#ifndef c_fmod
	#define c_fmod fmod
#endif
#ifndef c_fpclassify
	#define c_fpclassify fpclassify
#endif
#ifndef c_hypot
	#define c_hypot hypot
#endif
#ifndef c_isfinite
	#define c_isfinite isfinite
#endif
#ifndef c_isnormal
	#define c_isnormal isnormal
#endif
#ifndef c_isnan
	#define c_isnan isnan
#endif
#ifndef c_llround
	#define c_llround llround
#endif
#ifndef c_log
	#define c_log log
#endif
#ifndef c_log1p
	#define c_log1p log1p
#endif
#ifndef c_log10
	#define c_log10 log10
#endif
#ifndef c_log2
	#define c_log2 log2
#endif
#ifndef c_nearbyint
	#define c_nearbyint nearbyint
#endif
#ifndef c_pow
	#define c_pow pow
#endif
#ifndef c_rand
	#define c_rand rand
#endif
#ifndef c_round
	#define c_round round
#endif
#ifndef c_signbit
	#define c_signbit signbit
#endif
#ifndef c_sin
	#define c_sin sin
#endif
#ifndef c_sinh
	#define c_sinh sinh
#endif
#ifndef c_sqrt
	#define c_sqrt sqrt
#endif
#ifndef c_srand
	#define c_srand srand
#endif
#ifndef c_tan
	#define c_tan tan
#endif
#ifndef c_tanh
	#define c_tanh tanh
#endif
#ifndef c_trunc
	#define c_trunc trunc
#endif

/* STRING */

#ifndef c_memcpy
	#define c_memcpy memcpy
#endif
#ifndef c_memmove
	#define c_memmove memmove
#endif
#ifndef c_memset
	#define c_memset memset
#endif
#ifndef c_memcmp
	#define c_memcmp memcmp
#endif
#ifndef c_strcat
	#define c_strcat strcat
#endif
#ifndef c_strchr
	#define c_strchr strchr
#endif
#ifndef c_strcmp
	#define c_strcmp strcmp
#endif
#ifndef c_strcpy
	#define c_strcpy strcpy
#endif
#ifndef c_strlen
	#define c_strlen strlen
#endif
#ifndef c_strncat
	#define c_strncat strncat
#endif
#ifndef c_strncmp
	#define c_strncmp strncmp
#endif
#ifndef c_strncpy
	#define c_strncpy strncpy
#endif
#ifndef c_strstr
	#define c_strstr strstr
#endif
#ifndef c_strrchr
	#define c_strrchr strrchr
#endif
#ifndef c_strcspn
	#define c_strcspn strcspn
#endif
#ifndef c_strspn
	#define c_strspn strspn
#endif


/* READ MEMORY */

#ifndef c_read8
	#define c_read8(POINTER) *((txU1*)(POINTER))
#endif
#ifndef c_read16
	#define c_read16(POINTER) *((txU2*)(POINTER))
#endif
#ifndef c_read32
	#define c_read32(POINTER) *((txU4*)(POINTER))
#endif
#ifndef c_read16be
	#if mxBigEndian
		#define c_read16be(POINTER) *((txU2*)(POINTER))
	#else
		#define c_read16be(POINTER) ((((txU2)((txU1*)(POINTER))[0]) << 8) | ((txU2)((txU1*)(POINTER))[1]))
	#endif
#endif
#ifndef c_read32be
	#if mxBigEndian
		#define c_read32be(POINTER) *((txU4*)(POINTER))
	#else
		#define c_read32be(POINTER) ((((txU4)((txU1*)(POINTER))[0]) << 24) | (((txU4)((txU1*)(POINTER))[1]) << 16) | (((txU4)((txU1*)(POINTER))[2]) << 8) | ((txU4)((txU1*)(POINTER))[3]))
	#endif
#endif

#ifndef mxGetKeySlotID
	#define mxGetKeySlotID(SLOT) (SLOT)->ID
#endif
#ifndef mxGetKeySlotKind
	#define mxGetKeySlotKind(SLOT) (SLOT)->kind
#endif

#ifndef mxSeparator
	#if mxWindows
		#define mxSeparator '\\'
	#else
		#define mxSeparator '/'
	#endif
#endif

#ifndef C_PATH_MAX
	#if mxWindows
		#define C_PATH_MAX _MAX_PATH
	#else
		#include <limits.h>
		#define C_PATH_MAX PATH_MAX
	#endif
#endif

#if mxWindows
	#define C_ENOMEM ERROR_NOT_ENOUGH_MEMORY
	#define C_EINVAL ERROR_INVALID_DATA
#else
	#include <errno.h>
	#include <sys/stat.h>
	#define C_ENOMEM ENOMEM
	#define C_EINVAL EINVAL
#endif

#ifdef mxParse
	#if mxWindows
		#ifdef __cplusplus
		extern "C" {
		#endif
			extern char* c_realpath(const char* path, char* real);
		#ifdef __cplusplus
		}
		#endif
		#define mxParserThrowElse(_ASSERTION) { if (!(_ASSERTION)) { parser->error = GetLastError(); c_longjmp(parser->firstJump->jmp_buf, 1); } }
	#else
		#define c_realpath realpath
		#define mxParserThrowElse(_ASSERTION) { if (!(_ASSERTION)) { parser->error = errno; c_longjmp(parser->firstJump->jmp_buf, 1); } }
	#endif
#endif

#endif /* __XSPLATFORM__ */

/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 *
 *     Portions Copyright (C) 2010-2015 Marvell International Ltd.
 *     Portions Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Portions Copyright by Marvell International Ltd. and Kinoma, Inc. are 
 *     derived from KinomaJS/XS6 and used under the Apache 2.0 License.
 */

#ifndef __XSPLATFORM__
#define __XSPLATFORM__

#define mxBigEndian 0
#define mxLittleEndian 1

#define mxExport extern
#ifndef mxImport
	#define mxImport
#endif

#define XS_FUNCTION_NORETURN __attribute__((noreturn))
#define XS_FUNCTION_ANALYZER_NORETURN

#ifndef true
	#define true 1
	#define false 0
#endif

//#define mxReport 1
#define mxNoFunctionLength 1
#define mxNoFunctionName 1
#define mxHostFunctionPrimitive 1
//#define mxDebug 1
#define mxFewGlobalsTable 1


#include <stdint.h>
typedef int8_t txS1;
typedef uint8_t txU1;
typedef int16_t txS2;
typedef uint16_t txU2;
typedef int32_t txS4;
typedef uint32_t txU4;


typedef int txSocket;
#define mxNoSocket NULL

#include "xsgecko.h"

#define mxVolatile(type, name, value) type name = value; type *name ## Address __attribute__((unused)) = &name


/* C */

#include <ctype.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define C_EOF EOF
#define C_NULL NULL

#define c_tolower tolower
#define c_toupper toupper

typedef jmp_buf c_jmp_buf;

#define c_longjmp longjmp
#define c_setjmp setjmp

typedef va_list c_va_list;
#define c_va_arg va_arg
#define c_va_end va_end
#define c_va_start va_start

#define MY_MALLOC 0
#if MY_MALLOC
extern void *my_calloc(size_t nitems, size_t size);	
extern void *my_realloc(void *ptr, size_t size);	
extern void *my_malloc(size_t size);	
#define c_calloc my_calloc
#define c_malloc my_malloc
#define c_realloc my_realloc
#else
#define c_calloc calloc
#define c_malloc malloc
#define c_realloc realloc
#endif

#define c_exit exit
#define c_free free
#define c_qsort qsort
#define c_strtod strtod
#define c_strtol strtol
#define c_strtoul strtoul
#define c_vprintf vprintf
#define c_printf printf
#define c_vsnprintf vsnprintf
#define c_snprintf snprintf
#define c_fprintf fprintf


/* DATE */
	
#include <sys/time.h>
#include <time.h>

typedef time_t c_time_t;
typedef struct timeval c_timeval;
typedef struct tm c_tm;

#define c_timezone timezone
#define c_gettimeofday gettimeofday
#define c_gmtime gmtime
#define c_localtime localtime
#define c_mktime mktime
#define c_strftime strftime
#define c_time time
	
/* ERROR */
	
#if mxWindows
	#define C_ENOMEM ERROR_NOT_ENOUGH_MEMORY
	#define C_EINVAL ERROR_INVALID_DATA
#else
	#include <errno.h>
	#define C_ENOMEM ENOMEM
	#define C_EINVAL EINVAL
#endif
	
/* MATH */

#if mxWindows
	#include <math.h>
	#include <float.h>
	#define M_E        2.71828182845904523536
	#define M_LN2      0.693147180559945309417
	#define M_LN10     2.30258509299404568402
	#define M_LOG2E    1.44269504088896340736
	#define M_LOG10E   0.434294481903251827651
	#define M_PI       3.14159265358979323846
	#define M_SQRT1_2  0.707106781186547524401
	#define M_SQRT2    1.41421356237309504880
	#if _MSC_VER < 1800
		enum {
			FP_NAN          = 1,
			FP_INFINITE     = 2,
			FP_ZERO         = 3,
			FP_NORMAL       = 4,
			FP_SUBNORMAL    = 5
		};
		#define INFINITY *(double*)infinity
		extern unsigned long infinity[];
		#define NAN *(double*)nan
		extern unsigned long nan[];
		extern int fpclassify(double x);
		#define isfinite _isfinite
		#define isnan _isnan
		#define isnormal _isnormal
	#endif
#elif mxLinux
	#if !defined(__USE_ISOC99)
		#define __USE_ISOC99 1
		#include <math.h>
		#include <float.h>
		#undef __USE_ISOC99
	#else
		#include <math.h>
		#include <float.h>
	#endif
#else
	#include <math.h>
	#include <float.h>
#endif

#define C_DBL_MAX DBL_MAX
#define C_DBL_MIN (double)5e-324
#define C_EPSILON (double)2.2204460492503130808472633361816e-16
#define C_FP_INFINITE FP_INFINITE
#define C_FP_NAN FP_NAN
#define C_FP_NORMAL FP_NORMAL
#define C_FP_SUBNORMAL FP_SUBNORMAL
#define C_FP_ZERO FP_ZERO
#define C_INFINITY (double)INFINITY
#define C_M_E M_E
#define C_M_LN10 M_LN10
#define C_M_LN2 M_LN2
#define C_M_LOG10E M_LOG10E
#define C_M_LOG2E M_LOG2E
#define C_M_PI M_PI
#define C_M_SQRT1_2 M_SQRT1_2
#define C_M_SQRT2 M_SQRT2
#define C_MAX_SAFE_INTEGER (double)9007199254740991
#define C_MIN_SAFE_INTEGER (double)-9007199254740991
#define C_NAN NAN
#define C_RAND_MAX RAND_MAX

#define c_acos acos
#define c_acosh acosh
#define c_asin asin
#define c_asinh asinh
#define c_atan atan
#define c_atanh atanh
#define c_atan2 atan2
#define c_cbrt cbrt
#define c_ceil ceil
#define c_cos cos
#define c_cosh cosh
#define c_exp exp
#define c_expm1 expm1
#define c_fabs fabs
#define c_floor floor
#define c_fmod fmod
#define c_fpclassify fpclassify
#define c_hypot hypot
#define c_isfinite isfinite
#define c_isnormal isnormal
#define c_isnan isnan
#define c_llround llround
#define c_log log
#define c_log1p log1p
#define c_log10 log10
#define c_log2 log2
#define c_nearbyint nearbyint
#define c_pow pow
#define c_rand rand
#define c_round round
#define c_signbit signbit
#define c_sin sin
#define c_sinh sinh
#define c_sqrt sqrt
#define c_srand srand
#define c_tan tan
#define c_tanh tanh
#define c_trunc trunc

/* STRING */

#include <string.h>
#define c_memcpy memcpy
#define c_memmove memmove
#define c_memset memset
#define c_memcmp memcmp
#define c_strcat strcat
#define c_strchr strchr
#define c_strcmp strcmp
#define c_strcpy strcpy
#define c_strlen strlen
#define c_strncat strncat
#define c_strncmp strncmp
#define c_strncpy strncpy
#define c_strstr strstr
#define c_strrchr strrchr

#define ICACHE_FLASH_ATTR
#define ICACHE_STORE_ATTR	__attribute__((aligned(4)))
#define ICACHE_RODATA_ATTR	__attribute__((aligned(4))) __attribute__((section(".rodata")))
#define ICACHE_RAM_ATTR	
#define ICACHE_XS6RO_ATTR	__attribute__((aligned(4))) __attribute__((section(".rodata.1")))
#define ICACHE_XS6RO2_ATTR	__attribute__((aligned(4))) __attribute__((section(".rodata.2")))
#define ICACHE_XS6STRING_ATTR	__attribute__((aligned(4))) __attribute__((section(".rodata.3")))

#define espRead8(POINTER) *((txU1*)POINTER)
#define mxGetKeySlotID(SLOT) (SLOT)->ID
#define mxGetKeySlotKind(SLOT) (SLOT)->kind

#define c_read8(POINTER) *((txU1*)(POINTER))
#define c_read16(POINTER) *((txU2*)(POINTER))
#define c_read16be(POINTER) ((((txU2)((txU1*)POINTER)[0]) << 8) | ((txU2)((txU1*)POINTER)[1]))
#define c_read32(POINTER) *((txU4*)(POINTER))
#define c_read32be(POINTER) ((((txU4)((txU1*)POINTER)[0]) << 24) | (((txU4)((txU1*)POINTER)[1]) << 16) | (((txU4)((txU1*)POINTER)[2]) << 8) | ((txU4)((txU1*)POINTER)[3]))


/* MACHINE */

#define mxMachinePlatform \
	void* host; \
	txSocket connection; \
	void* reader; \
	txBoolean debugOnReceive; \
	txBoolean pendingSendBytes; \
	txBoolean inPrintf; \
	uint8_t *heap; \
	uint8_t *heap_ptr; \
	uint8_t *heap_pend;

#endif /* __XSPLATFORM__ */

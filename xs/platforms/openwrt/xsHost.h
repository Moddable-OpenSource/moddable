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
 */

#ifndef __XSHOST__
#define __XSHOST__

#include "stddef.h"
#include "stdint.h"
#include "stdarg.h"
#include "limits.h"
#include "stdio.h"
#include "stdlib.h"
#include "errno.h"
#include "malloc.h"
#include "setjmp.h"
#include "string.h"
#include "ctype.h"

/*
	link locations
*/

#define ICACHE_FLASH_ATTR
#define ICACHE_RODATA_ATTR
#define ICACHE_RAM_ATTR	
#define ICACHE_XS6RO_ATTR
#define ICACHE_XS6RO2_ATTR
#define ICACHE_XS6STRING_ATTR

#ifdef __cplusplus
extern "C" {
#endif

/*
	memory
*/

#define c_read8(POINTER) *((txU1*)(POINTER))
#define c_read16(POINTER) *((txU2*)(POINTER))
#define c_read16be(POINTER) ((((txU2)((txU1*)POINTER)[0]) << 8) | ((txU2)((txU1*)POINTER)[1]))
#define c_read32(POINTER) *((txU4*)(POINTER))
#define c_read32be(POINTER) ((((txU4)((txU1*)POINTER)[0]) << 24) | (((txU4)((txU1*)POINTER)[1]) << 16) | (((txU4)((txU1*)POINTER)[2]) << 8) | ((txU4)((txU1*)POINTER)[3]))

/*
	report
*/

extern void modLog_transmit(const char *msg);

#define modLog(msg) \
	do { \
		static const char scratch[] = msg ; \
		modLog_transmit(scratch); \
	} while (0)
#define modLogVar(msg) modLog_transmit(msg)
#define modLogInt(msg) \
	do { \
		char temp[10]; \
		sprintf(temp, "%d", msg); \
		modLog_transmit(temp); \
	} while (0)
#define modLogHex(msg) \
	do { \
		char temp[10]; \
		itoa((int)(msg), temp, 16); \
		modLog_transmit(temp); \
	} while (0)

/*
	serial
*/

/*
	timer
*/

uint32_t modMilliseconds();
void modDelayMilliseconds(uint32_t ms);

extern void modTimersExecute(void);
extern int modTimersNext(void);

/*
	sleep
*/
void openwrt_schedule(void);

/*
	critical section (one deep)
*/

#define modCriticalSectionDeclare
#define modCriticalSectionBegin()
#define modCriticalSectionEnd()

/*
	date and time
*/

typedef uint32_t modTime_t;

struct modTimeVal {
    modTime_t	tv_sec;     /* seconds */
    uint32_t	tv_usec;    /* microseconds */
};
typedef struct modTimeVal modTimeVal;

struct modTimeZone {
    int32_t			tz_minuteswest;     /* minutes west of Greenwich */
    int32_t			tz_dsttime;         /* type of DST correction */
};

struct modTm {
  int32_t	tm_sec;
  int32_t	tm_min;
  int32_t	tm_hour;
  int32_t	tm_mday;
  int32_t	tm_mon;
  int32_t	tm_year;
  int32_t	tm_wday;
  int32_t	tm_yday;
  int32_t	tm_isdst;
};
typedef struct modTm modTm;

void modGetTimeOfDay(struct modTimeVal *tv, struct modTimeZone *tz);
struct modTm *modGmTime(const modTime_t *timep);
struct modTm *modLocalTime(const modTime_t *timep);
modTime_t modMkTime(struct modTm *tm);
void modStrfTime(char *s, size_t max, const char *format, const struct modTm *tm);

void modSetTime(uint32_t seconds);							// since 1970 - UNIX time
int32_t modGetTimeZone(void);								// seconds
void modSetTimeZone(int32_t timeZoneOffset);				// seconds
int32_t modGetDaylightSavingsOffset(void);					// seconds
void modSetDaylightSavingsOffset(int32_t daylightSavings);	// seconds

/*
	math
*/

/*
	watchdog timer
*/

/*
	VM
*/

#ifdef __XS__
	extern xsMachine *ESP_cloneMachine(uint32_t allocation, uint32_t stack, uint32_t slotCount, const char *name);

	uint8_t modRunPromiseJobs(xsMachine *the);		// returns true if promises still pending
#else
	extern void *ESP_cloneMachine(uint32_t allocation, uint32_t stack, uint32_t slotCount, const char *name);
#endif

/*
	debugging
*/

void fxReceiveLoop(void);

/*
	messages
*/

typedef void (*modMessageDeliver)(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

#ifdef __XS__
	int modMessagePostToMachine(xsMachine *the, uint8_t *message, uint16_t messageLength, modMessageDeliver callback, void *refcon);
	int modMessageService(void);
#endif

/*
	 task
*/

#ifdef __XS__
	void modMachineTaskInit(xsMachine *the);
	void modMachineTaskUninit(xsMachine *the);
	void modMachineTaskWait(xsMachine *the);
	void modMachineTaskWake(xsMachine *the);
#endif

/*
	c libraries
*/

#define C_EOF EOF
#define C_NULL NULL

typedef jmp_buf c_jmp_buf;
#define c_longjmp longjmp
#define c_setjmp setjmp

typedef va_list c_va_list;
#define c_va_arg va_arg
#define c_va_end va_end
#define c_va_start va_start

#define c_vprintf vprintf
#define c_printf printf
#define c_vsnprintf vsnprintf
#define c_sprintf sprintf
#define c_snprintf snprintf
#define c_fprintf(FILE, ...) fprintf(FILE, __VA_ARGS__)

#define c_calloc calloc
#define c_exit(n) exit(n)
#define c_free free
#define c_malloc malloc
#define c_qsort qsort
#define c_realloc realloc
#define c_strtod strtod
#define c_strtol strtol
#define c_strtoul strtoul
	
/* DATE */
#include <time.h>
typedef time_t c_time_t;
typedef struct tm c_tm;
typedef struct timeval c_timeval;
#define c_localtime localtime
#define c_mktime mktime
#define c_timezone timezone 
#include <sys/time.h>
#define c_gettimeofday gettimeofday

/* ERROR */
	
#define C_ENOMEM ENOMEM
#define C_EINVAL EINVAL
	
/* MATH */

#include <math.h>
#include <float.h>
#define C_DBL_MIN (double)5e-324
#define C_DBL_MAX DBL_MAX
#define C_EPSILON (double)2.2204460492503130808472633361816e-16
#define C_FP_INFINITE FP_INFINITE
#define C_FP_NAN FP_NAN
#define C_FP_NORMAL FP_NORMAL
#define C_FP_SUBNORMAL FP_SUBNORMAL
#define C_FP_ZERO FP_ZERO
#define C_INFINITY INFINITY
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
#define C_RAND_MAX (0xFFFFFFFF)

#define c_acos acos
#define c_acosh acosh
#define c_asin asin
#define c_asinh asinh
#define c_atan atan
#define c_atanh atanh
#define c_atan2 atan2
#define c_cbrt cbrt
#define c_ceil ceil
#define c_clz __builtin_clz
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
#define c_strtok strtok
#define c_isEmpty(s) (!c_read8(s))

/* 32-BIT MEMORY */

/* STACK */

/* FLASH */

#ifdef __cplusplus
}
#endif

#endif


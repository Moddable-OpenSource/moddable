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

#ifndef __XS6ESP__
#define __XS6ESP__

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

#if defined(__XS__) && !ESP32
	#include "Arduino.h"
#endif

/*
	link locations
*/

#define ICACHE_STORE_ATTR __attribute__((aligned(4)))

#if ESP32
#if 1
	#define ICACHE_RODATA_ATTR  __attribute__((section(".rodata")))
	#define ICACHE_XS6RO_ATTR __attribute__((section(".sdata.1"))) __attribute__((aligned(4)))
	#define ICACHE_XS6RO2_ATTR __attribute__((section(".sdata2"))) __attribute__((aligned(4)))
	#define ICACHE_XS6STRING_ATTR __attribute((section(".data1"))) __attribute__((aligned(4)))
#else
	#define ICACHE_RODATA_ATTR  __attribute__((section(".rodata")))
	#define ICACHE_XS6RO_ATTR __attribute__((section(".rodata.xs6ro"))) __attribute__((aligned(4)))
	#define ICACHE_XS6RO2_ATTR __attribute__((section(".rodata.xs6ro2"))) __attribute__((aligned(4)))
	#define ICACHE_XS6STRING_ATTR __attribute((section(".rodata.str1.4"))) __attribute__((aligned(4)))
#endif
#else
	#define ICACHE_RODATA_ATTR  __attribute__((section(".irom.text")))
	#define ICACHE_XS6RO_ATTR __attribute__((section(".irom1.text"))) __attribute__((aligned(4)))
	#define ICACHE_XS6RO2_ATTR __attribute__((section(".irom2.text"))) __attribute__((aligned(4)))
	#define ICACHE_XS6STRING_ATTR __attribute__((section(".irom0.str.1"))) __attribute__((aligned(4)))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
	memory and strings
*/

extern uint8_t espRead8(const void *addr);
extern uint16_t espRead16(const void *addr);
extern uint32_t espRead32(const void *addr);
extern uint16_t espRead16be(const void *addr);
extern uint32_t espRead32be(const void *addr);
extern size_t espStrLen(const void *addr);
extern void espStrCpy(char *dst, const char *src);
extern void espStrNCpy(char *dst, const char *src, size_t count);
extern void espStrCat(char *dst, const char *src);
extern void espStrNCat(char *dst, const char *src, size_t count);
extern char *espStrChr(const char *str, int c);
extern char *espStrRChr(const char *str, int c);
extern char *espStrStr(const char *src, const char *search);
extern void espMemCpy(void *dst, const void *src, size_t count);
extern int espMemCmp(const void *a, const void *b, size_t count);
extern int espStrCmp(const char *ap, const char *bp);
extern int espStrNCmp(const char *ap, const char *bp, size_t count);

extern void *espMallocUint32(int count);
extern void espFreeUint32(void *t);

/*
	report
*/

extern void modLog_transmit(const char *msg);
extern void ESP_putc(int c);

#define modLog(msg) \
	do { \
		static const char scratch[] ICACHE_XS6STRING_ATTR = msg ; \
		modLog_transmit(scratch); \
	} while (0)
#define modLogVar(msg) modLog_transmit(msg)
#define modLogInt(msg) \
	do { \
		char temp[10]; \
		itoa(msg, temp, 10); \
		modLog_transmit(temp); \
	} while (0)
#define modLogHex(msg) \
	do { \
		char temp[10]; \
		itoa((int)(msg), temp, 16); \
		modLog_transmit(temp); \
	} while (0)

#define xmodLog(msg)
#define xmodLogVar(msg)
#define xmodLogInt(msg)
#define xmodLogHex(msg)

/*
	serial
*/

extern int ESP_getc(void);
extern void ESP_putc(int c);
extern uint8_t ESP_isReadable(void);

/*
	timer
*/

#if ESP32
	extern uint32_t modMilliseconds(void);
	#define modMicroseconds() (uint32_t)(modMilliseconds() * 1000)		//@@

	#define modDelayMilliseconds(ms) vTaskDelay(ms)
	#define modDelayMicroseconds(us) vTaskDelay(((us) + 500) / 1000)

#else
	#define modMilliseconds() (uint32_t)(millis())
	#define modMicroseconds() (uint32_t)(system_get_time())

	#define modDelayMilliseconds(ms) delay(ms)
	#define modDelayMicroseconds(us) ets_delay_us(us)
#endif

typedef struct modTimerRecord modTimerRecord;
typedef modTimerRecord *modTimer;
typedef void (*modTimerCallback)(modTimer timer, void *refcon, uint32_t refconSize);
extern modTimer modTimerAdd(int firstInterval, int secondInterval, modTimerCallback cb, void *refcon, int refconSize);
extern void modTimerRemove(modTimer timer);
extern modTimer modTimerCallWhenSafe(modTimerCallback cb, void *refcon, int refconSize);
extern void modTimerReschedule(modTimer timer, int firstInterval, int secondInterval);

extern void modTimersExecute(void);
extern int modTimersNext(void);
extern int modTimersNextScript(void);
extern void modTimersAdvanceTime(uint32_t advanceMS);

/*
	critical section
*/

#if !ESP32
	#define modCriticalSectionBegin() noInterrupts()
	#define modCriticalSectionEnd() interrupts()
#else
//	#define modCriticalSectionBegin() taskENTER_CRITICAL()
//	#define modCriticalSectionEnd() taskEXIT_CRITICAL()
//@@
	#define modCriticalSectionBegin()
	#define modCriticalSectionEnd()
#endif

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

double __ieee754_fmod_patch(double x, double y);

/*
	watchdog timer
*/

#define modWatchDogReset() system_soft_wdt_feed()

/*
	VM
*/

#ifdef __XS__
	extern xsMachine *gThe;		// the one XS6 virtual machine running
	extern xsMachine *ESP_cloneMachine(uint32_t allocation, uint32_t stack, uint32_t slotCount, uint8_t disableDebug);

	uint8_t modRunPromiseJobs(xsMachine *the);		// returns true if promises still pending
#else
	extern void *ESP_cloneMachine(uint32_t allocation, uint32_t stack, uint32_t slotCount, uint8_t disableDebug);
#endif

/*
	debugging
*/

#ifdef __XS__
	uint8_t triggerDebugCommand(xsMachine *the);
#endif

/*
	messages
*/

#ifdef __XS__
	int modMessagePostToMachine(xsMachine *the, xsSlot *obj, uint8_t *message, uint16_t messageLength, uint8_t messageKind);
	void modMessageService(void);
#endif

/*
	c libraries
*/

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

#if ESP32
	#define c_vprintf vprintf
	#define c_printf printf
	#define c_vsnprintf vsnprintf
	#define c_snprintf snprintf
	#define c_fprintf(FILE, ...) fprintf(FILE, __VA_ARGS__)
#else
	#define c_vprintf tfp_vprintf
	#define c_printf tfp_printf
	#define c_vsnprintf tfp_vsnprintf
	#define c_snprintf tfp_snprintf
	#define c_fprintf(FILE, ...) tfp_printf( __VA_ARGS__)
#endif

#define c_calloc calloc
#define c_exit(n) system_restart()
#define c_free free
#define c_malloc malloc
void selectionSort(void *base, size_t num, size_t width, int (*compare )(const void *, const void *));
#define c_qsort selectionSort
#define c_realloc realloc
#define c_strtod strtod
#define c_strtol strtol
#define c_strtoul strtoul
	
/* DATE */
#define c_tm modTm
#define c_timeval modTimeVal
#define c_time_t modTime_t
#define c_timezone modTimeZone

#define c_gettimeofday modGetTimeOfDay
#define c_gmtime modGmTime
#define c_localtime modLocalTime
#define c_mktime modMkTime
#define c_strftime modStrfTime

/* ERROR */
	
#define C_ENOMEM ENOMEM
#define C_EINVAL EINVAL
	
/* MATH */

#include <math.h>

//@@ MIN is entirely made up!
#define C_DBL_MIN		3.40282347e-38F
#define C_DBL_MAX		MAXFLOAT
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
#if ESP32
	#define C_RAND_MAX UINT32_MAX
#else
	#define C_RAND_MAX RAND_MAX
#endif

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
#if ESP32
	#define c_fmod fmod
#else
	#define c_fmod __ieee754_fmod_patch
#endif
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
#if ESP32
	#define c_rand esp_random
#else
	#define c_rand rand
#endif
#define c_round round
#define c_signbit signbit
#define c_sin sin
#define c_sinh sinh
#define c_sqrt sqrt
#define c_srand srand
#define c_tan tan
#define c_tanh tanh
#define c_trunc trunc

/* READ MEMORY */

#if ESP32
	#define c_read8(POINTER) (*((uint8_t *)(POINTER)))
	#define c_read16(POINTER) (*((uint16_t *)(POINTER)))
	#define c_read32(POINTER) (*((uint32_t *)(POINTER)))
#else
	#define c_read8 espRead8
	#define c_read16 espRead16
	#define c_read32 espRead32
#endif

#define c_read16be espRead16be
#define c_read32be espRead32be

/* STRING */

#define c_memcpy espMemCpy
#define c_memmove memmove
#define c_memset memset
#define c_memcmp espMemCmp
#define c_strcat espStrCat
#define c_strchr espStrChr
#define c_strcmp espStrCmp
#define c_strcpy espStrCpy
#define c_strlen espStrLen
#define c_strncat espStrNCat
#define c_strncmp espStrNCmp
#define c_strncpy espStrNCpy
#define c_strstr espStrStr
#define c_strrchr espStrRChr
#define c_isEmpty(s) (!c_read8(s))

/* 32-BIT MEMORY */

#define c_malloc_uint32 espMallocUint32
#define c_free_uint32 espFreeUint32

/* STACK */

#if !ESP32
	#define c_stackspace espStackSpace
#endif

/* FLASH */

#if ESP32
	#include "esp_partition.h"

	extern const esp_partition_t *gPartition;
	extern const void *gPartitionAddress;

	#define kModulesStart (gPartitionAddress)
	#define kModulesByteLength (gPartition ? gPartition->size : 0)
	#define kModulesEnd (kModulesStart + kModulesByteLength)
#else
	#define kFlashStart ((uint8_t *)0x040200000)
	#define kFlashEnd ((uint8_t *)0x040300000)

	extern uint8_t *espFindUnusedFlashStart(void);
	#define kModulesStart (espFindUnusedFlashStart())
	#define kModulesEnd kFlashEnd
	#define kModulesByteLength (kModulesEnd - kModulesStart)
#endif

#ifdef __cplusplus
}
#endif

#endif

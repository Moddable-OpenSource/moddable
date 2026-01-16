/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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

#ifndef __XSHOST__
#define __XSHOST__

#include <stdint.h>
#include <stddef.h>
#include "errno.h"

#include "mc.defines.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/printk.h>
#include <time.h>

#define zTHREAD_STACKSIZE	1024
#define zTHREAD_PRIORITY	7

/*
	link locations
 */

#define ICACHE_RAM_ATTR __attribute__((section(".data.xsram")))
#define ICACHE_FLASH_ATTR __attribute__((section(".flash")))
#define ICACHE_FLASH1_ATTR __attribute__((section(".flash.1")))
#define ICACHE_RODATA_ATTR __attribute__((section(".rodata")))
#define ICACHE_XS6RO_ATTR __attribute__((section(".rodata.xs6ro"))) __attribute__((aligned(4)))
#define ICACHE_XS6RO2_ATTR __attribute__((section(".rodata.xs6ro2"))) __attribute__((aligned(4)))
#define ICACHE_XS6STRING_ATTR __attribute__((section(".rodata.xs6string"))) __attribute__((aligned(4)))

#ifdef __cplusplus
extern "C" {
#endif


/*
    report
*/

extern void modLog_transmit(const char *msg);
extern void ESP_putc(int c);

//@@ MDK I'm using a local variable for scratch below to force the string into
// ram so EasyDMA can access it.
//		static const char scratch[] = msg ; 
//		char scratch[] = msg; 

#define modLog(msg) \
	do { \
		modLog_transmit(msg); \
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
    timer
*/

// #define modMilliseconds() (k_cyc_to_ms_trunc32(k_cycle_get_64()))
// #define modMicroseconds() (k_cyc_to_us_trunc32(k_cycle_get_64()))
#define modMilliseconds() ((uint32_t)k_uptime_get())
#define modMicroseconds() (modMilliseconds() * 1000)

#define modDelayMilliseconds(ms) k_msleep(ms)
#define modDelayMicroseconds(us) k_usleep(us)

extern void modTimersExecute(void);
extern int modTimersNext(void);

/*
	critical section
*/

#define modCriticalSectionDeclare
#define modCriticalSectionBegin()	k_sched_lock()		//@@ ??
#define modCriticalSectionEnd()		k_sched_unlock()

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
modTime_t modMkTime(struct modTm *tm);
void modStrfTime(char *s, size_t max, const char *format, const struct modTm *tm);

void modSetTime(uint32_t seconds);							// since 1970 - UNIX time
int32_t modGetTimeZone(void);								// seconds
void modSetTimeZone(int32_t timeZoneOffset);				// seconds
int32_t modGetDaylightSavingsOffset(void);					// seconds
void modSetDaylightSavingsOffset(int32_t daylightSavings);	// seconds

/*
	watchdog timer
*/

#if USE_WATCHDOG
	#define modWatchDogReset() nrfx_wdt_feed()
#else
	#define modWatchDogReset()
#endif

/*
    VM
*/

void xs_setup();
void xs_loop();

/*
	debugging
*/

void fxReceiveLoop(void);
void setupDebugger(uint32_t *running);
void flushDebugger(void);
extern struct k_thread *gMainTask;

/*
	messages
*/

typedef void (*modMessageDeliver)(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

#if defined(__XS__)
    int modMessagePostToMachine(xsMachine *the, uint8_t *message, uint16_t messageLength, modMessageDeliver callback, void *refcon);
    int modMessagePostToMachineFromISR(xsMachine *the, modMessageDeliver callback, void *refcon);
    int modMessageService(xsMachine *the, int maxDelayMS);
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

#define MOD_TASKS (true)
#define modTaskGetCurrent()		((uintptr_t)k_current_get());

#if MODDEF_XS_TEST
	extern uint8_t gSoftReset;
	#define modSoftReset() gSoftReset = 1
#endif

/* 
	c libraries
*/

#include <ctype.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define c_tolower tolower
#define c_toupper toupper

typedef jmp_buf c_jmp_buf;

#define c_longjmp longjmp
#define c_setjmp setjmp

typedef va_list c_va_list;
#define c_va_arg va_arg
#define c_va_end va_end
#define c_va_start va_start

extern void *my_calloc(size_t nitems, size_t size);
extern void *my_realloc(void *ptr, size_t size);
extern void *my_malloc(size_t size);
extern void my_free(void *ptr);
#define c_calloc k_calloc
#define c_malloc k_malloc
#define c_realloc k_realloc
#define c_free k_free

#define c_exit(n) sys_reboot(SYS_REBOOT_COLD)
#define c_qsort qsort
#define c_bsearch bsearch
#define c_strtod strtod
#define c_strtol strtol
#define c_strtoul strtoul
#define c_printf printf
#define c_vprintf vprintf
#define c_vsnprintf vsnprintf
#define c_snprintf snprintf
#define c_fprintf fprintf
//#define c_fprintf(a, ...) do { char out[256]; sprintf(out, __VA_ARGS__); modLogVar(out); } while (0)

#define myprintf(a, ...) do { char out[256]; sprintf(out, __VA_ARGS__); modLogVar(out); } while (0)

/* DATE */

#define c_tm struct tm
#define c_timeval modTimeVal
#define c_time_t time_t
#define c_timezone timezone

extern int gettimeofday(void *tv, void *unusedTZ);

#define c_gettimeofday modGetTimeOfDay
#define c_gmtime gmtime
c_tm *modLocalTime(const c_time_t *timep);
#define c_localtime modLocalTime
#define c_mktime mktime
#define c_strftime strftime
#define c_time time

#define C_EOF -1	// EOF
#define C_NULL NULL
#define C_ENOMEM ENOMEM
#define C_EINVAL EINVAL

#ifndef PATH_MAX
	#if MODDEF_XS_TEST
		#define PATH_MAX (256)
	#else
		#define PATH_MAX (128)
	#endif
#endif
#define C_PATH_MAX (PATH_MAX)

/* MATH */
#if 0	// fdlibm
#include "math.h"
#include "float.h"
#include "fdlibm.h"

#define C_M_E M_E
#define C_M_LN10 M_LN10
#define C_M_LN2 M_LN2
#define C_M_LOG10E M_LOG10E
#define C_M_LOG2E M_LOG2E
#define C_M_PI M_PI
#define C_M_SQRT1_2 M_SQRT1_2
#define C_M_SQRT2 M_SQRT2

#define C_NAN NAN
#define C_RAND_MAX RAND_MAX

#define C_FP_INFINITE 1
#define C_FP_NAN 0
#define C_FP_NORMAL 4
#define C_FP_SUBNORMAL 3
#define C_FP_ZERO 2

#define C_INFINITY (double)INFINITY

#define c_isfinite isfinite
#define c_isnormal isnormal
#define c_isnan isnan
#define c_fabs fabs
#define c_floor floor
#define c_sqrt sqrt
#define c_fpclassify fpclassify
#define c_log2 log2
#define c_rand rand
#define c_signbit signbit
#define C_MAX_SAFE_INTEGER (double)9007199254740991
#define C_MIN_SAFE_INTEGER (double)-9007199254740991
#define c_nearbyint nearbyint
#define c_round round
#define C_DBL_MAX DBL_MAX
#define C_DBL_MIN (double)5e-324
#define C_EPSILON (double)2.2204460492503130808472633361816e-16

#else /* NOT *** fdlibm */
#include <math.h>
#include <float.h>

#define C_DBL_MAX DBL_MAX
#define C_DBL_MIN (double)5e-324
#define C_EPSILON (double)2.2204460492503130808472633361816e-16
/*
#define C_FP_INFINITE FP_INFINITE
#define C_FP_NAN FP_NAN
#define C_FP_NORMAL FP_NORMAL
#define C_FP_SUBNORMAL FP_SUBNORMAL
#define C_FP_ZERO FP_ZERO
*/
#define C_M_E M_E
#define C_M_LN10 M_LN10
#define C_M_LN2 M_LN2
#define C_M_LOG10E M_LOG10E
#define C_M_LOG2E M_LOG2E
#define C_M_PI M_PI
#define C_M_SQRT1_2 M_SQRT1_2
#define C_M_SQRT2 M_SQRT2

#define C_RAND_MAX RAND_MAX

#define C_FP_INFINITE 1
#define C_FP_NAN 0
#define C_FP_NORMAL 4
#define C_FP_SUBNORMAL 3
#define C_FP_ZERO 2

#define C_INFINITY (double)INFINITY
#define C_MAX_SAFE_INTEGER (double)9007199254740991
#define C_MIN_SAFE_INTEGER (double)-9007199254740991
#define C_NAN ((double)NAN)
#define C_RAND_MAX RAND_MAX

#define C_FP_ILOGB0 FP_ILOGB0
#define C_FP_ILOGBNAN FP_ILOGBNAN
#define C_INT_MAX INT_MAX

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
#define c_ilogb ilogb
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

#define _M_LN2        0.693147180559945309417
#define M_E     2.7182818284590452354
#define M_LOG2E     1.4426950408889634074
#define M_LOG10E    0.43429448190325182765
#define M_LN2       _M_LN2
#define M_LN10      2.30258509299404568402 
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.78539816339744830962
#define M_1_PI      0.31830988618379067154
#define M_2_PI      0.63661977236758134308
#define M_2_SQRTPI  1.12837916709551257390
#define M_SQRT2     1.41421356237309504880
#define M_SQRT1_2   0.70710678118654752440

#endif

/* STRING */

uint16_t espRead16be(const void *addr);
uint32_t espRead32be(const void *addr);


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
#define c_isEmpty(s) (!c_read8(s))
#define c_strcspn strcspn
#define c_strspn strspn

/* ERROR */

#define mxGetKeySlotID(SLOT) (SLOT)->ID
#define mxGetKeySlotKind(SLOT) (SLOT)->kind


#define c_read8(POINTER) (*((uint8_t *)(POINTER)))
#define c_read16(POINTER) (*((uint16_t *)(POINTER)))
#define c_read32(POINTER) (*((uint32_t *)(POINTER)))
#define c_read16be espRead16be
#define c_read32be espRead32be

#if 0
/* FLASH */

extern nrf_fstorage_t fstorage;

// it appears that the flash offsets are placed into the memory map without any offset
#define kFlashStart ((uintptr_t)0)
#define kFlashSectorSize (modSPIFlashInit() ? fstorage.p_flash_info->erase_unit : 0)
#endif

#define kFlashSectorSize (4096)

extern uint8_t *espFindUnusedFlashStart(void);
#define kModulesStart ((uintptr_t)espFindUnusedFlashStart())

extern uint8_t _MODPREF_start;		// from linker
extern uint32_t _MODDABLE_start;	// from linker
extern uint32_t _MODDABLE_end;		// from linker
// #define kModulesEnd ((uintptr_t)&_MODPREF_start)
extern uint8_t _FSTORAGE_start;		// from linker
extern uint8_t _FSTORAGE_end;		// from linker

#define kModulesEnd ((uintptr_t)&_MODDABLE_end)
#define kModulesByteLength (kModulesEnd - kModulesStart)

enum {
	kPartitionMod = 1,
	kPartitionStorage,
	kPartitionBLEState
};

extern uint8_t modGetPartition(uint8_t which, uint32_t *offset, uint32_t *size);

extern uint8_t modSPIFlashInit(void);
extern uint8_t modSPIRead(uint32_t offset, uint32_t size, uint8_t *dst);
extern uint8_t modSPIWrite(uint32_t offset, uint32_t size, const uint8_t *src);
extern uint8_t modSPIErase(uint32_t offset, uint32_t size);

char *getModAtom(uint32_t atomTypeIn, int *atomSizeOut);

#ifdef __cplusplus
}
#endif

#endif /* __XSZEPHYR */


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

#include "applib/app_logging.h"
#include "applib/graphics/gcontext.h"
#include "applib/graphics/gtypes.h"
#include "applib/pbl_std/pbl_std.h"
#include "drivers/rtc.h"
#include "syscall/syscall.h"
#include "system/passert.h"
#include "util/time/time.h"
#include "kernel/pbl_malloc.h"


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


#include "mc.defines.h"


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

/* RESERVED MEMORY */
#if 0
extern uint32_t *dbl_reset_mem;

#define DFU_DBL_RESET_MEM		0x200041FC		// uint32_t, defined in bootloader
#define MOD_TIME_RESTORE_MEM	0x200041F0		// uint32_t + c_timeval
#define MOD_WAKEUP_REASON_MEM	0x200041E4		// uint32_t + uint32_t + uint32_t

#define BOOTLOADER_VER_MEM      0x200041D0  	// uint32_t

/* reset */
#define REBOOT_FAST_RESET		0x4ee5677e
#define REBOOT_TO_PROGRAMMING	0xbeefcafe
#define REBOOT_TO_VENDOR		0xf00dcafe
#define REBOOT_TO_OTA			0xfeed1cee

/* wake */
#define MOD_GPIO_WAKE_MAGIC		0x04dfcfbf
#define MOD_ANALOG_WAKE_MAGIC	0x9e60bfca

void nrf52_reboot(uint32_t kind);

#define nrf52_rebootToOTA()		nrf52_reboot(REBOOT_TO_OTA)
#define nrf52_rebootToDFU()		nrf52_reboot(REBOOT_TO_PROGRAMMING)
#define nrf52_rebootToVendor()	nrf52_reboot(REBOOT_TO_VENDOR)

extern void nrf52_get_mac(uint8_t *mac);

void nrf52_set_reset_reason(uint32_t resetReason);
uint32_t nrf52_get_reset_reason(void);

void nrf52_set_boot_latches(uint32_t bootLatch1, uint32_t bootLatch2);
uint32_t nrf52_get_boot_latch(uint32_t pin);
uint32_t *nrf52_get_boot_latches();
void nrf52_clear_boot_latch(uint32_t pin);

uint8_t nrf52_softdevice_enabled(void);

#define nrf52_bootloaderVersion()	(*((uint32_t*)BOOTLOADER_VER_MEM))
#endif

#define nrf52_reset()			pebble_reset()

/*
    timer
*/

#define modMilliseconds() ((uint32_t)(rtc_get_ticks()))
#define modMicroseconds() ((uint32_t)(rtc_get_ticks() * 1000))

#define modDelayMilliseconds(ms) vTaskDelay(ms)
#define modDelayMicroseconds(us) vTaskDelay(((us) + 500) / 1000)

extern void modTimersExecute(void);
extern int modTimersNext(void);

/*
	critical section
*/

#define modCriticalSectionDeclare
#define modCriticalSectionBegin()	vPortEnterCritical()
#define modCriticalSectionEnd()		vPortExitCritical()

/*
	date and time
*/

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
void setupDebugger(void);
void flushDebugger(void);
extern TaskHandle_t gMainTask;

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
#define modTaskGetCurrent()		((uintptr_t)xTaskGetCurrentTaskHandle());

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

#ifndef PATH_MAX
#define PATH_MAX 128
#endif


#define c_tolower tolower
#define c_toupper toupper

typedef jmp_buf c_jmp_buf;

#define c_longjmp longjmp
#define c_setjmp setjmp

typedef va_list c_va_list;
#define c_va_arg va_arg
#define c_va_end va_end
#define c_va_start va_start

#define c_calloc(a, b) app_calloc(a, b)
#define c_malloc(a) app_malloc(a)
#define c_realloc(a, b) app_realloc(a, b)
#define c_free(a) app_free(a)

#define c_exit(n) sys_app_fault(n)
#define c_qsort qsort
#define c_bsearch bsearch
#define c_strtod strtod
#define c_strtol strtol
#define c_strtoul strtoul
#define c_printf(...) APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, ##__VA_ARGS__)
#define c_vprintf vprintf
#define c_vsnprintf vsnprintf
#define c_snprintf snprintf
#define c_fprintf fprintf
//#define c_fprintf(a, ...) do { char out[256]; sprintf(out, __VA_ARGS__); modLogVar(out); } while (0)

#define myprintf(a, ...) do { char out[256]; sprintf(out, __VA_ARGS__); modLogVar(out); } while (0)

/* DATE */

struct pbl_timeval {
	 uint32_t	tv_sec;     /* seconds */
	 uint32_t	tv_usec;    /* microseconds */	
};

#define c_tm struct tm
#define c_timeval struct pbl_timeval
#define c_time_t time_t
#define c_timezone timezone

extern int pbl_gettimeofday(void *tv, void *unusedTZ);

#define c_gettimeofday pbl_gettimeofday
#define c_gmtime pbl_override_gmtime
#define c_localtime pbl_override_localtime
#define c_mktime pbl_override_mktime
#define c_strftime pbl_strftime
#define c_time pbl_override_time

/* ERROR */
//#if XS_PEBBLE
	#define NRF_ERROR_BASE_NUM		(0)
	#define NRF_ERROR_NO_MEM		(NRF_ERROR_BASE_NUM + 4)
	#define NRF_ERROR_INVALID_PARAM	(NRF_ERROR_BASE_NUM + 7)
//#endif

#define C_EOF -1	// EOF
#define C_NULL NULL
#define C_ENOMEM NRF_ERROR_NO_MEM
#define C_EINVAL NRF_ERROR_INVALID_PARAM

#ifndef PATH_MAX
	#define PATH_MAX 128
#endif
#define C_PATH_MAX 128	// PATH_MAX

/* MATH */

#if 1
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
#define C_FP_INFINITE 1
#define C_FP_NAN 0
#define C_FP_NORMAL 4
#define C_FP_SUBNORMAL 3
#define C_FP_ZERO 2

#define C_INFINITY (double)INFINITY
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

#endif

void qsort(void *base, size_t nel, size_t width, int (*compar)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t nel, size_t width, int (*compar)(const void *, const void*));

#ifndef M_E
#define M_E     2.7182818284590452354
#endif
#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#ifndef M_LN10
#define M_LN10      2.30258509299404568402
#endif

#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923
#endif

#define _M_LN2        0.693147180559945309417
#define M_LOG2E     1.4426950408889634074
#define M_LOG10E    0.43429448190325182765
#define M_LN2       _M_LN2
//#define M_LN10      2.30258509299404568402 
//#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.78539816339744830962
#define M_1_PI      0.31830988618379067154
#define M_2_PI      0.63661977236758134308
#define M_2_SQRTPI  1.12837916709551257390
#define M_SQRT2     1.41421356237309504880
#define M_SQRT1_2   0.70710678118654752440


/* STRING */

uint16_t espRead16be(const void *addr);
uint32_t espRead32be(const void *addr);


#include <string.h>
#define c_memcpy espMemCpy
#define c_memmove espMemMove
#define c_memset espMemSet
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
#define c_strcspn espStrcspn
#define c_strspn espStrspn

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

uint8_t nrf52_softdevice_enabled(void);

typedef void (*modOnSleepCallback)(uint32_t refCon);
struct modOnSleepRecord {
	struct modOnSleepRecord	*next;
	modOnSleepCallback	callback;
	uint32_t 			refCon;
};
typedef struct modOnSleepRecord modOnSleepRecord;
typedef struct modOnSleepRecord *modOnSleep;

void modAddOnSleepCallback(modOnSleepCallback callback, uint32_t refCon);
void modRemoveOnSleepCallback(modOnSleepCallback callback, uint32_t refCon);
void modRunOnSleepCallbacks(void);

void pebble_reset(void);

GFont modFindPebbleFont(const char *family, int size, int32_t *ascent, int32_t *descent, int32_t *leading);

//

//extern uint8_t espRead8(const void *addr);
//extern uint16_t espRead16(const void *addr);
//extern uint32_t espRead32(const void *addr);
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
extern size_t espStrcspn(const char *str, const char *strCharSet);
extern size_t espStrspn(const char *str, const char *strCharSet);
extern void *espMemMove(void *dst, const void *src, size_t n);
extern void *espMemSet(void *dst, int c, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* __XSNRF52__ */


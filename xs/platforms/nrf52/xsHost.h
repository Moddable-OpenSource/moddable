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

#ifndef __XSHOST__
#define __XSHOST__

#include <stdint.h>
#include <stddef.h>
#include "errno.h"

#include "FreeRTOS.h"
#include "task.h"

#include "nrf_fstorage_sd.h"

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
//		static const char scratch[] = msg ; \

#define modLog(msg) \
	do { \
		char scratch[] = msg; \
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

extern const char *gXSAbortStrings[];

/* RESERVED MEMORY */

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

#define nrf52_reset()			nrf52_reboot(0)
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

/*
    timer
*/
extern uint32_t nrf52_milliseconds();

#define modMilliseconds() ((uint32_t)(nrf52_milliseconds()))
#define modMicroseconds() ((uint32_t)(nrf52_milliseconds() * 1000))

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

#define MY_MALLOC 0
#if MY_MALLOC
extern void *my_calloc(size_t nitems, size_t size);
extern void *my_realloc(void *ptr, size_t size);
extern void *my_malloc(size_t size);
#define c_calloc my_calloc
#define c_malloc my_malloc
#define c_realloc my_realloc
#define c_free my_free
#else
extern void *pvPortCalloc(size_t nitems, size_t size);
extern void *pvPortRealloc(void *ptr, size_t size);
#define c_calloc pvPortCalloc
#define c_malloc pvPortMalloc
#define c_realloc pvPortRealloc
#define c_free vPortFree
#endif

#define c_exit(n) { nrf52_reset(); }
#define c_qsort qsort
#define c_strtod strtod
#define c_strtol strtol
#define c_strtoul strtoul
#define c_vprintf vprintf
#define c_printf printf
#define c_vsnprintf vsnprintf
#define c_snprintf snprintf
//#define c_fprintf fprintf
#define c_fprintf(a, ...) myprintf(a, __VA_ARGS__)

#define myprintf(a, ...) do { char out[256]; sprintf(out, __VA_ARGS__); modLogVar(out); } while (0)

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
//#define c_time time

/* ERROR */

#define C_EOF EOF
#define C_NULL NULL
#define C_ENOMEM NRF_ERROR_NO_MEM
#define C_EINVAL NRF_ERROR_INVALID_PARAM

#ifndef PATH_MAX
	#define PATH_MAX 128
#endif
#define C_PATH_MAX PATH_MAX

/* MATH */

#include <math.h>
#include <float.h>

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

#define C_ENOMEM NRF_ERROR_NO_MEM
#define C_EINVAL NRF_ERROR_INVALID_PARAM

#define mxGetKeySlotID(SLOT) (SLOT)->ID
#define mxGetKeySlotKind(SLOT) (SLOT)->kind


#define c_read8(POINTER) (*((uint8_t *)(POINTER)))
#define c_read16(POINTER) (*((uint16_t *)(POINTER)))
#define c_read32(POINTER) (*((uint32_t *)(POINTER)))
#define c_read16be espRead16be
#define c_read32be espRead32be
/* FLASH */

extern nrf_fstorage_t fstorage;

// it appears that the flash offsets are placed into the memory map without any offset
#define kFlashStart ((uintptr_t)0)
#define kFlashSectorSize (modSPIFlashInit() ? fstorage.p_flash_info->erase_unit : 0)

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

#ifdef __cplusplus
}
#endif

#endif /* __XSNRF52__ */


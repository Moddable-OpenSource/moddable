/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

#if ESP32
	#include "freertos/FreeRTOS.h"
	#include "freertos/task.h"
#endif

/* CPU */

#if !ESP32
	// esp8266
#elif ESP32 == 6
	#define kCPUESP32H2 1
	#define kTargetCPUCount 1
	#define kESP32TimerDef	int_clr
	#define XT_STACK_EXTRA_CLIB	1024
	#define XT_STACK_EXTRA 1024
#elif ESP32 == 5
	#define kCPUESP32C6 1
	#define kTargetCPUCount 1
	#define kESP32TimerDef	int_clr
	#define XT_STACK_EXTRA_CLIB	1024
	#define XT_STACK_EXTRA 1024
#elif ESP32 == 4
	#define kCPUESP32C3 1
	#define kTargetCPUCount 1
	#define kESP32TimerDef	int_clr
	#define XT_STACK_EXTRA_CLIB	1024
	#define XT_STACK_EXTRA 1024
#elif ESP32 == 3
	#define kCPUESP32S3 1
	#define kTargetCPUCount 2
	#define kESP32TimerDef	int_clr
#elif ESP32 == 2
	#define kCPUESP32S2 1
	#define kTargetCPUCount 1
	#define kESP32TimerDef	int_clr
#elif ESP32 == 1
	#define kCPUESP32	1
	#define kTargetCPUCount 2
	#define kESP32TimerDef	int_clr_timers
#else
	#error undefined platform
	#define kTargetCPUCount 1
	#define kESP32TimerDef	int_clr
#endif


/*
	link locations
*/

#if (ESP32 == 4)
	#define ICACHE_RODATA_ATTR __attribute__((section(".rodata")))
	#define ICACHE_XS6RO_ATTR __attribute__((section(".rodata.xs6ro"))) __attribute__((aligned(4)))
	#define ICACHE_XS6RO2_ATTR __attribute__((section(".rodata.xs6ro2"))) __attribute__((aligned(4)))
	#define ICACHE_XS6STRING_ATTR __attribute((section(".data"))) __attribute__((aligned(4)))
#elif ESP32
	#define ICACHE_RODATA_ATTR __attribute__((section(".rodata")))
	#define ICACHE_XS6RO_ATTR __attribute__((section(".rodata.xs6ro"))) __attribute__((aligned(4)))
	#define ICACHE_XS6RO2_ATTR __attribute__((section(".rodata.xs6ro2"))) __attribute__((aligned(4)))
	#define ICACHE_XS6STRING_ATTR __attribute((section(".rodata.str1.4"))) __attribute__((aligned(4)))
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
extern size_t espStrcspn(const char *str, const char *strCharSet);
extern size_t espStrspn(const char *str, const char *strCharSet);

extern void *espMallocUint32(int count);
extern void espFreeUint32(void *t);

#if ESP32
	#define modGetLargestMalloc(bytes) (heap_caps_get_largest_free_block(MALLOC_CAP_8BIT))
#endif

/*
	report
*/

extern void modLog_transmit(const char *msg);
extern void ESP_putc(int c);
extern void ESP_put(uint8_t *c, int count);


#if !ESP32
	#define modLog(msg) \
		do { \
			static const char scratch[] ICACHE_XS6STRING_ATTR = msg ; \
			modLog_transmit(scratch); \
		} while (0)
#else
	#define modLog(msg) \
		do { \
			static const char scratch[] = msg ; \
			modLog_transmit(scratch); \
		} while (0)
#endif
#define modLogVar(msg) modLog_transmit(msg)
#if kESP8266Version >= 24
	extern char* ltoa(long value, char* result, int base);
	#define itoa(value, result, base) ltoa(value, result, base)
#endif
#define modLogInt(msg) \
	do { \
		char temp[10]; \
		itoa((int)(msg), temp, 10); \
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

/*
	start-up
*/

extern void modPrelaunch(void);

/*
	serial
*/

extern int ESP_getc(void);
extern void ESP_putc(int c);
extern uint8_t ESP_isReadable(void);
extern uint8_t ESP_setBaud(int baud);

/*
	timer
*/

#if ESP32
	#define modMilliseconds() ((uint32_t)xTaskGetTickCount())
	#define modMicroseconds() ((uint32_t)esp_timer_get_time())

	extern volatile uint32_t gCPUTime;
	#define modMicrosecondsInstrumentation() (gCPUTime)

	#define modDelayMilliseconds(ms) vTaskDelay(ms)
	#define modDelayMicroseconds(us) vTaskDelay(((us) + 500) / 1000)

#else
	#define modMilliseconds() ((uint32_t)(millis()))
	#define modMicroseconds() ((uint32_t)(system_get_time()))

	#define modDelayMilliseconds(ms) delay(ms)
	#define modDelayMicroseconds(us) ets_delay_us(us)
#endif

extern void modTimersExecute(void);
extern int modTimersNext(void);

/*
	critical section (one deep)
*/

#if !ESP32
	#define modCriticalSectionDeclare
	#define modCriticalSectionBegin() noInterrupts()
	#define modCriticalSectionEnd() interrupts()
#else
	#define modCriticalSectionDeclare
	extern portMUX_TYPE gCriticalMux;
#if kCPUESP32C3 || kCPUESP32C6 || kCPUESP32H2
	#define modCriticalSectionBegin()	portENTER_CRITICAL_SAFE(&gCriticalMux)
	#define modCriticalSectionEnd()		portEXIT_CRITICAL_SAFE(&gCriticalMux)
#else
	#define modCriticalSectionBegin() vPortEnterCriticalSafe(&gCriticalMux)
	#define modCriticalSectionEnd() vPortExitCriticalSafe(&gCriticalMux)
#endif
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

#if !ESP32
	#define modWatchDogReset() system_soft_wdt_feed()
#else
	#if CONFIG_ESP_TASK_WDT_EN
		#define modWatchDogReset() esp_task_wdt_reset()
	#else
		#define modWatchDogReset()
	#endif
#endif

/*
	debugging
*/

void fxReceiveLoop(void);

/*
	messages
*/

typedef void (*modMessageDeliver)(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

#if defined(__XS__)
	int modMessagePostToMachine(xsMachine *the, uint8_t *message, uint16_t messageLength, modMessageDeliver callback, void *refcon);
	#if ESP32
		int modMessagePostToMachineFromISR(xsMachine *the, modMessageDeliver callback, void *refcon);
		void modMessageService(xsMachine *the, int maxDelayMS);
	#else
		int modMessagePostToMachineFromPool(xsMachine *the, modMessageDeliver callback, void *refcon);
		int modMessageService(void);
	#endif
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

#if ESP32
	#define MOD_TASKS (true)

	#define modTaskGetCurrent() ((uintptr_t)xTaskGetCurrentTaskHandle())
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
#if ESP32
	#define c_exit(n) esp_restart()
#else
	#define c_exit(n) {system_restart(); while (1) esp_yield();}
#endif
#define c_free free
#define c_malloc malloc
void selectionSort(void *base, size_t num, size_t width, int (*compare )(const void *, const void *));
#define c_qsort selectionSort
#define c_realloc realloc
#define c_strtod strtod
#define c_strtol strtol
#define c_strtoul strtoul
	
/* DATE */

#if ESP32
	#define c_tm struct tm
	#define c_timeval struct timeval
	#define c_time_t time_t
	#define c_timezone timezone

	#define c_gettimeofday gettimeofday
	#define c_mktime mktime
	#define c_localtime localtime
#else
	#define c_tm modTm
	#define c_timeval modTimeVal
	#define c_time_t modTime_t
	#define c_timezone modTimeZone

	#define c_gettimeofday modGetTimeOfDay
	#define c_mktime modMkTime
	#define c_localtime modLocalTime
#endif

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
//	#define C_RAND_MAX RAND_MAX
	#define C_RAND_MAX (0xFFFFFFFF)
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
#if ESP32
	double __ieee754_pow(double x, double y);
	#define c_pow __ieee754_pow
#else
	#define c_pow pow
#endif
#if ESP32
	#define c_rand esp_random
#else
	#define c_rand() (*(volatile uint32_t *)0x3FF20E44)
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

#if ESP32 && !defined(kCPUESP32C6) && !defined(kCPUESP32H2)
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
#if ESP32
	#define c_strcspn strcspn
	#define c_strspn strspn
#else
	#define c_strcspn espStrcspn
	#define c_strspn espStrspn
#endif

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
	extern const uint8_t *gPartitionAddress;

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
#define kFlashSectorSize (4096)

uint8_t modSPIRead(uint32_t offset, uint32_t size, uint8_t *dst);
uint8_t modSPIWrite(uint32_t offset, uint32_t size, const uint8_t *src);
uint8_t modSPIErase(uint32_t offset, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif

/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
#ifndef __XS6GECKO__
#define __XS6GECKO__

#include <stdint.h>
#include <stddef.h>
#include "malloc.h"
#include "em_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
	link locations
*/
#define ICACHE_RODATA_ATTR
#define ICACHE_XS6RO_ATTR
#define ICACHE_XS6RO2_ATTR
#define ICACHE_XS6STRING_ATTR

/*
	memory and strings
*/

#define espRead8(POINTER) *((txU1*)POINTER)

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

extern uint32_t gecko_milliseconds();
extern void gecko_delay(uint32_t ms);

#define modMilliseconds() gecko_milliseconds()
#define modDelayMilliseconds(ms) gecko_delay(ms)
#define modDelayMicroseconds(us) gecko_delay(((us) + 500) / 1000)

extern void modTimersExecute(void);
extern int modTimersNext(void);

/*
	critical section
*/

#define modCriticalSectionDeclare	CORE_DECLARE_IRQ_STATE
#define modCriticalSectionBegin()	CORE_ENTER_ATOMIC()
#define modCriticalSectionEnd()		CORE_EXIT_ATOMIC()

/*
	date and time
*/

typedef uint32_t modTime_t;

struct modTimeVal {
	modTime_t	tv_sec;		/* seconds */
	uint32_t	tv_usec;	/* microseconds */
};
typedef struct modTimeVal modTimeVal;

struct modTimeZone {
	int32_t		tz_minuteswest;		/* minutes west of Greenwich */
	int32_t		tz_dsttime;			/* type of DST correction */
};

struct modTm {
	int32_t		tm_sec;
	int32_t		tm_min;
	int32_t		tm_hour;
	int32_t		tm_mday;
	int32_t		tm_mon;
	int32_t		tm_year;
	int32_t		tm_wday;
	int32_t		tm_yday;
	int32_t		tm_isdst;
};
typedef struct modTm modTm;

void modGetTimeOfDay(struct modTimeVal *tv, struct modTimeZone *tz);
struct modTm *modGmTime(const modTime_t *timep);
struct modTm *modLocalTime(const modTime_t *timep);
modTime_t modMkTime(struct modTm *tm);
void modStrfTime(char *s, size_t max, const char *format, const struct modTm *tm);

void modSetTime(uint32_t seconds);					// since 1970 - UNIX epoch
int32_t	modGetTimeZone(void);						// seconds
void modSetTimeZone(int32_t timeZoneOffset);		// seconds
int32_t modGetDaylightSavingsOffset(void);			// seconds
void modSetDaylightSavingsOffset(int32_t daylightSavings);	// seconds
	
/*
    VM
*/

#ifdef __XS__
    extern xsMachine *gThe;     // the one XS6 virtual machine running
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
	int modMessagePostToMachineFromPool(xsMachine *the, modMessageDeliver callback, void *refcon);
	int modMessageService(void);
#endif

/*
	 task
*/

#if defined(__XS__)
	void modMachineTaskInit(xsMachine *the);
	void modMachineTaskUninit(xsMachine *the);
	void modMachineTaskWait(xsMachine *the);
	void modMachineTaskWake(xsMachine *the);
#endif

/*
	sleep
*/

uint32_t geckoGetResetCause();
uint32_t geckoGetPersistentValue(uint32_t reg);
void geckoSetPersistentValue(uint32_t reg, uint32_t val);
void geckoUnlatchPinRetention();

void geckoEnterEM1();
void geckoEnterEM2();
void geckoEnterEM3();
void geckoSleepEM4(uint32_t ms);

void geckoEM1Idle(uint32_t ms);		// idle ms at EM1 (to not disable various peripherals)

void gecko_schedule();			// at next interrupt, stop current gecko_delay

void geckoDisableSysTick();

void geckoStartRTCC();

#define kmdblTag 'mdbl'
#define kxtimTag 'xtim'
#define kSleepTagReg    31
#define kSleepRemainReg 30

/*
	debugger
*/
 
void setupDebugger();
extern int gDebuggerSetup;

extern uint32_t gMsgBuffer[1024];
extern uint32_t gMsgBufferCnt;
extern uint32_t gMsgBufferMax;
#define geckoLogNum(x)	(gMsgBuffer[(gMsgBufferCnt++ > gMsgBufferMax) ? 0 : gMsgBufferCnt] = x)

/*
	default types
*/

#if MIGHTY_GECKO || BLUE_GECKO || THUNDERBOARD2
    #define USE_CRYOTIMER   1   // use cryotimer for EM4 and delay
    #define USE_RTCC        1   // use RTCC for EM4 and delay and ticks
    #include "em_cryotimer.h"
    #include "em_rtcc.h"
#endif
#if GIANT_GECKO
    #define USE_BURTC        1   // use RTCC for EM4 and delay and ticks
    #define USE_RTC			1
	#include "em_burtc.h"
    #include "em_rtc.h"
#endif

#if 0

#if EFR32MG1P132F256GM48			// MightyGecko - Thunderboard Sense
	#define geckoDefaultI2C			I2C0
	#define geckoDefaultI2CClock	cmuClock_I2C0
	#define geckoNeedsI2CRoute 		0
	#define geckoDefaultGPIOPort	gpioPortD
	#define geckoDefaultGPIOPin		11
	#define geckoNeedsUSARTRoute 	0

#elif EFM32GG990F1024				// Giant Gecko SK3700
	#define geckoDefaultI2C			I2C1
	#define geckoDefaultI2CClock	cmuClock_I2C1
	#define geckoNeedsI2CRoute 		1
	#define geckoDefaultGPIOPort	gpioPortE
	#define geckoNeedsUSARTRoute	1

#elif EFR32MG12P332F1024GL125		// Mighty Gecko radio test (BRD4162A)
	#define geckoDefaultI2C			I2C0
	#define geckoDefaultI2CClock	cmuClock_I2C0
	#define geckoNeedsI2CRoute 		0
	#define geckoDefaultGPIOPort	gpioPortF
	#define geckoDefaultGPIOPin		4
	#define geckoNeedsUSARTRoute 	0
	
#endif

#endif

/*
	instrumentation
*/

#if defined(mxInstrument) && defined(__XS__)
	#include "modTimer.h"

	void espInstrumentMachineBegin(xsMachine *the, modTimerCallback instrumentationCallback, int count, char **names, char **units);
	void espInstrumentMachineEnd(xsMachine *the);
	void espInstrumentMachineReset(xsMachine *the);
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
#define c_snprintf snprintf
#define c_fprintf fprintf

#define c_exit exit

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
#define c_free free

#define c_qsort qsort
#define c_strtod strtod
#define c_strtol strtol
#define c_strtoul strtoul

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
	
#include <errno.h>
#define C_ENOMEM ENOMEM
#define C_EINVAL EINVAL

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

/* READ MEMORY */

#define c_read8(POINTER) *((txU1*)(POINTER))
#define c_read16(POINTER) *((txU2*)(POINTER))
#define c_read16be(POINTER) ((((txU2)((txU1*)POINTER)[0]) << 8) | ((txU2)((txU1*)POINTER)[1]))
#define c_read32(POINTER) *((txU4*)(POINTER))
#define c_read32be(POINTER) ((((txU4)((txU1*)POINTER)[0]) << 24) | (((txU4)((txU1*)POINTER)[1]) << 16) | (((txU4)((txU1*)POINTER)[2]) << 8) | ((txU4)((txU1*)POINTER)[3]))

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

#ifdef __cplusplus
}
#endif

#endif /* __XS6GECKO__ */


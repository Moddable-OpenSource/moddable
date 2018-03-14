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

#include "mc.defines.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define sint8_t int8_t
#define sint16_t int16_t

void xs_setup();
void xs_loop();

/*
    timer
*/
extern void modTimersExecute(void);
extern int modTimersNext(void);

#define modDelayMilliseconds(ms) gecko_delay(ms)
#define modDelayMicroseconds(us) gecko_delay(((us) + 500) / 1000)
#define modMilliseconds() gecko_milliseconds()

extern void gecko_delay(uint32_t ms);
extern uint32_t gecko_milliseconds();

/*
	critical section
*/
#define modCriticalSectionBegin()
#define modCriticalSectionEnd()

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
    report
*/

extern void modLog_transmit(const char *msg);
extern void ESP_putc(int c);


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

#define xmodLog(msg)
#define xmodLogVar(msg)
#define xmodLogInt(msg)

/*
    VM
*/

#ifdef __XS__
    extern xsMachine *gThe;     // the one XS6 virtual machine running
	extern void *ESP_cloneMachine(uint32_t allocation, uint32_t stackCount, uint32_t slotCount, const char *name);

	uint8_t modRunPromiseJobs(xsMachine *the);		// returns true if promises still pending
#else
	extern void *ESP_cloneMachine(uint32_t allocation, uint32_t stackCount, uint32_t slotCount, const char *name);

#endif

/*
	messages
*/
typedef void (*modMessageDeliver)(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

#ifdef __XS__
	int modMessagePostToMachine(xsMachine *the, uint8_t *message, uint16_t messageLength, modMessageDeliver callback, void *refcon);
	int modMessagePostToMachineFromPool(xsMachine *the, modMessageDeliver callback, void *refcon);
	int modMessageService(void);

	void modMachineTaskInit(xsMachine *the);
	void modMachineTaskUninit(xsMachine *the);
	void modMachineTaskWait(xsMachine *the);
	void modMachineTaskWake(xsMachine *the);
#endif

/*
	Sleep
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
#include <stdio.h>
void setupDebugger();
extern int gDebuggerSetup;
uint8_t ESP_isReadable();
void ESP_putc(int c);
int ESP_getc(void);
void modLog_transmit(const char *msg);

extern uint32_t gMsgBuffer[1024];
extern uint32_t gMsgBufferCnt;
extern uint32_t gMsgBufferMax;
#define geckoLogNum(x)	(gMsgBuffer[(gMsgBufferCnt++ > gMsgBufferMax) ? 0 : gMsgBufferCnt] = x)

/*
	Default types
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


#ifdef __cplusplus
}
#endif

#endif /* __XS6LINUX__ */


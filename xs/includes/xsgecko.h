/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

#ifndef __XS6GECKO__
#define __XS6GECKO__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define sint8_t int8_t
#define sint16_t int16_t
/*
    timer
*/

typedef struct modTimerRecord modTimerRecord;
typedef modTimerRecord *modTimer;
typedef void (*modTimerCallback)(modTimer timer, void *refcon, uint32_t refconSize);
extern modTimer modTimerAdd(int firstInterval, int secondInterval, modTimerCallback cb, void *refcon, int refconSize);
extern void modTimerRemove(modTimer timer);
extern modTimer modTimerCallWhenSafe(modTimerCallback cb, void *refcon, int refconSize);

extern void modTimersExecute(void);
extern int modTimersNext(void);
extern int modTimersNextScript(void);

#define modDelayMilliseconds(ms) delay(ms)
#define modDelayMicroseconds(us) delay(((us) + 500) / 1000)

extern uint32_t modMilliseconds(void);

/*
	critical section
*/
#define modCriticalSectionBegin()
#define modCriticalSectionEnd()

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
	extern void *ESP_cloneMachine(uint32_t allocation, uint32_t stackCount, uint32_t slotCount, uint8_t disableDebug);

	uint8_t xsRunPromiseJobs(xsMachine *the);		// returns true if promises still pending
#else
	extern void *ESP_cloneMachine(uint32_t allocation, uint32_t stackCount, uint32_t slotCount, uint8_t disableDebug);

#endif


/*
	Sleep
 */
uint32_t geckoGetResetCause();
uint32_t geckoGetPersistentValue(uint32_t reg);
void geckoSetPersistentValue(uint32_t reg, uint32_t val);
void geckoSleepEM4(uint32_t ms);

/*
	Default types
 */
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


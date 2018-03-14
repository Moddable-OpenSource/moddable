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

#include "xs.h"
#include "xsgecko.h"
#include "mc.defines.h"

#include "em_cmu.h"
#include "em_emu.h"
#include "em_rmu.h"
#include "em_gpio.h"

#include "gpiointerrupt/inc/gpiointerrupt.h"

#if MIGHTY_GECKO || BLUE_GECKO || THUNDERBOARD2
	#define USE_CRYOTIMER	1
	#include "em_cryotimer.h"
#endif
#if GIANT_GECKO
	#define USE_RTC			1
	#include "em_rtc.h"
#endif

#ifndef MODDEF_SLEEP_IDLELEVEL
	#define MODDEF_SLEEP_IDLELEVEL 3
#endif

#if useRTCC
	#include "em_rtcc.h"
#endif

#if THUNDERBOARD2
	extern uint32_t msTickCount;
#else
	volatile uint32_t msTickCount;
#endif

void gecko_delay(uint32_t ms);
//uint32_t setupRTCTimeout(uint32_t ms);

extern uint32_t gResetCause;
//uint32_t gGeckoSetDelayMode = 0;
uint32_t gModMaxIdleSleep = MODDEF_SLEEP_IDLELEVEL;
uint32_t gModIdleSleep = MODDEF_SLEEP_IDLELEVEL;
static uint8_t gecko_schedule_now = 0;		// interrupts delay

uint32_t geckoGetResetCause() {
	return gResetCause;
}


/*  /MODDABLE */

#if USE_CRYOTIMER
int findCRYOPeriod(uint32_t ms) {
	int i;
	for (i=0; i<33; i++) {
		if (ms < (1 << i))
			return i-1;
	}
	return 32;
}

uint32_t findCRYOPeriodMS(uint32_t cryoPeriod) {
	return (1 << cryoPeriod);
}

void CRYOTIMER_IRQHandler() {
	CRYOTIMER_IntClear(CRYOTIMER_IntGet());
}

#endif

void geckoConfigureSysTick() {
   uint32_t ticks;
   ticks = CMU_ClockFreqGet( cmuClock_CORE ) / 1000; /* 1 msec interrupts  */
   SysTick_Config( ticks );
}

void geckoDelayLoop( uint32_t ms )
{
   uint32_t curTicks;

   curTicks = msTickCount;
   while( ( msTickCount - curTicks ) < ms ) {
      EMU_EnterEM1();
   }

   return;
}

#if THUNDERBOARD2
#else
void SysTick_Handler( void )
{
   msTickCount++;
}
#endif

void geckoDisableSysTick() {
	SysTick->CTRL = 0;
}
void geckoEnableSysTick() {
	SysTick->CTRL = 1;
}

void geckoEnterEM1()
{
	geckoDisableSysTick();
	EMU_EnterEM1();
	geckoEnableSysTick();
}
void geckoEnterEM2()
{
	geckoDisableSysTick();
	EMU_EnterEM2(true);
	geckoEnableSysTick();
}
void geckoEnterEM3()
{
	geckoDisableSysTick();
	EMU_EnterEM3(true);
	geckoEnableSysTick();
}


#if USE_CRYOTIMER
uint32_t setupCryotimerTimeout(uint32_t delay)
{
    CRYOTIMER_Init_TypeDef init = CRYOTIMER_INIT_DEFAULT;
CMU_ClockEnable(cmuClock_CRYOTIMER, false);
    CMU_ClockEnable(cmuClock_CRYOTIMER, true);
    CRYOTIMER_IntClear(CRYOTIMER_IF_PERIOD);
    init.enable = true;
    init.osc = cryotimerOscULFRCO;		// 1000Hz clock
    init.presc = cryotimerPresc_1;
    init.period = findCRYOPeriod(delay);
    init.em4Wakeup = true;
    CRYOTIMER_Init(&init);

    CRYOTIMER_IntClear(CRYOTIMER_IF_PERIOD);
    CRYOTIMER_IntEnable(CRYOTIMER_IEN_PERIOD);
    NVIC_ClearPendingIRQ(CRYOTIMER_IRQn);
    NVIC_EnableIRQ(CRYOTIMER_IRQn);

	return (findCRYOPeriodMS(init.period));
}
#else
uint32_t setupRTCCTimeout(uint32_t delay) {
}

void RTC_IRQHandler() {
	RTC_IntClear(RTC_IntGet());
}

uint32_t setupRTCTimeout(uint32_t delay) {
	RTC_Init_TypeDef rtcInit = RTC_INIT_DEFAULT;

	CMU_ClockEnable(cmuClock_CORELE, true);

//	CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);
// The following two lines does the ClockSelect
	CMU->LFCLKSEL &= ~_CMU_LFCLKSEL_LFA_MASK;
	CMU->LFCLKSEL |= CMU_LFCLKSEL_LFAE_ULFRCO;

	CMU_ClockEnable(cmuClock_RTC, true);
	RTC_IntDisable(RTC_IEN_COMP1);
	RTC_IntClear(RTC_IEN_COMP1);
	NVIC_DisableIRQ(RTC_IRQn);
	rtcInit.comp0Top = 0;
	RTC_Init(&rtcInit);
	RTC_CompareSet(0, _RTC_COMP0_COMP0_MASK);
	if (RTC->CNT > 0x800000)
		RTC->CNT = 0;
	RTC_CompareSet(1, delay + RTC->CNT);

	NVIC_EnableIRQ(RTC_IRQn);
	RTC_IntEnable(RTC_IEN_COMP1);
	RTC_Enable(true);
}
#endif

void geckoStartRTCC(void)
{
#if MODDEF_SLEEP_RETENTION_MEMORY
   CMU_ClockEnable(cmuClock_CORELE, true);
    CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_ULFRCO); //<---Enable ULFRCO as LFECLK 1kHz clock

    /* need RTCC for retention register */
    CMU_ClockEnable(cmuClock_RTCC, true);
    RTCC_Init_TypeDef rtccInit = RTCC_INIT_DEFAULT;
      rtccInit.presc = rtccCntPresc_1;
      rtccInit.cntWrapOnCCV1 = true;
      rtccInit.debugRun = true;
      RTCC_Init(&rtccInit);
    RTCC_EM4WakeupEnable(false);		// we're using the cryotimer for this
#endif
}

uint32_t gecko_milliseconds(void)
{
	return msTickCount;
}

void gecko_delay(uint32_t ms)
{
#if USE_CRYOTIMER
	volatile uint32_t first, last;
	setupCryotimerTimeout(ms);
    last = first = CRYOTIMER->CNT;

	while ((last - first) < ms) {
		switch (gModIdleSleep) {
			case 1: geckoEnterEM1(); break;
			case 2: geckoEnterEM2(); break;
			default:
			case 3: geckoEnterEM3(); break;
		}
		last = CRYOTIMER->CNT;
		if (gecko_schedule_now)
			break;
	}
	gecko_schedule_now = 0;

    NVIC_DisableIRQ(CRYOTIMER_IRQn);
    msTickCount += (last - first);
    CMU_ClockEnable(cmuClock_CRYOTIMER, false);		// this will null out CRYOTIMER->CNT

#else
	volatile uint32_t first, last;

	setupRTCTimeout(ms);
	last = first = RTC->CNT;
	while( (last - first) < ms ) {
		switch (gModIdleSleep) {
			case 1: geckoEnterEM1(); break;
			case 2: geckoEnterEM2(); break;
			default:
			case 3: geckoEnterEM3(); break;
		}
		last = RTC->CNT;
	}

	msTickCount += (last - first);
#endif
}

void geckoUnlatchPinRetention() {
#if MODDEF_SLEEP_RETENTION_GPIO
	/** Retention through EM4 and wakeup: call EMU_UnlatchPinRetention() to unlatch */
	EMU_UnlatchPinRetention();
#endif
}

void geckoCheckSleepRemainder() {
#if 0
#if MODDEF_SLEEP_REPEAT_EM4
#if MODDEF_SLEEP_RETENTION_MEMORY
	uint32_t remainingTime, check;

	geckoStartRTCC();

	check = geckoGetPersistentValue(kSleepTagReg);
	if (check == kxtimTag) {
		remainingTime = geckoGetPersistentValue(kSleepRemainReg);
		geckoSleepEM4(remainingTime);
	}
#else
	#error sleep_retention_memory must be true for repeat em4
#endif
#endif
#endif
}


void geckoStoreGpioRetention() {
#if 0
	int pinStates = 0;

	if (GPIO_PinOutGet(MODDEF_IOTCONTROL_GPIOPOWER_PORT, MODDEF_IOTCONTROL_GPIOPOWER_PIN)) {
		pinStates += 0x1;
	}
	if (GPIO_PinOutGet(MODDEF_IOTCONTROL_SENSORPOWER_PORT, MODDEF_IOTCONTROL_SENSORPOWER_PIN)) {
		pinStates += 0x2;
	}
 	   
	geckoSetPersistentValue(29, pinStates);
#endif
}

void geckoRestoreGpioRetention() {
#if 0
	int pinStates;

	geckoStartRTCC();
	pinStates = geckoGetPersistentValue(29);
	if (pinStates & 0xfffffff0)
		pinStates = 0;
	else {
		if (pinStates & 0x1)
			GPIO_PinOutSet(MODDEF_IOTCONTROL_GPIOPOWER_PORT, MODDEF_IOTCONTROL_GPIOPOWER_PIN);
		else
			GPIO_PinOutClear(MODDEF_IOTCONTROL_GPIOPOWER_PORT, MODDEF_IOTCONTROL_GPIOPOWER_PIN);
		if (pinStates & 0x2)
			GPIO_PinOutSet(MODDEF_IOTCONTROL_SENSORPOWER_PORT, MODDEF_IOTCONTROL_SENSORPOWER_PIN);
		else
			GPIO_PinOutSet(MODDEF_IOTCONTROL_SENSORPOWER_PORT, MODDEF_IOTCONTROL_SENSORPOWER_PIN);
	}
#endif
}

void geckoRestoreFromSleep() {
	setupDebugger();
    geckoRestoreGpioRetention();
    geckoCheckSleepRemainder();
}

volatile uint8_t waitingForGPIO = 0;

void waitGPIOHandler(uint8_t pin) {
    waitingForGPIO = 0;
}

void waitForGPIO(int port, int pin) {
#if 0
    gecko_delay(5);
#else
    waitingForGPIO = 1;
    GPIOINT_CallbackRegister(pin, waitGPIOHandler);
    GPIO_IntConfig(port, pin, false, true, true);
//  GPIO_IntEnable(1<<pin); // enable is last parameter of GPIO_IntConfig
    while (waitingForGPIO && GPIO_PinInGet(port, pin))
        geckoEnterEM2();
//        gecko_delay(5);
    GPIO_IntDisable(1<<pin);
    GPIOINT_CallbackUnRegister(pin);
#endif
}

static int lastSleep;
void geckoSetMaxSleep(int sleep) {
	lastSleep = gModIdleSleep;
	gModIdleSleep = sleep;
}

void geckoEM1Idle(uint32_t ms) {
	uint32_t last;
	last = gModIdleSleep;
	gModIdleSleep = 1;
	gecko_delay(ms);
	gModIdleSleep = last;
}

// gecko_schedule causes the next interrupt to stop a gecko_delay loop
void gecko_schedule() {
	gecko_schedule_now = 1;		// interrupts delay
	// need to post an interrupt?
}


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

#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_burtc.h"
#include "em_rmu.h"
#include "em_gpio.h"

#include "xsHost.h"

#include "mc.defines.h"

#if MIGHTY_GECKO || BLUE_GECKO
	#define USE_CRYOTIMER	1
	#include "em_cryotimer.h"
	uint32_t setupCryotimerTimeout(uint32_t delay);
#endif
#if GIANT_GECKO
	#define USE_RTC			1
	#include "em_rtc.h"
#endif

#if USE_CRYOTIMER
#include "em_cryotimer.h"
#include "em_rtcc.h"
#include "em_bus.h"
#endif

void geckoDelayLoop( uint32_t ms );

void geckoStoreGpioRetention(void);

uint32_t geckoGetPersistentValue(uint32_t reg) {
#if USE_CRYOTIMER
	return RTCC->RET[reg].REG;
#else
	return BURTC->RET[reg].REG;
#endif
}

void geckoSetPersistentValue(uint32_t reg, uint32_t val) {
#if USE_CRYOTIMER
	RTCC->RET[reg].REG = val;
#else
	BURTC->RET[reg].REG = val;
#endif
}

void configEM4(void) {
#if USE_CRYOTIMER
	EMU_EM4Init_TypeDef init_EM4 = EMU_EM4INIT_DEFAULT;
	init_EM4.em4State = emuEM4Hibernate;
//	init_EM4.pinRetentionMode = emuPinRetentionEm4Exit;	// reset gpio pins after exit from EM4
#if MODDEF_SLEEP_RETENTION_GPIO
	init_EM4.pinRetentionMode = emuPinRetentionLatch;
	  /** Retention through EM4 and wakeup: call EMU_UnlatchPinRetention() to
	      release pins from retention after EM4 wakeup */
#else
	init_EM4.pinRetentionMode = emuPinRetentionDisable;
#endif
	init_EM4.retainUlfrco = true;
//	init_EM4.retainLfrco = true;
//	init_EM4.vScaleEM4HVoltage = emuVScaleEM4H_LowPower;	// maybe this is too low to run some gpio
	EMU_EM4Init( &init_EM4 );
#else
	EMU_EM4Init_TypeDef em4Init = EMU_EM4INIT_DEFAULT;

	em4Init.lockConfig = true;		/* Lock regulator, oscillator and BOD configuration.
									* This needs to be set when using the
									* voltage regulator in EM4 */
	em4Init.osc = emuEM4Osc_ULFRCO;	/* Select ULFRCO */
	em4Init.buRtcWakeup = true;		/* BURTC compare or overflow will generate reset */
	em4Init.vreg = true;			/* Enable voltage regulator. Needed for BURTC */
	EMU_EM4Init( &em4Init );
#endif
}

uint32_t configSleepClock(uint32_t ms) {
#if USE_CRYOTIMER
	return setupCryotimerTimeout(ms);
#else
	BURTC_Init_TypeDef burtcInit = BURTC_INIT_DEFAULT;

	RMU_ResetControl(rmuResetBU, rmuResetModeClear);

	burtcInit.mode = burtcModeEM4;			/* BURTC is enabled in EM0-EM4 */
	burtcInit.clkSel = burtcClkSelULFRCO;	/* Select ULFRCO as clock source */
	burtcInit.clkDiv = burtcClkDiv_128;

	BURTC_CompareSet(0, ms);     		/* Set top value for comparator */
	BURTC_IntClear( BURTC_IF_COMP0 );
	BURTC_IntEnable( BURTC_IF_COMP0 );	/* Enable compare interrupt flag */
	BURTC_Init(&burtcInit);
	return ms;
#endif
}

void configGPIO(void) {
#ifdef MODDEF_SLEEP_WAKEUP_PORT
#if MIGHTY_GECKO || BLUE_GECKO || THUNDERBOARD2
	GPIO_PinModeSet(MODDEF_SLEEP_WAKEUP_PORT, MODDEF_SLEEP_WAKEUP_PIN, gpioModeInputPullFilter, 1);
	while (!GPIO_PinInGet(MODDEF_SLEEP_WAKEUP_PORT, MODDEF_SLEEP_WAKEUP_PIN))
		geckoDelayLoop(1);		//debounce
//	GPIO_IntClear(0x0080);
	GPIO_IntClear(_GPIO_IFC_EM4WU_MASK | _GPIO_IFC_EXT_MASK);
	if (MODDEF_SLEEP_WAKEUP_PIN & 0x1)
		NVIC_EnableIRQ(GPIO_ODD_IRQn);
	else
		NVIC_EnableIRQ(GPIO_EVEN_IRQn);
	
	GPIO_IntConfig(MODDEF_SLEEP_WAKEUP_PORT, MODDEF_SLEEP_WAKEUP_PIN, true, false, true);
	GPIO_EM4EnablePinWakeup(MODDEF_SLEEP_WAKEUP_REGISTER, MODDEF_SLEEP_WAKEUP_LEVEL);

#elif GIANT_GECKO
	GPIO_PinModeSet(MODDEF_SLEEP_WAKEUP_PORT, MODDEF_SLEEP_WAKEUP_PIN, gpioModeInputPull, 1);
	GPIO_EM4EnablePinWakeup(MODDEF_SLEEP_WAKEUP_REGISTER, MODDEF_SLEEP_WAKEUP_LEVEL);

#else
	#error need wakeup pin code for new gecko platform
#endif
#endif
}

void modRadioSleep() __attribute__ ((weak));
void modRadioSleep()
{
}

void geckoSleepEM4(uint32_t ms) {
    modRadioSleep();
//	geckoSleepSensors();
//    CMU_HFRCOBandSet(cmuHFRCOFreq_1M0Hz);
	configEM4();
	configSleepClock(ms);
	geckoStoreGpioRetention();
	configGPIO();

	geckoDisableSysTick();
#if USE_CRYOTIMER
	EMU_EnterEM4H();
#else
	BURTC_Enable(true);
	EMU_EnterEM4();
#endif
}

void geckoSleepEM4UntilButton() {
#if MIGHTY_GECKO
	CRYOTIMER->EM4WUEN = 0;
	modRadioSleep();
	configEM4();
	configGPIO();
	EMU_EnterEM4H();
#endif
}


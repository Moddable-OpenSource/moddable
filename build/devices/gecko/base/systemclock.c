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

#include "mc.defines.h"

#include "em_cmu.h"
#include "em_rtcc.h"
#include "em_cryotimer.h"


#if useRTCC
	#include "em_rtc.h"
#endif

extern uint32_t msTickCount;

void gecko_delay(uint32_t ms);
void radioSleep();
void setupRTCwakeup(uint32_t ms);

extern uint32_t gResetCause;
uint32_t gGeckoSetDelayMode = 0;

uint32_t geckoGetResetCause() {
	return gResetCause;
}


/*  /MODDABLE */

#if useRTCC
int findCRYOPeriod(uint32_t ms) {
	int i;
	for (i=0; i<33; i++) {
		if (ms < (1 << i))
			return i-1;
	}
	return 32;
}

void CRYOTIMER_IRQHandler() {
	CRYOTIMER_IntClear(CRYOTIMER_IntGet());
}

void setCRYOTIMER_Timeout(uint32_t delay) {
	setupRTCwakeup(delay);
}
#endif

#if 1 // GO_TO_EM3

/******************************************************************************
 * @brief  Sets up the RTC
 *
 *****************************************************************************/
void setupRTCTimeout(uint32_t delay)
{
    CRYOTIMER_Init_TypeDef init = CRYOTIMER_INIT_DEFAULT;
    CMU_ClockEnable(cmuClock_CRYOTIMER, true);
    CRYOTIMER_IntClear(CRYOTIMER_IF_PERIOD);
    init.enable = true;
    init.osc = cryotimerOscULFRCO;
    init.presc = cryotimerPresc_1;
    init.period = findCRYOPeriod(delay);
    init.em4Wakeup = true;
    CRYOTIMER_Init(&init);

    CRYOTIMER_IntClear(CRYOTIMER_IF_PERIOD);
    CRYOTIMER_IntEnable(CRYOTIMER_IEN_PERIOD);
    NVIC_ClearPendingIRQ(CRYOTIMER_IRQn);
    NVIC_EnableIRQ(CRYOTIMER_IRQn);
}

void startRTCC(void)
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

void setupRTCwakeup(uint32_t ms) {
	uint32_t rtcDelay = ms;
	setupRTCTimeout(rtcDelay);
}
#endif

uint32_t gecko_milliseconds(void)
{
	return msTickCount;
}

void geckoDisableSysTick();
void geckoEnableSysTick();
extern bool radioStarted;
void gecko_delay(uint32_t ms)
{
    uint32_t cryo;
    setupRTCwakeup(ms);
    cryo = CRYOTIMER->CNT;

//    geckoDisableSysTick();
    if (radioStarted)
    	geckoEnterEM1();
    else
    	geckoEnterEM3();
    NVIC_DisableIRQ(CRYOTIMER_IRQn);
//    geckoEnableSysTick();
    msTickCount += (CRYOTIMER->CNT - cryo);
    CMU_ClockEnable(cmuClock_CRYOTIMER, false);		// this will null out CRYOTIMER->CNT
}




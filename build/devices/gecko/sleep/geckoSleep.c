#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_burtc.h"
#include "em_rmu.h"
#include "em_gpio.h"

#include "mc.defines.h"

#if useRTCC
#define xCRYOTIMER 1
#include "em_cryotimer.h"
#include "em_rtcc.h"
#include "em_bus.h"
#endif

uint32_t geckoGetPersistentValue(uint32_t reg) {
#if useRTCC
	return RTCC->RET[reg].REG;
#else
	return BURTC->RET[reg].REG;
#endif
}

void geckoSetPersistentValue(uint32_t reg, uint32_t val) {
#if useRTCC
	RTCC->RET[reg].REG = val;
#else
	BURTC->RET[reg].REG = val;
#endif
}

void configEM4(void) {
#if useRTCC
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
init_EM4.vScaleEM4HVoltage = emuVScaleEM4H_LowPower;
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

void configSleepClock(uint32_t ms) {
#if useRTCC
	setCRYOTIMER_Timeout(ms);
#else
	BURTC_Init_TypeDef burtcInit = BURTC_INIT_DEFAULT;

	burtcInit.mode = burtcModeEM4;			/* BURTC is enabled in EM0-EM4 */
	burtcInit.clkSel = burtcClkSelULFRCO;	/* Select ULFRCO as clock source */
//	burtcInit.clkDiv = burtcClkDiv_2;	/* Choose 1kHz ULFRCO clock frequency */
	burtcInit.clkDiv = burtcClkDiv_128;

	BURTC_CompareSet(0, ms);     		/* Set top value for comparator */
	BURTC_IntClear( BURTC_IF_COMP0 );
	BURTC_IntEnable( BURTC_IF_COMP0 );	/* Enable compare interrupt flag */
	BURTC_Init(&burtcInit);
#endif
}

void configGPIO(void) {
//#if EFR32MG12P332F1024GL125
#if MODDEF_MIGHTY
	GPIO_PinModeSet(MODDEF_SLEEP_WAKEUP_PORT, MODDEF_SLEEP_WAKEUP_PIN, gpioModeInputPullFilter, 1);
	while (!GPIO_PinInGet(MODDEF_SLEEP_WAKEUP_PORT, MODDEF_SLEEP_WAKEUP_PIN))
		UTIL_delay(1);		//debounce
//	GPIO_IntClear(0x0080);
	GPIO_IntClear(_GPIO_IFC_EM4WU_MASK | _GPIO_IFC_EXT_MASK);
	NVIC_EnableIRQ(GPIO_ODD_IRQn);
	GPIO_IntConfig(MODDEF_SLEEP_WAKEUP_PORT, MODDEF_SLEEP_WAKEUP_PIN, true, false, true);
	GPIO_EM4EnablePinWakeup(GPIO_EXTILEVEL_EM4WU1, 0);
#else
	GPIO_PinModeSet(MODDEF_SLEEP_WAKEUP_PORT, MODDEF_SLEEP_WAKEUP_PIN, gpioModeInputPull, 1);
	GPIO_EM4EnablePinWakeup(GPIO_EM4WUEN_EM4WUEN_F2, 0);
#endif
}

void radioSleep();
void geckoSleepEM4(uint32_t ms) {
	geckoDisableSysTick();
    radioSleep();
//	geckoSleepSensors();
//    CMU_HFRCOBandSet(cmuHFRCOFreq_1M0Hz);
	configEM4();
	configSleepClock(ms);
	configGPIO();
#if useRTCC
	EMU_EnterEM4H();
#else
	BURTC_Enable(true);
	EMU_EnterEM4();
#endif
}

void geckoSleepEM4UntilButton() {
//#if EFR32MG12P332F1024GL125
#if MODDEF_MIGHTY
	CRYOTIMER->EM4WUEN = 0;
	radioSleep();
	configEM4();
	configGPIO();
	EMU_EnterEM4H();
#endif
}

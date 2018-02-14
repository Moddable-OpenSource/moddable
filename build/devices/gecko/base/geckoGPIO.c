#include "xsPlatform.h"

#include "em_cmu.h"

#include "gpiointerrupt/inc/gpiointerrupt.h"

void geckoSetupGPIO() {
	CMU_ClockEnable(cmuClock_GPIO, true);
	GPIOINT_Init();
}


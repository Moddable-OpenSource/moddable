/*
 * Copyright (c) 2017-2018  Moddable Tech, Inc.
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

#include "xsPlatform.h"

#include "em_cmu.h"
#include "em_cmu.h"
#include "em_gpio.h"

#include "gpiointerrupt/inc/gpiointerrupt.h"

void geckoStoreGpioRetention(void) __attribute__ ((weak));
void geckoStoreGpioRetention()
{
}

void geckoRestoreGpioRetention(void) __attribute__ ((weak));
void geckoRestoreGpioRetention()
{
}

void geckoUnlatchPinRetention() {
#if MODDEF_SLEEP_RETENTION_GPIO
#if defined(_EMU_EM4CTRL_EM4IORETMODE_MASK)

    /** Retention through EM4 and wakeup: call EMU_UnlatchPinRetention() to unlatch */
    EMU_UnlatchPinRetention();
#endif
#endif
}

void geckoSetupGPIO() {
	CMU_ClockEnable(cmuClock_HFPER, true);
	CMU_ClockEnable(cmuClock_GPIO, true);
	GPIOINT_Init();
}


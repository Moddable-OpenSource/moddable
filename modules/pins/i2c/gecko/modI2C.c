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

#include "modI2C.h"
#include "xsgecko.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_core.h"

#define USE_I2C_SLEEP 1

#if !defined(MODDEF_I2C_PORT)
	#define MODDEF_I2C_PORT 0
#endif

#if MODDEF_I2C_PORT == 0
	#define I2C_PORT	I2C0
	#define I2C_CLOCK	cmuClock_I2C0
	#define I2C_IRQ		I2C0_IRQn
#elif MODDEF_I2C_PORT == 1
	#define I2C_PORT	I2C1
	#define I2C_CLOCK	cmuClock_I2C1
	#define I2C_IRQ		I2C1_IRQn
#else
	#error bad I2C port
#endif

// N.B. Cannot save pointer to modI2CConfiguration as it is allowed to move (stored in relocatable block)

static void modI2CActivate(modI2CConfiguration config);

static modI2CConfigurationRecord gI2CConfig;

void modI2CInit(modI2CConfiguration config)
{
	I2C_Init_TypeDef i2cInit = {
		true,			// enable
		true,			// master
		0,				// refFreq - use current ref clock
		I2C_FREQ_STANDARD_MAX,		// freq
		i2cClockHLRStandard
	};

	gI2CConfig.hz = 1;		// garbage value to ensure mismatch

	CMU_ClockEnable(cmuClock_HFPER, true);
	CMU_ClockEnable(I2C_CLOCK, true);

    GPIO_PinModeSet(MODDEF_I2C_SCL_PORT, MODDEF_I2C_SCL_PIN, gpioModeWiredAndPullUp, 1);
    GPIO_PinModeSet(MODDEF_I2C_SDA_PORT, MODDEF_I2C_SDA_PIN, gpioModeWiredAndPullUp, 1);

#if 0
	// clock out 9 pulses to set slave in a defined state
	{
	int i;
	for (i=0; i<9; i++) {
		GPIO_PinOutSet(MODDEF_I2C_SCL_PORT, MODDEF_I2C_SCL_PIN);
		GPIO_PinOutClear(MODDEF_I2C_SCL_PORT, MODDEF_I2C_SCL_PIN);
	}
	}
#endif

	i2cInit.freq = config->hz;

	I2C_Init(I2C_PORT, &i2cInit);

	I2C_PORT->ROUTEPEN = I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN;
	I2C_PORT->ROUTELOC0 = (MODDEF_I2C_SDA_LOCATION << _I2C_ROUTELOC0_SDALOC_SHIFT)
								 | (MODDEF_I2C_SCL_LOCATION << _I2C_ROUTELOC0_SCLLOC_SHIFT);

#if geckoNeedsI2CRoute
	I2C1->ROUTE = I2C_ROUTE_SDAPEN | I2C_ROUTE_SCLPEN | I2C_ROUTE_LOCATION_LOC1;	// Enable SDA, SDK at location #1
#endif

}

void modI2CUninit(modI2CConfiguration config)
{
    GPIO_PinModeSet(MODDEF_I2C_SDA_PORT, MODDEF_I2C_SDA_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(MODDEF_I2C_SCL_PORT, MODDEF_I2C_SCL_PIN, gpioModeDisabled, 0);
	I2C_Enable(I2C_PORT, false);
}

#define I2CSPM_TRANSFER_TIMEOUT 300000
static volatile I2C_TransferReturn_TypeDef I2C_transferStatus;

#if USE_I2C_SLEEP

#if MODDEF_I2C_PORT == 0
void I2C0_IRQHandler(void) {
	I2C_transferStatus = I2C_Transfer(I2C_PORT);
}
#elif MODDEF_I2C_PORT == 1
void I2C1_IRQHandler(void) {
	I2C_transferStatus = I2C_Transfer(I2C_PORT);
}
#endif

static void doTransfer(I2C_TransferSeq_TypeDef *tcb) {
	bool done = false;
	uint32_t timeout = I2CSPM_TRANSFER_TIMEOUT;
	CORE_irqState_t irqState;

	NVIC_EnableIRQ(I2C_IRQ);
	I2C_transferStatus = I2C_TransferInit(I2C_PORT, tcb);
	while (!done)  {
		irqState = CORE_EnterAtomic();
		if (I2C_transferStatus == i2cTransferInProgress && timeout--) {
//			EMU_EnterEM1();
			geckoEnterEM1();
		}
		else {
			done = true;
		}
		CORE_ExitAtomic(irqState);
	}
	NVIC_DisableIRQ(I2C_IRQ);
}

#endif

uint8_t modI2CWriteWrite(modI2CConfiguration config, uint8_t *w1Buffer, uint16_t w1Length, uint8_t *w2Buffer, uint16_t w2Length)
{
	uint32_t timeout = I2CSPM_TRANSFER_TIMEOUT;

	config->tcb.addr = config->address << 1;
	config->tcb.flags = I2C_FLAG_WRITE_WRITE;
	config->tcb.buf[0].data = w1Buffer;
	config->tcb.buf[0].len = w1Length;
	config->tcb.buf[1].data = w2Buffer;
	config->tcb.buf[1].len = w2Length;

#if USE_I2C_SLEEP
	doTransfer(&config->tcb);
#else
	I2C_transferStatus = I2C_TransferInit(I2C_PORT, &config->tcb);
	while (I2C_transferStatus == i2cTransferInProgress && timeout--) {
		I2C_transferStatus = I2C_Transfer(I2C_PORT);
	}
#endif

	return I2C_transferStatus;
}

uint8_t modI2CWriteRead(modI2CConfiguration config, uint8_t *wBuffer, uint16_t wLength, uint8_t *rBuffer, uint16_t rLength)
{
	uint32_t timeout = I2CSPM_TRANSFER_TIMEOUT;

	config->tcb.addr = config->address << 1;
	config->tcb.flags = I2C_FLAG_WRITE_READ;
	config->tcb.buf[0].data = wBuffer;
	config->tcb.buf[0].len = wLength;
	config->tcb.buf[1].data = rBuffer;
	config->tcb.buf[1].len = rLength;

#if USE_I2C_SLEEP
	doTransfer(&config->tcb);
#else
	I2C_transferStatus = I2C_TransferInit(I2C_PORT, &config->tcb);
	while (I2C_transferStatus == i2cTransferInProgress && timeout--) {
		I2C_transferStatus = I2C_Transfer(I2C_PORT);
	}
#endif

	return I2C_transferStatus;
}

uint8_t modI2CRead(modI2CConfiguration config, uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	uint32_t timeout = I2CSPM_TRANSFER_TIMEOUT;

	config->tcb.addr = config->address << 1;
	config->tcb.flags = I2C_FLAG_READ;
	config->tcb.buf[0].data = buffer;
	config->tcb.buf[0].len = length;
	config->tcb.buf[1].data = 0;
	config->tcb.buf[1].len = 0;

#if USE_I2C_SLEEP
	doTransfer(&config->tcb);
#else
	I2C_transferStatus = I2C_TransferInit(I2C_PORT, &config->tcb);
	while (I2C_transferStatus == i2cTransferInProgress && timeout--) {
		I2C_transferStatus = I2C_Transfer(I2C_PORT);
	}
#endif
	return I2C_transferStatus;
}


uint8_t modI2CWrite(modI2CConfiguration config, const uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	uint32_t timeout = I2CSPM_TRANSFER_TIMEOUT;

	config->tcb.addr = config->address << 1;
	config->tcb.flags = I2C_FLAG_WRITE;
	config->tcb.buf[0].data = buffer;
	config->tcb.buf[0].len = length;
	config->tcb.buf[1].data = 0;
	config->tcb.buf[1].len = 0;

#if USE_I2C_SLEEP
	doTransfer(&config->tcb);
#else
	I2C_transferStatus = I2C_TransferInit(I2C_PORT, &config->tcb);
	while (I2C_transferStatus == i2cTransferInProgress && timeout--) {
		I2C_transferStatus = I2C_Transfer(I2C_PORT);
	}
#endif
	return I2C_transferStatus;
}

void modI2CActivate(modI2CConfiguration config)
{
	if ((gI2CConfig.sda == config->sda) && (gI2CConfig.scl == config->scl) && (gI2CConfig.hz == config->hz))
		return;

	gI2CConfig = *config;

	if (config->hz)
		I2C_BusFreqSet(I2C_PORT, 0, config->hz, i2cClockHLRStandard);
}

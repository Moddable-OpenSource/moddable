/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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
#include "modI2C.h"
#include "xsPlatform.h"
#include "nrf.h"
#include "boards.h"

#include "queue.h"

#include "nrf_drv_twi.h"
#include "nrf_twi_mngr.h"

// N.B. Cannot save pointer to modI2CConfiguration as it is allowed to move (stored in relocatable block)

static uint8_t modI2CActivate(modI2CConfiguration config);

//static uint32_t gHz;		// non-zero when driver initialized
//static uint16_t gSda;
//static uint16_t gScl;
static modI2CConfiguration gConfig = NULL;

#ifndef MODDEF_I2C_INTERFACE
	#define MODDEF_I2C_INTERFACE 0
#endif

static const nrf_drv_twi_t gTwi = NRF_DRV_TWI_INSTANCE(MODDEF_I2C_INTERFACE);


/* Indicates if operation on TWI has ended. */
static volatile bool m_xfer_done = false;

BaseType_t gTWITask = 0;
QueueHandle_t gTWIQueue = NULL;

#define TWI_ERROR			13
#define TWI_READ_COMPLETE	14
#define TWI_WRITE_COMPLETE	15

#define TWI_QUEUE_LEN	8
#define TWI_QUEUE_ITEM_SIZE	4


void twi_handler(nrf_drv_twi_evt_t const *p_event, void *p_context) {
	uint32_t msg = 0;

	switch (p_event->type) {
		case NRF_DRV_TWI_EVT_DONE:
			if ((p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX) 
				|| (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_TXRX))
				msg = TWI_READ_COMPLETE;
			if ((p_event->xfer_desc.type == NRF_DRV_TWI_XFER_TX)
				|| (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_TXTX))
				msg = TWI_WRITE_COMPLETE;
			break;
		default:
			msg = TWI_ERROR;
			break;
	}
	if (msg)
		xQueueSendFromISR(gTWIQueue, &msg, NULL);
}

void modI2CInit(modI2CConfiguration config)
{
	ret_code_t ret;
	uint32_t	msg;

	if (!gTWIQueue)
		gTWIQueue = xQueueCreate(TWI_QUEUE_LEN, TWI_QUEUE_ITEM_SIZE);

	nrf_drv_twi_config_t twi_config = {
		.scl = MODDEF_I2C_SCL_PIN,
		.sda = MODDEF_I2C_SDA_PIN,
		.frequency = 0,
		.interrupt_priority = _PRIO_APP_HIGH,		// 2 APP_IRQ_PRIORITY_HIGH,
		.clear_bus_init = false
	};


	if (-1 != config->sda)
		twi_config.sda = config->sda;
	if (-1 != config->scl)
		twi_config.scl = config->scl;

	if (0 == config->nrfHz) {
		if (config->hz >= 400000)
			config->nrfHz = NRF_DRV_TWI_FREQ_400K;
		else if (config->hz >= 250000)
			config->nrfHz = NRF_DRV_TWI_FREQ_250K;
		else
			config->nrfHz = NRF_DRV_TWI_FREQ_100K;
	}
	twi_config.frequency = config->nrfHz;

	if (0 == config->timeout)
		config->timeout = portMAX_DELAY;

	ret = nrf_drv_twi_init(&gTwi, &twi_config, twi_handler, NULL);
	if (0 == ret)
		nrf_drv_twi_enable(&gTwi);
	else {
		modLog("I2CErr init");
		modLogInt(ret);
	}

	gConfig = config;

}

uint8_t waitForComplete(uint32_t timeout) {
	uint32_t msg = 0;
	if (xQueueReceive(gTWIQueue, (void*)&msg, timeout)) {
		switch (msg) {
			case TWI_READ_COMPLETE:
			case TWI_WRITE_COMPLETE:
				m_xfer_done = true;
				return 0;
			default:
				m_xfer_done = true;
				return 2;
		}
	}
	return 1;		// error - timeout
}

void modI2CUninit(modI2CConfiguration config)
{
	if (config == gConfig) {
		nrf_drv_twi_disable(&gTwi);
		nrf_drv_twi_uninit(&gTwi);
		gConfig = NULL;
	}
}

uint8_t modI2CRead(modI2CConfiguration config, uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	int ret;

	if (modI2CActivate(config))
		return 1;

	for (ret=0; ret<length; ret++) {
		buffer[ret] = 13+ret;
	}

	while (NRF_ERROR_BUSY == (ret = nrf_drv_twi_rx(&gTwi, config->address, buffer, length)))
		taskYIELD();

	if (0 != ret) {
		modLog("I2CErr rx");
		modLogInt(ret);
	}
	else {
		if (0 != waitForComplete(config->timeout))
			ret = NRF_ERROR_TIMEOUT;
	}

	return ret;
}

uint8_t modI2CWrite(modI2CConfiguration config, const uint8_t *buffer, uint16_t length, uint8_t sendStop)
{
	int ret;

	if (modI2CActivate(config))
		return 1;

	while (NRF_ERROR_BUSY == (ret = nrf_drv_twi_tx(&gTwi, config->address, buffer, length, sendStop ? 0 : 1)))
		taskYIELD();
		
	if (0 != ret) {
		modLog("I2CErr tx");
		modLogInt(ret);
	}
	else
		waitForComplete(config->timeout);

	return ret;
}

uint8_t modI2CActivate(modI2CConfiguration config)
{

//return 0;
	if (gConfig == config)
		return 0;

	if (gConfig) {
		if (gConfig->hz == config->hz || gConfig->sda == config->sda || gConfig->scl == config->scl) {
			return 0;
		}
		modI2CUninit(gConfig);
		gConfig = NULL;
	}

	modI2CInit(config);
	return 0;
}

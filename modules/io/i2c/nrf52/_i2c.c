/*
 * Copyright (c) 2019-2023 Moddable Tech, Inc.
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

/*
	I2C - uing ESP-IDF

	to do:
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "mc.xs.h"			// for xsID_* values
#include "xsHost.h"			// esp platform support

#include "builtinCommon.h"

#include "mc.defines.h"
#include "nrf.h"
#include "boards.h"

#include "queue.h"

#include "nrf_drv_twi.h"
#include "nrf_twi_mngr.h"
#include "nrf_twi_sensor.h"

struct I2CRecord {
	uint32_t					hz;
	nrf_drv_twi_frequency_t		nrfHz;
	uint32_t					data;
	uint32_t					clock;
	uint32_t					timeout;
	uint8_t						address;		// 7-bit
	uint8_t						pullup;
	uint8_t						port;
	xsSlot						obj;
	struct I2CRecord			*next;
};
typedef struct I2CRecord I2CRecord;
typedef struct I2CRecord *I2C;

static I2C gI2C;
static I2C gI2CActive;

static uint8_t i2cActivate(I2C i2c);
static uint8_t usingPins(uint32_t data, uint32_t clock);

#ifndef MODDEF_I2C_INTERFACE
	#define MODDEF_I2C_INTERFACE	0
#endif

static const nrf_drv_twi_t gTwi = NRF_DRV_TWI_INSTANCE(MODDEF_I2C_INTERFACE);
static BaseType_t gTWITask = 0;
static QueueHandle_t gTWIQueue = NULL;

#define TWI_ERROR			13
#define TWI_READ_COMPLETE	14
#define TWI_WRITE_COMPLETE	15

#define TWI_QUEUE_LEN		8
#define TWI_QUEUE_ITEM_SIZE	4

static void twi_handler(nrf_drv_twi_evt_t const *p_event, void *p_context)
{
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

static int waitForComplete(xsMachine *the, uint32_t timeout)
{
	uint32_t msg = 0;
	if (xQueueReceive(gTWIQueue, (void*)&msg, timeout)) {
		switch (msg) {
			case TWI_READ_COMPLETE:
			case TWI_WRITE_COMPLETE:
				return 0;
			default:
				return 2;
		}
	}
	return 1;		// error - timeout
}

void _xs_i2c_constructor(xsMachine *the)
{
	I2C i2c;
	int data, clock, hz, nrfHz, address;
	int timeout = 32000;
//	uint8_t pullup = GPIO_PULLUP_ENABLE;
	uint8_t pullup = 1;
	int port = MODDEF_I2C_INTERFACE;

	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_data))
		xsRangeError("data required");
	if (!xsmcHas(xsArg(0), xsID_clock))
		xsRangeError("clock required");
	if (!xsmcHas(xsArg(0), xsID_address))
		xsRangeError("address required");

	xsmcGet(xsVar(0), xsArg(0), xsID_data);
	data = builtinGetPin(the, &xsVar(0));

	xsmcGet(xsVar(0), xsArg(0), xsID_clock);
	clock = builtinGetPin(the, &xsVar(0));

	if (usingPins(data, clock))
		;
	else if (!builtinIsPinFree(data) || !builtinIsPinFree(clock))
		xsRangeError("inUse");

	nrf_drv_twi_config_t twi_config = {
		.scl = clock,
		.sda = data,
		.frequency = 0,
		.interrupt_priority = APP_IRQ_PRIORITY_HIGH,
		.clear_bus_init = false
	};

	xsmcGet(xsVar(0), xsArg(0), xsID_address);
	address = builtinGetPin(the, &xsVar(0));
	if ((address < 0) || (address > 127))
		xsRangeError("invalid address");

	for (i2c = gI2C; i2c; i2c = i2c->next) {
		if ((i2c->data == data) && (i2c->clock == clock) && (i2c->address == address))
			xsRangeError("duplicate address");
	}

	xsmcGet(xsVar(0), xsArg(0), xsID_hz);
	hz = xsmcToInteger(xsVar(0));
	if ((hz < 0) || (hz > 20000000))
		xsRangeError("invalid hz");

	if (hz >= 400000)
		nrfHz = NRF_DRV_TWI_FREQ_400K;
	else if (hz >= 250000)
		nrfHz = NRF_DRV_TWI_FREQ_250K;
	else
		nrfHz = NRF_DRV_TWI_FREQ_100K;

	twi_config.frequency = nrfHz;

	if (xsmcHas(xsArg(0), xsID_timeout)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_timeout);
		timeout = xsmcToInteger(xsVar(0)) * (80000000 / 1000);
	}
	if (0 == timeout)
		timeout = portMAX_DELAY;

	if (xsmcHas(xsArg(0), xsID_pullup)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_pullup);
		pullup = xsmcTest(xsVar(0)) ? 1 : 0;
	}

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = xsmcToInteger(xsVar(0));
		if (port != MODDEF_I2C_INTERFACE)
			xsRangeError("invalid port");
	}

	builtinInitializeTarget(the);
	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("invalid format");

	i2c = c_malloc(sizeof(I2CRecord));
	if (!i2c)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, i2c);
	i2c->obj = xsThis;
	xsRemember(i2c->obj);
	i2c->clock = clock;
	i2c->data = data;
	i2c->hz = hz;
	i2c->nrfHz = hz;
	i2c->address = address;
	i2c->timeout = timeout;
	i2c->pullup = pullup;
	i2c->port = (uint8_t)port;

	i2c->next = gI2C;
	gI2C = i2c;

	builtinUsePin(data);
	builtinUsePin(clock);

	if (0 == nrf_drv_twi_init(&gTwi, &twi_config, twi_handler, NULL))
		nrf_drv_twi_enable(&gTwi);
	else {
		modLog("I2CErr init");
	}

	gI2CActive = i2c;

	if (!gTWIQueue)
		gTWIQueue = xQueueCreate(TWI_QUEUE_LEN, TWI_QUEUE_ITEM_SIZE);
}

void _xs_i2c_destructor(void *data)
{
	I2C i2c = data;
	if (!i2c)
		return;

	if (i2c == gI2CActive) {
		gI2CActive = NULL;
		nrf_drv_twi_disable(&gTwi);
		nrf_drv_twi_uninit(&gTwi);
	}

	if (gI2C == i2c)
		gI2C = i2c->next;
	else {
		I2C walker;
		for (walker = gI2C; walker; walker = walker->next) {
			if (walker->next == i2c) {
				walker->next = i2c->next;
				break;
			}
		}
	}

	if (!usingPins(i2c->data, i2c->clock)) {
		builtinFreePin(i2c->data);
		builtinFreePin(i2c->clock);
	}

	c_free(i2c);
}

void _xs_i2c_close(xsMachine *the)
{
	I2C i2c = xsmcGetHostData(xsThis);
	if (!i2c) return;
	xsForget(i2c->obj);
	_xs_i2c_destructor(i2c);
	xsmcSetHostData(xsThis, NULL);
}

void _xs_i2c_read(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor);
	xsUnsignedValue length;
	int err;
	uint8_t stop = true;
	void *buffer;

	if (xsmcArgc > 1)
		stop = xsmcToBoolean(xsArg(1));;

	if (xsReferenceType == xsmcTypeOf(xsArg(0))) {
		xsmcGetBufferWritable(xsArg(0), &buffer, &length);
		xsmcSetInteger(xsResult, length);
	}
	else {
		length = xsmcToInteger(xsArg(0));
		buffer = xsmcSetArrayBuffer(xsResult, NULL, length);
	}

	if (!i2cActivate(i2c))
		xsUnknownError("activate failed");

	while (NRF_ERROR_BUSY == (err = nrf_drv_twi_rx(&gTwi, i2c->address, buffer, length)))
		taskYIELD();

	if (0 != err) {
		modLog("I2CErr rx");
	}
	else {
		if (0 != waitForComplete(the, i2c->timeout)) {
			modLog("I2C rx timeout");
		}
	}

}

void _xs_i2c_write(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor);
	int err;
	xsUnsignedValue length;
	uint8_t stop = true;
	void *buffer;

	if ((xsmcArgc > 1) && !xsmcTest(xsArg(1)))
		stop = xsmcToBoolean(xsArg(1));

	xsmcGetBufferReadable(xsArg(0), &buffer, &length);

	if (!i2cActivate(i2c))
		xsUnknownError("activate failed");

	while (NRF_ERROR_BUSY == (err = nrf_drv_twi_tx(&gTwi, i2c->address, buffer, length, stop ? 0 : 1)))
		taskYIELD();

	if (0 != err) {
		modLog("I2CErr tx");
	}
	else {
		if (0 != waitForComplete(the, i2c->timeout)) {
			modLog("I2C tx timeout");
		}
	}
}

void _xs_i2c_writeRead(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor);
	int err;
	xsUnsignedValue lengthWrite, lengthRead;
	uint8_t stop = true;
	void *bufferWrite, *bufferRead;

	if (xsmcArgc > 2)
		stop = xsmcToBoolean(xsArg(2));

	if (xsReferenceType == xsmcTypeOf(xsArg(1))) {
		xsResult = xsArg(1);
		xsmcGetBufferWritable(xsResult, &bufferRead, &lengthRead);
		xsmcSetInteger(xsResult, lengthRead);
	}
	else {
		lengthRead = xsmcToInteger(xsArg(1));
		bufferRead = xsmcSetArrayBuffer(xsResult, NULL, lengthRead);
	}

	xsmcGetBufferReadable(xsArg(0), &bufferWrite, &lengthWrite);

	if (!i2cActivate(i2c))
		xsUnknownError("activate failed");

	while (NRF_ERROR_BUSY == (err = nrf_drv_twi_tx(&gTwi, i2c->address, bufferWrite, lengthWrite, stop ? 0 : 1)))
		taskYIELD();

	if (0 != err)
		modLog("I2CErr tx");
	else {
		if (0 != waitForComplete(the, i2c->timeout))
			modLog("I2C txrx tx timeout");
	}

	while (NRF_ERROR_BUSY == (err = nrf_drv_twi_rx(&gTwi, i2c->address, bufferRead, lengthRead)))
		taskYIELD();
	
	if (0 != err)
		modLog("I2CErr rx");
	else {
		if (0 != waitForComplete(the, i2c->timeout))
			modLog("I2C txrx rx timeout");
	}

}

uint8_t i2cActivate(I2C i2c)
{

	if ((i2c == gI2CActive) ||
		(gI2CActive && (gI2CActive->data == i2c->data) && (gI2CActive->clock == i2c->clock) && (gI2CActive->hz == i2c->hz) && (gI2CActive->port == i2c->port) && (gI2CActive->pullup == i2c->pullup)))
		return 1;

	if (gI2CActive) {
		nrf_drv_twi_disable(&gTwi);
		nrf_drv_twi_uninit(&gTwi);
		gI2CActive = NULL;
	}

	nrf_drv_twi_config_t twi_config = {
		.scl = i2c->clock,
		.sda = i2c->data,
		.frequency = i2c->nrfHz,
		.interrupt_priority = APP_IRQ_PRIORITY_HIGH,
		.clear_bus_init = false
	};

int err;
	err = nrf_drv_twi_init(&gTwi, &twi_config, twi_handler, NULL);
	if (0 == err)
		nrf_drv_twi_enable(&gTwi);
	else {
		modLog("twi init failed");
		modLogInt(err);
		return 0;
	}

	gI2CActive = i2c;

	return 1;
}

uint8_t usingPins(uint32_t data, uint32_t clock)
{
	I2C walker;

	for (walker = gI2C; walker; walker = walker->next) {
		if ((walker->data == data) && (walker->clock == clock))
			return 1;
	}

	return 0;
}

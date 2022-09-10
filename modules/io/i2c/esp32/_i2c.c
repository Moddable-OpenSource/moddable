/*
 * Copyright (c) 2019-2022 Moddable Tech, Inc.
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

#include "driver/i2c.h"

#include "builtinCommon.h"

struct I2CRecord {
	uint32_t					hz;
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

void _xs_i2c_constructor(xsMachine *the)
{
	I2C i2c;
	int data, clock, hz, address;
	int timeout = 32000;
	uint8_t pullup = GPIO_PULLUP_ENABLE;
	int port = I2C_NUM_0;

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
	if ((hz <= 0) || (hz > 20000000))
		xsRangeError("invalid hz");

	if (xsmcHas(xsArg(0), xsID_timeout)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_timeout);
		timeout = xsmcToInteger(xsVar(0)) * (80000000 / 1000);
	}

	if (xsmcHas(xsArg(0), xsID_pullup)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_pullup);
		pullup = xsmcToBoolean(xsVar(0)) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
	}

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = xsmcToInteger(xsVar(0));
		if ((port < 0) || (port >= I2C_NUM_MAX))
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
	i2c->address = address;
	i2c->timeout = timeout;
	i2c->pullup = pullup;
	i2c->port = (uint8_t)port;

	i2c->next = gI2C;
	gI2C = i2c;

	builtinUsePin(data);
	builtinUsePin(clock);
}

void _xs_i2c_destructor(void *data)
{
	I2C i2c = data;
	if (!i2c)
		return;

	if (i2c == gI2CActive) {
		gI2CActive = NULL;
		i2c_driver_delete(i2c->port);
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
	if (i2c && xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor)) {
		xsForget(i2c->obj);
		_xs_i2c_destructor(i2c);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void _xs_i2c_read(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor);
	xsUnsignedValue length;
	int err;
	uint8_t stop = true;
	i2c_cmd_handle_t cmd;
	void *buffer;

	if (xsmcArgc > 1)
		stop = xsmcToBoolean(xsArg(1));

	if (xsReferenceType == xsmcTypeOf(xsArg(0))) {
		xsResult = xsArg(0);
		xsmcGetBufferWritable(xsResult, &buffer, &length);
	}
	else {
 		length = xsmcToInteger(xsArg(0));
		buffer = xsmcSetArrayBuffer(xsResult, NULL, length);
	}

	if (!i2cActivate(i2c))
		xsUnknownError("activate failed");

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_READ, 1);
	if (length > 0) {
		if (length > 1)
			i2c_master_read(cmd, buffer, length - 1, I2C_MASTER_ACK);
		i2c_master_read(cmd, ((uint8_t *)buffer) + length - 1, 1, I2C_MASTER_NACK);
	}
	if (stop)
		i2c_master_stop(cmd);
	err = i2c_master_cmd_begin(i2c->port, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ESP_OK != err)
		xsUnknownError("read failed");
}

void _xs_i2c_write(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor);
	int err;
	xsUnsignedValue length;
	uint8_t stop = true;
	i2c_cmd_handle_t cmd;
	void *buffer;

	if (xsmcArgc > 1)
		stop = xsmcToBoolean(xsArg(1));

	xsmcGetBufferReadable(xsArg(0), &buffer, &length);

	if (!i2cActivate(i2c))
		xsUnknownError("activate failed");

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	if (length == 0)
		i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_WRITE, 1);
	else {
		i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_WRITE, 1);
		i2c_master_write(cmd, (uint8_t *)buffer, length, 1);
	}

	if (stop)
		i2c_master_stop(cmd);
	err = i2c_master_cmd_begin(i2c->port, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (length == 0) {
		if (ESP_OK != err)
			xsmcSetInteger(xsResult, 1);
	}
	else if (ESP_OK != err)
		xsUnknownError("write failed");
}

uint8_t i2cActivate(I2C i2c)
{
	i2c_config_t conf;

	if ((i2c == gI2CActive) ||
		(gI2CActive && (gI2CActive->data == i2c->data) && (gI2CActive->clock == i2c->clock) && (gI2CActive->hz == i2c->hz) && (gI2CActive->port == i2c->port) && (gI2CActive->pullup == i2c->pullup)))
		return 1;

	if (gI2CActive) {
		i2c_driver_delete(gI2CActive->port);
		gI2CActive = NULL;
	}

	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = i2c->data;
	conf.scl_io_num = i2c->clock;
	conf.master.clk_speed = i2c->hz;
	conf.sda_pullup_en = i2c->pullup;
	conf.scl_pullup_en = i2c->pullup;
	conf.clk_flags = 0;
	if (ESP_OK != i2c_param_config(i2c->port, &conf))
		return 0;

	if (ESP_OK != i2c_driver_install(i2c->port, I2C_MODE_MASTER, 0, 0, 0))
		return 0;

	gI2CActive = i2c;

	i2c_set_timeout(i2c->port, i2c->timeout);

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

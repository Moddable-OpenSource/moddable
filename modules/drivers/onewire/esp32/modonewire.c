/*
 * Copyright (c) 2024  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 *
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h" // for xsID_ values
#include "mc.defines.h"

#include "modGPIO.h"

#include "onewire_bus.h"
#include "onewire_cmd.h"
#include "onewire_crc.h"
#include "onewire_device.h"

#define MODDEF_ONEWIRE_DRIVER_GPIO 16

typedef struct
{
	xsSlot obj;
	uint8_t pin;
	onewire_bus_handle_t bus;
} modOneWireRecord, *modOneWire;

typedef union
{
	struct fields {
		uint8_t family[1];         ///< family identifier (1 byte, LSB - read/write first)
		uint8_t serial_number[6];  ///< serial number (6 bytes)
		uint8_t crc[1];            ///< CRC check byte (1 byte, MSB - read/write last)
	} fields;                      ///< Provides access via field names

	uint8_t bytes[8];              ///< Provides raw byte access
} OneWireBus_ROMCode;


void xs_onewire_destructor(void *data)
{
	modOneWire onewire = data;
	if (NULL == onewire)
		return;
	onewire_bus_del(onewire->bus);
	c_free(onewire);
}

void xs_onewire(xsMachine *the)
{
	modOneWire onewire;
	int pin;

	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_pin))
		xsUnknownError("pin missing");

	onewire = c_malloc(sizeof(modOneWireRecord));
	if (!onewire)
		xsUnknownError("no memory");

	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	pin = xsmcToInteger(xsVar(0));

	onewire->obj = xsThis;
	onewire->pin = pin;

	xsRemember(onewire->obj);

	onewire_bus_config_t bus_config;
	bus_config.bus_gpio_num = pin;

	onewire_bus_rmt_config_t rmt_config = {
		.max_rx_bytes = 10, // 1 byte ROM command + 8 bytes ROM number + 1 byte device command
	};

	onewire_new_bus_rmt(&bus_config, &rmt_config, &onewire->bus);

	if (onewire->bus == NULL)
		xsUnknownError("can't init pin");

	xsmcSetHostData(xsThis, onewire);
}

void xs_onewire_close(xsMachine *the)
{
	modOneWire onewire = xsmcGetHostData(xsThis);
	xsForget(onewire->obj);
	xs_onewire_destructor(onewire);
	xsmcSetHostData(xsThis, NULL);
}

void copyFlipped(uint8_t *s, uint8_t *d, uint16_t count)
{
	int i;
	for (i=0; i<count; i++)
		d[count-i-1] = s[i];
}

void swap(uint8_t *b, uint16_t count)
{
	int i;
	for (i=0; i<=(count-1)/2; i++) {
		uint8_t t = b[i];
		b[i] = b[count-i-1];
		b[count-i-1] = t;
	}
}

void xs_onewire_read(xsMachine *the)
{
	modOneWire onewire = xsmcGetHostData(xsThis);

	int argc = xsmcArgc;

	if (argc == 0) {	// read and return one byte
		uint8_t value = 0;
		onewire_bus_read_bytes(onewire->bus, &value, 1);
		xsmcSetInteger(xsResult, value);
	}
	else {
		int count = xsmcToInteger(xsArg(0));
		xsmcSetArrayBuffer(xsResult, NULL, count);
		uint8_t *buffer;
		xsUnsignedValue size;

		xsmcGetBufferWritable(xsResult, &buffer, &size);
		onewire_bus_read_bytes(onewire->bus, (uint8_t *)buffer, size);
	}
}

void xs_onewire_write(xsMachine *the)
{
	modOneWire onewire = xsmcGetHostData(xsThis);
	uint8_t value = xsmcToInteger(xsArg(0));
	if ((value < 0) || (value > 255))
		xsRangeError("bad value");
	onewire_bus_write_bytes(onewire->bus, &value, 1);
}

void xs_onewire_select(xsMachine *the)
{
	modOneWire onewire = xsmcGetHostData(xsThis);
	OneWireBus_ROMCode rom_code = {};
	uint8_t *flipped = xsmcToArrayBuffer(xsArg(0));
	esp_err_t err;
	uint16_t size = sizeof(OneWireBus_ROMCode);
	
	uint8_t value = ONEWIRE_CMD_MATCH_ROM;

	copyFlipped(flipped, (uint8_t*)&rom_code.bytes, size);

	err = onewire_bus_reset(onewire->bus);
	if (err == ESP_OK)
		err = onewire_bus_write_bytes(onewire->bus, &value, 1);

	if (err == ESP_OK)
		err = onewire_bus_write_bytes(onewire->bus, (uint8_t*)&rom_code, sizeof(OneWireBus_ROMCode));
}

void xs_onewire_search(xsMachine *the)
{
	modOneWire onewire = xsmcGetHostData(xsThis);

	onewire_device_iter_handle_t iter = {};
	onewire_device_t device = {};
	esp_err_t err;

	xsmcVars(1);
	xsResult = xsNewArray(0);

	err = onewire_new_device_iter(onewire->bus, &iter);

	while (ESP_OK == err) {
    	err = onewire_device_iter_get_next(iter, &device);
		if (ESP_OK == err) {
			swap(&device.address, 8);
			xsmcSetArrayBuffer(xsVar(0), &device.address, 8);
			xsCall1(xsResult, xsID_push, xsVar(0));
		}
	}

	onewire_del_device_iter(iter);
}

void xs_onewire_isPresent(xsMachine *the)
{
	modOneWire onewire = xsmcGetHostData(xsThis);

	onewire_device_iter_handle_t iter = {};
	onewire_device_t device = {};
	esp_err_t err;
	uint8_t *id;

	if (8 != xsmcGetArrayBufferLength(xsArg(0)))
		xsUnknownError("invalid id");

	id = xsmcToArrayBuffer(xsArg(0));

	err = onewire_new_device_iter(onewire->bus, &iter);
	while (ESP_OK == err) {
    	err = onewire_device_iter_get_next(iter, &device);
		if (ESP_OK != err)
			break;
		swap(&device.address, 8);
		if (0 == espMemCmp(id, (uint8_t*)(&device.address), 8)) {
			xsResult = xsTrue;
			return;
		}
	}

	xsResult = xsFalse;
}

void xs_onewire_reset(xsMachine *the)
{
	modOneWire onewire = xsmcGetHostData(xsThis);
	bool present = false;
	if (ESP_OK == onewire_bus_reset(onewire->bus))
		present = true;
	xsmcSetBoolean(xsResult, present);
}

void xs_onewire_crc(xsMachine *the)
{
	uint8_t crc = 0;
	uint8_t *src;
	xsUnsignedValue len;
	int argc = xsmcArgc;

	xsmcGetBufferReadable(xsArg(0), &src, &len);
	if (argc > 1) {
		size_t arg_len = xsmcToInteger(xsArg(1));
		if (arg_len < len)
			len = arg_len;
	}

	crc = onewire_crc8(crc, src, len);
	xsmcSetInteger(xsResult, crc);
}


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

#include "xsmc.h"
#include "mc.xs.h"

typedef struct {
	xsMachine	*the;
	xsSlot		obj;

	uint8_t *advertisingData;
	uint8_t *scanResponseData;
} modBLERecord, *modBLE;

void xs_ble_initialize(xsMachine *the)
{
	modBLE ble;
	ble = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!ble)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, ble);
	ble->the = the;
	ble->obj = xsThis;
}

void xs_ble_close(xsMachine *the)
{
	modBLE ble = xsmcGetHostData(xsThis);
	xs_ble_destructor(ble);
	xsmcSetHostData(xsThis, NULL);
}

void xs_ble_destructor(void *data)
{
	modBLE ble = data;
	if (ble) {
		c_free(ble);
	}
}

void xs_ble_start_advertising(xsMachine *the)
{
	modBLE ble = xsmcGetHostData(xsThis);
	uint32_t intervalMin = xsmcToInteger(xsArg(0));
	uint32_t intervalMax = xsmcToInteger(xsArg(1));
	uint8_t *advertisingData = xsmcToArrayBuffer(xsArg(2));
	uint32_t advertisingDataLength = xsGetArrayBufferLength(xsArg(2));
	uint8_t *scanResponseData = xsmcTest(xsArg(3)) ? xsmcToArrayBuffer(xsArg(3)) : NULL;
	uint32_t scanResponseDataLength = xsmcTest(xsArg(3)) ? xsGetArrayBufferLength(xsArg(3)) : 0;
	
	ble->advertisingData = c_malloc(advertisingDataLength);
	if (!ble->advertisingData)
		xsUnknownError("no memory");
	c_memmove(ble->advertisingData, advertisingData, advertisingDataLength);
	if (scanResponseData) {
		ble->scanResponseData = c_malloc(scanResponseDataLength);
		if (!ble->scanResponseData)
			xsUnknownError("no memory");
		c_memmove(ble->scanResponseData, scanResponseData, scanResponseDataLength);
	}
}

void xs_ble_stop_advertising(xsMachine *the)
{
}


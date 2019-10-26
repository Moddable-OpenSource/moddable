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
#include "xsHost.h"
#include "mc.xs.h"
#include "mc.defines.h"
#include "modBLE.h"

//#include "mc.bleservices.c"

typedef struct modBLEConnectionRecord modBLEConnectionRecord;
typedef modBLEConnectionRecord *modBLEConnection;

struct modBLEConnectionRecord {
	struct modBLEConnectionRecord *next;

	xsMachine *the;
	xsSlot objConnection;
	xsSlot objClient;
};

typedef struct {
	xsMachine	*the;
	xsSlot		obj;

	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;
} modBLERecord, *modBLE;

static modBLE gBLE = NULL;

void xs_ble_client_initialize(xsMachine *the)
{
	if (NULL != gBLE)
		xsUnknownError("BLE already initialized");
	gBLE = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!gBLE)
		xsUnknownError("no memory");
	gBLE->the = the;
	gBLE->obj = xsThis;
	xsRemember(gBLE->obj);
	
	// Initialize platform Bluetooth modules
}

void xs_ble_client_close(xsMachine *the)
{
	xsForget(gBLE->obj);
	xs_ble_client_destructor(gBLE);
}

void xs_ble_client_destructor(void *data)
{
	modBLE ble = data;
	if (ble) {
		c_free(ble);
	}
	gBLE = NULL;
}

void xs_ble_client_set_local_privacy(xsMachine *the)
{
}

void xs_ble_client_start_scanning(xsMachine *the)
{
	uint8_t active = xsmcToBoolean(xsArg(0));
	uint16_t interval = xsmcToInteger(xsArg(1));
	uint16_t window = xsmcToInteger(xsArg(2));
}

void xs_ble_client_stop_scanning(xsMachine *the)
{
}

void xs_ble_client_connect(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t addressType = xsmcToInteger(xsArg(1));
}

void xs_ble_client_set_security_parameters(xsMachine *the)
{
	uint8_t encryption = xsmcToBoolean(xsArg(0));
	uint8_t bonding = xsmcToBoolean(xsArg(1));
	uint8_t mitm = xsmcToBoolean(xsArg(2));
	uint16_t ioCapability = xsmcToInteger(xsArg(3));
	
	gBLE->encryption = encryption;
	gBLE->bonding = bonding;
	gBLE->mitm = mitm;

	modBLESetSecurityParameters(encryption, bonding, mitm, ioCapability);
}

void xs_ble_client_passkey_reply(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t confirm = xsmcToBoolean(xsArg(1));
}

void xs_gap_connection_initialize(xsMachine *the)
{
	uint8_t conn_id;
	xsmcVars(1);	// xsArg(0) is client
	xsmcGet(xsVar(0), xsArg(0), xsID_connection);
	conn_id = xsmcToInteger(xsVar(0));
}
	
void xs_gap_connection_disconnect(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
}

void xs_gap_connection_read_rssi(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
}

void xs_gatt_client_discover_primary_services(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t argc = xsmcArgc;
}

void xs_gatt_service_discover_characteristics(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t start = xsmcToInteger(xsArg(1));
	uint16_t end = xsmcToInteger(xsArg(2));
}

void xs_gatt_characteristic_discover_all_characteristic_descriptors(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
}

void xs_gatt_characteristic_read_value(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
#if 0
	uint16_t auth = 0;
	uint16_t argc = xsmcArgc;
	if (argc > 2)
		auth = xsmcToInteger(xsArg(2));
#endif
}

void xs_gatt_characteristic_enable_notifications(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
}

void xs_gatt_characteristic_disable_notifications(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
}

void xs_gatt_descriptor_read_value(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
#if 0
	uint16_t auth = 0;
	uint16_t argc = xsmcArgc;
	if (argc > 2)
		auth = xsmcToInteger(xsArg(2));
#endif
}

void xs_gatt_descriptor_write_value(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
}

void xs_gatt_characteristic_write_without_response(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
}


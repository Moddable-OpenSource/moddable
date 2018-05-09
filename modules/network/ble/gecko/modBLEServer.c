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
#include "xsgecko.h"
#include "mc.xs.h"

#include "bg_types.h"
#include "native_gecko.h"

#include "mc.bleservices.c"

typedef struct {
	xsMachine *the;
	xsSlot obj;
	xsSlot objConnection;
	xsSlot objClient;

	int8_t id;
	bd_addr bda;
} modBLERecord, *modBLE;

static modBLE gBLE = NULL;

uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MODDEF_BLE_MAX_CONNECTIONS)];

static const gecko_configuration_t config = {
	.config_flags = 0,
	.sleep.flags = SLEEP_FLAGS_DEEP_SLEEP_ENABLE,
	.bluetooth.max_connections = MODDEF_BLE_MAX_CONNECTIONS,
	.bluetooth.heap = bluetooth_stack_heap,
	.bluetooth.sleep_clock_accuracy = 100, // ppm
	.bluetooth.heap_size = sizeof(bluetooth_stack_heap),
	.gattdb = &bg_gattdb_data,
#if (HAL_PA_ENABLE) && defined(FEATURE_PA_HIGH_POWER)
	.pa.config_enable = 1, // Enable high power PA
	.pa.input = GECKO_RADIO_PA_INPUT_VBAT, // Configure PA input to VBAT
#endif
};

static void addressToBuffer(bd_addr *bda, uint8_t *buffer);

void xs_ble_server_initialize(xsMachine *the)
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
	gecko_stack_init(&config);
	gecko_bgapi_class_system_init();
	gecko_bgapi_class_le_gap_init();
	gecko_bgapi_class_le_connection_init();
	gecko_bgapi_class_gatt_server_init();
}

void xs_ble_server_close(xsMachine *the)
{
	xsForget(gBLE->obj);
	xs_ble_server_destructor(gBLE);
}

void xs_ble_server_destructor(void *data)
{
	modBLE ble = data;
	if (ble)
		c_free(ble);
	gBLE = NULL;
}

void xs_ble_server_get_local_address(xsMachine *the)
{
	uint8_t buffer[6];
	struct gecko_msg_system_get_bt_address_rsp_t *rsp;
	rsp = gecko_cmd_system_get_bt_address();
	addressToBuffer(&rsp->address, buffer);
	xsmcSetArrayBuffer(xsResult, (void*)buffer, sizeof(buffer));
}

void xs_ble_server_set_device_name(xsMachine *the)
{
	const char *name = xsmcToString(xsArg(0));
	gecko_cmd_system_set_device_name(0, strlen(name), (uint8_t*)name);
}

void xs_ble_server_set_passkey(xsMachine *the)
{
}

void xs_ble_server_start_advertising(xsMachine *the)
{
	uint16_t intervalMin = xsmcToInteger(xsArg(0));
	uint16_t intervalMax = xsmcToInteger(xsArg(1));
	uint8_t *advertisingData = (uint8_t*)xsmcToArrayBuffer(xsArg(2));
	uint32_t advertisingDataLength = xsGetArrayBufferLength(xsArg(2));
	uint8_t *scanResponseData = xsmcTest(xsArg(3)) ? (uint8_t*)xsmcToArrayBuffer(xsArg(3)) : NULL;
	uint32_t scanResponseDataLength = xsmcTest(xsArg(3)) ? xsGetArrayBufferLength(xsArg(3)) : 0;
	
	gecko_cmd_le_gap_set_advertise_timing(0, intervalMin, intervalMax, 0, 0);
	gecko_cmd_le_gap_set_adv_data(0, advertisingDataLength, advertisingData);
	if (scanResponseData)
		gecko_cmd_le_gap_set_adv_data(1, scanResponseDataLength, scanResponseData);	
	gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);
}
	
void xs_ble_server_stop_advertising(xsMachine *the)
{
	gecko_cmd_le_gap_set_mode(le_gap_non_discoverable, le_gap_non_connectable);
}

void xs_ble_server_characteristic_notify_value(xsMachine *the)
{
}

void xs_ble_server_deploy(xsMachine *the)
{
	// server deployed automatically by gecko_stack_init()
}

static void systemBootEvent(struct gecko_msg_system_boot_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
	xsEndHost(gBLE->the);
}

static void addressToBuffer(bd_addr *bda, uint8_t *buffer)
{
	for (uint8_t i = 0; i < 6; ++i)
		buffer[i] = bda->addr[5 - i];
}

void ble_event_handler(struct gecko_cmd_packet* evt)
{
	switch(BGLIB_MSG_ID(evt->header)) {
		case gecko_evt_system_boot_id:
			systemBootEvent(&evt->data.evt_system_boot);
			break;
		default:
			break;
	}
}

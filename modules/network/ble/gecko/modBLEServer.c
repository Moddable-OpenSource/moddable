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
#include "modBLE.h"
#include "modTimer.h"

#include "bg_types.h"
#include "native_gecko.h"

#include "mc.bleservices.c"

typedef struct {
	xsMachine *the;
	xsSlot obj;

	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;

	modTimer timer;
	int8_t connection;
	bd_addr address;
	uint8_t bond;
} modBLERecord, *modBLE;

static modBLE gBLE = NULL;

#if MODDEF_BLE_MAX_CONNECTIONS != 1
	#error - only one ble client connection supported
#endif

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
static void uuidToBuffer(uint8array *uuid, uint8_t *buffer, uint16_t *length);
static void bleTimerCallback(modTimer timer, void *refcon, int refconSize);
static void ble_event_handler(struct gecko_cmd_packet* evt);

void xs_ble_server_initialize(xsMachine *the)
{
	if (NULL != gBLE)
		xsUnknownError("BLE already initialized");
	gBLE = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!gBLE)
		xsUnknownError("no memory");
	gBLE->connection = -1;
	gBLE->bond = 0xFF;
	gBLE->the = the;
	gBLE->obj = xsThis;
	xsRemember(gBLE->obj);
	
	// Initialize platform Bluetooth modules
	gecko_stack_init(&config);
	gecko_bgapi_class_system_init();
	gecko_bgapi_class_le_gap_init();
	gecko_bgapi_class_le_connection_init();
	gecko_bgapi_class_gatt_server_init();
	gecko_bgapi_class_sm_init();

	gBLE->timer = modTimerAdd(0, 20, bleTimerCallback, NULL, 0);
}

void xs_ble_server_close(xsMachine *the)
{
	xsForget(gBLE->obj);
	xs_ble_server_destructor(gBLE);
}

void xs_ble_server_destructor(void *data)
{
	modBLE ble = data;
	if (ble) {
		modTimerRemove(ble->timer);
		c_free(ble);
	}
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

void xs_ble_server_start_advertising(xsMachine *the)
{
	uint16_t intervalMin = xsmcToInteger(xsArg(0));
	uint16_t intervalMax = xsmcToInteger(xsArg(1));
	uint8_t *advertisingData = (uint8_t*)xsmcToArrayBuffer(xsArg(2));
	uint32_t advertisingDataLength = xsGetArrayBufferLength(xsArg(2));
	uint8_t *scanResponseData = xsmcTest(xsArg(3)) ? (uint8_t*)xsmcToArrayBuffer(xsArg(3)) : NULL;
	uint32_t scanResponseDataLength = xsmcTest(xsArg(3)) ? xsGetArrayBufferLength(xsArg(3)) : 0;
	uint8_t scan_rsp = scanResponseData ? 0 : 1;
	
	gecko_cmd_le_gap_set_advertise_timing(0, intervalMin, intervalMax, 0, 0);
	gecko_cmd_le_gap_set_adv_data(scan_rsp, advertisingDataLength, advertisingData);
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
	uint16_t handle = xsmcToInteger(xsArg(0));
	//uint16_t notify = xsmcToInteger(xsArg(1));
	gecko_cmd_gatt_server_send_characteristic_notification(gBLE->connection, handle, xsGetArrayBufferLength(xsArg(2)), xsmcToArrayBuffer(xsArg(2)));
}

void xs_ble_server_deploy(xsMachine *the)
{
	// server deployed automatically by gecko_stack_init()
}

void setSecurityParameters(uint8_t encryption, uint8_t bonding, uint8_t mitm)
{
	gBLE->encryption = encryption;
	gBLE->bonding = bonding;
	gBLE->mitm = mitm;
	if (bonding || (encryption && mitm))
		gecko_cmd_sm_set_bondable_mode(1);
}

void addressToBuffer(bd_addr *bda, uint8_t *buffer)
{
	for (uint8_t i = 0; i < 6; ++i)
		buffer[i] = bda->addr[5 - i];
}

void uuidToBuffer(uint8array *uuid, uint8_t *buffer, uint16_t *length)
{
	uint16_t len = uuid->len;
	for (uint8_t i = 0; i < len; ++i)
		buffer[i] = uuid->data[len - 1 - i];
	*length = len;
}

static const char_name_table *handleToCharName(uint16_t handle) {
	for (uint16_t i = 0; i < char_name_count; ++i) {
		if (char_names[i].handle == handle)
			return &char_names[i];
	}
	return NULL;
}

void bleTimerCallback(modTimer timer, void *refcon, int refconSize)
{
    struct gecko_cmd_packet* evt = gecko_peek_event();
    ble_event_handler(evt);
}

static void systemBootEvent(struct gecko_msg_system_boot_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
	xsEndHost(gBLE->the);
}

static void leConnectionOpenedEvent(struct gecko_msg_le_connection_opened_evt_t *evt)
{
	xsBeginHost(gBLE->the);
		
	// Ignore duplicate connection events
	if (-1 != gBLE->connection) {
		goto bail;
	}
	
	gBLE->connection = evt->connection;
	gBLE->address = evt->address;
	gBLE->bond = evt->bonding;
	
	if (gBLE->encryption || gBLE->mitm)
		gecko_cmd_sm_increase_security(evt->connection);

	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), evt->connection);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(2), evt->address.addr, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

static void leConnectionClosedEvent(struct gecko_msg_le_connection_closed_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	if (evt->connection != gBLE->connection)
		goto bail;
	gBLE->connection = -1;
	gBLE->bond = 0xFF;
	xsCall1(gBLE->obj, xsID_callback, xsString("onDisconnected"));
bail:
	xsEndHost(gBLE->the);
}

static void leConnectionParametersEvent(struct gecko_msg_le_connection_parameters_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	if (evt->security_mode > le_connection_mode1_level1) {
		xsCall1(gBLE->obj, xsID_callback, xsString("onAuthenticated"));
	}
	xsEndHost(gBLE->the);
}

static void gattServerCharacteristicStatus(struct gecko_msg_gatt_server_characteristic_status_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	if (evt->connection != gBLE->connection || gatt_server_client_config != evt->status_flags)
		goto bail;
	struct gecko_msg_gatt_server_read_attribute_type_rsp_t *uuid = gecko_cmd_gatt_server_read_attribute_type(evt->characteristic);
	if (0 != uuid->result)
		goto bail;
	char_name_table *char_name = (char_name_table *)handleToCharName(evt->characteristic);
	xsmcVars(6);
	xsVar(0) = xsmcNewObject();
	uint16_t buffer_length;
	uint8_t buffer[16];
	uuidToBuffer(&uuid->type, buffer, &buffer_length);
	xsmcSetArrayBuffer(xsVar(1), buffer, buffer_length);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	if (char_name) {
		xsmcSetString(xsVar(2), (char*)char_name->name);
		xsmcSet(xsVar(0), xsID_name, xsVar(2));
		xsmcSetString(xsVar(3), (char*)char_name->type);
		xsmcSet(xsVar(0), xsID_type, xsVar(3));
	}
	xsmcSetInteger(xsVar(4), evt->characteristic);
	xsmcSetInteger(xsVar(5), evt->client_config_flags);
	xsmcSet(xsVar(0), xsID_handle, xsVar(4));
	xsmcSet(xsVar(0), xsID_notify, xsVar(5));
	xsCall2(gBLE->obj, xsID_callback, xsString(gatt_disable == evt->client_config_flags ? "onCharacteristicNotifyDisabled" : "onCharacteristicNotifyEnabled"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

static void doReadOrWriteRequest(struct gecko_msg_gatt_server_user_write_request_evt_t *evt, uint8_t write)
{
	xsBeginHost(gBLE->the);
	if (evt->connection != gBLE->connection)
		goto bail;
	struct gecko_msg_gatt_server_read_attribute_type_rsp_t *uuid = gecko_cmd_gatt_server_read_attribute_type(evt->characteristic);
	if (0 != uuid->result)
		goto bail;
	char_name_table *char_name = (char_name_table *)handleToCharName(evt->characteristic);
	xsmcVars(4);
	xsVar(0) = xsmcNewObject();
	uint16_t buffer_length;
	uint8_t buffer[16];
	uuidToBuffer(&uuid->type, buffer, &buffer_length);
	xsmcSetArrayBuffer(xsVar(1), buffer, buffer_length);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	if (char_name) {
		xsmcSetString(xsVar(2), (char*)char_name->name);
		xsmcSet(xsVar(0), xsID_name, xsVar(2));
		xsmcSetString(xsVar(3), (char*)char_name->type);
		xsmcSet(xsVar(0), xsID_type, xsVar(3));
	}
	xsmcSetInteger(xsVar(2), evt->characteristic);
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	if (write) {
		xsmcSetArrayBuffer(xsVar(3), evt->value.data, evt->value.len);
		xsmcSet(xsVar(0), xsID_value, xsVar(3));
		xsCall2(gBLE->obj, xsID_callback, xsString("onCharacteristicWritten"), xsVar(0));
	}
	else {
		xsResult = xsCall2(gBLE->obj, xsID_callback, xsString("onCharacteristicRead"), xsVar(0));
		gecko_cmd_gatt_server_send_user_read_response(evt->connection, evt->characteristic, 0, xsGetArrayBufferLength(xsResult), xsmcToArrayBuffer(xsResult));
	}
bail:
	xsEndHost(gBLE->the);
}

static void gattServerUserReadRequest(struct gecko_msg_gatt_server_user_read_request_evt_t *evt)
{
	doReadOrWriteRequest((struct gecko_msg_gatt_server_user_write_request_evt_t *)evt, false);
}

static void gattServerUserWriteRequest(struct gecko_msg_gatt_server_user_write_request_evt_t *evt)
{
	doReadOrWriteRequest(evt, true);
}

static void smPasskeyDisplayEvent(struct gecko_msg_sm_passkey_display_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	if (evt->connection != gBLE->connection)
		goto bail;
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), gBLE->address.addr, 6);
	xsmcSetInteger(xsVar(2), evt->passkey);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSet(xsVar(0), xsID_passkey, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyDisplay"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

static void smPasskeyRequestEvent(struct gecko_msg_sm_passkey_request_evt_t *evt)
{
	uint32_t passkey;
	xsBeginHost(gBLE->the);
	if (evt->connection != gBLE->connection)
		goto bail;
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), gBLE->address.addr, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsResult = xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyRequested"), xsVar(0));
	passkey = xsmcToInteger(xsResult);
	gecko_cmd_sm_enter_passkey(evt->connection, passkey);
bail:
	xsEndHost(gBLE->the);
}

static void smPasskeyConfirmEvent(struct gecko_msg_sm_confirm_passkey_evt_t *evt)
{
	uint8_t confirm;
	xsBeginHost(gBLE->the);
	if (evt->connection != gBLE->connection)
		goto bail;
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), gBLE->address.addr, 6);
	xsmcSetInteger(xsVar(2), evt->passkey);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSet(xsVar(0), xsID_passkey, xsVar(2));
	xsResult = xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyConfirm"), xsVar(0));
	confirm = xsmcToBoolean(xsResult);
	gecko_cmd_sm_passkey_confirm(evt->connection, confirm);
bail:
	xsEndHost(gBLE->the);
}

static void smBondingFailedEvent(struct gecko_msg_sm_bonding_failed_evt_t *evt)
{
	switch(evt->reason) {
		case bg_err_smp_pairing_not_supported:
		case bg_err_bt_pin_or_key_missing:
			if (0xFF != gBLE->bond) {
				gecko_cmd_sm_delete_bonding(gBLE->bond);	// remove bond and try again
				gBLE->bond = 0xFF;
				gecko_cmd_sm_increase_security(evt->connection);
			}
			break;
		default:
			break;
	}
}

void ble_event_handler(struct gecko_cmd_packet* evt)
{
	switch(BGLIB_MSG_ID(evt->header)) {
		case gecko_evt_system_boot_id:
			systemBootEvent(&evt->data.evt_system_boot);
			break;
		case gecko_evt_le_connection_opened_id:
			leConnectionOpenedEvent(&evt->data.evt_le_connection_opened);
			break;
		case gecko_evt_le_connection_closed_id:
			leConnectionClosedEvent(&evt->data.evt_le_connection_closed);
			break;
		case gecko_evt_gatt_server_characteristic_status_id:
			gattServerCharacteristicStatus(&evt->data.evt_gatt_server_characteristic_status);
			break;
		case gecko_evt_gatt_server_user_read_request_id:
			gattServerUserReadRequest(&evt->data.evt_gatt_server_user_read_request);
			break;
		case gecko_evt_gatt_server_user_write_request_id:
			gattServerUserWriteRequest(&evt->data.evt_gatt_server_user_write_request);
			break;
		case gecko_evt_sm_passkey_display_id:
			smPasskeyDisplayEvent(&evt->data.evt_sm_passkey_display);
			break;
		case gecko_evt_sm_passkey_request_id:
			smPasskeyRequestEvent(&evt->data.evt_sm_passkey_request);
			break;
		case gecko_evt_sm_confirm_passkey_id:
			smPasskeyConfirmEvent(&evt->data.evt_sm_confirm_passkey);
			break;
		case gecko_evt_le_connection_parameters_id:
			leConnectionParametersEvent(&evt->data.evt_le_connection_parameters);
			break;
		case gecko_evt_sm_bonding_failed_id:
			smBondingFailedEvent(&evt->data.evt_sm_bonding_failed);
			break;
		default:
			break;
	}
}

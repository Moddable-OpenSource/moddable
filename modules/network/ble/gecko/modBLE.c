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

//#define LOGGING
#if defined(LOGGING)
	static void logEvent(uint32_t event);
	#define LOG_EVENT(event) logEvent(event)
	#define LOG_MSG(msg) modLog(msg)
	#define LOG_GAP_MSG(msg) modLog(msg)
	#define LOG_GATTC_MSG(msg) modLog(msg)
#else
	#define LOG_EVENT(event)
	#define LOG_MSG(msg)
	#define LOG_GAP_MSG(msg)
	#define LOG_GATTC_MSG(msg)
#endif

enum {
	GATT_PROCEDURE_NONE_ID = 0,
	GATT_PROCEDURE_SERVICES_ID,
	GATT_PROCEDURE_CHARACTERISTICS_ID,
	GATT_PROCEDURE_DESCRIPTORS_ID,
	GATT_PROCEDURE_CHARACTERISTIC_VALUE_ID
};

typedef struct {
	uint8_t id;
	xsSlot obj;
} gattProcedureRecord, *gattProcedure;

typedef struct modBLEConnectionRecord modBLEConnectionRecord;
typedef modBLEConnectionRecord *modBLEConnection;

struct modBLEConnectionRecord {
	struct modBLEConnectionRecord *next;

	xsMachine *the;
	xsSlot objConnection;
	xsSlot objClient;

	uint8_t id;
	bd_addr bda;
	gattProcedureRecord procedure;
};

typedef struct {
	xsMachine	*the;
	xsSlot		obj;

	// server

	// client connections
	modBLEConnection connections;
} modBLERecord, *modBLE;

static void modBLEConnectionAdd(modBLEConnection connection);
static void modBLEConnectionRemove(modBLEConnection connection);
static modBLEConnection modBLEConnectionFindByConnectionID(uint8_t conn_id);
static modBLEConnection modBLEConnectionFindByAddress(bd_addr *bda);

static void uuidToBuffer(uint8array *uuid, uint8_t *buffer, uint16_t *length);
static void bufferToUUID(uint8_t *buffer, uint8array *uuid, uint16_t length);

static modBLE gBLE = NULL;

#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 4
#endif
uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MAX_CONNECTIONS)];

// Gecko configuration parameters (see gecko_configuration.h)
static const gecko_configuration_t config = {
  .config_flags = 0,
  .sleep.flags = SLEEP_FLAGS_DEEP_SLEEP_ENABLE,
  .bluetooth.max_connections = MAX_CONNECTIONS,
  .bluetooth.heap = bluetooth_stack_heap,
  .bluetooth.heap_size = sizeof(bluetooth_stack_heap),
  .bluetooth.sleep_clock_accuracy = 100, // ppm
  .gattdb = NULL,	// @@
  .ota.flags = 0,
  .ota.device_name_len = 3,
  .ota.device_name_ptr = "OTA",
#if (HAL_PA_ENABLE) && defined(FEATURE_PA_HIGH_POWER)
  .pa.config_enable = 1, // Enable high power PA
  .pa.input = GECKO_RADIO_PA_INPUT_VBAT, // Configure PA input to VBAT
#endif // (HAL_PA_ENABLE) && defined(FEATURE_PA_HIGH_POWER)
};

void xs_ble_initialize(xsMachine *the)
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
	gecko_init(&config);
}

void xs_ble_close(xsMachine *the)
{
	xsForget(gBLE->obj);
	xs_ble_destructor(gBLE);
}

void xs_ble_destructor(void *data)
{
	modBLE ble = data;
	if (ble)
		c_free(ble);
	gBLE = NULL;
}

void xs_ble_set_device_name(xsMachine *the)
{
	const char *name = xsmcToString(xsArg(0));
	gecko_cmd_system_set_device_name(0, strlen(name), (uint8_t*)name);
}

void xs_ble_start_advertising(xsMachine *the)
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
	

void xs_ble_stop_advertising(xsMachine *the)
{
	gecko_cmd_le_gap_set_mode(le_gap_non_discoverable, le_gap_non_connectable);
}

void xs_ble_start_scanning(xsMachine *the)
{
	uint8_t active = xsmcToBoolean(xsArg(0));
	uint16_t interval = xsmcToInteger(xsArg(1));
	uint16_t window = xsmcToInteger(xsArg(2));
	
	gecko_cmd_le_gap_set_scan_parameters(interval, window, active ? 1 : 0);
   	gecko_cmd_le_gap_discover(le_gap_discover_generic);
}

void xs_ble_stop_scanning(xsMachine *the)
{
	gecko_cmd_le_gap_end_procedure();
}

void xs_ble_connect(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	bd_addr bda;

	c_memmove(&bda.addr, address, sizeof(bda.addr));
		
	// Ignore duplicate connection attempts
	if (modBLEConnectionFindByAddress(&bda)) {
		LOG_GAP_MSG("Ignoring duplicate connect attempt");
		return;
	};
	
	// Add a new connection record to be filled as the connection completes
	modBLEConnection connection = c_calloc(sizeof(modBLEConnectionRecord), 1);
	if (!connection)
		xsUnknownError("out of memory");
	connection->id = -1;
	c_memmove(&connection->bda, &bda, sizeof(bda));
	modBLEConnectionAdd(connection);
	
	gecko_cmd_le_gap_open(bda, le_gap_address_type_public);
}

modBLEConnection modBLEConnectionFindByConnectionID(uint8_t conn_id)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (conn_id == walker->id)
			break;
	return walker;
}

modBLEConnection modBLEConnectionFindByAddress(bd_addr *bda)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (0 == c_memcmp(bda->addr, walker->bda.addr, 6))
			break;
	return walker;
}

void modBLEConnectionAdd(modBLEConnection connection)
{
	if (!gBLE->connections)
		gBLE->connections = connection;
	else {
		modBLEConnection walker;
		for (walker = gBLE->connections; walker->next; walker = walker->next)
			;
		walker->next = connection;
	}
}

void modBLEConnectionRemove(modBLEConnection connection)
{
	modBLEConnection walker, prev = NULL;
	for (walker = gBLE->connections; NULL != walker; prev = walker, walker = walker->next) {
		if (connection == walker) {
			if (NULL == prev)
				gBLE->connections = walker->next;
			else
				prev->next = walker->next;
			c_free(connection);
			break;
		}
	}
}

void xs_gap_connection_initialize(xsMachine *the)
{
	uint8_t conn_id;
	xsmcVars(1);	// xsArg(0) is client
	xsmcGet(xsVar(0), xsArg(0), xsID_connection);
	conn_id = xsmcToInteger(xsVar(0));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	connection->the = the;
	connection->objConnection = xsThis;
	connection->objClient = xsArg(0);
}
	
void xs_gap_connection_disconnect(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	gecko_cmd_le_connection_close(conn_id);
}

void xs_gap_connection_read_rssi(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	gecko_cmd_le_connection_get_rssi(conn_id);
}

void xs_gatt_client_discover_primary_services(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t argc = xsmcArgc;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	connection->procedure.id = GATT_PROCEDURE_SERVICES_ID;
	connection->procedure.obj = xsThis;
	if (argc > 1) {
		uint8array uuid;
		bufferToUUID((uint8_t*)xsmcToArrayBuffer(xsArg(1)), &uuid, xsGetArrayBufferLength(xsArg(1)));
		gecko_cmd_gatt_discover_primary_services_by_uuid(conn_id, uuid.len, uuid.data);
	}
	else
		gecko_cmd_gatt_discover_primary_services(conn_id);
}

void xs_gatt_service_discover_characteristics(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t start = xsmcToInteger(xsArg(1));
	uint16_t end = xsmcToInteger(xsArg(2));
	uint32_t handle = ((uint32_t)start << 16) | end;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	connection->procedure.id = GATT_PROCEDURE_CHARACTERISTICS_ID;
	connection->procedure.obj = xsThis;
	if (argc > 3) {
		uint8array uuid;
		bufferToUUID((uint8_t*)xsmcToArrayBuffer(xsArg(3)), &uuid, xsGetArrayBufferLength(xsArg(3)));
		gecko_cmd_gatt_discover_characteristics_by_uuid(conn_id, handle, uuid.len, uuid.data);
	}
	else
		gecko_cmd_gatt_discover_characteristics(conn_id, handle);
}

void xs_gatt_characteristic_discover_all_characteristic_descriptors(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	connection->procedure.id = GATT_PROCEDURE_DESCRIPTORS_ID;
	connection->procedure.obj = xsThis;
	gecko_cmd_gatt_discover_descriptors(conn_id, characteristic);
}

void xs_gatt_characteristic_write_without_response(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	char *str;
		
	switch (xsmcTypeOf(xsArg(2))) {
		case xsStringType:
			str = xsmcToString(xsArg(2));
			gecko_cmd_gatt_write_characteristic_value_without_response(conn_id, handle, strlen(str), (uint8_t*)str);
			break;
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype))
				gecko_cmd_gatt_write_characteristic_value_without_response(conn_id, handle, xsGetArrayBufferLength(xsArg(2)), (uint8_t*)xsmcToArrayBuffer(xsArg(2)));
			else
				goto unknown;
			break;
		unknown:
		default:
			xsUnknownError("unsupported type");
			break;
	}
}

void xs_gatt_characteristic_read_value(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	connection->procedure.id = GATT_PROCEDURE_CHARACTERISTIC_VALUE_ID;
	connection->procedure.obj = xsThis;
	gecko_cmd_gatt_read_characteristic_value(conn_id, handle);
}

void xs_gatt_characteristic_enable_notifications(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	gecko_cmd_gatt_set_characteristic_notification(conn_id, handle, gatt_notification);
}

void xs_gatt_characteristic_disable_notifications(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	gecko_cmd_gatt_set_characteristic_notification(conn_id, handle, gatt_disable);
}

void xs_gatt_descriptor_write_value(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	switch (xsmcTypeOf(xsArg(2))) {
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype))
				gecko_cmd_gatt_write_descriptor_value(conn_id, handle, xsGetArrayBufferLength(xsArg(2)), (uint8_t*)xsmcToArrayBuffer(xsArg(2)));
			else
				goto unknown;
			break;
		unknown:
		default:
			xsUnknownError("unsupported type");
			break;
	}
}

void uuidToBuffer(uint8array *uuid, uint8_t *buffer, uint16_t *length)
{
	uint16_t len = uuid->len;
	for (uint8_t i = 0; i < len; ++i)
		buffer[i] = uuid->data[len - 1 - i];
	*length = len;
}

void bufferToUUID(uint8_t *buffer, uint8array *uuid, uint16_t length)
{
	for (uint8_t i = 0; i < length; ++i)
		uuid->data[i] = buffer[length - 1 - i];
	uuid->len = length;
}

static void systemBootEvent(struct gecko_msg_system_boot_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	xsCall1(gBLE->obj, xsID_callback, xsString("_onReady"));
	xsEndHost(gBLE->the);
}

static void leGapScanResponseEvent(struct gecko_msg_le_gap_scan_response_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), evt->data.data, evt->data.len);
	xsmcSetArrayBuffer(xsVar(2), evt->address.addr, 6);
	xsmcSet(xsVar(0), xsID_scanResponse, xsVar(1));
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("_onDiscovered"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void leConnectionOpenedEvent(struct gecko_msg_le_connection_opened_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAddress(&evt->address);
	if (!connection)
		xsUnknownError("connection not found");
		
	// Ignore duplicate connection events
	if (-1 != connection->id) {
		LOG_GAP_MSG("Ignoring duplicate connect event");
		goto bail;
	}
	connection->id = evt->connection;
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), evt->connection);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(2), evt->address.addr, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("_onConnected"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

static void leConnectionClosedEvent(struct gecko_msg_le_connection_closed_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
		
	// ignore multiple disconnects on same connection
	if (!connection) {
		LOG_GATTC_MSG("Ignoring duplicate disconnect event");
		goto bail;
	}	
	
	xsCall1(connection->objConnection, xsID_callback, xsString("_onDisconnected"));
	modBLEConnectionRemove(connection);
bail:
	xsEndHost(gBLE->the);
}

static void leConnectionRSSIEvent(struct gecko_msg_le_connection_rssi_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	xsCall2(connection->objConnection, xsID_callback, xsString("_onRSSI"), xsInteger(((int16_t)evt->rssi)));
	xsEndHost(gBLE->the);
}

static void gattServiceEvent(struct gecko_msg_gatt_service_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	uint16_t start, end, uuid_length;
	uint8_t uuid[16];
	uuidToBuffer(&evt->uuid, uuid, &uuid_length);
	start = ((evt->service >> 16) & 0xFFFF);
	end = (evt->service & 0xFFFF);
	xsmcVars(4);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), uuid, uuid_length);
	xsmcSetInteger(xsVar(2), start);
	xsmcSetInteger(xsVar(3), end);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSet(xsVar(0), xsID_start, xsVar(2));
	xsmcSet(xsVar(0), xsID_end, xsVar(3));
	xsCall2(connection->procedure.obj, xsID_callback, xsString("_onService"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattCharacteristicEvent(struct gecko_msg_gatt_characteristic_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(4);
	uint8_t uuid[16];
	uint16_t uuid_length;
	uuidToBuffer(&evt->uuid, uuid, &uuid_length);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), uuid, uuid_length);
	xsmcSetInteger(xsVar(2), evt->characteristic);
	xsmcSetInteger(xsVar(3), evt->properties);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsmcSet(xsVar(0), xsID_properties, xsVar(3));
	xsCall2(connection->procedure.obj, xsID_callback, xsString("_onCharacteristic"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattDescriptorEvent(struct gecko_msg_gatt_descriptor_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(3);
	uint8_t uuid[16];
	uint16_t uuid_length;
	uuidToBuffer(&evt->uuid, uuid, &uuid_length);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), uuid, uuid_length);
	xsmcSetInteger(xsVar(2), evt->descriptor);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->procedure.obj, xsID_callback, xsString("_onDescriptor"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattCharacteristicValueEvent(struct gecko_msg_gatt_characteristic_value_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(3);
	xsmcSetArrayBuffer(xsVar(1), evt->value.data, evt->value.len);
	xsmcSetInteger(xsVar(2), evt->characteristic);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->objClient, xsID_callback, xsString("_onCharacteristicValue"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattCharacteristicNotifyEvent(struct gecko_msg_gatt_characteristic_value_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(3);
	xsmcSetArrayBuffer(xsVar(1), evt->value.data, evt->value.len);
	xsmcSetInteger(xsVar(2), evt->characteristic);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->objClient, xsID_callback, xsString("_onCharacteristicNotification"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattProcedureCompletedEvent(struct gecko_msg_gatt_procedure_completed_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	switch(connection->procedure.id) {
		case GATT_PROCEDURE_SERVICES_ID:
			xsCall1(connection->procedure.obj, xsID_callback, xsString("_onService"));
			break;
		case GATT_PROCEDURE_CHARACTERISTICS_ID:
			xsCall1(connection->procedure.obj, xsID_callback, xsString("_onCharacteristic"));
			break;
		case GATT_PROCEDURE_DESCRIPTORS_ID:
			xsCall1(connection->procedure.obj, xsID_callback, xsString("_onDescriptor"));
			break;
		default:
			break;
	}
	connection->procedure.id = GATT_PROCEDURE_NONE_ID;
	xsmcSetNull(connection->procedure.obj);
	xsEndHost(gBLE->the);
}

void bglib_event_handler(struct gecko_cmd_packet* evt)
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
		case gecko_evt_le_connection_rssi_id:
			leConnectionRSSIEvent(&evt->data.evt_le_connection_rssi);
			break;
		case gecko_evt_le_gap_scan_response_id:
			leGapScanResponseEvent(&evt->data.evt_le_gap_scan_response);
			break;
		case gecko_evt_gatt_service_id:
			gattServiceEvent(&evt->data.evt_gatt_service);
			break;
		case gecko_evt_gatt_characteristic_id:
			gattCharacteristicEvent(&evt->data.evt_gatt_characteristic);
			break;
		case gecko_evt_gatt_descriptor_id:
			gattDescriptorEvent(&evt->data.evt_gatt_descriptor);
			break;
		case gecko_evt_gatt_characteristic_value_id:
			if (gatt_read_response == evt->data.evt_gatt_characteristic_value.att_opcode)
				gattCharacteristicValueEvent(&evt->data.evt_gatt_characteristic_value);
			else if (gatt_handle_value_notification == evt->data.evt_gatt_characteristic_value.att_opcode)
				gattCharacteristicNotifyEvent(&evt->data.evt_gatt_characteristic_value);
			break;
		case gecko_evt_gatt_procedure_completed_id:
			gattProcedureCompletedEvent(&evt->data.evt_gatt_procedure_completed);
			break;
	}
}

#if LOG_EVENT
static void logEvent(struct gecko_cmd_packet* evt)
{
	switch(BGLIB_MSG_ID(evt->header)) {
		case gecko_evt_system_boot_id: modLog("gecko_evt_system_boot_id"); break;
		case gecko_evt_le_connection_opened_id: modLog("gecko_evt_le_connection_opened_id"); break;
		case gecko_evt_le_connection_closed_id: modLog("gecko_evt_le_connection_closed_id"); break;
		case gecko_evt_le_connection_rssi_id: modLog("gecko_evt_le_connection_rssi_id"); break;
		case gecko_evt_le_gap_scan_response_id: modLog("gecko_evt_le_gap_scan_response_id"); break;
		case gecko_evt_gatt_service_id: modLog("gecko_evt_gatt_service_id"); break;
		case evt_gatt_characteristic_id: modLog("evt_gatt_characteristic_id"); break;
		case evt_gatt_descriptor_id: modLog("evt_gatt_descriptor_id"); break;
		case evt_gatt_characteristic_value_id: modLog("evt_gatt_characteristic_value_id"); break;
		case evt_gatt_procedure_completed_id: modLog("evt_gatt_procedure_completed_id"); break;
	}
}
#endif
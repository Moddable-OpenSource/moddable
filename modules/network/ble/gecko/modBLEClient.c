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

#include "bg_gattdb_def.h"

#ifdef __GNUC__
	#define GATT_HEADER(F) F __attribute__ ((section (".gatt_header"))) 
	#define GATT_DATA(F) F __attribute__ ((section (".gatt_data"))) 
#else
	#ifdef __ICCARM__
		#define GATT_HEADER(F) _Pragma("location=\".gatt_header\"") F 
		#define GATT_DATA(F) _Pragma("location=\".gatt_data\"") F 
	#else
		#define GATT_HEADER(F) F 
		#define GATT_DATA(F) F 
	#endif
#endif

GATT_DATA(const uint16_t bg_gattdb_data_uuidtable_16_map [])={0x2800,0x2801,0x2803};
GATT_DATA(const uint8_t bg_gattdb_data_uuidtable_128_map [])={0x0};
GATT_DATA(const struct bg_gattdb_attribute bg_gattdb_data_attributes_map[])={{0}};
GATT_DATA(const uint16_t bg_gattdb_data_attributes_dynamic_mapping_map[])={0};
GATT_DATA(const uint8_t bg_gattdb_data_adv_uuid16_map[])={0x0};
GATT_DATA(const uint8_t bg_gattdb_data_adv_uuid128_map[])={0x0};
GATT_HEADER(const struct bg_gattdb_def bg_gattdb_data)={
	.attributes=bg_gattdb_data_attributes_map,
    .attributes_max=0,
    .uuidtable_16_size=3,
    .uuidtable_16=bg_gattdb_data_uuidtable_16_map,
    .uuidtable_128_size=0,
    .uuidtable_128=bg_gattdb_data_uuidtable_128_map,
    .attributes_dynamic_max=0,
    .attributes_dynamic_mapping=bg_gattdb_data_attributes_dynamic_mapping_map,
    .adv_uuid16=bg_gattdb_data_adv_uuid16_map,
    .adv_uuid16_num=0,
    .adv_uuid128=bg_gattdb_data_adv_uuid128_map,
    .adv_uuid128_num=0,
    .caps_mask=0xffff,
    .enabled_caps=0xffff,
};

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

#define DEFAULT_MTU 247

enum {
	CMD_GATT_DISCOVER_SERVICES_ID = 0,
	CMD_GATT_DISCOVER_SERVICES_UUID_ID,
	CMD_GATT_DISCOVER_CHARACTERISTICS_ID,
	CMD_GATT_DISCOVER_CHARACTERISTICS_UUID_ID,
	CMD_GATT_DISCOVER_DESCRIPTORS_ID,
	CMD_GATT_CHARACTERISTIC_READ_VALUE_ID,
	CMD_GATT_CHARACTERISTIC_SET_NOTIFICATION_ID,
	CMD_GATT_CHARACTERISTIC_WRITE_WITHOUT_RESPONSE_ID,
	CMD_GATT_DESCRIPTOR_WRITE_VALUE_ID,
	CMD_GATT_DESCRIPTOR_READ_VALUE_ID
};

typedef struct {
  uint8 len;
  uint8  data[16];
} uuidRecord;

typedef struct gattProcedureRecord gattProcedureRecord;
typedef gattProcedureRecord *gattProcedure;

struct gattProcedureRecord {
	struct gattProcedureRecord *next;

	uint8_t cmd;
	uint8_t executed;
	uint8_t index;
	xsSlot obj;
	union {
		struct {
			uint8_t connection;
			uuidRecord uuid;
		} discover_services_param;
		struct {
			uint8_t connection;
			uint32_t service;
			uuidRecord uuid;
		} discover_characteristics_param;
		struct {
			uint8_t connection;
			uint16_t characteristic;
		} discover_descriptors_param;
		struct {
			uint8_t connection;
			uint8_t flags;
			uint16_t characteristic;
		} notification_param;
		struct {
			uint8_t connection;
			uint16_t handle;
		} read_value_param;
		struct {
			uint8_t connection;
			uint16_t handle;
			uint8_t *value;
			uint8_t length;
		} write_value_param;
	};
};

typedef struct modBLEConnectionRecord modBLEConnectionRecord;
typedef modBLEConnectionRecord *modBLEConnection;

struct modBLEConnectionRecord {
	struct modBLEConnectionRecord *next;

	xsMachine *the;
	xsSlot objConnection;
	xsSlot objClient;

	int8_t id;
	bd_addr bda;
	uint8_t bond;
	gattProcedure procedureQueue;
};

typedef struct {
	xsMachine	*the;
	xsSlot		obj;

	modTimer timer;

	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;
	
	// client connections
	modBLEConnection connections;
} modBLERecord, *modBLE;

static void modBLEConnectionAdd(modBLEConnection connection);
static void modBLEConnectionRemove(modBLEConnection connection);
static modBLEConnection modBLEConnectionFindByConnectionID(uint8_t conn_id);
static modBLEConnection modBLEConnectionFindByAddress(bd_addr *bda);

static void uuidToBuffer(uint8array *uuid, uint8_t *buffer, uint16_t *length);
static void bufferToUUID(uint8_t *buffer, uuidRecord *uuid, uint16_t length);
static void addressToBuffer(bd_addr *bda, uint8_t *buffer);
static void bufferToAddress(uint8_t *buffer, bd_addr *bda);
static void bleTimerCallback(modTimer timer, void *refcon, int refconSize);
static void ble_event_handler(struct gecko_cmd_packet* evt);

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
	gecko_stack_init(&config);
	gecko_bgapi_class_system_init();
	gecko_bgapi_class_le_gap_init();
	gecko_bgapi_class_le_connection_init();
	gecko_bgapi_class_gatt_init();

	gBLE->timer = modTimerAdd(0, 20, bleTimerCallback, NULL, 0);
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
		modTimerRemove(ble->timer);
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
	
	gecko_cmd_le_gap_set_scan_parameters(interval, window, active ? 1 : 0);
   	gecko_cmd_le_gap_discover(le_gap_discover_generic);
}

void xs_ble_client_stop_scanning(xsMachine *the)
{
	gecko_cmd_le_gap_end_procedure();
}

void xs_ble_client_connect(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	bd_addr bda;

	bufferToAddress(address, &bda);
		
	// Ignore duplicate connection attempts
	if (modBLEConnectionFindByAddress(&bda)) {
		return;
	};
	
	// Add a new connection record to be filled as the connection completes
	modBLEConnection connection = c_calloc(sizeof(modBLEConnectionRecord), 1);
	if (!connection)
		xsUnknownError("out of memory");
	connection->id = -1;
	connection->bond = 0xFF;
	c_memmove(&connection->bda, &bda, sizeof(bda));
	modBLEConnectionAdd(connection);
	
	gecko_cmd_le_gap_open(bda, le_gap_address_type_public);
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

	if (bonding || (encryption && mitm))
		gecko_cmd_sm_set_bondable_mode(1);

	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetBoolean(xsVar(1), gBLE->encryption);
	xsmcSet(xsVar(0), xsID_encryption, xsVar(1));
	xsmcSetBoolean(xsVar(1), gBLE->bonding);
	xsmcSet(xsVar(0), xsID_bonding, xsVar(1));
	xsmcSetBoolean(xsVar(1), gBLE->mitm);
	xsmcSet(xsVar(0), xsID_mitm, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onSecurityParameters"), xsVar(0));
	xsEndHost(gBLE->the);
}

void xs_ble_client_passkey_reply(xsMachine *the)
{
	bd_addr bda;
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t confirm = xsmcToBoolean(xsArg(1));
	bufferToAddress(address, &bda);
	
	modBLEConnection connection = modBLEConnectionFindByAddress(&bda);
	if (!connection) return;
	
	gecko_cmd_sm_passkey_confirm(connection->id, confirm);
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

static void gattProcedureExecute(gattProcedure procedure)
{
	procedure->executed = true;
	
	switch(procedure->cmd) {
		case CMD_GATT_DISCOVER_SERVICES_ID:
			gecko_cmd_gatt_discover_primary_services(procedure->discover_services_param.connection);
			break;
		case CMD_GATT_DISCOVER_SERVICES_UUID_ID:
			gecko_cmd_gatt_discover_primary_services_by_uuid(procedure->discover_services_param.connection, procedure->discover_services_param.uuid.len, (const uint8_t*)&procedure->discover_services_param.uuid.data);
			break;
		case CMD_GATT_DISCOVER_CHARACTERISTICS_ID:
			gecko_cmd_gatt_discover_characteristics(procedure->discover_characteristics_param.connection, procedure->discover_characteristics_param.service);
			break;
		case CMD_GATT_DISCOVER_CHARACTERISTICS_UUID_ID:
			gecko_cmd_gatt_discover_characteristics_by_uuid(procedure->discover_characteristics_param.connection, procedure->discover_characteristics_param.service, procedure->discover_characteristics_param.uuid.len, (const uint8_t*)&procedure->discover_characteristics_param.uuid.data);
			break;
		case CMD_GATT_DISCOVER_DESCRIPTORS_ID:
			gecko_cmd_gatt_discover_descriptors(procedure->discover_descriptors_param.connection, procedure->discover_descriptors_param.characteristic);
			break;
		case CMD_GATT_CHARACTERISTIC_READ_VALUE_ID:
			gecko_cmd_gatt_read_characteristic_value(procedure->read_value_param.connection, procedure->read_value_param.handle);
			break;
		case CMD_GATT_CHARACTERISTIC_SET_NOTIFICATION_ID:
			gecko_cmd_gatt_set_characteristic_notification(procedure->notification_param.connection, procedure->notification_param.characteristic, procedure->notification_param.flags);
			break;
		case CMD_GATT_CHARACTERISTIC_WRITE_WITHOUT_RESPONSE_ID:
			gecko_cmd_gatt_write_characteristic_value_without_response(procedure->write_value_param.connection, procedure->write_value_param.handle, procedure->write_value_param.length, procedure->write_value_param.value);
			c_free(procedure->write_value_param.value);
			break;
		case CMD_GATT_DESCRIPTOR_WRITE_VALUE_ID:
			gecko_cmd_gatt_write_descriptor_value(procedure->write_value_param.connection, procedure->write_value_param.handle, procedure->write_value_param.length, procedure->write_value_param.value);
			c_free(procedure->write_value_param.value);
			break;
		case CMD_GATT_DESCRIPTOR_READ_VALUE_ID:
			gecko_cmd_gatt_read_descriptor_value(procedure->read_value_param.connection, procedure->read_value_param.handle);
			break;
		default:
			procedure->executed = false;	// @@
			break;
	}
}

static void gattProcedureQueueAndDo(xsMachine *the, modBLEConnection connection, gattProcedure procedure)
{
	gattProcedure proc = c_malloc(sizeof(gattProcedureRecord));
	if (!proc)
		xsUnknownError("out of memory");
	*proc = *procedure;
	if (!connection->procedureQueue) {
		connection->procedureQueue = proc;
		//xsTrace("gattProcedureQueueAndDo executing procedure\n");
		gattProcedureExecute(proc);
	}
	else {
		gattProcedure walker;
		for (walker = connection->procedureQueue; walker->next; walker = walker->next)
			;
		walker->next = proc;
		//xsTrace("gattProcedureQueueAndDo queuing procedure\n");
	}
}

void xs_gatt_client_discover_primary_services(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t argc = xsmcArgc;
	gattProcedureRecord procedure = {0};
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	procedure.obj = xsThis;
	procedure.discover_services_param.connection = conn_id;
	if (argc > 1) {
		uuidRecord uuid;
		bufferToUUID((uint8_t*)xsmcToArrayBuffer(xsArg(1)), &uuid, xsGetArrayBufferLength(xsArg(1)));
		procedure.cmd = CMD_GATT_DISCOVER_SERVICES_UUID_ID;
		procedure.discover_services_param.uuid = uuid;
	}
	else {
		procedure.cmd = CMD_GATT_DISCOVER_SERVICES_ID;
	}
	gattProcedureQueueAndDo(the, connection, &procedure);
}

void xs_gatt_service_discover_characteristics(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t start = xsmcToInteger(xsArg(1));
	uint16_t end = xsmcToInteger(xsArg(2));
	uint32_t service = ((uint32_t)start << 16) | end;
	gattProcedureRecord procedure = {0};
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	procedure.obj = xsThis;
	procedure.discover_characteristics_param.connection = conn_id;
	procedure.discover_characteristics_param.service = service;
	if (argc > 3) {
		uuidRecord uuid;
		bufferToUUID((uint8_t*)xsmcToArrayBuffer(xsArg(3)), &uuid, xsGetArrayBufferLength(xsArg(3)));
		procedure.cmd = CMD_GATT_DISCOVER_CHARACTERISTICS_UUID_ID;
		procedure.discover_characteristics_param.uuid = uuid;
	}
	else {
		procedure.cmd = CMD_GATT_DISCOVER_CHARACTERISTICS_ID;
	}
	gattProcedureQueueAndDo(the, connection, &procedure);
}

void xs_gatt_characteristic_discover_all_characteristic_descriptors(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
	gattProcedureRecord procedure = {0};
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	procedure.obj = xsThis;
	procedure.cmd = CMD_GATT_DISCOVER_DESCRIPTORS_ID;
	procedure.discover_descriptors_param.connection = conn_id;
	procedure.discover_descriptors_param.characteristic = characteristic;
	gattProcedureQueueAndDo(the, connection, &procedure);
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
	gattProcedureRecord procedure = {0};
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	procedure.obj = connection->objClient;
	procedure.cmd = CMD_GATT_CHARACTERISTIC_READ_VALUE_ID;
	procedure.read_value_param.connection = conn_id;
	procedure.read_value_param.handle = handle;
	gattProcedureQueueAndDo(the, connection, &procedure);
}

static void set_notifications(xsMachine *the, xsSlot theThis, modBLEConnection connection, uint16_t handle, uint8_t flags)
{
	gattProcedureRecord procedure = {0};
	procedure.obj = theThis;
	procedure.cmd = CMD_GATT_CHARACTERISTIC_SET_NOTIFICATION_ID;
	procedure.notification_param.connection = connection->id;
	procedure.notification_param.characteristic = handle;
	procedure.notification_param.flags = flags;
	gattProcedureQueueAndDo(the, connection, &procedure);
}

void xs_gatt_characteristic_enable_notifications(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	set_notifications(the, xsThis, connection, characteristic, gatt_notification);
}

void xs_gatt_characteristic_disable_notifications(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	set_notifications(the, xsThis, connection, characteristic, gatt_disable);
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
	gattProcedureRecord procedure = {0};
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	procedure.obj = connection->objClient;
	procedure.cmd = CMD_GATT_DESCRIPTOR_READ_VALUE_ID;
	procedure.read_value_param.connection = conn_id;
	procedure.read_value_param.handle = handle;
	gattProcedureQueueAndDo(the, connection, &procedure);
}

void xs_gatt_descriptor_write_value(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	switch (xsmcTypeOf(xsArg(2))) {
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype)) {
				gattProcedureRecord procedure = {0};
				uint16_t length = xsGetArrayBufferLength(xsArg(2));
				uint8_t *buffer = c_malloc(length);
				if (!buffer)
					xsUnknownError("out of memory");
				c_memmove(buffer, (uint8_t*)xsmcToArrayBuffer(xsArg(2)), length);
				procedure.obj = xsThis;
				procedure.cmd = CMD_GATT_DESCRIPTOR_WRITE_VALUE_ID;
				procedure.write_value_param.connection = conn_id;
				procedure.write_value_param.handle = handle;
				procedure.write_value_param.value = buffer;
				procedure.write_value_param.length = length;
				gattProcedureQueueAndDo(the, connection, &procedure);
			}
			else
				goto unknown;
			break;
		unknown:
		default:
			xsUnknownError("unsupported type");
			break;
	}
}

void xs_gatt_characteristic_write_without_response(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	char *str;
	uint8_t *buffer;
	uint8_t length;
		
	switch (xsmcTypeOf(xsArg(2))) {
		case xsStringType:
			str = xsmcToString(xsArg(2));
			length = strlen(str);
			buffer = c_malloc(length);
			if (!buffer)
				xsUnknownError("out of memory");
			c_memmove(buffer, str, length);
			break;
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype)) {
				length = xsGetArrayBufferLength(xsArg(2));
				buffer = c_malloc(length);
				if (!buffer)
					xsUnknownError("out of memory");
				c_memmove(buffer, xsmcToArrayBuffer(xsArg(2)), length);
			}
			else
				goto unknown;
			break;
		unknown:
		default:
			xsUnknownError("unsupported type");
			break;
	}
	if (!connection->procedureQueue) {
		gecko_cmd_gatt_write_characteristic_value_without_response(conn_id, handle, length, buffer);
	}
	else {
		//xsTrace("queuing write_without_response\n");
		gattProcedureRecord procedure = {0};
		xsmcSetUndefined(procedure.obj);
		procedure.cmd = CMD_GATT_CHARACTERISTIC_WRITE_WITHOUT_RESPONSE_ID;
		procedure.write_value_param.connection = conn_id;
		procedure.write_value_param.handle = handle;
		procedure.write_value_param.value = buffer;
		procedure.write_value_param.length = length;
		gattProcedureQueueAndDo(the, connection, &procedure);
	}
}

void uuidToBuffer(uint8array *uuid, uint8_t *buffer, uint16_t *length)
{
	c_memmove(buffer, uuid->data, uuid->len);
	*length = uuid->len;
}

void bufferToUUID(uint8_t *buffer, uuidRecord *uuid, uint16_t length)
{
	c_memmove(uuid->data, buffer, length);
	uuid->len = length;
}

static void addressToBuffer(bd_addr *bda, uint8_t *buffer)
{
	for (uint8_t i = 0; i < 6; ++i)
		buffer[i] = bda->addr[5 - i];
}

static void bufferToAddress(uint8_t *buffer, bd_addr *bda)
{
	for (uint8_t i = 0; i < 6; ++i)
		bda->addr[i] = buffer[5 - i];
}

void bleTimerCallback(modTimer timer, void *refcon, int refconSize)
{
    struct gecko_cmd_packet* evt = gecko_peek_event();
    ble_event_handler(evt);
}

static void systemBootEvent(struct gecko_msg_system_boot_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	gecko_cmd_gatt_set_max_mtu(DEFAULT_MTU);
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
	xsEndHost(gBLE->the);
}

static void leGapScanResponseEvent(struct gecko_msg_le_gap_scan_response_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	uint8_t addr[6];
	xsmcVars(3);
	addressToBuffer(&evt->address, addr);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), evt->data.data, evt->data.len);
	xsmcSetArrayBuffer(xsVar(2), addr, 6);
	xsmcSet(xsVar(0), xsID_scanResponse, xsVar(1));
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onDiscovered"), xsVar(0));
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
		goto bail;
	}
	connection->id = evt->connection;
	connection->bond = evt->bonding;
	
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
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
		
	// ignore multiple disconnects on same connection
	if (!connection) {
		goto bail;
	}	
	
	xsCall1(connection->objConnection, xsID_callback, xsString("onDisconnected"));
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
	xsCall2(connection->objConnection, xsID_callback, xsString("onRSSI"), xsInteger(((int16_t)evt->rssi)));
	xsEndHost(gBLE->the);
}

static void gattServiceEvent(struct gecko_msg_gatt_service_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	uint16_t start, end, buffer_length;
	uint8_t buffer[16];
	uuidToBuffer(&evt->uuid, buffer, &buffer_length);
	start = ((evt->service >> 16) & 0xFFFF);
	end = (evt->service & 0xFFFF);
	xsmcVars(4);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), buffer, buffer_length);
	xsmcSetInteger(xsVar(2), start);
	xsmcSetInteger(xsVar(3), end);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSet(xsVar(0), xsID_start, xsVar(2));
	xsmcSet(xsVar(0), xsID_end, xsVar(3));
	xsCall2(connection->procedureQueue->obj, xsID_callback, xsString("onService"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattCharacteristicEvent(struct gecko_msg_gatt_characteristic_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(4);
	uint8_t buffer[16];
	uint16_t buffer_length;
	uuidToBuffer(&evt->uuid, buffer, &buffer_length);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), buffer, buffer_length);
	xsmcSetInteger(xsVar(2), evt->characteristic);
	xsmcSetInteger(xsVar(3), evt->properties);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsmcSet(xsVar(0), xsID_properties, xsVar(3));
	xsCall2(connection->procedureQueue->obj, xsID_callback, xsString("onCharacteristic"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattDescriptorEvent(struct gecko_msg_gatt_descriptor_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(3);
	uint8_t buffer[16];
	uint16_t buffer_length;
	uuidToBuffer(&evt->uuid, buffer, &buffer_length);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), buffer, buffer_length);
	xsmcSetInteger(xsVar(2), evt->descriptor);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->procedureQueue->obj, xsID_callback, xsString("onDescriptor"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattDescriptorValueEvent(struct gecko_msg_gatt_descriptor_value_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), evt->value.data, evt->value.len);
	xsmcSetInteger(xsVar(2), evt->descriptor);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->procedureQueue->obj, xsID_callback, xsString("onDescriptorValue"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattCharacteristicValueEvent(struct gecko_msg_gatt_characteristic_value_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), evt->value.data, evt->value.len);
	xsmcSetInteger(xsVar(2), evt->characteristic);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->procedureQueue->obj, xsID_callback, xsString("onCharacteristicValue"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattCharacteristicNotifyEvent(struct gecko_msg_gatt_characteristic_value_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), evt->value.data, evt->value.len);
	xsmcSetInteger(xsVar(2), evt->characteristic);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->objClient, xsID_callback, xsString("onCharacteristicNotification"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattProcedureCompletedEvent(struct gecko_msg_gatt_procedure_completed_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	gattProcedure procedure;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	//xsTrace("in gattProcedureCompletedEvent\n");
	if (!connection)
		xsUnknownError("connection not found");
	if (0 != evt->result)
		xsUnknownError("procedure completed error");
	procedure = connection->procedureQueue;
	connection->procedureQueue = connection->procedureQueue->next;
	switch(procedure->cmd) {
		case CMD_GATT_DISCOVER_SERVICES_ID:
		case CMD_GATT_DISCOVER_SERVICES_UUID_ID:
			xsCall1(procedure->obj, xsID_callback, xsString("onService"));
			break;
		case CMD_GATT_DISCOVER_CHARACTERISTICS_ID:
		case CMD_GATT_DISCOVER_CHARACTERISTICS_UUID_ID:
			xsCall1(procedure->obj, xsID_callback, xsString("onCharacteristic"));
			break;
		case CMD_GATT_DISCOVER_DESCRIPTORS_ID:
			xsCall1(procedure->obj, xsID_callback, xsString("onDescriptor"));
			break;
		default:
			break;
	}
	c_free(procedure);
		
	// execute all queued procedures that don't generate completion events
	while ((NULL != connection->procedureQueue) && !connection->procedureQueue->executed) {
		procedure = connection->procedureQueue;
		gattProcedureExecute(procedure);
		if (xsUndefinedType == xsmcTypeOf(procedure->obj)) {
			connection->procedureQueue = procedure->next;
			c_free(procedure);
		}
	}
	
	xsEndHost(gBLE->the);
}

static void smBondingFailedEvent(struct gecko_msg_sm_bonding_failed_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	switch(evt->reason) {
		case bg_err_smp_pairing_not_supported:
		case bg_err_bt_pin_or_key_missing:
			if (0xFF != connection->bond) {
				gecko_cmd_sm_delete_bonding(connection->bond);	// remove bond and try again
				connection->bond = 0xFF;
				gecko_cmd_sm_increase_security(connection->id);
			}
			break;
		default:
			break;
	}
	xsEndHost(gBLE->the);
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
		case gecko_evt_le_connection_rssi_id:
			leConnectionRSSIEvent(&evt->data.evt_le_connection_rssi);
			break;
		case gecko_evt_le_gap_scan_response_id:
			if (0 != evt->data.evt_le_gap_scan_response.data.len)
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
		case gecko_evt_gatt_descriptor_value_id:
			gattDescriptorValueEvent(&evt->data.evt_gatt_descriptor_value);
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
		case gecko_evt_sm_bonding_failed_id:
			smBondingFailedEvent(&evt->data.evt_sm_bonding_failed);
			break;
		default:
			break;
	}
}

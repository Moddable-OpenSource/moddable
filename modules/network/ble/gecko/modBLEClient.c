/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
#include "modBLECommon.h"
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

#include "mc.bleservices.c"

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
#define OBJ_CLIENT(_c) ((modBLEClientConnection)_c)->objClient

typedef struct {
  uint8 len;
  uint8 data[16];
} uuidRecord;

typedef struct {
  uint8_t addr[6];
  uint8_t addrType;
} bondingRemoveAddressRecord, *bondingRemoveAddress;

typedef struct gattProcedureRecord gattProcedureRecord;
typedef gattProcedureRecord *gattProcedure;

struct gattProcedureRecord {
	uint8_t connection;
	uint8_t flag;
	uint32_t cmd;
	uint32_t refcon;
	xsSlot target;
};

typedef struct modBLEClientConnectionRecord modBLEClientConnectionRecord;
typedef modBLEClientConnectionRecord *modBLEClientConnection;

struct modBLEClientConnectionRecord {
	modBLEConnectionPart;
	
	xsSlot objClient;

	uint8_t bond;
	gattProcedureRecord procedure;
	uint16_t handles[char_name_count];
};

typedef struct modBLEScannedPacketRecord modBLEScannedPacketRecord;
typedef modBLEScannedPacketRecord *modBLEScannedPacket;

struct modBLEScannedPacketRecord {
	struct modBLEScannedPacketRecord *next;

	uint8_t packet_type;
	uint8_t addr[6];
};

typedef struct {
	xsMachine *the;
	xsSlot obj;

	modTimer timer;

	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;
	
	// scanning
	uint8_t duplicates;
	uint8_t useWhitelist;
	modBLEScannedPacket scanned;
	
	bondingRemoveAddress bondingToRemove;
} modBLERecord, *modBLE;

static void uuidToBuffer(uint8array *uuid, uint8_t *buffer, uint16_t *length);
static void bufferToUUID(uint8_t *buffer, uuidRecord *uuid, uint16_t length);
static void bleTimerCallback(modTimer timer, void *refcon, int refconSize);
static void clearScanned(modBLE ble);
static void ble_event_handler(struct gecko_cmd_packet* evt);
static void setGATTProcedure(modBLEConnection connection, uint32_t cmd, xsSlot target, uint32_t refcon);

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
	gBLE->duplicates = true;
	xsRemember(gBLE->obj);
	
	// Initialize platform Bluetooth modules
	gecko_stack_init(&config);
	gecko_bgapi_class_system_init();
	gecko_bgapi_class_le_gap_init();
	gecko_bgapi_class_le_connection_init();
	gecko_bgapi_class_gatt_init();
	//gecko_bgapi_class_sm_init();

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
	if (!ble) return;
	
	modBLEConnection connection = modBLEConnectionGetFirst();
	while (connection != NULL) {
		modBLEConnection next = connection->next;
		if (kBLEConnectionTypeClient == connection->type) {
			xsMachine *the = connection->the;
			xsForget(connection->objConnection);
			modBLEConnectionRemove(connection);
		}
		connection = next;
	}
	
	modTimerRemove(ble->timer);
	clearScanned(ble);
	modBLEWhitelistClear();
	if (ble->bondingToRemove)
		c_free(ble->bondingToRemove);
	c_free(ble);
	gBLE = NULL;
}

void xs_ble_client_set_local_privacy(xsMachine *the)
{
}

void xs_ble_client_start_scanning(xsMachine *the)
{
	uint8_t active = xsmcToBoolean(xsArg(0));
	uint8_t duplicates = xsmcToBoolean(xsArg(1));
	uint32_t interval = xsmcToInteger(xsArg(2));
	uint32_t window = xsmcToInteger(xsArg(3));
	uint16_t filterPolicy = xsmcToInteger(xsArg(4));
	
	gBLE->duplicates = duplicates;
	gBLE->useWhitelist = (kBLEScanFilterPolicyWhitelist == filterPolicy || kBLEScanFilterWhitelistNotResolvedDirected == filterPolicy);
		
	gecko_cmd_le_gap_set_scan_parameters(interval, window, active ? 1 : 0);
   	gecko_cmd_le_gap_discover(le_gap_discover_generic);
}

void xs_ble_client_stop_scanning(xsMachine *the)
{
	gecko_cmd_le_gap_end_procedure();
	clearScanned(gBLE);
}

void xs_ble_client_connect(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t addressType = xsmcToInteger(xsArg(1));
	bd_addr bda;

	// Ignore duplicate connection attempts
	if (modBLEConnectionFindByAddress(address)) return;
	
	c_memmove(bda.addr, address, 6);
		
	// Add a new connection record to be filled as the connection completes
	modBLEClientConnection connection = c_calloc(sizeof(modBLEClientConnectionRecord), 1);
	if (!connection)
		xsUnknownError("out of memory");
	connection->id = kInvalidConnectionID;
	c_memmove(&connection->address, address, 6);
	connection->addressType = addressType;
	connection->bond = 0xFF;
	modBLEConnectionAdd((modBLEConnection)connection);
	
	gecko_cmd_le_gap_open(bda, addressType);
}

void xs_ble_client_set_security_parameters(xsMachine *the)
{
	uint8_t encryption = xsmcToBoolean(xsArg(0));
	uint8_t bonding = xsmcToBoolean(xsArg(1));
	uint8_t mitm = xsmcToBoolean(xsArg(2));
	uint16_t ioCapability = xsmcToInteger(xsArg(3));
	uint16_t err;
	
	gBLE->encryption = encryption;
	gBLE->bonding = bonding;
	gBLE->mitm = mitm;

	err = modBLESetSecurityParameters(encryption, bonding, mitm, ioCapability);
	if (0 != err)
		xsUnknownError("invalid security params");

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
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t confirm = xsmcToBoolean(xsArg(1));
	
	modBLEConnection connection = modBLEConnectionFindByAddress(address);
	if (!connection) return;
	
	gecko_cmd_sm_passkey_confirm(connection->id, confirm);
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
	OBJ_CLIENT(connection) = xsArg(0);
	xsRemember(connection->objConnection);
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

void xs_gap_connection_exchange_mtu(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t mtu = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	connection->mtu_exchange_pending = 1;
	gecko_cmd_gatt_set_max_mtu(mtu);
}

void xs_gatt_client_discover_primary_services(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t argc = xsmcArgc;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	setGATTProcedure(connection, argc > 1 ? gecko_cmd_gatt_discover_primary_services_by_uuid_id : gecko_cmd_gatt_discover_primary_services_id, xsThis, 0);
	if (argc > 1) {
		uuidRecord uuid;
		bufferToUUID((uint8_t*)xsmcToArrayBuffer(xsArg(1)), &uuid, xsmcGetArrayBufferLength(xsArg(1)));
		gecko_cmd_gatt_discover_primary_services_by_uuid(conn_id, uuid.len, (uint8_t*)&uuid.data);
	}
	else {
		gecko_cmd_gatt_discover_primary_services(conn_id);
	}
}

void xs_gatt_service_discover_characteristics(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t start = xsmcToInteger(xsArg(1));
	uint16_t end = xsmcToInteger(xsArg(2));
	uint32_t service = ((uint32_t)start << 16) | end;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	
	setGATTProcedure(connection, argc > 3 ? gecko_cmd_gatt_discover_characteristics_by_uuid_id : gecko_cmd_gatt_discover_characteristics_id, xsThis, 0);
	if (argc > 3) {
		uuidRecord uuid;
		bufferToUUID((uint8_t*)xsmcToArrayBuffer(xsArg(3)), &uuid, xsmcGetArrayBufferLength(xsArg(3)));
		gecko_cmd_gatt_discover_characteristics_by_uuid(conn_id, service, uuid.len, (uint8_t*)&uuid.data);
	}
	else {
		gecko_cmd_gatt_discover_characteristics(conn_id, service);
	}
}

void xs_gatt_characteristic_discover_all_characteristic_descriptors(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	
	setGATTProcedure(connection, gecko_cmd_gatt_discover_descriptors_id, xsThis, 0);
	gecko_cmd_gatt_discover_descriptors(conn_id, characteristic);
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
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	
	setGATTProcedure(connection, gecko_cmd_gatt_read_characteristic_value_id, OBJ_CLIENT(connection), 0);
	gecko_cmd_gatt_read_characteristic_value(conn_id, handle);
}

void xs_gatt_characteristic_enable_notifications(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint32_t characteristic = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	setGATTProcedure(connection, gecko_cmd_gatt_set_characteristic_notification_id, xsThis, (characteristic << 16) | gatt_notification);
	gecko_cmd_gatt_set_characteristic_notification(conn_id, characteristic, gatt_notification);
}

void xs_gatt_characteristic_disable_notifications(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint32_t characteristic = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	setGATTProcedure(connection, gecko_cmd_gatt_set_characteristic_notification_id, xsThis, (characteristic << 16) | gatt_disable);
	gecko_cmd_gatt_set_characteristic_notification(conn_id, characteristic, gatt_disable);
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
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	setGATTProcedure(connection, gecko_cmd_gatt_read_descriptor_value_id, OBJ_CLIENT(connection), 0);
	gecko_cmd_gatt_read_descriptor_value(conn_id, handle);
}

void xs_gatt_descriptor_write_value(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	
	switch (xsmcTypeOf(xsArg(2))) {
		case xsStringType: {
			char *str = xsmcToString(xsArg(2));
			setGATTProcedure(connection, gecko_cmd_gatt_write_descriptor_value_id, xsThis, 0);
			gecko_cmd_gatt_write_descriptor_value(conn_id, handle, c_strlen(str), str);
			break;
		}
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype)) {
				setGATTProcedure(connection, gecko_cmd_gatt_write_descriptor_value_id, xsThis, 0);
				gecko_cmd_gatt_write_descriptor_value(conn_id, handle, xsmcGetArrayBufferLength(xsArg(2)), (uint8_t*)xsmcToArrayBuffer(xsArg(2)));
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
	
	xsSlot slot;
	xsmcSetUndefined(slot);

	switch (xsmcTypeOf(xsArg(2))) {
		case xsStringType: {
			char *str = xsmcToString(xsArg(2));
			setGATTProcedure(connection, gecko_cmd_gatt_write_characteristic_value_without_response_id, slot, 0);
			gecko_cmd_gatt_write_characteristic_value_without_response(conn_id, handle, c_strlen(str), str);
			break;
		}
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype)) {
				setGATTProcedure(connection, gecko_cmd_gatt_write_characteristic_value_without_response_id, slot, 0);
				gecko_cmd_gatt_write_characteristic_value_without_response(conn_id, handle, xsmcGetArrayBufferLength(xsArg(2)), (uint8_t*)xsmcToArrayBuffer(xsArg(2)));
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

static void clearScanned(modBLE ble)
{
	modBLEScannedPacket walker = ble->scanned;
	while (walker) {
		modBLEScannedPacket next = walker->next;
		c_free(walker);
		walker = next;
	}
	ble->scanned = NULL;
}

static int modBLEConnectionSaveAttHandle(modBLEClientConnection connection, uint8_t *uuid, uint16_t uuid_length, uint16_t handle)
{
	int result = -1;
	for (int i = 0; i < char_name_count; ++i) {
		if (uuid_length == char_names[i].uuid_length) {
			if (0 == c_memcmp(char_names[i].uuid, uuid, uuid_length)) {
				connection->handles[i] = handle;
				result = i;
				goto bail;
			}
		}
	}

bail:
	return result;
}

void modBLEClientBondingRemove(char *address, uint8_t addressType)
{
	if (!gBLE) return;

	gBLE->bondingToRemove = c_malloc(sizeof(bondingRemoveAddressRecord));
	if (gBLE->bondingToRemove) {
		gBLE->bondingToRemove->addrType = addressType;
		c_memmove(gBLE->bondingToRemove->addr, address, 6);
		gecko_cmd_sm_list_all_bondings();
	}
}

void setGATTProcedure(modBLEConnection connection, uint32_t cmd, xsSlot target, uint32_t refcon)
{
	gattProcedure procedure = &((modBLEClientConnection)connection)->procedure;
	procedure->connection = connection->id;
	procedure->cmd = cmd;
	procedure->target = target;
	procedure->refcon = refcon;
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

	if (gBLE->useWhitelist && !modBLEWhitelistContains(evt->address_type, evt->address.addr))
		goto bail;
	
	if (!gBLE->duplicates) {
		modBLEScannedPacket scanned = gBLE->scanned;
		while (scanned) {
			if (evt->packet_type == scanned->packet_type && 0 == c_memcmp(evt->address.addr, scanned->addr, 6))
				goto bail;
			scanned = scanned->next;
		}
		modBLEScannedPacket address = c_calloc(1, sizeof(modBLEScannedPacketRecord));
		if (!address)
			xsUnknownError("out of memory");
		address->packet_type = evt->packet_type;
		c_memmove(address->addr, evt->address.addr, 6);
		if (!gBLE->scanned)
			gBLE->scanned = address;
		else {
			modBLEScannedPacket walker;
			for (walker = gBLE->scanned; walker->next; walker = walker->next)
				;
			walker->next = address;
		}
	}
	
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), evt->data.data, evt->data.len);
	xsmcSet(xsVar(0), xsID_scanResponse, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), evt->address.addr, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), evt->address_type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsmcSetInteger(xsVar(1), evt->rssi);
	xsmcSet(xsVar(0), xsID_rssi, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onDiscovered"), xsVar(0));
	
bail:
	xsEndHost(gBLE->the);
}

static void leConnectionOpenedEvent(struct gecko_msg_le_connection_opened_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByAddress(evt->address.addr);
	if (!connection)
		xsUnknownError("connection not found");
		
	// Ignore duplicate connection events
	if (kInvalidConnectionID != connection->id) goto bail;

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
	xsmcSetInteger(xsVar(1), evt->address_type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
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
	
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), evt->connection);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), connection->address, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), connection->addressType);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(connection->objConnection, xsID_callback, xsString("onDisconnected"), xsVar(0));
	xsForget(connection->objConnection);

	modBLEConnectionRemove(connection);
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
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(evt->connection);
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
	xsCall2(connection->procedure.target, xsID_callback, xsString("onService"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattCharacteristicEvent(struct gecko_msg_gatt_characteristic_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	int index = modBLEConnectionSaveAttHandle(connection, evt->uuid.data, evt->uuid.len, evt->characteristic);
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
	if (-1 != index) {
		xsmcSetString(xsVar(2), (char*)char_names[index].name);
		xsmcSet(xsVar(0), xsID_name, xsVar(2));
		xsmcSetString(xsVar(2), (char*)char_names[index].type);
		xsmcSet(xsVar(0), xsID_type, xsVar(2));
	}
	xsCall2(connection->procedure.target, xsID_callback, xsString("onCharacteristic"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattDescriptorEvent(struct gecko_msg_gatt_descriptor_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	int index = modBLEConnectionSaveAttHandle(connection, evt->uuid.data, evt->uuid.len, evt->descriptor);
	xsmcVars(3);
	uint8_t buffer[16];
	uint16_t buffer_length;
	uuidToBuffer(&evt->uuid, buffer, &buffer_length);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), buffer, buffer_length);
	xsmcSetInteger(xsVar(2), evt->descriptor);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	if (-1 != index) {
		xsmcSetString(xsVar(2), (char*)char_names[index].name);
		xsmcSet(xsVar(0), xsID_name, xsVar(2));
		xsmcSetString(xsVar(2), (char*)char_names[index].type);
		xsmcSet(xsVar(0), xsID_type, xsVar(2));
	}
	xsCall2(connection->procedure.target, xsID_callback, xsString("onDescriptor"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattDescriptorValueEvent(struct gecko_msg_gatt_descriptor_value_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), evt->value.data, evt->value.len);
	xsmcSetInteger(xsVar(2), evt->descriptor);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->procedure.target, xsID_callback, xsString("onDescriptorValue"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattCharacteristicValueEvent(struct gecko_msg_gatt_characteristic_value_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), evt->value.data, evt->value.len);
	xsmcSetInteger(xsVar(2), evt->characteristic);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->procedure.target, xsID_callback, xsString("onCharacteristicValue"), xsVar(0));
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
	xsCall2(OBJ_CLIENT(connection), xsID_callback, xsString("onCharacteristicNotification"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattProcedureCompletedEvent(struct gecko_msg_gatt_procedure_completed_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	gattProcedure procedure;
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	if (0 != evt->result)
		xsUnknownError("procedure completed error");
	switch(connection->procedure.cmd) {
		case gecko_cmd_gatt_discover_primary_services_id:
		case gecko_cmd_gatt_discover_primary_services_by_uuid_id:
			xsCall1(connection->procedure.target, xsID_callback, xsString("onService"));
			break;
		case gecko_cmd_gatt_discover_characteristics_id:
		case gecko_cmd_gatt_discover_characteristics_by_uuid_id:
			xsCall1(connection->procedure.target, xsID_callback, xsString("onCharacteristic"));
			break;
		case gecko_cmd_gatt_discover_descriptors_id:
			xsCall1(connection->procedure.target, xsID_callback, xsString("onDescriptor"));
			break;
		case gecko_cmd_gatt_set_characteristic_notification_id: {
			uint32_t characteristic = connection->procedure.refcon >> 16;
			uint8_t flag = connection->procedure.refcon & 0x3;
			xsmcVars(2);
			xsVar(0) = xsmcNewObject();
			xsmcSetInteger(xsVar(1), characteristic);
			xsmcSet(xsVar(0), xsID_handle, xsVar(1));
			xsCall2(connection->procedure.target, xsID_callback,
				flag == gatt_notification ? xsString("onCharacteristicNotificationEnabled") : xsString("onCharacteristicNotificationDisabled"),
				xsVar(0));
			break;
		}
		default:
			break;
	}
	
	xsEndHost(gBLE->the);
}

static void smBondingFailedEvent(struct gecko_msg_sm_bonding_failed_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(evt->connection);
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

static void smListBondingEntryEvent(struct gecko_msg_sm_list_bonding_entry_evt_t *evt)
{
	if (gBLE->bondingToRemove && (evt->address_type == gBLE->bondingToRemove->addrType) && (0 == c_memcmp(&evt->address.addr, gBLE->bondingToRemove->addr, 6))) {
		gecko_cmd_sm_delete_bonding(evt->bonding);
		xsBeginHost(gBLE->the);
		xsmcVars(2);
		xsVar(0) = xsmcNewObject();
		xsmcSetArrayBuffer(xsVar(1), evt->address.addr, 6);
		xsmcSet(xsVar(0), xsID_address, xsVar(1));
		xsmcSetInteger(xsVar(1), evt->address_type);
		xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
		xsCall2(gBLE->obj, xsID_callback, xsString("onBondingDeleted"), xsVar(0));
		xsEndHost(gBLE->the);
	}
}

static void smListAllBondingsCompleteEvent(void)
{
	if (gBLE->bondingToRemove) {
		c_free(gBLE->bondingToRemove);
		gBLE->bondingToRemove = NULL;
	}
}

static void smBondedEvent(struct gecko_msg_sm_bonded_evt_t *evt)
{
	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection)
		xsUnknownError("connection not found");
	connection->bond = evt->bonding;
	if (0xFF != connection->bond) {
		xsmcVars(2);
		xsVar(0) = xsmcNewObject();
		xsmcSetBoolean(xsVar(1), 0xFF != connection->bond);
		xsmcSet(xsVar(0), xsID_bonded, xsVar(1));
		xsCall2(gBLE->obj, xsID_callback, xsString("onAuthenticated"), xsVar(0));
	}
	xsEndHost(gBLE->the);
}

static void gattMTUExchangedEvent(struct gecko_msg_gatt_mtu_exchanged_evt_t *evt)
{
	modBLEConnection connection = modBLEConnectionFindByConnectionID(evt->connection);
	if (!connection || !connection->mtu_exchange_pending) return;
	
	connection->mtu_exchange_pending = 0;
	xsBeginHost(gBLE->the);
		xsCall2(connection->objConnection, xsID_callback, xsString("onMTUExchanged"), xsInteger(evt->mtu));
	xsEndHost(gBLE->the);
}

void ble_event_handler(struct gecko_cmd_packet* evt)
{
	if (!gBLE) return;
	
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
		case gecko_evt_gatt_mtu_exchanged_id:
			gattMTUExchangedEvent(&evt->data.evt_gatt_mtu_exchanged);
			break;
		case gecko_evt_sm_list_bonding_entry_id:
			smListBondingEntryEvent(&evt->data.evt_sm_list_bonding_entry);
			break;
		case gecko_evt_sm_list_all_bondings_complete_id:
			smListAllBondingsCompleteEvent();
			break;
		case gecko_evt_sm_bonded_id:
			smBondedEvent(&evt->data.evt_sm_bonded);
			break;
		default:
			break;
	}
}

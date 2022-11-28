/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
#include "modBLE.h"
#include "modBLECommon.h"

#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "esp_nimble_hci.h"

#include "mc.bleservices.c"

#define LOG_GAP 0
#if LOG_GAP
	#define LOG_GAP_EVENT(event) logGAPEvent(event)
	#define LOG_GAP_MSG(msg) modLog(msg)
	#define LOG_GAP_INT(i) modLogInt(i)
#else
	#define LOG_GAP_EVENT(event)
	#define LOG_GAP_MSG(msg)
	#define LOG_GAP_INT(i)
#endif

typedef struct modBLEClientConnectionRecord modBLEClientConnectionRecord;
typedef modBLEClientConnectionRecord *modBLEClientConnection;

struct modBLEClientConnectionRecord {
	modBLEConnectionPart;
	
	xsSlot objClient;
};

typedef struct {
	xsMachine *the;
	xsSlot obj;
	
	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;
	uint8_t iocap;

	uint8_t terminating;
	
	modBLEMessageQueueRecord discoveryQueue;
	
	modBLEMessageQueueRecord notificationQueue;
} modBLERecord, *modBLE;

typedef struct {
	uint16_t conn_id;
	uint16_t handle;
	uint16_t length;
	uint8_t isCharacteristic;
	uint8_t data[1];
} attributeReadDataRecord, *attributeReadData;

typedef struct {
	uint16_t conn_id;
	xsSlot obj;
} attributeSearchRecord;

typedef struct {
	uint16_t conn_id;
	xsSlot obj;
	uint16_t characteristicStartHandle;
	uint16_t serviceStartHandle;
	uint16_t serviceEndHandle;
	uint16_t characteristicEndHandle;
} descriptorHandleSearchRecord;

typedef struct {
	uint8_t enable;
	uint16_t conn_id;
	uint16_t handle;
} characteristicNotificationEnabledRecord, *characteristicNotificationEnabled;

typedef struct {
	uint16_t conn_id;
	uint16_t mtu;
} mtuExchangedRecord;

typedef struct {
	modBLEMessageQueueEntryPart;
	struct ble_gatt_svc service;
	uint8_t completed;
} serviceSearchRecord;

typedef struct {
	modBLEMessageQueueEntryPart;
	struct ble_gatt_chr characteristic;
	uint8_t completed;
} characteristicSearchRecord;

typedef struct {
	modBLEMessageQueueEntryPart;
	struct ble_gatt_dsc descriptor;
	uint8_t completed;
} descriptorSearchRecord;

typedef struct {
	modBLEMessageQueueEntryPart;
	struct ble_gap_disc_desc disc;
	uint8_t data[1];
} deviceDiscoveryRecord;

typedef struct {
	modBLEMessageQueueEntryPart;
	uint8_t isNotification;
	uint16_t handle;
	uint16_t length;
	uint8_t data[1];
} attributeNotificationRecord, *attributeNotification;

static void uuidToBuffer(uint8_t *buffer, ble_uuid_any_t *uuid, uint16_t *length);
static void bufferToUUID(ble_uuid_any_t *uuid, uint8_t *buffer, uint16_t length);

static void nimble_on_sync(void);

static int nimble_gap_event(struct ble_gap_event *event, void *arg);
static int nimble_service_event(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_svc *service, void *arg);
static int nimble_characteristic_event(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *chr, void *arg);
static int nimble_descriptor_event(uint16_t conn_handle, const struct ble_gatt_error *error, uint16_t chr_def_handle, const struct ble_gatt_dsc *dsc, void *arg);
static int nimble_read_event(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg);
static int nimble_subscribe_event(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg);
static int nimble_mtu_event(uint16_t conn_handle, const struct ble_gatt_error *error, uint16_t mtu, void *arg);

static void onConnected(struct ble_gap_conn_desc *desc);
static void connectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void scanResultEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void serviceDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void characteristicDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void descriptorDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void characteristicHandleDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void notificationEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void bondingRemovedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void setSecurityParametersEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void logGAPEvent(struct ble_gap_event *event);

static modBLE gBLE = NULL;

void xs_ble_client_initialize(xsMachine *the)
{
	if (NULL != gBLE)
		xsUnknownError("BLE already initialized");
	gBLE = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!gBLE)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, gBLE);
	gBLE->the = the;
	gBLE->obj = xsThis;
	xsRemember(gBLE->obj);
	
	modBLEMessageQueueConfigure(&gBLE->notificationQueue, the, notificationEvent, NULL);

	ble_hs_cfg.sync_cb = nimble_on_sync;

	esp_err_t err = modBLEPlatformInitialize();
	if (ESP_OK != err)
		xsUnknownError("ble initialization failed");
}

void xs_ble_client_close(xsMachine *the)
{
	modBLE *ble = xsmcGetHostData(xsThis);
	if (!ble) return;
	
	xsForget(gBLE->obj);
	xs_ble_client_destructor(gBLE);
	xsmcSetHostData(xsThis, NULL);
}

void xs_ble_client_destructor(void *data)
{
	modBLE ble = data;
	if (!ble) return;
	
	ble->terminating = true;
	modBLEConnection connection = modBLEConnectionGetFirst();
	while (connection != NULL) {
		modBLEConnection next = modBLEConnectionGetNext(connection);
		if (kBLEConnectionTypeClient == connection->type) {
			xsMachine *the = connection->the;
			ble_gap_terminate(connection->id, BLE_ERR_REM_USER_CONN_TERM);
			if (the)
				xsForget(connection->objConnection);
			modBLEConnectionRemove(connection);
		}
		connection = next;
	}
	modBLEMessageQueueEmpty(&ble->discoveryQueue);
	modBLEMessageQueueEmpty(&ble->notificationQueue);
	c_free(ble);
	gBLE = NULL;

	modBLEPlatformTerminate();
}

void xs_ble_client_set_local_privacy(xsMachine *the)
{
	uint8_t enable = xsmcToBoolean(xsArg(0));
	if (enable) {
    	ble_addr_t addr;
    	// 1 == non-resolvable random private address, 0 == static random address
		ble_hs_id_gen_rnd(1, &addr);
		ble_hs_id_set_rnd(addr.val);
	}
	else {
		// @@
	}
}

void xs_ble_client_start_scanning(xsMachine *the)
{
	uint8_t active = xsmcToBoolean(xsArg(0));
	uint8_t duplicates = xsmcToBoolean(xsArg(1));
	uint32_t interval = xsmcToInteger(xsArg(2));
	uint32_t window = xsmcToInteger(xsArg(3));
	uint16_t filterPolicy = xsmcToInteger(xsArg(4));
	uint8_t own_addr_type;
	struct ble_gap_disc_params disc_params;

	switch(filterPolicy) {
		case kBLEScanFilterPolicyWhitelist:
			filterPolicy = BLE_HCI_SCAN_FILT_USE_WL;
			break;
		case kBLEScanFilterNotResolvedDirected:
			filterPolicy = BLE_HCI_SCAN_FILT_NO_WL_INITA;
			break;
		case kBLEScanFilterWhitelistNotResolvedDirected:
			filterPolicy = BLE_HCI_SCAN_FILT_USE_WL_INITA;
			break;
		default:
			filterPolicy = BLE_HCI_SCAN_FILT_NO_WL;
			break;
	}

	ble_hs_id_infer_auto(0, &own_addr_type);

	c_memset(&disc_params, 0, sizeof(disc_params));
	disc_params.passive = !active;
	disc_params.filter_duplicates = !duplicates;
	disc_params.itvl = interval;
	disc_params.window = window;
	disc_params.filter_policy = filterPolicy;

	modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, scanResultEvent, NULL);
	ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params, nimble_gap_event, NULL);
 }

void xs_ble_client_stop_scanning(xsMachine *the)
{
	ble_gap_disc_cancel();
}

void xs_ble_client_connect(xsMachine *the)
{
	ble_addr_t addr;
	
	addr.type = xsmcToInteger(xsArg(1));;
	xsmcGetArrayBufferData(xsArg(0), 0, &addr.val, 6);

// Ignore duplicate connection attempts
	if (ble_gap_conn_active()) return;
	
	ble_gap_disc_cancel();

	// Check if there has already been a connection established with this peer.
	// This can happen if a BLEServer instance connects prior to the BLEClient instance.
	// If so, skip the connection request below.
	struct ble_gap_conn_desc desc;
	if (0 == ble_gap_conn_find_by_addr(&addr, &desc)) {
		onConnected(&desc);
		return;
	}

	// Add a new connection record to be filled as the connection completes
	modBLEClientConnection connection = c_calloc(sizeof(modBLEClientConnectionRecord), 1);
	if (!connection)
		xsUnknownError("no memory");
	connection->id = kInvalidConnectionID;
	connection->type = kBLEConnectionTypeClient;
	connection->addressType = addr.type;
	c_memmove(connection->address, addr.val, 6);
	modBLEConnectionAdd((modBLEConnection)connection);

	ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &addr, BLE_HS_FOREVER, NULL, nimble_gap_event, NULL);
}

void modBLEClientBondingRemoved(char *address, uint8_t addressType)
{
	ble_addr_t addr;
	
	if (!gBLE) return;
	
	addr.type = addressType;
	c_memmove(addr.val, address, 6);
	modMessagePostToMachine(gBLE->the, (void*)&addr, sizeof(addr), bondingRemovedEvent, NULL);
}

void bondingRemovedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	ble_addr_t *addr = (ble_addr_t *)message;

	if (!gBLE) return;

	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsmcSetNewObject(xsVar(0));
	xsmcSetArrayBuffer(xsVar(1), addr->val, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), addr->type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsStringX("onBondingDeleted"), xsVar(0));
	xsEndHost(gBLE->the);
}

void setSecurityParametersEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsmcSetNewObject(xsVar(0));
	xsmcSetBoolean(xsVar(1), gBLE->encryption);
	xsmcSet(xsVar(0), xsID_encryption, xsVar(1));
	xsmcSetBoolean(xsVar(1), gBLE->bonding);
	xsmcSet(xsVar(0), xsID_bonding, xsVar(1));
	xsmcSetBoolean(xsVar(1), gBLE->mitm);
	xsmcSet(xsVar(0), xsID_mitm, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsStringX("onSecurityParameters"), xsVar(0));
	xsEndHost(gBLE->the);
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
	gBLE->iocap = ioCapability;

	modBLESetSecurityParameters(encryption, bonding, mitm, ioCapability);

	if (mitm) {	// generate random address
		ble_addr_t addr;
		ble_hs_id_gen_rnd(1, &addr);
		ble_hs_id_set_rnd(addr.val);
		modMessagePostToMachine(gBLE->the, NULL, 0, setSecurityParametersEvent, NULL);
	}
}

void xs_ble_client_passkey_reply(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	modBLEConnection connection;
	
	connection = modBLEConnectionFindByAddress(address);
	if (!connection)
		xsUnknownError("connection not found");
	struct ble_sm_io pkey = {0};
	pkey.action = BLE_SM_IOACT_NUMCMP;
	pkey.numcmp_accept = xsmcToBoolean(xsArg(1));
	ble_sm_inject_io(connection->id, &pkey);
}

static void readyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsBeginHost(gBLE->the);
	xsCall1(gBLE->obj, xsID_callback, xsStringX("onReady"));
	xsEndHost(gBLE->the);
}

void ble_client_on_reset(int reason)
{
	if (gBLE)
		xs_ble_client_close(gBLE->the);
}

void nimble_on_sync(void)
{
	ble_hs_util_ensure_addr(0);
	modMessagePostToMachine(gBLE->the, NULL, 0, readyEvent, NULL);
}

void xs_gap_connection_initialize(xsMachine *the)
{
	uint16_t conn_id;
	xsmcVars(1);	// xsArg(0) is client
	xsmcGet(xsVar(0), xsArg(0), xsID_connection);
	conn_id = xsmcToInteger(xsVar(0));
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	connection->the = the;
	connection->objConnection = xsThis;
	connection->objClient = xsArg(0);
	xsRemember(connection->objConnection);
}
	
void xs_gap_connection_disconnect(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	ble_gap_terminate(conn_id, BLE_ERR_REM_USER_CONN_TERM);
}

void xs_gap_connection_read_rssi(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	int8_t rssi;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	if (0 == ble_gap_conn_rssi(conn_id, &rssi)) {
		xsBeginHost(the);
		xsCall2(connection->objConnection, xsID_callback, xsStringX("onRSSI"), xsInteger(rssi));
		xsEndHost(the);
	}
}

void xs_gap_connection_exchange_mtu(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t mtu = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	if (0 != ble_att_set_preferred_mtu(mtu))
		xsRangeError("invalid mtu");
	ble_gattc_exchange_mtu(conn_id, nimble_mtu_event, NULL);
}

void xs_gatt_client_discover_primary_services(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t argc = xsmcArgc;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, serviceDiscoveryEvent, NULL);
	if (argc > 1) {
		ble_uuid_any_t uuid;
		bufferToUUID(&uuid, (uint8_t*)xsmcToArrayBuffer(xsArg(1)), xsmcGetArrayBufferLength(xsArg(1)));
		ble_gattc_disc_svc_by_uuid(conn_id, (const ble_uuid_t *)&uuid, nimble_service_event, NULL);
	}
	else {
		ble_gattc_disc_all_svcs(conn_id, nimble_service_event, NULL);
	}
}

void xs_gatt_service_discover_characteristics(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t start = xsmcToInteger(xsArg(1));
	uint16_t end = xsmcToInteger(xsArg(2));
	attributeSearchRecord *csr;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	csr = c_malloc(sizeof(attributeSearchRecord));
	
	if (NULL != csr) {
		csr->conn_id = conn_id;
		csr->obj = xsThis;
		modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, characteristicDiscoveryEvent, csr);
		if (argc > 3) {
			ble_uuid_any_t uuid;
			bufferToUUID(&uuid, (uint8_t*)xsmcToArrayBuffer(xsArg(3)), xsmcGetArrayBufferLength(xsArg(3)));
			ble_gattc_disc_chrs_by_uuid(conn_id, start, end, (const ble_uuid_t *)&uuid, nimble_characteristic_event, NULL);
		}
		else {
			ble_gattc_disc_all_chrs(conn_id, start, end, nimble_characteristic_event, NULL);
		}
	}
}

void xs_gatt_characteristic_discover_all_characteristic_descriptors(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	descriptorHandleSearchRecord *dsr;
	if (!connection) return;
	dsr = c_calloc(1, sizeof(descriptorHandleSearchRecord));
	
	if (NULL != dsr) {
		xsmcVars(2);
		dsr->conn_id = conn_id;
		dsr->obj = xsThis;
		xsmcGet(xsVar(0), xsThis, xsID_handle);
		dsr->characteristicStartHandle = xsmcToInteger(xsVar(0));
		xsmcGet(xsVar(0), xsThis, xsID_service);
		xsmcGet(xsVar(1), xsVar(0), xsID_start);
		dsr->serviceStartHandle = xsmcToInteger(xsVar(1));
		xsmcGet(xsVar(1), xsVar(0), xsID_end);
		dsr->serviceEndHandle = xsmcToInteger(xsVar(1));
		
		// discover all service characteristics to find last handle in this characteristic
		modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, characteristicHandleDiscoveryEvent, dsr);
		ble_gattc_disc_all_chrs(conn_id, dsr->serviceStartHandle, dsr->serviceEndHandle, nimble_characteristic_event, NULL);
	}
}

void xs_gatt_characteristic_write_without_response(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	uint8_t *buffer = xsmcToArrayBuffer(xsArg(2));
	uint16_t length = xsmcGetArrayBufferLength(xsArg(2));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	int result = ble_gattc_write_no_rsp_flat(conn_id, handle, buffer, length);
	xsmcSetInteger(xsResult, result);
}

void xs_gatt_characteristic_read_value(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	uint16_t auth = 0;
	
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	
	if (argc > 2)
		auth = xsmcToInteger(xsArg(2));

	ble_gattc_read(conn_id, handle, nimble_read_event, (void*)1L);
}

void xs_gatt_characteristic_enable_notifications(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
    uint8_t data[2] = {0x01, 0x00};
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	characteristicNotificationEnabled cne = (characteristicNotificationEnabled)c_malloc(sizeof(characteristicNotificationEnabledRecord));
	if (NULL != cne) {
		cne->enable = 1;
		cne->conn_id = conn_id;
		cne->handle = handle;
		ble_gattc_write_flat(conn_id, handle + 1, data, sizeof(data), nimble_subscribe_event, cne);
	}
}

void xs_gatt_characteristic_disable_notifications(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
    uint8_t data[2] = {0x00, 0x00};
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	characteristicNotificationEnabled cne = (characteristicNotificationEnabled)c_malloc(sizeof(characteristicNotificationEnabledRecord));
	if (NULL != cne) {
		cne->enable = 0;
		cne->conn_id = conn_id;
		cne->handle = handle;
		ble_gattc_write_flat(conn_id, handle + 1, data, sizeof(data), nimble_subscribe_event, cne);
	}
}

void xs_gatt_descriptor_read_value(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	uint16_t auth = 0;
	
	if (argc > 2)
		auth = xsmcToInteger(xsArg(2));
		
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	ble_gattc_read(conn_id, handle, nimble_read_event, (void*)0L);
}

void xs_gatt_descriptor_write_value(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	uint8_t *buffer = xsmcToArrayBuffer(xsArg(2));
	uint16_t length = xsmcGetArrayBufferLength(xsArg(2));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	ble_gattc_write_no_rsp_flat(conn_id, handle, buffer, length);
}

void uuidToBuffer(uint8_t *buffer, ble_uuid_any_t *uuid, uint16_t *length)
{
	if (uuid->u.type == BLE_UUID_TYPE_16) {
		*length = 2;
		buffer[0] = uuid->u16.value & 0xFF;
		buffer[1] = (uuid->u16.value >> 8) & 0xFF;
	}
	else if (uuid->u.type == BLE_UUID_TYPE_32) {
		*length = 4;
		buffer[0] = uuid->u32.value & 0xFF;
		buffer[1] = (uuid->u32.value >> 8) & 0xFF;
		buffer[2] = (uuid->u32.value >> 16) & 0xFF;
		buffer[3] = (uuid->u32.value >> 24) & 0xFF;
	}
	else {
		*length = 16;
		c_memmove(buffer, uuid->u128.value, *length);
	}
}

void bufferToUUID(ble_uuid_any_t *uuid, uint8_t *buffer, uint16_t length)
{
	if (length == 2) {
		uuid->u.type = BLE_UUID_TYPE_16;
		uuid->u16.value = buffer[0] | (buffer[1] << 8);
	}
	else if (length == 4) {
		uuid->u.type = BLE_UUID_TYPE_32;
		uuid->u32.value = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
	}
	else {
		uuid->u.type = BLE_UUID_TYPE_128;
		c_memmove(uuid->u128.value, buffer, length);
	}
}

static const char_name_table *uuidToCharName(ble_uuid_any_t *uuid)
{
	for (int service_index = 0; service_index < service_count; ++service_index) {
		const struct ble_gatt_svc_def *service = &gatt_svr_svcs[service_index];
		int characteristic_index = 0;
		for (int att_index = 0; att_index < attribute_counts[service_index]; ++att_index) {
			const struct ble_gatt_chr_def *characteristic = &service->characteristics[characteristic_index];
			if (0 == ble_uuid_cmp((const ble_uuid_t*)uuid, (const ble_uuid_t*)characteristic->uuid)) {
				for (int k = 0; k < char_name_count; ++k) {
					const char_name_table *char_name = &char_names[k];
					if (service_index == char_name->service_index && att_index == char_name->att_index) {
						return char_name;
					}
				}
			}
			else if (NULL != characteristic->descriptors) {
				uint16_t descriptor_index = 0;
				const struct ble_gatt_dsc_def *descriptor = &characteristic->descriptors[descriptor_index];
				while (NULL != descriptor->uuid) {
					if (0 == ble_uuid_cmp((const ble_uuid_t*)uuid, (const ble_uuid_t*)descriptor->uuid)) {
						for (int k = 0; k < char_name_count; ++k) {
							const char_name_table *char_name = &char_names[k];
							if (service_index == char_name->service_index && att_index == char_name->att_index) {
								return char_name;
							}
						}
					}
					++att_index;
					descriptor = &characteristic->descriptors[++descriptor_index];
				}
			}
			++characteristic_index;
		}
	}
bail:
	return NULL;
}

static void onConnected(struct ble_gap_conn_desc *desc)
{
	xsBeginHost(gBLE->the);
	xsmcVars(1);
	xsmcSetNewObject(xsVar(0));
	xsmcSetInteger(xsResult, desc->conn_handle);
	xsmcSet(xsVar(0), xsID_connection, xsResult);
	xsmcSetArrayBuffer(xsResult, desc->peer_id_addr.val, 6);
	xsmcSet(xsVar(0), xsID_address, xsResult);
	xsmcSetInteger(xsResult, desc->peer_id_addr.type);
	xsmcSet(xsVar(0), xsID_addressType, xsResult);
	xsCall2(gBLE->obj, xsID_callback, xsStringX("onConnected"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void scanResultEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	deviceDiscoveryRecord *entry;
	while (NULL != (entry = (deviceDiscoveryRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {
		struct ble_gap_disc_desc *disc = &entry->disc;
		xsBeginHost(gBLE->the);
		xsmcVars(2);
		xsmcSetNewObject(xsVar(0));
		xsmcSetArrayBuffer(xsVar(1), entry->data, disc->length_data);
		xsmcSet(xsVar(0), xsID_scanResponse, xsVar(1));
		xsmcSetArrayBuffer(xsVar(1), disc->addr.val, 6);
		xsmcSet(xsVar(0), xsID_address, xsVar(1));
		xsmcSetInteger(xsVar(1), disc->addr.type);
		xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
		xsmcSetInteger(xsVar(1), disc->rssi);
		xsmcSet(xsVar(0), xsID_rssi, xsVar(1));
		xsCall2(gBLE->obj, xsID_callback, xsStringX("onDiscovered"), xsVar(0));
		xsEndHost(gBLE->the);
		
		c_free(entry);
	}
}

static void connectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_conn_desc *desc = (struct ble_gap_conn_desc *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAddressAndType(desc->peer_id_addr.val, desc->peer_id_addr.type);
	if (!connection) {
		// @@ workaround for https://github.com/espressif/esp-idf/issues/5693
		connection = modBLEConnectionGetFirst();
		while (NULL != connection) {
			if (kInvalidConnectionID == connection->id)
				break;
			connection = modBLEConnectionGetNext(connection);
		}
		if (!connection)
			xsUnknownError("connection not found");
	}
		
	if (0xFFFF != desc->conn_handle) {
		if (kInvalidConnectionID != connection->id) {
			LOG_GAP_MSG("Ignoring duplicate connect event");
			goto bail;
		}
		connection->id = desc->conn_handle;
		onConnected(desc);
	}
	else {
		LOG_GAP_MSG("BLE_GAP_EVENT_CONNECT failed");
		xsForget(connection->objConnection);
		modBLEConnectionRemove(connection);
	}
bail:
	xsEndHost(gBLE->the);
}

static void disconnectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_conn_desc *desc = (struct ble_gap_conn_desc *)message;
	if (!gBLE)
		return;

	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(desc->conn_handle);
	
	// ignore multiple disconnects on same connection
	if (!connection) {
		LOG_GAP_MSG("Ignoring duplicate disconnect event");
		goto bail;
	}	
	
	xsmcVars(2);
	xsmcSetNewObject(xsVar(0));
	xsmcSetInteger(xsVar(1), desc->conn_handle);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), desc->peer_id_addr.val, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), desc->peer_id_addr.type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(connection->objConnection, xsID_callback, xsStringX("onDisconnected"), xsVar(0));
	xsForget(connection->objConnection);
	modBLEConnectionRemove(connection);
bail:
	xsEndHost(gBLE->the);
}

static void serviceDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	serviceSearchRecord *entry;
	xsBeginHost(gBLE->the);
	xsmcVars(4);
	while (NULL != (entry = (serviceSearchRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {		
		modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection)
			xsUnknownError("connection not found");
		if (entry->completed) {
			xsCall1(connection->objClient, xsID_callback, xsStringX("onService"));
		}
		else {
			struct ble_gatt_svc *service = &entry->service;
			uint16_t length;
			uint8_t buffer[16];
			uuidToBuffer(buffer, &service->uuid, &length);
			xsmcSetNewObject(xsVar(0));
			xsmcSetArrayBuffer(xsVar(1), buffer, length);
			xsmcSetInteger(xsVar(2), service->start_handle);
			xsmcSetInteger(xsVar(3), service->end_handle);
			xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
			xsmcSet(xsVar(0), xsID_start, xsVar(2));
			xsmcSet(xsVar(0), xsID_end, xsVar(3));
			xsCall2(connection->objClient, xsID_callback, xsStringX("onService"), xsVar(0));
		}	
		c_free(entry);
	}
	xsEndHost(gBLE->the);
}

static void characteristicDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	characteristicSearchRecord *entry;
	xsBeginHost(gBLE->the);
	xsmcVars(4);
	while (NULL != (entry = (characteristicSearchRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {		
		modBLEConnection connection = modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection)
			xsUnknownError("connection not found");
		attributeSearchRecord *csr = (attributeSearchRecord *)refcon;
		if (entry->completed) {
			xsCall1(csr->obj, xsID_callback, xsStringX("onCharacteristic"));
			c_free(csr);
		}
		else {
			struct ble_gatt_chr *chr = &entry->characteristic;
			const char_name_table *char_name = uuidToCharName(&chr->uuid);
			uint16_t length;
			uint8_t buffer[16];
			uuidToBuffer(buffer, &chr->uuid, &length);
			xsmcSetNewObject(xsVar(0));
			xsmcSetArrayBuffer(xsVar(1), buffer, length);
			xsmcSetInteger(xsVar(2), chr->val_handle);
			xsmcSetInteger(xsVar(3), chr->properties);
			xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
			xsmcSet(xsVar(0), xsID_handle, xsVar(2));
			xsmcSet(xsVar(0), xsID_properties, xsVar(3));
			if (NULL != char_name) {
				xsmcSetString(xsVar(2), (char*)char_name->name);
				xsmcSet(xsVar(0), xsID_name, xsVar(2));
				xsmcSetString(xsVar(2), (char*)char_name->type);
				xsmcSet(xsVar(0), xsID_type, xsVar(2));
			}
			xsCall2(csr->obj, xsID_callback, xsStringX("onCharacteristic"), xsVar(0));
		}
		c_free(entry);
	}
	xsEndHost(gBLE->the);	
}

static void characteristicHandleDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	characteristicSearchRecord *entry;
	xsBeginHost(gBLE->the);
	while (NULL != (entry = (characteristicSearchRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {		
		modBLEConnection connection = modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection)
			xsUnknownError("connection not found");
		descriptorHandleSearchRecord *dsr = (descriptorHandleSearchRecord *)refcon;
		if (entry->completed) {
			if (0 == dsr->characteristicEndHandle)
				dsr->characteristicEndHandle = dsr->serviceEndHandle;
	
			attributeSearchRecord *dsr2 = c_calloc(1, sizeof(attributeSearchRecord));
			dsr2->conn_id = dsr->conn_id;
			dsr2->obj = dsr->obj;
			modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, descriptorDiscoveryEvent, dsr2);
			ble_gattc_disc_all_dscs(dsr->conn_id, dsr->characteristicStartHandle, dsr->characteristicEndHandle, nimble_descriptor_event, NULL);
			c_free(dsr);
		}
		else {
			struct ble_gatt_chr *chr = &entry->characteristic;
			if (chr->def_handle > dsr->characteristicStartHandle && 0 == dsr->characteristicEndHandle)
				dsr->characteristicEndHandle = chr->def_handle - 1;
		}
		c_free(entry);
	}
	xsEndHost(gBLE->the);	
}

static void descriptorDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	descriptorSearchRecord *entry;
	xsBeginHost(gBLE->the);
	xsmcVars(4);
	while (NULL != (entry = (descriptorSearchRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {		
		modBLEConnection connection = modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection)
			xsUnknownError("connection not found");
		attributeSearchRecord *dsr = (attributeSearchRecord *)refcon;
		if (entry->completed) {
			xsCall1(dsr->obj, xsID_callback, xsStringX("onDescriptor"));
			c_free(dsr);
		}
		else {
			struct ble_gatt_dsc *dsc = &entry->descriptor;
			const char_name_table *char_name = uuidToCharName(&dsc->uuid);
			uint16_t length;
			uint8_t buffer[16];
			uuidToBuffer(buffer, &dsc->uuid, &length);
			xsmcSetNewObject(xsVar(0));
			xsmcSetArrayBuffer(xsVar(1), buffer, length);
			xsmcSetInteger(xsVar(2), dsc->handle);
			xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
			xsmcSet(xsVar(0), xsID_handle, xsVar(2));
			if (NULL != char_name) {
				xsmcSetString(xsVar(2), (char*)char_name->name);
				xsmcSet(xsVar(0), xsID_name, xsVar(2));
				xsmcSetString(xsVar(2), (char*)char_name->type);
				xsmcSet(xsVar(0), xsID_type, xsVar(2));
			}
			xsCall2(dsr->obj, xsID_callback, xsStringX("onDescriptor"), xsVar(0));
		}
		c_free(entry);
	}
	xsEndHost(gBLE->the);
}

static void notificationEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	attributeNotificationRecord *entry;
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	while (NULL != (entry = (attributeNotificationRecord*)modBLEMessageQueueDequeue(&gBLE->notificationQueue))) {
		modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection)
			xsUnknownError("connection not found");
		xsmcSetNewObject(xsVar(0));
		xsmcSetArrayBuffer(xsVar(1), entry->data, entry->length);
		xsmcSetInteger(xsVar(2), entry->handle);
		xsmcSet(xsVar(0), xsID_value, xsVar(1));
		xsmcSet(xsVar(0), xsID_handle, xsVar(2));
		xsCall2(connection->objClient, xsID_callback, xsStringX("onCharacteristicNotification"), xsVar(0));
		c_free(entry);
	}	
	xsEndHost(gBLE->the);
}

static void attributeReadEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	attributeReadData value = (attributeReadData)refcon;
	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(value->conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(3);
	xsmcSetNewObject(xsVar(0));
	xsmcSetArrayBuffer(xsVar(1), value->data, value->length);
	xsmcSetInteger(xsVar(2), value->handle);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->objClient, xsID_callback, value->isCharacteristic ? xsStringX("onCharacteristicValue") : xsStringX("onDescriptorValue"), xsVar(0));
	c_free(value);
	xsEndHost(gBLE->the);
}

static void notificationEnabledEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	characteristicNotificationEnabled cne = (characteristicNotificationEnabled)refcon;
	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(cne->conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(2);
	xsmcSetNewObject(xsVar(0));
	xsmcSetInteger(xsVar(1), cne->handle);
	xsmcSet(xsVar(0), xsID_handle, xsVar(1));
	xsCall2(connection->objClient, xsID_callback, cne->enable ? xsStringX("onCharacteristicNotificationEnabled") : xsStringX("onCharacteristicNotificationDisabled"), xsVar(0));
	c_free(cne);
	xsEndHost(gBLE->the);
}

static void passkeyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_event *event = (struct ble_gap_event *)message;
	struct ble_sm_io pkey = {0};
	
	modBLEConnection connection = modBLEConnectionFindByConnectionID(event->passkey.conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
		
	pkey.action = event->passkey.params.action;
	
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	xsmcSetNewObject(xsVar(0));

    if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
    	pkey.passkey = (c_rand() % 999999) + 1;
		xsmcSetArrayBuffer(xsVar(1), connection->address, 6);
		xsmcSetInteger(xsVar(2), pkey.passkey);
		xsmcSet(xsVar(0), xsID_address, xsVar(1));
		xsmcSet(xsVar(0), xsID_passkey, xsVar(2));
		xsCall2(gBLE->obj, xsID_callback, xsStringX("onPasskeyDisplay"), xsVar(0));
		ble_sm_inject_io(event->passkey.conn_handle, &pkey);
	}
    else if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
		xsmcSetArrayBuffer(xsVar(1), connection->address, 6);
		xsmcSet(xsVar(0), xsID_address, xsVar(1));
		if (gBLE->iocap == KeyboardOnly)
			xsCall2(gBLE->obj, xsID_callback, xsStringX("onPasskeyInput"), xsVar(0));
		else {
			xsResult = xsCall2(gBLE->obj, xsID_callback, xsStringX("onPasskeyRequested"), xsVar(0));
			pkey.passkey = xsmcToInteger(xsResult);
			ble_sm_inject_io(event->passkey.conn_handle, &pkey);
		}
	}
	else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
		xsmcSetArrayBuffer(xsVar(1), connection->address, 6);
		xsmcSetInteger(xsVar(2), event->passkey.params.numcmp);
		xsmcSet(xsVar(0), xsID_address, xsVar(1));
		xsmcSet(xsVar(0), xsID_passkey, xsVar(2));
		xsCall2(gBLE->obj, xsID_callback, xsStringX("onPasskeyConfirm"), xsVar(0));
	}
	
	xsEndHost(gBLE->the);
}

static void encryptionChangeEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_event *event = (struct ble_gap_event *)message;
	
	if (!gBLE)
		return;
		
	if (0 == event->enc_change.status) {
		struct ble_gap_conn_desc desc;
        int rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        if (0 == rc) {
        	if (desc.sec_state.encrypted) {
				xsBeginHost(gBLE->the);
				xsmcVars(2);
				xsmcSetNewObject(xsVar(0));
				xsmcSetBoolean(xsVar(1), desc.sec_state.bonded);
				xsmcSet(xsVar(0), xsID_bonded, xsVar(1));
				xsCall2(gBLE->obj, xsID_callback, xsStringX("onAuthenticated"), xsVar(0));
				xsEndHost(gBLE->the);
        	}
        }
	}
}

static void mtuExchangedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	mtuExchangedRecord *mer = (mtuExchangedRecord*)message;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(mer->conn_id);
	if (!connection) return;
	
	xsBeginHost(gBLE->the);
		xsCall2(connection->objConnection, xsID_callback, xsStringX("onMTUExchanged"), xsInteger(mer->mtu));
	xsEndHost(gBLE->the);
}


static int nimble_service_event(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_svc *service, void *arg)
{
	int rc = 0;
	
	serviceSearchRecord *entry = c_malloc(sizeof(serviceSearchRecord));
	if (NULL == entry)
		goto bail;
	entry->conn_id = conn_handle;
	entry->completed = false;
	if (NULL != service)
		entry->service = *service;

    switch (error->status) {
		case 0:
        	break;
    	case BLE_HS_EDONE:
    		entry->completed = true;
			break;
    	default:
    		entry->completed = true;
        	rc = error->status;
        	break;
    }

	modBLEMessageQueueEnqueue(&gBLE->discoveryQueue, (modBLEMessageQueueEntry)entry);

bail:
    return rc;
}

static int nimble_characteristic_event(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *chr, void *arg)
{
	int rc = 0;
	
	characteristicSearchRecord *entry = c_malloc(sizeof(characteristicSearchRecord));
	if (NULL == entry)
		goto bail;
	entry->conn_id = conn_handle;
	entry->completed = false;
	if (NULL != chr)
		entry->characteristic = *chr;

    switch (error->status) {
		case 0:
        	break;
    	case BLE_HS_EDONE:
    		entry->completed = true;
			break;
    	default:
    		entry->completed = true;
        	rc = error->status;
        	break;
    }

	modBLEMessageQueueEnqueue(&gBLE->discoveryQueue, (modBLEMessageQueueEntry)entry);

bail:
    return rc;
}

static int nimble_descriptor_event(uint16_t conn_handle, const struct ble_gatt_error *error, uint16_t chr_def_handle, const struct ble_gatt_dsc *dsc, void *arg)
{
	int rc = 0;
	
	descriptorSearchRecord *entry = c_malloc(sizeof(descriptorSearchRecord));
	if (NULL == entry)
		goto bail;
	entry->conn_id = conn_handle;
	entry->completed = false;
	if (NULL != dsc)
		entry->descriptor = *dsc;

    switch (error->status) {
		case 0:
        	break;
    	case BLE_HS_EDONE:
    		entry->completed = true;
			break;
    	default:
    		entry->completed = true;
        	rc = error->status;
        	break;
    }

	modBLEMessageQueueEnqueue(&gBLE->discoveryQueue, (modBLEMessageQueueEntry)entry);

bail:
    return rc;
}

static int nimble_read_event(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
	uint32_t isCharacteristic = ((uint32_t)arg) == 1L;
    if (error->status == 0) {
    	attributeReadData value = c_malloc(sizeof(attributeReadDataRecord) + attr->om->om_len);
    	if (NULL != value) {
    		value->isCharacteristic = (uint8_t)isCharacteristic;
    		value->conn_id = conn_handle;
    		value->handle = attr->handle;
    		value->length = attr->om->om_len;
    		c_memmove(value->data, attr->om->om_data, attr->om->om_len);
			modMessagePostToMachine(gBLE->the, NULL, 0, attributeReadEvent, (void*)value);
    	}
    }

	return 0;
}

static int nimble_subscribe_event(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
	characteristicNotificationEnabled cne = (characteristicNotificationEnabled)arg;
    if (error->status == 0)
		modMessagePostToMachine(gBLE->the, NULL, 0, notificationEnabledEvent, (void*)cne);
}

static int nimble_gap_event(struct ble_gap_event *event, void *arg)
{
    int rc = 0;
	struct ble_gap_conn_desc desc;

	LOG_GAP_EVENT(event);

	if (!gBLE || gBLE->terminating)
		goto bail;

    switch (event->type) {
		case BLE_GAP_EVENT_DISC:
			if (0 != event->disc.length_data) {
				deviceDiscoveryRecord *entry = c_malloc(sizeof(deviceDiscoveryRecord) - 1 + event->disc.length_data);
				if (entry) {
					entry->disc = event->disc;
					c_memmove(entry->data, event->disc.data, event->disc.length_data);
					modBLEMessageQueueEnqueue(&gBLE->discoveryQueue, (modBLEMessageQueueEntry)entry);
				}
			}
			break;
		case BLE_GAP_EVENT_CONNECT:
			if (event->connect.status == 0) {
				if (0 == ble_gap_conn_find(event->connect.conn_handle, &desc)) {
					if (gBLE->mitm || gBLE->encryption)
						ble_gap_security_initiate(desc.conn_handle);
					modMessagePostToMachine(gBLE->the, (uint8_t*)&desc, sizeof(desc), connectEvent, NULL);
					goto bail;
				}
			}
			c_memset(&desc, 0, sizeof(desc));
			desc.conn_handle = 0xFFFF;
			modMessagePostToMachine(gBLE->the, (uint8_t*)&desc, sizeof(desc), connectEvent, NULL);
			break;
		case BLE_GAP_EVENT_DISCONNECT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&event->disconnect.conn, sizeof(event->disconnect.conn), disconnectEvent, NULL);
			break;
		case BLE_GAP_EVENT_NOTIFY_RX: {
			attributeNotificationRecord *entry = c_malloc(sizeof(attributeNotificationRecord) + event->notify_rx.om->om_len);
			if (NULL != entry) {
				entry->isNotification = !event->notify_rx.indication;
				entry->conn_id = event->notify_rx.conn_handle;
				entry->handle = event->notify_rx.attr_handle;
				entry->length = event->notify_rx.om->om_len;
				c_memmove(entry->data, event->notify_rx.om->om_data, event->notify_rx.om->om_len);
				modBLEMessageQueueEnqueue(&gBLE->notificationQueue, (modBLEMessageQueueEntry)entry);
			}
			break;
		}
		case BLE_GAP_EVENT_ENC_CHANGE:
			modMessagePostToMachine(gBLE->the, (uint8_t*)event, sizeof(struct ble_gap_event), encryptionChangeEvent, NULL);
			break;
		case BLE_GAP_EVENT_REPEAT_PAIRING:
			// delete old bond and accept new link
			if (0 == ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc))
				ble_store_util_delete_peer(&desc.peer_id_addr);
			return BLE_GAP_REPEAT_PAIRING_RETRY;
			break;
		case BLE_GAP_EVENT_PASSKEY_ACTION:
			modMessagePostToMachine(gBLE->the, (uint8_t*)event, sizeof(struct ble_gap_event), passkeyEvent, NULL);
			break;
		default:
			break;
    }
    
bail:
	return rc;
}

static int nimble_mtu_event(uint16_t conn_handle, const struct ble_gatt_error *error, uint16_t mtu, void *arg)
{
    if (error->status == 0) {
    	mtuExchangedRecord mer;
    	mer.conn_id = conn_handle;
    	mer.mtu = mtu;
		modMessagePostToMachine(gBLE->the, (uint8_t*)&mer, sizeof(mtuExchangedRecord), mtuExchangedEvent, NULL);
    }
    return 0;
}

void logGAPEvent(struct ble_gap_event *event) {
	switch(event->type) {
		case BLE_GAP_EVENT_CONNECT: modLog("BLE_GAP_EVENT_CONNECT"); break;
		case BLE_GAP_EVENT_DISCONNECT: modLog("BLE_GAP_EVENT_DISCONNECT"); break;
		case BLE_GAP_EVENT_CONN_UPDATE: modLog("BLE_GAP_EVENT_CONN_UPDATE"); break;
		case BLE_GAP_EVENT_CONN_UPDATE_REQ: modLog("BLE_GAP_EVENT_CONN_UPDATE_REQ"); break;
		case BLE_GAP_EVENT_L2CAP_UPDATE_REQ: modLog("BLE_GAP_EVENT_L2CAP_UPDATE_REQ"); break;
		case BLE_GAP_EVENT_TERM_FAILURE: modLog("BLE_GAP_EVENT_TERM_FAILURE"); break;
		case BLE_GAP_EVENT_DISC: modLog("BLE_GAP_EVENT_DISC"); break;
		case BLE_GAP_EVENT_DISC_COMPLETE: modLog("BLE_GAP_EVENT_DISC_COMPLETE"); break;
		case BLE_GAP_EVENT_ADV_COMPLETE: modLog("BLE_GAP_EVENT_ADV_COMPLETE"); break;
		case BLE_GAP_EVENT_ENC_CHANGE: modLog("BLE_GAP_EVENT_ENC_CHANGE"); break;
		case BLE_GAP_EVENT_PASSKEY_ACTION: modLog("BLE_GAP_EVENT_PASSKEY_ACTION"); break;
		case BLE_GAP_EVENT_NOTIFY_RX: modLog("BLE_GAP_EVENT_NOTIFY_RX"); break;
		case BLE_GAP_EVENT_NOTIFY_TX: modLog("BLE_GAP_EVENT_NOTIFY_TX"); break;
		case BLE_GAP_EVENT_SUBSCRIBE: modLog("BLE_GAP_EVENT_SUBSCRIBE"); break;
		case BLE_GAP_EVENT_MTU: modLog("BLE_GAP_EVENT_MTU"); break;
		case BLE_GAP_EVENT_IDENTITY_RESOLVED: modLog("BLE_GAP_EVENT_IDENTITY_RESOLVED"); break;
		case BLE_GAP_EVENT_REPEAT_PAIRING: modLog("BLE_GAP_EVENT_REPEAT_PAIRING"); break;
		case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE: modLog("BLE_GAP_EVENT_PHY_UPDATE_COMPLETE"); break;
		case BLE_GAP_EVENT_EXT_DISC: modLog("BLE_GAP_EVENT_EXT_DISC"); break;
	}
}

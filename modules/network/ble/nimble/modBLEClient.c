/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
#include "xsesp.h"
#include "mc.xs.h"
#include "modBLE.h"
#include "modBLECommon.h"

#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "esp_nimble_hci.h"

//#include "mc.bleservices.c"

#define LOG_GATTC 0
#if LOG_GATTC
	#define LOG_GATTC_EVENT(event) logGATTCEvent(event)
	#define LOG_GATTC_MSG(msg) modLog(msg)
	#define LOG_GATTC_INT(i) modLogInt(i)
#else
	#define LOG_GATTC_EVENT(event)
	#define LOG_GATTC_MSG(msg)
	#define LOG_GATTC_INT(i)
#endif

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

typedef struct modBLENotificationRecord modBLENotificationRecord;
typedef modBLENotificationRecord *modBLENotification;

struct modBLENotificationRecord {
	struct modBLENotificationRecord *next;
};

typedef struct modBLEConnectionRecord modBLEConnectionRecord;
typedef modBLEConnectionRecord *modBLEConnection;

struct modBLEConnectionRecord {
	struct modBLEConnectionRecord *next;

	xsMachine	*the;
	xsSlot		objConnection;
	xsSlot		objClient;

	ble_addr_t	bda;
	uint16_t	conn_id;
	
	// char_name_table handles
	
	// registered notifications
	modBLENotification notifications;
};

typedef struct {
	xsMachine	*the;
	xsSlot		obj;
	
	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;

	modBLEConnection connections;
	uint8_t terminating;
} modBLERecord, *modBLE;

static void modBLEConnectionAdd(modBLEConnection connection);
static void modBLEConnectionRemove(modBLEConnection connection);
static modBLEConnection modBLEConnectionFindByConnectionID(uint16_t conn_id);
static modBLEConnection modBLEConnectionFindByAppID(uint16_t app_id);
static modBLEConnection modBLEConnectionFindByAddress(ble_addr_t *bda);

static void uuidToBuffer(uint8_t *buffer, ble_uuid_any_t *uuid, uint16_t *length);
static void bufferToUUID(ble_uuid_any_t *uuid, uint8_t *buffer, uint16_t length);

static void on_reset(int reason);
static void on_sync(void);
static int on_read(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg);
static int on_write(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg);

static void nimble_host_task(void *param);
static void ble_host_task(void *param);

static int gap_event(struct ble_gap_event *event, void *arg);
static int service_event(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_svc *service, void *arg);

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
	
	// Initialize platform Bluetooth modules
	esp_err_t err = modBLEPlatformInitialize();
	if (ESP_OK != err)
		xsUnknownError("ble initialization failed");

	ble_hs_cfg.reset_cb = on_reset;
	ble_hs_cfg.sync_cb = on_sync;

	nimble_port_freertos_init(ble_host_task);
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
	modBLEConnection connections = ble->connections, next;
	while (connections != NULL) {
		modBLEConnection connection = connections;
		connections = connections->next;
		ble_gap_terminate(connection->conn_id, BLE_ERR_REM_USER_CONN_TERM);
		c_free(connection);
	}
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
	uint32_t interval = xsmcToInteger(xsArg(1));
	uint32_t window = xsmcToInteger(xsArg(2));
	uint8_t own_addr_type;
	struct ble_gap_disc_params disc_params;

	ble_hs_id_infer_auto(0, &own_addr_type);

	c_memset(&disc_params, 0, sizeof(disc_params));
	disc_params.filter_duplicates = 1;
	disc_params.passive = !active;
	disc_params.itvl = interval;
	disc_params.window = window;

	ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params, gap_event, NULL);
 }

void xs_ble_client_stop_scanning(xsMachine *the)
{
	ble_gap_disc_cancel();
}

void xs_ble_client_connect(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	ble_addr_t addr;
	uint8_t own_addr_type;

	// Ignore duplicate connection attempts
	if (ble_gap_conn_active()) return;
	
	ble_gap_disc_cancel();
	
	addr.type = BLE_ADDR_PUBLIC;
	c_memmove(&addr.val, address, 6);
		
	// Add a new connection record to be filled as the connection completes
	modBLEConnection connection = c_calloc(sizeof(modBLEConnectionRecord), 1);
	if (!connection)
		xsUnknownError("out of memory");
	connection->conn_id = -1;
	c_memmove(&connection->bda, &addr, sizeof(addr));
	modBLEConnectionAdd(connection);
	
	ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &addr, BLE_HS_FOREVER, NULL, gap_event, NULL);
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

//	if (mitm)
//		esp_ble_gap_config_local_privacy(true);	// generate random address
}

void xs_ble_client_passkey_reply(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t confirm = xsmcToBoolean(xsArg(1));
}

static void readyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsBeginHost(gBLE->the);
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
	xsEndHost(gBLE->the);
}

void nimble_host_task(void *param)
{
	nimble_port_run();
}

void ble_host_task(void *param)
{
	nimble_host_task(param);
}

void on_reset(int reason)
{
	// fatal controller reset - all connections have been closed
	// @@
}

void on_sync(void)
{
	ble_hs_util_ensure_addr(0);
	modMessagePostToMachine(gBLE->the, NULL, 0, readyEvent, NULL);
}

int on_read(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
	return 0;
}

int on_write(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
	return 0;
}

modBLEConnection modBLEConnectionFindByConnectionID(uint16_t conn_id)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (conn_id == walker->conn_id)
			break;
	return walker;
}

modBLEConnection modBLEConnectionFindByAddress(ble_addr_t *bda)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (0 == ble_addr_cmp(&walker->bda, bda))
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
			while (connection->notifications) {
				modBLENotification notification = connection->notifications;
				connection->notifications = notification->next;
				c_free(notification);
			}
			c_free(connection);
			break;
		}
	}
}

void xs_gap_connection_initialize(xsMachine *the)
{
	uint16_t conn_id;
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
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	ble_gap_terminate(connection->conn_id, BLE_ERR_REM_USER_CONN_TERM);
}

void xs_gap_connection_read_rssi(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	int8_t rssi;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	if (0 == ble_gap_conn_rssi(conn_id, &rssi)) {
		xsBeginHost(the);
		xsCall2(connection->objConnection, xsID_callback, xsString("onRSSI"), xsInteger(rssi));
		xsEndHost(the);
	}
}

void xs_gatt_client_discover_primary_services(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t argc = xsmcArgc;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	if (argc > 1) {
		ble_uuid_any_t uuid;
		bufferToUUID(&uuid, (uint8_t*)xsmcToArrayBuffer(xsArg(1)), xsGetArrayBufferLength(xsArg(1)));
		ble_gattc_disc_svc_by_uuid(conn_id, (const ble_uuid_t *)&uuid, service_event, NULL);
	}
	else {
		ble_gattc_disc_all_svcs(conn_id, service_event, NULL);
	}
}

static int modBLEConnectionSaveAttHandle(modBLEConnection connection, ble_uuid_t *uuid, uint16_t handle)
{
	int result = -1;
bail:
	return result;
}


void xs_gatt_service_discover_characteristics(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t start = xsmcToInteger(xsArg(1));
	uint16_t end = xsmcToInteger(xsArg(2));
}

void xs_gatt_characteristic_discover_all_characteristic_descriptors(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
    uint16_t count = 0;
}

void xs_gatt_characteristic_write_without_response(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
}

void xs_gatt_characteristic_read_value(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	uint16_t auth = 0;
	
	if (argc > 2)
		auth = xsmcToInteger(xsArg(2));

	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
}

void xs_gatt_characteristic_enable_notifications(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
}

void xs_gatt_characteristic_disable_notifications(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
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
}

void xs_gatt_descriptor_write_value(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	uint16_t value = xsmcToInteger(xsArg(2));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
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

static void localPrivacyCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
}

static void scanResultEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_disc_desc *disc = (struct ble_gap_disc_desc *)message;
	uint8_t *data = (uint8_t*)refcon;
	
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), data, disc->length_data);
	xsmcSetArrayBuffer(xsVar(2), disc->addr.val, 6);
	xsmcSet(xsVar(0), xsID_scanResponse, xsVar(1));
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onDiscovered"), xsVar(0));
	c_free(data);
	xsEndHost(gBLE->the);
}

static void connectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_conn_desc *desc = (struct ble_gap_conn_desc *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAddress(&desc->peer_id_addr);
	if (!connection)
		xsUnknownError("connection not found");
		
	if (-1 != desc->conn_handle) {
		if (-1 != connection->conn_id) {
			LOG_GATTC_MSG("Ignoring duplicate connect event");
			goto bail;
		}
		connection->conn_id = desc->conn_handle;
		xsmcVars(3);
		xsVar(0) = xsmcNewObject();
		xsmcSetInteger(xsVar(1), desc->conn_handle);
		xsmcSet(xsVar(0), xsID_connection, xsVar(1));
		xsmcSetArrayBuffer(xsVar(2), &desc->peer_id_addr.val, 6);
		xsmcSet(xsVar(0), xsID_address, xsVar(2));
		xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));
	}
	else {
#if LOG_GATTC
		LOG_GATTC_MSG("ESP_GATTC_OPEN_EVT failed");
#endif
		modBLEConnectionRemove(connection);
		xsCall1(gBLE->obj, xsID_callback, xsString("onDisconnected"));
	}
bail:
	xsEndHost(gBLE->the);
}

static void disconnectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_conn_desc *desc = (struct ble_gap_conn_desc *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(desc->conn_handle);
	
	// ignore multiple disconnects on same connection
	if (!connection) {
		LOG_GATTC_MSG("Ignoring duplicate disconnect event");
		goto bail;
	}	
	
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), desc->conn_handle);
	xsCall2(connection->objConnection, xsID_callback, xsString("onDisconnected"), xsVar(0));
	modBLEConnectionRemove(connection);
bail:
	xsEndHost(gBLE->the);
}

static void gapPasskeyConfirmEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
}

static void gapPasskeyNotifyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
}

static void gapPasskeyRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
}

static void gapAuthCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
}

static void serviceDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gatt_svc *service = (struct ble_gatt_svc*)message;
	uint16_t conn_id = (uint16_t)(uint32_t)refcon;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	if (NULL != service) {
		uint16_t length;
		uint8_t buffer[16];
		uuidToBuffer(buffer, &service->uuid, &length);
		xsmcVars(4);
		xsVar(0) = xsmcNewObject();
		xsmcSetArrayBuffer(xsVar(1), buffer, length);
		xsmcSetInteger(xsVar(2), service->start_handle);
		xsmcSetInteger(xsVar(3), service->end_handle);
		xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
		xsmcSet(xsVar(0), xsID_start, xsVar(2));
		xsmcSet(xsVar(0), xsID_end, xsVar(3));
		xsCall2(connection->objClient, xsID_callback, xsString("onService"), xsVar(0));
	}
	else
		xsCall1(connection->objClient, xsID_callback, xsString("onService"));
	xsEndHost(gBLE->the);
}

static int service_event(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_svc *service, void *arg)
{
	int rc = 0;
	uint32_t conn_id = conn_handle;
	
    switch (error->status) {
		case 0:
			modMessagePostToMachine(gBLE->the, (uint8_t*)service, sizeof(struct ble_gatt_svc), serviceDiscoveryEvent, (void*)conn_id);
        	break;
    	case BLE_HS_EDONE:
			modMessagePostToMachine(gBLE->the, NULL, 0, serviceDiscoveryEvent, (void*)conn_id);
			break;
    	default:
        	rc = error->status;
        	break;
    }

    if (rc != 0)
		modMessagePostToMachine(gBLE->the, NULL, 0, serviceDiscoveryEvent, (void*)conn_id);

    return rc;
}


static int gap_event(struct ble_gap_event *event, void *arg)
{
    int rc = 0;
	struct ble_gap_conn_desc desc;

	LOG_GAP_EVENT(event);

	if (!gBLE || gBLE->terminating)
		goto bail;

    switch (event->type) {
		case BLE_GAP_EVENT_DISC:
			if (0 != event->disc.length_data) {
				uint8_t *data = c_malloc(event->disc.length_data);
				if (NULL != data) {
					c_memmove(data, event->disc.data, event->disc.length_data);
					modMessagePostToMachine(gBLE->the, (uint8_t*)&event->disc, sizeof(event->disc), scanResultEvent, data);
				}
			}
			break;
		case BLE_GAP_EVENT_CONNECT:
			if (event->connect.status == 0) {
				if (0 == ble_gap_conn_find(event->connect.conn_handle, &desc)) {
					modMessagePostToMachine(gBLE->the, (uint8_t*)&desc, sizeof(desc), connectEvent, NULL);
					goto bail;
				}
			}
			c_memset(&desc, 0, sizeof(desc));
			desc.conn_handle = -1;
			modMessagePostToMachine(gBLE->the, (uint8_t*)&desc, sizeof(desc), connectEvent, NULL);
			break;
		case BLE_GAP_EVENT_DISCONNECT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&event->disconnect.conn, sizeof(event->disconnect.conn), disconnectEvent, NULL);
			break;
		default:
			break;
    }
    
bail:
	return rc;
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

static void gattcNotifyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
}

static void gattcReadCharEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
}

static void gattcRegisterNotifyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
}

static void gattcUnregisterNotifyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
}

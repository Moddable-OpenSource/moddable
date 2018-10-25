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
#include "xsesp.h"
#include "mc.xs.h"
#include "modBLE.h"
#include "modBLECommon.h"

#include "FreeRTOSConfig.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gattc_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

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

	uint16_t char_handle;
};

typedef struct modBLEConnectionRecord modBLEConnectionRecord;
typedef modBLEConnectionRecord *modBLEConnection;

struct modBLEConnectionRecord {
	struct modBLEConnectionRecord *next;

	xsMachine	*the;
	xsSlot		objConnection;
	xsSlot		objClient;

	esp_bd_addr_t bda;
	esp_gatt_if_t gattc_if;
	int16_t conn_id;
	uint16_t app_id;
	
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

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

static void modBLEConnectionAdd(modBLEConnection connection);
static void modBLEConnectionRemove(modBLEConnection connection);
static modBLEConnection modBLEConnectionFindByConnectionID(uint16_t conn_id);
static modBLEConnection modBLEConnectionFindByAppID(uint16_t app_id);
static modBLEConnection modBLEConnectionFindByAddress(esp_bd_addr_t *bda);

static void uuidToBuffer(uint8_t *buffer, esp_bt_uuid_t *uuid, uint16_t *length);
static void bufferToUUID(esp_bt_uuid_t *uuid, uint8_t *buffer, uint16_t length);

static void logGATTCEvent(esp_gattc_cb_event_t event);
static void logGAPEvent(esp_gap_ble_cb_event_t event);

static modBLE gBLE = NULL;
static int gAPP_ID = 1;

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

	// Register callbacks
	if (ESP_OK == err)
		err = esp_ble_gap_register_callback(gap_event_handler);
	if (ESP_OK == err)
		err = esp_ble_gattc_register_callback(gattc_event_handler);
	if (ESP_OK != err)
		xsUnknownError("ble initialization failed");
    
	// Stack is ready
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
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
		esp_ble_gattc_app_unregister(connection->gattc_if);
		modBLENotification notifications = connection->notifications;
		while (notifications != NULL) {
			modBLENotification notification = notifications;
			notifications = notifications->next;
			c_free(notification);
		}
		c_free(connection);
	}
	c_free(ble);
	gBLE = NULL;

	modBLEPlatformTerminate();
}

void xs_ble_client_set_local_privacy(xsMachine *the)
{
	uint8_t enable = xsmcToBoolean(xsArg(0));
	esp_ble_gap_config_local_privacy(enable);
}

void xs_ble_client_start_scanning(xsMachine *the)
{
	uint8_t active = xsmcToBoolean(xsArg(0));
	uint32_t interval = xsmcToInteger(xsArg(1));
	uint32_t window = xsmcToInteger(xsArg(2));
	esp_ble_scan_params_t scan_params;
	
	scan_params.scan_type = active ? BLE_SCAN_TYPE_ACTIVE : BLE_SCAN_TYPE_PASSIVE;
	scan_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
	scan_params.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
	scan_params.scan_interval = interval;
	scan_params.scan_window = window;
	esp_ble_gap_set_scan_params(&scan_params);
}

void xs_ble_client_stop_scanning(xsMachine *the)
{
	esp_ble_gap_stop_scanning();
}

void xs_ble_client_connect(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	esp_bd_addr_t bda;

	c_memmove(&bda, address, sizeof(bda));
		
	// Ignore duplicate connection attempts
	if (modBLEConnectionFindByAddress(&bda)) {
		LOG_GATTC_MSG("Ignoring duplicate connect attempt");
		return;
	};
	
	// Add a new connection record to be filled as the connection completes
	modBLEConnection connection = c_calloc(sizeof(modBLEConnectionRecord), 1);
	if (!connection)
		xsUnknownError("out of memory");
	connection->conn_id = -1;
	connection->app_id = gAPP_ID++;
	connection->gattc_if = ESP_GATT_IF_NONE;
	c_memmove(&connection->bda, &bda, sizeof(bda));
	modBLEConnectionAdd(connection);
	
	// register application client and connect when ESP_GATTC_REG_EVT received
	esp_ble_gattc_app_register(connection->app_id);
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

	if (mitm)
		esp_ble_gap_config_local_privacy(true);	// generate random address
}

void xs_ble_client_passkey_reply(xsMachine *the)
{
	esp_bd_addr_t bda;
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t confirm = xsmcToBoolean(xsArg(1));
	c_memmove(&bda, address, sizeof(bda));
	esp_ble_confirm_reply(bda, confirm);
}

modBLEConnection modBLEConnectionFindByConnectionID(uint16_t conn_id)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (conn_id == walker->conn_id)
			break;
	return walker;
}

modBLEConnection modBLEConnectionFindByAppID(uint16_t app_id)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (app_id == walker->app_id)
			break;
	return walker;
}

modBLEConnection modBLEConnectionFindByInterface(esp_gatt_if_t gattc_if)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (gattc_if == walker->gattc_if)
			break;
	return walker;
}

modBLEConnection modBLEConnectionFindByAddress(esp_bd_addr_t *bda)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (0 == c_memcmp(bda, walker->bda, sizeof(esp_bd_addr_t)))
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
				//esp_ble_gattc_unregister_for_notify(connection->gattc_if, connection->bda, notification->char_handle);
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
	esp_ble_gap_disconnect(connection->bda);
}

void xs_gap_connection_read_rssi(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	esp_ble_gap_read_rssi(connection->bda);
}

void xs_gatt_client_discover_primary_services(xsMachine *the)
{
	esp_bt_uuid_t uuid;
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t argc = xsmcArgc;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	if (argc > 1)
		bufferToUUID(&uuid, (uint8_t*)xsmcToArrayBuffer(xsArg(1)), xsGetArrayBufferLength(xsArg(1)));
	esp_ble_gattc_search_service(connection->gattc_if, conn_id, (argc > 1 ? &uuid : NULL));
}

typedef struct {
	xsSlot obj;
	uint16_t count;
	esp_gattc_char_elem_t char_elem_result[1];
} characteristicSearchRecord;

static void charSearchResultEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	characteristicSearchRecord *csr = (characteristicSearchRecord*)refcon;
	xsBeginHost(gBLE->the);
	for (int i = 0; i < csr->count; ++i) {
		esp_gattc_char_elem_t *char_elem = &csr->char_elem_result[i];
		uint16_t length;
		uint8_t buffer[ESP_UUID_LEN_128];
		uuidToBuffer(buffer, &char_elem->uuid, &length);
		xsmcVars(4);
		xsVar(0) = xsmcNewObject();
		xsmcSetArrayBuffer(xsVar(1), buffer, length);
		xsmcSetInteger(xsVar(2), char_elem->char_handle);
		xsmcSetInteger(xsVar(3), char_elem->properties);
		xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
		xsmcSet(xsVar(0), xsID_handle, xsVar(2));
		xsmcSet(xsVar(0), xsID_properties, xsVar(3));
		xsCall2(csr->obj, xsID_callback, xsString("onCharacteristic"), xsVar(0));
	}
	xsCall1(csr->obj, xsID_callback, xsString("onCharacteristic"));
	c_free(csr);
	xsEndHost(gBLE->the);
}

void xs_gatt_service_discover_characteristics(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t start = xsmcToInteger(xsArg(1));
	uint16_t end = xsmcToInteger(xsArg(2));
	uint16_t count = 0;
	characteristicSearchRecord *csr;
	uint16_t length;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	esp_ble_gattc_get_attr_count(connection->gattc_if, conn_id, ESP_GATT_DB_CHARACTERISTIC, start, end, 0, &count);
	if (count > 0) {
		uint16_t length = sizeof(characteristicSearchRecord) + (sizeof(esp_gattc_char_elem_t) * count);
		characteristicSearchRecord *csr = c_malloc(length);
		if (!csr)
			xsUnknownError("out of memory");
		if (argc > 3) {
			esp_bt_uuid_t uuid;
			bufferToUUID(&uuid, (uint8_t*)xsmcToArrayBuffer(xsArg(3)), xsGetArrayBufferLength(xsArg(3)));
			if (ESP_OK != esp_ble_gattc_get_char_by_uuid(connection->gattc_if, conn_id, start, end, uuid, &csr->char_elem_result[0], &count))
				count = 0;
		}
		else {
			if (ESP_OK != esp_ble_gattc_get_all_char(connection->gattc_if, conn_id, start, end, &csr->char_elem_result[0], &count, 0))
				count = 0;
		}
		if (0 != count) {
			csr->obj = xsThis;
			csr->count = count;
			modMessagePostToMachine(gBLE->the, NULL, 0, charSearchResultEvent, csr);
		}
		else {
			c_free(csr);
			xsCall1(xsThis, xsID_callback, xsString("onCharacteristic"));
		}
	}
	else
		xsCall1(xsThis, xsID_callback, xsString("onCharacteristic"));
}

typedef struct {
	xsSlot obj;
	uint16_t count;
	esp_gattc_descr_elem_t descr_elem_result[1];
} descriptorSearchRecord;

static void descSearchResultEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	descriptorSearchRecord *dsr = (descriptorSearchRecord*)refcon;
	xsBeginHost(gBLE->the);
	for (int i = 0; i < dsr->count; ++i) {
		esp_gattc_descr_elem_t *descr_elem = &dsr->descr_elem_result[i];
		uint16_t length;
		uint8_t buffer[ESP_UUID_LEN_128];
		uuidToBuffer(buffer, &descr_elem->uuid, &length);
		xsmcVars(3);
		xsVar(0) = xsmcNewObject();
		xsmcSetArrayBuffer(xsVar(1), buffer, length);
		xsmcSetInteger(xsVar(2), descr_elem->handle);
		xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
		xsmcSet(xsVar(0), xsID_handle, xsVar(2));
		xsCall2(dsr->obj, xsID_callback, xsString("onDescriptor"), xsVar(0));
	}
	xsCall1(dsr->obj, xsID_callback, xsString("onDescriptor"));
	c_free(dsr);
	xsEndHost(gBLE->the);
}

void xs_gatt_characteristic_discover_all_characteristic_descriptors(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
    uint16_t count = 0;
	descriptorSearchRecord *dsr;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	esp_ble_gattc_get_attr_count(connection->gattc_if, conn_id, ESP_GATT_DB_DESCRIPTOR, 0, 0, handle, &count);
	if (count > 0) {
		uint16_t length = sizeof(descriptorSearchRecord) + (sizeof(esp_gattc_descr_elem_t) * count);
		descriptorSearchRecord *dsr = c_malloc(length);
		if (!dsr)
			xsUnknownError("out of memory");
		xsmcVars(3);
		if (ESP_OK != esp_ble_gattc_get_all_descr(connection->gattc_if, conn_id, handle, &dsr->descr_elem_result[0], &count, 0))
			count = 0;
		if (0 != count) {
			dsr->obj = xsThis;
			dsr->count = count;
			modMessagePostToMachine(gBLE->the, NULL, 0, descSearchResultEvent, dsr);
		}
		else {
			c_free(dsr);
			xsCall1(xsThis, xsID_callback, xsString("onDescriptor"));
		}
	}
	else
		xsCall1(xsThis, xsID_callback, xsString("onDescriptor"));
}

void xs_gatt_characteristic_write_without_response(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	esp_gatt_write_type_t write_type = ESP_GATT_WRITE_TYPE_RSP;
	esp_gatt_auth_req_t auth_req = ESP_GATT_AUTH_REQ_NONE;
	char *str;
		
	switch (xsmcTypeOf(xsArg(2))) {
		case xsStringType:
			str = xsmcToString(xsArg(2));
			esp_ble_gattc_write_char(connection->gattc_if, conn_id, handle, c_strlen(str), (uint8_t*)str, write_type, auth_req);
			break;
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype))
				esp_ble_gattc_write_char(connection->gattc_if, conn_id, handle, xsGetArrayBufferLength(xsArg(2)), (uint8_t*)xsmcToArrayBuffer(xsArg(2)), write_type, auth_req);
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
	uint16_t argc = xsmcArgc;
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	uint16_t auth = 0;
	
	if (argc > 2)
		auth = xsmcToInteger(xsArg(2));

	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	esp_gatt_auth_req_t auth_req = auth;
	esp_ble_gattc_read_char(connection->gattc_if, conn_id, handle, auth_req);
}

void xs_gatt_characteristic_enable_notifications(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	modBLENotification walker, notification = c_calloc(sizeof(modBLENotificationRecord), 1);
	if (!notification)
		xsUnknownError("out of memory");
	if (!connection->notifications)
		connection->notifications = notification;
	else {
		for (walker = connection->notifications; walker->next; walker = walker->next)
			;
		walker->next = notification;
	}
	notification->char_handle = handle;
	esp_ble_gattc_register_for_notify(connection->gattc_if, connection->bda, handle);
}

void xs_gatt_characteristic_disable_notifications(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	esp_ble_gattc_unregister_for_notify(connection->gattc_if, connection->bda, handle);
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
	esp_gatt_auth_req_t auth_req = auth;
	esp_ble_gattc_read_char_descr(connection->gattc_if, conn_id, handle, auth_req);
}

void xs_gatt_descriptor_write_value(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	uint16_t value = xsmcToInteger(xsArg(2));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	switch (xsmcTypeOf(xsArg(2))) {
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype))
				esp_ble_gattc_write_char_descr(connection->gattc_if, conn_id, handle, xsGetArrayBufferLength(xsArg(2)), (uint8_t*)xsmcToArrayBuffer(xsArg(2)), ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
			else
				goto unknown;
			break;
		unknown:
		default:
			xsUnknownError("unsupported type");
			break;
	}
}

void uuidToBuffer(uint8_t *buffer, esp_bt_uuid_t *uuid, uint16_t *length)
{
	if (uuid->len == ESP_UUID_LEN_16) {
		*length = ESP_UUID_LEN_16;
		buffer[0] = uuid->uuid.uuid16 & 0xFF;
		buffer[1] = (uuid->uuid.uuid16 >> 8) & 0xFF;
	}
	else if (uuid->len == ESP_UUID_LEN_32) {
		*length = ESP_UUID_LEN_32;
		buffer[0] = uuid->uuid.uuid32 & 0xFF;
		buffer[1] = (uuid->uuid.uuid32 >> 8) & 0xFF;
		buffer[2] = (uuid->uuid.uuid32 >> 16) & 0xFF;
		buffer[3] = (uuid->uuid.uuid32 >> 24) & 0xFF;
	}
	else {
		*length = ESP_UUID_LEN_128;
		c_memmove(buffer, uuid->uuid.uuid128, *length);
	}
}

void bufferToUUID(esp_bt_uuid_t *uuid, uint8_t *buffer, uint16_t length)
{
	if (length == ESP_UUID_LEN_16) {
		uuid->uuid.uuid16 = buffer[0] | (buffer[1] << 8);
	}
	else if (length == ESP_UUID_LEN_32) {
		uuid->uuid.uuid32 = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
	}
	else
		c_memmove(uuid->uuid.uuid128, buffer, length);
	uuid->len = length;
}

static void localPrivacyCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
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

static void scanResultEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_scan_result_evt_param *scan_rst = (struct ble_scan_result_evt_param *)message;
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), scan_rst->ble_adv, scan_rst->adv_data_len + scan_rst->scan_rsp_len);
	xsmcSetArrayBuffer(xsVar(2), scan_rst->bda, 6);
	xsmcSet(xsVar(0), xsID_scanResponse, xsVar(1));
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onDiscovered"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void rssiCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_read_rssi_cmpl_evt_param *read_rssi_cmpl = (struct ble_read_rssi_cmpl_evt_param *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAddress(&read_rssi_cmpl->remote_addr);
	if (!connection)
		xsUnknownError("connection not found");
	xsCall2(connection->objConnection, xsID_callback, xsString("onRSSI"), xsInteger(read_rssi_cmpl->rssi));
	xsEndHost(gBLE->the);
}

static void gapPasskeyConfirmEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	esp_ble_sec_key_notif_t *key_notif = (esp_ble_sec_key_notif_t *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAddress(&key_notif->bd_addr);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), key_notif->bd_addr, 6);
	xsmcSetInteger(xsVar(2), key_notif->passkey);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSet(xsVar(0), xsID_passkey, xsVar(2));
	xsResult = xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyConfirm"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gapPasskeyNotifyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	esp_ble_sec_key_notif_t *key_notif = (esp_ble_sec_key_notif_t *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAddress(&key_notif->bd_addr);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), key_notif->passkey);
	xsmcSet(xsVar(0), xsID_passkey, xsVar(1));
	xsCall2(connection->objConnection, xsID_callback, xsString("onPasskeyDisplay"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gapPasskeyRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	esp_ble_sec_req_t *ble_req = (esp_ble_sec_req_t *)message;
	uint32_t passkey;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAddress(&ble_req->bd_addr);
	if (!connection)
		xsUnknownError("connection not found");
	xsResult = xsCall1(connection->objConnection, xsID_callback, xsString("onPasskeyRequested"));
	passkey = xsmcToInteger(xsResult);
	xsEndHost(gBLE->the);
	esp_ble_passkey_reply(ble_req->bd_addr, true, passkey);
}

static void gapAuthCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	esp_ble_auth_cmpl_t *auth_cmpl = (esp_ble_auth_cmpl_t *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAddress(&auth_cmpl->bd_addr);
	if (!connection)
		xsUnknownError("connection not found");
	xsCall1(gBLE->obj, xsID_callback, xsString("onAuthenticated"));
	xsEndHost(gBLE->the);
}

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	LOG_GAP_EVENT(event);

	if (!gBLE || gBLE->terminating)
		return;

	switch(event) {
		case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
        	if (ESP_GATT_OK == param->local_privacy_cmpl.status)
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->local_privacy_cmpl, sizeof(struct ble_local_privacy_cmpl_evt_param), localPrivacyCompleteEvent, gBLE);
#if LOG_GAP
			else {
				LOG_GAP_MSG("ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT failed, status =");
				LOG_GAP_INT(param->local_privacy_cmpl.status);
			}
#endif
			break;
		case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
			esp_ble_gap_start_scanning(0);			// 0 == scan until explicitly stopped
			break;
		case ESP_GAP_BLE_SCAN_RESULT_EVT:
			if (ESP_GAP_SEARCH_INQ_RES_EVT == param->scan_rst.search_evt)
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->scan_rst, sizeof(struct ble_scan_result_evt_param), scanResultEvent, gBLE);
            break;
        case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT:
        	if (ESP_GATT_OK == param->read_rssi_cmpl.status)
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->read_rssi_cmpl, sizeof(struct ble_read_rssi_cmpl_evt_param), rssiCompleteEvent, gBLE);
#if LOG_GAP
			else {
				LOG_GAP_MSG("ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT failed, status =");
				LOG_GAP_INT(param->read_rssi_cmpl.status);
			}
#endif
        	break;
		case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->ble_security.key_notif, sizeof(esp_ble_sec_key_notif_t), gapPasskeyNotifyEvent, NULL);
			break;
		case ESP_GAP_BLE_NC_REQ_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->ble_security.key_notif, sizeof(esp_ble_sec_key_notif_t), gapPasskeyConfirmEvent, NULL);
			break;
		case ESP_GAP_BLE_PASSKEY_REQ_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->ble_security.ble_req, sizeof(esp_ble_sec_req_t), gapPasskeyRequestEvent, NULL);
			break;
     	case ESP_GAP_BLE_AUTH_CMPL_EVT:
			if (param->ble_security.auth_cmpl.success)
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->ble_security.auth_cmpl, sizeof(esp_ble_auth_cmpl_t), gapAuthCompleteEvent, NULL);
#if LOG_GAP
			else {
				LOG_GAP_MSG("ESP_GAP_BLE_AUTH_CMPL_EVT failed, status =");
				LOG_GAP_INT(param->ble_security.auth_cmpl.fail_reason);
			}
#endif
     		break;
		case ESP_GAP_BLE_SEC_REQ_EVT:
			esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
     		break;
		default:
			break;
    }
}

static void logGAPEvent(esp_gap_ble_cb_event_t event) {
	switch(event) {
		case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT: modLog("ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT: modLog("ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: modLog("ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_SCAN_RESULT_EVT: modLog("ESP_GAP_BLE_SCAN_RESULT_EVT"); break;
		case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT: modLog("ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT: modLog("ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: modLog("ESP_GAP_BLE_SCAN_START_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_AUTH_CMPL_EVT: modLog("ESP_GAP_BLE_AUTH_CMPL_EVT"); break;
		case ESP_GAP_BLE_KEY_EVT: modLog("ESP_GAP_BLE_KEY_EVT"); break;
		case ESP_GAP_BLE_SEC_REQ_EVT: modLog("ESP_GAP_BLE_SEC_REQ_EVT"); break;
		case ESP_GAP_BLE_PASSKEY_NOTIF_EVT: modLog("ESP_GAP_BLE_PASSKEY_NOTIF_EVT"); break;
		case ESP_GAP_BLE_PASSKEY_REQ_EVT: modLog("ESP_GAP_BLE_PASSKEY_REQ_EVT"); break;
		case ESP_GAP_BLE_OOB_REQ_EVT: modLog("ESP_GAP_BLE_OOB_REQ_EVT"); break;
		case ESP_GAP_BLE_LOCAL_IR_EVT: modLog("ESP_GAP_BLE_LOCAL_IR_EVT"); break;
		case ESP_GAP_BLE_LOCAL_ER_EVT: modLog("ESP_GAP_BLE_LOCAL_ER_EVT"); break;
		case ESP_GAP_BLE_NC_REQ_EVT: modLog("ESP_GAP_BLE_NC_REQ_EVT"); break;
		case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT: modLog("ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: modLog("ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT: modLog("ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT"); break;
		case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT: modLog("ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT"); break;
		case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT: modLog("ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT: modLog("ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT: modLog("ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT: modLog("ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT: modLog("ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT: modLog("ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT"); break;
		case ESP_GAP_BLE_UPDATE_WHITELIST_COMPLETE_EVT: modLog("ESP_GAP_BLE_UPDATE_WHITELIST_COMPLETE_EVT"); break;
	}
}

static void gattcRegisterEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_reg_evt_param *reg = (struct gattc_reg_evt_param *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAppID(reg->app_id);
	if (!connection)
		xsUnknownError("connection not found");
	esp_ble_gattc_open(connection->gattc_if, connection->bda, BLE_ADDR_TYPE_PUBLIC, true);
	xsEndHost(gBLE->the);
}

static void gattcOpenEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_open_evt_param *open = (struct gattc_open_evt_param *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAddress(&open->remote_bda);
	if (!connection)
		xsUnknownError("connection not found");
		
	if (ESP_GATT_OK == open->status) {
		if (-1 != connection->conn_id) {
			LOG_GATTC_MSG("Ignoring duplicate connect event");
			goto bail;
		}
		connection->conn_id = open->conn_id;
		xsmcVars(3);
		xsVar(0) = xsmcNewObject();
		xsmcSetInteger(xsVar(1), open->conn_id);
		xsmcSet(xsVar(0), xsID_connection, xsVar(1));
		xsmcSetArrayBuffer(xsVar(2), open->remote_bda, 6);
		xsmcSet(xsVar(0), xsID_address, xsVar(2));
		xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));
	}
	else {
#if LOG_GATTC
		LOG_GATTC_MSG("ESP_GATTC_OPEN_EVT failed, status =");
		LOG_GATTC_INT(open->status);
#endif
		if (ESP_GATT_IF_NONE != connection->gattc_if)
			esp_ble_gattc_app_unregister(connection->gattc_if);
		modBLEConnectionRemove(connection);
		if (gAPP_ID > 1)
			--gAPP_ID;
		xsCall1(gBLE->obj, xsID_callback, xsString("onDisconnected"));
	}
bail:
	xsEndHost(gBLE->the);
}

static void gattcCloseEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_close_evt_param *close = (struct gattc_close_evt_param *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(close->conn_id);
	
	// ignore multiple disconnects on same connection
	if (!connection) {
		LOG_GATTC_MSG("Ignoring duplicate disconnect event");
		goto bail;
	}	
	
	esp_ble_gattc_app_unregister(connection->gattc_if);
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), close->conn_id);
	xsCall2(connection->objConnection, xsID_callback, xsString("onDisconnected"), xsVar(0));
	modBLEConnectionRemove(connection);
bail:
	xsEndHost(gBLE->the);
}

static void gattcSearchResultEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_search_res_evt_param *search_res = (struct gattc_search_res_evt_param *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(search_res->conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	uint16_t length;
	uint8_t buffer[ESP_UUID_LEN_128];
	uuidToBuffer(buffer, &search_res->srvc_id.uuid, &length);
	xsmcVars(4);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), buffer, length);
	xsmcSetInteger(xsVar(2), search_res->start_handle);
	xsmcSetInteger(xsVar(3), search_res->end_handle);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSet(xsVar(0), xsID_start, xsVar(2));
	xsmcSet(xsVar(0), xsID_end, xsVar(3));
	xsCall2(connection->objClient, xsID_callback, xsString("onService"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattcSearchCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_search_cmpl_evt_param *search_cmpl = (struct gattc_search_cmpl_evt_param *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(search_cmpl->conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	xsCall1(connection->objClient, xsID_callback, xsString("onService"));
	xsEndHost(gBLE->the);
}

static void doCharEvent(void *the, const char *callback, uint16_t conn_id, uint16_t handle, uint8_t *value, uint16_t value_len)
{
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), value, value_len);
	xsmcSetInteger(xsVar(2), handle);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->objClient, xsID_callback, xsString(callback), xsVar(0));
	c_free(value);
	xsEndHost(gBLE->the);
}

static void gattcNotifyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_notify_evt_param *notify = (struct gattc_notify_evt_param *)message;
	doCharEvent(the, "onCharacteristicNotification", notify->conn_id, notify->handle, notify->value, notify->value_len);
}

static void gattcReadCharEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_read_char_evt_param *read = (struct gattc_read_char_evt_param *)message;
	uint32_t event = (uint32_t)refcon;
	doCharEvent(the, ESP_GATTC_READ_CHAR_EVT == event ? "onCharacteristicValue" : "onDescriptorValue", read->conn_id, read->handle, read->value, read->value_len);
}

static void gattcRegisterNotifyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_reg_for_notify_evt_param *reg_for_notify = (struct gattc_reg_for_notify_evt_param *)message;
    modBLEConnection connection = (modBLEConnection)refcon;
	esp_gattc_descr_elem_t result;
	uint16_t count = 1;
	esp_bt_uuid_t uuid;
	uuid.len = ESP_UUID_LEN_16;
	uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
	if (ESP_GATT_OK == esp_ble_gattc_get_descr_by_char_handle(connection->gattc_if, connection->conn_id, reg_for_notify->handle, uuid, &result, &count)) {
		uint16_t notify_en = 1;
		esp_ble_gattc_write_char_descr(connection->gattc_if, connection->conn_id, result.handle, sizeof(notify_en), (uint8_t*)&notify_en, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
	}
}

void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

	LOG_GATTC_EVENT(event);
	
	if (!gBLE || gBLE->terminating) return;

    switch (event) {
		case ESP_GATTC_REG_EVT:
        	if (param->reg.status == ESP_GATT_OK) {
				modCriticalSectionBegin();
				modBLEConnection connection = modBLEConnectionFindByAppID(param->reg.app_id);
				if (connection)
					connection->gattc_if = gattc_if;
				modCriticalSectionEnd();
#if LOG_GATTC
				if (!connection)
					LOG_GATTC_MSG("ESP_GATTC_REG_EVT failed to find connection");
#endif
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->reg, sizeof(struct gattc_reg_evt_param), gattcRegisterEvent, NULL);
        	}
#if LOG_GATTC
			else {
				LOG_GATTC_MSG("ESP_GATTC_REG_EVT failed, status =");
				LOG_GATTC_INT(param->reg.status);
			}
#endif
        	break;
    	case ESP_GATTC_OPEN_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->open, sizeof(struct gattc_open_evt_param), gattcOpenEvent, NULL);
    		break;
    	case ESP_GATTC_CLOSE_EVT:
        	if (param->close.status == ESP_GATT_OK)
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->close, sizeof(struct gattc_close_evt_param), gattcCloseEvent, NULL);
#if LOG_GATTC
			else {
				LOG_GATTC_MSG("ESP_GATTC_CLOSE_EVT failed, reason =");
				LOG_GATTC_INT(param->close.reason);
			}
#endif
    		break;
    	case ESP_GATTC_DISCONNECT_EVT:
#if LOG_GATTC
 			LOG_GATTC_MSG("ESP_GATTC_DISCONNECT_EVT, reason =");
			LOG_GATTC_INT(param->disconnect.reason);
#endif
   			break;
		case ESP_GATTC_SEARCH_RES_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->search_res, sizeof(struct gattc_search_res_evt_param), gattcSearchResultEvent, NULL);
			break;
		case ESP_GATTC_SEARCH_CMPL_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->search_cmpl, sizeof(struct gattc_search_cmpl_evt_param), gattcSearchCompleteEvent, NULL);
			break;
		case ESP_GATTC_REG_FOR_NOTIFY_EVT:
			if (param->reg_for_notify.status == ESP_GATT_OK) {
 				modCriticalSectionBegin();
				modBLEConnection connection = modBLEConnectionFindByInterface(gattc_if);
 				modCriticalSectionEnd();
 				if (connection)
					modMessagePostToMachine(gBLE->the, (uint8_t*)&param->reg_for_notify, sizeof(struct gattc_reg_for_notify_evt_param), gattcRegisterNotifyEvent, connection);
#if LOG_GATTC
				else
					LOG_GATTC_MSG("ESP_GATTC_REG_FOR_NOTIFY_EVT failed to find connection");
#endif
			}
#if LOG_GATTC
			else {
				LOG_GATTC_MSG("ESP_GATTC_REG_FOR_NOTIFY_EVT failed, status =");
				LOG_GATTC_INT(param->reg_for_notify.status);
			}
#endif
			break;
		case ESP_GATTC_NOTIFY_EVT:
			if (param->notify.value_len) {
				uint8_t *value = c_malloc(param->notify.value_len);
				if (NULL != value) {
					struct gattc_notify_evt_param notify = param->notify;
					c_memmove(value, param->notify.value, param->notify.value_len);
					notify.value = value;
					modMessagePostToMachine(gBLE->the, (uint8_t*)&notify, sizeof(struct gattc_notify_evt_param), gattcNotifyEvent, NULL);
				}
			}
			break;
		case ESP_GATTC_READ_CHAR_EVT:
		case ESP_GATTC_READ_DESCR_EVT:
			if (param->read.value_len) {
				uint8_t *value = c_malloc(param->read.value_len);
				if (NULL != value) {
					struct gattc_read_char_evt_param read = param->read;
					c_memmove(value, param->read.value, param->read.value_len);
					read.value = value;
					modMessagePostToMachine(gBLE->the, (uint8_t*)&read, sizeof(struct gattc_read_char_evt_param), gattcReadCharEvent, (void*)event);
				}
			}
			break;
	}
}

static void logGATTCEvent(esp_gattc_cb_event_t event) {
	switch(event) {
		case ESP_GATTC_REG_EVT: modLog("ESP_GATTC_REG_EVT"); break;
		case ESP_GATTC_UNREG_EVT: modLog("ESP_GATTC_UNREG_EVT"); break;
		case ESP_GATTC_OPEN_EVT: modLog("ESP_GATTC_OPEN_EVT"); break;
		case ESP_GATTC_READ_CHAR_EVT: modLog("ESP_GATTC_READ_CHAR_EVT"); break;
		case ESP_GATTC_WRITE_CHAR_EVT: modLog("ESP_GATTC_WRITE_CHAR_EVT"); break;
		case ESP_GATTC_CLOSE_EVT: modLog("ESP_GATTC_CLOSE_EVT"); break;
		case ESP_GATTC_SEARCH_CMPL_EVT: modLog("ESP_GATTC_SEARCH_CMPL_EVT"); break;
		case ESP_GATTC_SEARCH_RES_EVT: modLog("ESP_GATTC_SEARCH_RES_EVT"); break;
		case ESP_GATTC_READ_DESCR_EVT: modLog("ESP_GATTC_READ_DESCR_EVT"); break;
		case ESP_GATTC_WRITE_DESCR_EVT: modLog("ESP_GATTC_WRITE_DESCR_EVT"); break;
		case ESP_GATTC_NOTIFY_EVT: modLog("ESP_GATTC_NOTIFY_EVT"); break;
		case ESP_GATTC_PREP_WRITE_EVT: modLog("ESP_GATTC_PREP_WRITE_EVT"); break;
		case ESP_GATTC_EXEC_EVT: modLog("ESP_GATTC_EXEC_EVT"); break;
		case ESP_GATTC_ACL_EVT: modLog("ESP_GATTC_ACL_EVT"); break;
		case ESP_GATTC_CANCEL_OPEN_EVT: modLog("ESP_GATTC_CANCEL_OPEN_EVT"); break;
		case ESP_GATTC_SRVC_CHG_EVT: modLog("ESP_GATTC_SRVC_CHG_EVT"); break;
		case ESP_GATTC_ENC_CMPL_CB_EVT: modLog("ESP_GATTC_ENC_CMPL_CB_EVT"); break;
		case ESP_GATTC_CFG_MTU_EVT: modLog("ESP_GATTC_CFG_MTU_EVT"); break;
		case ESP_GATTC_ADV_DATA_EVT: modLog("ESP_GATTC_ADV_DATA_EVT"); break;
		case ESP_GATTC_MULT_ADV_ENB_EVT: modLog("ESP_GATTC_MULT_ADV_ENB_EVT"); break;
		case ESP_GATTC_MULT_ADV_UPD_EVT: modLog("ESP_GATTC_MULT_ADV_UPD_EVT"); break;
		case ESP_GATTC_MULT_ADV_DATA_EVT: modLog("ESP_GATTC_MULT_ADV_DATA_EVT"); break;
		case ESP_GATTC_MULT_ADV_DIS_EVT: modLog("ESP_GATTC_MULT_ADV_DIS_EVT"); break;
		case ESP_GATTC_CONGEST_EVT: modLog("ESP_GATTC_CONGEST_EVT"); break;
		case ESP_GATTC_BTH_SCAN_ENB_EVT: modLog("ESP_GATTC_BTH_SCAN_ENB_EVT"); break;
		case ESP_GATTC_BTH_SCAN_CFG_EVT: modLog("ESP_GATTC_BTH_SCAN_CFG_EVT"); break;
		case ESP_GATTC_BTH_SCAN_RD_EVT: modLog("ESP_GATTC_BTH_SCAN_RD_EVT"); break;
		case ESP_GATTC_BTH_SCAN_THR_EVT: modLog("ESP_GATTC_BTH_SCAN_THR_EVT"); break;
		case ESP_GATTC_BTH_SCAN_PARAM_EVT: modLog("ESP_GATTC_BTH_SCAN_PARAM_EVT"); break;
		case ESP_GATTC_BTH_SCAN_DIS_EVT: modLog("ESP_GATTC_BTH_SCAN_DIS_EVT"); break;
		case ESP_GATTC_SCAN_FLT_CFG_EVT: modLog("ESP_GATTC_SCAN_FLT_CFG_EVT"); break;
		case ESP_GATTC_SCAN_FLT_PARAM_EVT: modLog("ESP_GATTC_SCAN_FLT_PARAM_EVT"); break;
		case ESP_GATTC_SCAN_FLT_STATUS_EVT: modLog("ESP_GATTC_SCAN_FLT_STATUS_EVT"); break;
		case ESP_GATTC_ADV_VSC_EVT: modLog("ESP_GATTC_ADV_VSC_EVT"); break;
		case ESP_GATTC_REG_FOR_NOTIFY_EVT: modLog("ESP_GATTC_REG_FOR_NOTIFY_EVT"); break;
		case ESP_GATTC_UNREG_FOR_NOTIFY_EVT: modLog("ESP_GATTC_UNREG_FOR_NOTIFY_EVT"); break;
		case ESP_GATTC_CONNECT_EVT: modLog("ESP_GATTC_CONNECT_EVT"); break;
		case ESP_GATTC_DISCONNECT_EVT: modLog("ESP_GATTC_DISCONNECT_EVT"); break;
		case ESP_GATTC_READ_MULTIPLE_EVT: modLog("ESP_GATTC_READ_MULTIPLE_EVT"); break;
		case ESP_GATTC_QUEUE_FULL_EVT: modLog("ESP_GATTC_QUEUE_FULL_EVT"); break;
	}
}
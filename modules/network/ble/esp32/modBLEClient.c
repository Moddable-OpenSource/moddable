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
#else
	#define LOG_GATTC_EVENT(event)
	#define LOG_GATTC_MSG(msg)
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

static modBLE gBLE = NULL;
static int gAPP_ID = 0;

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
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg))
	ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
	ESP_ERROR_CHECK(esp_bluedroid_init());
	ESP_ERROR_CHECK(esp_bluedroid_enable());

	// Register callbacks
	ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gattc_register_callback(gattc_event_handler));
    
	// Stack is ready
	xsCall1(gBLE->obj, xsID_callback, xsString("_onReady"));
}

void xs_ble_client_close(xsMachine *the)
{
	gBLE->terminating = true;
	modBLEConnection connections = gBLE->connections, next;
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
	xsForget(gBLE->obj);
	xs_ble_client_destructor(gBLE);
	esp_bluedroid_disable();
	esp_bluedroid_deinit();
	esp_bt_controller_deinit();
}

void xs_ble_client_destructor(void *data)
{
	modBLE ble = data;
	if (ble) {
		c_free(ble);
	}
	gBLE = NULL;
}

void xs_ble_client_set_device_name(xsMachine *the)
{
	esp_ble_gap_set_device_name(xsmcToString(xsArg(0)));
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
			esp_ble_gattc_app_unregister(connection->gattc_if);
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

void xs_gatt_service_discover_characteristics(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t start = xsmcToInteger(xsArg(1));
	uint16_t end = xsmcToInteger(xsArg(2));
	uint16_t count = 0;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	esp_ble_gattc_get_attr_count(connection->gattc_if, conn_id, ESP_GATT_DB_CHARACTERISTIC, start, end, 0, &count);
	if (count > 0) {
		xsmcVars(4);
		esp_gattc_char_elem_t *char_elem_result = c_malloc(sizeof(esp_gattc_char_elem_t) * count);
		if (!char_elem_result)
			xsUnknownError("out of memory");
		if (argc > 3) {
			esp_bt_uuid_t uuid;
			bufferToUUID(&uuid, (uint8_t*)xsmcToArrayBuffer(xsArg(3)), xsGetArrayBufferLength(xsArg(3)));
			esp_ble_gattc_get_char_by_uuid(connection->gattc_if, conn_id, start, end, uuid, char_elem_result, &count);
		}
		else
			esp_ble_gattc_get_all_char(connection->gattc_if, conn_id, start, end, char_elem_result, &count, 0);
		for (int i = 0; i < count; ++i) {
			uint16_t length;
			uint8_t buffer[ESP_UUID_LEN_128];
			esp_gattc_char_elem_t *char_elem = &char_elem_result[i];
			uuidToBuffer(buffer, &char_elem->uuid, &length);
			xsVar(0) = xsmcNewObject();
			xsmcSetArrayBuffer(xsVar(1), buffer, length);
			xsmcSetInteger(xsVar(2), char_elem->char_handle);
			xsmcSetInteger(xsVar(3), char_elem->properties);
			xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
			xsmcSet(xsVar(0), xsID_handle, xsVar(2));
			xsmcSet(xsVar(0), xsID_properties, xsVar(3));
			xsCall2(xsThis, xsID_callback, xsString("_onCharacteristic"), xsVar(0));
		}
		c_free(char_elem_result);
	}
	xsCall1(xsThis, xsID_callback, xsString("_onCharacteristic"));	// procedure complete
}

void xs_gatt_characteristic_discover_all_characteristic_descriptors(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
    uint16_t count = 0;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	esp_ble_gattc_get_attr_count(connection->gattc_if, conn_id, ESP_GATT_DB_DESCRIPTOR, 0, 0, handle, &count);
	if (count > 0) {
		xsmcVars(3);
		esp_gattc_descr_elem_t *descr_elem_result = c_malloc(sizeof(esp_gattc_descr_elem_t) * count);
		if (!descr_elem_result)
			xsUnknownError("out of memory");
		esp_ble_gattc_get_all_descr(connection->gattc_if, conn_id, handle, descr_elem_result, &count, 0);
		for (int i = 0; i < count; ++i) {
			uint16_t length;
			uint8_t buffer[ESP_UUID_LEN_128];
			esp_gattc_descr_elem_t *descr_elem = &descr_elem_result[i];
			uuidToBuffer(buffer, &descr_elem->uuid, &length);
			xsVar(0) = xsmcNewObject();
			xsmcSetArrayBuffer(xsVar(1), buffer, length);
			xsmcSetInteger(xsVar(2), descr_elem->handle);
			xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
			xsmcSet(xsVar(0), xsID_handle, xsVar(2));
			xsCall2(xsThis, xsID_callback, xsString("_onDescriptor"), xsVar(0));
		}
		c_free(descr_elem_result);
	}
	xsCall1(xsThis, xsID_callback, xsString("_onDescriptor"));	// procedure complete
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
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	esp_gatt_auth_req_t auth_req = ESP_GATT_AUTH_REQ_NONE;
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
modLog("xs_gatt_characteristic_disable_notifications - unregister for notify");
	esp_ble_gattc_unregister_for_notify(connection->gattc_if, connection->bda, handle);
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
		buffer[1] = uuid->uuid.uuid16 & 0xFF;
		buffer[0] = (uuid->uuid.uuid16 >> 8) & 0xFF;
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
		for (uint8_t i = 0; i < ESP_UUID_LEN_128; ++i)
			buffer[i] = uuid->uuid.uuid128[ESP_UUID_LEN_128 - 1 - i];
	}
}

void bufferToUUID(esp_bt_uuid_t *uuid, uint8_t *buffer, uint16_t length)
{
	if (length == ESP_UUID_LEN_16) {
		uuid->uuid.uuid16 = buffer[1] | (buffer[0] << 8);
	}
	else if (length == ESP_UUID_LEN_32) {
		uuid->uuid.uuid32 = buffer[3] | (buffer[2] << 8) | (buffer[1] << 16) | (buffer[0] << 24);
	}
	else {
		for (uint8_t i = 0; i < ESP_UUID_LEN_128; ++i)
			uuid->uuid.uuid128[i] = buffer[ESP_UUID_LEN_128 - 1 - i];
	}
	uuid->len = length;
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
	xsCall2(gBLE->obj, xsID_callback, xsString("_onDiscovered"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void rssiCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_read_rssi_cmpl_evt_param *read_rssi_cmpl = (struct ble_read_rssi_cmpl_evt_param *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAddress(&read_rssi_cmpl->remote_addr);
	if (!connection)
		xsUnknownError("connection not found");
	xsCall2(connection->objConnection, xsID_callback, xsString("_onRSSI"), xsInteger(read_rssi_cmpl->rssi));
	xsEndHost(gBLE->the);
}

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	switch(event) {
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
        	break;
		default:
			break;
    }
}

static void gattcRegisterEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_reg_evt_param *reg = (struct gattc_reg_evt_param *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAppID(reg->app_id);
	if (!connection)
		xsUnknownError("connection not found");
	esp_ble_gattc_open(connection->gattc_if, connection->bda, true);
	xsEndHost(gBLE->the);
}

static void gattcOpenEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_open_evt_param *open = (struct gattc_open_evt_param *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAddress(&open->remote_bda);
	if (!connection)
		xsUnknownError("connection not found");
		
	// Ignore duplicate connection events
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
	xsCall2(gBLE->obj, xsID_callback, xsString("_onConnected"), xsVar(0));
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
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), close->conn_id);
	xsCall2(connection->objConnection, xsID_callback, xsString("_onDisconnected"), xsVar(0));
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
	xsCall2(connection->objClient, xsID_callback, xsString("_onService"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gattcSearchCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_search_cmpl_evt_param *search_cmpl = (struct gattc_search_cmpl_evt_param *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(search_cmpl->conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	xsCall1(connection->objClient, xsID_callback, xsString("_onService"));
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
	xsEndHost(gBLE->the);
}

static void gattcNotifyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_notify_evt_param *notify = (struct gattc_notify_evt_param *)message;
	doCharEvent(the, "_onCharacteristicNotification", notify->conn_id, notify->handle, notify->value, notify->value_len);
}

static void gattcReadCharEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_read_char_evt_param *read = (struct gattc_read_char_evt_param *)message;
	doCharEvent(the, "_onCharacteristicValue", read->conn_id, read->handle, read->value, read->value_len);
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
	
    switch (event) {
		case ESP_GATTC_REG_EVT:
        	if (param->reg.status == ESP_GATT_OK) {
				modCriticalSectionBegin();
				modBLEConnection connection = modBLEConnectionFindByAppID(param->reg.app_id);
				if (connection)
					connection->gattc_if = gattc_if;
				modCriticalSectionEnd();
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->reg, sizeof(struct gattc_reg_evt_param), gattcRegisterEvent, NULL);
        	}
        	else
				modLogInt(param->reg.status);
        	break;
    	case ESP_GATTC_OPEN_EVT:
        	if (param->open.status == ESP_GATT_OK)
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->open, sizeof(struct gattc_open_evt_param), gattcOpenEvent, NULL);
    		break;
    	case ESP_GATTC_CLOSE_EVT:
        	if (param->close.status == ESP_GATT_OK)
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->close, sizeof(struct gattc_close_evt_param), gattcCloseEvent, NULL);
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
			}
			break;
		case ESP_GATTC_NOTIFY_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->notify, sizeof(struct gattc_notify_evt_param), gattcNotifyEvent, NULL);
			break;
		case ESP_GATTC_READ_CHAR_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->read, sizeof(struct gattc_read_char_evt_param), gattcReadCharEvent, NULL);
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
		case ESP_GATTC_READ_MUTIPLE_EVT: modLog("ESP_GATTC_READ_MUTIPLE_EVT"); break;
		case ESP_GATTC_QUEUE_FULL_EVT: modLog("ESP_GATTC_QUEUE_FULL_EVT"); break;
	}
}
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

#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

enum {
	IDLE = 0,
	ADVERTISING,
	SCANNING,
	CONNECTING,
	CONNECTED,
	SERVICES
};

typedef struct modBLEConnectionRecord modBLEConnectionRecord;
typedef modBLEConnectionRecord *modBLEConnection;

struct modBLEConnectionRecord {
	struct modBLEConnectionRecord  *next;

	xsMachine	*the;
	
	xsSlot		objConnection;
	xsSlot		objClient;

	esp_bd_addr_t bda;
	esp_gatt_if_t gattc_if;
	uint16_t conn_id;
	uint16_t app_id;
};

typedef struct {
	xsMachine	*the;
	xsSlot		obj;

	uint16_t state;
	
	// server
	esp_gatt_if_t gatts_if;
	uint8_t *advertisingData;
	uint8_t *scanResponseData;
	esp_ble_adv_params_t adv_params;
	esp_ble_scan_params_t scan_params;
	uint8_t adv_config_done;

	// client connections
	modBLEConnection connections;
} modBLERecord, *modBLE;

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

static void modBLEConnectionAdd(modBLEConnection connection);
static void modBLEConnectionRemove(modBLEConnection connection);
static modBLEConnection modBLEConnectionFindByConnectionID(uint16_t conn_id);
static modBLEConnection modBLEConnectionFindByAppID(uint16_t app_id);
static modBLEConnection modBLEConnectionFindByAddress(esp_bd_addr_t *bda);

static void uuidToBuffer(esp_bt_uuid_t *uuid, uint8_t *buffer, uint16_t *length);

// @@ The ESP32 BT APIs don't support a refcon to tuck away this kind of stuff for use in callbacks...
static modBLE gBLE = NULL;
static int gAPP_ID = 0;

void xs_ble_initialize(xsMachine *the)
{
	if (NULL != gBLE)
		xsUnknownError("BLE already initialized");
	gBLE = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!gBLE)
		xsUnknownError("no memory");
	gBLE->the = the;
	gBLE->obj = xsThis;
	
	// Initialize platform Bluetooth modules
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg))
	ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
	ESP_ERROR_CHECK(esp_bluedroid_init());
	ESP_ERROR_CHECK(esp_bluedroid_enable());

	// Register callbacks
	ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    //ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gattc_register_callback(gattc_event_handler));
    
   // ESP_ERROR_CHECK(esp_ble_gatts_app_register(gAPP_ID));

	// Stack is ready
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
}

void xs_ble_close(xsMachine *the)
{
	xs_ble_destructor(gBLE);
}

void xs_ble_destructor(void *data)
{
	modBLE ble = data;
	if (ble) {
		if (gBLE->advertisingData)
			c_free(gBLE->advertisingData);
		if (gBLE->scanResponseData)
			c_free(gBLE->scanResponseData);
		c_free(ble);
	}
	gBLE = NULL;
}

void xs_ble_purge(xsMachine *the)
{
	xsCollectGarbage();
}

void xs_ble_set_device_name(xsMachine *the)
{
	char *deviceName = xsmcToString(xsArg(0));
	esp_ble_gap_set_device_name(deviceName);
}

void xs_ble_start_advertising(xsMachine *the)
{
	uint32_t intervalMin = xsmcToInteger(xsArg(0));
	uint32_t intervalMax = xsmcToInteger(xsArg(1));
	uint8_t *advertisingData = (uint8_t*)xsmcToArrayBuffer(xsArg(2));
	uint32_t advertisingDataLength = xsGetArrayBufferLength(xsArg(2));
	uint8_t *scanResponseData = xsmcTest(xsArg(3)) ? (uint8_t*)xsmcToArrayBuffer(xsArg(3)) : NULL;
	uint32_t scanResponseDataLength = xsmcTest(xsArg(3)) ? xsGetArrayBufferLength(xsArg(3)) : 0;
	
	// Save the advertising and scan response data. The buffers cannot be freed until the GAP callback confirmation.
	gBLE->advertisingData = (uint8_t*)c_malloc(advertisingDataLength);
	if (!gBLE->advertisingData)
		xsUnknownError("no memory");
	c_memmove(gBLE->advertisingData, advertisingData, advertisingDataLength);
	if (scanResponseData) {
		gBLE->scanResponseData = (uint8_t*)c_malloc(scanResponseDataLength);
		if (!gBLE->scanResponseData)
			xsUnknownError("no memory");
		c_memmove(gBLE->scanResponseData, scanResponseData, scanResponseDataLength);
	}
	
	// Initialize the advertising parameters
	gBLE->adv_params.adv_int_min = intervalMin;
	gBLE->adv_params.adv_int_max = intervalMax;
	gBLE->adv_params.adv_type = ADV_TYPE_IND;
	gBLE->adv_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
	gBLE->adv_params.channel_map = ADV_CHNL_ALL;
	gBLE->adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;

	// Set the advertising and scan response data
	gBLE->adv_config_done = adv_config_flag;
	esp_ble_gap_config_adv_data_raw(advertisingData, advertisingDataLength);
	if (scanResponseData) {
		gBLE->adv_config_done |= scan_rsp_config_flag;
		esp_ble_gap_config_scan_rsp_data_raw(scanResponseData, scanResponseDataLength);
	}
}
	

void xs_ble_stop_advertising(xsMachine *the)
{
	esp_ble_gap_stop_advertising();
}

void xs_ble_start_scanning(xsMachine *the)
{
	uint8_t active = xsmcToBoolean(xsArg(0));
	uint32_t interval = xsmcToInteger(xsArg(1));
	uint32_t window = xsmcToInteger(xsArg(2));
	
	// Set the scan parameters
	gBLE->scan_params.scan_type = active ? BLE_SCAN_TYPE_ACTIVE : BLE_SCAN_TYPE_PASSIVE;
	gBLE->scan_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
	gBLE->scan_params.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
	gBLE->scan_params.scan_interval = interval;
	gBLE->scan_params.scan_window = window;
	esp_ble_gap_set_scan_params(&gBLE->scan_params);
}

void xs_ble_stop_scanning(xsMachine *the)
{
	esp_ble_gap_stop_scanning();
}

void xs_ble_connect(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	
	// Add a new connection record to be filled as the connection completes
	modBLEConnection connection = c_calloc(sizeof(modBLEConnectionRecord), 1);
	if (!connection)
		xsUnknownError("out of memory");
	connection->app_id = gAPP_ID++;
	connection->gattc_if = ESP_GATT_IF_NONE;
	c_memmove(connection->bda, address, sizeof(esp_bd_addr_t));
	modBLEConnectionAdd(connection);
	
	gBLE->state = CONNECTING;

	// register application client and connect when ESP_GATTC_REG_EVT received
    ESP_ERROR_CHECK(esp_ble_gattc_app_register(connection->app_id));
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
			c_free(connection);
			break;
		}
	}
}

void xs_gap_connection_initialize(xsMachine *the)
{
	modBLEConnection connection;
	uint16_t conn_id;
	xsmcVars(1);	// xsArg(0) is client
	xsmcGet(xsVar(0), xsArg(0), xsID_connection);
	conn_id = xsmcToInteger(xsVar(0));
	connection = modBLEConnectionFindByConnectionID(conn_id);
	if (connection) {
		connection->the = the;
		connection->objConnection = xsThis;
		connection->objClient = xsArg(0);
	}
}
	
void xs_gap_connection_disconnect(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (connection) {
		esp_ble_gap_disconnect(connection->bda);
	}
}

void xs_gap_connection_read_rssi(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (connection)
		esp_ble_gap_read_rssi(connection->bda);
}

void xs_gatt_client_initialize(xsMachine *the)
{
}

void xs_gatt_client_discover_all_primary_services(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (connection)
		esp_ble_gattc_search_service(connection->gattc_if, conn_id, NULL);
}

void xs_gatt_service_discover_all_characteristics(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t start = xsmcToInteger(xsArg(1));
	uint16_t end = xsmcToInteger(xsArg(2));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (connection) {
        uint16_t count  = 0;
		ESP_ERROR_CHECK(esp_ble_gattc_get_attr_count(connection->gattc_if, conn_id, ESP_GATT_DB_CHARACTERISTIC, start, end, 0, &count));
		if (count > 0) {
			xsmcVars(4);
            esp_gattc_char_elem_t *char_elem_result = c_malloc(sizeof(esp_gattc_char_elem_t) * count);
			if (!char_elem_result)
				xsUnknownError("out of memory");
			ESP_ERROR_CHECK(esp_ble_gattc_get_all_char(connection->gattc_if, conn_id, start, end, char_elem_result, &count, 0));
			for (int i = 0; i < count; ++i) {
				uint16_t uuid_length;
				uint8_t uuid[ESP_UUID_LEN_128];
				esp_gattc_char_elem_t *char_elem = &char_elem_result[i];
				uuidToBuffer(&char_elem->uuid, uuid, &uuid_length);
				xsVar(0) = xsmcNewObject();
				xsmcSetArrayBuffer(xsVar(1), uuid, uuid_length);
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
}

void xs_gatt_characteristic_discover_all_characteristic_descriptors(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (connection) {
        uint16_t count  = 0;
		ESP_ERROR_CHECK(esp_ble_gattc_get_attr_count(connection->gattc_if, conn_id, ESP_GATT_DB_DESCRIPTOR, 0, 0, handle, &count));
		if (count > 0) {
			xsmcVars(3);
            esp_gattc_descr_elem_t *descr_elem_result = c_malloc(sizeof(esp_gattc_descr_elem_t) * count);
			if (!descr_elem_result)
				xsUnknownError("out of memory");
			ESP_ERROR_CHECK(esp_ble_gattc_get_all_descr(connection->gattc_if, conn_id, handle, descr_elem_result, &count, 0));
			for (int i = 0; i < count; ++i) {
				uint16_t uuid_length;
				uint8_t uuid[ESP_UUID_LEN_128];
				esp_gattc_descr_elem_t *descr_elem = &descr_elem_result[i];
				uuidToBuffer(&descr_elem->uuid, uuid, &uuid_length);
				xsVar(0) = xsmcNewObject();
				xsmcSetArrayBuffer(xsVar(1), uuid, uuid_length);
				xsmcSetInteger(xsVar(2), descr_elem->handle);
				xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
				xsmcSet(xsVar(0), xsID_handle, xsVar(2));
				xsCall2(xsThis, xsID_callback, xsString("_onDescriptor"), xsVar(0));
			}
			c_free(descr_elem_result);
		}
		xsCall1(xsThis, xsID_callback, xsString("_onDescriptor"));	// procedure complete
	}
}

void xs_gatt_descriptor_write_value(xsMachine *the)
{
}

void uuidToBuffer(esp_bt_uuid_t *uuid, uint8_t *buffer, uint16_t *length)
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
		c_memmove(buffer, uuid->uuid.uuid128, ESP_UUID_LEN_128);
	}
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
		if (connection)
			xsCall2(connection->objConnection, xsID_callback, xsString("onRSSI"), xsInteger(read_rssi_cmpl->rssi));
	xsEndHost(gBLE->the);
}

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	xsBeginHost(gBLE->the);

	switch(event) {
		case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
			gBLE->adv_config_done &= ~adv_config_flag;
			if (0 == gBLE->adv_config_done)
				esp_ble_gap_start_advertising(&gBLE->adv_params);
        	break;
		case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
			gBLE->adv_config_done &= ~scan_rsp_config_flag;
			if (0 == gBLE->adv_config_done)
				esp_ble_gap_start_advertising(&gBLE->adv_params);
			break;
		case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:	// The advertising data can be freed after advertising starts
			c_free(gBLE->advertisingData);
			gBLE->advertisingData = NULL;
			if (gBLE->scanResponseData) {
				c_free(gBLE->scanResponseData);
				gBLE->scanResponseData = NULL;
			}
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
        	break;
		default:
			break;
    }
    
	xsEndHost(gBLE->the);
}

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	xsBeginHost(gBLE->the);
		switch(event) {
			case ESP_GATTS_REG_EVT:
				gBLE->gatts_if = gatts_if;
				break;
			case ESP_GATTS_CONNECT_EVT:
				break;
			case ESP_GATTS_DISCONNECT_EVT:
				break;
		}
	xsEndHost(gBLE->the);
}

static void gattcConnectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_connect_evt_param *connect = (struct gattc_connect_evt_param *)message;
	xsBeginHost(gBLE->the);
//	if (gBLE->state == CONNECTING) {
		modBLEConnection connection = modBLEConnectionFindByAddress(&connect->remote_bda);
		if (connection) {
			connection->conn_id = connect->conn_id;
			//gBLE->state = CONNECTED;
			xsmcVars(3);
			xsVar(0) = xsmcNewObject();
			xsmcSetInteger(xsVar(1), connect->conn_id);
			xsmcSet(xsVar(0), xsID_connection, xsVar(1));
			xsmcSetArrayBuffer(xsVar(2), connect->remote_bda, 6);
			xsmcSet(xsVar(0), xsID_address, xsVar(2));
			xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));
		}
//	}
	xsEndHost(gBLE->the);
}

static void gattcDisconnectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_disconnect_evt_param *disconnect = (struct gattc_disconnect_evt_param *)message;
	xsBeginHost(gBLE->the);
		modBLEConnection connection = modBLEConnectionFindByConnectionID(disconnect->conn_id);
		if (connection) {
			xsCall1(connection->objConnection, xsID_callback, xsString("onDisconnected"));
			modBLEConnectionRemove(connection);
		}
	xsEndHost(gBLE->the);
}

static void gattcSearchResultEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_search_res_evt_param *search_res = (struct gattc_search_res_evt_param *)message;
	xsBeginHost(gBLE->the);
		modBLEConnection connection = modBLEConnectionFindByConnectionID(search_res->conn_id);
		if (connection) {
			uint16_t uuid_length;
			uint8_t uuid[ESP_UUID_LEN_128];
			uuidToBuffer(&search_res->srvc_id.uuid, uuid, &uuid_length);
			xsmcVars(4);
			xsVar(0) = xsmcNewObject();
			xsmcSetArrayBuffer(xsVar(1), uuid, uuid_length);
			xsmcSetInteger(xsVar(2), search_res->start_handle);
			xsmcSetInteger(xsVar(3), search_res->end_handle);
			xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
			xsmcSet(xsVar(0), xsID_start, xsVar(2));
			xsmcSet(xsVar(0), xsID_end, xsVar(3));
			xsCall2(connection->objClient, xsID_callback, xsString("_onService"), xsVar(0));
		}
	xsEndHost(gBLE->the);
}

static void gattcSearchCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gattc_search_cmpl_evt_param *search_cmpl = (struct gattc_search_cmpl_evt_param *)message;
	xsBeginHost(gBLE->the);
		modBLEConnection connection = modBLEConnectionFindByConnectionID(search_cmpl->conn_id);
		if (connection)
			xsCall1(connection->objClient, xsID_callback, xsString("_onService"));
	xsEndHost(gBLE->the);
}

void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
	xsBeginHost(gBLE->the);
	
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
		case ESP_GATTC_REG_EVT:
        	if (param->reg.status == ESP_GATT_OK) {
				modBLEConnection connection = modBLEConnectionFindByAppID(p_data->reg.app_id);
				if (connection) {
					connection->gattc_if = gattc_if;
					esp_ble_gattc_open(gattc_if, connection->bda, true);
				}
        	}
        	break;
    	case ESP_GATTC_CONNECT_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->connect, sizeof(struct gattc_connect_evt_param), gattcConnectEvent, gBLE);
    		break;
    	case ESP_GATTC_DISCONNECT_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->disconnect, sizeof(struct gattc_disconnect_evt_param), gattcDisconnectEvent, gBLE);
    		break;
		case ESP_GATTC_SEARCH_RES_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->search_res, sizeof(struct gattc_search_res_evt_param), gattcSearchResultEvent, gBLE);
			break;
		case ESP_GATTC_SEARCH_CMPL_EVT: {
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->search_cmpl, sizeof(struct gattc_search_cmpl_evt_param), gattcSearchCompleteEvent, gBLE);
			break;
		}
	}

	xsEndHost(gBLE->the);
}


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

#define PROFILE_A_APP_ID 0

enum {
	IDLE = 0,
	ADVERTISING,
	SCANNING,
	CONNECTING,
	CONNECTED,
	SERVICES
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

	// client
	esp_gatt_if_t gattc_if;
	uint16_t conn_id;
} modBLERecord, *modBLE;

typedef struct {
	xsMachine	*the;
	xsSlot		obj;

	esp_gatt_if_t gattc_if;
	uint16_t conn_id;
} modBLEConnectionRecord, *modBLEConnection;

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

// @@ The ESP32 BT APIs don't support a refcon to tuck away this kind of stuff...
static xsMachine *gThe;
static xsSlot gThis;


void xs_ble_initialize(xsMachine *the)
{
	modBLE ble;
	ble = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!ble)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, ble);
	ble->the = gThe = the;
	ble->obj = gThis = xsThis;
	
	// Initialize platform Bluetooth modules
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg))
	ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
	ESP_ERROR_CHECK(esp_bluedroid_init());
	ESP_ERROR_CHECK(esp_bluedroid_enable());

	// Register callbacks
	ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gattc_register_callback(gattc_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(PROFILE_A_APP_ID));
    ESP_ERROR_CHECK(esp_ble_gattc_app_register(PROFILE_A_APP_ID));

	// Stack is ready
	xsCall1(ble->obj, xsID_callback, xsString("onReady"));
}

void xs_ble_close(xsMachine *the)
{
	modBLE ble = (modBLE)xsmcGetHostData(xsThis);
	xs_ble_destructor(ble);
	xsmcSetHostData(xsThis, NULL);
}

void xs_ble_destructor(void *data)
{
	modBLE ble = data;
	if (ble) {
		if (ble->advertisingData)
			c_free(ble->advertisingData);
		if (ble->scanResponseData)
			c_free(ble->scanResponseData);
		c_free(ble);
	}
}

void xs_ble_set_device_name(xsMachine *the)
{
	char *deviceName = xsmcToString(xsArg(0));
	esp_ble_gap_set_device_name(deviceName);
}

void xs_ble_start_advertising(xsMachine *the)
{
	modBLE ble = (modBLE)xsmcGetHostData(xsThis);
	uint32_t intervalMin = xsmcToInteger(xsArg(0));
	uint32_t intervalMax = xsmcToInteger(xsArg(1));
	uint8_t *advertisingData = (uint8_t*)xsmcToArrayBuffer(xsArg(2));
	uint32_t advertisingDataLength = xsGetArrayBufferLength(xsArg(2));
	uint8_t *scanResponseData = xsmcTest(xsArg(3)) ? (uint8_t*)xsmcToArrayBuffer(xsArg(3)) : NULL;
	uint32_t scanResponseDataLength = xsmcTest(xsArg(3)) ? xsGetArrayBufferLength(xsArg(3)) : 0;
	
	// Save the advertising and scan response data. The buffers cannot be freed until the GAP callback confirmation.
	ble->advertisingData = (uint8_t*)c_malloc(advertisingDataLength);
	if (!ble->advertisingData)
		xsUnknownError("no memory");
	c_memmove(ble->advertisingData, advertisingData, advertisingDataLength);
	if (scanResponseData) {
		ble->scanResponseData = (uint8_t*)c_malloc(scanResponseDataLength);
		if (!ble->scanResponseData)
			xsUnknownError("no memory");
		c_memmove(ble->scanResponseData, scanResponseData, scanResponseDataLength);
	}
	
	// Initialize the advertising parameters
	ble->adv_params.adv_int_min = intervalMin;
	ble->adv_params.adv_int_max = intervalMax;
	ble->adv_params.adv_type = ADV_TYPE_IND;
	ble->adv_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
	ble->adv_params.channel_map = ADV_CHNL_ALL;
	ble->adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;

	// Set the advertising and scan response data
	ble->adv_config_done = adv_config_flag;
	esp_ble_gap_config_adv_data_raw(advertisingData, advertisingDataLength);
	if (scanResponseData) {
		ble->adv_config_done |= scan_rsp_config_flag;
		esp_ble_gap_config_scan_rsp_data_raw(scanResponseData, scanResponseDataLength);
	}
}
	

void xs_ble_stop_advertising(xsMachine *the)
{
	esp_ble_gap_stop_advertising();
}

void xs_ble_start_scanning(xsMachine *the)
{
	modBLE ble = (modBLE)xsmcGetHostData(xsThis);
	uint8_t active = xsmcToBoolean(xsArg(0));
	uint32_t interval = xsmcToInteger(xsArg(1));
	uint32_t window = xsmcToInteger(xsArg(2));
	
	// Set the scan parameters
	ble->scan_params.scan_type = active ? BLE_SCAN_TYPE_ACTIVE : BLE_SCAN_TYPE_PASSIVE;
	ble->scan_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
	ble->scan_params.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
	ble->scan_params.scan_interval = interval;
	ble->scan_params.scan_window = window;
	esp_ble_gap_set_scan_params(&ble->scan_params);
}

void xs_ble_stop_scanning(xsMachine *the)
{
	esp_ble_gap_stop_scanning();
}

void xs_ble_connect(xsMachine *the)
{
	modBLE ble = (modBLE)xsmcGetHostData(xsThis);
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	esp_bd_addr_t bda;
	c_memmove(bda, address, sizeof(bda));
	ble->state = CONNECTING;
    esp_ble_gattc_open(ble->gattc_if, bda, true);
}

void xs_ble_connection_initialize(xsMachine *the)
{
}
	
void xs_ble_connection_disconnect(xsMachine *the)
{
}

void xs_ble_gatt_client_discover_all_primary_services(xsMachine *the)
{
	esp_gatt_if_t gattc_if = xsmcToInteger(xsArg(0));
	uint16_t conn_id = xsmcToInteger(xsArg(1));
modLog("discover all primary services");
	esp_ble_gattc_search_service(gattc_if, conn_id, NULL);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	xsBeginHost(gThe);
	
	modBLE ble = (modBLE)xsmcGetHostData(gThis);

	switch(event) {
		case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
			ble->adv_config_done &= ~adv_config_flag;
			if (0 == ble->adv_config_done)
				esp_ble_gap_start_advertising(&ble->adv_params);
        	break;
		case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
			ble->adv_config_done &= ~scan_rsp_config_flag;
			if (0 == ble->adv_config_done)
				esp_ble_gap_start_advertising(&ble->adv_params);
			break;
		case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:	// The advertising data can be freed after advertising starts
			c_free(ble->advertisingData);
			ble->advertisingData = NULL;
			if (ble->scanResponseData) {
				c_free(ble->scanResponseData);
				ble->scanResponseData = NULL;
			}
			break;
		case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
			break;
		case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
			esp_ble_gap_start_scanning(0);			// 0 == scan until explicitly stopped
			break;
		case ESP_GAP_BLE_SCAN_RESULT_EVT:
			if ((ble->state < CONNECTING) && (ESP_GAP_SEARCH_INQ_RES_EVT == param->scan_rst.search_evt)) {
				esp_ble_gap_cb_param_t *scan_result = param;
				xsmcVars(3);
				xsVar(0) = xsmcNewObject();
				xsmcSetArrayBuffer(xsVar(1), scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len);
				xsmcSetArrayBuffer(xsVar(2), scan_result->scan_rst.bda, 6);
				xsmcSet(xsVar(0), xsID_scanResponse, xsVar(1));
				xsmcSet(xsVar(0), xsID_address, xsVar(2));
				xsCall2(ble->obj, xsID_callback, xsString("onDiscovered"), xsVar(0));
			}
            break;
		default:
			break;
    }
    
	xsEndHost(gThe);
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	xsBeginHost(gThe);
	
	modBLE ble = (modBLE)xsmcGetHostData(gThis);

	switch(event) {
        case ESP_GATTS_REG_EVT:
        	ble->gatts_if = gatts_if;
            break;
    	case ESP_GATTS_CONNECT_EVT:
        	break;
    	case ESP_GATTS_DISCONNECT_EVT:
    	    break;
	}
	
	xsEndHost(gThe);
}

static void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
	xsBeginHost(gThe);
	
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;
	modBLE ble = (modBLE)xsmcGetHostData(gThis);

    switch (event) {
		case ESP_GATTC_REG_EVT:
        	ble->gattc_if = gattc_if;
        	break;
    	case ESP_GATTC_CONNECT_EVT:
    		if (ble->state == CONNECTING) {
    			ble->state = CONNECTED;
				xsmcVars(4);
				xsVar(0) = xsmcNewObject();
				xsmcSetInteger(xsVar(1), gattc_if);
				xsmcSetInteger(xsVar(2), p_data->connect.conn_id);
				xsmcSet(xsVar(0), xsID_iface, xsVar(1));
				xsmcSet(xsVar(0), xsID_connection, xsVar(2));
				xsmcSetArrayBuffer(xsVar(3), p_data->connect.remote_bda, 6);
				xsmcSet(xsVar(0), xsID_address, xsVar(3));
				xsCall2(ble->obj, xsID_callback, xsString("onConnected"), xsVar(0));
    		}
    		break;
		case ESP_GATTC_SEARCH_RES_EVT: {
			esp_gatt_srvc_id_t *srvc_id =(esp_gatt_srvc_id_t *)&p_data->search_res.srvc_id;
			uint8_t uuid[16];
			uint16_t uuid_length;
modLog("found service");
			if (1/*srvc_id->is_primary*/) {
modLog("primary service");
				if (srvc_id->id.uuid.len == ESP_UUID_LEN_16) {
					uuid_length = ESP_UUID_LEN_16;
					uuid[0] = srvc_id->id.uuid.uuid.uuid16 & 0xFF;
					uuid[1] = (srvc_id->id.uuid.uuid.uuid16 >> 8) & 0xFF;
				}
				if (srvc_id->id.uuid.len == ESP_UUID_LEN_32) {
					uuid_length = ESP_UUID_LEN_32;
					uuid[0] = srvc_id->id.uuid.uuid.uuid32 & 0xFF;
					uuid[1] = (srvc_id->id.uuid.uuid.uuid32 >> 8) & 0xFF;
					uuid[2] = (srvc_id->id.uuid.uuid.uuid32 >> 16) & 0xFF;
					uuid[3] = (srvc_id->id.uuid.uuid.uuid32 >> 24) & 0xFF;
				}
				else {
					uuid_length = ESP_UUID_LEN_128;
					c_memmove(uuid, srvc_id->id.uuid.uuid.uuid128, ESP_UUID_LEN_128);
				}
				xsmcVars(4);
				xsVar(0) = xsmcNewObject();
				xsmcSetArrayBuffer(xsVar(1), uuid, uuid_length);
				xsmcSetInteger(xsVar(2), p_data->search_res.start_handle);
				xsmcSetInteger(xsVar(3), p_data->search_res.end_handle);
				xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
				xsmcSet(xsVar(0), xsID_start, xsVar(2));
				xsmcSet(xsVar(0), xsID_end, xsVar(3));
modLog("calling callback");
				xsCall2(ble->obj, xsID_callback, xsString("_onService"), xsVar(0));
			}
			break;
		}
		case ESP_GATTC_SEARCH_CMPL_EVT:
			xsCall1(ble->obj, xsID_callback, xsString("_onService"));	// procedure complete
			break;
	}

	xsEndHost(gThe);
}


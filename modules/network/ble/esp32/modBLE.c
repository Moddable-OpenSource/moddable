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
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

#define PROFILE_A_APP_ID 0

typedef struct {
	xsMachine	*the;
	xsSlot		obj;

	uint8_t *advertisingData;
	uint8_t *scanResponseData;
	
	esp_ble_adv_params_t adv_params;
	uint8_t adv_config_done;
} modBLERecord, *modBLE;

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

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
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(PROFILE_A_APP_ID));

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
		default:
			break;
    }
    
	xsEndHost(gThe);
}

void xs_ble_set_device_name(xsMachine *the)
{
	char *deviceName = xsmcToString(xsArg(0));
	esp_ble_gap_set_device_name(deviceName);
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	xsBeginHost(gThe);
	
	modBLE ble = (modBLE)xsmcGetHostData(gThis);

	switch(event) {
        case ESP_GATTS_REG_EVT:
            break;
    	case ESP_GATTS_CONNECT_EVT:
			xsCall1(ble->obj, xsID_callback, xsString("onConnected"));
        	break;
    	case ESP_GATTS_DISCONNECT_EVT:
    	    break;
	}
	
	xsEndHost(gThe);
}



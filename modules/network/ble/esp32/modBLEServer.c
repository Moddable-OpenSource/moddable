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

#include "mc.bleservices.c"

#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

typedef struct {
	xsMachine	*the;
	xsSlot		obj;
	
	// server
	esp_gatt_if_t gatts_if;
	uint8_t *advertisingData;
	uint8_t *scanResponseData;
	esp_ble_adv_params_t adv_params;
	uint8_t adv_config_done;
	
	// services
	uint16_t handles[attribute_count];
	
	// connection
	esp_bd_addr_t remote_bda;
	uint32_t passkey;
	int16_t conn_id;
	uint16_t app_id;
	uint8_t terminating;
} modBLERecord, *modBLE;

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static void uuidToBuffer(uint8_t *buffer, esp_bt_uuid_t *uuid, uint16_t *length);

static modBLE gBLE = NULL;
static int gAPP_ID = 0;

void xs_ble_server_initialize(xsMachine *the)
{
	if (NULL != gBLE)
		xsUnknownError("BLE already initialized");
	gBLE = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!gBLE)
		xsUnknownError("no memory");
	gBLE->the = the;
	gBLE->obj = xsThis;
	gBLE->app_id = gAPP_ID++;
	gBLE->conn_id = -1;
	gBLE->gatts_if = ESP_GATT_IF_NONE;
	xsRemember(gBLE->obj);
	
	// Initialize platform Bluetooth modules
	esp_err_t err;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	err = esp_bt_controller_init(&bt_cfg);
	if (ESP_OK == err)
		err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ESP_OK == err)
		err = esp_bluedroid_init();
	if (ESP_OK == err)
		err = esp_bluedroid_enable();

	// Register callbacks
	if (ESP_OK == err)
		err = esp_ble_gatts_register_callback(gatts_event_handler);
	if (ESP_OK == err)
		err = esp_ble_gap_register_callback(gap_event_handler);
	if (ESP_OK == err)
		err = esp_ble_gatts_app_register(gBLE->app_id);
	if (ESP_OK == err)
		err = esp_ble_gatt_set_local_mtu(500);
	if (ESP_OK != err)
		xsUnknownError("ble initialization failed");
}

void xs_ble_server_close(xsMachine *the)
{
	gBLE->terminating = true;
	esp_ble_gatts_app_unregister(gBLE->gatts_if);
	xsForget(gBLE->obj);
	xs_ble_server_destructor(gBLE);
	esp_bluedroid_disable();
	esp_bluedroid_deinit();
	esp_bt_controller_deinit();
}

void xs_ble_server_destructor(void *data)
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

void xs_ble_server_set_device_name(xsMachine *the)
{
	esp_ble_gap_set_device_name(xsmcToString(xsArg(0)));
}

void xs_ble_server_set_passkey(xsMachine *the)
{
	gBLE->passkey = xsmcToInteger(xsArg(0));
}

void xs_ble_server_start_advertising(xsMachine *the)
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
	
void xs_ble_server_stop_advertising(xsMachine *the)
{
	esp_ble_gap_stop_advertising();
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

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	if (!gBLE || gBLE->terminating) return;

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
		ESP_GAP_BLE_PASSKEY_REQ_EVT:
			esp_ble_passkey_reply(gBLE->remote_bda, true, gBLE->passkey);
			break;
		default:
			break;
    }
}

void xs_ble_server_deploy(xsMachine *the)
{
	if (attribute_count > 0)
		esp_ble_gatts_create_attr_tab(gatt_db, gBLE->gatts_if, attribute_count, 0);
}

static void gattsRegisterEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	// Stack is ready
	xsBeginHost(gBLE->the);
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
	xsEndHost(gBLE->the);
}

static void gattsConnectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gatts_connect_evt_param *connect = (struct gatts_connect_evt_param *)message;
	xsBeginHost(gBLE->the);
	if (-1 != gBLE->conn_id)
		goto bail;
	gBLE->conn_id = connect->conn_id;
	c_memmove(&gBLE->remote_bda, &connect->remote_bda, sizeof(esp_bd_addr_t));
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), connect->conn_id);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(2), connect->remote_bda, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

static void gattsDisconnectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gatts_disconnect_evt_param *disconnect = (struct gatts_disconnect_evt_param *)message;
	xsBeginHost(gBLE->the);
	if (disconnect->conn_id != gBLE->conn_id)
		goto bail;
	gBLE->conn_id = -1;
	xsCall1(gBLE->obj, xsID_callback, xsString("onDisconnected"));
bail:
	xsEndHost(gBLE->the);
}

static const esp_attr_desc_t *handleToAttDesc(uint16_t handle) {
	for (uint16_t i = 0; i < attribute_count; ++i)
		if (handle == gBLE->handles[i])
			return &gatt_db[i].att_desc;
	return NULL;
}

static void gattsReadEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gatts_read_evt_param *read = (struct gatts_read_evt_param *)message;
	esp_bt_uuid_t uuid;
	uint8_t buffer[ESP_UUID_LEN_128];
	uint16_t uuid_length;
	const esp_attr_desc_t *att_desc = handleToAttDesc(read->handle);
	if (NULL == att_desc) return;
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	uuid.len = att_desc->uuid_length;
	c_memmove(uuid.uuid.uuid128, att_desc->uuid_p, att_desc->uuid_length);
	uuidToBuffer(buffer, &uuid, &uuid_length);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), buffer, uuid_length);
	xsmcSetInteger(xsVar(2), read->handle);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsResult = xsCall2(gBLE->obj, xsID_callback, xsString("onCharacteristicRead"), xsVar(0));
	esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)c_calloc(sizeof(esp_gatt_rsp_t), 1);
	if (gatt_rsp != NULL) {
        gatt_rsp->attr_value.handle = read->handle;
		gatt_rsp->attr_value.len = xsGetArrayBufferLength(xsResult);
        c_memmove(gatt_rsp->attr_value.value, xsmcToArrayBuffer(xsResult), gatt_rsp->attr_value.len);
        esp_ble_gatts_send_response(gBLE->gatts_if, read->conn_id, read->trans_id, ESP_GATT_OK, gatt_rsp);
        c_free(gatt_rsp);
	}
	xsEndHost(gBLE->the);
}

static void gattsWriteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gatts_write_evt_param *write = (struct gatts_write_evt_param *)message;
	uint8_t *value = refcon;
	
	xsBeginHost(gBLE->the);
	esp_bt_uuid_t uuid;
	uint8_t buffer[ESP_UUID_LEN_128];
	uint16_t uuid_length;
	const esp_attr_desc_t *att_desc = handleToAttDesc(write->handle);
	if (NULL == att_desc) goto bail;
	if (write->need_rsp)
		esp_ble_gatts_send_response(gBLE->gatts_if, write->conn_id, write->trans_id, ESP_GATT_OK, NULL);
	xsmcVars(4);
	c_memmove(uuid.uuid.uuid128, att_desc->uuid_p, att_desc->uuid_length);
	uuid.len = att_desc->uuid_length;
	uuidToBuffer(buffer, &uuid, &uuid_length);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), buffer, uuid_length);
	xsmcSetInteger(xsVar(2), write->handle);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsmcSetArrayBuffer(xsVar(3), value, write->len);
	xsmcSet(xsVar(0), xsID_value, xsVar(3));
	xsCall2(gBLE->obj, xsID_callback, xsString("onCharacteristicWritten"), xsVar(0));
bail:
	c_free(value);
	xsEndHost(gBLE->the);
}

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	uint8_t *value;
	
	if (!gBLE || gBLE->terminating) return;
	
	switch(event) {
		case ESP_GATTS_REG_EVT:
        	if (param->reg.status == ESP_GATT_OK) {
        		gBLE->gatts_if = gatts_if;
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->reg, sizeof(struct gatts_reg_evt_param), gattsRegisterEvent, NULL);
        	}
			break;
		case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        	if (param->add_attr_tab.status == ESP_GATT_OK) {
				c_memmove(gBLE->handles, param->add_attr_tab.handles, sizeof(gBLE->handles));
				esp_ble_gatts_start_service(param->add_attr_tab.handles[0]);
			}
			break;
		case ESP_GATTS_CONNECT_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->connect, sizeof(struct gatts_connect_evt_param), gattsConnectEvent, NULL);
			break;
		case ESP_GATTS_DISCONNECT_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->disconnect, sizeof(struct gatts_disconnect_evt_param), gattsDisconnectEvent, NULL);
			break;
		case ESP_GATTS_READ_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->read, sizeof(struct gatts_read_evt_param), gattsReadEvent, NULL);
			break;
		case ESP_GATTS_WRITE_EVT:
			value = c_malloc(param->write.len);
			if (NULL == value) {
				if (param->write.need_rsp)
					esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_ERROR, NULL);
			}
			else {
				c_memmove(value, param->write.value, param->write.len);
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->write, sizeof(struct gatts_write_evt_param), gattsWriteEvent, value);
			}
			break;
		case ESP_GATTS_START_EVT:
		case ESP_GATTS_STOP_EVT:
			break;
	}
}

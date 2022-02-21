/*
 * Copyright (c) 2016-2022 Moddable Tech, Inc.
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

#define DEVICE_FRIENDLY_NAME "Moddable"

#define LOG_GATTS 0
#if LOG_GATTS
	#define LOG_GATTS_EVENT(event) logGATTSEvent(event)
	#define LOG_GATTS_MSG(msg) modLog(msg)
	#define LOG_GATTS_INT(i) modLogInt(i)
#else
	#define LOG_GATTS_EVENT(event)
	#define LOG_GATTS_MSG(msg)
	#define LOG_GATTS_INT(i)
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

typedef struct {
	xsMachine	*the;
	xsSlot		obj;

	// server
	esp_gatt_if_t gatts_if;
	uint8_t *advertisingData;
	uint8_t *scanResponseData;
	esp_ble_adv_params_t adv_params;
	uint8_t adv_config_done;
	uint8_t deviceNameSet;

	// services
	uint8_t deployServices;
	uint16_t handles[service_count][max_attribute_count];

	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;
	uint8_t iocap;

	// connection
	esp_bd_addr_t remote_bda;
	int16_t conn_id;
	uint16_t app_id;
} modBLERecord, *modBLE;

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static void addressToBuffer(esp_bd_addr_t *bda, uint8_t *buffer);
static void bufferToAddress(uint8_t *buffer, esp_bd_addr_t *bda);
static void uuidToBuffer(uint8_t *buffer, esp_bt_uuid_t *uuid, uint16_t *length);

static const char_name_table *handleToCharName(uint16_t handle);
static const esp_gatts_attr_db_t *handleToAtt(uint16_t handle);

static void bleServerCloseEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void logGATTSEvent(esp_gatts_cb_event_t event);
static void logGAPEvent(esp_gap_ble_cb_event_t event);

static modBLE gBLE = NULL;
static int gAPP_ID = 1;

void xs_ble_server_initialize(xsMachine *the)
{
	if (NULL != gBLE)
		xsUnknownError("BLE already initialized");
	gBLE = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!gBLE)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, gBLE);
	gBLE->the = the;
	gBLE->obj = xsThis;
	gBLE->app_id = gAPP_ID++;
	gBLE->conn_id = -1;
	gBLE->gatts_if = ESP_GATT_IF_NONE;
	xsRemember(gBLE->obj);

	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_deployServices)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_deployServices);
		gBLE->deployServices = xsmcToBoolean(xsVar(0));
	}
	else
		gBLE->deployServices = true;

	// Initialize platform Bluetooth modules
	esp_err_t err = modBLEPlatformInitialize();

	// Register callbacks
	if (ESP_OK == err)
		err = esp_ble_gatts_register_callback(gatts_event_handler);
	if (ESP_OK == err)
		err = esp_ble_gap_register_callback(gap_event_handler);
	if (ESP_OK == err)
		err = esp_ble_gatts_app_register(gBLE->app_id);

	if (ESP_OK != err)
		xsUnknownError("ble initialization failed");
}

void xs_ble_server_close(xsMachine *the)
{
	modBLE ble = xsmcGetHostData(xsThis);
	if (!ble) return;

	gBLE = NULL;
	xsForget(ble->obj);
	xsmcSetHostData(xsThis, NULL);
	modMessagePostToMachine(ble->the, NULL, 0, bleServerCloseEvent, ble);
}

void xs_ble_server_destructor(void *data)
{
	modBLE ble = data;
	if (!ble) return;

	if (ble->deployServices) {
		for (uint16_t i = 0; i < service_count; ++i)
			if (ble->handles[i][0])
				esp_ble_gatts_delete_service(ble->handles[i][0]);
	}
	if (-1 != ble->conn_id)
		esp_ble_gatts_close(ble->gatts_if, ble->conn_id);
	esp_ble_gatts_app_unregister(ble->gatts_if);
	if (ble->advertisingData)
		c_free(ble->advertisingData);
	if (ble->scanResponseData)
		c_free(ble->scanResponseData);
	c_free(ble);
	gBLE = NULL;

	modBLEPlatformTerminate();
}

void xs_ble_server_disconnect(xsMachine *the)
{
	if (-1 != gBLE->conn_id)
		esp_ble_gatts_close(gBLE->gatts_if, gBLE->conn_id);
}

void xs_ble_server_get_local_address(xsMachine *the)
{
	const uint8_t *addr = (const uint8_t *)esp_bt_dev_get_address();
	uint8_t buffer[6];
	addressToBuffer((esp_bd_addr_t*)addr, buffer);
	xsmcSetArrayBuffer(xsResult, (void*)buffer, 6);
}

void xs_ble_server_set_device_name(xsMachine *the)
{
	gBLE->deviceNameSet = 1;
	esp_ble_gap_set_device_name(xsmcToString(xsArg(0)));
}

void xs_ble_server_start_advertising(xsMachine *the)
{
	AdvertisingFlags flags = xsmcToInteger(xsArg(0));
	uint16_t intervalMin = xsmcToInteger(xsArg(1));
	uint16_t intervalMax = xsmcToInteger(xsArg(2));
	uint8_t filterPolicy = xsmcToInteger(xsArg(3));
	uint8_t *advertisingData = (uint8_t*)xsmcToArrayBuffer(xsArg(4));
	uint32_t advertisingDataLength = xsmcGetArrayBufferLength(xsArg(4));
	uint8_t *scanResponseData = xsmcTest(xsArg(5)) ? (uint8_t*)xsmcToArrayBuffer(xsArg(5)) : NULL;
	uint32_t scanResponseDataLength = xsmcTest(xsArg(5)) ? xsmcGetArrayBufferLength(xsArg(5)) : 0;

	switch(filterPolicy) {
		case kBLEAdvFilterPolicyWhitelistScans:
			filterPolicy = ADV_FILTER_ALLOW_SCAN_WLST_CON_ANY;
			break;
		case kBLEAdvFilterPolicyWhitelistConnections:
			filterPolicy = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST;
			break;
		case kBLEAdvFilterPolicyWhitelistScansConnections:
			filterPolicy = ADV_FILTER_ALLOW_SCAN_WLST_CON_WLST;
			break;
		default:
			filterPolicy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
			break;
	}

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
	gBLE->adv_params.adv_type = (flags & (LE_LIMITED_DISCOVERABLE_MODE | LE_GENERAL_DISCOVERABLE_MODE)) ? ADV_TYPE_IND : ADV_TYPE_NONCONN_IND;
	gBLE->adv_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
	gBLE->adv_params.channel_map = ADV_CHNL_ALL;
	gBLE->adv_params.adv_filter_policy = filterPolicy;

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

void xs_ble_server_characteristic_notify_value(xsMachine *the)
{
	uint16_t handle = xsmcToInteger(xsArg(0));
	uint16_t notify = xsmcToInteger(xsArg(1));
	esp_ble_gatts_send_indicate(gBLE->gatts_if, gBLE->conn_id, handle, xsmcGetArrayBufferLength(xsArg(2)), xsmcToArrayBuffer(xsArg(2)), (bool)(0 == notify));
}

void xs_ble_server_set_security_parameters(xsMachine *the)
{
	uint8_t encryption = xsmcToBoolean(xsArg(0));
	uint8_t bonding = xsmcToBoolean(xsArg(1));
	uint8_t mitm = xsmcToBoolean(xsArg(2));
	uint8_t iocap = xsmcToInteger(xsArg(3));
	uint16_t err;

	gBLE->encryption = encryption;
	gBLE->bonding = bonding;
	gBLE->mitm = mitm;
	gBLE->iocap = iocap;

	err = modBLESetSecurityParameters(encryption, bonding, mitm, iocap);
	if (ESP_OK != err)
		xsUnknownError("invalid security params");
}

void xs_ble_server_passkey_input(xsMachine *the)
{
//	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint32_t passkey = xsmcToInteger(xsArg(1));
	esp_ble_passkey_reply(gBLE->remote_bda, true, passkey);
}

void xs_ble_server_passkey_reply(xsMachine *the)
{
	esp_bd_addr_t bda;
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t confirm = xsmcToBoolean(xsArg(1));
	bufferToAddress(address, &bda);
	esp_ble_confirm_reply(bda, confirm);
}

void xs_ble_server_get_service_attributes(xsMachine *the)
{
	esp_bt_uuid_t uuid;
	const esp_gatts_attr_db_t *attr;
	uint16_t serviceIndex;
	uint8_t found = false;
	uint16_t argc = xsmcArgc;
	uint16_t length = xsmcGetArrayBufferLength(xsArg(0));
	uint8_t *buffer = xsmcToArrayBuffer(xsArg(0));
	
	xsmcVars(2);
	xsResult = xsmcNewArray(0);

	for (serviceIndex = 0; !found && (serviceIndex < service_count); ++serviceIndex) {
		attr = &gatt_db[serviceIndex][0];
		if (attr->att_desc.length == length && 0 == c_memcmp(buffer, attr->att_desc.value, length)) {
			found = true;
			break;
		}
	}
	if (!found) return;

	for (uint16_t j = 0; j < attribute_counts[serviceIndex]; ++j) {
		const char_name_table *char_name;
		const uint8_t *value;
		uint16_t handle = gBLE->handles[serviceIndex][j];
		attr = &gatt_db[serviceIndex][j];

		xsVar(0) = xsmcNewObject();
		xsmcSetInteger(xsVar(1), handle);
		xsmcSet(xsVar(0), xsID_handle, xsVar(1));

		xsmcSetArrayBuffer(xsVar(1), attr->att_desc.uuid_p, attr->att_desc.uuid_length);
		xsmcSet(xsVar(0), xsID_uuid, xsVar(1));

		if (ESP_GATT_OK == esp_ble_gatts_get_attr_value(handle, &length, &value) && 0 != length) {
			xsmcSetArrayBuffer(xsVar(1), (uint8_t*)value, length);
			xsmcSet(xsVar(0), xsID_value, xsVar(1));
		}
		for (uint16_t k = 0; k < char_name_count; ++k) {
			if (serviceIndex == char_names[k].service_index && j == char_names[k].att_index) {
				xsmcSetString(xsVar(1), (char*)char_names[k].name);
				xsmcSet(xsVar(0), xsID_name, xsVar(1));
				xsmcSetString(xsVar(1), (char*)char_names[k].type);
				xsmcSet(xsVar(0), xsID_type, xsVar(1));
				break;
			}
		}
		xsCall1(xsResult, xsID_push, xsVar(0));
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

static void addressToBuffer(esp_bd_addr_t *bda, uint8_t *buffer)
{
	uint8_t *address = (uint8_t*)bda;
	buffer[0] = address[5];
	buffer[1] = address[4];
	buffer[2] = address[3];
	buffer[3] = address[2];
	buffer[4] = address[1];
	buffer[5] = address[0];
}

static void bufferToAddress(uint8_t *buffer, esp_bd_addr_t *bda)
{
	uint8_t *address = (uint8_t*)bda;
	address[0] = buffer[5];
	address[1] = buffer[4];
	address[2] = buffer[3];
	address[3] = buffer[2];
	address[4] = buffer[1];
	address[5] = buffer[0];
}

static void bleServerCloseEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modBLE ble = refcon;

	xs_ble_server_destructor(ble);
}

static void gapPasskeyNotifyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	esp_ble_sec_key_notif_t *key_notif = (esp_ble_sec_key_notif_t *)message;
	uint8_t buffer[6];
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	addressToBuffer(&key_notif->bd_addr, buffer);
	xsmcSetArrayBuffer(xsVar(1), buffer, 6);
	xsmcSetInteger(xsVar(2), key_notif->passkey);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSet(xsVar(0), xsID_passkey, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyDisplay"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gapPasskeyConfirmEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	esp_ble_sec_key_notif_t *key_notif = (esp_ble_sec_key_notif_t *)message;
	uint8_t buffer[6];
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	addressToBuffer(&key_notif->bd_addr, buffer);
	xsmcSetArrayBuffer(xsVar(1), buffer, 6);
	xsmcSetInteger(xsVar(2), key_notif->passkey);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSet(xsVar(0), xsID_passkey, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyConfirm"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gapPasskeyRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	esp_ble_sec_req_t *ble_req = (esp_ble_sec_req_t *)message;
	uint32_t passkey;
	uint8_t buffer[6];
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	addressToBuffer(&gBLE->remote_bda, buffer);
	xsmcSetArrayBuffer(xsVar(1), buffer, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	if (gBLE->iocap == KeyboardOnly)
		xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyInput"), xsVar(0));
	else {
		xsResult = xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyRequested"), xsVar(0));
		passkey = xsmcToInteger(xsResult);
		esp_ble_passkey_reply(gBLE->remote_bda, true, passkey);
	}
	xsEndHost(gBLE->the);
}

static void gapAuthCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	esp_ble_auth_cmpl_t *auth_cmpl = (esp_ble_auth_cmpl_t *)message;
	if (!gBLE) return;
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetBoolean(xsVar(1), auth_cmpl->auth_mode & ESP_LE_AUTH_BOND);
	xsmcSet(xsVar(0), xsID_bonded, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onAuthenticated"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gapRemoveBondCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_remove_bond_dev_cmpl_evt_param *remove_bond_dev_cmpl = (struct ble_remove_bond_dev_cmpl_evt_param *)message;
	if (!gBLE) return;
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), remove_bond_dev_cmpl->bd_addr, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), kBLEAddressTypePublic);	// @@
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onBondingDeleted"), xsVar(0));
	xsEndHost(gBLE->the);
}

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	LOG_GAP_EVENT(event);

	if (!gBLE) return;

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
			if (gBLE->advertisingData) {
				c_free(gBLE->advertisingData);
				gBLE->advertisingData = NULL;
			}
			if (gBLE->scanResponseData) {
				c_free(gBLE->scanResponseData);
				gBLE->scanResponseData = NULL;
			}
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
		case ESP_GAP_BLE_SEC_REQ_EVT:
			esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
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
		case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT:
			if (ESP_GATT_OK == param->remove_bond_dev_cmpl.status)
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->remove_bond_dev_cmpl, sizeof(struct ble_remove_bond_dev_cmpl_evt_param), gapRemoveBondCompleteEvent, NULL);
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

void xs_ble_server_deploy(xsMachine *the)
{
	if (!gBLE->deployServices) return;
	
	for (uint16_t i = 0; i < service_count; ++i) {
		esp_gatts_attr_db_t *gatts_attr_db = (esp_gatts_attr_db_t*)&gatt_db[i];
		esp_attr_desc_t *att_desc = (esp_attr_desc_t*)&gatts_attr_db->att_desc;
		if (ESP_UUID_LEN_16 == att_desc->uuid_length && 0x00 == att_desc->value[0] && 0x18 == att_desc->value[1])
			continue;	// ESP-IDF doesn't want apps registering gap service
		esp_ble_gatts_create_attr_tab(gatts_attr_db, gBLE->gatts_if, attribute_counts[i], i);
	}
}

static void gattsRegisterEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	// Stack is ready
	xsBeginHost(gBLE->the);
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
	xsEndHost(gBLE->the);

	// Set appearance from app GAP service when available
	// Set device name from GAP service if app hasn't already set device name in onReady() callback
	char *device_name = NULL;
	uint16_t appearance = 128;	// generic computer

	for (uint16_t i = 0; i < service_count; ++i) {
		esp_gatts_attr_db_t *gatts_attr_db = (esp_gatts_attr_db_t*)&gatt_db[i];
		esp_attr_desc_t *att_desc = (esp_attr_desc_t*)&gatts_attr_db->att_desc;
		if (ESP_UUID_LEN_16 == att_desc->uuid_length && 0x00 == att_desc->value[0] && 0x18 == att_desc->value[1]) {
			for (uint16_t j = 1; j < attribute_counts[i]; ++j) {
				att_desc = &gatts_attr_db[j].att_desc;
				if (0 == gBLE->deviceNameSet && ESP_UUID_LEN_16 == att_desc->uuid_length && 0x2A00 == *(uint16_t*)att_desc->uuid_p) {
					device_name = c_calloc(1, att_desc->length + 1);
					c_memmove(device_name, att_desc->value, att_desc->length);
				}
				if (ESP_UUID_LEN_16 == att_desc->uuid_length && 0x2A01 == *(uint16_t*)att_desc->uuid_p) {
					appearance = att_desc->value[1] << 8 | att_desc->value[0];
				}
			}
			break;
		}
	}

	esp_ble_gap_config_local_icon(appearance);
	if (NULL != device_name) {
		esp_ble_gap_set_device_name(device_name);
		c_free(device_name);
	}
	else if (0 == gBLE->deviceNameSet) {
		esp_ble_gap_set_device_name(DEVICE_FRIENDLY_NAME);
	}
}

static void gattsConnectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gatts_connect_evt_param *connect = (struct gatts_connect_evt_param *)message;
	uint8_t buffer[6];
	if (!gBLE) return;
	xsBeginHost(gBLE->the);
	if (-1 != gBLE->conn_id)
		goto bail;
	gBLE->conn_id = connect->conn_id;
	c_memmove(&gBLE->remote_bda, &connect->remote_bda, sizeof(esp_bd_addr_t));
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), connect->conn_id);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	addressToBuffer(&connect->remote_bda, buffer);
	xsmcSetArrayBuffer(xsVar(2), buffer, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

static void gattsDisconnectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct gatts_disconnect_evt_param *disconnect = (struct gatts_disconnect_evt_param *)message;
	uint8_t buffer[6];
	if (!gBLE) return;
	xsBeginHost(gBLE->the);
	if (disconnect->conn_id != gBLE->conn_id)
		goto bail;
	gBLE->conn_id = -1;
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), disconnect->conn_id);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	addressToBuffer(&disconnect->remote_bda, buffer);
	xsmcSetArrayBuffer(xsVar(2), buffer, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onDisconnected"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

const esp_gatts_attr_db_t *handleToAtt(uint16_t handle) {
	for (uint16_t i = 0; i < service_count; ++i) {
		for (uint16_t j = 0; j < attribute_counts[i]; ++j) {
			if (handle == gBLE->handles[i][j])
				return &gatt_db[i][j];
		}
	}
	return NULL;
}

const char_name_table *handleToCharName(uint16_t handle) {
	for (uint16_t i = 0; i < service_count; ++i) {
		for (uint16_t j = 0; j < attribute_counts[i]; ++j) {
			if (handle == gBLE->handles[i][j]) {
				for (uint16_t k = 0; k < char_name_count; ++k) {
					if (char_names[k].service_index == i && char_names[k].att_index == j)
						return &char_names[k];
				}
			}
		}
	}
	return NULL;
}

static void gattsReadEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	struct gatts_read_evt_param *read = (struct gatts_read_evt_param *)message;
	esp_bt_uuid_t uuid;
	uint8_t buffer[ESP_UUID_LEN_128];
	uint16_t uuid_length;
	const esp_gatts_attr_db_t *att;
	const esp_attr_desc_t *att_desc;
	const char_name_table *char_name;

	att = handleToAtt(read->handle);
	if (NULL == att) return;

#if LOG_GATTS
	att_desc = &att->att_desc;
	char_name = handleToCharName(read->handle);
	if (char_name) {
		xsTrace("reading characteristic "); xsTrace(char_name->name); xsTrace("\n");
	}
	else {
		uuid.len = att_desc->uuid_length;
		c_memmove(uuid.uuid.uuid128, att_desc->uuid_p, att_desc->uuid_length);
		uuidToBuffer(buffer, &uuid, &uuid_length);
		modLog("reading characteristic"); modLogHex(buffer[1]); modLogHex(buffer[0]);
	}
#endif

	if (ESP_GATT_AUTO_RSP == att->attr_control.auto_rsp) return;

	att_desc = &att->att_desc;
	char_name = handleToCharName(read->handle);

	xsBeginHost(gBLE->the);
	xsmcVars(5);
	uuid.len = att_desc->uuid_length;
	c_memmove(uuid.uuid.uuid128, att_desc->uuid_p, att_desc->uuid_length);
	uuidToBuffer(buffer, &uuid, &uuid_length);

	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), buffer, uuid_length);
	xsmcSetInteger(xsVar(2), read->handle);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	if (char_name) {
		xsmcSetString(xsVar(3), (char*)char_name->name);
		xsmcSet(xsVar(0), xsID_name, xsVar(3));
		xsmcSetString(xsVar(4), (char*)char_name->type);
		xsmcSet(xsVar(0), xsID_type, xsVar(4));
	}
	xsResult = xsCall2(gBLE->obj, xsID_callback, xsString("onCharacteristicRead"), xsVar(0));
	if (xsUndefinedType != xsmcTypeOf(xsResult)) {
		esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)c_calloc(sizeof(esp_gatt_rsp_t), 1);
		if (gatt_rsp != NULL) {
			gatt_rsp->attr_value.handle = read->handle;
			gatt_rsp->attr_value.len = xsmcGetArrayBufferLength(xsResult);
			c_memmove(gatt_rsp->attr_value.value, xsmcToArrayBuffer(xsResult), gatt_rsp->attr_value.len);
			esp_ble_gatts_send_response(gBLE->gatts_if, read->conn_id, read->trans_id, ESP_GATT_OK, gatt_rsp);
			c_free(gatt_rsp);
		}
	}
bail:
	xsEndHost(gBLE->the);
}

static void gattsWriteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	struct gatts_write_evt_param *write = (struct gatts_write_evt_param *)message;
	uint8_t *value = refcon;

	xsBeginHost(gBLE->the);
	esp_bt_uuid_t uuid;
	uint8_t buffer[ESP_UUID_LEN_128];
	uint16_t uuid_length;
	uint8_t notify = 0xFF;
	const esp_gatts_attr_db_t *att;
	const esp_attr_desc_t *att_desc;
	const char_name_table *char_name;
	uint8_t cccd;

	att = handleToAtt(write->handle);
	if (NULL == att) return;

	att_desc = &att->att_desc;
	char_name = handleToCharName(write->handle);

#if LOG_GATTS
	if (char_name) {
		xsTrace("writing characteristic "); xsTrace(char_name->name); xsTrace("\n");
	}
	else {
		uuid.len = att_desc->uuid_length;
		c_memmove(uuid.uuid.uuid128, att_desc->uuid_p, att_desc->uuid_length);
		uuidToBuffer(buffer, &uuid, &uuid_length);
		modLog("writing characteristic"); modLogHex(buffer[1]); modLogHex(buffer[0]);
	}
#endif
	cccd = (sizeof(uint16_t) == att_desc->uuid_length && 0x2902 == *(uint16_t*)att_desc->uuid_p && 2 == write->len);
	
	if (write->need_rsp && !cccd) {
		esp_ble_gatts_send_response(gBLE->gatts_if, write->conn_id, write->trans_id, ESP_GATT_OK, NULL);
	}
	xsmcVars(6);
	if (cccd && write->len == 2) {
		uint16_t descr_value = value[1]<<8 | value[0];
		if (descr_value < 0x0003) {
			att = handleToAtt(write->handle - 1);
			att_desc = &att->att_desc;
			char_name = handleToCharName(write->handle - 1);
			notify = (uint8_t)descr_value;
		}
		else {
			xsUnknownError("invalid cccd value");
		}
	} 
	else {
		xsUnknownError("invalid cccd value length");
	}
	c_memmove(uuid.uuid.uuid128, att_desc->uuid_p, att_desc->uuid_length);
	uuid.len = att_desc->uuid_length;
	uuidToBuffer(buffer, &uuid, &uuid_length);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), buffer, uuid_length);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	if (char_name) {
		xsmcSetString(xsVar(2), (char*)char_name->name);
		xsmcSet(xsVar(0), xsID_name, xsVar(2));
		xsmcSetString(xsVar(3), (char*)char_name->type);
		xsmcSet(xsVar(0), xsID_type, xsVar(3));
	}
	if (0xFF != notify) {
		xsmcSetInteger(xsVar(4), write->handle - 1);
		xsmcSetInteger(xsVar(5), notify);
		xsmcSet(xsVar(0), xsID_handle, xsVar(4));
		xsmcSet(xsVar(0), xsID_notify, xsVar(5));
		xsCall2(gBLE->obj, xsID_callback, xsString(0 == notify ? "onCharacteristicNotifyDisabled" : "onCharacteristicNotifyEnabled"), xsVar(0));
	}
	else {
		xsmcSetInteger(xsVar(2), write->handle);
		xsmcSet(xsVar(0), xsID_handle, xsVar(2));
		xsmcSetArrayBuffer(xsVar(3), value, write->len);
		xsmcSet(xsVar(0), xsID_value, xsVar(3));
		xsCall2(gBLE->obj, xsID_callback, xsString("onCharacteristicWritten"), xsVar(0));
	}
bail:
	c_free(value);
	xsEndHost(gBLE->the);
}

static void gattsMTUExchangedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	struct gatts_mtu_evt_param *mtu = (struct gatts_mtu_evt_param *)message;
	if (mtu->conn_id != gBLE->conn_id)
		return;

	xsBeginHost(gBLE->the);
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), mtu->mtu);
	xsCall2(gBLE->obj, xsID_callback, xsString("onMTUExchanged"), xsVar(0));
	xsEndHost(gBLE->the);
}

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	uint8_t *value;

	LOG_GATTS_EVENT(event);

	if (!gBLE) return;

	switch(event) {
		case ESP_GATTS_REG_EVT:
        	if (param->reg.status == ESP_GATT_OK) {
        		gBLE->gatts_if = gatts_if;
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->reg, sizeof(struct gatts_reg_evt_param), gattsRegisterEvent, NULL);
        	}
			break;
		case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        	if (param->add_attr_tab.status == ESP_GATT_OK) {
        		uint16_t uuid_length = param->add_attr_tab.svc_uuid.len;
        		for (uint16_t i = 0; i < service_count; ++i) {
        			if (uuid_length == gatt_db[i][0].att_desc.length && 0 == c_memcmp(param->add_attr_tab.svc_uuid.uuid.uuid128, gatt_db[i][0].att_desc.value, uuid_length)) {
						c_memmove(gBLE->handles[i], param->add_attr_tab.handles, sizeof(uint16_t) * param->add_attr_tab.num_handle);
						esp_ble_gatts_start_service(param->add_attr_tab.handles[0]);
        			}
        		}
			}
			break;
		case ESP_GATTS_CONNECT_EVT:
			if (gBLE->mitm)
    			esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
    		else if (gBLE->encryption)
    			esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT);
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
			if ((NULL == value) && param->write.len) {
#if LOG_GATTS
				LOG_GATTS_MSG("ESP_GATTS_WRITE_EVT failed value malloc");
#endif
				if (param->write.need_rsp)
					esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_ERROR, NULL);
			}
			else {
				if (param->write.len)
					c_memmove(value, param->write.value, param->write.len);
				modMessagePostToMachine(gBLE->the, (uint8_t*)&param->write, sizeof(struct gatts_write_evt_param), gattsWriteEvent, value);
			}
			break;
#if LOG_GATTS
		case ESP_GATTS_START_EVT:
			if (param->start.status != ESP_GATT_OK) {
				LOG_GATTS_MSG("ESP_GATTS_START_EVT failed, status =");
				LOG_GATTS_INT(param->start.status);
			}
			break;
    	case ESP_GATTS_CONF_EVT:
			if (param->conf.status != ESP_GATT_OK) {
				LOG_GATTS_MSG("ESP_GATTS_CONF_EVT failed, status =");
				LOG_GATTS_INT(param->conf.status);
			}
        	break;
#endif
		case ESP_GATTS_MTU_EVT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&param->mtu, sizeof(struct gatts_mtu_evt_param), gattsMTUExchangedEvent, NULL);
			break;
	}
}

static void logGATTSEvent(esp_gatts_cb_event_t event) {
	switch(event) {
		case ESP_GATTS_REG_EVT: modLog("ESP_GATTS_REG_EVT"); break;
		case ESP_GATTS_READ_EVT: modLog("ESP_GATTS_READ_EVT"); break;
		case ESP_GATTS_WRITE_EVT: modLog("ESP_GATTS_WRITE_EVT"); break;
		case ESP_GATTS_EXEC_WRITE_EVT: modLog("ESP_GATTS_EXEC_WRITE_EVT"); break;
		case ESP_GATTS_MTU_EVT: modLog("ESP_GATTS_MTU_EVT"); break;
		case ESP_GATTS_CONF_EVT: modLog("ESP_GATTS_CONF_EVT"); break;
		case ESP_GATTS_UNREG_EVT: modLog("ESP_GATTS_UNREG_EVT"); break;
		case ESP_GATTS_CREATE_EVT: modLog("ESP_GATTS_CREATE_EVT"); break;
		case ESP_GATTS_ADD_INCL_SRVC_EVT: modLog("ESP_GATTS_ADD_INCL_SRVC_EVT"); break;
		case ESP_GATTS_ADD_CHAR_EVT: modLog("ESP_GATTS_ADD_CHAR_EVT"); break;
		case ESP_GATTS_ADD_CHAR_DESCR_EVT: modLog("ESP_GATTS_ADD_CHAR_DESCR_EVT"); break;
		case ESP_GATTS_DELETE_EVT: modLog("ESP_GATTS_DELETE_EVT"); break;
		case ESP_GATTS_START_EVT: modLog("ESP_GATTS_START_EVT"); break;
		case ESP_GATTS_STOP_EVT: modLog("ESP_GATTS_STOP_EVT"); break;
		case ESP_GATTS_CONNECT_EVT: modLog("ESP_GATTS_CONNECT_EVT"); break;
		case ESP_GATTS_DISCONNECT_EVT: modLog("ESP_GATTS_DISCONNECT_EVT"); break;
		case ESP_GATTS_OPEN_EVT: modLog("ESP_GATTS_OPEN_EVT"); break;
		case ESP_GATTS_CANCEL_OPEN_EVT: modLog("ESP_GATTS_CANCEL_OPEN_EVT"); break;
		case ESP_GATTS_CLOSE_EVT: modLog("ESP_GATTS_CLOSE_EVT"); break;
		case ESP_GATTS_LISTEN_EVT: modLog("ESP_GATTS_LISTEN_EVT"); break;
		case ESP_GATTS_CONGEST_EVT: modLog("ESP_GATTS_CONGEST_EVT"); break;
		case ESP_GATTS_RESPONSE_EVT: modLog("ESP_GATTS_RESPONSE_EVT"); break;
		case ESP_GATTS_CREAT_ATTR_TAB_EVT: modLog("ESP_GATTS_CREAT_ATTR_TAB_EVT"); break;
		case ESP_GATTS_SET_ATTR_VAL_EVT: modLog("ESP_GATTS_SET_ATTR_VAL_EVT"); break;
	}
}

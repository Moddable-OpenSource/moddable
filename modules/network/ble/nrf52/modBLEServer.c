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
#include "xsHost.h"
#include "mc.xs.h"
#include "mc.defines.h"
#include "modBLE.h"

#include "sdk_errors.h"
#include "ble.h"
#include "ble_conn_params.h"
#include "ble_gap.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_freertos.h"

//#include "mc.bleservices.c"

#define APP_BLE_CONN_CFG_TAG 1
#define APP_BLE_OBSERVER_PRIO 3
#define FIRST_CONN_PARAMS_UPDATE_DELAY      5000                                    /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY       30000                                   /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT        3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

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

#define LOG_GAP 1
#if LOG_GAP
	#define LOG_GAP_EVENT(event) logGAPEvent(event)
	#define LOG_GAP_MSG(msg) modLog(msg)
	#define LOG_GAP_INT(i) modLogInt(i)
#else
	#define LOG_GAP_EVENT(event)
	#define LOG_GAP_MSG(msg)
	#define LOG_GAP_INT(i)
#endif

static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
static void bleServerCloseEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void bleServerConnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void bleServerReadyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

typedef struct {
	xsMachine *the;
	xsSlot obj;

	// connection
	int16_t conn_handle;
	ble_gap_addr_t remote_bda;
	
	// advertising
	uint8_t advertising;
	uint8_t adv_handle;
	uint8_t adv_data[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
	uint8_t scan_rsp_data[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
	
	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;
	uint8_t iocap;
	uint8_t bond;
	bool erase_bonds;
} modBLERecord, *modBLE;

static modBLE gBLE = NULL;

#if MODDEF_BLE_MAX_CONNECTIONS != 1
	#error - only one ble client connection supported
#endif

void xs_ble_server_initialize(xsMachine *the)
{
	uint32_t ram_start = 0;
    ble_conn_params_init_t cp_init;
    ret_code_t err_code;
    
	if (NULL != gBLE)
		xsUnknownError("BLE already initialized");
	gBLE = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!gBLE)
		xsUnknownError("no memory");
	gBLE->conn_handle = -1;
	gBLE->bond = 0xFF;
	gBLE->the = the;
	gBLE->obj = xsThis;
	xsRemember(gBLE->obj);
	
	// Initialize platform Bluetooth modules
    err_code = nrf_sdh_enable_request();
    if (NRF_SUCCESS == err_code) {
		// Configure the BLE stack using the default settings.
		// Fetch the start address of the application RAM.
		err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    }

    // Enable BLE stack
    if (NRF_SUCCESS == err_code)
    	err_code = nrf_sdh_ble_enable(&ram_start);

	// Initialize connection parameters (required)
    if (NRF_SUCCESS == err_code) {
		c_memset(&cp_init, 0, sizeof(cp_init));
		cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
		cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
		cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
		err_code = ble_conn_params_init(&cp_init);
    }

	if (NRF_SUCCESS != err_code)
		xsUnknownError("ble initialization failed");

    // Register a handler for BLE events.
	NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

    // Create a FreeRTOS task for the BLE stack.
	nrf_sdh_freertos_init(NULL, &gBLE->erase_bonds);
    
	modMessagePostToMachine(the, NULL, 0, bleServerReadyEvent, NULL);
}

void xs_ble_server_close(xsMachine *the)
{
	modBLE ble = xsmcGetHostData(xsThis);
	if (!ble) return;

	gBLE = NULL;
	xsForget(gBLE->obj);
	xsmcSetHostData(xsThis, NULL);
	modMessagePostToMachine(ble->the, NULL, 0, bleServerCloseEvent, ble);
	xs_ble_server_destructor(gBLE);
}

void xs_ble_server_destructor(void *data)
{
	modBLE ble = data;
	if (!ble) return;

	if (-1 != ble->conn_handle)
		sd_ble_gap_disconnect(ble->conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		
	c_free(ble);
	gBLE = NULL;
}

void xs_ble_server_disconnect(xsMachine *the)
{
	if (-1 != gBLE->conn_handle)
		sd_ble_gap_disconnect(gBLE->conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
}

void xs_ble_server_get_local_address(xsMachine *the)
{
    ret_code_t err_code;
    ble_gap_addr_t addr;
    
	err_code = sd_ble_gap_addr_get(&addr);
	if (NRF_SUCCESS == err_code) {
		xsmcSetArrayBuffer(xsResult, (void*)&addr.addr[0], 6);
	}
}

void xs_ble_server_set_device_name(xsMachine *the)
{
    ret_code_t err_code;
	ble_gap_conn_sec_mode_t sec_mode;
	const char *name = xsmcToString(xsArg(0));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode, name, c_strlen(name));
	if (NRF_SUCCESS != err_code)
		xsUnknownError("ble set device name failed");
}

void xs_ble_server_start_advertising(xsMachine *the)
{
    ret_code_t err_code;
	ble_gap_adv_data_t adv_data;
	ble_gap_adv_params_t adv_params;
	
	if (gBLE->advertising)
		xsUnknownError("ble already advertising");
		
modLog("in start advertising");
	uint16_t intervalMin = xsmcToInteger(xsArg(0));
	uint16_t intervalMax = xsmcToInteger(xsArg(1));
	uint8_t *advertisingData = (uint8_t*)xsmcToArrayBuffer(xsArg(2));
	uint32_t advertisingDataLength = xsGetArrayBufferLength(xsArg(2));
	uint8_t *scanResponseData = xsmcTest(xsArg(3)) ? (uint8_t*)xsmcToArrayBuffer(xsArg(3)) : NULL;
	uint32_t scanResponseDataLength = xsmcTest(xsArg(3)) ? xsGetArrayBufferLength(xsArg(3)) : 0;

    c_memset(&adv_data, 0, sizeof(ble_gap_adv_data_t));
	c_memmove(&gBLE->adv_data[0], advertisingData, advertisingDataLength);
	adv_data.adv_data.p_data = &gBLE->adv_data[0];
	adv_data.adv_data.len = advertisingDataLength;
	if (NULL != scanResponseData) {
		c_memmove(&gBLE->scan_rsp_data[0], scanResponseData, scanResponseDataLength);
		adv_data.scan_rsp_data.p_data = &gBLE->scan_rsp_data[0];
		adv_data.scan_rsp_data.len = scanResponseDataLength;
	}

    c_memset(&adv_params, 0, sizeof(ble_gap_adv_params_t));
    adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
    adv_params.interval        = intervalMin;

	gBLE->adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
	err_code = sd_ble_gap_adv_set_configure(&gBLE->adv_handle, &adv_data, &adv_params);
	if (NRF_SUCCESS == err_code)
		err_code = sd_ble_gap_adv_start(gBLE->adv_handle, APP_BLE_CONN_CFG_TAG);
	
	if (NRF_SUCCESS != err_code) {
		modLogInt(err_code);
		xsUnknownError("ble start advertising failed");
	}
	
	gBLE->advertising = 1;
}
	
void xs_ble_server_stop_advertising(xsMachine *the)
{
    ret_code_t err_code;
    
	if (!gBLE->advertising)
		xsUnknownError("ble not advertising");
		
	gBLE->advertising = 0;
	err_code = sd_ble_gap_adv_stop(gBLE->adv_handle);
	if (NRF_ERROR_INVALID_STATE != err_code) {
		modLogInt(err_code);
		xsUnknownError("ble stop advertising failed");
	}
}

void xs_ble_server_characteristic_notify_value(xsMachine *the)
{
	uint16_t handle = xsmcToInteger(xsArg(0));
	//uint16_t notify = xsmcToInteger(xsArg(1));
}

void xs_ble_server_deploy(xsMachine *the)
{
}

void xs_ble_server_set_security_parameters(xsMachine *the)
{
	uint8_t encryption = xsmcToBoolean(xsArg(0));
	uint8_t bonding = xsmcToBoolean(xsArg(1));
	uint8_t mitm = xsmcToBoolean(xsArg(2));
	uint8_t iocap = xsmcToInteger(xsArg(3));
	
	gBLE->encryption = encryption;
	gBLE->bonding = bonding;
	gBLE->mitm = mitm;
	gBLE->iocap = iocap;

	modBLESetSecurityParameters(encryption, bonding, mitm, iocap);
}

void xs_ble_server_passkey_input(xsMachine *the)
{
//	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint32_t passkey = xsmcToInteger(xsArg(1));
}

void xs_ble_server_passkey_reply(xsMachine *the)
{
	//uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t confirm = xsmcToBoolean(xsArg(1));
}

void xs_ble_server_get_service_attributes(xsMachine *the)
{
	// @@ TBD
}

void bleServerReadyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;
	
	xsBeginHost(the);
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
	xsEndHost(the);
}

void bleServerCloseEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modBLE ble = refcon;
	xs_ble_server_destructor(ble);
}

void bleServerConnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	int16_t conn_handle = gap_evt->conn_handle;
	xsBeginHost(gBLE->the);
	if (-1 != gBLE->conn_handle)
		goto bail;
	gBLE->conn_handle = conn_handle;
	gBLE->remote_bda = gap_evt->params.connected.peer_addr;
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), conn_handle);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(2), gBLE->remote_bda.addr, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

void bleServerDisconnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

    ret_code_t err_code;
	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	int16_t conn_handle = gap_evt->conn_handle;
	
	if (gBLE->advertising) {
		gBLE->advertising = 0;
		err_code = sd_ble_gap_adv_stop(gBLE->adv_handle);
		if (NRF_ERROR_INVALID_STATE != err_code) {
			modLogInt(err_code);
			xsUnknownError("ble stop advertising failed");
		}
	}
	xsBeginHost(gBLE->the);
	if (conn_handle != gBLE->conn_handle)
		goto bail;
	gBLE->conn_handle = -1;
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), conn_handle);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(2), gBLE->remote_bda.addr, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onDisconnected"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

static void logGAPEvent(uint16_t evt_id) {
	switch(evt_id) {
		case BLE_GAP_EVT_CONNECTED: modLog("BLE_GAP_EVT_CONNECTED"); break;
		case BLE_GAP_EVT_DISCONNECTED: modLog("BLE_GAP_EVT_DISCONNECTED"); break;
		case BLE_GAP_EVT_CONN_PARAM_UPDATE: modLog("BLE_GAP_EVT_CONN_PARAM_UPDATE"); break;
		case BLE_GAP_EVT_SEC_PARAMS_REQUEST: modLog("BLE_GAP_EVT_SEC_PARAMS_REQUEST"); break;
		case BLE_GAP_EVT_SEC_INFO_REQUEST: modLog("BLE_GAP_EVT_SEC_INFO_REQUEST"); break;
		case BLE_GAP_EVT_PASSKEY_DISPLAY: modLog("BLE_GAP_EVT_PASSKEY_DISPLAY"); break;
		case BLE_GAP_EVT_KEY_PRESSED: modLog("BLE_GAP_EVT_KEY_PRESSED"); break;
		case BLE_GAP_EVT_AUTH_KEY_REQUEST: modLog("BLE_GAP_EVT_AUTH_KEY_REQUEST"); break;
		case BLE_GAP_EVT_LESC_DHKEY_REQUEST: modLog("BLE_GAP_EVT_LESC_DHKEY_REQUEST"); break;
		case BLE_GAP_EVT_AUTH_STATUS: modLog("BLE_GAP_EVT_AUTH_STATUS"); break;
		case BLE_GAP_EVT_CONN_SEC_UPDATE: modLog("BLE_GAP_EVT_CONN_SEC_UPDATE"); break;
		case BLE_GAP_EVT_TIMEOUT: modLog("BLE_GAP_EVT_TIMEOUT"); break;
		case BLE_GAP_EVT_RSSI_CHANGED: modLog("BLE_GAP_EVT_RSSI_CHANGED"); break;
		case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST: modLog("BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST"); break;
		case BLE_GAP_EVT_SCAN_REQ_REPORT: modLog("BLE_GAP_EVT_SCAN_REQ_REPORT"); break;
		case BLE_GAP_EVT_PHY_UPDATE_REQUEST: modLog("BLE_GAP_EVT_PHY_UPDATE_REQUEST"); break;
		case BLE_GAP_EVT_PHY_UPDATE: modLog("BLE_GAP_EVT_PHY_UPDATE"); break;
		case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST: modLog("BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST"); break;
		case BLE_GAP_EVT_DATA_LENGTH_UPDATE: modLog("BLE_GAP_EVT_DATA_LENGTH_UPDATE"); break;
		case BLE_GAP_EVT_QOS_CHANNEL_SURVEY_REPORT: modLog("BLE_GAP_EVT_QOS_CHANNEL_SURVEY_REPORT"); break;
		case BLE_GAP_EVT_ADV_SET_TERMINATED: modLog("BLE_GAP_EVT_ADV_SET_TERMINATED"); break;
	}
}

void ble_evt_handler(const ble_evt_t *p_ble_evt, void * p_context)
{
    uint32_t err_code;

	if (!gBLE) return;
	
	LOG_GAP_EVENT(p_ble_evt->header.evt_id);
	
    switch (p_ble_evt->header.evt_id)
    {
		case BLE_GAP_EVT_CONNECTED:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&p_ble_evt->evt.gap_evt, sizeof(ble_gap_evt_t), bleServerConnectedEvent, NULL);
			break;
		case BLE_GAP_EVT_DISCONNECTED:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&p_ble_evt->evt.gap_evt, sizeof(ble_gap_evt_t), bleServerDisconnectedEvent, NULL);
			break;
			
		case BLE_GAP_EVT_CONN_PARAM_UPDATE:
			break;
		case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
			break;
		case BLE_GAP_EVT_SEC_INFO_REQUEST:
			break;
		case BLE_GAP_EVT_PASSKEY_DISPLAY:
			break;
		case BLE_GAP_EVT_KEY_PRESSED:
			break;
		case BLE_GAP_EVT_AUTH_KEY_REQUEST:
			break;
		case BLE_GAP_EVT_LESC_DHKEY_REQUEST:
			break;
		case BLE_GAP_EVT_AUTH_STATUS:
			break;
		case BLE_GAP_EVT_CONN_SEC_UPDATE:
			break;
		case BLE_GAP_EVT_TIMEOUT:
			break;
		case BLE_GAP_EVT_RSSI_CHANGED:
			break;
		case BLE_GAP_EVT_ADV_REPORT:
			break;
		case BLE_GAP_EVT_SEC_REQUEST:
			break;
		case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
			break;
		case BLE_GAP_EVT_SCAN_REQ_REPORT:
			break;
		case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
			break;
		case BLE_GAP_EVT_PHY_UPDATE:
			break;
		case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST:
			break;
		case BLE_GAP_EVT_DATA_LENGTH_UPDATE:
			break;
		case BLE_GAP_EVT_QOS_CHANNEL_SURVEY_REPORT:
			break;
		case BLE_GAP_EVT_ADV_SET_TERMINATED:
			break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
        	break;
        case BLE_GATTS_EVT_TIMEOUT:
        	break;
        case BLE_GATTS_EVT_WRITE:
            break;

        default:
            break;
    }
}

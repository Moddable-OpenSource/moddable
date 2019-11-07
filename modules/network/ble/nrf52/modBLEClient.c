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
#include "nrf_ble_gatt.h"
#include "nrf_ble_scan.h"
#include "nrf_sdh.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"

#define APP_BLE_CONN_CFG_TAG 1
#define APP_BLE_OBSERVER_PRIO 3
#define FIRST_CONN_PARAMS_UPDATE_DELAY      5000                                    /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY       30000                                   /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT        3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

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

#define LOG_SCAN 0
#if LOG_SCAN
	#define LOG_SCAN_EVENT(event) logScanEvent(event)
	#define LOG_SCAN_MSG(msg) modLog(msg)
	#define LOG_SCAN_INT(i) modLogInt(i)
#else
	#define LOG_SCAN_EVENT(event)
	#define LOG_SCAN_MSG(msg)
	#define LOG_SCAN_INT(i)
#endif

#define LOG_PM 0
#if LOG_PM
	#define LOG_PM_EVENT(event) logPMEvent(event)
	#define LOG_PM_MSG(msg) modLog(msg)
	#define LOG_PM_INT(i) modLogInt(i)
#else
	#define LOG_PM_EVENT(event)
	#define LOG_PM_MSG(msg)
	#define LOG_PM_INT(i)
#endif

typedef struct {
	uint16_t uuid_length;
	uint8_t  *uuid_p;
	uint16_t perm;
	uint16_t max_length;
	uint16_t length;
	uint8_t  *value;
} attr_desc_t;

typedef struct {
	attr_desc_t att_desc;
} gatts_attr_db_t;

#include "mc.bleservices.c"

typedef struct modBLEConnectionRecord modBLEConnectionRecord;
typedef modBLEConnectionRecord *modBLEConnection;

struct modBLEConnectionRecord {
	struct modBLEConnectionRecord *next;

	xsMachine	*the;
	xsSlot		objConnection;
	xsSlot		objClient;

	ble_gap_addr_t bda;
	uint16_t conn_handle;
	uint8_t mtu_exchange_pending;
	
	// char_name_table handles
	uint16_t handles[char_name_count];
};

typedef struct {
	xsMachine	*the;
	xsSlot		obj;

	// gap
	ble_gap_scan_params_t scan_params;
	
	// gatt
	nrf_ble_gatt_t m_gatt;

	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;

	modBLEConnection connections;
} modBLERecord, *modBLE;

static void modBLEConnectionAdd(modBLEConnection connection);
static void modBLEConnectionRemove(modBLEConnection connection);
static modBLEConnection modBLEConnectionFindByConnectionID(uint16_t conn_id);
static modBLEConnection modBLEConnectionFindByAddress(ble_gap_addr_t *bda);

static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
static void pm_evt_handler(pm_evt_t const * p_evt);

static void bleClientCloseEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void bleClientReadyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void gapConnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gapDisconnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gapAdvReportEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gapAuthStatusEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static modBLE gBLE = NULL;

NRF_BLE_SCAN_DEF(m_scan);

void xs_ble_client_initialize(xsMachine *the)
{
	uint32_t ram_start = 0;
    ble_conn_params_init_t cp_init;
    ret_code_t err_code;

	if (NULL != gBLE)
		xsUnknownError("BLE already initialized");
	gBLE = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!gBLE)
		xsUnknownError("no memory");
	gBLE->the = the;
	gBLE->obj = xsThis;
	xsRemember(gBLE->obj);
	
	// Initialize platform Bluetooth modules
    err_code = nrf_sdh_enable_request();
    if (NRF_SUCCESS == err_code) {
		// Configure the BLE stack using the default BLE settings defined in the sdk_config.h file.
		// Fetch the start address of the application RAM.
		err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    }

    // Enable BLE stack
    if (NRF_SUCCESS == err_code)
    	err_code = nrf_sdh_ble_enable(&ram_start);

	// Initialize GATT module
    if (NRF_SUCCESS == err_code)
		err_code = nrf_ble_gatt_init(&gBLE->m_gatt, NULL);
    
	// Initialize connection parameters (required)
    if (NRF_SUCCESS == err_code) {
		c_memset(&cp_init, 0, sizeof(cp_init));
		cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
		cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
		cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
		err_code = ble_conn_params_init(&cp_init);
    }

	// Initialize the peer manager
    if (NRF_SUCCESS == err_code) {
		err_code = pm_init();
		if (NRF_SUCCESS == err_code) {
			ble_gap_sec_params_t sec_params;
			c_memset(&sec_params, 0, sizeof(ble_gap_sec_params_t));
			sec_params.io_caps        = BLE_GAP_IO_CAPS_NONE;
			sec_params.min_key_size   = 7;
			sec_params.max_key_size   = 16;
			err_code = pm_sec_params_set(&sec_params);
		}
		if (NRF_SUCCESS == err_code)
			err_code = pm_register(pm_evt_handler);
    }

	if (NRF_SUCCESS != err_code)
		xsUnknownError("ble initialization failed");

    // Register a handler for BLE events.
	NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

    // Create a FreeRTOS task for the BLE stack.
	nrf_sdh_freertos_init(NULL, NULL);
    
	modMessagePostToMachine(the, NULL, 0, bleClientReadyEvent, NULL);
}

void xs_ble_client_close(xsMachine *the)
{
	modBLE ble = xsmcGetHostData(xsThis);
	if (!ble) return;

	gBLE = NULL;
	xsForget(ble->obj);
	xsmcSetHostData(xsThis, NULL);
	modMessagePostToMachine(ble->the, NULL, 0, bleClientCloseEvent, ble);
}

void xs_ble_client_destructor(void *data)
{
	modBLE ble = data;
	if (!ble) return;
	
	c_free(ble);
	gBLE = NULL;

	nrf_sdh_disable_request();
}

void xs_ble_client_set_local_privacy(xsMachine *the)
{
}

void xs_ble_client_start_scanning(xsMachine *the)
{
	ret_code_t err_code;
	uint8_t active = xsmcToBoolean(xsArg(0));
	uint16_t interval = xsmcToInteger(xsArg(1));
	uint16_t window = xsmcToInteger(xsArg(2));
	ble_gap_scan_params_t *scan_params = &gBLE->scan_params;
	nrf_ble_scan_init_t scan_init;

	c_memset(scan_params, 0, sizeof(ble_gap_scan_params_t));
	scan_params->active = active;
	scan_params->filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL;
	scan_params->interval = interval;
	scan_params->window = window;

    c_memset(&scan_init, 0, sizeof(scan_init));
    scan_init.p_scan_param = scan_params;

    err_code = nrf_ble_scan_init(&m_scan, &scan_init, NULL);
    if (NRF_SUCCESS == err_code)
    	err_code = nrf_ble_scan_start(&m_scan);
	if (NRF_SUCCESS != err_code)
		xsUnknownError("ble start scan failed");
}

void xs_ble_client_stop_scanning(xsMachine *the)
{
	nrf_ble_scan_stop();
}

void xs_ble_client_connect(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t addressType = xsmcToInteger(xsArg(1));
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
}

void xs_ble_client_passkey_reply(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t confirm = xsmcToBoolean(xsArg(1));
}

void xs_gap_connection_initialize(xsMachine *the)
{
	uint8_t conn_id;
	xsmcVars(1);	// xsArg(0) is client
	xsmcGet(xsVar(0), xsArg(0), xsID_connection);
	conn_id = xsmcToInteger(xsVar(0));
}
	
void xs_gap_connection_disconnect(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
}

void xs_gap_connection_read_rssi(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
}

void xs_gap_connection_exchange_mtu(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t mtu = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

}

void xs_gatt_client_discover_primary_services(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t argc = xsmcArgc;
}

void xs_gatt_service_discover_characteristics(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t start = xsmcToInteger(xsArg(1));
	uint16_t end = xsmcToInteger(xsArg(2));
}

void xs_gatt_characteristic_discover_all_characteristic_descriptors(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
}

void xs_gatt_characteristic_read_value(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
#if 0
	uint16_t auth = 0;
	uint16_t argc = xsmcArgc;
	if (argc > 2)
		auth = xsmcToInteger(xsArg(2));
#endif
}

void xs_gatt_characteristic_enable_notifications(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
}

void xs_gatt_characteristic_disable_notifications(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
}

void xs_gatt_descriptor_read_value(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
#if 0
	uint16_t auth = 0;
	uint16_t argc = xsmcArgc;
	if (argc > 2)
		auth = xsmcToInteger(xsArg(2));
#endif
}

void xs_gatt_descriptor_write_value(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
}

void xs_gatt_characteristic_write_without_response(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
}

modBLEConnection modBLEConnectionFindByConnectionID(uint16_t conn_handle)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (conn_handle == walker->conn_handle)
			break;
	return walker;
}

modBLEConnection modBLEConnectionFindByAddress(ble_gap_addr_t *bda)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (0 == c_memcmp(bda, (uint8_t*)&walker->bda, sizeof(ble_gap_addr_t)))
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

void bleClientReadyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;
	
	xsBeginHost(the);
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
	xsEndHost(the);
}

void bleClientCloseEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modBLE ble = refcon;
	xs_ble_client_destructor(ble);
}

void gapConnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
#if 0
	if (!gBLE) return;

	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	int16_t conn_handle = gap_evt->conn_handle;
	xsBeginHost(gBLE->the);
	if (BLE_CONN_HANDLE_INVALID != gBLE->conn_handle)
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
#endif
}

void gapDisconnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
#if 0
	if (!gBLE) return;

    ret_code_t err_code;
	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	int16_t conn_handle = gap_evt->conn_handle;
	
	if (conn_handle != gBLE->conn_handle) return;
	xsBeginHost(gBLE->the);
	gBLE->conn_handle = BLE_CONN_HANDLE_INVALID;
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), conn_handle);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(2), gBLE->remote_bda.addr, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onDisconnected"), xsVar(0));
	xsEndHost(gBLE->the);
#endif
}

static void gapAdvReportEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	ble_gap_evt_adv_report_t const * p_evt_adv_report = (ble_gap_evt_adv_report_t const *)message;
	uint8_t *data = (uint8_t*)refcon;

	if (!gBLE) goto bail;
	
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), data, p_evt_adv_report->data.len);
	xsmcSet(xsVar(0), xsID_scanResponse, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), (void*)&p_evt_adv_report->peer_addr.addr[0], 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), p_evt_adv_report->peer_addr.addr_type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onDiscovered"), xsVar(0));
	xsEndHost(gBLE->the);

bail:
	c_free(data);
}

static void gapAuthStatusEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	ble_gap_evt_auth_status_t const * p_evt_auth_status = (ble_gap_evt_auth_status_t const *)message;
	if (BLE_GAP_SEC_STATUS_SUCCESS == p_evt_auth_status->auth_status) {
		xsBeginHost(gBLE->the);
		xsCall1(gBLE->obj, xsID_callback, xsString("onAuthenticated"));
		xsEndHost(gBLE->the);
	}
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
		case BLE_GAP_EVT_ADV_REPORT: modLog("BLE_GAP_EVT_ADV_REPORT"); break;
		case BLE_GAP_EVT_ADV_SET_TERMINATED: modLog("BLE_GAP_EVT_ADV_SET_TERMINATED"); break;

        case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP: modLog("BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP"); break;
        case BLE_GATTC_EVT_REL_DISC_RSP: modLog("BLE_GATTC_EVT_REL_DISC_RSP"); break;
        case BLE_GATTC_EVT_CHAR_DISC_RSP: modLog("BLE_GATTC_EVT_CHAR_DISC_RSP"); break;
        case BLE_GATTC_EVT_DESC_DISC_RSP: modLog("BLE_GATTC_EVT_DESC_DISC_RSP"); break;
        case BLE_GATTC_EVT_ATTR_INFO_DISC_RSP: modLog("BLE_GATTC_EVT_ATTR_INFO_DISC_RSP"); break;
        case BLE_GATTC_EVT_CHAR_VAL_BY_UUID_READ_RSP: modLog("BLE_GATTC_EVT_CHAR_VAL_BY_UUID_READ_RSP"); break;
        case BLE_GATTC_EVT_READ_RSP: modLog("BLE_GATTC_EVT_READ_RSP"); break;
        case BLE_GATTC_EVT_CHAR_VALS_READ_RSP: modLog("BLE_GATTC_EVT_CHAR_VALS_READ_RSP"); break;
        case BLE_GATTC_EVT_WRITE_RSP: modLog("BLE_GATTC_EVT_WRITE_RSP"); break;
        case BLE_GATTC_EVT_HVX: modLog("BLE_GATTC_EVT_HVX"); break;
        case BLE_GATTC_EVT_EXCHANGE_MTU_RSP: modLog("BLE_GATTC_EVT_EXCHANGE_MTU_RSP"); break;
        case BLE_GATTC_EVT_TIMEOUT: modLog("BLE_GATTC_EVT_TIMEOUT"); break;
        case BLE_GATTC_EVT_WRITE_CMD_TX_COMPLETE: modLog("BLE_GATTC_EVT_WRITE_CMD_TX_COMPLETE"); break;
	}
}

void ble_evt_handler(const ble_evt_t *p_ble_evt, void * p_context)
{
    uint32_t err_code;

	if (!gBLE) return;
	
	LOG_GAP_EVENT(p_ble_evt->header.evt_id);
	
    switch (p_ble_evt->header.evt_id)
    {
    	case BLE_GAP_EVT_ADV_REPORT: {
			ble_gap_evt_adv_report_t const * p_evt_adv_report = &p_ble_evt->evt.gap_evt.params.adv_report;
			if (0 != p_evt_adv_report->data.len) {
				uint8_t *data = c_malloc(p_evt_adv_report->data.len);
				if (NULL != data) {
					c_memmove(data, p_evt_adv_report->data.p_data, p_evt_adv_report->data.len);
					modMessagePostToMachine(gBLE->the, (uint8_t*)p_evt_adv_report, sizeof(ble_gap_evt_adv_report_t), gapAdvReportEvent, data);
				}
			}
   			break;
    	}
		case BLE_GAP_EVT_CONNECTED:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&p_ble_evt->evt.gap_evt, sizeof(ble_gap_evt_t), gapConnectedEvent, NULL);
			break;
		case BLE_GAP_EVT_DISCONNECTED:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&p_ble_evt->evt.gap_evt, sizeof(ble_gap_evt_t), gapDisconnectedEvent, NULL);
			break;
		case BLE_GAP_EVT_AUTH_STATUS: {
			ble_gap_evt_auth_status_t const * p_evt_auth_status = &p_ble_evt->evt.gap_evt.params.auth_status;
			modMessagePostToMachine(gBLE->the, (uint8_t*)p_evt_auth_status, sizeof(ble_gap_evt_auth_status_t), gapAuthStatusEvent, NULL);
			break;
		}
        case BLE_GAP_EVT_SEC_INFO_REQUEST:
            err_code = sd_ble_gap_sec_info_reply(p_ble_evt->evt.gap_evt.conn_handle, NULL, NULL, NULL);
#if LOG_GAP
			if (NRF_SUCCESS != err_code) {
				LOG_GAP_MSG("BLE_GAP_EVT_SEC_INFO_REQUEST failed, err_code =");
				LOG_GAP_INT(err_code);
			}
#endif
        	break;
			
        default:
            break;
    }
}

static void logScanEvent(uint16_t evt_id) {
	switch(evt_id) {
		case NRF_BLE_SCAN_EVT_FILTER_MATCH: modLog("NRF_BLE_SCAN_EVT_FILTER_MATCH"); break;
		case NRF_BLE_SCAN_EVT_WHITELIST_REQUEST: modLog("NRF_BLE_SCAN_EVT_WHITELIST_REQUEST"); break;
		case NRF_BLE_SCAN_EVT_WHITELIST_ADV_REPORT: modLog("NRF_BLE_SCAN_EVT_WHITELIST_ADV_REPORT"); break;
		case NRF_BLE_SCAN_EVT_NOT_FOUND: modLog("NRF_BLE_SCAN_EVT_NOT_FOUND"); break;
		case NRF_BLE_SCAN_EVT_SCAN_TIMEOUT: modLog("NRF_BLE_SCAN_EVT_SCAN_TIMEOUT"); break;
		case NRF_BLE_SCAN_EVT_SCAN_REQ_REPORT: modLog("NRF_BLE_SCAN_EVT_SCAN_REQ_REPORT"); break;
		case NRF_BLE_SCAN_EVT_CONNECTING_ERROR: modLog("NRF_BLE_SCAN_EVT_CONNECTING_ERROR"); break;
		case NRF_BLE_SCAN_EVT_CONNECTED: modLog("NRF_BLE_SCAN_EVT_CONNECTED"); break;
	}
}

static void logPMEvent(uint16_t evt_id) {
	switch(evt_id) {
		case PM_EVT_BONDED_PEER_CONNECTED: modLog("PM_EVT_BONDED_PEER_CONNECTED"); break;
		case PM_EVT_CONN_SEC_START: modLog("PM_EVT_CONN_SEC_START"); break;
		case PM_EVT_CONN_SEC_SUCCEEDED: modLog("PM_EVT_CONN_SEC_SUCCEEDED"); break;
		case PM_EVT_CONN_SEC_FAILED: modLog("PM_EVT_CONN_SEC_FAILED"); break;
		case PM_EVT_CONN_SEC_CONFIG_REQ: modLog("PM_EVT_CONN_SEC_CONFIG_REQ"); break;
		case PM_EVT_CONN_SEC_PARAMS_REQ: modLog("PM_EVT_CONN_SEC_PARAMS_REQ"); break;
		case PM_EVT_STORAGE_FULL: modLog("PM_EVT_STORAGE_FULL"); break;
		case PM_EVT_ERROR_UNEXPECTED: modLog("PM_EVT_ERROR_UNEXPECTED"); break;
		case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED: modLog("PM_EVT_PEER_DATA_UPDATE_SUCCEEDED"); break;
		case PM_EVT_PEER_DATA_UPDATE_FAILED: modLog("PM_EVT_PEER_DATA_UPDATE_FAILED"); break;
		case PM_EVT_PEER_DELETE_SUCCEEDED: modLog("PM_EVT_PEER_DELETE_SUCCEEDED"); break;
		case PM_EVT_PEERS_DELETE_FAILED: modLog("PM_EVT_PEERS_DELETE_FAILED"); break;
		case PM_EVT_LOCAL_DB_CACHE_APPLIED: modLog("PM_EVT_LOCAL_DB_CACHE_APPLIED"); break;
		case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED: modLog("PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED"); break;
		case PM_EVT_SERVICE_CHANGED_IND_SENT: modLog("PM_EVT_SERVICE_CHANGED_IND_SENT"); break;
		case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED: modLog("PM_EVT_SERVICE_CHANGED_IND_CONFIRMED"); break;
		case PM_EVT_SLAVE_SECURITY_REQ: modLog("PM_EVT_SLAVE_SECURITY_REQ"); break;
		case PM_EVT_FLASH_GARBAGE_COLLECTED: modLog("PM_EVT_FLASH_GARBAGE_COLLECTED"); break;
		case PM_EVT_FLASH_GARBAGE_COLLECTION_FAILED: modLog("PM_EVT_FLASH_GARBAGE_COLLECTION_FAILED"); break;
	}
}

void pm_evt_handler(pm_evt_t const * p_evt)
{
	LOG_PM_EVENT(p_evt->evt_id);
	
    pm_handler_on_pm_evt(p_evt);
//  pm_handler_disconnect_on_sec_failure(p_evt);
	pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id) {
    	case PM_EVT_CONN_SEC_FAILED:
    		break;
        default:
            break;
    }
}

/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
#include "modBLECommon.h"

#include "sdk_errors.h"
#include "sdk_config.h"
#include "ble.h"
#include "ble_conn_params.h"
#include "ble_gap.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_freertos.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_scan.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"

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

#include "mc.bleservices.c"

typedef enum {
	NONE = 0,
	CHARACTERISTIC_WRITE_VALUE = 1,
	CHARACTERISTIC_WRITE_WITHOUT_RESPONSE,
	CHARACTERISTIC_READ_VALUE,
	DESCRIPTOR_WRITE_VALUE,
	DESCRIPTOR_READ_VALUE
} gattProcedureID;

typedef struct modBLEClientConnectionRecord modBLEClientConnectionRecord;
typedef modBLEClientConnectionRecord *modBLEClientConnection;

typedef struct {
	xsSlot obj;
	uint16_t conn_handle;
	gattProcedureID id;
	ble_uuid_t searchUUID;
	ble_gattc_handle_range_t handle_range;
	uint16_t discovery_handle;
} gattProcedureRecord;

struct modBLEClientConnectionRecord {
	modBLEConnectionPart;
	
	xsSlot objClient;
	uint8_t authenticated;
	
	// gatt procedure
	gattProcedureRecord gattProcedure;
	
	// char_name_table handles
	uint16_t handles[char_name_count];
};

typedef struct modBLEScannedPacketRecord modBLEScannedPacketRecord;
typedef modBLEScannedPacketRecord *modBLEScannedPacket;

struct modBLEScannedPacketRecord {
	struct modBLEScannedPacketRecord *next;

	uint16_t type;
	uint8_t addr[6];
};

typedef struct {
	xsMachine	*the;
	xsSlot		obj;

	// gap
	ble_gap_scan_params_t scan_params;
	nrf_ble_scan_init_t scan_init;
	
	// gatt
	nrf_ble_gatt_t m_gatt;

	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;
	uint8_t iocap;

	// scanning
	uint8_t duplicates;
	modBLEScannedPacket scanned;

	modBLEMessageQueueRecord discoveryQueue;
	modBLEMessageQueueRecord notificationQueue;
} modBLERecord, *modBLE;

typedef struct {
	modBLEMessageQueueEntryPart;
	ble_gap_evt_adv_report_t adv_report;
	uint8_t data[1];
} deviceDiscoveryRecord;

typedef struct {
	modBLEMessageQueueEntryPart;
	uint8_t completed;
	uint16_t count;
	ble_gattc_service_t services[1];
} serviceSearchRecord;

typedef struct {
	modBLEMessageQueueEntryPart;
	uint8_t completed;
	uint16_t count;
	ble_gattc_char_t chars[1];
} characteristicSearchRecord;

typedef struct {
	modBLEMessageQueueEntryPart;
	uint8_t completed;
	uint16_t count;
	ble_gattc_desc_t descs[1];
} descriptorSearchRecord;

typedef struct {
	modBLEMessageQueueEntryPart;
	ble_gattc_evt_hvx_t hvx;
} attributeNotificationRecord;

static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
static void pm_evt_handler(pm_evt_t const * p_evt);
static void scan_evt_handler(scan_evt_t const * p_scan_evt);

static void bleClientCloseEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void bleClientReadyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void gapAdvReportEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gapAuthKeyRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gapAuthStatusEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gapConnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gapDisconnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gapRSSIChangedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void gattcCharacteristicDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gattcCharacteristicNotificationEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gattcCharacteristicReadEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gattcCharacteristicWriteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gattcCharacteristicsForDescriptorsDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gattcDescriptorDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gattcMTUExchangedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gattcServiceDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void pmConnSecSucceededEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void clearScanned(modBLE ble);

static modBLE gBLE = NULL;

NRF_BLE_SCAN_DEF(m_scan);	// device scanning module

void xs_ble_client_initialize(xsMachine *the)
{
	modBLEPlatformInitializeDataRecord init;
	ble_conn_params_init_t *p_cp_init;
    ret_code_t err_code;

	if (NULL != gBLE)
		xsUnknownError("BLE already initialized");
	gBLE = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!gBLE)
		xsUnknownError("no memory");
	gBLE->the = the;
	gBLE->obj = xsThis;
	gBLE->iocap = 0xFF;
	gBLE->duplicates = true;
	xsmcSetHostData(xsThis, gBLE);
	xsRemember(gBLE->obj);
	
	modBLEMessageQueueConfigure(&gBLE->notificationQueue, the, gattcCharacteristicNotificationEvent, NULL);

	// Initialize platform Bluetooth modules
	init.p_gatt = &gBLE->m_gatt;
	init.pm_event_handler = pm_evt_handler;
	init.vs_uuid_count = vendor_specific_uuid_count;
	init.p_vs_uuids = vendor_uuids;
	p_cp_init = &init.cp_init;
	c_memset(p_cp_init, 0, sizeof(ble_conn_params_init_t));
	p_cp_init->first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
	p_cp_init->next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
	p_cp_init->max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
	
	err_code = modBLEPlatformInitialize(&init);
	
	// Initialize default scan params - required when connecting without first scanning
	if (NRF_SUCCESS == err_code) {
		ble_gap_scan_params_t *scan_params = &gBLE->scan_params;
		nrf_ble_scan_init_t *scan_init = &gBLE->scan_init;

		scan_init->p_scan_param = scan_params;
		scan_params->active = 1;
		scan_params->filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL;
		scan_params->interval = 0x50;
		scan_params->window = 0x30;

		err_code = nrf_ble_scan_init(&m_scan, scan_init, NULL);
	}
	
	if (NRF_SUCCESS != err_code)
		xsUnknownError("ble initialization failed");

    // Register a handler for GAP and GATTC events
	NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

	// Notify app that stack is ready
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
	
	modBLEConnection connection = modBLEConnectionGetFirst();
	while (connection != NULL) {
		modBLEConnection next = modBLEConnectionGetNext(connection);
		if (kBLEConnectionTypeClient == connection->type) {
			xsMachine *the = connection->the;
			xsForget(connection->objConnection);
			modBLEConnectionRemove(connection);
		}
		connection = next;
	}
	
	modBLEMessageQueueEmpty(&ble->discoveryQueue);
	modBLEMessageQueueEmpty(&ble->notificationQueue);
	clearScanned(ble);

	c_free(ble);
	gBLE = NULL;

	modBLEPlatformTerminate();
}

void xs_ble_client_set_local_privacy(xsMachine *the)
{
}

void xs_ble_client_start_scanning(xsMachine *the)
{
	ret_code_t err_code;
	uint8_t active = xsmcToBoolean(xsArg(0));
	uint8_t duplicates = xsmcToBoolean(xsArg(1));
	uint32_t interval = xsmcToInteger(xsArg(2));
	uint32_t window = xsmcToInteger(xsArg(3));
	uint16_t filterPolicy = xsmcToInteger(xsArg(4));
	ble_gap_scan_params_t *scan_params = &gBLE->scan_params;
	nrf_ble_scan_init_t *scan_init = &gBLE->scan_init;

	// Terminate any pending connection attempts. Otherwise scanning doesn't seem to return any results.
	// TBD: Add pending connection cancel API?
	modBLEConnection connection;
	while (NULL != (connection = modBLEConnectionFindByConnectionID(kInvalidConnectionID)))
		modBLEConnectionRemove(connection);
	sd_ble_gap_connect_cancel();

	switch(filterPolicy) {
		case kBLEScanFilterPolicyWhitelist:
			filterPolicy = BLE_GAP_SCAN_FP_WHITELIST;
			break;
		case kBLEScanFilterNotResolvedDirected:
			filterPolicy = BLE_GAP_SCAN_FP_ALL_NOT_RESOLVED_DIRECTED;
			break;
		case kBLEScanFilterWhitelistNotResolvedDirected:
			filterPolicy = BLE_GAP_SCAN_FP_WHITELIST_NOT_RESOLVED_DIRECTED;
			break;
		default:
			filterPolicy = BLE_GAP_SCAN_FP_ACCEPT_ALL;
			break;
	}

	gBLE->duplicates = duplicates;

	c_memset(scan_params, 0, sizeof(ble_gap_scan_params_t));
	scan_params->active = active;
	scan_params->filter_policy = filterPolicy;
	scan_params->interval = interval;
	scan_params->window = window;

    c_memset(scan_init, 0, sizeof(scan_init));
    scan_init->p_scan_param = scan_params;

	modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, gapAdvReportEvent, NULL);

    err_code = nrf_ble_scan_init(&m_scan, scan_init, scan_evt_handler);
    if (NRF_SUCCESS == err_code)
    	err_code = nrf_ble_scan_start(&m_scan);
	if (NRF_SUCCESS != err_code)
		xsUnknownError("ble start scan failed");
}

void xs_ble_client_stop_scanning(xsMachine *the)
{
	nrf_ble_scan_stop();
	clearScanned(gBLE);
}

void xs_ble_client_connect(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t addressType = xsmcToInteger(xsArg(1));
	
	ble_gap_addr_t addr;
	addr.addr_id_peer = 0;
	addr.addr_type = addressType;
	c_memmove(&addr.addr, address, BLE_GAP_ADDR_LEN);
		
	// Ignore duplicate connection attempts
	if (modBLEConnectionFindByAddressAndType(address, addressType)) {
		LOG_GATTC_MSG("xs_ble_client_connect: Ignoring duplicate connect attempt");
		return;
	}
	
	// Add a new connection record to be filled as the connection completes
	modBLEConnection connection = c_calloc(sizeof(modBLEClientConnectionRecord), 1);
	if (!connection)
		xsUnknownError("out of memory");
	connection->id = kInvalidConnectionID;
	connection->type = kBLEConnectionTypeClient;
	connection->addressType = addressType;
	c_memmove(connection->address, address, 6);
	modBLEConnectionAdd((modBLEConnection)connection);
		
	// The third argument is an existing connection handle, when available.
	// This can happen when the server establishes a connection that gets passed to the client.
	// When there is an existing connection handle, post the gapConnectedEvent so this client can populate the connection record and notify the application.
	if (argc > 2) {
		ble_gap_evt_t gap_evt = {0};
		gap_evt.conn_handle = xsmcToInteger(xsArg(2));
		gap_evt.params.connected.peer_addr = addr;
		modMessagePostToMachine(the, (uint8_t*)&gap_evt, sizeof(ble_gap_evt_t), gapConnectedEvent, NULL);
	}
	else {
		sd_ble_gap_connect(&addr, &m_scan.scan_params, &m_scan.conn_params, APP_BLE_CONN_CFG_TAG);
	}
}

void xs_ble_client_set_security_parameters(xsMachine *the)
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

#if (NRF_BLE_LESC_ENABLED == 1)
	if (mitm && (iocap == DisplayYesNo))
		modLog("# warning: LE secure connections require a Nordic SDK patch. Refer to the Moddable nRF52 README.md for details.");
#endif

	err = modBLESetSecurityParameters(encryption, bonding, mitm, iocap);
	if (NRF_SUCCESS != err)
		xsUnknownError("invalid security params");
		
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

void xs_ble_client_passkey_reply(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint32_t digits = xsmcToInteger(xsArg(1));
	char passkey[7];
	
	modBLEConnection connection = modBLEConnectionFindByAddress(address);
	if (!connection)
		xsUnknownError("connection not found");

	itoa(digits, passkey, 10);
	sd_ble_gap_auth_key_reply(connection->id, BLE_GAP_AUTH_KEY_TYPE_PASSKEY, passkey);
}

void xs_gap_connection_initialize(xsMachine *the)
{
	uint8_t conn_id;
	xsmcVars(1);	// xsArg(0) is client
	xsmcGet(xsVar(0), xsArg(0), xsID_connection);
	conn_id = xsmcToInteger(xsVar(0));
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	connection->the = the;
	connection->objConnection = xsThis;
	connection->objClient = xsArg(0);
	xsRemember(connection->objConnection);
}
	
void xs_gap_connection_disconnect(xsMachine *the)
{
	uint8_t conn_handle = xsmcToInteger(xsArg(0));
	sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
}

void xs_gap_connection_read_rssi(xsMachine *the)
{
	uint8_t conn_handle = xsmcToInteger(xsArg(0));
	sd_ble_gap_rssi_start(conn_handle, 0, 0);
}

void xs_gap_connection_exchange_mtu(xsMachine *the)
{
	uint16_t conn_handle = xsmcToInteger(xsArg(0));
	uint16_t mtu = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;
	
	connection->mtu_exchange_pending = 1;
	sd_ble_gattc_exchange_mtu_request(conn_handle, mtu);
}

void xs_gatt_client_discover_primary_services(xsMachine *the)
{
	uint8_t conn_handle = xsmcToInteger(xsArg(0));
	uint16_t argc = xsmcArgc;

	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;
	c_memset(&connection->gattProcedure, 0, sizeof(gattProcedureRecord));
	if (argc > 1)
		bufferToUUID(&connection->gattProcedure.searchUUID, (uint8_t*)xsmcToArrayBuffer(xsArg(1)), xsmcGetArrayBufferLength(xsArg(1)));
	else
		connection->gattProcedure.searchUUID.uuid = 0;
	connection->gattProcedure.obj = xsThis;
	connection->gattProcedure.conn_handle = conn_handle;
	
	modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, gattcServiceDiscoveryEvent, NULL);

	sd_ble_gattc_primary_services_discover(conn_handle, 1, (argc > 1 ? &connection->gattProcedure.searchUUID : NULL));
}

void xs_gatt_service_discover_characteristics(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint8_t conn_handle = xsmcToInteger(xsArg(0));

	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;
	
	c_memset(&connection->gattProcedure, 0, sizeof(gattProcedureRecord));
	connection->gattProcedure.handle_range.start_handle = xsmcToInteger(xsArg(1));
	connection->gattProcedure.handle_range.end_handle = xsmcToInteger(xsArg(2));
	connection->gattProcedure.obj = xsThis;
	connection->gattProcedure.conn_handle = conn_handle;
	if (argc > 3) {
		bufferToUUID(&connection->gattProcedure.searchUUID, (uint8_t*)xsmcToArrayBuffer(xsArg(3)), xsmcGetArrayBufferLength(xsArg(3)));
	}
	else
		connection->gattProcedure.searchUUID.uuid = 0;
		
	modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, gattcCharacteristicDiscoveryEvent, NULL);

	sd_ble_gattc_characteristics_discover(conn_handle, &connection->gattProcedure.handle_range);
}

void xs_gatt_characteristic_discover_all_characteristic_descriptors(xsMachine *the)
{
	uint8_t conn_handle = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
	ble_uuid_t uuid;
	uint16_t start, end;

	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;
	
	// Discover all service characteristics to find the range of characteristic handles for searching descriptors
	c_memset(&connection->gattProcedure, 0, sizeof(gattProcedureRecord));
	connection->gattProcedure.searchUUID.uuid = 0;
	connection->gattProcedure.obj = xsThis;
	connection->gattProcedure.discovery_handle = characteristic;
	connection->gattProcedure.conn_handle = conn_handle;

	xsmcVars(2);
	xsmcGet(xsVar(0), xsThis, xsID_service);
	xsmcGet(xsVar(1), xsVar(0), xsID_start);
	start = xsmcToInteger(xsVar(1));
	xsmcGet(xsVar(1), xsVar(0), xsID_end);
	end = xsmcToInteger(xsVar(1));
	connection->gattProcedure.handle_range.start_handle = start;
	connection->gattProcedure.handle_range.end_handle = end;
	
	modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, gattcCharacteristicsForDescriptorsDiscoveryEvent, NULL);

	sd_ble_gattc_characteristics_discover(conn_handle, &connection->gattProcedure.handle_range);
}

void xs_gatt_characteristic_read_value(xsMachine *the)
{
	uint8_t conn_handle = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	ret_code_t err_code;
#if 0
	uint16_t auth = 0;
	uint16_t argc = xsmcArgc;
	if (argc > 2)
		auth = xsmcToInteger(xsArg(2));
#endif
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;
	
	c_memset(&connection->gattProcedure, 0, sizeof(gattProcedureRecord));
	connection->gattProcedure.id = CHARACTERISTIC_READ_VALUE;
	
	sd_ble_gattc_read(connection->id, handle, 0);
}

static void writeAttribute(uint16_t conn_handle, uint16_t attribute, uint8_t write_op, uint8_t *data, uint16_t len)
{
	ble_gattc_write_params_t write_params;
	write_params.write_op = write_op;
	write_params.handle = attribute;
	write_params.flags = 0;
	write_params.offset = 0;
	write_params.len = len;
	write_params.p_value = data;

	sd_ble_gattc_write(conn_handle, &write_params);
}

void xs_gatt_characteristic_enable_notifications(xsMachine *the)
{
	uint8_t conn_handle = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
    uint8_t data[2] = {0x01, 0x00};

	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;

	writeAttribute(conn_handle, characteristic + 1, BLE_GATT_OP_WRITE_CMD, data, 2);
}

void xs_gatt_characteristic_disable_notifications(xsMachine *the)
{
	uint8_t conn_handle = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
    uint8_t data[2] = {0x00, 0x00};

	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;

	writeAttribute(conn_handle, characteristic + 1, BLE_GATT_OP_WRITE_CMD, data, 2);
}

void xs_gatt_descriptor_write_value(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint8_t conn_handle = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	uint8_t needResponse = 0;
	uint8_t *data;
	uint16_t len;

	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;

	if (argc > 3)
		needResponse = xsmcToInteger(xsArg(3));

	switch (xsmcTypeOf(xsArg(2))) {
		case xsStringType:
			data = xsmcToString(xsArg(2));
			len = c_strlen(data);
			break;
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype)) {
				data = xsmcToArrayBuffer(xsArg(2));
				len = xsmcGetArrayBufferLength(xsArg(2));
			}
			else
				goto unknown;
			break;
		unknown:
		default:
			xsUnknownError("unsupported type");
			break;
	}

	c_memset(&connection->gattProcedure, 0, sizeof(gattProcedureRecord));
	connection->gattProcedure.id = DESCRIPTOR_WRITE_VALUE;
	
	writeAttribute(conn_handle, handle, needResponse ? BLE_GATT_OP_WRITE_REQ : BLE_GATT_OP_WRITE_CMD, data, len);
}

void xs_gatt_characteristic_write_without_response(xsMachine *the)
{
	uint8_t conn_handle = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	uint8_t *data;
	uint16_t len;

	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;

	switch (xsmcTypeOf(xsArg(2))) {
		case xsStringType:
			data = xsmcToString(xsArg(2));
			len = c_strlen(data);
			break;
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype)) {
				data = xsmcToArrayBuffer(xsArg(2));
				len = xsmcGetArrayBufferLength(xsArg(2));
			}
			else
				goto unknown;
			break;
		unknown:
		default:
			xsUnknownError("unsupported type");
			break;
	}

	c_memset(&connection->gattProcedure, 0, sizeof(gattProcedureRecord));
	connection->gattProcedure.id = CHARACTERISTIC_WRITE_WITHOUT_RESPONSE;

	writeAttribute(conn_handle, handle, BLE_GATT_OP_WRITE_CMD, data, len);
}

void xs_gatt_descriptor_read_value(xsMachine *the)
{
	uint16_t conn_handle = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
#if 0
	uint16_t auth = 0;
	uint16_t argc = xsmcArgc;
	if (argc > 2)
		auth = xsmcToInteger(xsArg(2));
#endif
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;
	
	c_memset(&connection->gattProcedure, 0, sizeof(gattProcedureRecord));
	connection->gattProcedure.id = DESCRIPTOR_READ_VALUE;

	sd_ble_gattc_read(conn_handle, handle, 0);
}

int modBLEConnectionSaveAttHandle(modBLEClientConnection connection, ble_uuid_t *uuid, uint16_t handle)
{
	int result = -1;
	for (int service_index = 0; service_index < service_count; ++service_index) {
		for (int att_index = 0; att_index < attribute_counts[service_index]; ++att_index) {
			const gatts_attr_db_t *att_db = &gatt_db[service_index][att_index];
			const attr_desc_t *att_desc = &att_db->att_desc;
			ble_uuid_t att_desc_uuid;
			bufferToUUID(&att_desc_uuid, att_desc->uuid_p, att_desc->uuid_length);
			if (BLE_UUID_EQ(&att_desc_uuid, uuid)) {
				for (int k = 0; k < char_name_count; ++k) {
					const char_name_table *char_name = &char_names[k];
					if (service_index == char_name->service_index && att_index == char_name->att_index) {
						connection->handles[k] = handle;
						result = k;
						goto bail;
					}
				}
			}
		}
	}
bail:
	return result;
}

void modBLEClientBondingRemoved(ble_gap_addr_t *peer_addr)
{
	if (!gBLE) return;

	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), peer_addr->addr, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), peer_addr->addr_type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onBondingDeleted"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void clearScanned(modBLE ble)
{
	modBLEScannedPacket walker = ble->scanned;
	while (walker) {
		modBLEScannedPacket next = walker->next;
		c_free(walker);
		walker = next;
	}
	ble->scanned = NULL;
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
	if (!gBLE) return;

	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	ble_gap_evt_connected_t const * p_evt_connected = (ble_gap_evt_connected_t const *)&gap_evt->params.connected;
	uint16_t conn_handle = gap_evt->conn_handle;
	modBLEConnection connection;
	
	xsBeginHost(gBLE->the);
	
	connection = modBLEConnectionFindByAddressAndType((uint8_t*)&p_evt_connected->peer_addr.addr, p_evt_connected->peer_addr.addr_type);

	if (!connection)
		xsUnknownError("connection not found");
		
	if (kInvalidConnectionID != connection->id)
		goto bail;
		
	connection->id = conn_handle;

	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), conn_handle);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), (uint8_t*)&p_evt_connected->peer_addr.addr[0], BLE_GAP_ADDR_LEN);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), p_evt_connected->peer_addr.addr_type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));

bail:
	xsEndHost(gBLE->the);
}

void gapDisconnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	ble_gap_evt_disconnected_t const * p_evt_disconnected = (ble_gap_evt_disconnected_t const *)&gap_evt->params.disconnected;
	uint16_t conn_handle = gap_evt->conn_handle;

	xsBeginHost(gBLE->the);
	
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);

	// ignore multiple disconnects on same connection
	if (!connection) {
		LOG_GATTC_MSG("Ignoring duplicate disconnect event");
		goto bail;
	}	
	
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), conn_handle);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), connection->address, BLE_GAP_ADDR_LEN);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), connection->addressType);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	
	// remove connection before calling onDisconnected callback, in case app tries to reconnect from the callback
	modBLEConnectionRemove(connection);

	xsCall2(connection->objConnection, xsID_callback, xsString("onDisconnected"), xsVar(0));
	xsForget(connection->objConnection);

bail:
	xsEndHost(gBLE->the);
}

void gapAdvReportEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	deviceDiscoveryRecord *entry;

	if (!gBLE) return;

	xsBeginHost(gBLE->the);
	xsmcVars(2);
	while (NULL != (entry = (deviceDiscoveryRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {
		ble_gap_evt_adv_report_t *p_evt_adv_report = &entry->adv_report;
		if (!gBLE->duplicates) {
			uint16_t type =
				(p_evt_adv_report->type.connectable   << 0) |
				(p_evt_adv_report->type.scannable     << 1) |
				(p_evt_adv_report->type.directed      << 2) |
				(p_evt_adv_report->type.scan_response << 3) |
				(p_evt_adv_report->type.extended_pdu  << 4) |
				(p_evt_adv_report->type.status        << 5) |
				(p_evt_adv_report->type.reserved      << 7);
			uint8_t found = false;
			
			modBLEScannedPacket scanned = gBLE->scanned;
			while (scanned && !found) {
				found = (type == scanned->type && 0 == c_memcmp(p_evt_adv_report->peer_addr.addr, scanned->addr, 6));
				scanned = scanned->next;
			}
			if (found) {
				c_free(entry);
				continue;
			}
			modBLEScannedPacket address = c_calloc(1, sizeof(modBLEScannedPacketRecord));
			if (!address)
				xsUnknownError("out of memory");
			address->type = type;
			c_memmove(address->addr, p_evt_adv_report->peer_addr.addr, 6);
			if (!gBLE->scanned)
				gBLE->scanned = address;
			else {
				modBLEScannedPacket walker;
				for (walker = gBLE->scanned; walker->next; walker = walker->next)
					;
				walker->next = address;
			}
		}
		
		xsVar(0) = xsmcNewObject();
		xsmcSetArrayBuffer(xsVar(1), entry->data, p_evt_adv_report->data.len);
		xsmcSet(xsVar(0), xsID_scanResponse, xsVar(1));
		xsmcSetArrayBuffer(xsVar(1), (void*)&p_evt_adv_report->peer_addr.addr[0], BLE_GAP_ADDR_LEN);
		xsmcSet(xsVar(0), xsID_address, xsVar(1));
		xsmcSetInteger(xsVar(1), p_evt_adv_report->peer_addr.addr_type);
		xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
		xsmcSetInteger(xsVar(1), p_evt_adv_report->rssi);
		xsmcSet(xsVar(0), xsID_rssi, xsVar(1));
		xsCall2(gBLE->obj, xsID_callback, xsString("onDiscovered"), xsVar(0));
		
		c_free(entry);
	}
	xsEndHost(gBLE->the);
}

void gapAuthKeyRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	ble_gap_evt_auth_key_request_t const * p_evt_auth_key_request = (ble_gap_evt_auth_key_request_t const *)&gap_evt->params.auth_key_request;
	uint16_t conn_handle = gap_evt->conn_handle;

	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), connection->address, BLE_GAP_ADDR_LEN);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyInput"), xsVar(0));
	xsEndHost(gBLE->the);
}

void gapPasskeyDisplayEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	ble_gap_evt_passkey_display_t const * p_evt_passkey_display = (ble_gap_evt_passkey_display_t const *)&gap_evt->params.passkey_display;
	uint16_t conn_handle = gap_evt->conn_handle;

	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), connection->address, BLE_GAP_ADDR_LEN);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), atoi(p_evt_passkey_display->passkey));
	xsmcSet(xsVar(0), xsID_passkey, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString(p_evt_passkey_display->match_request ? "onPasskeyConfirm" : "onPasskeyDisplay"), xsVar(0));
	xsEndHost(gBLE->the);
}

void gapRSSIChangedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	ble_gap_evt_rssi_changed_t const * p_evt_rssi_changed = &gap_evt->params.rssi_changed;
	uint16_t conn_handle = gap_evt->conn_handle;

	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
	xsCall2(connection->objConnection, xsID_callback, xsString("onRSSI"), xsInteger(p_evt_rssi_changed->rssi));
	xsEndHost(gBLE->the);
}

static void pmConnSecSucceededEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	pm_evt_t *pm_evt = (pm_evt_t*)message;
	pm_conn_secured_evt_t const * conn_sec_succeeded = (pm_conn_secured_evt_t const *)&pm_evt->params.conn_sec_succeeded;
	uint16_t conn_handle = pm_evt->conn_handle;

	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
		
	// Only send one "onAuthenticated" callback per successful connection
	if (connection->authenticated) goto bail;
	
	connection->authenticated = 1;

	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	
	// The PM_CONN_SEC_PROCEDURE_ENCRYPTION procedure uses a LTK that was shared during a previous bonding procedure to encrypt the link.
	// We therefore consider consider the connection bonded when completing the PM_CONN_SEC_PROCEDURE_BONDING or PM_CONN_SEC_PROCEDURE_ENCRYPTION
	// security procedures.
	uint8_t bonded = (PM_CONN_SEC_PROCEDURE_ENCRYPTION == conn_sec_succeeded->procedure || PM_CONN_SEC_PROCEDURE_BONDING == conn_sec_succeeded->procedure);
	xsmcSetBoolean(xsVar(1), bonded);
	xsmcSet(xsVar(0), xsID_bonded, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onAuthenticated"), xsVar(0));
	
bail:
	xsEndHost(gBLE->the);
}

static void pmGapConnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	ble_gap_evt_connected_t const * p_evt_connected = (ble_gap_evt_connected_t const *)&gap_evt->params.connected;
	uint16_t conn_handle = gap_evt->conn_handle;
	modBLEClientConnection connection;

	if (!gBLE) return;
	
	xsBeginHost(gBLE->the);

	connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");

	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), connection->id);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), (uint8_t*)&connection->address[0], BLE_GAP_ADDR_LEN);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), connection->addressType);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));
	
	xsEndHost(gBLE->the);
}

void gattcServiceDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	serviceSearchRecord *entry;
	uint16_t i, end_handle = 0;

	if (!gBLE) return;
	
	xsBeginHost(gBLE->the);
	while (NULL != (entry = (serviceSearchRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {		
		modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection || (connection->id != connection->gattProcedure.conn_handle))
			xsUnknownError("connection not found");
		if (entry->completed) {
			xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onService"));
			c_free(entry);
			goto bail;
		}
		for (i = 0; i < entry->count; ++i) {
			uint8_t buffer[UUID_LEN_128];
			uint16_t length;
			ble_gattc_service_t const *service = &entry->services[i];
	
			// @@ Likely 128-bit UUID - TBD
			if (BLE_UUID_TYPE_UNKNOWN == service->uuid.type) {
				if (service->handle_range.end_handle > end_handle)
					end_handle = service->handle_range.end_handle;
				continue;
			}
	
			uuidToBuffer(buffer, (ble_uuid_t *)&service->uuid, &length);
			if (0 != length) {
				xsmcVars(2);
				xsVar(0) = xsmcNewObject();
				xsmcSetArrayBuffer(xsVar(1), buffer, length);
				xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
				xsmcSetInteger(xsVar(1), service->handle_range.start_handle);
				xsmcSet(xsVar(0), xsID_start, xsVar(1));
				xsmcSetInteger(xsVar(1), service->handle_range.end_handle);
				xsmcSet(xsVar(0), xsID_end, xsVar(1));
				xsCall2(connection->objClient, xsID_callback, xsString("onService"), xsVar(0));
				if (service->handle_range.end_handle > end_handle)
					end_handle = service->handle_range.end_handle;
			}
		}
		c_free(entry);
		
		if (connection->gattProcedure.searchUUID.uuid != 0 || 65535 == end_handle) {
			xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onService"));
		}
		else {
			ret_code_t err_code = sd_ble_gattc_primary_services_discover(connection->id, end_handle + 1, NULL);
			if (NRF_SUCCESS != err_code) {
				xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onService"));
			}
		}
	}
	
bail:
	xsEndHost(gBLE->the);
}

void gattcCharacteristicDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	characteristicSearchRecord *entry;
	uint16_t i, end_handle = 0;
	ret_code_t err_code;

	if (!gBLE) return;
	
	xsBeginHost(gBLE->the);
	while (NULL != (entry = (characteristicSearchRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {
		modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection || (connection->id != connection->gattProcedure.conn_handle))
			xsUnknownError("connection not found");
		if (entry->completed) {
			xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onCharacteristic"));
			c_free(entry);
			goto bail;
		}
		for (i = 0; i < entry->count; ++i) {
			uint8_t buffer[UUID_LEN_128];
			uint16_t length;
			ble_gattc_char_t const *characteristic = &entry->chars[i];
		
			// @@ Likely 128-bit UUID - TBD
			if (BLE_UUID_TYPE_UNKNOWN == characteristic->uuid.type) {
				if (characteristic->handle_value > end_handle)
					end_handle = characteristic->handle_value;
				continue;
			}

			uuidToBuffer(buffer, (ble_uuid_t *)&characteristic->uuid, &length);
			if ((connection->gattProcedure.searchUUID.uuid != 0) && BLE_UUID_NEQ(&connection->gattProcedure.searchUUID, &characteristic->uuid)) {
				if (characteristic->handle_value > end_handle)
					end_handle = characteristic->handle_value;
				continue;
			}
			
			int index = modBLEConnectionSaveAttHandle(connection, (ble_uuid_t *)&characteristic->uuid, characteristic->handle_value);
			
			xsmcVars(2);
			xsVar(0) = xsmcNewObject();
			xsmcSetArrayBuffer(xsVar(1), buffer, length);
			xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
			xsmcSetInteger(xsVar(1), characteristic->handle_value);
			xsmcSet(xsVar(0), xsID_handle, xsVar(1));

			uint8_t properties = 0;
			if (characteristic->char_props.read)
				properties |= GATT_CHAR_PROP_BIT_READ;
			if (characteristic->char_props.write_wo_resp)
				properties |= GATT_CHAR_PROP_BIT_WRITE_NR;
			if (characteristic->char_props.write)
				properties |= GATT_CHAR_PROP_BIT_WRITE;
			if (characteristic->char_props.notify)
				properties |= GATT_CHAR_PROP_BIT_NOTIFY;
			if (characteristic->char_props.indicate)
				properties |= GATT_CHAR_PROP_BIT_INDICATE;
			xsmcSetInteger(xsVar(1), properties);
			xsmcSet(xsVar(0), xsID_properties, xsVar(1));
		
			if (-1 != index) {
				xsmcSetString(xsVar(1), (char*)char_names[index].name);
				xsmcSet(xsVar(0), xsID_name, xsVar(1));
				xsmcSetString(xsVar(1), (char*)char_names[index].type);
				xsmcSet(xsVar(0), xsID_type, xsVar(1));
			}
			xsCall2(connection->gattProcedure.obj, xsID_callback, xsString("onCharacteristic"), xsVar(0));
		
			if (connection->gattProcedure.searchUUID.uuid != 0) {
				xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onCharacteristic"));
				modBLEMessageQueueEmpty(&gBLE->discoveryQueue);
				c_free(entry);
				goto bail;
			}
			if (characteristic->handle_value > end_handle)
				end_handle = characteristic->handle_value;
		}
		c_free(entry);
		
		connection->gattProcedure.handle_range.start_handle = end_handle + 1;
		err_code = sd_ble_gattc_characteristics_discover(connection->id, &connection->gattProcedure.handle_range);
		if (NRF_SUCCESS != err_code)
			xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onCharacteristic"));
	}
	
bail:
	xsEndHost(gBLE->the);
}

void gattcCharacteristicsForDescriptorsDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	characteristicSearchRecord *entry;
	uint16_t i, end_handle = 0;
	ret_code_t err_code;

	if (!gBLE) return;
	
	xsBeginHost(gBLE->the);
	while (NULL != (entry = (characteristicSearchRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {		
		modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection || (connection->id != connection->gattProcedure.conn_handle))
			xsUnknownError("connection not found");
		if (entry->completed) {
			c_free(entry);
			goto bail;
		}
		for (i = 0; i < entry->count; ++i) {
			uint8_t buffer[UUID_LEN_128];
			uint16_t length;
			ble_gattc_char_t const *characteristic = &entry->chars[i];

			// @@ Likely 128-bit UUID - TBD
			if (BLE_UUID_TYPE_UNKNOWN == characteristic->uuid.type) {
				if (characteristic->handle_value > end_handle)
					end_handle = characteristic->handle_value;
				continue;
			}

			if (connection->gattProcedure.discovery_handle < characteristic->handle_value) {
				connection->gattProcedure.handle_range.start_handle = connection->gattProcedure.discovery_handle + 1;
				connection->gattProcedure.handle_range.end_handle = characteristic->handle_decl - 1;
				connection->gattProcedure.discovery_handle = 0;
				modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, gattcDescriptorDiscoveryEvent, NULL);
				c_free(entry);
				sd_ble_gattc_descriptors_discover(connection->gattProcedure.conn_handle, &connection->gattProcedure.handle_range);
				goto bail;
			}
			if (characteristic->handle_value > end_handle)
				end_handle = characteristic->handle_value;
		}
		c_free(entry);

		connection->gattProcedure.handle_range.start_handle = end_handle + 1;
		sd_ble_gattc_characteristics_discover(connection->id, &connection->gattProcedure.handle_range);
	}

bail:
	xsEndHost(gBLE->the);
}

void gattcDescriptorDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	descriptorSearchRecord *entry;
	uint16_t i, end_handle = 0;
	ret_code_t err_code;

	if (!gBLE) return;
	
	xsBeginHost(gBLE->the);
	while (NULL != (entry = (descriptorSearchRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {		
		modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection || (connection->id != connection->gattProcedure.conn_handle))
			xsUnknownError("connection not found");
		if (entry->completed) {
			xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onDescriptor"));
			c_free(entry);
			goto bail;
		}
		for (i = 0; i < entry->count; ++i) {
			uint8_t buffer[UUID_LEN_128];
			uint16_t length;
			ble_gattc_desc_t const *descriptor = &entry->descs[i];
		
			// @@ Likely 128-bit UUID - TBD
			if (BLE_UUID_TYPE_UNKNOWN == descriptor->uuid.type) {
				end_handle = descriptor->handle;
				continue;
			}
			// Don't report CCCDs
			else if (0x2902 == descriptor->uuid.uuid) {
				end_handle = descriptor->handle;
				continue;
			}

			uuidToBuffer(buffer, (ble_uuid_t *)&descriptor->uuid, &length);
			
			int index = modBLEConnectionSaveAttHandle(connection, (ble_uuid_t *)&descriptor->uuid, descriptor->handle);
			
			xsmcVars(2);
			xsVar(0) = xsmcNewObject();
			xsmcSetArrayBuffer(xsVar(1), buffer, length);
			xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
			xsmcSetInteger(xsVar(1), descriptor->handle);
			xsmcSet(xsVar(0), xsID_handle, xsVar(1));
		
			if (-1 != index) {
				xsmcSetString(xsVar(1), (char*)char_names[index].name);
				xsmcSet(xsVar(0), xsID_name, xsVar(1));
				xsmcSetString(xsVar(1), (char*)char_names[index].type);
				xsmcSet(xsVar(0), xsID_type, xsVar(1));
			}
			xsCall2(connection->gattProcedure.obj, xsID_callback, xsString("onDescriptor"), xsVar(0));
		
			end_handle = descriptor->handle;
		}
		c_free(entry);

		connection->gattProcedure.handle_range.start_handle = end_handle + 1;
		err_code = sd_ble_gattc_descriptors_discover(connection->gattProcedure.conn_handle, &connection->gattProcedure.handle_range);
		if (NRF_SUCCESS != err_code)
			xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onDescriptor"));
	}
	
bail:
	xsEndHost(gBLE->the);
}

void gattcCharacteristicReadEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;
	
	ble_gattc_evt_read_rsp_t const *read_rsp = (ble_gattc_evt_read_rsp_t const *)message;
	uint32_t conn_handle = (uint32_t)refcon;

	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
	
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), (void*)read_rsp->data, read_rsp->len);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSetInteger(xsVar(1), read_rsp->handle);
	xsmcSet(xsVar(0), xsID_handle, xsVar(1));
	
	xsCall2(connection->objClient, xsID_callback, xsString(connection->gattProcedure.id == CHARACTERISTIC_READ_VALUE ? "onCharacteristicValue" : "onDescriptorValue"), xsVar(0));
	
	xsEndHost(gBLE->the);
}

static void gattcCharacteristicNotificationEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	attributeNotificationRecord *entry;

	if (!gBLE) return;

	xsBeginHost(gBLE->the);
	xsmcVars(2);
	while (NULL != (entry = (attributeNotificationRecord*)modBLEMessageQueueDequeue(&gBLE->notificationQueue))) {
		modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection)
			xsUnknownError("connection not found");
		ble_gattc_evt_hvx_t const *hvx = (ble_gattc_evt_hvx_t const *)&entry->hvx;
		xsVar(0) = xsmcNewObject();
		xsmcSetArrayBuffer(xsVar(1), (void*)hvx->data, hvx->len);
		xsmcSet(xsVar(0), xsID_value, xsVar(1));
		xsmcSetInteger(xsVar(1), hvx->handle);
		xsmcSet(xsVar(0), xsID_handle, xsVar(1));
		xsCall2(connection->objClient, xsID_callback, xsString("onCharacteristicNotification"), xsVar(0));
		c_free(entry);
	}
	xsEndHost(gBLE->the);
}

static void gattcCharacteristicWriteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;
	
	ble_gattc_evt_write_rsp_t const *write_rsp = (ble_gattc_evt_write_rsp_t const *)message;
	uint32_t conn_handle = (uint32_t)refcon;

	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
		
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), write_rsp->handle);
	xsmcSet(xsVar(0), xsID_handle, xsVar(1));
	xsCall2(connection->objClient, xsID_callback, xsString("onDescriptorWritten"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

static void gattcMTUExchangedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;
	
	ble_gattc_evt_exchange_mtu_rsp_t const *mtu = (ble_gattc_evt_exchange_mtu_rsp_t const *)message;
	uint32_t conn_handle = (uint32_t)refcon;

	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection || !connection->mtu_exchange_pending) return;
	
	connection->mtu_exchange_pending = 0;
	xsBeginHost(gBLE->the);
	xsCall2(connection->objConnection, xsID_callback, xsString("onMTUExchanged"), xsInteger(mtu->server_rx_mtu));
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
				deviceDiscoveryRecord *entry = c_malloc(sizeof(deviceDiscoveryRecord) - 1 + p_evt_adv_report->data.len);
				if (NULL != entry) {
					entry->conn_id = p_ble_evt->evt.gap_evt.conn_handle;
					entry->adv_report = *p_evt_adv_report;
					c_memmove(entry->data, p_evt_adv_report->data.p_data, p_evt_adv_report->data.len);
					modBLEMessageQueueEnqueue(&gBLE->discoveryQueue, (modBLEMessageQueueEntry)entry);
				}
			}
   			break;
    	}
		case BLE_GAP_EVT_AUTH_KEY_REQUEST:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&p_ble_evt->evt.gap_evt, sizeof(ble_gap_evt_t), gapAuthKeyRequestEvent, NULL);
			break;
		case BLE_GAP_EVT_CONNECTED:
			if (0xFF != gBLE->iocap)
				pm_conn_secure(p_ble_evt->evt.gap_evt.conn_handle, false);
			modMessagePostToMachine(gBLE->the, (uint8_t*)&p_ble_evt->evt.gap_evt, sizeof(ble_gap_evt_t), gapConnectedEvent, NULL);
			break;
		case BLE_GAP_EVT_DISCONNECTED:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&p_ble_evt->evt.gap_evt, sizeof(ble_gap_evt_t), gapDisconnectedEvent, NULL);
			break;
        case BLE_GAP_EVT_PASSKEY_DISPLAY:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&p_ble_evt->evt.gap_evt, sizeof(ble_gap_evt_t), gapPasskeyDisplayEvent, NULL);
        	break;
        case BLE_GAP_EVT_RSSI_CHANGED:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&p_ble_evt->evt.gap_evt, sizeof(ble_gap_evt_t), gapRSSIChangedEvent, NULL);
			sd_ble_gap_rssi_stop(p_ble_evt->evt.gap_evt.conn_handle);       	
        	break;
		case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST: {
			ble_gap_data_length_params_t dlp;
			dlp.max_rx_octets = BLE_GAP_DATA_LENGTH_AUTO;
			dlp.max_tx_octets = BLE_GAP_DATA_LENGTH_AUTO;
			dlp.max_rx_time_us = BLE_GAP_DATA_LENGTH_AUTO;
			dlp.max_tx_time_us = BLE_GAP_DATA_LENGTH_AUTO;
			sd_ble_gap_data_length_update(p_ble_evt->evt.gap_evt.conn_handle, &dlp, NULL);
			break;
		}
		case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
			ble_gap_phys_t const phys = {
				.rx_phys = BLE_GAP_PHY_AUTO,
				.tx_phys = BLE_GAP_PHY_AUTO,
			};
			sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
			break;
		}
		case BLE_GAP_EVT_SEC_INFO_REQUEST:
			sd_ble_gap_sec_info_reply(p_ble_evt->evt.gap_evt.conn_handle, NULL, NULL, NULL);
			break;
			
		case BLE_GATTC_EVT_CHAR_DISC_RSP: {
			characteristicSearchRecord *entry;
			ble_gattc_evt_char_disc_rsp_t const *char_disc_rsp = &p_ble_evt->evt.gattc_evt.params.char_disc_rsp;
			uint16_t len = sizeof(characteristicSearchRecord) + ((char_disc_rsp->count - 1) * sizeof(ble_gattc_char_t));
			entry = c_malloc(len);
			if (NULL != entry) {
				entry->conn_id = p_ble_evt->evt.gattc_evt.conn_handle;
				entry->completed = p_ble_evt->evt.gattc_evt.gatt_status != 0;
				entry->count = char_disc_rsp->count;
				c_memmove(entry->chars, char_disc_rsp->chars, char_disc_rsp->count * sizeof(ble_gattc_char_t));
				modBLEMessageQueueEnqueue(&gBLE->discoveryQueue, (modBLEMessageQueueEntry)entry);
			}
			break;
		}
		case BLE_GATTC_EVT_DESC_DISC_RSP: {
			descriptorSearchRecord *entry;
			ble_gattc_evt_desc_disc_rsp_t const *desc_disc_rsp = &p_ble_evt->evt.gattc_evt.params.desc_disc_rsp;
			uint16_t len = sizeof(descriptorSearchRecord) + ((desc_disc_rsp->count - 1) * sizeof(ble_gattc_desc_t));
			entry = c_malloc(len);
			if (NULL != entry) {
				entry->conn_id = p_ble_evt->evt.gattc_evt.conn_handle;
				entry->completed = p_ble_evt->evt.gattc_evt.gatt_status != 0;	
				entry->count = desc_disc_rsp->count;
				c_memmove(entry->descs, desc_disc_rsp->descs, desc_disc_rsp->count * sizeof(ble_gattc_desc_t));
				modBLEMessageQueueEnqueue(&gBLE->discoveryQueue, (modBLEMessageQueueEntry)entry);
			}
			break;
		}
        case BLE_GATTC_EVT_EXCHANGE_MTU_RSP:
			if (BLE_GATT_STATUS_SUCCESS == p_ble_evt->evt.gattc_evt.gatt_status) {
				ble_gattc_evt_exchange_mtu_rsp_t const *mtu = &p_ble_evt->evt.gattc_evt.params.exchange_mtu_rsp;
				modMessagePostToMachine(gBLE->the, (uint8_t*)mtu, sizeof(ble_gattc_evt_exchange_mtu_rsp_t), gattcMTUExchangedEvent, (void*)(uint32_t)p_ble_evt->evt.gattc_evt.conn_handle);
			}
        	break;
        case BLE_GATTC_EVT_HVX:
			if (BLE_GATT_STATUS_SUCCESS == p_ble_evt->evt.gattc_evt.gatt_status) {
				attributeNotificationRecord *entry;
				ble_gattc_evt_hvx_t const *hvx = &p_ble_evt->evt.gattc_evt.params.hvx;
				entry = c_malloc(sizeof(attributeNotificationRecord) + hvx->len - 1);
				if (NULL != entry) {
					entry->conn_id = p_ble_evt->evt.gattc_evt.conn_handle;
					c_memmove(&entry->hvx, hvx, sizeof(ble_gattc_evt_hvx_t) - 1 + hvx->len);
					modBLEMessageQueueEnqueue(&gBLE->notificationQueue, (modBLEMessageQueueEntry)entry);
				}
			}
        	break;
		case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP: {
			serviceSearchRecord *entry;
			ble_gattc_evt_prim_srvc_disc_rsp_t const *prim_srvc_disc_rsp = &p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp;
			uint16_t len = sizeof(serviceSearchRecord) + ((prim_srvc_disc_rsp->count - 1) * sizeof(ble_gattc_service_t));
			entry = c_malloc(len);
			if (NULL != entry) {
				entry->conn_id = p_ble_evt->evt.gattc_evt.conn_handle;
				entry->completed = p_ble_evt->evt.gattc_evt.gatt_status != 0;	
				entry->count = prim_srvc_disc_rsp->count;
				c_memmove(entry->services, prim_srvc_disc_rsp->services, prim_srvc_disc_rsp->count * sizeof(ble_gattc_service_t));
				modBLEMessageQueueEnqueue(&gBLE->discoveryQueue, (modBLEMessageQueueEntry)entry);
			}
			break;
		}
        case BLE_GATTC_EVT_READ_RSP:
			if (BLE_GATT_STATUS_SUCCESS == p_ble_evt->evt.gattc_evt.gatt_status) {
				ble_gattc_evt_read_rsp_t const *read_rsp = &p_ble_evt->evt.gattc_evt.params.read_rsp;
				modMessagePostToMachine(gBLE->the, (uint8_t*)read_rsp, sizeof(ble_gattc_evt_read_rsp_t) + read_rsp->len - 1, gattcCharacteristicReadEvent, (void*)(uint32_t)p_ble_evt->evt.gattc_evt.conn_handle);
			}
        	break;
        case BLE_GATTC_EVT_WRITE_RSP:
			if (BLE_GATT_STATUS_SUCCESS == p_ble_evt->evt.gattc_evt.gatt_status) {
				ble_gattc_evt_write_rsp_t const *write_rsp = &p_ble_evt->evt.gattc_evt.params.write_rsp;
				modMessagePostToMachine(gBLE->the, (uint8_t*)write_rsp, sizeof(ble_gattc_evt_write_rsp_t) + write_rsp->len - 1, gattcCharacteristicWriteEvent, (void*)(uint32_t)p_ble_evt->evt.gattc_evt.conn_handle);
			}
        	break;
        	
        default:
            break;
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
		case PM_EVT_PEER_DELETE_FAILED: modLog("PM_EVT_PEER_DELETE_FAILED"); break;
		case PM_EVT_PEERS_DELETE_SUCCEEDED: modLog("PM_EVT_PEERS_DELETE_SUCCEEDED"); break;
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
	
	//pm_handler_on_pm_evt(p_evt);
	pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id) {
    	case PM_EVT_CONN_SEC_FAILED:
            // Rebond if one party has lost its keys
            if (p_evt->params.conn_sec_failed.error == PM_CONN_SEC_ERROR_PIN_OR_KEY_MISSING)
				pm_conn_secure(p_evt->conn_handle, true);
    		break;
		case PM_EVT_CONN_SEC_SUCCEEDED:
			modMessagePostToMachine(gBLE->the, (uint8_t*)p_evt, sizeof(pm_evt_t), pmConnSecSucceededEvent, NULL);
			break;
		case PM_EVT_CONN_SEC_CONFIG_REQ: {
			pm_conn_sec_config_t pm_conn_sec_config;
			pm_conn_sec_config.allow_repairing = true;
			pm_conn_sec_config_reply(p_evt->conn_handle, &pm_conn_sec_config);
			break;
		}
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
		case NRF_BLE_SCAN_EVT_CONNECTING_ERROR: modLog("NRF_BLE_SCAN_EVT_CONNECTING_ERROR"); break;
		case NRF_BLE_SCAN_EVT_CONNECTED: modLog("NRF_BLE_SCAN_EVT_CONNECTED"); break;
	}
}

// This scan_evt_handler() function seems to be required to support the whitelist, even though it is empty.
static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
	LOG_SCAN_EVENT(p_scan_evt->scan_evt_id);

    switch(p_scan_evt->scan_evt_id) {
        case NRF_BLE_SCAN_EVT_WHITELIST_REQUEST:
        	break;
        case NRF_BLE_SCAN_EVT_CONNECTING_ERROR:
        	break;
        case NRF_BLE_SCAN_EVT_SCAN_TIMEOUT:
        	break;
        case NRF_BLE_SCAN_EVT_FILTER_MATCH:
            break;
        case NRF_BLE_SCAN_EVT_WHITELIST_ADV_REPORT:
            break;
        default:
			break;
    }
}

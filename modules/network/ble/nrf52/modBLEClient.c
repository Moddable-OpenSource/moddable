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
#include "modBLECommon.h"

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

typedef struct {
	xsSlot obj;
	ble_uuid_t searchUUID;
	ble_gattc_handle_range_t handle_range;
	uint16_t discovery_handle;
} gattProcedureRecord;

typedef struct {
	uint16_t conn_handle;
	uint16_t gatt_status;
	uint8_t rsp[1];
} gattDiscoveryRecord;

typedef struct modBLEConnectionRecord modBLEConnectionRecord;
typedef modBLEConnectionRecord *modBLEConnection;

struct modBLEConnectionRecord {
	struct modBLEConnectionRecord *next;

	xsMachine	*the;
	xsSlot		objConnection;
	xsSlot		objClient;

	ble_gap_addr_t addr;
	uint16_t conn_handle;
	uint8_t mtu_exchange_pending;
	
	// gatt procedure
	gattProcedureRecord gattProcedure;
	
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
	uint8_t iocap;

	modBLEConnection connections;
} modBLERecord, *modBLE;

static void modBLEConnectionAdd(modBLEConnection connection);
static void modBLEConnectionRemove(modBLEConnection connection);
static modBLEConnection modBLEConnectionFindByConnectionID(uint16_t conn_id);
static modBLEConnection modBLEConnectionFindByAddress(ble_gap_addr_t *addr);
static modBLEConnection modBLEConnectionFindByAddressBytes(uint8_t *addr);
static int modBLEConnectionSaveAttHandle(modBLEConnection connection, ble_uuid_t *uuid, uint16_t handle);

static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
static void pm_evt_handler(pm_evt_t const * p_evt);

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
static void gattcCharacteristicsForDescriptorsDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gattcDescriptorDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gattcMTUExchangedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void gattcServiceDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

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
	xsRemember(gBLE->obj);
	
	// Initialize platform Bluetooth modules
	init.p_gatt = &gBLE->m_gatt;
	init.pm_event_handler = pm_evt_handler;
	p_cp_init = &init.cp_init;
	c_memset(p_cp_init, 0, sizeof(ble_conn_params_init_t));
	p_cp_init->first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
	p_cp_init->next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
	p_cp_init->max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
	
	err_code = modBLEPlatformInitialize(&init);
	
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
	
	modBLEConnection connections = ble->connections, next;
	while (connections != NULL) {
		modBLEConnection connection = connections;
		connections = connections->next;
		c_free(connection);
	}

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
	ble_gap_addr_t addr;
	ret_code_t err_code;

	addr.addr_id_peer = 0;
	addr.addr_type = addressType;
	c_memmove(&addr.addr, address, BLE_GAP_ADDR_LEN);
		
	// Ignore duplicate connection attempts
	if (modBLEConnectionFindByAddress(&addr)) {
		LOG_GATTC_MSG("Ignoring duplicate connect attempt");
		return;
	};
	
	// Add a new connection record to be filled as the connection completes
	modBLEConnection connection = c_calloc(sizeof(modBLEConnectionRecord), 1);
	if (!connection)
		xsUnknownError("out of memory");
	connection->conn_handle = BLE_CONN_HANDLE_INVALID;
	connection->addr = addr;
	modBLEConnectionAdd(connection);
	
	err_code = sd_ble_gap_connect(&addr, &m_scan.scan_params, &m_scan.conn_params, APP_BLE_CONN_CFG_TAG);
	if (NRF_SUCCESS != err_code)
		xsUnknownError("ble connect failed");
}

void xs_ble_client_set_security_parameters(xsMachine *the)
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

void xs_ble_client_passkey_reply(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint32_t digits = xsmcToInteger(xsArg(1));
	char passkey[7];
	
	modBLEConnection connection = modBLEConnectionFindByAddressBytes(address);
	if (!connection)
		xsUnknownError("connection not found");

	itoa(digits, passkey, 10);
	sd_ble_gap_auth_key_reply(connection->conn_handle, BLE_GAP_AUTH_KEY_TYPE_PASSKEY, passkey);
}

void xs_gap_connection_initialize(xsMachine *the)
{
	uint8_t conn_id;
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

	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;
	if (argc > 1)
		bufferToUUID(&connection->gattProcedure.searchUUID, (uint8_t*)xsmcToArrayBuffer(xsArg(1)), xsGetArrayBufferLength(xsArg(1)));
	else
		connection->gattProcedure.searchUUID.uuid = 0;
	connection->gattProcedure.obj = xsThis;
	
	sd_ble_gattc_primary_services_discover(conn_handle, 1, (argc > 1 ? &connection->gattProcedure.searchUUID : NULL));
}

void xs_gatt_service_discover_characteristics(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint8_t conn_handle = xsmcToInteger(xsArg(0));

	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;
	
	connection->gattProcedure.handle_range.start_handle = xsmcToInteger(xsArg(1));
	connection->gattProcedure.handle_range.end_handle = xsmcToInteger(xsArg(2));
	connection->gattProcedure.obj = xsThis;
	if (argc > 3) {
		bufferToUUID(&connection->gattProcedure.searchUUID, (uint8_t*)xsmcToArrayBuffer(xsArg(3)), xsGetArrayBufferLength(xsArg(3)));
	}
	else
		connection->gattProcedure.searchUUID.uuid = 0;
		
	sd_ble_gattc_characteristics_discover(conn_handle, &connection->gattProcedure.handle_range);
}

void xs_gatt_characteristic_discover_all_characteristic_descriptors(xsMachine *the)
{
	uint8_t conn_handle = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
	ble_uuid_t uuid;
	uint16_t start, end;

	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;
	
	// Discover all service characteristics to find the range of characteristic handles for searching descriptors
	connection->gattProcedure.searchUUID.uuid = 0;
	connection->gattProcedure.obj = xsThis;
	connection->gattProcedure.discovery_handle = characteristic;

	xsmcVars(2);
	xsmcGet(xsVar(0), xsThis, xsID_service);
	xsmcGet(xsVar(1), xsVar(0), xsID_start);
	start = xsmcToInteger(xsVar(1));
	xsmcGet(xsVar(1), xsVar(0), xsID_end);
	end = xsmcToInteger(xsVar(1));
	connection->gattProcedure.handle_range.start_handle = start;
	connection->gattProcedure.handle_range.end_handle = end;
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
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;
	
	sd_ble_gattc_read(connection->conn_handle, handle, 0);
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

	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
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
				len = xsGetArrayBufferLength(xsArg(2));
			}
			else
				goto unknown;
			break;
		unknown:
		default:
			xsUnknownError("unsupported type");
			break;
	}
	writeAttribute(conn_handle, handle, BLE_GATT_OP_WRITE_CMD, data, len);
}

void xs_gatt_characteristic_write_without_response(xsMachine *the)
{
	uint8_t conn_handle = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	uint8_t *data;
	uint16_t len;

	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;

	switch (xsmcTypeOf(xsArg(2))) {
		case xsStringType:
			data = xsmcToString(xsArg(2));
			len = c_strlen(data);
			break;
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype)) {
				data = xsmcToArrayBuffer(xsArg(2));
				len = xsGetArrayBufferLength(xsArg(2));
			}
			else
				goto unknown;
			break;
		unknown:
		default:
			xsUnknownError("unsupported type");
			break;
	}
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
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection) return;
	
	sd_ble_gattc_read(conn_handle, handle, 0);
}

modBLEConnection modBLEConnectionFindByConnectionID(uint16_t conn_handle)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (conn_handle == walker->conn_handle)
			break;
	return walker;
}

modBLEConnection modBLEConnectionFindByAddress(ble_gap_addr_t *addr)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (0 == c_memcmp(addr, (uint8_t*)&walker->addr, sizeof(ble_gap_addr_t)))
			break;
	return walker;
}

modBLEConnection modBLEConnectionFindByAddressBytes(uint8_t *addr)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (0 == c_memcmp(addr, (uint8_t*)&walker->addr.addr, BLE_GAP_ADDR_LEN))
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

int modBLEConnectionSaveAttHandle(modBLEConnection connection, ble_uuid_t *uuid, uint16_t handle)
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

void bleClientReadyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;
	
	for (int service_index = 0; service_index < service_count; ++service_index) {
		const gatts_attr_db_t *att_db = &gatt_db[service_index][0];
		const attr_desc_t *att_desc = &att_db->att_desc;
		
		// register vendor specific 128-bit uuids
		if (UUID_LEN_128 == att_desc->length) {
			uint8_t uuid_type;
			ble_uuid128_t ble_uuid_128;
			ble_uuid_128 = *(ble_uuid128_t*)att_desc->value;
			sd_ble_uuid_vs_add(&ble_uuid_128, &uuid_type);
		}
	}

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
	int16_t conn_handle = gap_evt->conn_handle;
	
	xsBeginHost(gBLE->the);
	
	modBLEConnection connection = modBLEConnectionFindByAddress((ble_gap_addr_t*)&p_evt_connected->peer_addr);
	if (!connection)
		xsUnknownError("connection not found");
		
	if (BLE_CONN_HANDLE_INVALID != connection->conn_handle)
		goto bail;
		
	connection->conn_handle = conn_handle;

	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), conn_handle);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), (uint8_t*)&p_evt_connected->peer_addr.addr[0], BLE_GAP_ADDR_LEN);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));

bail:
	xsEndHost(gBLE->the);
}

void gapDisconnectedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	ble_gap_evt_disconnected_t const * p_evt_disconnected = (ble_gap_evt_disconnected_t const *)&gap_evt->params.disconnected;
	int16_t conn_handle = gap_evt->conn_handle;

	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	
	// ignore multiple disconnects on same connection
	if (!connection) {
		LOG_GATTC_MSG("Ignoring duplicate disconnect event");
		goto bail;
	}	
	
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), conn_handle);
	xsCall2(connection->objConnection, xsID_callback, xsString("onDisconnected"), xsVar(0));
	modBLEConnectionRemove(connection);

bail:
	xsEndHost(gBLE->the);
}

void gapAdvReportEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	ble_gap_evt_adv_report_t const * p_evt_adv_report = (ble_gap_evt_adv_report_t const *)message;

	if (!gBLE) return;
	
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), p_evt_adv_report->data.p_data, p_evt_adv_report->data.len);
	xsmcSet(xsVar(0), xsID_scanResponse, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), (void*)&p_evt_adv_report->peer_addr.addr[0], BLE_GAP_ADDR_LEN);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), p_evt_adv_report->peer_addr.addr_type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onDiscovered"), xsVar(0));
	xsEndHost(gBLE->the);
}

void gapAuthStatusEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	ble_gap_evt_auth_status_t const * p_evt_auth_status = (ble_gap_evt_auth_status_t const *)&gap_evt->params.auth_status;
	int16_t conn_handle = gap_evt->conn_handle;

	if (BLE_GAP_SEC_STATUS_SUCCESS == p_evt_auth_status->auth_status) {
		xsBeginHost(gBLE->the);
		modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
		if (!connection)
			xsUnknownError("connection not found");
		xsCall1(gBLE->obj, xsID_callback, xsString("onAuthenticated"));
		xsEndHost(gBLE->the);
	}
}

void gapAuthKeyRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	ble_gap_evt_auth_key_request_t const * p_evt_auth_key_request = (ble_gap_evt_auth_key_request_t const *)&gap_evt->params.auth_key_request;
	int16_t conn_handle = gap_evt->conn_handle;

	ble_gap_evt_auth_key_request_t const * p_evt_auth_key_request = (ble_gap_evt_auth_key_request_t const *)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), connection->addr.addr, BLE_GAP_ADDR_LEN);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyInput"), xsVar(0));
	xsEndHost(gBLE->the);
}

void gapPasskeyDisplayEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;

	ble_gap_evt_t *gap_evt = (ble_gap_evt_t*)message;
	ble_gap_evt_passkey_display_t const * p_evt_passkey_display = (ble_gap_evt_passkey_display_t const *)&gap_evt->params.passkey_display;
	int16_t conn_handle = gap_evt->conn_handle;

	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), connection->addr.addr, BLE_GAP_ADDR_LEN);
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
	int16_t conn_handle = gap_evt->conn_handle;

	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
	xsCall2(connection->objConnection, xsID_callback, xsString("onRSSI"), xsInteger(p_evt_rssi_changed->rssi));
	xsEndHost(gBLE->the);
}

void gattcServiceDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	ret_code_t err_code;
	gattDiscoveryRecord *gdr;
	uint16_t end_handle = 0;

	gdr = (gattDiscoveryRecord *)refcon;
	ble_gattc_evt_prim_srvc_disc_rsp_t *prim_srvc_disc_rsp = (ble_gattc_evt_prim_srvc_disc_rsp_t *)gdr->rsp;
	uint16_t conn_handle = gdr->conn_handle;

	if (!gBLE) {
		c_free(gdr);
		return;
	}
	
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");

	if (BLE_GATT_STATUS_SUCCESS != gdr->gatt_status) {
		xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onService"));
		goto bail;
	}
	xsmcVars(2);
	for (uint16_t i = 0; i < prim_srvc_disc_rsp->count; ++i) {
		uint8_t buffer[UUID_LEN_128];
		uint16_t length;
		ble_gattc_service_t const *service = &prim_srvc_disc_rsp->services[i];
		
		// @@ Likely 128-bit UUID - TBD
		if (BLE_UUID_TYPE_UNKNOWN == service->uuid.type) {
			if (service->handle_range.end_handle > end_handle)
				end_handle = service->handle_range.end_handle;
			continue;
		}
		
		uuidToBuffer(buffer, (ble_uuid_t *)&service->uuid, &length);
		if (0 != length) {
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
	if (connection->gattProcedure.searchUUID.uuid != 0 || 65535 == end_handle) {
		xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onService"));
	}
	else {
		err_code = sd_ble_gattc_primary_services_discover(connection->conn_handle, end_handle + 1, NULL);
		if (NRF_SUCCESS != err_code) {
			xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onService"));
		}
	}
bail:
	c_free(gdr);
	xsEndHost(gBLE->the);
}

void gattcCharacteristicDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	ret_code_t err_code;
	gattDiscoveryRecord *gdr;
	uint16_t end_handle = 0;

	gdr = (gattDiscoveryRecord *)refcon;
	ble_gattc_evt_char_disc_rsp_t *char_disc_rsp = (ble_gattc_evt_char_disc_rsp_t *)gdr->rsp;
	uint16_t conn_handle = gdr->conn_handle;
	
	if (!gBLE) {
		c_free(gdr);
		return;
	}
	
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
    
	if (BLE_GATT_STATUS_SUCCESS != gdr->gatt_status) {
		xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onCharacteristic"));
		goto bail;
	}

	xsmcVars(2);
	
	for (uint16_t i = 0; i < char_disc_rsp->count; ++i) {
		uint8_t buffer[UUID_LEN_128];
		uint16_t length;
		ble_gattc_char_t const *characteristic = &char_disc_rsp->chars[i];
		
		// @@ Likely 128-bit UUID - TBD
		if (BLE_UUID_TYPE_UNKNOWN == characteristic->uuid.type) {
			if (characteristic->handle_value > end_handle);
				end_handle = characteristic->handle_value;
			continue;
		}

		uuidToBuffer(buffer, (ble_uuid_t *)&characteristic->uuid, &length);
		if ((connection->gattProcedure.searchUUID.uuid != 0) && BLE_UUID_NEQ(&connection->gattProcedure.searchUUID, &characteristic->uuid)) {
			if (characteristic->handle_value > end_handle);
				end_handle = characteristic->handle_value;
			continue;
		}
			
		int index = modBLEConnectionSaveAttHandle(connection, (ble_uuid_t *)&characteristic->uuid, characteristic->handle_value);
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
			goto bail;
		}
		if (characteristic->handle_value > end_handle);
			end_handle = characteristic->handle_value;
	}

	connection->gattProcedure.handle_range.start_handle = end_handle + 1;
	err_code = sd_ble_gattc_characteristics_discover(connection->conn_handle, &connection->gattProcedure.handle_range);
	if (NRF_SUCCESS != err_code)
		xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onCharacteristic"));
		
bail:
	c_free(gdr);
	xsEndHost(gBLE->the);
}

void gattcCharacteristicsForDescriptorsDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	ret_code_t err_code;
	gattDiscoveryRecord *gdr;
	uint16_t end_handle = 0;

	gdr = (gattDiscoveryRecord *)refcon;
	ble_gattc_evt_char_disc_rsp_t *char_disc_rsp = (ble_gattc_evt_char_disc_rsp_t *)gdr->rsp;
	uint16_t conn_handle = gdr->conn_handle;
	
	if (!gBLE) {
		c_free(gdr);
		return;
	}
	
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
    
	if (BLE_GATT_STATUS_SUCCESS != gdr->gatt_status) {
		goto bail;
	}

	xsmcVars(2);
	
	for (uint16_t i = 0; i < char_disc_rsp->count; ++i) {
		uint8_t buffer[UUID_LEN_128];
		uint16_t length;
		ble_gattc_char_t const *characteristic = &char_disc_rsp->chars[i];
		
		// @@ Likely 128-bit UUID - TBD
		if (BLE_UUID_TYPE_UNKNOWN == characteristic->uuid.type) {
			if (characteristic->handle_value > end_handle);
				end_handle = characteristic->handle_value;
			continue;
		}

		if (connection->gattProcedure.discovery_handle < characteristic->handle_value) {
			connection->gattProcedure.handle_range.start_handle = connection->gattProcedure.discovery_handle + 1;
			connection->gattProcedure.handle_range.end_handle = characteristic->handle_decl - 1;
			connection->gattProcedure.discovery_handle = 0;
			sd_ble_gattc_descriptors_discover(conn_handle, &connection->gattProcedure.handle_range);
			goto bail;
		}
		if (characteristic->handle_value > end_handle);
			end_handle = characteristic->handle_value;
	}
	connection->gattProcedure.handle_range.start_handle = end_handle + 1;
	sd_ble_gattc_characteristics_discover(connection->conn_handle, &connection->gattProcedure.handle_range);

bail:
	c_free(gdr);
	xsEndHost(gBLE->the);
}

void gattcDescriptorDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	ret_code_t err_code;
	gattDiscoveryRecord *gdr;
	uint16_t end_handle = 0;

	gdr = (gattDiscoveryRecord *)refcon;
	ble_gattc_evt_desc_disc_rsp_t *desc_disc_rsp = (ble_gattc_evt_desc_disc_rsp_t *)gdr->rsp;
	uint16_t conn_handle = gdr->conn_handle;
	
	if (!gBLE) {
		c_free(gdr);
		return;
	}
	
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
    
	if (BLE_GATT_STATUS_SUCCESS != gdr->gatt_status) {
		xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onDescriptor"));
		goto bail;
	}

	xsmcVars(2);
		
	for (uint16_t i = 0; i < desc_disc_rsp->count; ++i) {
		uint8_t buffer[UUID_LEN_128];
		uint16_t length;
		ble_gattc_desc_t const *descriptor = &desc_disc_rsp->descs[i];
		
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
	
	connection->gattProcedure.handle_range.start_handle = end_handle + 1;
	err_code = sd_ble_gattc_descriptors_discover(conn_handle, &connection->gattProcedure.handle_range);
	if (NRF_SUCCESS != err_code)
		xsCall1(connection->gattProcedure.obj, xsID_callback, xsString("onDescriptor"));
bail:
	c_free(gdr);
	xsEndHost(gBLE->the);
}

void gattcCharacteristicReadEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;
	
	ble_gattc_evt_read_rsp_t const *read_rsp = (ble_gattc_evt_read_rsp_t const *)message;
	uint32_t conn_handle = (uint32_t)refcon;

	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
	
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), (void*)read_rsp->data, read_rsp->len);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSetInteger(xsVar(1), read_rsp->handle);
	xsmcSet(xsVar(0), xsID_handle, xsVar(1));
	xsCall2(connection->objClient, xsID_callback, xsString("onCharacteristicValue"), xsVar(0));
	
	xsEndHost(gBLE->the);
}

static void gattcCharacteristicNotificationEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;
	
	ble_gattc_evt_hvx_t const *hvx = (ble_gattc_evt_hvx_t const *)message;
	uint32_t conn_handle = (uint32_t)refcon;
	
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_handle);
	if (!connection)
		xsUnknownError("connection not found");
	
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), (void*)hvx->data, hvx->len);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSetInteger(xsVar(1), hvx->handle);
	xsmcSet(xsVar(0), xsID_handle, xsVar(1));
	xsCall2(connection->objClient, xsID_callback, xsString("onCharacteristicNotification"), xsVar(0));

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
			if (0 != p_evt_adv_report->data.len)
				modMessagePostToMachine(gBLE->the, (uint8_t*)p_evt_adv_report, sizeof(ble_gap_evt_adv_report_t), gapAdvReportEvent, NULL);
   			break;
    	}
		case BLE_GAP_EVT_AUTH_KEY_REQUEST:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&p_ble_evt->evt.gap_evt, sizeof(ble_gap_evt_t), gapAuthKeyRequestEvent, NULL);
			break;
		case BLE_GAP_EVT_AUTH_STATUS:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&p_ble_evt->evt.gap_evt, sizeof(ble_gap_evt_t), gapAuthStatusEvent, NULL);
			break;
		case BLE_GAP_EVT_CONNECTED:
			if (0xFF != gBLE->iocap)
				pm_handler_secure_on_connection(p_ble_evt);
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
			
		case BLE_GATTC_EVT_CHAR_DISC_RSP: {
			gattDiscoveryRecord *gdr;
			ble_gattc_evt_char_disc_rsp_t const *char_disc_rsp = &p_ble_evt->evt.gattc_evt.params.char_disc_rsp;
			uint16_t len = sizeof(ble_gattc_evt_char_disc_rsp_t) + (char_disc_rsp->count * sizeof(ble_gattc_char_t));
			gdr = c_malloc(len);
			if (NULL != gdr) {
				gdr->conn_handle = p_ble_evt->evt.gattc_evt.conn_handle;
				gdr->gatt_status = p_ble_evt->evt.gattc_evt.gatt_status;	
				c_memmove(gdr->rsp, char_disc_rsp, len);
				
				modBLEConnection connection = modBLEConnectionFindByConnectionID(gdr->conn_handle);
				if (NULL != connection) {
					if (0 != connection->gattProcedure.discovery_handle)
						modMessagePostToMachine(gBLE->the, NULL, 0, gattcCharacteristicsForDescriptorsDiscoveryEvent, (void*)gdr);
					else
						modMessagePostToMachine(gBLE->the, NULL, 0, gattcCharacteristicDiscoveryEvent, (void*)gdr);
				}
			}
			break;
		}
		case BLE_GATTC_EVT_DESC_DISC_RSP: {
			gattDiscoveryRecord *gdr;
			ble_gattc_evt_desc_disc_rsp_t const *desc_disc_rsp = &p_ble_evt->evt.gattc_evt.params.desc_disc_rsp;
			uint16_t len = sizeof(ble_gattc_evt_desc_disc_rsp_t) + (desc_disc_rsp->count * sizeof(ble_gattc_desc_t));
			gdr = c_malloc(len);
			if (NULL != gdr) {
				gdr->conn_handle = p_ble_evt->evt.gattc_evt.conn_handle;
				gdr->gatt_status = p_ble_evt->evt.gattc_evt.gatt_status;	
				c_memmove(gdr->rsp, desc_disc_rsp, len);
				modMessagePostToMachine(gBLE->the, NULL, 0, gattcDescriptorDiscoveryEvent, (void*)gdr);
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
				ble_gattc_evt_hvx_t const *hvx = &p_ble_evt->evt.gattc_evt.params.hvx;
				modMessagePostToMachine(gBLE->the, (uint8_t*)hvx, sizeof(ble_gattc_evt_hvx_t) + hvx->len - 1, gattcCharacteristicNotificationEvent, (void*)(uint32_t)p_ble_evt->evt.gattc_evt.conn_handle);
			}
        	break;
		case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP: {
			gattDiscoveryRecord *gdr;
			ble_gattc_evt_prim_srvc_disc_rsp_t const *prim_srvc_disc_rsp = &p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp;
			uint16_t len = sizeof(ble_gattc_evt_prim_srvc_disc_rsp_t) + (prim_srvc_disc_rsp->count * sizeof(ble_gattc_service_t));
			gdr = c_malloc(len);
			if (NULL != gdr) {
				gdr->conn_handle = p_ble_evt->evt.gattc_evt.conn_handle;
				gdr->gatt_status = p_ble_evt->evt.gattc_evt.gatt_status;	
				c_memmove(gdr->rsp, prim_srvc_disc_rsp, len);
				modMessagePostToMachine(gBLE->the, NULL, 0, gattcServiceDiscoveryEvent, (void*)gdr);
			}
			break;
		}
        case BLE_GATTC_EVT_READ_RSP:
			if (BLE_GATT_STATUS_SUCCESS == p_ble_evt->evt.gattc_evt.gatt_status) {
				ble_gattc_evt_read_rsp_t const *read_rsp = &p_ble_evt->evt.gattc_evt.params.read_rsp;
				modMessagePostToMachine(gBLE->the, (uint8_t*)read_rsp, sizeof(ble_gattc_evt_read_rsp_t) + read_rsp->len - 1, gattcCharacteristicReadEvent, (void*)(uint32_t)p_ble_evt->evt.gattc_evt.conn_handle);
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

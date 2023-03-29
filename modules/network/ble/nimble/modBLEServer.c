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

#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/ble_store.h"
#include "host/ble_uuid.h"
#include "esp_nimble_hci.h"
#include "services/gap/ble_svc_gap.h"
#include "nimble/nimble_port.h"

#include "mc.bleservices.c"

#define DEVICE_FRIENDLY_NAME "Moddable"

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

#define LOG_GATT 0
#if LOG_GATT
	#define LOG_GATT_EVENT(event) logGATTEvent(event)
	#define LOG_GATT_MSG(msg) modLog(msg)
	#define LOG_GATT_INT(i) modLogInt(i)
#else
	#define LOG_GATT_EVENT(event)
	#define LOG_GATT_MSG(msg)
	#define LOG_GATT_INT(i)
#endif

typedef struct {
	xsMachine	*the;
	xsSlot		obj;
	
	// server
	ble_addr_t	bda;
	
	// services
	uint8_t deployServices;
	uint16_t handles[service_count][max_attribute_count];
	
	// requests
	uint8_t requestPending;
	void *requestResult;
	
	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;
	uint8_t iocap;
	
	// connection
	int16_t conn_id;
	ble_addr_t remote_bda;
} modBLERecord, *modBLE;

typedef struct {
	uint16_t conn_id;
	uint16_t handle;
	uint16_t length;
	ble_uuid_any_t uuid;
	uint8_t isCharacteristic;
	uint8_t data[1];
} attributeDataRecord, *attributeData;

typedef struct {
	uint8_t notify;
	uint16_t conn_id;
	uint16_t handle;
	ble_uuid_any_t uuid;
} notificationStateRecord, *notificationState;

typedef struct {
	uint16_t length;
	uint8_t data[1];
} readDataRequestRecord, *readDataRequest;

static void uuidToBuffer(uint8_t *buffer, ble_uuid_any_t *uuid, uint16_t *length);
static const char_name_table *handleToCharName(uint16_t handle);
static const ble_uuid16_t *handleToUUID(uint16_t handle);

static void bleServerCloseEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void bondingRemovedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void logGAPEvent(struct ble_gap_event *event);
static void logGATTEvent(uint8_t op);

static void nimble_on_sync(void);
static void nimble_on_register(struct ble_gatt_register_ctxt *ctxt, void *arg);
static int nimble_gap_event(struct ble_gap_event *event, void *arg);

static modBLE gBLE = NULL;

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
	gBLE->conn_id = -1;
	xsRemember(gBLE->obj);
	
	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_deployServices)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_deployServices);
		gBLE->deployServices = xsmcToBoolean(xsVar(0));
	}
	else
		gBLE->deployServices = true;

	ble_hs_cfg.sync_cb = nimble_on_sync;
	ble_hs_cfg.gatts_register_cb = nimble_on_register;
	ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

	esp_err_t err = modBLEPlatformInitialize();
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
	
	if (-1 != ble->conn_id) {
		modBLEConnection connection = modBLEConnectionFindByConnectionID(ble->conn_id);
		if (NULL != connection)
			modBLEConnectionRemove(connection);
	}
	if (ble->deployServices && (0 != service_count))
		ble_gatts_reset();
	c_free(ble);
	gBLE = NULL;
	
	modBLEPlatformTerminate();
}

void xs_ble_server_disconnect(xsMachine *the)
{
	if (-1 != gBLE->conn_id)
		ble_gap_terminate(gBLE->conn_id, BLE_ERR_REM_USER_CONN_TERM);
}

void xs_ble_server_get_local_address(xsMachine *the)
{
	xsmcSetArrayBuffer(xsResult, gBLE->bda.val, 6);
}

void xs_ble_server_set_device_name(xsMachine *the)
{
	ble_svc_gap_device_name_set(xsmcToString(xsArg(0)));
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
	struct ble_gap_adv_params adv_params;
	
	switch(filterPolicy) {
		case kBLEAdvFilterPolicyWhitelistScans:
			filterPolicy = BLE_HCI_ADV_FILT_SCAN;
			break;
		case kBLEAdvFilterPolicyWhitelistConnections:
			filterPolicy = BLE_HCI_ADV_FILT_CONN;
			break;
		case kBLEAdvFilterPolicyWhitelistScansConnections:
			filterPolicy = BLE_HCI_ADV_FILT_BOTH;
			break;
		default:
			filterPolicy = BLE_HCI_ADV_FILT_NONE;
			break;
	}

	c_memset(&adv_params, 0, sizeof(adv_params));
	adv_params.conn_mode = (flags & (LE_LIMITED_DISCOVERABLE_MODE | LE_GENERAL_DISCOVERABLE_MODE)) ? BLE_GAP_CONN_MODE_UND : BLE_GAP_CONN_MODE_NON;
	adv_params.disc_mode = (flags & LE_GENERAL_DISCOVERABLE_MODE) ? BLE_GAP_DISC_MODE_GEN : (flags & LE_LIMITED_DISCOVERABLE_MODE ? BLE_GAP_DISC_MODE_LTD : BLE_GAP_DISC_MODE_NON);
	adv_params.itvl_min = intervalMin;
	adv_params.itvl_max = intervalMax;
	adv_params.filter_policy = filterPolicy;
	if (NULL != advertisingData)
		ble_gap_adv_set_data(advertisingData, advertisingDataLength);
	if (NULL != scanResponseData)
		ble_gap_adv_rsp_set_data(scanResponseData, scanResponseDataLength);
	ble_gap_adv_start(gBLE->bda.type, NULL, BLE_HS_FOREVER, &adv_params, nimble_gap_event, NULL);
}
	
void xs_ble_server_stop_advertising(xsMachine *the)
{
	ble_gap_adv_stop();
}

void xs_ble_server_characteristic_notify_value(xsMachine *the)
{
	uint16_t handle = xsmcToInteger(xsArg(0));
	uint16_t notify = xsmcToInteger(xsArg(1));
	struct os_mbuf *om;

	om = ble_hs_mbuf_from_flat(xsmcToArrayBuffer(xsArg(2)), xsmcGetArrayBufferLength(xsArg(2)));
	if (notify)
		ble_gattc_notify_custom(gBLE->conn_id, handle, om);
	else
		ble_gattc_indicate_custom(gBLE->conn_id, handle, om);
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
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint32_t passkey = xsmcToInteger(xsArg(1));
	if (0 == c_memcmp(address, gBLE->remote_bda.val, 6)) {
		struct ble_sm_io pkey = {0};
		pkey.action = BLE_SM_IOACT_INPUT;
		pkey.passkey = passkey;
		ble_sm_inject_io(gBLE->conn_id, &pkey);
	}
}

void xs_ble_server_passkey_reply(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	if (0 == c_memcmp(address, gBLE->remote_bda.val, 6)) {
		struct ble_sm_io pkey = {0};
		pkey.action = BLE_SM_IOACT_NUMCMP;
		pkey.numcmp_accept = xsmcToBoolean(xsArg(1));
		ble_sm_inject_io(gBLE->conn_id, &pkey);
	}
}

void xs_ble_server_get_service_attributes(xsMachine *the)
{
	// @@ TBD
}

void xs_ble_server_deploy(xsMachine *the)
{
	// services deployed from readyEvent(), since stack requires deploy before advertising
}

static void deployServices(xsMachine *the)
{
	static const ble_uuid16_t BT_UUID_GAP = BLE_UUID16_INIT(0x1800);
	static const ble_uuid16_t BT_UUID_GAP_DEVICE_NAME = BLE_UUID16_INIT(0x2A00);
	static const ble_uuid16_t BT_UUID_GAP_APPEARANCE = BLE_UUID16_INIT(0x2A01);
	char *device_name = NULL;
	uint16_t appearance = BLE_SVC_GAP_APPEARANCE_GEN_COMPUTER;
	struct ble_gatt_access_ctxt ctxt = {0};
	struct ble_gatt_chr_def chr_def = {0};

	if (0 == service_count)
		return;
	
	ctxt.chr = &chr_def;
    
	// Set device name and appearance from app GAP service when available
    
	for (int service_index = 0; service_index < service_count; ++service_index) {
		const struct ble_gatt_svc_def *service = &gatt_svr_svcs[service_index];
		if (0 == ble_uuid_cmp((const ble_uuid_t*)service->uuid, &BT_UUID_GAP.u)) {
			int index = 0;
			const struct ble_gatt_chr_def *characteristic = &service->characteristics[index];
			while (characteristic->uuid != 0) {
				if (0 == ble_uuid_cmp((const ble_uuid_t*)characteristic->uuid, &BT_UUID_GAP_DEVICE_NAME.u)) {
					chr_def.uuid = &BT_UUID_GAP_DEVICE_NAME.u;
					ctxt.om = os_msys_get_pkthdr(32, 0);
					if (0 == gatt_svr_chr_static_value_access_cb(0, 0, &ctxt, NULL)) {
						device_name = c_calloc(1, ctxt.om->om_len + 1);
						if (NULL == device_name)
							xsUnknownError("no memory");
						c_memmove(device_name, ctxt.om->om_data, ctxt.om->om_len);
					}
					os_mbuf_free_chain(ctxt.om);
				}
				if (0 == ble_uuid_cmp((const ble_uuid_t*)characteristic->uuid, &BT_UUID_GAP_APPEARANCE.u)) {
					chr_def.uuid = &BT_UUID_GAP_APPEARANCE.u;
					ctxt.om = os_msys_get_pkthdr(2, 0);
					if (0 == gatt_svr_chr_static_value_access_cb(0, 0, &ctxt, NULL)) {
						c_memmove(&appearance, ctxt.om->om_data, ctxt.om->om_len);
					}
					os_mbuf_free_chain(ctxt.om);
				}
				characteristic = &service->characteristics[++index];
			}
			break;
		}
	}
	
	int rc = ble_gatts_reset();

	if (0 == rc)
		rc = ble_gatts_count_cfg(gatt_svr_svcs);
	if (0 == rc)
		rc = ble_gatts_add_svcs(gatt_svr_svcs);
	if (0 == rc)
		rc = ble_gatts_start();
	if (0 == rc) {
		ble_svc_gap_device_appearance_set(appearance);
		if (NULL != device_name) {
			ble_svc_gap_device_name_set(device_name);
			c_free(device_name);
		}
		else
			ble_svc_gap_device_name_set(DEVICE_FRIENDLY_NAME);
	}
	
	if (0 != rc)
		xsUnknownError("failed to start services");
}

void modBLEServerBondingRemoved(char *address, uint8_t addressType)
{
	ble_addr_t addr;
	
	if (!gBLE) return;
	
	addr.type = addressType;
	c_memmove(addr.val, address, 6);
	modMessagePostToMachine(gBLE->the, (void*)&addr, sizeof(addr), bondingRemovedEvent, NULL);
}

static void readyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE) return;
	
	ble_hs_util_ensure_addr(0);
	ble_hs_id_infer_auto(0, &gBLE->bda.type);
	ble_hs_id_copy_addr(gBLE->bda.type, gBLE->bda.val, NULL);

	if (gBLE->deployServices)
		deployServices(the);

	xsBeginHost(gBLE->the);
	xsCall1(gBLE->obj, xsID_callback, xsStringX("onReady"));
	xsEndHost(gBLE->the);
}

static void connectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_conn_desc *desc = (struct ble_gap_conn_desc *)message;
	
	if (!gBLE) return;
	
	xsBeginHost(gBLE->the);		
	if (-1 != desc->conn_handle) {
		if (-1 != gBLE->conn_id) {
			LOG_GAP_MSG("Ignoring duplicate connect event");
			goto bail;
		}
		gBLE->conn_id = desc->conn_handle;
		gBLE->remote_bda = desc->peer_id_addr;
		
		modBLEConnection connection = c_calloc(sizeof(modBLEConnectionRecord), 1);
		if (!connection)
			xsUnknownError("out of memory");
			
		connection->id = gBLE->conn_id;
		connection->type = kBLEConnectionTypeServer;
		connection->addressType = gBLE->remote_bda.type;
		c_memmove(connection->address, gBLE->remote_bda.val, 6);
		modBLEConnectionAdd(connection);
	}
	else {
		LOG_GAP_MSG("BLE_GAP_EVENT_CONNECT failed");
	}
	xsmcVars(2);
	xsmcSetNewObject(xsVar(0));
	xsmcSetInteger(xsVar(1), desc->conn_handle);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), desc->peer_id_addr.val, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), desc->peer_id_addr.type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, -1 != desc->conn_handle ? xsStringX("onConnected") : xsStringX("onDisconnected"), xsVar(0));
	
bail:
	xsEndHost(gBLE->the);
}

static void disconnectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_conn_desc *desc = (struct ble_gap_conn_desc *)message;

	if (!gBLE) return;
	
	xsBeginHost(gBLE->the);
	
	// ignore multiple disconnects on same connection
	if (-1 == gBLE->conn_id) {
		LOG_GAP_MSG("Ignoring duplicate disconnect event");
		goto bail;
	}	
	
	modBLEConnection connection = modBLEConnectionFindByConnectionID(gBLE->conn_id);
	if (NULL != connection)
		modBLEConnectionRemove(connection);

	gBLE->conn_id = -1;
	xsmcVars(2);
	xsmcSetNewObject(xsVar(0));
	xsmcSetInteger(xsVar(1), desc->conn_handle);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), desc->peer_id_addr.val, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), desc->peer_id_addr.type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsStringX("onDisconnected"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

static void writeEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	attributeData value = (attributeData)refcon;
	uint8_t buffer[16];
	uint16_t length;
	const char_name_table *char_name;
	
	if (!gBLE) goto bail;

	char_name = handleToCharName(value->handle);

	xsBeginHost(gBLE->the);

	xsmcVars(4);
	xsmcSetNewObject(xsVar(0));
	uuidToBuffer(buffer, &value->uuid, &length);
	xsmcSetArrayBuffer(xsVar(1), buffer, length);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	if (char_name) {
		xsmcSetString(xsVar(2), (char*)char_name->name);
		xsmcSet(xsVar(0), xsID_name, xsVar(2));
		xsmcSetString(xsVar(3), (char*)char_name->type);
		xsmcSet(xsVar(0), xsID_type, xsVar(3));
	}
	xsmcSetInteger(xsVar(2), value->handle);
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsmcSetArrayBuffer(xsVar(3), value->data, value->length);
	xsmcSet(xsVar(0), xsID_value, xsVar(3));
	xsCall2(gBLE->obj, xsID_callback, xsStringX("onCharacteristicWritten"), xsVar(0));

	xsEndHost(gBLE->the);
	
bail:
	c_free(value);
}

static void readEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gatt_access_ctxt *ctxt = (struct ble_gatt_access_ctxt *)refcon;
	const struct ble_gatt_chr_def *chr = (const struct ble_gatt_chr_def *)ctxt->chr;
	const ble_uuid_t *uuid = chr->uuid;
	const char_name_table *char_name;
	uint8_t buffer[16];
	uint16_t uuid_length;
	
	if (!gBLE) return;

	char_name = handleToCharName(*chr->val_handle);
	uuidToBuffer(buffer, (ble_uuid_any_t *)chr->uuid, &uuid_length);

	xsBeginHost(gBLE->the);
	xsmcVars(5);

	xsmcSetNewObject(xsVar(0));
	xsmcSetArrayBuffer(xsVar(1), buffer, uuid_length);
	xsmcSetInteger(xsVar(2), *chr->val_handle);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	if (char_name) {
		xsmcSetString(xsVar(3), (char*)char_name->name);
		xsmcSet(xsVar(0), xsID_name, xsVar(3));
		xsmcSetString(xsVar(4), (char*)char_name->type);
		xsmcSet(xsVar(0), xsID_type, xsVar(4));
	}
	xsResult = xsCall2(gBLE->obj, xsID_callback, xsStringX("onCharacteristicRead"), xsVar(0));
	if (xsUndefinedType != xsmcTypeOf(xsResult)) {
		readDataRequest data = c_malloc(sizeof(readDataRequestRecord) + xsmcGetArrayBufferLength(xsResult));
		if (NULL != data) {
			data->length = xsmcGetArrayBufferLength(xsResult);
			c_memmove(data->data, xsmcToArrayBuffer(xsResult), data->length);
			gBLE->requestResult = data;
		}
	}

	xsEndHost(gBLE->the);
	gBLE->requestPending = false;
}

static void notificationStateEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	notificationState state = (notificationState)refcon;

	if (!gBLE) goto bail;

	if (state->conn_id != gBLE->conn_id)
		goto bail;
		
	xsBeginHost(gBLE->the);
	uint16_t uuid_length;
	uint8_t buffer[16];
	char_name_table *char_name = (char_name_table *)handleToCharName(state->handle);
	const ble_uuid16_t *uuid = handleToUUID(state->handle);
	
	xsmcVars(6);
	xsmcSetNewObject(xsVar(0));
	uuidToBuffer(buffer, (ble_uuid_any_t *)uuid, &uuid_length);
	xsmcSetArrayBuffer(xsVar(1), buffer, uuid_length);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	if (char_name) {
		xsmcSetString(xsVar(2), (char*)char_name->name);
		xsmcSet(xsVar(0), xsID_name, xsVar(2));
		xsmcSetString(xsVar(3), (char*)char_name->type);
		xsmcSet(xsVar(0), xsID_type, xsVar(3));
	}
	xsmcSetInteger(xsVar(4), state->handle);
	xsmcSetInteger(xsVar(5), state->notify);
	xsmcSet(xsVar(0), xsID_handle, xsVar(4));
	xsmcSet(xsVar(0), xsID_notify, xsVar(5));
	xsCall2(gBLE->obj, xsID_callback, xsStringX(state->notify ? "onCharacteristicNotifyEnabled" : "onCharacteristicNotifyDisabled"), xsVar(0));
	xsEndHost(gBLE->the);

bail:
	c_free(state);
}

static void passkeyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_event *event = (struct ble_gap_event *)message;
	struct ble_sm_io pkey = {0};
	
	if (!gBLE) return;
	
	pkey.action = event->passkey.params.action;
	
	if (event->passkey.conn_handle != gBLE->conn_id)
		xsUnknownError("connection not found");
		
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	xsmcSetNewObject(xsVar(0));

    if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
    	pkey.passkey = (c_rand() % 999999) + 1;
		xsmcSetArrayBuffer(xsVar(1), gBLE->remote_bda.val, 6);
		xsmcSetInteger(xsVar(2), pkey.passkey);
		xsmcSet(xsVar(0), xsID_address, xsVar(1));
		xsmcSet(xsVar(0), xsID_passkey, xsVar(2));
		xsCall2(gBLE->obj, xsID_callback, xsStringX("onPasskeyDisplay"), xsVar(0));
		ble_sm_inject_io(event->passkey.conn_handle, &pkey);
	}
    else if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
		xsmcSetArrayBuffer(xsVar(1), gBLE->remote_bda.val, 6);
		xsmcSet(xsVar(0), xsID_address, xsVar(1));
		if (gBLE->iocap == KeyboardOnly)
			xsCall2(gBLE->obj, xsID_callback, xsStringX("onPasskeyInput"), xsVar(0));
		else {
			xsResult = xsCall2(gBLE->obj, xsID_callback, xsStringX("onPasskeyRequested"), xsVar(0));
			pkey.passkey = xsmcToInteger(xsResult);
			ble_sm_inject_io(event->passkey.conn_handle, &pkey);
		}
	}
	else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
		xsmcSetArrayBuffer(xsVar(1), gBLE->remote_bda.val, 6);
		xsmcSetInteger(xsVar(2), event->passkey.params.numcmp);
		xsmcSet(xsVar(0), xsID_address, xsVar(1));
		xsmcSet(xsVar(0), xsID_passkey, xsVar(2));
		xsCall2(gBLE->obj, xsID_callback, xsStringX("onPasskeyConfirm"), xsVar(0));
	}
	
bail:
	xsEndHost(gBLE->the);
}

static void bleServerCloseEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modBLE ble = refcon;

	xs_ble_server_destructor(ble);
}

static void encryptionChangeEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_event *event = (struct ble_gap_event *)message;
	
	if (!gBLE)
		return;
		
		
	LOG_GAP_MSG("Encryption change status=");
	LOG_GAP_INT(event->enc_change.status);
		
	if (0 == event->enc_change.status) {
		struct ble_gap_conn_desc desc;
        int rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        if (0 == rc) {
        	if (desc.sec_state.encrypted) {
				xsBeginHost(gBLE->the);
				xsmcVars(2);
				xsmcSetNewObject(xsVar(0));
				xsmcSetBoolean(xsVar(1), desc.sec_state.bonded);
				xsmcSet(xsVar(0), xsID_bonded, xsVar(1));
				xsCall2(gBLE->obj, xsID_callback, xsStringX("onAuthenticated"), xsVar(0));
				xsEndHost(gBLE->the);
        	}
        }
	}
}

static void mtuExchangedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_event *event = (struct ble_gap_event *)message;
	
	if (!gBLE)
		return;

	if (event->mtu.conn_handle != gBLE->conn_id)
		return;
		
	xsBeginHost(gBLE->the);
	xsmcSetInteger(xsResult, event->mtu.value);
	xsCall2(gBLE->obj, xsID_callback, xsStringX("onMTUExchanged"), xsResult);
	xsEndHost(gBLE->the);
}

void bondingRemovedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	ble_addr_t *addr = (ble_addr_t *)message;

	if (!gBLE) return;

	xsBeginHost(gBLE->the);
	xsmcSetNewObject(xsVar(0));
	xsmcSetArrayBuffer(xsResult, addr->val, 6);
	xsmcSet(xsVar(0), xsID_address, xsResult);
	xsmcSetInteger(xsResult, addr->type);
	xsmcSet(xsVar(0), xsID_addressType, xsResult);
	xsCall2(gBLE->obj, xsID_callback, xsStringX("onBondingDeleted"), xsVar(0));
	xsEndHost(gBLE->the);
}

void ble_server_on_reset(int reason)
{
	if (gBLE)
		xs_ble_server_close(gBLE->the);
}

void nimble_on_sync(void)
{
	if (!gBLE) return;
	
	ble_hs_util_ensure_addr(0);
	modMessagePostToMachine(gBLE->the, NULL, 0, readyEvent, NULL);
}

static void nimble_on_register(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
	int service_index, att_index, char_index, dsc_index;
	
	if (!gBLE) return;

	switch (ctxt->op) {
		case BLE_GATT_REGISTER_OP_CHR: 
		case BLE_GATT_REGISTER_OP_DSC: {
			const struct ble_gatt_svc_def *svc_def = (ctxt->op == BLE_GATT_REGISTER_OP_CHR) ? ctxt->chr.svc_def : ctxt->dsc.svc_def;
			const struct ble_gatt_chr_def *chr_def = ctxt->chr.chr_def;
			const struct ble_gatt_dsc_def *dsc_def = ctxt->dsc.dsc_def;

			for (service_index = 0; service_index < service_count; ++service_index) {
				const struct ble_gatt_svc_def *service = &gatt_svr_svcs[service_index];
				if (0 == ble_uuid_cmp((const ble_uuid_t*)svc_def->uuid, (const ble_uuid_t*)service->uuid)) {
					att_index = 0;
					char_index = 0;
					const struct ble_gatt_chr_def *characteristic = &service->characteristics[char_index];
					while (characteristic->uuid != 0) {
						if (ctxt->op == BLE_GATT_REGISTER_OP_CHR && chr_def == characteristic) {
							gBLE->handles[service_index][att_index] = ctxt->chr.val_handle;
							return;
						}
						++att_index;

						if (characteristic->descriptors != NULL) {
							dsc_index = 0;
							const struct ble_gatt_dsc_def *descriptor = &characteristic->descriptors[dsc_index];
							while (descriptor->uuid != 0) {
								if (ctxt->op == BLE_GATT_REGISTER_OP_DSC && dsc_def == descriptor) {
									gBLE->handles[service_index][att_index] = ctxt->dsc.handle;
									return;
								}
								descriptor = &characteristic->descriptors[++dsc_index];
								++att_index;
							}
						}
						characteristic = &service->characteristics[++char_index];
					}
				}
			}
			break;
		}
		default:
			break;
	}
}

static int nimble_gap_event(struct ble_gap_event *event, void *arg)
{
    int rc = 0;
	struct ble_gap_conn_desc desc;

	LOG_GAP_EVENT(event);

	if (!gBLE) goto bail;

    switch (event->type) {
		case BLE_GAP_EVENT_CONNECT:
			if (event->connect.status == 0) {
				if (0 == ble_gap_conn_find(event->connect.conn_handle, &desc)) {
					if (gBLE->mitm || gBLE->encryption)
						ble_gap_security_initiate(desc.conn_handle);
					modMessagePostToMachine(gBLE->the, (uint8_t*)&desc, sizeof(desc), connectEvent, NULL);
					goto bail;
				}
			}
			c_memset(&desc, 0, sizeof(desc));
			desc.conn_handle = -1;
			modMessagePostToMachine(gBLE->the, (uint8_t*)&desc, sizeof(desc), connectEvent, NULL);
			break;
		case BLE_GAP_EVENT_DISCONNECT:
			modMessagePostToMachine(gBLE->the, (uint8_t*)&event->disconnect.conn, sizeof(event->disconnect.conn), disconnectEvent, NULL);
			break;
		case BLE_GAP_EVENT_SUBSCRIBE: {
			notificationState state = c_malloc(sizeof(notificationStateRecord));
			if (NULL != state) {
				state->notify = event->subscribe.cur_notify || event->subscribe.cur_indicate;
				state->conn_id = event->subscribe.conn_handle;
				state->handle = event->subscribe.attr_handle;
				modMessagePostToMachine(gBLE->the, NULL, 0, notificationStateEvent, state);
			}
			break;
		}
		case BLE_GAP_EVENT_ENC_CHANGE:
			modMessagePostToMachine(gBLE->the, (uint8_t*)event, sizeof(struct ble_gap_event), encryptionChangeEvent, NULL);
			break;
		case BLE_GAP_EVENT_REPEAT_PAIRING:
			// delete old bond and accept new link
			if (0 == ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc))
				ble_store_util_delete_peer(&desc.peer_id_addr);
			return BLE_GAP_REPEAT_PAIRING_RETRY;
			break;
		case BLE_GAP_EVENT_PASSKEY_ACTION:
			modMessagePostToMachine(gBLE->the, (uint8_t*)event, sizeof(struct ble_gap_event), passkeyEvent, NULL);
			break;
		case BLE_GAP_EVENT_MTU:
			modMessagePostToMachine(gBLE->the, (uint8_t*)event, sizeof(struct ble_gap_event), mtuExchangedEvent, NULL);
			break;
		default:
			break;
    }
    
bail:
	return rc;
}

void uuidToBuffer(uint8_t *buffer, ble_uuid_any_t *uuid, uint16_t *length)
{
	if (uuid->u.type == BLE_UUID_TYPE_16) {
		*length = 2;
		buffer[0] = uuid->u16.value & 0xFF;
		buffer[1] = (uuid->u16.value >> 8) & 0xFF;
	}
	else if (uuid->u.type == BLE_UUID_TYPE_32) {
		*length = 4;
		buffer[0] = uuid->u32.value & 0xFF;
		buffer[1] = (uuid->u32.value >> 8) & 0xFF;
		buffer[2] = (uuid->u32.value >> 16) & 0xFF;
		buffer[3] = (uuid->u32.value >> 24) & 0xFF;
	}
	else {
		*length = 16;
		c_memmove(buffer, uuid->u128.value, *length);
	}
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

const ble_uuid16_t *handleToUUID(uint16_t handle) {
	for (int service_index = 0; service_index < service_count; ++service_index) {
		const struct ble_gatt_svc_def *service = &gatt_svr_svcs[service_index];
		int index = 0;
		const struct ble_gatt_chr_def *chr_def = &service->characteristics[index];
		while (chr_def->uuid != 0) {
			if (handle == *chr_def->val_handle)
				return (ble_uuid16_t *)chr_def->uuid;
			chr_def = &service->characteristics[++index];
		}
	}
	return NULL;
}

int gatt_svr_chr_dynamic_value_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	LOG_GATT_EVENT(ctxt->op);
	
	if (!gBLE) goto bail;

	switch (ctxt->op) {
		case BLE_GATT_ACCESS_OP_READ_DSC:
        case BLE_GATT_ACCESS_OP_READ_CHR: {
        	// The read request must be satisfied from this task.
        	gBLE->requestPending = true;
        	gBLE->requestResult = NULL;
#if !USE_EVENT_TIMER
        	modMessagePostToMachine(gBLE->the, NULL, 0, readEvent, (void*)ctxt);
        	while (gBLE->requestPending) {
        		modDelayMilliseconds(5);
        	}
#else
			readEvent(gBLE->the, ctxt, NULL, 0);
#endif
        	if (NULL == gBLE->requestResult)
        		return BLE_ATT_ERR_INSUFFICIENT_RES;
        	else {
        		readDataRequest data = (readDataRequest)gBLE->requestResult;
        		os_mbuf_append(ctxt->om, data->data, data->length);
        		c_free(data);
        	}
        	return 0;
        	break;
        }
        case BLE_GATT_ACCESS_OP_WRITE_DSC:
		case BLE_GATT_ACCESS_OP_WRITE_CHR: {
			uint8_t *data;
			uint16_t length = sizeof(attributeDataRecord) + ctxt->om->om_len;
			attributeData value = c_malloc(length);
			if (NULL != value) {
				value->isCharacteristic = (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR);
				value->conn_id = conn_handle;
				value->handle = attr_handle;
				value->length = ctxt->om->om_len;
				c_memmove(value->data, ctxt->om->om_data, ctxt->om->om_len);
				ble_uuid_copy(&value->uuid, ctxt->chr->uuid);
				modMessagePostToMachine(gBLE->the, NULL, 0, writeEvent, (void*)value);
			}
			break;
		}
	}

bail:
	return 0;
}

void logGAPEvent(struct ble_gap_event *event) {
	switch(event->type) {
		case BLE_GAP_EVENT_CONNECT: modLog("BLE_GAP_EVENT_CONNECT"); break;
		case BLE_GAP_EVENT_DISCONNECT: modLog("BLE_GAP_EVENT_DISCONNECT"); break;
		case BLE_GAP_EVENT_CONN_UPDATE: modLog("BLE_GAP_EVENT_CONN_UPDATE"); break;
		case BLE_GAP_EVENT_CONN_UPDATE_REQ: modLog("BLE_GAP_EVENT_CONN_UPDATE_REQ"); break;
		case BLE_GAP_EVENT_L2CAP_UPDATE_REQ: modLog("BLE_GAP_EVENT_L2CAP_UPDATE_REQ"); break;
		case BLE_GAP_EVENT_TERM_FAILURE: modLog("BLE_GAP_EVENT_TERM_FAILURE"); break;
		case BLE_GAP_EVENT_DISC: modLog("BLE_GAP_EVENT_DISC"); break;
		case BLE_GAP_EVENT_DISC_COMPLETE: modLog("BLE_GAP_EVENT_DISC_COMPLETE"); break;
		case BLE_GAP_EVENT_ADV_COMPLETE: modLog("BLE_GAP_EVENT_ADV_COMPLETE"); break;
		case BLE_GAP_EVENT_ENC_CHANGE: modLog("BLE_GAP_EVENT_ENC_CHANGE"); break;
		case BLE_GAP_EVENT_PASSKEY_ACTION: modLog("BLE_GAP_EVENT_PASSKEY_ACTION"); break;
		case BLE_GAP_EVENT_NOTIFY_RX: modLog("BLE_GAP_EVENT_NOTIFY_RX"); break;
		case BLE_GAP_EVENT_NOTIFY_TX: modLog("BLE_GAP_EVENT_NOTIFY_TX"); break;
		case BLE_GAP_EVENT_SUBSCRIBE: modLog("BLE_GAP_EVENT_SUBSCRIBE"); break;
		case BLE_GAP_EVENT_MTU: modLog("BLE_GAP_EVENT_MTU"); break;
		case BLE_GAP_EVENT_IDENTITY_RESOLVED: modLog("BLE_GAP_EVENT_IDENTITY_RESOLVED"); break;
		case BLE_GAP_EVENT_REPEAT_PAIRING: modLog("BLE_GAP_EVENT_REPEAT_PAIRING"); break;
		case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE: modLog("BLE_GAP_EVENT_PHY_UPDATE_COMPLETE"); break;
		case BLE_GAP_EVENT_EXT_DISC: modLog("BLE_GAP_EVENT_EXT_DISC"); break;
	}
}

void logGATTEvent(uint8_t op)
{
	switch(op) {
		case BLE_GATT_ACCESS_OP_READ_CHR: modLog("BLE_GATT_ACCESS_OP_READ_CHR"); break;
		case BLE_GATT_ACCESS_OP_WRITE_CHR: modLog("BLE_GATT_ACCESS_OP_WRITE_CHR"); break;
		case BLE_GATT_ACCESS_OP_READ_DSC: modLog("BLE_GATT_ACCESS_OP_READ_DSC"); break;
		case BLE_GATT_ACCESS_OP_WRITE_DSC: modLog("BLE_GATT_ACCESS_OP_WRITE_DSC"); break;
	}
}

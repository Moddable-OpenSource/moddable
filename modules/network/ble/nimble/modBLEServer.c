/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
#include "modBLE.h"
#include "modBLECommon.h"

#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "esp_nimble_hci.h"
#include "services/gap/ble_svc_gap.h"

//#include "mc.bleservices.c"

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

typedef struct {
	xsMachine	*the;
	xsSlot		obj;
	
	// server
	ble_addr_t	bda;
	
	// services
	
	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;
	
	// connection
	int16_t conn_id;
	ble_addr_t remote_bda;
	uint8_t terminating;
} modBLERecord, *modBLE;

static void uuidToBuffer(uint8_t *buffer, ble_uuid_any_t *uuid, uint16_t *length);

static void logGAPEvent(struct ble_gap_event *event);

static void nimble_host_task(void *param);
static void ble_host_task(void *param);
static void nimble_on_reset(int reason);
static void nimble_on_sync(void);

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
	
	esp_err_t err = modBLEPlatformInitialize();
	if (ESP_OK != err)
		xsUnknownError("ble initialization failed");

	ble_hs_cfg.reset_cb = nimble_on_reset;
	ble_hs_cfg.sync_cb = nimble_on_sync;

	ble_svc_gap_init();
	ble_svc_gatt_init();

	ble_svc_gap_device_name_set(DEVICE_FRIENDLY_NAME);
	ble_svc_gap_device_appearance_set(BLE_SVC_GAP_APPEARANCE_GEN_COMPUTER);

	nimble_port_freertos_init(ble_host_task);
}

void xs_ble_server_close(xsMachine *the)
{
	modBLE *ble = xsmcGetHostData(xsThis);
	if (!ble) return;
	
	xsForget(gBLE->obj);
	xs_ble_server_destructor(gBLE);
	xsmcSetHostData(xsThis, NULL);
}

void xs_ble_server_destructor(void *data)
{
	modBLE ble = data;
	if (!ble) return;
	
	ble->terminating = true;
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
	xsmcSetArrayBuffer(xsResult, (void*)gBLE->bda.val, 6);
}

void xs_ble_server_set_device_name(xsMachine *the)
{
	ble_svc_gap_device_name_set(xsmcToString(xsArg(0)));
}

void xs_ble_server_start_advertising(xsMachine *the)
{
	uint32_t intervalMin = xsmcToInteger(xsArg(0));
	uint32_t intervalMax = xsmcToInteger(xsArg(1));
	uint8_t *advertisingData = (uint8_t*)xsmcToArrayBuffer(xsArg(2));
	uint32_t advertisingDataLength = xsGetArrayBufferLength(xsArg(2));
	uint8_t *scanResponseData = xsmcTest(xsArg(3)) ? (uint8_t*)xsmcToArrayBuffer(xsArg(3)) : NULL;
	uint32_t scanResponseDataLength = xsmcTest(xsArg(3)) ? xsGetArrayBufferLength(xsArg(3)) : 0;	
	struct ble_gap_adv_params adv_params;
	
	c_memset(&adv_params, 0, sizeof(adv_params));
	adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
	adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
	adv_params.itvl_min = intervalMin;
	adv_params.itvl_max = intervalMax;
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
}

void xs_ble_server_set_security_parameters(xsMachine *the)
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

void xs_ble_server_passkey_reply(xsMachine *the)
{
}

void xs_ble_server_deploy(xsMachine *the)
{
}

static void readyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	char *device_name = NULL;

	ble_hs_id_infer_auto(0, &gBLE->bda.type);
	ble_hs_id_copy_addr(gBLE->bda.type, gBLE->bda.val, NULL);

	// Set device name and appearance from app GAP service when available

	xsBeginHost(gBLE->the);
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
	xsEndHost(gBLE->the);
}

static void connectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_conn_desc *desc = (struct ble_gap_conn_desc *)message;
	xsBeginHost(gBLE->the);		
	if (-1 != desc->conn_handle) {
		if (-1 != gBLE->conn_id) {
			LOG_GAP_MSG("Ignoring duplicate connect event");
			goto bail;
		}
		gBLE->conn_id = desc->conn_handle;
		gBLE->remote_bda = desc->peer_id_addr;
		xsmcVars(3);
		xsVar(0) = xsmcNewObject();
		xsmcSetInteger(xsVar(1), desc->conn_handle);
		xsmcSet(xsVar(0), xsID_connection, xsVar(1));
		xsmcSetArrayBuffer(xsVar(2), &desc->peer_id_addr.val, 6);
		xsmcSet(xsVar(0), xsID_address, xsVar(2));
		xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));
	}
	else {
		LOG_GAP_MSG("BLE_GAP_EVENT_CONNECT failed");
		xsCall1(gBLE->obj, xsID_callback, xsString("onDisconnected"));
	}
bail:
	xsEndHost(gBLE->the);
}

static void disconnectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	struct ble_gap_conn_desc *desc = (struct ble_gap_conn_desc *)message;
	xsBeginHost(gBLE->the);
	
	// ignore multiple disconnects on same connection
	if (-1 == gBLE->conn_id) {
		LOG_GAP_MSG("Ignoring duplicate disconnect event");
		goto bail;
	}	
	
	gBLE->conn_id = -1;
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), desc->conn_handle);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), desc->peer_id_addr.val, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onDisconnected"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

void nimble_host_task(void *param)
{
	nimble_port_run();
}

void ble_host_task(void *param)
{
	nimble_host_task(param);
}

void nimble_on_reset(int reason)
{
	// fatal controller reset - all connections have been closed
	if (gBLE)
		xs_ble_server_close(gBLE->the);
}

void nimble_on_sync(void)
{
	ble_hs_util_ensure_addr(0);
	modMessagePostToMachine(gBLE->the, NULL, 0, readyEvent, NULL);
}

static int nimble_gap_event(struct ble_gap_event *event, void *arg)
{
    int rc = 0;
	struct ble_gap_conn_desc desc;

	LOG_GAP_EVENT(event);

	if (!gBLE || gBLE->terminating)
		goto bail;

    switch (event->type) {
		case BLE_GAP_EVENT_CONNECT:
			if (event->connect.status == 0) {
				if (0 == ble_gap_conn_find(event->connect.conn_handle, &desc)) {
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

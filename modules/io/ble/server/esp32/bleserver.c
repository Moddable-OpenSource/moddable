/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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


/*
	To do:
	
		- onWarning
		- received needed for onRead / onWrite descriptor?

*/

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"

#include "builtinCommon.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "nimble/ble.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "esp_nimble_hci.h"
#include "ble.h"
#include "ble_hs_hci_priv.h"

static void xs_ble_ready(void);
static void xs_ble_nimble_task(void *param);
static void nimble_on_register(struct ble_gatt_register_ctxt *ctxt, void *arg);
static void nimble_on_reset(int reason);

static uint8_t gNimBLEInititalized = 0;

struct BLEGATTServerConnectionRecord {
	struct BLEGATTServerConnectionRecord	*next;
	struct BLEServerRecord	*server;
	xsSlot	*obj;
	uint16_t conn_handle;
	uint16_t maximumWrite;
};
typedef struct BLEGATTServerConnectionRecord BLEGATTServerConnectionRecord;
typedef struct BLEGATTServerConnectionRecord *BLEGATTServerConnection;

struct BLEGATTServerNotificationRecord {
	struct BLEGATTServerNotificationRecord	*next;
	struct BLEServerRecord *server;
	struct BLEGATTServerCharacteristicRecord *gsc;
	xsSlot *callback;
	uint16_t conn_handle;
	struct os_mbuf *om;
	int error;
};
typedef struct BLEGATTServerNotificationRecord BLEGATTServerNotificationRecord;
typedef struct BLEGATTServerNotificationRecord *BLEGATTServerNotification;

struct BLEGATTServerDescriptorRecord {
	ble_uuid_any_t	uuid;

	uint8_t			byteLength;		// 0 if using callbacks, 8 or less is embedded data, more than 8 is dataPtr
	union {
		uint8_t	bytes[2 * sizeof(xsSlot *)];
		uint8_t	*bytesPtr;
		struct {
			xsSlot	*onRead;
			xsSlot	*onWrite;
		} callbacks;
	};

	uint16_t		handle;
};
typedef struct BLEGATTServerDescriptorRecord BLEGATTServerDescriptorRecord;
typedef struct BLEGATTServerDescriptorRecord *BLEGATTServerDescriptor;

struct BLEGATTServerCharacteristicRecord {
	ble_uuid_any_t	uuid;

	uint16_t		byteLength;		// 0 if using callbacks, 16 or less is embedded data, more than 8 is dataPtr
	union {
		uint8_t	bytes[4 * sizeof(xsSlot *)];
		uint8_t	*bytesPtr;
		struct {
			xsSlot	*onRead;
			xsSlot	*onWrite;
			xsSlot	*onSubscribe;
			xsSlot	*onUnsubscribe;
			xsSlot	*obj;
		} callbacks;
	};

	uint16_t		valueHandle;

	int				properties;
	uint8_t			notify:1;
	uint8_t			indicate:1;

	int				descriptorsLength;
	BLEGATTServerDescriptor	descriptors;
};
typedef struct BLEGATTServerCharacteristicRecord BLEGATTServerCharacteristicRecord;
typedef struct BLEGATTServerCharacteristicRecord *BLEGATTServerCharacteristic;

struct BLEGATTServerServiceRecord {
	struct BLEGATTServerServiceRecord	*next;

	ble_uuid_any_t					uuid;

	struct ble_gatt_svc_def 		svc;
	uint32_t						ZERO;		// terminates svc list

	int									characteristicsLength;
	BLEGATTServerCharacteristicRecord	characteristics[];

};
typedef struct BLEGATTServerServiceRecord BLEGATTServerServiceRecord;
typedef struct BLEGATTServerServiceRecord *BLEGATTServerService;

struct BLEServerRecord {
	xsMachine	*the;
	xsSlot		obj;
	SemaphoreHandle_t 	mutex;

	BLEGATTServerService	services;

	xsSlot		*onReady;
	xsSlot		*onConnect;
	xsSlot		*onDisconnect;
	
	xsSlot		*onPasskey;
	xsSlot		*onSecured;

	xsSlot		*connectionPrototype;
	xsSlot		*characteristicPrototype;

	BLEGATTServerConnection	connections;
	BLEGATTServerNotification notifications;

	uint8_t		ready:1;
	uint8_t		advertising:1;
	uint8_t		addressType;
	uint8_t		secure;
	uint8_t		immediate;
	uint16_t	mtu;
};
typedef struct BLEServerRecord BLEServerRecord;
typedef struct BLEServerRecord *BLEServer;

static BLEServer gServer = C_NULL;

static int accessCharacteristic(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int handleGAPEvent(struct ble_gap_event *event, void *arg);

void xs_gattserver_destructor(void *data)
{
	BLEServer server = data;
	if (!server) return;

	while (server->services) {
		BLEGATTServerService service = server->services;
		BLEGATTServerService next = service->next;

		if (service->svc.characteristics) {
			//@@ descriptors
			//@@ dataPtr of characteristics and descriptors
			c_free((void *)service->svc.characteristics);
		}

		//@@
		c_free(service);
		server->services = next;
	}

	//@@ notifications
	//@@ connections

//@@
	c_free(server);
}

static void xs_gattserver_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	BLEServer server = it;

	if (server->onReady)
		(*markRoot)(the, server->onReady);
	if (server->onConnect)
		(*markRoot)(the, server->onConnect);
	if (server->onDisconnect)
		(*markRoot)(the, server->onDisconnect);
	if (server->onPasskey)
		(*markRoot)(the, server->onPasskey);
	if (server->onSecured)
		(*markRoot)(the, server->onSecured);
	(*markRoot)(the, server->connectionPrototype);
	(*markRoot)(the, server->characteristicPrototype);

	for (BLEGATTServerService service =	server->services; service; service = service->next) {
		for (int i = 0; i < service->characteristicsLength; i++) {
			BLEGATTServerCharacteristic characteristic = &service->characteristics[i];
			if (0 == characteristic->byteLength) {
				if (characteristic->callbacks.onRead)
					(*markRoot)(the, characteristic->callbacks.onRead);
				if (characteristic->callbacks.onWrite)
					(*markRoot)(the, characteristic->callbacks.onWrite);
				if (characteristic->callbacks.onSubscribe)
					(*markRoot)(the, characteristic->callbacks.onSubscribe);
				if (characteristic->callbacks.onUnsubscribe)
					(*markRoot)(the, characteristic->callbacks.onUnsubscribe);
				(*markRoot)(the, characteristic->callbacks.obj);
			}
			for (int j = 0; j < characteristic->descriptorsLength; j++) {
				BLEGATTServerDescriptor descriptor = &characteristic->descriptors[j];
				if (0 == descriptor->byteLength) {
					if (descriptor->callbacks.onRead)
						(*markRoot)(the, descriptor->callbacks.onRead);
					if (descriptor->callbacks.onWrite)
						(*markRoot)(the, descriptor->callbacks.onWrite);
				}
			}
		}
	}

	for (BLEGATTServerConnection connection = server->connections; connection; connection = connection->next)
		(*markRoot)(the, connection->obj);

	for (BLEGATTServerNotification notification = server->notifications; notification; notification = notification->next) {
		if (notification->callback)
			(*markRoot)(the, notification->callback);
	}
}

static const xsHostHooks ICACHE_RODATA_ATTR xsBLEServerHooks = {
	xs_gattserver_destructor,
	xs_gattserver_mark,
	NULL
};

void xs_gattserver_build(xsMachine *the)
{
	if (gServer)
		xsUnknownError("only one");

	xsmcVars(2);

	xsSlot *onReady = builtinGetCallback(the, xsID_onReady);
	xsSlot *onConnect = builtinGetCallback(the, xsID_onConnect);
	xsSlot *onDisconnect = builtinGetCallback(the, xsID_onDisconnect);
	xsSlot *onPasskey = builtinGetCallback(the, xsID_onPasskey);
	xsSlot *onSecured = builtinGetCallback(the, xsID_onSecured);

	uint8_t secure = 0, authenticate = 0, immediate = 0, bond = 0, display = 0, keyboard = 0;
	if (xsmcHas(xsArg(0), xsID_security)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_security);

		xsmcGet(xsVar(1), xsVar(0), xsID_authenticate);
		authenticate = xsmcTest(xsVar(1));

		xsmcGet(xsVar(1), xsVar(0), xsID_immediate);
		immediate = xsmcTest(xsVar(1));

		xsmcGet(xsVar(1), xsVar(0), xsID_bond);
		bond = xsmcTest(xsVar(1));

		xsmcGet(xsVar(1), xsVar(0), xsID_display);
		display = xsmcTest(xsVar(1));

		xsmcGet(xsVar(1), xsVar(0), xsID_keyboard);
		if (xsStringType == xsmcTypeOf(xsVar(1))) {
			if (c_strcmp("yes/no", xsmcToString(xsVar(1))))
				xsRangeError("bad value");
			keyboard = 2;
			
			if (0 == display)
				xsUnknownError("missing display");
		}
		else
			keyboard = xsmcTest(xsVar(1));

		if (authenticate && !(keyboard || display))
			xsUnknownError("authenticate requires keyboard and/or display");

		secure = 1;
	}

	builtinInitializeTarget(the);

	int mtu = 0;
	if (xsmcHas(xsArg(0), xsID_mtu)) {
		xsSlot tmp;
		xsmcGet(tmp, xsArg(0), xsID_mtu);
		mtu = xsmcToInteger(tmp);
		if (mtu > BLE_ATT_MTU_MAX)
			mtu = BLE_ATT_MTU_MAX;
		else if (mtu < BLE_ATT_MTU_DFLT)
			mtu = BLE_ATT_MTU_DFLT;
	}

	BLEServer server = c_calloc(1, sizeof(BLEServerRecord));
	if (!server)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, server);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsBLEServerHooks);

	server->obj = xsThis;
	server->the = the;
	xsRemember(server->obj);
	server->mutex = xSemaphoreCreateMutex();
	server->onReady = onReady;
	server->onConnect = onConnect;
	server->onDisconnect = onDisconnect;
	server->onPasskey = onPasskey;
	server->onSecured = onSecured;
	server->connectionPrototype = xsmcToReference(xsArg(1));
	server->characteristicPrototype = xsmcToReference(xsArg(2));
	server->mtu = (uint16_t)mtu;
	server->secure = secure;
	server->immediate = immediate;

	gServer = server;

	ble_hs_cfg.sm_sc = secure;
	if (secure) {
		if (keyboard) {
			if (display)
				ble_hs_cfg.sm_io_cap = (2 == keyboard) ? BLE_HS_IO_DISPLAY_YESNO : BLE_HS_IO_KEYBOARD_DISPLAY;		
			else
				ble_hs_cfg.sm_io_cap = BLE_HS_IO_KEYBOARD_ONLY;		
		}
		else
			ble_hs_cfg.sm_io_cap = display ? BLE_HS_IO_DISPLAY_ONLY : BLE_HS_IO_NO_INPUT_OUTPUT;
		ble_hs_cfg.sm_bonding = bond;
		ble_hs_cfg.sm_mitm = authenticate;
	}

	if (0 == gNimBLEInititalized++) {
		ble_hs_cfg.sync_cb = xs_ble_ready;
		ble_hs_cfg.gatts_register_cb = nimble_on_register;
		ble_hs_cfg.gatts_register_arg = server;
		ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
		ble_hs_cfg.reset_cb = nimble_on_reset;

		nimble_port_init();
		ble_store_config_init();
		nimble_port_freertos_init(xs_ble_nimble_task);
	}
	else {
		xs_ble_ready();
	}
}

void xs_gattserver_close(xsMachine *the)
{
	BLEServer server = xsmcGetHostData(xsThis);
	if (server && xsmcGetHostDataValidate(xsThis, (void *)&xsBLEServerHooks)) {
		xsForget(server->obj);
		xs_gattserver_destructor(server);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_gattserver_configure(xsMachine *the)
{
	BLEServer server = xsmcGetHostDataValidate(xsThis, (void *)&xsBLEServerHooks);
	void *data;
	xsUnsignedValue dataLength;

	if (xsmcTest(xsArg(0))) {
		xsmcGetBufferReadable(xsArg(0), &data, &dataLength);
		xsmcSetStringBuffer(xsResult, data, dataLength + 1);
		char *name = xsmcToString(xsResult);
		name[dataLength] = 0;
		ble_svc_gap_device_name_set(name);
	}
	if (xsmcTest(xsArg(1))) {
		xsmcGetBufferReadable(xsArg(1), &data, &dataLength);
		if (2 == dataLength)
			ble_svc_gap_device_appearance_set(*(uint16_t *)data);
	}
}

void xs_gattserver_addService(xsMachine *the)
{
	BLEServer server = xsmcGetHostDataValidate(xsThis, (void *)&xsBLEServerHooks);
	ble_uuid_any_t uuid;

	if (server->ready)
		xsUnknownError("can't add service now");

	xsmcVars(5);
	xsmcGet(xsVar(0), xsArg(0), xsID_uuid);
	if (ble_uuid_from_str(&uuid, xsmcToString(xsVar(0))))
		xsRangeError("bad uuid");

	xsmcGet(xsVar(0), xsArg(0), xsID_characteristics);
	xsmcGet(xsVar(1), xsVar(0), xsID_length);
	int characteristicsLength = xsmcToInteger(xsVar(1));
	if (characteristicsLength <= 0)
		xsRangeError("invalid characteristics");

	BLEGATTServerService service = c_calloc(1, sizeof(BLEGATTServerServiceRecord) + (characteristicsLength * sizeof(BLEGATTServerCharacteristicRecord)));
	if (!service)
		xsUnknownError("no memory");
	service->uuid = uuid;
	service->characteristicsLength = characteristicsLength;

	int nimbleDescriptorsNeeded = 0;

	xsTry {
		int i;
		for (i = 0; i < characteristicsLength; i++) {
			BLEGATTServerCharacteristic gsc = &service->characteristics[i];
			xsmcGetIndex(xsVar(1), xsVar(0), i);
			xsmcGet(xsVar(2), xsVar(1), xsID_uuid);
			if (ble_uuid_from_str(&gsc->uuid, xsmcToString(xsVar(2))))
				xsRangeError("bad uuid");

			xsmcGet(xsVar(2), xsVar(1), xsID_properties);
			if (!xsmcTest(xsVar(2)))
				xsUnknownError("properies missing");
			gsc->properties = xsmcToInteger(xsVar(2));
			if (xsmcHas(xsVar(1), xsID_value)) {
				xsmcGet(xsVar(2), xsVar(1), xsID_value);
				void *data;
				xsUnsignedValue dataLength;
				xsmcGetBufferReadable(xsVar(2), &data, &dataLength);
				if (dataLength > (BLE_ATT_MTU_MAX - 3))		// imprecise... negotiated size may be smaller
					xsRangeError("value too big");
				gsc->byteLength = (uint16_t)dataLength;
				if (dataLength <= sizeof(gsc->bytes))
					c_memmove(gsc->bytes, data, dataLength);
				else {
					gsc->bytesPtr = c_malloc(dataLength);
					if (C_NULL == gsc->bytesPtr)
						xsUnknownError("no memory");
					c_memmove(gsc->bytesPtr, data, dataLength);
				}
			}
			else {
				xsmcGet(xsVar(2), xsVar(1), xsID_onRead);
				gsc->callbacks.onRead = xsmcToReference(xsVar(2));
				xsmcGet(xsVar(2), xsVar(1), xsID_onWrite);
				gsc->callbacks.onWrite = xsmcToReference(xsVar(2));
				xsmcGet(xsVar(2), xsVar(1), xsID_onSubscribe);
				gsc->callbacks.onSubscribe = xsmcToReference(xsVar(2));
				xsmcGet(xsVar(2), xsVar(1), xsID_onUnsubscribe);
				gsc->callbacks.onUnsubscribe = xsmcToReference(xsVar(2));
				if ((C_NULL == gsc->callbacks.onRead) != !(gsc->properties & BLE_GATT_CHR_PROP_READ))
					xsUnknownError("inconsistent read");
				if ((C_NULL == gsc->callbacks.onWrite) != !(gsc->properties & (BLE_GATT_CHR_PROP_WRITE_NO_RSP | BLE_GATT_CHR_PROP_WRITE | BLE_GATT_CHR_PROP_AUTH_SIGN_WRITE)))
					xsUnknownError("inconsistent write");
				if (!!gsc->callbacks.onUnsubscribe != !!(gsc->properties & (BLE_GATT_CHR_PROP_NOTIFY | BLE_GATT_CHR_PROP_INDICATE)))
					xsUnknownError("inconsistent onSubscribe");

				xsVar(3) = xsNewHostInstance(xsReference(server->characteristicPrototype));
				gsc->callbacks.obj = xsmcToReference(xsVar(3));
				xsmcSetHostData(xsVar(3), gsc);
			}

			xsmcGet(xsVar(2), xsVar(1), xsID_descriptors);
			if (!xsmcTest(xsVar(2)))
				continue;
			xsmcGet(xsVar(3), xsVar(2), xsID_length);
			int descriptorsLength = xsmcToInteger(xsVar(3));
			if (descriptorsLength <= 0)
				xsRangeError("invalid descriptors");

			gsc->descriptors = c_calloc(1, sizeof(BLEGATTServerDescriptorRecord) + (descriptorsLength * sizeof(BLEGATTServerDescriptorRecord)));
			if (!gsc->descriptors)
				xsUnknownError("no memory");
			gsc->descriptorsLength = descriptorsLength;
			nimbleDescriptorsNeeded += descriptorsLength + (descriptorsLength ? 1 : 0); 
			for (int i = 0; i < gsc->descriptorsLength; i++) {
				BLEGATTServerDescriptor gsd = &gsc->descriptors[i];
				xsmcGetIndex(xsVar(3), xsVar(2), i);

				xsmcGet(xsVar(4), xsVar(3), xsID_uuid);
				if (ble_uuid_from_str(&gsd->uuid, xsmcToString(xsVar(4))))
					xsRangeError("bad uuid");

				if (xsmcHas(xsVar(3), xsID_value)) {
					if (xsmcHas(xsVar(3), xsID_onRead) || xsmcHas(xsVar(3), xsID_onWrite))
						xsUnknownError("invalid - no callbacks with value");
					xsmcGet(xsVar(4), xsVar(3), xsID_value);
					void *data;
					xsUnsignedValue dataLength;
					xsmcGetBufferReadable(xsVar(4), &data, &dataLength);
					if (dataLength > (BLE_ATT_MTU_MAX - 3))		// imprecise... negotiated size may be smaller
						xsRangeError("value too big");
					gsd->byteLength = (uint16_t)dataLength;
					if (dataLength <= sizeof(gsd->bytes))
						c_memmove(gsd->bytes, data, dataLength);
					else {
						gsd->bytesPtr = c_malloc(dataLength);
						if (C_NULL == gsd->bytesPtr)
							xsUnknownError("no memory");
						c_memmove(gsd->bytesPtr, data, dataLength);
					}
				}
				else {
					xsmcGet(xsVar(4), xsVar(3), xsID_onRead);
					gsd->callbacks.onRead = xsmcToReference(xsVar(4));
					xsmcGet(xsVar(4), xsVar(3), xsID_onWrite);
					gsd->callbacks.onWrite = xsmcToReference(xsVar(4));
					if (!gsd->callbacks.onRead && !gsd->callbacks.onWrite)
						xsUnknownError("invalid - no onRead, onWrite, or value");

//					xsVar(3) = xsNewHostInstance(xsReference(server->descriptorPrototype));
//					gsd->callbacks.obj = xsmcToReference(xsVar(3));
//					xsmcSetHostData(xsVar(3), gsd);
				}
			}
		}

		// build NimBLE service structures
		service->svc.type = BLE_GATT_SVC_TYPE_PRIMARY;
		service->svc.uuid = &service->uuid.u;
		service->svc.includes = C_NULL;
		service->svc.characteristics = c_calloc(1, ((service->characteristicsLength + 1) * sizeof(struct ble_gatt_chr_def)) +
									(nimbleDescriptorsNeeded * sizeof(struct ble_gatt_dsc_def)));
		struct ble_gatt_dsc_def *dsc = (void *)(service->svc.characteristics + service->characteristicsLength + 1);
		if (!service->svc.characteristics)
			xsUnknownError("no memory");
		for (i = 0; i < characteristicsLength; i++) {
			BLEGATTServerCharacteristic gsc = &service->characteristics[i];
			struct ble_gatt_chr_def *chr = (struct ble_gatt_chr_def *)&service->svc.characteristics[i];
			chr->uuid = &gsc->uuid.u;
			chr->access_cb = accessCharacteristic;
			chr->arg = gsc;
			chr->descriptors = gsc->descriptorsLength ? dsc : C_NULL;
			chr->flags = gsc->properties;
			chr->min_key_size = 0;		//@@
			chr->val_handle = &gsc->valueHandle;
			chr->cpfd = C_NULL;
			
			BLEGATTServerDescriptor gsd = gsc->descriptors;
			for (int j = 0; j < gsc->descriptorsLength; j++, gsd++, dsc++) {
				dsc->uuid = &gsd->uuid.u;
				if (gsd->byteLength)
					dsc->att_flags |= BLE_ATT_F_READ;
				else {
					if (gsd->callbacks.onRead)
						dsc->att_flags |= BLE_ATT_F_READ;
					if (gsd->callbacks.onWrite)
						dsc->att_flags |= BLE_ATT_F_WRITE;
				}
				dsc->access_cb = accessCharacteristic;
				dsc->arg = gsc;
			}
			if (gsc->descriptorsLength)
				dsc++;		// terminate descriptors of this characteristic
		}
	}
	xsCatch {
		//@@ fully dispose service
		c_free(service);
		xsThrow(xsException);
	}

	BLEGATTServerService walker = server->services;
	if (C_NULL == walker)
		server->services = service;
	else {
		while (C_NULL != walker->next)
			walker = walker->next;
		walker->next = service;
	}
}

void xs_gattserver_deleteService(xsMachine *the)
{
	BLEServer server = xsmcGetHostDataValidate(xsThis, (void *)&xsBLEServerHooks);
	//@@
}

static void ensureAdvertising(BLEServer server)
{
	if (server->advertising && !ble_gap_adv_active()) {
		struct ble_gap_adv_params advp = {0};
		advp.conn_mode = BLE_GAP_CONN_MODE_UND;
		advp.disc_mode = BLE_GAP_DISC_MODE_GEN;
		ble_gap_adv_start(server->addressType, NULL, BLE_HS_FOREVER, &advp, handleGAPEvent, server);		// ignore error deliberately
	}
}

void xs_gattserver_startAdvertising(xsMachine *the)
{
	BLEServer server = xsmcGetHostDataValidate(xsThis, (void *)&xsBLEServerHooks);

	void *data;
	xsUnsignedValue dataLength;
	xsmcGetBufferReadable(xsArg(0), &data, &dataLength);
	int error = ble_gap_adv_set_data(data, dataLength);
	if (error)
		xsUnknownError("invalid adv");

	if (xsmcArgc > 1) {
		xsmcGetBufferReadable(xsArg(1), &data, &dataLength);
		error = ble_gap_adv_rsp_set_data(data, dataLength);
		if (error)
			xsUnknownError("invalid adv");
	}
	else
		ble_gap_adv_rsp_set_data(C_NULL, 0);

	server->advertising = 1;
	ensureAdvertising(server);
}

void xs_gattserver_stopAdvertising(xsMachine *the)
{
	BLEServer server = xsmcGetHostDataValidate(xsThis, (void *)&xsBLEServerHooks);
	
	server->advertising = 0;
	ble_gap_adv_stop();
}


static BLEGATTServerCharacteristic findCharacteristic(BLEServer server, uint16_t attr_handle)
{
	for (BLEGATTServerService service = server->services; service; service = service->next) {
		for (int i = 0; i < service->characteristicsLength; i++) {
			BLEGATTServerCharacteristic characteristic = &service->characteristics[i];
			if (characteristic->valueHandle == attr_handle)
				return characteristic;
		}
	}

	return C_NULL;
}

static BLEGATTServerConnection findConnection(BLEServer server, uint16_t conn_handle)
{
	for (BLEGATTServerConnection connection = server->connections; connection; connection = connection->next) {
		if (connection->conn_handle == conn_handle)
			return connection;
	}

	return C_NULL;
}

typedef struct {
	xsSlot	*callback;
	xsSlot	*receiver;
	xsSlot	*connection;
	struct os_mbuf *om;
	uint8_t failed;
	volatile uint8_t done;
} BLEAccessCharacteristicRecord, *BLEAccessCharacteristic;

static void readCharacteristic(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	BLEAccessCharacteristic ac = refcon;

	xsBeginHost(the);	
		xsResult = xsCallFunction1(xsReference(ac->callback), ac->receiver ? xsReference(ac->receiver) : xsGlobal, xsReference(ac->connection));

		void *data;
		xsUnsignedValue dataLength;
		xsmcGetBufferReadable(xsResult, &data, &dataLength);

		ac->failed = os_mbuf_append(ac->om, data, dataLength) ? 1 : 0;
	xsEndHost(the);

	ac->done = 1;
}

static void writeCharacteristic(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	BLEAccessCharacteristic ac = refcon;

	xsBeginHost(the);
		int length = OS_MBUF_PKTLEN(ac->om);
		void *buffer = xsmcSetArrayBuffer(xsResult, C_NULL, length);
		os_mbuf_copydata(ac->om, 0, length, buffer);

		xsCallFunction2(xsReference(ac->callback), ac->receiver ? xsReference(ac->receiver) : xsGlobal, xsResult, xsReference(ac->connection));

		ac->failed = 0;
	xsEndHost(the);

	ac->done = 1;
}

int accessCharacteristic(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	BLEGATTServerConnection connection = findConnection(gServer, conn_handle);		//@@ gServer

	switch (ctxt->op) {
		case BLE_GATT_ACCESS_OP_READ_CHR: {
			BLEGATTServerCharacteristic gsc = arg;

			if (gsc->byteLength)
				return os_mbuf_append(ctxt->om, (gsc->byteLength <= sizeof(gsc->bytes)) ? gsc->bytes : gsc->bytesPtr, gsc->byteLength) ? BLE_ATT_ERR_INSUFFICIENT_RES : 0;

			BLEAccessCharacteristicRecord ac = {
				.callback = gsc->callbacks.onRead,
				.receiver = gsc->callbacks.obj,
				.connection = connection->obj,
				.om = ctxt->om,
				.failed = 1,
				.done = 0
			};
			modMessagePostToMachine(gServer->the, C_NULL, 0, readCharacteristic, &ac);		//@@ the

			while (!ac.done)
				modDelayMilliseconds(1);		//@@ mutex?

			return ac.failed ? BLE_ATT_ERR_INSUFFICIENT_RES : 0;
			} break;

		case BLE_GATT_ACCESS_OP_WRITE_CHR: {
			BLEGATTServerCharacteristic gsc = arg;

			BLEAccessCharacteristicRecord ac = {
				.callback = gsc->callbacks.onWrite,
				.receiver = gsc->callbacks.obj,
				.connection = connection->obj,
				.om = ctxt->om,
				.failed = 1,
				.done = 0
			};
			modMessagePostToMachine(gServer->the, C_NULL, 0, writeCharacteristic, &ac);		//@@ the

			while (!ac.done)
				modDelayMilliseconds(1);		//@@ mutex?

			return ac.failed ? BLE_ATT_ERR_INSUFFICIENT_RES : 0;
			} break;
		
		case BLE_GATT_ACCESS_OP_READ_DSC: {
			BLEGATTServerCharacteristic gsc = arg;
			BLEGATTServerDescriptor gsd = gsc->descriptors;
			
			while (gsd->handle != attr_handle)
				gsd++;
			
			if (gsd->byteLength)
				return os_mbuf_append(ctxt->om, (gsd->byteLength <= sizeof(gsd->bytes)) ? gsd->bytes : gsd->bytesPtr, gsd->byteLength) ? BLE_ATT_ERR_INSUFFICIENT_RES : 0;

			BLEAccessCharacteristicRecord ac = {
				.callback = gsd->callbacks.onRead,
				.receiver = C_NULL,
				.connection = connection->obj,
				.om = ctxt->om,
				.failed = 1,
				.done = 0
			};
			modMessagePostToMachine(gServer->the, C_NULL, 0, readCharacteristic, &ac);		//@@ the

			while (!ac.done)
				modDelayMilliseconds(1);		//@@ mutex?

			return ac.failed ? BLE_ATT_ERR_INSUFFICIENT_RES : 0;
			} break;
	
		case BLE_GATT_ACCESS_OP_WRITE_DSC: {
			BLEGATTServerCharacteristic gsc = arg;
			BLEGATTServerDescriptor gsd = gsc->descriptors;
			
			while (gsd->handle != attr_handle)
				gsd++;

			BLEAccessCharacteristicRecord ac = {
				.callback = gsd->callbacks.onWrite,
				.receiver = C_NULL,
				.connection = connection->obj,
				.om = ctxt->om,
				.failed = 1,
				.done = 0
			};
			modMessagePostToMachine(gServer->the, C_NULL, 0, writeCharacteristic, &ac);		//@@ the

			while (!ac.done)
				modDelayMilliseconds(1);		//@@ mutex?

			return ac.failed ? BLE_ATT_ERR_INSUFFICIENT_RES : 0;
			} break;
	}

	return 0;
}

static void deliverReady(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	BLEServer server = refcon;

	server->ready = 1;

	ble_gatts_reset();
	ble_svc_gap_init();

	int err = 0;
	for (BLEGATTServerService service = server->services; (C_NULL != service) && (0 == err); service = service->next) {
		err = ble_gatts_count_cfg(&service->svc);
		if (!err)
			err = ble_gatts_add_svcs(&service->svc);
	}

	if (!err)
		err = ble_gatts_start();

	if (err)
		xsUnknownError("failed to register ble services");

	if (C_NULL == server->onReady)
		return;

	xsBeginHost(the);
		xsCallFunction0(xsReference(server->onReady), server->obj);
	xsEndHost(the);
}

static void deliverConnect(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	BLEServer server = refcon;
	uint16_t conn_handle = *(uint16_t *)message;

	xsBeginHost(the);
		xsResult = xsNewHostInstance(xsReference(server->connectionPrototype));
		BLEGATTServerConnection connection = c_calloc(1, sizeof(BLEGATTServerConnectionRecord));
		if (!connection)
			xsUnknownError("no memory");

		connection->next = server->connections;
		connection->server = server;
		connection->obj = xsmcToReference(xsResult);
		connection->conn_handle = conn_handle;
		connection->maximumWrite = ble_att_mtu(conn_handle) - 3;
		xsmcSetHostData(xsResult, connection);
		server->connections = connection;

		if (server->onConnect)
			xsCallFunction1(xsReference(server->onConnect), server->obj, xsResult);
	xsEndHost(the);

	ensureAdvertising(server);	// NimBLE disables advertising on connection. the client still wants it, so reenable. This will fail if all NimBLE GATT connections are used but it is the best we can do

	if (server->secure && server->immediate)
		ble_gap_security_initiate(conn_handle);
}

static void deliverDisconnect(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsMachine *the = theIn;
	BLEServer server = refcon;
	struct ble_gap_conn_desc *conn = (struct ble_gap_conn_desc *)message;
	BLEGATTServerConnection connection = findConnection(server, conn->conn_handle);
	if (C_NULL == connection)
		return;		// ready called close on connection instance

	if (server->onDisconnect) {
		xsBeginHost(the);
			xsCallFunction1(xsReference(server->onDisconnect), server->obj, xsReference(connection->obj));
		xsEndHost(the);
	}

	if (connection == server->connections)
		server->connections = connection->next;
	else {
		for (BLEGATTServerConnection walker = server->connections; walker; walker = walker->next) {
			if (walker->next == connection) {
				walker->next = connection->next;
				break;
			}
		}
	}

	the->scratch = xsReference(connection->obj);
	xsmcSetHostData(the->scratch, C_NULL);
	c_free(connection);

	ensureAdvertising(server);		// NimBLE disables advertising when all GATT connections used, so check about reenabling after dropping a connection
}

static void deliverOnSecured(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsMachine *the = theIn;
	BLEServer server = refcon;
	uint16_t conn_handle = *(uint16_t *)message;
	BLEGATTServerConnection connection = findConnection(server, conn_handle);

	struct ble_gap_conn_desc desc;
	if (0 != ble_gap_conn_find(conn_handle, &desc))
		return;

	xsBeginHost(the);
		xsSlot tmp;
		xsmcSetNewObject(xsResult);
		xsmcSetBoolean(tmp, desc.sec_state.encrypted);
		xsmcSet(xsResult, xsID_encrypted, tmp);
		xsmcSetBoolean(tmp, desc.sec_state.authenticated);
		xsmcSet(xsResult, xsID_authenticated, tmp);
		xsmcSetBoolean(tmp, desc.sec_state.bonded);
		xsmcSet(xsResult, xsID_bonded, tmp);
		xsmcSetInteger(tmp, desc.sec_state.key_size);
		xsmcSet(xsResult, xsID_keySize, tmp);
		xsCallFunction2(xsReference(server->onSecured), server->obj, xsReference(connection->obj), xsResult);
	xsEndHost(the);
}

static void deliverOnPasskey(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsMachine *the = theIn;
	BLEServer server = refcon;
	struct ble_gap_event *event = (struct ble_gap_event *)message;
	BLEGATTServerConnection connection = findConnection(server, event->passkey.conn_handle);
	struct ble_gap_passkey_params *params = &event->passkey.params;

	xsBeginHost(the);
		xsmcVars(2);
		switch (params->action) {
			case BLE_SM_IOACT_NONE:
				xsmcSetStringX(xsVar(0), "none");
				break;
			case BLE_SM_IOACT_OOB:
				xsmcSetStringX(xsVar(0), "outOfBand");
				break;
			case BLE_SM_IOACT_INPUT:
				xsmcSetStringX(xsVar(0), "input");
				break;
			case BLE_SM_IOACT_DISP: {
				struct ble_sm_io pkey = {
					.action = BLE_SM_IOACT_DISP,
					.passkey = (c_rand() % 999999) + 1
				};
				ble_sm_inject_io(connection->conn_handle, &pkey);

				xsmcSetStringX(xsVar(0), "display");
				xsmcSetInteger(xsVar(1), pkey.passkey);
				} break;
			case BLE_SM_IOACT_NUMCMP:
				xsmcSetStringX(xsVar(0), "compareNumber");
				xsmcSetInteger(xsVar(1), params->numcmp);
				break;
		}
		xsCallFunction3(xsReference(server->onPasskey), server->obj, xsReference(connection->obj), xsVar(0), xsVar(1));
	xsEndHost(the);
}

typedef struct {
	BLEGATTServerCharacteristic gsc;
	uint16_t conn_handle;
	uint16_t attr_handle;
	uint8_t notify;
	uint8_t indicate;
} BLESubscribeRecord, *BLESubscribe;

static void deliverSubscribe(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	BLEServer server = refcon;
	BLESubscribe sub = (BLESubscribe)message;
	BLEGATTServerCharacteristic gsc = findCharacteristic(server, sub->attr_handle);
	BLEGATTServerConnection connection = findConnection(server, sub->conn_handle);
	uint8_t enable = sub->notify || sub->indicate;
	gsc->notify = sub->notify;
	gsc->indicate = sub->indicate;

	if (gsc->callbacks.onSubscribe && enable) {
		xsBeginHost(server->the);
			xsCallFunction1(xsReference(gsc->callbacks.onSubscribe), xsReference(gsc->callbacks.obj), xsReference(connection->obj));
		xsEndHost(server->the);
	}
	else if (gsc->callbacks.onUnsubscribe && !enable) {
		xsBeginHost(server->the);
			xsCallFunction1(xsReference(gsc->callbacks.onUnsubscribe), xsReference(gsc->callbacks.obj), xsReference(connection->obj));
		xsEndHost(server->the);
	}
}

static void doNotification(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void deliverNotificationComplete(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	BLEGATTServerNotification gsn = refcon;
	BLEServer server = gsn->server;

	if (gsn->callback) {
		xsBeginHost(the);
			xsmcSetInteger(xsResult, gsn->error);
			xsCallFunction1(xsReference(gsn->callback), xsGlobal, xsResult);
		xsEndHost(the);
	}

	server->notifications = gsn->next;
	c_free(gsn);
	if (server->notifications)
		doNotification(the, server->notifications, C_NULL, 0);
}

int handleGAPEvent(struct ble_gap_event *event, void *arg)
{
	BLEServer server = arg;

	switch (event->type) {
		case BLE_GAP_EVENT_CONNECT:
			if (0 == event->connect.status)
				modMessagePostToMachine(server->the, (void *)&event->connect.conn_handle, sizeof(event->connect.conn_handle), deliverConnect, server);
			else {
//				printf("CONNECTION FAILED\n");
				//@@ connection failed... restart advertising or notify or ??
			}
			break;

		case BLE_GAP_EVENT_DISCONNECT:
			modMessagePostToMachine(server->the, (void *)&event->disconnect.conn, sizeof(event->disconnect.conn), deliverDisconnect, server);
			break;

		case BLE_GAP_EVENT_MTU:
			(findConnection(server, event->mtu.conn_handle))->maximumWrite = event->mtu.value - 3; 
			break;

		case BLE_GAP_EVENT_PARING_COMPLETE:
			if ((0 == event->enc_change.status) && server->onSecured)
				modMessagePostToMachine(server->the, (uint8_t *)&event->enc_change.conn_handle, sizeof(event->enc_change.conn_handle), deliverOnSecured, server);
			break;

		case BLE_GAP_EVENT_PASSKEY_ACTION:
			if (server->onPasskey)
				modMessagePostToMachine(server->the, (uint8_t *)event, sizeof(*event), deliverOnPasskey, server);
			break;

		case BLE_GAP_EVENT_SUBSCRIBE: {
			BLESubscribeRecord sub = {
				.conn_handle = event->subscribe.conn_handle,
				.attr_handle = event->subscribe.attr_handle,
				.notify = event->subscribe.cur_notify,
				.indicate = event->subscribe.cur_indicate,
			};
			modMessagePostToMachine(server->the, (void *)&sub, sizeof(sub), deliverSubscribe, server);
			} break;

		case BLE_GAP_EVENT_NOTIFY_TX: {
			BLEGATTServerNotification gsn = server->notifications;
			BLEGATTServerCharacteristic gsc = gsn->gsc;

			if (gsc->indicate && (0 == event->notify_tx.status))
				return 0;			// transmitted and waiting on receipt notification

			gsn->error = (BLE_HS_EDONE == event->notify_tx.status) ? 0 : event->notify_tx.status;
			modMessagePostToMachine(gsn->server->the, C_NULL, 0, deliverNotificationComplete, gsn);
			} break;

		default:
//			printf("unhandled GAP event %d\n", (int)event->type);
			break;
	}

	return 0;
}

void xs_gattserverconnection_destructor(void *data)
{
	if (!data) return;
	c_free(data);
}

void doNotification(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	BLEGATTServerNotification gsn = refcon;
	BLEGATTServerCharacteristic gsc = gsn->gsc;
	if (gsc->indicate)
		gsn->error = ble_gatts_indicate_custom(gsn->conn_handle, gsc->valueHandle, gsn->om);
	else
		gsn->error = ble_gatts_notify_custom(gsn->conn_handle, gsc->valueHandle, gsn->om);
	gsn->om = C_NULL;

	if (gsn->error)
		modMessagePostToMachine(gsn->server->the, C_NULL, 0, deliverNotificationComplete, gsn);
}

void xs_gattserverconnection_notify(xsMachine *the)
{
	BLEGATTServerConnection connection = xsmcGetHostDataValidate(xsThis, xs_gattserverconnection_destructor);
	BLEServer server = connection->server;
	BLEGATTServerCharacteristic gsc = xsmcGetHostDataValidate(xsArg(0), xs_gattservercharacteristic_destructor);
	if (!gsc->indicate && !gsc->notify)
		xsUnknownError("disabled");

	xsSlot *callback = C_NULL;
	if (xsmcArgc > 2)
		callback = xsmcToReference(xsArg(2));

	BLEGATTServerNotification gsn = c_calloc(1, sizeof(BLEGATTServerNotificationRecord));
	if (!gsn)
		xsUnknownError("no memory");

	void *data;
	xsUnsignedValue dataLength;
	xsmcGetBufferReadable(xsArg(1), &data, &dataLength);
	gsn->om = ble_hs_mbuf_from_flat(data, (uint16_t)dataLength);
	gsn->gsc = gsc;
	gsn->callback = callback;
	gsn->conn_handle = connection->conn_handle;
	gsn->server = server;

	if (C_NULL == server->notifications) {
		server->notifications = gsn;
		modMessagePostToMachine(server->the, C_NULL, 0, doNotification, gsn);		// cannot send immediately... we could be in a NimBLE callback
	}
	else {
		BLEGATTServerNotification walker = server->notifications;
		while (walker->next)
			walker = walker->next;
		walker->next = gsn;
	}
}

void xs_gattserverconnection_close(xsMachine *the)
{
	BLEGATTServerConnection connection = xsmcGetHostDataValidate(xsThis, xs_gattserverconnection_destructor);
	BLEServer server = connection->server;
	ble_gap_terminate(connection->conn_handle, BLE_ERR_REM_USER_CONN_TERM);

	xsmcSetHostData(xsThis, C_NULL);

	if (connection == server->connections)
		server->connections = connection->next;
	else {
		BLEGATTServerConnection walker = server->connections;
		while (walker->next)
			walker = walker->next;
		walker->next = connection->next;
	}

	c_free(connection);		
}

void xs_gattserverconnection_get_maximumWrite(xsMachine *the)
{
	BLEGATTServerConnection connection = xsmcGetHostDataValidate(xsThis, xs_gattserverconnection_destructor);

	xsmcSetInteger(xsResult, connection->maximumWrite);
}

void xs_gattserverconnection_replyToPasskey(xsMachine *the)
{
	BLEGATTServerConnection connection = xsmcGetHostDataValidate(xsThis, xs_gattserverconnection_destructor);
	struct ble_sm_io pkey = {0};
	const char *action = xsmcToString(xsArg(0));

	if (0 == c_strcmp(action, "input")) {
		pkey.action = BLE_SM_IOACT_INPUT;
		if (xsmcArgc > 1)
			pkey.passkey = xsmcToUnsigned(xsArg(1));
		else
			pkey.passkey = 0;
	}
	else if (0 == c_strcmp(action, "display")) {
		pkey.action = BLE_SM_IOACT_DISP;
		pkey.passkey = xsmcToUnsigned(xsArg(1));
	}
	else if (0 == c_strcmp(action, "compareNumber")) {
		pkey.action = BLE_SM_IOACT_NUMCMP;
		pkey.numcmp_accept = xsmcToBoolean(xsArg(1));
	}
	else if (0 == c_strcmp(action, "outOfBand")) {
		pkey.action = BLE_SM_IOACT_OOB;

		uint8_t *data;
		xsUnsignedValue count;
		xsmcGetBufferReadable(xsArg(1), (void **)&data, &count);
		if (sizeof(pkey.oob) != count)
			xsRangeError("expected 16 bytes");
		c_memmove(pkey.oob, data, count);
	}
	else
		xsUnknownError("invalid action");

	int err = ble_sm_inject_io(connection->conn_handle, &pkey);
	if (err)
		xsUnknownError("failed");
}

void xs_gattservercharacteristic_destructor(void *data)
{
	// nothing to do? host data is owned by server
}


void xs_ble_nimble_task(void *param)
{
	nimble_port_run();
	nimble_port_freertos_deinit();
}


void nimble_on_reset(int reason)
{
//	printf("nimble_on_reset %d\n", reason);
}

void nimble_on_register(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
	BLEServer server = arg;

	if (BLE_GATT_REGISTER_OP_DSC != ctxt->op)
		return;

	BLEGATTServerCharacteristic gsc = findCharacteristic(server, *(ctxt->dsc.chr_def->val_handle));
	const struct ble_gatt_dsc_def *dsc = ctxt->dsc.chr_def->descriptors;
	BLEGATTServerDescriptor gsd = gsc->descriptors;
	while (dsc != ctxt->dsc.dsc_def)
		gsd++, dsc++;
	gsd->handle = ctxt->dsc.handle;
}

void xs_ble_ready(void)
{
	ble_hs_id_infer_auto(0, &gServer->addressType);
	if (gServer->mtu)
		ble_att_set_preferred_mtu(gServer->mtu);
	modMessagePostToMachine(gServer->the, C_NULL, 0, deliverReady, gServer);
}

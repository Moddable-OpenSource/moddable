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

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"

#include "builtinCommon.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "esp_nimble_hci.h"
#include "ble.h"
#include "ble_hs_hci_priv.h"

#if MYNEWT_VAL(BLE_ENABLE_CONN_REATTEMPT)
	#error Espressif reconnect not supported. Disable using CONFIG_BT_NIMBLE_ENABLE_CONN_REATTEMPT in SDKCONFIG.
#endif

static void xs_ble_ready(void);
static void xs_ble_nimble_task(void *param);

static uint8_t gNimBLEInititalized = 0;

static void getUUIDList(xsMachine *the, xsSlot *items, int *length, ble_uuid_any_t **uuids)
{
	xsSlot tmp;
	xsmcGet(tmp, *items, xsID_length);
	*length = xsmcToInteger(tmp);
	*uuids = c_malloc(sizeof(ble_uuid_any_t) * *length);
	if (!*uuids)
		xsUnknownError("no memory");

	for (int i = 0; i < *length; i++) {
		xsmcGetIndex(tmp, *items, i);
		if (ble_uuid_from_str(&(*uuids)[i], xsmcToString(tmp)))
			xsUnknownError("invalid uuid");
	}
}

static uint8_t uuidInList(const ble_uuid_any_t *uuid, const ble_uuid_any_t *uuids, int length)
{
	uint8_t found = 0;
	for (int i = 0; i < length && !found; i++)
		found |= 0 == ble_uuid_cmp(&uuid->u, &uuids[i].u);
	
	return found;
}


static void *gScanner;

struct AdvertisingPacketRecord {
	struct AdvertisingPacketRecord	*next;
	ble_addr_t	address;
	int8_t		rssi;
	int			byteLength;
	uint8_t		payload[];
};
typedef struct AdvertisingPacketRecord AdvertisingPacketRecord;
typedef struct AdvertisingPacketRecord *AdvertisingPacket;

struct ServiceFilterRecord {
	struct ServiceFilterRecord	*next;
	ble_uuid_any_t				uuid;
};
typedef struct ServiceFilterRecord ServiceFilterRecord;
typedef struct ServiceFilterRecord *ServiceFilter;

typedef struct {
	xsMachine	*the;
	xsSlot		obj;
	xsSlot		*onReadable;
	xsSlot		*onError;
	xsSlot		*advertisementConstructor;
	
	struct AdvertisingPacketRecord	*packets;
	SemaphoreHandle_t 	mutex;

	int			error;
	uint8_t		active;
	uint8_t		inflight;
	uint8_t		closed;
	
	ble_uuid_any_t	*serviceFilters;
	int				serviceFiltersLength;
} BLEScannerRecord, *BLEScanner;

static int scan_gap_event_cb(struct ble_gap_event *event, void *arg);
static void deliverScanError(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void deliverScanReadable(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

void xs_gapclient_destructor(void *data)
{
	BLEScanner scan = data;
	if (!data)
		return;

	scan->closed = true;

	if (scan == gScanner)
		gScanner = C_NULL;

	if (scan->active)
		ble_gap_disc_cancel();

	if (scan->mutex) {
		xSemaphoreTake(scan->mutex, portMAX_DELAY);
			while (scan->packets) {
				void *next = scan->packets->next;
				c_free(scan->packets);
				scan->packets = next;
			}
		xSemaphoreGive(scan->mutex);
		vSemaphoreDelete(scan->mutex);
	}

	if (scan->serviceFilters)
		c_free(scan->serviceFilters);

	if (!scan->inflight)
		c_free(scan);
}

void xs_gapclient_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	BLEScanner scan = it;

	if (scan->onReadable)
		(*markRoot)(the, scan->onReadable);
	if (scan->onError)
		(*markRoot)(the, scan->onError);
	if (scan->advertisementConstructor)
		(*markRoot)(the, scan->advertisementConstructor);
}

static const xsHostHooks ICACHE_RODATA_ATTR xsBLEScannerHooks = {
	xs_gapclient_destructor,
	xs_gapclient_mark,
	NULL
};

void xs_gapclient_build(xsMachine *the)
{
	BLEScanner scan;

	xsSlot *onReadable = builtinGetCallback(the, xsID_onReadable);
	xsSlot *onError = builtinGetCallback(the, xsID_onError);

	builtinInitializeTarget(the);

	scan = c_calloc(1, sizeof(BLEScannerRecord));
	if (!scan)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, scan);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsBLEScannerHooks);

	scan->obj = xsThis;
	scan->the = the;
	xsRemember(scan->obj);
	scan->onReadable = onReadable;
	scan->onError = onError;
	scan->advertisementConstructor = xsmcToReference(xsArg(1));
	scan->mutex = xSemaphoreCreateMutex();

	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_services)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_services);
		getUUIDList(the, &xsVar(0), &scan->serviceFiltersLength, &scan->serviceFilters);
	}

	if (0 == gNimBLEInititalized++) {
		gScanner = scan;
		ble_hs_cfg.sync_cb = xs_ble_ready;
		nimble_port_init();
		ble_store_config_init();		
		nimble_port_freertos_init(xs_ble_nimble_task);
	}
	else
		xs_ble_ready();
}

void xs_gapclient_close(xsMachine *the)
{
	BLEScanner scan = xsmcGetHostData(xsThis);
	if (scan && xsmcGetHostDataValidate(xsThis, (void *)&xsBLEScannerHooks)) {
		xsForget(scan->obj);
		xs_gapclient_destructor(scan);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_gapclient_read(xsMachine *the)
{
	BLEScanner scan = xsmcGetHostDataValidate(xsThis, (void *)&xsBLEScannerHooks);

	if (!scan->packets)
		return;

	xSemaphoreTake(scan->mutex, portMAX_DELAY);
	AdvertisingPacket packet = scan->packets;
	scan->packets = packet->next;
	xSemaphoreGive(scan->mutex);

	xsSlot tmp;
	xsmcSetInteger(tmp, packet->byteLength);
	xsResult = xsNewFunction1(xsReference(scan->advertisementConstructor), tmp);
	c_memmove(xsmcToArrayBuffer(xsResult), packet->payload, packet->byteLength);

	char address[24];
	snprintf(address, sizeof(address), "%02X:%02X:%02X:%02X:%02X:%02X/%02X",
         packet->address.val[5], packet->address.val[4], packet->address.val[3],
         packet->address.val[2], packet->address.val[1], packet->address.val[0],
         packet->address.type);
	 xsmcVars(1);
	 xsmcSetString(xsVar(0), address);
	 xsmcSet(xsResult, xsID_address, xsVar(0));
	 if (127 != packet->rssi) {			// 127 indicates rssi unavailable
		xsmcSetInteger(xsVar(0), packet->rssi);
		xsmcSet(xsResult, xsID_rssi, xsVar(0));
	 }

	c_free(packet);
}

void xs_advertisement_get(xsMachine *the)
{
	uint8_t *data;
	xsUnsignedValue count;
	xsmcGetBufferReadable(xsThis, (void **)&data, &count);
	int type = xsmcToInteger(xsArg(0));
	if ((type < 0) || (type > 255))
		xsUnknownError("invalid");

	for (int offset = 0; offset < count - 3; offset += 1 + data[offset]) {
		int size = data[offset];
		if ((size < 2) || (size > (count - offset)))
			break;
		if (data[offset + 1] != type)
			continue;

		xsmcSetArrayBuffer(xsResult, C_NULL, size - 1);
		xsmcGetBufferReadable(xsThis, (void **)&data, &count);
		c_memmove(xsmcToArrayBuffer(xsResult), data + offset + 2, size - 1);
		break;
	}
}

void xs_ble_ready(void)
{
	BLEScanner scan = gScanner;
	if (!scan)
		return;

	gScanner = C_NULL;

    struct ble_gap_disc_params scan_params = {0};
    scan_params.passive = 0; // active scan
    scan_params.itvl = 0x0010; // ~10ms
    scan_params.window = 0x0010; // ~10ms
    scan_params.filter_policy = 0;
    scan_params.limited = 0;

	scan->error = ble_gap_disc(0, BLE_HS_FOREVER, &scan_params, scan_gap_event_cb, scan);
    if (0 != scan->error) {
		if (scan->onError)
			modMessagePostToMachine(scan->the, C_NULL, 0, deliverScanError, scan);
		return;
	}

	scan->active = true;
}

void deliverScanReadable(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	BLEScanner scan = refcon;
	
	if (scan->closed) {
		c_free(scan);
		return;
	}
	
	xSemaphoreTake(scan->mutex, portMAX_DELAY);
		scan->inflight = false;
		int count = 0;
		for (AdvertisingPacket packet = scan->packets; C_NULL != packet; packet = packet->next)
			count +=1;
	xSemaphoreGive(scan->mutex);

	xsBeginHost(the);
		xsmcSetInteger(xsResult, count);
		xsCallFunction1(xsReference(scan->onReadable), scan->obj, xsResult);
	xsEndHost(the);
}

void deliverScanError(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	BLEScanner scan = refcon;

	if (scan->closed) {
		c_free(scan);
		return;
	}

	xsBeginHost(the);
		xsmcSetInteger(xsResult, scan->error);
		xsCallFunction1(xsReference(scan->onError), scan->obj, xsResult);
	xsEndHost(the);
}

int scan_gap_event_cb(struct ble_gap_event *event, void *arg)
{
	BLEScanner scan = arg;
	
	if (scan->closed)
		return -1;

	if (BLE_GAP_EVENT_DISC != event->type)
		return 0;

	if (scan->serviceFiltersLength) {
		struct ble_hs_adv_fields fields;
		uint8_t foundService = 0;
		if (0 == ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data)) {
			for (int i = 0; i < scan->serviceFiltersLength && !foundService; i++) {
				ble_uuid_any_t *service = &scan->serviceFilters[i]; 
				switch (service->u.type) {
					case BLE_UUID_TYPE_16:
						for (int j = 0; j < fields.num_uuids16; j++)
							foundService |= 0 == ble_uuid_cmp(&service->u, &fields.uuids16[j].u);
						break;
					case BLE_UUID_TYPE_32:
						for (int j = 0; j < fields.num_uuids32; j++)
							foundService |= 0 == ble_uuid_cmp(&service->u, &fields.uuids32[j].u);
						break;
					case BLE_UUID_TYPE_128:
						for (int j = 0; j < fields.num_uuids128; j++)
							foundService |= 0 == ble_uuid_cmp(&service->u, &fields.uuids128[j].u);
				}
			}
		}
		if (!foundService)
			return 0;
	}


	uint8_t notify = 0;
	AdvertisingPacket packet = (AdvertisingPacket)c_malloc(sizeof(AdvertisingPacketRecord) + event->disc.length_data);
	packet->next = C_NULL;
	packet->address = event->disc.addr;
	packet->byteLength = event->disc.length_data;
	c_memmove(packet->payload, event->disc.data, event->disc.length_data);
	packet->rssi = event->disc.rssi;
	xSemaphoreTake(scan->mutex, portMAX_DELAY);
		notify = !scan->inflight; 
		scan->inflight = true;
		
		AdvertisingPacket *walker = &scan->packets;
		while (*walker)
			walker = &(*walker)->next;
		*walker = packet;
	xSemaphoreGive(scan->mutex);
	if (notify && scan->onReadable)
		modMessagePostToMachine(scan->the, C_NULL, 0, deliverScanReadable, scan);

	return 0;
}

void xs_ble_nimble_task(void *param)
{
	nimble_port_run();
	nimble_port_freertos_deinit();
}

struct BLEGATTCharacteristicValueRecord {
	struct BLEGATTCharacteristicValueRecord *next;
	uint16_t	val_handle;
	
	uint16_t	byteLength;
	uint8_t		payload[];
};
typedef struct BLEGATTCharacteristicValueRecord BLEGATTCharacteristicValueRecord;
typedef struct BLEGATTCharacteristicValueRecord *BLEGATTCharacteristicValue;


typedef struct {
	xsMachine	*the;
	xsSlot		obj;
	
	xsSlot		*onReadable;
	xsSlot		*onError;
	xsSlot		*onSecured;
	xsSlot		*onPasskey;

	ble_addr_t	address;

	uint16_t	conn_handle;

	uint8_t		connected;
	uint8_t		connecting;
	uint16_t	mtu;
	
	int			result;
	xsSlot		*request;
	xsSlot		*responsePrototype;
	struct ble_gatt_svc	requestSvc;

	uint8_t			isPrimaryServices;
	int				primaryServicesUUIDsLength;
	ble_uuid_any_t	*primaryServicesUUIDs;
	struct ble_gatt_svc 	*primaryServices;
	int				primaryServicesLength;

	uint8_t			isCharacteristics;
	int				characteristicsUUIDsLength;
	ble_uuid_any_t	*characteristicsUUIDs;
	struct ble_gatt_chr 	*characteristics;
	int				characteristicsLength;
	
	uint8_t			isDescriptors;
	int				descriptorsUUIDsLength;
	ble_uuid_any_t	*descriptorsUUIDs;
	struct ble_gatt_dsc 	*descriptors;
	int				descriptorsLength;	
	
	uint8_t			isRead;
	uint16_t		readResultByteLength;
	void			*readResult;
	
	uint8_t			isClose;

	uint8_t 		secure;
	uint8_t			immediate;

	BLEGATTCharacteristicValue		values;
	uint8_t							onReadableInFlight;
} GATTClientRecord, *GATTClient;

typedef struct {
	struct ble_gatt_chr	chr;
	uint16_t		svc_start_handle;
	uint16_t		svc_end_handle;
	uint16_t		cccdHandle;
} GATTClientCharacteristicRecord, *GATTClientCharacteristic;

static int onGATTConnectionEvent(struct ble_gap_event *event, void *arg);
static void gattClientExecuted(GATTClient gc, int result);

void xs_gattclient_destructor(void *data)
{
	GATTClient gc = data;
	if (!gc)
		return;

	while (gc->values) {
		BLEGATTCharacteristicValue next = gc->values->next;
		c_free(gc->values);
		gc->values = next;
	}

	c_free(gc);
}

static void xs_gattclient_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	GATTClient gc = it;
	
	if (gc->onReadable)
		(*markRoot)(the, gc->onReadable);
	if (gc->onError)
		(*markRoot)(the, gc->onError);
	if (gc->onSecured)
		(*markRoot)(the, gc->onSecured);
	if (gc->onPasskey)
		(*markRoot)(the, gc->onPasskey);
	if (gc->request)
		(*markRoot)(the, gc->request);
	if (gc->responsePrototype)
		(*markRoot)(the, gc->responsePrototype);
}

static const xsHostHooks ICACHE_RODATA_ATTR xsGATTClientHooks = {
	xs_gattclient_destructor,
	xs_gattclient_mark,
	NULL
};

void xs_gattclient_constructor(xsMachine *the)
{
	ble_addr_t address;
	int mtu = 0;
	uint8_t secure = 0, authenticate = 0, bond = 0, display = 0, keyboard = 0, immediate = 0;

	xsmcVars(2);

	if (xsmcHas(xsArg(0), xsID_mtu)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_mtu);
		mtu = xsmcToInteger(xsVar(0));
		if (mtu > BLE_ATT_MTU_MAX)
			mtu = BLE_ATT_MTU_MAX;
		else if (mtu < BLE_ATT_MTU_DFLT)
			mtu = BLE_ATT_MTU_DFLT;
	}

	xsmcGet(xsVar(0), xsArg(0), xsID_address);
    if (sscanf(xsmcToString(xsVar(0)),
               "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx/%2hhx",
               &address.val[5], &address.val[4], &address.val[3],
               &address.val[2], &address.val[1], &address.val[0],
               &address.type) != 7)
		xsUnknownError("invalid address");

	xsSlot *onReadable = builtinGetCallback(the, xsID_onReadable);
	xsSlot *onError = builtinGetCallback(the, xsID_onError);
	xsSlot *onSecured = builtinGetCallback(the, xsID_onSecured);
	xsSlot *onPasskey = builtinGetCallback(the, xsID_onPasskey);

	if (xsmcHas(xsArg(0), xsID_security)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_security);

		xsmcGet(xsVar(1), xsVar(0), xsID_authenticate);
		authenticate = xsmcTest(xsVar(1));

		xsmcGet(xsVar(1), xsVar(0), xsID_immediate);
		immediate = xsmcTest(xsVar(1));

		xsmcGet(xsVar(1), xsVar(0), xsID_bond);
		bond = xsmcTest(xsVar(1));

		xsmcGet(xsVar(1), xsVar(0), xsID_ioCapabilities);
		char *ioCap = xsmcToString(xsVar(1));
		if (0 == c_strcmp(ioCap, "display"))
			display = 1;
		else if (0 == c_strcmp(ioCap, "numbers"))
			keyboard = 1;
		else if (0 == c_strcmp(ioCap, "display+numbers")) {
			display = 1;
			keyboard = 1;
		}
		else if (0 == c_strcmp(ioCap, "display+confirm")) {
			display = 1;
			keyboard = 2;
		}

		if (authenticate && !(keyboard || display))
			xsUnknownError("authenticate requires keyboard and/or display");

		secure = 1;
	}

	GATTClient gc = c_calloc(1, sizeof(GATTClientRecord));
	if (C_NULL == gc)
		xsUnknownError("no memory");

	gc->the = the;
	gc->obj = xsThis;
	xsRemember(gc->obj);
	gc->address = address;
	gc->onReadable = onReadable;
	gc->onError = onError;
	gc->onSecured = onSecured;
	gc->onPasskey = onPasskey;
	gc->mtu = (uint16_t)mtu;
	gc->secure = secure;
	gc->immediate = immediate;

	xsmcSetHostData(xsThis, gc);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsGATTClientHooks);

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
}

void xs_gattclient_close(xsMachine *the)
{
	if (C_NULL == xsmcGetHostData(xsThis))
		return;
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	if (xsmcArgc) {
		gc->request = xsmcToReference(xsArg(0));
		gc->isClose = 1;
		if (gc->connected) {
			if (0 != ble_gap_terminate(gc->conn_handle, BLE_ERR_REM_USER_CONN_TERM))
				gattClientExecuted(gc, 0);
		}
		else
			gattClientExecuted(gc, 0);
	}
	else {
		if (gc->connecting)
			ble_gap_conn_cancel();
	}
}

static void deliverExecutionResult(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	GATTClient gc = refcon;
	uint8_t isClose = gc->isClose;

	xsBeginHost(gc->the);
	xsmcVars(5);
	if (gc->result) {
		xsmcSetInteger(xsResult, gc->result);
		xsResult = xsNew1(xsGlobal, xsID_Error, xsResult);
	}
	else {
		xsmcSetNull(xsResult);
		if (gc->isPrimaryServices) {
			xsVar(0) = xsNew0(xsGlobal, xsID_Array);
			for (int i = 0; i < gc->primaryServicesLength; i++) {
				struct ble_gatt_svc *svc = &gc->primaryServices[i];

				xsVar(1) = xsNewHostInstance(xsReference(gc->responsePrototype));
				xsmcSetHostChunk(xsVar(1), svc, sizeof(*svc));

				xsmcSetIndex(xsVar(0), i, xsVar(1));
			}
		}
		else if (gc->isCharacteristics) {
			xsVar(0) = xsNew0(xsGlobal, xsID_Array);
			for (int i = 0; i < gc->characteristicsLength; i++) {
				GATTClientCharacteristicRecord gcc;
				gcc.chr = gc->characteristics[i];
				gcc.svc_start_handle = gc->requestSvc.start_handle; 
				gcc.svc_end_handle = gc->requestSvc.end_handle;
				gcc.cccdHandle = 0; 

				xsVar(1) = xsNewHostInstance(xsReference(gc->responsePrototype));
				xsmcSetHostChunk(xsVar(1), &gcc, sizeof(gcc));

				xsmcSetIndex(xsVar(0), i, xsVar(1));
			}
		}
		else if (gc->isDescriptors) {
			xsVar(0) = xsNew0(xsGlobal, xsID_Array);
			for (int i = 0; i < gc->descriptorsLength; i++) {
				struct ble_gatt_dsc *dsc = &gc->descriptors[i];

				xsVar(1) = xsNewHostInstance(xsReference(gc->responsePrototype));
				xsmcSetHostChunk(xsVar(1), dsc, sizeof(*dsc));

				xsmcSetIndex(xsVar(0), i, xsVar(1));

				static const ble_uuid16_t uuid_declaration = BLE_UUID16_INIT(0x2902);
				if (ble_uuid_cmp(&dsc->uuid.u, &uuid_declaration.u) == 0) {
					xsVar(4) = xsReference(gc->request);
					xsmcGet(xsVar(4), xsVar(4), xsID_params);
					xsmcGetIndex(xsVar(4), xsVar(4), 0);
					GATTClientCharacteristic gcc = xsmcGetHostChunkValidate(xsVar(4), xs_gattclientcharacteristic_destructor);
					gcc->cccdHandle = dsc->handle;
				}
			}
		}
		else if (gc->isRead)
			xsmcSetArrayBuffer(xsVar(0), gc->readResult, gc->readResultByteLength);
	}

	gc->isPrimaryServices = false;
	if (gc->primaryServicesUUIDs) {
		c_free(gc->primaryServicesUUIDs);
		gc->primaryServicesUUIDs = C_NULL;
	}
	if (gc->primaryServices) {
		c_free(gc->primaryServices);
		gc->primaryServices = C_NULL;
	}

	gc->isCharacteristics = false;
	if (gc->characteristicsUUIDs) {
		c_free(gc->characteristicsUUIDs);
		gc->characteristicsUUIDs = C_NULL;
	}
	if (gc->characteristics) {
		c_free(gc->characteristics);
		gc->characteristics = C_NULL;
	}

	gc->isDescriptors = false;
	if (gc->descriptorsUUIDs) {
		c_free(gc->descriptorsUUIDs);
		gc->descriptorsUUIDs = C_NULL;
	}
	if (gc->descriptors) {
		c_free(gc->descriptors);
		gc->descriptors = C_NULL;
	}

	gc->isRead = false;
	if (gc->readResult) {
		c_free(gc->readResult);
		gc->readResult = C_NULL;
	}

	xsSlot *request = gc->request;
	gc->request = C_NULL;
	gc->responsePrototype = C_NULL;
	xsCall2(xsReference(request), xsID_executed, xsResult, xsVar(0));
	xsEndHost(gc->the);

	if (isClose) {
		xsmcSetHostData(gc->obj, C_NULL);
		xsmcSetHostDestructor(gc->obj, C_NULL);
		xsForget(gc->obj);
		xs_gattclient_destructor(gc);
	}
}

void gattClientExecuted(GATTClient gc, int result)
{
	gc->result = result;
	modMessagePostToMachine(gc->the, C_NULL, 0, deliverExecutionResult, gc);
}

static void deliverOnReadable(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	GATTClient gc = refcon;
	int count = 0;
	
	for (BLEGATTCharacteristicValue value = gc->values; C_NULL != value; value = value->next)
		count += 1;

	gc->onReadableInFlight = 0; 

	xsBeginHost(the);
		xsmcSetInteger(xsResult, count);
		xsCallFunction1(xsReference(gc->onReadable), gc->obj, xsResult);
	xsEndHost(the);
}

static void deliverOnError(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	GATTClient gc = refcon;

	xsBeginHost(the);
		xsCallFunction0(xsReference(gc->onError), gc->obj);
	xsEndHost(the);
}

static void deliverOnSecured(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	GATTClient gc = refcon;

	struct ble_gap_conn_desc desc;
	if (0 != ble_gap_conn_find(gc->conn_handle, &desc))
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
		xsCallFunction1(xsReference(gc->onSecured), gc->obj, xsResult);
	xsEndHost(the);
}

static void deliverOnPasskey(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	GATTClient gc = refcon;
	struct ble_gap_passkey_params *params = (struct ble_gap_passkey_params *)message;

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
				ble_sm_inject_io(gc->conn_handle, &pkey);

				xsmcSetStringX(xsVar(0), "display");
				xsmcSetInteger(xsVar(1), pkey.passkey);
				} break;
			case BLE_SM_IOACT_NUMCMP:
				xsmcSetStringX(xsVar(0), "compareNumber");
				xsmcSetInteger(xsVar(1), params->numcmp);
				break;
		}
		xsCallFunction2(xsReference(gc->onPasskey), gc->obj, xsVar(0), xsVar(1));
	xsEndHost(the);
}

static int onGATTMCUExchanged(uint16_t conn_handle, const struct ble_gatt_error *error, uint16_t mtu, void *arg)
{
	GATTClient gc = arg;

	if (0 == error->status)
		gc->mtu = mtu;
	else
		gc->mtu = ble_att_mtu(gc->conn_handle);

	gattClientExecuted(gc, 0);

	if (gc->secure && gc->immediate)
		ble_gap_security_initiate(conn_handle);

	return 0;
}

int onGATTConnectionEvent(struct ble_gap_event *event, void *arg)
{
	GATTClient gc = arg;

	switch (event->type) {
		case BLE_GAP_EVENT_CONNECT:
			gc->connecting = false;
			if (event->connect.status) {
				gattClientExecuted(gc, event->connect.status);
				return 0;
			}
			gc->connected = true;
			gc->conn_handle = event->connect.conn_handle;
			if (gc->mtu)
				ble_gattc_exchange_mtu(gc->conn_handle, onGATTMCUExchanged, gc);
			else {
				gc->mtu = ble_att_mtu(gc->conn_handle);
				gattClientExecuted(gc, 0);

				if (gc->secure && gc->immediate)
					ble_gap_security_initiate(gc->conn_handle);
			}
			break;

		case BLE_GAP_EVENT_DISCONNECT:
			gc->connecting = false;
			gc->connected = false;
			if (gc->isClose)
				gattClientExecuted(gc, 0);
			else if (gc->onError)
				modMessagePostToMachine(gc->the, C_NULL, 0, deliverOnError, gc);
			break;
		
		case BLE_GAP_EVENT_MTU:
			gc->mtu = event->mtu.value;
			break;

		case BLE_GAP_EVENT_ENC_CHANGE:
			if ((0 == event->enc_change.status) && gc->onSecured)
				modMessagePostToMachine(gc->the, C_NULL, 0, deliverOnSecured, gc);
			break;
		
		case BLE_GAP_EVENT_PASSKEY_ACTION:
			if (gc->onPasskey)
				modMessagePostToMachine(gc->the, (uint8_t *)&event->passkey.params, sizeof(event->passkey.params), deliverOnPasskey, gc);
			break;

		case BLE_GAP_EVENT_NOTIFY_RX: {
			BLEGATTCharacteristicValue value = c_malloc(sizeof(BLEGATTCharacteristicValueRecord) + event->notify_rx.om->om_len);
			if (C_NULL == value)
				esp_panic_handler();
			value->next = C_NULL;
			value->byteLength = event->notify_rx.om->om_len;
			c_memmove(value->payload, event->notify_rx.om->om_data, value->byteLength);
			
			value->val_handle = event->notify_rx.attr_handle;
			
			BLEGATTCharacteristicValue walker = gc->values;
			if (C_NULL == walker)
				gc->values = value;
			else {
				for (; C_NULL != walker; walker = walker->next) {
					if (C_NULL == walker->next) {
						walker->next = value;
						break;
					}
				}
			}

			if (!gc->onReadableInFlight && gc->onReadable) {
				gc->onReadableInFlight = 1;
				modMessagePostToMachine(gc->the, C_NULL, 0, deliverOnReadable, gc);
			}
			} break;
	}

	return 0;
}

void xs_gattclient_connect(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	gc->request = xsmcToReference(xsArg(0));

	if (gc->mtu)
		ble_att_set_preferred_mtu(gc->mtu);

	struct ble_gap_conn_params conn_params = {
		.scan_itvl = 0x0010,
		.scan_window = 0x0010,
		.itvl_min = 0x0018,  // 30 ms
		.itvl_max = 0x0028,  // 50 ms
		.latency = 0,
		.supervision_timeout = 0x0100, // 2.56 sec
		.min_ce_len = 0,
		.max_ce_len = 0,
	};

	ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &gc->address,
		 BLE_HS_FOREVER, &conn_params,
		 onGATTConnectionEvent, gc);

	gc->connecting = 1;
}

static int onGATTServiceDiscovered(uint16_t conn_handle, const struct ble_gatt_error *error,
			 const struct ble_gatt_svc *svc, void *arg)
{
	GATTClient gc = arg;
	int status = error->status;

	if (svc) {
		if (gc->primaryServicesUUIDsLength) {
			if (!uuidInList(&svc->uuid, gc->primaryServicesUUIDs, gc->primaryServicesUUIDsLength))
				return 0;
		}

		gc->primaryServices = c_realloc(gc->primaryServices, (gc->primaryServicesLength + 1) * sizeof(struct ble_gatt_svc));
		if (C_NULL == gc->primaryServices)
			status = ESP_ERR_NO_MEM;
		else
			gc->primaryServices[gc->primaryServicesLength++] = *svc;
	}

	if (status) {
		if (BLE_HS_EDONE == error->status)
			gattClientExecuted(gc, 0);
		else
			gattClientExecuted(gc, status);
	}

	return 0;
}

void xs_gattclient_getPrimaryServices(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	gc->request = xsmcToReference(xsArg(0));
	gc->responsePrototype = xsmcToReference(xsArg(2));

	gc->isPrimaryServices = true;
	gc->primaryServices = C_NULL;
	gc->primaryServicesLength = 0;

	if (xsmcTest(xsArg(1)))
		getUUIDList(the, &xsArg(1), &gc->primaryServicesUUIDsLength, &gc->primaryServicesUUIDs);

	int err;
	if (1 == gc->primaryServicesUUIDsLength) {
		gc->primaryServicesUUIDsLength = 0;
		err = ble_gattc_disc_svc_by_uuid(gc->conn_handle, &gc->primaryServicesUUIDs[0].u, onGATTServiceDiscovered, gc);
		c_free(gc->primaryServicesUUIDs);
		gc->primaryServicesUUIDs = C_NULL;
	}
	else
		err = ble_gattc_disc_all_svcs(gc->conn_handle, onGATTServiceDiscovered, gc);

	if (err)
		gattClientExecuted(gc, err);
}

static int onGATTCharacteristicDiscovered(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *chr, void *arg)
{
	GATTClient gc = arg;
	int status = error->status;

	if (chr) {
		if (gc->characteristicsUUIDsLength) {
			if (!uuidInList(&chr->uuid, gc->characteristicsUUIDs, gc->characteristicsUUIDsLength))
				return 0;
		}

		gc->characteristics = c_realloc(gc->characteristics, (gc->characteristicsLength + 1) * sizeof(struct ble_gatt_chr));
		if (C_NULL == gc->characteristics)
			status = ESP_ERR_NO_MEM;
		else
			gc->characteristics[gc->characteristicsLength++] = *chr;
	}

	if (status) {
		if (BLE_HS_EDONE == status)
			gattClientExecuted(gc, 0);
		else
			gattClientExecuted(gc, status);
	}

	return 0;
}

void xs_gattclient_getCharacteristics(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	gc->request = xsmcToReference(xsArg(0));
	gc->requestSvc =  *(struct ble_gatt_svc *)xsmcGetHostChunkValidate(xsArg(1), xs_gattclientservice_destructor);
	gc->responsePrototype = xsmcToReference(xsArg(3));
	
	gc->isCharacteristics = true;
	gc->characteristics = C_NULL;
	gc->characteristicsLength = 0;

	if (xsmcTest(xsArg(2)))
		getUUIDList(the, &xsArg(2), &gc->characteristicsUUIDsLength, &gc->characteristicsUUIDs);

	if (gc->requestSvc.start_handle == gc->requestSvc.end_handle)
		gattClientExecuted(gc, 0);
	else if (1 == gc->characteristicsUUIDsLength) {
		gc->characteristicsUUIDsLength = 0;
		ble_gattc_disc_chrs_by_uuid(gc->conn_handle, gc->requestSvc.start_handle, gc->requestSvc.end_handle, &gc->characteristicsUUIDs[0].u, onGATTCharacteristicDiscovered, gc);
		c_free(gc->characteristicsUUIDs);
		gc->characteristicsUUIDs = C_NULL;
	}
	else {
		int err = ble_gattc_disc_all_chrs(gc->conn_handle, gc->requestSvc.start_handle, gc->requestSvc.end_handle, onGATTCharacteristicDiscovered, gc);
		if (err)
			gattClientExecuted(gc, err);
	}
}

static int onGATTDescriptorDiscovered(uint16_t conn_handle, const struct ble_gatt_error *error, uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc, void *arg)
{
	GATTClient gc = arg;
	int status = error->status;
	static const ble_uuid16_t uuid_declaration = BLE_UUID16_INIT(0x2803);

	if (dsc) {
		if (ble_uuid_cmp(&dsc->uuid.u, &uuid_declaration.u) == 0) {
			gattClientExecuted(gc, 0);
			return BLE_HS_EDONE;
		}

		if (gc->descriptorsUUIDsLength) {
			if (!uuidInList(&dsc->uuid, gc->descriptorsUUIDs, gc->descriptorsUUIDsLength))
				return 0;
		}

		gc->descriptors = c_realloc(gc->descriptors, (gc->descriptorsLength + 1) * sizeof(struct ble_gatt_dsc));
		if (C_NULL == gc->descriptors)
			status = ESP_ERR_NO_MEM;
		else
			gc->descriptors[gc->descriptorsLength++] = *dsc;
	}

	if (status) {
		if (BLE_HS_EDONE == status)
			gattClientExecuted(gc, 0);
		else
			gattClientExecuted(gc, status);
	}

	return 0;
}

void xs_gattclient_getDescriptors(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	gc->request = xsmcToReference(xsArg(0));
	GATTClientCharacteristic gcc = (GATTClientCharacteristic)xsmcGetHostChunkValidate(xsArg(1), xs_gattclientcharacteristic_destructor);
	uint16_t start_handle = gcc->chr.val_handle, end_handle = gcc->svc_end_handle;
	gc->responsePrototype = xsmcToReference(xsArg(3));
	
	gc->isDescriptors = true;
	gc->descriptors = C_NULL;
	gc->descriptorsLength = 0;

	if (xsmcTest(xsArg(2)))
		getUUIDList(the, &xsArg(2), &gc->descriptorsUUIDsLength, &gc->descriptorsUUIDs);

	if (start_handle == end_handle) {
		gattClientExecuted(gc, 0);
		return;
	}

	int err = ble_gattc_disc_all_dscs(gc->conn_handle, start_handle, end_handle, onGATTDescriptorDiscovered, gc);
	if (err)
		gattClientExecuted(gc, err);
}

static int onGATTRead(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
	GATTClient gc = arg;
	int status = error->status;

	if (0 == status) {
		gc->readResultByteLength = OS_MBUF_PKTLEN(attr->om);
		gc->readResult = c_malloc(gc->readResultByteLength);
		if (C_NULL == gc->readResult)
			status = ESP_ERR_NO_MEM;
		else
			os_mbuf_copydata(attr->om, 0, gc->readResultByteLength, gc->readResult);
	}

	gattClientExecuted(gc, status);

	return 0;
}

void xs_gattclient_read(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	gc->request = xsmcToReference(xsArg(0));
	xsDestructor destructor = xsGetHostDestructor(xsArg(1));
	uint16_t handle;
	if (destructor == xs_gattclientcharacteristic_destructor)
		handle = ((GATTClientCharacteristic)xsmcGetHostChunk(xsArg(1)))->chr.val_handle;
	else if (destructor == xs_gattclientdescriptor_destructor)
		handle = ((struct ble_gatt_dsc *)xsmcGetHostChunk(xsArg(1)))->handle;
	else
		xsUnknownError("not characteristic or descriptor");

	ble_gattc_read(gc->conn_handle, handle, onGATTRead, gc);

	gc->isRead = true;
}


//@@ mutex on queue
void xs_gattclient_readValue(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	BLEGATTCharacteristicValue value = gc->values;
	gc->values = value->next;

	xsmcSetArrayBuffer(xsResult, value->payload, value->byteLength);
	xsSlot tmp;
	xsmcSetInteger(tmp, value->val_handle);
	xsmcSet(xsResult, xsID_handle, tmp); 
	c_free(value);
}

static int onGATTWritten(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
	GATTClient gc = arg;

	gattClientExecuted(gc, error->status);

	return 0;
}

void xs_gattclient_write(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	gc->request = xsmcToReference(xsArg(0));
	xsDestructor destructor = xsGetHostDestructor(xsArg(1));
	uint16_t handle;
	uint8_t withResponse = 1;
	if (destructor == xs_gattclientcharacteristic_destructor) {
		GATTClientCharacteristic gcc = (GATTClientCharacteristic)xsmcGetHostChunk(xsArg(1)); 
		handle = gcc->chr.val_handle;
		if ((gcc->chr.properties & BLE_GATT_CHR_PROP_WRITE_NO_RSP) && xsmcHas(xsArg(3), xsID_response)) {
			xsSlot tmp;
			xsmcGet(tmp, xsArg(3), xsID_response);
			withResponse = xsmcTest(tmp);		// options.response false for no-response request (default is true)
		}
	}
	else if (destructor == xs_gattclientdescriptor_destructor)
		handle = ((struct ble_gatt_dsc *)xsmcGetHostChunk(xsArg(1)))->handle;
	else
		xsUnknownError("not characteristic or descriptor");

	void *data;
	xsUnsignedValue count;

	xsmcGetBufferReadable(xsArg(2), &data, &count);
	if (withResponse)
		ble_gattc_write_flat(gc->conn_handle, handle, data, count, onGATTWritten, gc);
	else {
		ble_gattc_write_no_rsp_flat(gc->conn_handle, handle, data, count);
		gattClientExecuted(gc, 0);
	}
}

void xs_gattclient_get_maximumWrite(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	xsmcSetInteger(xsResult, gc->mtu ? (gc->mtu - 3) : 0);
}

// could save some bytes by only storing used portion of UUID (16 and 32 bits)
void xs_gattclient_store(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	xsDestructor destructor = xsGetHostDestructor(xsArg(0));
	if (xs_gattclientservice_destructor == destructor) {
		uint8_t *result = xsmcSetArrayBuffer(xsResult, C_NULL, sizeof(uint16_t) + sizeof(struct ble_gatt_svc));
		*(uint16_t *)result = 0x7370;
		c_memmove(result + sizeof(uint16_t), xsmcGetHostChunk(xsArg(0)), sizeof(struct ble_gatt_svc));
	}
	else if (xs_gattclientcharacteristic_destructor == destructor) {
		uint8_t *result = xsmcSetArrayBuffer(xsResult, C_NULL, sizeof(uint16_t) + sizeof(GATTClientCharacteristicRecord));
		*(uint16_t *)result = 0x6863;
		c_memmove(result + sizeof(uint16_t), xsmcGetHostChunk(xsArg(0)), sizeof(GATTClientCharacteristicRecord));
	}
	else if (xs_gattclientdescriptor_destructor == destructor) {
		uint8_t *result = xsmcSetArrayBuffer(xsResult, C_NULL, sizeof(uint16_t) + sizeof(struct ble_gatt_dsc));
		*(uint16_t *)result = 0x7364;
		c_memmove(result + sizeof(uint16_t), xsmcGetHostChunk(xsArg(0)), sizeof(struct ble_gatt_dsc));
	}
	else
		xsUnknownError("unrecognized");
}

void xs_gattclient_restore(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	void *data;
	xsUnsignedValue count;

	xsmcGetBufferReadable(xsArg(0), &data, &count);
	if (count <= 2)
		goto bad;
	switch (*(uint16_t *)data) {
		case 0x7370:
			if (count != (sizeof(uint16_t) + sizeof(struct ble_gatt_svc)))
				goto bad;
			xsResult = xsNewHostInstance(xsArg(1));
			break;
		case 0x6863:
			if (count != (sizeof(uint16_t) + sizeof(GATTClientCharacteristicRecord)))
				goto bad;
			xsResult = xsNewHostInstance(xsArg(2));
			break;
		case 0x7364:
			if (count != (sizeof(uint16_t) + sizeof(struct ble_gatt_dsc)))
				goto bad;
			xsResult = xsNewHostInstance(xsArg(3));
			break;
		default:
			goto bad;
	}

	void *buffer = xsmcSetHostChunk(xsResult, C_NULL, count - sizeof(uint16_t));
	xsmcGetBufferReadable(xsArg(0), &data, &count);
	c_memmove(buffer, sizeof(uint16_t) + (uint8_t *)data, count - sizeof(uint16_t));
	return;

bad:
	xsUnknownError("bad data");
}

void xs_gattclient_getCCCD(xsMachine *the)
{
	GATTClientCharacteristic gcc = xsmcGetHostChunkValidate(xsArg(0), xs_gattclientcharacteristic_destructor);
	struct ble_gatt_dsc dsc = {
		.handle = gcc->cccdHandle,
		.uuid.u16 = BLE_UUID16_INIT(0x2902)
	};
	if (!dsc.handle)
		return;
	
	xsResult = xsNewHostInstance(xsArg(1));
	xsmcSetHostChunk(xsResult, &dsc, sizeof(dsc));
}

void xs_gattclient_replyToPasskey(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	struct ble_sm_io pkey = {0};
	const char *action = xsmcToString(xsArg(0));

	if (0 == c_strcmp(action, "input")) {
		pkey.action = BLE_SM_IOACT_INPUT;
		if (xsmcArgc > 1)
			pkey.passkey = (uint32_t)xsmcToNumber(xsArg(1));
		else
			pkey.passkey = 0;
	}
	else if (0 == c_strcmp(action, "display")) {
		pkey.action = BLE_SM_IOACT_DISP;
		pkey.passkey = (uint32_t)xsmcToNumber(xsArg(1));
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

	int err = ble_sm_inject_io(gc->conn_handle, &pkey);
	if (err)
		xsUnknownError("failed");
}

void xs_gattclientservice_destructor(void *data) 
{
}

void xs_gattclientservice_get_uuid(xsMachine *the)
{
	struct ble_gatt_svc *svc = xsmcGetHostChunkValidate(xsThis, xs_gattclientservice_destructor);
	char buffer[BLE_UUID_STR_LEN];
	ble_uuid_to_str(&svc->uuid.u, buffer);
	xsmcSetString(xsResult, ('x' == buffer[1]) ? buffer + 2 : buffer);
}

void xs_gattclientcharacteristic_destructor(void *)
{
}

void xs_gattclientcharacteristic_get_uuid(xsMachine *the)
{
	GATTClientCharacteristic gcc = xsmcGetHostChunkValidate(xsThis, xs_gattclientcharacteristic_destructor);
	char buffer[BLE_UUID_STR_LEN];
	ble_uuid_to_str(&gcc->chr.uuid.u, buffer);
	xsmcSetString(xsResult, ('x' == buffer[1]) ? buffer + 2 : buffer);
}

void xs_gattclientcharacteristic_get_handle(xsMachine *the)
{
	GATTClientCharacteristic gcc = xsmcGetHostChunkValidate(xsThis, xs_gattclientcharacteristic_destructor);
	xsmcSetInteger(xsResult, gcc->chr.val_handle);
}

void xs_gattclientcharacteristic_get_properties(xsMachine *the)
{
	GATTClientCharacteristic gcc = xsmcGetHostChunkValidate(xsThis, xs_gattclientcharacteristic_destructor);
	xsmcSetInteger(xsResult, gcc->chr.properties);
}

void xs_gattclientdescriptor_destructor(void *)
{
}

void xs_gattclientdescriptor_get_uuid(xsMachine *the)
{
	struct ble_gatt_dsc *dsc = xsmcGetHostChunkValidate(xsThis, xs_gattclientdescriptor_destructor);
	char buffer[BLE_UUID_STR_LEN];
	ble_uuid_to_str(&dsc->uuid.u, buffer);
	xsmcSetString(xsResult, ('x' == buffer[1]) ? buffer + 2 : buffer);
}

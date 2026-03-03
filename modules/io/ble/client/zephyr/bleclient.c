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
	- Security
	- Fully deinitialize
	- Limit number of advertising packets in queue (can grow until memory is exhausted)
*/

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"

#include "builtinCommon.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>


#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <string.h>

static int bt_uuid_from_str(struct bt_uuid *uuid, const char *str);

static uint8_t gBLEInititalized = 0;

static void getUUIDList(xsMachine *the, xsSlot *items, int *length, struct bt_uuid_128 **uuids)
{
	xsSlot tmp;
	xsmcGet(tmp, *items, xsID_length);
	*length = xsmcToInteger(tmp);
	*uuids = c_malloc(sizeof(struct bt_uuid_128) * *length);
	if (!*uuids)
		xsUnknownError("no memory");

	for (int i = 0; i < *length; i++) {
		xsmcGetIndex(tmp, *items, i);
		if (bt_uuid_from_str(&(*uuids)[i].uuid, xsmcToString(tmp)))
			xsUnknownError("invalid uuid");
	}
}

static uint8_t uuidInList(const struct bt_uuid_128 *uuid, const struct bt_uuid_128 *uuids, int length)
{
	uint8_t found = 0;
	for (int i = 0; i < length && !found; i++)
		found |= 0 == bt_uuid_cmp(&uuid->uuid, &uuids[i].uuid);
	
	return found;
}


struct AdvertisingPacketRecord {
	struct AdvertisingPacketRecord	*next;
	bt_addr_le_t			address;
	int8_t					rssi;
	int						byteLength;
	uint8_t					payload[];
};
typedef struct AdvertisingPacketRecord AdvertisingPacketRecord;
typedef struct AdvertisingPacketRecord *AdvertisingPacket;

struct ServiceFilterRecord {
	struct ServiceFilterRecord	*next;
	struct bt_uuid_128			uuid;
};
typedef struct ServiceFilterRecord ServiceFilterRecord;
typedef struct ServiceFilterRecord *ServiceFilter;

typedef struct {
	xsMachine	*the;
	xsSlot		obj;
	xsSlot		*onReadable;
	xsSlot		*onError;
	xsSlot		*advertisementConstructor;

	struct bt_le_scan_cb cbs;

	struct AdvertisingPacketRecord	*packets;

	int			error;
	uint8_t		started;
	uint8_t		inflight;
	uint8_t		closed;
	uint8_t		filterMatch;
	
	struct bt_uuid_128	*serviceFilters;
	int				serviceFiltersLength;
} BLEScannerRecord, *BLEScanner;

static void gap_scan_found(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf);
static void deliverScanReadable(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static BLEScanner gScanner;

void xs_gapclient_destructor(void *data)
{
	BLEScanner scan = data;
	if (!data)
		return;

	scan->closed = true;

	if (scan == gScanner)
		gScanner = C_NULL;

	if (scan->started)
		bt_le_scan_stop();

	if (scan->cbs.recv)
		bt_le_scan_cb_unregister(&scan->cbs);

	while (scan->packets) {
		void *next = scan->packets->next;
		c_free(scan->packets);
		scan->packets = next;
	}

	if (scan->serviceFilters)
		c_free(scan->serviceFilters);

	if (!scan->inflight)
		c_free(scan);

//@@	if (0 == --gBLEInititalized)
//@@		bt_disable();
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

void xs_gapclient(xsMachine *the)
{
	if (gScanner)
		xsRangeError("only one");

	xsSlot *onReadable = builtinGetCallback(the, xsID_onReadable);
	xsSlot *onError = builtinGetCallback(the, xsID_onError);

	builtinInitializeTarget(the);

	BLEScanner scan = c_calloc(1, sizeof(BLEScannerRecord));
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

	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_services)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_services);
		getUUIDList(the, &xsVar(0), &scan->serviceFiltersLength, &
		scan->serviceFilters);
	}

	if (0 == gBLEInititalized) {
		int err = bt_enable(NULL);
		xsLog("bt_enable err = %d\n", err);
		if (err < 0)
			xsUnknownError("BT enable fail");
	}
	gBLEInititalized++;

	scan->cbs.recv = gap_scan_found;
	if (bt_le_scan_cb_register(&scan->cbs) < 0)
		xsUnknownError("register callback fail");

	//@@ options - passive / active
	if (bt_le_scan_start(BT_LE_SCAN_ACTIVE, C_NULL) < 0)
		xsUnknownError("scan start fail");

	scan->started = 1;
	gScanner = scan;
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

	builtinCriticalSectionBegin();
	AdvertisingPacket packet = scan->packets;
	scan->packets = packet->next;
	builtinCriticalSectionEnd();

	xsSlot tmp;
	xsmcSetInteger(tmp, packet->byteLength);
	xsResult = xsNewFunction1(xsReference(scan->advertisementConstructor), tmp);
	c_memmove(xsmcToArrayBuffer(xsResult), packet->payload, packet->byteLength);

	char address[24];
	snprintf(address, sizeof(address), "%02X:%02X:%02X:%02X:%02X:%02X/%02X",
		packet->address.a.val[5], packet->address.a.val[4], packet->address.a.val[3],
		packet->address.a.val[2], packet->address.a.val[1], packet->address.a.val[0],
		packet->address.type);
	 xsmcVars(1);
	 xsmcSetString(xsVar(0), address);
	 xsmcSet(xsResult, xsID_address, xsVar(0));
	 if (BT_GAP_RSSI_INVALID != packet->rssi) {
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

void deliverScanReadable(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	BLEScanner scan = refcon;
	
	if (scan->closed) {
		c_free(scan);
		return;
	}

	builtinCriticalSectionBegin();
		scan->inflight = false;
		int count = 0;
		for (AdvertisingPacket packet = scan->packets; C_NULL != packet; packet = packet->next)
			count +=1;
	builtinCriticalSectionEnd();

	xsBeginHost(the);
		xsmcSetInteger(xsResult, count);
		xsCallFunction1(xsReference(scan->onReadable), scan->obj, xsResult);
	xsEndHost(the);
}

static bool scanFilter(struct bt_data *data, void *user_data)
{
	BLEScanner scan = user_data;

	switch (data->type) {
		case BT_DATA_UUID16_SOME:
		case BT_DATA_UUID16_ALL:
			for (int i = 0; i < scan->serviceFiltersLength; i++) {
			 	for (int j = 0; j < (data->data_len >> 1); j++) {
					struct bt_uuid_16 uuid = {
						.uuid.type = BT_UUID_TYPE_16,
						.val = sys_get_le16(&data->data[j << 1])
					};
					scan->filterMatch |= 0 == bt_uuid_cmp(&uuid.uuid, &scan->serviceFilters[i].uuid);
				}
			}
			break;

		case BT_DATA_UUID32_SOME:
		case BT_DATA_UUID32_ALL:
			for (int i = 0; i < scan->serviceFiltersLength; i++) {
			 	for (int j = 0; j < (data->data_len >> 2); j++) {
					struct bt_uuid_32 uuid = {
						.uuid.type = BT_UUID_TYPE_32,
						.val = sys_get_le32(&data->data[j << 2])
					};
					scan->filterMatch |= 0 == bt_uuid_cmp(&uuid.uuid, &scan->serviceFilters[i].uuid);
				}
			}
			break;

		case BT_DATA_UUID128_SOME:
		case BT_DATA_UUID128_ALL:
			for (int i = 0; i < scan->serviceFiltersLength; i++) {
			 	for (int j = 0; j < (data->data_len >> 4); j++) {
					struct bt_uuid_128 uuid = {
						.uuid.type = BT_UUID_TYPE_128,
					};
					c_memcpy(&uuid.val, &data->data[j << 4], 16);					scan->filterMatch |= 0 == bt_uuid_cmp(&uuid.uuid, &scan->serviceFilters[i].uuid);
				}
			}
			break;
	}

	return !scan->filterMatch;
}

void gap_scan_found(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	BLEScanner scan = gScanner;

	if (!scan || scan->closed)
		return;

	if (scan->serviceFiltersLength) {
		struct net_buf_simple_state state;
		net_buf_simple_save(buf, &state);

		scan->filterMatch = 0;
		bt_data_parse(buf, scanFilter, scan);
		if (!scan->filterMatch)
			return;

		net_buf_simple_restore(buf, &state);
	}

	uint8_t notify = 0;
	AdvertisingPacket packet = (AdvertisingPacket)c_malloc(sizeof(AdvertisingPacketRecord) + buf->len);
	packet->next = C_NULL;
	packet->address = *(info->addr);
	packet->byteLength = buf->len;
	c_memmove(packet->payload, buf->data, buf->len);
	packet->rssi = info->rssi;
	builtinCriticalSectionBegin();
		notify = !scan->inflight; 
		scan->inflight = true;
		
		AdvertisingPacket *walker = &scan->packets;
		while (*walker)
			walker = &(*walker)->next;
		*walker = packet;
	builtinCriticalSectionEnd();
	if (notify && scan->onReadable)
		modMessagePostToMachine(scan->the, C_NULL, 0, deliverScanReadable, scan);
}

struct BLEGATTCharacteristicValueRecord {
	struct BLEGATTCharacteristicValueRecord *next;
	uint16_t	val_handle;
	
	uint16_t	byteLength;
	uint8_t		payload[];
};
typedef struct BLEGATTCharacteristicValueRecord BLEGATTCharacteristicValueRecord;
typedef struct BLEGATTCharacteristicValueRecord *BLEGATTCharacteristicValue;

struct BLEGATTSubscriptionRecord {
	struct BLEGATTSubscriptionRecord *next;
	struct bt_gatt_subscribe_params params;
};
typedef struct BLEGATTSubscriptionRecord BLEGATTSubscriptionRecord;
typedef struct BLEGATTSubscriptionRecord *BLEGATTSubscription;

struct ble_gatt_svc {
	struct bt_uuid_128	uuid;
	uint16_t start_handle;
	uint16_t end_handle;
};

struct ble_gatt_chr {
	struct bt_uuid_128	uuid;
	uint8_t	properties;
	uint16_t def_handle;
	uint16_t val_handle;
};

struct ble_gatt_dsc {
	struct bt_uuid_128	uuid;
	uint16_t	handle;
};

typedef struct {
	void			*next;
	xsMachine	*the;
	xsSlot		obj;
	
	xsSlot		*onReadable;
	xsSlot		*onError;
	xsSlot		*onSecured;
	xsSlot		*onPasskey;

	bt_addr_le_t	address;

	struct bt_conn *conn;

	uint8_t		connected;
	uint8_t		connecting;
	uint16_t	mtu;

	int			result;
	xsSlot		*request;
	xsSlot		*responsePrototype;

	struct ble_gatt_svc	requestSvc;

	struct bt_gatt_discover_params discoverParams;

	uint8_t			isPrimaryServices;
	int				primaryServicesUUIDsLength;
	struct bt_uuid_128	*primaryServicesUUIDs;
	struct ble_gatt_svc 	*primaryServices;
	int				primaryServicesLength;

	uint8_t			isCharacteristics;
	int				characteristicsUUIDsLength;
	struct bt_uuid_128	*characteristicsUUIDs;
	struct ble_gatt_chr 	*characteristics;
	int				characteristicsLength;
	
	uint8_t			isDescriptors;
	int				descriptorsUUIDsLength;
	struct bt_uuid_128	*descriptorsUUIDs;
	struct ble_gatt_dsc 	*descriptors;
	int				descriptorsLength;	
	
	uint8_t			isRead;
	uint16_t		readResultByteLength;
	void			*readResult;
	struct bt_gatt_read_params readParams;
	
	uint8_t			isWrite;
	struct bt_gatt_write_params writeParams;
	void				*writeBuffer;

	uint8_t			isClose;

	BLEGATTCharacteristicValue		values;
	uint8_t							onReadableInFlight;

	uint8_t 		secure;
	uint8_t			immediate;

	struct bt_conn_cb connectionCallbacks;
	struct bt_gatt_exchange_params mtu_exchange_params;

	BLEGATTSubscription		subscriptions;
} GATTClientRecord, *GATTClient;

static GATTClient gClients;

typedef struct {
	struct ble_gatt_chr	chr;
	uint16_t		svc_start_handle;
	uint16_t		svc_end_handle;
	uint16_t		cccdHandle;
} GATTClientCharacteristicRecord, *GATTClientCharacteristic;

static GATTClient findClient(struct bt_conn *conn);
static void onGATTConnected(struct bt_conn *conn, uint8_t err);
static void onGATTDisconnected(struct bt_conn *conn, uint8_t reason);
static void gattClientExecuted(GATTClient gc, int result);

void xs_gattclient_destructor(void *data)
{
	GATTClient gc = data;
	if (!gc)
		return;

	while (gc->subscriptions) {
		BLEGATTSubscription next = gc->subscriptions->next;
		c_free(gc->subscriptions);
		gc->subscriptions = next;
	}

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
	bt_addr_le_t address;
	int mtu = 0;
	uint8_t secure = 0, authenticate = 0, bond = 0, display = 0, keyboard = 0, immediate = 0;

	xsmcVars(2);

	if (xsmcHas(xsArg(0), xsID_mtu)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_mtu);
		mtu = xsmcToInteger(xsVar(0));
		if (mtu > 247)		//@@ constant?
			mtu = 247;
		else if (mtu < 23)	//@@ constant?
			mtu = 23;
	}

	xsmcGet(xsVar(0), xsArg(0), xsID_address);
	if (sscanf(xsmcToString(xsVar(0)),
		"%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx/%2hhx",
		&address.a.val[5], &address.a.val[4], &address.a.val[3],
		&address.a.val[2], &address.a.val[1], &address.a.val[0],
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

	gc->connectionCallbacks.connected = onGATTConnected;
	gc->connectionCallbacks.disconnected = onGATTDisconnected;
	bt_conn_cb_register(&gc->connectionCallbacks);

#if 0
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
#endif

	gc->next = gClients;
	gClients = gc;
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
			if (0 != bt_conn_disconnect(gc->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN))
				gattClientExecuted(gc, 0);
		}
		else
			gattClientExecuted(gc, 0);
	}
	else {
		if (gc->connecting)
			bt_conn_disconnect(gc->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
}

static GATTClient findClient(struct bt_conn *conn)
{
	for (GATTClient gc = gClients; C_NULL != gc; gc = gc->next) {
		if (gc->conn == conn)
			return gc;
	}

	printk("Can't find BT Connection!");
	return C_NULL;
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

				static struct bt_uuid_16 uuid_declaration = BT_UUID_INIT_16(0x2902);
				if (0 == bt_uuid_cmp(&dsc->uuid.uuid, &uuid_declaration.uuid)) {
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

	gc->isWrite = false;
	if (gc->writeBuffer) {
		c_free(gc->writeBuffer);
		gc->writeBuffer = C_NULL;
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
#if 0
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
		xsmcSetInteger(tmp, desc.sec_state.key_size);			// bt_conn_enc_key_size
		xsmcSet(xsResult, xsID_keySize, tmp);
		xsCallFunction1(xsReference(gc->onSecured), gc->obj, xsResult);
	xsEndHost(the);
#endif
}

static void deliverOnPasskey(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	GATTClient gc = refcon;
#if 0
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
#endif
}

void onGATTMCUExchanged(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	GATTClient gc = findClient(conn);

	if (0 == err)
		gc->mtu = bt_gatt_get_mtu(conn);

	gattClientExecuted(gc, 0);

//@@	if (gc->secure && gc->immediate)
//@@		ble_gap_security_initiate(conn_handle);
}

void onGATTConnected(struct bt_conn *conn, uint8_t err)
{
	GATTClient gc = findClient(conn);

	gc->connecting = 0;

	if (err) {
		gattClientExecuted(gc, err);
		return;
	}

	gc->connected = 1;

	gc->mtu = bt_gatt_get_mtu(conn);
	gc->mtu_exchange_params.func = onGATTMCUExchanged;
	bt_gatt_exchange_mtu(conn, &gc->mtu_exchange_params);
}

void onGATTDisconnected(struct bt_conn *conn, uint8_t reason)
{
	GATTClient gc = findClient(conn);

	gc->connecting = 0;
	gc->connected = 0;
	if (gc->isClose)
		gattClientExecuted(gc, 0);
	else if (gc->onError)
		modMessagePostToMachine(gc->the, C_NULL, 0, deliverOnError, gc);
}

void xs_gattclient_connect(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	gc->request = xsmcToReference(xsArg(0));

	// if (gc->mtu)
	// 	bt_att_set_max_mtu(gc->mtu);

	int result = bt_conn_le_create(&gc->address, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &gc->conn);
	if (result < 0) {
		xsLog("connection failed %d\n", result);
		xsUnknownError("connection failed");
	}

	gc->connecting = 1;
}

static uint8_t onGATTServiceDiscovered(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params)
{
	GATTClient gc = findClient(conn);

	if (C_NULL == attr) {
		gattClientExecuted(gc, 0);
		return BT_GATT_ITER_STOP;
	}

	struct bt_gatt_service_val *gatt_service = (struct bt_gatt_service_val *)attr->user_data;

	if (gc->primaryServicesUUIDsLength &&
			!uuidInList((const struct bt_uuid_128 *)gatt_service->uuid, gc->primaryServicesUUIDs, gc->primaryServicesUUIDsLength))
		return 0;

	gc->primaryServices = c_realloc(gc->primaryServices, (gc->primaryServicesLength + 1) * sizeof(struct ble_gatt_svc));
	if (C_NULL == gc->primaryServices) {
		gattClientExecuted(gc, 1);
		return BT_GATT_ITER_STOP;
	}
	struct ble_gatt_svc svc = {
		.start_handle = attr->handle,
		.end_handle = gatt_service->end_handle,
		.uuid = *(struct bt_uuid_128 *)gatt_service->uuid		// over copies in most cases
	};

	gc->primaryServices[gc->primaryServicesLength++] = svc;

	return BT_GATT_ITER_CONTINUE;
}

void xs_gattclient_getPrimaryServices(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);

	gc->request = xsmcToReference(xsArg(0));
	gc->responsePrototype = xsmcToReference(xsArg(2));

	gc->isPrimaryServices = true;
	gc->primaryServices = C_NULL;
	gc->primaryServicesLength = 0;

	gc->discoverParams.uuid = C_NULL;
	gc->discoverParams.func = onGATTServiceDiscovered;
	gc->discoverParams.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	gc->discoverParams.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	gc->discoverParams.type = BT_GATT_DISCOVER_PRIMARY;

	if (xsmcTest(xsArg(1)))
		getUUIDList(the, &xsArg(1), &gc->primaryServicesUUIDsLength, &gc->primaryServicesUUIDs);

	if (1 == gc->primaryServicesUUIDsLength) {
		gc->primaryServicesUUIDsLength = 0;
		gc->discoverParams.uuid = &gc->primaryServicesUUIDs[0].uuid;
	}

	int err = bt_gatt_discover(gc->conn, &gc->discoverParams);
	if (err)
		gattClientExecuted(gc, err);
}

static uint8_t onGATTCharacteristicDiscovered(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params)
{
	GATTClient gc = findClient(conn);

	if (C_NULL == attr) {
		gattClientExecuted(gc, 0);
		return BT_GATT_ITER_STOP;
	}

	struct bt_gatt_chrc *gatt_chrc = (struct bt_gatt_chrc *)attr->user_data;

	if (gc->characteristicsUUIDsLength &&
			!uuidInList((const struct bt_uuid_128 *)gatt_chrc->uuid, gc->characteristicsUUIDs, gc->characteristicsUUIDsLength))
		return 0;

	gc->characteristics = c_realloc(gc->characteristics, (gc->characteristicsLength + 1) * sizeof(struct ble_gatt_chr));
	if (C_NULL == gc->characteristics) {
		gattClientExecuted(gc, 1);
		return BT_GATT_ITER_STOP;
	}
	struct ble_gatt_chr chr = {
		.def_handle = attr->handle,
		.val_handle = gatt_chrc->value_handle,
		.properties = gatt_chrc->properties,
		.uuid = *(struct bt_uuid_128 *)gatt_chrc->uuid		// over copies in most cases
	};

	gc->characteristics[gc->characteristicsLength++] = chr;

	return BT_GATT_ITER_CONTINUE;
}

void xs_gattclient_getCharacteristics(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);

	gc->request = xsmcToReference(xsArg(0));
	gc->requestSvc = *(struct ble_gatt_svc *)xsmcGetHostChunkValidate(xsArg(1), xs_gattclientservice_destructor);
	gc->responsePrototype = xsmcToReference(xsArg(3));
	
	gc->isCharacteristics = true;
	gc->characteristics = C_NULL;
	gc->characteristicsLength = 0;

	gc->discoverParams.uuid = C_NULL;
	gc->discoverParams.func = onGATTCharacteristicDiscovered;
	gc->discoverParams.start_handle = gc->requestSvc.start_handle;
	gc->discoverParams.end_handle = gc->requestSvc.end_handle;
	gc->discoverParams.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	if (xsmcTest(xsArg(2)))
		getUUIDList(the, &xsArg(2), &gc->characteristicsUUIDsLength, &gc->characteristicsUUIDs);

	if (1 == gc->characteristicsUUIDsLength) {
		gc->characteristicsUUIDsLength = 0;
		gc->discoverParams.uuid = &gc->characteristicsUUIDs[0].uuid;
	}

	int err = bt_gatt_discover(gc->conn, &gc->discoverParams);
	if (err)
		gattClientExecuted(gc, err);
}

static uint8_t onGATTDescriptorDiscovered(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params)
{
	GATTClient gc = findClient(conn);
	static struct bt_uuid_16 uuid_declaration = BT_UUID_INIT_16(0x2803);

	if ((C_NULL == attr) || (0 == bt_uuid_cmp(attr->uuid, &uuid_declaration.uuid))) {
		gattClientExecuted(gc, 0);
		return BT_GATT_ITER_STOP;
	}

	if (gc->descriptorsUUIDsLength &&
			!uuidInList((const struct bt_uuid_128 *)attr->uuid, gc->descriptorsUUIDs, gc->descriptorsUUIDsLength))
		return BT_GATT_ITER_CONTINUE;

	gc->descriptors = c_realloc(gc->descriptors, (gc->descriptorsLength + 1) * sizeof(struct ble_gatt_dsc));
	if (C_NULL == gc->descriptors) {
		gattClientExecuted(gc, 1);
		return BT_GATT_ITER_STOP;
	}
	struct ble_gatt_dsc dsc = {
		.handle = attr->handle,
		.uuid = *(struct bt_uuid_128 *)attr->uuid		// over copies in most cases
	};

	gc->descriptors[gc->descriptorsLength++] = dsc;

	return BT_GATT_ITER_CONTINUE;
}

void xs_gattclient_getDescriptors(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);

	gc->request = xsmcToReference(xsArg(0));
	GATTClientCharacteristic gcc = (GATTClientCharacteristic)xsmcGetHostChunkValidate(xsArg(1), xs_gattclientcharacteristic_destructor);
	gc->responsePrototype = xsmcToReference(xsArg(3));
	
	gc->isDescriptors = true;
	gc->descriptors = C_NULL;
	gc->descriptorsLength = 0;

	gc->discoverParams.uuid = C_NULL;
	gc->discoverParams.func = onGATTDescriptorDiscovered;
	gc->discoverParams.start_handle = gcc->chr.val_handle;		//@@ +1 ?
	gc->discoverParams.end_handle = gcc->svc_end_handle;
	gc->discoverParams.type = BT_GATT_DISCOVER_DESCRIPTOR;

	if (gc->discoverParams.start_handle == gc->discoverParams.end_handle) {
		gattClientExecuted(gc, 0);
		return;
	}

	if (xsmcTest(xsArg(2))) {
		getUUIDList(the, &xsArg(2), &gc->descriptorsUUIDsLength, &gc->descriptorsUUIDs);

		if (1 == gc->descriptorsUUIDsLength) {
			gc->descriptorsUUIDsLength = 0;
			gc->discoverParams.uuid = &gc->descriptorsUUIDs[0].uuid;
		}
	}

	int err = bt_gatt_discover(gc->conn, &gc->discoverParams);
	if (err)
		gattClientExecuted(gc, err);
}

static uint8_t onGATTRead(struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params, const void *data, uint16_t length)
{
	GATTClient gc = findClient(conn);

	if (0 == err) {
		gc->readResultByteLength = length;
		gc->readResult = c_malloc(length);
		if (C_NULL == gc->readResult)
			err = ~0;			//@@ no memory
		else
			c_memcpy(gc->readResult, data, length);
	}

	gattClientExecuted(gc, err);

	return BT_GATT_ITER_STOP;
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

	c_memset(&gc->readParams,0, sizeof(gc->readParams));
	gc->readParams.func = onGATTRead;
	gc->readParams.handle_count = 1;
	gc->readParams.single.handle = handle;
	gc->readParams.single.offset = 0;
	gc->isRead = true;
	
	int err = bt_gatt_read(gc->conn, &gc->readParams);
	if (err < 0)
		gattClientExecuted(gc, err);
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

static void onGATTWritten(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	GATTClient gc = findClient(conn);

	gattClientExecuted(gc, err);
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
		if ((gcc->chr.properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) && xsmcHas(xsArg(3), xsID_response)) {
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

	if (withResponse) {
		gc->writeBuffer = c_malloc(count);
		if (!gc->writeBuffer)
			xsUnknownError("no memory");
		c_memcpy(gc->writeBuffer, data, count);

		c_memset(&gc->writeParams,0, sizeof(gc->writeParams));
		gc->writeParams.func = onGATTWritten;
		gc->writeParams.handle = handle;
		gc->writeParams.offset = 0;
		gc->writeParams.data = gc->writeBuffer;
		gc->writeParams.length = count;
		gc->isWrite = true;
		bt_gatt_write(gc->conn, &gc->writeParams);
	}
	else {
		int result = bt_gatt_write_without_response(gc->conn, handle, data, count, false);
		gattClientExecuted(gc, result);
	}
}

static uint8_t onGATTNotify(struct bt_conn *conn, struct bt_gatt_subscribe_params *params, const void *data, uint16_t length)
{
	GATTClient gc = findClient(conn);		//@@ NULL on disconnect??
	BLEGATTSubscription subscription = (BLEGATTSubscription)(((uint8_t *)params) - offsetof(BLEGATTSubscriptionRecord, params));

	if (C_NULL == data) {		// unsubscribe
		if (subscription == gc->subscriptions)
			gc->subscriptions = subscription->next;
		else {
			for (BLEGATTSubscription walker = gc->subscriptions; NULL != walker; walker = walker->next) {
				if (walker->next == subscription) {
					walker->next = subscription->next;
					break;
				}
			}
		}
		return BT_GATT_ITER_STOP;
	}

	BLEGATTCharacteristicValue value = c_malloc(sizeof(BLEGATTCharacteristicValueRecord) + length);
	if (C_NULL == value)
		return BT_GATT_ITER_CONTINUE;

	value->next = C_NULL;
	value->byteLength = length;
	value->val_handle = params->value_handle;
	c_memcpy(value->payload, data, length);

	//@@ mutex
	if (C_NULL == gc->values)
		gc->values = value;
	else {
		for (BLEGATTCharacteristicValue walker = gc->values; C_NULL != walker; walker = walker->next) {
			if (C_NULL == walker->next)
				walker->next = value;
				break;
		}
	}

	if (!gc->onReadableInFlight && gc->onReadable) {
		gc->onReadableInFlight = 1;
		modMessagePostToMachine(gc->the, C_NULL, 0, deliverOnReadable, gc);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void onGATTSubscribed(struct bt_conn *conn, uint8_t err, struct bt_gatt_subscribe_params *params)
{
	GATTClient gc = findClient(conn);

	if (err)
		onGATTNotify(gc->conn, params, C_NULL, 0);		// remove subscription

	gattClientExecuted(gc, err);
}

void xs_gattclient_subscribe(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
	gc->request = xsmcToReference(xsArg(0));

	int action = xsmcToInteger(xsArg(2));
	GATTClientCharacteristic gcc = (GATTClientCharacteristic)xsmcGetHostChunk(xsArg(1)); 

	BLEGATTSubscription current;
	for (current = gc->subscriptions; NULL != current; current = current->next) {
		if (current->params.value_handle == gcc->chr.val_handle)
			break;
	}

	if (0 == action) {
		if (C_NULL == current)
			xsUnknownError("no subscribed");

		onGATTNotify(gc->conn, &current->params, C_NULL, 0);		// remove subscription

		int err = bt_gatt_unsubscribe(gc->conn, &current->params);
		if (err)
			xsUnknownError("failed");
	}
	else {
		if (current) 
			xsUnknownError("already subscribed");

		BLEGATTSubscription subscription = c_calloc(1, sizeof(BLEGATTSubscriptionRecord));
		if (!subscription)
			xsUnknownError("no memory");

		subscription->params.subscribe = onGATTSubscribed;
		subscription->params.notify = onGATTNotify;
		subscription->params.value = (1 == action) ? BT_GATT_CCC_NOTIFY : BT_GATT_CCC_INDICATE;
		subscription->params.value_handle = gcc->chr.val_handle;
		subscription->params.ccc_handle = gcc->cccdHandle;

		//@@ mutex
		subscription->next = gc->subscriptions;
		gc->subscriptions = subscription;

		int err = bt_gatt_subscribe(gc->conn, &subscription->params);
		if (err)
			xsUnknownError("failed");
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
	/* GATTClient gc = */ (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
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
	/* GATTClient gc = */ (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
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
	static struct bt_uuid_16 uuid_declaration = BT_UUID_INIT_16(0x2902);
	struct ble_gatt_dsc dsc = {
		.handle = gcc->cccdHandle,
	};
	c_memcpy((uint8_t *)&dsc.uuid, (uint8_t *)&uuid_declaration, sizeof(uuid_declaration));

	if (!dsc.handle)
		return;
	
	xsResult = xsNewHostInstance(xsArg(1));
	xsmcSetHostChunk(xsResult, &dsc, sizeof(dsc));
}

void xs_gattclient_replyToPasskey(xsMachine *the)
{
	GATTClient gc = (GATTClient)xsmcGetHostDataValidate(xsThis, (void *)&xsGATTClientHooks);
#if 0
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
#endif
}

void xs_gattclientservice_destructor(void *data) 
{
}

void xs_gattclientservice_get_uuid(xsMachine *the)
{
	struct ble_gatt_svc *svc = xsmcGetHostChunkValidate(xsThis, xs_gattclientservice_destructor);
	char buffer[BT_UUID_STR_LEN];
	bt_uuid_to_str(&svc->uuid.uuid, buffer, sizeof(buffer));
	xsmcSetString(xsResult, ('x' == buffer[1]) ? buffer + 2 : buffer);
}

void xs_gattclientcharacteristic_destructor(void *)
{
}

void xs_gattclientcharacteristic_get_uuid(xsMachine *the)
{
	GATTClientCharacteristic gcc = xsmcGetHostChunkValidate(xsThis, xs_gattclientcharacteristic_destructor);
	char buffer[BT_UUID_STR_LEN];
	bt_uuid_to_str(&gcc->chr.uuid.uuid, buffer, sizeof(buffer));
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
	char buffer[BT_UUID_STR_LEN];
	bt_uuid_to_str(&dsc->uuid.uuid, buffer, sizeof(buffer));
	xsmcSetString(xsResult, ('x' == buffer[1]) ? buffer + 2 : buffer);
}


// generated by Claude.AI on October 21, 2025
static int parse_uuid16_32(const char *str, struct bt_uuid *uuid, bool is_32bit)
{
	const char *hex = str;
	uint8_t buf[4];
	size_t len;

	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
		hex = str + 2;

	len = strlen(hex);
	if (len == 0 || len > (is_32bit ? 8 : 4))
		return -EINVAL;

	if (hex2bin(hex, len, buf, sizeof(buf)) != (len + 1) / 2)
		return -EINVAL;

	if (is_32bit) {
		struct bt_uuid_32 *uuid32 = (struct bt_uuid_32 *)uuid;
		uuid32->uuid.type = BT_UUID_TYPE_32;
		uuid32->val = sys_get_be32(buf);
	} else {
		struct bt_uuid_16 *uuid16 = (struct bt_uuid_16 *)uuid;
		uuid16->uuid.type = BT_UUID_TYPE_16;
		uuid16->val = (len <= 2) ? buf[0] : sys_get_be16(buf);
	}

	return 0;
}

static int parse_uuid128(const char *str, struct bt_uuid *uuid)
{
	struct bt_uuid_128 *uuid128 = (struct bt_uuid_128 *)uuid;
	char hex[33];
	int j = 0;

	if (strlen(str) != 36 || str[8] != '-' || str[13] != '-' || 
		str[18] != '-' || str[23] != '-') {
		return -EINVAL;
	}

	uuid128->uuid.type = BT_UUID_TYPE_128;

	for (int i = 0; i < 36; i++) {
		if (str[i] != '-') {
			hex[j++] = str[i];
		}
	}
	hex[j] = '\0';

	if (hex2bin(hex, 32, uuid128->val, 16) != 16)
		return -EINVAL;

	for (int i = 0; i < 8; i++) {
		uint8_t tmp = uuid128->val[i];
		uuid128->val[i] = uuid128->val[15 - i];
		uuid128->val[15 - i] = tmp;
	}

	return 0;
}

int bt_uuid_from_str(struct bt_uuid *uuid, const char *str)
{
	size_t len = strlen(str);

	if (len == 36 && str[8] == '-')
		return parse_uuid128(str, uuid);

	if (len <= 10) {
		const char *hex = (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) ? str + 2 : str;
		return parse_uuid16_32(str, uuid, strlen(hex) > 4);
	}

	return -EINVAL;
}

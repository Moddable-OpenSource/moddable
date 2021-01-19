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
#include "mc.xs.h"
#include "xsHost.h"
#include "modBLE.h"
#include "modBLESM.h"

#define QAPI_USE_BLE
#include "qapi.h"

#include "mc.bleservices.c"

#pragma GCC diagnostic ignored "-Wswitch"	// disable warning on missing switch case values

#define LOG_GATTC 0
#if LOG_GATTC
	static void logGATTCEvent(qapi_BLE_GATT_Client_Event_Type_t event);
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
	static void logGAPEvent(qapi_BLE_GAP_LE_Event_Type_t event);
	#define LOG_GAP_EVENT(event) logGAPEvent(event)
	#define LOG_GAP_MSG(msg) modLog(msg)
#else
	#define LOG_GAP_EVENT(event)
	#define LOG_GAP_MSG(msg)
#endif

#define LOG_GAP_AUTHENTICATION 0
#if LOG_GAP_AUTHENTICATION
	static void logGAPAuthenticationEvent(qapi_BLE_GAP_LE_Authentication_Event_Type_t event);
	#define LOG_GAP_AUTHENTICATION_EVENT(event) logGAPAuthenticationEvent(event)
	#define LOG_GAP_AUTHENTICATION_MSG(msg) modLog(msg)
#else
	#define LOG_GAP_AUTHENTICATION_EVENT(event)
	#define LOG_GAP_AUTHENTICATION_MSG(msg)
#endif

enum {
	GATT_PROCEDURE_NONE = 0,
	GATT_PROCEDURE_DISCOVER_SERVICES,
	GATT_PROCEDURE_DISCOVER_CHARACTERISTICS,
	GATT_PROCEDURE_DISCOVER_DESCRIPTORS,
	GATT_PROCEDURE_READ_CHARACTERISTIC,
	GATT_PROCEDURE_ENABLE_CHARACTERISTIC_NOTIFICATIONS,
	GATT_PROCEDURE_DISABLE_CHARACTERISTIC_NOTIFICATIONS,
	GATT_PROCEDURE_READ_DESCRIPTOR
};

typedef struct {
	uint8_t what;
	uint32_t transaction_id;
	uint32_t refcon;
	xsSlot target;
} GATTProcedureRecord, *GATTProcedure;

typedef struct modBLEClientConnectionRecord modBLEClientConnectionRecord;
typedef modBLEClientConnectionRecord *modBLEClientConnection;

struct modBLEClientConnectionRecord {
	modBLEConnectionPart;
	xsSlot objClient;

	uint8_t bond;
	
	uint8_t Flags;
	uint8_t EncryptionKeySize;
	qapi_BLE_Encryption_Key_t IRK;
	qapi_BLE_Long_Term_Key_t LTK;
	qapi_BLE_Random_Number_t Rand;
	qapi_BLE_GAP_LE_Address_Type_t IdentityAddressType;
	qapi_BLE_BD_ADDR_t IdentityAddressBD_ADDR;
	qapi_BLE_GAP_LE_White_List_Entry_t WhiteListEntry;
	qapi_BLE_GAP_LE_Resolving_List_Entry_t ResolvingListEntry;
	uint16_t EDIV;

	uint32_t passkey;

	// char_name_table handles
	uint16_t handles[char_name_count];

	// pending procedure
	GATTProcedureRecord procedure;
};

typedef struct {
	xsMachine	*the;
	xsSlot		obj;

	uint32_t stackID;
	qapi_BLE_BD_ADDR_t localAddress;	
	
	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;
	uint16_t ioCapability;
	qapi_BLE_Encryption_Key_t ER;	// encryption root key
	qapi_BLE_Encryption_Key_t IR;	// identity root key
	qapi_BLE_Encryption_Key_t DHK;
	qapi_BLE_Encryption_Key_t IRK;
   	qapi_BLE_GAP_LE_Extended_Pairing_Capabilities_t pairingCapabilities;
	
	// client connections
	uint8_t scanning;
	qapi_BLE_GAP_LE_Connection_Parameters_t connectionParams;
	uint32_t scanInterval;
	uint32_t scanWindow;

	modBLEMessageQueueRecord discoveryQueue;
	modBLEMessageQueueRecord notificationQueue;
} modBLERecord, *modBLE;

typedef struct {
	uint16_t start;
	uint16_t end;
	qapi_BLE_GATT_UUID_t uuid;
} CharacteristicDiscoveryRequest;

typedef struct {
	uint32_t conn_id;
	uint16_t handle;
} DescriptorDiscoveryRequest;

typedef struct {
	uint32_t conn_id;
	uint16_t handle;
	uint8_t enable;
} CharacteristicNotificationRequest;

typedef struct {
	modBLEMessageQueueEntryPart;
	qapi_BLE_GAP_LE_Advertising_Report_Data_t disc;
	uint8_t data[1];
} deviceDiscoveryRecord;

typedef struct {
	modBLEMessageQueueEntryPart;
	qapi_BLE_GATT_Service_Information_t service;
	uint8_t completed;
} serviceSearchRecord;

typedef struct {
	modBLEMessageQueueEntryPart;
	uint8_t completed;
	uint16_t count;
	qapi_BLE_GATT_Characteristic_Descriptor_Information_t descriptorList[1];
} descriptorSearchRecord;

typedef struct {
	modBLEMessageQueueEntryPart;
	uint16_t count;
	qapi_BLE_GATT_Characteristic_Entry_t CharacteristicEntryList[1];
} characteristicSearchRecord;

typedef struct {
	modBLEMessageQueueEntryPart;
	qapi_BLE_GATT_Server_Notification_Data_t notification;
	uint8_t data[1];
} attributeNotificationRecord;

static void uuidToBuffer(qapi_BLE_GATT_UUID_t *uuid, uint8_t *buffer, uint16_t *length);
static void bufferToUUID(uint8_t *buffer, qapi_BLE_GATT_UUID_t *uuid, uint16_t length);
static uint8_t UUIDEqual(qapi_BLE_GATT_UUID_t *uuid1, qapi_BLE_GATT_UUID_t *uuid2);

static void readyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void setGATTProcedure(modBLEClientConnection connection, uint8_t what, xsSlot target, uint32_t transaction_id, uint32_t refcon);
static void completeProcedure(uint32_t conn_id, char *which);

static void QAPI_BLE_BTPSAPI GATT_Connection_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, uint32_t CallbackParameter);
static void QAPI_BLE_BTPSAPI GAP_LE_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GAP_LE_Event_Data_t *GAP_LE_Event_Data, uint32_t CallbackParameter);
static void QAPI_BLE_BTPSAPI GATT_Client_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Client_Event_Data_t *GATT_Client_Event_Data, uint32_t CallbackParameter);
static void QAPI_BLE_BTPSAPI GATT_Service_Discovery_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Service_Discovery_Event_Data_t *GATT_Service_Discovery_Event_Data, uint32_t CallbackParameter);
static void QAPI_BLE_BTPSAPI GATT_Descriptor_Discovery_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Service_Discovery_Event_Data_t *GATT_Service_Discovery_Event_Data, uint32_t CallbackParameter);

static void characteristicDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void deviceDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void descriptorDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void notificationEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void serviceDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void bondingRemovedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static modBLE gBLE = NULL;

uint32_t gBluetoothStackID = 0;

void xs_ble_client_initialize(xsMachine *the)
{
	qapi_BLE_HCI_DriverInformation_t HCI_DriverInformation;
	qapi_BLE_GAP_LE_Connection_Parameters_t *connectionParams;
	int result;
	
	if (NULL != gBLE)
		xsUnknownError("BLE already initialized");
	gBLE = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!gBLE)
		xsUnknownError("no memory");
	gBLE->the = the;
	gBLE->obj = xsThis;
	xsRemember(gBLE->obj);
	
	modBLEMessageQueueConfigure(&gBLE->notificationQueue, the, notificationEvent, NULL);

	// Set defaults (based on QCLI ble_demo.c)
	gBLE->scanWindow = 50;
	gBLE->scanInterval = 100;
	connectionParams = &gBLE->connectionParams;
	connectionParams->Connection_Interval_Min    = 50;
	connectionParams->Connection_Interval_Max    = 200;
	connectionParams->Minimum_Connection_Length  = 0;
	connectionParams->Maximum_Connection_Length  = 10000;
	connectionParams->Slave_Latency              = 0;
	connectionParams->Supervision_Timeout        = 20000;

	// Initialize platform Bluetooth modules
	QAPI_BLE_HCI_DRIVER_SET_COMM_INFORMATION(&HCI_DriverInformation, 1, 115200, QAPI_BLE_COMM_PROTOCOL_UART_E);
	result = qapi_BLE_BSC_Initialize(&HCI_DriverInformation, 0);
	if (result <= 0)
		xsUnknownError("BLE initialization failed");
	gBluetoothStackID = gBLE->stackID = result;
	
	result = qapi_BLE_GATT_Initialize(gBLE->stackID, QAPI_BLE_GATT_INITIALIZATION_FLAGS_SUPPORT_LE, GATT_Connection_Event_Callback, 0);
	if (result != 0)
		xsUnknownError("BLE GATT initialization failed");
		
	qapi_BLE_GAP_Query_Local_BD_ADDR(gBLE->stackID, &gBLE->localAddress);

	modMessagePostToMachine(the, NULL, 0, readyEvent, NULL);
}

void xs_ble_client_close(xsMachine *the)
{
	xsForget(gBLE->obj);
	xs_ble_client_destructor(gBLE);
}

void xs_ble_client_destructor(void *data)
{
	modBLE ble = data;
	if (!ble) return;
	
	modBLEWhitelistClear();
	qapi_BLE_GATT_Cleanup(ble->stackID);
	qapi_BLE_BSC_Shutdown(ble->stackID);
	modBLEConnection connection = modBLEConnectionGetFirst();
	while (connection != NULL) {
		modBLEConnection next = connection->next;
		if (kBLEConnectionTypeClient == connection->type) {
			xsMachine *the = connection->the;
			xsForget(connection->objConnection);
			modBLEConnectionRemove(connection);
		}
		connection = next;
	}
	
	modBLEMessageQueueEmpty(&ble->discoveryQueue);
	modBLEMessageQueueEmpty(&ble->notificationQueue);
	c_free(ble);
	gBluetoothStackID = 0;
	gBLE = NULL;
}

void xs_ble_client_set_local_privacy(xsMachine *the)
{
}

void xs_ble_client_start_scanning(xsMachine *the)
{
	uint8_t active = xsmcToBoolean(xsArg(0));
	uint8_t duplicates = xsmcToBoolean(xsArg(1));
	uint32_t interval = xsmcToInteger(xsArg(2));
	uint32_t window = xsmcToInteger(xsArg(3));
	uint16_t filterPolicy = xsmcToInteger(xsArg(4));
	uint32_t result;
	
	switch(filterPolicy) {
		case kBLEScanFilterPolicyWhitelist:
			filterPolicy = QAPI_BLE_FP_WHITE_LIST_E;
			break;
		case kBLEScanFilterNotResolvedDirected:
			filterPolicy = QAPI_BLE_FP_NO_WHITE_LIST_DIRECTED_RPA_E;
			break;
		case kBLEScanFilterWhitelistNotResolvedDirected:
			filterPolicy = QAPI_BLE_FP_WHITE_LIST_DIRECTED_RPA_E;
			break;
		default:
			filterPolicy = QAPI_BLE_FP_NO_FILTER_E;
			break;
	}

	gBLE->scanInterval = (uint32_t)(interval / 0.625);	// convert to 1 ms units
	gBLE->scanWindow = (uint32_t)(window / 0.625);

	modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, deviceDiscoveryEvent, NULL);

	result = qapi_BLE_GAP_LE_Perform_Scan(
		gBLE->stackID,
		active ? QAPI_BLE_ST_ACTIVE_E : QAPI_BLE_ST_PASSIVE_E,
		gBLE->scanInterval,
		gBLE->scanWindow,
		QAPI_BLE_LAT_PUBLIC_E,
		filterPolicy,
		!duplicates,	// @@ duplicate filtering doesn't seem to work
		GAP_LE_Event_Callback,
		0L
	);
	if (0 == result)
		gBLE->scanning = true;
}

void xs_ble_client_stop_scanning(xsMachine *the)
{
	if (gBLE->scanning) {
		qapi_BLE_GAP_LE_Cancel_Scan(gBLE->stackID);
		gBLE->scanning = 0;
	}
}

void xs_ble_client_connect(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t addressType = xsmcToInteger(xsArg(1));
	qapi_BLE_BD_ADDR_t remote_addr;
	int result;

	if (gBLE->scanning) {
		qapi_BLE_GAP_LE_Cancel_Scan(gBLE->stackID);
		gBLE->scanning = 0;
	}

	// Ignore duplicate connection attempts
	if (modBLEConnectionFindByAddress(address)) return;
	
	// Add a new connection record to be filled as the connection completes
	modBLEClientConnection connection = c_calloc(sizeof(modBLEClientConnectionRecord), 1);
	if (!connection)
		xsUnknownError("out of memory");
	connection->id = kInvalidConnectionID;
	c_memmove(&connection->address, address, 6);
	connection->addressType = addressType;
	connection->bond = 0xFF;

	modBLEConnectionAdd((modBLEConnection)connection);
	
	c_memmove(&remote_addr, address, 6);

	result = qapi_BLE_GAP_LE_Create_Connection(
		gBLE->stackID,
		gBLE->scanInterval,
		gBLE->scanWindow,
		QAPI_BLE_FP_NO_FILTER_E,
		addressType,			// remote address type
		&remote_addr,			// remote address
		QAPI_BLE_LAT_PUBLIC_E,	// local address type
		&gBLE->connectionParams,
		GAP_LE_Event_Callback,
		0L
	);
	if (result < 0)
		xsUnknownError("unable to create connection");
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
	gBLE->ioCapability = ioCapability;

	qapi_BLE_GAP_LE_Set_Pairability_Mode(gBLE->stackID,
		encryption ? QAPI_BLE_LPM_PAIRABLE_MODE_ENABLE_EXTENDED_EVENTS_E : QAPI_BLE_LPM_PAIRABLE_MODE_E);
		
	configurePairingCapabilities(encryption, bonding, mitm, ioCapability, &gBLE->pairingCapabilities);
	generateEncryptionKeys(gBLE->stackID, &gBLE->ER, &gBLE->IR, &gBLE->DHK, &gBLE->IRK);
		
	qapi_BLE_GAP_LE_Register_Remote_Authentication(gBLE->stackID, GAP_LE_Event_Callback, 0L);

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
	qapi_BLE_GAP_LE_Authentication_Response_Information_t GAP_LE_Authentication_Response_Information;
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t confirm = xsmcToBoolean(xsArg(1));
	qapi_BLE_BD_ADDR_t remote_addr;
	modBLEClientConnection connection;
	
	connection = (modBLEClientConnection)modBLEConnectionFindByAddress(address);
	if (!connection) return;
	
	c_memmove(&remote_addr, address, 6);

	GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = (confirm ? QAPI_BLE_LAR_CONFIRMATION_E : QAPI_BLE_LAR_ERROR_E);
	GAP_LE_Authentication_Response_Information.Authentication_Data_Length = sizeof(uint32_t);
	GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = connection->passkey;											
	qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, remote_addr, &GAP_LE_Authentication_Response_Information);									
}

void xs_gap_connection_initialize(xsMachine *the)
{
	uint32_t conn_id;
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
	uint32_t conn_id = xsmcToInteger(xsArg(0));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	qapi_BLE_BD_ADDR_t remote_addr;
	if (!connection)
		xsUnknownError("connection not found");
	c_memmove(&remote_addr, connection->address, 6);
	qapi_BLE_GAP_LE_Disconnect(gBLE->stackID, remote_addr);
}

void xs_gap_connection_read_rssi(xsMachine *the)
{
	xsUnknownError("unimplemented");	// @@ Only available from HCI connection??
}

void xs_gap_connection_exchange_mtu(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t mtu = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	connection->mtu_exchange_pending = 1;
	if (qapi_BLE_GATT_Exchange_MTU_Request(gBLE->stackID, conn_id, mtu, GATT_Client_Event_Callback, 0L) <= 0)
		xsRangeError("invalid mtu");
}

void xs_gatt_client_discover_primary_services(xsMachine *the)
{
	uint32_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t argc = xsmcArgc;
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	setGATTProcedure(connection, GATT_PROCEDURE_DISCOVER_SERVICES, connection->objClient, 0, 0);
	modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, serviceDiscoveryEvent, NULL);
		
	if (argc > 1) {
		qapi_BLE_GATT_UUID_t uuid;
		bufferToUUID((uint8_t*)xsmcToArrayBuffer(xsArg(1)), &uuid, xsmcGetArrayBufferLength(xsArg(1)));
		qapi_BLE_GATT_Start_Service_Discovery(gBLE->stackID, conn_id, 1, &uuid, GATT_Service_Discovery_Event_Callback, conn_id);
	}
	else {
		qapi_BLE_GATT_Start_Service_Discovery(gBLE->stackID, conn_id, 0, NULL, GATT_Service_Discovery_Event_Callback, conn_id);
	}
}

void xs_gatt_service_discover_characteristics(xsMachine *the)
{
	uint16_t argc = xsmcArgc;
	uint32_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t start = xsmcToInteger(xsArg(1));
	uint16_t end = xsmcToInteger(xsArg(2));
	CharacteristicDiscoveryRequest *request;
	uint32_t transaction_id;
	
   	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	request = c_calloc(1, sizeof(CharacteristicDiscoveryRequest));
	if (!request)
		xsUnknownError("out of memory");
		
	request->start = start;
	request->end = end;
	if (argc > 3)
		bufferToUUID((uint8_t*)xsmcToArrayBuffer(xsArg(3)), &request->uuid, xsmcGetArrayBufferLength(xsArg(3)));
	
	setGATTProcedure(connection, GATT_PROCEDURE_DISCOVER_CHARACTERISTICS, xsThis, 0, (uint32_t)request);
	modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, characteristicDiscoveryEvent, NULL);
			
	transaction_id = qapi_BLE_GATT_Discover_Characteristics(gBLE->stackID, conn_id, start, end, GATT_Client_Event_Callback, 0);
	if (transaction_id < 0)
		xsUnknownError("discover characteristics failed");
	else
		connection->procedure.transaction_id = transaction_id;
}

void xs_gatt_characteristic_discover_all_characteristic_descriptors(xsMachine *the)
{
	uint32_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	DescriptorDiscoveryRequest *request;
	qapi_BLE_GATT_UUID_t uuid;
	
   	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	request = c_calloc(1, sizeof(DescriptorDiscoveryRequest));
	if (!request)
		xsUnknownError("out of memory");
		
	request->handle = handle;
	request->conn_id = conn_id;
		
	setGATTProcedure(connection, GATT_PROCEDURE_DISCOVER_DESCRIPTORS, xsThis, 0, (uint32_t)request);
	modBLEMessageQueueConfigure(&gBLE->discoveryQueue, the, descriptorDiscoveryEvent, NULL);
	
	// we use service discovery to discover all descriptors by matching the parent characteristic in the discovery results
	xsmcVars(2);
	xsmcGet(xsVar(0), xsThis, xsID_service);
	xsmcGet(xsVar(1), xsVar(0), xsID_uuid);
	bufferToUUID((uint8_t*)xsmcToArrayBuffer(xsVar(1)), &uuid, xsmcGetArrayBufferLength(xsVar(1)));
	qapi_BLE_GATT_Start_Service_Discovery(gBLE->stackID, conn_id, 1, &uuid, GATT_Descriptor_Discovery_Event_Callback, (uint32_t)request);
}

void xs_gatt_characteristic_read_value(xsMachine *the)
{
	uint32_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
#if 0
	uint16_t argc = xsmcArgc;
	uint16_t auth = 0;
	
	if (argc > 2)
		auth = xsmcToInteger(xsArg(2));
#endif		
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	setGATTProcedure(connection, GATT_PROCEDURE_READ_CHARACTERISTIC, connection->objClient, 0, handle);
	
	qapi_BLE_GATT_Read_Value_Request(gBLE->stackID, conn_id, handle, GATT_Client_Event_Callback, 0);
}

void xs_gatt_descriptor_read_value(xsMachine *the)
{
	uint32_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
#if 0
	uint16_t argc = xsmcArgc;
	uint16_t auth = 0;
	
	if (argc > 2)
		auth = xsmcToInteger(xsArg(2));
#endif
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	setGATTProcedure(connection, GATT_PROCEDURE_READ_DESCRIPTOR, connection->objClient, 0, handle);
	
	qapi_BLE_GATT_Read_Value_Request(gBLE->stackID, conn_id, handle, GATT_Client_Event_Callback, 0);
}

void xs_gatt_characteristic_enable_notifications(xsMachine *the)
{
	CharacteristicNotificationRequest *request;
    uint8_t data[2] = {0x01, 0x00};
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;    
	
	request = c_calloc(1, sizeof(CharacteristicNotificationRequest));
	if (!request)
		xsUnknownError("out of memory");

	request->handle = characteristic;
	request->conn_id = conn_id;
	request->enable = true;

	setGATTProcedure(connection, GATT_PROCEDURE_ENABLE_CHARACTERISTIC_NOTIFICATIONS, connection->objClient, 0, (uint32_t)request);

	qapi_BLE_GATT_Write_Request(gBLE->stackID, conn_id, characteristic + 1, sizeof(data), data, GATT_Client_Event_Callback, 0);
}

void xs_gatt_characteristic_disable_notifications(xsMachine *the)
{
	CharacteristicNotificationRequest *request;
	uint8_t data[2] = {0x00, 0x00};
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;    

	request = c_calloc(1, sizeof(CharacteristicNotificationRequest));
	if (!request)
		xsUnknownError("out of memory");

	request->handle = characteristic;
	request->conn_id = conn_id;
	request->enable = false;
	
	setGATTProcedure(connection, GATT_PROCEDURE_DISABLE_CHARACTERISTIC_NOTIFICATIONS, connection->objClient, 0, (uint32_t)request);

	qapi_BLE_GATT_Write_Request(gBLE->stackID, conn_id, characteristic + 1, sizeof(data), data, GATT_Client_Event_Callback, 0);
}

void xs_gatt_descriptor_write_value(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	switch (xsmcTypeOf(xsArg(2))) {
		case xsStringType: {
			char *str = xsmcToString(xsArg(2));
			qapi_BLE_GATT_Write_Without_Response_Request(gBLE->stackID, conn_id, handle, c_strlen(str), (uint8_t*)str);
			break;
		}
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype)) {
				qapi_BLE_GATT_Write_Without_Response_Request(gBLE->stackID, conn_id, handle, xsmcGetArrayBufferLength(xsArg(2)), (uint8_t*)xsmcToArrayBuffer(xsArg(2)));
			}
			else
				goto unknown;
			break;
		unknown:
		default:
			xsUnknownError("unsupported type");
			break;
	}
}

void xs_gatt_characteristic_write_without_response(xsMachine *the)
{
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	switch (xsmcTypeOf(xsArg(2))) {
		case xsStringType: {
			char *str = xsmcToString(xsArg(2));
			qapi_BLE_GATT_Write_Without_Response_Request(gBLE->stackID, conn_id, handle, c_strlen(str), (uint8_t*)str);
			break;
		}
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype)) {
				qapi_BLE_GATT_Write_Without_Response_Request(gBLE->stackID, conn_id, handle, xsmcGetArrayBufferLength(xsArg(2)), (uint8_t*)xsmcToArrayBuffer(xsArg(2)));
			}
			else
				goto unknown;
			break;
		unknown:
		default:
			xsUnknownError("unsupported type");
			break;
	}
}

static void setGATTProcedure(modBLEClientConnection connection, uint8_t what, xsSlot target, uint32_t transaction_id, uint32_t refcon)
{
	GATTProcedure procedure = &connection->procedure;
	procedure->what = what;
	procedure->transaction_id = transaction_id;
	procedure->target = target;
	procedure->refcon = refcon;
}

static void completeProcedure(uint32_t conn_id, char *which)
{
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	xsSlot obj = connection->procedure.target;
	
	c_memset(&connection->procedure, 0, sizeof(connection->procedure));
	
	if (NULL != which) {
		xsBeginHost(gBLE->the);
		xsCall1(obj, xsID_callback, xsString(which));
		xsEndHost(gBLE->the);
	}
}

void uuidToBuffer(qapi_BLE_GATT_UUID_t *uuid, uint8_t *buffer, uint16_t *length)
{
	if (uuid->UUID_Type == QAPI_BLE_GU_UUID_16_E) {
		*length = 2;
		c_memmove(buffer, &uuid->UUID.UUID_16, *length);
	}
	else if (uuid->UUID_Type == QAPI_BLE_GU_UUID_128_E) {
		*length = 16;
		c_memmove(buffer, &uuid->UUID.UUID_128, *length);
	}
}

void bufferToUUID(uint8_t *buffer, qapi_BLE_GATT_UUID_t *uuid, uint16_t length)
{
	if (2 == length) {
		uuid->UUID_Type = QAPI_BLE_GU_UUID_16_E;
		QAPI_BLE_ASSIGN_BLUETOOTH_UUID_16(uuid->UUID.UUID_16, buffer[1], buffer[0]);
	}
	else if (16 == length) {
		uuid->UUID_Type = QAPI_BLE_GU_UUID_128_E;
		QAPI_BLE_ASSIGN_BLUETOOTH_UUID_128(
			uuid->UUID.UUID_128,
			buffer[15], buffer[14], buffer[13], buffer[12], buffer[11], buffer[10], buffer[9], buffer[8],
			buffer[7], buffer[6], buffer[5], buffer[4], buffer[3], buffer[2], buffer[1], buffer[0]
		);
	}
}

uint8_t UUIDEqual(qapi_BLE_GATT_UUID_t *uuid1, qapi_BLE_GATT_UUID_t *uuid2)
{
	uint8_t result;
	
	if (uuid1->UUID_Type != uuid2->UUID_Type)
		return 0;
		
	switch(uuid1->UUID_Type) {
		case QAPI_BLE_GU_UUID_16_E:
			result = QAPI_BLE_COMPARE_UUID_16(uuid1->UUID.UUID_16, uuid2->UUID.UUID_16);
			break;
		case QAPI_BLE_GU_UUID_128_E:
			result = QAPI_BLE_COMPARE_UUID_128(uuid1->UUID.UUID_128, uuid2->UUID.UUID_128);
			break;
		default:
			result = 0;
			break;
	}
	return result;
}

static int modBLEConnectionSaveAttHandle(modBLEClientConnection connection, qapi_BLE_GATT_UUID_t *uuid, uint16_t handle)
{
	int result = -1;
	for (int service_index = 0; service_index < service_count; ++service_index) {
		for (int att_index = 0; att_index < attribute_counts[service_index]; ++att_index) {
			const qapi_BLE_GATT_Service_Attribute_Entry_t *att_entry = &ServiceTable[service_index][att_index];
			if (QAPI_BLE_AET_CHARACTERISTIC_VALUE_128_E == att_entry->Attribute_Entry_Type || QAPI_BLE_AET_CHARACTERISTIC_VALUE_16_E == att_entry->Attribute_Entry_Type) {
				uint8_t equal;
				if (QAPI_BLE_AET_CHARACTERISTIC_VALUE_128_E == att_entry->Attribute_Entry_Type)
					equal = (0 == c_memcmp(att_entry->Attribute_Value, &uuid->UUID.UUID_128, 16));
				else
					equal = (0 == c_memcmp(att_entry->Attribute_Value, &uuid->UUID.UUID_16, 4));
				if (equal) {
					for (int k = 0; k < char_name_count; ++k) {
						const char_name_table *char_name = &char_names[k];
						if (service_index == char_name->service_index && att_index == char_name->att_index) {
							connection->handles[k] = handle;
							result = k;
							break;
						}
					}
					goto bail;
				}
			}
		}
	}
	
bail:
	return result;
}

void modBLEClientBondingRemoved(qapi_BLE_BD_ADDR_t *address, qapi_BLE_GAP_LE_Address_Type_t addressType)
{
	qapi_BLE_GAP_LE_White_List_Entry_t entry;
	
	if (!gBLE) return;
	
	entry.Address_Type = addressType;
	c_memmove(&entry.Address, address, 6);
	modMessagePostToMachine(gBLE->the, (void*)&entry, sizeof(entry), bondingRemovedEvent, NULL);
}

void bondingRemovedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GAP_LE_White_List_Entry_t *entry = (qapi_BLE_GAP_LE_White_List_Entry_t *)message;
	
	if (!gBLE) return;

	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), &entry->Address, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), entry->Address_Type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onBondingDeleted"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void readyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsBeginHost(gBLE->the);
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
	xsEndHost(gBLE->the);
}

static void connectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Device_Connection_Data_t *result = (qapi_BLE_GATT_Device_Connection_Data_t*)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAddress((uint8_t*)&result->RemoteDevice);
	if (!connection)
		xsUnknownError("connection not found");
		
	// Ignore duplicate connection events
	if (kInvalidConnectionID != connection->id)
		goto bail;

	connection->id = result->ConnectionID;
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), connection->id);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	xsmcSetArrayBuffer(xsVar(1), &result->RemoteDevice, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), connection->addressType);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));
bail:
	xsEndHost(gBLE->the);
}

static void disconnectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Device_Disconnection_Data_t *result = (qapi_BLE_GATT_Device_Disconnection_Data_t*)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(result->ConnectionID);
	
	// ignore multiple disconnects on same connection
	if (!connection) goto bail;

	xsmcVars(2);
	xsmcSetInteger(xsVar(0), connection->id);
	xsmcSetArrayBuffer(xsVar(1), &result->RemoteDevice, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), connection->addressType);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(connection->objConnection, xsID_callback, xsString("onDisconnected"), xsVar(0));
	xsForget(connection->objConnection);
	modBLEConnectionRemove(connection);
bail:
	xsEndHost(gBLE->the);
}

static void deviceDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	if (!gBLE->scanning) {
		modBLEMessageQueueEmpty(&gBLE->discoveryQueue);
	}
	else {
		deviceDiscoveryRecord *entry;
		xsBeginHost(gBLE->the);
		xsmcVars(2);
		while (NULL != (entry = (deviceDiscoveryRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {
			qapi_BLE_GAP_LE_Advertising_Report_Data_t *disc = &entry->disc;
			xsVar(0) = xsmcNewObject();
			xsmcSetArrayBuffer(xsVar(1), entry->data, disc->Raw_Report_Length);
			xsmcSet(xsVar(0), xsID_scanResponse, xsVar(1));
			xsmcSetArrayBuffer(xsVar(1), &disc->BD_ADDR, 6);
			xsmcSet(xsVar(0), xsID_address, xsVar(1));
			xsmcSetInteger(xsVar(1), disc->Address_Type);
			xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
			xsmcSetInteger(xsVar(1), disc->RSSI);
			xsmcSet(xsVar(0), xsID_rssi, xsVar(1));
			xsCall2(gBLE->obj, xsID_callback, xsString("onDiscovered"), xsVar(0));
			c_free(entry);
		}
		xsEndHost(gBLE->the);
	}
}

static void serviceDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	serviceSearchRecord *entry;
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	while (NULL != (entry = (serviceSearchRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {		
		modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection)
			xsUnknownError("connection not found");
		if (entry->completed)
			completeProcedure(entry->conn_id, "onService");
		else {
			qapi_BLE_GATT_Service_Information_t *service = &entry->service;
			uint8_t buffer[16];
			uint16_t length;
			xsVar(0) = xsmcNewObject();
			uuidToBuffer(&service->UUID, buffer, &length);
			xsmcSetArrayBuffer(xsVar(1), buffer, length);
			xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
			xsmcSetInteger(xsVar(1), service->Service_Handle);
			xsmcSet(xsVar(0), xsID_start, xsVar(1));
			xsmcSetInteger(xsVar(1), service->End_Group_Handle);
			xsmcSet(xsVar(0), xsID_end, xsVar(1));
			xsCall2(connection->procedure.target, xsID_callback, xsString("onService"), xsVar(0));
		}	
		c_free(entry);
	}
	xsEndHost(gBLE->the);
}

static void descriptorDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	descriptorSearchRecord *entry;
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	while (NULL != (entry = (descriptorSearchRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {		
		modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection)
			xsUnknownError("connection not found");
		if (entry->completed)
			completeProcedure(entry->conn_id, "onDescriptor");
		else {
			uint8_t buffer[16];
			uint16_t i, length;
			for (i = 0; i < entry->count; ++i) {
				qapi_BLE_GATT_Characteristic_Descriptor_Information_t *descriptorInfo = &entry->descriptorList[i];
				int index = modBLEConnectionSaveAttHandle(connection, &descriptorInfo->Characteristic_Descriptor_UUID, descriptorInfo->Characteristic_Descriptor_Handle);
				xsVar(0) = xsmcNewObject();
				uuidToBuffer(&descriptorInfo->Characteristic_Descriptor_UUID, buffer, &length);
				xsmcSetArrayBuffer(xsVar(1), buffer, length);
				xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
				xsmcSetInteger(xsVar(1), descriptorInfo->Characteristic_Descriptor_Handle);
				xsmcSet(xsVar(0), xsID_handle, xsVar(1));
				if (-1 != index) {
					xsmcSetString(xsVar(1), (char*)char_names[index].name);
					xsmcSet(xsVar(0), xsID_name, xsVar(1));
					xsmcSetString(xsVar(1), (char*)char_names[index].type);
					xsmcSet(xsVar(0), xsID_type, xsVar(1));
				}
				xsCall2(connection->procedure.target, xsID_callback, xsString("onDescriptor"), xsVar(0));
			}
		}	
		c_free(entry);
	}
	xsEndHost(gBLE->the);
}

static void characteristicDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	characteristicSearchRecord *entry;
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	while (NULL != (entry = (characteristicSearchRecord*)modBLEMessageQueueDequeue(&gBLE->discoveryQueue))) {		
		modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection)
			xsUnknownError("connection not found");
		uint16_t start;
		CharacteristicDiscoveryRequest *request = (CharacteristicDiscoveryRequest*)connection->procedure.refcon;
		uint8_t discoverOneCharacteristic = (0 != request->uuid.UUID.UUID_16.UUID_Byte0);
		for (uint16_t i = 0; i < entry->count; ++i) {
			uint8_t buffer[16];
			uint16_t length;
			qapi_BLE_GATT_Characteristic_Entry_t *characteristic = &entry->CharacteristicEntryList[i];
			if (discoverOneCharacteristic) {
				if (!UUIDEqual(&request->uuid, &characteristic->CharacteristicValue.CharacteristicUUID))
					continue;
			}
			int index = modBLEConnectionSaveAttHandle(connection, &characteristic->CharacteristicValue.CharacteristicUUID, characteristic->CharacteristicValue.CharacteristicValueHandle);
			uuidToBuffer(&characteristic->CharacteristicValue.CharacteristicUUID, buffer, &length);
			xsVar(0) = xsmcNewObject();
			xsmcSetArrayBuffer(xsVar(1), buffer, length);
			xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
			xsmcSetInteger(xsVar(1), characteristic->CharacteristicValue.CharacteristicValueHandle);
			xsmcSet(xsVar(0), xsID_handle, xsVar(1));
			xsmcSetInteger(xsVar(1), characteristic->CharacteristicValue.CharacteristicProperties);
			xsmcSet(xsVar(0), xsID_properties, xsVar(1));
			if (-1 != index) {
				xsmcSetString(xsVar(1), (char*)char_names[index].name);
				xsmcSet(xsVar(0), xsID_name, xsVar(1));
				xsmcSetString(xsVar(1), (char*)char_names[index].type);
				xsmcSet(xsVar(0), xsID_type, xsVar(1));
			}
			xsCall2(connection->procedure.target, xsID_callback, xsString("onCharacteristic"), xsVar(0));
			if (discoverOneCharacteristic)
				entry->count = 0;	// done
		}
		if (0 == entry->count)
			start = request->end;
		else
			start = entry->CharacteristicEntryList[entry->count-1].CharacteristicValue.CharacteristicValueHandle + 1;
	
		if (start < request->end) {
			connection->procedure.transaction_id = qapi_BLE_GATT_Discover_Characteristics(gBLE->stackID, entry->conn_id, start, request->end, GATT_Client_Event_Callback, 0);
		}
		else {
			c_free(request);
			completeProcedure(entry->conn_id, "onCharacteristic");
		}
		c_free(entry);
	}
	xsEndHost(gBLE->the);
}

static void characteristicReadValueEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Read_Response_Data_t *result = (qapi_BLE_GATT_Read_Response_Data_t*)message;
	uint32_t conn_id = result->ConnectionID;
	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
		
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), result->AttributeValue, result->AttributeValueLength);
	xsmcSetInteger(xsVar(2), connection->procedure.refcon);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->procedure.target, xsID_callback, xsString("onCharacteristicValue"), xsVar(0));
	
	completeProcedure(conn_id, NULL);
	c_free(result->AttributeValue);
	xsEndHost(gBLE->the);
}

static void characteristicWriteValueEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Write_Response_Data_t *result = (qapi_BLE_GATT_Write_Response_Data_t*)message;
	uint32_t conn_id = result->ConnectionID;
	CharacteristicNotificationRequest *request = NULL;
	
	xsBeginHost(gBLE->the);
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
		
	if (connection->procedure.what != GATT_PROCEDURE_ENABLE_CHARACTERISTIC_NOTIFICATIONS && connection->procedure.what != GATT_PROCEDURE_DISABLE_CHARACTERISTIC_NOTIFICATIONS)
		goto bail;
		
	request = (CharacteristicNotificationRequest*)connection->procedure.refcon;
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), request->handle);
	xsmcSet(xsVar(0), xsID_handle, xsVar(1));
	xsCall2(connection->objClient, xsID_callback, request->enable ? xsString("onCharacteristicNotificationEnabled") : xsString("onCharacteristicNotificationDisabled"), xsVar(0));
	
	completeProcedure(conn_id, NULL);
	
bail:
	if (request)
		c_free(request);
	xsEndHost(gBLE->the);
}

static void notificationEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	attributeNotificationRecord *entry;
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	while (NULL != (entry = (attributeNotificationRecord*)modBLEMessageQueueDequeue(&gBLE->notificationQueue))) {
		modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(entry->conn_id);
		if (!connection)
			xsUnknownError("connection not found");
		qapi_BLE_GATT_Server_Notification_Data_t *notification = &entry->notification;
		xsVar(0) = xsmcNewObject();
		xsmcSetArrayBuffer(xsVar(1), entry->data, notification->AttributeValueLength);
		xsmcSet(xsVar(0), xsID_value, xsVar(1));
		xsmcSetInteger(xsVar(1), notification->AttributeHandle);
		xsmcSet(xsVar(0), xsID_handle, xsVar(1));
		xsCall2(connection->objClient, xsID_callback, xsString("onCharacteristicNotification"), xsVar(0));
		c_free(entry);
	}	
	xsEndHost(gBLE->the);
}

static void clientErrorEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Request_Error_Data_t *result = (qapi_BLE_GATT_Request_Error_Data_t*)message;
	uint32_t conn_id = result->ConnectionID;
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	
	if (result->TransactionID == connection->procedure.transaction_id) {
		if (GATT_PROCEDURE_DISCOVER_CHARACTERISTICS == connection->procedure.what) {
			c_free((void*)connection->procedure.refcon);
			completeProcedure(conn_id, "onCharacteristic");
		}
	}
}

static void mtuExchangedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Exchange_MTU_Response_Data_t *result = (qapi_BLE_GATT_Exchange_MTU_Response_Data_t*)message;
	uint32_t conn_id = result->ConnectionID;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection || !connection->mtu_exchange_pending) return;
	
	connection->mtu_exchange_pending = 0;
	xsBeginHost(gBLE->the);
		xsCall2(connection->objConnection, xsID_callback, xsString("onMTUExchanged"), xsInteger(result->ServerMTU));
	xsEndHost(gBLE->the);
}

static void gapPasskeyRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GAP_LE_Authentication_Event_Data_t *Authentication_Event_data = (qapi_BLE_GAP_LE_Authentication_Event_Data_t*)message;
	qapi_BLE_GAP_LE_Authentication_Response_Information_t GAP_LE_Authentication_Response_Information;
	uint32_t passkey;
	modBLEConnection connection;
	
	connection = modBLEConnectionFindByAddress((uint8_t*)&Authentication_Event_data->BD_ADDR);
	if (!connection) return;
	
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), &Authentication_Event_data->BD_ADDR, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsResult = xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyRequested"), xsVar(0));
	passkey = xsmcToInteger(xsResult);
	xsEndHost(gBLE->the);
	
	GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type  = QAPI_BLE_LAR_PASSKEY_E;
	GAP_LE_Authentication_Response_Information.Authentication_Data_Length  = (uint8_t)(sizeof(uint32_t));
	GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = passkey;
	qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, Authentication_Event_data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
}

static void gapPasskeyNotifyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GAP_LE_Authentication_Event_Data_t *Authentication_Event_data = (qapi_BLE_GAP_LE_Authentication_Event_Data_t*)message;
	uint32_t passkey = Authentication_Event_data->Authentication_Event_Data.Confirmation_Request.Display_Passkey;
	modBLEConnection connection;
	
	connection = modBLEConnectionFindByAddress((uint8_t*)&Authentication_Event_data->BD_ADDR);
	if (!connection) return;
		
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), &Authentication_Event_data->BD_ADDR, 6);
	xsmcSetInteger(xsVar(2), passkey);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSet(xsVar(0), xsID_passkey, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyDisplay"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gapPasskeyConfirmEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GAP_LE_Authentication_Event_Data_t *Authentication_Event_data = (qapi_BLE_GAP_LE_Authentication_Event_Data_t*)message;
	uint32_t passkey = Authentication_Event_data->Authentication_Event_Data.Confirmation_Request.Display_Passkey;
	
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByAddress((uint8_t*)&Authentication_Event_data->BD_ADDR);
	if (!connection) return;
	
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	connection->passkey = passkey;
	xsmcSetArrayBuffer(xsVar(1), &Authentication_Event_data->BD_ADDR.BD_ADDR0, 6);
	xsmcSetInteger(xsVar(2), passkey);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSet(xsVar(0), xsID_passkey, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyConfirm"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gapAuthCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GAP_LE_Authentication_Event_Data_t *Authentication_Event_Data = (qapi_BLE_GAP_LE_Authentication_Event_Data_t *)message;
	qapi_BLE_GAP_LE_Pairing_Status_t *Pairing_Status = &Authentication_Event_Data->Authentication_Event_Data.Pairing_Status;
	qapi_BLE_BD_ADDR_t remote_addr;
	modBLEBondedDevice device;
	modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByAddress((uint8_t*)&Authentication_Event_Data->BD_ADDR);
	if (NULL == connection) return;
	c_memmove(&remote_addr, connection->address, 6);
	if (QAPI_BLE_GAP_LE_PAIRING_STATUS_NO_ERROR == Pairing_Status->Status) {
		uint8_t bonded = 0;
		if (gBLE->bonding && ((connection->Flags & (DEVICE_INFO_FLAGS_LTK_VALID | DEVICE_INFO_FLAGS_IRK_VALID)) == (DEVICE_INFO_FLAGS_LTK_VALID | DEVICE_INFO_FLAGS_IRK_VALID))) {
			bonded = 1;
			device = modBLEBondedDevicesFindByAddress(remote_addr);
			if (NULL != device)
				modBLEBondedDevicesRemove(device);
			device = c_calloc(sizeof(modBLEBondedDeviceRecord), 1);
			if (NULL != device) {
				device->Flags = connection->Flags;
				device->LastAddress = remote_addr;
				device->LastAddressType = connection->addressType;
				device->IdentityAddress = connection->IdentityAddressBD_ADDR;
				device->IdentityAddressType = connection->IdentityAddressType;
				device->EncryptionKeySize = connection->EncryptionKeySize;
				device->IRK = connection->IRK;
				device->LTK = connection->LTK;
				modBLEBondedDevicesAdd(device);
			}
		}
		xsBeginHost(gBLE->the);
		xsmcVars(2);
		xsVar(0) = xsmcNewObject();
		xsmcSetBoolean(xsVar(1), bonded);
		xsmcSet(xsVar(0), xsID_bonded, xsVar(1));
		xsCall2(gBLE->obj, xsID_callback, xsString("onAuthenticated"), xsVar(0));
		xsEndHost(gBLE->the);
	}
	else {
		device = modBLEBondedDevicesFindByAddress(remote_addr);
		if (NULL != device)
			modBLEBondedDevicesRemove(device);
	}
}

void QAPI_BLE_BTPSAPI GATT_Connection_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, uint32_t CallbackParameter)
{
	if (gBLE && gBLE->stackID && GATT_Connection_Event_Data) {
		switch(GATT_Connection_Event_Data->Event_Data_Type) {
			case QAPI_BLE_ET_GATT_CONNECTION_DEVICE_CONNECTION_E:
				if (GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data) {
					qapi_BLE_GATT_Device_Connection_Data_t GATT_Device_Connection_Data = *GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data;
					modMessagePostToMachine(gBLE->the, (uint8_t*)&GATT_Device_Connection_Data, sizeof(qapi_BLE_GATT_Device_Connection_Data_t), connectEvent, NULL);
				}
				break;
			case QAPI_BLE_ET_GATT_CONNECTION_DEVICE_DISCONNECTION_E:
				if (GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data) {
					qapi_BLE_GATT_Device_Disconnection_Data_t GATT_Device_Disconnection_Data = *GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data;
					modMessagePostToMachine(gBLE->the, (uint8_t*)&GATT_Device_Disconnection_Data, sizeof(qapi_BLE_GATT_Device_Disconnection_Data_t), disconnectEvent, NULL);				
				}
				break;
			case QAPI_BLE_ET_GATT_CONNECTION_SERVER_NOTIFICATION_E:
				if (GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data) {
					qapi_BLE_GATT_Server_Notification_Data_t GATT_Server_Notification_Data = *GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data;
					attributeNotificationRecord *entry = (attributeNotificationRecord*)c_malloc(sizeof(attributeNotificationRecord) - 1 + GATT_Server_Notification_Data.AttributeValueLength);
					if (NULL != entry) {
						entry->notification = GATT_Server_Notification_Data;
						entry->conn_id = GATT_Server_Notification_Data.ConnectionID;
						c_memmove(entry->data, GATT_Server_Notification_Data.AttributeValue, GATT_Server_Notification_Data.AttributeValueLength);
						modBLEMessageQueueEnqueue(&gBLE->notificationQueue, (modBLEMessageQueueEntry)entry);
					}
				}
				break;
			default:
				break;
		}
	}
}

void QAPI_BLE_BTPSAPI GAP_LE_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GAP_LE_Event_Data_t *GAP_LE_Event_Data, uint32_t CallbackParameter)
{
	if (gBLE && gBLE->stackID && GAP_LE_Event_Data) {
	
		LOG_GAP_EVENT(GAP_LE_Event_Data->Event_Data_Type);

		switch(GAP_LE_Event_Data->Event_Data_Type) {
			case QAPI_BLE_ET_LE_CONNECTION_COMPLETE_E: {
				qapi_BLE_GAP_LE_Connection_Complete_Event_Data_t *Connection_Complete_Event_Data = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data;
				if (QAPI_BLE_HCI_ERROR_CODE_NO_ERROR == Connection_Complete_Event_Data->Status) {
					modBLEConnection connection = modBLEConnectionFindByAddress((uint8_t*)&Connection_Complete_Event_Data->Peer_Address);
					if (connection)
						connection->addressType = Connection_Complete_Event_Data->Peer_Address_Type;
				}
				break;
			}
			case QAPI_BLE_ET_LE_AUTHENTICATION_E: {
				qapi_BLE_GAP_LE_Authentication_Event_Data_t *Authentication_Event_Data = GAP_LE_Event_Data->Event_Data.GAP_LE_Authentication_Event_Data;
   				if (NULL != Authentication_Event_Data) {
					qapi_BLE_GAP_LE_Authentication_Response_Information_t GAP_LE_Authentication_Response_Information;
					qapi_BLE_GAP_LE_Security_Information_t GAP_LE_Security_Information;
					qapi_BLE_Random_Number_t RandomNumber;
					qapi_BLE_Long_Term_Key_t GeneratedLTK;
     				uint16_t EDIV, LocalDiv;
					int Result;
					modBLEClientConnection connection;
					
					LOG_GAP_AUTHENTICATION_EVENT(Authentication_Event_Data->GAP_LE_Authentication_Event_Type);
					
					switch (Authentication_Event_Data->GAP_LE_Authentication_Event_Type) {
						case QAPI_BLE_LAT_LONG_TERM_KEY_REQUEST_E:
							GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = QAPI_BLE_LAR_LONG_TERM_KEY_E;
							GAP_LE_Authentication_Response_Information.Authentication_Data_Length = 0;

							c_memset(&RandomNumber, 0, sizeof(RandomNumber));
							EDIV = 0;

							/* Check to see if this is a request for a SC generated Long Term Key */
                     		if ((Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.EDIV == EDIV) && (QAPI_BLE_COMPARE_RANDOM_NUMBER(Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.Rand, RandomNumber))) {
								/* Search for the entry for this slave to store the information into */
								connection = (modBLEClientConnection)modBLEConnectionFindByAddress((uint8_t*)&Authentication_Event_Data->BD_ADDR);
								if (connection) {
									/* Check to see if the LTK is valid.         */
									if (connection->Flags & DEVICE_INFO_FLAGS_LTK_VALID) {
										/* Respond with the stored Long Term Key. */
										GAP_LE_Authentication_Response_Information.Authentication_Data_Length                                        = QAPI_BLE_GAP_LE_LONG_TERM_KEY_INFORMATION_DATA_SIZE;
										GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Encryption_Key_Size = connection->EncryptionKeySize;

										c_memcpy(&(GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Long_Term_Key), &(connection->LTK), QAPI_BLE_LONG_TERM_KEY_SIZE);
									}
								}
							}
							else {
								/* The other side of a connection is requesting */
								/* that we start encryption.  Thus we should    */
								/* regenerate LTK for this connection and send  */
								/* it to the chip.                              */
								Result = qapi_BLE_GAP_LE_Regenerate_Long_Term_Key(gBLE->stackID, (qapi_BLE_Encryption_Key_t *)&gBLE->DHK, (qapi_BLE_Encryption_Key_t *)&gBLE->ER, Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.EDIV, &(Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.Rand), &GeneratedLTK);
								if (QAPI_OK == Result) {
									/* Respond with the Re-Generated Long Term Key */
									GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type                                        = QAPI_BLE_LAR_LONG_TERM_KEY_E;
									GAP_LE_Authentication_Response_Information.Authentication_Data_Length                                        = QAPI_BLE_GAP_LE_LONG_TERM_KEY_INFORMATION_DATA_SIZE;
									GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Encryption_Key_Size = QAPI_BLE_GAP_LE_MAXIMUM_ENCRYPTION_KEY_SIZE;
									GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Long_Term_Key       = GeneratedLTK;
								}
							}
							qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
							break;
						case QAPI_BLE_LAT_SECURITY_REQUEST_E: {
							/* Determine if we have previously paired with the */
							/* device. If we have paired we will attempt to    */
							/* re-establish security using a previously        */
							/* exchanged LTK.                                  */
							modBLEBondedDevice device = modBLEBondedDevicesFindByAddress(Authentication_Event_Data->BD_ADDR);
							if (device) {
								modBLEClientConnection connection = (modBLEClientConnection)modBLEConnectionFindByAddress((uint8_t*)&Authentication_Event_Data->BD_ADDR);
								
								/* Determine if a Valid Long Term Key is stored */
								/* for this device.                             */
								if ((NULL != connection) && (device->Flags & DEVICE_INFO_FLAGS_LTK_VALID)) {
									/* Attempt to re-establish security to this  */
									/* device.                                   */
									GAP_LE_Security_Information.Local_Device_Is_Master                                      = TRUE;
									c_memcpy(&(GAP_LE_Security_Information.Security_Information.Master_Information.LTK), &(device->LTK), sizeof(device->LTK));
									GAP_LE_Security_Information.Security_Information.Master_Information.EDIV                = connection->EDIV;
									c_memcpy(&(GAP_LE_Security_Information.Security_Information.Master_Information.Rand), &(connection->Rand), sizeof(connection->Rand));
									GAP_LE_Security_Information.Security_Information.Master_Information.Encryption_Key_Size = device->EncryptionKeySize;
									qapi_BLE_GAP_LE_Reestablish_Security(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Security_Information, GAP_LE_Event_Callback, 0);
								}
								else {
									/* We do not have a stored Link Key for this */
									/* device so go ahead and pair to this       */
									/* device.                                   */
									/* Start the pairing process.				 */
									qapi_BLE_GAP_LE_Extended_Pairing_Capabilities_t ExtendedCapabilities;
									configurePairingCapabilities(gBLE->encryption, gBLE->bonding, gBLE->mitm, gBLE->ioCapability, &ExtendedCapabilities);

									if ((Result = qapi_BLE_GAP_LE_Extended_Pair_Remote_Device(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &ExtendedCapabilities, GAP_LE_Event_Callback, 0)) == QAPI_BLE_BTPS_ERROR_SECURE_CONNECTIONS_NOT_SUPPORTED) {
										/* Since Secure Connections isn't supported go ahead and */
										/* disable our request for Secure Connections and        */
										/* re-submit our request.                                */
										ExtendedCapabilities.Flags &= ~QAPI_BLE_GAP_LE_EXTENDED_PAIRING_CAPABILITIES_FLAGS_SECURE_CONNECTIONS;

										/* Try this again.                                       */
										Result = qapi_BLE_GAP_LE_Extended_Pair_Remote_Device(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &ExtendedCapabilities, GAP_LE_Event_Callback, 0);
									}
								}
							}
							else {
								/* We do not have a stored Link Key for this */
								/* device so go ahead and pair to this       */
								/* device.                                   */
								/* Start the pairing process.				 */
								qapi_BLE_GAP_LE_Extended_Pairing_Capabilities_t ExtendedCapabilities;
								configurePairingCapabilities(gBLE->encryption, gBLE->bonding, gBLE->mitm, gBLE->ioCapability, &ExtendedCapabilities);

								if ((Result = qapi_BLE_GAP_LE_Extended_Pair_Remote_Device(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &ExtendedCapabilities, GAP_LE_Event_Callback, 0)) == QAPI_BLE_BTPS_ERROR_SECURE_CONNECTIONS_NOT_SUPPORTED) {
									/* Since Secure Connections isn't supported go ahead and */
									/* disable our request for Secure Connections and        */
									/* re-submit our request.                                */
									ExtendedCapabilities.Flags &= ~QAPI_BLE_GAP_LE_EXTENDED_PAIRING_CAPABILITIES_FLAGS_SECURE_CONNECTIONS;

									/* Try this again.                                       */
									Result = qapi_BLE_GAP_LE_Extended_Pair_Remote_Device(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &ExtendedCapabilities, GAP_LE_Event_Callback, 0);
								}							
							}
							break;
						}
						case QAPI_BLE_LAT_CONFIRMATION_REQUEST_E:
							switch (Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type) {
								case QAPI_BLE_CRT_NONE_E:
									/* Handle the just works request.            */
									GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = QAPI_BLE_LAR_CONFIRMATION_E;

									/* By setting the Authentication_Data_Length */
									/* to any NON-ZERO value we are informing the*/
									/* GAP LE Layer that we are accepting Just   */
									/* Works Pairing.                            */
									GAP_LE_Authentication_Response_Information.Authentication_Data_Length = sizeof(uint32_t);
									
									if (QAPI_BLE_LIC_DISPLAY_ONLY_E == gBLE->pairingCapabilities.IO_Capability)
										GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Display_Passkey;
									
									qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);											
									break;
								case QAPI_BLE_CRT_PASSKEY_E:
									modMessagePostToMachine(gBLE->the, (uint8_t*)Authentication_Event_Data, sizeof(qapi_BLE_GAP_LE_Authentication_Event_Data_t), gapPasskeyRequestEvent, NULL);
									break;
								case QAPI_BLE_CRT_DISPLAY_E:
									modMessagePostToMachine(gBLE->the, (uint8_t*)Authentication_Event_Data, sizeof(qapi_BLE_GAP_LE_Authentication_Event_Data_t), gapPasskeyNotifyEvent, NULL);
									break;
								default:
									break;
							}
							break;
						case QAPI_BLE_LAT_IDENTITY_INFORMATION_REQUEST_E:
							/* Store our identity information to send to the   */
							/* remote device since it has been requested.      */
							GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type                            = QAPI_BLE_LAR_IDENTITY_INFORMATION_E;
							GAP_LE_Authentication_Response_Information.Authentication_Data_Length                            = (uint8_t)QAPI_BLE_GAP_LE_IDENTITY_INFORMATION_DATA_SIZE;
							GAP_LE_Authentication_Response_Information.Authentication_Data.Identity_Information.Address      = gBLE->localAddress;
							GAP_LE_Authentication_Response_Information.Authentication_Data.Identity_Information.Address_Type = QAPI_BLE_LAT_PUBLIC_IDENTITY_E;
							GAP_LE_Authentication_Response_Information.Authentication_Data.Identity_Information.IRK          = gBLE->IRK;

							qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
							break;
						case QAPI_BLE_LAT_ENCRYPTION_INFORMATION_REQUEST_E:
							/* Generate new LTK, EDIV and Rand and respond with*/
							/* them.                                           */
							if (!QAPI_BLE_COMPARE_NULL_BD_ADDR(Authentication_Event_Data->BD_ADDR)) {
								if (QAPI_OK == qapi_BLE_GAP_LE_Generate_Long_Term_Key(gBLE->stackID, &gBLE->DHK, &gBLE->ER, &GAP_LE_Authentication_Response_Information.Authentication_Data.Encryption_Information.LTK, &LocalDiv, &GAP_LE_Authentication_Response_Information.Authentication_Data.Encryption_Information.EDIV, &GAP_LE_Authentication_Response_Information.Authentication_Data.Encryption_Information.Rand)) {
									GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type                                     = QAPI_BLE_LAR_ENCRYPTION_INFORMATION_E;
									GAP_LE_Authentication_Response_Information.Authentication_Data_Length                                     = QAPI_BLE_GAP_LE_ENCRYPTION_INFORMATION_DATA_SIZE;
									GAP_LE_Authentication_Response_Information.Authentication_Data.Encryption_Information.Encryption_Key_Size = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Encryption_Key_Size;
									qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
								}
							}
							break;
						case QAPI_BLE_LAT_ENCRYPTION_INFORMATION_E:
							/* Search for the entry for this slave to store the*/
							/* information into.                               */
							connection = (modBLEClientConnection)modBLEConnectionFindByAddress((uint8_t*)&Authentication_Event_Data->BD_ADDR);
							if (connection) {
								c_memcpy(&(connection->LTK), &(Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.LTK), sizeof(connection->LTK));
								connection->EDIV              = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.EDIV;
								c_memcpy(&(connection->Rand), &(Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Rand), sizeof(connection->Rand));
								connection->EncryptionKeySize = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Encryption_Key_Size;
								connection->Flags            |= DEVICE_INFO_FLAGS_LTK_VALID;
							}
							break;
						case QAPI_BLE_LAT_SECURITY_ESTABLISHMENT_COMPLETE_E:
							/* If this failed due to a LTK issue then we should delete the LTK */
							if (Authentication_Event_Data->Authentication_Event_Data.Security_Establishment_Complete.Status == QAPI_BLE_GAP_LE_SECURITY_ESTABLISHMENT_STATUS_CODE_LONG_TERM_KEY_ERROR) {
								connection = (modBLEClientConnection)modBLEConnectionFindByAddress((uint8_t*)&Authentication_Event_Data->BD_ADDR);
								if (connection) {
									/* Clear the flag indicating the LTK is valid */
									connection->Flags &= ~DEVICE_INFO_FLAGS_LTK_VALID;
								}
							}
							break;
						case QAPI_BLE_LAT_PAIRING_STATUS_E:
							if (Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Status != QAPI_BLE_GAP_LE_PAIRING_STATUS_NO_ERROR) {
								/* Disconnect the Link.                         */
								/* * NOTE * Since we failed to pair, the remote */
								/*          device information will be deleted  */
								/*          when the GATT Disconnection event is*/
								/*          received.                           */
								qapi_BLE_GAP_LE_Disconnect(gBLE->stackID, Authentication_Event_Data->BD_ADDR);
							}
							modMessagePostToMachine(gBLE->the, (uint8_t*)Authentication_Event_Data, sizeof(qapi_BLE_GAP_LE_Authentication_Event_Data_t), gapAuthCompleteEvent, NULL);
							break;
						case QAPI_BLE_LAT_IDENTITY_INFORMATION_E:
							/* Search for the entry for this slave to store the*/
							/* information into.                               */
							connection = (modBLEClientConnection)modBLEConnectionFindByAddress((uint8_t*)&Authentication_Event_Data->BD_ADDR);
							if (connection) {
								c_memcpy(&(connection->IRK), &(Authentication_Event_Data->Authentication_Event_Data.Identity_Information.IRK), sizeof(connection->IRK));
								connection->IdentityAddressBD_ADDR = Authentication_Event_Data->Authentication_Event_Data.Identity_Information.Address;
								connection->IdentityAddressType    = Authentication_Event_Data->Authentication_Event_Data.Identity_Information.Address_Type;
								connection->Flags                 |= DEVICE_INFO_FLAGS_IRK_VALID;

								/* Setup the resolving list entry to add to the */
								/* resolving list.                              */
								connection->ResolvingListEntry.Peer_Identity_Address      = connection->IdentityAddressBD_ADDR;
								connection->ResolvingListEntry.Peer_Identity_Address_Type = connection->IdentityAddressType;
								connection->ResolvingListEntry.Peer_IRK                   = connection->IRK;
								connection->ResolvingListEntry.Local_IRK                  = gBLE->IRK;
							}
							break;
						case QAPI_BLE_LAT_EXTENDED_CONFIRMATION_REQUEST_E:
							switch (Authentication_Event_Data->Authentication_Event_Data.Extended_Confirmation_Request.Request_Type) {
								case QAPI_BLE_CRT_NONE_E:
									/* Handle the just works request.            */
									GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = QAPI_BLE_LAR_CONFIRMATION_E;

									/* By setting the Authentication_Data_Length */
									/* to any NON-ZERO value we are informing the*/
									/* GAP LE Layer that we are accepting Just   */
									/* Works Pairing.                            */
									GAP_LE_Authentication_Response_Information.Authentication_Data_Length = sizeof(uint32_t);

									if (QAPI_BLE_LIC_DISPLAY_ONLY_E == gBLE->pairingCapabilities.IO_Capability)
										GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Display_Passkey;
									
									qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);											
									break;
								case QAPI_BLE_CRT_PASSKEY_E:
									modMessagePostToMachine(gBLE->the, (uint8_t*)Authentication_Event_Data, sizeof(qapi_BLE_GAP_LE_Authentication_Event_Data_t), gapPasskeyRequestEvent, NULL);
									break;
								case QAPI_BLE_CRT_DISPLAY_E:
									modMessagePostToMachine(gBLE->the, (uint8_t*)Authentication_Event_Data, sizeof(qapi_BLE_GAP_LE_Authentication_Event_Data_t), gapPasskeyNotifyEvent, NULL);									
									GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type  = QAPI_BLE_LAR_PASSKEY_E;
									GAP_LE_Authentication_Response_Information.Authentication_Data_Length  = (uint8_t)(sizeof(uint32_t));
									GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = Authentication_Event_Data->Authentication_Event_Data.Extended_Confirmation_Request.Display_Passkey;
									qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
									break;
								case QAPI_BLE_CRT_DISPLAY_YES_NO_E:
									/* Check to see if this is Just Works or Numeric Comparison */
									if (Authentication_Event_Data->Authentication_Event_Data.Extended_Confirmation_Request.Flags & QAPI_BLE_GAP_LE_EXTENDED_CONFIRMATION_REQUEST_FLAGS_JUST_WORKS_PAIRING) {
										if (QAPI_BLE_LIC_DISPLAY_ONLY_E == &gBLE->pairingCapabilities)
											GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Display_Passkey;
										
										GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = QAPI_BLE_LAR_CONFIRMATION_E;
										GAP_LE_Authentication_Response_Information.Authentication_Data_Length = sizeof(uint32_t);
										
										qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);									
									}
									else {
										/* This is numeric comparison so go ahead */
										/* and display the numeric value to       */
										/* confirm.                               */
										modMessagePostToMachine(gBLE->the, (uint8_t*)Authentication_Event_Data, sizeof(qapi_BLE_GAP_LE_Authentication_Event_Data_t), gapPasskeyConfirmEvent, NULL);
									}
                              		break;
								case QAPI_BLE_CRT_OOB_SECURE_CONNECTIONS_E:	// not supported
									GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = QAPI_BLE_LAR_OUT_OF_BAND_DATA_E;
									GAP_LE_Authentication_Response_Information.Authentication_Data_Length = QAPI_BLE_GAP_LE_SECURE_CONNECTIONS_OOB_DATA_SIZE;
									GAP_LE_Authentication_Response_Information.Authentication_Data.Secure_Connections_OOB_Data.Flags = QAPI_BLE_GAP_LE_SECURE_CONNECTIONS_OOB_DATA_FLAGS_OOB_NOT_RECEIVED;
									qapi_BLE_GAP_LE_Authentication_Response(BluetoothStackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
									break;
							}
							break;
						}
						break;
					}
				}
				break;
			case QAPI_BLE_ET_LE_ADVERTISING_REPORT_E:
				for (uint16_t i = 0; i < GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Number_Device_Entries; i++) {
					qapi_BLE_GAP_LE_Advertising_Report_Data_t *Advertising_Data = &GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Advertising_Data[i];
					if (Advertising_Data->Raw_Report_Length) {
						deviceDiscoveryRecord *entry = c_malloc(sizeof(deviceDiscoveryRecord) - 1 + Advertising_Data->Raw_Report_Length);
						if (NULL != entry) {
							entry->disc = *Advertising_Data;
							c_memmove(entry->data, Advertising_Data->Raw_Report_Data, Advertising_Data->Raw_Report_Length);
							modBLEMessageQueueEnqueue(&gBLE->discoveryQueue, (modBLEMessageQueueEntry)entry);
						}
					}
				}
				break;
			default:
				break;
		}
	}
}

void QAPI_BLE_BTPSAPI GATT_Client_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Client_Event_Data_t *GATT_Client_Event_Data, uint32_t CallbackParameter)
{
	if (gBLE && gBLE->stackID && GATT_Client_Event_Data) {
	
		LOG_GATTC_EVENT(GATT_Client_Event_Data->Event_Data_Type);
		
		switch(GATT_Client_Event_Data->Event_Data_Type) {
			case QAPI_BLE_ET_GATT_CLIENT_READ_RESPONSE_E:
 				if (GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data) {
 					qapi_BLE_GATT_Read_Response_Data_t GATT_Read_Response_Data = *GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data;
 					uint8_t *value = c_malloc(GATT_Read_Response_Data.AttributeValueLength);
 					c_memmove(value, GATT_Read_Response_Data.AttributeValue, GATT_Read_Response_Data.AttributeValueLength);
 					GATT_Read_Response_Data.AttributeValue = value;
					modMessagePostToMachine(gBLE->the, (uint8_t*)&GATT_Read_Response_Data, sizeof(GATT_Read_Response_Data), characteristicReadValueEvent, NULL);
				}
				break;
			case QAPI_BLE_ET_GATT_CLIENT_WRITE_RESPONSE_E:
 				if (GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data) {
 					qapi_BLE_GATT_Write_Response_Data_t GATT_Write_Response_Data = *GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data;
					modMessagePostToMachine(gBLE->the, (uint8_t*)&GATT_Write_Response_Data, sizeof(GATT_Write_Response_Data), characteristicWriteValueEvent, NULL);
				}
				break;
			case QAPI_BLE_ET_GATT_CLIENT_CHARACTERISTIC_DISCOVERY_RESPONSE_E: {
				if (GATT_Client_Event_Data->Event_Data.GATT_Characteristic_Discovery_Response_Data) {
					characteristicSearchRecord *entry;
					qapi_BLE_GATT_Characteristic_Discovery_Response_Data_t GATT_Characteristic_Discovery_Response_Data = *GATT_Client_Event_Data->Event_Data.GATT_Characteristic_Discovery_Response_Data;
					if (0 == GATT_Characteristic_Discovery_Response_Data.NumberOfCharacteristics)
						entry = c_malloc(sizeof(characteristicSearchRecord));
					else
						entry = c_malloc(sizeof(characteristicSearchRecord) + ((GATT_Characteristic_Discovery_Response_Data.NumberOfCharacteristics - 1) * sizeof(qapi_BLE_GATT_Characteristic_Entry_t)));
					if (NULL != entry) {
						entry->conn_id = GATT_Characteristic_Discovery_Response_Data.ConnectionID;
						entry->count = GATT_Characteristic_Discovery_Response_Data.NumberOfCharacteristics;
						if (0 != entry->count)
							c_memmove(entry->CharacteristicEntryList, GATT_Characteristic_Discovery_Response_Data.CharacteristicEntryList, entry->count * sizeof(qapi_BLE_GATT_Characteristic_Entry_t));
						modBLEMessageQueueEnqueue(&gBLE->discoveryQueue, (modBLEMessageQueueEntry)entry);
					}
				}
				break;
			}
			case QAPI_BLE_ET_GATT_CLIENT_ERROR_RESPONSE_E: {
				if (GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data) {
					qapi_BLE_GATT_Request_Error_Data_t GATT_Request_Error_Data = *GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data;
					modMessagePostToMachine(gBLE->the, (uint8_t*)&GATT_Request_Error_Data, sizeof(GATT_Request_Error_Data), clientErrorEvent, NULL);	
				}				
				break;
			}
			case QAPI_BLE_ET_GATT_CLIENT_EXCHANGE_MTU_RESPONSE_E: {
				if (GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data) {
					qapi_BLE_GATT_Exchange_MTU_Response_Data_t GATT_Exchange_MTU_Response_Data = *GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data;
					modMessagePostToMachine(gBLE->the, (uint8_t*)&GATT_Exchange_MTU_Response_Data, sizeof(GATT_Exchange_MTU_Response_Data), mtuExchangedEvent, NULL);					
				}
				break;
			}
			default:
				break;
		}
	}
}

void QAPI_BLE_BTPSAPI GATT_Service_Discovery_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Service_Discovery_Event_Data_t *GATT_Service_Discovery_Event_Data, uint32_t CallbackParameter)
{
	if (BluetoothStackID && GATT_Service_Discovery_Event_Data) {
		switch(GATT_Service_Discovery_Event_Data->Event_Data_Type) {
			case QAPI_BLE_ET_GATT_SERVICE_DISCOVERY_COMPLETE_E:
            case QAPI_BLE_ET_GATT_SERVICE_DISCOVERY_INDICATION_E: {
				serviceSearchRecord *entry = c_malloc(sizeof(serviceSearchRecord));
				if (NULL != entry) {
					qapi_BLE_GATT_Service_Discovery_Indication_Data_t GATT_Service_Discovery_Indication_Data = *GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data;
					qapi_BLE_GATT_Service_Information_t *serviceInformation = &GATT_Service_Discovery_Indication_Data.ServiceInformation;
					entry->conn_id = GATT_Service_Discovery_Indication_Data.ConnectionID;
					entry->completed = (QAPI_BLE_ET_GATT_SERVICE_DISCOVERY_COMPLETE_E == GATT_Service_Discovery_Event_Data->Event_Data_Type);
					if (!entry->completed)
						entry->service = *serviceInformation;
					modBLEMessageQueueEnqueue(&gBLE->discoveryQueue, (modBLEMessageQueueEntry)entry);
				}
				break;
			}
			default:
				break;
		}
	}
}

void QAPI_BLE_BTPSAPI GATT_Descriptor_Discovery_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Service_Discovery_Event_Data_t *GATT_Service_Discovery_Event_Data, uint32_t CallbackParameter)
{
	DescriptorDiscoveryRequest *request = (DescriptorDiscoveryRequest*)CallbackParameter;
	
	if (BluetoothStackID && GATT_Service_Discovery_Event_Data) {
		switch(GATT_Service_Discovery_Event_Data->Event_Data_Type) {
			case QAPI_BLE_ET_GATT_SERVICE_DISCOVERY_COMPLETE_E:
            case QAPI_BLE_ET_GATT_SERVICE_DISCOVERY_INDICATION_E: {
				qapi_BLE_GATT_Service_Discovery_Indication_Data_t *GATT_Service_Discovery_Indication_Data = GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data;
            	descriptorSearchRecord *entry = NULL;
				if (QAPI_BLE_ET_GATT_SERVICE_DISCOVERY_COMPLETE_E == GATT_Service_Discovery_Event_Data->Event_Data_Type) {
					entry = c_malloc(sizeof(descriptorSearchRecord));
					if (NULL != entry)
						entry->completed = true;
				}
				else {
					qapi_BLE_GATT_Characteristic_Information_t *CharacteristicInformationList = GATT_Service_Discovery_Indication_Data->CharacteristicInformationList;					
					uint32_t i, NumberOfCharacteristics = GATT_Service_Discovery_Indication_Data->NumberOfCharacteristics;
					for (i = 0; i < NumberOfCharacteristics; ++i) {
						if (request->handle == CharacteristicInformationList[i].Characteristic_Handle) {
							if (0 == CharacteristicInformationList[i].NumberOfDescriptors)
								entry = c_malloc(sizeof(descriptorSearchRecord));
							else
								entry = c_malloc(sizeof(descriptorSearchRecord) + ((CharacteristicInformationList[i].NumberOfDescriptors - 1) * sizeof(qapi_BLE_GATT_Characteristic_Descriptor_Information_t)));
							if (NULL != entry) {
								entry->count = CharacteristicInformationList[i].NumberOfDescriptors;
								entry->completed = false;	
								if (0 != entry->count)
									c_memmove(entry->descriptorList, CharacteristicInformationList[i].DescriptorList, entry->count * sizeof(qapi_BLE_GATT_Characteristic_Descriptor_Information_t));
							}
							break;
						}
					}				
				}
				if (NULL != entry) {
					entry->conn_id = GATT_Service_Discovery_Indication_Data->ConnectionID;
					modBLEMessageQueueEnqueue(&gBLE->discoveryQueue, (modBLEMessageQueueEntry)entry);
				}
				break;
			}
			default:
				break;
		}
	}
}

#if LOG_GATTC
static void logGATTCEvent(qapi_BLE_GATT_Client_Event_Type_t event) {
	switch(event) {
		case QAPI_BLE_ET_GATT_CLIENT_ERROR_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_ERROR_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_CLIENT_SERVICE_DISCOVERY_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_SERVICE_DISCOVERY_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_CLIENT_SERVICE_DISCOVERY_BY_UUID_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_SERVICE_DISCOVERY_BY_UUID_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_CLIENT_INCLUDED_SERVICES_DISCOVERY_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_INCLUDED_SERVICES_DISCOVERY_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_CLIENT_CHARACTERISTIC_DISCOVERY_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_CHARACTERISTIC_DISCOVERY_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_CLIENT_CHARACTERISTIC_DESCRIPTOR_DISCOVERY_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_CHARACTERISTIC_DESCRIPTOR_DISCOVERY_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_CLIENT_READ_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_READ_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_CLIENT_READ_LONG_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_READ_LONG_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_CLIENT_READ_BY_UUID_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_READ_BY_UUID_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_CLIENT_READ_MULTIPLE_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_READ_MULTIPLE_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_CLIENT_WRITE_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_WRITE_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_CLIENT_PREPARE_WRITE_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_PREPARE_WRITE_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_CLIENT_EXECUTE_WRITE_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_EXECUTE_WRITE_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_CLIENT_EXCHANGE_MTU_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_CLIENT_EXCHANGE_MTU_RESPONSE_E"); break;
	}
}
#endif

#if LOG_GAP
static void logGAPEvent(qapi_BLE_GAP_LE_Event_Type_t event) {
	switch(event) {
		case QAPI_BLE_ET_LE_REMOTE_FEATURES_RESULT_E: modLog("QAPI_BLE_ET_LE_REMOTE_FEATURES_RESULT_E"); break;
		case QAPI_BLE_ET_LE_ADVERTISING_REPORT_E: modLog("QAPI_BLE_ET_LE_ADVERTISING_REPORT_E"); break;
		case QAPI_BLE_ET_LE_CONNECTION_COMPLETE_E: modLog("QAPI_BLE_ET_LE_CONNECTION_COMPLETE_E"); break;
		case QAPI_BLE_ET_LE_DISCONNECTION_COMPLETE_E: modLog("QAPI_BLE_ET_LE_DISCONNECTION_COMPLETE_E"); break;
		case QAPI_BLE_ET_LE_ENCRYPTION_CHANGE_E: modLog("QAPI_BLE_ET_LE_ENCRYPTION_CHANGE_E"); break;
		case QAPI_BLE_ET_LE_ENCRYPTION_REFRESH_COMPLETE_E: modLog("QAPI_BLE_ET_LE_ENCRYPTION_REFRESH_COMPLETE_E"); break;
		case QAPI_BLE_ET_LE_AUTHENTICATION_E: modLog("QAPI_BLE_ET_LE_AUTHENTICATION_E"); break;
		case QAPI_BLE_ET_LE_CONNECTION_PARAMETER_UPDATE_REQUEST_E: modLog("QAPI_BLE_ET_LE_CONNECTION_PARAMETER_UPDATE_REQUEST_E"); break;
		case QAPI_BLE_ET_LE_CONNECTION_PARAMETER_UPDATE_RESPONSE_E: modLog("QAPI_BLE_ET_LE_CONNECTION_PARAMETER_UPDATE_RESPONSE_E"); break;
		case QAPI_BLE_ET_LE_CONNECTION_PARAMETER_UPDATED_E: modLog("QAPI_BLE_ET_LE_CONNECTION_PARAMETER_UPDATED_E"); break;
		case QAPI_BLE_ET_LE_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED_E: modLog("QAPI_BLE_ET_LE_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED_E"); break;
		case QAPI_BLE_ET_LE_DIRECT_ADVERTISING_REPORT_E: modLog("QAPI_BLE_ET_LE_DIRECT_ADVERTISING_REPORT_E"); break;
		case QAPI_BLE_ET_LE_DATA_LENGTH_CHANGE_E: modLog("QAPI_BLE_ET_LE_DATA_LENGTH_CHANGE_E"); break;
		case QAPI_BLE_ET_LE_PHY_UPDATE_COMPLETE_E: modLog("QAPI_BLE_ET_LE_PHY_UPDATE_COMPLETE_E"); break;
		case QAPI_BLE_ET_LE_EXTENDED_ADVERTISING_REPORT_E: modLog("QAPI_BLE_ET_LE_EXTENDED_ADVERTISING_REPORT_E"); break;
		case QAPI_BLE_ET_LE_RFU0_E: modLog("QAPI_BLE_ET_LE_RFU0_E"); break;
		case QAPI_BLE_ET_LE_RFU1_E: modLog("QAPI_BLE_ET_LE_RFU1_E"); break;
		case QAPI_BLE_ET_LE_RFU2_E: modLog("QAPI_BLE_ET_LE_RFU2_E"); break;
		case QAPI_BLE_ET_LE_SCAN_TIMEOUT_E: modLog("QAPI_BLE_ET_LE_SCAN_TIMEOUT_E"); break;
		case QAPI_BLE_ET_LE_ADVERTISING_SET_TERMINATED_E: modLog("QAPI_BLE_ET_LE_ADVERTISING_SET_TERMINATED_E"); break;
		case QAPI_BLE_ET_LE_SCAN_REQUEST_RECEIVED_E: modLog("QAPI_BLE_ET_LE_SCAN_REQUEST_RECEIVED_E"); break;
		case QAPI_BLE_ET_LE_CHANNEL_SELECTION_ALGORITHM_UPDATE_E: modLog("QAPI_BLE_ET_LE_CHANNEL_SELECTION_ALGORITHM_UPDATE_E"); break;
	}
}
#endif

#if LOG_GAP_AUTHENTICATION
static void logGAPAuthenticationEvent(qapi_BLE_GAP_LE_Authentication_Event_Type_t event) {
	switch(event) {
		case QAPI_BLE_LAT_LONG_TERM_KEY_REQUEST_E: modLog("     QAPI_BLE_LAT_LONG_TERM_KEY_REQUEST_E"); break;
		case QAPI_BLE_LAT_SECURITY_REQUEST_E: modLog("     QAPI_BLE_LAT_SECURITY_REQUEST_E"); break;
		case QAPI_BLE_LAT_PAIRING_REQUEST_E: modLog("     QAPI_BLE_LAT_PAIRING_REQUEST_E"); break;
		case QAPI_BLE_LAT_CONFIRMATION_REQUEST_E: modLog("     QAPI_BLE_LAT_CONFIRMATION_REQUEST_E"); break;
		case QAPI_BLE_LAT_PAIRING_STATUS_E: modLog("     QAPI_BLE_LAT_PAIRING_STATUS_E"); break;
		case QAPI_BLE_LAT_ENCRYPTION_INFORMATION_REQUEST_E: modLog("     QAPI_BLE_LAT_ENCRYPTION_INFORMATION_REQUEST_E"); break;
		case QAPI_BLE_LAT_IDENTITY_INFORMATION_REQUEST_E: modLog("     QAPI_BLE_LAT_IDENTITY_INFORMATION_REQUEST_E"); break;
		case QAPI_BLE_LAT_SIGNING_INFORMATION_REQUEST_E: modLog("     QAPI_BLE_LAT_SIGNING_INFORMATION_REQUEST_E"); break;
		case QAPI_BLE_LAT_ENCRYPTION_INFORMATION_E: modLog("     QAPI_BLE_LAT_ENCRYPTION_INFORMATION_E"); break;
		case QAPI_BLE_LAT_IDENTITY_INFORMATION_E: modLog("     QAPI_BLE_LAT_IDENTITY_INFORMATION_E"); break;
		case QAPI_BLE_LAT_SIGNING_INFORMATION_E: modLog("     QAPI_BLE_LAT_SIGNING_INFORMATION_E"); break;
		case QAPI_BLE_LAT_SECURITY_ESTABLISHMENT_COMPLETE_E: modLog("     QAPI_BLE_LAT_SECURITY_ESTABLISHMENT_COMPLETE_E"); break;
		case QAPI_BLE_LAT_KEYPRESS_NOTIFICATION_E: modLog("     QAPI_BLE_LAT_KEYPRESS_NOTIFICATION_E"); break;
		case QAPI_BLE_LAT_EXTENDED_PAIRING_REQUEST_E: modLog("     QAPI_BLE_LAT_EXTENDED_PAIRING_REQUEST_E"); break;
		case QAPI_BLE_LAT_EXTENDED_CONFIRMATION_REQUEST_E: modLog("     QAPI_BLE_LAT_EXTENDED_CONFIRMATION_REQUEST_E"); break;
		case QAPI_BLE_LAT_EXTENDED_OUT_OF_BAND_INFORMATION_E: modLog("     QAPI_BLE_LAT_EXTENDED_OUT_OF_BAND_INFORMATION_E"); break;
	}
}
#endif



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
	xsSlot obj;
} GATTProcedureRecord;

typedef struct modBLEConnectionRecord modBLEConnectionRecord;
typedef modBLEConnectionRecord *modBLEConnection;

struct modBLEConnectionRecord {
	struct modBLEConnectionRecord *next;

	xsMachine *the;
	xsSlot objConnection;
	xsSlot objClient;

	uint32_t id;
	qapi_BLE_BD_ADDR_t bd_addr;
	qapi_BLE_GAP_LE_Address_Type_t bd_addr_type;
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
	modBLEConnection connections;
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

static void modBLEConnectionAdd(modBLEConnection connection);
static void modBLEConnectionRemove(modBLEConnection connection);
static modBLEConnection modBLEConnectionFindByConnectionID(uint32_t conn_id);
static modBLEConnection modBLEConnectionFindByAddress(qapi_BLE_BD_ADDR_t bd_addr);

static void uuidToBuffer(qapi_BLE_GATT_UUID_t *uuid, uint8_t *buffer, uint16_t *length);
static void bufferToUUID(uint8_t *buffer, qapi_BLE_GATT_UUID_t *uuid, uint16_t length);
static uint8_t UUIDEqual(qapi_BLE_GATT_UUID_t *uuid1, qapi_BLE_GATT_UUID_t *uuid2);

static void addressToBuffer(qapi_BLE_BD_ADDR_t BD_ADDR, uint8_t *buffer);
static void bufferToAddress(uint8_t *buffer, qapi_BLE_BD_ADDR_t *BD_ADDR);

static void readyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void completeProcedure(uint32_t conn_id, char *which);

static void QAPI_BLE_BTPSAPI GATT_Connection_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, uint32_t CallbackParameter);
static void QAPI_BLE_BTPSAPI GAP_LE_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GAP_LE_Event_Data_t *GAP_LE_Event_Data, uint32_t CallbackParameter);
static void QAPI_BLE_BTPSAPI GATT_Client_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Client_Event_Data_t *GATT_Client_Event_Data, uint32_t CallbackParameter);
static void QAPI_BLE_BTPSAPI GATT_Service_Discovery_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Service_Discovery_Event_Data_t *GATT_Service_Discovery_Event_Data, uint32_t CallbackParameter);
static void QAPI_BLE_BTPSAPI GATT_Descriptor_Discovery_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Service_Discovery_Event_Data_t *GATT_Service_Discovery_Event_Data, uint32_t CallbackParameter);

static modBLE gBLE = NULL;

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
	gBLE->stackID = result;
	
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
	if (ble) {
		qapi_BLE_GATT_Cleanup(ble->stackID);
		qapi_BLE_BSC_Shutdown(ble->stackID);
		modBLEConnection connections = ble->connections;
		while (connections != NULL) {
			modBLEConnection connection = connections;
			connections = connections->next;
			c_free(connection);
		}
		c_free(ble);
	}
	gBLE = NULL;
}

void xs_ble_client_set_local_privacy(xsMachine *the)
{
}

void xs_ble_client_start_scanning(xsMachine *the)
{
	uint8_t active = xsmcToBoolean(xsArg(0));
	uint16_t interval = xsmcToInteger(xsArg(1));
	uint16_t window = xsmcToInteger(xsArg(2));
	uint32_t result;
	
	gBLE->scanInterval = (uint32_t)(interval / 0.625);	// convert to 1 ms units
	gBLE->scanWindow = (uint32_t)(window / 0.625);

	result = qapi_BLE_GAP_LE_Perform_Scan(
		gBLE->stackID,
		active ? QAPI_BLE_ST_ACTIVE_E : QAPI_BLE_ST_PASSIVE_E,
		gBLE->scanInterval,
		gBLE->scanWindow,
		QAPI_BLE_LAT_PUBLIC_E,
		QAPI_BLE_FP_NO_FILTER_E,
		false,							// no filtering
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
	qapi_BLE_BD_ADDR_t bd_addr;
	int result;

	if (gBLE->scanning) {
		qapi_BLE_GAP_LE_Cancel_Scan(gBLE->stackID);
		gBLE->scanning = 0;
	}

	bufferToAddress(address, &bd_addr);

	// Ignore duplicate connection attempts
	if (modBLEConnectionFindByAddress(bd_addr)) {
		return;
	};
	
	// Add a new connection record to be filled as the connection completes
	modBLEConnection connection = c_calloc(sizeof(modBLEConnectionRecord), 1);
	if (!connection)
		xsUnknownError("out of memory");
	connection->id = 0;
	connection->bond = 0xFF;
	connection->bd_addr = bd_addr;
	modBLEConnectionAdd(connection);
	
	result = qapi_BLE_GAP_LE_Create_Connection(
		gBLE->stackID,
		gBLE->scanInterval,
		gBLE->scanWindow,
		QAPI_BLE_FP_NO_FILTER_E,
		addressType,			// remote address type
		&bd_addr,				// remote address
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
	qapi_BLE_BD_ADDR_t bd_addr;
	modBLEConnection connection;
	
	bufferToAddress(address, &bd_addr);
	connection = modBLEConnectionFindByAddress(bd_addr);
	if (!connection) return;
	
	GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = (confirm ? QAPI_BLE_LAR_CONFIRMATION_E : QAPI_BLE_LAR_ERROR_E);
	GAP_LE_Authentication_Response_Information.Authentication_Data_Length = sizeof(uint32_t);
	GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = connection->passkey;											
	qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, bd_addr, &GAP_LE_Authentication_Response_Information);									
}

modBLEConnection modBLEConnectionFindByConnectionID(uint32_t conn_id)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (conn_id == walker->id)
			break;
	return walker;
}

modBLEConnection modBLEConnectionFindByAddress(qapi_BLE_BD_ADDR_t bd_addr)
{
	modBLEConnection walker;
	for (walker = gBLE->connections; NULL != walker; walker = walker->next)
		if (QAPI_BLE_COMPARE_BD_ADDR(bd_addr, walker->bd_addr))
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

void xs_gap_connection_initialize(xsMachine *the)
{
	uint32_t conn_id;
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
	uint32_t conn_id = xsmcToInteger(xsArg(0));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
	qapi_BLE_GAP_LE_Disconnect(gBLE->stackID, connection->bd_addr);
}

void xs_gap_connection_read_rssi(xsMachine *the)
{
	xsUnknownError("unimplemented");	// @@ Only available from HCI connection??
}

void xs_gatt_client_discover_primary_services(xsMachine *the)
{
	uint32_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t argc = xsmcArgc;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	connection->procedure.what = GATT_PROCEDURE_DISCOVER_SERVICES;
	connection->procedure.obj = connection->objClient;
	
	if (argc > 1) {
		qapi_BLE_GATT_UUID_t uuid;
		bufferToUUID((uint8_t*)xsmcToArrayBuffer(xsArg(1)), &uuid, xsGetArrayBufferLength(xsArg(1)));
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
	
   	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	request = c_calloc(1, sizeof(CharacteristicDiscoveryRequest));
	if (!request)
		xsUnknownError("out of memory");
		
	request->start = start;
	request->end = end;
	if (argc > 3)
		bufferToUUID((uint8_t*)xsmcToArrayBuffer(xsArg(3)), &request->uuid, xsGetArrayBufferLength(xsArg(3)));
	
	connection->procedure.what = GATT_PROCEDURE_DISCOVER_CHARACTERISTICS;
	connection->procedure.obj = xsThis;
	connection->procedure.refcon = (uint32_t)request;	
	
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
	
   	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	request = c_calloc(1, sizeof(DescriptorDiscoveryRequest));
	if (!request)
		xsUnknownError("out of memory");
		
	request->handle = handle;
	request->conn_id = conn_id;
		
	connection->procedure.what = GATT_PROCEDURE_DISCOVER_DESCRIPTORS;
	connection->procedure.obj = xsThis;
	connection->procedure.refcon = (uint32_t)request;

	// we use service discovery to discover all descriptors by matching the parent characteristic in the discovery results
	xsmcVars(2);
	xsmcGet(xsVar(0), xsThis, xsID_service);
	xsmcGet(xsVar(1), xsVar(0), xsID_uuid);
	bufferToUUID((uint8_t*)xsmcToArrayBuffer(xsVar(1)), &uuid, xsGetArrayBufferLength(xsVar(1)));
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
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	connection->procedure.what = GATT_PROCEDURE_READ_CHARACTERISTIC;
	connection->procedure.obj = connection->objClient;
	connection->procedure.refcon = handle;
	
	qapi_BLE_GATT_Read_Value_Request(gBLE->stackID, conn_id, handle, GATT_Client_Event_Callback, conn_id);
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
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	connection->procedure.what = GATT_PROCEDURE_READ_DESCRIPTOR;
	connection->procedure.obj = connection->objClient;
	connection->procedure.refcon = handle;
	
	qapi_BLE_GATT_Read_Value_Request(gBLE->stackID, conn_id, handle, GATT_Client_Event_Callback, conn_id);
}

void xs_gatt_characteristic_enable_notifications(xsMachine *the)
{
	CharacteristicNotificationRequest *request;
    uint8_t data[2] = {0x01, 0x00};
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;    
	
	request = c_calloc(1, sizeof(CharacteristicNotificationRequest));
	if (!request)
		xsUnknownError("out of memory");

	request->handle = characteristic;
	request->conn_id = conn_id;
	request->enable = true;

	connection->procedure.what = GATT_PROCEDURE_ENABLE_CHARACTERISTIC_NOTIFICATIONS;
	connection->procedure.obj = connection->objClient;
	connection->procedure.refcon = (uint32_t)request;

	qapi_BLE_GATT_Write_Request(gBLE->stackID, conn_id, characteristic + 1, sizeof(data), data, GATT_Client_Event_Callback, conn_id);
}

void xs_gatt_characteristic_disable_notifications(xsMachine *the)
{
	CharacteristicNotificationRequest *request;
	uint8_t data[2] = {0x00, 0x00};
	uint8_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t characteristic = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;    

	request = c_calloc(1, sizeof(CharacteristicNotificationRequest));
	if (!request)
		xsUnknownError("out of memory");

	request->handle = characteristic;
	request->conn_id = conn_id;
	request->enable = false;
	
	connection->procedure.what = GATT_PROCEDURE_DISABLE_CHARACTERISTIC_NOTIFICATIONS;
	connection->procedure.obj = connection->objClient;
	connection->procedure.refcon = (uint32_t)request;

	qapi_BLE_GATT_Write_Request(gBLE->stackID, conn_id, characteristic + 1, sizeof(data), data, GATT_Client_Event_Callback, conn_id);
}

void xs_gatt_descriptor_write_value(xsMachine *the)
{
	uint16_t conn_id = xsmcToInteger(xsArg(0));
	uint16_t handle = xsmcToInteger(xsArg(1));
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	char *str;
	switch (xsmcTypeOf(xsArg(2))) {
		case xsStringType:
			str = xsmcToString(xsArg(2));
			qapi_BLE_GATT_Write_Without_Response_Request(gBLE->stackID, conn_id, handle, c_strlen(str), (uint8_t*)str);
			break;
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype)) {
				qapi_BLE_GATT_Write_Without_Response_Request(gBLE->stackID, conn_id, handle, xsGetArrayBufferLength(xsArg(2)), (uint8_t*)xsmcToArrayBuffer(xsArg(2)));
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
	char *str;

	switch (xsmcTypeOf(xsArg(2))) {
		case xsStringType:
			str = xsmcToString(xsArg(2));
			qapi_BLE_GATT_Write_Without_Response_Request(gBLE->stackID, conn_id, handle, c_strlen(str), (uint8_t*)str);
			break;
		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype)) {
				qapi_BLE_GATT_Write_Without_Response_Request(gBLE->stackID, conn_id, handle, xsGetArrayBufferLength(xsArg(2)), (uint8_t*)xsmcToArrayBuffer(xsArg(2)));
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

static void completeProcedure(uint32_t conn_id, char *which)
{
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;

	xsSlot obj = connection->procedure.obj;
	
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

static void addressToBuffer(qapi_BLE_BD_ADDR_t BD_ADDR, uint8_t *buffer)
{
	buffer[0] = BD_ADDR.BD_ADDR5;
	buffer[1] = BD_ADDR.BD_ADDR4;
	buffer[2] = BD_ADDR.BD_ADDR3;
	buffer[3] = BD_ADDR.BD_ADDR2;
	buffer[4] = BD_ADDR.BD_ADDR1;
	buffer[5] = BD_ADDR.BD_ADDR0;
}

static void bufferToAddress(uint8_t *buffer, qapi_BLE_BD_ADDR_t *BD_ADDR)
{
	BD_ADDR->BD_ADDR5 = buffer[0];
	BD_ADDR->BD_ADDR4 = buffer[1];
	BD_ADDR->BD_ADDR3 = buffer[2];
	BD_ADDR->BD_ADDR2 = buffer[3];
	BD_ADDR->BD_ADDR1 = buffer[4];
	BD_ADDR->BD_ADDR0 = buffer[5];
}

static int modBLEConnectionSaveAttHandle(modBLEConnection connection, qapi_BLE_GATT_UUID_t *uuid, uint16_t handle)
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

static void readyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsBeginHost(gBLE->the);
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
	xsEndHost(gBLE->the);
}

static void connectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Device_Connection_Data_t *result = (qapi_BLE_GATT_Device_Connection_Data_t*)message;
	uint8_t buffer[6];
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByAddress(result->RemoteDevice);
	if (!connection)
		xsUnknownError("connection not found");
		
	// Ignore duplicate connection events
	if (0 != connection->id)
		goto bail;

	connection->id = result->ConnectionID;
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), connection->id);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	addressToBuffer(result->RemoteDevice, buffer);
	xsmcSetArrayBuffer(xsVar(2), buffer, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
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
	if (!connection) {
		goto bail;
	}	
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), connection->id);
	xsCall2(connection->objConnection, xsID_callback, xsString("onDisconnected"), xsVar(0));
	modBLEConnectionRemove(connection);
bail:
	xsEndHost(gBLE->the);
}

static void deviceDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GAP_LE_Advertising_Report_Data_t *result = (qapi_BLE_GAP_LE_Advertising_Report_Data_t*)message;
	uint8_t buffer[6];
	
	if (!gBLE->scanning) {
		c_free(result->Raw_Report_Data);
		return;
	}
	
	xsBeginHost(gBLE->the);
	xsmcVars(4);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), result->Raw_Report_Data, result->Raw_Report_Length);
	addressToBuffer(result->BD_ADDR, buffer);
	xsmcSetArrayBuffer(xsVar(2), buffer, 6);
	xsmcSetInteger(xsVar(3), result->Address_Type);
	xsmcSet(xsVar(0), xsID_scanResponse, xsVar(1));
	xsmcSet(xsVar(0), xsID_address, xsVar(2));
	xsmcSet(xsVar(0), xsID_addressType, xsVar(3));
	xsCall2(gBLE->obj, xsID_callback, xsString("onDiscovered"), xsVar(0));
	c_free(result->Raw_Report_Data);
	xsEndHost(gBLE->the);
}

static void serviceDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Service_Discovery_Indication_Data_t *GATT_Service_Discovery_Indication_Data = (qapi_BLE_GATT_Service_Discovery_Indication_Data_t*)message;
	qapi_BLE_GATT_Service_Information_t *serviceInformation = &GATT_Service_Discovery_Indication_Data->ServiceInformation;
	uint32_t conn_id = GATT_Service_Discovery_Indication_Data->ConnectionID;
	uint8_t buffer[16];
	uint16_t length;
		
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
		
	xsmcVars(4);
	
	xsVar(0) = xsmcNewObject();
	uuidToBuffer(&serviceInformation->UUID, buffer, &length);
	xsmcSetArrayBuffer(xsVar(1), buffer, length);
	xsmcSetInteger(xsVar(2), serviceInformation->Service_Handle);
	xsmcSetInteger(xsVar(3), serviceInformation->End_Group_Handle);
	xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	xsmcSet(xsVar(0), xsID_start, xsVar(2));
	xsmcSet(xsVar(0), xsID_end, xsVar(3));
	xsCall2(connection->procedure.obj, xsID_callback, xsString("onService"), xsVar(0));

	xsEndHost(gBLE->the);
}

static void descriptorDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Characteristic_Information_t *characteristicInfo = (qapi_BLE_GATT_Characteristic_Information_t*)message;
	DescriptorDiscoveryRequest *request = (DescriptorDiscoveryRequest*)refcon;
	uint32_t conn_id = (uint32_t)request->conn_id;
	uint8_t buffer[16];
	uint16_t i, length;
		
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
		
	xsmcVars(3);

	for (i = 0; i < characteristicInfo->NumberOfDescriptors; ++i) {
		qapi_BLE_GATT_Characteristic_Descriptor_Information_t *descriptorInfo = &characteristicInfo->DescriptorList[i];
		int index = modBLEConnectionSaveAttHandle(connection, &descriptorInfo->Characteristic_Descriptor_UUID, descriptorInfo->Characteristic_Descriptor_Handle);
		xsVar(0) = xsmcNewObject();
		uuidToBuffer(&descriptorInfo->Characteristic_Descriptor_UUID, buffer, &length);
		xsmcSetArrayBuffer(xsVar(1), buffer, length);
		xsmcSetInteger(xsVar(2), descriptorInfo->Characteristic_Descriptor_Handle);
		xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
		xsmcSet(xsVar(0), xsID_handle, xsVar(2));
		if (-1 != index) {
			xsmcSetString(xsVar(2), (char*)char_names[index].name);
			xsmcSet(xsVar(0), xsID_name, xsVar(2));
			xsmcSetString(xsVar(2), (char*)char_names[index].type);
			xsmcSet(xsVar(0), xsID_type, xsVar(2));
		}
		xsCall2(connection->procedure.obj, xsID_callback, xsString("onDescriptor"), xsVar(0));
	}
	c_free(characteristicInfo->DescriptorList);

	xsEndHost(gBLE->the);
}

static void serviceDiscoveryCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Service_Discovery_Complete_Data_t *GATT_Service_Discovery_Complete_Data = (qapi_BLE_GATT_Service_Discovery_Complete_Data_t*)message;
	completeProcedure(GATT_Service_Discovery_Complete_Data->ConnectionID, "onService");
}

static void descriptorDiscoveryCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Service_Discovery_Complete_Data_t *GATT_Service_Discovery_Complete_Data = (qapi_BLE_GATT_Service_Discovery_Complete_Data_t*)message;
	completeProcedure(GATT_Service_Discovery_Complete_Data->ConnectionID, "onDescriptor");
}

static void characteristicDiscoveryEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	uint8_t buffer[16];
	uint16_t i, start, length;
	qapi_BLE_GATT_Characteristic_Discovery_Response_Data_t *result = (qapi_BLE_GATT_Characteristic_Discovery_Response_Data_t*)message;
	uint32_t conn_id = result->ConnectionID;
		
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
		
	CharacteristicDiscoveryRequest *request = (CharacteristicDiscoveryRequest*)connection->procedure.refcon;
	
	xsmcVars(4);
	xsVar(0) = xsmcNewObject();
	for (i = 0; i < result->NumberOfCharacteristics; ++i) {
		qapi_BLE_GATT_Characteristic_Entry_t *characteristic = &result->CharacteristicEntryList[i];
		if (0 != request->uuid.UUID.UUID_16.UUID_Byte0) {
			if (!UUIDEqual(&request->uuid, &characteristic->CharacteristicValue.CharacteristicUUID))
				continue;
		}
		int index = modBLEConnectionSaveAttHandle(connection, &characteristic->CharacteristicValue.CharacteristicUUID, characteristic->CharacteristicValue.CharacteristicValueHandle);
		uuidToBuffer(&characteristic->CharacteristicValue.CharacteristicUUID, buffer, &length);
		xsmcSetArrayBuffer(xsVar(1), buffer, length);
		xsmcSetInteger(xsVar(2), characteristic->CharacteristicValue.CharacteristicValueHandle);
		xsmcSetInteger(xsVar(3), characteristic->CharacteristicValue.CharacteristicProperties);
		xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
		xsmcSet(xsVar(0), xsID_handle, xsVar(2));
		xsmcSet(xsVar(0), xsID_properties, xsVar(3));
		if (-1 != index) {
			xsmcSetString(xsVar(2), (char*)char_names[index].name);
			xsmcSet(xsVar(0), xsID_name, xsVar(2));
			xsmcSetString(xsVar(2), (char*)char_names[index].type);
			xsmcSet(xsVar(0), xsID_type, xsVar(2));
		}
		xsCall2(connection->procedure.obj, xsID_callback, xsString("onCharacteristic"), xsVar(0));
	}
		
	c_free(result->CharacteristicEntryList);
	
	if (0 == result->NumberOfCharacteristics)
		start = request->end;
	else
		start = result->CharacteristicEntryList[result->NumberOfCharacteristics-1].CharacteristicValue.CharacteristicValueHandle + 1;
		
	if (start < request->end) {
		connection->procedure.transaction_id = qapi_BLE_GATT_Discover_Characteristics(gBLE->stackID, conn_id, start, request->end, GATT_Client_Event_Callback, conn_id);
	}
	else {
		c_free(request);
		completeProcedure(conn_id, "onCharacteristic");
	}

	xsEndHost(gBLE->the);
}

static void characteristicReadValueEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Read_Response_Data_t *result = (qapi_BLE_GATT_Read_Response_Data_t*)message;
	uint32_t conn_id = result->ConnectionID;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection)
		xsUnknownError("connection not found");
		
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), result->AttributeValue, result->AttributeValueLength);
	xsmcSetInteger(xsVar(2), connection->procedure.refcon);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->procedure.obj, xsID_callback, xsString("onCharacteristicValue"), xsVar(0));
	
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
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
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
	qapi_BLE_GATT_Server_Notification_Data_t *result = (qapi_BLE_GATT_Server_Notification_Data_t*)message;
	xsBeginHost(gBLE->the);
	modBLEConnection connection = modBLEConnectionFindByConnectionID(result->ConnectionID);
	if (!connection)
		xsUnknownError("connection not found");
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	xsmcSetArrayBuffer(xsVar(1), result->AttributeValue, result->AttributeValueLength);
	xsmcSetInteger(xsVar(2), result->AttributeHandle);
	xsmcSet(xsVar(0), xsID_value, xsVar(1));
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	xsCall2(connection->objClient, xsID_callback, xsString("onCharacteristicNotification"), xsVar(0));
	c_free(result->AttributeValue);
	xsEndHost(gBLE->the);
}

static void clientErrorEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Request_Error_Data_t *result = (qapi_BLE_GATT_Request_Error_Data_t*)message;
	uint32_t conn_id = result->ConnectionID;
	modBLEConnection connection = modBLEConnectionFindByConnectionID(conn_id);
	if (!connection) return;
	
	if (result->TransactionID == connection->procedure.transaction_id) {
		if (GATT_PROCEDURE_DISCOVER_CHARACTERISTICS == connection->procedure.what) {
			c_free((void*)connection->procedure.refcon);
			completeProcedure(conn_id, "onCharacteristic");
		}
	}
}

static void gapPasskeyRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GAP_LE_Authentication_Event_Data_t *Authentication_Event_data = (qapi_BLE_GAP_LE_Authentication_Event_Data_t*)message;
	qapi_BLE_GAP_LE_Authentication_Response_Information_t GAP_LE_Authentication_Response_Information;
	uint32_t passkey;
	uint8_t buffer[6];
	modBLEConnection connection;
	
	connection = modBLEConnectionFindByAddress(Authentication_Event_data->BD_ADDR);
	if (!connection) return;
	
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	addressToBuffer(Authentication_Event_data->BD_ADDR, buffer);
	xsmcSetArrayBuffer(xsVar(1), buffer, 6);
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
	uint8_t buffer[6];
	modBLEConnection connection;
	
	connection = modBLEConnectionFindByAddress(Authentication_Event_data->BD_ADDR);
	if (!connection) return;
		
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	addressToBuffer(Authentication_Event_data->BD_ADDR, buffer);
	xsmcSetArrayBuffer(xsVar(1), buffer, 6);
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
	uint8_t buffer[6];
	modBLEConnection connection;
	
	connection = modBLEConnectionFindByAddress(Authentication_Event_data->BD_ADDR);
	if (!connection) return;
	
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	addressToBuffer(Authentication_Event_data->BD_ADDR, buffer);
	connection->passkey = passkey;
	xsmcSetArrayBuffer(xsVar(1), buffer, 6);
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
	modBLEBondedDevice device;
	modBLEConnection connection = modBLEConnectionFindByAddress(Authentication_Event_Data->BD_ADDR);
	if (NULL == connection) return;
	if (QAPI_BLE_GAP_LE_PAIRING_STATUS_NO_ERROR == Pairing_Status->Status) {
		if (gBLE->bonding && ((connection->Flags & (DEVICE_INFO_FLAGS_LTK_VALID | DEVICE_INFO_FLAGS_IRK_VALID)) == (DEVICE_INFO_FLAGS_LTK_VALID | DEVICE_INFO_FLAGS_IRK_VALID))) {
			device = modBLEBondedDevicesFindByAddress(connection->bd_addr);
			if (NULL != device)
				modBLEBondedDevicesRemove(device);
			device = c_calloc(sizeof(modBLEBondedDeviceRecord), 1);
			if (NULL != device) {
				device->Flags = connection->Flags;
				device->LastAddress = connection->bd_addr;
				device->LastAddressType = connection->bd_addr_type;
				device->IdentityAddress = connection->IdentityAddressBD_ADDR;
				device->IdentityAddressType = connection->IdentityAddressType;
				device->EncryptionKeySize = connection->EncryptionKeySize;
				device->IRK = connection->IRK;
				device->LTK = connection->LTK;
				modBLEBondedDevicesAdd(device);
			}
		}
		xsBeginHost(gBLE->the);
		xsCall1(gBLE->obj, xsID_callback, xsString("onAuthenticated"));
		xsEndHost(gBLE->the);
	}
	else {
		device = modBLEBondedDevicesFindByAddress(connection->bd_addr);
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
					uint8_t *value = c_malloc(GATT_Server_Notification_Data.AttributeValueLength);
					c_memmove(value, GATT_Server_Notification_Data.AttributeValue, GATT_Server_Notification_Data.AttributeValueLength);
					GATT_Server_Notification_Data.AttributeValue = value;
					modMessagePostToMachine(gBLE->the, (uint8_t*)&GATT_Server_Notification_Data, sizeof(GATT_Server_Notification_Data), notificationEvent, NULL);				
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
					modBLEConnection connection = modBLEConnectionFindByAddress(Connection_Complete_Event_Data->Peer_Address);
					if (connection)
						connection->bd_addr_type = Connection_Complete_Event_Data->Peer_Address_Type;
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
					modBLEConnection connection;
					
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
								connection = modBLEConnectionFindByAddress(Authentication_Event_Data->BD_ADDR);
								if (connection) {
									modBLEConnection DeviceInfo = connection;
									/* Check to see if the LTK is valid.         */
									if (DeviceInfo->Flags & DEVICE_INFO_FLAGS_LTK_VALID) {
										/* Respond with the stored Long Term Key. */
										GAP_LE_Authentication_Response_Information.Authentication_Data_Length                                        = QAPI_BLE_GAP_LE_LONG_TERM_KEY_INFORMATION_DATA_SIZE;
										GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Encryption_Key_Size = DeviceInfo->EncryptionKeySize;

										c_memcpy(&(GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Long_Term_Key), &(DeviceInfo->LTK), QAPI_BLE_LONG_TERM_KEY_SIZE);
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
								modBLEConnection connection = modBLEConnectionFindByAddress(Authentication_Event_Data->BD_ADDR);
								
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
								}							}
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
							connection = modBLEConnectionFindByAddress(Authentication_Event_Data->BD_ADDR);
							if (connection) {
								modBLEConnection DeviceInfo = connection;
								c_memcpy(&(DeviceInfo->LTK), &(Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.LTK), sizeof(DeviceInfo->LTK));
								DeviceInfo->EDIV              = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.EDIV;
								c_memcpy(&(DeviceInfo->Rand), &(Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Rand), sizeof(DeviceInfo->Rand));
								DeviceInfo->EncryptionKeySize = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Encryption_Key_Size;
								DeviceInfo->Flags            |= DEVICE_INFO_FLAGS_LTK_VALID;
							}
							break;
						case QAPI_BLE_LAT_SECURITY_ESTABLISHMENT_COMPLETE_E:
							/* If this failed due to a LTK issue then we should delete the LTK */
							if (Authentication_Event_Data->Authentication_Event_Data.Security_Establishment_Complete.Status == QAPI_BLE_GAP_LE_SECURITY_ESTABLISHMENT_STATUS_CODE_LONG_TERM_KEY_ERROR) {
								connection = modBLEConnectionFindByAddress(Authentication_Event_Data->BD_ADDR);
								if (connection) {
									modBLEConnection DeviceInfo = connection;
									/* Clear the flag indicating the LTK is valid */
									DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_LTK_VALID;
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
							connection = modBLEConnectionFindByAddress(Authentication_Event_Data->BD_ADDR);
							if (connection) {
								modBLEConnection DeviceInfo = connection;
								c_memcpy(&(DeviceInfo->IRK), &(Authentication_Event_Data->Authentication_Event_Data.Identity_Information.IRK), sizeof(DeviceInfo->IRK));
								DeviceInfo->IdentityAddressBD_ADDR = Authentication_Event_Data->Authentication_Event_Data.Identity_Information.Address;
								DeviceInfo->IdentityAddressType    = Authentication_Event_Data->Authentication_Event_Data.Identity_Information.Address_Type;
								DeviceInfo->Flags                 |= DEVICE_INFO_FLAGS_IRK_VALID;

								/* Setup the resolving list entry to add to the */
								/* resolving list.                              */
								DeviceInfo->ResolvingListEntry.Peer_Identity_Address      = DeviceInfo->IdentityAddressBD_ADDR;
								DeviceInfo->ResolvingListEntry.Peer_Identity_Address_Type = DeviceInfo->IdentityAddressType;
								DeviceInfo->ResolvingListEntry.Peer_IRK                   = DeviceInfo->IRK;
								DeviceInfo->ResolvingListEntry.Local_IRK                  = gBLE->IRK;
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
					qapi_BLE_GAP_LE_Advertising_Report_Data_t Advertising_Data = GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Advertising_Data[i];
					if (Advertising_Data.Raw_Report_Length) {
						uint8_t *value = c_malloc(Advertising_Data.Raw_Report_Length);
						c_memmove(value, Advertising_Data.Raw_Report_Data, Advertising_Data.Raw_Report_Length);
						Advertising_Data.Raw_Report_Data = value;
						modMessagePostToMachine(gBLE->the, (uint8_t*)&Advertising_Data, sizeof(Advertising_Data), deviceDiscoveryEvent, NULL);
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
					qapi_BLE_GATT_Characteristic_Discovery_Response_Data_t GATT_Characteristic_Discovery_Response_Data = *GATT_Client_Event_Data->Event_Data.GATT_Characteristic_Discovery_Response_Data;
					uint8_t *characteristics = c_malloc(GATT_Characteristic_Discovery_Response_Data.NumberOfCharacteristics * sizeof(qapi_BLE_GATT_Characteristic_Entry_t));
					c_memmove(characteristics, GATT_Characteristic_Discovery_Response_Data.CharacteristicEntryList, GATT_Characteristic_Discovery_Response_Data.NumberOfCharacteristics * sizeof(qapi_BLE_GATT_Characteristic_Entry_t));
					GATT_Characteristic_Discovery_Response_Data.CharacteristicEntryList = (qapi_BLE_GATT_Characteristic_Entry_t*)characteristics;
					modMessagePostToMachine(gBLE->the, (uint8_t*)&GATT_Characteristic_Discovery_Response_Data, sizeof(GATT_Characteristic_Discovery_Response_Data), characteristicDiscoveryEvent, NULL);					
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
			default:
				break;
		}
	}
}

void QAPI_BLE_BTPSAPI GATT_Service_Discovery_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Service_Discovery_Event_Data_t *GATT_Service_Discovery_Event_Data, uint32_t CallbackParameter)
{
	if (BluetoothStackID && GATT_Service_Discovery_Event_Data) {
		switch(GATT_Service_Discovery_Event_Data->Event_Data_Type) {
            case QAPI_BLE_ET_GATT_SERVICE_DISCOVERY_INDICATION_E: {
				qapi_BLE_GATT_Service_Discovery_Indication_Data_t GATT_Service_Discovery_Indication_Data = *GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data;
				modMessagePostToMachine(gBLE->the, (uint8_t*)&GATT_Service_Discovery_Indication_Data, sizeof(GATT_Service_Discovery_Indication_Data), serviceDiscoveryEvent, NULL);
				break;
			}
            case QAPI_BLE_ET_GATT_SERVICE_DISCOVERY_COMPLETE_E: {
				qapi_BLE_GATT_Service_Discovery_Complete_Data_t GATT_Service_Discovery_Complete_Data = *GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Complete_Data;
				modMessagePostToMachine(gBLE->the, (uint8_t*)&GATT_Service_Discovery_Complete_Data, sizeof(GATT_Service_Discovery_Complete_Data), serviceDiscoveryCompleteEvent, NULL);
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
            case QAPI_BLE_ET_GATT_SERVICE_DISCOVERY_INDICATION_E: {
				qapi_BLE_GATT_Service_Discovery_Indication_Data_t *GATT_Service_Discovery_Indication_Data = GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data;
				uint32_t i, NumberOfCharacteristics = GATT_Service_Discovery_Indication_Data->NumberOfCharacteristics;
				qapi_BLE_GATT_Characteristic_Information_t *CharacteristicInformationList = GATT_Service_Discovery_Indication_Data->CharacteristicInformationList;					
				for (i = 0; i < NumberOfCharacteristics; ++i) {
					if (request->handle == CharacteristicInformationList[i].Characteristic_Handle) {
						qapi_BLE_GATT_Characteristic_Descriptor_Information_t *descriptorList = c_malloc(CharacteristicInformationList[i].NumberOfDescriptors * sizeof(qapi_BLE_GATT_Characteristic_Descriptor_Information_t));
						if (descriptorList) {
							qapi_BLE_GATT_Characteristic_Information_t characteristicInfo = CharacteristicInformationList[i];
							c_memmove(descriptorList, CharacteristicInformationList[i].DescriptorList, CharacteristicInformationList[i].NumberOfDescriptors * sizeof(qapi_BLE_GATT_Characteristic_Descriptor_Information_t));
							characteristicInfo.DescriptorList = descriptorList;
							modMessagePostToMachine(gBLE->the, (uint8_t*)&characteristicInfo, sizeof(characteristicInfo), descriptorDiscoveryEvent, (void*)request);
						}
						break;
					}
				}
				break;
			}
            case QAPI_BLE_ET_GATT_SERVICE_DISCOVERY_COMPLETE_E: {
				qapi_BLE_GATT_Service_Discovery_Complete_Data_t GATT_Service_Discovery_Complete_Data = *GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Complete_Data;
				modMessagePostToMachine(gBLE->the, (uint8_t*)&GATT_Service_Discovery_Complete_Data, sizeof(GATT_Service_Discovery_Complete_Data), descriptorDiscoveryCompleteEvent, NULL);
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



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
#include "modBLESM.h"

#define QAPI_USE_BLE
#include "qapi.h"

#pragma GCC diagnostic ignored "-Wswitch"	// disable warning on missing switch case values

#define DEVICE_FRIENDLY_NAME "Moddable"

#define LOG_GATTS 0
#if LOG_GATTS
	static void logGATTSEvent(qapi_BLE_GATT_Server_Event_Type_t event);
	#define LOG_GATTS_EVENT(event) logGATTSEvent(event)
	#define LOG_GATTS_MSG(msg) modLog(msg)
	#define LOG_GATTS_INT(i) modLogInt(i)
#else
	#define LOG_GATTS_EVENT(event)
	#define LOG_GATTS_MSG(msg)
	#define LOG_GATTS_INT(i)
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

#define LOG_UNHANDLED_CALLBACK_EVENTS 0

#define UUID16EqualBytes(_uuid, _b1, _b0) (_b1 == _uuid.UUID_Byte1 && _b0 == _uuid.UUID_Byte0)
#define QAPI_BLE_COMPARE_BD_ADDR(_x, _y) (((_x).BD_ADDR0 == (_y).BD_ADDR0) && ((_x).BD_ADDR1 == (_y).BD_ADDR1) && ((_x).BD_ADDR2 == (_y).BD_ADDR2) && ((_x).BD_ADDR3 == (_y).BD_ADDR3) && ((_x).BD_ADDR4 == (_y).BD_ADDR4) && ((_x).BD_ADDR5 == (_y).BD_ADDR5))

#include "mc.bleservices.c"

typedef struct {
	uint16_t start;
	uint16_t end;
	uint16_t service_index;
	uint32_t service_id;
} serviceHandleTableRecord;

typedef struct modBLEPeerConnectionRecord modBLEPeerConnectionRecord;
typedef modBLEPeerConnectionRecord *modBLEPeerConnection;

struct modBLEPeerConnectionRecord {
	struct modBLEPeerConnectionRecord *next;

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
};

typedef struct {
	xsMachine *the;
	xsSlot obj;

	uint32_t stackID;
	uint32_t gapsID;
	uint8_t advertising;
	qapi_BLE_BD_ADDR_t localAddress;	
	
	// services
	uint8_t deployServices;
	serviceHandleTableRecord serviceHandles[service_count];
	
	// security
	uint8_t encryption;
	uint8_t bonding;
	uint8_t mitm;
	uint8_t iocap;
	qapi_BLE_Encryption_Key_t ER;	// encryption root key
	qapi_BLE_Encryption_Key_t IR;	// identity root key
	qapi_BLE_Encryption_Key_t DHK;
	qapi_BLE_Encryption_Key_t IRK;
   	qapi_BLE_GAP_LE_Extended_Pairing_Capabilities_t pairingCapabilities;
	
	// connection
	modBLEPeerConnectionRecord connection;
} modBLERecord, *modBLE;

static modBLE gBLE = NULL;

#if MODDEF_BLE_MAX_CONNECTIONS != 1
	#error - only one ble client connection supported
#endif

static void readyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void bondingRemovedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void addressToBuffer(qapi_BLE_BD_ADDR_t BD_ADDR, uint8_t *buffer);
static void QAPI_BLE_BTPSAPI GAP_LE_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GAP_LE_Event_Data_t *GAP_LE_Event_Data, uint32_t CallbackParameter);
static void QAPI_BLE_BTPSAPI GATT_Connection_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, uint32_t CallbackParameter);
static void QAPI_BLE_BTPSAPI Server_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Server_Event_Data_t *GATT_Server_Event_Data, uint32_t CallbackParameter);

uint32_t gBluetoothStackID = 0;

void xs_ble_server_initialize(xsMachine *the)
{
	qapi_BLE_HCI_DriverInformation_t HCI_DriverInformation;
	int result;
	uint32_t serviceID, appearance = 0;
	char *device_name = NULL;
	uint8_t i, j, found = 0;
	
	if (NULL != gBLE)
		xsUnknownError("BLE already initialized");
	gBLE = (modBLE)c_calloc(sizeof(modBLERecord), 1);
	if (!gBLE)
		xsUnknownError("no memory");
	gBLE->the = the;
	gBLE->obj = xsThis;
	gBLE->connection.id = -1;
	gBLE->connection.bond = 0xFF;

	xsRemember(gBLE->obj);
	
	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_deployServices)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_deployServices);
		gBLE->deployServices = xsmcToBoolean(xsVar(0));
	}
	else
		gBLE->deployServices = true;

	// Initialize platform Bluetooth modules
	QAPI_BLE_HCI_DRIVER_SET_COMM_INFORMATION(&HCI_DriverInformation, 1, 115200, QAPI_BLE_COMM_PROTOCOL_UART_E);
	result = qapi_BLE_BSC_Initialize(&HCI_DriverInformation, 0);
	if (result <= 0)
		xsUnknownError("BLE initialization failed");
	gBluetoothStackID = gBLE->stackID = result;
	
	result = qapi_BLE_GATT_Initialize(gBLE->stackID, QAPI_BLE_GATT_INITIALIZATION_FLAGS_SUPPORT_LE | QAPI_BLE_GATT_INITIALIZATION_FLAGS_DISABLE_SERVICE_CHANGED_CHARACTERISTIC, GATT_Connection_Event_Callback, 0);
	if (result != 0)
		xsUnknownError("BLE GATT initialization failed");

	// Initialize GAPS service and use device name and appearance from app GAP service when available
	for (i = 0; !found && (i < service_count); ++i) {
		if (QAPI_BLE_AET_PRIMARY_SERVICE_16_E == ServiceTable[i][0].Attribute_Entry_Type) {
			qapi_BLE_GATT_Primary_Service_16_Entry_t *service = (qapi_BLE_GATT_Primary_Service_16_Entry_t*)ServiceTable[i][0].Attribute_Value;
			if (UUID16EqualBytes(service->Service_UUID, 0x18, 0x00)) {
				found = 1;
				service->Service_UUID.UUID_Byte0 = 0; service->Service_UUID.UUID_Byte1 = 0;	// don't register app GAP service
				for (j = 1; j < attribute_counts[i]; ++j) {
					if (QAPI_BLE_AET_CHARACTERISTIC_VALUE_16_E == ServiceTable[i][j].Attribute_Entry_Type) {
						qapi_BLE_GATT_Characteristic_Value_16_Entry_t *characteristic = (qapi_BLE_GATT_Characteristic_Value_16_Entry_t*)ServiceTable[i][j].Attribute_Value;
						if (UUID16EqualBytes(characteristic->Characteristic_Value_UUID, 0x2A, 0x00)) {
							device_name = c_calloc(1, characteristic->Characteristic_Value_Length + 1);
							c_memmove(device_name, characteristic->Characteristic_Value, characteristic->Characteristic_Value_Length);
						}	
						if (UUID16EqualBytes(characteristic->Characteristic_Value_UUID, 0x2A, 0x01)) {
							appearance = characteristic->Characteristic_Value[1] << 8 | characteristic->Characteristic_Value[0];
						}
					}
				}
				break;
			}
		}
	}

	result = qapi_BLE_GAPS_Initialize_Service(gBLE->stackID, &serviceID);
	if (result <= 0)
		xsUnknownError("BLE GAPS initialization failed");
	gBLE->gapsID = result;
	qapi_BLE_GAPS_Set_Device_Name(gBLE->stackID, gBLE->gapsID, device_name ? device_name : DEVICE_FRIENDLY_NAME);
	qapi_BLE_GAPS_Set_Device_Appearance(gBLE->stackID, gBLE->gapsID, appearance ? appearance : QAPI_BLE_GAP_DEVICE_APPEARANCE_VALUE_GENERIC_COMPUTER);
		
	if (device_name)
		c_free(device_name);
		
	qapi_BLE_GAP_Query_Local_BD_ADDR(gBLE->stackID, &gBLE->localAddress);
            
	modMessagePostToMachine(the, NULL, 0, readyEvent, NULL);
}

void xs_ble_server_disconnect(xsMachine *the)
{
	if (-1 != gBLE->connection.id)
		qapi_BLE_GAP_LE_Disconnect(gBLE->stackID, gBLE->connection.bd_addr);
}

void xs_ble_server_close(xsMachine *the)
{
	modBLE ble = gBLE;
	if (!ble) return;

	gBLE = NULL;
	xsForget(ble->obj);
	xs_ble_server_destructor(ble);
}

void xs_ble_server_destructor(void *data)
{
	modBLE ble = data;
	if (!ble) return;
	
	qapi_BLE_GAPS_Cleanup_Service(ble->stackID, ble->gapsID);	
	
	if (ble->deployServices) {
		for (int16_t i = 0; i < service_count; ++i) {
			if (0 != ble->serviceHandles[i].service_id)
				qapi_BLE_GATT_Un_Register_Service(ble->stackID, ble->serviceHandles[i].service_id);
		}
	}
	
	modBLEWhitelistClear();
	qapi_BLE_GATT_Cleanup(ble->stackID);
	qapi_BLE_BSC_Shutdown(ble->stackID);
	c_free(ble);

	gBluetoothStackID = 0;
	gBLE = NULL;
}

void xs_ble_server_get_local_address(xsMachine *the)
{
	qapi_BLE_BD_ADDR_t bd_addr;
	uint8_t buffer[6];
	qapi_BLE_GAP_Query_Local_BD_ADDR(gBLE->stackID, &bd_addr);
	addressToBuffer(bd_addr, buffer);
	xsmcSetArrayBuffer(xsResult, (void*)buffer, 6);
}

void xs_ble_server_set_device_name(xsMachine *the)
{
	qapi_BLE_GAPS_Set_Device_Name(gBLE->stackID, gBLE->gapsID, xsmcToString(xsArg(0)));
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
	qapi_BLE_GAP_LE_Advertising_Parameters_t AdvertisingParameters;
	qapi_BLE_GAP_LE_Connectability_Parameters_t ConnectabilityParameters;
	
	if (advertisingData) {
		qapi_BLE_Advertising_Data_t data;
		c_memset(&data, 0, sizeof(data));
		c_memmove(&data, advertisingData, advertisingDataLength);
		qapi_BLE_GAP_LE_Set_Advertising_Data(gBLE->stackID, advertisingDataLength, &data);
	}
          
	if (scanResponseData) {
		qapi_BLE_Scan_Response_Data_t data;
		c_memset(&data, 0, sizeof(data));
		c_memmove(&data, scanResponseData, scanResponseDataLength);
		qapi_BLE_GAP_LE_Set_Scan_Response_Data(gBLE->stackID, scanResponseDataLength, &data);
	}

	AdvertisingParameters.Advertising_Channel_Map   = QAPI_BLE_HCI_LE_ADVERTISING_CHANNEL_MAP_DEFAULT;
	
	if (kBLEAdvFilterPolicyWhitelistScansConnections == filterPolicy) {
		AdvertisingParameters.Scan_Request_Filter       = QAPI_BLE_FP_WHITE_LIST_E;
		AdvertisingParameters.Connect_Request_Filter    = QAPI_BLE_FP_WHITE_LIST_E;
	}
	else if (kBLEAdvFilterPolicyWhitelistConnections == filterPolicy) {
		AdvertisingParameters.Scan_Request_Filter       = QAPI_BLE_FP_NO_FILTER_E;
		AdvertisingParameters.Connect_Request_Filter    = QAPI_BLE_FP_WHITE_LIST_E;
	}
	else if (kBLEAdvFilterPolicyWhitelistScans == filterPolicy) {
		AdvertisingParameters.Scan_Request_Filter       = QAPI_BLE_FP_WHITE_LIST_E;
		AdvertisingParameters.Connect_Request_Filter    = QAPI_BLE_FP_NO_FILTER_E;
	}
	else {
		AdvertisingParameters.Scan_Request_Filter       = QAPI_BLE_FP_NO_FILTER_E;
		AdvertisingParameters.Connect_Request_Filter    = QAPI_BLE_FP_NO_FILTER_E;
	}
	
	AdvertisingParameters.Advertising_Interval_Min  = (uint32_t)(intervalMin / 0.625);	// convert to 1 ms units;
	AdvertisingParameters.Advertising_Interval_Max  = (uint32_t)(intervalMax / 0.625);
	
	ConnectabilityParameters.Connectability_Mode   = (flags & (LE_LIMITED_DISCOVERABLE_MODE | LE_GENERAL_DISCOVERABLE_MODE)) ? QAPI_BLE_LCM_CONNECTABLE_E : QAPI_BLE_LCM_NON_CONNECTABLE_E;
	ConnectabilityParameters.Own_Address_Type      = QAPI_BLE_LAT_PUBLIC_E;
	ConnectabilityParameters.Direct_Address_Type   = QAPI_BLE_LAT_PUBLIC_E;
	QAPI_BLE_ASSIGN_BD_ADDR(ConnectabilityParameters.Direct_Address, 0, 0, 0, 0, 0, 0);  
	               
	if (0 == qapi_BLE_GAP_LE_Advertising_Enable(gBLE->stackID, true, &AdvertisingParameters, &ConnectabilityParameters, GAP_LE_Event_Callback, 0L))
		gBLE->advertising = 1;
}
	
void xs_ble_server_stop_advertising(xsMachine *the)
{
	if (gBLE->advertising) {
		gBLE->advertising = 0;
		qapi_BLE_GAP_LE_Advertising_Disable(gBLE->stackID);
	}
}

void xs_ble_server_characteristic_notify_value(xsMachine *the)
{
	uint16_t handle = xsmcToInteger(xsArg(0));
	uint16_t notify = xsmcToInteger(xsArg(1));
	uint8_t *value = xsmcToArrayBuffer(xsArg(2));
	uint32_t length = xsmcGetArrayBufferLength(xsArg(2));
	
 	for (int16_t i = 0; i < service_count; ++i) {
 		if (handle >= gBLE->serviceHandles[i].start && handle <= gBLE->serviceHandles[i].end) {
  			if (notify)
  				qapi_BLE_GATT_Handle_Value_Notification(gBLE->stackID, gBLE->serviceHandles[i].service_id, gBLE->connection.id, handle - gBLE->serviceHandles[i].start, length, value);
  			else
  				qapi_BLE_GATT_Handle_Value_Indication(gBLE->stackID, gBLE->serviceHandles[i].service_id, gBLE->connection.id, handle - gBLE->serviceHandles[i].start, length, value);
  			break;
  		}
 	}
}

void xs_ble_server_deploy(xsMachine *the)
{
	if (!gBLE->deployServices) return;
	
	for (int16_t i = 0; i < service_count; ++i) {
		int result;
		qapi_BLE_GATT_Attribute_Handle_Group_t ServiceHandleGroupResult;
		qapi_BLE_GATT_Primary_Service_16_Entry_t *service;
		
		// If the service UUID is 0x0000 then don't register. This was the GAP service that has already been deployed.
		service = (qapi_BLE_GATT_Primary_Service_16_Entry_t*)ServiceTable[i][0].Attribute_Value;
		if (UUID16EqualBytes(service->Service_UUID, 0x00, 0x00))
			continue;
			
		result = qapi_BLE_GATT_Register_Service(gBLE->stackID, QAPI_BLE_GATT_SERVICE_FLAGS_LE_SERVICE, attribute_counts[i], &ServiceTable[i][0], &ServiceHandleGroupResult, Server_Event_Callback, 0L);
		if (result <= 0)
			xsUnknownError("service deploy failed");
		gBLE->serviceHandles[i].service_index = i;
		gBLE->serviceHandles[i].service_id = result;
		gBLE->serviceHandles[i].start = ServiceHandleGroupResult.Starting_Handle;
		gBLE->serviceHandles[i].end = ServiceHandleGroupResult.Ending_Handle;
	}
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
	
	qapi_BLE_GAP_LE_Set_Pairability_Mode(gBLE->stackID,
		encryption ? QAPI_BLE_LPM_PAIRABLE_MODE_ENABLE_EXTENDED_EVENTS_E : QAPI_BLE_LPM_PAIRABLE_MODE_E);
		
	configurePairingCapabilities(encryption, bonding, mitm, iocap, &gBLE->pairingCapabilities);
	generateEncryptionKeys(gBLE->stackID, &gBLE->ER, &gBLE->IR, &gBLE->DHK, &gBLE->IRK);
		
	qapi_BLE_GAP_LE_Register_Remote_Authentication(gBLE->stackID, GAP_LE_Event_Callback, 0L);
}

void xs_ble_server_passkey_input(xsMachine *the)
{
	qapi_BLE_GAP_LE_Authentication_Response_Information_t GAP_LE_Authentication_Response_Information;
	//uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint32_t passkey = xsmcToInteger(xsArg(1));
	GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = QAPI_BLE_LAR_PASSKEY_E;
	GAP_LE_Authentication_Response_Information.Authentication_Data_Length = sizeof(uint32_t);
	GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = passkey;											
	qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, gBLE->connection.bd_addr, &GAP_LE_Authentication_Response_Information);									
}

void xs_ble_server_passkey_reply(xsMachine *the)
{
	qapi_BLE_GAP_LE_Authentication_Response_Information_t GAP_LE_Authentication_Response_Information;
	//uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t confirm = xsmcToBoolean(xsArg(1));
	GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = (confirm ? QAPI_BLE_LAR_CONFIRMATION_E : QAPI_BLE_LAR_ERROR_E);
	GAP_LE_Authentication_Response_Information.Authentication_Data_Length = sizeof(uint32_t);
	GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = gBLE->connection.passkey;											
	qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, gBLE->connection.bd_addr, &GAP_LE_Authentication_Response_Information);									
}

void xs_ble_server_get_service_attributes(xsMachine *the)
{
	// @@ TBD
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

static const char_name_table *attributeToCharName(uint32_t service_id, uint16_t att_index)
{
	for (uint16_t i = 0; i < service_count; ++i) {
		if (service_id == gBLE->serviceHandles[i].service_id) {
			for (uint16_t k = 0; k < char_name_count; ++k) {
				if (char_names[k].service_index == i && char_names[k].att_index == att_index)
					return &char_names[k];
			}
		}
	}
	return NULL;
}

static qapi_BLE_GATT_Service_Attribute_Entry_t *attributeToAttributeEntry(uint32_t service_id, uint16_t att_index)
{
	for (uint16_t i = 0; i < service_count; ++i) {
		if (service_id == gBLE->serviceHandles[i].service_id)
			return &ServiceTable[gBLE->serviceHandles[i].service_index][att_index];
	}
	return NULL;
}

static int16_t attributeToHandle(uint32_t service_id, uint16_t att_index)
{
	for (uint16_t i = 0; i < service_count; ++i) {
		if (service_id == gBLE->serviceHandles[i].service_id)
			return att_index + gBLE->serviceHandles[i].start;
	}
	return -1;
}

void modBLEServerBondingRemoved(qapi_BLE_BD_ADDR_t *address, qapi_BLE_GAP_LE_Address_Type_t addressType)
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
	if (!gBLE) return;
	
	xsBeginHost(gBLE->the);
	xsCall1(gBLE->obj, xsID_callback, xsString("onReady"));
	xsEndHost(gBLE->the);
}

static void connectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Device_Connection_Data_t *result = (qapi_BLE_GATT_Device_Connection_Data_t*)message;
	uint8_t buffer[6];
	
	if (!gBLE) return;
	
	// ignore multiple connections on same connection
	if (-1 != gBLE->connection.id)
		return;	
		
 	xsBeginHost(gBLE->the);
	gBLE->connection.id = result->ConnectionID;
	gBLE->connection.bd_addr = result->RemoteDevice;
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), gBLE->connection.id);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	addressToBuffer(result->RemoteDevice, buffer);
	xsmcSetArrayBuffer(xsVar(1), buffer, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), gBLE->connection.bd_addr_type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onConnected"), xsVar(0));
	xsEndHost(gBLE->the);

	if (gBLE->encryption || gBLE->mitm)
		qapi_BLE_GAP_LE_Extended_Request_Security(gBLE->stackID, gBLE->connection.bd_addr, &gBLE->pairingCapabilities, GAP_LE_Event_Callback, 0);
}

static void disconnectEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Device_Disconnection_Data_t *result = (qapi_BLE_GATT_Device_Disconnection_Data_t*)message;
	uint8_t buffer[6];
		
	if (!gBLE) return;
	
	// ignore multiple disconnects on same connection
	if (result->ConnectionID != gBLE->connection.id)
		return;
			
	gBLE->connection.id = -1;
	c_memset(&gBLE->connection.bd_addr, 0, sizeof(gBLE->connection.bd_addr));

	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	xsmcSetInteger(xsVar(1), result->ConnectionID);
	xsmcSet(xsVar(0), xsID_connection, xsVar(1));
	addressToBuffer(result->RemoteDevice, buffer);
	xsmcSetArrayBuffer(xsVar(1), buffer, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSetInteger(xsVar(1), gBLE->connection.bd_addr_type);
	xsmcSet(xsVar(0), xsID_addressType, xsVar(1));
	xsCall2(gBLE->obj, xsID_callback, xsString("onDisconnected"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void readRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Read_Request_Data_t *request = (qapi_BLE_GATT_Read_Request_Data_t*)message;
	qapi_BLE_GATT_Service_Attribute_Entry_t *attributeEntry, *characteristicDeclarationEntry;
	uint8_t *buffer;
	
	if (!gBLE) return;
	
	if (request->ConnectionID != gBLE->connection.id) {
		return; 	
	}
		
	char_name_table *char_name = (char_name_table *)attributeToCharName(request->ServiceID, request->AttributeOffset);
	int16_t handle = attributeToHandle(request->ServiceID, request->AttributeOffset);

	attributeEntry = attributeToAttributeEntry(request->ServiceID, request->AttributeOffset);
	if (!attributeEntry)
		return;
		
	if (QAPI_BLE_AET_CHARACTERISTIC_VALUE_16_E == attributeEntry->Attribute_Entry_Type || QAPI_BLE_AET_CHARACTERISTIC_VALUE_128_E == attributeEntry->Attribute_Entry_Type) {
		characteristicDeclarationEntry = attributeToAttributeEntry(request->ServiceID, request->AttributeOffset - 1);
		if (!characteristicDeclarationEntry)
			return;
	}
	
	// Ensure that link has been encrypted when reading/writing encrypted properties
	if (char_name && char_name->encrypted) {
		qapi_BLE_GAP_Encryption_Mode_t GAPEncryptionMode;
		if ((qapi_BLE_GAP_LE_Query_Encryption_Mode(gBLE->stackID, request->RemoteDevice, &GAPEncryptionMode) == 0) && (GAPEncryptionMode == QAPI_BLE_EM_DISABLED_E)) {
			qapi_BLE_GATT_Error_Response(gBLE->stackID, request->TransactionID, request->AttributeOffset, QAPI_BLE_ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION);
			return;
		}
	}
	
	// Auto-respond if the attribute has a predefined value or if it is a notify-only attribute
	switch(attributeEntry->Attribute_Entry_Type) {
		case QAPI_BLE_AET_CHARACTERISTIC_VALUE_16_E:
		case QAPI_BLE_AET_CHARACTERISTIC_DESCRIPTOR_16_E: {
			qapi_BLE_GATT_Characteristic_Value_16_Entry_t *valueEntry = (qapi_BLE_GATT_Characteristic_Value_16_Entry_t*)attributeEntry->Attribute_Value;
			if (valueEntry && valueEntry->Characteristic_Value_Length) {
				if (NULL != valueEntry->Characteristic_Value) {
					qapi_BLE_GATT_Read_Response(gBLE->stackID, request->TransactionID, valueEntry->Characteristic_Value_Length, valueEntry->Characteristic_Value);
					return;
				}
				else {
					qapi_BLE_GATT_Characteristic_Declaration_16_Entry_t *declEntry = (qapi_BLE_GATT_Characteristic_Declaration_16_Entry_t*)characteristicDeclarationEntry->Attribute_Value;
					if (QAPI_BLE_GATT_CHARACTERISTIC_PROPERTIES_NOTIFY == declEntry->Properties) {
						buffer = c_calloc(1, valueEntry->Characteristic_Value_Length);
						qapi_BLE_GATT_Read_Response(gBLE->stackID, request->TransactionID, valueEntry->Characteristic_Value_Length, buffer);
						c_free(buffer);
						return;
					}
				}
			}
			break;
		}
		case QAPI_BLE_AET_CHARACTERISTIC_VALUE_128_E: 
		case QAPI_BLE_AET_CHARACTERISTIC_DESCRIPTOR_128_E:{
			qapi_BLE_GATT_Characteristic_Value_128_Entry_t *valueEntry = (qapi_BLE_GATT_Characteristic_Value_128_Entry_t*)attributeEntry->Attribute_Value;
			if (valueEntry && valueEntry->Characteristic_Value_Length) {
				if (NULL != valueEntry->Characteristic_Value) {
					qapi_BLE_GATT_Read_Response(gBLE->stackID, request->TransactionID, valueEntry->Characteristic_Value_Length, valueEntry->Characteristic_Value);
					return;
				}
				else {
					qapi_BLE_GATT_Characteristic_Declaration_128_Entry_t *declEntry = (qapi_BLE_GATT_Characteristic_Declaration_128_Entry_t*)characteristicDeclarationEntry->Attribute_Value;
					if (QAPI_BLE_GATT_CHARACTERISTIC_PROPERTIES_NOTIFY == declEntry->Properties) {
						buffer = c_calloc(1, valueEntry->Characteristic_Value_Length);
						qapi_BLE_GATT_Read_Response(gBLE->stackID, request->TransactionID, valueEntry->Characteristic_Value_Length, buffer);
						c_free(buffer);
						return;
					}
				}
			}
			break;
		}
		default:
			break;
	}

	xsBeginHost(gBLE->the);
	xsmcVars(4);
	xsVar(0) = xsmcNewObject();
	
	if (QAPI_BLE_AET_CHARACTERISTIC_VALUE_16_E == attributeEntry->Attribute_Entry_Type) {
		xsmcSetArrayBuffer(xsVar(1), (uint8_t*)&((qapi_BLE_GATT_Characteristic_Value_16_Entry_t*)attributeEntry)->Characteristic_Value_UUID, 2);
		xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	}
	else if (QAPI_BLE_AET_CHARACTERISTIC_VALUE_128_E == attributeEntry->Attribute_Entry_Type) {
		xsmcSetArrayBuffer(xsVar(1), (uint8_t*)&((qapi_BLE_GATT_Characteristic_Value_16_Entry_t*)attributeEntry)->Characteristic_Value_UUID, 16);
		xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
	}

	xsmcSetInteger(xsVar(2), handle);
	xsmcSet(xsVar(0), xsID_handle, xsVar(2));
	if (char_name) {
		xsmcSetString(xsVar(2), (char*)char_name->name);
		xsmcSet(xsVar(0), xsID_name, xsVar(2));
		xsmcSetString(xsVar(3), (char*)char_name->type);
		xsmcSet(xsVar(0), xsID_type, xsVar(3));
	}
	
	xsResult = xsCall2(gBLE->obj, xsID_callback, xsString("onCharacteristicRead"), xsVar(0));
	qapi_BLE_GATT_Read_Response(gBLE->stackID, request->TransactionID, xsmcGetArrayBufferLength(xsResult), xsmcToArrayBuffer(xsResult));
	
	xsEndHost(gBLE->the);
}

static void writeRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Write_Request_Data_t *request = (qapi_BLE_GATT_Write_Request_Data_t*)message;
	qapi_BLE_GATT_Service_Attribute_Entry_t *attributeEntry;
	uint8_t notify = 0xFF;
			
 	if (!gBLE) return;
	
	xsBeginHost(gBLE->the);	
 	
 	if (request->ConnectionID != gBLE->connection.id)
		goto bail; 	
		
	const char_name_table *char_name = (char_name_table *)attributeToCharName(request->ServiceID, request->AttributeOffset);
	int16_t handle = attributeToHandle(request->ServiceID, request->AttributeOffset);
	
	attributeEntry = attributeToAttributeEntry(request->ServiceID, request->AttributeOffset);
	if (!attributeEntry)
		goto bail;
		
	// Ensure that link has been encrypted when reading/writing encrypted properties
	if (char_name && char_name->encrypted) {
		qapi_BLE_GAP_Encryption_Mode_t GAPEncryptionMode;
		if ((qapi_BLE_GAP_LE_Query_Encryption_Mode(gBLE->stackID, request->RemoteDevice, &GAPEncryptionMode) == 0) && (GAPEncryptionMode == QAPI_BLE_EM_DISABLED_E)) {
			qapi_BLE_GATT_Error_Response(gBLE->stackID, request->TransactionID, request->AttributeOffset, QAPI_BLE_ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION);
			goto bail;
		}
	}

	if (2 == request->AttributeValueLength && QAPI_BLE_AET_CHARACTERISTIC_DESCRIPTOR_16_E == attributeEntry->Attribute_Entry_Type) {
		qapi_BLE_GATT_Characteristic_Descriptor_16_Entry_t *valueEntry = (qapi_BLE_GATT_Characteristic_Descriptor_16_Entry_t*)attributeEntry->Attribute_Value;
		if (UUID16EqualBytes(valueEntry->Characteristic_Descriptor_UUID, 0x29, 0x02)) {
			uint16_t descr_value = request->AttributeValue[1]<<8 | request->AttributeValue[0];
			if (descr_value < 0x0003) {
				char_name = attributeToCharName(request->ServiceID, request->AttributeOffset - 1);
				notify = (uint8_t)descr_value;
			}
		}
	}
	
	xsmcVars(6);
	xsVar(0) = xsmcNewObject();
	
	switch (attributeEntry->Attribute_Entry_Type) {
		case QAPI_BLE_AET_CHARACTERISTIC_VALUE_16_E:
		case QAPI_BLE_AET_CHARACTERISTIC_DESCRIPTOR_16_E:
			xsmcSetArrayBuffer(xsVar(1), (uint8_t*)&((qapi_BLE_GATT_Characteristic_Value_16_Entry_t*)attributeEntry)->Characteristic_Value_UUID, 2);
			xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
			break;
		case QAPI_BLE_AET_CHARACTERISTIC_VALUE_128_E:
		case QAPI_BLE_AET_CHARACTERISTIC_DESCRIPTOR_128_E:
			xsmcSetArrayBuffer(xsVar(1), (uint8_t*)&((qapi_BLE_GATT_Characteristic_Value_128_Entry_t*)attributeEntry)->Characteristic_Value_UUID, 16);
			xsmcSet(xsVar(0), xsID_uuid, xsVar(1));
			break;
	}
	
	// let client know we received the write request
	qapi_BLE_GATT_Write_Response(gBLE->stackID, request->TransactionID);
	
	if (char_name) {
		xsmcSetString(xsVar(2), (char*)char_name->name);
		xsmcSet(xsVar(0), xsID_name, xsVar(2));
		xsmcSetString(xsVar(3), (char*)char_name->type);
		xsmcSet(xsVar(0), xsID_type, xsVar(3));
	}
	
	if (0xFF != notify) {
		xsmcSetInteger(xsVar(4), handle - 1);
		xsmcSetInteger(xsVar(5), notify);
		xsmcSet(xsVar(0), xsID_handle, xsVar(4));
		xsmcSet(xsVar(0), xsID_notify, xsVar(5));
		xsCall2(gBLE->obj, xsID_callback, xsString(0 == notify ? "onCharacteristicNotifyDisabled" : "onCharacteristicNotifyEnabled"), xsVar(0));
	}
	else {
		xsmcSetInteger(xsVar(2), handle);
		xsmcSet(xsVar(0), xsID_handle, xsVar(2));
		xsmcSetArrayBuffer(xsVar(3), request->AttributeValue, request->AttributeValueLength);
		xsmcSet(xsVar(0), xsID_value, xsVar(3));
		xsCall2(gBLE->obj, xsID_callback, xsString("onCharacteristicWritten"), xsVar(0));
	}
	
bail:
	c_free(request->AttributeValue);
	xsEndHost(gBLE->the);
}

static void gapPasskeyRequestEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GAP_LE_Authentication_Response_Information_t GAP_LE_Authentication_Response_Information;
	uint32_t passkey;
	uint8_t buffer[6];

	if (!gBLE) return;
	
	xsBeginHost(gBLE->the);
	xsmcVars(2);
	xsVar(0) = xsmcNewObject();
	addressToBuffer(gBLE->connection.bd_addr, buffer);
	xsmcSetArrayBuffer(xsVar(1), buffer, 6);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	if (gBLE->iocap == KeyboardOnly)
		xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyInput"), xsVar(0));
	else {
		xsResult = xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyRequested"), xsVar(0));
		passkey = xsmcToInteger(xsResult);
		GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type  = QAPI_BLE_LAR_PASSKEY_E;
		GAP_LE_Authentication_Response_Information.Authentication_Data_Length  = (uint8_t)(sizeof(uint32_t));
		GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = passkey;
		qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, gBLE->connection.bd_addr, &GAP_LE_Authentication_Response_Information);
	}
	xsEndHost(gBLE->the);
}

static void gapPasskeyNotifyEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	uint32_t passkey = *(uint32_t*)message;
	uint8_t buffer[6];

	if (!gBLE) return;
	
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	addressToBuffer(gBLE->connection.bd_addr, buffer);
	xsmcSetArrayBuffer(xsVar(1), buffer, 6);
	xsmcSetInteger(xsVar(2), passkey);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSet(xsVar(0), xsID_passkey, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyDisplay"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gapPasskeyConfirmEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	uint32_t passkey = *(uint32_t*)message;
	uint8_t buffer[6];

	if (!gBLE) return;
	
	xsBeginHost(gBLE->the);
	xsmcVars(3);
	xsVar(0) = xsmcNewObject();
	addressToBuffer(gBLE->connection.bd_addr, buffer);
	gBLE->connection.passkey = passkey;
	xsmcSetArrayBuffer(xsVar(1), buffer, 6);
	xsmcSetInteger(xsVar(2), passkey);
	xsmcSet(xsVar(0), xsID_address, xsVar(1));
	xsmcSet(xsVar(0), xsID_passkey, xsVar(2));
	xsCall2(gBLE->obj, xsID_callback, xsString("onPasskeyConfirm"), xsVar(0));
	xsEndHost(gBLE->the);
}

static void gapAuthCompleteEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GAP_LE_Pairing_Status_t *Pairing_Status = (qapi_BLE_GAP_LE_Pairing_Status_t *)message;
	modBLEBondedDevice device;
	uint8_t bonded = 0;

	if (!gBLE) return;
	
	if (QAPI_BLE_GAP_LE_PAIRING_STATUS_NO_ERROR == Pairing_Status->Status) {
		if (gBLE->bonding && ((gBLE->connection.Flags & (DEVICE_INFO_FLAGS_LTK_VALID | DEVICE_INFO_FLAGS_IRK_VALID)) == (DEVICE_INFO_FLAGS_LTK_VALID | DEVICE_INFO_FLAGS_IRK_VALID))) {
			device = modBLEBondedDevicesFindByAddress(gBLE->connection.bd_addr);
			if (NULL != device)
				modBLEBondedDevicesRemove(device);
			device = c_calloc(sizeof(modBLEBondedDeviceRecord), 1);
			if (NULL != device) {
				device->Flags = gBLE->connection.Flags;
				device->LastAddress = gBLE->connection.bd_addr;
				device->LastAddressType = gBLE->connection.bd_addr_type;
				device->IdentityAddress = gBLE->connection.IdentityAddressBD_ADDR;
				device->IdentityAddressType = gBLE->connection.IdentityAddressType;
				device->EncryptionKeySize = gBLE->connection.EncryptionKeySize;
				device->IRK = gBLE->connection.IRK;
				device->LTK = gBLE->connection.LTK;
				modBLEBondedDevicesAdd(device);
				bonded = 1;
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
		device = modBLEBondedDevicesFindByAddress(gBLE->connection.bd_addr);
		if (NULL != device)
			modBLEBondedDevicesRemove(device);
	}
}

static void mtuExchangedEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	qapi_BLE_GATT_Device_Connection_MTU_Update_Data_t *result = (qapi_BLE_GATT_Device_Connection_MTU_Update_Data_t*)message;

 	if (result->ConnectionID != gBLE->connection.id)
 		return;
	
	xsBeginHost(gBLE->the);
		xsmcVars(1);
		xsmcSetInteger(xsVar(0), result->MTU);
		xsCall2(gBLE->obj, xsID_callback, xsString("onMTUExchanged"), xsVar(0));
	xsEndHost(gBLE->the);
}

#if LOG_UNHANDLED_CALLBACK_EVENTS
typedef struct {
	int event;
	char name[64];
} UnhandledCallbackEventRecord;

static void unhandledCallbackEvent(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	UnhandledCallbackEventRecord *u = (UnhandledCallbackEventRecord*)message;
	xsTrace(u->name);
	modLogInt(u->event);
}
#endif

void QAPI_BLE_BTPSAPI GAP_LE_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GAP_LE_Event_Data_t *GAP_LE_Event_Data, uint32_t CallbackParameter)
{
	if (gBLE && BluetoothStackID && GAP_LE_Event_Data) {
	
		LOG_GAP_EVENT(GAP_LE_Event_Data->Event_Data_Type);
	
		switch(GAP_LE_Event_Data->Event_Data_Type) {
			case QAPI_BLE_ET_LE_CONNECTION_COMPLETE_E: {
				qapi_BLE_GAP_LE_Connection_Complete_Event_Data_t *Connection_Complete_Event_Data = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data;
				if (QAPI_BLE_HCI_ERROR_CODE_NO_ERROR == Connection_Complete_Event_Data->Status)
					gBLE->connection.bd_addr_type = Connection_Complete_Event_Data->Peer_Address_Type;
				break;
			}
			case QAPI_BLE_ET_LE_AUTHENTICATION_E: {
				qapi_BLE_GAP_LE_Authentication_Event_Data_t *Authentication_Event_Data = GAP_LE_Event_Data->Event_Data.GAP_LE_Authentication_Event_Data;
   				if (NULL != Authentication_Event_Data) {
					qapi_BLE_GAP_LE_Authentication_Response_Information_t GAP_LE_Authentication_Response_Information;
					qapi_BLE_Random_Number_t RandomNumber;
					qapi_BLE_Long_Term_Key_t GeneratedLTK;
     				uint16_t EDIV, LocalDiv;
					int Result;
					
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
								if (QAPI_BLE_COMPARE_BD_ADDR(gBLE->connection.bd_addr, Authentication_Event_Data->BD_ADDR)) {
									modBLEPeerConnection DeviceInfo = &gBLE->connection;
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
						case QAPI_BLE_LAT_PAIRING_REQUEST_E:
						case QAPI_BLE_LAT_EXTENDED_PAIRING_REQUEST_E:
							GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = QAPI_BLE_LAR_PAIRING_CAPABILITIES_E;
							GAP_LE_Authentication_Response_Information.Authentication_Data_Length = QAPI_BLE_GAP_LE_EXTENDED_PAIRING_CAPABILITIES_SIZE;
							configurePairingCapabilities(gBLE->encryption, gBLE->bonding, gBLE->mitm, gBLE->iocap, &(GAP_LE_Authentication_Response_Information.Authentication_Data.Extended_Pairing_Capabilities));
							if (QAPI_BLE_BTPS_ERROR_SECURE_CONNECTIONS_NOT_SUPPORTED == qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information)) {
								GAP_LE_Authentication_Response_Information.Authentication_Data.Extended_Pairing_Capabilities.Flags &= ~QAPI_BLE_GAP_LE_EXTENDED_PAIRING_CAPABILITIES_FLAGS_SECURE_CONNECTIONS;
								qapi_BLE_GAP_LE_Authentication_Response(gBLE->stackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
							}
							break;
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
									modMessagePostToMachine(gBLE->the, NULL, 0, gapPasskeyRequestEvent, NULL);
									break;
								case QAPI_BLE_CRT_DISPLAY_E:
									modMessagePostToMachine(gBLE->the, (uint8_t*)&Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Display_Passkey, sizeof(uint32_t), gapPasskeyNotifyEvent, NULL);
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
							if (QAPI_BLE_COMPARE_BD_ADDR(gBLE->connection.bd_addr, Authentication_Event_Data->BD_ADDR)) {
								modBLEPeerConnection DeviceInfo = &gBLE->connection;
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
								if (QAPI_BLE_COMPARE_BD_ADDR(gBLE->connection.bd_addr, Authentication_Event_Data->BD_ADDR)) {
									modBLEPeerConnection DeviceInfo = &gBLE->connection;
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
							modMessagePostToMachine(gBLE->the, (uint8_t*)&Authentication_Event_Data->Authentication_Event_Data.Pairing_Status, sizeof(qapi_BLE_GAP_LE_Pairing_Status_t), gapAuthCompleteEvent, NULL);
							break;
						case QAPI_BLE_LAT_IDENTITY_INFORMATION_E:
							/* Search for the entry for this slave to store the*/
							/* information into.                               */
							if (QAPI_BLE_COMPARE_BD_ADDR(gBLE->connection.bd_addr, Authentication_Event_Data->BD_ADDR)) {
								modBLEPeerConnection DeviceInfo = &gBLE->connection;
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
									modMessagePostToMachine(gBLE->the, NULL, 0, gapPasskeyRequestEvent, NULL);
									break;
								case QAPI_BLE_CRT_DISPLAY_E:
									modMessagePostToMachine(gBLE->the, (uint8_t*)&Authentication_Event_Data->Authentication_Event_Data.Extended_Confirmation_Request.Display_Passkey, sizeof(uint32_t), gapPasskeyNotifyEvent, NULL);
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
										modMessagePostToMachine(gBLE->the, (uint8_t*)&Authentication_Event_Data->Authentication_Event_Data.Extended_Confirmation_Request.Display_Passkey, sizeof(uint32_t), gapPasskeyConfirmEvent, NULL);
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
#if LOG_UNHANDLED_CALLBACK_EVENTS
			default: {
				UnhandledCallbackEventRecord u;
				c_strcpy(u.name, "GAP_LE_Event_Callback:\n");
				u.event = GAP_LE_Event_Data->Event_Data_Type;
				modMessagePostToMachine(gBLE->the, (uint8_t*)&u, sizeof(u), unhandledCallbackEvent, NULL);
				break;
			}
#endif
		}
	}
}

void QAPI_BLE_BTPSAPI GATT_Connection_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, uint32_t CallbackParameter)
{
	if (gBLE && BluetoothStackID && GATT_Connection_Event_Data) {
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
#if LOG_UNHANDLED_CALLBACK_EVENTS
			default: {
				UnhandledCallbackEventRecord u;
				c_strcpy(u.name, "GATT_Connection_Event_Callback:\n");
				u.event = GATT_Connection_Event_Data->Event_Data_Type;
				modMessagePostToMachine(gBLE->the, (uint8_t*)&u, sizeof(u), unhandledCallbackEvent, NULL);				
				break;
			}
#endif
		}
	}
}

void QAPI_BLE_BTPSAPI Server_Event_Callback(uint32_t BluetoothStackID, qapi_BLE_GATT_Server_Event_Data_t *GATT_Server_Event_Data, uint32_t CallbackParameter)
{
	if (gBLE && BluetoothStackID && GATT_Server_Event_Data) {
	
		LOG_GATTS_EVENT(GATT_Server_Event_Data->Event_Data_Type);
		
		switch(GATT_Server_Event_Data->Event_Data_Type) {
			case QAPI_BLE_ET_GATT_SERVER_READ_REQUEST_E:
            	if (GATT_Server_Event_Data->Event_Data.GATT_Read_Request_Data) {
					qapi_BLE_GATT_Read_Request_Data_t rrd = *GATT_Server_Event_Data->Event_Data.GATT_Read_Request_Data;
					modMessagePostToMachine(gBLE->the, (uint8_t*)&rrd, sizeof(rrd), readRequestEvent, 0);
				}
            	break;
			case QAPI_BLE_ET_GATT_SERVER_WRITE_REQUEST_E:
				if (GATT_Server_Event_Data->Event_Data.GATT_Write_Request_Data) {
					qapi_BLE_GATT_Write_Request_Data_t wrd = *GATT_Server_Event_Data->Event_Data.GATT_Write_Request_Data;
					uint8_t *value = c_malloc(wrd.AttributeValueLength);
					if ((NULL == value) && wrd.AttributeValueLength) {
						qapi_BLE_GATT_Error_Response(gBLE->stackID, wrd.TransactionID, wrd.AttributeOffset, QAPI_BLE_ATT_PROTOCOL_ERROR_CODE_REQUEST_NOT_SUPPORTED);
					}
					else {
						if (wrd.AttributeValueLength)
							c_memmove(value, wrd.AttributeValue, wrd.AttributeValueLength);
						wrd.AttributeValue = value;
						modMessagePostToMachine(gBLE->the, (uint8_t*)&wrd, sizeof(wrd), writeRequestEvent, 0);	
					}
				}
				break;
			case QAPI_BLE_ET_GATT_SERVER_DEVICE_CONNECTION_MTU_UPDATE_E:
				if (GATT_Server_Event_Data->Event_Data.GATT_Device_Connection_MTU_Update_Data) {
					qapi_BLE_GATT_Device_Connection_MTU_Update_Data_t mtu = *GATT_Server_Event_Data->Event_Data.GATT_Device_Connection_MTU_Update_Data;
					modMessagePostToMachine(gBLE->the, (uint8_t*)&mtu, sizeof(mtu), mtuExchangedEvent, 0);					
				}
				break;
				
#if LOG_UNHANDLED_CALLBACK_EVENTS
			default: {
				UnhandledCallbackEventRecord u;
				c_strcpy(u.name, "Server_Event_Callback:\n");
				u.event = GATT_Server_Event_Data->Event_Data_Type;
				modMessagePostToMachine(gBLE->the, (uint8_t*)&u, sizeof(u), unhandledCallbackEvent, NULL);				
				break;
			}
#endif
		}
	}
}

#if LOG_GATTS
static void logGATTSEvent(qapi_BLE_GATT_Server_Event_Type_t event) {
	switch(event) {
		case QAPI_BLE_ET_GATT_SERVER_DEVICE_CONNECTION_E: modLog("QAPI_BLE_ET_GATT_SERVER_DEVICE_CONNECTION_E"); break;
		case QAPI_BLE_ET_GATT_SERVER_DEVICE_DISCONNECTION_E: modLog("QAPI_BLE_ET_GATT_SERVER_DEVICE_DISCONNECTION_E"); break;
		case QAPI_BLE_ET_GATT_SERVER_READ_REQUEST_E: modLog("QAPI_BLE_ET_GATT_SERVER_READ_REQUEST_E"); break;
		case QAPI_BLE_ET_GATT_SERVER_WRITE_REQUEST_E: modLog("QAPI_BLE_ET_GATT_SERVER_WRITE_REQUEST_E"); break;
		case QAPI_BLE_ET_GATT_SERVER_SIGNED_WRITE_REQUEST_E: modLog("QAPI_BLE_ET_GATT_SERVER_SIGNED_WRITE_REQUEST_E"); break;
		case QAPI_BLE_ET_GATT_SERVER_EXECUTE_WRITE_REQUEST_E: modLog("QAPI_BLE_ET_GATT_SERVER_EXECUTE_WRITE_REQUEST_E"); break;
		case QAPI_BLE_ET_GATT_SERVER_EXECUTE_WRITE_CONFIRMATION_E: modLog("QAPI_BLE_ET_GATT_SERVER_EXECUTE_WRITE_CONFIRMATION_E"); break;
		case QAPI_BLE_ET_GATT_SERVER_CONFIRMATION_RESPONSE_E: modLog("QAPI_BLE_ET_GATT_SERVER_CONFIRMATION_RESPONSE_E"); break;
		case QAPI_BLE_ET_GATT_SERVER_DEVICE_CONNECTION_MTU_UPDATE_E: modLog("QAPI_BLE_ET_GATT_SERVER_DEVICE_CONNECTION_MTU_UPDATE_E"); break;
		case QAPI_BLE_ET_GATT_SERVER_DEVICE_BUFFER_EMPTY_E: modLog("QAPI_BLE_ET_GATT_SERVER_DEVICE_BUFFER_EMPTY_E"); break;
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




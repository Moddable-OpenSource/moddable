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
#include "modBLESM.h"

#define QAPI_USE_BLE
#include "qapi.h"
#include "qapi_persist.h"

#define kBondsRoot "/spinor/moddable/prefs/ble"
#define kBondsName "bonds"
#define kBondsExt ".bin"

typedef struct {
	uint16_t count;
	modBLEBondedDeviceRecord devices[1];
} modBLEStoredBondedDevicesRecord, *modBLEStoredBondedDevices;

static modBLEBondedDevice gBondedDeviceList = NULL;

static int loadBondedDevices(void);
static int storeBondedDevices(void);

void xs_ble_sm_delete_all_bondings(xsMachine *the)
{
	int result;
	qapi_Persist_Handle_t Handle = NULL;
	result = qapi_Persist_Initialize(&Handle, kBondsRoot, kBondsName, kBondsExt, NULL, 0);
	if (0 == result) {
		qapi_Persist_Delete(Handle);
		qapi_Persist_Cleanup(Handle);
	}
	gBondedDeviceList = NULL;
}

void xs_ble_sm_delete_bonding(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
	uint8_t addressType = xsmcToInteger(xsArg(1));
	qapi_BLE_BD_ADDR_t bd_addr;	
	modBLEBondedDevice device;

	c_memmove(&bd_addr, address, 6);
	device = modBLEBondedDevicesFindByAddress(bd_addr);
	if (NULL != device) {
		modBLEBondedDevicesRemove(device);
#if MODDEF_BLE_CLIENT
		modBLEClientBondingRemoved(&bd_addr, addressType);
#endif
#if MODDEF_BLE_SERVER
		modBLEServerBondingRemoved(&bd_addr, addressType);
#endif
	}
}

void modBLEBondedDevicesAdd(modBLEBondedDevice device)
{
	modBLEBondedDevice walker;
	if (NULL == gBondedDeviceList)
		loadBondedDevices();
	if (NULL == gBondedDeviceList)
		gBondedDeviceList = device;
	else {
		for (walker = gBondedDeviceList; walker->next; walker = walker->next)
			;
		walker->next = device;
	}
	storeBondedDevices();
}

void modBLEBondedDevicesRemove(modBLEBondedDevice device)
{
	modBLEBondedDevice walker, prev = NULL;
	if (NULL == gBondedDeviceList)
		loadBondedDevices();
	for (walker = gBondedDeviceList; NULL != walker; prev = walker, walker = walker->next) {
		if (device == walker) {
			if (NULL == prev)
				gBondedDeviceList = walker->next;
			else
				prev->next = walker->next;
			c_free(device);
			break;
		}
	}
	storeBondedDevices();
}

modBLEBondedDevice modBLEBondedDevicesFindByAddress(qapi_BLE_BD_ADDR_t bd_addr)
{
	modBLEBondedDevice walker;
	if (NULL == gBondedDeviceList)
		loadBondedDevices();
	for (walker = gBondedDeviceList; NULL != walker; walker = walker->next)
		if (QAPI_BLE_COMPARE_BD_ADDR(bd_addr, walker->LastAddress))
			break;
	return walker;
}

int loadBondedDevices(void)
{
	qapi_Persist_Handle_t Handle = NULL;
	uint8_t *data = NULL;
	modBLEStoredBondedDevices devices;
	modBLEBondedDevice device, walker;
	uint32_t length;
	int i, result;
	result = qapi_Persist_Initialize(&Handle, kBondsRoot, kBondsName, kBondsExt, NULL, 0);
	if (0 != result) goto bail;
	result = qapi_Persist_Get(Handle, &length, &data);
	if (0 != result) goto bail;
	devices = (modBLEStoredBondedDevices)data;
	for (i = 0; i < devices->count; ++i) {
		device = c_malloc(sizeof(modBLEBondedDeviceRecord));
		if (!device) {
			result = -1;
			goto bail;
		}
		c_memmove(device, &devices->devices[i], sizeof(modBLEBondedDeviceRecord));
		if (NULL == gBondedDeviceList)
			gBondedDeviceList = device;
		else {
			modBLEBondedDevice walker;
			for (walker = gBondedDeviceList; walker->next; walker = walker->next)
				;
			walker->next = device;
		}
	}
bail:
	if (0 != result) {
		walker = gBondedDeviceList;
		while (walker) {
			device = walker;
			walker = walker->next;
			c_free(device);
		}
		gBondedDeviceList = NULL;
	}
	if (NULL != Handle) {
		if (NULL != data)
			qapi_Persist_Free(Handle, data);
		qapi_Persist_Cleanup(Handle);
	}
	return result;
}

int storeBondedDevices(void)
{
	qapi_Persist_Handle_t Handle = NULL;
	modBLEStoredBondedDevices devices = NULL;
	modBLEBondedDevice walker;
	uint32_t length;
	int i, count, result;
	
	if (NULL == gBondedDeviceList) return 0;
	result = qapi_Persist_Initialize(&Handle, kBondsRoot, kBondsName, kBondsExt, NULL, 0);
	if (0 != result) goto bail;
	for (walker = gBondedDeviceList, count = 0; NULL != walker; walker = walker->next)
		++count;
	length = sizeof(modBLEStoredBondedDevicesRecord) + ((count - 1) * sizeof(modBLEBondedDeviceRecord));
	devices = c_malloc(length);
	if (!devices) {
		result = -1;
		goto bail;
	}
	devices->count = count;
	for (walker = gBondedDeviceList, i = 0; NULL != walker; ++i, walker = walker->next) {
		devices->devices[i] = *walker;
		devices->devices[i].next = NULL;
	}
	result = qapi_Persist_Put(Handle, length, (uint8_t*)devices);
	if (0 != result) goto bail;
bail:
	if (NULL != devices)
		c_free(devices);
	if (NULL != Handle)
		qapi_Persist_Cleanup(Handle);
	return result;
}

void configurePairingCapabilities(uint8_t encryption, uint8_t bonding, uint8_t mitm, uint16_t ioCapability, qapi_BLE_GAP_LE_Extended_Pairing_Capabilities_t *Capabilities)
{
	c_memset(Capabilities, 0, QAPI_BLE_GAP_LE_EXTENDED_PAIRING_CAPABILITIES_SIZE);

	Capabilities->Bonding_Type = (bonding ? QAPI_BLE_LBT_BONDING_E : QAPI_BLE_LBT_NO_BONDING_E);
  	switch(ioCapability) {
 		case NoInputNoOutput:
 			Capabilities->IO_Capability = QAPI_BLE_LIC_NO_INPUT_NO_OUTPUT_E;
 			break;
 		case DisplayOnly:
 			Capabilities->IO_Capability = QAPI_BLE_LIC_DISPLAY_ONLY_E;
 			break;
 		case KeyboardOnly:
 			Capabilities->IO_Capability = QAPI_BLE_LIC_KEYBOARD_ONLY_E;
 			break;
 		case KeyboardDisplay:
 			Capabilities->IO_Capability = QAPI_BLE_LIC_KEYBOARD_DISPLAY_E;
 			break;
 		case DisplayYesNo:
 			Capabilities->IO_Capability = QAPI_BLE_LIC_DISPLAY_YES_NO_E;
 			break;
 	} 	
	if (mitm)
		Capabilities->Flags |= QAPI_BLE_GAP_LE_EXTENDED_PAIRING_CAPABILITIES_FLAGS_MITM_REQUESTED;
	if (encryption)
		Capabilities->Flags |= QAPI_BLE_GAP_LE_EXTENDED_PAIRING_CAPABILITIES_FLAGS_SECURE_CONNECTIONS;

	Capabilities->Maximum_Encryption_Key_Size = QAPI_BLE_GAP_LE_MAXIMUM_ENCRYPTION_KEY_SIZE;

	Capabilities->Receiving_Keys.Encryption_Key     = TRUE;
	Capabilities->Receiving_Keys.Identification_Key = TRUE;
	Capabilities->Receiving_Keys.Signing_Key        = FALSE;
	Capabilities->Receiving_Keys.Link_Key           = FALSE;

	Capabilities->Sending_Keys.Encryption_Key       = TRUE;
	Capabilities->Sending_Keys.Identification_Key   = TRUE;
	Capabilities->Sending_Keys.Signing_Key          = FALSE;
	Capabilities->Sending_Keys.Link_Key             = FALSE;
}

void generateEncryptionKeys(uint32_t stackID, qapi_BLE_Encryption_Key_t *ER, qapi_BLE_Encryption_Key_t *IR, qapi_BLE_Encryption_Key_t *DHK, qapi_BLE_Encryption_Key_t *IRK)
{
	uint8_t                  Status;
	unsigned int             MaxSize;
	qapi_BLE_Random_Number_t RandomNumber;

	QAPI_BLE_ASSIGN_ENCRYPTION_KEY(*ER, 0x51, 0x4A, 0xEE, 0x80, 0xD0, 0x19, 0xB2, 0x16, 0x45, 0x20, 0xB2, 0x13, 0x37, 0xE1, 0xBA, 0x28);
	QAPI_BLE_ASSIGN_ENCRYPTION_KEY(*IR, 0xFE, 0xC9, 0xFC, 0x48, 0x8C, 0x3C, 0x23, 0x95, 0xC0, 0x70, 0x6B, 0x09, 0x88, 0xA0, 0x09, 0x41);

	MaxSize = (sizeof(qapi_BLE_Random_Number_t) > (sizeof(qapi_BLE_Encryption_Key_t) / 2)) ? (sizeof(qapi_BLE_Encryption_Key_t) / 2) : sizeof(qapi_BLE_Random_Number_t);

	if ((!qapi_BLE_HCI_LE_Rand(stackID, &Status, &RandomNumber)) && (!Status))
		c_memcpy(ER, &RandomNumber, MaxSize);

	if ((!qapi_BLE_HCI_LE_Rand(stackID, &Status, &RandomNumber)) && (!Status))
		c_memcpy(&(((uint8_t *)ER)[sizeof(qapi_BLE_Encryption_Key_t) / 2]), &RandomNumber, MaxSize);

	if ((!qapi_BLE_HCI_LE_Rand(stackID, &Status, &RandomNumber)) && (!Status))
		c_memcpy(IR, &RandomNumber, MaxSize);

	if ((!qapi_BLE_HCI_LE_Rand(stackID,  &Status,  &RandomNumber)) && (!Status))
		c_memcpy(&(((uint8_t *)IR)[sizeof(qapi_BLE_Encryption_Key_t) / 2]), &RandomNumber, MaxSize);

	qapi_BLE_GAP_LE_Diversify_Function(stackID, IR, 1, 0, IRK);
	qapi_BLE_GAP_LE_Diversify_Function(stackID, IR, 3, 0, DHK);
}




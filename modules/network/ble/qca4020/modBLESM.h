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

#ifndef __mod_ble_sm__
#define __mod_ble_sm__

#define QAPI_USE_BLE
#include "qapi.h"

#define DEVICE_INFO_FLAGS_LTK_VALID                         0x01
#define DEVICE_INFO_FLAGS_IRK_VALID                         0x02
#define DEVICE_INFO_FLAGS_ADDED_TO_WHITE_LIST               0x20
#define DEVICE_INFO_FLAGS_ADDED_TO_RESOLVING_LIST           0x40

typedef struct modBLEBondedDeviceRecord modBLEBondedDeviceRecord;
typedef modBLEBondedDeviceRecord *modBLEBondedDevice;

struct modBLEBondedDeviceRecord {
	struct modBLEBondedDeviceRecord *next;

	uint8_t							Flags;				// bitmask flags above
	qapi_BLE_BD_ADDR_t				LastAddress;		// remote address
	qapi_BLE_GAP_LE_Address_Type_t	LastAddressType;	// remote address type
	qapi_BLE_BD_ADDR_t				IdentityAddress;	// identity address and type from QAPI_BLE_LAT_IDENTITY_INFORMATION_E event
	qapi_BLE_GAP_LE_Address_Type_t	IdentityAddressType;
	uint8_t							EncryptionKeySize;	// key size from QAPI_BLE_LAT_ENCRYPTION_INFORMATION_E (1) event
	qapi_BLE_Long_Term_Key_t		LTK;				// long term key from QAPI_BLE_LAT_ENCRYPTION_INFORMATION_E event
	qapi_BLE_Encryption_Key_t		IRK;				// IRK from QAPI_BLE_LAT_IDENTITY_INFORMATION_E (2) event
};

void modBLEBondedDevicesAdd(modBLEBondedDevice device);
void modBLEBondedDevicesRemove(modBLEBondedDevice device);
modBLEBondedDevice modBLEBondedDevicesFindByAddress(qapi_BLE_BD_ADDR_t bd_addr);

void configurePairingCapabilities(uint8_t encryption, uint8_t bonding, uint8_t mitm, uint16_t ioCapability, qapi_BLE_GAP_LE_Extended_Pairing_Capabilities_t *Capabilities);
void generateEncryptionKeys(uint32_t stackID, qapi_BLE_Encryption_Key_t *ER, qapi_BLE_Encryption_Key_t *IR, qapi_BLE_Encryption_Key_t *DHK, qapi_BLE_Encryption_Key_t *IRK);

void modBLEClientBondingRemoved(qapi_BLE_BD_ADDR_t *address, qapi_BLE_GAP_LE_Address_Type_t addressType);
void modBLEServerBondingRemoved(qapi_BLE_BD_ADDR_t *address, qapi_BLE_GAP_LE_Address_Type_t addressType);

#endif

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

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"
#include "modBLE.h"
#include "qapi_ble_gap.h"

static modBLEWhitelistAddress gWhitelist = NULL;

static int setWhitelist(void);
static int qca4020ClearWhitelist(void);

extern uint32_t gBluetoothStackID;	// @@

void xs_gap_whitelist_add(xsMachine *the)
{
	modBLEWhitelistAddress entry;
	BLEAddressType addressType;
	uint8_t *address;
	int rc;
	
	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_addressType);
	addressType = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_address);
	address = (uint8_t*)xsmcToArrayBuffer(xsVar(0));
	
	if (modBLEWhitelistContains(addressType, address))
		return;

	entry = c_calloc(1, sizeof(modBLEWhitelistAddressRecord));
	if (NULL == entry)
		xsUnknownError("out of memory");
		
	c_memmove(entry->address, address, 6);
	entry->addressType = addressType;
	
	if (NULL == gWhitelist)
		gWhitelist = entry;
	else {
		modBLEWhitelistAddress walker;
		for (walker = gWhitelist; walker->next; walker = walker->next)
			;
		walker->next = entry;
	}
	
	rc = setWhitelist();
	if (0 != rc)
		xsUnknownError("whitelist add failed");
}

void xs_gap_whitelist_remove(xsMachine *the)
{
	BLEAddressType addressType;
	uint8_t *address;
	int rc;
	
	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_addressType);
	addressType = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_address);
	address = (uint8_t*)xsmcToArrayBuffer(xsVar(0));

	modBLEWhitelistAddress walker, prev = NULL;
	for (walker = gWhitelist; NULL != walker; prev = walker, walker = walker->next) {
		if (addressType == walker->addressType && 0 == c_memcmp(address, walker->address, sizeof(walker->address))) {
			if (NULL == prev)
				gWhitelist = walker->next;
			else
				prev->next = walker->next;
			c_free(walker);
			break;
		}
	}
	
	if (NULL == gWhitelist)
		rc = qca4020ClearWhitelist();
	else
		rc = setWhitelist();
		
	if (0 != rc)
		xsUnknownError("whitelist remove failed");
}

void xs_gap_whitelist_clear(xsMachine *the)
{
	modBLEWhitelistClear();
}

void modBLEWhitelistClear(void)
{
	modBLEWhitelistAddress walker = gWhitelist;
	
	while (walker != NULL) {
		modBLEWhitelistAddress addr = walker;
		walker = walker->next;
		c_free(addr);
	}
	
	gWhitelist = NULL;
	
	qca4020ClearWhitelist();
}

static int setWhitelist()
{
	modBLEWhitelistAddress walker;
	uint16_t count;
	uint32_t added;
	qapi_BLE_GAP_LE_White_List_Entry_t *entries = NULL;
	int rc = 0;
	
	if (NULL == gWhitelist)
		goto bail;
		
	walker = gWhitelist;
	count = 0;
	while (walker != NULL) {
		++count;
		walker = walker->next;
	}
	
	entries = c_calloc(1, count * sizeof(qapi_BLE_GAP_LE_White_List_Entry_t));
	if (NULL == entries) {
		rc = -1;
		goto bail;
	}
	
	rc = qca4020ClearWhitelist();
	if (rc < 0)
		goto bail;
		
	walker = gWhitelist;
	count = 0;
	while (walker != NULL) {
		switch(walker->addressType) {
			case kBLEAddressTypeRandom:
				entries[count].Address_Type = QAPI_BLE_LAT_RANDOM_E;
				break;
			case kBLEAddressTypeRPAPublic:
				entries[count].Address_Type = QAPI_BLE_LAT_PUBLIC_IDENTITY_E;
				break;
			case kBLEAddressTypeRPARandom:
				entries[count].Address_Type = QAPI_BLE_LAT_RANDOM_IDENTITY_E;
				break;
			case kBLEAddressTypePublic:
			default:
				entries[count].Address_Type = QAPI_BLE_LAT_PUBLIC_E;
				break;
		}
		c_memmove(&entries[count].Address, walker->address, 6);
		++count;
		walker = walker->next;
	}
	rc = qapi_BLE_GAP_LE_Add_Device_To_White_List(gBluetoothStackID, count, entries, &added);
	
bail:
	if (NULL != entries)
		c_free(entries);
		
	return rc;
}

int modBLEWhitelistContains(uint8_t addressType, uint8_t *address)
{
	modBLEWhitelistAddress walker = gWhitelist;

	while (NULL != walker) {
		if (addressType == walker->addressType && 0 == c_memcmp(address, walker->address, 6))
			return 1;
		walker = walker->next;
	}
		
	return 0;
}

int qca4020ClearWhitelist(void)
{
	uint32_t removed;
	return qapi_BLE_GAP_LE_Remove_Device_From_White_List(gBluetoothStackID, 0, NULL, &removed);
}


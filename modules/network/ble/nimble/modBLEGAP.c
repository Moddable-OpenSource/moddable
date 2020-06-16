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
#include "ble.h"
#include "ble_hs_hci_priv.h"

static modBLEWhitelistAddress gWhitelist = NULL;

static int setWhitelist(void);

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
	entry = c_calloc(1, sizeof(modBLEWhitelistAddressRecord));
	if (NULL == entry)
		xsUnknownError("out of memory");
		
	switch(addressType) {
		case kBLEAddressTypeRandom:
			entry->addressType = BLE_ADDR_RANDOM;
			break;
		case kBLEAddressTypeRPAPublic:
			entry->addressType = BLE_ADDR_PUBLIC_ID;
			break;
		case kBLEAddressTypeRPARandom:
			entry->addressType = BLE_ADDR_RANDOM_ID;
			break;
		case kBLEAddressTypePublic:
		default:
			entry->addressType = BLE_ADDR_PUBLIC;
			break;
	}
	c_memmove(entry->address, address, 6);
	
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
	
	rc = setWhitelist();
	if (0 != rc)
		xsUnknownError("whitelist remove failed");
}

void xs_gap_whitelist_clear(xsMachine *the)
{
	modBLEWhitelistAddress walker = gWhitelist;
	
	while (walker != NULL) {
		modBLEWhitelistAddress addr = walker;
		walker = walker->next;
		c_free(addr);
	}
	
	ble_hs_lock();
	ble_hs_hci_cmd_tx_empty_ack(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CLEAR_WHITE_LIST), NULL, 0);  
    ble_hs_unlock();
}

static int setWhitelist()
{
	modBLEWhitelistAddress walker;
	uint16_t count;
	ble_addr_t *addr = NULL;
	int rc = 0;
	
	if (NULL == gWhitelist)
		goto bail;
		
	walker = gWhitelist;
	count = 0;
	while (walker != NULL) {
		++count;
		walker = walker->next;
	}
	
	addr = c_malloc(count * sizeof(ble_addr_t));
	if (NULL == addr) {
		rc = -1;
		goto bail;
	}
		
	ble_hs_lock();
    rc = ble_hs_hci_cmd_tx_empty_ack(
        BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CLEAR_WHITE_LIST),
        NULL, 0);
    ble_hs_unlock();
    
	if (0 != rc)
		goto bail;
		
	walker = gWhitelist;
	count = 0;
	while (walker != NULL) {
		addr[count].type = walker->addressType;
		c_memmove(addr[count].val, walker->address, 6);
		++count;
		walker = walker->next;
	}
	rc = ble_gap_wl_set(addr, count);
	
bail:
	if (NULL != addr)
		c_free(addr);
		
	return rc;
}

modBLEWhitelistAddress modBLEGetWhitelist()
{
	return gWhitelist;
}


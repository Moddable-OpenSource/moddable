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
#include "nrf_sdh_ble.h"

static modBLEWhitelistAddress gWhitelist = NULL;

static int setWhitelist(void);
static int nrf52ClearWhitelist(void);

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
		rc = nrf52ClearWhitelist();
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
	
	nrf52ClearWhitelist();
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

int setWhitelist()
{
	modBLEWhitelistAddress walker;
	uint16_t count;
	ble_gap_addr_t const * whitelist_addr_ptrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
	ble_gap_addr_t whitelist_addrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
	int rc = 0;

	if (NULL == gWhitelist)
		goto bail;
		
	walker = gWhitelist;
	count = 0;
	while (walker != NULL) {
		++count;
		walker = walker->next;
	}
	
	rc = nrf52ClearWhitelist();
    
	if (0 != rc)
		goto bail;
		
	walker = gWhitelist;
	count = 0;
	while (walker != NULL) {
		switch(walker->addressType) {
			case kBLEAddressTypeRandom:
				whitelist_addrs[count].addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
				break;
			case kBLEAddressTypeRPAPublic:
				whitelist_addrs[count].addr_type = BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_RESOLVABLE;
				break;
			case kBLEAddressTypeRPARandom:
				whitelist_addrs[count].addr_type = BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE;
				break;
			case kBLEAddressTypePublic:
			default:
				whitelist_addrs[count].addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;
				break;
		}
		whitelist_addrs[count].addr_id_peer = 0;
		c_memmove(whitelist_addrs[count].addr, walker->address, 6);
		++count;
		walker = walker->next;
	}
	
    for (int i = 0; i < BLE_GAP_WHITELIST_ADDR_MAX_COUNT; i++)
        whitelist_addr_ptrs[i] = &whitelist_addrs[i];

	rc = sd_ble_gap_whitelist_set(whitelist_addr_ptrs, count);
	
bail:
	return rc;
}

static int nrf52ClearWhitelist(void)
{
	return sd_ble_gap_whitelist_set(NULL, 0);
}


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
#include "esp_bt.h"
#include "esp_gap_ble_api.h"

static modBLEWhitelistAddress gWhitelist = NULL;

void xs_gap_whitelist_add(xsMachine *the)
{
	modBLEWhitelistAddress entry;
	BLEAddressType addressType;
	uint8_t *address;
	esp_bd_addr_t bda;
	int ret;
	
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

	for (int i = 0; i < 6; ++i)
		bda[i] = address[5 - i];

	ret = esp_ble_gap_update_whitelist(ESP_BLE_WHITELIST_ADD, bda, addressType);
	
	if (0 != ret)
		xsUnknownError("whitelist add failed");
}

void xs_gap_whitelist_remove(xsMachine *the)
{
	BLEAddressType addressType;
	uint8_t *address;
	esp_bd_addr_t bda;
	int ret = 0;
	
	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_addressType);
	addressType = xsmcToInteger(xsVar(0));
	xsmcGet(xsVar(0), xsArg(0), xsID_address);
	address = (uint8_t*)xsmcToArrayBuffer(xsVar(0));

	modBLEWhitelistAddress walker, prev = NULL;
	for (walker = gWhitelist; NULL != walker; prev = walker, walker = walker->next) {
		if (0 == c_memcmp(address, walker->address, sizeof(walker->address))) {
			if (NULL == prev)
				gWhitelist = walker->next;
			else
				prev->next = walker->next;

			for (int i = 0; i < 6; ++i)
				bda[i] = address[5 - i];
				
			ret = esp_ble_gap_update_whitelist(ESP_BLE_WHITELIST_REMOVE, bda, addressType);
			
			c_free(walker);
			break;
		}
	}

	if (0 != ret)
		xsUnknownError("whitelist remove failed");
}

void xs_gap_whitelist_clear(xsMachine *the)
{
	modBLEWhitelistClear();
}

void modBLEWhitelistClear(void)
{
	esp_bd_addr_t bda;
	modBLEWhitelistAddress walker = gWhitelist;
	
	while (walker != NULL) {
		modBLEWhitelistAddress addr = walker;
		walker = walker->next;
		for (int i = 0; i < 6; ++i)
			bda[i] = addr->address[5 - i];
		esp_ble_gap_update_whitelist(ESP_BLE_WHITELIST_REMOVE, bda, addr->addressType);
		c_free(addr);
	}
	gWhitelist = NULL;
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

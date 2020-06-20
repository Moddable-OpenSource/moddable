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
#include "peer_manager.h"

static modBLEWhitelistAddress gWhitelist = NULL;

static int setWhitelist(void);
static modBLEWhitelistAddress findInWhitelist(BLEAddressType addressType, uint8_t *address);
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
	
	if (findInWhitelist(addressType, address))
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
	modBLEWhitelistAddress walker = gWhitelist;
	
	while (walker != NULL) {
		modBLEWhitelistAddress addr = walker;
		walker = walker->next;
		c_free(addr);
	}
	
	nrf52ClearWhitelist();
}

static int setWhitelist()
{
	modBLEWhitelistAddress walker;
	uint16_t count;
	uint32_t added;
	int rc = 0;
	
	// @@ TBD
	
	if (NULL == gWhitelist)
		goto bail;
		
	walker = gWhitelist;
	count = 0;
	while (walker != NULL) {
		++count;
		walker = walker->next;
	}
	
bail:		
	return rc;
}

static modBLEWhitelistAddress findInWhitelist(BLEAddressType addressType, uint8_t *address)
{
	modBLEWhitelistAddress walker = gWhitelist;
	
	if (NULL == walker)
		return NULL;
		
	while (NULL != walker) {
		if (walker->addressType == addressType && 0 == c_memcmp(walker->address, address, 6))
			return walker;
		walker = walker->next;
	}		
	
	return NULL;
}

static int nrf52ClearWhitelist(void)
{
	return pm_whitelist_set(NULL, 0);
}


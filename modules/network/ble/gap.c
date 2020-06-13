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

static modBLEWhitelistAddress gWhitelist = NULL;

void xs_gap_set_whitelist(xsMachine *the)
{
	int c = xsToInteger(xsGet(xsArg(0), xsID_length));
	AddressType addressType;
	uint8_t *address;
	modBLEWhitelistAddress entry, whitelist = NULL;
	
	modBLEGAPClearWhitelist();

	xsmcVars(2);
	for (int i = 0; i < c; i++) {
		xsSlot slot = xsInteger(i);
		xsmcGetAt(xsVar(0), xsArg(0), slot);
		xsmcGet(xsVar(1), xsVar(0), xsID_addressType);
		addressType = xsmcToInteger(xsVar(1));
		xsmcGet(xsVar(1), xsVar(0), xsID_address);
		address = (uint8_t*)xsmcToArrayBuffer(xsVar(1));
		entry = c_calloc(1, sizeof(modBLEWhitelistAddressRecord));
		if (NULL == entry)
			xsUnknownError("out of memory");
		entry->addressType = addressType;
		c_memmove(entry->address, address, 6);
		if (NULL == whitelist)
			whitelist = entry;
		else {
			modBLEWhitelistAddress walker;
			for (walker = whitelist; walker->next; walker = walker->next)
				;
			walker->next = entry;
		}
	}
	gWhitelist = whitelist;
}

void xs_gap_clear_whitelist(xsMachine *the)
{
	modBLEGAPClearWhitelist();
}

modBLEWhitelistAddress modBLEGAPGetWhitelist()
{
	return gWhitelist;
}

void modBLEGAPClearWhitelist()
{
	modBLEWhitelistAddress walker = gWhitelist;
	while (NULL != walker) {
		modBLEWhitelistAddress address = walker;
		walker = walker->next;
		c_free(address);
	}
	gWhitelist = NULL;
}


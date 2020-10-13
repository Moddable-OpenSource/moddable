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

#include "xsHost.h"
#include "modBLE.h"

static modBLEConnection gConnections = NULL;

modBLEConnection modBLEConnectionFindByConnectionID(int16_t id)
{
	modBLEConnection walker;
	modCriticalSectionDeclare;
	modCriticalSectionBegin();
	for (walker = gConnections; NULL != walker; walker = walker->next)
		if (id == walker->id)
			goto bail;
			
bail:
	modCriticalSectionEnd();
	return walker;
}

modBLEConnection modBLEConnectionFindByAddress(uint8_t *address)
{
	modBLEConnection walker;
	modCriticalSectionDeclare;
	modCriticalSectionBegin();
	for (walker = gConnections; NULL != walker; walker = walker->next) {
		if (0 == c_memcmp(address, walker->address, 6))
			goto bail;
	}
bail:
	modCriticalSectionEnd();
	return walker;
}

modBLEConnection modBLEConnectionFindByAddressAndType(uint8_t *address, uint8_t addressType)
{
	modBLEConnection walker;
	modCriticalSectionDeclare;
	modCriticalSectionBegin();
	for (walker = gConnections; NULL != walker; walker = walker->next) {
		if (0 == c_memcmp(address, walker->address, 6) && addressType == walker->addressType)
			goto bail;
	}
bail:
	modCriticalSectionEnd();
	return walker;
}

void modBLEConnectionAdd(modBLEConnection connection)
{
	modCriticalSectionDeclare;
	modCriticalSectionBegin();
	if (!gConnections)
		gConnections = connection;
	else {
		modBLEConnection walker;
		for (walker = gConnections; walker->next; walker = walker->next)
			;
		walker->next = connection;
	}
	modCriticalSectionEnd();
}

void modBLEConnectionRemove(modBLEConnection connection)
{
	modBLEConnection walker, prev = NULL;
	modCriticalSectionDeclare;
	modCriticalSectionBegin();
	for (walker = gConnections; NULL != walker; prev = walker, walker = walker->next) {
		if (connection == walker) {
			if (NULL == prev)
				gConnections = walker->next;
			else
				prev->next = walker->next;
			c_free(connection);
			goto bail;
		}
	}
bail:
	modCriticalSectionEnd();
}

modBLEConnection modBLEConnectionGetFirst(void)
{
	return gConnections;
}

modBLEConnection modBLEConnectionGetNext(modBLEConnection connection)
{
	modBLEConnection walker = NULL;
	modCriticalSectionDeclare;
	modCriticalSectionBegin();
	if (!connection)
		goto bail;
	walker = connection->next;
bail:
	modCriticalSectionEnd();
	return walker;
}

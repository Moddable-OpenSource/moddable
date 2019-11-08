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

#include "xsHost.h"
#include "modBLECommon.h"

const uint16_t primary_service_uuid = 0x2800;
const uint16_t character_declaration_uuid = 0x2803;
const uint16_t character_client_config_uuid = 0x2902;

static int16_t useCount = 0;

void uuidToBuffer(uint8_t *buffer, ble_uuid_t *uuid, uint16_t *length)
{
	uint8_t uuid_le_len;
	sd_ble_uuid_encode(uuid, &uuid_le_len, buffer);
	*length = uuid_le_len;
}

void bufferToUUID(ble_uuid_t *uuid, uint8_t *buffer, uint16_t length)
{
	sd_ble_uuid_decode(length, buffer, uuid);
}

int modBLEPlatformInitialize(void)
{
	if (0 != useCount++)
		return 0;

}

int modBLEPlatformTerminate(void)
{
	if (0 != --useCount)
		return 0;

	
	return 0;
}

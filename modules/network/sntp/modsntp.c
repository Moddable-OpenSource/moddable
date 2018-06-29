/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

void xs_sntp_packet(xsMachine *the)
{
	unsigned char *packet;

	xsmcSetArrayBuffer(xsResult, NULL, 48);

	packet = xsmcToArrayBuffer(xsResult);
	packet[0] = (4 << 3) | (3 << 0);		// version 4, mode 3 (client)
}

void xs_sntp_toNumber(xsMachine *the)
{
	uint8_t bytes[4];
	int32_t result;

	bytes[0] = (uint8_t)xsmcToInteger(xsArg(0));
	bytes[1] = (uint8_t)xsmcToInteger(xsArg(1));
	bytes[2] = (uint8_t)xsmcToInteger(xsArg(2));
	bytes[3] = (uint8_t)xsmcToInteger(xsArg(3));

	result = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];

	result -= 2208988800UL;		// convert from NTP to Unix Epoch time value

	xsmcSetInteger(xsResult, result);
}

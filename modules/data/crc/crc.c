/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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

#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#include "xsHost.h"

static int table_configured = 0;	// will contain polynomial when configured
static int initial_val = 0;			// initial value
static uint8_t crc8_table[257];		// initial value at the end

static void setup_crc8_table(uint8_t polynomial, uint8_t initial_value) {
	int i, j, cur;

	if (table_configured == polynomial && initial_value == initial_val)
		return;

	initial_val = initial_value;
	for (i=0; i<256; i++) {
		for (cur=i, j=0; j<8; j++) {
			if (cur & 0x80)
				cur = ((cur << 1) ^ polynomial) % 256;
			else
				cur = (cur << 1) % 256;
		}
		crc8_table[i] = cur;
	}
	table_configured = polynomial;
}

void xs_setup_crc8_table(xsMachine *the) {
	int poly, initial;
	poly = xsmcToInteger(xsArg(0));
	initial = xsmcToInteger(xsArg(1));
	setup_crc8_table(poly, initial);
}

uint8_t checksum8(uint8_t *bytes, uint32_t length) {
	uint8_t c = initial_val;
	uint32_t i;

	for (i=0; i<length; i++)
		c = crc8_table[(c ^ bytes[i]) % 256];

	return c;
}

void xs_checksum8(xsMachine *the) {
	int len;
	uint8_t *p;

	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		len = xsmcGetArrayBufferLength(xsArg(0));
		p = xsmcToArrayBuffer(xsArg(0));
	}
	if (xsmcArgc > 1)
		len = xsmcToInteger(xsArg(1));

	xsmcSetInteger(xsResult, checksum8(p, len));
}


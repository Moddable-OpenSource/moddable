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

static uint8_t table8_init = 0;	// will contain polynomial when configured
static uint8_t *crc8_table = NULL;
//static uint8_t crc8_table[256];
static uint16_t table16_init = 0; // will contain polynomial when configured
static uint16_t *crc16_table = NULL;
//static uint16_t crc16_table[256];

static void setup_crc8_table(uint8_t polynomial) {
	int i, j, cur;

	if (NULL == crc8_table)
		crc8_table = (uint8_t*)c_malloc(256);
	else if (table8_init == polynomial)
		return;

	for (i=0; i<256; i++) {
		for (cur=i, j=0; j<8; j++) {
			if (cur & 0x80)
				cur = ((cur << 1) ^ polynomial) % 256;
			else
				cur = (cur << 1) % 256;
		}
		crc8_table[i] = cur;
	}
	table8_init = polynomial;
}

// https://graphics.stanford.edu/~seander/bithacks.html
uint8_t reflect8(uint8_t b) {
	return ((b * 0x0802LU & 0x22110LU) | (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16; 
}

uint8_t checksum8(uint8_t *bytes, uint32_t length, uint8_t poly, uint8_t initial, uint8_t refIn, uint8_t refOut, uint8_t xorOut) {
	uint8_t c = initial;
	uint32_t i;

	setup_crc8_table(poly);
	for (i=0; i<length; i++)
		if (refIn)
			c = crc8_table[(c ^ reflect8(bytes[i])) % 256];
		else
			c = crc8_table[(c ^ bytes[i]) % 256];

	if (refOut)
		return reflect8(c) ^ xorOut;
	else
		return c ^ xorOut;
}

void xs_checksum8(xsMachine *the) {
	int len;
	uint8_t *p;
	uint8_t initial, poly, refIn, refOut, xorOut;
	
	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype))
		p = xsmcToArrayBuffer(xsArg(0));
	len = xsmcToInteger(xsArg(1));
	poly = xsmcToInteger(xsArg(2));
	initial = xsmcToInteger(xsArg(3));
	refIn = xsmcToInteger(xsArg(4));
	refOut = xsmcToInteger(xsArg(5));
	xorOut = xsmcToInteger(xsArg(6));

	xsmcSetInteger(xsResult, checksum8(p, len, poly, initial, refIn, refOut, xorOut));
}

static void setup_crc16_table(uint16_t polynomial) {
	int i, j;
	uint16_t crc;

	if (NULL == crc16_table)
		crc16_table = (uint16_t*)c_malloc(512);
	else if (table16_init == polynomial)
		return;

	for (i=0; i<256; i++) {
		for (crc=i<<8, j=0; j<8; j++) {
			if (crc & 0x8000) {
				crc <<= 1;
				crc ^= polynomial;
			}
			else
				crc <<= 1;
		}
		crc16_table[i] = crc;
	}
	table16_init = polynomial;
}

uint16_t checksum16(uint8_t *bytes, uint32_t length, uint16_t poly, uint16_t initial, uint8_t refIn, uint8_t refOut, uint16_t xorOut) {
	int i, x;
	uint16_t crc = initial;
	uint8_t b;

	setup_crc16_table(poly);
	for (i=0; i<length; i++) {
		b = bytes[i];
		if (refIn)
			b = reflect8(b);
		crc = crc ^ (b << 8);
		crc = ((crc << 8) ^ crc16_table[crc >> 8]);
	}
	if (refOut) {
		return ((reflect8(crc & 0xff) << 8) | (reflect8(crc >> 8))) ^ xorOut;
	}
	else
		return crc ^ xorOut;
}

void xs_checksum16(xsMachine *the) {
	int len;
	uint8_t *p;
	uint8_t refIn, refOut;
	uint16_t initial, poly, xorOut;
	
	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype))
		p = xsmcToArrayBuffer(xsArg(0));
	len = xsmcToInteger(xsArg(1));
	poly = xsmcToInteger(xsArg(2));
	initial = xsmcToInteger(xsArg(3));
	refIn = xsmcToInteger(xsArg(4));
	refOut = xsmcToInteger(xsArg(5));
	xorOut = xsmcToInteger(xsArg(6));
	xsmcSetInteger(xsResult, checksum16(p, len, poly, initial, refIn, refOut, xorOut));
}


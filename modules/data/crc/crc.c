/*
 * Copyright (c) 2021-2026 Moddable Tech, Inc.
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
#include "xsHost.h"

// https://graphics.stanford.edu/~seander/bithacks.html
uint8_t reflect8(uint8_t b) {
	return ((b * 0x0802LU & 0x22110LU) | (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

static void make_crc8_table(uint8_t polynomial, uint8_t *crc8_table)
{
	int i, j, cur;

	for (i=0; i<256; i++) {
		for (cur=i, j=0; j<8; j++) {
			if (cur & 0x80)
				cur = ((cur << 1) ^ polynomial) % 256;
			else
				cur = (cur << 1) % 256;
		}
		crc8_table[i] = cur;
	}
}

static uint8_t checksum8(uint8_t *bytes, uint32_t length, uint8_t *crc8_table, uint8_t crc, uint8_t refIn) {
	uint32_t i;

	for (i=0; i<length; i++) {
		uint8_t b = bytes[i];
		if (refIn)
			b = reflect8(b);
		crc = crc8_table[(crc ^ b) % 256];
	}
	return crc;
}

typedef struct {
	uint8_t	reset;
	uint8_t	initial;
	uint8_t	reflectInput;
	uint8_t	reflectOutput;
	uint8_t	xorOutput;
	uint8_t	table[256];
} CRC8Record, *CRC8;

void xs_crc8_destructor(void *data)
{
}

void xs_crc8(xsMachine *the)
{
	int argc = xsmcArgc;
	int polynomial = xsmcToInteger(xsArg(0));
	int initial = (argc > 1) ? xsmcToInteger(xsArg(1)) : 0;
	int reflectInput = (argc > 2) ? xsmcToInteger(xsArg(2)) : 0;
	int reflectOutput = (argc > 3) ? xsmcToInteger(xsArg(3)) : 0;
	int xorOutput = (argc > 4) ? xsmcToInteger(xsArg(4)) : 0;
	CRC8 crc8 = xsmcSetHostChunk(xsThis, C_NULL, sizeof(CRC8Record));

	crc8->reset = initial;
	crc8->initial = initial;
	crc8->reflectInput = reflectInput;
	crc8->reflectOutput = reflectOutput;
	crc8->xorOutput = xorOutput;

	make_crc8_table(polynomial, crc8->table);
}

void xs_crc8_close(xsMachine *the)
{
	xsmcSetHostData(xsThis, C_NULL);
}

void xs_crc8_reset(xsMachine *the)
{
	CRC8 crc8 = xsmcGetHostChunkValidate(xsThis, xs_crc8_destructor);

	crc8->initial = crc8->reset;
	xsResult = xsThis;
}

void xs_crc8_checksum(xsMachine *the)
{
	CRC8 crc8 = xsmcGetHostChunkValidate(xsThis, xs_crc8_destructor);
	uint8_t *data;
	uint32_t length;

	xsmcGetBufferReadable(xsArg(0), (void **)&data, &length);
	crc8->initial = checksum8(data, length, crc8->table, crc8->initial, crc8->reflectInput);

	uint8_t crc = crc8->initial;
	if (crc8->reflectOutput)
		crc = reflect8(crc);

	xsmcSetInteger(xsResult, crc ^ crc8->xorOutput);
}


static void make_crc16_table(uint16_t polynomial, uint16_t *crc16_table) {
	int i, j;
	uint16_t crc;

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
}

static uint16_t checksum16(uint8_t *bytes, uint32_t length, uint16_t *crc16_table, uint16_t crc, uint8_t refIn) {
	uint32_t i;

	for (i=0; i<length; i++) {
		uint8_t b = bytes[i];
		if (refIn)
			b = reflect8(b);
		crc = crc ^ (b << 8);
		crc = ((crc << 8) ^ crc16_table[crc >> 8]);
	}
	return crc;
}

typedef struct {
	uint16_t	reset;
	uint16_t	initial;
	uint16_t	reflectInput;
	uint16_t	reflectOutput;
	uint16_t	xorOutput;
	uint16_t	table[256];
} CRC16Record, *CRC16;

void xs_crc16_destructor(void *data)
{
}

void xs_crc16(xsMachine *the)
{
	int argc = xsmcArgc;
	int polynomial = xsmcToInteger(xsArg(0));
	int initial = (argc > 1) ? xsmcToInteger(xsArg(1)) : 0;
	int reflectInput = (argc > 2) ? xsmcToInteger(xsArg(2)) : 0;
	int reflectOutput = (argc > 3) ? xsmcToInteger(xsArg(3)) : 0;
	int xorOutput = (argc > 4) ? xsmcToInteger(xsArg(4)) : 0;
	CRC16 crc16 = xsmcSetHostChunk(xsThis, C_NULL, sizeof(CRC16Record));

	crc16->reset = initial;
	crc16->initial = initial;
	crc16->reflectInput = reflectInput;
	crc16->reflectOutput = reflectOutput;
	crc16->xorOutput = xorOutput;

	make_crc16_table(polynomial, crc16->table);
}

void xs_crc16_close(xsMachine *the)
{
	xsmcSetHostData(xsThis, C_NULL);
}

void xs_crc16_reset(xsMachine *the)
{
	CRC16 crc16 = xsmcGetHostChunkValidate(xsThis, xs_crc16_destructor);

	crc16->initial = crc16->reset;
	xsResult = xsThis;
}

void xs_crc16_checksum(xsMachine *the)
{
	CRC16 crc16 = xsmcGetHostChunkValidate(xsThis, xs_crc16_destructor);
	uint8_t *data;
	uint32_t length;

	xsmcGetBufferReadable(xsArg(0), (void **)&data, &length);
	crc16->initial = checksum16(data, length, crc16->table, crc16->initial, crc16->reflectInput);

	uint16_t crc = crc16->initial; 
	if (crc16->reflectOutput)
		crc = (reflect8(crc & 0xff) << 8) | reflect8(crc >> 8);
	xsmcSetInteger(xsResult, crc ^ crc16->xorOutput);
}

static void make_crc32_table(uint32_t polynomial, uint32_t *crc32_table) {
	int i, j;
	uint32_t crc;

	for (i=0; i<256; i++) {
		for (crc=(uint32_t)i<<24, j=0; j<8; j++) {
			if (crc & 0x80000000) {
				crc <<= 1;
				crc ^= polynomial;
			}
			else
				crc <<= 1;
		}
		crc32_table[i] = crc;
	}
}

static uint32_t checksum32(uint8_t *bytes, uint32_t length, uint32_t *crc32_table, uint32_t crc, uint8_t refIn) {
	uint32_t i;

	for (i=0; i<length; i++) {
		uint8_t b = bytes[i];
		if (refIn)
			b = reflect8(b);
		crc = crc ^ ((uint32_t)b << 24);
		crc = ((crc << 8) ^ crc32_table[crc >> 24]);
	}
	return crc;
}

typedef struct {
	uint32_t	reset;
	uint32_t	initial;
	uint32_t	reflectInput;
	uint32_t	reflectOutput;
	uint32_t	xorOutput;
	uint32_t	table[256];
} CRC32Record, *CRC32;

void xs_crc32_destructor(void *data)
{
}

void xs_crc32(xsMachine *the)
{
	int argc = xsmcArgc;
	uint32_t polynomial = xsmcToUnsigned(xsArg(0));
	uint32_t initial = (argc > 1) ? xsmcToUnsigned(xsArg(1)) : 0;
	int reflectInput = (argc > 2) ? xsmcToInteger(xsArg(2)) : 0;
	int reflectOutput = (argc > 3) ? xsmcToInteger(xsArg(3)) : 0;
	uint32_t xorOutput = (argc > 4) ? xsmcToUnsigned(xsArg(4)) : 0;
	CRC32 crc32 = xsmcSetHostChunk(xsThis, C_NULL, sizeof(CRC32Record));

	crc32->reset = initial;
	crc32->initial = initial;
	crc32->reflectInput = reflectInput;
	crc32->reflectOutput = reflectOutput;
	crc32->xorOutput = xorOutput;

	make_crc32_table(polynomial, crc32->table);
}

void xs_crc32_close(xsMachine *the)
{
	xsmcSetHostData(xsThis, C_NULL);
}

void xs_crc32_reset(xsMachine *the)
{
	CRC32 crc32 = xsmcGetHostChunkValidate(xsThis, xs_crc32_destructor);

	crc32->initial = crc32->reset;
	xsResult = xsThis;
}

void xs_crc32_checksum(xsMachine *the)
{
	CRC32 crc32 = xsmcGetHostChunkValidate(xsThis, xs_crc32_destructor);
	uint8_t *data;
	uint32_t length;

	xsmcGetBufferReadable(xsArg(0), (void **)&data, &length);
	crc32->initial = checksum32(data, length, crc32->table, crc32->initial, crc32->reflectInput);

	uint32_t crc = crc32->initial;
	if (crc32->reflectOutput)
		crc = ((uint32_t)reflect8(crc & 0xff) << 24) | ((uint32_t)reflect8((crc >> 8) & 0xff) << 16) | ((uint32_t)reflect8((crc >> 16) & 0xff) << 8) | reflect8(crc >> 24);
	xsmcSetUnsigned(xsResult, crc ^ crc32->xorOutput);
}

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

#include "xsmc.h"
#include "xsesp.h"
#include "mc.xs.h"			// for xsID_ values

#include "spi_flash.h"
#include "user_interface.h"

//static const int FLASH_INT_MASK = ((B10 << 8) | B00111010);
static const int FLASH_INT_MASK = ((2 << 8) | 0x3A);

extern uint8_t _MODPREF_start;
extern uint8_t _MODPREF_end;

#define kPreferencesMagic 0x81213141

enum {
	kPrefsTypeBoolean = 1,
	kPrefsTypeInteger = 2,
	kPrefsTypeString = 3,
	kPrefsTypeBuffer = 4,
};

#define kBufferSize (64)

static void resetPrefs(void);
static uint8_t findPrefsBlock(uint32_t *offset);
static uint8_t findPrefOffset(const char *domain, const char *key, uint32_t *entryOffset, uint32_t *valueOffset, uint32_t *entrySize, uint8_t *buffer);
static int getPrefSize(const uint8_t *pref);
static uint8_t erasePref(const char *domain, const char *key, uint8_t *buffer);
static uint8_t setPref(xsMachine *the, char *domain, char *name, uint8_t type, uint8_t *value, uint16_t byteCount);

void xs_preference_set(xsMachine *the)
{
	uint8_t success;
	uint8_t boolean;
	int32_t integer;
	double dbl;
	char *str;

	if ((c_strlen(xsmcToString(xsArg(0))) > 31) || (c_strlen(xsmcToString(xsArg(1))) > 31))
		xsUnknownError("too long");

	switch (xsmcTypeOf(xsArg(2))) {
		case xsBooleanType:
			boolean = xsmcToBoolean(xsArg(2));
			success = setPref(the, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeBoolean, (uint8_t *)&boolean, 1);
			break;

		case xsIntegerType:
			integer = xsmcToInteger(xsArg(2));
			success = setPref(the, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeInteger, (uint8_t *)&integer, sizeof(integer));
			break;

		case xsNumberType:
			dbl = xsmcToNumber(xsArg(2));
			integer = (int32_t)dbl;
			if (dbl != integer)
				xsUnknownError("float unsupported");
			success = setPref(the, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeInteger, (uint8_t *)&integer, sizeof(integer));
			break;

		case xsStringType:
			str = xsmcToString(xsArg(2));
			success = setPref(the, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeString, (uint8_t *)str, c_strlen(str) + 1);
			break;

		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype))
				success = setPref(the, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeBuffer, xsmcToArrayBuffer(xsArg(2)), xsGetArrayBufferLength(xsArg(2)));
			else
				goto unknown;
			break;

		unknown:
		default:
			xsUnknownError("unsupported type");
	}

	if (!success)
		xsUnknownError("can't save prefs");
}

void xs_preference_get(xsMachine *the)
{
	uint32_t entryOffset, valueOffset, entrySize;
	uint8_t buffer[kBufferSize] __attribute__((aligned(4)));
	uint8_t *pref = buffer;

	if (!findPrefOffset(xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), &entryOffset, &valueOffset, &entrySize, buffer))
		return;

	switch (*pref++) {
		case kPrefsTypeBoolean:
			xsmcSetBoolean(xsResult, *pref);
			break;
		case kPrefsTypeInteger:
			xsmcSetInteger(xsResult, c_read32(pref));
			break;
		case kPrefsTypeString:
			xsmcSetString(xsResult, pref);
			break;
		case kPrefsTypeBuffer:
			xsmcSetArrayBuffer(xsResult, pref + 2, c_read16(pref));
			break;
	}
}

void xs_preference_delete(xsMachine *the)
{
	uint8_t buffer[kBufferSize] __attribute__((aligned(4)));
	uint8_t success = erasePref(xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), buffer);

	if (!success)
		xsUnknownError("can't save prefs");
}

void xs_preference_keys(xsMachine *the)
{
	uint32_t offset, endOffset;
	uint8_t buffer[kBufferSize] __attribute__((aligned(4)));

	xsResult = xsNewArray(0);

	if (!findPrefsBlock(&offset))
		return;

	xsmcVars(1);

	endOffset = offset + SPI_FLASH_SEC_SIZE;
	offset += sizeof(uint32_t);	// skip signature

	while (offset < endOffset) {
		uint8_t *b = buffer;
		const char *domain;
		uint8_t match;
		uint32 use = endOffset - offset;
		if (use > kBufferSize)
			use = kBufferSize;

		spi_flash_read(offset + (uint8_t *)&_MODPREF_start - kFlashStart, (uint32 *)buffer, use);
		while (use) {
			if (0xff == *b)		// uninitialized
				return;

			if (*b)
				break; 				// found something

			b += 1;					// erased, keep looking
			use -= 1;
			offset += 1;
		}
		if (0 == use) continue;		// end of buffer with nothing

		spi_flash_read(offset + (uint8_t *)&_MODPREF_start - kFlashStart, (uint32 *)buffer, kBufferSize);
		domain = xsmcToString(xsArg(0));
		match = 0 == c_strcmp(buffer, domain);
		offset += c_strlen(buffer) + 1;

		spi_flash_read(offset + (uint8_t *)&_MODPREF_start - kFlashStart, (uint32 *)buffer, kBufferSize);
		if (match) {
			xsVar(0) = xsString(buffer);
			xsCall1(xsResult, xsID_push, xsVar(0));
		}
		offset += c_strlen(buffer) + 1;

		spi_flash_read(offset + (uint8_t *)&_MODPREF_start - kFlashStart, (uint32 *)buffer, kBufferSize);
		offset += getPrefSize(buffer);
		offset = (offset + 3) & ~3;			// round to a long
	}
}

void xs_preference_reset(xsMachine *the)
{
	resetPrefs();
}

void resetPrefs(void)
{
	uint32_t magic = kPreferencesMagic;
	uint32_t sector = ((uint8_t *)&_MODPREF_start - kFlashStart) / SPI_FLASH_SEC_SIZE;

	ets_isr_mask(FLASH_INT_MASK);
	spi_flash_erase_sector(sector);
	spi_flash_erase_sector(sector + 1);
	ets_isr_unmask(FLASH_INT_MASK);

	spi_flash_write(0 + &_MODPREF_start - kFlashStart, (uint32 *)&magic, sizeof(magic));
}

uint8_t findPrefsBlock(uint32_t *offset)
{
	uint32_t magic;

	*offset = 0;
	spi_flash_read(0 + (uint8_t *)&_MODPREF_start - kFlashStart, &magic, sizeof(magic));
	if (kPreferencesMagic == magic)
		return 1;

	*offset = SPI_FLASH_SEC_SIZE;
	spi_flash_read(SPI_FLASH_SEC_SIZE + (uint8_t *)&_MODPREF_start - kFlashStart, &magic, sizeof(magic));
	if (kPreferencesMagic == magic)
		return 1;

	resetPrefs();

	return 0;
}

uint8_t findPrefOffset(const char *domain, const char *key, uint32_t *entryOffset, uint32_t *valueOffset, uint32_t *entrySize, uint8_t *buffer)
{
	uint32_t offset, endOffset;

	if (!findPrefsBlock(&offset)) {
		if (domain && key)
			return 0;
	}

	endOffset = offset + SPI_FLASH_SEC_SIZE;
	offset += sizeof(uint32_t); // skip signature
	while (offset < endOffset) {
		uint8_t *b = buffer;
		uint8_t match;
		uint32_t valueSize;
		uint32 use = endOffset - offset;
		if (use > kBufferSize)
			use = kBufferSize;

		spi_flash_read(offset + (uint8_t *)&_MODPREF_start - kFlashStart, (uint32 *)buffer, use);
		while (use) {
			if (0xff == *b)	{		// uninitialized
				if (!domain && !key) {
					*entryOffset = offset;
					return 1;
				}
				return 0;
			}

			if (*b)
				break; 				// found something

			b += 1;					// erased, keep looking
			use -= 1;
			offset += 1;
		}
		if (0 == use) continue;		// end of buffer with nothing

		*entryOffset = offset;
		spi_flash_read(offset + (uint8_t *)&_MODPREF_start - kFlashStart, (uint32 *)buffer, kBufferSize);
		match = domain && (0 == c_strcmp(buffer, domain));
		offset += c_strlen(buffer) + 1;

		spi_flash_read(offset + (uint8_t *)&_MODPREF_start - kFlashStart, (uint32 *)buffer, kBufferSize);
		if (match)
			match = 0 == c_strcmp(buffer, key);
		offset += c_strlen(buffer) + 1;
		*valueOffset = offset;

		spi_flash_read(offset + (uint8_t *)&_MODPREF_start - kFlashStart, (uint32 *)buffer, kBufferSize);
		valueSize = getPrefSize(buffer);
		offset += valueSize;
		offset = (offset + 3) & ~3;			// round to a long
		*entrySize = offset - *entryOffset;
		if (match)
			return 1;
	}

	if (!domain && !key) {
		*entryOffset = offset - 1;
		return 1;
	}

	return 0;
}

uint8_t erasePref(const char *domain, const char *key, uint8_t *buffer)
{
	uint32_t entryOffset, valueOffset, entrySize, offset;

	if (!findPrefOffset(domain, key, &entryOffset, &valueOffset, &entrySize, buffer))
		return 1;

	c_memset(buffer, 0, kBufferSize);
	offset = entryOffset;
	while (entrySize) {
		int use = (entrySize > kBufferSize) ? kBufferSize : entrySize;
		if (SPI_FLASH_RESULT_OK != spi_flash_write(offset + &_MODPREF_start - kFlashStart, (uint32 *)buffer, use))
			return 0;
		offset += use;
		entrySize -= use;
	}

	return 1;
}

uint8_t setPref(xsMachine *the, char *domain, char *key, uint8_t type, uint8_t *value, uint16_t byteCount)
{
	uint8_t buffer[kBufferSize * 2] __attribute__((aligned(4))), *pref = buffer;
	uint32_t prefSize, prefsEnd, valueOffset, entrySize, prefsFree;

	prefSize = (c_strlen(domain) + 1) + (c_strlen(key) + 1) + 1 + byteCount + ((kPrefsTypeBuffer == type) ? 2 : 0);
	prefSize = (prefSize + 3) & ~3;
	if ((prefSize > sizeof(buffer)) || (byteCount > 63))
		return 0;

	if (!erasePref(domain, key, buffer))
		return 0;

	if (!findPrefOffset(NULL, NULL, &prefsEnd, &valueOffset, &entrySize, buffer))
		return 0;

	prefsFree = SPI_FLASH_SEC_SIZE - (prefsEnd & (SPI_FLASH_SEC_SIZE - 1));
	if (prefsFree < prefSize) { // compact
		uint32_t srcOffset = (prefsEnd < SPI_FLASH_SEC_SIZE) ? 0 : SPI_FLASH_SEC_SIZE;
		uint32_t dstOffset = (prefsEnd < SPI_FLASH_SEC_SIZE) ? SPI_FLASH_SEC_SIZE : 0;
		uint32_t srcOffsetSave = srcOffset;

		if (SPI_FLASH_RESULT_OK != spi_flash_erase_sector((dstOffset + &_MODPREF_start - kFlashStart) / SPI_FLASH_SEC_SIZE))
			return 0;

		*(uint32_t *)buffer = kPreferencesMagic;
		spi_flash_write(dstOffset + &_MODPREF_start - kFlashStart, (uint32 *)buffer, sizeof(uint32_t));
		dstOffset += sizeof(uint32_t);
		srcOffset += sizeof(uint32_t);

		while (srcOffset < prefsEnd) {
			uint8_t *b = buffer;
			uint32_t entrySize;
			uint32_t use = prefsEnd - srcOffset;
			if (use > sizeof(buffer))
				use = sizeof(buffer);

			spi_flash_read(srcOffset + &_MODPREF_start - kFlashStart, (uint32 *)buffer, use);
			while (use--) {
				if (0xff == *b)
					break;

				if (0 == *b) {
					srcOffset += 1;
					b += 1;
					continue;
				}

				spi_flash_read(srcOffset + &_MODPREF_start - kFlashStart, (uint32 *)buffer, sizeof(buffer));		// will always hold a full preference record
				b = buffer;
				entrySize = c_strlen(b) + 1;
				b += entrySize;
				entrySize += c_strlen(b) + 1;
				entrySize += getPrefSize(buffer + entrySize);
				entrySize = (entrySize + 3) & ~3;
				spi_flash_write(dstOffset + &_MODPREF_start - kFlashStart, (uint32 *)buffer, entrySize);
				dstOffset += entrySize;
				srcOffset += entrySize;
				break;
			}
		}

		*(uint32_t *)buffer = 0;	// invalidated previous block
		spi_flash_write(srcOffsetSave + &_MODPREF_start - kFlashStart, (uint32 *)buffer, sizeof(uint32_t));

		if (!findPrefOffset(NULL, NULL, &prefsEnd, &valueOffset, &entrySize, buffer))
			return 0;

		prefsFree = SPI_FLASH_SEC_SIZE - (prefsEnd & (SPI_FLASH_SEC_SIZE - 1));
		if (prefsFree < prefSize)
			return 0;		// not enough space
	}

	// write preference at the end
	c_memset(buffer, 0, prefSize);
	c_strcpy(pref, domain);
	pref += c_strlen(domain) + 1;
	c_strcpy(pref, key);
	pref += c_strlen(key) + 1;
	*pref++ = type;
	if (kPrefsTypeBuffer == type) {
		pref[0] = (uint8_t)byteCount;
		pref[1] = (uint8_t)(byteCount >> 8);
		pref += 2;
	}
	c_memcpy(pref, value, byteCount);

	return (SPI_FLASH_RESULT_OK == spi_flash_write(prefsEnd + &_MODPREF_start - kFlashStart, (uint32 *)buffer, prefSize)) ? 1 : 0;
}

int getPrefSize(const uint8_t *pref)
{
	switch (*(uint8_t *)pref) {
		case kPrefsTypeBoolean: return 1 + 1;
		case kPrefsTypeInteger: return 1 + 4;
		case kPrefsTypeString:  return 1 + c_strlen(pref + 1) + 1;
		case kPrefsTypeBuffer:  return 1 + 2 + c_read16(pref + 1);
	}

	return SPI_FLASH_SEC_SIZE;		// force to skip to end of preferences
}

/*

// ONE SPI BLOCK (4 KB)... no more. no less.

SIGNATURE
[
    (long word align)
	domain
	key
	type (byte) - boolean, integer, string, arraybuffer
	value (for ArrayBuffer, preceeded by 16-bit count)
]

*/

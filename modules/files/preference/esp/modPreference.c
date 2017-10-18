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

#include "spi_flash.h"
#include "user_interface.h"

//static const int FLASH_INT_MASK = ((B10 << 8) | B00111010);
static const int FLASH_INT_MASK = ((2 << 8) | 0x3A);

extern uint8_t _MODPREF_start;
extern uint8_t _MODPREF_end;

#define kPreferencesMagic 0x80121314

enum {
	kPrefsTypeBoolean = 1,
	kPrefsTypeInteger = 2,
	kPrefsTypeString = 3,
	kPrefsTypeBuffer = 4,
};

static uint8_t *loadPrefs(void);
static uint8_t *findPref(uint8_t *prefs, const char *domain, const char *key);
static uint8_t savePrefs(uint8_t *prefs);		// 0 = failure, 1 = success
static uint8_t setPref(uint8_t *buffer, char *domain, char *name, uint8_t type, uint8_t *value, uint16_t byteCount);
static int getPrefSize(uint8_t *pref);

void xs_preference_set(xsMachine *the)
{
	uint8_t *prefs = loadPrefs();
	uint8_t success;
	uint8_t boolean;
	int32_t integer;
	double dbl;
	char *str;

	if (!prefs)
		xsUnknownError("can't load prefs");

	switch (xsmcTypeOf(xsArg(2))) {
		case xsBooleanType:
			boolean = xsmcToBoolean(xsArg(2));
			success = setPref(prefs, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeBoolean, (uint8_t *)&boolean, 1);
			break;

		case xsIntegerType:
			integer = xsmcToInteger(xsArg(2));
			success = setPref(prefs, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeInteger, (uint8_t *)&integer, sizeof(integer));
			break;

		case xsNumberType:
			dbl = xsmcToNumber(xsArg(2));
			if (dbl != (int)dbl) {
				c_free(prefs);
				xsUnknownError("float unsupported");
			}
			success = setPref(prefs, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeInteger, (uint8_t *)&integer, sizeof(integer));
			break;

		case xsStringType:
			str = xsmcToString(xsArg(2));
			success = setPref(prefs, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeString, (uint8_t *)str, c_strlen(str) + 1);
			break;

		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype))
				success = setPref(prefs, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeBuffer, xsmcToArrayBuffer(xsArg(2)), xsGetArrayBufferLength(xsArg(2)));
			else
				goto unknown;
			break;

		unknown:
		default:
			c_free(prefs);
			xsUnknownError("unsupported type");
	}


	c_free(prefs);

	if (!success)
		xsUnknownError("can't save prefs");
}

void xs_preference_get(xsMachine *the)
{
	uint8_t *prefs = loadPrefs();
	uint8_t *pref;
	uint8_t success;

	if (!prefs)
		xsUnknownError("can't load prefs");

	pref = findPref(prefs, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)));
	if (NULL == pref) {
		c_free(prefs);
		return;
	}

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
			xsResult = xsArrayBuffer(pref + 2, c_read16(pref));
			break;
	}

	c_free(prefs);
}

void xs_preference_delete(xsMachine *the)
{
	uint8_t *prefs = loadPrefs();
	uint8_t success;

	if (!prefs)
		xsUnknownError("can't load prefs");

	success = setPref(prefs, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), 0, NULL, 0);

	c_free(prefs);

	if (!success)
		xsUnknownError("can't save prefs");
}

/*
void xs_preference_reset(xsMachine *the)
{
	ets_isr_mask(FLASH_INT_MASK);
	spi_flash_erase_sector(((uint8_t *)&_MODPREF_start - kFlashStart) / SPI_FLASH_SEC_SIZE);
	ets_isr_unmask(FLASH_INT_MASK);
}
*/

uint8_t *loadPrefs(void)
{
	SpiFlashOpResult result;
	uint8_t *prefs;

	prefs = c_malloc(SPI_FLASH_SEC_SIZE);
	if (!prefs) return NULL;

	ets_isr_mask(FLASH_INT_MASK);
	result = spi_flash_read((uint8_t *)&_MODPREF_start - kFlashStart, (uint32 *)prefs, SPI_FLASH_SEC_SIZE);
	ets_isr_unmask(FLASH_INT_MASK);

	if (SPI_FLASH_RESULT_OK != result) {
		c_free(prefs);
		return NULL;
	}

	if (kPreferencesMagic != *(uint32_t *)prefs) {
		c_memset(prefs, 0xFF, SPI_FLASH_SEC_SIZE);
		*(uint32_t *)prefs = 0;		// erase needed
	}

	return prefs;
}

// returns pointer to data type byte
// pass NULL for domain and key to return a pointer to start of free space
uint8_t *findPref(uint8_t *prefs, const char *domain, const char *key)
{
	uint8_t *prefsStart = prefs;
	uint8_t *prefsEnd = prefs + (&_MODPREF_end - &_MODPREF_start);

	prefs += sizeof(uint32_t);	// skip signature
	while (prefs < prefsEnd) {
		uint8_t match = 0;

		if (0xff == *prefs)		// uninitialized
			break;

		if (0 == *prefs) {		// erased
			prefs += 1;
			continue;
		}

		if (domain)
			match = 0 == c_strcmp(prefs, domain);
		prefs += c_strlen(prefs) + 1;
		if (key && match)
			match = 0 == c_strcmp(prefs, key);
		prefs += c_strlen(prefs) + 1;
		if (match)
			return prefs;

		switch (*prefs++) {
			case kPrefsTypeBoolean: prefs += 1; break;
			case kPrefsTypeInteger: prefs += 4; break;
			case kPrefsTypeString: prefs += c_strlen(prefs) + 1; break;
			case kPrefsTypeBuffer: prefs += 2 + c_read16(prefs); break;
			default: {
				modLog("corrupt");
				c_memset(prefs, 0xFF, SPI_FLASH_SEC_SIZE);
				*(uint32_t *)prefsStart = 0;		// invalid so it flushes
				return NULL;		// corrupt
				}
		}
	}

	return (!domain && !key) ? prefs : NULL;
}

uint8_t savePrefs(uint8_t *prefs)
{
	SpiFlashOpResult result;

	if (0 == *(uint32_t *)prefs) {
		ets_isr_mask(FLASH_INT_MASK);
		result = spi_flash_erase_sector((&_MODPREF_start - kFlashStart) / SPI_FLASH_SEC_SIZE);
		ets_isr_unmask(FLASH_INT_MASK);
		if (SPI_FLASH_RESULT_OK != result)
			return 0;

		*(uint32_t *)prefs = kPreferencesMagic;	// buffer written
	}

	ets_isr_mask(FLASH_INT_MASK);
	result = spi_flash_write(&_MODPREF_start - kFlashStart, (uint32 *)prefs, SPI_FLASH_SEC_SIZE);
	ets_isr_unmask(FLASH_INT_MASK);

	if (SPI_FLASH_RESULT_OK != result)
		return 0;

	return 1;
}

// pass NULL for value to remove property
uint8_t setPref(uint8_t *prefs, char *domain, char *key, uint8_t type, uint8_t *value, uint16_t byteCount)
{
	uint8_t *prevPref = findPref(prefs, domain, key);
	int prefSize = (c_strlen(domain) + 1) + (c_strlen(key) + 1) + 1 + byteCount + ((kPrefsTypeBuffer == type) ? 2 : 0);
	uint8_t *prefsEnd = findPref(prefs, NULL, NULL);
	int prefsFree;

	if (NULL == prefsEnd) {
		savePrefs(prefs);		// likely corrupt. flush
		return 0;
	}

	if (prevPref) {
		prevPref -= (c_strlen(domain) + 1) + (c_strlen(key) + 1);
		c_memset(prevPref, 0, getPrefSize(prevPref));		// erase previous value
	}

	if (NULL == value)						// no new value, just erase
		return prevPref ? savePrefs(prefs) : 1;

	prefsFree = SPI_FLASH_SEC_SIZE - (prefsEnd - prefs);
	if (prefsFree < prefSize) {
		// compact to make room
		uint8_t *from = prefs, *to = prefs;

		while (from < prefsEnd) {
			int thisSize;

			if (0xff == *from)
				break;

			if (0 == *from) {
				from++;
				continue;
			}

			thisSize = getPrefSize(from);
			while (thisSize--)
				*to++ = *from++;
		}

		c_memset(to, 0xff, SPI_FLASH_SEC_SIZE - (to - prefs));

		// flag that erase is needed
		*(uint32_t *)prefs = 0;

		// make sure there's enough space now
		prefsEnd = findPref(prefs, NULL, NULL);
		prefsFree = SPI_FLASH_SEC_SIZE - (prefsEnd - prefs);
		if (prefsFree < prefSize) {
			modLog("prefs full");
			return 0;
		}
	}

	// write preference at the end
	c_strcpy(prefsEnd, domain);
	prefsEnd += c_strlen(domain) + 1;
	c_strcpy(prefsEnd, key);
	prefsEnd += c_strlen(key) + 1;
	*prefsEnd++ = type;
	if (kPrefsTypeBuffer == type) {
		prefsEnd[0] = (uint8_t)byteCount;
		prefsEnd[1] = (uint8_t)(byteCount >> 8);
		prefsEnd += 2;
	}
	c_memcpy(prefsEnd, value, byteCount);

	return savePrefs(prefs);
}

int getPrefSize(uint8_t *pref)
{
	int thisSize = c_strlen(pref) + 1;
	thisSize += c_strlen(pref + thisSize) + 1;
	switch (*(uint8_t *)(pref + thisSize++)) {
		case kPrefsTypeBoolean: thisSize += 1; break;
		case kPrefsTypeInteger: thisSize += 4; break;
		case kPrefsTypeString: thisSize += c_strlen(pref + thisSize) + 1; break;
		case kPrefsTypeBuffer: thisSize += 2 + c_read16(pref + thisSize); break;
		default: modLog("corrupt getPrefSize"); break;
	}

	return thisSize;
}

/*

// ONE SPI BLOCK (4 KB)... no more. no less.

SIGNATURE
[
	domain
	key
	type (byte) - boolean, integer, string, arraybuffer
	value (for ArrayBuffer, preceeded by 16-bit count)
]

*/

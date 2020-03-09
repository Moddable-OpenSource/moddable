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

#include "xsmc.h"
#include "mc.xs.h"
#include "xsHost.h"

#include "qapi_persist.h"

#define kPreferencesMagic 0x81213141

enum {
	kPrefsTypeBoolean = 1,
	kPrefsTypeInteger = 2,
	kPrefsTypeString = 3,
	kPrefsTypeBuffer = 4
};

#define kBufferSize (64)
#define kPrefsRoot "/spinor/moddable/prefs/"
#define kPrefsExt ".bin"

/**
	Preferences file format:
	
	kPrefsMagic
	type (byte) - boolean, integer, string, arraybuffer
	value (for ArrayBuffer, preceeded by 16-bit count)
**/

static uint8_t setPref(const char *domain, const char *key, uint8_t type, uint8_t *value, uint16_t byteCount);
static qapi_Status_t initPref(const char *domain, const char *key, qapi_Persist_Handle_t *Handle);
static void closePref(qapi_Persist_Handle_t Handle, uint8_t *Data);

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
			success = setPref(xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeBoolean, (uint8_t *)&boolean, 1);
			break;

		case xsIntegerType:
			integer = xsmcToInteger(xsArg(2));
			success = setPref(xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeInteger, (uint8_t *)&integer, sizeof(integer));
			break;

		case xsNumberType:
			dbl = xsmcToNumber(xsArg(2));
			integer = (int32_t)dbl;
			if (dbl != integer)
				xsUnknownError("float unsupported");
			success = setPref(xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeInteger, (uint8_t *)&integer, sizeof(integer));
			break;

		case xsStringType:
			str = xsmcToString(xsArg(2));
			success = setPref(xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeString, (uint8_t *)str, c_strlen(str) + 1);
			break;

		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype))
				success = setPref(xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeBuffer, xsmcToArrayBuffer(xsArg(2)), xsmcGetArrayBufferLength(xsArg(2)));
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
	const char *domain = xsmcToString(xsArg(0));
	const char *key = xsmcToString(xsArg(1));
	uint32_t DataLength;
	uint8_t *pref, *Data = NULL;
	qapi_Persist_Handle_t Handle = 0;
	qapi_Status_t result;

	result = initPref(domain, key, &Handle);
	if (QAPI_OK != result)
		goto bail;
		
	result = qapi_Persist_Get(Handle, &DataLength, &Data);
	if (QAPI_OK != result) {
		result = QAPI_OK;
		return;	// pref likely doesn't exist yet
	}
		
	pref = Data;
	if (kPreferencesMagic != c_read32(pref)) {
		result = -1;
		goto bail;
	}
		
	pref += sizeof(kPreferencesMagic);
	
	switch (*pref++) {
		case kPrefsTypeBoolean:
			xsmcSetBoolean(xsResult, *pref);
			break;
		case kPrefsTypeInteger:
			xsmcSetInteger(xsResult, c_read32(pref));
			break;
		case kPrefsTypeString:
			xsmcSetString(xsResult, (char*)pref);
			break;
		case kPrefsTypeBuffer:
			xsmcSetArrayBuffer(xsResult, pref + 2, c_read16(pref));
			break;
	}
	
bail:
	closePref(Handle, Data);
	if (QAPI_OK != result)
		xsUnknownError("can't read pref");
}

void xs_preference_delete(xsMachine *the)
{
	const char *domain = xsmcToString(xsArg(0));
	const char *key = xsmcToString(xsArg(1));
	qapi_Persist_Handle_t Handle = 0;
	qapi_Status_t result;

	result = initPref(domain, key, &Handle);
	if (QAPI_OK != result)
		goto bail;

	qapi_Persist_Delete(Handle);
	
bail:
	closePref(Handle, NULL);
}

void xs_preference_keys(xsMachine *the)
{
	xsUnknownError("unimplemented");
}

uint8_t setPref(const char *domain, const char *key, uint8_t type, uint8_t *value, uint16_t byteCount)
{
	uint32_t prefSize;
	uint8_t buffer[kBufferSize * 2];
	uint8_t *pref = buffer;
	qapi_Status_t result;
	qapi_Persist_Handle_t Handle = 0;
	
	if (byteCount > 63)
		return 0;
		
	result = initPref(domain, key, &Handle);
	if (QAPI_OK != result)
		goto bail;

	// build pref entry
	prefSize = sizeof(kPreferencesMagic) + 1 + byteCount + ((kPrefsTypeBuffer == type) ? 2 : 0);
	if (prefSize > sizeof(buffer)) {
		result = QAPI_ERR_INVALID_PARAM;
		goto bail;
	}
	c_memset(buffer, 0, prefSize);
	pref[0] = (uint8_t)(kPreferencesMagic & 0xFF);
	pref[1] = (uint8_t)((kPreferencesMagic >> 8) & 0xFF);
	pref[2] = (uint8_t)((kPreferencesMagic >> 16) & 0xFF);
	pref[3] = (uint8_t)((kPreferencesMagic >> 24) & 0xFF);
	pref += sizeof(kPreferencesMagic);
	*pref++ = type;
	if (kPrefsTypeBuffer == type) {
		pref[0] = (uint8_t)byteCount;
		pref[1] = (uint8_t)(byteCount >> 8);
		pref += 2;
	}
	c_memcpy(pref, value, byteCount);
	
	// write pref
	result = qapi_Persist_Put(Handle, prefSize, buffer);
	if (QAPI_OK != result)
		goto bail;
			
bail:
	closePref(Handle, NULL);
	return (QAPI_OK == result ? 1 : 0);
}

qapi_Status_t initPref(const char *domain, const char *key, qapi_Persist_Handle_t *Handle)
{
	char namePrefix[64];
	char *p = namePrefix;
	uint16_t i, length = c_strlen(key);
		
	c_strcpy(p, domain);
	p += c_strlen(domain);
	*p++ = '_';
	for (i = 0; i < length; ++i) {
		char c = key[i];
		if ('\\' == c)
			*p++ = '-';
		else
			*p++ = c;
	}
	*p = 0;

	return qapi_Persist_Initialize(Handle, kPrefsRoot, namePrefix, kPrefsExt, NULL, 0);
}

void closePref(qapi_Persist_Handle_t Handle, uint8_t *Data)
{
	if (0 != Handle) {
		if (NULL != Data)
			qapi_Persist_Free(Handle, Data);
		qapi_Persist_Cleanup(Handle);
	}
}



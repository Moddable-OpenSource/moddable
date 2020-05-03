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
#include "mc.defines.h"
#include "mc.xs.h"			// for xsID_ values

#define kKeyRoot "SOFTWARE\\moddable.tech\\"

enum {
	kPrefsTypeBoolean = 0,
	kPrefsTypeInteger,
	kPrefsTypeNumber,
	kPrefsTypeString,
	kPrefsTypeBuffer
};

static DWORD setPref(xsMachine *the, char *domain, char *name, uint8_t type, uint8_t *value, uint16_t byteCount);

void xs_preference_delete(xsMachine *the)
{
	char szSubKey[1024];
	c_strcpy(szSubKey, kKeyRoot);
	c_strcat(szSubKey, PIU_DOT_SIGNATURE);
	c_strcat(szSubKey, "\\");
	c_strcat(szSubKey, xsmcToString(xsArg(0)));
	RegDeleteKeyValue(HKEY_CURRENT_USER, szSubKey, xsmcToString(xsArg(1)));
}

void xs_preference_get(xsMachine *the)
{
	char szSubKey[1024];
	HKEY hKey = NULL;
	uint8_t *data = NULL;
	char *name = xsmcToString(xsArg(1));
	DWORD dwResult, size;
	c_strcpy(szSubKey, kKeyRoot);
	c_strcat(szSubKey, PIU_DOT_SIGNATURE);
	c_strcat(szSubKey, "\\");
	c_strcat(szSubKey, xsmcToString(xsArg(0)));

	dwResult = RegOpenKeyEx(HKEY_CURRENT_USER, szSubKey, 0, KEY_READ, &hKey);
	if (ERROR_SUCCESS != dwResult) goto bail;

	dwResult = RegQueryValueEx(hKey, name, 0, NULL, NULL, &size);
	if (ERROR_SUCCESS != dwResult) goto bail;

	data = c_malloc(size);
	if (!data) goto bail;

	dwResult = RegQueryValueEx(hKey, name, 0, NULL, data, &size);
	if (ERROR_SUCCESS != dwResult) goto bail;

	uint8_t type = data[0];
	uint8_t *p = data + 1;
	switch (type) {
		case kPrefsTypeBoolean: {
			xsBooleanValue b;
			c_memmove(&b, p, sizeof(xsBooleanValue));
			xsResult = xsBoolean(b);
		} break;
		case kPrefsTypeInteger: {
			xsIntegerValue i;
			c_memmove(&i, p, sizeof(xsIntegerValue));
			xsResult = xsInteger(i);
		} break;
		case kPrefsTypeNumber: {
			xsNumberValue n;
			c_memmove(&n, p, sizeof(xsNumberValue));
			xsResult = xsNumber(n);
		} break;
		case kPrefsTypeString: {
			xsResult = xsString(p);
		} break;
		case kPrefsTypeBuffer: {
			xsmcSetArrayBuffer(xsResult, p, size - 1);
		} break;
	}

bail:
	if (data)
		c_free(data);
	if (hKey)
		RegCloseKey(hKey);
}

void xs_preference_set(xsMachine *the)
{
	DWORD dwResult;

	switch (xsmcTypeOf(xsArg(2))) {
		case xsBooleanType: {
			xsBooleanValue b = xsmcToBoolean(xsArg(2));
			dwResult = setPref(the, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeBoolean, (uint8_t*)&b, sizeof(xsBooleanValue));
		} break;
		case xsIntegerType: {
			xsIntegerValue i = xsmcToInteger(xsArg(2));
			dwResult = setPref(the, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeInteger, (uint8_t*)&i, sizeof(xsIntegerValue));
		} break;
		case xsNumberType: {
			xsNumberValue n = xsmcToNumber(xsArg(2));
			dwResult = setPref(the, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeNumber, (uint8_t*)&n, sizeof(xsNumberValue));
		} break;
		case xsStringType: {
			xsStringValue s = xsmcToString(xsArg(2));
			dwResult = setPref(the, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeString, (uint8_t*)s, (uint16_t)(c_strlen(s) + 1));
		} break;
		case xsReferenceType: {
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype))
				dwResult = setPref(the, xsmcToString(xsArg(0)), xsmcToString(xsArg(1)), kPrefsTypeBuffer, xsmcToArrayBuffer(xsArg(2)), xsmcGetArrayBufferLength(xsArg(2)));
			else
				goto unknown;
		} break;

		unknown:
		default:
			xsUnknownError("unsupported type");
	}

	if (ERROR_SUCCESS != dwResult)
		xsUnknownError("can't save prefs");
}

void xs_preference_keys(xsMachine *the)
{
	char szSubKey[1024], szName[64];
	DWORD dwResult, dwIndex = 0, dwSize = sizeof(szName);
	HKEY hKey = NULL;

	xsResult = xsNewArray(0);

	c_strcpy(szSubKey, kKeyRoot);
	c_strcat(szSubKey, PIU_DOT_SIGNATURE);
	c_strcat(szSubKey, "\\");
	c_strcat(szSubKey, xsmcToString(xsArg(0)));

	dwResult = RegOpenKeyEx(HKEY_CURRENT_USER, szSubKey, 0, KEY_QUERY_VALUE, &hKey);
	if (ERROR_SUCCESS != dwResult) goto bail;

	xsmcVars(1);
	dwResult = RegEnumValue(hKey, dwIndex++, szName, &dwSize, NULL, NULL, NULL, NULL);
	while (ERROR_SUCCESS == dwResult) {
		xsVar(0) = xsString(szName);
		xsCall1(xsResult, xsID_push, xsVar(0));
		dwSize = sizeof(szName);
		dwResult = RegEnumValue(hKey, dwIndex++, szName, &dwSize, NULL, NULL, NULL, NULL);
	}

bail:
	if (hKey)
		RegCloseKey(hKey);
}

static DWORD setPref(xsMachine *the, char *domain, char *name, uint8_t type, uint8_t *value, uint16_t byteCount)
{
	char szSubKey[1024];
	HKEY hKey = NULL;
	uint8_t *data = NULL;
	DWORD dwResult;
	c_strcpy(szSubKey, kKeyRoot);
	c_strcat(szSubKey, PIU_DOT_SIGNATURE);
	c_strcat(szSubKey, "\\");
	c_strcat(szSubKey, domain);

	dwResult = RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
	if (ERROR_SUCCESS != dwResult) goto bail;

	data = c_malloc(byteCount + 1);
	if (!data) {
		dwResult = 1;
		goto bail;
	}
	data[0] = type;
	c_memmove(&data[1], value, byteCount);
	dwResult = RegSetValueEx(hKey, name, 0, REG_BINARY, data, byteCount + 1);

bail:
	if (data)
		c_free(data);
	if (hKey)
		RegCloseKey(hKey);
	return dwResult;
}

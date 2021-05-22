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

#include "xsmc.h"
#include "xsHost.h"
#include "modPreference.h"

#include "nvs_flash/include/nvs_flash.h"

void xs_preference_set(xsMachine *the)
{
	esp_err_t err;
	nvs_handle handle;
	uint8_t b;
	int32_t integer;
	char *str, key[64];
	double dbl;

	xsmcToStringBuffer(xsArg(1), key, sizeof(key));

	err = nvs_open(xsmcToString(xsArg(0)), NVS_READWRITE, &handle);
	if (ESP_OK != err)
		xsUnknownError("nvs_open fail");

	nvs_erase_key(handle, key);		// ESP IDF bug: if type of key changes, new type is ignored. work around by deleting first.
	if ((ESP_OK != err) && (ESP_ERR_NVS_NOT_FOUND != err))
		goto bail;

	switch (xsmcTypeOf(xsArg(2))) {
		case xsBooleanType:
			err = nvs_set_u8(handle, key, xsmcToBoolean(xsArg(2)));
			break;

		case xsIntegerType:
			err = nvs_set_i32(handle, key, xsmcToInteger(xsArg(2)));
			break;

		case xsNumberType:
			dbl = xsmcToNumber(xsArg(2));
			if (dbl != (int)dbl) {
				nvs_close(handle);
				xsUnknownError("float unsupported");
			}
			err = nvs_set_i32(handle, key, (int)dbl);
			break;

		case xsStringType:
			err = nvs_set_str(handle, key, xsmcToString(xsArg(2)));
			break;

		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype))
				err = nvs_set_blob(handle, key, xsmcToArrayBuffer(xsArg(2)), xsmcGetArrayBufferLength(xsArg(2)));
			else
				goto unknown;
			break;

		unknown:
		default:
			nvs_close(handle);
			xsUnknownError("unsupported type");
	}

	if (ESP_OK != err) goto bail;

	err = nvs_commit(handle);

bail:
	nvs_close(handle);

	if (ESP_OK != err)
		xsUnknownError("nvs set fail");
}

void xs_preference_get(xsMachine *the)
{
	esp_err_t err;
	nvs_handle handle;
	uint8_t b;
	int32_t integer;
	char *str, key[64];

	xsmcToStringBuffer(xsArg(1), key, sizeof(key));

	err = nvs_open(xsmcToString(xsArg(0)), NVS_READONLY, &handle);
	if (ESP_OK != err)
		return;  // most likely that domain doesn't exist yet

	if (ESP_OK == (err = nvs_get_u8(handle, key, &b)))
		xsmcSetBoolean(xsResult, b);
	else if (ESP_OK == (err = nvs_get_i32(handle, key, &integer)))
		xsmcSetInteger(xsResult, integer);
	else if (ESP_OK == (err = nvs_get_str(handle, key, NULL, &integer))) {
		xsResult = xsStringBuffer(NULL, integer);
		err = nvs_get_str(handle, key, xsmcToString(xsResult), &integer);
	}
	else if (ESP_OK == (err = nvs_get_blob(handle, key, NULL, &integer))) {
		xsmcSetArrayBuffer(xsResult, NULL, integer);
		err = nvs_get_blob(handle, key, xsmcToArrayBuffer(xsResult), &integer);
	}
	else
		xsmcSetUndefined(xsResult);	// not an error if not found, just undefined

	if (err == ESP_ERR_NVS_NOT_FOUND)
		err = ESP_OK;

bail:
	nvs_close(handle);

	if (ESP_OK != err)
		xsUnknownError("nvs get fail");
}

void xs_preference_delete(xsMachine *the)
{
	esp_err_t err;
	nvs_handle handle;
	char key[64];

	xsmcToStringBuffer(xsArg(1), key, sizeof(key));

	err = nvs_open(xsmcToString(xsArg(0)), NVS_READWRITE, &handle);
	if (ESP_OK != err)
		return;  // most likely that domain doesn't exist yet

	err = nvs_erase_key(handle, key);
	if (ESP_ERR_NVS_NOT_FOUND == err) err = ESP_OK;
	if (ESP_OK != err) goto bail;

	err = nvs_commit(handle);

bail:
	nvs_close(handle);

	if (ESP_OK != err)
		xsUnknownError("nvs erase fail");
}

void xs_preference_keys(xsMachine *the)
{
	int i = 0;
	nvs_iterator_t it;
	
	xsmcSetNewArray(xsResult, 0);
	
	it = nvs_entry_find(NVS_DEFAULT_PART_NAME, xsmcToString(xsArg(0)), NVS_TYPE_ANY);
	if (!it)
		return;

	xsmcVars(1);

	while (it) {
        nvs_entry_info_t info;

        nvs_entry_info(it, &info);

		xsmcSetString(xsVar(0), info.key);
		xsmcSetIndex(xsResult, i++, xsVar(0));

        it = nvs_entry_next(it);
	}
}

uint8_t modPreferenceSet(char *domain, char *key, uint8_t prefType, uint8_t *value, uint16_t byteCount)
{
	nvs_handle handle;
	uint8_t resultCode = -1;

	if (ESP_OK == nvs_open(domain, NVS_READWRITE, &handle)) {
		int result = -1;

		nvs_erase_key(handle, key);		// ESP IDF bug: if type of key changes, new type is ignored. work around by deleting first.

		if (kPrefsTypeBoolean == prefType)
			result = nvs_set_u8(handle, key, *(uint8_t *)value);
		else if (kPrefsTypeInteger == prefType)
			result = nvs_set_i32(handle, key, *(int32_t *)value);
		else if (kPrefsTypeString == prefType) {
			char *str = c_calloc(1, byteCount + 1);
			if (str) {
				c_memcpy(str, value, byteCount);
				result = nvs_set_str(handle, key, str);
				c_free(str);
			}
		}
		else if (kPrefsTypeBuffer == prefType)
			result = nvs_set_blob(handle, key, value, byteCount);

		resultCode = result ? -1 : 0;
		nvs_close(handle);
	}

	return resultCode ? 0 : 1;
}


uint8_t modPreferenceGet(char *domain, char *key, uint8_t *type, uint8_t *value, uint16_t byteCountIn, uint16_t *byteCountOut)
{
	nvs_handle handle;
	uint8_t resultCode = -1;

	if (ESP_OK == nvs_open(domain, NVS_READONLY, &handle)) {
		int32_t size = byteCountIn;
		resultCode = 0;
		if (!nvs_get_u8(handle, key, value)) {
			*type = kPrefsTypeBoolean;
			*byteCountOut = 1;
		}
		else if (!nvs_get_i32(handle, key, (int32_t *)value)) {
			*type = kPrefsTypeInteger;
			*byteCountOut = 4;
		}
		else if (!nvs_get_str(handle, key, value, &size)) {
			*type = kPrefsTypeString;
			*byteCountOut = (uint16_t)size;
		}
		else if (!nvs_get_blob(handle, key, value, &size)) {
			*type = kPrefsTypeBuffer;
			*byteCountOut = (uint16_t)size;
		}
		else
			resultCode = -2;

		nvs_close(handle);
	}

	return resultCode ? 0 : 1;
}

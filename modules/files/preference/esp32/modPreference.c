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

#include "nvs_flash/include/nvs_flash.h"

void xs_preference_set(xsMachine *the)
{
	esp_err_t err;
	nvs_handle handle;
	uint8_t b;
	int32_t integer;
	char *str;
	double dbl;

	err = nvs_open(xsmcToString(xsArg(0)), NVS_READWRITE, &handle);
	if (ESP_OK != err)
		xsUnknownError("nvs_open fail");

	switch (xsmcTypeOf(xsArg(2))) {
		case xsBooleanType:
			err = nvs_set_u8(handle, xsmcToString(xsArg(1)), xsmcToBoolean(xsArg(2)));
			break;

		case xsIntegerType:
			err = nvs_set_i32(handle, xsmcToString(xsArg(1)), xsmcToInteger(xsArg(2)));
			break;

		case xsNumberType:
			dbl = xsmcToNumber(xsArg(2));
			if (dbl != (int)dbl) {
				nvs_close(handle);
				xsUnknownError("float unsupported");
			}
			err = nvs_set_i32(handle, xsmcToString(xsArg(1)), (int)dbl);
			break;

		case xsStringType:
			err = nvs_set_str(handle, xsmcToString(xsArg(1)), xsmcToString(xsArg(2)));
			break;


		case xsReferenceType:
			if (xsmcIsInstanceOf(xsArg(2), xsArrayBufferPrototype))
				err = nvs_set_blob(handle, xsmcToString(xsArg(1)), xsmcToArrayBuffer(xsArg(2)), xsGetArrayBufferLength(xsArg(2)));
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
	char *str, *key;

	err = nvs_open(xsmcToString(xsArg(0)), NVS_READONLY, &handle);
	if (ESP_OK != err)
		xsUnknownError("nvs_open fail");

	key = xsmcToString(xsArg(1));
	if (ESP_OK == nvs_get_u8(handle, key, &b))
		xsmcSetBoolean(xsResult, b);
	else if (ESP_OK == nvs_get_i32(handle, key, &integer))
		xsmcSetInteger(xsResult, integer);
	else if (ESP_OK == nvs_get_str(handle, key, NULL, &integer)) {
		xsResult = xsStringBuffer(NULL, integer + 1);
		err = nvs_get_str(handle, key, xsmcToString(xsResult), &integer);
	}
	else if (ESP_OK == nvs_get_blob(handle, key, NULL, &integer)) {
		xsResult = xsArrayBuffer(NULL, integer);
		err = nvs_get_blob(handle, key, xsmcToArrayBuffer(xsResult), &integer);
	}
	else
		xsmcSetUndefined(xsResult);	// not an error if not found, just undefined

bail:
	nvs_close(handle);

	if (ESP_OK != err)
		xsUnknownError("nvs get fail");
}

void xs_preference_delete(xsMachine *the)
{
	esp_err_t err;
	nvs_handle handle;

	err = nvs_open(xsmcToString(xsArg(0)), NVS_READWRITE, &handle);
	if (ESP_OK != err)
		xsUnknownError("nvs_open fail");

	err = nvs_erase_key(handle, xsmcToString(xsArg(1)));
	if (ESP_ERR_NVS_NOT_FOUND == err) err = ESP_OK;
	if (ESP_OK != err) goto bail;

	err = nvs_commit(handle);

bail:
	nvs_close(handle);

	if (ESP_OK != err)
		xsUnknownError("nvs erase fail");
}

/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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
#include "app_update/include/esp_ota_ops.h"

typedef struct {
	esp_ota_handle_t			handle;
	const esp_partition_t		*partition;
} xsOTARecord, *xsOTA;

void xs_ota_destructor(void *data)
{
	xsOTA ota = data;

	if (ota) {
		if (ota->handle)
			esp_ota_end(ota->handle);
		c_free(ota);
	}
}

void xs_ota(xsMachine *the)
{
	esp_err_t err;
	esp_ota_handle_t handle;
	xsOTA ota = c_calloc(1, sizeof(xsOTARecord));
	if (NULL == ota)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, ota);

	ota->partition = esp_ota_get_next_update_partition(NULL);
	if (NULL == ota->partition)
		xsUnknownError("no update parition");

	err = esp_ota_begin(ota->partition, OTA_SIZE_UNKNOWN, &ota->handle);
	if (ESP_OK != err)
		xsUnknownError("begin failed");
}

void xs_ota_write(xsMachine *the)
{
	esp_err_t err;
	xsOTA ota = xsmcGetHostData(xsThis);
	void *data = xsmcToArrayBuffer(xsArg(0));
	int size = xsGetArrayBufferLength(xsArg(0));

	err = esp_ota_write(ota->handle, data, size);
	if (ESP_OK != err)
		xsUnknownError("write failed");
}

void xs_ota_cancel(xsMachine *the)
{
	xsOTA ota = xsmcGetHostData(xsThis);
	if (!ota->handle)
		return;

	esp_ota_end(ota->handle);
	ota->handle = 0;
}

void xs_ota_complete(xsMachine *the)
{
	esp_err_t err;
	xsOTA ota = xsmcGetHostData(xsThis);

	if (!ota->handle)
		xsUnknownError("bad state");

	err = esp_ota_end(ota->handle);
	if (ESP_OK != err)
		xsUnknownError("end failed");
	ota->handle = 0;

	err = esp_ota_set_boot_partition(ota->partition);
	if (ESP_OK != err)
		xsUnknownError("can't change boot partition");

	esp_restart();		//@@ perhaps do elsewhere
}

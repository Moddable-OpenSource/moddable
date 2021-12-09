/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
#include "mc.xs.h"			// for xsID_ values

#include "esp_partition.h"
#include "app_update/include/esp_ota_ops.h"

void xs_flash(xsMachine *the)
{
	const esp_partition_t *partition;
	char *partitionName = xsmcToString(xsArg(0));

	if (0 == c_strcmp(partitionName, "xs"))
		partition = esp_partition_find_first(0x40, 1,  NULL);
	else if (0 == c_strcmp(partitionName, "running"))
		partition = esp_ota_get_running_partition();
	else if (0 == c_strcmp(partitionName, "nextota"))
		partition = esp_ota_get_next_update_partition(NULL);
	else
		partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, partitionName);

	if (!partition)
		xsUnknownError("can't find partition");

	xsmcSetHostData(xsThis, (void *)partition);
}

void xs_flash_destructor(void *data)
{
}

void xs_flash_erase(xsMachine *the)
{
	esp_partition_t *partition = xsmcGetHostDataValidate(xsThis, (void *)xs_flash_destructor);
	int sector = xsmcToInteger(xsArg(0));

	if (ESP_OK != esp_partition_erase_range(partition, sector * SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE))
		xsUnknownError("erase failed");
}

void xs_flash_read(xsMachine *the)
{
	esp_partition_t *partition = xsmcGetHostDataValidate(xsThis, (void *)xs_flash_destructor);
	int offset = xsmcToInteger(xsArg(0));
	int byteLength = xsmcToInteger(xsArg(1));

	xsmcSetArrayBuffer(xsResult, NULL, byteLength);
	if (ESP_OK != esp_partition_read(partition, offset, xsmcToArrayBuffer(xsResult), byteLength))
		xsUnknownError("read failed");
}

void xs_flash_write(xsMachine *the)
{
	esp_partition_t *partition = xsmcGetHostDataValidate(xsThis, (void *)xs_flash_destructor);
	int offset = xsmcToInteger(xsArg(0));
	int byteLength = xsmcToInteger(xsArg(1));
	void *buffer;
	xsUnsignedValue bufferLength;

	xsmcGetBufferReadable(xsArg(2), &buffer, &bufferLength);
	if ((byteLength <= 0) || (bufferLength < (xsUnsignedValue)byteLength))
		xsUnknownError("invalid");

	if (ESP_OK != esp_partition_write(partition, offset, buffer, byteLength))
		xsUnknownError("write failed");
}

void xs_flash_map(xsMachine *the)
{
	esp_partition_t *partition = xsmcGetHostDataValidate(xsThis, (void *)xs_flash_destructor);
	int size = partition->size;
	const void *partitionAddress;
	spi_flash_mmap_handle_t handle;

	if (ESP_OK != esp_partition_mmap(partition, 0, size, SPI_FLASH_MMAP_DATA, &partitionAddress, &handle))
		xsUnknownError("map failed");

	xsmcVars(1);

	xsResult = xsNewHostObject(NULL);
	xsmcSetHostBuffer(xsResult, (void *)partitionAddress, size);
	xsmcPetrifyHostBuffer(xsResult);
	xsmcSetInteger(xsVar(0), size);
	xsDefine(xsResult, xsID_byteLength, xsVar(0), xsDontDelete | xsDontSet);
}

void xs_flash_byteLength(xsMachine *the)
{
	esp_partition_t *partition = xsmcGetHostDataValidate(xsThis, (void *)xs_flash_destructor);
	xsmcSetInteger(xsResult, partition->size);
}

void xs_flash_blockSize(xsMachine *the)
{
	xsmcSetInteger(xsResult, SPI_FLASH_SEC_SIZE);
}

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

struct modFlashRecord {
	const esp_partition_t *partition;
	spi_flash_mmap_handle_t map;
	const void *partitionAddress;
};

typedef struct modFlashRecord modFlashRecord;
typedef struct modFlashRecord *modFlash;

void xs_flash(xsMachine *the)
{
	modFlashRecord mrf = {0};
	char *partitionName = xsmcToString(xsArg(0));

	if (0 == c_strcmp(partitionName, "xs"))
		mrf.partition = esp_partition_find_first(0x40, 1,  NULL);
	else if (0 == c_strcmp(partitionName, "running"))
		mrf.partition = esp_ota_get_running_partition();
	else if (0 == c_strcmp(partitionName, "nextota"))
		mrf.partition = esp_ota_get_next_update_partition(NULL);
	else
		mrf.partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, partitionName);

	if (!mrf.partition)
		xsUnknownError("can't find partition");

	xsmcSetHostChunk(xsThis, (void *)&mrf, sizeof(mrf));
}

void xs_flash_destructor(void *data)
{
	modFlash mrf = data;

	if (mrf && mrf->map)
		spi_flash_munmap(mrf->map);
}

void xs_flash_erase(xsMachine *the)
{
	modFlash mrf = xsmcGetHostChunkValidate(xsThis, xs_flash_destructor);
	const esp_partition_t *partition = mrf->partition;
	int sector = xsmcToInteger(xsArg(0));

	if (ESP_OK != esp_partition_erase_range(partition, sector * SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE))
		xsUnknownError("erase failed");
}

void xs_flash_read(xsMachine *the)
{
	modFlash mrf = xsmcGetHostChunkValidate(xsThis, xs_flash_destructor);
	const esp_partition_t *partition = mrf->partition;
	int offset = xsmcToInteger(xsArg(0));
	int byteLength = xsmcToInteger(xsArg(1));

	xsmcSetArrayBuffer(xsResult, NULL, byteLength);
	if (ESP_OK != esp_partition_read(partition, offset, xsmcToArrayBuffer(xsResult), byteLength))
		xsUnknownError("read failed");
}

void xs_flash_write(xsMachine *the)
{
	modFlash mrf = xsmcGetHostChunkValidate(xsThis, xs_flash_destructor);
	const esp_partition_t *partition = mrf->partition;
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
	modFlash mrf = xsmcGetHostChunkValidate(xsThis, xs_flash_destructor);
	const esp_partition_t *partition = mrf->partition;
	int size = partition->size;
	const void *partitionAddress = mrf->partitionAddress;
	spi_flash_mmap_handle_t map = mrf->map;
	
	if (!mrf->map) {
		if (ESP_OK != esp_partition_mmap(partition, 0, size, SPI_FLASH_MMAP_DATA, &partitionAddress, &map))
			xsUnknownError("map failed");
		mrf->map = map;
		mrf->partitionAddress = partitionAddress;
	}

	xsmcVars(1);
	xsResult = xsNewHostObject(NULL);
	xsmcSetHostBuffer(xsResult, (void *)partitionAddress, size);
	xsmcPetrifyHostBuffer(xsResult);
	xsmcSetInteger(xsVar(0), size);
	xsmcDefine(xsResult, xsID_byteLength, xsVar(0), xsDontDelete | xsDontSet);
	xsmcDefine(xsResult, xsID_flash, xsThis, xsDontDelete | xsDontSet);	// reference flash instance so it isn't GC-ed while map is alive
}

void xs_flash_byteLength(xsMachine *the)
{
	modFlash mrf = xsmcGetHostChunkValidate(xsThis, xs_flash_destructor);
	const esp_partition_t *partition = mrf->partition;
	xsmcSetInteger(xsResult, partition->size);
}

void xs_flash_blockSize(xsMachine *the)
{
	xsmcSetInteger(xsResult, SPI_FLASH_SEC_SIZE);
}

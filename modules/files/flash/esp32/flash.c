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

#include "esp_partition.h"

struct modFlashRecord {
	const esp_partition_t *partition;
	uint32_t partitionStart;
	uint32_t partitionByteLength;
	uint32_t partitionEnd;
};

typedef struct modFlashRecord modFlashRecord;
typedef struct modFlashRecord *modFlash;

void xs_flash(xsMachine *the)
{
	modFlashRecord flash;

	if (xsStringType == xsmcTypeOf(xsArg(0))) {
		char *partition = xsmcToString(xsArg(0));

		if (0 == c_strcmp(partition, "xs")) {
			flash.partition = esp_partition_find_first(0x40, 1,  NULL);
			if (!flash.partition)
				xsUnknownError("can't find xs partition");
		}
		else {
			flash.partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, partition);
			if (!flash.partition)
			xsUnknownError("can't find xs partition");
		}

		flash.partitionByteLength = flash.partition->size;
	}
	else {
		xsUnknownError("must specify partion name");

		//flash.partitionEnd = flash.partitionStart + flash.partitionByteLength;
	}

	xsmcSetHostChunk(xsThis, &flash, sizeof(flash));
}

void xs_flash_destructor(void *data)
{
}

void xs_flash_erase(xsMachine *the)
{
	modFlash flash = xsmcGetHostChunk(xsThis);
	int sector = xsmcToInteger(xsArg(0));

	if (ESP_OK != esp_partition_erase_range(flash->partition, sector * SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE))
		xsUnknownError("erase failed");
}

void xs_flash_read(xsMachine *the)
{
	modFlash flash = xsmcGetHostChunk(xsThis);
	int offset = xsmcToInteger(xsArg(0));
	int byteLength = xsmcToInteger(xsArg(1));

	xsResult = xsArrayBuffer(NULL, byteLength);
	if (ESP_OK != esp_partition_read(flash->partition, offset, xsmcToArrayBuffer(xsResult), byteLength))
		xsUnknownError("read failed");
}

void xs_flash_write(xsMachine *the)
{
	modFlash flash = xsmcGetHostChunk(xsThis);
	int offset = xsmcToInteger(xsArg(0));
	int byteLength = xsmcToInteger(xsArg(1));
	void *buffer = xsmcToArrayBuffer(xsArg(2));
	if (ESP_OK != esp_partition_write(flash->partition, offset, buffer, byteLength))
		xsUnknownError("write failed");
}

void xs_flash_map(xsMachine *the)
{
	modFlash flash = xsmcGetHostChunk(xsThis);
	const void *partitionAddress;
	spi_flash_mmap_handle_t handle;

	if (ESP_OK != esp_partition_mmap(flash->partition, 0, flash->partition->size, SPI_FLASH_MMAP_DATA, &partitionAddress, &handle))
		xsUnknownError("map failed");

	xsmcVars(1);

	xsResult = xsNewHostObject(NULL);
	xsmcSetHostData(xsResult, (void *)partitionAddress);
	xsmcSetInteger(xsVar(0), flash->partition->size);
	xsmcSet(xsResult, xsID_byteLength, xsVar(0));
}

void xs_flash_byteLength(xsMachine *the)
{
	modFlash flash = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, flash->partitionByteLength);
}

void xs_flash_blockSize(xsMachine *the)
{
	xsmcSetInteger(xsResult, SPI_FLASH_SEC_SIZE);
}


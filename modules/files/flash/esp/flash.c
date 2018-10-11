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

#include "user_interface.h"
#include "spi_flash.h"

//static const int FLASH_INT_MASK = ((B10 << 8) | B00111010);
static const int FLASH_INT_MASK = ((2 << 8) | 0x3A);

struct modFlashRecord {
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
			flash.partitionStart = (uintptr_t)kModulesStart - (uintptr_t)kFlashStart;
			flash.partitionByteLength = kModulesByteLength;
		}
		else
			xsUnknownError("unknown partition");
	}
	else {
		flash.partitionStart = xsmcToInteger(xsArg(0));
		flash.partitionByteLength = xsmcToInteger(xsArg(1));
	}

	flash.partitionEnd = flash.partitionStart + flash.partitionByteLength;
	xsmcSetHostChunk(xsThis, &flash, sizeof(flash));
}

void xs_flash_destructor(void *data)
{
}

void xs_flash_erase(xsMachine *the)
{
	modFlash flash = xsmcGetHostChunk(xsThis);
	SpiFlashOpResult result;
	int sector = xsmcToInteger(xsArg(0)) + (flash->partitionStart / SPI_FLASH_SEC_SIZE);
	if ((sector < 0) || (sector >= (flash->partitionEnd / SPI_FLASH_SEC_SIZE)))
		xsUnknownError("invalid sector");

    ets_isr_mask(FLASH_INT_MASK);
	result = spi_flash_erase_sector(sector);
    ets_isr_unmask(FLASH_INT_MASK);

	if (SPI_FLASH_RESULT_OK != result)
		modLog("erase fail");
}

void xs_flash_read(xsMachine *the)
{
	modFlash flash = xsmcGetHostChunk(xsThis);
	SpiFlashOpResult result;
	int offset = xsmcToInteger(xsArg(0));
	int byteLength = xsmcToInteger(xsArg(1));
	void *buffer;

	if ((offset < 0) || (offset >= flash->partitionByteLength))
		xsUnknownError("invalid offset");

	xsResult = xsArrayBuffer(NULL, byteLength);
	buffer = xsmcToArrayBuffer(xsResult);

    ets_isr_mask(FLASH_INT_MASK);
	result = spi_flash_read(offset + flash->partitionStart, buffer, byteLength);
    ets_isr_unmask(FLASH_INT_MASK);

	if (SPI_FLASH_RESULT_OK != result)
		modLog("read fail");
}

void xs_flash_write(xsMachine *the)
{
	modFlash flash = xsmcGetHostChunk(xsThis);
	SpiFlashOpResult result;
	int offset = xsmcToInteger(xsArg(0));
	int byteLength = xsmcToInteger(xsArg(1));
	void *buffer;

	if ((offset < 0) || (offset >= flash->partitionByteLength))
		xsUnknownError("invalid offset");

	buffer = xsmcToArrayBuffer(xsArg(2));

    ets_isr_mask(FLASH_INT_MASK);
	result = spi_flash_write(offset + flash->partitionStart, buffer, byteLength);
    ets_isr_unmask(FLASH_INT_MASK);

	if (SPI_FLASH_RESULT_OK != result)
		modLog("write fail");
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

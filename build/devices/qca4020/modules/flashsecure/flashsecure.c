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

#if __ets__
#include "xsesp.h"

#include "flash_sr.h"
#endif

struct modFlashSecureRecord {
	int section;
};

typedef struct modFlashSecureRecord modFlashSecureRecord;
typedef struct modFlashSecureRecord *modFlashSecure;

void xs_flashsecure(xsMachine *the)
{
	modFlashSecureRecord flash;

	flash.section = xsmcToInteger(xsArg(0));
	xsmcSetHostChunk(xsThis, &flash, sizeof(flash));
}

void xs_flashsecure_destructor(void *data)
{
}

void xs_flashsecure_erase(xsMachine *the)
{
	modFlashSecure flash = xsmcGetHostChunk(xsThis);

//	if (ESP_OK != esp_spi_flash_erase_sec_reg(flash->section))
		xsUnknownError("erase failed");
}

void xs_flashsecure_read(xsMachine *the)
{
	modFlashSecure flash = xsmcGetHostChunk(xsThis);
	int offset = xsmcToInteger(xsArg(0));
	int byteLength = xsmcToInteger(xsArg(1));

	if (0 != offset)
		xsUnknownError("must read at offset 0");

	xsResult = xsArrayBuffer(NULL, byteLength);
//	if (ESP_OK != esp_spi_flash_read_sec_reg(flash->section, xsmcToArrayBuffer(xsResult), byteLength))
		xsUnknownError("read failed");
}

void xs_flashsecure_write(xsMachine *the)
{
	modFlashSecure flash = xsmcGetHostChunk(xsThis);
	int offset = xsmcToInteger(xsArg(0));
	int byteLength = xsmcToInteger(xsArg(1));
	void *buffer = xsmcToArrayBuffer(xsArg(2));

	if (0 != offset)
		xsUnknownError("must write to offset 0");

//	if (ESP_OK != esp_spi_flash_write_sec_reg(flash->section, buffer, byteLength))
		xsUnknownError("write failed");
}

void xs_flashsecure_byteLength(xsMachine *the)
{
	modFlashSecure flash = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, 64);
}

void xs_flashsecure_blockSize(xsMachine *the)
{
	xsmcSetInteger(xsResult, 64);
}


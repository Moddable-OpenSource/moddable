/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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
#include "mc.xs.h"      // for xsID_ values
#include "builtinCommon.h"

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

struct xsPartitionInfo {
	uint8_t		id;
	const char	*label;
};

#define PARTITION_INFO(node_id) \
	{ \
		.id = DT_FIXED_PARTITION_ID(node_id), \
		.label = DT_PROP(node_id, label), \
	},

static const struct xsPartitionInfo gPartitions[] = {
	DT_FOREACH_STATUS_OKAY_VARGS(fixed_partitions, DT_FOREACH_CHILD, PARTITION_INFO)
};

struct xsFlashIteratorRecord {
	int index;
};
typedef struct xsFlashIteratorRecord xsFlashIteratorRecord;
typedef struct xsFlashIteratorRecord *xsFlashIterator;

struct xsFlashRecord {
	const struct xsPartitionInfo	*partition;
	const struct flash_area			*fa;
	uint32_t								blockLength;
	uint8_t								readOnly;
};
typedef struct xsFlashRecord xsFlashRecord;
typedef struct xsFlashRecord *xsFlash;

/*
	Flash Iterator
*/

void xs_flashIterator_(xsMachine *the)
{
	xsFlashIteratorRecord fi = {.index = 0};

	xsmcSetHostChunk(xsThis, &fi, sizeof(fi));
}

void xs_flashIterator_destructor(void *data)
{
}

void xs_flashIterator_next(xsMachine *the)
{
	xsFlashIterator fi = xsmcGetHostChunkValidate(xsThis, xs_flashIterator_destructor);

	xsmcVars(2);
	if (fi->index < ARRAY_SIZE(gPartitions)) {
		xsmcSetFalse(xsVar(0));
		xsmcSetString(xsVar(1), (char *)gPartitions[fi->index++].label);
	}
	else {
		xsmcSetTrue(xsVar(0));
	}

	xsResult = xsNewObject();
	xsmcDefine(xsResult, xsID_done, xsVar(0), xsDefault);
	xsmcDefine(xsResult, xsID_value, xsVar(1), xsDefault);
}

void xs_flashIterator_return(xsMachine *the)
{
	xsFlashIterator fi = xsmcGetHostChunkValidate(xsThis, xs_flashIterator_destructor);

	xs_flashIterator_destructor(fi);
	fi->index = ARRAY_SIZE(gPartitions);
	xs_flashIterator_next(the);
	xsmcSetHostChunk(xsThis, C_NULL, 0);
}

/*
	Flash
*/

void xs_flashstorage_destructor(void *data)
{
		xsFlash flash = data;
		if (flash && flash->fa)
			flash_area_close(flash->fa);
}

void xs_flashstorage_close(xsMachine *the)
{
	if (!xsmcGetHostChunk(xsThis)) 
		return;

	xsFlash flash = xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);

	flash_area_close(flash->fa);

	xsmcSetHostChunk(xsThis, NULL, 0);
	xsmcSetHostDestructor(xsThis, NULL);
}

void xs_flashstorage_open(xsMachine *the)
{
	if (!xsmcHas(xsArg(0), xsID_path))
		xsUnknownError("no path");

	xsmcGet(xsResult, xsArg(0), xsID_path);
	char *partitionName = xsmcToString(xsResult);
	xsFlashRecord flash = {0};

	int i;
	for (i = 0; i < ARRAY_SIZE(gPartitions); i++) {
		if (0 == c_strcmp(partitionName, gPartitions[i].label)) {
			flash.partition = &gPartitions[i];
			break;
		}
	}
	if (!flash.partition)
		xsUnknownError("can't find partition");

	if (xsmcHas(xsArg(0), xsID_mode)) {
		xsmcGet(xsResult, xsArg(0), xsID_mode);
		char *modeStr = xsmcToString(xsResult);
		if (0 == c_strcmp(modeStr, "r"))
			flash.readOnly = 1;
		else if (0 == c_strcmp(modeStr, "r+"))
			;
		else
			xsUnknownError("invalid mode");		//@@ range error
	}

	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("unsupported");

	if (flash_area_open(flash.partition->id, &flash.fa) != 0)
		xsUnknownError("can't open partition");

	struct flash_pages_info page_info;
	if (flash_get_page_info_by_offs(flash.fa->fa_dev, flash.fa->fa_off, &page_info) < 0) {
		flash_area_close(flash.fa);		
		xsUnknownError("can't get page info");
	}
	flash.blockLength = page_info.size;

	xsResult = xsNewHostInstance(xsArg(1));
	xsmcSetHostChunk(xsResult, (void *)&flash, sizeof(flash));
}

void xs_flashstorage_eraseBlock(xsMachine *the)
{
	xsFlash flash = xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);
	const struct flash_area *fa = flash->fa;
	uint32_t blockLength = flash->blockLength;
	if (flash->readOnly)
		xsUnknownError("read-only");
	int start = xsmcToInteger(xsArg(0));
	int stop = start + 1;
	if (xsmcArgc > 1)
		stop = xsmcToInteger(xsArg(1));

	int result = flash_area_erase(fa, start * blockLength, (stop - start) * blockLength);
	if (result < 0)
		xsUnknownError("erase failed");
}

void xs_flashstorage_read(xsMachine *the)
{
	xsFlash flash = xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);
	const struct flash_area *fa = flash->fa;
	void *buffer;
	xsUnsignedValue byteLength;
	int position = xsmcToInteger(xsArg(1));

	int type = xsmcTypeOf(xsArg(0));
	if ((xsIntegerType == type) || (xsNumberType == type)) {
 		byteLength = xsmcToInteger(xsArg(0));
		xsmcSetArrayBuffer(xsResult, NULL, byteLength);
		buffer = xsmcToArrayBuffer(xsResult);
	}
	else {
		xsmcGetBufferWritable(xsArg(0), &buffer, &byteLength);
		xsmcSetInteger(xsResult, byteLength);
	}

	if (flash_area_read(fa, position, buffer, byteLength) < 0)
		xsUnknownError("read failed");
}

void xs_flashstorage_write(xsMachine *the)
{
	xsFlash flash = xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);
	const struct flash_area *fa = flash->fa;
	if (flash->readOnly)
		xsUnknownError("read-only");
	int position = xsmcToInteger(xsArg(1));

	void *buffer;
	xsUnsignedValue bufferLength;
	xsmcGetBufferReadable(xsArg(0), &buffer, &bufferLength);

	int ret = flash_area_write(fa, position, buffer, bufferLength);
	xsLog("write returned %d\n", ret);
	if (ret < 0)
		xsUnknownError("write failed");
}

void xs_flashstorage_status(xsMachine *the)
{
	xsFlash flash = xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);
	const struct flash_area *fa = flash->fa;
	uint32_t blockLength = flash->blockLength;

	xsmcSetNewObject(xsResult);
	
	xsSlot tmp;
	xsmcSetInteger(tmp, fa->fa_size);
	xsmcSet(xsResult, xsID_size, tmp);

	xsmcSetInteger(tmp, blockLength);
	xsmcSet(xsResult, xsID_blockLength, tmp);

	xsmcSetInteger(tmp, fa->fa_size / blockLength);
	xsmcSet(xsResult, xsID_blocks, tmp);

	xsmcSetInteger(tmp, flash_area_align(fa));
	xsmcSet(xsResult, xsID_writeAlign, tmp);

	xsmcSetInteger(tmp, flash_area_erased_val(fa));
	xsmcSet(xsResult, xsID_eraseValue, tmp);
}

/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
#include "flash-esp.h"

#define kSlotByteLength (0x100000)
#define kAppStartOffset (0x1000)

extern uint8_t _SPIFFS_start;		// from linker
extern uint8_t _SPIFFS_end;

static const char *gPartitionNames[kESPFlashPartitionCount] = {"running", "nextota", "xs", "storage"};

struct xsFlashIteratorRecord {
	uint8_t		index;
};
typedef struct xsFlashIteratorRecord xsFlashIteratorRecord;
typedef struct xsFlashIteratorRecord *xsFlashIterator;

struct xsFlashRecord {
	uint32_t	offset;
	uint32_t	byteLength;
	uint8_t		partition;
	uint8_t		readOnly;
};
typedef struct xsFlashRecord xsFlashRecord;
typedef struct xsFlashRecord *xsFlash;

static uint8_t getPartitionRange(int partition, uint32_t *offset, uint32_t *byteLength)
{
	uint32_t slot = espFlashMappedOffset();

	switch (partition) {
		case kESPFlashPartitionRunning:
			*offset = slot + kAppStartOffset;
			*byteLength = kSlotByteLength - kAppStartOffset;
			return 1;

		case kESPFlashPartitionNextOTA:
			if (!espHasOTASlots())
				return 0;
			*offset = (slot ^ kSlotByteLength) + kAppStartOffset;
			*byteLength = kSlotByteLength - kAppStartOffset;
			return 1;

		case kESPFlashPartitionXS: {
			uint8_t *start = kModulesStart;
			if (!start || (start >= kFlashEnd))
				return 0;
			*offset = espMappedToFlashOffset(start);
			*byteLength = kFlashEnd - start;
			} return 1;

		case kESPFlashPartitionStorage:
			*offset = (uintptr_t)&_SPIFFS_start - (uintptr_t)kFlashStart;
			*byteLength = (uintptr_t)&_SPIFFS_end - (uintptr_t)&_SPIFFS_start;
			return 0 != *byteLength;
	}

	return 0;
}

/*
	Flash Iterator
*/

void xs_flashIterator_(xsMachine *the)
{
	xsFlashIteratorRecord fi = {0};

	xsmcSetHostChunk(xsThis, &fi, sizeof(fi));
}

void xs_flashIterator_destructor(void *data)
{
}

void xs_flashIterator_next(xsMachine *the)
{
	xsFlashIterator fi = xsmcGetHostChunkValidate(xsThis, xs_flashIterator_destructor);
	uint32_t offset, byteLength;

	while ((fi->index < kESPFlashPartitionCount) && !getPartitionRange(fi->index, &offset, &byteLength))
		fi->index += 1;

	xsmcVars(2);
	if (fi->index < kESPFlashPartitionCount) {
		xsmcSetFalse(xsVar(0));
		xsmcSetString(xsVar(1), (char *)gPartitionNames[fi->index]);
		fi->index += 1;
	}
	else
		xsmcSetTrue(xsVar(0));

	xsResult = xsNewObject();
	xsmcDefine(xsResult, xsID_done, xsVar(0), xsDefault);
	xsmcDefine(xsResult, xsID_value, xsVar(1), xsDefault);
}

void xs_flashIterator_return(xsMachine *the)
{
	xsFlashIterator fi = xsmcGetHostChunkValidate(xsThis, xs_flashIterator_destructor);

	fi->index = kESPFlashPartitionCount;
	xs_flashIterator_next(the);
}

/*
	Flash
*/

void xs_flashstorage_destructor(void *data)
{
}

void xs_flashstorage_close(xsMachine *the)
{
	if (!xsmcGetHostChunk(xsThis))
		return;

	xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);

	xsmcSetHostChunk(xsThis, NULL, 0);
	xsmcSetHostDestructor(xsThis, NULL);
}

void xs_flashstorage_open(xsMachine *the)
{
	xsFlashRecord flash = {0};
	int partition;

	if (!xsmcGet(xsResult, xsArg(0), xsID_path))
		xsUnknownError("no path");
	char *partitionName = xsmcToString(xsResult);
	for (partition = 0; partition < kESPFlashPartitionCount; partition++) {
		if (0 == c_strcmp(partitionName, gPartitionNames[partition]))
			break;
	}
	if ((kESPFlashPartitionCount == partition) || !getPartitionRange(partition, &flash.offset, &flash.byteLength))
		xsUnknownError("can't find partition");
	flash.partition = (uint8_t)partition;
	flash.readOnly = kESPFlashPartitionRunning == partition;		// writing the running application crashes

	if (xsmcGet(xsResult, xsArg(0), xsID_mode)) {
		char *modeStr = xsmcToString(xsResult);
		if (0 == c_strcmp(modeStr, "r"))
			flash.readOnly = 1;
		else if (0 == c_strcmp(modeStr, "r+")) {
			if (flash.readOnly)
				xsUnknownError("read-only partition");
		}
		else
			xsRangeError("invalid mode");
	}

	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("unsupported");

	xsResult = xsNewHostInstance(xsArg(1));
	xsmcSetHostChunk(xsResult, (void *)&flash, sizeof(flash));
}

static uint32_t checkRange(xsMachine *the, xsFlash flash, xsNumberValue position, uint32_t byteLength)
{
	if ((byteLength > flash->byteLength) || !(position >= 0) || (position > (flash->byteLength - byteLength)) || (position != (uint32_t)position))
		xsRangeError("out of range");

	return (uint32_t)position;
}

void xs_flashstorage_eraseBlock(xsMachine *the)
{
	int start = xsmcToInteger(xsArg(0));
	int stop = start + 1;
	if ((xsmcArgc > 1) && (xsUndefinedType != xsmcTypeOf(xsArg(1))))
		stop = xsmcToInteger(xsArg(1));

	xsFlash flash = xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);
	if (flash->readOnly)
		xsUnknownError("read-only");
	if ((start < 0) || (stop < start) || ((uint32_t)stop > (flash->byteLength / kFlashSectorSize)))
		xsRangeError("invalid block");

	if (!modSPIErase(flash->offset + (start * kFlashSectorSize), (stop - start) * kFlashSectorSize))
		xsUnknownError("erase failed");
}

void xs_flashstorage_read(xsMachine *the)
{
	xsNumberValue byteOffset = xsmcToNumber(xsArg(1));
	uint32_t offset;
	void *buffer;
	xsUnsignedValue byteLength;

	int type = xsmcTypeOf(xsArg(0));
	if ((xsIntegerType == type) || (xsNumberType == type)) {
		int count = xsmcToInteger(xsArg(0));
		if (count < 0)
			xsRangeError("invalid byteLength");
		byteLength = count;
		xsFlash flash = xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);
		offset = flash->offset + checkRange(the, flash, byteOffset, byteLength);
		xsmcSetArrayBuffer(xsResult, NULL, byteLength);
		buffer = xsmcToArrayBuffer(xsResult);
	}
	else {
		xsFlash flash = xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);
		xsmcGetBufferWritable(xsArg(0), &buffer, &byteLength);
		offset = flash->offset + checkRange(the, flash, byteOffset, byteLength);
		xsmcSetInteger(xsResult, byteLength);
	}

	if (!modSPIRead(offset, byteLength, buffer))
		xsUnknownError("read failed");
}

void xs_flashstorage_write(xsMachine *the)
{
	xsNumberValue byteOffset = xsmcToNumber(xsArg(1));
	xsFlash flash = xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);
	if (flash->readOnly)
		xsUnknownError("read-only");

	void *buffer;
	xsUnsignedValue byteLength;
	xsmcGetBufferReadable(xsArg(0), &buffer, &byteLength);
	uint32_t offset = flash->offset + checkRange(the, flash, byteOffset, byteLength);

	if (!modSPIWrite(offset, byteLength, buffer))
		xsUnknownError("write failed");
}

void xs_flashstorage_status(xsMachine *the)
{
	xsFlash flash = xsmcGetHostChunkValidate(xsThis, xs_flashstorage_destructor);

	xsmcSetNewObject(xsResult);

	xsmcVars(1);
	xsmcSetInteger(xsVar(0), flash->byteLength);
	xsmcSet(xsResult, xsID_size, xsVar(0));

	xsmcSetInteger(xsVar(0), kFlashSectorSize);
	xsmcSet(xsResult, xsID_blockLength, xsVar(0));

	xsmcSetInteger(xsVar(0), flash->byteLength / kFlashSectorSize);
	xsmcSet(xsResult, xsID_blocks, xsVar(0));
}

int getESPFlashPartition(xsMachine *the, xsSlot *from, uint8_t *writable)
{
	xsFlash flash = xsmcGetHostChunkValidate(*from, xs_flashstorage_destructor);
	*writable = !flash->readOnly;
	return flash->partition;
}

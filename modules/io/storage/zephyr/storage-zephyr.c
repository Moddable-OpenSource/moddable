/*
 * Copyright (c) 2024-2026  Moddable Tech, Inc.
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

#include <zephyr/settings/settings.h>

struct xsStorageIteratorRecord {
	uint32_t		offset;
	char			keys[];
};
typedef struct xsStorageIteratorRecord xsStorageIteratorRecord;
typedef struct xsStorageIteratorRecord *xsStorageIterator;

struct xsDirectoryRecord {
	uint8_t		format;
	uint8_t		readOnly;
	uint8_t		directoryLength;
	char		path[];
};
typedef struct xsDirectoryRecord xsDirectoryRecord;
typedef struct xsDirectoryRecord *xsDirectory;

/*
	helper
*/

#define throwIf(a) _throwIf(the, a)

static void _throwIf(xsMachine *the, int err)
{
	if (0 != err)
		xsUnknownError("setting error");
}

/*
	Storage Iterator
*/

static int measureIterator(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg, void *param)
{
	uint32_t *bytesNeeded = param;
	*bytesNeeded += c_strlen(key) + 1;
	return 0;
}

static int collectIterator(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg, void *param)
{
	xsStorageIterator iterator = param;
	c_strcpy(iterator->keys + iterator->offset, key);
	iterator->offset += c_strlen(key) + 1;
	return 0;
}

void xs_storageIterator_constructor(xsMachine *the)
{
	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsArg(0), xs_directorystorage_destructor);
	uint32_t bytesNeeded = 0;

	d->path[d->directoryLength - 1] = 0;
	int err = settings_load_subtree_direct(d->path, measureIterator, &bytesNeeded);
	d->path[d->directoryLength - 1] = SETTINGS_NAME_SEPARATOR;
	if (err)
		xsUnknownError("iterator measure failed");

	xsStorageIterator iterator = xsmcSetHostChunk(xsThis, C_NULL, sizeof(xsStorageIteratorRecord) + bytesNeeded + 1);
	d = (xsDirectory)xsmcGetHostChunkValidate(xsArg(0), xs_directorystorage_destructor);
	d->path[d->directoryLength - 1] = 0;
	err = settings_load_subtree_direct(d->path, collectIterator, iterator);
	d->path[d->directoryLength - 1] = SETTINGS_NAME_SEPARATOR;
	iterator->offset = 0;
	if (err)
		xsUnknownError("iterator collect failed");
}

void xs_storageIterator_destructor(void *data)
{
}

void xs_storageIterator_next(xsMachine *the)
{
	xsStorageIterator iterator = xsmcGetHostChunkValidate(xsThis, xs_storageIterator_destructor);
	char *key = iterator->keys + iterator->offset;
	int keyLength = c_strlen(key);
	
	xsmcVars(2);
	if (*key) {
		xsmcSetFalse(xsVar(0));
		xsmcSetStringBuffer(xsVar(1), C_NULL, keyLength);
		iterator = xsmcGetHostChunkValidate(xsThis, xs_storageIterator_destructor);
		key = iterator->keys + iterator->offset;
		c_strcpy(xsmcToString(xsVar(1)), key);
		iterator->offset += keyLength + 1;
	}
	else
		xsmcSetTrue(xsVar(0));

	xsResult = xsNewObject();
	xsmcDefine(xsResult, xsID_done, xsVar(0), xsDefault);
	xsmcDefine(xsResult, xsID_value, xsVar(1), xsDefault);
}

void xs_storageIterator_return(xsMachine *the)
{
	xsStorageIterator iterator = xsmcGetHostChunkValidate(xsThis, xs_storageIterator_destructor);

	iterator->offset = 0;
	iterator->keys[0] = 0;
	xs_storageIterator_destructor(iterator);
	xs_storageIterator_next(the);
	xsmcSetHostChunk(xsThis, NULL, 0);
}

/*
	Directory
*/

void xs_directorystorage_destructor(void *data)
{
}

void xs_directorystorage_close(xsMachine *the)
{
	if (!xsmcGetHostChunk(xsThis)) 
		return;

	xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);

	xsmcSetHostChunk(xsThis, NULL, 0);
	xsmcSetHostDestructor(xsThis, NULL);
}

void xs_directorystorage_open(xsMachine *the)
{
	uint8_t readOnly = 0;
	char domain[64];
	static uint8_t initialized = 0;

	if (!initialized) {
		throwIf(settings_subsys_init());
		initialized = true;
	}

	xsmcVars(1);
	if (!xsmcHas(xsArg(0), xsID_path))
		xsUnknownError("no path");
	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	xsmcToStringBuffer(xsVar(0), domain, sizeof(domain));
	size_t length = c_strlen(domain);
	if (0 == length) xsUnknownError("invalid");
	if ('/' == domain[length - 1])
		domain[length - 1] = 0;

	if (xsmcHas(xsArg(0), xsID_mode)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_mode);
		char *modeStr = xsmcToString(xsVar(0));
		if (0 == c_strcmp(modeStr, "r"))
			readOnly = 1;
		else if (0 == c_strcmp(modeStr, "r+"))
			;
		else
			xsUnknownError("invalid mode");
	}

	xsDirectoryRecord d = {0};
	d.format = builtinInitializeFormat(the, kIOFormatBuffer);
	d.readOnly = readOnly;
	d.directoryLength = c_strlen(domain);
	if (kIOFormatInvalid == d.format)
		xsRangeError("unsupported");

	xsResult = xsNewHostInstance(xsArg(1));
	xsDirectory dp = xsmcSetHostChunk(xsResult, &d, sizeof(d) + d.directoryLength + 64);
	c_strcpy(dp->path, domain);
	dp->path[dp->directoryLength++] = SETTINGS_NAME_SEPARATOR;
}

void xs_directorystorage_delete(xsMachine *the)
{
	char key[64];
	xsmcToStringBuffer(xsArg(0), key, sizeof(key));

	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	if (d->readOnly)
		xsUnknownError("read-only");

	c_strcpy(d->path + d->directoryLength, key);

	throwIf(settings_delete(d->path));
}

void xs_directorystorage_read(xsMachine *the)
{
	char key[64];
	xsmcToStringBuffer(xsArg(0), key, sizeof(key));

	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	c_strcpy(d->path + d->directoryLength, key);

	ssize_t valueLength = settings_get_val_len(d->path);
	throwIf(valueLength < 0);
	if (0 == valueLength)
		return;

	uint8_t *value = c_malloc(valueLength);
	throwIf(C_NULL == value);

	xsTry {
		int result = settings_load_one(d->path, value, valueLength);
		throwIf(result < 0);

		valueLength -= 1;

		if (value[0] != d->format)
			xsTypeError("mismatch");

		switch (d->format) {
			case kIOFormatBuffer: {
				if (xsmcArgc > 1) {
					void *data;
					xsUnsignedValue dataLength;

					xsmcGetBufferWritable(xsArg(1), &data, &dataLength);
					if (dataLength < valueLength)
						xsRangeError("buffer too small");
					c_memcpy(data, value + 1, valueLength);
					xsmcSetInteger(xsResult, valueLength);
				}
				else
					xsmcSetArrayBuffer(xsResult, value + 1, valueLength);
				} break;

			case kIOFormatString:
				xsmcSetStringBuffer(xsResult, value + 1, valueLength);
				break;

			case kIOFormatUint8:
				xsmcSetInteger(xsResult, *(uint8_t *)(value + 1));
				break;

			case kIOFormatInt8:
				xsmcSetInteger(xsResult, *(int8_t *)(value + 1));
				break;

			case kIOFormatUint16:
				xsmcSetInteger(xsResult, *(uint16_t *)(value + 1));
				break;

			case kIOFormatInt16:
				xsmcSetInteger(xsResult, *(int16_t *)(value + 1));
				break;

			case kIOFormatUint32: {
				uint32_t i = *(uint32_t *)(value + 1);
				xsmcSetUnsigned(xsResult, i);
				} break;

			case kIOFormatInt32:
				xsmcSetInteger(xsResult, *(int32_t *)(value + 1));
				break;

			case kIOFormatInt64:
				fxFromBigInt64(the, &xsResult, *(int64_t *)(value + 1));
				break;

			case kIOFormatUint64:
				fxFromBigUint64(the, &xsResult, *(uint64_t *)(value + 1));
				break;

			default:
				xsUnknownError("unexpected");
				break;
		}
	}
	xsCatch {
		c_free(value);
		xsThrow(xsException);
	}

	c_free(value);
}

void xs_directorystorage_write(xsMachine *the)
{
	char key[64];
	xsmcToStringBuffer(xsArg(0), key, sizeof(key));

	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	if (d->readOnly)
		xsUnknownError("read-only");

	c_strcpy(d->path + d->directoryLength, key);

	uint8_t buffer[16];
	buffer[0] = d->format;
	switch (d->format) {
		case kIOFormatBuffer: {
			void *data;
			xsUnsignedValue dataLength;

			xsmcGetBufferReadable(xsArg(1), &data, &dataLength);
			uint8_t *value = c_malloc(dataLength + 1);
			if (C_NULL == value)
				xsUnknownError("no memory");
			value[0] = d->format;
			c_memmove(value + 1, data, dataLength);
			int err = settings_save_one(d->path, value, 1 + dataLength);
			c_free(value);
			throwIf(err);
			} break;

		case kIOFormatString:{
			void *data = xsmcToString(xsArg(1));
			d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
			xsUnsignedValue dataLength = c_strlen(data);

			uint8_t *value = c_malloc(dataLength + 1);
			if (C_NULL == value)
				xsUnknownError("no memory");
			value[0] = d->format;
			c_memmove(value + 1, data, dataLength);
			int err = settings_save_one(d->path, value, 1 + dataLength);
			c_free(value);
			throwIf(err);
			} break;

		case kIOFormatUint8:
			buffer[1] = (uint8_t)xsmcToInteger(xsArg(1));
			d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
			throwIf(settings_save_one(d->path, buffer, 1 + 1));
			break;

		case kIOFormatInt8:
			buffer[1] = (int8_t)xsmcToInteger(xsArg(1));
			d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
			throwIf(settings_save_one(d->path, buffer, 1 + 1));
			break;
		
		case kIOFormatUint16:
			*(uint16_t *)(buffer + 1) = (uint16_t)xsmcToInteger(xsArg(1));
			d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
			throwIf(settings_save_one(d->path, buffer, 1 + 2));
			break;

		case kIOFormatInt16:
			*(int16_t *)(buffer + 1) = (int16_t)xsmcToInteger(xsArg(1));
			d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
			throwIf(settings_save_one(d->path, buffer, 1 + 2));
			break;
		
		case kIOFormatUint32:
			*(uint32_t *)(buffer + 1) = xsmcToUnsigned(xsArg(1));
			d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
			throwIf(settings_save_one(d->path, buffer, 1 + 4));
			break;

		case kIOFormatInt32:
			*(int32_t *)(buffer + 1) = (int32_t)xsmcToInteger(xsArg(1));
			d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
			throwIf(settings_save_one(d->path, buffer, 1 + 4));
			break;

		case kIOFormatInt64:
			*(int64_t *)(buffer + 1) = (int64_t)fxToBigInt64(the, &xsArg(1));
			d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
			throwIf(settings_save_one(d->path, buffer, 1 + 8));
			break;

		case kIOFormatUint64:
			*(uint64_t *)(buffer + 1) = (uint64_t)fxToBigUint64(the, &xsArg(1));
			d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
			throwIf(settings_save_one(d->path, buffer, 1 + 8));
			break;

		default:
			xsUnknownError("unexpected");
			break;
	}

	throwIf(settings_commit_subtree(d->path));
}

void xs_directorystorage_format_get(xsMachine *the)
{
	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	builtinGetFormat(the, d->format);
}

void xs_directorystorage_format_set(xsMachine *the)
{
	static const uint8_t formats[] = {kIOFormatBuffer, kIOFormatString, kIOFormatUint8, kIOFormatInt8, kIOFormatUint16, kIOFormatInt16, kIOFormatUint32, kIOFormatInt32, kIOFormatUint64, kIOFormatInt64, kIOFormatInvalid};
	uint8_t format = builtinSetFormat(the), i = 0;

	while (kIOFormatInvalid != formats[i]) {
		if (formats[i++] == format) {
			xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
			d->format = format;
			return;
		}
	}

	xsRangeError("unsupported");
}

/*
 * Copyright (c) 2025-2026  Moddable Tech, Inc.
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

#include "services/settings/settings_file.h"

struct xsStorageIteratorRecord {
	struct xsStorageIteratorRecord *next;
	char key[];
};
typedef struct xsStorageIteratorRecord xsStorageIteratorRecord;
typedef struct xsStorageIteratorRecord *xsStorageIterator;

struct xsDirectoryRecord {
	SettingsFile	file;
	uint8_t			format;
	uint8_t			readOnly:1;
	uint8_t			isPebbleNative:1;		// no type information saved in entries
};
typedef struct xsDirectoryRecord xsDirectoryRecord;
typedef struct xsDirectoryRecord *xsDirectory;

/*
	helpers
*/

#define throwIf(a) _throwIf(the, a)

static void _throwIf(xsMachine *the, status_t err)
{
	if (S_SUCCESS != err)
		xsUnknownError("key-value error");
}

/*
	Storage Iterator
*/

static bool addOne(SettingsFile *file, SettingsRecordInfo *info, void *context)
{
	xsStorageIterator *head = context;
	xsStorageIterator item = c_malloc(sizeof(xsStorageIteratorRecord) + info->key_len + 1);
	item->next = *head;
	info->get_key(file, item->key, info->key_len);
	item->key[info->key_len] = 0;
	*head = item;
	return true;
}

void xs_storageIterator_constructor(xsMachine *the)
{
	xsDirectory d = xsmcGetHostChunkValidate(xsArg(0), xs_directorystorage_destructor);
	xsStorageIterator iterator = C_NULL;

	throwIf(settings_file_each(&d->file, addOne, &iterator));

	xsmcSetHostData(xsThis, iterator);
}

void xs_storageIterator_destructor(void *data)
{
	xsStorageIterator iterator = data;
	while (iterator) {
		xsStorageIterator next = iterator->next;
		c_free(iterator);
		iterator = next;
	}
}

void xs_storageIterator_next(xsMachine *the)
{
	xsStorageIterator iterator = xsmcGetHostData(xsThis);
	if (iterator)
		xsmcGetHostDataValidate(xsThis, xs_storageIterator_destructor);

	xsmcVars(2);
	if (iterator) {
		xsStorageIterator next = iterator->next;

		xsmcSetFalse(xsVar(0));
		xsmcSetString(xsVar(1), iterator->key);

		xsmcSetHostData(xsThis, next);
		c_free(iterator);		
	}
	else {
		xsmcSetTrue(xsVar(0));
	}

	xsResult = xsNewObject();
	xsmcDefine(xsResult, xsID_done, xsVar(0), xsDefault);
	xsmcDefine(xsResult, xsID_value, xsVar(1), xsDefault);
}

void xs_storageIterator_return(xsMachine *the)
{
	xsStorageIterator iterator = xsmcGetHostData(xsThis);
	if (iterator)
		xsmcGetHostDataValidate(xsThis, xs_storageIterator_destructor);
	xs_storageIterator_destructor(iterator);
	xsmcSetHostData(xsThis, NULL);
	xs_storageIterator_next(the);
}

/*
	Directory
*/

void xs_directorystorage_destructor(void *data)
{
	xsDirectory d = data;
	if (d)
		settings_file_close(&d->file);
}

void xs_directorystorage_close(xsMachine *the)
{
	if (!xsmcGetHostChunk(xsThis)) 
		return;

	xsDirectory d = xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	xs_directorystorage_destructor(d);

	xsmcSetHostChunk(xsThis, NULL, 0);
	xsmcSetHostDestructor(xsThis, NULL);
}

void xs_directorystorage_open(xsMachine *the)
{
	char domain[64];
	xsDirectoryRecord d = {0};

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
			d.readOnly = true;
		else if (0 == c_strcmp(modeStr, "r+"))
			;
		else
			xsUnknownError("invalid mode");
	}

	if (xsmcHas(xsArg(0), xsID_pebble)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_pebble);
		d.isPebbleNative = xsmcTest(xsVar(0));
	}

	d.format = builtinInitializeFormat(the, kIOFormatBuffer);
	if ((kIOFormatInvalid == d.format) || ((kIOFormatBuffer != d.format) && d.isPebbleNative))
		xsRangeError("unsupported");

	throwIf(settings_file_open(&d.file, domain, 8192));

	xsResult = xsNewHostInstance(xsArg(1));
	xsmcSetHostChunk(xsResult, &d, sizeof(d));
}

void xs_directorystorage_delete(xsMachine *the)
{
	char key[64];
	xsmcToStringBuffer(xsArg(0), key, sizeof(key));

	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	if (d->readOnly)
		xsUnknownError("read-only");

	status_t err = settings_file_delete(&d->file, key, c_strlen(key));
	if (E_DOES_NOT_EXIST == err) err = S_SUCCESS;
	throwIf(err);
}

void xs_directorystorage_read(xsMachine *the)
{
	char key[64];
	xsmcToStringBuffer(xsArg(0), key, sizeof(key));

	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	uint8_t format = d->format;

	int size = settings_file_get_len(&d->file, key, c_strlen(key));
	if (0 == size)
		return;

	if (d->isPebbleNative) {
		void *buffer = xsmcSetArrayBuffer(xsResult, NULL, size);
		d = (xsDirectory)xsmcGetHostChunk(xsThis);
		throwIf(settings_file_get(&d->file, key, c_strlen(key), buffer, size));
		return;
	}

	uint8_t *buffer = c_malloc(size + 1);		// xsTry / xsCatch to clean 
	throwIf(settings_file_get(&d->file, key, c_strlen(key), buffer, size));
	if (format != buffer[0]) {
		c_free(buffer);
		xsTypeError("mismatch");
	}
	buffer[size] = 0;		// for convenience in string case

	switch (format) {
		case kIOFormatBuffer:
			if (xsmcArgc > 1) {
				void *data;
				xsUnsignedValue dataLength;

				xsmcGetBufferWritable(xsArg(1), &data, &dataLength);
				if ((int)dataLength < (size - 1))
					xsRangeError("buffer too small");
				c_memmove(data, buffer + 1, size - 1);
				xsmcSetInteger(xsResult, size - 1);
			}
			else {
				xsmcSetArrayBuffer(xsResult, buffer + 1, size - 1);
			}
			break;

		case kIOFormatString:
			xsmcSetString(xsResult, (char *)(buffer + 1));
			break;

			default:
			xsUnknownError("unexpected");
			break;
	}

	if (buffer)
		c_free(buffer);
}

void xs_directorystorage_write(xsMachine *the)
{
	char key[64];
	xsmcToStringBuffer(xsArg(0), key, sizeof(key));

	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	uint8_t format = d->format;

	if (d->readOnly)
		xsUnknownError("read-only");

	if (d->isPebbleNative) {
		void *data;
		xsUnsignedValue dataSize;
		xsmcGetBufferReadable(xsArg(1), &data, &dataSize);
		throwIf(settings_file_set(&d->file, key, c_strlen(key), data, dataSize));
		return;
	}

	uint8_t *buf = C_NULL;
	int err = 0;

	switch (format) {
		case kIOFormatBuffer: {
			void *data;
			xsUnsignedValue length;

			xsmcGetBufferReadable(xsArg(1), &data, &length);
			buf = c_malloc(length + 1);
			if (!buf) {
				err = -1;
				goto bail;
			}
			buf[0] = kIOFormatBuffer;
			c_memmove(&buf[1], data, length);
			err = settings_file_set(&d->file, key, c_strlen(key), buf, length + 1);
		} break;

		case kIOFormatString: {
			void *data;
			xsUnsignedValue length;

			data = xsmcToString(xsArg(1));
			length = c_strlen(data);
			buf = c_malloc(length + 1);
			if (!buf) {
				err = -1;
				goto bail;
			}
			buf[0] = kIOFormatString;
			c_memmove(&buf[1], data, length);
			d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);		// refresh as xsmcToString could move / close
			err = settings_file_set(&d->file, key, c_strlen(key), buf, length + 1);
		} break;

		default:
			xsUnknownError("unexpected");
			break;
	}

bail:
	if (buf)
		c_free(buf);

	throwIf(err);
}

void xs_directorystorage_format_get(xsMachine *the)
{
	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	builtinGetFormat(the, d->format);
}

void xs_directorystorage_format_set(xsMachine *the)
{
	static const uint8_t formats[] = {kIOFormatBuffer, kIOFormatString, kIOFormatInvalid};
	uint8_t format = builtinSetFormat(the), i = 0;

	while (kIOFormatInvalid != formats[i]) {
		if (formats[i++] == format) {
			xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
			if (d->isPebbleNative && (kIOFormatBuffer != format))
				xsRangeError("format must be buffer");
			d->format = format;
			return;
		}
	}

	xsRangeError("unsupported");
}

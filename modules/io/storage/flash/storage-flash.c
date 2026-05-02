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
#include "mc.xs.h"
#include "builtinCommon.h"

#if MOD_TASKS
	#include "semphr.h"

	static SemaphoreHandle_t gStorageMutex;

	#define storageTake() \
		do { \
			if (NULL == gStorageMutex) \
				gStorageMutex = xSemaphoreCreateMutex(); \
			xSemaphoreTake(gStorageMutex, portMAX_DELAY); \
		} while (0)
	#define storageGive() xSemaphoreGive(gStorageMutex)
#else
	#define storageTake()
	#define storageGive()
#endif

extern uint8_t _MODPREF_start;

#define kStorageStartOffset ((uintptr_t)&_MODPREF_start - (uintptr_t)kFlashStart)
#define kStorageMagic 0x81213141

#define kBufferSize (64)

#define kStorageFormatOffset 0x80

#define kPrefsTypeBoolean 1
#define kPrefsTypeInteger 2
#define kPrefsTypeString 3
#define kPrefsTypeBuffer 4

struct xsDirectoryRecord {
	uint8_t		format;
	uint8_t		readOnly;
	uint8_t		domainLength;
	char			domain[];
};
typedef struct xsDirectoryRecord xsDirectoryRecord;
typedef struct xsDirectoryRecord *xsDirectory;

struct xsStorageIteratorRecord {
	uint32_t	offset;
	char		keys[];
};
typedef struct xsStorageIteratorRecord xsStorageIteratorRecord;
typedef struct xsStorageIteratorRecord *xsStorageIterator;

/*
	flash helpers
*/

static void resetStorage(void);
static uint8_t findStorageBlock(uint32_t *offset);
static uint8_t findEntryOffset(const char *domain, const char *key, uint32_t *entryOffset, uint32_t *valueOffset, uint32_t *entrySize, uint8_t *buffer);
static int getEntryValueSize(const uint8_t *pref);
static uint8_t eraseEntry(const char *domain, const char *key, uint8_t *buffer);
static uint8_t writeEntry(const char *domain, const char *key, uint8_t format, const uint8_t *value, uint16_t byteCount);

static void resetStorage(void)
{
	uint32_t magic = kStorageMagic;

	modSPIErase(kStorageStartOffset, kFlashSectorSize << 1);
	modSPIWrite(kStorageStartOffset, sizeof(magic), (uint8_t *)&magic);
}

static uint8_t findStorageBlock(uint32_t *offset)
{
	uint32_t magic;

	modSPIRead(kStorageStartOffset, sizeof(magic), (uint8_t *)&magic);
	if (kStorageMagic == magic) {
		*offset = kStorageStartOffset;
		return 1;
	}

	modSPIRead(kStorageStartOffset + kFlashSectorSize, sizeof(magic), (uint8_t *)&magic);
	if (kStorageMagic == magic) {
		*offset = kStorageStartOffset + kFlashSectorSize;
		return 1;
	}

	resetStorage();

	return 0;
}

static int getEntryValueSize(const uint8_t *pref)
{
	uint8_t type = *pref;

	switch (type) {
		case kPrefsTypeBoolean: return 1 + 1;
		case kPrefsTypeInteger: return 1 + 4;
		case kPrefsTypeString:  return 1 + c_strlen((char *)pref + 1) + 1;
		case kPrefsTypeBuffer:  return 1 + 2 + c_read16(pref + 1);
	}

	type -= kStorageFormatOffset;
	switch (type) {
		case kIOFormatUint8:  case kIOFormatInt8:   return 1 + 1;
		case kIOFormatUint16: case kIOFormatInt16:  return 1 + 2;
		case kIOFormatUint32: case kIOFormatInt32:  return 1 + 4;
		case kIOFormatUint64: case kIOFormatInt64:  return 1 + 8;
		case kIOFormatString: case kIOFormatBuffer: return 1 + 2 + c_read16(pref + 1);
	}

	return kFlashSectorSize;
}

static uint8_t findEntryOffset(const char *domain, const char *key, uint32_t *entryOffset, uint32_t *valueOffset, uint32_t *entrySize, uint8_t *buffer)
{
	uint32_t offset, endOffset;

	if (!findStorageBlock(&offset)) {
		if (domain && key)
			return 0;
	}

	endOffset = offset + kFlashSectorSize;
	offset += sizeof(uint32_t);
	while (offset < endOffset) {
		uint8_t *b = buffer;
		uint8_t match;
		uint32_t valueSize;
		uint32_t use = endOffset - offset;
		if (use > kBufferSize)
			use = kBufferSize;

		modSPIRead(offset, use, buffer);
		while (use) {
			if (0xff == *b) {
				if (!domain && !key) {
					*entryOffset = offset;
					return 1;
				}
				return 0;
			}

			if (*b)
				break;

			b += 1;
			use -= 1;
			offset += 1;
		}
		if (0 == use) continue;

		*entryOffset = offset;
		modSPIRead(offset, kBufferSize, buffer);
		match = domain && (0 == c_strcmp((char *)buffer, domain));
		offset += c_strlen((char *)buffer) + 1;

		modSPIRead(offset, kBufferSize, buffer);
		if (match)
			match = 0 == c_strcmp((char *)buffer, key);
		offset += c_strlen((char *)buffer) + 1;
		*valueOffset = offset;

		modSPIRead(offset, kBufferSize, buffer);
		valueSize = getEntryValueSize(buffer);
		if (match && (*buffer >= kStorageFormatOffset))
			match = 1;
		else if (match)
			match = 0;
		offset += valueSize;
		offset = (offset + 3) & ~3;
		*entrySize = offset - *entryOffset;
		if (match)
			return 1;
	}

	if (!domain && !key) {
		*entryOffset = offset - 1;
		return 1;
	}

	return 0;
}

static uint8_t eraseEntry(const char *domain, const char *key, uint8_t *buffer)
{
	uint32_t entryOffset, valueOffset, entrySize, offset;

	if (!findEntryOffset(domain, key, &entryOffset, &valueOffset, &entrySize, buffer))
		return 1;

	c_memset(buffer, 0, kBufferSize);
	offset = entryOffset;
	while (entrySize) {
		int use = (entrySize > kBufferSize) ? kBufferSize : entrySize;
		if (!modSPIWrite(offset, use, buffer))
			return 0;
		offset += use;
		entrySize -= use;
	}

	return 1;
}

static uint8_t writeEntry(const char *domain, const char *key, uint8_t format, const uint8_t *value, uint16_t byteCount)
{
	uint8_t buffer[kBufferSize * 2] __attribute__((aligned(4))), *p = buffer;
	uint32_t prefSize, prefsEnd, valueOffset, entrySize, prefsFree;
	uint8_t isVariable = (kIOFormatBuffer == format) || (kIOFormatString == format);

	prefSize = (c_strlen(domain) + 1) + (c_strlen(key) + 1) + 1 + byteCount + (isVariable ? 2 : 0);
	prefSize = (prefSize + 3) & ~3;
	if ((prefSize > sizeof(buffer)) || (byteCount > 63))
		return 0;

	storageTake();

	if (!eraseEntry(domain, key, buffer))
		goto fail;

	if (!findEntryOffset(NULL, NULL, &prefsEnd, &valueOffset, &entrySize, buffer))
		goto fail;

	prefsFree = kFlashSectorSize - (prefsEnd & (kFlashSectorSize - 1));
	if (prefsFree < prefSize) {
		uint32_t srcOffset = prefsEnd & ~(kFlashSectorSize - 1);
		uint32_t dstOffset = kStorageStartOffset + ((srcOffset == kStorageStartOffset) ? kFlashSectorSize : 0);
		uint32_t srcOffsetSave = srcOffset;

		if (!modSPIErase(dstOffset, kFlashSectorSize))
			goto fail;

		*(uint32_t *)buffer = kStorageMagic;
		modSPIWrite(dstOffset, sizeof(uint32_t), buffer);
		dstOffset += sizeof(uint32_t);
		srcOffset += sizeof(uint32_t);

		while (srcOffset < prefsEnd) {
			uint8_t *b = buffer;
			uint32_t compactEntrySize;
			uint32_t use = prefsEnd - srcOffset;
			if (use > sizeof(buffer))
				use = sizeof(buffer);

			modSPIRead(srcOffset, use, buffer);
			while (use--) {
				if (0xff == *b)
					break;

				if (0 == *b) {
					srcOffset += 1;
					b += 1;
					continue;
				}

				modSPIRead(srcOffset, sizeof(buffer), buffer);
				b = buffer;
				compactEntrySize = c_strlen((char *)b) + 1;
				b += compactEntrySize;
				compactEntrySize += c_strlen((char *)b) + 1;
				compactEntrySize += getEntryValueSize(buffer + compactEntrySize);
				compactEntrySize = (compactEntrySize + 3) & ~3;
				modSPIWrite(dstOffset, compactEntrySize, buffer);
				dstOffset += compactEntrySize;
				srcOffset += compactEntrySize;
				break;
			}
		}

		*(uint32_t *)buffer = 0;
		modSPIWrite(srcOffsetSave, sizeof(uint32_t), buffer);

		if (!findEntryOffset(NULL, NULL, &prefsEnd, &valueOffset, &entrySize, buffer))
			goto fail;

		prefsFree = kFlashSectorSize - (prefsEnd & (kFlashSectorSize - 1));
		if (prefsFree < prefSize)
			goto fail;
	}

	c_memset(buffer, 0, prefSize);
	c_strcpy((char *)p, domain);
	p += c_strlen(domain) + 1;
	c_strcpy((char *)p, key);
	p += c_strlen(key) + 1;
	*p++ = format + kStorageFormatOffset;
	if (isVariable) {
		p[0] = (uint8_t)byteCount;
		p[1] = (uint8_t)(byteCount >> 8);
		p += 2;
	}
	c_memcpy(p, value, byteCount);

	storageGive();
	return modSPIWrite(prefsEnd, prefSize, buffer);

fail:
	storageGive();
	return 0;
}

/*
	Storage Iterator
*/

void xs_storageIterator_constructor(xsMachine *the)
{
	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsArg(0), xs_directorystorage_destructor);
	uint8_t buffer[kBufferSize] __attribute__((aligned(4)));
	const char *domain = d->domain;
	xsStorageIterator iterator = C_NULL;

	storageTake();

	uint32_t startOffset;
	if (!findStorageBlock(&startOffset)) {
		storageGive();
		(void)xsmcSetHostChunk(xsThis, C_NULL, sizeof(xsStorageIteratorRecord) + 1);
		return;
	}

	for (int pass = 0; pass < 2; pass++) {
		uint32_t bytesNeeded = 0, keyOffset = 0;
		uint32_t offset = startOffset + sizeof(uint32_t);
		uint32_t endOffset = startOffset + kFlashSectorSize;

		while (offset < endOffset) {
			uint8_t *b = buffer;
			uint32_t use = endOffset - offset;
			if (use > kBufferSize)
				use = kBufferSize;

			modSPIRead(offset, use, buffer);
			while (use) {
				if (0xff == *b)
					goto passDone;

				if (*b)
					break;

				b += 1;
				use -= 1;
				offset += 1;
			}
			if (0 == use) continue;

			modSPIRead(offset, kBufferSize, buffer);
			uint8_t match = (0 == c_strcmp((char *)buffer, domain));
			offset += c_strlen((char *)buffer) + 1;

			modSPIRead(offset, kBufferSize, buffer);
			uint32_t keyLen = c_strlen((char *)buffer) + 1;
			if (match) {
				if (pass)
					c_strcpy(iterator->keys + keyOffset, (char *)buffer);
				offset += keyLen;

				modSPIRead(offset, kBufferSize, buffer);
				if (*buffer >= kStorageFormatOffset) {
					bytesNeeded += keyLen;
					keyOffset += keyLen;
				}
				else if (pass)
					c_memset(iterator->keys + keyOffset, 0, keyLen);
				offset += getEntryValueSize(buffer);
			}
			else {
				offset += keyLen;
				modSPIRead(offset, kBufferSize, buffer);
				offset += getEntryValueSize(buffer);
			}
			offset = (offset + 3) & ~3;
		}

	passDone:
		if (0 == pass) {
			iterator = xsmcSetHostChunk(xsThis, C_NULL, sizeof(xsStorageIteratorRecord) + bytesNeeded + 1);
			d = (xsDirectory)xsmcGetHostChunkValidate(xsArg(0), xs_directorystorage_destructor);
			domain = d->domain;
		}
		else
			iterator->keys[keyOffset] = 0;
	}

	storageGive();
}

void xs_storageIterator_destructor(void *data)
{
}

void xs_storageIterator_next(xsMachine *the)
{
	xsStorageIterator iterator = xsmcGetHostChunkValidate(xsThis, xs_storageIterator_destructor);
	char *key = iterator->keys + iterator->offset;

	xsmcVars(2);
	if (*key) {
		int keyLength = c_strlen(key);
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
	if (kIOFormatInvalid == d.format)
		xsRangeError("unsupported");

	length = c_strlen(domain);
	if (length > 31) xsUnknownError("too long");
	d.domainLength = (uint8_t)length;

	xsResult = xsNewHostInstance(xsArg(1));
	xsDirectory dp = xsmcSetHostChunk(xsResult, &d, sizeof(xsDirectoryRecord) + length + 1);
	c_strcpy(dp->domain, domain);
}

void xs_directorystorage_delete(xsMachine *the)
{
	char key[64];
	xsmcToStringBuffer(xsArg(0), key, sizeof(key));

	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	if (d->readOnly)
		xsUnknownError("read-only");

	storageTake();

	uint8_t buffer[kBufferSize] __attribute__((aligned(4)));
	uint8_t success = eraseEntry(d->domain, key, buffer);

	storageGive();

	if (!success)
		xsUnknownError("delete failed");
}

void xs_directorystorage_read(xsMachine *the)
{
	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	uint8_t format = d->format;
	char domain[64], key[64];

	c_strcpy(domain, d->domain);
	xsmcToStringBuffer(xsArg(0), key, sizeof(key));

	uint32_t entryOffset, valueOffset, entrySize;
	uint8_t buffer[kBufferSize] __attribute__((aligned(4)));

	storageTake();

	if (!findEntryOffset(domain, key, &entryOffset, &valueOffset, &entrySize, buffer)) {
		storageGive();
		return;
	}

	uint8_t storedFormat = buffer[0] - kStorageFormatOffset;
	if (storedFormat != format) {
		storageGive();
		xsTypeError("mismatch");
	}

	switch (format) {
		case kIOFormatBuffer: {
			uint16_t length = c_read16(buffer + 1);
			if (length <= (kBufferSize - 3)) {
				if (xsmcArgc > 1) {
					void *data;
					xsUnsignedValue dataLength;

					storageGive();
					xsmcGetBufferWritable(xsArg(1), &data, &dataLength);
					if (dataLength < length)
						xsRangeError("buffer too small");
					c_memcpy(data, buffer + 3, length);
					xsmcSetInteger(xsResult, length);
				}
				else {
					storageGive();
					xsmcSetArrayBuffer(xsResult, buffer + 3, length);
				}
			}
			else {
				uint8_t *value = c_malloc(length);
				if (C_NULL == value) {
					storageGive();
					xsUnknownError("no memory");
				}
				modSPIRead(valueOffset + 3, length, value);
				storageGive();

				if (xsmcArgc > 1) {
					void *data;
					xsUnsignedValue dataLength;

					xsmcGetBufferWritable(xsArg(1), &data, &dataLength);
					if (dataLength < length) {
						c_free(value);
						xsRangeError("buffer too small");
					}
					c_memcpy(data, value, length);
					c_free(value);
					xsmcSetInteger(xsResult, length);
				}
				else {
					xsmcSetArrayBuffer(xsResult, value, length);
					c_free(value);
				}
			}
			} break;

		case kIOFormatString: {
			uint16_t length = c_read16(buffer + 1);
			if (length <= (kBufferSize - 3)) {
				storageGive();
				xsmcSetStringBuffer(xsResult, (char *)buffer + 3, length);
			}
			else {
				uint8_t *value = c_malloc(length + 1);
				if (C_NULL == value) {
					storageGive();
					xsUnknownError("no memory");
				}
				modSPIRead(valueOffset + 3, length, value);
				value[length] = 0;
				storageGive();
				xsmcSetString(xsResult, (char *)value);
				c_free(value);
			}
			} break;

		case kIOFormatUint8:
			storageGive();
			xsmcSetInteger(xsResult, c_read8(buffer + 1));
			break;

		case kIOFormatInt8:
			storageGive();
			xsmcSetInteger(xsResult, (int8_t)c_read8(buffer + 1));
			break;

		case kIOFormatUint16:
			storageGive();
			xsmcSetInteger(xsResult, c_read16(buffer + 1));
			break;

		case kIOFormatInt16:
			storageGive();
			xsmcSetInteger(xsResult, (int16_t)c_read16(buffer + 1));
			break;

		case kIOFormatUint32:
			storageGive();
			xsmcSetUnsigned(xsResult, c_read32(buffer + 1));
			break;

		case kIOFormatInt32:
			storageGive();
			xsmcSetInteger(xsResult, (int32_t)c_read32(buffer + 1));
			break;

		case kIOFormatInt64: {
			int64_t value;
			c_memcpy(&value, buffer + 1, 8);
			storageGive();
			fxFromBigInt64(the, &xsResult, value);
			} break;

		case kIOFormatUint64: {
			uint64_t value;
			c_memcpy(&value, buffer + 1, 8);
			storageGive();
			fxFromBigUint64(the, &xsResult, value);
			} break;

		default:
			storageGive();
			xsUnknownError("unexpected");
			break;
	}
}

void xs_directorystorage_write(xsMachine *the)
{
	xsDirectory d = (xsDirectory)xsmcGetHostChunkValidate(xsThis, xs_directorystorage_destructor);
	uint8_t format = d->format;
	char domain[64];
	char key[64];

	if (d->readOnly)
		xsUnknownError("read-only");

	c_strcpy(domain, d->domain);
	xsmcToStringBuffer(xsArg(0), key, sizeof(key));

	uint8_t success;

	switch (format) {
		case kIOFormatBuffer: {
			void *data;
			xsUnsignedValue length;

			xsmcGetBufferReadable(xsArg(1), &data, &length);
			success = writeEntry(domain, key, format, data, (uint16_t)length);
			} break;

		case kIOFormatString: {
			char *str = xsmcToString(xsArg(1));
			success = writeEntry(domain, key, format, (uint8_t *)str, c_strlen(str));
			} break;

		case kIOFormatUint8: {
			uint8_t value = (uint8_t)xsmcToInteger(xsArg(1));
			success = writeEntry(domain, key, format, &value, 1);
			} break;

		case kIOFormatInt8: {
			int8_t value = (int8_t)xsmcToInteger(xsArg(1));
			success = writeEntry(domain, key, format, (uint8_t *)&value, 1);
			} break;

		case kIOFormatUint16: {
			uint16_t value = (uint16_t)xsmcToInteger(xsArg(1));
			success = writeEntry(domain, key, format, (uint8_t *)&value, 2);
			} break;

		case kIOFormatInt16: {
			int16_t value = (int16_t)xsmcToInteger(xsArg(1));
			success = writeEntry(domain, key, format, (uint8_t *)&value, 2);
			} break;

		case kIOFormatUint32: {
			uint32_t value = xsToUnsigned(xsArg(1));
			success = writeEntry(domain, key, format, (uint8_t *)&value, 4);
			} break;

		case kIOFormatInt32: {
			int32_t value = (int32_t)xsmcToInteger(xsArg(1));
			success = writeEntry(domain, key, format, (uint8_t *)&value, 4);
			} break;

		case kIOFormatInt64: {
			int64_t value = (int64_t)fxToBigInt64(the, &xsArg(1));
			success = writeEntry(domain, key, format, (uint8_t *)&value, 8);
			} break;

		case kIOFormatUint64: {
			uint64_t value = (uint64_t)fxToBigUint64(the, &xsArg(1));
			success = writeEntry(domain, key, format, (uint8_t *)&value, 8);
			} break;

		default:
			xsUnknownError("unexpected");
			return;
	}

	if (!success)
		xsUnknownError("write failed");
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

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
#include "mc.xs.h"
#include "xsHost.h"

/*
	zip record declarations based on FskZip.c from KinomaJS

	https://github.com/Kinoma/kinomajs/blob/master/extensions/fsZip/sources/FskZip.c
*/


#if defined(__clang__)
	#pragma clang diagnostic ignored "-Waddress-of-packed-member"
#endif

#pragma pack(2)

#if mxWindows
	#define PACKED
#else
	#define PACKED __attribute__((packed))
#endif

typedef struct {
	uint32_t		signature;
	uint16_t		versionMade;
	uint16_t		versionNeeded;
	uint16_t		flags;
	uint16_t		compressionMethod;
	uint16_t		modTime;
	uint16_t		modDate;
	uint32_t		crc32;
	uint32_t		compressedSize;
	uint32_t		uncompressedSize;
	uint16_t		fileNameLen;
	uint16_t		extraFieldLen;
	uint16_t		commentLen;
	uint16_t		diskNumber;
	uint16_t		internalFileAttributes;
	uint32_t		externalFileAttributes;
	uint32_t		relativeOffset;
} PACKED zipCentralDirectoryFileRecord, *zipCentralDirectoryFile;

typedef struct {
	uint32_t		signature;
	uint16_t		numberOfThisDisk;
	uint16_t		numberOfDiskWithCentralDir;
	uint16_t		centralDirectoryCountOnThisDisk;
	uint16_t		centralDirectoryCount;
	uint32_t		centralDirectorySize;
	uint32_t		centralDirectoryOffset;
	uint16_t		commentLength;
} PACKED zipEndOfCentralDirectoryRecord, *zipEndOfCentralDirectory;

typedef struct {
	uint32_t		signature;
	uint16_t		versionNeeded;
	uint16_t		flags;
	uint16_t		compressionMethod;
	uint16_t		modTime;
	uint16_t		modDate;
	uint32_t		crc32;
	uint32_t		compressedSize;
	uint32_t		uncompressedSize;
	uint16_t		fileNameLen;
	uint16_t		extraFieldLen;
} PACKED zipLocalFileHeaderRecord, *zipLocalFileHeader;

#pragma pack()

/*
	in-memory zip utilities
*/

typedef struct {
	const uint8_t					*data;
	uint32_t						dataSize;

	zipCentralDirectoryFile			cd;
	uint32_t						cdCount;
} ZipRecord, *Zip;

static uint8_t ZipOpen(Zip *zip, const uint8_t *data, uint32_t dataSize);
static void ZipClose(Zip zip);
static zipLocalFileHeader ZipFindFile(Zip zip, const char *path, uint8_t **data, uint32_t *dataSize, uint32_t *crc, uint16_t *compressionMethod);

uint8_t ZipOpen(Zip *zipOut, const uint8_t *data, uint32_t dataSize)
{
	Zip zip = NULL;
	const uint8_t *scan;
	zipEndOfCentralDirectory ecd = NULL;
	zipCentralDirectoryFile cd;
	uint32_t cdCount;

	*zipOut = NULL;

	// scan backwards for the end of central directory by looking for 'P' 'K' 0x05 0x06 signature
	for (scan = data + dataSize - sizeof(zipEndOfCentralDirectoryRecord); scan >= data; scan -= 1) {
		if (('P' == c_read8(scan + 0)) && ('K' == c_read8(scan + 1)) &&
			(0x05 == c_read8(scan + 2)) && (0x06 == c_read8(scan + 3))) {
			ecd = (zipEndOfCentralDirectory)scan;
			break;
		}
	}

	if (NULL == ecd)
		return 0;

	if (0 != c_read16(&ecd->numberOfThisDisk))
		return 0;

	cdCount = c_read16(&ecd->centralDirectoryCount);
	cd = (zipCentralDirectoryFile)(data + c_read32(&ecd->centralDirectoryOffset));


	if (('P' != c_read8(((char *)cd) + 0)) || ('K' != c_read8(((char *)cd) + 1)) ||
		(0x01 != c_read8(((char *)cd) + 2)) || (0x02 != c_read8(((char *)cd) + 3)))
		return 0;

	zip = c_malloc(sizeof(ZipRecord));
	if (!zip) return 0;

	zip->data = data;
	zip->dataSize = dataSize;
	zip->cd = cd;
	zip->cdCount = cdCount;

	*zipOut = zip;

	return 1;
}

void ZipClose(Zip zip)
{
	if (!zip) return;

	c_free(zip);
}

zipLocalFileHeader ZipFindFile(Zip zip, const char *path, uint8_t **data, uint32_t *dataSize, uint32_t *crc32, uint16_t *compressionMethod)
{
	const char *walker = (const char *)zip->cd;
	uint32_t i;
	size_t pathLen = c_strlen(path);

	for (i = 0; i < zip->cdCount; i++) {
		zipCentralDirectoryFile item = (zipCentralDirectoryFile)walker;
		uint16_t fileNameLen = c_read16(&item->fileNameLen);

		if ((fileNameLen == pathLen) &&
			!c_strncmp(walker + sizeof(zipCentralDirectoryFileRecord), path, pathLen)) {
			zipLocalFileHeader lf = (zipLocalFileHeader)(zip->data + c_read32(&item->relativeOffset));

			*data = ((uint8_t *)lf) + sizeof(zipLocalFileHeaderRecord) +
						c_read16(&lf->fileNameLen) + c_read16(&lf->extraFieldLen);

			if (0 == c_read16(&lf->flags)) {		//@@ this check may not be quite right
				*dataSize = c_read32(&lf->compressedSize);
				*crc32 = c_read32(&lf->crc32);
				*compressionMethod = c_read16(&lf->compressionMethod);
			}
			else {
				*dataSize = c_read32(&item->compressedSize);
				*crc32 = c_read32(&item->crc32);
				*compressionMethod = c_read16(&item->compressionMethod);
			}

			return lf;
		}

		walker += sizeof(zipCentralDirectoryFileRecord) +
					fileNameLen + c_read16(&item->extraFieldLen) +
					c_read16(&item->commentLen);
	}

	return NULL;
}

/*
	XS bindings
*/

void xs_zip_destructor(void *data)
{
	ZipClose((Zip)data);
}

void xs_zip(xsMachine *the)
{
	uint8_t *data;
	xsUnsignedValue dataSize;
	Zip zip;

	if (xsBufferRelocatable == xsmcGetBufferReadable(xsArg(0), (void **)&data, &dataSize))
		xsUnknownError("invalild");

	xsmcSet(xsThis, xsID_buffer, xsArg(0));

	if (0 == ZipOpen(&zip, data, dataSize))
		xsErrorPrintf("can't handle zip archive");

	xsmcSetHostData(xsThis, zip);
}

void xs_zip_file_map(xsMachine *the)
{
	Zip zip = xsmcGetHostData(xsThis);
	char *path = xsmcToString(xsArg(0));
	uint8_t *data;
	uint32_t dataSize, crc32;
	uint16_t compressionMethod;

	if (NULL == ZipFindFile(zip, path, &data, &dataSize, &crc32, &compressionMethod))
		xsErrorPrintf("can't find path");

	xsResult = xsNewHostObject(NULL);
	xsmcSetHostBuffer(xsResult, data, dataSize);
	xsmcVars(1);
	xsVar(0) = xsInteger(dataSize);
	xsmcDefine(xsResult, xsID_byteLength, xsVar(0), xsDefault);
}

typedef struct {
	uint8_t				*data;
	uint32_t			dataSize;
	uint32_t			crc32;
	uint16_t			compressionMethod;
	uint32_t			position;
} xsZipFileRecord, *xsZipFile;

void xs_zip_file_destructor(void *data)
{
	if (data)
		c_free(data);
}

void xs_zip_File(xsMachine *the)
{
	Zip zip = xsmcGetHostData(xsArg(0));
	char *path;
	uint8_t *data;
	uint32_t dataSize, crc32;
	uint16_t compressionMethod;
	xsZipFile zf;

	xsmcSet(xsThis, xsID_ZIP, xsArg(0));

	path = xsmcToString(xsArg(1));
	if (NULL == ZipFindFile(zip, path, &data, &dataSize, &crc32, &compressionMethod))
		xsErrorPrintf("can't find path");

	zf = c_malloc(sizeof(xsZipFileRecord));
	if (!zf)
		xsErrorPrintf("out of memory");

	zf->data = data;
	zf->dataSize = dataSize;
	zf->crc32 = crc32;
	zf->compressionMethod = compressionMethod;
	zf->position = 0;

	xsmcSetHostData(xsThis, zf);
}

void xs_zip_file_read(xsMachine *the)
{
	int argc = xsmcArgc;
	int dstLen;
	xsSlot *s1, *s2;
	xsZipFile zf = xsmcGetHostData(xsThis);
	if (NULL == zf)
		xsErrorPrintf("file closed");

	dstLen = zf->dataSize - zf->position;
	if (argc > 1) {
		int requestedLen = xsmcToInteger(xsArg(1));
		if (requestedLen <= 0)
			xsErrorPrintf("invalid length");

		if (requestedLen < dstLen)
			dstLen = requestedLen;
	}

	s1 = &xsArg(0);

	xsmcVars(1);
	xsmcGet(xsVar(0), xsGlobal, xsID_String);
	s2 = &xsVar(0);
	if (s1->data[2] == s2->data[2])
		xsResult = xsStringBuffer((char *)zf->data + zf->position, dstLen);
	else
		xsmcSetArrayBuffer(xsResult, zf->data + zf->position, dstLen);

	zf->position += dstLen;
}

void xs_zip_file_close(xsMachine *the)
{
	xs_zip_file_destructor(xsmcGetHostData(xsThis));
	xsmcSetHostData(xsThis, NULL);
}

void xs_zip_file_get_length(xsMachine *the)
{
	xsZipFile zf = xsmcGetHostData(xsThis);
	if (NULL == zf)
		xsErrorPrintf("file closed");

	xsResult = xsInteger(zf->dataSize);
}

void xs_zip_file_get_position(xsMachine *the)
{
	xsZipFile zf = xsmcGetHostData(xsThis);
	if (NULL == zf)
		xsErrorPrintf("file closed");

	xsResult = xsInteger(zf->position);
}

void xs_zip_file_set_position(xsMachine *the)
{
	uint32_t position;
	xsZipFile zf = xsmcGetHostData(xsThis);
	if (NULL == zf)
		xsErrorPrintf("file closed");

	position = (uint32_t)xsmcToInteger(xsArg(0));
	if (position >= zf->position)
		xsErrorPrintf("invalid position");

	zf->position = position;
}

void xs_zip_file_get_method(xsMachine *the)
{
	xsZipFile zf = xsmcGetHostData(xsThis);
	if (NULL == zf)
		xsErrorPrintf("file closed");

	xsResult = xsInteger(zf->compressionMethod);
}

void xs_zip_file_get_crc(xsMachine *the)
{
	xsZipFile zf = xsmcGetHostData(xsThis);
	if (NULL == zf)
		xsErrorPrintf("file closed");

	xsResult = xsNumber(c_read32(&zf->crc32));
}

typedef struct {
	Zip						zip;
	zipCentralDirectoryFile cdf;
	int						cdRemain;
	size_t					pathLen;
	char					path[1];
} xsZipFileIteratorRecord, *xsZipFileIterator;

void xs_zip_file_iterator_destructor(void *data)
{
	if (data)
		c_free(data);
}

void xs_zip_file_Iterator(xsMachine *the)
{
	Zip zip = xsmcGetHostData(xsArg(0));
	char *path;
	size_t pathLen;
	xsZipFileIterator zfi;

	xsmcSet(xsThis, xsID_ZIP, xsArg(0));

	path = xsmcToString(xsArg(1));
	pathLen = c_strlen(path);

	if ('/' != c_read8(path + pathLen - 1))
		xsErrorPrintf("directory path must end with /");

	if ('/' == c_read8(path)) {
		if (1 != pathLen)
			xsErrorPrintf("only root directory path can begin with /");
		path += 1;
		pathLen -= 1;
	}

	zfi = c_malloc(sizeof(xsZipFileIteratorRecord) - 1 + pathLen);
	if (!zfi)
		xsErrorPrintf("out of memory");

	zfi->zip = zip;
	zfi->cdf = zip->cd;
	zfi->cdRemain = zip->cdCount;
	zfi->pathLen = c_strlen(path);
	c_memcpy(zfi->path, path, pathLen + 1);

	xsmcSetHostData(xsThis, zfi);
}

void xs_zip_file_iterator_next(xsMachine *the)
{
	xsZipFileIterator zfi = xsmcGetHostData(xsThis);

	xsmcVars(1);
	while (zfi->cdRemain) {
		zipCentralDirectoryFile item = zfi->cdf;
		uint16_t fileNameLen = c_read16(&item->fileNameLen);
		char *itemPath = ((char *)item) + sizeof(zipCentralDirectoryFileRecord);
		uint8_t ok;
		uint16_t i;

		// skip past this item
		zfi->cdf = (zipCentralDirectoryFile)(((char *)item) + sizeof(zipCentralDirectoryFileRecord) +
					fileNameLen + c_read16(&item->extraFieldLen) +
					c_read16(&item->commentLen));
		zfi->cdRemain -= 1;

		// see if there is a hit
		if (fileNameLen <= zfi->pathLen)
			continue;

		if (0 != c_strncmp(itemPath, zfi->path, zfi->pathLen))
			continue;

		itemPath += zfi->pathLen;
		fileNameLen -= zfi->pathLen;

		if ('/' == c_read8(itemPath + fileNameLen - 1)) {
			// directory
			xsResult = xsmcNewObject();
			xsmcSetStringBuffer(xsVar(0), itemPath, fileNameLen - 1);
			xsmcSet(xsResult, xsID_name, xsVar(0));
			return;
		}

		for (i = 0, ok = 1; i < fileNameLen; i++) {
			if ('/' == c_read8(itemPath + i))
				ok = 0;
		}
		if (!ok) continue;

		// file
		xsResult = xsmcNewObject();
		xsmcSetStringBuffer(xsVar(0), itemPath, fileNameLen);
		xsmcSet(xsResult, xsID_name, xsVar(0));
		xsmcSetInteger(xsVar(0), c_read32(&item->compressedSize));
		xsmcSet(xsResult, xsID_length, xsVar(0));
		return;
	}
}


/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
#include "modInstrumentation.h"
#include "mc.xs.h"			// for xsID_ values

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>

#define PATH_MAX 1024

typedef struct {
	HANDLE hFind;
	WIN32_FIND_DATA ffd;
	char path[1];
} iteratorRecord, *iter;

static FILE* getFile(xsMachine* the)
{
	FILE* result = xsmcGetHostData(xsThis);
	if (!result)
		xsUnknownError("closed");
	return result;
}

void xs_file_destructor(void *data)
{
	if (data) {
		fclose((FILE*)data);
		modInstrumentationAdjust(Files, -1);
	}
}

void xs_File(xsMachine *the)
{
	int argc = xsmcArgc;
	FILE *file;
	char *path;
	uint8_t write = (argc < 2) ? 0 : xsmcToBoolean(xsArg(1));

	path = xsmcToString(xsArg(0));
	file = fopen(path, write ? "rb+" : "rb");
	if (NULL == file) {
		if (write)
			file = fopen(path, "wb+");
		if (NULL == file)
			xsUnknownError("file not found");
	}
	xsmcSetHostData(xsThis, (void *)((uintptr_t)file));
	
	modInstrumentationAdjust(Files, +1);
}

void xs_file_read(xsMachine *the)
{
	FILE* file = getFile(the);
	int32_t result;
	int argc = xsmcArgc;
	int dstLen = (argc < 2) ? -1 : xsmcToInteger(xsArg(1));
	void *dst;
	xsSlot *s1, *s2;
	struct stat buf;
	int32_t position = ftell(file);

	fstat(_fileno(file), &buf);
	if ((-1 == dstLen) || (buf.st_size < (position + dstLen))) {
		if (position >= buf.st_size)
			xsUnknownError("read past end of file");
		dstLen = buf.st_size - position;
	}

	s1 = &xsArg(0);

	xsmcVars(1);
	xsmcGet(xsVar(0), xsGlobal, xsID_String);
	s2 = &xsVar(0);
	if (s1->data[2] == s2->data[2]) {
		xsResult = xsStringBuffer(NULL, dstLen);
		dst = xsmcToString(xsResult);
	}
	else {
		xsmcSetArrayBuffer(xsResult, NULL, dstLen);
		dst = xsmcToArrayBuffer(xsResult);
	}

	result = fread(dst, 1, dstLen, file);
	if (result != dstLen)
		xsUnknownError("file read failed");
}

void xs_file_write(xsMachine *the)
{
	FILE* file = getFile(the);
	int32_t result;
	int argc = xsmcArgc, i;

	for (i = 0; i < argc; i++) {
		uint8_t* src;
		int32_t srcLen;
		int type = xsmcTypeOf(xsArg(i));
		uint8_t temp;

		if (xsStringType == type) {
			src = xsmcToString(xsArg(i));
			srcLen = c_strlen(src);
		}
		else if ((xsIntegerType == type) || (xsNumberType == type)) {
			temp = (uint8_t)xsmcToInteger(xsArg(i));
			src = &temp;
			srcLen = 1;
		}
		else {
			src = xsmcToArrayBuffer(xsArg(i));
			srcLen = xsmcGetArrayBufferLength(xsArg(i));
		}

		result = fwrite(src, 1, srcLen, file);
		if (result != srcLen)
			xsUnknownError("file write failed");
	}
	result = fflush(file);
	if (0 != result)
		xsUnknownError("file flush failed");
}

void xs_file_close(xsMachine *the)
{
	FILE* file = getFile(the);
	xs_file_destructor((void*)((uintptr_t)file));
	xsmcSetHostData(xsThis, (void*)NULL);
}

void xs_file_get_length(xsMachine *the)
{
	FILE* file = getFile(the);
	struct stat buf;

	fstat(_fileno(file), &buf);
	xsResult = xsInteger(buf.st_size);
}

void xs_file_get_position(xsMachine *the)
{
	FILE* file = getFile(the);
	int32_t position = ftell(file);
	xsResult = xsInteger(position);
}

void xs_file_set_position(xsMachine *the)
{
	FILE* file = getFile(the);
	int32_t position = xsmcToInteger(xsArg(0));
	fseek(file, position, SEEK_SET);
}

void xs_file_delete(xsMachine *the)
{
	int32_t result;
	char* path = xsmcToString(xsArg(0));

	result = _unlink(path);

	xsResult = xsBoolean(result == 0);
}

void xs_file_exists(xsMachine *the)
{
	struct stat buf;
	int32_t result;
	char* path = xsmcToString(xsArg(0));

	result = stat(path, &buf);

	xsResult = xsBoolean(result == 0);
}

void xs_file_rename(xsMachine *the)
{
	char* path;
	char toPath[PATH_MAX + 1];
	int result;

	xsmcToStringBuffer(xsArg(1), toPath, sizeof(toPath));
	path = xsmcToString(xsArg(0));
	if ('/' != toPath[0]) {
		if (c_strchr(toPath + 1, '/'))
			xsUnknownError("invalid to");

		char *slash = c_strrchr(path, '/');
		if (!slash)
			xsUnknownError("invalid from");

		size_t pathLength = slash - path + 1;
		if (pathLength >= (c_strlen(path) + sizeof(toPath)))
			xsUnknownError("path too long");

		c_strcpy(toPath, path);
		xsmcToStringBuffer(xsArg(1), toPath + pathLength, sizeof(toPath) - pathLength);
	}

	result = rename(path, toPath);
	xsResult = xsBoolean(result == 0);
}

void xs_directory_create(xsMachine *the)
{
	char *path = xsmcToString(xsArg(0));
	int result = _mkdir(path);
	if (result && (EEXIST != errno)){
		if (errno == ENOENT){
			xsUnknownError("path not found");
		}else{
			xsUnknownError("failed");
		}
	}
}

void xs_directory_delete(xsMachine *the)
{
	char *path = xsmcToString(xsArg(0));
	int result = _rmdir(path);
	if (result){
		switch (errno){
			case ENOTEMPTY:
				xsUnknownError("path is not a directory, is not empty, or is the current working directory");
				break;
			case ENOENT:
				xsUnknownError("path is invalid");
				break;
			case EACCES:
				xsUnknownError("a program has an open handle to path");
				break;
			default:
				xsUnknownError("failed");
		}
	}
}

void xs_file_iterator_destructor(void *data)
{
	iter d = data;

	if (d) {
		if ((NULL != d->hFind) && (INVALID_HANDLE_VALUE != d->hFind))
			FindClose(d->hFind);
		free(d);

		modInstrumentationAdjust(Files, -1);
	}
}

void xs_File_Iterator(xsMachine *the)
{
	iter d;
	int i;
	char *p;

	p = xsmcToString(xsArg(0));
	i = c_strlen(p);
	if (i == 0) {
		xsUnknownError("no directory to iterate on");
	}
	d = calloc(1, sizeof(iteratorRecord) + i + 3);
	strcpy(d->path, p);
	if (p[i - 1] != '\\')
		d->path[i] = '\\';
	c_strcat(d->path, "*");

	xsmcSetHostData(xsThis, d);

	modInstrumentationAdjust(Files, +1);
}

void xs_file_iterator_next(xsMachine *the)
{
	iter d = xsmcGetHostData(xsThis);

	while (true) {
		uint8_t done = false;

		if (NULL == d->hFind) {
			d->hFind = FindFirstFile(d->path, &d->ffd);
			if (INVALID_HANDLE_VALUE == d->hFind)
				done = true;
		}
		else {
			DWORD dwResult = FindNextFile(d->hFind, &d->ffd);
			if (0 == dwResult)
				done = true;
		}
		if (done) {
			xs_file_iterator_destructor(d);
			xsmcSetHostData(xsThis, NULL);
			return;
		}
		if ((0 == c_strcmp(d->ffd.cFileName, ".")) ||
			(0 == c_strcmp(d->ffd.cFileName, "..")) ||
			(0 != (FILE_ATTRIBUTE_HIDDEN & d->ffd.dwFileAttributes)))
			continue;
		break;
	}
	xsResult = xsmcNewObject();
	xsmcVars(1);
	xsmcSetString(xsVar(0), d->ffd.cFileName);
	xsmcSet(xsResult, xsID_name, xsVar(0));

	if (!(d->ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		xsmcSetInteger(xsVar(0), d->ffd.nFileSizeLow);	// @@
		xsmcSet(xsResult, xsID_length, xsVar(0));
	}
}

void xs_file_system_config(xsMachine *the)
{
	xsResult = xsmcNewObject();
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), PATH_MAX);
	xsmcSet(xsResult, xsID_maxPathLength, xsVar(0));
}

void xs_file_system_info(xsMachine *the)
{
	ULARGE_INTEGER freeSpace, totalSpace;
	ULONG usedSpace;
	GetDiskFreeSpaceEx(NULL, &freeSpace, &totalSpace, NULL);
	xsResult = xsmcNewObject();
	xsmcVars(1);

	// Stub implementation when values are > 32 bits
	if (0 == freeSpace.HighPart && 0 == totalSpace.HighPart) {
		usedSpace = totalSpace.LowPart - freeSpace.LowPart;
		xsmcSetInteger(xsVar(0), totalSpace.LowPart);
		xsmcSet(xsResult, xsID_total, xsVar(0));
	}
	else {
		usedSpace = -1;
		xsmcSetInteger(xsVar(0), -1);
		xsmcSet(xsResult, xsID_total, xsVar(0));
	}
	xsmcSetInteger(xsVar(0), usedSpace);
	xsmcSet(xsResult, xsID_used, xsVar(0));
}

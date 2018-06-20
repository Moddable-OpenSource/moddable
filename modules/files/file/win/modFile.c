/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
#include "mc.xs.h"			// for xsID_ values

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PATH_MAX 1024

typedef struct {
	HANDLE hFind;
	WIN32_FIND_DATA ffd;
	char path[1];
} iteratorRecord, *iter;

void xs_file_destructor(void *data)
{
	if (data && ((uintptr_t)-1 != (uintptr_t)data))
		fclose((FILE *)data);
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
			file = fopen(path, write ? "ab+" : "rb");
		if (NULL == file)
			xsUnknownError("file not found");
	}
	xsmcSetHostData(xsThis, (void *)((uintptr_t)file));
}

void xs_file_read(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	FILE *file = (FILE*)data;
	int32_t result;
	int argc = xsmcArgc;
	int dstLen = (argc < 2) ? -1 : xsmcToInteger(xsArg(1));
	void *dst;
	xsSlot *s1, *s2;
	struct stat buf;
	int fno;
	int32_t position = ftell(file);

	fno = _fileno(file);
	fstat(fno, &buf);
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
		xsResult = xsArrayBuffer(NULL, dstLen);
		dst = xsmcToArrayBuffer(xsResult);
	}

	result = fread(dst, 1, dstLen, file);
	if (result != dstLen)
		xsUnknownError("file read failed");
}

void xs_file_write(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	FILE *file = ((FILE *)data);
	int32_t result;
	int argc = xsmcArgc, i;

	for (i = 0; i < argc; i++) {
		uint8_t *src;
		int32_t srcLen;

		if (xsStringType == xsmcTypeOf(xsArg(i))) {
			src = (uint8_t *)xsmcToString(xsArg(i));
			srcLen = strlen((char *)src);
		}
		else {
			src = xsmcToArrayBuffer(xsArg(i));
			srcLen = xsGetArrayBufferLength(xsArg(i));
		}

		while (srcLen) {	// spool through RAM for data in flash
			unsigned char *buffer[128];
			int use = (srcLen <= (int)sizeof(buffer)) ? srcLen : 128;

			memcpy(buffer, src, use);
			src += use;
			srcLen -= use;

			result = fwrite(buffer, 1, use, file);
			if (result != use)
				xsUnknownError("file write failed");
			result = fflush(file);
			if (0 != result)
				xsUnknownError("file flush failed");
		}
	}
}

void xs_file_close(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	FILE *file = ((FILE*)data);
	xs_file_destructor((void *)((int)file));
	xsmcSetHostData(xsThis, (void *)NULL);
}

void xs_file_get_length(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	FILE *file = (FILE*)data;
	struct stat buf;
	int fno;

	fno = _fileno(file);
	fstat(fno, &buf);
	xsResult = xsInteger(buf.st_size);
}

void xs_file_get_position(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	FILE *file = (FILE*)data;
	int32_t position = ftell(file);
	xsResult = xsInteger(position);
}

void xs_file_set_position(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	FILE *file = ((FILE*)data);
	int32_t position = xsmcToInteger(xsArg(0));
	fseek(file, position, SEEK_SET);
}

void xs_file_delete(xsMachine *the)
{
	char path[PATH_MAX];
	int32_t result;

	xsmcToStringBuffer(xsArg(0), path, PATH_MAX);
	result = _unlink(path);

	xsResult = xsBoolean(result == 0);
}

void xs_file_exists(xsMachine *the)
{
	char *path;
	struct stat buf;
	int32_t result;

	path = xsmcToString(xsArg(0));
	result = stat(path, &buf);

	xsResult = xsBoolean(result == 0);
}

void xs_file_rename(xsMachine *the)
{
	char *path;
	char *name;
	int32_t result;

	path = xsmcToString(xsArg(0));
	name = xsmcToString(xsArg(1));
	result = rename(path, name);

	xsResult = xsBoolean(result == 0);
}

void xs_file_iterator_destructor(void *data)
{
	iter d = data;

	if (d) {
		if ((NULL != d->hFind) && (INVALID_HANDLE_VALUE != d->hFind))
			FindClose(d->hFind);
		free(d);
	}
}

void xs_File_Iterator(xsMachine *the)
{
	iter d;
	int i;
	char *p;

	p = xsmcToString(xsArg(0));
	i = strlen(p);
	if (i == 0) {
		xsUnknownError("no directory to iterate on");
	}
	d = calloc(1, sizeof(iteratorRecord) + i + 3);
	strcpy(d->path, p);
	if (p[i - 1] != '\\')
		d->path[i] = '\\';
	strcat(d->path, "*");

	xsmcSetHostData(xsThis, d);
}

void xs_file_iterator_next(xsMachine *the)
{
	iter d = xsmcGetHostData(xsThis);

	while (true) {
		if (NULL == d->hFind) {
			d->hFind = FindFirstFile(d->path, &d->ffd);
			if (INVALID_HANDLE_VALUE == d->hFind)
				return;
		}
		else {
			DWORD dwResult = FindNextFile(d->hFind, &d->ffd);
			if (0 == dwResult)
				return;
		}
		if ((0 == strcmp(d->ffd.cFileName, ".")) ||
			(0 == strcmp(d->ffd.cFileName, "..")) ||
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
	xsUnknownError("umimplemented");
}

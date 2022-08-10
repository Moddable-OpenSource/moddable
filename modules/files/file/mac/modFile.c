/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
	DIR *dir;
	int rootPathLen;
	char path[1];
} iteratorRecord, *iter;

static FILE *getFile(xsMachine *the)
{
	FILE *result = xsmcGetHostData(xsThis);
	if (!result)
		xsUnknownError("closed");
	return result;
}

void xs_file_destructor(void *data)
{
	if (data) {
		fclose((FILE *)data);

		modInstrumentationAdjust(Files, -1);
	}
}

void xs_File(xsMachine *the)
{
	int argc = xsmcArgc;
	FILE *file;
	uint8_t write = (argc < 2) ? 0 : xsmcToBoolean(xsArg(1));
	char *path = xsmcToString(xsArg(0));

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
	FILE *file = getFile(the);
	int32_t result;
	int argc = xsmcArgc;
	int dstLen = (argc < 2) ? -1 : xsmcToInteger(xsArg(1));
	void *dst;
	xsSlot *s1, *s2;
	struct stat buf;
	int32_t position = ftell(file);

	fstat(fileno(file), &buf);
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
		xsmcGet(xsVar(0), xsGlobal, xsID_ArrayBuffer);
		s2 = &xsVar(0);
		if (s1->data[2] == s2->data[2]) {	
		xsmcSetArrayBuffer(xsResult, NULL, dstLen);
		dst = xsmcToArrayBuffer(xsResult);
	}
		else {
			xsUnsignedValue len;
			xsmcGetBufferWritable(xsArg(0), (void **)&dst, &len);
			if (((uint32_t)dstLen) > len)
				dstLen = len;
			xsResult = xsArg(0);
		}
	}

	result = fread(dst, 1, dstLen, file);
	if (result != dstLen)
		xsUnknownError("file read failed");
}

void xs_file_write(xsMachine *the)
{
	FILE *file = getFile(the);
	int32_t result;
	int argc = xsmcArgc, i;

	for (i = 0; i < argc; i++) {
		uint8_t *src;
		int32_t srcLen;
		int type = xsmcTypeOf(xsArg(i));
		uint8_t temp;

		if (xsStringType == type) {
			src = (uint8_t*)xsmcToString(xsArg(i));
			srcLen = c_strlen((const char *)src);
		}
		else if ((xsIntegerType == type) || (xsNumberType == type)) {
			temp = (uint8_t)xsmcToInteger(xsArg(i));
			src = &temp;
			srcLen = 1;
		}
		else {
			xsUnsignedValue len;
			xsmcGetBufferReadable(xsArg(i), (void **)&src, &len);
			srcLen = len;
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
	FILE *file = getFile(the);
	xs_file_destructor((void *)((uintptr_t)file));
	xsmcSetHostData(xsThis, (void *)NULL);
}

void xs_file_get_length(xsMachine *the)
{
	FILE *file = getFile(the);
	struct stat buf;

	fstat(fileno(file), &buf);
	xsResult = xsInteger(buf.st_size);
}

void xs_file_get_position(xsMachine *the)
{
	FILE *file = getFile(the);
	int32_t position = ftell(file);
	xsResult = xsInteger(position);
}

void xs_file_set_position(xsMachine *the)
{
	FILE *file = getFile(the);
	int32_t position = xsmcToInteger(xsArg(0));
	fseek(file, position, SEEK_SET);
}

void xs_file_delete(xsMachine *the)
{
	int32_t result;
	char *path = xsmcToString(xsArg(0));

	result = unlink(path);

	xsResult = xsBoolean(result == 0);
}

void xs_file_exists(xsMachine *the)
{
	struct stat buf;
	int32_t result;
	char *path = xsmcToString(xsArg(0));

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
	int result = mkdir(path, 0755);
	if (result && (EEXIST != errno))
		xsUnknownError("failed");
}

void xs_directory_delete(xsMachine *the)
{
	char *path = xsmcToString(xsArg(0));
	int result = rmdir(path);
	if (result && (ENOENT != errno))
		xsUnknownError("failed");
}

void xs_file_iterator_destructor(void *data)
{
	iter d = data;

	if (d) {
		if (d->dir)
			closedir(d->dir);
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
	if (i == 0)
		xsUnknownError("bad path");
	d = c_calloc(1, sizeof(iteratorRecord) + i + 2 + PATH_MAX + 1);
	c_strcpy(d->path, p);
	if (p[i - 1] != '/')
		d->path[i++] = '/';
	d->rootPathLen = i;

	if (NULL == (d->dir = opendir(d->path))) {
		c_free(d);
		xsUnknownError("failed to open directory");
	}
	xsmcSetHostData(xsThis, d);

	modInstrumentationAdjust(Files, +1);
}

void xs_file_iterator_next(xsMachine *the)
{
	iter d = xsmcGetHostData(xsThis);
	struct dirent *de;
	struct stat buf;

	if (!d || !d->dir) return;

	do {
		if (NULL == (de = readdir(d->dir))) {
			xs_file_iterator_destructor(d);
			xsmcSetHostData(xsThis, NULL);
			return;
		}
	} while ((DT_DIR != de->d_type) && (DT_REG != de->d_type));

	xsResult = xsmcNewObject();
	xsmcVars(1);
	xsmcSetString(xsVar(0), de->d_name);
	xsmcSet(xsResult, xsID_name, xsVar(0));

	if (DT_REG == de->d_type) {
		c_strcpy(d->path + d->rootPathLen, de->d_name);
		if (-1 == stat(d->path, &buf))
			xsUnknownError("stat error");
		xsmcSetInteger(xsVar(0), buf.st_size);
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
	xsResult = xsmcNewObject();
}

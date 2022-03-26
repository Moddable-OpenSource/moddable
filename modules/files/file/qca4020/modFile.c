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
#include "xsHost.h"

#define QAPI_USE_FILESYSTEM
#include "qapi.h"

typedef struct {
	int fd;
	char path[1];
} xsFileRecord, *xsFile;

typedef struct {
	qapi_fs_iter_handle_t handle;
	char path[1];
} iteratorRecord, *iter;

void xs_file_destructor(void *data)
{
	xsFile file = (xsFile)data;
	if (file) {
		if (-1 != file->fd)
			qapi_Fs_Close(file->fd);
		c_free(file);
	}
}

void xs_File(xsMachine *the)
{
	qapi_Status_t result;
	int fd, argc = xsmcArgc;
	uint8_t write = (argc < 2) ? 0 : xsmcToBoolean(xsArg(1));
	char *path = xsmcToString(xsArg(0));
	xsFile file;

	result = qapi_Fs_Open(path, write ? QAPI_FS_O_RDWR | QAPI_FS_O_CREAT : QAPI_FS_O_RDONLY, &fd);

	if (QAPI_OK != result)
		xsUnknownError("file not found");
		
	file = c_malloc(sizeof(xsFileRecord) + c_strlen(path));
	if (!file)
		xsUnknownError("out of memory");

	file->fd = fd;
	c_strcpy(file->path, path);
	xsmcSetHostData(xsThis, (void *)file);
}

void xs_file_read(xsMachine *the)
{
	xsFile file = (xsFile)xsmcGetHostData(xsThis);
	qapi_Status_t result;
	int argc = xsmcArgc;
	int dstLen = (argc < 2) ? -1 : xsmcToInteger(xsArg(1));
	void *dst;
	xsSlot *s1, *s2;
	struct qapi_fs_stat_type buf;
	int32_t position;
	uint32_t bytes_read;
	
	result = qapi_Fs_Lseek(file->fd, 0, QAPI_FS_SEEK_CUR, &position);
	if (QAPI_OK != result)
		xsUnknownError("file read failed");

	result = qapi_Fs_Stat(file->path, &buf);
	if (QAPI_OK != result)
		xsUnknownError("file read failed");

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

	result = qapi_Fs_Read(file->fd, dst, dstLen, &bytes_read);
	if (QAPI_OK != result)
		xsUnknownError("file read failed");
}

void xs_file_write(xsMachine *the)
{
	xsFile file = (xsFile)xsmcGetHostData(xsThis);
	qapi_Status_t result;
	int argc = xsmcArgc, i;
	uint32_t bytes_written;

	for (i = 0; i < argc; i++) {
		uint8_t *src;
		int32_t srcLen;
		int type = xsmcTypeOf(xsArg(i));
		uint8_t temp;

		if (xsStringType == type) {
			src = (uint8_t *)xsmcToString(xsArg(i));
			srcLen = c_strlen((char *)src);
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

		result = qapi_Fs_Write(file->fd, src, srcLen, &bytes_written);
		if (QAPI_OK != result)
			xsUnknownError("file write failed");
	}
}

void xs_file_close(xsMachine *the)
{
	xsFile file = (xsFile)xsmcGetHostData(xsThis);
	xs_file_destructor((void *)file);
	xsmcSetHostData(xsThis, (void *)NULL);
}

void xs_file_get_length(xsMachine *the)
{
	xsFile file = (xsFile)xsmcGetHostData(xsThis);
	qapi_Status_t result;
	struct qapi_fs_stat_type buf;

	result = qapi_Fs_Stat(file->path, &buf);
	if (QAPI_OK != result)
		xsUnknownError("file stat failed");
	xsResult = xsInteger(buf.st_size);
}

void xs_file_get_position(xsMachine *the)
{
	xsFile file = (xsFile)xsmcGetHostData(xsThis);
	int32_t position;	
	
	qapi_Fs_Lseek(file->fd, 0, QAPI_FS_SEEK_CUR, &position);
	xsResult = xsInteger(position);
}

void xs_file_set_position(xsMachine *the)
{
	xsFile file = (xsFile)xsmcGetHostData(xsThis);
	int32_t position = xsmcToInteger(xsArg(0));
	
	qapi_Fs_Lseek(file->fd, position, QAPI_FS_SEEK_SET, &position);
	xsResult = xsInteger(position);
}

void xs_file_delete(xsMachine *the)
{
	char path[QAPI_FS_MAX_FILE_PATH_LEN];
	qapi_Status_t result;
	
	xsmcToStringBuffer(xsArg(0), path, QAPI_FS_MAX_FILE_PATH_LEN);
	result = qapi_Fs_Unlink(path);
	xsResult = xsBoolean(QAPI_OK == result);
}

void xs_file_exists(xsMachine *the)
{
	char *path = xsmcToString(xsArg(0));
	struct qapi_fs_stat_type buf;
	qapi_Status_t result;

	result = qapi_Fs_Stat(path, &buf);
	xsResult = xsBoolean(QAPI_OK == result);
}

void xs_file_rename(xsMachine *the)
{
	char *path;
	char toPath[PATH_MAX + 1];
	qapi_Status_t result;

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

	result = qapi_Fs_Rename(path, toPath);
	xsResult = xsBoolean(QAPI_OK == result);
}

void xs_directory_create(xsMachine *the)
{
	xsUnknownError("unimplemented");
}

void xs_directory_delete(xsMachine *the)
{
	xsUnknownError("unimplemented");
}

void xs_file_iterator_destructor(void *data)
{
	iter d = data;

	if (d) {
		if (NULL != d->handle)
			qapi_Fs_Iter_Close(d->handle);
		free(d);
		modInstrumentationAdjust(Files, -1);
	}
}

void xs_File_Iterator(xsMachine *the)
{
	iter d;
	int i;
	char *p;
	qapi_Status_t result;

	p = xsmcToString(xsArg(0));
	i = c_strlen(p);
	if (i == 0)
		xsUnknownError("no directory to iterate on");
	d = c_calloc(1, sizeof(iteratorRecord) + i + 2);
	c_strcpy(d->path, p);
	if (p[i-1] != '/')
		d->path[i] = '/';

	result = qapi_Fs_Iter_Open(d->path, &d->handle);
	if (QAPI_OK != result)
		xsUnknownError("failed to open directory");

	xsmcSetHostData(xsThis, d);

	modInstrumentationAdjust(Files, +1);
}

void xs_file_iterator_next(xsMachine *the)
{
	iter d = xsmcGetHostData(xsThis);
	struct qapi_fs_iter_entry qapi_iter_entry;
	qapi_Status_t result;
	
	result = qapi_Fs_Iter_Next(d->handle, &qapi_iter_entry);
	if (QAPI_OK != result) {
		if (QAPI_ERR_NO_ENTRY == result) {
			xs_file_iterator_destructor(d);
			xsmcSetHostData(xsThis, NULL);
			return;
		}
		xsUnknownError("file iterator failed");
	}

	xsResult = xsmcNewObject();
	xsmcVars(1);
	xsmcSetString(xsVar(0), qapi_iter_entry.file_path);
	xsmcSet(xsResult, xsID_name, xsVar(0));

	if (qapi_iter_entry.file_path[c_strlen(qapi_iter_entry.file_path) - 1] != '/') {
		xsmcSetInteger(xsVar(0), qapi_iter_entry.sbuf.st_size);
		xsmcSet(xsResult, xsID_length, xsVar(0));
	}
}

void xs_file_system_config(xsMachine *the)
{
	xsResult = xsmcNewObject();
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), QAPI_FS_MAX_FILE_PATH_LEN);
	xsmcSet(xsResult, xsID_maxPathLength, xsVar(0));
}

void xs_file_system_info(xsMachine *the)
{
	xsResult = xsmcNewObject();
}

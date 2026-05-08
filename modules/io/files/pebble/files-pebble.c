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
 
 /*
	To do:

		- consolidate path concatenation and checking
		- directory.status() can't return size for open files
		- openFile can create a file with the name of an existing directory (to avoid it is a full scan of the file system)
 */

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"      // for xsID_ values

#include "moddableAppState.h"

#include "applib/app.h"
#include "services/filesystem/app_file.h"
#include "services/filesystem/pfs.h"

/*
	helpers
*/

#define throwIf(a) \
	{ \
		int __err = a; \
		if (__err < 0) \
			xsUnknownError("operation failed %d", __err); \
	}

#define getPath(s) _getPath(the, &s)

enum {
	kPathStateAny,
	kPathStateSlash,
	kPathStateDot,
	kPathStateDotDot,
};

static char *_getPath(xsMachine *the, xsSlot *slot)
{
	char *path = xsmcToString(*slot);
	char *p = path;
	int state = kPathStateSlash;

	if (0 == c_read8(p))
		return "./";

	while (true) {
		switch (c_read8(p++)) {
			case '.': 
				if (kPathStateSlash == state)
					state = kPathStateDot;
				else if (kPathStateDot == state)
					state = kPathStateDotDot;
				else
					state = 0;
				break;
			case '/':
				if ((kPathStateSlash == state) || (kPathStateDot == state) || (kPathStateDotDot == state))
					xsUnknownError("bad path");
				state = kPathStateSlash;
				break;
			case 0:
				if ((kPathStateDot == state) || (kPathStateDotDot == state))
					xsUnknownError("bad path");
				return path;
			default:
				state = kPathStateAny;
				break;
		}
	}

	return NULL;		// can never reach here
}

static bool pfsStatus(const char *name)
{
	ModdablePebbleAppState state = (ModdablePebbleAppState)app_state_get_js_memory_api_context();
	xsMachine *the = state->the;

	if (xsUndefinedType != xsmcTypeOf(xsVar(0)))
		return false;

	size_t rootLen = c_strlen(state->root);
	if (0 == c_strncmp(name, state->root, rootLen)) {
		if ('/' == name[rootLen])
			xsmcSetInteger(xsVar(0), 2);		// directory
	}

	return false;
}

/*
	File
*/

struct xsFileRecord {
	int		fd;
};
typedef struct xsFileRecord xsFileRecord;
typedef struct xsFileRecord *xsFile;

void xs_filepfs_destructor(void *data)
{
	xsFile f = data;
	if (f)
		pfs_close(f->fd);
}

#define getFile(slot) ((xsFile)xsmcGetHostChunkValidate(slot, xs_filepfs_destructor))->fd

void xs_filepfs(xsMachine *the)
{
	xsUnknownError("use openFile");
}

void xs_filepfs_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis)) 
		return;

	pfs_close(getFile(xsThis));
	xsmcSetHostData(xsThis, NULL);
}

void xs_filepfs_read(xsMachine *the)
{
	int fd = getFile(xsThis);
	void *buffer;
	xsUnsignedValue length;
	int position = xsmcToInteger(xsArg(1));
	uint8_t returnLength = 0;

	int type = xsmcTypeOf(xsArg(0));
	if ((xsIntegerType == type) || (xsNumberType == type)) {
 		length = xsmcToInteger(xsArg(0));
		xsmcSetArrayBufferResizable(xsResult, NULL, length, length);
		xsArg(0) = xsResult;
		buffer = xsmcToArrayBuffer(xsResult);
	}
	else {
		xsResult = xsArg(0);
		xsmcGetBufferWritable(xsResult, &buffer, &length);
		returnLength = 1;
	}

	throwIf(pfs_seek(fd, position, FSeekSet));

	int result = pfs_read(fd, buffer, length);
	throwIf(result);

	if (returnLength)
		xsmcSetInteger(xsResult, result);
	else if ((xsUnsignedValue)result != length)
		xsmcSetArrayBufferLength(xsResult, result);
}

void xs_filepfs_write(xsMachine *the)
{
	int fd = getFile(xsThis);
	int position = xsmcToInteger(xsArg(1));
	void *buffer;
	xsUnsignedValue length;
	xsmcGetBufferWritable(xsArg(0), &buffer, &length);

	// verify the write can succeed
	uint32_t offset = 0, remain = length;
	while (remain) {
		uint8_t bytes[FILE_MAX_NAME_LEN + 1];
		uint32_t use = remain;
		if (use > sizeof(bytes))
			use = sizeof(bytes);
		throwIf(pfs_seek(fd, position + offset, FSeekSet));
		throwIf(pfs_read(fd, bytes, use));
		
		for (uint32_t j = 0; j < use; j++) {
			uint8_t byte = ((uint8_t *)buffer)[offset + j];
			if (byte != (byte & bytes[j]))
				xsUnknownError("NOR write would fail");
		}

		offset += use;
		remain -= use;
	}

	// write
	throwIf(pfs_seek(fd, position, FSeekSet));
	throwIf(pfs_write(fd, buffer, length));
}

void xs_filepfs_status(xsMachine *the)
{
	int fd = getFile(xsThis);

	xsResult = xsArg(0);

	xsmcVars(1);
	xsmcSetInteger(xsVar(0), (int)pfs_get_file_size(fd));
	xsmcSet(xsResult, xsID_size, xsVar(0));

	xsmcSetInteger(xsVar(0), (int)1);
	xsmcSet(xsResult, xsID_mode, xsVar(0));
}

/*
	Directory
*/

struct xsDirectoryRecord {
	char path[1];
};
typedef struct xsDirectoryRecord xsDirectoryRecord;
typedef struct xsDirectoryRecord *xsDirectory;

void xs_directorypfs_destructor(void *data)
{
}

#define getDirectoryPath(slot) ((xsDirectory)xsmcGetHostChunkValidate(slot, xs_directorypfs_destructor))->path

static void buildFullPath(xsMachine *the, char *fullPath, const char *filePath)
{
	const char *dirPath = getDirectoryPath(xsThis);
	size_t len = c_strlen(dirPath) + (filePath ? c_strlen(filePath) : 0);
	if (len > FILE_MAX_NAME_LEN)
		xsUnknownError("path too long");
	c_strcpy(fullPath, dirPath);
	if (filePath)
		c_strcat(fullPath, filePath);
}

void xs_directorypfs(xsMachine *the)
{
	xsUnknownError("use openDirectory");
}

void xs_directorypfs_bootstrap(xsMachine *the)
{	
	char path[APP_FILE_NAME_PREFIX_LENGTH * 2 + 1];
	app_file_name_make(path, sizeof(path), app_get_app_id(), "xs/", 3);
	xsmcSetHostChunk(xsArg(0), path, c_strlen(path) + 1);
}

void xs_directorypfs_close(xsMachine *the)
{
	xsmcSetHostData(xsThis, NULL);
}

void xs_directorypfs_openFile(xsMachine *the)
{
	(void)getDirectoryPath(xsThis);

	xsResult = xsNewHostInstance(xsArg(1));

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_mode);
	uint8_t op_flags;
	if (xsUndefinedType == xsmcTypeOf(xsVar(0)))
		op_flags = OP_FLAG_READ;
	else {
		char *modestr = xsmcToString(xsVar(0));
		if (!c_strcmp(modestr, "r"))
			op_flags = OP_FLAG_READ;
		else if (!c_strcmp(modestr, "r+"))
			op_flags = OP_FLAG_READ | OP_FLAG_WRITE;
		else if (!c_strcmp(modestr, "w+"))
			op_flags = OP_FLAG_READ | OP_FLAG_WRITE;
		else if (!c_strcmp(modestr, "overwrite"))
			op_flags = OP_FLAG_OVERWRITE;
		else
			xsUnknownError("invalid mode");
	}

	xsmcGet(xsVar(0), xsArg(0), xsID_size);
	int size = xsmcToInteger(xsVar(0));		// if not present, this will be zero which is fine for existing files and will throw for new files from pfs_open... all correct
	if (size < 0)
		xsUnknownError("invalid size");

	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	char *filePath = getPath(xsVar(0));
	char fullPath[FILE_MAX_NAME_LEN + 1];
	buildFullPath(the, fullPath, filePath);

	xsmcGet(xsVar(0), xsArg(0), xsID_round);
	if (xsmcTest(xsVar(0)))
		size = pfs_sector_optimal_size(size, c_strlen(fullPath));

	//@@ check for a directory with this name

	xsFileRecord f;
	f.fd = pfs_open(fullPath, op_flags, FILE_TYPE_STATIC, size);
	throwIf(f.fd);

	xsmcSetHostChunk(xsResult, &f, sizeof(f));
}

void xs_directorypfs_openDirectory(xsMachine *the)
{
	xsResult = xsNewHostInstance(xsArg(1));

	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	char *filePath = getPath(xsVar(0));
	char fullPath[FILE_MAX_NAME_LEN + 1];
	buildFullPath(the, fullPath, filePath);

	xsmcSetHostChunk(xsResult, fullPath, c_strlen(fullPath) + 1);
}

void xs_directorypfs_delete(xsMachine *the)
{
	char *filePath = getPath(xsArg(0));
	char fullPath[FILE_MAX_NAME_LEN + 1];
	buildFullPath(the, fullPath, filePath);

	int result = pfs_remove(fullPath);
	if (E_DOES_NOT_EXIST == result) {
		xsmcSetFalse(xsResult);

		xsmcVars(1);
		setModdableAppState(root, fullPath);
		pfs_delete_file_list(pfs_create_file_list(pfsStatus));
		if (xsUndefinedType != xsmcTypeOf(xsVar(0)))
			xsUnknownError("non-empty directory");
	}
	else {
		throwIf(result);
		xsmcSetTrue(xsResult);
	}
}

void xs_directorypfs_status(xsMachine *the)
{
	char *filePath = getPath(xsArg(0));
	char fullPath[FILE_MAX_NAME_LEN + 1];
	buildFullPath(the, fullPath, filePath);

	xsmcVars(1);
	xsResult = xsArg(2);

	int fd = pfs_open(fullPath, OP_FLAG_READ, FILE_TYPE_STATIC, 0);
	if ((fd >= 0) || (E_BUSY == fd)) {
		if (E_BUSY != fd) {
			xsmcSetInteger(xsVar(0), (int)pfs_get_file_size(fd));
			pfs_close(fd);
			xsmcSet(xsResult, xsID_size, xsVar(0));
		}

		xsmcSetInteger(xsVar(0), 1);		// file
	}
	else {	// could be a directory. 
		setModdableAppState(root, fullPath);

		pfs_delete_file_list(pfs_create_file_list(pfsStatus));

		if (xsUndefinedType == xsmcTypeOf(xsVar(0)))
			xsmcSetInteger(xsVar(0), 0);		// not file, directory, or link
	}

	xsmcSet(xsResult, xsID_mode, xsVar(0));
}

// mostly works as expected but doesn't "create" the directory (pebble can't) so a scan after this won't find it. But creating a file in the directory will succeed.
void xs_directorypfs_createDirectory(xsMachine *the)
{
	char *filePath = getPath(xsArg(0));
	char fullPath[FILE_MAX_NAME_LEN + 1];
	buildFullPath(the, fullPath, filePath);

	int fd = pfs_open(fullPath, OP_FLAG_READ, FILE_TYPE_STATIC, 0);
	if ((fd >= 0) || (E_BUSY == fd)) {
		if (fd >= 0)
			pfs_close(fd);
		xsUnknownError("file exists");
	}

	xsmcVars(1);
	setModdableAppState(root, fullPath);
	pfs_delete_file_list(pfs_create_file_list(pfsStatus));
	if (xsUndefinedType == xsmcTypeOf(xsVar(0)))
		xsmcSetFalse(xsResult);		// new directoty
	else
		xsmcSetTrue(xsResult);		// already exists
}

/*
	Scan
*/

static bool pfsScan(const char *name)
{
	ModdablePebbleAppState state = (ModdablePebbleAppState)app_state_get_js_memory_api_context();
	xsMachine *the = state->the;
	size_t rootLen = c_strlen(state->root);

	if (0 == c_strncmp(name, state->root, rootLen)) {
		char entry[FILE_MAX_NAME_LEN];
		c_strcpy(entry, name + rootLen);
		char *slash = c_strchr(entry, '/');
		if (slash)
			*slash= 0;

		xsmcSetString(xsVar(0), entry);
		xsVar(1) = xsCall1(xsResult, xsID_includes, xsVar(0));
		if (!xsmcTest(xsVar(1)))
			xsCall1(xsResult, xsID_push, xsVar(0));
//@@		xsmcCall_noResult(xsResult, xsID_push, xsVar(0));
	}

	return false;
}

void xs_directorypfs_scan(xsMachine *the)
{
	xsmcVars(2);
	xsResult = xsmcNewArray(0);

	char fullPath[FILE_MAX_NAME_LEN + 1];
	char *filePath = (xsmcArgc > 0) ? getPath(xsVar(0)) : C_NULL;
	buildFullPath(the, fullPath, filePath);
	setModdableAppState(root, fullPath);

	pfs_delete_file_list(pfs_create_file_list(pfsScan));
}

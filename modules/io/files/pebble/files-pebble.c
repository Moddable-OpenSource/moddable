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

#include <errno.h>

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"      // for xsID_ values
#include "modInstrumentation.h"

#include "moddableAppState.h"

#include "applib/app.h"
#include "services/process_management/app_file.h"
#include "pbl/fs/fs.h"
#include "pbl/fs/pfs.h"

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

#define kMountPoint "/"
#define kMountPointLength (sizeof(kMountPoint) - 1)
#define kMaxPathLength (kMountPointLength + PBL_FS_NAME_MAX)

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

	if (0 == c_read8(p)) {
		xsmcSetStringX(*slot, "./");
		return xsmcToString(*slot);
	}

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

typedef void (*scanEntry)(xsMachine *the, const char *name, const char *root);

struct scanState {
	struct pbl_fs_dir dir;
	struct pbl_fs_dirent entry;
};

// root is a full path; names in the mount are relative to it
static void scanFiles(xsMachine *the, const char *root, scanEntry callback)
{
	struct scanState *scan = c_malloc(sizeof(*scan));
	if (!scan)
		xsUnknownError("no memory");

	int err = pbl_fs_opendir(&scan->dir, kMountPoint);
	if (err) {
		c_free(scan);
		throwIf(err);
	}

	const char *relativeRoot = root + kMountPointLength;
	xsTry {
		while ((0 == pbl_fs_readdir(&scan->dir, &scan->entry)) && scan->entry.name[0])
			callback(the, scan->entry.name, relativeRoot);
	}
	xsCatch {
		pbl_fs_closedir(&scan->dir);
		c_free(scan);
		xsThrow(xsException);
	}

	pbl_fs_closedir(&scan->dir);
	c_free(scan);
}

static void pfsStatus(xsMachine *the, const char *name, const char *root)
{
	if (xsUndefinedType != xsmcTypeOf(xsVar(0)))
		return;

	size_t rootLen = c_strlen(root);
	if (0 == c_strncmp(name, root, rootLen)) {
		if ('/' == name[rootLen])
			xsmcSetInteger(xsVar(0), 2);		// directory
	}
}

/*
	File
*/

struct xsFileRecord {
	struct pbl_fs_file	file;
};
typedef struct xsFileRecord xsFileRecord;
typedef struct xsFileRecord *xsFile;

void xs_filepfs_destructor(void *data)
{
	xsFile f = data;
	if (f) {
		pbl_fs_close(&f->file);
		modInstrumentationAdjust(Files, -1);
	}
}

// the host chunk can move on allocation, so get the file right before using it
#define getFile(slot) (&((xsFile)xsmcGetHostChunkValidate(slot, xs_filepfs_destructor))->file)

void xs_filepfs(xsMachine *the)
{
	xsUnknownError("use openFile");
}

void xs_filepfs_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis)) 
		return;

	pbl_fs_close(getFile(xsThis));
	xsmcSetHostData(xsThis, NULL);
	modInstrumentationAdjust(Files, -1);
}

void xs_filepfs_read(xsMachine *the)
{
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

	struct pbl_fs_file *file = getFile(xsThis);
	throwIf(pbl_fs_seek(file, position, PBL_FS_SEEK_SET));

	int result = pbl_fs_read(file, buffer, length);
	throwIf(result);

	if (returnLength)
		xsmcSetInteger(xsResult, result);
	else if ((xsUnsignedValue)result != length)
		xsmcSetArrayBufferLength(xsResult, result);
}

void xs_filepfs_write(xsMachine *the)
{
	int position = xsmcToInteger(xsArg(1));
	void *buffer;
	xsUnsignedValue length;
	xsmcGetBufferWritable(xsArg(0), &buffer, &length);

	struct pbl_fs_file *file = getFile(xsThis);

	// verify the write can succeed
	uint32_t offset = 0, remain = length;
	while (remain) {
		uint8_t bytes[256];
		uint32_t use = remain;
		if (use > sizeof(bytes))
			use = sizeof(bytes);
		throwIf(pbl_fs_seek(file, position + offset, PBL_FS_SEEK_SET));
		int result = pbl_fs_read(file, bytes, use);
		throwIf(result);
		if ((uint32_t)result != use)
			xsUnknownError("write past end of file");
		
		for (uint32_t j = 0; j < use; j++) {
			uint8_t byte = ((uint8_t *)buffer)[offset + j];
			if (byte != (byte & bytes[j]))
				xsUnknownError("NOR write would fail");
		}

		offset += use;
		remain -= use;
	}

	// write
	throwIf(pbl_fs_seek(file, position, PBL_FS_SEEK_SET));
	int result = pbl_fs_write(file, buffer, length);
	throwIf(result);
	if ((xsUnsignedValue)result != length)
		xsUnknownError("partial write");
}

void xs_filepfs_status(xsMachine *the)
{
	int size = (int)pbl_fs_size(getFile(xsThis));

	xsResult = xsArg(0);

	xsmcVars(1);
	xsmcSetInteger(xsVar(0), size);
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
	if (len > kMaxPathLength)
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
	char path[kMountPointLength + APP_FILE_NAME_PREFIX_LENGTH * 2 + 1];
	c_strcpy(path, kMountPoint);
	app_file_name_make(path + kMountPointLength, sizeof(path) - kMountPointLength, app_get_app_id(), "xs/", 3);
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
	uint32_t flags;
	if (xsUndefinedType == xsmcTypeOf(xsVar(0)))
		flags = PBL_FS_O_READ;
	else {
		char *modestr = xsmcToString(xsVar(0));
		if (!c_strcmp(modestr, "r"))
			flags = PBL_FS_O_READ;
		else if (!c_strcmp(modestr, "r+"))
			flags = PBL_FS_O_RDWR | PBL_FS_O_CREATE;
		else if (!c_strcmp(modestr, "w+"))
			flags = PBL_FS_O_RDWR | PBL_FS_O_CREATE;
		else if (!c_strcmp(modestr, "overwrite"))
			flags = PBL_FS_O_REPLACE;
		else
			xsUnknownError("invalid mode");
	}

	xsmcGet(xsVar(0), xsArg(0), xsID_size);
	int size = xsmcToInteger(xsVar(0));		// if not present, this will be zero which is fine for existing files and will throw for new files... all correct
	if (size < 0)
		xsUnknownError("invalid size");

	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	char *filePath = getPath(xsVar(0));
	char fullPath[kMaxPathLength + 1];
	buildFullPath(the, fullPath, filePath);

	xsmcGet(xsVar(0), xsArg(0), xsID_round);
	if (xsmcTest(xsVar(0))) {
		size = pbl_fs_pfs_optimal_size(fullPath, size);
		throwIf(size);
	}

	//@@ check for a directory with this name

	xsFileRecord f;
	int err = pbl_fs_open(&f.file, fullPath, flags);
	if ((0 == err) && (0 == pbl_fs_size(&f.file))) {
		err = size ? pbl_fs_truncate(&f.file, size) : -EINVAL;
		if (err)
			pbl_fs_close(&f.file);
	}
	throwIf(err);

	xsmcSetHostChunk(xsResult, &f, sizeof(f));

	modInstrumentationAdjust(Files, +1);
}

void xs_directorypfs_openDirectory(xsMachine *the)
{
	xsResult = xsNewHostInstance(xsArg(1));

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	char *filePath = getPath(xsVar(0));
	char fullPath[kMaxPathLength + 1];
	buildFullPath(the, fullPath, filePath);

	xsmcSetHostChunk(xsResult, fullPath, c_strlen(fullPath) + 1);
}

void xs_directorypfs_delete(xsMachine *the)
{
	char *filePath = getPath(xsArg(0));
	char fullPath[kMaxPathLength + 1];
	buildFullPath(the, fullPath, filePath);

	int result = pbl_fs_unlink(fullPath);
	if (-ENOENT == result) {
		xsmcSetFalse(xsResult);

		xsmcVars(1);
		scanFiles(the, fullPath, pfsStatus);
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
	char fullPath[kMaxPathLength + 1];
	buildFullPath(the, fullPath, filePath);

	xsmcVars(1);
	xsResult = xsArg(2);

	struct pbl_fs_dirent *entry = c_malloc(sizeof(*entry));
	if (!entry)
		xsUnknownError("no memory");
	int err = pbl_fs_stat(fullPath, entry);
	size_t size = entry->size;
	c_free(entry);

	if (0 == err) {
		xsmcSetInteger(xsVar(0), (int)size);
		xsmcSet(xsResult, xsID_size, xsVar(0));

		xsmcSetInteger(xsVar(0), 1);		// file
	}
	else {	// could be a directory. 
		scanFiles(the, fullPath, pfsStatus);

		if (xsUndefinedType == xsmcTypeOf(xsVar(0)))
			xsmcSetInteger(xsVar(0), 0);		// not file, directory, or link
	}

	xsmcSet(xsResult, xsID_mode, xsVar(0));
}

// mostly works as expected but doesn't "create" the directory (pebble can't) so a scan after this won't find it. But creating a file in the directory will succeed.
void xs_directorypfs_createDirectory(xsMachine *the)
{
	char *filePath = getPath(xsArg(0));
	char fullPath[kMaxPathLength + 1];
	buildFullPath(the, fullPath, filePath);

	struct pbl_fs_dirent *entry = c_malloc(sizeof(*entry));
	if (!entry)
		xsUnknownError("no memory");
	int err = pbl_fs_stat(fullPath, entry);
	c_free(entry);
	if (0 == err)
		xsUnknownError("file exists");

	xsmcVars(1);
	scanFiles(the, fullPath, pfsStatus);
	if (xsUndefinedType == xsmcTypeOf(xsVar(0)))
		xsmcSetTrue(xsResult);		// new directory
	else
		xsmcSetFalse(xsResult);		// already exists
}

/*
	Scan
*/

static void pfsScan(xsMachine *the, const char *name, const char *root)
{
	size_t rootLen = c_strlen(root);

	if (0 == c_strncmp(name, root, rootLen)) {
		char entry[PBL_FS_NAME_MAX + 1];
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
}

void xs_directorypfs_scan(xsMachine *the)
{
	xsmcVars(2);
	xsResult = xsmcNewArray(0);

	char fullPath[kMaxPathLength + 1];
	char *filePath = (xsmcArgc > 0) ? getPath(xsArg(0)) : C_NULL;
	buildFullPath(the, fullPath, filePath);

	scanFiles(the, fullPath, pfsScan);
}

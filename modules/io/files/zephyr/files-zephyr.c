/*
 * Copyright (c) 2024-2025  Moddable Tech, Inc.
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
#include "mc.devicetree.h"

#include <zephyr/device.h>
#include <zephyr/fs/fs.h>

#if kModZephyrFSCount

/*
	helpers
*/

#define throwIf(a) if ((a) < 0) xsUnknownError(strerror(-(a)))

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

	return NULL;        // can never reach here
}

#define appendPath(dirPath, slot) _appendPath(the, dirPath, &slot)

static char *_appendPath(xsMachine *the, char *dirPath, xsSlot *slot)
{
	size_t dirLen = c_strlen(dirPath);
	char *path = _getPath(the, slot);
	char *result = fxNewChunk(the, dirLen + 1 + c_strlen(path) + 1);
	path = xsmcToString(*slot);		// refresh
	c_strcpy(result, dirPath);
	c_strcat(result, "/");
	c_strcat(result, path);
	size_t resultLen = c_strlen(result);
	if ('/' == result[resultLen - 1])
		result[resultLen - 1] = 0;		// remove trailing slash
	return result;
}

/*
	File
*/

struct xsFileRecord {
	struct fs_file_t fsf;
	fs_mode_t mode;
	char path[];
};
typedef struct xsFileRecord xsFileRecord;
typedef struct xsFileRecord *xsFile;

void xs_filezephyr_destructor(void *data)
{
	xsFile xf = data;
	if (xf) {
		fs_close(&xf->fsf);
		c_free(xf);
	}
}

#define getFile(slot) ((xsFile)xsmcGetHostChunkValidate(slot, xs_filezephyr_destructor))->fd

void xs_filezephyr(xsMachine *the)
{
	xsUnknownError("use openFile");
}

void xs_filezephyr_close(xsMachine *the)
{
	if (!xsmcGetHostData(xsThis)) 
		return;

	xsFile xf = xsmcGetHostData(xsThis);
	xs_filezephyr_destructor(xf);
	xsmcSetHostData(xsThis, NULL);
}

void xs_filezephyr_read(xsMachine *the)
{
	xsFile xf = (xsFile)xsmcGetHostData(xsThis);;
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

	throwIf(fs_seek(&xf->fsf, position, FS_SEEK_SET));
	int result = fs_read(&xf->fsf, buffer, length);
	throwIf(result);

	if (returnLength)
		xsmcSetInteger(xsResult, result);
	else if ((xsUnsignedValue)result != length)
		xsmcSetArrayBufferLength(xsResult, result);
}

void xs_filezephyr_write(xsMachine *the)
{
	xsFile xf = (xsFile)xsmcGetHostData(xsThis);
	int position = xsmcToInteger(xsArg(1));
	void *buffer;
	xsUnsignedValue length;
	if (0 == (xf->mode & FS_O_WRITE))
		xsUnknownError("read only");

	xsmcGetBufferWritable(xsArg(0), &buffer, &length);

	throwIf(fs_seek(&xf->fsf, position, FS_SEEK_SET));
	throwIf(fs_write(&xf->fsf, buffer, length));
}

void xs_filezephyr_status(xsMachine *the)
{
	xsFile xf = (xsFile)xsmcGetHostData(xsThis);;
	struct fs_dirent entry;

	throwIf(fs_stat(xf->path, &entry));

	xsResult = xsArg(0);

	xsmcVars(1);
	xsmcSetInteger(xsVar(0), entry.size);
	xsmcSet(xsResult, xsID_size, xsVar(0));

	xsmcSetInteger(xsVar(0), entry.type);
	xsmcSet(xsResult, xsID_mode, xsVar(0));
}

void xs_filezephyr_setSize(xsMachine *the)
{
	xsFile xf = (xsFile)xsmcGetHostData(xsThis);;
	throwIf(fs_truncate(&xf->fsf, xsmcToInteger(xsArg(0))));
}

void xs_filezephyr_flush(xsMachine *the)
{
	xsFile xf = (xsFile)xsmcGetHostData(xsThis);;
	throwIf(fs_sync(&xf->fsf));
}

/*
	Directory
*/

struct xsDirectoryRecord {
	int placeholder;
	char path[];
};
typedef struct xsDirectoryRecord xsDirectoryRecord;
typedef struct xsDirectoryRecord *xsDirectory;

void xs_directoryzephyr_destructor(void *data)
{
	xsDirectory d = data;
	if (d)
		c_free(d);
}

void xs_directoryzephyr(xsMachine *the)
{
	xsUnknownError("use openDirectory");
}

// #define PARTITION_NODE DT_NODELABEL(lfs1)
// FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
// static struct fs_mount_t *mountpoint = &FS_FSTAB_ENTRY(PARTITION_NODE);

void xs_directoryzephyr_bootstrap(xsMachine *the)
{
	const struct modZephyrFS *fs = modZephyrGetFS(MODDEF_ZEPHYR_FILESYSTEM_DEFAULT);

	throwIf(fs_mount(fs->mountpoint));

	const char *path = fs->mountpoint->mnt_point;
	if (xsmcTest(xsArg(1)))
		path = xsmcToString(xsArg(1));

	xsDirectory d = c_malloc(sizeof(xsDirectoryRecord) + c_strlen(path) + 1);
	if (!d)
		xsUnknownError("no memory");
	c_strcpy(d->path, path);
	xsmcSetHostData(xsArg(0), d);

	struct fs_dirent entry;
	throwIf(fs_stat(d->path, &entry));
	if (FS_DIR_ENTRY_DIR != entry.type)
		xsUnknownError("not directory");
}

void xs_directoryzephyr_close(xsMachine *the)
{
	xsDirectory d = xsmcGetHostData(xsThis);
	xs_directoryzephyr_destructor(d);
	xsmcSetHostData(xsThis, NULL);
}

void xs_directoryzephyr_openFile(xsMachine *the)
{
	xsDirectory d = xsmcGetHostData(xsThis);

	xsResult = xsNewHostInstance(xsArg(1));

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_mode);
	char *modestr;
	fs_mode_t mode;
	if (xsUndefinedType == xsmcTypeOf(xsVar(0)))
		modestr = "r";
	else
		modestr = xsmcToString(xsVar(0));
	if (!c_strcmp(modestr, "r"))
		mode = FS_O_READ;
	else if (!c_strcmp(modestr, "r+"))
		mode = FS_O_RDWR;
	else if (!c_strcmp(modestr, "w"))
		mode = FS_O_WRITE | FS_O_CREATE | FS_O_TRUNC;
	else if (!c_strcmp(modestr, "w+"))
		mode = FS_O_RDWR | FS_O_CREATE | FS_O_TRUNC;
	else
		xsUnknownError("invalid mode");

	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	char *path = appendPath(d->path, xsVar(0));

	struct fs_dirent entry;
	int result = fs_stat(path, &entry);
	if (result < 0) {
		if (!(mode & FS_O_CREATE))
			throwIf(result);
	}
	else {
		if (FS_DIR_ENTRY_FILE != entry.type)
			xsUnknownError("not file");
	}

	xsFile xf = (xsFile)c_calloc(1, sizeof(xsFileRecord) + c_strlen(path) + 1);
	xsmcSetHostData(xsResult, xf);
	xf->mode = mode;
	c_strcpy(xf->path, path);
	throwIf(fs_open(&xf->fsf, path, mode));
}

void xs_directoryzephyr_openDirectory(xsMachine *the)
{
	xsDirectory xd = xsmcGetHostData(xsThis);
	xsResult = xsNewHostInstance(xsArg(1));

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	char *path = appendPath(xd->path, xsArg(0));

	struct fs_dirent entry;
	throwIf(fs_stat(path, &entry));
	if (FS_DIR_ENTRY_DIR != entry.type)
		xsUnknownError("not directory");

	xd = c_malloc(sizeof(xsDirectoryRecord) + c_strlen(path) + 1);
	if (!xd)
		xsUnknownError("no memory");
	c_strcpy(xd->path, path);
	xsmcSetHostData(xsResult, xd);
}

void xs_directoryzephyr_delete(xsMachine *the)
{
	xsDirectory xd = xsmcGetHostData(xsThis);
	char *path = appendPath(xd->path, xsArg(0));

	struct fs_dirent entry;
	int result = fs_stat(path, &entry);
	if (result < 0) {
		if (-ENOENT == result) {
			xsmcSetFalse(xsResult);
			return;
		}
		throwIf(result);
	}

	throwIf(fs_unlink(path));
	xsmcSetTrue(xsResult);
}

void xs_directoryzephyr_move(xsMachine *the)
{
	xsDirectory xd = xsmcGetHostData(xsThis);
	char *tmp = appendPath(xd->path, xsArg(0));
	char *fromPath = c_malloc(c_strlen(tmp) + 1);
	if (!fromPath)
		xsUnknownError("no memory");
	c_strcpy(fromPath, tmp);

	xsTry {
		char *toPath;
		if (xsmcArgc > 2)
			toPath = appendPath(((xsDirectory)xsmcGetHostData(xsArg(2)))->path, xsArg(1));
		else
			toPath = appendPath(xd->path, xsArg(1));

		throwIf(fs_rename(fromPath, toPath));
	}
	xsCatch {
		c_free(fromPath);
		xsThrow(xsException);
	}
}

void xs_directoryzephyr_status(xsMachine *the)
{
	xsDirectory xd = xsmcGetHostData(xsThis);
	struct fs_dirent entry;
	char *path = appendPath(xd->path, xsArg(0));

	throwIf(fs_stat(path, &entry));

	xsResult = xsArg(2);

	xsmcVars(1);
	xsmcSetInteger(xsVar(0), entry.size);
	xsmcSet(xsResult, xsID_size, xsVar(0));

	xsmcSetInteger(xsVar(0), entry.type);
	xsmcSet(xsResult, xsID_mode, xsVar(0));
}

void xs_directoryzephyr_createDirectory(xsMachine *the)
{
	xsDirectory xd = xsmcGetHostData(xsThis);
	char *path = appendPath(xd->path, xsArg(0));

	//@@ if exists and is FILE, throw error

	int result = fs_mkdir(path);
	if (result < 0) {
		if (-EEXIST == result) {
			xsmcSetFalse(xsResult);
			return;
		}
		throwIf(result);
	}
	xsmcSetTrue(xsResult);
}

/*
	Scan
*/

void xs_directory_iterator_zephyr(xsMachine *the)
{
	xsDirectory xd = xsmcGetHostData(xsArg(0));
	char *path;
	if (xsmcArgc > 1)
		path = appendPath(xd->path, xsArg(1));
	else
		path = xd->path;

	struct fs_dir_t dir;
	fs_dir_t_init(&dir);
	throwIf(fs_opendir(&dir, path));

	xsmcSetHostChunk(xsThis, &dir, sizeof(dir));
}

void xs_directory_iterator_zephyr_destructor(void *data)
{
	struct fs_dir_t *dir = data;
	if (!dir)
		return;
	fs_closedir(dir);
}

void xs_directory_iterator_zephyr_next(xsMachine *the)
{
	xsmcVars(2);
	xsmcSetTrue(xsVar(0));
	xsmcSetUndefined(xsVar(1));
	struct fs_dir_t *dir = xsmcGetHostChunk(xsThis);
	if (dir) {
		struct fs_dirent entry;
	
		if ((0 == fs_readdir(dir, &entry)) && entry.name[0]) {
			xsmcSetFalse(xsVar(0));
			xsmcSetString(xsVar(1), entry.name);
		}
		else {
			xs_directory_iterator_zephyr_destructor(dir);
			xsmcSetHostChunk(xsThis, NULL, 0);
		}
	}
	xsResult = xsNewObject();
	xsmcDefine(xsResult, xsID_done, xsVar(0), xsDefault);
	xsmcDefine(xsResult, xsID_value, xsVar(1), xsDefault);
}

void xs_directory_iterator_zephyr_return(xsMachine *the)
{
	xsmcVars(2);
	xsmcSetTrue(xsVar(0));
	xsmcSetUndefined(xsVar(1));
	if (xsmcGetHostChunk(xsThis)) {
		xs_directory_iterator_zephyr_destructor(xsmcGetHostChunk(xsThis));
		xsmcSetHostChunk(xsThis, NULL, 0);
	}
	xsResult = xsNewObject();
	xsmcDefine(xsResult, xsID_done, xsVar(0), xsDefault);
	xsmcDefine(xsResult, xsID_value, xsVar(1), xsDefault);
}

/*
	Stat
*/

void xs_stat_isFile(xsMachine *the)
{
	xsmcGet(xsResult, xsThis, xsID_mode);
	xsmcSetBoolean(xsResult, FS_DIR_ENTRY_FILE == xsmcToInteger(xsResult));
}

void xs_stat_isDirectory(xsMachine *the)
{
	xsmcGet(xsResult, xsThis, xsID_mode);
	xsmcSetBoolean(xsResult, FS_DIR_ENTRY_DIR == xsmcToInteger(xsResult));
}

#else
void xs_directoryzephyr_bootstrap(xsMachine *) {}
void xs_stat_isFile(xsMachine *) {}
void xs_stat_isDirectory(xsMachine *) {}
void xs_filezephyr_destructor(void *) {}
void xs_filezephyr(xsMachine *) {}
void xs_filezephyr_close(xsMachine *) {}
void xs_filezephyr_read(xsMachine *) {}
void xs_filezephyr_write(xsMachine *) {}
void xs_filezephyr_status(xsMachine *) {}
void xs_filezephyr_setSize(xsMachine *) {}
void xs_filezephyr_flush(xsMachine *) {}
void xs_directory_iterator_zephyr_destructor(void *) {}
void xs_directory_iterator_zephyr(xsMachine *) {}
void xs_directory_iterator_zephyr_next(xsMachine *) {}
void xs_directory_iterator_zephyr_return(xsMachine *) {}
void xs_directoryzephyr_openFile(xsMachine *) {}
void xs_directoryzephyr_openDirectory(xsMachine *) {}
void xs_directoryzephyr_status(xsMachine *) {}
void xs_directoryzephyr_destructor(void *) {}
void xs_directoryzephyr(xsMachine *) {}
void xs_directoryzephyr_close(xsMachine *) {}
void xs_directoryzephyr_delete(xsMachine *) {}
void xs_directoryzephyr_move(xsMachine *) {}
void xs_directoryzephyr_createDirectory(xsMachine *) {}
#endif

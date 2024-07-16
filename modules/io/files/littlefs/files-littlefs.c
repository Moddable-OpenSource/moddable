/*
 * Copyright (c) 2016-2024  Moddable Tech, Inc.
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
 
#include "lfs.h"

#include "xsmc.h"
#include "xsHost.h"
#include "modInstrumentation.h"
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#if ESP32
	#include "freertos/FreeRTOS.h"
	#include "freertos/semphr.h"
	#include "spi_flash/include/spi_flash_mmap.h"
#elif nrf52
	#include "FreeRTOS.h"
	#include "semphr.h"
#elif PICO_BUILD
#endif

#ifndef MODDEF_FILE_PARTITION 
    #define MODDEF_FILE_PARTITION "storage"
#endif
#ifndef MODDEF_FILE_LFS_READ_SIZE
	#define MODDEF_FILE_LFS_READ_SIZE 16
#endif
#ifndef MODDEF_FILE_LFS_PROG_SIZE
	#define MODDEF_FILE_LFS_PROG_SIZE 16
#endif
#ifndef MODDEF_FILE_LFS_CACHE_SIZE
	#define MODDEF_FILE_LFS_CACHE_SIZE 16
#endif
#ifndef MODDEF_FILE_LFS_LOOKAHEAD_SIZE
	#define MODDEF_FILE_LFS_LOOKAHEAD_SIZE 16
#endif
#ifndef MODDEF_FILE_LFS_BLOCK_CYCLES
	#define MODDEF_FILE_LFS_BLOCK_CYCLES 500
#endif
#ifndef MODDEF_FILE_LFS_PARTITION_SIZE  
	#define MODDEF_FILE_LFS_PARTITION_SIZE (131072)
#endif

#ifdef __ets__
	#if !ESP32
		#define MOD_LFS_COPYTORAM 1
	#endif
#elif nrf52 || PICO_BUILD
#else
	#define MOD_LFS_RAMDISK 1

	#define kBlockSize (4096)
	#define kBlockCount ((MODDEF_FILE_LFS_PARTITION_SIZE + kBlockSize - 1) / kBlockSize)

	uint8_t gRAMDisk[kBlockSize * kBlockCount]; 
#endif

typedef struct {
	struct lfs_config		config;
	lfs_t					lfs;

	uint32_t				useCount;

#if ESP32
	const esp_partition_t	*partition;
#elif nrf52 || PICO_BUILD
	uint32_t				offset;
#endif
} xsLittleFSRecord, *xsLittleFS;

static void startLittlefs(xsMachine *the);
static void stopLittlefs(void);
static void lfs_error(xsMachine *the, int code);

static xsLittleFS gLFS;

#if defined(INC_FREERTOS_H) && defined(LFS_THREADSAFE) 
	static SemaphoreHandle_t gLFSMutex;
#endif

/*
	helpers
*/

#define throwIf(a) _throwIf(the, a)

static void _throwIf(xsMachine *the, int result)
{
	if (result < 0)
		lfs_error(the, result);
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

#define appendPath(dirPath, slot) _appendPath(the, dirPath, &slot)

static char *_appendPath(xsMachine *the, char *dirPath, xsSlot *slot)
{
	size_t dirLen = c_strlen(dirPath);
	char *path = _getPath(the, slot);
	char *result = fxNewChunk(the, dirLen + c_strlen(path) + 1);
	path = xsmcToString(*slot);		// refresh
	c_strcpy(result, dirPath);
	c_strcat(result, path);
	size_t resultLen = c_strlen(result);
	if ('/' == result[resultLen - 1])
		result[resultLen - 1] = 0;		// remove trainling slash
	return result;
}


/*
	File
*/

void xs_filelittlefs_destructor(void *data)
{
	lfs_file_t *f = data;
	if (f) {
		lfs_file_close(&gLFS->lfs, f);
		stopLittlefs();
	}
}

#define getFile(slot) ((lfs_file_t *)xsmcGetHostDataValidate(slot, xs_filelittlefs_destructor))

void xs_filelittlefs(xsMachine *the)
{
	xsUnknownError("use openFile");
}

void xs_filelittlefs_close(xsMachine *the)
{
	if (!xsGetHostDataIf(xsThis)) 
		return;

	xs_filelittlefs_destructor(getFile(xsThis));
	xsmcSetHostData(xsThis, NULL);
	xsSetHostDestructor(xsThis, NULL);
}

void xs_filelittlefs_read(xsMachine *the)
{
	lfs_file_t *file = getFile(xsThis);
	int position = xsmcToInteger(xsArg(1));
	void *buffer;
	xsUnsignedValue length;
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

	throwIf(lfs_file_seek(&gLFS->lfs, file, position, LFS_SEEK_SET));
	int result = lfs_file_read(&gLFS->lfs, file, buffer, length);
	throwIf(result);

	if (returnLength)
		xsmcSetInteger(xsResult, result);
	else if ((xsUnsignedValue)result != length)
		xsmcSetArrayBufferLength(xsResult, result);
}

void xs_filelittlefs_write(xsMachine *the)
{
	lfs_file_t *file = getFile(xsThis);
	if (!(LFS_O_WRONLY & file->flags))		// with asserts disabled, LFS allows truncate on read-only file!
		xsUnknownError("read-only");

	int position = xsmcToInteger(xsArg(1));
	void *buffer;
	xsUnsignedValue length;
	xsmcGetBufferWritable(xsArg(0), &buffer, &length);

	throwIf(lfs_file_seek(&gLFS->lfs, file, position, LFS_SEEK_SET));
	throwIf(lfs_file_write(&gLFS->lfs, file, buffer, length));
}

void xs_filelittlefs_status(xsMachine *the)
{
	lfs_file_t *file = getFile(xsThis);
	lfs_soff_t size =  lfs_file_size(&gLFS->lfs, file);
	throwIf(size);

	xsResult = xsArg(0);

	xsmcVars(1);
	xsmcSetInteger(xsVar(0), size);
	xsmcSet(xsResult, xsID_size, xsVar(0));

	xsmcSetInteger(xsVar(0), LFS_TYPE_REG);
	xsmcSet(xsResult, xsID_mode, xsVar(0));
}

void xs_filelittlefs_setSize(xsMachine *the)
{
	lfs_file_t *file = getFile(xsThis);
	if (!(LFS_O_WRONLY & file->flags))		// with asserts disabled, LFS allows truncate on read-only file!
		xsUnknownError("read-only");
	throwIf(lfs_file_truncate(&gLFS->lfs, file, xsmcToInteger(xsArg(0))));
}

void xs_filelittlefs_flush(xsMachine *the)
{
	lfs_file_t *file = getFile(xsThis);
	throwIf(lfs_file_sync(&gLFS->lfs, file));
}

/*
	Directory
*/

void xs_direectorylittlefs_destructor(void *data)
{
	if (data) {
		c_free(data);
		stopLittlefs();
	}
}

// directory storage is the directory path, always with a trailing "/" for concatenation with relative paths
#define getDirectory(slot) ((char *)xsmcGetHostDataValidate(slot, xs_direectorylittlefs_destructor))

void xs_direectorylittlefs(xsMachine *the)
{
	xsTrace("Directory constructor is temporary for bootstrapping. Will throw in the future.\n");

	xsmcGet(xsResult, xsArg(0), xsID_path);
	char *path = xsmcToString(xsResult);			//@@ not checked with getPath.... for bootstrap
	size_t len = c_strlen(path) + 1;
	int needsSlash = path[len - 1] != '/';
	char *p = c_malloc(len + needsSlash);
	if (!p) xsUnknownError("no memory");
	c_memcpy(p, path, len);
	if (needsSlash) {
		p[len - 1] = '/';
		p[len] = 0;
	}

	xsmcSetHostData(xsThis, p);
	startLittlefs(the);
}

void xs_direectorylittlefs_close(xsMachine *the)
{
	if (!xsGetHostDataIf(xsThis)) 
		return;

	xs_direectorylittlefs_destructor(xsmcGetHostData(xsThis));
	xsmcSetHostData(xsThis, NULL);
	xsSetHostDestructor(xsThis, NULL);
}

void xs_direectorylittlefs_openFile(xsMachine *the)
{
	char *dirPath = getDirectory(xsThis);

	xsResult = xsNewHostInstance(xsArg(1));

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_mode);
	char *modestr;
	int mode;
	if (xsUndefinedType == xsmcTypeOf(xsVar(0)))
		modestr = "r";
	else
		modestr = xsmcToString(xsVar(0));
	if (!c_strcmp(modestr, "r"))
		mode = LFS_O_RDONLY;
	else if (!c_strcmp(modestr, "r+"))
		mode = LFS_O_RDWR;
	else if (!c_strcmp(modestr, "w"))
		mode = LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC;
	else if (!c_strcmp(modestr, "w+"))
		mode = LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC;
	else
		xsUnknownError("invalid mode");

	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	char *path = appendPath(dirPath, xsVar(0));

	struct lfs_info info;
	int result = lfs_stat(&gLFS->lfs, path, &info);
	if (result < 0) {
		if (!(mode & LFS_O_CREAT))
			throwIf(result);
	}
	else {
		if (LFS_TYPE_REG != info.type)
			xsUnknownError("not file");
	}

	lfs_file_t *f = c_calloc(sizeof(lfs_file_t), 1);
	if (!f)
		xsUnknownError("no memory");

    result = lfs_file_open(&gLFS->lfs, f, path, mode);
    if (result < 0) {
		c_free(f);
		throwIf(result);
    }

	xsmcSetHostData(xsResult, f);
	startLittlefs(the);
}

void xs_direectorylittlefs_openDirectory(xsMachine *the)
{
	char *dirPath = getDirectory(xsThis);

	xsResult = xsNewHostInstance(xsArg(1));

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	char *path = appendPath(dirPath, xsVar(0));

	struct lfs_info info;
	throwIf(lfs_stat(&gLFS->lfs, path, &info));

	if (LFS_TYPE_DIR != (info.type))
		xsUnknownError("not directory");

	size_t len = c_strlen(path) + 1;
	int needsSlash = path[len - 1] != '/';
	char *p = c_malloc(len + needsSlash);
	if (!p) xsUnknownError("no memory");
	c_memcpy(p, path, len);
	if (needsSlash) {
		p[len - 1] = '/';
		p[len] = 0;
	}

	xsmcSetHostData(xsResult, p);
	startLittlefs(the);
}

void xs_direectorylittlefs_delete(xsMachine *the)
{
	char *dirPath = getDirectory(xsThis);
	char *path = appendPath(dirPath, xsArg(0));

	struct lfs_info info;
	int result = lfs_stat(&gLFS->lfs, path, &info);
	if (result < 0) {
		if (LFS_ERR_NOENT == result) {
			xsmcSetFalse(xsResult);
			return;
		}
		throwIf(result);
	}

	throwIf(lfs_remove(&gLFS->lfs, path));
	xsmcSetTrue(xsResult);
}

void xs_direectorylittlefs_move(xsMachine *the)
{
	char *fromPath = C_NULL, *toPath = C_NULL;

	xsTry {
		char *t = getDirectory(xsThis);
		t = appendPath(t, xsArg(0));
		fromPath = c_malloc(c_strlen(t) + 1);
		c_strcpy(fromPath, t);

		t = (xsmcArgc > 2) ? getDirectory(xsArg(2)) : getDirectory(xsThis);
		t = appendPath(t, xsArg(1));
		toPath = c_malloc(c_strlen(t) + 1);
		c_strcpy(toPath, t);

		throwIf(lfs_rename(&gLFS->lfs, fromPath, toPath));

		if (fromPath) c_free(fromPath);
		if (toPath) c_free(toPath);
	}
	xsCatch {
		if (fromPath) c_free(fromPath);
		if (toPath) c_free(toPath);
		xsThrow(xsException);
	}

}

void xs_direectorylittlefs_status(xsMachine *the)
{
	char *dirPath = getDirectory(xsThis);
	struct lfs_info info;
	char *path = appendPath(dirPath, xsArg(0));

	throwIf(lfs_stat(&gLFS->lfs, path, &info));

	xsResult = xsArg(1);

	xsmcVars(1);
	xsmcSetInteger(xsVar(0), (LFS_TYPE_REG == info.type) ? info.size : 0);		// littlefs doesn't write to size field for directory
	xsmcSet(xsResult, xsID_size, xsVar(0));

	xsmcSetInteger(xsVar(0), info.type);
	xsmcSet(xsResult, xsID_mode, xsVar(0));
}

void xs_direectorylittlefs_createDirectory(xsMachine *the)
{
	char *dirPath = getDirectory(xsThis);
	char *path = appendPath(dirPath, xsArg(0));

	int result = lfs_mkdir(&gLFS->lfs, path);
	if (result < 0) {
		if (LFS_ERR_EXIST == result) {
			xsmcSetFalse(xsResult);
			return;
		}
		throwIf(result);
	}
	xsmcSetTrue(xsResult);
}

void xs_direectorylittlefs_link(xsMachine *the)
{
	getDirectory(xsThis);	// reject instance with invalid "this" first, as exepected by tests
	xsUnknownError("unsupported");
}

/*
	Scan
*/

void xs_direectorylittlefs_scan(xsMachine *the)
{
	char *dirPath = getDirectory(xsThis);

	xsResult = xsNewHostInstance(xsArg(1));

	char *path = appendPath(dirPath, xsArg(0));

	struct lfs_info info;
	throwIf(lfs_stat(&gLFS->lfs, path, &info));

	if (LFS_TYPE_DIR != info.type)
		xsUnknownError("not directory");

	lfs_dir_t *d = c_calloc(1, sizeof(lfs_dir_t));
	if (!d)
		xsUnknownError("no memory");

	int result = lfs_dir_open(&gLFS->lfs, d, path);
	if (result < 0) {
		c_free(d);
		throwIf(result);
	}
	xsmcSetHostData(xsResult, d);
	startLittlefs(the);
}

void xs_scanlittlefs_destructor(void *data)
{
	lfs_dir_t *d = data;
	if (!d) return;

	lfs_dir_close(&gLFS->lfs, d);
	c_free(d);
	stopLittlefs();
}

#define getScan(slot) ((lfs_dir_t *)xsmcGetHostDataValidate(slot, xs_scanlittlefs_destructor))

void xs_scanlittlefs(xsMachine *the)
{
	xsUnknownError("use scan");
}

void xs_scanlittlefs_close(xsMachine *the)
{
	if (!xsGetHostDataIf(xsThis)) 
		return;

	xs_scanlittlefs_destructor(xsmcGetHostDataValidate(xsThis, xs_scanlittlefs_destructor));
	xsmcSetHostData(xsThis, NULL);
	xsSetHostDestructor(xsThis, NULL);
}

void xs_scanlittlefs_read(xsMachine *the)
{
	lfs_dir_t *d = getScan(xsThis);

	while (true) {
		struct lfs_info info;
		if (lfs_dir_read(&gLFS->lfs, d, &info) <= 0) {
			xs_scanlittlefs_close(the);
			return;
		}

		if (!c_strcmp(info.name, ".") || !c_strcmp(info.name, ".."))
			continue;

		xsmcSetString(xsResult, info.name);
		break;
	}
}

/*
	Stat
*/

void xs_stat_isFile(xsMachine *the)
{
	xsmcGet(xsResult, xsThis, xsID_mode);
	xsmcSetBoolean(xsResult, LFS_TYPE_REG == xsmcToInteger(xsResult));
}

void xs_stat_isDirectory(xsMachine *the)
{
	xsmcGet(xsResult, xsThis, xsID_mode);
	xsmcSetBoolean(xsResult, LFS_TYPE_DIR == xsmcToInteger(xsResult));
}

void xs_stat_isSymbolicLink(xsMachine *the)
{
	xsmcSetFalse(xsResult);
}




#ifdef mxDebug

struct lfsError {
	int32_t		code;
	char		*msg;
};

const struct lfsError gLFSErrors[] ICACHE_RODATA_ATTR = {
	{LFS_ERR_IO, "Error during device operation"},
	{LFS_ERR_CORRUPT, "Corrupted"},
	{LFS_ERR_NOENT, "No directory entry"},
	{LFS_ERR_EXIST, "Entry already exists"},
	{LFS_ERR_NOTDIR, "Entry is not a dir"},
	{LFS_ERR_ISDIR, "Entry is a dir"},
	{LFS_ERR_NOTEMPTY, "Dir is not empty"},
	{LFS_ERR_BADF, "Bad file number"},
	{LFS_ERR_FBIG, "File too large"},
	{LFS_ERR_INVAL, "Invalid parameter"},
	{LFS_ERR_NOSPC, "No space left on device"},
	{LFS_ERR_NOMEM, "No more memory available"},
	{LFS_ERR_NOATTR, "No data/attr available"},
	{LFS_ERR_NAMETOOLONG, "File name too long"},
	{0, NULL}
};

void lfs_error(xsMachine *the, int code)
{
	const struct lfsError *e;

	if (0 == code)
		return;

	for (e = gLFSErrors; e->code; e++) {
		if (e->code == code)
			xsUnknownError(e->msg);
	}

	xsUnknownError("LFS ERR");
}

#else

void lfs_error(xsMachine *the, int code)
{
	char msg[64];
	xsmcSetString(xsResult, "LFS_ERR: ");
	xsResult = xsCall1(xsResult, xsID_concat, xsInteger(code));
	xsmcToStringBuffer(xsResult, msg, sizeof(msg));
	xsUnknownError(msg);
}

#endif

#if MOD_LFS_RAMDISK

static int lfs_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    c_memcpy(buffer, &gRAMDisk[block * cfg->block_size + off], size);

    return 0;
}

static int lfs_prog(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    c_memcpy(&gRAMDisk[block * cfg->block_size + off], buffer, size);
    
    return 0;
}

static int lfs_erase(const struct lfs_config *cfg, lfs_block_t block)
{
	c_memset(&gRAMDisk[block * cfg->block_size], -1, cfg->block_size);

	return 0;
}

static int lfs_sync(const struct lfs_config *cfg)
{
	return 0;
}

#elif ESP32

static int lfs_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    xsLittleFS lfs = cfg->context;

	if (ESP_OK != esp_partition_read(lfs->partition, block * cfg->block_size + off, buffer, size))
		return -1;

    return 0;
}

static int lfs_prog(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    xsLittleFS lfs = cfg->context;

	if (ESP_OK != esp_partition_write(lfs->partition, block * cfg->block_size + off, buffer, size))
		return -1;
    
    return 0;
}

static int lfs_erase(const struct lfs_config *cfg, lfs_block_t block)
{
    xsLittleFS lfs = cfg->context;

	if (ESP_OK != esp_partition_erase_range(lfs->partition, block * SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE))
		return -1;

	return 0;
}

static int lfs_sync(const struct lfs_config *cfg)
{
	return 0;
}

#elif defined(__ets__)

extern uint32_t _SPIFFS_start;
extern uint32_t _SPIFFS_end;

#define SPIFFS_PHYS_ADDR ((uint32_t) (&_SPIFFS_start) - 0x40200000)
#define SPIFFS_PHYS_SIZE ((uint32_t) (&_SPIFFS_end) - (uint32_t) (&_SPIFFS_start))

static int lfs_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
	if (!modSPIRead(block * kFlashSectorSize + off + SPIFFS_PHYS_ADDR, size, buffer))
		return -1;

    return 0;
}

static int lfs_prog(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
	if (!modSPIWrite(block * kFlashSectorSize + off + SPIFFS_PHYS_ADDR, size, buffer))
		return -1;
    
    return 0;
}

static int lfs_erase(const struct lfs_config *cfg, lfs_block_t block)
{
	if (!modSPIErase(block * kFlashSectorSize + SPIFFS_PHYS_ADDR, cfg->block_size))
		return -1;

	return 0;
}

static int lfs_sync(const struct lfs_config *cfg)
{
	return 0;
}

#elif nrf52 || PICO_BUILD

static int lfs_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    xsLittleFS lfs = cfg->context;

	if (!modSPIRead(block * kFlashSectorSize + off + lfs->offset, size, buffer))
		return -1;

    return 0;
}

static int lfs_prog(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    xsLittleFS lfs = cfg->context;

	if (!modSPIWrite(block * kFlashSectorSize + off + lfs->offset, size, buffer))
		return -1;
    
    return 0;
}

static int lfs_erase(const struct lfs_config *cfg, lfs_block_t block)
{
    xsLittleFS lfs = cfg->context;

	if (!modSPIErase(block * kFlashSectorSize + lfs->offset, cfg->block_size))
		return -1;

	return 0;
}

static int lfs_sync(const struct lfs_config *cfg)
{
	return 0;
}

#endif

#if defined(INC_FREERTOS_H) && defined(LFS_THREADSAFE)

static int lfs_lock(const struct lfs_config *cfg)
{
	if (gLFS)
		xSemaphoreTake(gLFSMutex, portMAX_DELAY);
	return 0;
}

static int lfs_unlock(const struct lfs_config *cfg)
{
	if (gLFS)
		xSemaphoreGive(gLFSMutex);
	return 0;
}

#elif defined(LFS_THREADSAFE)

static int lfs_lock(const struct lfs_config *cfg)
{
	return 0;
}

static int lfs_unlock(const struct lfs_config *cfg)
{
	return 0;
}

#endif

void startLittlefs(xsMachine *the)
{
	if (gLFS) {
		gLFS->useCount += 1;
		return;
	}

#if defined(INC_FREERTOS_H) && defined(LFS_THREADSAFE)
	if (!gLFSMutex) {
		modCriticalSectionBegin();
		if (!gLFSMutex)
			gLFSMutex = xSemaphoreCreateMutex();
		xSemaphoreTake(gLFSMutex, portMAX_DELAY);
		modCriticalSectionEnd();
	}
	else
		xSemaphoreTake(gLFSMutex, portMAX_DELAY);
#endif

#if ESP32
	const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, MODDEF_FILE_PARTITION);
	if (!partition) {
#if defined(INC_FREERTOS_H) && defined(LFS_THREADSAFE)
		xSemaphoreGive(gLFSMutex);
#endif
		xsUnknownError("can't find partition");
	}
#elif nrf52 || PICO_BUILD
	uint32_t storageOffset, storageSize;
	if (!modGetPartition(kPartitionStorage, &storageOffset, &storageSize))
		xsUnknownError("no storage");
#endif

	xsLittleFS lfs = (xsLittleFS)c_calloc(sizeof(xsLittleFSRecord), 1);
	if (NULL == lfs) {
#if defined(INC_FREERTOS_H) && defined(LFS_THREADSAFE)
		xSemaphoreGive(gLFSMutex);
#endif
		xsUnknownError("no memory");
	}

	lfs->useCount = 1;
	lfs->config.context = lfs;
#if ESP32
	lfs->partition = partition;
#endif

	lfs->config.read  = lfs_read;
    lfs->config.prog  = lfs_prog;
    lfs->config.erase = lfs_erase;
    lfs->config.sync  = lfs_sync,
#if defined(LFS_THREADSAFE)
    lfs->config.lock  = lfs_lock,
    lfs->config.unlock  = lfs_unlock,
#endif

    // block device configuration
    lfs->config.read_size = MODDEF_FILE_LFS_READ_SIZE;
    lfs->config.prog_size = MODDEF_FILE_LFS_PROG_SIZE;
#if MOD_LFS_RAMDISK
    lfs->config.block_size = kBlockSize;
    lfs->config.block_count = kBlockCount;
#elif ESP32
    lfs->config.block_size = SPI_FLASH_SEC_SIZE;
    lfs->config.block_count = lfs->partition->size / SPI_FLASH_SEC_SIZE;
#elif defined(__ets__)
    lfs->config.block_size = kFlashSectorSize;
    lfs->config.block_count = SPIFFS_PHYS_SIZE / kFlashSectorSize;
#elif nrf52 || PICO_BUILD
	lfs->offset = storageOffset;
    lfs->config.block_size = kFlashSectorSize;
    lfs->config.block_count = storageSize / kFlashSectorSize;
#endif
    lfs->config.cache_size = MODDEF_FILE_LFS_CACHE_SIZE;
    lfs->config.lookahead_size = MODDEF_FILE_LFS_LOOKAHEAD_SIZE;
    lfs->config.block_cycles = MODDEF_FILE_LFS_BLOCK_CYCLES;

    int err = lfs_mount(&lfs->lfs, &lfs->config);
    if (err) {
		lfs_format(&lfs->lfs, &lfs->config);
		err = lfs_mount(&lfs->lfs, &lfs->config);
    }
	if (err)
		c_free(lfs);
	else
		gLFS = lfs;

#if defined(INC_FREERTOS_H) && defined(LFS_THREADSAFE)
	xSemaphoreGive(gLFSMutex);
#endif

	if (err)
		lfs_error(the, err);
}

void stopLittlefs(void)
{
	xsLittleFS lfs = gLFS;
	if (!lfs)
		return;

#if defined(INC_FREERTOS_H) && defined(LFS_THREADSAFE)
	xSemaphoreTake(gLFSMutex, portMAX_DELAY);
#endif

	lfs->useCount -= 1;
	if (0 == lfs->useCount) {
		gLFS = NULL;

		lfs_unmount(&lfs->lfs);

		c_free(lfs);
	}

#if defined(INC_FREERTOS_H) && defined(LFS_THREADSAFE)
	xSemaphoreGive(gLFSMutex);
#endif
}

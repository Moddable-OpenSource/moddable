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
 
#include "lfs.h"

#include "xsmc.h"
#include "xsHost.h"
#include "modInstrumentation.h"
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#if ESP32
	#include "freertos/FreeRTOS.h"
	#include "freertos/semphr.h"
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

#ifdef __ets__
	#if !ESP32
		#define MOD_LFS_COPYTORAM 1
	#endif
#else
	#define MOD_LFS_RAMDISK 1

	#define kBlockSize (4096)
	#define kBlockCount (16)
	
	uint8_t gRAMDisk[kBlockSize * kBlockCount]; 
#endif

static const xsHostHooks modLittlefsHooksFile ICACHE_RODATA_ATTR = {
	xs_file_destructor,
	NULL,
	NULL
};

static const xsHostHooks modLittlefsHooksIterator ICACHE_RODATA_ATTR = {
	xs_file_iterator_destructor,
	NULL,
	NULL
};

typedef struct {
	struct lfs_config		config;
	lfs_t					lfs;
	
	uint32_t				useCount;

#if ESP32
	const esp_partition_t	*partition;
#endif
} xsLittleFSRecord, *xsLittleFS;

static void startLittlefs(xsMachine *the);
static void stopLittlefs(void);

static xsLittleFS gLFS;

#if defined(INC_FREERTOS_H) && defined(LFS_THREADSAFE) 
	static SemaphoreHandle_t gLFSMutex;
#endif

void xs_file_destructor(void *data)
{
	lfs_file_t *file = data;
	if (file) {
		lfs_file_close(&gLFS->lfs, file);
		c_free(file);
		stopLittlefs();
		modInstrumentationAdjust(Files, -1);
	}
}

void xs_File(xsMachine *the)
{
	int argc = xsmcArgc;
	lfs_file_t *file;
	uint8_t write = (argc < 2) ? 0 : xsmcToBoolean(xsArg(1));;
	int err;
#if MOD_LFS_COPYTORAM
	char path[LFS_NAME_MAX + 1];
	xsmcToStringBuffer(xsArg(0), path, sizeof(path));
#else
	char *path = xsmcToString(xsArg(0));
#endif

	startLittlefs(the);

	file = c_calloc(sizeof(lfs_file_t), 1);
	if (!file) {
		stopLittlefs();
		xsUnknownError("no memory");
	}

    err = lfs_file_open(&gLFS->lfs, file, path, write ? (LFS_O_RDWR | LFS_O_CREAT) : LFS_O_RDONLY);
	if (err < 0) {
		c_free(file);
		stopLittlefs();
		xsUnknownError("file not found");
	}
	xsmcSetHostData(xsThis, file);
	xsSetHostHooks(xsThis, &modLittlefsHooksFile);

	modInstrumentationAdjust(Files, +1);
}

void xs_file_read(xsMachine *the)
{
	lfs_file_t *file = xsmcGetHostDataValidate(xsThis, (void *)&modLittlefsHooksFile);
	int result;
	int argc = xsmcArgc;
	int dstLen = (argc < 2) ? -1 : xsmcToInteger(xsArg(1));
	void *dst;
	xsSlot *s1, *s2;
	lfs_soff_t size = lfs_file_size(&gLFS->lfs, file);
	lfs_soff_t position = lfs_file_tell(&gLFS->lfs, file);

	if ((-1 == dstLen) || (size < (position + dstLen))) {
		if (position >= size)
			xsUnknownError("read past end of file");
		dstLen = size - position;
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

	result = lfs_file_read(&gLFS->lfs, file, dst, dstLen);
	if (result != dstLen)
		xsUnknownError("file read failed");
}

void xs_file_write(xsMachine *the)
{
	lfs_file_t *file = xsmcGetHostDataValidate(xsThis, (void *)&modLittlefsHooksFile);
	int result;
	int argc = xsmcArgc, i;

	for (i = 0; i < argc; i++) {
		unsigned char *src;
		xsUnsignedValue srcLen;
		int type = xsmcTypeOf(xsArg(i));
		uint8_t temp;

		if (xsStringType == type) {
			src = (unsigned char *)xsmcToString(xsArg(i));
			srcLen = c_strlen((char *)src);
		}
		else if ((xsIntegerType == type) || (xsNumberType == type)) {
			temp = (uint8_t)xsmcToInteger(xsArg(i));
			src = (unsigned char *)&temp;
			srcLen = 1;
		}
		else
			xsmcGetBufferReadable(xsArg(i), (void **)&src, &srcLen);

#if MOD_LFS_COPYTORAM
		while (srcLen) {	// spool through RAM for data in flash
			unsigned char *buffer[128];
			int use = (srcLen <= sizeof(buffer)) ? srcLen : 128;

			c_memcpy(buffer, src, use);
			src += use;
			srcLen -= use;

			result = lfs_file_write(&gLFS->lfs, file, buffer, use);
			if (result != use)
				xsUnknownError("file write failed");
		}
#else
		result = lfs_file_write(&gLFS->lfs, file, src, srcLen);
		if (result != (int)srcLen)
			xsUnknownError("file write failed");
#endif
	}
}

void xs_file_close(xsMachine *the)
{
	lfs_file_t *file = xsmcGetHostDataValidate(xsThis, (void *)&modLittlefsHooksFile);
	xs_file_destructor(file);
	xsmcSetHostData(xsThis, NULL);
	xsmcSetHostDestructor(xsThis, NULL);
}

void xs_file_get_length(xsMachine *the)
{
	lfs_file_t *file = xsmcGetHostDataValidate(xsThis, (void *)&modLittlefsHooksFile);
	lfs_soff_t size = lfs_file_size(&gLFS->lfs, file);
	if (size < 0)
		xsUnknownError("failed");
	xsmcSetInteger(xsResult, size);
}

void xs_file_get_position(xsMachine *the)
{
	lfs_file_t *file = xsmcGetHostDataValidate(xsThis, (void *)&modLittlefsHooksFile);
	lfs_soff_t position = lfs_file_tell(&gLFS->lfs, file);
	if (position < 0)
		xsUnknownError("failed");
	xsmcSetInteger(xsResult, position);
}

void xs_file_set_position(xsMachine *the)
{
	lfs_file_t *file = xsmcGetHostDataValidate(xsThis, (void *)&modLittlefsHooksFile);
	lfs_soff_t position = xsmcToInteger(xsArg(0));
	position = lfs_file_seek(&gLFS->lfs, file, position, LFS_SEEK_SET);
	if (position < 0)
		xsUnknownError("failed");
}

void xs_file_delete(xsMachine *the)
{
	int result;
#if MOD_LFS_COPYTORAM
	char path[LFS_NAME_MAX + 1];
	xsmcToStringBuffer(xsArg(0), path, sizeof(path));
#else
	char *path = xsmcToString(xsArg(0));
#endif

	startLittlefs(the);

	result = lfs_remove(&gLFS->lfs, path);

	stopLittlefs();

	xsmcSetBoolean(xsResult, result >= 0);
}

void xs_file_exists(xsMachine *the)
{
	int result;
	struct lfs_info info;
#if MOD_LFS_COPYTORAM
	char path[LFS_NAME_MAX + 1];
	xsmcToStringBuffer(xsArg(0), path, sizeof(path));
#else
	char *path = xsmcToString(xsArg(0));
#endif

	startLittlefs(the);

	result = lfs_stat(&gLFS->lfs, path, &info);

	stopLittlefs();

	xsmcSetBoolean(xsResult, result >= 0);
}

void xs_file_rename(xsMachine *the)
{
	char path[LFS_NAME_MAX + 1];
	char toPath[LFS_NAME_MAX + 1];
	int result;
	
	xsmcToStringBuffer(xsArg(0), path, sizeof(path));
	xsmcToStringBuffer(xsArg(1), toPath, sizeof(toPath));
	if ('/' != toPath[0]) {
		if (c_strchr(toPath + 1, '/'))
			xsUnknownError("invalid to");

		char *slash = c_strrchr(path, '/');
		if (!slash)
			xsUnknownError("invalid from");

		size_t pathLength = slash - path + 1;
		if (pathLength >= (c_strlen(path) + sizeof(toPath)))		//@@ sizeof(toPath)??
			xsUnknownError("path too long");

		c_strcpy(toPath, path);
		xsmcToStringBuffer(xsArg(1), toPath + pathLength, sizeof(toPath) - pathLength);
	}

	startLittlefs(the);
	result = lfs_rename(&gLFS->lfs, path, toPath);
	stopLittlefs();

	xsmcSetBoolean(xsResult, result >= 0);
}

void xs_directory_create(xsMachine *the)
{
	int result;
#if MOD_LFS_COPYTORAM
	char path[LFS_NAME_MAX + 1];
	xsmcToStringBuffer(xsArg(0), path, sizeof(path));
#else
	char *path = xsmcToString(xsArg(0));
#endif

	startLittlefs(the);
	result = lfs_mkdir(&gLFS->lfs, path);
	stopLittlefs();

	if (result < 0) {
		if (LFS_ERR_EXIST != result)
			xsUnknownError("failed");
	}
}

void xs_directory_delete(xsMachine *the)
{
	int result;
#if MOD_LFS_COPYTORAM
	char path[LFS_NAME_MAX + 1];
	xsmcToStringBuffer(xsArg(0), path, sizeof(path));
#else
	char *path = xsmcToString(xsArg(0));
#endif

	startLittlefs(the);

	result = lfs_remove(&gLFS->lfs, path);

	stopLittlefs();

	xsmcSetBoolean(xsResult, result >= 0);
}

void xs_file_iterator_destructor(void *data)
{
	lfs_dir_t *d = data;

	if (d) {
		lfs_dir_close(&gLFS->lfs, d);
		c_free(d);
		stopLittlefs();
		modInstrumentationAdjust(Files, -1);
	}
}

void xs_File_Iterator(xsMachine *the)
{
	lfs_dir_t *d;
#if MOD_LFS_COPYTORAM
	char path[LFS_NAME_MAX + 1];
	xsmcToStringBuffer(xsArg(0), path, sizeof(path));
#else
	char *path = xsmcToString(xsArg(0));
#endif

	startLittlefs(the);

	d = c_calloc(1, sizeof(lfs_dir_t));
	if (!d) {
		stopLittlefs();
		xsUnknownError("no memory for iterator");
	}

	if (lfs_dir_open(&gLFS->lfs, d, path) < 0) {
		c_free(d);
		stopLittlefs();
		xsUnknownError("failed to open directory");
	}
	xsmcSetHostData(xsThis, d);
	xsSetHostHooks(xsThis, &modLittlefsHooksIterator);

	modInstrumentationAdjust(Files, +1);
}

void xs_file_iterator_next(xsMachine *the)
{
	lfs_dir_t *d = xsmcGetHostDataValidate(xsThis, (void *)&modLittlefsHooksIterator);
	struct lfs_info info;

	if (lfs_dir_read(&gLFS->lfs, d, &info) <= 0) {
		xs_file_iterator_destructor(d);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
		return;
	}

	xsmcSetNewObject(xsResult);
	xsmcVars(1);
	xsmcSetString(xsVar(0), info.name);
	xsmcSet(xsResult, xsID_name, xsVar(0));
	if (LFS_TYPE_REG == info.type) {
		xsmcSetInteger(xsVar(0), info.size);
		xsmcSet(xsResult, xsID_length, xsVar(0));
	}
}

void xs_file_system_config(xsMachine *the)
{
	xsmcSetNewObject(xsResult);
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), LFS_NAME_MAX);
	xsmcSet(xsResult, xsID_maxPathLength, xsVar(0));
}

void xs_file_system_info(xsMachine *the)
{
	lfs_ssize_t used, block_size, block_count;

	startLittlefs(the);

	used = lfs_fs_size(&gLFS->lfs);

	block_size = gLFS->config.block_size;
	block_count = gLFS->config.block_count;

	stopLittlefs();

	if (used < 0)
		xsUnknownError("system info failed");

	xsmcSetNewObject(xsResult);
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), block_count * block_size);
	xsmcSet(xsResult, xsID_total, xsVar(0));
	xsmcSetInteger(xsVar(0), used * block_size);
	xsmcSet(xsResult, xsID_used, xsVar(0));
}

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
		xsUnknownError("mount failed");
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

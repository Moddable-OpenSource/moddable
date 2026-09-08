/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
#include "modInstrumentation.h"
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

/*
	configuration
*/

#ifndef MODDEF_FILE_ROOT
	#define MODDEF_FILE_ROOT "/mod"
#endif
#ifndef MODDEF_FILE_PARTITION
	#define MODDEF_FILE_PARTITION "storage"
#endif
#ifndef MODDEF_FILE_MAXOPEN
	#define MODDEF_FILE_MAXOPEN (5)
#endif
#ifndef MODDEF_FILE_FAT32
	#define MODDEF_FILE_FAT32 0
#endif
#ifndef MODDEF_FILE_SDSPI
	#define MODDEF_FILE_SDSPI 0
#endif
#ifndef MODDEF_FILE_SDMMC
	#define MODDEF_FILE_SDMMC 0
#endif
#ifndef MODDEF_FILE_LITTLEFS
	#define MODDEF_FILE_LITTLEFS 0
#endif

#if MODDEF_FILE_SDSPI || MODDEF_FILE_SDMMC
	#define kMountSDCard 1
	#define kMountFAT 1
#elif MODDEF_FILE_FAT32
	#define kMountSDCard 0
	#define kMountFAT 1
#else
	#define kMountSDCard 0
	#define kMountFAT 0
#endif

#ifndef MODDEF_FILE_FORMAT
	#if kMountSDCard
		#define MODDEF_FILE_FORMAT 0		// never format a removable card unless asked
	#else
		#define MODDEF_FILE_FORMAT 1
	#endif
#endif

// the SD card slot is described by the target's "sdcard" defines, like "spi" and "i2c"
#if kMountSDCard
	#ifndef MODDEF_SDCARD_HZ
		#define MODDEF_SDCARD_HZ (20000000)
	#endif
	#ifndef MODDEF_SDCARD_CD
		#define MODDEF_SDCARD_CD (-1)
	#endif
#endif

#if MODDEF_FILE_SDSPI
	#include "esp_vfs_fat.h"
	#include "sdmmc_cmd.h"
	#include "driver/sdspi_host.h"
	#include "driver/spi_master.h"

	#ifndef MODDEF_SDCARD_CS
		#error "sdcard.cs required"
	#endif
	#ifndef MODDEF_SDCARD_SPI_PORT
		#define MODDEF_SDCARD_SPI_PORT SDSPI_DEFAULT_HOST
	#endif
	#ifndef MODDEF_SDCARD_MISO
		#ifdef MODDEF_SPI_MISO_PIN
			#define MODDEF_SDCARD_MISO MODDEF_SPI_MISO_PIN
		#else
			#error "sdcard.miso or spi.miso_pin required"
		#endif
	#endif
	#ifndef MODDEF_SDCARD_MOSI
		#ifdef MODDEF_SPI_MOSI_PIN
			#define MODDEF_SDCARD_MOSI MODDEF_SPI_MOSI_PIN
		#else
			#error "sdcard.mosi or spi.mosi_pin required"
		#endif
	#endif
	#ifndef MODDEF_SDCARD_SCK
		#ifdef MODDEF_SPI_SCK_PIN
			#define MODDEF_SDCARD_SCK MODDEF_SPI_SCK_PIN
		#else
			#error "sdcard.sck or spi.sck_pin required"
		#endif
	#endif
#elif MODDEF_FILE_SDMMC
	#include "esp_vfs_fat.h"
	#include "sdmmc_cmd.h"
	#include "driver/sdmmc_host.h"

	#if !SOC_SDMMC_HOST_SUPPORTED
		#error "this target has no SDMMC host: use file.sdspi"
	#endif
	#ifndef MODDEF_SDCARD_WIDTH
		#define MODDEF_SDCARD_WIDTH (1)
	#endif
	#ifndef MODDEF_SDCARD_INTERNALPULLUP
		#define MODDEF_SDCARD_INTERNALPULLUP (0)
	#endif
	#if SOC_SDMMC_USE_GPIO_MATRIX
		#if !defined(MODDEF_SDCARD_CLK) || !defined(MODDEF_SDCARD_CMD) || !defined(MODDEF_SDCARD_D0)
			#error "sdcard.clk, cmd, and d0 required"
		#endif
		#if (4 == MODDEF_SDCARD_WIDTH) && (!defined(MODDEF_SDCARD_D1) || !defined(MODDEF_SDCARD_D2) || !defined(MODDEF_SDCARD_D3))
			#error "sdcard.d1, d2, and d3 required for width 4"
		#endif
	#endif
#elif MODDEF_FILE_FAT32
	#include "esp_vfs_fat.h"
	#include "wear_levelling.h"
#elif MODDEF_FILE_LITTLEFS
	#include "esp_littlefs.h"
#else
	#include "esp_spiffs.h"
#endif

/*
	mount
*/

static uint16_t gUseCount;

#if kMountSDCard
	static sdmmc_card_t *gCard;
#endif
#if MODDEF_FILE_SDSPI
	static uint8_t gSPIBusOwned;
#endif
#if kMountFAT && !kMountSDCard
	static wl_handle_t gWLHandle = WL_INVALID_HANDLE;
#endif

static void mountFS(xsMachine *the)
{
	esp_err_t err;

	if (0 != gUseCount) {
		gUseCount += 1;
		return;
	}

#if kMountFAT
	esp_vfs_fat_mount_config_t conf = {
		.format_if_mount_failed = MODDEF_FILE_FORMAT,
		.max_files = MODDEF_FILE_MAXOPEN,
		.allocation_unit_size = 0
	};

	#if MODDEF_FILE_SDSPI
		sdmmc_host_t host = SDSPI_HOST_DEFAULT();
		host.slot = MODDEF_SDCARD_SPI_PORT;
		host.max_freq_khz = MODDEF_SDCARD_HZ / 1000;

		spi_bus_config_t bus = {
			.miso_io_num = MODDEF_SDCARD_MISO,
			.mosi_io_num = MODDEF_SDCARD_MOSI,
			.sclk_io_num = MODDEF_SDCARD_SCK,
			.quadwp_io_num = -1,
			.quadhd_io_num = -1,
			.max_transfer_sz = 4096
		};
		err = spi_bus_initialize(host.slot, &bus, SPI_DMA_CH_AUTO);
		if (ESP_OK == err)
			gSPIBusOwned = 1;
		else if (ESP_ERR_INVALID_STATE != err)		// already initialized by another driver: share it
			xsUnknownError(esp_err_to_name(err));

		sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
		slot.host_id = host.slot;
		slot.gpio_cs = MODDEF_SDCARD_CS;
		slot.gpio_cd = MODDEF_SDCARD_CD;

		err = esp_vfs_fat_sdspi_mount(MODDEF_FILE_ROOT, &host, &slot, &conf, &gCard);
		if ((ESP_OK != err) && gSPIBusOwned) {
			spi_bus_free(host.slot);
			gSPIBusOwned = 0;
		}
	#elif MODDEF_FILE_SDMMC
		sdmmc_host_t host = SDMMC_HOST_DEFAULT();
		#ifdef MODDEF_SDCARD_SLOT
			host.slot = MODDEF_SDCARD_SLOT;
		#endif
		host.max_freq_khz = MODDEF_SDCARD_HZ / 1000;
		if (1 == MODDEF_SDCARD_WIDTH)
			host.flags &= ~(SDMMC_HOST_FLAG_4BIT | SDMMC_HOST_FLAG_8BIT);

		sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
		slot.width = MODDEF_SDCARD_WIDTH;
		slot.cd = MODDEF_SDCARD_CD;
		#if SOC_SDMMC_USE_GPIO_MATRIX
			slot.clk = MODDEF_SDCARD_CLK;
			slot.cmd = MODDEF_SDCARD_CMD;
			slot.d0 = MODDEF_SDCARD_D0;
			#if 4 == MODDEF_SDCARD_WIDTH
				slot.d1 = MODDEF_SDCARD_D1;
				slot.d2 = MODDEF_SDCARD_D2;
				slot.d3 = MODDEF_SDCARD_D3;
			#endif
		#endif
		#if MODDEF_SDCARD_INTERNALPULLUP
			slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
		#endif

		err = esp_vfs_fat_sdmmc_mount(MODDEF_FILE_ROOT, &host, &slot, &conf, &gCard);
	#else
		err = esp_vfs_fat_spiflash_mount_rw_wl(MODDEF_FILE_ROOT, MODDEF_FILE_PARTITION, &conf, &gWLHandle);
	#endif
#elif MODDEF_FILE_LITTLEFS
	esp_vfs_littlefs_conf_t conf = {
		.base_path = MODDEF_FILE_ROOT,
		.partition_label = MODDEF_FILE_PARTITION,
		.format_if_mount_failed = MODDEF_FILE_FORMAT
	};
	err = esp_vfs_littlefs_register(&conf);
#else
	esp_vfs_spiffs_conf_t conf = {
		.base_path = MODDEF_FILE_ROOT,
		.partition_label = MODDEF_FILE_PARTITION,
		.max_files = MODDEF_FILE_MAXOPEN,
		.format_if_mount_failed = MODDEF_FILE_FORMAT
	};
	err = esp_vfs_spiffs_register(&conf);
#endif

	if (ESP_OK != err) {
		char msg[64];
		snprintf(msg, sizeof(msg), "mount failed: %s (0x%x)", esp_err_to_name(err), err);
		xsUnknownError(msg);
	}

	gUseCount = 1;
}

static void unmountFS(void)
{
	if (0 == gUseCount)
		return;
	if (0 != --gUseCount)
		return;

#if kMountSDCard
	esp_vfs_fat_sdcard_unmount(MODDEF_FILE_ROOT, gCard);
	gCard = NULL;
	#if MODDEF_FILE_SDSPI
		if (gSPIBusOwned) {
			spi_bus_free(MODDEF_SDCARD_SPI_PORT);
			gSPIBusOwned = 0;
		}
	#endif
#elif kMountFAT
	esp_vfs_fat_spiflash_unmount_rw_wl(MODDEF_FILE_ROOT, gWLHandle);
	gWLHandle = WL_INVALID_HANDLE;
#elif MODDEF_FILE_LITTLEFS
	esp_vfs_littlefs_unregister(MODDEF_FILE_PARTITION);
#else
	esp_vfs_spiffs_unregister(MODDEF_FILE_PARTITION);
#endif
}

/*
	helpers
*/

#define throwIf(a) do { if ((a) < 0) xsUnknownError(strerror(errno)); } while (0)

#define getPath(s) _getPath(the, &s)

enum {
	kPathStateAny,
	kPathStateSlash,
	kPathStateDot,
	kPathStateDotDot,
};

// validates a subpath string: no leading "/", no "//", no "." or ".." names. Rejects "\" because FatFs treats it as a separator.
static char *_getPath(xsMachine *the, xsSlot *slot)
{
	char *path = xsmcToString(*slot);
	char *p = path;
	int state = kPathStateSlash;

	while (true) {
		switch (c_read8(p++)) {
			case '.': 
				if (kPathStateSlash == state)
					state = kPathStateDot;
				else if (kPathStateDot == state)
					state = kPathStateDotDot;
				else
					state = kPathStateAny;
				break;
			case '/':
				if ((kPathStateSlash == state) || (kPathStateDot == state) || (kPathStateDotDot == state))
					xsUnknownError("bad path");
				state = kPathStateSlash;
				break;
			case '\\':
				xsUnknownError("bad path");
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
	size_t pathLen = c_strlen(path);
	char *result = fxNewChunk(the, dirLen + 1 + pathLen + 1);
	path = xsmcToString(*slot);		// refresh
	c_strcpy(result, dirPath);
	if (pathLen) {
		result[dirLen] = '/';
		c_strcpy(result + dirLen + 1, path);
		if ('/' == result[dirLen + pathLen])
			result[dirLen + pathLen] = 0;		// remove trailing slash
	}
	return result;
}

#define isSelf(slot) c_isEmpty(xsmcToString(slot))

// the live size of an open file. fstat is stale on esp_littlefs until the file is synced, so seek to the end instead.
static off_t fileSize(xsMachine *the, int fd)
{
	off_t size = lseek(fd, 0, SEEK_END);
	throwIf(size);
	return size;
}

// zero-fill [from, to): FatFs leaves extended regions undefined and SPIFFS cannot grow a file with truncate
static void zeroFill(xsMachine *the, int fd, off_t from, off_t to)
{
	uint8_t zeros[256];

	c_memset(zeros, 0, sizeof(zeros));
	throwIf(lseek(fd, from, SEEK_SET));
	while (from < to) {
		size_t use = to - from;
		if (use > sizeof(zeros))
			use = sizeof(zeros);
		ssize_t result = write(fd, zeros, use);
		throwIf(result);
		if ((size_t)result != use)
			xsUnknownError("partial write");
		from += result;
	}
}

/*
	File
*/

struct xsFileRecord {
	int		fd;
	int		flags;
};
typedef struct xsFileRecord xsFileRecord;
typedef struct xsFileRecord *xsFile;

void xs_filevfs_destructor(void *data)
{
	xsFile f = data;
	if (f) {
		close(f->fd);
		unmountFS();
		modInstrumentationAdjust(Files, -1);
	}
}

#define getFile(slot) ((xsFile)xsmcGetHostChunkValidate(slot, xs_filevfs_destructor))

void xs_filevfs(xsMachine *the)
{
	xsUnknownError("use openFile");
}

void xs_filevfs_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis)) 
		return;

	xs_filevfs_destructor(getFile(xsThis));
	xsmcSetHostData(xsThis, NULL);
}

void xs_filevfs_read(xsMachine *the)
{
	xsFile f = getFile(xsThis);
	int fd = f->fd;
	if (O_WRONLY == (f->flags & O_ACCMODE))
		xsUnknownError("write only");
	void *buffer;
	xsUnsignedValue length;
	off_t position = xsmcToInteger(xsArg(1));
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

	ssize_t result = 0;
	if (position < fileSize(the, fd)) {		// never seek past the end: FatFs extends a writable file when lseek does, and SPIFFS fails the seek
		throwIf(lseek(fd, position, SEEK_SET));
		result = read(fd, buffer, length);
		throwIf(result);
	}

	if (returnLength)
		xsmcSetInteger(xsResult, result);
	else if ((xsUnsignedValue)result != length)
		xsmcSetArrayBufferLength(xsResult, result);
}

void xs_filevfs_write(xsMachine *the)
{
	xsFile f = getFile(xsThis);
	int fd = f->fd;
	if (O_RDONLY == (f->flags & O_ACCMODE))
		xsUnknownError("read only");
	off_t position = xsmcToInteger(xsArg(1));
	void *buffer;
	xsUnsignedValue length;
	xsmcGetBufferReadable(xsArg(0), &buffer, &length);

	off_t size = fileSize(the, fd);
	if (position > size)
		zeroFill(the, fd, size, position);

	throwIf(lseek(fd, position, SEEK_SET));
	ssize_t result = write(fd, buffer, length);
	throwIf(result);
	if ((xsUnsignedValue)result != length)
		xsUnknownError("partial write");
}

void xs_filevfs_status(xsMachine *the)
{
	int fd = getFile(xsThis)->fd;
	struct stat st;

	throwIf(fstat(fd, &st));
	off_t size = fileSize(the, fd);

	xsResult = xsArg(0);

	xsmcVars(1);
	xsmcSetInteger(xsVar(0), size);
	xsmcSet(xsResult, xsID_size, xsVar(0));

	xsmcSetInteger(xsVar(0), st.st_mode);
	xsmcSet(xsResult, xsID_mode, xsVar(0));
}

void xs_filevfs_setSize(xsMachine *the)
{
	xsFile f = getFile(xsThis);
	int fd = f->fd;
	if (O_RDONLY == (f->flags & O_ACCMODE))
		xsUnknownError("read only");
	off_t length = xsmcToInteger(xsArg(0));

	off_t size = fileSize(the, fd);
	if (length < size)
		throwIf(ftruncate(fd, length));
	else if (length > size)
		zeroFill(the, fd, size, length);
}

void xs_filevfs_flush(xsMachine *the)
{
	int fd = getFile(xsThis)->fd;
	throwIf(fsync(fd));
}

/*
	Directory
*/

void xs_directoryvfs_destructor(void *data)
{
	if (data) {
		c_free(data);
		unmountFS();
	}
}

#define getDirectory(slot) ((char *)xsmcGetHostDataValidate(slot, xs_directoryvfs_destructor))

void xs_directoryvfs(xsMachine *the)
{
	xsUnknownError("use openDirectory");
}

void xs_directoryvfs_bootstrap(xsMachine *the)
{
	char *sub = "";

	if (xsmcTest(xsArg(1)))
		sub = xsmcToString(xsArg(1));

	mountFS(the);

	xsTry {
		size_t rootLen = c_strlen(MODDEF_FILE_ROOT), subLen = c_strlen(sub);
		int needsSlash = subLen && ('/' != sub[0]);
		char *path = c_malloc(rootLen + needsSlash + subLen + 1);
		if (!path)
			xsUnknownError("no memory");
		c_strcpy(path, MODDEF_FILE_ROOT);
		if (needsSlash)
			path[rootLen] = '/';
		c_strcpy(path + rootLen + needsSlash, sub);
		size_t len = c_strlen(path);
		while ((len > rootLen) && ('/' == path[len - 1]))
			path[--len] = 0;
		xsmcSetHostData(xsArg(0), path);

		if (len != rootLen) {		// volume root always exists, and some file systems cannot stat it
			struct stat st;
			throwIf(stat(path, &st));
			if (!S_ISDIR(st.st_mode))
				xsUnknownError("not directory");
		}
	}
	xsCatch {
		if (xsmcGetHostData(xsArg(0))) {
			c_free(xsmcGetHostData(xsArg(0)));
			xsmcSetHostData(xsArg(0), NULL);
		}
		unmountFS();
		xsThrow(xsException);
	}
}

void xs_directoryvfs_close(xsMachine *the)
{
	if (!xsGetHostDataIf(xsThis)) 
		return;

	xs_directoryvfs_destructor(getDirectory(xsThis));
	xsmcSetHostData(xsThis, NULL);
	xsSetHostDestructor(xsThis, NULL);
}

void xs_directoryvfs_openFile(xsMachine *the)
{
	char *dirPath = getDirectory(xsThis);

	xsResult = xsNewHostInstance(xsArg(1));

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_mode);
	char *modestr;
	int flags;
	if (xsUndefinedType == xsmcTypeOf(xsVar(0)))
		modestr = "r";
	else
		modestr = xsmcToString(xsVar(0));
	if (!c_strcmp(modestr, "r"))
		flags = O_RDONLY;
	else if (!c_strcmp(modestr, "r+"))
		flags = O_RDWR;
	else if (!c_strcmp(modestr, "w"))
		flags = O_WRONLY | O_CREAT | O_TRUNC;
	else if (!c_strcmp(modestr, "w+"))
		flags = O_RDWR | O_CREAT | O_TRUNC;
	else
		xsUnknownError("invalid mode");

	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	char *path = appendPath(dirPath, xsVar(0));

	struct stat st;
	int result = stat(path, &st);
	if (result < 0) {
		if (!(flags & O_CREAT))
			throwIf(result);
	}
	else {
		if (!S_ISREG(st.st_mode))
			xsUnknownError("not file");
	}

	xsFileRecord f;
	f.flags = flags;
	f.fd = open(path, flags, 0666);
	throwIf(f.fd);

	xsmcSetHostChunk(xsResult, &f, sizeof(f));
	mountFS(the);
	modInstrumentationAdjust(Files, +1);
}

void xs_directoryvfs_openDirectory(xsMachine *the)
{
	char *dirPath = getDirectory(xsThis);

	xsResult = xsNewHostInstance(xsArg(1));

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_path);
	int self = isSelf(xsVar(0));
	char *path = appendPath(dirPath, xsVar(0));

	struct stat st;
	int result = stat(path, &st);
	if (result < 0) {
		if (!self || (ENOENT != errno))
			throwIf(result);
	}
	else if (!S_ISDIR(st.st_mode))
		xsUnknownError("not directory");

	size_t len = c_strlen(path) + 1;
	char *p = c_malloc(len);
	if (!p)
		xsUnknownError("no memory");
	c_memcpy(p, path, len);

	xsmcSetHostData(xsResult, p);
	mountFS(the);
}

void xs_directoryvfs_delete(xsMachine *the)
{
	char *dirPath = getDirectory(xsThis);
	char *path = appendPath(dirPath, xsArg(0));

	struct stat st;
	int result = stat(path, &st);
	if (result < 0) {
		if (ENOENT == errno) {
			xsmcSetFalse(xsResult);
			return;
		}
		throwIf(result);
	}

	if (S_ISDIR(st.st_mode))
		throwIf(rmdir(path));
	else
		throwIf(unlink(path));
	xsmcSetTrue(xsResult);
}

void xs_directoryvfs_move(xsMachine *the)
{
	char *fromPath = C_NULL;

	xsTry {
		char *t = getDirectory(xsThis);
		t = appendPath(t, xsArg(0));
		fromPath = c_malloc(c_strlen(t) + 1);
		if (!fromPath)
			xsUnknownError("no memory");
		c_strcpy(fromPath, t);

		t = (xsmcArgc > 2) ? getDirectory(xsArg(2)) : getDirectory(xsThis);
		t = appendPath(t, xsArg(1));

		int result = rename(fromPath, t);
		if ((result < 0) && (EEXIST == errno)) {
			// FatFs refuses to rename onto an existing file. Emulate POSIX semantics: replace it.
			struct stat from, to;
			if ((0 == stat(t, &to)) && S_ISREG(to.st_mode) && (0 == stat(fromPath, &from)) && S_ISREG(from.st_mode)) {
				throwIf(unlink(t));
				result = rename(fromPath, t);
			}
			else
				errno = EEXIST;
		}
		throwIf(result);

		c_free(fromPath);
	}
	xsCatch {
		if (fromPath)
			c_free(fromPath);
		xsThrow(xsException);
	}
}

void xs_directoryvfs_status(xsMachine *the)
{
	char *dirPath = getDirectory(xsThis);
	struct stat st;
	int self = isSelf(xsArg(0));
	char *path = appendPath(dirPath, xsArg(0));

	xsResult = xsArg(2);
	xsmcVars(1);

	int result = stat(path, &st);
	if (result < 0) {
		if (ENOENT != errno)
			throwIf(result);

		xsmcSetInteger(xsVar(0), self ? S_IFDIR : 0);
		xsmcSet(xsResult, xsID_mode, xsVar(0));
		return;
	}

	xsmcSetInteger(xsVar(0), st.st_size);
	xsmcSet(xsResult, xsID_size, xsVar(0));

	xsmcSetInteger(xsVar(0), st.st_mode);
	xsmcSet(xsResult, xsID_mode, xsVar(0));
}

void xs_directoryvfs_createDirectory(xsMachine *the)
{
	char *dirPath = getDirectory(xsThis);
	char *path = appendPath(dirPath, xsArg(0));

	int result = mkdir(path, 0777);
	if (result < 0) {
		if (EEXIST == errno) {
			struct stat st;
			if ((0 == stat(path, &st)) && S_ISDIR(st.st_mode)) {
				xsmcSetFalse(xsResult);
				return;
			}
			errno = EEXIST;
		}
		throwIf(result);
	}
	xsmcSetTrue(xsResult);
}

/*
	Scan
*/

void xs_directory_iterator_vfs_destructor(void *data)
{
	DIR *dir = data;
	if (dir) {
		closedir(dir);
		unmountFS();
		modInstrumentationAdjust(Files, -1);
	}
}

void xs_directory_iterator_vfs(xsMachine *the)
{
	char *dirPath = getDirectory(xsArg(0)), *path = dirPath;
	int self = 1;

	if (xsmcArgc > 1) {
		self = isSelf(xsArg(1));
		path = appendPath(dirPath, xsArg(1));
	}

	struct stat st;
	int result = stat(path, &st);
	if (result < 0) {
		if (!self || (ENOENT != errno))
			throwIf(result);
	}
	else if (!S_ISDIR(st.st_mode))
		xsUnknownError("not directory");

	DIR *dir = opendir(path);
	if (!dir)
		xsUnknownError(strerror(errno));

	xsmcSetHostData(xsThis, dir);
	mountFS(the);
	modInstrumentationAdjust(Files, +1);
}

void xs_directory_iterator_vfs_next(xsMachine *the)
{
	xsmcVars(2);
	xsmcSetTrue(xsVar(0));
	xsmcSetUndefined(xsVar(1));
	DIR *dir = xsmcGetHostDataValidate(xsThis, xs_directory_iterator_vfs_destructor);
	if (dir) {
		struct dirent *de;
		while ((de = readdir(dir))) {
			if (c_strcmp(".", de->d_name) && c_strcmp("..", de->d_name))
				break;
		}
		if (de) {
			xsmcSetFalse(xsVar(0));
			xsmcSetString(xsVar(1), de->d_name);
		}
		else {
			xs_directory_iterator_vfs_destructor(dir);
			xsmcSetHostData(xsThis, NULL);
		}
	}
	xsmcSetNewObject(xsResult);
	xsmcDefine(xsResult, xsID_done, xsVar(0), xsDefault);
	xsmcDefine(xsResult, xsID_value, xsVar(1), xsDefault);
}

void xs_directory_iterator_vfs_return(xsMachine *the)
{
	xsmcVars(2);
	xsmcSetTrue(xsVar(0));
	xsmcSetUndefined(xsVar(1));
	DIR *dir = xsmcGetHostDataValidate(xsThis, xs_directory_iterator_vfs_destructor);
	if (dir) {
		xs_directory_iterator_vfs_destructor(dir);
		xsmcSetHostData(xsThis, NULL);
	}
	xsmcSetNewObject(xsResult);
	xsmcDefine(xsResult, xsID_done, xsVar(0), xsDefault);
	xsmcDefine(xsResult, xsID_value, xsVar(1), xsDefault);
}

/*
	Stat
*/

void xs_stat_isFile(xsMachine *the)
{
	xsmcGet(xsResult, xsThis, xsID_mode);
	xsmcSetBoolean(xsResult, S_ISREG(xsmcToInteger(xsResult)));
}

void xs_stat_isDirectory(xsMachine *the)
{
	xsmcGet(xsResult, xsThis, xsID_mode);
	xsmcSetBoolean(xsResult, S_ISDIR(xsmcToInteger(xsResult)));
}

/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

#ifdef __ets__

#include "spiffs.h"

#undef c_memcpy
#undef c_printf
#undef c_memset

#include "xsmc.h"
#include "xsesp.h"
#include "modInstrumentation.h"
#include "mc.xs.h"			// for xsID_ values

#include "malloc.h"

static int startSPIFFS(void);
static void stopSPIFFS(void);

static spiffs *gSPIFFS;

void xs_file_destructor(void *data)
{
	if (data) {
		if (-1 != (uintptr_t)data) {
			SPIFFS_close(gSPIFFS, *((spiffs_file*)(&data)));
			stopSPIFFS();
		}
		modInstrumentationAdjust(Files, -1);
	}
}

void xs_File(xsMachine *the)
{
	int argc = xsmcArgc;
	spiffs_file file;
	char path[SPIFFS_OBJ_NAME_LEN];
	uint8_t write = (argc < 2) ? 0 : xsmcToBoolean(xsArg(1));;

	startSPIFFS();

	xsmcToStringBuffer(xsArg(0), path, SPIFFS_OBJ_NAME_LEN);		// in case name is in ROM
	file = SPIFFS_open(gSPIFFS, path, write ? (SPIFFS_CREAT | SPIFFS_RDWR) : SPIFFS_RDONLY, 0);
	if (file < 0) {
		stopSPIFFS();
		xsUnknownError("file not found");
	}
	xsmcSetHostData(xsThis, (void *)((int)file));

	modInstrumentationAdjust(Files, +1);
}

void xs_file_read(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	spiffs_file file = *((spiffs_file*)(&data));
	s32_t result;
	int argc = xsmcArgc;
	int dstLen = (argc < 2) ? -1 : xsmcToInteger(xsArg(1));
	void *dst;
	xsSlot *s1, *s2;
	spiffs_stat stat;
	s32_t position = SPIFFS_lseek(gSPIFFS, file, 0, SPIFFS_SEEK_CUR);

	SPIFFS_fstat(gSPIFFS, file, &stat);
	if ((-1 == dstLen) || (stat.size < (position + dstLen))) {
		if (position >= stat.size)
			xsUnknownError("read past end of file");
		dstLen = stat.size - position;
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

	result = SPIFFS_read(gSPIFFS, file, dst, dstLen);
	if (result != dstLen)
		xsUnknownError("file read failed");
}

void xs_file_write(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	spiffs_file file = *((spiffs_file*)(&data));
	s32_t result;
	int argc = xsmcArgc, i;

	for (i = 0; i < argc; i++) {
		unsigned char *src;
		s32_t srcLen;

		if (xsStringType == xsmcTypeOf(xsArg(i))) {
			src = xsmcToString(xsArg(i));
			srcLen = espStrLen(src);
		}
		else {
			src = xsmcToArrayBuffer(xsArg(i));
			srcLen = xsGetArrayBufferLength(xsArg(i));
		}

		while (srcLen) {	// spool through RAM for data in flash
			unsigned char *buffer[128];
			int use = (srcLen <= sizeof(buffer)) ? srcLen : 128;

			espMemCpy(buffer, src, use);
			src += use;
			srcLen -= use;

			result = SPIFFS_write(gSPIFFS, file, buffer, use);
			if (result != use)
				xsUnknownError("file write failed");
		}
	}
}

void xs_file_close(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	spiffs_file file = *((spiffs_file*)(&data));
	xs_file_destructor((void *)((int)file));
	xsmcSetHostData(xsThis, NULL);
}

void xs_file_get_length(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	spiffs_file file = *((spiffs_file*)(&data));
	spiffs_stat stat;
	SPIFFS_fstat(gSPIFFS, file, &stat);
	xsResult = xsInteger(stat.size);
}

void xs_file_get_position(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	spiffs_file file = *((spiffs_file*)(&data));
	s32_t position = SPIFFS_lseek(gSPIFFS, file, 0, SPIFFS_SEEK_CUR);
	xsResult = xsInteger(position);
}

void xs_file_set_position(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	spiffs_file file = *((spiffs_file*)(&data));
	s32_t position = xsmcToInteger(xsArg(0));
	SPIFFS_lseek(gSPIFFS, file, position, SPIFFS_SEEK_SET);
}

void xs_file_delete(xsMachine *the)
{
	char path[SPIFFS_OBJ_NAME_LEN];
	s32_t result;

	startSPIFFS();

	xsmcToStringBuffer(xsArg(0), path, SPIFFS_OBJ_NAME_LEN);		// in case name is in ROM
	result = SPIFFS_remove(gSPIFFS, path);

	stopSPIFFS();

	xsResult = xsBoolean(result == SPIFFS_OK);
}

void xs_file_exists(xsMachine *the)
{
	char path[SPIFFS_OBJ_NAME_LEN];
	spiffs_stat stat;
	s32_t result;

	startSPIFFS();

	xsmcToStringBuffer(xsArg(0), path, SPIFFS_OBJ_NAME_LEN);		// in case name is in ROM
	result = SPIFFS_stat(gSPIFFS, path, &stat);

	stopSPIFFS();

	xsResult = xsBoolean(result == SPIFFS_OK);
}

void xs_file_rename(xsMachine *the)
{
	char path[SPIFFS_OBJ_NAME_LEN];
	char name[SPIFFS_OBJ_NAME_LEN];
	s32_t result;

	startSPIFFS();

	xsmcToStringBuffer(xsArg(0), path, SPIFFS_OBJ_NAME_LEN);		// in case name is in ROM
	xsmcToStringBuffer(xsArg(1), name, SPIFFS_OBJ_NAME_LEN);			// in case name is in ROM
	result = SPIFFS_rename(gSPIFFS, path, name);

	stopSPIFFS();

	xsResult = xsBoolean(result == SPIFFS_OK);
}

void xs_file_iterator_destructor(void *data)
{
	spiffs_DIR *d = data;

	if (d) {
		SPIFFS_closedir(d);
		free(d);
		stopSPIFFS();
		modInstrumentationAdjust(Files, -1);
	}
}

void xs_File_Iterator(xsMachine *the)
{
	char *path = xsmcToString(xsArg(0));
	spiffs_DIR *d;

	startSPIFFS();

	d = calloc(1, sizeof(spiffs_DIR));
	if (!d)
		xsUnknownError("no memory for iterator");

	if (NULL == SPIFFS_opendir(gSPIFFS, path, d)) {
		free(d);
		stopSPIFFS();
		xsUnknownError("failed to open directory");
	}
	xsmcSetHostData(xsThis, d);

	modInstrumentationAdjust(Files, +1);
}

void xs_file_iterator_next(xsMachine *the)
{
	spiffs_DIR *d = xsmcGetHostData(xsThis);
	struct spiffs_dirent de;

	if (!d) return;

	do {
		if (NULL == SPIFFS_readdir(d, &de)) {
			xs_file_iterator_destructor(d);
			xsmcSetHostData(xsThis, NULL);
			return;
		}
	} while ((SPIFFS_TYPE_HARD_LINK == de.type) || (SPIFFS_TYPE_SOFT_LINK == de.type));

	xsResult = xsmcNewObject();
	xsmcVars(1);
	xsmcSetString(xsVar(0), de.name);
	xsmcSet(xsResult, xsID_name, xsVar(0));
	if (SPIFFS_TYPE_FILE == de.type) {
		xsmcSetInteger(xsVar(0), de.size);
		xsmcSet(xsResult, xsID_length, xsVar(0));
	}
}

void xs_file_system_config(xsMachine *the)
{
	xsResult = xsmcNewObject();
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), SPIFFS_OBJ_NAME_LEN - 1);
	xsmcSet(xsResult, xsID_maxPathLength, xsVar(0));
}

void xs_file_system_info(xsMachine *the)
{
	s32_t result;
	u32_t total, used;

	startSPIFFS();

	result = SPIFFS_info(gSPIFFS, &total, &used);

	stopSPIFFS();

	if (result != SPIFFS_OK)
		xsUnknownError("system info failed");

	xsResult = xsmcNewObject();
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), total);
	xsmcSet(xsResult, xsID_total, xsVar(0));
	xsmcSetInteger(xsVar(0), used);
	xsmcSet(xsResult, xsID_used, xsVar(0));
}

/*
	ESP SPIFFS fun... borrowed from various parts of ESP Arduino SPIFF code
*/

static uint16_t gUseCount;
static u8_t *_workBuf, *_fdsBuf, *_cacheBuf;		// these can be merged into a single pointer

extern int32_t spiffs_hal_write_c(uint32_t addr, uint32_t size, uint8_t *src);
extern int32_t spiffs_hal_erase_c(uint32_t addr, uint32_t size);
extern int32_t spiffs_hal_read_c(uint32_t addr, uint32_t size, uint8_t *dst);

extern uint32_t _SPIFFS_start;
extern uint32_t _SPIFFS_end;
extern uint32_t _SPIFFS_page;
extern uint32_t _SPIFFS_block;

#define SPIFFS_PHYS_ADDR ((uint32_t) (&_SPIFFS_start) - 0x40200000)
#define SPIFFS_PHYS_SIZE ((uint32_t) (&_SPIFFS_end) - (uint32_t) (&_SPIFFS_start))
#define SPIFFS_PHYS_PAGE ((uint32_t) &_SPIFFS_page)
#define SPIFFS_PHYS_BLOCK ((uint32_t) &_SPIFFS_block)

#define SPIFFS_MAX_OPEN_FDS (5)

#define FLASH_SECTOR_SIZE 0x1000

static void _check_cb(spiffs_check_type type, spiffs_check_report report, uint32_t arg1, uint32_t arg2) {}

int startSPIFFS(void)
{
	if (0 != gUseCount++)
		return 0;

	spiffs_config config = {0};

	config.hal_read_f       = &spiffs_hal_read_c;
	config.hal_write_f      = &spiffs_hal_write_c;
	config.hal_erase_f      = &spiffs_hal_erase_c;
	config.phys_size        = SPIFFS_PHYS_SIZE;
	config.phys_addr        = SPIFFS_PHYS_ADDR;
	config.phys_erase_block = FLASH_SECTOR_SIZE;
	config.log_block_size   = SPIFFS_PHYS_BLOCK;
	config.log_page_size    = SPIFFS_PHYS_PAGE;

	gSPIFFS = calloc(1, sizeof(spiffs));
	gSPIFFS->cfg.log_page_size = config.log_page_size;

	size_t workBufSize = 2 * SPIFFS_PHYS_PAGE;
	size_t fdsBufSize = SPIFFS_buffer_bytes_for_filedescs(gSPIFFS, SPIFFS_MAX_OPEN_FDS);
	size_t cacheBufSize = SPIFFS_buffer_bytes_for_cache(gSPIFFS, SPIFFS_MAX_OPEN_FDS);

	_workBuf = (u8_t *)malloc(workBufSize);
	_fdsBuf = (u8_t *)malloc(fdsBufSize);
	_cacheBuf = (u8_t *)malloc(cacheBufSize);

	int err = SPIFFS_mount(gSPIFFS, &config, _workBuf,
							_fdsBuf, fdsBufSize, _cacheBuf, cacheBufSize,
							_check_cb);
	if (!err) return 0;

	modLog("SPIFFS_mount failed. Formatting.");

	if (SPIFFS_OK != SPIFFS_format(gSPIFFS)) {
		modLog("SPIFFS_format failed.");
		goto fail;
	}

	modLog("SPIFFS_format succeeded.");

	err = SPIFFS_mount(gSPIFFS, &config, _workBuf,
							_fdsBuf, fdsBufSize, _cacheBuf, cacheBufSize,
							_check_cb);
	if (err) goto fail;

	return 0;

fail:
	gUseCount--;

	free((void *)gSPIFFS);
	free(_workBuf);
	free(_fdsBuf);
	free(_cacheBuf);

	gSPIFFS = NULL;
	_workBuf = _fdsBuf = _cacheBuf = NULL;
}

void stopSPIFFS(void)
{
	if (0 != --gUseCount)
		return;

	SPIFFS_unmount(gSPIFFS);

	free((void *)gSPIFFS);
	free(_workBuf);
	free(_fdsBuf);
	free(_cacheBuf);

	gSPIFFS = NULL;
	_workBuf = _fdsBuf = _cacheBuf = NULL;
}

#else // !__ets__

#include "xsmc.h"
#include "xslinux.h"
#include <stdio.h>
#include <malloc.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
	DIR *dir;
	char path[1];
} iteratorRecord, *iter;

void xs_file_destructor(void *data)
{
	if (data && (-1 != (uintptr_t)data)) {
		fclose((FILE*)data);
	}
}

void xs_File(xsMachine *the)
{
	int argc = xsmcArgc;
	FILE *file;
	char path[PATH_MAX];
	uint8_t write = (argc < 2) ? 0 : xsmcToBoolean(xsArg(1));

	xsmcToStringBuffer(xsArg(0), path, PATH_MAX);		// in case name is in ROM
fprintf(stderr, "path: %s - write: %d\n", path, write);
	file = fopen(path, write ? "a+" : "r");
	if (NULL == file) {
fprintf(stderr, "errno: %d\n", errno);
		xsUnknownError("file not found");
	}
	xsmcSetHostData(xsThis, (void *)((int)file));
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

	fno = fileno(file);
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
		unsigned char *src;
		int32_t srcLen;

		if (xsStringType == xsmcTypeOf(xsArg(i))) {
			src = xsmcToString(xsArg(i));
			srcLen = strlen(src);
		}
		else {
			src = xsmcToArrayBuffer(xsArg(i));
			srcLen = xsGetArrayBufferLength(xsArg(i));
		}

		while (srcLen) {	// spool through RAM for data in flash
			unsigned char *buffer[128];
			int use = (srcLen <= sizeof(buffer)) ? srcLen : 128;

			memcpy(buffer, src, use);
			src += use;
			srcLen -= use;

			result = fwrite(buffer, 1, use, file);
			if (result != use)
				xsUnknownError("file write failed");
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

	fno = fileno(file);
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
	result = unlink(path);

	xsResult = xsBoolean(result == 0);
}

void xs_file_exists(xsMachine *the)
{
	char path[PATH_MAX];
	struct stat buf;
	int32_t result;

	xsmcToStringBuffer(xsArg(0), path, PATH_MAX);		// in case name is in ROM
	result = stat(path, &buf);

	xsResult = xsBoolean(result == 0);
}

void xs_file_rename(xsMachine *the)
{
	char path[PATH_MAX];
	char name[PATH_MAX];
	int32_t result;

	xsmcToStringBuffer(xsArg(0), path, PATH_MAX);		// in case name is in ROM
	xsmcToStringBuffer(xsArg(1), name, PATH_MAX);			// in case name is in ROM
	result = rename(path, name);

	xsResult = xsBoolean(result == 0);
}

void xs_file_iterator_destructor(void *data)
{
	iter d = data;

	if (d) {
		if (d->dir)
			closedir(d->dir);
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
	d = calloc(1, sizeof(iteratorRecord) + i + 2);
	strcpy(d->path, p);
	if (p[i-1] != '/')
		d->path[i] = '/';

	if (NULL == (d->dir = opendir(d->path))) {
		xsUnknownError("failed to open directory");
	}
	xsmcSetHostData(xsThis, d);
}

void xs_file_iterator_next(xsMachine *the)
{
	iter d = xsmcGetHostData(xsThis);
	struct dirent *de;
	struct stat buf;
	char path[PATH_MAX];

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

	sprintf(path, "%s%s", d->path, de->d_name);
	if (DT_REG == de->d_type) {
		if (-1 == stat(path, &buf))
			fprintf(stderr, "stat %s returns errno: %d\n", path, errno);
		xsmcSetInteger(xsVar(0), buf.st_size);
		xsmcSet(xsResult, xsID_length, xsVar(0));
fprintf(stderr, "d iterator name: %s length: %d\n", path, buf.st_size);
	}
}

//@@ new symbols!!
void xs_file_system_config(xsMachine *the)
{
	xsResult = xsmcNewObject();
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), PATH_MAX);
	xsmcSet(xsResult, xsID_maxPathLength, xsVar(0));
}

void xs_file_system_info(xsMachine *the)
{
	int32_t result;
	struct statfs buf;

	result = statfs("/", &buf);

	if (result != 0)
		xsUnknownError("system info failed");

	xsResult = xsmcNewObject();
	xsmcVars(1);
	xsmcSetInteger(xsVar(0), buf.f_bsize * buf.f_blocks);
	xsmcSet(xsResult, xsID_total, xsVar(0));
	xsmcSetInteger(xsVar(0), buf.f_bsize * buf.f_bavail);
	xsmcSet(xsResult, xsID_used, xsVar(0));
}


#endif

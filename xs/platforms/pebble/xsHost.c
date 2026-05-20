/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
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
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

#include <_ansi.h>
#include <setjmp.h>
#include "xsAll.h"
#include "xs.h"
#include "xsScript.h"
#include "xsPlatform.h"
#include "xsHosts.h"
#include "mc.defines.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "light_mutex.h"
#include "queue.h"

#include "applib/app_heap_util.h"
#include "applib/app_logging.h"
#include "services/evented_timer.h"
#include "system/passert.h"

#ifndef MODDEF_XS_MODS
	#define MODDEF_XS_MODS	0
#endif

#ifdef mxInstrument
	#include "modTimer.h"
	#include "modInstrumentation.h"
	#include "moddableAppState.h"
	#include "applib/moddable/moddable.h"

	static void espInitInstrumentation(txMachine *the);
	static void espSampleInstrumentation(modTimer timer, void *refcon, int refconSize);

	#define espInstrumentCount kModInstrumentationSlotHeapSize - kModInstrumentationPixelsDrawn
	static char* const espInstrumentNames[espInstrumentCount] ICACHE_XS6RO_ATTR = {
		(char *)"Pixels drawn",
		(char *)"Frames drawn",
		(char *)"Timers",
		(char *)"Files",
		(char *)"Poco display list used",
		(char *)"Piu command List used",
#if kModInstrumentationHasTurns
		(char *)"Event loop",
#endif
		(char *)"App bytes free",
#if kModInstrumentationHasCPU
		(char *)"CPU",
#endif
	};

	static char* const espInstrumentUnits[espInstrumentCount] ICACHE_XS6RO_ATTR = {
		(char *)" pixels",
		(char *)" frames",
		(char *)" timers",
		(char *)" files",
		(char *)" bytes",
		(char *)" bytes",
#if kModInstrumentationHasTurns
		(char *)" turns",
#endif
		(char *)" bytes",
#if kModInstrumentationHasCPU
		(char *)" percent",
#endif
	};

	// LightMandle_t gInstrumentMutex;
#endif

int pbl_gettimeofday(void *tvp, void *unusedTZ)
{
	if (tvp) {
		struct pbl_timeval *tv = tvp;
		uint16_t ms = 0;
		time_t t = 0;
	
		time_ms(&t, &ms);
	
		tv->tv_sec = t;
		tv->tv_usec = 1000 * (int)ms;
	}

	return 0;
}

static char gTransmitBuffer[90];		// not thread safe.... also... there is a maximum size enforced by Pebble so don't grow this without checking that...
static char *gTransmit = gTransmitBuffer;

void modLog_transmit(const char *msg)
{
	if (!msg) return;

	do {
		while (*msg && ((gTransmit - gTransmitBuffer) < (int)(sizeof(gTransmitBuffer) - 1)))
			*gTransmit++ = *msg++;
		int end = 10 == gTransmit[-1];
		if (end)
			gTransmit[-1] = 0;
		if (end || ((gTransmit - gTransmitBuffer) >= (int)(sizeof(gTransmitBuffer) - 1))) {
			*gTransmit++ = 0;

//			PBL_LOG_ERR("%s", gTransmitBuffer);
			APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, "%s", gTransmitBuffer);
			gTransmit = gTransmitBuffer;
		}
	} while (*msg);
}

/*
	Instrumentation
*/

#ifdef mxInstrument

void modInstrumentationSetup(xsMachine *the)
{
	if (!(getModdableAppState(creationFlags) & kModdableCreationFlagLogInstrumentation) 
#ifdef mxDebug
			&& !the->connected
#endif
		)
		return;

	espInitInstrumentation(the);

	modInstrumentMachineBegin(the, espSampleInstrumentation, espInstrumentCount, (char**)espInstrumentNames, (char**)espInstrumentUnits);
}

static int32_t modInstrumentationAppFreeMemory(void *theIn)
{
	return heap_bytes_free();
}

void espInitInstrumentation(txMachine *the)
{
#if MODDEF_XS_TEST
	static uint8_t initialized = 0;
	if (initialized)
		return;
	initialized = 1;
#endif

	modInstrumentationInit();
	modInstrumentationSetCallback(SystemFreeMemory, modInstrumentationAppFreeMemory);

	modInstrumentationSetCallback(SlotHeapSize, (ModInstrumentationGetter)modInstrumentationSlotHeapSize);
	modInstrumentationSetCallback(ChunkHeapSize, (ModInstrumentationGetter)modInstrumentationChunkHeapSize);
	modInstrumentationSetCallback(KeysUsed, (ModInstrumentationGetter)modInstrumentationKeysUsed);
	modInstrumentationSetCallback(GarbageCollectionCount, (ModInstrumentationGetter)modInstrumentationGarbageCollectionCount);
	modInstrumentationSetCallback(ModulesLoaded, (ModInstrumentationGetter)modInstrumentationModulesLoaded);
	modInstrumentationSetCallback(StackRemain, (ModInstrumentationGetter)modInstrumentationStackRemain);
	modInstrumentationSetCallback(PromisesSettledCount, (ModInstrumentationGetter)modInstrumentationPromisesSettledCount);
}

void espSampleInstrumentation(modTimer timer, void *refcon, int refconSize)
{
	txInteger values[espInstrumentCount];
	int what;
	xsMachine *the = *(xsMachine **)refcon;

	// xSemaphoreTake(gInstrumentMutex, portMAX_DELAY);

	for (what = kModInstrumentationPixelsDrawn; what <= (kModInstrumentationSlotHeapSize - 1); what++)
		values[what - kModInstrumentationPixelsDrawn] = modInstrumentationGet_(the, what);

#if kModInstrumentationHasTurns
	if (values[kModInstrumentationTurns - kModInstrumentationPixelsDrawn])
		values[kModInstrumentationTurns - kModInstrumentationPixelsDrawn] -= 1;		// ignore the turn that generates instrumentation
#endif

	fxSampleInstrumentation(the, espInstrumentCount, values);

	modInstrumentationSet(PixelsDrawn, 0);
	modInstrumentationSet(FramesDrawn, 0);
	modInstrumentationSet(PocoDisplayListUsed, 0);
	modInstrumentationSet(PiuCommandListUsed, 0);
#if kModInstrumentationHasTurns
	modInstrumentationSet(Turns, 0);
#endif
	modInstrumentMachineReset(the);

	// xSemaphoreGive(gInstrumentMutex);
}

#endif

static LightMutexHandle_t gFlashMutex = NULL;

void modMachineTaskInit(xsMachine *the)
{
	if (NULL == gFlashMutex)
		gFlashMutex = xLightMutexCreate();
}

void modMachineTaskUninit(xsMachine *the)
{
}

void modMachineTaskWait(xsMachine *the)
{
}

void modMachineTaskWake(xsMachine *the)
{
}

/*
	promises
*/

static void doRunPromiseJobs(void *machine)
{
	fxRunPromiseJobs((txMachine *)machine);
}

void fxQueuePromiseJobs(txMachine* the)
{
  evented_timer_register(0, false, doRunPromiseJobs, the);
}

/*
	 user installable modules
*/

#if MODDEF_XS_MODS
static txBoolean spiRead(void *src, size_t offset, void *buffer, size_t size)
{
	return modSPIRead(offset + (uintptr_t)src - (uintptr_t)kFlashStart, size, buffer);
}

static txBoolean spiWrite(void *dst, size_t offset, void *buffer, size_t size)
{
	offset += (uintptr_t)dst;

	if ((offset + kFlashSectorSize) > (uintptr_t)kModulesEnd)
		return 0;		// attempted write beyond end of available space

	if (!(offset & (kFlashSectorSize - 1))) {		// if offset is at start of a sector, erase that sector
		if (!modSPIErase(offset - (uintptr_t)kFlashStart, kFlashSectorSize))
			return 0;
	}

	return modSPIWrite(offset - (uintptr_t)kFlashStart, size, buffer);
}

void *modInstallMods(xsMachine *the, void *preparationIn, uint8_t *status)
{
	txPreparation *preparation = preparationIn;
	void *result = NULL;

	if (NULL == gFlashMutex)
		gFlashMutex = xSemaphoreCreateMutex();

	if (fxMapArchive(the, preparation, (void *)kModulesStart, kFlashSectorSize, spiRead, spiWrite)) {
		result = (void *)kModulesStart;
		fxSetArchive(the, result);
	}

	if (XS_ATOM_ERROR == c_read32be((void *)(4 + kModulesStart))) {
		*status = *(8 + (uint8_t *)kModulesStart);
		modLog("mod failed");
	}
	else
		*status = 0;

	return result;
}

#endif /* MODDEF_XS_MODS */

/*
	flash
 */
#if 0	// MDK
#include "nrf_fstorage_sd.h"

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
//    .evt_handler = NULL,		// don't need this?
    .start_addr = 1,
    .end_addr   = 0x100000,
};

uint8_t modSPIFlashInit(void)
{
	if (!fstorage.start_addr)
		return 1;

	fstorage.start_addr = 0;
    if (NRF_SUCCESS != nrf_fstorage_init(&fstorage, &nrf_fstorage_sd, NULL))
		return 0;

	fstorage.start_addr = 1;

	return 1;
}

static void wait_for_flash_ready(void)
{
    while (nrf_fstorage_is_busy(&fstorage))
		sd_app_evt_wait();
}

uint8_t modSPIRead(uint32_t offset, uint32_t size, uint8_t *dst)
{
	uint8_t temp[4] __attribute__ ((aligned (4)));
	uint32_t toAlign;

	xSemaphoreTake(gFlashMutex, portMAX_DELAY);

	if (!modSPIFlashInit()) {
		xSemaphoreGive(gFlashMutex);
		return 0;
	}

	if (offset & 3) {		// long align offset
		if (NRF_SUCCESS != nrf_fstorage_read(&fstorage, offset & ~3, temp, 4)) {
			xSemaphoreGive(gFlashMutex);
			return 0;
		}
		wait_for_flash_ready();

		toAlign = 4 - (offset & 3);
		c_memcpy(dst, temp + 4 - toAlign, (size < toAlign) ? size : toAlign);

		if (size <= toAlign)
			goto done;

		dst += toAlign;
		offset += toAlign;
		size -= toAlign;
	}

	toAlign = size & ~3;
	if (toAlign) {
//@@ need case here for misaligned destination
		size -= toAlign;
		if (NRF_SUCCESS != nrf_fstorage_read(&fstorage, offset, dst, toAlign)) {
			xSemaphoreGive(gFlashMutex);
			return 0;
		}
		wait_for_flash_ready();

		dst += toAlign;
		offset += toAlign;
	}

	if (size) {				// long align tail
		if (NRF_SUCCESS != nrf_fstorage_read(&fstorage, offset, temp, 4)) {
			xSemaphoreGive(gFlashMutex);
			return 0;
		}
		wait_for_flash_ready();

		c_memcpy(dst, temp, size);
	}

done:
	xSemaphoreGive(gFlashMutex);
	return 1;
}

uint8_t modSPIWrite(uint32_t offset, uint32_t size, const uint8_t *src)
{
	uint8_t temp[512] __attribute__ ((aligned (4)));
	uint32_t toAlign;

	xSemaphoreTake(gFlashMutex, portMAX_DELAY);

	if (!modSPIFlashInit()) {
		xSemaphoreGive(gFlashMutex);
		return 0;
	}

	if (offset & 3) {		// long align offset
		toAlign = 4 - (offset & 3);
		c_memset(temp, 0xFF, 4);
		c_memcpy(temp + 4 - toAlign, src, (size < toAlign) ? size : toAlign);
		if (NRF_SUCCESS != nrf_fstorage_write(&fstorage, offset & ~3, temp, 4, NULL)) {
			xSemaphoreGive(gFlashMutex);
			return 0;
		}
		wait_for_flash_ready();

		if (size <= toAlign) {
			xSemaphoreGive(gFlashMutex);
			return 1;
		}

		src += toAlign;
		offset += toAlign;
		size -= toAlign;
	}

	toAlign = size & ~3;
	if (toAlign) {
		size -= toAlign;
		if (3 & (uintptr_t)src) {	// src is not long aligned, copy through stack
			while (toAlign) {
				uint32_t use = (toAlign > sizeof(temp)) ? sizeof(temp) : toAlign;
				c_memcpy(temp, src, use);
				if (NRF_SUCCESS != nrf_fstorage_write(&fstorage, offset, temp, use, NULL)) {
					xSemaphoreGive(gFlashMutex);
					return 0;
				}
				wait_for_flash_ready();

				toAlign -= use;
				src += use;
				offset += use;
			}
		}
		else {
			if (NRF_SUCCESS != nrf_fstorage_write(&fstorage, offset, src, toAlign, NULL)) {
				xSemaphoreGive(gFlashMutex);
				return 0;
			}
			wait_for_flash_ready();

			src += toAlign;
			offset += toAlign;
		}
	}

	if (size) {			// long align tail
		c_memset(temp, 0xFF, 4);
		c_memcpy(temp, src, size);
		if (NRF_SUCCESS != nrf_fstorage_write(&fstorage, offset, temp, 4, NULL)) {
			xSemaphoreGive(gFlashMutex);
			return 0;
		}
		wait_for_flash_ready();
	}

	xSemaphoreGive(gFlashMutex);
	return 1;
}

uint8_t modSPIErase(uint32_t offset, uint32_t size)
{
	xSemaphoreTake(gFlashMutex, portMAX_DELAY);

	if (!modSPIFlashInit()) {
		xSemaphoreGive(gFlashMutex);
		return 0;
	}

	if ((offset & (fstorage.p_flash_info->erase_unit - 1)) || (size & (fstorage.p_flash_info->erase_unit - 1))) {
		xSemaphoreGive(gFlashMutex);
		return 0;
	}

	size /= fstorage.p_flash_info->erase_unit;

	if (NRF_SUCCESS != nrf_fstorage_erase(&fstorage, offset, size, NULL)) {
		xSemaphoreGive(gFlashMutex);
		return 0;
	}
	wait_for_flash_ready();

	xSemaphoreGive(gFlashMutex);
	return 1;
}

uint8_t *espFindUnusedFlashStart(void)
{
	uintptr_t modStart;
	extern uint32_t __start_unused_space;

	if (!modSPIFlashInit())
		return NULL;

	modStart = (uintptr_t)&__start_unused_space;
	modStart += fstorage.p_flash_info->erase_unit - 1;
	modStart -= modStart % fstorage.p_flash_info->erase_unit;

	/*
		this assumes:
		- the .data. section follows the application image
		- it is no bigger than 4096 bytes
		- empty space follows the .data. section
	*/
	modStart += 4096;

	return (uint8_t *)modStart;
}
#endif	// MDK

void nrf52_reboot(uint32_t kind)
{
	pebble_reset();
}

//---------- alignment for memory

uint16_t espRead16be(const void *addr)
{
	uint16_t result = 0;
	const uint32_t *p = (const uint32_t *)(~3 & (uint32_t)addr);
	switch (3 & (uint32_t)addr) {
		case 3:	result = (uint16_t)((*p >> 24) | (p[1] << 8)); break;
		case 2:	result = (uint16_t) (*p >> 16); break;
		case 1:	result = (uint16_t) (*p >>  8); break;
		case 0:	result = (uint16_t) (*p); break;
}

	return (result >> 8) | (result << 8);
}

uint32_t espRead32be(const void *addr)
{
	uint32_t result = 0;
	const uint32_t *p = (const uint32_t *)(~3 & (uint32_t)addr);
	switch (3 & (uint32_t)addr) {
		case 0:	result = *p; break;
		case 1:	result = (p[0] >>  8) | (p[1] << 24); break;
		case 2:	result = (p[0] >> 16) | (p[1] << 16); break;
		case 3:	result = (p[0] >> 24) | (p[1] <<  8); break;
	}
	return (result << 24) | ((result & 0xff00) << 8)  | ((result >> 8) & 0xff00) | (result >> 24);
}

double nearbyint(double x)
{
	return c_floor(x);		//@@ really not correct.
}

int errno;
int *__errno(void) { return &errno; };

#include "piuAll.h"
#include "applib/fonts/fonts.h"

struct PebbleFontRecord {
	const char		*family;
	uint16_t			size;
	const char		*res_key;
};
typedef struct PebbleFontRecord PebbleFontRecord;
typedef struct PebbleFontRecord *PebbleFont;

static const PebbleFontRecord gFonts[] = {
    {.family = "Gothic-Bold", .size = 18, .res_key = FONT_KEY_GOTHIC_18_BOLD},
    {.family = "Gothic-Regular", .size = 9,  .res_key = FONT_KEY_GOTHIC_09},
    {.family = "Gothic-Regular", .size = 14, .res_key = FONT_KEY_GOTHIC_14},
    {.family = "Gothic-Bold", .size = 14, .res_key = FONT_KEY_GOTHIC_14_BOLD},
    {.family = "Gothic-Regular", .size = 18, .res_key = FONT_KEY_GOTHIC_18},
    {.family = "Gothic-Regular", .size = 24, .res_key = FONT_KEY_GOTHIC_24},
    {.family = "Gothic-Bold", .size = 24, .res_key = FONT_KEY_GOTHIC_24_BOLD},
    {.family = "Gothic-Regular", .size = 28, .res_key = FONT_KEY_GOTHIC_28},
    {.family = "Gothic-Bold", .size = 28, .res_key = FONT_KEY_GOTHIC_28_BOLD},
    {.family = "Gothic-Regular", .size = 36, .res_key = FONT_KEY_GOTHIC_36},
    {.family = "Gothic-Bold", .size = 36, .res_key = FONT_KEY_GOTHIC_36_BOLD},

    {.family = "Bitham-Black", .size = 30, .res_key = FONT_KEY_BITHAM_30_BLACK},
    {.family = "Bitham-Bold", .size = 42, .res_key = FONT_KEY_BITHAM_42_BOLD},
    {.family = "Bitham-Light", .size = 42, .res_key = FONT_KEY_BITHAM_42_LIGHT},
    {.family = "Bitham-Medium", .size = 42, .res_key = FONT_KEY_BITHAM_42_MEDIUM_NUMBERS},
    {.family = "Bitham-Medium", .size = 34, .res_key = FONT_KEY_BITHAM_34_MEDIUM_NUMBERS},
    {.family = "Bitham-Light", .size = 34, .res_key = FONT_KEY_BITHAM_34_LIGHT_SUBSET},
    {.family = "Bitham-Light", .size = 18, .res_key = FONT_KEY_BITHAM_18_LIGHT_SUBSET},

    {.family = "Roboto-Condensed", .size = 21, .res_key = FONT_KEY_ROBOTO_CONDENSED_21},
    {.family = "Roboto-Bold", .size = 49, .res_key = FONT_KEY_ROBOTO_BOLD_SUBSET_49},

    {.family = "DroidSerif-Bold", .size = 28, .res_key = FONT_KEY_DROID_SERIF_28_BOLD},

    {.family = "Leco-Bold", .size = 20, .res_key = FONT_KEY_LECO_20_BOLD_NUMBERS},
    {.family = "Leco-Bold", .size = 26, .res_key = FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM},
    {.family = "Leco-Bold", .size = 32, .res_key = FONT_KEY_LECO_32_BOLD_NUMBERS},
    {.family = "Leco-Bold", .size = 36, .res_key = FONT_KEY_LECO_36_BOLD_NUMBERS},
    {.family = "Leco-Bold", .size = 38, .res_key = FONT_KEY_LECO_38_BOLD_NUMBERS},
    {.family = "Leco-Regular", .size = 42, .res_key = FONT_KEY_LECO_42_NUMBERS},
    {.family = "Leco-Light", .size = 28, .res_key = FONT_KEY_LECO_28_LIGHT_NUMBERS},

	 {0}
};

GFont modFindPebbleFont(const char *family, int size, int32_t *ascent, int32_t *descent, int32_t *leading)
{
	PebbleFont f = (PebbleFont)gFonts;

	for (; C_NULL != f->family; f++) {
		if ((size == f->size) && !c_strcmp(family, f->family))
			break;
	}

	GFont font = C_NULL;
	if (f->family) {
		font = fonts_get_system_font(f->res_key);
	}
	if (!font) {
// 		font = fonts_get_system_font(FONT_KEY_FONT_FALLBACK);
// 		if (!font)
			return C_NULL;
	}

	uint16_t height = fonts_get_font_height(font);
	*descent = fonts_get_font_cap_offset(font);
	*leading = 1;
	*ascent = height - *descent - *leading;
	return font;
}



// all hail gary davidian: http://opensource.apple.com//source/xnu/xnu-1456.1.26/libkern/ppc/strlen.s
size_t espStrLen(const void *addr)
{
	static const uint32_t mask[] ICACHE_XS6RO2_ATTR = {0, 0x000000FF, 0x0000FFFF, 0x00FFFFFF};
	int len = 3 & (uint32_t)addr;
	const uint32_t *src = (const uint32_t *)(-len + (uint32_t)addr);
	uint32_t data = *src++ | mask[len];

	len = -len;

	while (true) {
		uint32_t y = data + 0xFEFEFEFF;
		uint32_t z = ~data & 0x80808080;

		if (0 != (y & z))
			break;

		len += 4;
		data = *src++;
	}

	// three more bytes, at most, since there is a 0 somewhere in this long
	if (data & 0x00ff) {
		len += 1;
		if (data & 0x00ff00) {
			len += 1;
			if (data & 0x00ff0000)
				len += 1;
		}
	}

	return (size_t)len;
}

//@@ this could be much faster, especially when both strings are aligned
int espStrCmp(const char *ap, const char *bp)
{
	while (true) {
		uint8_t a = c_read8(ap);
		uint8_t b = c_read8(bp);

		if ((a != b) || !a)
			return a - b;

		ap += 1;
		bp += 1;
	}
}

int espStrNCmp(const char *ap, const char *bp, size_t count)
{
	while (count--) {
		uint8_t a = c_read8(ap);
		uint8_t b = c_read8(bp);

		if ((a != b) || !a)
			return a - b;

		ap += 1;
		bp += 1;
	}

	return 0;
}

void espStrCpy(char *dst, const char *src)
{
	uint8_t c;

	do {
		c = c_read8(src++);
		*dst++ = c;
	} while (c);
}

void espStrNCpy(char *dst, const char *src, size_t count)
{
	char c;

	if (0 == count) return;

	do {
		c = c_read8(src++);
		*dst++ = c;
	} while (--count && c);

	while (count--)
		*dst++ = 0;
}

void espStrCat(char *dst, const char *src)
{
	while (0 != c_read8(dst))
		dst++;

	espStrCpy(dst, src);
}

void espStrNCat(char *dst, const char *src, size_t count)
{
	while (0 != c_read8(dst))
		dst++;

	while (count--) {
		char c = c_read8(src++);
		if (0 == c)
			break;

		*dst++ = c;
	}

	*dst++ = 0;
}

char *espStrChr(const char *str, int c)
{
	do {
		char value = c_read8(str);
		if (!value)
			return NULL;

		if (value == (char)c)
			return (char *)str;

		str++;
	} while (true);
}

char *espStrRChr(const char *str, int c)
{
	const char *result = NULL;

	do {
		str = espStrChr(str, c);
		if (!str)
			break;

		result = str;
		str += 1;
	} while (true);

	return (char *)result;
}

char *espStrStr(const char *src, const char *search)
{
	char searchFirst = c_read8(search++);
	char c;

	if (0 == searchFirst)
		return (char *)src;

	while ((c = c_read8(src++))) {
		const char *ap, *bp;
		uint8_t a, b;

		if (c != searchFirst)
			continue;

		ap = src, bp = search;
		while (true) {
			b = c_read8(bp++);
			if (!b)
				return (char *)src - 1;

			a = c_read8(ap++);
			if ((a != b) || !a)
				break;
		}
	}

	return NULL;
}

// modeled on newlib
size_t espStrcspn(const char *str, const char *strCharSet)
{
	const char *s = str;

	while (c_read8(str)) {
		const char *cs = strCharSet;
		char c, sc = c_read8(str);

		while (0 != (c = c_read8(cs++))) {
			if (sc == c)
				return str - s;
		}

		str++;
	}

	return str - s;
}

size_t espStrspn(const char *str, const char *strCharSet)
{
	const char *s = str;

	while (c_read8(str)) {
		const char *cs = strCharSet;
		char c, sc = c_read8(str);

		while (0 != (c = c_read8(cs++))) {
			if (sc == c)
				break;
		}

		if (0 == c)
			return str - s;

		str++;
	}

	return str - s;
}

void espMemCpy(void *dst, const void *src, size_t count)
{
	const uint8_t *s = src;
	uint8_t *d = dst;

	if (count > 8) {
		// align source
		uint8_t align = 3 & (uintptr_t)s;
		uint32_t data;

		if (align) {
			data = *(uint32_t *)(~3 & (uintptr_t)s);
			if (3 == align) {
				d[0] = (uint8_t)(data >> 24);
				count -= 1;
				s += 1, d += 1;
			}
			else if (2 == align) {
				d[0] = (uint8_t)(data >> 16);
				d[1] = (uint8_t)(data >> 24);
				count -= 2;
				s += 2, d += 2;
			}
			else if (1 == align) {
				d[0] = (uint8_t)(data >>  8);
				d[1] = (uint8_t)(data >> 16);
				d[2] = (uint8_t)(data >> 24);
				count -= 3;
				s += 3, d += 3;
			}
		}

		// read longs and write longs
		align = 3 & (uintptr_t)d;
		if (0 == align) {
			while (count >= 16) {
				*(uint32_t *)&d[0] = *(uint32_t *)&s[0];
				*(uint32_t *)&d[4] = *(uint32_t *)&s[4];
				*(uint32_t *)&d[8] = *(uint32_t *)&s[8];
				*(uint32_t *)&d[12] = *(uint32_t *)&s[12];
				count -= 16;
				s += 16, d += 16;
			}

			while (count >= 4) {
				*(uint32_t *)d = *(uint32_t *)s;
				count -= 4;
				s += 4, d += 4;
			}
		}
		else if (3 == align) {
			data = *(uint32_t *)s;
			*d++ = (uint8_t)(data);
			s += 1;
			count -= 1;
			while (count >= 4) {
				uint32_t next = *(uint32_t *)(3 + (uintptr_t)s);
				*(uint32_t *)d = (data >> 8) | (next << 24);
				count -= 4;
				s += 4, d += 4;
				data = next;
			}
		}
		else if (2 == align) {
			data = *(uint32_t *)s;
			*d++ = (uint8_t)(data);
			*d++ = (uint8_t)(data >>  8);
			s += 2;
			count -= 2;
			while (count >= 4) {
				uint32_t next = *(uint32_t *)(2 + (uintptr_t)s);
				*(uint32_t *)d = (data >> 16) | (next << 16);
				count -= 4;
				s += 4, d += 4;
				data = next;
			}
		}
		else if (1 == align) {
			data = *(uint32_t *)s;
			*d++ = (uint8_t)(data);
			*d++ = (uint8_t)(data >>  8);
			*d++ = (uint8_t)(data >> 16);
			s += 3;
			count -= 3;
			while (count >= 4) {
				uint32_t next = *(uint32_t *)(1 + (uintptr_t)s);
				*(uint32_t *)d = (data >> 24) | (next << 8);
				count -= 4;
				s += 4, d += 4;
				data = next;
			}
		}
	}

	// tail
	while (count--)
		*d++ = c_read8(s++);
}

int espMemCmp(const void *a, const void *b, size_t count)
{
	const uint8_t *a8 = a;
	const uint8_t *b8 = b;

	while (count--) {
		uint8_t av = c_read8(a8++);
		uint8_t bv = c_read8(b8++);
		if (av == bv)
			continue;
		return av - bv;
	}

	return 0;
}

void *espMemMove(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;

    if (d == s || n == 0) {
        return dest;
    }

    if (d < s) {
        // Safe to copy forward
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        // Regions overlap; copy backward
        for (size_t i = n; i != 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }

    return dest;
}

void *espMemSet(void *dst, int c, size_t n) {
    unsigned char *d = dst;
	while (n--)
		*d++ = c;
    return dst;
}

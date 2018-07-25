/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

#include "xsAll.h"
#include "xs.h"
#include "mc.defines.h"

#include "xsScript.h"

#if ESP32
	#define rtc_timeval timeval
	#define rtctime_gettimeofday(a) gettimeofday(a, NULL)

	portMUX_TYPE gCriticalMux = portMUX_INITIALIZER_UNLOCKED;
#else
	#include "Arduino.h"
	#include "rtctime.h"
	#include "spi_flash.h"
#endif

#ifdef mxInstrument
	#include "modTimer.h"
	#include "modInstrumentation.h"
	static void espStartInstrumentation(txMachine* the);
#endif

ICACHE_RAM_ATTR uint8_t espRead8(const void *addr)
{
	const uint32_t *p = (const uint32_t *)(~3 & (uint32_t)addr);
	return *p >> ((3 & (uint32_t)addr) << 3);
}

ICACHE_RAM_ATTR uint16_t espRead16(const void *addr)
{
	const uint32_t *p = (const uint32_t *)(~3 & (uint32_t)addr);
	switch (3 & (uint32_t)addr) {
		case 3:	return (uint16_t)((*p >> 24) | (p[1] << 8));
		case 2:	return (uint16_t) (*p >> 16);
		case 1:	return (uint16_t) (*p >>  8);
		case 0:	return (uint16_t) (*p);
	}
}

ICACHE_RAM_ATTR uint32_t espRead32(const void *addr)
{
	const uint32_t *p = (const uint32_t *)(~3 & (uint32_t)addr);
	switch (3 & (uint32_t)addr) {
		case 0:	return *p;
		case 1:	return (p[0] >>  8) | (p[1] << 24);
		case 2:	return (p[0] >> 16) | (p[1] << 16);
		case 3:	return (p[0] >> 24) | (p[1] <<  8);
	}
}

ICACHE_RAM_ATTR uint16_t espRead16be(const void *addr)
{
	uint16_t result;
	const uint32_t *p = (const uint32_t *)(~3 & (uint32_t)addr);
	switch (3 & (uint32_t)addr) {
		case 3:	result = (uint16_t)((*p >> 24) | (p[1] << 8)); break;
		case 2:	result = (uint16_t) (*p >> 16); break;
		case 1:	result = (uint16_t) (*p >>  8); break;
		case 0:	result = (uint16_t) (*p); break;
	}

	return (result >> 8) | (result << 8);
}

ICACHE_RAM_ATTR uint32_t espRead32be(const void *addr)
{
	uint32_t result;
	const uint32_t *p = (const uint32_t *)(~3 & (uint32_t)addr);
	switch (3 & (uint32_t)addr) {
		case 0:	result = *p; break;
		case 1:	result = (p[0] >>  8) | (p[1] << 24); break;
		case 2:	result = (p[0] >> 16) | (p[1] << 16); break;
		case 3:	result = (p[0] >> 24) | (p[1] <<  8); break;
	}
	return (result << 24) | ((result & 0xff00) << 8)  | ((result >> 8) & 0xff00) | (result >> 24);
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
		uint8_t a = espRead8(ap);
		uint8_t b = espRead8(bp);

		if ((a != b) || !a)
			return a - b;

		ap += 1;
		bp += 1;
	}
}

int espStrNCmp(const char *ap, const char *bp, size_t count)
{
	while (count--) {
		uint8_t a = espRead8(ap);
		uint8_t b = espRead8(bp);

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
		c = espRead8(src++);
		*dst++ = c;
	} while (c);
}

void espStrNCpy(char *dst, const char *src, size_t count)
{
	char c;

	if (0 == count) return;

	do {
		c = espRead8(src++);
		*dst++ = c;
	} while (--count && c);

	while (count--)
		*dst++ = 0;
}

void espStrCat(char *dst, const char *src)
{
	while (0 != espRead8(dst))
		dst++;

	espStrCpy(dst, src);
}

void espStrNCat(char *dst, const char *src, size_t count)
{
	while (0 != espRead8(dst))
		dst++;

	while (count--) {
		char c = espRead8(src++);
		if (0 == c)
			break;

		*dst++ = c;
	}

	*dst++ = 0;
}

char *espStrChr(const char *str, int c)
{
	do {
		char value = espRead8(str);
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
	char searchFirst = espRead8(search++);
	char c;

	if (0 == searchFirst)
		return (char *)src;

	while ((c = espRead8(src++))) {
		const char *ap, *bp;
		uint8_t a, b;

		if (c != searchFirst)
			continue;

		ap = src, bp = search;
		while (true) {
			b = espRead8(bp++);
			if (!b)
				return (char *)src - 1;

			a = espRead8(ap++);
			if ((a != b) || !a)
				break;
		}
	}

	return NULL;
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
		*d++ = espRead8(s++);
}

int espMemCmp(const void *a, const void *b, size_t count)
{
	const uint8_t *a8 = a;
	const uint8_t *b8 = b;

	while (count--) {
		uint8_t av = espRead8(a8++);
		uint8_t bv = espRead8(b8++);
		if (av == bv)
			continue;
		return av - bv;
	}

	return 0;
}

#if !ESP32
	extern const uint32_t *gUnusedInstructionRAM;
	static uint32_t *gUint32Memory;
#endif


void *espMallocUint32(int byteCount)
{
#if ESP32
	return heap_caps_malloc(byteCount, MALLOC_CAP_32BIT);
#else
	const char *start = (char *)gUnusedInstructionRAM[-2];
	const char *end = start + gUnusedInstructionRAM[-1];
	uint32_t *result;

	if (byteCount & 3)
		return NULL;		// must be multiple of uint32_t in size

	if (NULL == gUint32Memory)
		gUint32Memory = (uint32_t *)start;

	result = gUint32Memory;
	if ((byteCount + (char *)result) > end)
		return c_malloc(byteCount);

	gUint32Memory += byteCount;

	return result;
#endif
}

void espFreeUint32(void *t)
{
#if ESP32
	if (t)
		heap_caps_free(t);
#else
	const char *start = (char *)gUnusedInstructionRAM[-2];
	const char *end = start + gUnusedInstructionRAM[-1];

	if (!t) return;

	if ((t < (void *)start) || (t >= (void *)end)) {
		c_free(t);
		return;
	}

	gUint32Memory = t;		//@@ assumes alloc/free are paired
#endif
}

#if !ESP32
#include "cont.h"

int espStackSpace(void)
{
	extern cont_t g_cont __attribute__ ((aligned (16)));
	int free_ = 0;

	return (char *)&free_ - (char *)&g_cont.stack[0];
}
#endif

static int32_t gTimeZoneOffset = -8 * 60 * 60;		// Menlo Park
static int16_t gDaylightSavings = 60 * 60;			// summer time

static modTm gTM;		//@@ eliminate with _r calls

static const uint8_t gDaysInMonth[] ICACHE_XS6RO2_ATTR = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define isLeapYear(YEAR) (!(YEAR % 4) && ((YEAR % 100) || !(YEAR % 400)))

// algorithm based on arduino version. separate implementation. http://www.pucebaboon.com/ESP8266/

struct modTm *modGmTime(const modTime_t *timep)
{
	uint32_t t = *timep;
	int days = 0;

	gTM.tm_sec = t % 60;
	t /= 60;

	gTM.tm_min = t % 60;
	t /= 60;

	gTM.tm_hour = t % 24;
	t /= 24;

	gTM.tm_wday = (t + 4) % 7;

	gTM.tm_year = 1970;
	while (true) {
		int daysInYear = 365;

		if (isLeapYear(gTM.tm_year))
			daysInYear += 1;

		if ((days + daysInYear) >= t)
			break;

		gTM.tm_year += 1;
		days += daysInYear;
	}

	t -= days;

	gTM.tm_yday = t;

	for (gTM.tm_mon = 0; gTM.tm_mon < 12; gTM.tm_mon++) {
		uint8_t daysInMonth = espRead8(gDaysInMonth + gTM.tm_mon);

		if ((1 == gTM.tm_mon) && isLeapYear(gTM.tm_year))
			daysInMonth = 29;

		if (t < daysInMonth)
			break;

		t -= daysInMonth;
	}
	gTM.tm_mday = t + 1;
	gTM.tm_year -= 1900;

	return &gTM;
}

struct modTm *modLocalTime(const modTime_t *timep)
{
	modTime_t t = *timep + gTimeZoneOffset + gDaylightSavings;
	return modGmTime(&t);
}

modTime_t modMkTime(struct modTm *tm)
{
#if ESP32
	struct tm theTm;
	theTm.tm_year = tm->tm_year;
	theTm.tm_mon = tm->tm_mon;
	theTm.tm_mday = tm->tm_mday;
	theTm.tm_hour = tm->tm_hour;
	theTm.tm_min = tm->tm_min;
	theTm.tm_sec = tm->tm_sec;

	return (modTime_t)(mktime(&theTm)) - (gTimeZoneOffset + gDaylightSavings);
#else
	return (modTime_t)(system_mktime(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec)) -
			(gTimeZoneOffset + gDaylightSavings);
#endif
}

static uint8_t gRTCInit = false;

void modGetTimeOfDay(struct modTimeVal *tv, struct modTimeZone *tz)
{
#if !ESP32
	if (!gRTCInit) {
		rtctime_early_startup();
		rtctime_late_startup();
		gRTCInit = true;
		modSetTime(819195899);
	}
#endif

	if (tv) {
		struct rtc_timeval rtc_tv;

		rtctime_gettimeofday(&rtc_tv);
		tv->tv_sec = rtc_tv.tv_sec;
		tv->tv_usec = rtc_tv.tv_usec;
	}

	if (tz) {
		tz->tz_minuteswest = gTimeZoneOffset;
		tz->tz_dsttime = gDaylightSavings;
	}
}

void modSetTime(uint32_t seconds)
{
#if !ESP32
	struct rtc_timeval rtc_tv;

	if (!gRTCInit) {
		rtctime_early_startup();
		rtctime_late_startup();
		gRTCInit = true;
	}

	rtc_tv.tv_sec = seconds;
	rtc_tv.tv_usec = 0;

	rtctime_settimeofday(&rtc_tv);
#else
	struct timeval tv;
	struct timezone tz;

	tv.tv_sec = seconds;
	tv.tv_usec = 0;

	settimeofday(&tv, NULL);		//@@ implementation doesn't use timezone yet....
#endif
}

void modSetTimeZone(int32_t timeZoneOffset)
{
	gTimeZoneOffset = timeZoneOffset;
}

int32_t modGetTimeZone(void)
{
	return gTimeZoneOffset;
}

void modSetDaylightSavingsOffset(int32_t daylightSavings)
{
	gDaylightSavings = daylightSavings;
}

int32_t modGetDaylightSavingsOffset(void)
{
	return gDaylightSavings;
}

#if MODDEF_XS_MODS
	static void *installModules(txPreparation *preparation);
	static char *findNthAtom(uint32_t atomTypeIn, int index, const uint8_t *xsb, int xsbSize, int *atomSizeOut);
	#define findAtom(atomTypeIn, xsb, xsbSize, atomSizeOut) findNthAtom(atomTypeIn, 0, xsb, xsbSize, atomSizeOut);

	static uint8_t gHasMods;
#endif

void *ESP_cloneMachine(uint32_t allocation, uint32_t stackCount, uint32_t slotCount, const char *name)
{
	extern txPreparation* xsPreparation();
	void *result;
	txMachine root;
	txPreparation *prep = xsPreparation();
	txCreation creation;
	uint8_t *context[2];

	if ((prep->version[0] != XS_MAJOR_VERSION) || (prep->version[1] != XS_MINOR_VERSION) || (prep->version[2] != XS_PATCH_VERSION))
		modLog("version mismatch");

	creation = prep->creation;

	root.preparation = prep;
#if MODDEF_XS_MODS
	root.archive = installModules(prep);
	gHasMods = root.archive != NULL;
#else
	root.archive = NULL;
#endif
	root.keyArray = prep->keys;
	root.keyCount = prep->keyCount + prep->creation.keyCount;
	root.keyIndex = prep->keyCount;
	root.nameModulo = prep->nameModulo;
	root.nameTable = prep->names;
	root.symbolModulo = prep->symbolModulo;
	root.symbolTable = prep->symbols;

	root.stack = &prep->stack[0];
	root.stackBottom = &prep->stack[0];
	root.stackTop = &prep->stack[prep->stackCount];

	root.firstHeap = &prep->heap[0];
	root.freeHeap = &prep->heap[prep->heapCount - 1];
	root.aliasCount = prep->aliasCount;

	if (0 == allocation)
		allocation = creation.staticSize;

	if (allocation) {
		if (stackCount)
			creation.stackCount = stackCount;

		if (slotCount)
			creation.initialHeapCount = slotCount;

		context[0] = c_malloc(allocation);
		if (NULL == context[0]) {
			modLog("failed to allocate xs block");
			return NULL;
		}
		context[1] = context[0] + allocation;

		result = fxCloneMachine(&creation, &root, name ? (txString)name : "main", context);
		if (NULL == result) {
			if (context[0])
				c_free(context[0]);
			return NULL;
		}

		((txMachine *)result)->context = NULL;
	}
	else {
		result = fxCloneMachine(&prep->creation, &root, "main", NULL);
		if (NULL == result)
			return NULL;
	}

	((txMachine *)result)->preparation = prep;

#ifdef mxInstrument
	espStartInstrumentation(result);
#endif

	return result;
}

static uint16_t gSetupPending = 0;

void setStepDone(xsMachine *the)
{
	gSetupPending -= 1;
	if (gSetupPending)
		return;

	xsBeginHost(the);
		xsResult = xsGet(xsGlobal, mxID(_require));
		xsResult = xsCall1(xsResult, mxID(_weak), xsString("main"));
		if (xsTest(xsResult) && xsIsInstanceOf(xsResult, xsFunctionPrototype))
			xsCallFunction0(xsResult, xsGlobal);
	xsEndHost(the);
}

void mc_setup(xsMachine *the)
{
	extern txPreparation* xsPreparation();
	txPreparation *preparation = xsPreparation();
	txInteger scriptCount = preparation->scriptCount;
	txScript* script = preparation->scripts;

	gSetupPending = 1;

	xsBeginHost(the);
		xsVars(2);
		xsVar(0) = xsNewHostFunction(setStepDone, 0);
		xsVar(1) = xsGet(xsGlobal, mxID(_require));

		while (scriptCount--) {
			if (0 == c_strncmp(script->path, "setup/", 6)) {
				char path[PATH_MAX];
				char *dot;

				c_strcpy(path, script->path);
				dot = c_strchr(path, '.');
				if (dot)
					*dot = 0;

				xsResult = xsCall1(xsVar(1), mxID(_weak), xsString(path));
				if (xsTest(xsResult) && xsIsInstanceOf(xsResult, xsFunctionPrototype)) {
					gSetupPending += 1;
					xsCallFunction1(xsResult, xsGlobal, xsVar(0));
				}
			}
			script++;
		}
	xsEndHost(the);

	setStepDone(the);
}

void *mc_xs_chunk_allocator(txMachine* the, size_t size)
{
	if (the->heap_ptr + size <= the->heap_pend) {
		void *ptr = the->heap_ptr;
		the->heap_ptr += size;
		return ptr;
	}

	modLog("!!! xs: failed to allocate chunk !!!\n");
	return NULL;
}

void mc_xs_chunk_disposer(txMachine* the, void *data)
{
	/* @@ too lazy but it should work... */
	if ((uint8_t *)data < the->heap_ptr)
		the->heap_ptr = data;

	if (the->heap_ptr == the->heap) {
		if (the->context) {
			uint8_t **context = the->context;
			context[0] = NULL;
		}
		c_free(the->heap);		// VM is terminated
	}
}

void *mc_xs_slot_allocator(txMachine* the, size_t size)
{
	if (the->heap_pend - size >= the->heap_ptr) {
		void *ptr = the->heap_pend - size;
		the->heap_pend -= size;
		return ptr;
	}

	modLog("!!! xs: failed to allocate slots !!!\n");
	return NULL;
}

void mc_xs_slot_disposer(txMachine *the, void *data)
{
	/* nothing to do */
}

void* fxAllocateChunks(txMachine* the, txSize theSize)
{
	if ((NULL == the->stack) && (NULL == the->heap)) {
		// initialization
		uint8_t **context = the->context;
		if (context) {
			the->heap = the->heap_ptr = context[0];
			the->heap_pend = context[1];
		}
	}

	if (NULL == the->heap)
		return c_malloc(theSize);

	return mc_xs_chunk_allocator(the, theSize);
}

txSlot* fxAllocateSlots(txMachine* the, txSize theCount)
{
	txSlot* result;

	if (NULL == the->heap)
		return (txSlot*)c_malloc(theCount * sizeof(txSlot));

	result = (txSlot *)mc_xs_slot_allocator(the, theCount * sizeof(txSlot));
	if (!result) {
		fxReport(the, "# Slot allocation: failed. trying to make room...\n");
		fxCollect(the, 1);	/* expecting memory from the chunk pool */
		if (the->firstBlock != C_NULL && the->firstBlock->limit == mc_xs_chunk_allocator(the, 0)) {	/* sanity check just in case */
			fxReport(the, "# Slot allocation: %d bytes returned\n", the->firstBlock->limit - the->firstBlock->current);
			the->maximumChunksSize -= the->firstBlock->limit - the->firstBlock->current;
			the->heap_ptr = the->firstBlock->current;
			the->firstBlock->limit = the->firstBlock->current;
		}
		result = (txSlot *)mc_xs_slot_allocator(the, theCount * sizeof(txSlot));
	}

	return result;
}

void fxFreeChunks(txMachine* the, void* theChunks)
{
	if (NULL == the->heap)
		c_free(theChunks);

	mc_xs_chunk_disposer(the, theChunks);
}

void fxFreeSlots(txMachine* the, void* theSlots)
{
	if (NULL == the->heap)
		c_free(theSlots);

	mc_xs_slot_disposer(the, theSlots);
}

void fxBuildKeys(txMachine* the)
{
}

static txBoolean fxFindScript(txMachine* the, txString path, txID* id)
{
	txPreparation* preparation = the->preparation;
	txInteger c = preparation->scriptCount;
	txScript* script = preparation->scripts;
	path += preparation->baseLength;
	c_strcat(path, ".xsb");
	while (c > 0) {
		if (!c_strcmp(path, script->path)) {
			path -= preparation->baseLength;
			*id = fxNewNameC(the, path);
			return 1;
		}
		c--;
		script++;
	}
	*id = XS_NO_ID;
	return 0;
}

#if MODDEF_XS_MODS

#define FOURCC(c1, c2, c3, c4) (((c1) << 24) | ((c2) << 16) | ((c3) << 8) | (c4))

static uint8_t *findMod(txMachine *the, char *name, int *modSize)
{
	uint8_t *xsb = (uint8_t *)kModulesStart;
	int modsSize;
	uint8_t *mods;
	int index = 0;
	int nameLen;
	char *dot;

	if (!xsb || !gHasMods) return NULL;

	mods = findAtom(FOURCC('M', 'O', 'D', 'S'), xsb, c_read32be(xsb), &modsSize);
	if (!mods) return NULL;

	dot = c_strchr(name, '.');
	if (dot)
		nameLen = dot - name;
	else
		nameLen = c_strlen(name);

	while (true) {
		uint8_t *aName = findNthAtom(FOURCC('P', 'A', 'T', 'H'), ++index, mods, modsSize, NULL);
		if (!aName)
			break;
		if (0 == c_strncmp(name, aName, nameLen)) {
			if (0 == c_strcmp(".xsb", aName + nameLen))
				return findNthAtom(FOURCC('C', 'O', 'D', 'E'), index, mods, modsSize, modSize);
		}
	}

	return NULL;
}
#endif

txID fxFindModule(txMachine* the, txID moduleID, txSlot* slot)
{
	txPreparation* preparation = the->preparation;
	char name[PATH_MAX];
	char path[PATH_MAX];
	txBoolean absolute = 0, relative = 0, search = 0;
	txInteger dot = 0;
	txString slash;
	txID id;

	fxToStringBuffer(the, slot, name, sizeof(name));
#if MODDEF_XS_MODS
	if (findMod(the, name, NULL)) {
		c_strcpy(path, "/");
		c_strcat(path, name);
		c_strcat(path, ".xsb");
		return fxNewNameC(the, path);
	}
#endif

	if (!c_strncmp(name, "/", 1)) {
		absolute = 1;
	}	
	else if (!c_strncmp(name, "./", 2)) {
		dot = 1;
		relative = 1;
	}	
	else if (!c_strncmp(name, "../", 3)) {
		dot = 2;
		relative = 1;
	}
	else {
		relative = 1;
		search = 1;
	}
	if (absolute) {
		c_strcpy(path, preparation->base);
		c_strcat(path, name + 1);
		if (fxFindScript(the, path, &id))
			return id;
	}
	if (relative && (moduleID != XS_NO_ID)) {
		c_strcpy(path, fxGetKeyName(the, moduleID));
		slash = c_strrchr(path, '/');
		if (!slash)
			return XS_NO_ID;
		if (dot == 0)
			slash++;
		else if (dot == 2) {
			*slash = 0;
			slash = c_strrchr(path, '/');
			if (!slash)
				return XS_NO_ID;
		}
		if (!c_strncmp(path, preparation->base, preparation->baseLength)) {
			*slash = 0;
			c_strcat(path, name + dot);
			if (fxFindScript(the, path, &id))
				return id;
		}
	}
	if (search) {
		c_strcpy(path, preparation->base);
		c_strcat(path, name);
		if (fxFindScript(the, path, &id))
			return id;
	}
	return XS_NO_ID;
}

void fxLoadModule(txMachine* the, txID moduleID)
{
	txPreparation* preparation = the->preparation;
	txString path = fxGetKeyName(the, moduleID) + preparation->baseLength;
	txInteger c = preparation->scriptCount;
	txScript* script = preparation->scripts;
#if MODDEF_XS_MODS
	uint8_t *mod;
	int modSize;

	mod = findMod(the, path, &modSize);
	if (mod) {
		txScript aScript;

		aScript.callback = NULL;
		aScript.symbolsBuffer = NULL;
		aScript.symbolsSize = 0;
		aScript.codeBuffer = mod;
		aScript.codeSize = modSize;
		aScript.hostsBuffer = NULL;
		aScript.hostsSize = 0;
		aScript.path = path - preparation->baseLength;
		aScript.version[0] = XS_MAJOR_VERSION;
		aScript.version[1] = XS_MINOR_VERSION;
		aScript.version[2] = XS_PATCH_VERSION;
		aScript.version[3] = 0;

		fxResolveModule(the, moduleID, &aScript, C_NULL, C_NULL);
		return;
	}
#endif

	while (c > 0) {
		if (!c_strcmp(path, script->path)) {
			fxResolveModule(the, moduleID, script, C_NULL, C_NULL);
			return;
		}
		c--;
		script++;
	}
}

void fxMarkHost(txMachine* the, txMarkRoot markRoot)
{
	the->host = C_NULL;
}

txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags)
{
	extern txPreparation* xsPreparation();
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	txScript* script = NULL;
	txPreparation *prep = xsPreparation();
	fxInitializeParser(parser, the, the->parserBufferSize, the->parserTableModulo);
	parser->firstJump = &jump;
	if (c_setjmp(jump.jmp_buf) == 0) {
		fxParserTree(parser, stream, getter, flags, NULL);
		fxParserHoist(parser);
		fxParserBind(parser);
		script = fxParserCode(parser);
	}
#ifdef mxInstrument
	if (the->peakParserSize < parser->total)
		the->peakParserSize = parser->total;
#endif
	fxTerminateParser(parser);
	return script;
}

void fxSweepHost(txMachine* the)
{
}

/*
	Instrumentation
*/

#ifdef mxInstrument

static void espSampleInstrumentation(modTimer timer, void *refcon, int32_t refconSize);

#define espInstrumentCount kModInstrumentationSystemFreeMemory - kModInstrumentationPixelsDrawn + 1
static char* espInstrumentNames[espInstrumentCount] ICACHE_XS6RO_ATTR = {
	(char *)"Pixels drawn",
	(char *)"Frames drawn",
	(char *)"Network bytes read",
	(char *)"Network bytes written",
	(char *)"Network sockets",
	(char *)"Timers",
	(char *)"Files",
	(char *)"Poco display list used",
	(char *)"Piu command List used",
	(char *)"System bytes free",
};

static char* espInstrumentUnits[espInstrumentCount] ICACHE_XS6RO_ATTR = {
	(char *)" pixels",
	(char *)" frames",
	(char *)" bytes",
	(char *)" bytes",
	(char *)" sockets",
	(char *)" timers",
	(char *)" files",
	(char *)" bytes",
	(char *)" bytes",
	(char *)" bytes",
};

txMachine *gInstrumentationThe;

static int32_t modInstrumentationSystemFreeMemory(void)
{
#if ESP32
	return (uint32_t)esp_get_free_heap_size();
#else
	return (int32_t)system_get_free_heap_size();
#endif
}

static int32_t modInstrumentationSlotHeapSize(void)
{
	return gInstrumentationThe->currentHeapCount * sizeof(txSlot);
}

static int32_t modInstrumentationChunkHeapSize(void)
{
	return gInstrumentationThe->currentChunksSize;
}

static int32_t modInstrumentationKeysUsed(void)
{
	return gInstrumentationThe->keyIndex - gInstrumentationThe->keyOffset;
}

static int32_t modInstrumentationGarbageCollectionCount(void)
{
	return gInstrumentationThe->garbageCollectionCount;
}

static int32_t modInstrumentationModulesLoaded(void)
{
	return gInstrumentationThe->loadedModulesCount;
}

static int32_t modInstrumentationStackRemain(void)
{
	if (gInstrumentationThe->stackPeak > gInstrumentationThe->stack)
		gInstrumentationThe->stackPeak = gInstrumentationThe->stack;
	return (gInstrumentationThe->stackTop - gInstrumentationThe->stackPeak) * sizeof(txSlot);
}

static modTimer gInstrumentationTimer;

void espDebugBreak(txMachine* the, uint8_t stop)
{
	if (stop) {
		fxCollectGarbage(the);
		the->garbageCollectionCount -= 1;
		espSampleInstrumentation(NULL, NULL, 0);
	}
	else
		modTimerReschedule(gInstrumentationTimer, 1000, 1000);
}

void espStartInstrumentation(txMachine *the)
{
	if ((NULL == the->connection) || gInstrumentationThe)
		return;

	modInstrumentationInit();
	modInstrumentationSetCallback(SystemFreeMemory, modInstrumentationSystemFreeMemory);

	modInstrumentationSetCallback(SlotHeapSize, modInstrumentationSlotHeapSize);
	modInstrumentationSetCallback(ChunkHeapSize, modInstrumentationChunkHeapSize);
	modInstrumentationSetCallback(KeysUsed, modInstrumentationKeysUsed);
	modInstrumentationSetCallback(GarbageCollectionCount, modInstrumentationGarbageCollectionCount);
	modInstrumentationSetCallback(ModulesLoaded, modInstrumentationModulesLoaded);
	modInstrumentationSetCallback(StackRemain, modInstrumentationStackRemain);

	fxDescribeInstrumentation(the, espInstrumentCount, espInstrumentNames, espInstrumentUnits);

	gInstrumentationTimer = modTimerAdd(0, 1000, espSampleInstrumentation, NULL, 0);
	gInstrumentationThe = the;

	the->onBreak = espDebugBreak;
}

void espSampleInstrumentation(modTimer timer, void *refcon, int32_t refconSize)
{
	txInteger values[espInstrumentCount];
	int what;

	for (what = kModInstrumentationPixelsDrawn; what <= kModInstrumentationSystemFreeMemory; what++)
		values[what - kModInstrumentationPixelsDrawn] = modInstrumentationGet_(what);

	values[kModInstrumentationTimers - kModInstrumentationPixelsDrawn] -= 1;	// remove timer used by instrumentation
	fxSampleInstrumentation(gInstrumentationThe, espInstrumentCount, values);

	modInstrumentationSet(PixelsDrawn, 0);
	modInstrumentationSet(FramesDrawn, 0);
	modInstrumentationSet(PocoDisplayListUsed, 0);
	modInstrumentationSet(PiuCommandListUsed, 0);
	modInstrumentationSet(NetworkBytesRead, 0);
	modInstrumentationSet(NetworkBytesWritten, 0);
	gInstrumentationThe->garbageCollectionCount = 0;
	gInstrumentationThe->stackPeak = gInstrumentationThe->stack;
	gInstrumentationThe->peakParserSize = 0;
}
#endif

#if ESP32

uint32_t modMilliseconds(void)
{
	return xTaskGetTickCount();
}

#endif

/*
	messages
*/

#if ESP32

typedef struct modMessageRecord modMessageRecord;
typedef modMessageRecord *modMessage;

struct modMessageRecord {
	char				*message;
	modMessageDeliver	callback;
	void				*refcon;
	uint16_t			length;
};

int modMessagePostToMachine(xsMachine *the, uint8_t *message, uint16_t messageLength, modMessageDeliver callback, void *refcon)
{
	modMessageRecord msg;

	if (message && messageLength) {
		msg.message = c_malloc(messageLength);
		if (!msg.message) return -1;

		c_memmove(msg.message, message, messageLength);
	}
	else
		msg.message = NULL;
	msg.length = messageLength;
	msg.callback = callback;
	msg.refcon = refcon;

	xQueueSend(the->msgQueue, &msg, portMAX_DELAY);

	return 0;
}

int modMessagePostToMachineFromISR(xsMachine *the, modMessageDeliver callback, void *refcon)
{
	modMessageRecord msg;
	portBASE_TYPE ignore;

	msg.message = NULL;
	msg.length = 0;
	msg.callback = callback;
	msg.refcon = refcon;

	xQueueSendFromISR(the->msgQueue, &msg, &ignore);

	return 0;
}

void modMessageService(xsMachine *the, int maxDelayMS)
{
	unsigned portBASE_TYPE count = uxQueueMessagesWaiting(the->msgQueue);

#if CONFIG_TASK_WDT
	modWatchDogReset();
	if (maxDelayMS >= (CONFIG_TASK_WDT_TIMEOUT_S * 1000)) {
		#if CONFIG_TASK_WDT_TIMEOUT_S <= 1
			maxDelayMS = 500;
		#else
			maxDelayMS = (CONFIG_TASK_WDT_TIMEOUT_S - 1) * 1000;
		#endif
	}
#endif

	while (true) {
		modMessageRecord msg;

		if (!xQueueReceive(the->msgQueue, &msg, maxDelayMS)) {
			modWatchDogReset();
			return;
		}

		(msg.callback)(the, msg.refcon, msg.message, msg.length);
		if (msg.message)
			c_free(msg.message);

		maxDelayMS = 0;
		if (count <= 1)
			break;
		count -= 1;
	}
}

void modMachineTaskInit(xsMachine *the)
{
	the->task = xTaskGetCurrentTaskHandle();
	the->msgQueue = xQueueCreate(10, sizeof(modMessageRecord));
}

void modMachineTaskUninit(xsMachine *the)
{
	if (the->msgQueue) {
		modMessageRecord msg;

		while (xQueueReceive(the->msgQueue, &msg, 0)) {
			if (msg.message)
				c_free(msg.message);
		}

		vQueueDelete(the->msgQueue);
	}
}

void modMachineTaskWait(xsMachine *the)
{
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

void modMachineTaskWake(xsMachine *the)
{
	xTaskNotifyGive(the->task);
}

#else

typedef struct modMessageRecord modMessageRecord;
typedef modMessageRecord *modMessage;

struct modMessageRecord {
	modMessage			next;
	xsMachine			*the;
	modMessageDeliver	callback;
	void				*refcon;
	uint16_t			length;
	uint8_t				marked;
	uint8_t				isStatic;		// this doubles as a flag to indicate entry is use gMessagePool
	char				message[1];
};

static modMessage gMessageQueue;

extern void esp_schedule();

static void appendMessage(modMessage msg)
{
	msg->next = NULL;
	msg->marked = 0;

	modCriticalSectionBegin();
	if (NULL == gMessageQueue) {
		gMessageQueue = msg;
		esp_schedule();
	}
	else {
		modMessage walker;

		for (walker = gMessageQueue; NULL != walker->next; walker = walker->next)
			;
		walker->next = msg;
	}
	modCriticalSectionEnd();
}

int modMessagePostToMachine(xsMachine *the, uint8_t *message, uint16_t messageLength, modMessageDeliver callback, void *refcon)
{
	modMessage msg = c_malloc(sizeof(modMessageRecord) + messageLength);
	if (!msg) return -1;

	msg->the = the;
	msg->callback = callback;
	msg->refcon = refcon;
	msg->isStatic = 0;

	if (message && messageLength)
		c_memmove(msg->message, message, messageLength);
	msg->length = messageLength;

	appendMessage(msg);

	return 0;
}

#define kMessagePoolCount (2)
static modMessageRecord gMessagePool[kMessagePoolCount];

int modMessagePostToMachineFromPool(xsMachine *the, modMessageDeliver callback, void *refcon)
{
	modMessage msg;
	uint8_t i;

	modCriticalSectionBegin();

	for (i = 0, msg = gMessagePool; i < kMessagePoolCount; i++, msg++) {
		if (!msg->isStatic) {
			msg->isStatic = 1;
			break;
		}
	}

	modCriticalSectionEnd();

	if ((gMessagePool + kMessagePoolCount) == msg) {
		modLog("message pool full");
		return -1;
	}

	msg->the = the;
	msg->callback = callback;
	msg->refcon = refcon;
	msg->length = 0;

	appendMessage(msg);

	return 0;
}

int modMessageService(void)
{
	modMessage msg = gMessageQueue;
	while (msg) {
		msg->marked = 1;
		msg = msg->next;
	}

	msg = gMessageQueue;
	while (msg && msg->marked) {
		modMessage next;

		if (msg->callback)
			(msg->callback)(msg->the, msg->refcon, msg->message, msg->length);

		modCriticalSectionBegin();
		next = msg->next;
		gMessageQueue = next;
		modCriticalSectionEnd();

		if (msg->isStatic)
			msg->isStatic = 0;		// return to pool
		else
			c_free(msg);
		msg = next;
	}

	return gMessageQueue ? 1 : 0;
}

void modMachineTaskInit(xsMachine *the) {}
void modMachineTaskUninit(xsMachine *the)
{
	modMessage msg = gMessageQueue;

	while (msg) {
		if (msg->the == the)
			msg->callback = NULL;
		msg = msg->next;
	}
}
#endif

/*
	promises
*/

static void doRunPromiseJobs(void *machine, void *refcon, uint8_t *message, uint16_t messageLength)
{
	fxRunPromiseJobs((txMachine *)machine);
}

void fxQueuePromiseJobs(txMachine* the)
{
	modMessagePostToMachine(the, NULL, 0, doRunPromiseJobs, NULL);
}

/*
	 user installable modules
*/

#if MODDEF_XS_MODS

#if ESP32

const esp_partition_t *gPartition;
const void *gPartitionAddress;

static txBoolean spiRead(void *src, size_t offset, void *buffer, size_t size)
{
	const esp_partition_t *partition = src;
	esp_partition_read(partition, (uintptr_t)offset, buffer, size);
	return 1;
}

static txBoolean spiWrite(void *dst, size_t offset, void *buffer, size_t size)
{
	const esp_partition_t *partition = dst;
	int result = esp_partition_erase_range(partition, (uintptr_t)offset, (size + SPI_FLASH_SEC_SIZE - 1) & ~(SPI_FLASH_SEC_SIZE - 1));
	if (ESP_OK != result)
		return 0;

	if (ESP_OK != esp_partition_write(partition, (uintptr_t)offset, buffer, size)) {
		modLog("write fail");
		return 0;
	}

	return 1;
}

void *installModules(txPreparation *preparation)
{
	const esp_partition_t *partition = esp_partition_find_first(0x40, 1,  NULL);
	const void *partitionAddress;
	if (!partition) return NULL;

	if (fxMapArchive(preparation, (void *)partition, (void *)partition, SPI_FLASH_SEC_SIZE, spiRead, spiWrite)) {
		spi_flash_mmap_handle_t handle;

		if (ESP_OK != esp_partition_mmap(partition, 0, partition->size, SPI_FLASH_MMAP_DATA, &partitionAddress, &handle))
			return NULL;
	}
	else
		return 0;

	gPartition = partition;
	gPartitionAddress = partitionAddress;

	return (void *)partitionAddress;
}

#else /* ESP8266 */

static const int FLASH_INT_MASK = ((2 << 8) | 0x3A);

extern uint8_t _XSMOD_start;
extern uint8_t _XSMOD_end;

#define kModulesInstallStart ((uintptr_t)&_XSMOD_start)
#define kModulesInstallEnd ((uintptr_t)&_XSMOD_end)

static txBoolean spiRead(void *src, size_t offset, void *buffer, size_t size)
{
	return modSPIRead(offset + (uintptr_t)src - (uintptr_t)kFlashStart, size, buffer);
}

static txBoolean spiWrite(void *dst, size_t offset, void *buffer, size_t size)
{
	offset += (uintptr_t)dst;

	if ((offset + SPI_FLASH_SEC_SIZE) > (uintptr_t)kModulesInstallStart)
		return 0;		// attempted write beyond end of available space

	if (!(offset & (SPI_FLASH_SEC_SIZE - 1))) {		// if offset is at start of a sector, erase that sector
		if (!modSPIErase(offset - (uintptr_t)kFlashStart, SPI_FLASH_SEC_SIZE))
			return 0;
	}

	return modSPIWrite(offset - (uintptr_t)kFlashStart, size, buffer);
}

void *installModules(txPreparation *preparation)
{
	uint8_t buffer[8];
	uint32_t modulesAtomSize, modulesAtomType;
	uint32_t freeSpace = kModulesInstallStart - (uint32_t)kModulesStart;
	
	spiRead((void *)kModulesInstallStart, 0, buffer, 8);
	modulesAtomSize = c_read32be(buffer);
	modulesAtomType = c_read32be(buffer + 4);
	
	if (modulesAtomType != XS_ATOM_ARCHIVE) return NULL;
	
	if (freeSpace < modulesAtomSize){
		modLog("mod is too large to install");
		return NULL;
	}

	if (fxMapArchive(preparation, (void *)kModulesInstallStart, kModulesStart, SPI_FLASH_SEC_SIZE, spiRead, spiWrite))
		return kModulesStart;

	return NULL;
}


#endif

char *findNthAtom(uint32_t atomTypeIn, int index, const uint8_t *xsb, int xsbSize, int *atomSizeOut)
{
	const uint8_t *atom = xsb, *xsbEnd = xsb + xsbSize;

	if (0 == index) {	// hack - only validate XS_A header at root...
		if (c_read32be(xsb + 4) != FOURCC('X', 'S', '_', 'A'))
			return NULL;

		atom += 8;
	}

	while (atom < xsbEnd) {
		int32_t atomSize = c_read32be(atom);
		uint32_t atomType = c_read32be(atom + 4);

		if ((atomSize < 8) || ((atom + atomSize) > xsbEnd))
			return NULL;

		if (atomType == atomTypeIn) {
			index -= 1;
			if (index <= 0) {
				if (atomSizeOut)
					*atomSizeOut = atomSize - 8;
				return (char *)(atom + 8);
			}
		}
		atom += atomSize;
	}

	if (atomSizeOut)
		*atomSizeOut = 0;

	return NULL;
}

#endif /* MODDEF_XS_MODS */

#if !ESP32
#include "flash_utils.h"

uint8_t *espFindUnusedFlashStart(void)
{
	image_header_t header;
	section_header_t section;
	uint32_t sectionFlashAddress = APP_START_OFFSET + sizeof(image_header_t);
	
	spi_flash_read(APP_START_OFFSET, (void *)&header, sizeof(image_header_t));
	spi_flash_read(sectionFlashAddress, (void *)&section, sizeof(section_header_t));
	
	while (header.num_segments--){
		sectionFlashAddress += section.size + sizeof(section_header_t);		
		spi_flash_read(sectionFlashAddress, (void *)&section, sizeof(section_header_t));
	}
	sectionFlashAddress += (uint32_t)kFlashStart;
	return (uint8 *)(((SPI_FLASH_SEC_SIZE - 1) + sectionFlashAddress) & ~(SPI_FLASH_SEC_SIZE - 1));
}

uint8_t modSPIRead(uint32_t offset, uint32_t size, uint8_t *dst)
{
	uint8_t temp[512] __attribute__ ((aligned (4)));
	uint32_t toAlign;

	if (offset & 3) {		// long align offset
		if (SPI_FLASH_RESULT_OK != spi_flash_read(offset & ~3, (uint32_t *)temp, 4))
			return 0;

		toAlign = 4 - (offset & 3);
		c_memcpy(dst, temp + 4 - toAlign, (size < toAlign) ? size : toAlign);

		if (size <= toAlign)
			return 1;

		dst += toAlign;
		offset += toAlign;
		size -= toAlign;
	}

	toAlign = size & ~3;
	if (toAlign) {
		size -= toAlign;
		if (((uintptr_t)dst) & ~3) {	// dst is not long aligned, copy through stack
			while (toAlign) {
				uint32_t use = (toAlign > sizeof(temp)) ? sizeof(temp) : toAlign;

				if (SPI_FLASH_RESULT_OK != spi_flash_read(offset, (uint32_t *)temp, use))
					return 0;
				c_memmove(dst, temp, use);

				toAlign -= use;
				dst += use;
				offset += use;
			}
		}
		else {
			if (SPI_FLASH_RESULT_OK != spi_flash_read(offset, (uint32_t *)dst, toAlign))
				return 0;
			dst += toAlign;
			offset += toAlign;
		}
	}

	if (size) {				// long align tail
		if (SPI_FLASH_RESULT_OK != spi_flash_read(offset, (uint32_t *)temp, 4))
			return 0;
		c_memcpy(dst, temp, size);
	}

	return 1;
}

uint8_t modSPIWrite(uint32_t offset, uint32_t size, const uint8_t *src)
{
	uint8_t temp[512] __attribute__ ((aligned (4)));
	uint32_t toAlign;

	if (offset & 3) {		// long align offset
		toAlign = 4 - (offset & 3);
		c_memset(temp, 0xFF, 4);
		c_memcpy(temp + 4 - toAlign, src, (size < toAlign) ? size : toAlign);
		if (SPI_FLASH_RESULT_OK != spi_flash_write(offset & ~3, (uint32_t *)temp, 4))
			return 0;

		if (size <= toAlign)
			return 1;

		src += toAlign;
		offset += toAlign;
		size -= toAlign;
	}

	toAlign = size & ~3;
	if (toAlign) {
		size -= toAlign;
		if (((uintptr_t)src) & ~3) {	// src is not long aligned, copy through stack
			while (toAlign) {
				uint32_t use = (toAlign > sizeof(temp)) ? sizeof(temp) : toAlign;
				c_memcpy(temp, src, use);
				if (SPI_FLASH_RESULT_OK != spi_flash_write(offset, (uint32_t *)temp, use))
					return 0;

				toAlign -= use;
				src += use;
				offset += use;
			}
		}
		else {
			if (SPI_FLASH_RESULT_OK != spi_flash_write(offset, (uint32_t *)src, toAlign))
				return 0;
			src += toAlign;
			offset += toAlign;
		}
	}

	if (size) {			// long align tail
		c_memset(temp, 0xFF, 4);
		c_memcpy(temp, src, size);
		if (SPI_FLASH_RESULT_OK != spi_flash_write(offset, (uint32_t *)temp, 4))
			return 0;
	}

	return 1;
}

uint8_t modSPIErase(uint32_t offset, uint32_t size)
{
	if ((offset & (SPI_FLASH_SEC_SIZE - 1)) || (size & (SPI_FLASH_SEC_SIZE - 1)))
		return 0;

	offset /= SPI_FLASH_SEC_SIZE;
	size /= SPI_FLASH_SEC_SIZE;
	while (size--) {
		optimistic_yield(10000);
		if (SPI_FLASH_RESULT_OK != spi_flash_erase_sector(offset++))
			return 0;
	}

	return 1;
}
#endif

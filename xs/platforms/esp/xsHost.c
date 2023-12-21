/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

#include "xsHosts.h"

#include "xsScript.h"

#if ESP32
	#include "sys/time.h"
	#define rtc_timeval timeval
	#define rtctime_gettimeofday(a) gettimeofday(a, NULL)

	portMUX_TYPE gCriticalMux = portMUX_INITIALIZER_UNLOCKED;

	#define INSTRUMENT_CPULOAD 1
	#if INSTRUMENT_CPULOAD
		#include "driver/gptimer.h"

		static uint16_t gCPUCounts[kTargetCPUCount * 2];
		static TaskHandle_t gIdles[kTargetCPUCount];
		static bool timer_group0_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *event, void *ctx);

		volatile uint32_t gCPUTime;
	#endif

	#include "freertos/task.h"
	#if MODDEF_XS_MODS
		#include "spi_flash/include/spi_flash_mmap.h"

		const esp_partition_t *gPartition;
		const uint8_t *gPartitionAddress;
	#endif
#else
	#include "Arduino.h"
	#include "rtctime.h"
	#include "spi_flash.h"

	#define FLASH_INT_MASK (((2 << 8) | 0x3A))
#endif

#ifdef mxInstrument
	#include "modTimer.h"
	#include "modInstrumentation.h"

	static void espInitInstrumentation(txMachine *the);
	static void espSampleInstrumentation(modTimer timer, void *refcon, int refconSize);

	#define espInstrumentCount kModInstrumentationSlotHeapSize - kModInstrumentationPixelsDrawn
	static char* const espInstrumentNames[espInstrumentCount] ICACHE_XS6RO_ATTR = {
		(char *)"Pixels drawn",
		(char *)"Frames drawn",
		(char *)"Network bytes read",
		(char *)"Network bytes written",
		(char *)"Network sockets",
		(char *)"Timers",
		(char *)"Files",
		(char *)"Poco display list used",
		(char *)"Piu command List used",
	#if ESP32
		(char *)"SPI flash erases",
	#endif
		(char *)"Turns",
		(char *)"System bytes free",
	#if ESP32
		#if kTargetCPUCount == 1
			(char *)"CPU",
		#else
			(char *)"CPU 0",
			(char *)"CPU 1",
		#endif
	#endif
	};

	static char* const espInstrumentUnits[espInstrumentCount] ICACHE_XS6RO_ATTR = {
		(char *)" pixels",
		(char *)" frames",
		(char *)" bytes",
		(char *)" bytes",
		(char *)" sockets",
		(char *)" timers",
		(char *)" files",
		(char *)" bytes",
		(char *)" bytes",
	#if ESP32
		(char *)" sectors",
	#endif
		(char *)" turns",
		(char *)" bytes",
	#if ESP32
		(char *)" percent",
		#if kTargetCPUCount > 1
			(char *)" percent",
		#endif
	#endif
	};

	#if ESP32
		SemaphoreHandle_t gInstrumentMutex;
	#endif
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

uint16_t espRead16be(const void *addr)
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

uint32_t espRead32be(const void *addr)
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

		while (c = c_read8(cs++)) {
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

		while (c = c_read8(cs++)) {
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
		uint8_t daysInMonth = c_read8(gDaysInMonth + gTM.tm_mon);

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

#if ESP32
static void updateTZ(void)
{
	int32_t offset = gTimeZoneOffset + gDaylightSavings;
	int32_t hours, minutes;
	char zone[10];

	zone[0] = 'U';
	zone[1] = 'T';
	zone[2] = 'C';
	zone[3] = (offset >= 0) ? '-' : '+';		// yes, backwards

	if (offset < 0)
		offset = -offset;
	offset /= 60;	// seconds to minutes
	hours = offset / 60;
	minutes = offset % 60;

	zone[4] = (hours / 10) + '0';
	zone[5] = (hours % 10) + '0';
	zone[6] = ':';
	zone[7] = (minutes / 10) + '0';
	zone[8] = (minutes % 10) + '0';
	zone[9] = 0;

	setenv("TZ", zone, 1);
	tzset();
}
#else
	#define updateTZ()
#endif

void modSetTimeZone(int32_t timeZoneOffset)
{
	gTimeZoneOffset = timeZoneOffset;
	updateTZ();
}

int32_t modGetTimeZone(void)
{
	return gTimeZoneOffset;
}

void modSetDaylightSavingsOffset(int32_t daylightSavings)
{
	gDaylightSavings = daylightSavings;
	updateTZ();
}

int32_t modGetDaylightSavingsOffset(void)
{
	return gDaylightSavings;
}

void modPrelaunch(void)
{
#if defined(mxDebug) && defined(MODDEF_STARTUP_DEBUGDELAYMS)
	modDelayMilliseconds(MODDEF_STARTUP_DEBUGDELAYMS);
#elif !defined(mxDebug) && defined(MODDEF_STARTUP_RELEASEDELAYMS)
	modDelayMilliseconds(MODDEF_STARTUP_RELEASEDELAYMS);
#elif defined(MODDEF_STARTUP_DELAYMS)
	modDelayMilliseconds(MODDEF_STARTUP_DELAYMS);
#endif
}

/*
	Instrumentation
*/

#ifdef mxInstrument

void modInstrumentationSetup(xsMachine *the)
{
	espInitInstrumentation(the);
	modInstrumentMachineBegin(the, espSampleInstrumentation, espInstrumentCount, (char**)espInstrumentNames, (char**)espInstrumentUnits);
}

static int32_t modInstrumentationSystemFreeMemory(void *theIn)
{
	txMachine *the = theIn;
#if ESP32
	return (uint32_t)heap_caps_get_free_size(MALLOC_CAP_8BIT);
#else
	return (int32_t)system_get_free_heap_size();
#endif
}

#if INSTRUMENT_CPULOAD
static gptimer_handle_t gLoadTimer = NULL;

static int32_t modInstrumentationCPU0(void *theIn)
{
	int32_t result, total = (gCPUCounts[0] + gCPUCounts[1]);
	if (!total)
		return 0;
	result = (100 * gCPUCounts[0]) / total;
	gCPUCounts[0] = gCPUCounts[1] = 0;
	return result;
}

#if kTargetCPUCount > 1
static int32_t modInstrumentationCPU1(void *theIn)
{
	int32_t result, total = (gCPUCounts[2] + gCPUCounts[3]);
	if (!total)
		return 0;
	result = (100 * gCPUCounts[2]) / total;
	gCPUCounts[2] = gCPUCounts[3] = 0;
	return result;
}
#endif
#endif


void espInitInstrumentation(txMachine *the)
{
	modInstrumentationInit();
	modInstrumentationSetCallback(SystemFreeMemory, modInstrumentationSystemFreeMemory);

	modInstrumentationSetCallback(SlotHeapSize, (ModInstrumentationGetter)modInstrumentationSlotHeapSize);
	modInstrumentationSetCallback(ChunkHeapSize, (ModInstrumentationGetter)modInstrumentationChunkHeapSize);
	modInstrumentationSetCallback(KeysUsed, (ModInstrumentationGetter)modInstrumentationKeysUsed);
	modInstrumentationSetCallback(GarbageCollectionCount, (ModInstrumentationGetter)modInstrumentationGarbageCollectionCount);
	modInstrumentationSetCallback(ModulesLoaded, (ModInstrumentationGetter)modInstrumentationModulesLoaded);
	modInstrumentationSetCallback(StackRemain, (ModInstrumentationGetter)modInstrumentationStackRemain);
	modInstrumentationSetCallback(PromisesSettledCount, (ModInstrumentationGetter)modInstrumentationPromisesSettledCount);

#if INSTRUMENT_CPULOAD
	modInstrumentationSetCallback(CPU0, modInstrumentationCPU0);
#if kTargetCPUCount > 1
	modInstrumentationSetCallback(CPU1, modInstrumentationCPU1);
#endif
/*
	timer_config_t config = {
		.divider = 16,
		.counter_dir = TIMER_COUNT_UP,
		.counter_en = TIMER_PAUSE,
		.alarm_en = TIMER_ALARM_EN,
		.auto_reload = 1,
	};
*/
	gptimer_config_t config = {
		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
		.direction = GPTIMER_COUNT_UP,
		.resolution_hz = 1000 * 1000,		// 1 tick = 1 us
	};
	gptimer_alarm_config_t alarm = {
		.reload_count = 0,
		.alarm_count = 800,
		.flags.auto_reload_on_alarm = true,
	};
	gptimer_event_callbacks_t callbacks = {
		.on_alarm = timer_group0_isr
	};

	gIdles[0] = xTaskGetIdleTaskHandleForCPU(0);
#if kTargetCPUCount > 1
	gIdles[1] = xTaskGetIdleTaskHandleForCPU(1);
#endif

	if (!gLoadTimer) {
		gptimer_new_timer(&config, &gLoadTimer);
		gptimer_set_alarm_action(gLoadTimer, &alarm);
		gptimer_register_event_callbacks(gLoadTimer, &callbacks, NULL);
		gptimer_enable(gLoadTimer);
		gptimer_start(gLoadTimer);
	}

#endif

#if ESP32
	if (!gInstrumentMutex)
		gInstrumentMutex = xSemaphoreCreateMutex();
#endif
}

void espSampleInstrumentation(modTimer timer, void *refcon, int refconSize)
{
	txInteger values[espInstrumentCount];
	int what;
	xsMachine *the = *(xsMachine **)refcon;

#if ESP32
	xSemaphoreTake(gInstrumentMutex, portMAX_DELAY);
#endif

	for (what = kModInstrumentationPixelsDrawn; what <= (kModInstrumentationSlotHeapSize - 1); what++)
		values[what - kModInstrumentationPixelsDrawn] = modInstrumentationGet_(the, what);

	if (values[kModInstrumentationTurns - kModInstrumentationPixelsDrawn])
        values[kModInstrumentationTurns - kModInstrumentationPixelsDrawn] -= 1;     // ignore the turn that generates instrumentation

	fxSampleInstrumentation(the, espInstrumentCount, values);

	modInstrumentationSet(PixelsDrawn, 0);
	modInstrumentationSet(FramesDrawn, 0);
	modInstrumentationSet(PocoDisplayListUsed, 0);
	modInstrumentationSet(PiuCommandListUsed, 0);
	modInstrumentationSet(NetworkBytesRead, 0);
	modInstrumentationSet(NetworkBytesWritten, 0);
	modInstrumentationSet(Turns, 0);
#if ESP32
	modInstrumentationSet(SPIFlashErases, 0);
#endif
	modInstrumentMachineReset(the);

#if ESP32
	xSemaphoreGive(gInstrumentMutex);
#endif
}

#if INSTRUMENT_CPULOAD
bool IRAM_ATTR timer_group0_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *event, void *ctx)
{
//    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
//    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);

	gCPUCounts[0 + (xTaskGetCurrentTaskHandleForCPU(0) == gIdles[0])] += 1;
#if kTargetCPUCount > 1
	gCPUCounts[2 + (xTaskGetCurrentTaskHandleForCPU(1) == gIdles[1])] += 1;
#endif

	gCPUTime += 1250;
}
#endif
#endif

/*
	messages
*/

#if ESP32

#ifndef MODDEF_TASK_QUEUEWAIT
	#ifdef mxDebug
		#define MODDEF_TASK_QUEUEWAIT (1000)
	#else
		#define MODDEF_TASK_QUEUEWAIT (portMAX_DELAY)
	#endif
#endif

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

#ifdef mxDebug
	if (0xffff == messageLength) {
		msg.message = NULL;
		msg.callback = callback;
		msg.refcon = refcon;
		msg.length = 0;
		xQueueSendToBack(the->dbgQueue, &msg, portMAX_DELAY);
		return 0;
	}
#endif

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

	if (pdTRUE == xQueueSendToBack(the->msgQueue, &msg, MODDEF_TASK_QUEUEWAIT))
		return 0;

	if (msg.message)
		c_free(msg.message);

	return -2;
}

int modMessagePostToMachineFromISR(xsMachine *the, modMessageDeliver callback, void *refcon)
{
	modMessageRecord msg;
	portBASE_TYPE ignore;

	msg.message = NULL;
	msg.length = 0;
	msg.callback = callback;
	msg.refcon = refcon;

	xQueueSendToBackFromISR(the->msgQueue, &msg, &ignore);

	return 0;
}

void modMessageService(xsMachine *the, int maxDelayMS)
{
	modMessageRecord msg;

#if CONFIG_ESP_TASK_WDT_EN
	modWatchDogReset();
    #ifndef CONFIG_ESP_TASK_WDT_TIMEOUT_S
		// The default timeout is 5s, but it can be changed using this CONFIG decl as well as
		// dynamically via esp_task_wdt_reconfigure, we assume "worst case" 1s here if it's not
		// defined (could actually be set lower dynamically). There is no way to extract the
		// current timeout value from esp-idf.
		#define CONFIG_ESP_TASK_WDT_TIMEOUT_S (1)
	#endif
	if (maxDelayMS >= (CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000)) {
		#if CONFIG_ESP_TASK_WDT_TIMEOUT_S <= 1
			maxDelayMS = 500;
		#else
			maxDelayMS = (CONFIG_ESP_TASK_WDT_TIMEOUT_S - 1) * 1000;
		#endif
	}
#endif

#ifdef mxDebug
	while (true) {
		QueueSetMemberHandle_t queue = xQueueSelectFromSet(the->queues, maxDelayMS);
		if (!queue)
			break;

		if (!xQueueReceive(queue, &msg, 0))
			break;

		(msg.callback)(the, msg.refcon, msg.message, msg.length);
		if (msg.message)
			c_free(msg.message);

		maxDelayMS = 0;
	}
#else
	while (xQueueReceive(the->msgQueue, &msg, maxDelayMS)) {
		(msg.callback)(the, msg.refcon, msg.message, msg.length);
		if (msg.message)
			c_free(msg.message);

		maxDelayMS = 0;
	}
#endif

	modWatchDogReset();
}

#ifndef modTaskGetCurrent
	#error make sure MOD_TASKS and modTaskGetCurrent are defined
#endif

#ifndef MODDEF_TASK_QUEUELENGTH
	#define MODDEF_TASK_QUEUELENGTH (10)
#endif

#define kDebugQueueLength (4)

void modMachineTaskInit(xsMachine *the)
{
	the->task = (void *)modTaskGetCurrent();
	the->msgQueue = xQueueCreate(MODDEF_TASK_QUEUELENGTH, sizeof(modMessageRecord));
#ifdef mxDebug
	the->dbgQueue = xQueueCreate(kDebugQueueLength, sizeof(modMessageRecord));

	the->queues = xQueueCreateSet(MODDEF_TASK_QUEUELENGTH + kDebugQueueLength);
	xQueueAddToSet(the->msgQueue, the->queues);
	xQueueAddToSet(the->dbgQueue, the->queues);
#endif

#if MODDEF_XS_MODS
	#if ESP32
		spi_flash_mmap_handle_t handle;

		gPartition = esp_partition_find_first(0x40, 1,  NULL);
		if (gPartition) {
			esp_partition_mmap(gPartition, 0, gPartition->size, SPI_FLASH_MMAP_DATA, (const void **)&gPartitionAddress, &handle);
		}
	#endif
#endif

}

void modMachineTaskUninit(xsMachine *the)
{
	modMessageRecord msg;

	if (the->msgQueue) {
		while (xQueueReceive(the->msgQueue, &msg, 0)) {
			if (msg.message)
				c_free(msg.message);
		}

#ifdef mxDebug
		xQueueRemoveFromSet(the->msgQueue, the->queues);
#endif
		vQueueDelete(the->msgQueue);
	}

#ifdef mxDebug
	if (the->dbgQueue) {
		while (xQueueReceive(the->dbgQueue, &msg, 0))
			;
		xQueueRemoveFromSet(the->dbgQueue, the->queues);
		vQueueDelete(the->dbgQueue);
	}
	if (the->queues)
		vQueueDelete(the->queues);
#endif
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
	uint16_t			length;
	uint8_t				isStatic;		// this doubles as a flag to indicate entry is use gMessagePool
	modMessageDeliver	callback;
	void				*refcon;
	char				message[];
};

static modMessage gMessageQueue;

extern void esp_schedule();

static void appendMessage(modMessage msg)
{
	msg->next = NULL;

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
#ifdef mxDebug
	if (0xffff == messageLength)
		messageLength = 0;
#endif

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

#define kMessagePoolCount (4)
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
	modMessage msg;
	modCriticalSectionBegin();
	msg = gMessageQueue;
	gMessageQueue = NULL;
	modCriticalSectionEnd();

	while (msg) {
		modMessage next = msg->next;

		if (msg->callback)
			(msg->callback)(msg->the, msg->refcon, msg->message, msg->length);

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

#if ESP32
esp_err_t __wrap_spi_flash_erase_sector(size_t sector)
{
	extern esp_err_t __real_spi_flash_erase_sector(size_t sector);
#ifdef mxInstrument
	modInstrumentationAdjust(SPIFlashErases, +1);
#endif
	return __real_spi_flash_erase_sector(sector);
}

esp_err_t __wrap_spi_flash_erase_range(uint32_t start_addr, uint32_t size)
{
	extern esp_err_t __real_spi_flash_erase_range(uint32_t start_addr, uint32_t size);
#ifdef mxInstrument
	modInstrumentationAdjust(SPIFlashErases, (size / kFlashSectorSize));
#endif
	return __real_spi_flash_erase_range(start_addr, size);
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

static txBoolean spiRead(void *src, size_t offset, void *buffer, size_t size)
{
	const esp_partition_t *partition = src;
	esp_partition_read(partition, (uintptr_t)offset, buffer, size);
	return 1;
}

static txBoolean spiWrite(void *dst, size_t offset, void *buffer, size_t size)
{
	const esp_partition_t *partition = dst;
//@@ this erase seems wrong... unless offset is always a sector boundary?
	int result = esp_partition_erase_range(partition, (uintptr_t)offset, (size + kFlashSectorSize - 1) & ~(kFlashSectorSize - 1));
	if (ESP_OK != result)
		return 0;

	if (ESP_OK != esp_partition_write(partition, (uintptr_t)offset, buffer, size)) {
		modLog("write fail");
		return 0;
	}

	return 1;
}

void *modInstallMods(xsMachine *the, void *preparationIn, uint8_t *status)
{
	txPreparation *preparation = preparationIn; 
	void *result = NULL;

	if (!gPartitionAddress)
		return NULL;

	if (fxMapArchive(the, preparation, (void *)gPartition, kFlashSectorSize, spiRead, spiWrite)) {
		result = (void *)gPartitionAddress;
		fxSetArchive(the, result);
	}

	if (XS_ATOM_ERROR == c_read32be(4 + kModulesStart)) {
		*status = *(8 + (uint8_t *)kModulesStart);
		modLog("mod failed");
	}
	else
		*status = 0;

	return result;
}

#else /* ESP8266 */

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

	if (fxMapArchive(the, preparation, (void *)kModulesStart, kFlashSectorSize, spiRead, spiWrite)) {
		fxSetArchive(the, kModulesStart);
		return kModulesStart;
	}

	return NULL;
}

#endif

#endif /* MODDEF_XS_MODS */

#if !ESP32
#include "flash_utils.h"

// declarations from rboot.h
// standard rom header
typedef struct {
	// general rom header
	uint8 magic;
	uint8 count;
	uint8 flags1;
	uint8 flags2;
	void* entry;
} rom_header;

typedef struct {
	// general rom header
	uint8 magic;
	uint8 count; // second magic for new header
	uint8 flags1;
	uint8 flags2;
	uint32 entry;
	// new type rom, lib header
	uint32 add; // zero
	uint32 len; // length of irom section
} rom_header_new;

uint8_t *espFindUnusedFlashStart(void)
{
	rom_header_new header;
	uint32_t pos = APP_START_OFFSET;

	spi_flash_read(pos, (void *)&header, sizeof(rom_header_new));
	if ((0xea != header.magic) || (4 != header.count)) {
		modLog("unrecognized boot loader");
		return NULL;
	}

	pos += header.len + sizeof(rom_header_new);
	spi_flash_read(pos, (void *)&header, sizeof(rom_header));

	pos += sizeof(rom_header);
	while (header.count--) {
		section_header_t section;
		spi_flash_read(pos, (void *)&section, sizeof(section_header_t));
		pos += section.size + sizeof(section_header_t);
	}

	pos += (uint32_t)kFlashStart;
	return (uint8 *)(((kFlashSectorSize - 1) + pos) & ~(kFlashSectorSize - 1));
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

	ets_isr_mask(FLASH_INT_MASK);

	if (offset & 3) {		// long align offset
		toAlign = 4 - (offset & 3);
		c_memset(temp, 0xFF, 4);
		c_memcpy(temp + 4 - toAlign, src, (size < toAlign) ? size : toAlign);
		if (SPI_FLASH_RESULT_OK != spi_flash_write(offset & ~3, (uint32_t *)temp, 4))
			goto fail;

		if (size <= toAlign)
			goto bail;

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
				if (SPI_FLASH_RESULT_OK != spi_flash_write(offset, (uint32_t *)temp, use))
					goto fail;

				toAlign -= use;
				src += use;
				offset += use;
			}
		}
		else {
			if (SPI_FLASH_RESULT_OK != spi_flash_write(offset, (uint32_t *)src, toAlign))
				goto fail;
			src += toAlign;
			offset += toAlign;
		}
	}

	if (size) {			// long align tail
		c_memset(temp, 0xFF, 4);
		c_memcpy(temp, src, size);
		if (SPI_FLASH_RESULT_OK != spi_flash_write(offset, (uint32_t *)temp, 4))
			goto fail;
	}

bail:
	ets_isr_unmask(FLASH_INT_MASK);
	return 1;

fail:
	ets_isr_unmask(FLASH_INT_MASK);
	return 0;
}

uint8_t modSPIErase(uint32_t offset, uint32_t size)
{
	if ((offset & (kFlashSectorSize - 1)) || (size & (kFlashSectorSize - 1)))
		return 0;

	offset /= kFlashSectorSize;
	size /= kFlashSectorSize;
	while (size--) {
		int err;

		optimistic_yield(10000);
    	ets_isr_mask(FLASH_INT_MASK);
    	err = spi_flash_erase_sector(offset++);
    	ets_isr_unmask(FLASH_INT_MASK);
		if (SPI_FLASH_RESULT_OK != err)
			return 0;
	}

	return 1;
}
#endif

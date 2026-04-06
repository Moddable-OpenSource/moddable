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

//#include <_ansi.h>
#include <setjmp.h>
#include "xsAll.h"
#include "xs.h"
#include "xsScript.h"
#include "xsPlatform.h"
#include "xsHosts.h"
#include "mc.defines.h"

#include <stdio.h>

#ifdef mxInstrument
	#include "modTimer.h"
	#include "modInstrumentation.h"

	#define INSTRUMENT_CPULOAD 0		//@@ zephyr
	#define kTargetCPUCount 1

	#if INSTRUMENT_CPULOAD
		static uint32_t gCPUCounts[kTargetCPUCount * 2];
		static k_tid_t gIdles[kTargetCPUCount];
		static void cpuTimerHandler(nrf_timer_event_t event_type, void* p_context);

		volatile uint32_t gCPUTime;
	#endif

	static void espInitInstrumentation(txMachine *the);
	static void espSampleInstrumentation(modTimer timer, void *refcon, int refconSize);

	#define espInstrumentCount kModInstrumentationSlotHeapSize - kModInstrumentationPixelsDrawn
	static char* const espInstrumentNames[espInstrumentCount] ICACHE_XS6RO_ATTR = {
		(char *)"Pixels drawn",
		(char *)"Frames drawn",
#if kModInstrumentationHasNetwork
		(char *)"Network bytes read",
		(char *)"Network bytes written",
		(char *)"Network sockets",
#endif
		(char *)"Timers",
		(char *)"Files",
		(char *)"Poco display list used",
		(char *)"Piu command List used",
		(char *)"Event loop",
		(char *)"System bytes free",
#if kModInstrumentationHasCPU
		(char *)"CPU",
#endif
	};

	static char* const espInstrumentUnits[espInstrumentCount] ICACHE_XS6RO_ATTR = {
		(char *)" pixels",
		(char *)" frames",
#if kModInstrumentationHasNetwork
		(char *)" bytes",
		(char *)" bytes",
		(char *)" sockets",
#endif
		(char *)" timers",
		(char *)" files",
		(char *)" bytes",
		(char *)" bytes",
		(char *)" turns",
		(char *)" bytes",
#if kModInstrumentationHasCPU
		(char *)" percent",
#endif
	};

	struct k_mutex gInstrumentMutex;
#endif

void modLog_transmit(const char *msg)
{
	printf("%s", msg);
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

#include <zephyr/sys/sys_heap.h>

static int32_t modInstrumentationSystemFreeMemory(void *theIn)
{
	struct k_heap *ha;
	int n, free_bytes = 0;

	n = k_heap_array_get(&ha);
	for (int i = 0; i < n; i++) {
		struct sys_memory_stats stats;
		sys_heap_runtime_stats_get(&ha[i].heap, &stats);
		free_bytes += stats.free_bytes;
	}

	return free_bytes;
}

#if INSTRUMENT_CPULOAD
static int32_t modInstrumentationCPU0(void *theIn)
{
	int32_t result, total = (gCPUCounts[0] + gCPUCounts[1]);
	if (!total)
		return 0;
	result = (100 * gCPUCounts[0]) / total;
	gCPUCounts[0] = gCPUCounts[1] = 0;
	return result;
}
#endif

void espInitInstrumentation(txMachine *the)
{
#if MODDEF_XS_TEST
	static uint8_t initialized = 0;
	if (initialized)
		return;
	initialized = 1;
#endif

	modInstrumentationInit();
	modInstrumentationSetCallback(SystemFreeMemory, modInstrumentationSystemFreeMemory);

	modInstrumentationSetCallback(SlotHeapSize, (ModInstrumentationGetter)modInstrumentationSlotHeapSize);
	modInstrumentationSetCallback(ChunkHeapSize, (ModInstrumentationGetter)modInstrumentationChunkHeapSize);
	modInstrumentationSetCallback(KeysUsed, (ModInstrumentationGetter)modInstrumentationKeysUsed);
	modInstrumentationSetCallback(GarbageCollectionCount, (ModInstrumentationGetter)modInstrumentationGarbageCollectionCount);
	modInstrumentationSetCallback(ModulesLoaded, (ModInstrumentationGetter)modInstrumentationModulesLoaded);
	modInstrumentationSetCallback(StackRemain, (ModInstrumentationGetter)modInstrumentationStackRemain);
	modInstrumentationSetCallback(PromisesSettledCount, (ModInstrumentationGetter)modInstrumentationPromisesSettledCount);

	k_mutex_init(&gInstrumentMutex);

#if INSTRUMENT_CPULOAD
/** @@MDK zephyr
	modInstrumentationSetCallback(CPU0, modInstrumentationCPU0);

	nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
	nrf_drv_timer_init(&cpuTimer, &timer_cfg, cpuTimerHandler);
	uint32_t ticks = nrf_drv_timer_us_to_ticks(&cpuTimer, CPUTIMER_US);
	nrf_drv_timer_extended_compare(&cpuTimer, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

	gIdles[0] = xTaskGetIdleTaskHandle();

	nrf_drv_timer_clear(&cpuTimer);
	nrf_drv_timer_enable(&cpuTimer);
**/
#endif
}

extern struct k_mutex gDebugMutex;

void espSampleInstrumentation(modTimer timer, void *refcon, int refconSize)
{
	txInteger values[espInstrumentCount];
	int what;
	xsMachine *the = *(xsMachine **)refcon;

	k_mutex_lock(&gInstrumentMutex, K_FOREVER);

	for (what = kModInstrumentationPixelsDrawn; what <= (kModInstrumentationSlotHeapSize - 1); what++)
		values[what - kModInstrumentationPixelsDrawn] = modInstrumentationGet_(the, what);

	if (values[kModInstrumentationTurns - kModInstrumentationPixelsDrawn])
		values[kModInstrumentationTurns - kModInstrumentationPixelsDrawn] -= 1;		// ignore the turn that generates instrumentation

	fxSampleInstrumentation(the, espInstrumentCount, values);

	modInstrumentationSet(PixelsDrawn, 0);
	modInstrumentationSet(FramesDrawn, 0);
	modInstrumentationSet(PocoDisplayListUsed, 0);
	modInstrumentationSet(PiuCommandListUsed, 0);
#if kModInstrumentationHasNetwork
	modInstrumentationSet(NetworkBytesRead, 0);
	modInstrumentationSet(NetworkBytesWritten, 0);
#endif
	modInstrumentationSet(Turns, 0);
	modInstrumentMachineReset(the);

	k_mutex_unlock(&gInstrumentMutex);
}

#if INSTRUMENT_CPULOAD
static void cpuTimerHandler(nrf_timer_event_t event_type, void* p_context)
{
/**@@MDK zephyr
	switch (event_type) {
		case NRF_TIMER_EVENT_COMPARE0:
			gCPUCounts[0 + (xTaskGetCurrentTaskHandle() == gIdles[0])] += 1;
			gCPUTime += 1250;
			break;
	}
**/
}
#endif
#endif

/*
	messages
*/
#ifndef MODDEF_TASK_QUEUEWAIT
	#ifdef mxDebug
		#define MODDEF_TASK_QUEUEWAIT	K_MSEC(5)
	#else
		#define MODDEF_TASK_QUEUEWAIT	K_FOREVER
	#endif
#endif
typedef struct modMessageRecord modMessageRecord;
typedef modMessageRecord *modMessage;

struct modMessageRecord {
	uint8_t				*message;
	modMessageDeliver   callback;
	void                *refcon;
	uint16_t            length;
	uint8_t				embeddedMessage;
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
		return k_msgq_put(&the->msgQueue, &msg, MODDEF_TASK_QUEUEWAIT);		//@@ separate queue for debug msgs
	}
#endif

	msg.embeddedMessage = 0;
	if (message && messageLength) {
		if (messageLength <= sizeof(msg.message)) {
			msg.embeddedMessage = 1;
			c_memmove(&msg.message, message, messageLength);
		}
		else {
			msg.message = c_malloc(messageLength);
			if (!msg.message) return -1;

			c_memmove(msg.message, message, messageLength);
		}
	}
	else
		msg.message = NULL;

	msg.length = messageLength;
	msg.callback = callback;
	msg.refcon = refcon;

	if (0 == k_msgq_put(&the->msgQueue, &msg, MODDEF_TASK_QUEUEWAIT))
		return 0;

	if (msg.message && !msg.embeddedMessage)
		c_free(msg.message);

	return -2;
}

int modMessagePostToMachineFromISR(xsMachine *the, modMessageDeliver callback, void *refcon)
{
	modMessageRecord msg;

	msg.message = NULL;
	msg.length = 0;
	msg.callback = callback;
	msg.refcon = refcon;
	msg.embeddedMessage = 0;

	k_msgq_put(&the->msgQueue, &msg, K_NO_WAIT);

	return 0;
}

void modMessageService(xsMachine *the, int maxDelayMS)
{
	modMessageRecord msg;
	uint32_t startTime = modMilliseconds();
	uint32_t maxDuration = (uint32_t)maxDelayMS;
	uint32_t count;
	int err;

	count = k_msgq_num_used_get(&the->msgQueue);
	if (!count) count = 1;

#if !mxDebug
	modWatchDogReset();
#endif

	do {
		err = k_msgq_get(&the->msgQueue, &msg, Z_TIMEOUT_MS(maxDelayMS));
		if (0 == err) {
			if (msg.embeddedMessage)
				(msg.callback)(the, msg.refcon, (uint8_t *)&msg.message, msg.length);
			else {
				(msg.callback)(the, msg.refcon, msg.message, msg.length);
				if (msg.message)
					c_free(msg.message);
			}
		}

		if (!--count)
			break;
		maxDelayMS = 0;
	} while ((modMilliseconds() - startTime) < maxDuration);

	modWatchDogReset();
}

#ifndef modTaskGetCurrent
	#error make sure MOD_TASKS and modTaskGetCurrent are defined
#endif

#ifndef MODDEF_TASK_QUEUELENGTH
	#define MODDEF_TASK_QUEUELENGTH	(10)
#endif

#define kDebugQueueLength (4)

void modMachineTaskInit(xsMachine *the)
{
	the->task = (void *)modTaskGetCurrent();
	k_msgq_alloc_init(&the->msgQueue, sizeof(modMessageRecord), MODDEF_TASK_QUEUELENGTH);
}

void modMachineTaskUninit(xsMachine *the)
{
	modMessageRecord msg;

	while (0 == k_msgq_get(&the->msgQueue, &msg, K_NO_WAIT)) {
		if (msg.message && !msg.embeddedMessage)
			c_free(msg.message);
	}

	k_msgq_cleanup(&the->msgQueue);
}

void modMachineTaskWait(xsMachine *the)
{
	// unused
}

void modMachineTaskWake(xsMachine *the)
{
	// unused
}

/*
	promises
*/

static void doRunPromiseJobs(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	fxRunPromiseJobs((txMachine *)the);
}

void fxQueuePromiseJobs(txMachine* the)
{
	modMessagePostToMachine(the, NULL, 0, doRunPromiseJobs, NULL);
}

/*
	 user installable modules
*/

#if MODDEF_XS_MODS

static struct flash_area *gModFlashArea = C_NULL;
static uint32_t gModFlashBlockSize = 0;
static const void *gPartitionAddress = C_NULL;
uint8_t gModWriteAlign = 0;

static txBoolean spiRead(void *src, size_t offset, void *buffer, size_t size)
{
	return modSPIRead(offset + (uintptr_t)src - (uintptr_t)gPartitionAddress, size, buffer);
}

static txBoolean spiWrite(void *dst, size_t offset, void *buffer, size_t size)
{
	offset += (uintptr_t)dst;

	if ((offset + size) > (uintptr_t)kModulesEnd)
		return 0;		// attempted write beyond end of available space

	// depends on fxMapArchive writng one sector at a time
	if (!(offset & (gModFlashBlockSize - 1))) {		// offset at start of a sector, erase sector
		if (!modSPIErase(offset - (uintptr_t)gPartitionAddress, gModFlashBlockSize))
			return 0;
	}

	return modSPIWrite(offset - (uintptr_t)gPartitionAddress, size, buffer);
}

void *modInstallMods(xsMachine *the, void *preparationIn, uint8_t *status)
{
	if (!modSPIFlashInit() || (0 == gModFlashBlockSize))
		return 0;

	txPreparation *preparation = preparationIn;
	void *result = NULL;
	if (fxMapArchive(the, preparation, (void *)kModulesStart, gModFlashBlockSize, spiRead, spiWrite)) {
		result = (void *)kModulesStart;
		fxSetArchive(the, result);
	}

	if (XS_ATOM_ERROR == c_read32be((void *)(4 + kModulesStart)))
		*status = *(8 + (uint8_t *)kModulesStart);
	else
		*status = 0;

	return result;
}

/*
	flash
 */

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#if defined(CONFIG_SOC_SERIES_ESP32) || \
    defined(CONFIG_SOC_SERIES_ESP32S2) || \
    defined(CONFIG_SOC_SERIES_ESP32S3) || \
    defined(CONFIG_SOC_SERIES_ESP32C1) || \
    defined(CONFIG_SOC_SERIES_ESP32C3) || \
    defined(CONFIG_SOC_SERIES_ESP32C6) || \
    defined(CONFIG_SOC_SERIES_ESP32H2)	 
	#define CONFIG_SOC_FAMILY_ESP32 1
#endif

#ifdef CONFIG_SOC_FAMILY_ESP32
	#include <spi_flash_mmap.h>
#endif

uint8_t modSPIFlashInit(void)
{
	if (gModFlashArea)
		return 1;

	int gModPartitionID = FIXED_PARTITION_ID(xs_mod);
	const struct flash_area *fa;
	if (0 != flash_area_open(gModPartitionID, &fa))
		return 0;

	gModFlashArea = (struct flash_area *)fa;
	gModWriteAlign = flash_area_align(gModFlashArea);

	struct flash_pages_info page_info;
	if (flash_get_page_info_by_offs(gModFlashArea->fa_dev, gModFlashArea->fa_off, &page_info) < 0) {
		flash_area_close(gModFlashArea);		
		return 0;
	}
	gModFlashBlockSize = page_info.size;

#if defined(CONFIG_SOC_FAMILY_ESP32)
	spi_flash_mmap_handle_t handle;
	spi_flash_mmap(gModFlashArea->fa_off, gModFlashArea->fa_size, SPI_FLASH_MMAP_DATA, &gPartitionAddress, &handle);
#elif defined(CONFIG_SOC_FAMILY_STM32)
	gPartitionAddress = (void *)(0x08000000 + gModFlashArea->fa_off);
#else // fallback - load into RAM
	uint32_t modSize;
	if (!modSPIRead(0, sizeof(uint32_t), (uint8_t *)&modSize))
		return 0;
	modSize = c_read32be(&modSize);
	gPartitionAddress = c_malloc(modSize);
	if (C_NULL == gPartitionAddress)
		return 0;
	if (!modSPIRead(0, modSize, (uint8_t *)gPartitionAddress)) {
		c_free(gPartitionAddress);
		gPartitionAddress = C_NULL;
		return 0;
	}
#endif

	return 1;
}

uint32_t modGetFlashStart(void)
{
	if (!modSPIFlashInit())
		return 0;

	return (uintptr_t)gPartitionAddress;
}

uint32_t modGetFlashSectorSize(void)
{
	if (!modSPIFlashInit())
		return 0;

	return gModFlashBlockSize;
}

uint32_t modGetModulesStart(void)
{
	if (!modSPIFlashInit())
		return 0;

	return (uintptr_t)gPartitionAddress;
}

uint32_t modGetModulesEnd(void)
{
	if (!modSPIFlashInit())
		return 0;

	return (uintptr_t)gPartitionAddress + gModFlashArea->fa_size;
}

uint8_t modSPIRead(uint32_t offset, uint32_t size, uint8_t *dst)
{
	if (!modSPIFlashInit())
		return 0;

	if (flash_area_read(gModFlashArea, offset, dst, size) < 0)
		return 0;
	
	return 1;
}

uint8_t modSPIWrite(uint32_t offset, uint32_t size, const uint8_t *src)
{
	if (!modSPIFlashInit())
		return 0;

	size = (size + gModWriteAlign - 1) & ~(gModWriteAlign - 1);		// ugly hack, but works for mods. the alternative is to make all writers aware of gModWriteAlign

	if (flash_area_write(gModFlashArea, offset, src, size) < 0)
		return 0;

	return 1;
}

uint8_t modSPIErase(uint32_t offset, uint32_t size)
{
	if (!modSPIFlashInit())
		return 0;

	if (flash_area_erase(gModFlashArea, offset, size) < 0)
		return 0;

	return 1;
}

#endif /* MODDEF_XS_MODS */

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

static int32_t gTimeZoneOffset = -8 * 60 * 60;      // Menlo Park
static int16_t gDaylightSavings = 60 * 60;          // summer time

static modTm gTM;		//@@ eliminate with _r calls

static const uint8_t gDaysInMonth[] ICACHE_XS6RO2_ATTR = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define isLeapYear(YEAR) (!(YEAR % 4) && ((YEAR % 100) || !(YEAR % 400)))

// Get Day of Year
int getDOY(int year, int month, int day) {
    int dayCount[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int dayOfYear = dayCount[month] + day;
    if(month > 1 && isLeapYear(year)) dayOfYear++;
    return dayOfYear - 1;		// tm_yday is 0-365
};

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

		if ((days + daysInYear) > t)
			break;
		gTM.tm_year += 1;
		days += daysInYear;
	}
	t -= days;
	gTM.tm_yday = t;
	for (gTM.tm_mon = 0; gTM.tm_mon < 12; gTM.tm_mon++) {
		uint8_t daysInMonth = gDaysInMonth[gTM.tm_mon];
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

c_tm *modLocalTime(const c_time_t *timep)
{
	c_time_t t = *timep + gTimeZoneOffset + gDaylightSavings;
	return c_gmtime(&t);
}

// http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15
modTime_t modMkTime(struct modTm *tm)
{
	modTime_t t;
	uint16_t yday;

	yday = getDOY(tm->tm_year + 1900, tm->tm_mon, tm->tm_mday);

	t =      tm->tm_sec
		+   (tm->tm_min * 60)
		+   (tm->tm_hour * 3600)
		+   (yday * 86400)
		+  ((tm->tm_year-70) * 31536000)
		+ (((tm->tm_year-69)/4) * 86400)
		- (((tm->tm_year-1)/100) * 86400)
		+ (((tm->tm_year+299)/400)*86400);
	t = t - (gTimeZoneOffset + gDaylightSavings);

	return t;
}

void modGetTimeOfDay(struct modTimeVal *tv, struct modTimeZone *tz)
{
	struct timespec ts;

	sys_clock_gettime(SYS_CLOCK_REALTIME, &ts);

	if (tv) {
		tv->tv_sec = ts.tv_sec;
		tv->tv_usec = ts.tv_nsec / 1000;
	}
	if (tz) {
//		tz->tz_minuteswest = gTimeZoneOffset;
//		tz->tz_dsttime = gDaylightSavings;
	}
}

#include <zephyr/sys/clock.h>

void modSetTime(uint32_t seconds)
{
	//@@ untested?
	struct timespec ts = { .tv_sec = seconds };
	sys_clock_settime(SYS_CLOCK_REALTIME, &ts);
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



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
#include "xsScript.h"
#include "xsPlatform.h"
#include "xsHosts.h"
#include "mc.defines.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#ifndef MODDEF_XS_MODS
	#define MODDEF_XS_MODS	0
#endif

#ifdef mxInstrument
	#include "modTimer.h"
	#include "modInstrumentation.h"

	#define INSTRUMENT_CPULOAD 1
	#define kTargetCPUCount 1

	#if INSTRUMENT_CPULOAD
		#include "nrf_drv_timer.h"

		static uint32_t gCPUCounts[kTargetCPUCount * 2];
		static TaskHandle_t gIdles[kTargetCPUCount];
		static void cpuTimerHandler(nrf_timer_event_t event_type, void* p_context);

		volatile uint32_t gCPUTime;
		static nrf_drv_timer_t cpuTimer = NRF_DRV_TIMER_INSTANCE(3);
		#define CPUTIMER_US 800
	#endif

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
		(char *)"Event loop",
		(char *)"System bytes free",
		(char *)"CPU",
	};

	static char* const espInstrumentUnits[espInstrumentCount] ICACHE_XS6RO_ATTR = {
		(char *)" pixels",
		(char *)" frames",
		(char *)" timers",
		(char *)" files",
		(char *)" bytes",
		(char *)" bytes",
		(char *)" turns",
		(char *)" bytes",
		(char *)" percent",
	};

	SemaphoreHandle_t gInstrumentMutex;
#endif


/*
	settimeofday, daylightsavingstime
 */
static int32_t gTimeZoneOffset = -8 * 60 * 60;      // Menlo Park
static int16_t gDaylightSavings = 60 * 60;          // summer time

static uint8_t gTimeOfDaySet = 0;
static uint32_t gTimeOfDayOffset = 0;	// seconds to add to gMS to get TOD

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
	modTime_t theTime;
	uint32_t ms;

	ms = modMilliseconds();
	theTime = (ms / 1000) + gTimeOfDayOffset;

	if (tv) {
		tv->tv_sec = theTime;
		tv->tv_usec = (ms % 1000) * 1000;
	}
	if (tz) {
//		tz->tz_minuteswest = gTimeZoneOffset;
//		tz->tz_dsttime = gDaylightSavings;
	}
}

void modSetTime(uint32_t seconds)
{
	uint32_t ms;
	ms = modMilliseconds();

	gTimeOfDayOffset = seconds - (ms / 1000);
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
	return (int32_t)nrf52_memory_remaining();
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

	gInstrumentMutex = xSemaphoreCreateMutex();

#if INSTRUMENT_CPULOAD
	modInstrumentationSetCallback(CPU0, modInstrumentationCPU0);

	nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
	nrf_drv_timer_init(&cpuTimer, &timer_cfg, cpuTimerHandler);
	uint32_t ticks = nrf_drv_timer_us_to_ticks(&cpuTimer, CPUTIMER_US);
	nrf_drv_timer_extended_compare(&cpuTimer, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

	gIdles[0] = xTaskGetIdleTaskHandle();

	nrf_drv_timer_clear(&cpuTimer);
	nrf_drv_timer_enable(&cpuTimer);
#endif
}

extern SemaphoreHandle_t gDebugMutex;
#include "semphr.h"

void espSampleInstrumentation(modTimer timer, void *refcon, int refconSize)
{
	txInteger values[espInstrumentCount];
	int what;
	xsMachine *the = *(xsMachine **)refcon;

	xSemaphoreTake(gInstrumentMutex, portMAX_DELAY);

	for (what = kModInstrumentationPixelsDrawn; what <= (kModInstrumentationSlotHeapSize - 1); what++)
		values[what - kModInstrumentationPixelsDrawn] = modInstrumentationGet_(the, what);

	if (values[kModInstrumentationTurns - kModInstrumentationPixelsDrawn])
		values[kModInstrumentationTurns - kModInstrumentationPixelsDrawn] -= 1;		// ignore the turn that generates instrumentation

	fxSampleInstrumentation(the, espInstrumentCount, values);

	modInstrumentationSet(PixelsDrawn, 0);
	modInstrumentationSet(FramesDrawn, 0);
	modInstrumentationSet(PocoDisplayListUsed, 0);
	modInstrumentationSet(PiuCommandListUsed, 0);
	modInstrumentationSet(Turns, 0);
	modInstrumentMachineReset(the);

	xSemaphoreGive(gInstrumentMutex);
}

#if INSTRUMENT_CPULOAD
static void cpuTimerHandler(nrf_timer_event_t event_type, void* p_context)
{
	switch (event_type) {
		case NRF_TIMER_EVENT_COMPARE0:
			gCPUCounts[0 + (xTaskGetCurrentTaskHandle() == gIdles[0])] += 1;
			gCPUTime += 1250;
			break;
	}
}
#endif
#endif

/*
	messages
*/
#ifndef MODDEF_TASK_QUEUEWAIT
	#ifdef mxDebug
		#define MODDEF_TASK_QUEUEWAIT	(1000)
	#else
		#define MODDEF_TASK_QUEUEWAIT	(portMAX_DELAY)
	#endif
#endif
typedef struct modMessageRecord modMessageRecord;
typedef modMessageRecord *modMessage;

struct modMessageRecord {
	char				*message;
	modMessageDeliver   callback;
	void                *refcon;
	uint16_t            length;
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

#if !mxDebug
	modWatchDogReset();
	if (maxDelayMS >= NRFX_WDT_CONFIG_RELOAD_VALUE) {
		#if NRFX_WDT_CONFIG_RELOAD_VALUE <= 1000
			maxDelayMS = 500;
		#else
			maxDelayMS = NRFX_WDT_CONFIG_RELOAD_VALUE - 1000;
		#endif
	}
#endif

#ifdef mxDebug
	while (true) {
		QueueSetMemberHandle_t queue = xQueueSelectFromSet(the->queues, ((uint64_t)maxDelayMS << 10) / 1000);
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
	while (xQueueReceive(the->msgQueue, &msg, ((uint64_t)maxDelayMS << 10) / 1000)) {
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
	#define MODDEF_TASK_QUEUELENGTH	(10)
#endif

#define kDebugQueueLength (4)

static SemaphoreHandle_t gFlashMutex = NULL;

void modMachineTaskInit(xsMachine *the)
{
	if (NULL == gFlashMutex)
		gFlashMutex = xSemaphoreCreateMutex();

	the->task = (void *)modTaskGetCurrent();
	the->msgQueue = xQueueCreate(MODDEF_TASK_QUEUELENGTH, sizeof(modMessageRecord));
#ifdef mxDebug
	the->dbgQueue = xQueueCreate(kDebugQueueLength, sizeof(modMessageRecord));

	the->queues = xQueueCreateSet(MODDEF_TASK_QUEUELENGTH + kDebugQueueLength);
	xQueueAddToSet(the->msgQueue, the->queues);
	xQueueAddToSet(the->dbgQueue, the->queues);
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

	if (XS_ATOM_ERROR == c_read32be((void *)(4 + kModulesStart)))
		*status = *(8 + (uint8_t *)kModulesStart);
	else
		*status = 0;

	return result;
}

#endif /* MODDEF_XS_MODS */

#ifndef MODDEF_FILE_LFS_PARTITION_SIZE
	#define MODDEF_FILE_LFS_PARTITION_SIZE (65536)
#endif

uint8_t modGetPartition(uint8_t which, uint32_t *offsetOut, uint32_t *sizeOut)
{
	uint32_t offset, size;
	if ((kPartitionMod == which) || (kPartitionStorage == which)) {
		uint32_t modSize = 0, storageSize = 0;

		offset = kModulesStart;

#if MODDEF_XS_MODS
		if (XS_ATOM_ARCHIVE == c_read32be((void *)(4 + offset)))
			modSize = ((c_read32be((void *)(offset)) + kFlashSectorSize - 1) / kFlashSectorSize) * kFlashSectorSize;
#else
		if (which == kPartitionMod)
			return 0;
#endif

		if ((kModulesEnd - (offset + modSize)) >= MODDEF_FILE_LFS_PARTITION_SIZE)
			storageSize = (((MODDEF_FILE_LFS_PARTITION_SIZE + kFlashSectorSize - 1) / kFlashSectorSize) * kFlashSectorSize);

		if (kPartitionStorage == which) {
			offset = kModulesEnd - storageSize;
			size = storageSize;
		}
		else {
//			offset = kModulesStart;		// set above
			size = (kModulesEnd - offset) - MODDEF_FILE_LFS_PARTITION_SIZE;
		}
	}
	else if (kPartitionBLEState == which) {
		offset = (uintptr_t)&_FSTORAGE_start;
		size = &_FSTORAGE_end - &_FSTORAGE_start;
	}
	else
		return 0;

	if (offsetOut) *offsetOut = offset;
	if (sizeOut) *sizeOut = size;

	return 1;
}

/*
	flash
 */

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

#if MY_MALLOC
char synergyDebugStr[256];
void *my_calloc(size_t nitems, size_t size) {
	void *ret;
	ret = calloc(nitems, size);
	if (NULL == ret) {
		sprintf(synergyDebugStr, "# calloc failed %ld\n", size);
	}
	return ret;
}

void *my_realloc(void *ptr, size_t size) {
	void *ret;
	ret = realloc(ptr, size);
	if (NULL == ret) {
		sprintf(synergyDebugStr, "# realloc failed %ld\n", size);
	}
	return ret;
}

void *my_malloc(size_t size) {
	void *ret;
	ret = malloc(size);
	if (NULL == ret) {
		sprintf(synergyDebugStr, "# malloc failed %ld\n", size);
	}
	return ret;
}
#endif

#include "nrf_sdh.h"

uint32_t *dbl_reset_mem = ((uint32_t*)DFU_DBL_RESET_MEM);

void nrf52_reboot(uint32_t kind)
{
	(*dbl_reset_mem) = kind;
#ifdef SOFTDEVICE_PRESENT
	if (nrf_sdh_is_enabled())
		sd_nvic_SystemReset();
	else
		NVIC_SystemReset();
#else
	NVIC_SystemReset();
#endif
}

void nrf52_get_mac(uint8_t *mac) {
	*(uint32_t*)mac = NRF_FICR->DEVICEID[0];
	*(uint16_t*)(mac + 4) = NRF_FICR->DEVICEID[1] & 0xffff;
}

static uint32_t gResetReason;

void nrf52_set_reset_reason(uint32_t resetReason)
{
	gResetReason = resetReason;
}

uint32_t nrf52_get_reset_reason(void)
{
	return gResetReason;
}

static uint32_t gBootLatches[2] = {0};
void nrf52_set_boot_latches(uint32_t bootLatch1, uint32_t bootLatch2)
{
	gBootLatches[0] = bootLatch1;
	gBootLatches[1] = bootLatch2;
}

uint32_t nrf52_get_boot_latch(uint32_t pin)
{
	if (pin < 32)
		return (gBootLatches[0] & (1 << pin));

	pin -= 32;
	return (gBootLatches[1] & (1 << pin));
}

uint32_t *nrf52_get_boot_latches()
{
	return gBootLatches;
}

void nrf52_clear_boot_latch(uint32_t pin)
{
	if (pin < 32)
		gBootLatches[0] &= ~(1 << pin);
	else {
		pin -= 32;
		gBootLatches[1] &= ~(1 << pin);
	}
}


uint8_t nrf52_softdevice_enabled(void)
{
#ifdef SOFTDEVICE_PRESENT
	return nrf_sdh_is_enabled();
#else
	return false;
#endif
}

//---------- alignment for memory

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


modOnSleepRecord *modOnSleepCallbacks = NULL;

void modAddOnSleepCallback(modOnSleepCallback callback, uint32_t refCon)
{
	modOnSleep onSleep = modOnSleepCallbacks;

	while (onSleep) {
		if (onSleep->callback == callback && onSleep->refCon == refCon)
			break;
		onSleep = onSleep->next;
	}

	if (!onSleep) {
		onSleep = c_calloc(1, sizeof(modOnSleepRecord));
		if (!onSleep)
			return;
		onSleep->next = modOnSleepCallbacks;
		modOnSleepCallbacks = onSleep;
	}

	onSleep->callback = callback;
	onSleep->refCon = refCon;
}

void modRemoveOnSleepCallback(modOnSleepCallback callback, uint32_t refCon)
{
	modOnSleep onSleep = modOnSleepCallbacks, last;

	if (!onSleep)
		return;

	if (onSleep->callback == callback && onSleep->refCon == refCon)
		modOnSleepCallbacks = onSleep->next;
	else {
		while (NULL != (last = onSleep)) {	// intentional set of last
			onSleep = onSleep->next;
			if (onSleep && onSleep->callback == callback && onSleep->refCon == refCon) {
				last->next = onSleep->next;
				break;
			}
		}
	}

	if (onSleep)
		c_free(onSleep);
}

void modRunOnSleepCallbacks()
{
	modOnSleep onSleep = modOnSleepCallbacks;

	while (onSleep) {
		onSleep->callback(onSleep->refCon);
		onSleep = onSleep->next;
	}
}


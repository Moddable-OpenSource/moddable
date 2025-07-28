/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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

#include "applib/app_logging.h"
#include "services/common/evented_timer.h"
#include "system/passert.h"

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

	LightMandle_t gInstrumentMutex;
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

void modLog_transmit(const char *msg)
{
  PBL_LOG(LOG_LEVEL_ALWAYS, "%s", msg);
//	APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, "%s", msg);
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

	gInstrumentMutex = xLightMutexCreate();

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

extern LightMutexHandle_t gDebugMutex;

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
	uint8_t				*message;
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
#if 0	// MDK
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
#endif // MDK
}

#ifndef modTaskGetCurrent
	#error make sure MOD_TASKS and modTaskGetCurrent are defined
#endif

#ifndef MODDEF_TASK_QUEUELENGTH
	#define MODDEF_TASK_QUEUELENGTH	(10)
#endif

#define kDebugQueueLength (4)

static LightMutexHandle_t gFlashMutex = NULL;

void modMachineTaskInit(xsMachine *the)
{
	if (NULL == gFlashMutex)
		gFlashMutex = xLightMutexCreate();

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


uint8_t nrf52_softdevice_enabled(void)
{
#ifdef SOFTDEVICE_PRESENT
//	return nrf_sdh_is_enabled();
#else
	return false;
#endif
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
	if (f)
		font = fonts_get_system_font(f->res_key);
	if (!font) {
		font = fonts_get_system_font(FONT_KEY_FONT_FALLBACK);
		if (!font)
			return C_NULL;
	}

	uint16_t height = fonts_get_font_height(font);
	*descent = fonts_get_font_cap_offset(font);
	*leading = 1;
	*ascent = height - *descent - *leading;
	return font;
}

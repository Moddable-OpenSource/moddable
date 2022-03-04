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
	#define MODDEF_XS_MODS
#endif

#ifdef mxInstrument
	#include "modTimer.h"
	#include "modInstrumentation.h"

	static void espInitInstrumentation(txMachine *the);
	static void espSampleInstrumentation(modTimer timer, void *refcon, int refconSize);

	#define espInstrumentCount kModInstrumentationSystemFreeMemory - kModInstrumentationPixelsDrawn + 1
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
		(char *)"Event loop",
		(char *)"System bytes free",
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
		(char *)" turns",
		(char *)" bytes",
	};
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

	ms = nrf52_milliseconds();
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
	ms = nrf52_milliseconds();

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


//#if MODDEF_XS_MODS
	static void *installModules(txPreparation *preparation);
	static char *findNthAtom(uint32_t atomTypeIn, int index, const uint8_t *xsb, int xsbSize, int *atomSizeOut);
	#define findAtom(atomTypeIn, xsb, xsbSize, atomSizeOut) findNthAtom(atomTypeIn, 0, xsb, xsbSize, atomSizeOut);

	static uint8_t gHasMods;
//#endif

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
}

void espSampleInstrumentation(modTimer timer, void *refcon, int refconSize)
{
	txInteger values[espInstrumentCount];
	int what;
	xsMachine *the = *(xsMachine **)refcon;

	for (what = kModInstrumentationPixelsDrawn; what <= kModInstrumentationSystemFreeMemory; what++)
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
	modInstrumentMachineReset(the);
}

#endif

/*
	messages
*/
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
		xQueueSendToFront(the->msgQueue, &msg, portMAX_DELAY);
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
#ifdef mxDebug
	do {
		if (uxQueueSpacesAvailable(the->msgQueue) > 1) {		// keep one entry free for debugger
			xQueueSendToBack(the->msgQueue, &msg, portMAX_DELAY);
			break;
		}
		vTaskDelay(5);
	} while (1);
#else
	xQueueSendToBack(the->msgQueue, &msg, portMAX_DELAY);
#endif
	
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

	xQueueSendToBackFromISR(the->msgQueue, &msg, &ignore);

	return 0;
}

void modMessageService(xsMachine *the, int maxDelayMS)
{
	unsigned portBASE_TYPE count = uxQueueMessagesWaiting(the->msgQueue);

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

	while (true) {
		modMessageRecord msg;

		if (!xQueueReceive(the->msgQueue, &msg, ((uint64_t)maxDelayMS << 10) / 1000)) {
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

#ifndef modTaskGetCurrent
	#error make sure MOD_TASKS and modTaskGetCurrent are defined
#endif

static SemaphoreHandle_t gFlashMutex;

void modMachineTaskInit(xsMachine *the)
{
	the->task = (void *)modTaskGetCurrent();
	the->msgQueue = xQueueCreate(10, sizeof(modMessageRecord));

	if (NULL == gFlashMutex)
		gFlashMutex = xSemaphoreCreateMutex();
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

//#if MODDEF_XS_MODS

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

void *modInstallModules(void *preparationIn, uint8_t *status)
{
	txPreparation *preparation = preparationIn;
	void *result = NULL;

	if (fxMapArchive(preparation, (void *)kModulesStart, (void *)kModulesStart, kFlashSectorSize, spiRead, spiWrite))
		return (void *)kModulesStart;

	if (XS_ATOM_ERROR == c_read32be(4 + kModulesStart)) {
		*status = *(8 + (uint8_t *)kModulesStart);
		modLog("mod failed");
	}
	else
		*status = 0;

	return result;
}

//#endif /* MODDEF_XS_MODS */

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

#define DFU_DBL_RESET_MEM	0x20007F7C		// defined in bootloader
#define DFU_MODDABLE_MAGIC	0xBEEFCAFE		// boot directly to DFU
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

uint8_t nrf52_softdevice_enabled(void)
{
#ifdef SOFTDEVICE_PRESENT
	return nrf_sdh_is_enabled();
#else
	return false;
#endif
}

//---------- alignment for memory
//#define ICACHE_RAM_ATTR

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

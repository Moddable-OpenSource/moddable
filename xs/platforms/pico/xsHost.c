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
#include "mc.defines.h"

#include "xsHosts.h"

#include <stdio.h>

#include "hardware/sync.h"
#include "pico/cyw43_arch.h"

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
		(char *)" bytes"
	};
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
		case 3: return (uint16_t)((*p >> 24) | (p[1] << 8));
		case 2: return (uint16_t) (*p >> 16);
		case 1: return (uint16_t) (*p >>  8);
		case 0: return (uint16_t) (*p);
	}
}

ICACHE_RAM_ATTR uint32_t espRead32(const void *addr)
{
	const uint32_t *p = (const uint32_t *)(~3 & (uint32_t)addr);
	switch (3 & (uint32_t)addr) {
		case 0: return *p;
		case 1: return (p[0] >>  8) | (p[1] << 24);
		case 2: return (p[0] >> 16) | (p[1] << 16);
		case 3: return (p[0] >> 24) | (p[1] <<  8);
	}
}

uint16_t espRead16be(const void *addr)
{
	uint16_t result;
	const uint32_t *p = (const uint32_t *)(~3 & (uint32_t)addr);
	switch (3 & (uint32_t)addr) {
		case 3: result = (uint16_t)((*p >> 24) | (p[1] << 8)); break;
		case 2: result = (uint16_t) (*p >> 16); break;
		case 1: result = (uint16_t) (*p >>  8); break;
		case 0: result = (uint16_t) (*p); break;
	}
	return (result >> 8) | (result << 8);
}

uint32_t espRead32be(const void *addr)
{
	uint32_t result;
	const uint32_t *p = (const uint32_t *)(~3 & (uint32_t)addr);
	switch (3 & (uint32_t)addr) {
		case 0: result = *p; break;
		case 1: result = (p[0] >>  8) | (p[1] << 24); break;
		case 2: result = (p[0] >> 16) | (p[1] << 16); break;
		case 3: result = (p[0] >> 24) | (p[1] <<  8); break;
	}
	return (result << 24) | ((result & 0xff00) << 8)  | ((result >> 8) & 0xff00) | (result >> 24);
}


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

	ms = pico_milliseconds();
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
	ms = pico_milliseconds();

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
	return (int32_t)pico_memory_remaining();
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
	modInstrumentationSetCallback(SystemFreeMemory, (ModInstrumentationGetter)modInstrumentationSystemFreeMemory);

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
	modMessage			next;
	xsMachine			*the;
	modMessageDeliver	callback;
	void				*refcon;
	uint16_t			length;
	uint8_t				marked;
	uint8_t				isStatic;
	char				message[1];
};

static volatile modMessage gMessageQueue = NULL;

// caller is responsible for setting critical section
static void appendMessage(modMessage msg)
{
	msg->next = NULL;
	msg->marked = 0;

	if (NULL == gMessageQueue) {
		gMessageQueue = msg;
	}
	else {
		modMessage walker;

		for (walker = gMessageQueue; NULL != walker->next; walker = walker->next)
			;
		walker->next = msg;
	}

	__sev();		// wake main task
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

	modCriticalSectionBegin();
	appendMessage(msg);
	modCriticalSectionEnd();

	return 0;
}

#define kMessagePoolCount (4)
static modMessageRecord gMessagePool[kMessagePoolCount];

int modMessagePostToMachineFromISR(xsMachine *the, modMessageDeliver callback, void *refcon)
{
	modMessage msg;
	uint8_t i;

	for (i = 0, msg = gMessagePool; i < kMessagePoolCount; i++, msg++) {
		if (!msg->isStatic) {
			msg->isStatic = 1;
			break;
		}
	}

	if ((gMessagePool + kMessagePoolCount) == msg) {
//		modLog("message pool full");
		return -1;
	}

	msg->the = the;	
	msg->callback = callback;
	msg->refcon = refcon;
	msg->length = 0;

	appendMessage(msg);

	return 0;
}

int modMessageService(xsMachine *the, int maxDelayMS)
{
	absolute_time_t until = make_timeout_time_ms(maxDelayMS);

	modCriticalSectionDeclare;
	modCriticalSectionBegin();
	modMessage msg = gMessageQueue;
	while (msg) {
		msg->marked = 1;
		msg = msg->next;
	}
	modCriticalSectionEnd();

	modWatchDogReset();

	msg = gMessageQueue;
	while (msg && msg->marked) {
		modMessage next;

//		if (msg->callback && (!msg->the || (msg->the == the)))
		if (msg->callback)			// like esp
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

#if 1
	// workaround for best_effort_wfe_or_timeout hanging on small/zero/negative timeouts
	while (!gMessageQueue) {
		absolute_time_t now = make_timeout_time_ms(0);
		if (absolute_time_diff_us(until, now) < 1000)
			break;

		if (best_effort_wfe_or_timeout(until))
			break;
		tight_loop_contents();
	}
#else
	while (!gMessageQueue && !best_effort_wfe_or_timeout(until))
		;
#endif

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

void *modInstallMods(void *preparationIn, uint8_t *status)
{
	txPreparation *preparation = preparationIn;
	void *result = NULL;

	if (fxMapArchive(preparation, (void *)kModulesStart, (void *)kModulesStart, kFlashSectorSize, spiRead, spiWrite))
		result = (void *)kModulesStart;

	if (XS_ATOM_ERROR == c_read32be(4 + kModulesStart)) {
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

uint8_t modSPIFlashInit(void)
{
	return 1;
}

uint8_t modSPIRead(uint32_t offset, uint32_t size, uint8_t *dst)
{
	c_memcpy(dst, (void *)(offset + kFlashStart), size);
	return 1;
}

uint8_t modSPIWrite(uint32_t offset, uint32_t size, const uint8_t *src)
{
	uint8_t temp[512] __attribute__ ((aligned (4)));
	uint32_t toAlign;

	if (!modSPIFlashInit()) {
		return 0;
	}

	if (offset & 255) {		// long align offset
		toAlign = 256 - (offset & 255);
		c_memset(temp, 0xFF, 256);
		c_memcpy(temp + 256 - toAlign, src, (size < toAlign) ? size : toAlign);

		uint32_t status = save_and_disable_interrupts();
		flash_range_program(offset & ~255, temp, 256);
		restore_interrupts(status);

		if (size <= toAlign) {
			return 1;
		}

		src += toAlign;
		offset += toAlign;
		size -= toAlign;
	}

	toAlign = size & ~255;
	if (toAlign) {
		size -= toAlign;
		if (255 & (uintptr_t)src) {	// src is not long aligned, copy through stack
			while (toAlign) {
				uint32_t use = (toAlign > sizeof(temp)) ? sizeof(temp) : toAlign;
				c_memcpy(temp, src, use);
				uint32_t status = save_and_disable_interrupts();
				flash_range_program(offset, temp, use);
				restore_interrupts(status);

				toAlign -= use;
				src += use;
				offset += use;
			}
		}
		else {
			uint32_t status = save_and_disable_interrupts();
			flash_range_program(offset, src, toAlign);
			restore_interrupts(status);

			src += toAlign;
			offset += toAlign;
		}
	}

	if (size) {			// long align tail
		c_memset(temp, 0xFF, 256);
		c_memcpy(temp, src, size);
		uint32_t status = save_and_disable_interrupts();
		flash_range_program(offset, temp, 256);
		restore_interrupts(status);
	}

	return 1;
}

uint8_t modSPIErase(uint32_t offset, uint32_t size)
{
	if (!modSPIFlashInit()) {
		return 0;
	}

	if ((offset & (FLASH_SECTOR_SIZE -1)) || (size & (FLASH_SECTOR_SIZE -1))) {
		return 0;
	}

	uint32_t status = save_and_disable_interrupts();
	flash_range_erase(offset, size);
	restore_interrupts(status);
	return 1;
}

// FLASH_PAGE_SIZE	 (1u << 8)
// FLASH_SECTOR_SIZE (1u << 12)
// FLASH_BLOCK_SIZE  (1u << 16)

uint8_t *espFindUnusedFlashStart(void)
{
	uintptr_t modStart;
	extern uint32_t __start_unused_space;

	if (!modSPIFlashInit())
		return NULL;

	modStart = (uintptr_t)&__start_unused_space;
	modStart += FLASH_SECTOR_SIZE - 1;
	modStart -= modStart % FLASH_SECTOR_SIZE;

	/*
		this assumes:
		- the .data. section follows the application image
		- it is no bigger than 4096 bytes
		- empty space follows the .data. section
	*/
//	modStart += 4096;
	modStart += FLASH_SECTOR_SIZE;

	return (uint8_t *)modStart;
}


#include "pico/bootrom.h"
#include "hardware/watchdog.h"

void pico_reboot(uint32_t kind)
{
	if (kind == 1)
		reset_usb_boot(0, 0);
	else
		watchdog_reboot(0, 0, 0);
}

static uint32_t gResetReason;

void pico_set_reset_reason(uint32_t resetReason)
{
	gResetReason = resetReason;
}

void pico_get_mac(uint8_t *id_out)		// 64 bit identifier
{
	flash_get_unique_id(id_out);
}

#if CYW43_LWIP
static uint32_t sCYW43_useCount = 0;
int pico_use_cyw43() {
	int err = 0;
	if (0 == sCYW43_useCount++) {
		err = cyw43_arch_init();
		if (err) {
			sCYW43_useCount--;
			return err;
		}
	}
	return 0;
}

void pico_unuse_cyw43() {
	if (0 == --sCYW43_useCount)
		cyw43_arch_deinit();
}

int pico_cyw43_inited() {
	return (sCYW43_useCount > 0);
}

#endif

/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
#include "xsHost.h"

#include "qurt_error.h"
#include "qapi_rtc.h"
#include "qapi_heap_status.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdio.h>

#ifdef mxInstrument
	#include "modTimer.h"
	#include "modInstrumentation.h"

	static void espInitInstrumentation(txMachine *the);
	static void espSampleInstrumentation(modTimer timer, void *refcon, int refconSize);
	void espInstrumentMachineBegin(txMachine *the, modTimerCallback instrumentationCallback, int count, char **names, char **units);
	void espInstrumentMachineEnd(txMachine *the);
	void espInstrumentMachineReset(txMachine *the);

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
	#if ESP32
		(char *)"SPI flash erases",
	#endif
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
	#if ESP32
		(char *)" sectors",
	#endif
		(char *)" bytes",
	};
#endif

extern void* xsPreparationAndCreation(xsCreation **creation);

int modMessagePostToMachine(xsMachine *the, uint8_t *message, uint16_t messageLength, modMessageDeliver callback, void *refcon);

/*
	settimeofday, daylightsavingstime
 */
static int32_t gTimeZoneOffset = -8 * 60 * 60;      // Menlo Park
static int16_t gDaylightSavings = 60 * 60;          // summer time
static uint8_t gRTCSet = 0;

static modTm gTM;		//@@ eliminate with _r calls

static const uint8_t gDaysInMonth[] ICACHE_XS6RO2_ATTR = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define isLeapYear(YEAR) (!(YEAR % 4) && ((YEAR % 100) || !(YEAR % 400)))

// Get Day of Year
int getDOY(uint16_t year, uint16_t month, uint16_t day) {
    int dayCount[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int dayOfYear = dayCount[month] + day;
    if (month > 1 && isLeapYear(year)) dayOfYear++;
    return dayOfYear;
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
	
	t =	tm->tm_sec
		+ (tm->tm_min * 60)
		+ (tm->tm_hour * 3600)
		+ (tm->tm_yday * 86400)
		+ ((tm->tm_year-70) * 31536000)
		+ (((tm->tm_year-69)/4) * 86400)
		- (((tm->tm_year-1)/100) * 86400)
		+ (((tm->tm_year+299)/400)*86400)
		- (gTimeZoneOffset + gDaylightSavings);

	return t;
}

void modGetTimeOfDay(struct modTimeVal *tv, struct modTimeZone *tz)
{
	qapi_Time_t tm;
	modTime_t theTime;
	modTm mt;

	if (!gRTCSet) {
		modSetTime(0);
		gRTCSet = 1;
	}
	qapi_Core_RTC_Get(&tm);

	mt.tm_sec = tm.second;
	mt.tm_min = tm.minute;
	mt.tm_hour = tm.hour;
	mt.tm_year = tm.year - 1900;
	mt.tm_mon = tm.month - 1;
	mt.tm_mday = tm.day - 1;
	mt.tm_yday = getDOY(tm.year, tm.month - 1, tm.day - 1);
	mt.tm_isdst = 0;
	theTime = modMkTime(&mt);

	if (tv) {
		tv->tv_sec = theTime;
		tv->tv_usec = 0;
	}
	if (tz) {
		tz->tz_minuteswest = gTimeZoneOffset;
		tz->tz_dsttime = gDaylightSavings;
	}
}

void modSetTime(uint32_t seconds)
{
	struct modTm *modT;
	qapi_Time_t qT;
	modTime_t t;

	t = seconds;
	modT = modGmTime(&t);
	qT.year = modT->tm_year + 1900;
	qT.month = modT->tm_mon + 1;
	qT.day = modT->tm_mday;
	qT.hour = modT->tm_hour;
	qT.minute = modT->tm_min;
	qT.second = modT->tm_sec;
	qT.day_Of_Week = (0 == modT->tm_wday ? 6 : modT->tm_wday - 1);
	qapi_Core_RTC_Set(&qT);
	gRTCSet = 1;
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
	xsMachine *result;
	uint8_t *context[2];
	xsCreation *creationP;
	void *preparation = xsPreparationAndCreation(&creationP);
	void *archive;

#if MODDEF_XS_MODS
	archive = installModules(preparation);
	gHasMods = NULL != archive;
#else
	archive = NULL;
#endif

	if (0 == allocation)
		allocation = creationP->staticSize;

	if (allocation) {
		xsCreation creation = *creationP;

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

		result = xsPrepareMachine(&creation, preparation, name ? (txString)name : "main", context, archive);
		if (NULL == result) {
			if (context[0])
				c_free(context[0]);
			return NULL;
		}
	}
	else {
		result = xsPrepareMachine(NULL, preparation, "main", NULL, archive);
		if (NULL == result)
			return NULL;
	}

	xsSetContext(result, NULL);

	return result;
}

static uint16_t gSetupPending = 0;

void modLoadModule(void *theIn, const char *name)
{
	xsMachine *the = theIn;

	xsBeginHost(the);
		xsResult = xsAwaitImport(name, XS_IMPORT_DEFAULT);
		if (xsTest(xsResult) && xsIsInstanceOf(xsResult, xsFunctionPrototype))
			xsCallFunction0(xsResult, xsGlobal);
	xsEndHost(the);
}

void setStepDone(xsMachine *the)
{
	gSetupPending -= 1;
	if (gSetupPending)
		return;

	modLoadModule(the, "main");
}

void mc_setup(xsMachine *the)
{
	txPreparation *preparation = xsPreparationAndCreation(NULL);
	txInteger scriptCount = preparation->scriptCount;
	txScript* script = preparation->scripts;

#ifdef mxInstrument
	espInitInstrumentation(the);
	espInstrumentMachineBegin(the, espSampleInstrumentation, espInstrumentCount, espInstrumentNames, espInstrumentUnits);
#endif

	gSetupPending = 1;

	xsBeginHost(the);
		xsVars(2);
		xsVar(0) = xsNewHostFunction(setStepDone, 0);

		while (scriptCount--) {
			if (0 == c_strncmp(script->path, "setup/", 6)) {
				char path[PATH_MAX];
				char *dot;

				c_strcpy(path, script->path);
				dot = c_strchr(path, '.');
				if (dot)
					*dot = 0;

				xsResult = xsAwaitImport(path, XS_IMPORT_DEFAULT);
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

	fxReport(the, "!!! xs: failed to allocate %d bytes for chunk !!!\n", size);
	xsDebugger();
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

	fxReport(the, "!!! xs: failed to allocate %d bytes for slots !!!\n", size);
	xsDebugger();
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
		fxCollect(the, XS_COMPACT_FLAG | XS_ORGANIC_FLAG);	/* expecting memory from the chunk pool */
		if (the->firstBlock != C_NULL && the->firstBlock->limit == mc_xs_chunk_allocator(the, 0)) {	/* sanity check just in case */
			fxReport(the, "# Slot allocation: %d bytes returned\n", the->firstBlock->limit - the->firstBlock->current);
			the->maximumChunksSize -= the->firstBlock->limit - the->firstBlock->current;
			the->heap_ptr = the->firstBlock->current;
			the->firstBlock->limit = the->firstBlock->current;
		}
		result = (txSlot *)mc_xs_slot_allocator(the, theCount * sizeof(txSlot));
	}

	if (!result) {
		fxReport(the, "# can't make memory for slots\n");
		xsDebugger();
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

static txBoolean fxFindScript(txMachine* the, txSlot* realm, txString path, txID* id)
{
	txPreparation* preparation = the->preparation;
	txInteger c = preparation->scriptCount;
	txScript* script = preparation->scripts;
#if MODDEF_XS_MODS
	uint8_t *mod;
	int modSize;

	mod = findMod(the, path + preparation->baseLength, &modSize);
	if (mod) {
		*id = fxNewNameC(the, path);
		return 1;
	}
#endif
	while (c > 0) {
		if (!c_strcmp(path + preparation->baseLength, script->path)) {
			*id = fxNewNameC(the, path);
			return 1;
		}
		c--;
		script++;
	}
	return 0;
}

txID fxFindModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* slot)
{
	txPreparation* preparation = the->preparation;
	char name[PATH_MAX];
	char path[PATH_MAX];
	txBoolean absolute = 0, relative = 0, search = 0;
	txInteger dot = 0;
	txString slash;
	txID id;

	fxToStringBuffer(the, slot, name, sizeof(name));
// #if MODDEF_XS_MODS
// 	if (findMod(the, name, NULL)) {
// 		c_strcpy(path, "/");
// 		c_strcat(path, name);
// 		c_strcat(path, ".xsb");
// 		return fxNewNameC(the, path);
// 	}
// #endif
// 
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
		if (fxFindScript(the, realm, path, &id))
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
			if (fxFindScript(the, realm, path, &id))
				return id;
		}
#if 0
		*slash = 0;
		c_strcat(path, name + dot);
		if (!c_strncmp(path, "xsbug://", 8)) {
			return fxNewNameC(the, path);
		}
#endif
	}
	if (search) {
		c_strcpy(path, preparation->base);
		c_strcat(path, name);
		c_strcat(path, ".xsb");
		if (fxFindScript(the, realm, path, &id)) {
// 			fxReport(the, "SEARCH %s\n", path);
			return id;
		}
	}
	return XS_NO_ID;
}

void fxLoadModule(txMachine* the, txSlot* realm, txID moduleID)
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

		fxResolveModule(the, realm, moduleID, &aScript, C_NULL, C_NULL);
		return;
	}
#endif

	while (c > 0) {
		if (!c_strcmp(path, script->path)) {
			fxResolveModule(the, realm, moduleID, script, C_NULL, C_NULL);
			return;
		}
		c--;
		script++;
	}
	
#if 0
	path -= preparation->baseLength;
	if (!c_strncmp(path, "xsbug://", 8))
		fxDebugImport(the, path);
#endif
}

txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags)
{
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	txScript* script = NULL;
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

/*
	Instrumentation
*/

#ifdef mxInstrument

static int32_t modInstrumentationSystemFreeMemory(void *theIn)
{
	txMachine *the = theIn;
	return (int32_t)xPortGetFreeHeapSize();
}

static int32_t modInstrumentationSlotHeapSize(void *theIn)
{
	txMachine *the = theIn;
	return the->currentHeapCount * sizeof(txSlot);
}

static int32_t modInstrumentationChunkHeapSize(void *theIn)
{
	txMachine *the = theIn;
	return the->currentChunksSize;
}

static int32_t modInstrumentationKeysUsed(void *theIn)
{
	txMachine *the = theIn;
	return the->keyIndex - the->keyOffset;
}

static int32_t modInstrumentationGarbageCollectionCount(void *theIn)
{
	txMachine *the = theIn;
	return the->garbageCollectionCount;
}

static int32_t modInstrumentationModulesLoaded(void *theIn)
{
	txMachine *the = theIn;
	return the->loadedModulesCount;
}

static int32_t modInstrumentationStackRemain(void *theIn)
{
	txMachine *the = theIn;
	if (the->stackPeak > the->stack)
		the->stackPeak = the->stack;
	return (the->stackTop - the->stackPeak) * sizeof(txSlot);
}

#ifdef mxDebug
void espDebugBreak(txMachine* the, uint8_t stop)
{
	if (stop) {
		the->DEBUG_LOOP = 1;
		fxCollectGarbage(the);
		modInstrumentationAdjust(GarbageCollectionCount, -1);
		((modTimerCallback)the->instrumentationCallback)(NULL, &the, sizeof(the));
	}
	else {
		the->DEBUG_LOOP = 0;
		modTimerReschedule(the->instrumentationTimer, 1000, 1000);
	}
}
#endif

void espInitInstrumentation(txMachine *the)
{
	modInstrumentationInit();
	modInstrumentationSetCallback(SystemFreeMemory, modInstrumentationSystemFreeMemory);

	modInstrumentationSetCallback(SlotHeapSize, modInstrumentationSlotHeapSize);
	modInstrumentationSetCallback(ChunkHeapSize, modInstrumentationChunkHeapSize);
	modInstrumentationSetCallback(KeysUsed, modInstrumentationKeysUsed);
	modInstrumentationSetCallback(GarbageCollectionCount, modInstrumentationGarbageCollectionCount);
	modInstrumentationSetCallback(ModulesLoaded, modInstrumentationModulesLoaded);
	modInstrumentationSetCallback(StackRemain, modInstrumentationStackRemain);
}

void espSampleInstrumentation(modTimer timer, void *refcon, int refconSize)
{
	txInteger values[espInstrumentCount];
	int what;
	xsMachine *the = *(xsMachine **)refcon;

	for (what = kModInstrumentationPixelsDrawn; what <= kModInstrumentationSystemFreeMemory; what++)
		values[what - kModInstrumentationPixelsDrawn] = modInstrumentationGet_(the, what);

	fxSampleInstrumentation(the, espInstrumentCount, values);

	modInstrumentationSet(PixelsDrawn, 0);
	modInstrumentationSet(FramesDrawn, 0);
	modInstrumentationSet(PocoDisplayListUsed, 0);
	modInstrumentationSet(PiuCommandListUsed, 0);
	modInstrumentationSet(NetworkBytesRead, 0);
	modInstrumentationSet(NetworkBytesWritten, 0);
#if ESP32
	modInstrumentationSet(SPIFlashErases, 0);
#endif
	espInstrumentMachineReset(the);
}

void espInstrumentMachineBegin(txMachine *the, modTimerCallback instrumentationCallback, int count, char **names, char **units)
{
	the->instrumentationCallback = instrumentationCallback;
	the->instrumentationTimer = modTimerAdd(0, 1000, instrumentationCallback, &the, sizeof(the));
	modInstrumentationAdjust(Timers, -1);

#ifdef mxDebug
	the->onBreak = espDebugBreak;
#endif
	fxDescribeInstrumentation(the, count, names, units);
}

void espInstrumentMachineEnd(txMachine *the)
{
	if (!the->instrumentationTimer)
		return;

	modInstrumentationAdjust(Timers, +1);
	modTimerRemove(the->instrumentationTimer);
}

void espInstrumentMachineReset(txMachine *the)
{
	the->garbageCollectionCount = 0;
	the->stackPeak = the->stack;
	the->peakParserSize = 0;
	the->floatingPointOps = 0;
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
		if (uxQueueSpacesAvailable(the->msgQueue) > 1) {			// keep one entry free for debugger
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

	xQueueSendFromISR(the->msgQueue, &msg, &ignore);

	return 0;
}

void modMessageService(xsMachine *the, int maxDelayMS)
{
	unsigned portBASE_TYPE count = uxQueueMessagesWaiting(the->msgQueue);

	qca4020_watchdog();

	while (true) {
		modMessageRecord msg;

		if (!xQueueReceive(the->msgQueue, &msg, maxDelayMS)) {
			qca4020_watchdog();
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

void modMachineTaskInit(xsMachine *the)
{
	the->task = (void *)modTaskGetCurrent();
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


static void doRunPromiseJobs(void *machine, void *refcon, uint8_t *message, uint16_t messageLength)
{
	fxRunPromiseJobs((txMachine *)machine);
}

void fxQueuePromiseJobs(txMachine* the)
{
	modMessagePostToMachine(the, NULL, 0, doRunPromiseJobs, NULL);
}



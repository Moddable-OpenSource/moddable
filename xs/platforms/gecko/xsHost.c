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
#include "xsScript.h"
#include "xsHost.h"

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

extern uint32_t gDeviceUnique;
int modMessagePostToMachine(xsMachine *the, uint8_t *message, uint16_t messageLength, modMessageDeliver callback, void *refcon);

/*
	settimeofday, daylightsavingstime
 */
static int32_t gTimeZoneOffset = -8 * 60 * 60;      // Menlo Park
static int16_t gDaylightSavings = 60 * 60;          // summer time

void modSetTime(uint32_t seconds)
{
#if 0
    struct timeval tv;
//  struct timezone tz;

    tv.tv_sec = seconds;
    tv.tv_usec = 0;

//  c_settimeofday(&tv, NULL);
//          //// NEED TO IMPLEMENT SETTIMEOFDAY
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
#endif

void *ESP_cloneMachine(uint32_t allocation, uint32_t stackCount, uint32_t slotCount, const char *name)
{
	xsMachine *result;
	uint8_t *context[2];
	xsCreation *creationP;
	void *preparation = xsPreparationAndCreation(&creationP);
	void *archive;

#if MODDEF_XS_MODS
	archive = installModules(prep);
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

void setStepDone(xsMachine *the)
{
	gSetupPending -= 1;
	if (gSetupPending)
		return;

	xsBeginHost(the);
		xsResult = xsAwaitImport("main", XS_IMPORT_DEFAULT);
		if (xsTest(xsResult) && xsIsInstanceOf(xsResult, xsFunctionPrototype))
			xsCallFunction0(xsResult, xsGlobal);
	xsEndHost(the);
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
				char path[128];
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
		fxCollect(the, XS_COMPACT_FLAG | XS_ORGANIC_FLAG);	/* expecting memory from the chunk pool */
		if (the->firstBlock != C_NULL && the->firstBlock->limit == mc_xs_chunk_allocator(the, 0)) {	/* sanity check just in case */
			fxReport(the, "# Slot allocation: %d bytes returned\n", the->firstBlock->limit - the->firstBlock->current);
			the->maximumChunksSize -= the->firstBlock->limit - the->firstBlock->current;
			the->heap_ptr = (uint8_t*)the->firstBlock->current;
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

	if (!xsb) return NULL;

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

txID fxFindModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* slot)
{
	txPreparation* preparation = the->preparation;
	char name[128];
	char path[128];
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
			fxResolveModule(the,realm,  moduleID, script, C_NULL, C_NULL);
			return;
		}
		c--;
		script++;
	}
}

txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags)
{
	return C_NULL;
}

static void doRunPromiseJobs(void *machine, void *refcon, uint8_t *message, uint16_t messageLength)
{
	fxRunPromiseJobs((txMachine *)machine);
}

void fxQueuePromiseJobs(txMachine* the)
{
	modMessagePostToMachine(the, NULL, 0, doRunPromiseJobs, NULL);
}

#if 0
typedef uint8_t (*RunPromiseJobs)(xsMachine *the);

static uint8_t xsRunPromiseJobs_pending(txMachine *the);
static RunPromiseJobs gRunPromiseJobs;

uint8_t xsRunPromiseJobs_pending(txMachine *the)
{
	gRunPromiseJobs = NULL;
	if (!mxPendingJobs.value.reference->next)
		return 0;

	fxRunPromiseJobs(the);

	if (0 == mxPendingJobs.value.reference->next)
		return 0;

	gRunPromiseJobs = xsRunPromiseJobs_pending;
	return 1;
}

uint8_t modRunPromiseJobs(txMachine *the)
{
	return gRunPromiseJobs ? gRunPromiseJobs(the) : 0;
}

void fxQueuePromiseJobs(txMachine* the)
{
	gRunPromiseJobs = xsRunPromiseJobs_pending;
}

#endif

/*
	Instrumentation
*/

#ifdef mxInstrument

static int32_t modInstrumentationSystemFreeMemory(void *theIn)
{
	txMachine *the = theIn;
//	return (int32_t)xPortGetFreeHeapSize();
	return (int32_t)0;
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
	modMessage          next;
	xsMachine           *the;
	modMessageDeliver   callback;
	void                *refcon;
	uint16_t            length;
	uint8_t             marked;
	uint8_t				isStatic;		// this doubles as a flag to indicate entry is use gMessagePool
	uint8_t				message[1];
};

static modMessage gMessageQueue;

static void appendMessage(modMessage msg)
{
	modCriticalSectionDeclare;
	msg->next = NULL;
	msg->marked = 0;

	modCriticalSectionBegin();
	if (NULL == gMessageQueue) {
		gMessageQueue = msg;
		gecko_schedule();
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
	modCriticalSectionDeclare;
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
	modCriticalSectionDeclare;
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




#if MY_MALLOC
char _debugStrBuffer[256];
void *my_calloc(size_t nitems, size_t size) {
	void *ret;
	ret = calloc(nitems, size);
	if (NULL == ret) {
		sprintf(_debugStrBuffer, "# calloc failed %ld\n", size);
	}
	return ret;
}

void *my_realloc(void *ptr, size_t size) {
	void *ret;
	ret = realloc(ptr, size);
	if (NULL == ret) {
		sprintf(_debugStrBuffer, "# realloc failed %ld\n", size);
	}
	return ret;
}

void *my_malloc(size_t size) {
	void *ret;
	ret = malloc(size);
	if (NULL == ret) {
		sprintf(_debugStrBuffer, "# malloc failed %ld\n", size);
	}
	return ret;
}
#endif




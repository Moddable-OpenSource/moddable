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
#include "mc.defines.h"

#include "xsScript.h"
#include "xsHosts.h"
#include "xsHost.h"

#ifdef mxInstrument
	#include "modInstrumentation.h"
#endif

extern void *xsPreparationAndCreation(xsCreation **creation);

#if MODDEF_XS_MODS
	static uint8_t *findMod(xsMachine *the, char *name, int *modSize);
#endif

/*
	XS memory 
*/

static void *mc_xs_slot_allocator(txMachine* the, size_t size)
{
	if (the->heap_pend - size < the->heap_ptr)
		return NULL;

	void *ptr = the->heap_pend - size;
	the->heap_pend -= size;
	return ptr;
}

void *fxAllocateChunks(txMachine* the, txSize theSize)
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

	txBlock* block = the->firstBlock;
	if (block) {	// reduce size by number of free bytes in current chunk heap
		txSize grow = theSize - sizeof(txBlock) - (block->limit - block->current);
		if (the->heap_ptr + grow <= the->heap_pend) {
			the->heap_ptr += grow;
			block->limit = (txByte*)the->heap_ptr - theSize; // fxGrowChunks adds theSize
			the->maximumChunksSize += grow - (theSize - sizeof(txBlock));		 // fxGrowChunks adds theSize - sizeof(txBlocK)
			return block->limit;
		}
	}
	else if (the->heap_ptr + theSize <= the->heap_pend) {
		void *ptr = the->heap_ptr;
		the->heap_ptr += theSize;
		return ptr;
	}

	return NULL;
}

txSlot *fxAllocateSlots(txMachine* the, txSize theCount)
{
	txSize needed = theCount * sizeof(txSlot);

	if (NULL == the->heap) {
#ifndef modGetLargestMalloc
		return c_malloc(needed);
#else
		extern void fxGrowSlots(txMachine* the, txSize theCount); 
		static uint8_t *pending;		//@@ not thread safe...

		if (NULL == the->stack)
			return c_malloc(needed);

		if (pending) {
			txSlot *result = (txSlot *)pending;
			pending = NULL;
			return result;
		}

		while (needed) {
			size_t largest = modGetLargestMalloc() & ~0x0F;
			if (largest > needed)
				largest = needed;
			pending = c_malloc(largest);
			if (!pending) 
				return NULL;

			fxGrowSlots(the, largest / sizeof(txSlot));

#ifdef mxDebug
			if (pending)
				fxAbort(the, XS_FATAL_CHECK_EXIT);
#endif

			needed -= largest;
		}

		return (void *)-1;
#endif
	}

	txSlot *result = (txSlot *)mc_xs_slot_allocator(the, needed);
	if (!result && the->firstBlock) {
#ifdef mxDebug
		fxReport(the, "# Slot allocation: failed. trying to make room...\n");
#endif
		fxCollect(the, XS_COMPACT_FLAG | XS_ORGANIC_FLAG);	/* expecting memory from the chunk pool */
#ifdef mxDebug
		fxReport(the, "# Slot allocation: %d bytes returned\n", the->firstBlock->limit - the->firstBlock->current);
#endif
		the->maximumChunksSize -= the->firstBlock->limit - the->firstBlock->current;
		the->heap_ptr = the->firstBlock->current;
		the->firstBlock->limit = the->firstBlock->current;

		result = (txSlot *)mc_xs_slot_allocator(the, needed);
	}

	return result;
}

void fxFreeChunks(txMachine* the, void* theChunks)
{
	if (NULL == the->heap)
		c_free(theChunks);
	else {
		/* @@ too lazy but it should work... */
		if ((uint8_t *)theChunks < the->heap_ptr)
			the->heap_ptr = theChunks;

		if (the->heap_ptr == the->heap) {
			if (the->context) {
				uint8_t **context = the->context;
				context[0] = NULL;
			}
			c_free(the->heap);		// VM is terminated
		}
	}
}

void fxFreeSlots(txMachine* the, void* theSlots)
{
	if (NULL == the->heap)
		c_free(theSlots);
	else
		; /* nothing to do */
}

void fxBuildKeys(txMachine* the)
{
}

/*
	xs modules
*/

static txBoolean fxFindScript(txMachine* the, txSlot* realm, txString path, txID* id)
{
	txPreparation* preparation = the->preparation;
	txInteger c = preparation->scriptCount;
	txScript* script = preparation->scripts;
#if MODDEF_XS_MODS
	uint8_t *mod;
	int modSize;

	mod = findMod(the, path, &modSize);
	if (mod) {
		*id = fxNewNameC(the, path);
		return 1;
	}
#endif
	while (c > 0) {
		if (!c_strcmp(path, script->path)) {
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
#if MODDEF_XS_TEST
	char extension[5] = "";
#endif
	char name[PATH_MAX];
	char buffer[PATH_MAX];
	txInteger dot = 0;
	txString slash;
	txString path;
	txID id;
	fxToStringBuffer(the, slot, name, sizeof(name));
	if (name[0] == '.') {
		if (name[1] == '/') {
			dot = 1;
		}
		else if ((name[1] == '.') && (name[2] == '/')) {
			dot = 2;
		}
	}
	slash = c_strrchr(name, '/');
	if (!slash)
		slash = name;
	slash = c_strrchr(slash, '.');
	if (slash && (!c_strcmp(slash, ".js") || !c_strcmp(slash, ".mjs"))) {
#if MODDEF_XS_TEST
		c_strcpy(extension, slash);
#endif
		*slash = 0;
	}
	if (dot) {
		if (moduleID == XS_NO_ID)
			return XS_NO_ID;
		buffer[0] = '/';
		path = buffer + 1;
		c_strcpy(path, fxGetKeyName(the, moduleID));
		slash = c_strrchr(buffer, '/');
		if (!slash)
			return XS_NO_ID;
		if (dot == 2) {
			*slash = 0;
			slash = c_strrchr(buffer, '/');
			if (!slash)
				return XS_NO_ID;
		}
		*slash = 0;
		c_strcat(buffer, name + dot);
	}
	else
		path = name;
	if (fxFindScript(the, realm, path, &id))
		return id;
#if MODDEF_XS_TEST
	if (!c_strncmp(path, "xsbug://", 8)) {
		c_strcat(path, extension);
		return fxNewNameC(the, path);
	}
#endif
	return XS_NO_ID;
}

void fxLoadModule(txMachine* the, txSlot* module, txID moduleID)
{
	txPreparation* preparation = the->preparation;
	txString path = fxGetKeyName(the, moduleID);
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
		aScript.path = path;
		aScript.version[0] = XS_MAJOR_VERSION;
		aScript.version[1] = XS_MINOR_VERSION;
		aScript.version[2] = XS_PATCH_VERSION;
		aScript.version[3] = 0;

		fxResolveModule(the, module, moduleID, &aScript, C_NULL, C_NULL);
		return;
	}
#endif

	while (c > 0) {
		if (!c_strcmp(path, script->path)) {
			fxResolveModule(the, module, moduleID, script, C_NULL, C_NULL);
			return;
		}
		c--;
		script++;
	}
	
#if MODDEF_XS_TEST
	if (!c_strncmp(path, "xsbug://", 8))
		fxDebugImport(the, module, path);
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
#if defined(mxDebug) && !MODDEF_XS_TEST
		if (fxIsConnected(the)) {
			char tag[16];
			flags |= mxDebugFlag;
			fxGenerateTag(the, tag, sizeof(tag), C_NULL);
			fxFileEvalString(the, ((txStringStream*)stream)->slot->value.string, tag);
			parser->path = fxNewParserSymbol(parser, tag);
		}
#endif
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

#if MODDEF_XS_MODS

#define FOURCC(c1, c2, c3, c4) (((c1) << 24) | ((c2) << 16) | ((c3) << 8) | (c4))

static char *findNthAtom(uint32_t atomTypeIn, int index, const uint8_t *xsb, int xsbSize, int *atomSizeOut);
#define findAtom(atomTypeIn, xsb, xsbSize, atomSizeOut) findNthAtom(atomTypeIn, 0, xsb, xsbSize, atomSizeOut);

static uint8_t *findMod(txMachine *the, char *name, int *modSize)
{
	uint8_t *xsb = (uint8_t *)kModulesStart;
	int modsSize;
	uint8_t *mods;
	int index = 0;
	int nameLen;
	char *dot;

	if (!xsb || !the->archive) return NULL;

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
			return findNthAtom(FOURCC('C', 'O', 'D', 'E'), index, mods, modsSize, modSize);
		}
	}

	return NULL;
}

char *modGetModAtom(txMachine *the, uint32_t atomTypeIn, int *atomSizeOut)
{
	uint8_t *xsb = (uint8_t *)kModulesStart;
	if (!xsb || !the->archive) return NULL;

	return findNthAtom(atomTypeIn, 0, xsb, c_read32be(xsb), atomSizeOut);
}

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

#endif


/*
	create VM
*/

txMachine *modCloneMachine(uint32_t allocation, uint32_t stackCount, uint32_t slotCount, uint32_t keyCount, const char *name)
{
	txMachine *the;
	xsCreation *creationP;
	void *preparation = xsPreparationAndCreation(&creationP);

#if MODDEF_XS_MODS
	uint8_t modStatus = 0;
	void *archive;
	archive = modInstallMods(preparation, &modStatus);
#else
	#define archive (NULL)
#endif

	if (!name)
		name = ((txPreparation *)preparation)->main;

	if (0 == allocation)
		allocation = creationP->staticSize;

	if (allocation) {
		xsCreation creation = *creationP;
		uint8_t *context[2];

		if (stackCount)
			creation.stackCount = stackCount;

		if (slotCount)
			creation.initialHeapCount = slotCount;
		
		if (keyCount)
			creation.initialKeyCount = keyCount;

		context[0] = c_malloc(allocation);
		if (NULL == context[0]) {
			modLog("failed to allocate xs block");
			return NULL;
		}
		context[1] = context[0] + allocation;

		the = xsPrepareMachine(&creation, preparation, (char *)name, context, archive);
		if (NULL == the) {
			if (context[0])
				c_free(context[0]);
			return NULL;
		}

		xsSetContext(the, NULL);
	}
	else {
		the = xsPrepareMachine(NULL, preparation, (char *)name, NULL, archive);
		if (NULL == the)
			return NULL;
	}

#if MODDEF_XS_MODS
	if (modStatus)
		xsLog("Mod failed: %s\n", gXSAbortStrings[modStatus]);
#endif

	return the;
}

static uint16_t gSetupPending = 0;

#if MODDEF_MAIN_ASYNC
static void setStepDoneFulfilled(xsMachine *the)
{
	xsResult = xsGet(xsArg(0), xsID("default"));
		if (xsTest(xsResult) && xsIsInstanceOf(xsResult, xsFunctionPrototype))
			xsCallFunction0(xsResult, xsGlobal);
}

static void setStepDoneRejected(xsMachine *the)
{
}
#endif

static void setStepDone(txMachine *the)
{
	gSetupPending -= 1;
	if (gSetupPending)
		return;

	xsBeginHost(the);
#if MODDEF_MAIN_ASYNC
		xsVars(2);
		xsResult = xsAwaitImport(((txPreparation *)xsPreparationAndCreation(NULL))->main, XS_IMPORT_ASYNC);
		xsVar(0) = xsNewHostFunction(setStepDoneFulfilled, 1);
		xsVar(1) = xsNewHostFunction(setStepDoneRejected, 1);
		xsCall2(xsResult, xsID("then"), xsVar(0), xsVar(1));
#else	
		xsResult = xsAwaitImport(((txPreparation *)xsPreparationAndCreation(NULL))->main, XS_IMPORT_DEFAULT);
		if (xsTest(xsResult) && xsIsInstanceOf(xsResult, xsFunctionPrototype))
			xsCallFunction0(xsResult, xsGlobal);
#endif
	xsEndHost(the);
}

void modRunMachineSetup(txMachine *the)
{
	txPreparation *preparation = xsPreparationAndCreation(NULL);
	txInteger scriptCount = preparation->scriptCount;
	txScript* script = preparation->scripts;

#ifdef mxInstrument
	modInstrumentationSetup(the);
#endif

	gSetupPending = 1;

	xsBeginHost(the);
		xsVars(1);
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

#ifdef mxInstrument
#ifdef mxDebug

void modDebugBreak(xsMachine* the, uint8_t stop)
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

void modInstrumentMachineBegin(xsMachine *the, modTimerCallback instrumentationCallback, int count, char **names, char **units)
{
	the->instrumentationCallback = instrumentationCallback;
	the->instrumentationTimer = modTimerAdd(0, 1000, instrumentationCallback, &the, sizeof(the));
	modInstrumentationAdjust(Timers, -1);

#ifdef mxDebug
	the->onBreak = modDebugBreak;
#endif
	fxDescribeInstrumentation(the, count, names, units);
}

void modInstrumentMachineEnd(xsMachine *the)
{
	if (!the->instrumentationTimer)
		return;

	modInstrumentationAdjust(Timers, +1);
	modTimerRemove(the->instrumentationTimer);
	the->instrumentationTimer = NULL;
}

void modInstrumentMachineReset(xsMachine *the)
{
	the->garbageCollectionCount = 0;
	the->stackPeak = the->stack;
	the->peakParserSize = 0;
	the->floatingPointOps = 0;
}

int32_t modInstrumentationSlotHeapSize(xsMachine *the)
{
	return the->currentHeapCount * sizeof(txSlot);
}

int32_t modInstrumentationChunkHeapSize(xsMachine *the)
{
	return the->currentChunksSize;
}

int32_t modInstrumentationKeysUsed(xsMachine *the)
{
	return the->keyIndex - the->keyOffset;
}

int32_t modInstrumentationGarbageCollectionCount(xsMachine *the)
{
	return the->garbageCollectionCount;
}

int32_t modInstrumentationModulesLoaded(xsMachine *the)
{
	return the->loadedModulesCount;
}

int32_t modInstrumentationStackRemain(xsMachine *the)
{
	if (the->stackPeak > the->stack)
		the->stackPeak = the->stack;
	return (the->stackTop - the->stackPeak) * sizeof(txSlot);
}

#endif

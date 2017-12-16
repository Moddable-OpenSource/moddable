/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 *
 *     Portions Copyright (C) 2010-2015 Marvell International Ltd.
 *     Portions Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Portions Copyright by Marvell International Ltd. and Kinoma, Inc. are 
 *     derived from KinomaJS/XS6 and used under the Apache 2.0 License.
 */

#include "xsAll.h"
#include "xs.h"
#include "xsScript.h"
#include "xsgecko.h"


static txBoolean fxFindScript(txMachine *the, txString path, txID* id);

#ifdef mxInstrument
	#include "modInstrumentation.h"
	static void espStartInstrumentation(txMachine *the);
#endif


static txBoolean fxFindScript(txMachine *the, txString path, txID *id)
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


txID fxFindModule(txMachine* the, txID moduleID, txSlot* slot)
{
	txPreparation* preparation = the->preparation;
	char name[128];
	char path[128];
	txBoolean absolute = 0, relative = 0, search = 0;
	txInteger dot = 0;
	txString slash;
	txID id;

	fxToStringBuffer(the, slot, name, sizeof(name));
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
//	extern void modTimersMark(txMachine *the, txMarkRoot markRoot);
	the->host = C_NULL;
//	modTimersMark(the, markRoot);
}

txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags)
{
	return C_NULL;
}

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
	// queued jobs serviced from the main loop
	gRunPromiseJobs = xsRunPromiseJobs_pending;
}

void fxSweepHost(txMachine* the)
{
}

/*
	Instrumentation
*/

#ifdef mxInstrument

static void espSampleInstrumentation(modTimer timer, void *refcon, uint32_t refconSize);

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
    
extern txMachine *gThe;

static int32_t modInstrumentationSystemFreeMemory(void)
{
//	return (int32_t)xPortGetFreeHeapSize();
	return (int32_t)0;
}

static int32_t modInstrumentationSlotHeapSize(void)
{
	return gThe->currentHeapCount * sizeof(txSlot);
}

static int32_t modInstrumentationChunkHeapSize(void)
{
	return gThe->currentChunksSize;
}

static int32_t modInstrumentationKeysUsed(void)
{
	return gThe->keyIndex - gThe->keyOffset;
}

static int32_t modInstrumentationGarbageCollectionCount(void)
{
	return gThe->garbageCollectionCount;
}

static int32_t modInstrumentationModulesLoaded(void)
{
	return gThe->loadedModulesCount;
}

static int32_t modInstrumentationStackRemain(void)
{
	if (gThe->stackPeak > gThe->stack)
		gThe->stackPeak = gThe->stack;
	return (gThe->stackTop - gThe->stackPeak) * sizeof(txSlot);
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

	the->onBreak = espDebugBreak;
}

void espSampleInstrumentation(modTimer timer, void *refcon, uint32_t refconSize)
{
	txInteger values[espInstrumentCount];
	int what;

	for (what = kModInstrumentationPixelsDrawn; what <= kModInstrumentationSystemFreeMemory; what++)
		values[what - kModInstrumentationPixelsDrawn] = modInstrumentationGet_(what);

	values[kModInstrumentationTimers - kModInstrumentationPixelsDrawn] -= 1;    // remove timer used by instrumentation
	fxSampleInstrumentation(gThe, espInstrumentCount, values);

	modInstrumentationSet(PixelsDrawn, 0);
	modInstrumentationSet(FramesDrawn, 0);
	modInstrumentationSet(PocoDisplayListUsed, 0);
	modInstrumentationSet(NetworkBytesRead, 0);
	modInstrumentationSet(NetworkBytesWritten, 0);
	gThe->garbageCollectionCount = 0;
	gThe->stackPeak = gThe->stack;
}
#endif

/*
    RegExp stubs
*/

void fx_RegExp_prototype_get_flags(txMachine* the) {}
void fx_RegExp_prototype_get_global(txMachine* the) {}
void fx_RegExp_prototype_get_ignoreCase(txMachine* the) {}
void fx_RegExp_prototype_get_multiline(txMachine* the) {}
void fx_RegExp_prototype_get_source(txMachine* the) {}
void fx_RegExp_prototype_get_sticky(txMachine* the) {}
void fx_RegExp_prototype_get_unicode(txMachine* the) {}
void fx_RegExp_prototype_compile(txMachine* the) {}
void fx_RegExp_prototype_exec(txMachine* the) {}
void fx_RegExp_prototype_match(txMachine* the) {}
void fx_RegExp_prototype_replace(txMachine* the) {}
void fx_RegExp_prototype_search(txMachine* the) {}
void fx_RegExp_prototype_split(txMachine* the) {}
void fx_RegExp_prototype_test(txMachine* the) {}
void fx_RegExp_prototype_toString(txMachine* the) {}
void fx_RegExp(txMachine* the) {}
void fxInitializeRegExp(txMachine* the) {}

const txHostFunctionBuilder gx_RegExp_prototype_builders[] ICACHE_RODATA_ATTR = {
    { fx_RegExp_prototype_compile, 0, mxID(_compile) },
    { fx_RegExp_prototype_exec, 1, mxID(_exec) },
    { fx_RegExp_prototype_match, 1, mxID(_Symbol_match) },
    { fx_RegExp_prototype_replace, 2, mxID(_Symbol_replace) },
    { fx_RegExp_prototype_search, 1, mxID(_Symbol_search) },
    { fx_RegExp_prototype_split, 2, mxID(_Symbol_split) },
    { fx_RegExp_prototype_test, 1, mxID(_test) },
    { fx_RegExp_prototype_toString, 0, mxID(_toString) },
    { C_NULL, 0, 0 },
};

txSlot* fxNewRegExpInstance(txMachine *the) {return NULL;}
txBoolean fxIsRegExp(txMachine* the, txSlot* slot) {return 0;}


extern void gecko_delay(uint32_t ms);
extern uint32_t gecko_milliseconds();

void delay(uint32_t ms) {
	gecko_delay(ms);
}

uint32_t modMilliseconds() {
	return gecko_milliseconds();
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

void modMessageService(void) {}

extern uint32_t gDeviceUnique;

void *ESP_cloneMachine(uint32_t allocation, uint32_t stackCount, uint32_t slotCount, uint8_t disableDebug)
{
	extern txPreparation* xsPreparation();
	void *result;
	txMachine root;
	txPreparation *prep = xsPreparation();
	txCreation creation;
	uint8_t *context[3];
	char name[20];

	if ((prep->version[0] != XS_MAJOR_VERSION) || (prep->version[1] != XS_MINOR_VERSION) || (prep->version[2] != XS_PATCH_VERSION))
		modLog("version mismatch");

	snprintf(name, 20, "gecko %x", gDeviceUnique);

	creation = prep->creation;

	root.preparation = prep;
	root.archive = NULL;
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
		context[2] = (void*)(uintptr_t)disableDebug;

		result = fxCloneMachine(&prep->creation, &root, name, context);
		if (NULL == result) {
			if (context[0])
				c_free(context[0]);
			return NULL;
		}

		((txMachine *)result)->context = NULL;
	}
	else {
		result = fxCloneMachine(&prep->creation, &root, name, NULL);
		if (NULL == result)
			return NULL;
	}

	((txMachine *)result)->preparation = prep;
#ifdef mxInstrument
	espStartInstrumentation(result);
#endif

	return result;
}

uint8_t xsRunPromiseJobs(txMachine *the)
{
	if (!mxPendingJobs.value.reference->next)
		return 0;

	fxRunPromiseJobs(the);

	return mxPendingJobs.value.reference->next ? 1 : 0;
}

void* mc_xs_chunk_allocator(txMachine *the, size_t size)
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
			context[0] == NULL;
		}
		c_free(the->heap);		// VM is terminated
	}
}

void *mc_xs_slot_allocator(txMachine *the, size_t size)
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

	return mc_xs_chunk_allocator(the, theSize);
}

txSlot* fxAllocateSlots(txMachine* the, txSize theCount)
{
	txSlot* result;

	if (NULL == the->heap)
		return (txSlot*)c_malloc(theCount * sizeof(txSlot));

	result = (txSlot *)mc_xs_slot_allocator(the, theCount * sizeof(txSlot));
	if (!result) {
		fxReport(the, "# Slot allocation: failed. trying to make a room...\n");
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



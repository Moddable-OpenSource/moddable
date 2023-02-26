/*
 * Copyright (c) 2018-2021  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xsAll.h"
#include "xsScript.h"
#include "xs.h"
#include "mc.xs.h"

#ifdef mxUseFreeRTOSTasks	
	#include "FreeRTOS.h"
	#include "freertos/queue.h"
	#include "freertos/semphr.h"

	typedef TaskHandle_t txCondition;
	typedef struct {
		QueueHandle_t handle;
		StaticSemaphore_t buffer;
	} txMutex;
	typedef TaskHandle_t txThread;
	#define fxCreateCondition(CONDITION) *(CONDITION) = xTaskGetCurrentTaskHandle()
	#define fxCreateMutex(MUTEX) (MUTEX)->handle = xSemaphoreCreateMutexStatic(&((MUTEX)->buffer))
	#define fxDeleteCondition(CONDITION) *(CONDITION) = NULL
	#define fxDeleteMutex(MUTEX) vSemaphoreDelete((MUTEX)->handle)
	#define fxLockMutex(MUTEX) xSemaphoreTake((MUTEX)->handle, portMAX_DELAY)
	#define fxUnlockMutex(MUTEX) xSemaphoreGive((MUTEX)->handle)
	#define fxSleepCondition(CONDITION,MUTEX) (xSemaphoreGive((MUTEX)->handle), ulTaskNotifyTake(pdTRUE, portMAX_DELAY), xSemaphoreTake((MUTEX)->handle, portMAX_DELAY))
	#define fxWakeCondition(CONDITION) xTaskNotifyGive(*(CONDITION));
#elif mxWindows
	#include <direct.h>
	#include <errno.h>
	#include <process.h>
	typedef CONDITION_VARIABLE txCondition;
	typedef CRITICAL_SECTION txMutex;
	typedef HANDLE txThread;
	#define fxCreateCondition(CONDITION) InitializeConditionVariable(CONDITION)
	#define fxCreateMutex(MUTEX) InitializeCriticalSection(MUTEX)
	#define fxDeleteCondition(CONDITION) (void)(CONDITION)
	#define fxDeleteMutex(MUTEX) DeleteCriticalSection(MUTEX)
	#define fxLockMutex(MUTEX) EnterCriticalSection(MUTEX)
	#define fxUnlockMutex(MUTEX) LeaveCriticalSection(MUTEX)
	#define fxSleepCondition(CONDITION,MUTEX) SleepConditionVariableCS(CONDITION,MUTEX,INFINITE)
	#define fxWakeCondition(CONDITION) WakeConditionVariable(CONDITION)
#else
	#include <dirent.h>
	#include <pthread.h>
	#include <signal.h>
	#include <sys/stat.h>
	#include <unistd.h>
	typedef pthread_cond_t txCondition;
	typedef pthread_mutex_t txMutex;
	typedef pthread_t txThread;
	#define fxCreateCondition(CONDITION) pthread_cond_init(CONDITION,NULL)
	#define fxCreateMutex(MUTEX) pthread_mutex_init(MUTEX,NULL)
	#define fxDeleteCondition(CONDITION) pthread_cond_destroy(CONDITION)
	#define fxDeleteMutex(MUTEX) pthread_mutex_destroy(MUTEX)
	#define fxLockMutex(MUTEX) pthread_mutex_lock(MUTEX)
	#define fxUnlockMutex(MUTEX) pthread_mutex_unlock(MUTEX)
	#define fxSleepCondition(CONDITION,MUTEX) pthread_cond_wait(CONDITION,MUTEX)
	#define fxWakeCondition(CONDITION) pthread_cond_signal(CONDITION)
#endif
//extern txPreparation* xsPreparation();

typedef struct sxAgent txAgent;
typedef struct sxAgentCluster txAgentCluster;
typedef struct sxAgentReport txAgentReport;

struct sxAgent {
	txAgent* next;
	txThread thread;
	txInteger scriptLength;
	char script[1];
};

struct sxAgentReport {
	txAgentReport* next;
	char message[1];
};

struct sxAgentCluster {
	txAgent* firstAgent;
	txAgent* lastAgent;
	
	txInteger count;
	txCondition countCondition;
	txMutex countMutex;

	void* dataBuffer;
	txCondition dataCondition;
	txMutex dataMutex;
	txInteger dataValue;

	txAgentReport* firstReport;
	txAgentReport* lastReport;
	txMutex reportMutex;
	
	txMachine root;
};

static void _262_agent_leaving(xsMachine* the);
static void _262_agent_receiveBroadcast(xsMachine* the);
static void _262_agent_report(xsMachine* the);
#ifdef mxUseFreeRTOSTasks	
static void _262_agent_start_aux(void *it);
#elif mxWindows
static unsigned int __stdcall _262_agent_start_aux(void* it);
#else
static void* _262_agent_start_aux(void* it);
#endif

static txAgentCluster gxAgentCluster;

enum {
	TEST262_SYNC,
	TEST262_ASYNC_WAITING,
	TEST262_ASYNC_DONE,
};

static txInteger gxTest262Async = TEST262_SYNC;

void _xsbug_done(txMachine* the)
{
	if (gxTest262Async == TEST262_SYNC)
		xsTrace("!!! unexpected $DONE: not waiting !!!\n");
	else if (gxTest262Async == TEST262_ASYNC_DONE)
		xsTrace("!!! unexpected $DONE: already done !!!\n");
	else {
		gxTest262Async = TEST262_ASYNC_DONE;
		if ((mxArgc > 0) && (xsTest(xsArg(0))))
			fxBubble(the, 2, fxToString(the, mxArgv(0)), 0, "test262");
		else
			fxBubble(the, 2, "<", 0, "test262");
	}
}

// stack grows UP
// pxTaskGetStackStart (on ESP32) returns end of stack (!)

#if 0
void testOne(int count)
{
	char foo[256];

	modLogInt(count);
//	modLogHex((uintptr_t)pxTaskGetStackStart(NULL));
	int free = (uintptr_t)foo - (uintptr_t)pxTaskGetStackStart(NULL);
	modLogInt(free);
	if (free <= 304)
		return;

	testOne(count + 1);
}
#endif

void _xsbug_main(txMachine* the)
{
	txPreparation* preparation = xsPreparation();
	txMachine* root;

	c_memset(&gxAgentCluster, 0, sizeof(txAgentCluster));
	
	fxCreateCondition(&(gxAgentCluster.countCondition));
	fxCreateMutex(&(gxAgentCluster.countMutex));
	fxCreateCondition(&(gxAgentCluster.dataCondition));
	fxCreateMutex(&(gxAgentCluster.dataMutex));
	fxCreateMutex(&(gxAgentCluster.reportMutex));
	
	root = &(gxAgentCluster.root);
	root->preparation = preparation;
	root->keyArray = preparation->keys;
	root->keyCount = (txID)preparation->keyCount + (txID)preparation->creation.initialKeyCount;
	root->keyIndex = (txID)preparation->keyCount;
	root->nameModulo = preparation->nameModulo;
	root->nameTable = preparation->names;
	root->symbolModulo = preparation->symbolModulo;
	root->symbolTable = preparation->symbols;
	
	root->stack = &preparation->stack[0];
	root->stackBottom = &preparation->stack[0];
	root->stackTop = &preparation->stack[preparation->stackCount];
	
	root->firstHeap = &preparation->heap[0];
	root->freeHeap = &preparation->heap[preparation->heapCount - 1];
	root->aliasCount = (txID)preparation->aliasCount;
	
	gxTest262Async = TEST262_SYNC;
	xsResult = xsNewHostFunction(_xsbug_done, 1);
	xsSet(xsGlobal, xsID("$DONE"), xsResult);
	fxBubble(the, 2, ">", 0, "test262");
}

static void _xsbug_importFulfilled(txMachine* the)
{
	if (gxTest262Async == TEST262_SYNC)
		fxBubble(the, 2, "<", 0, "test262");
}

static void _xsbug_importRejected(txMachine* the)
{
	fxBubble(the, 2, fxToString(the, mxArgv(0)), 0, "test262");
}

void _xsbug_import_(txMachine* the)
{
	txSlot* realm = mxProgram.value.reference->next->value.module.realm;
	mxPushSlot(mxArgv(0));
	fxRunImport(the, realm, XS_NO_ID);
	mxDub();
	fxGetID(the, mxID(_then));
	mxCall();
	fxNewHostFunction(the, _xsbug_importFulfilled, 1, XS_NO_ID, XS_NO_ID);
	fxNewHostFunction(the, _xsbug_importRejected, 1, XS_NO_ID, XS_NO_ID);
	mxRunCount(2);
	mxPop();
	if (xsTest(xsArg(1)))
		gxTest262Async = TEST262_ASYNC_WAITING;
}

void _xsbug_module_(txMachine* the)
{
	mxTry(the) {
		txParser _parser;
		txParser* parser = &_parser;
		txParserJump jump;
		txString name = NULL;
		txScript* script = NULL;
		fxInitializeParser(parser, the, the->parserBufferSize, the->parserTableModulo);
		parser->firstJump = &jump;
		if (c_setjmp(jump.jmp_buf) == 0) {
			txStringStream stream;
			stream.slot = mxArgv(3);
			stream.offset = 0;
			stream.size = fxToInteger(the, mxArgv(5)) - 1;
			parser->path = fxNewParserSymbol(parser, fxToString(the, mxArgv(1)));
			fxParserTree(parser, &stream, fxStringGetter, mxDebugFlag | mxStrictFlag, &name);
			fxParserHoist(parser);
			fxParserBind(parser);
			script = fxParserCode(parser);
		}
	#ifdef mxInstrument
		if (the->peakParserSize < parser->total)
			the->peakParserSize = parser->total;
	#endif
		fxTerminateParser(parser);
 		if (script) {
 			fxResolveModule(the, mxArgv(0), XS_NO_ID, script, C_NULL, C_NULL);
		}
 		else {
			mxSyntaxError("invalid module");
 		}
	}
	mxCatch(the) {
		fxJump(the);
	}
}

void _xsbug_script_(txMachine* the)
{
	txSlot* realm = mxProgram.value.reference->next->value.module.realm;
	mxTry(the) {
		txParser _parser;
		txParser* parser = &_parser;
		txParserJump jump;
		txString name = NULL;
		txScript* script = NULL;
		fxInitializeParser(parser, the, the->parserBufferSize, the->parserTableModulo);
		parser->firstJump = &jump;
		if (c_setjmp(jump.jmp_buf) == 0) {
			txStringStream stream;
			stream.slot = mxArgv(3);
			stream.offset = 0;
			stream.size = fxToInteger(the, mxArgv(5)) - 1;
			parser->path = fxNewParserSymbol(parser, fxToString(the, mxArgv(1)));
			fxParserTree(parser, &stream, fxStringGetter, mxProgramFlag | mxDebugFlag | mxStrictFlag, &name);
			fxParserHoist(parser);
			fxParserBind(parser);
			script = fxParserCode(parser);
		}
	#ifdef mxInstrument
		if (the->peakParserSize < parser->total)
			the->peakParserSize = parser->total;
	#endif
		name = fxNewParserChunk(parser, 8 + parser->path->length);
		strcpy(name, "xsbug://");
		strcat(name, parser->path->string);
		mxModuleInstanceInternal(mxProgram.value.reference)->value.module.id = fxID(the, name);
		fxTerminateParser(parser);
		if (xsTest(xsArg(2)))
			gxTest262Async = TEST262_ASYNC_WAITING;
		fxRunScript(the, script, mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, mxProgram.value.reference);
		mxPop();
		if (gxTest262Async == TEST262_SYNC)
			fxBubble(the, 2, "<", 0, "test262");
	}
	mxCatch(the) {
		mxPush(mxException);
		fxBubble(the, 2, fxToString(the, the->stack), 0, "test262");
		mxPop();
	}
}

void fx_agent_get_safeBroadcast(xsMachine* the)
{
	xsResult = xsGet(xsThis, xsID("broadcast"));
}

void fx_agent_set_safeBroadcast(xsMachine* the)
{
}

void _262_agent_broadcast(txMachine* the)
{
	if (xsIsInstanceOf(xsArg(0), xsTypedArrayPrototype)) {
		xsArg(0) = xsGet(xsArg(0), xsID("buffer"));
	}
#ifdef mxUseFreeRTOSTasks	
	txAgent* agent = gxAgentCluster.firstAgent;
	gxAgentCluster.dataBuffer = xsMarshallAlien(xsArg(0));
	if (mxArgc > 1)
		gxAgentCluster.dataValue = xsToInteger(xsArg(1));
	while (agent) {
		if (agent->thread)
			xTaskNotifyGive(agent->thread);
		agent = agent->next;
	}
#else
    fxLockMutex(&(gxAgentCluster.dataMutex));
	gxAgentCluster.dataBuffer = xsMarshallAlien(xsArg(0));
	if (mxArgc > 1)
		gxAgentCluster.dataValue = xsToInteger(xsArg(1));
#if mxWindows
	WakeAllConditionVariable(&(gxAgentCluster.dataCondition));
#else
	pthread_cond_broadcast(&(gxAgentCluster.dataCondition));
#endif
    fxUnlockMutex(&(gxAgentCluster.dataMutex));
#endif
    fxLockMutex(&(gxAgentCluster.countMutex));
    while (gxAgentCluster.count > 0)
		fxSleepCondition(&(gxAgentCluster.countCondition), &(gxAgentCluster.countMutex));
    fxUnlockMutex(&(gxAgentCluster.countMutex));
}

void _262_agent_getReport(txMachine* the)
{
	txAgentReport* report = C_NULL;
    fxLockMutex(&(gxAgentCluster.reportMutex));
	report = gxAgentCluster.firstReport;
	if (report)
		gxAgentCluster.firstReport = report->next;
    fxUnlockMutex(&(gxAgentCluster.reportMutex));
    if (report) {
    	xsResult = xsString(report->message);
    	c_free(report);
    }
    else
    	xsResult = xsNull;
}

void _262_agent_leaving(txMachine* the)
{
}

void _262_agent_monotonicNow(xsMachine* the)
{
	xsResult = xsNumber(fxDateNow());
// #ifdef mxUseFreeRTOSTasks
// 	TickType_t ticks = xTaskGetTickCount();
// 	txNumber now = (txNumber)ticks * 1000.0 / (txNumber)configTICK_RATE_HZ;
//     xsResult = xsNumber(c_trunc(now));
// #else
// 	struct timespec now;
// 	clock_gettime(CLOCK_MONOTONIC, &now);
//     xsResult = xsNumber(((txNumber)(now.tv_sec) * 1000.0) + ((txNumber)(now.tv_nsec / 1000000)));
// #endif		
}

void _262_agent_receiveBroadcast(xsMachine* the)
{
#ifdef mxUseFreeRTOSTasks
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	xsResult = xsDemarshallAlien(gxAgentCluster.dataBuffer);
#else
    fxLockMutex(&(gxAgentCluster.dataMutex));
	while (gxAgentCluster.dataBuffer == NULL)
		fxSleepCondition(&(gxAgentCluster.dataCondition), &(gxAgentCluster.dataMutex));
	xsResult = xsDemarshallAlien(gxAgentCluster.dataBuffer);
    fxUnlockMutex(&(gxAgentCluster.dataMutex));
#endif		
    fxLockMutex(&(gxAgentCluster.countMutex));
    gxAgentCluster.count--;
    fxWakeCondition(&(gxAgentCluster.countCondition));
    fxUnlockMutex(&(gxAgentCluster.countMutex));
	xsCallFunction2(xsArg(0), xsGlobal, xsResult, xsInteger(gxAgentCluster.dataValue));
}

void _262_agent_report(xsMachine* the)
{
	xsStringValue message = xsToString(xsArg(0));
	xsIntegerValue messageLength = c_strlen(message);
	txAgentReport* report = c_malloc(sizeof(txAgentReport) + messageLength);
	if (!report) xsUnknownError("not enough memory");
    report->next = C_NULL;
	c_memcpy(&(report->message[0]), message, messageLength + 1);
    fxLockMutex(&(gxAgentCluster.reportMutex));
    if (gxAgentCluster.firstReport)
		gxAgentCluster.lastReport->next = report;
    else
		gxAgentCluster.firstReport = report;
	gxAgentCluster.lastReport = report;
    fxUnlockMutex(&(gxAgentCluster.reportMutex));
}

void _262_agent_sleep(txMachine* the)
{
	xsIntegerValue delay = xsToInteger(xsArg(0));
#ifdef mxUseFreeRTOSTasks
	vTaskDelay(pdMS_TO_TICKS(delay));
#elif mxWindows
	Sleep(delay);
#else	
	usleep(delay * 1000);
#endif
}

void _262_agent_start(txMachine* the)
{
	xsStringValue script = xsToString(xsArg(0));
	xsIntegerValue scriptLength = c_strlen(script);
	txAgent* agent = c_malloc(sizeof(txAgent) + scriptLength);
	if (!agent) xsUnknownError("not enough memory");
	c_memset(agent, 0, sizeof(txAgent));
	if (gxAgentCluster.firstAgent)
		gxAgentCluster.lastAgent->next = agent;
	else
		gxAgentCluster.firstAgent = agent;
	gxAgentCluster.lastAgent = agent;
	gxAgentCluster.count++;
	agent->scriptLength = scriptLength;
	c_memcpy(&(agent->script[0]), script, scriptLength + 1);
#ifdef mxUseFreeRTOSTasks
	#if 0 == CONFIG_LOG_DEFAULT_LEVEL
		#define kStack ((5 * 1024) / sizeof(StackType_t))
	#else
		#define kStack ((6 * 1024) / sizeof(StackType_t))
	#endif
	xTaskCreate(_262_agent_start_aux, "agent", kStack, agent, 8, &(agent->thread));
#elif mxWindows
	agent->thread = (HANDLE)_beginthreadex(NULL, 0, _262_agent_start_aux, agent, 0, NULL);
#else	
    pthread_create(&(agent->thread), NULL, &_262_agent_start_aux, agent);
#endif
}

#ifdef mxUseFreeRTOSTasks
void _262_agent_start_aux(void *it)
#elif mxWindows
unsigned int __stdcall _262_agent_start_aux(void* it)
#else
void* _262_agent_start_aux(void* it)
#endif
{
	xsCreation creation = {
		1536, 				/* initialChunkSize */
		512, 				/* incrementalChunkSize */
		512, 				/* initialHeapCount */
		64, 				/* incrementalHeapCount */
		256, 				/* stackCount */
		1024, 				/* initialKeyCount */
		0,	 				/* incrementalKeyCount */
		127, 				/* nameModulo */
		127,				/* symbolModulo */
		2048,				/* parserBufferSize */
		127,				/* parserTableModulo */
	};
	txAgent* agent = it;
	txMachine* machine = fxCloneMachine(&creation, &(gxAgentCluster.root), "agent", NULL);
	xsBeginHost(machine);
	{
		xsTry {
			txSlot* slot;
			txStringCStream stream;
			
			slot = fxLastProperty(the, fxNewHostObject(the, NULL));
			slot = fxNextHostFunctionProperty(the, slot, _262_agent_leaving, 0, xsID("leaving"), XS_GET_ONLY); 
			slot = fxNextHostFunctionProperty(the, slot, _262_agent_monotonicNow, 0, xsID("monotonicNow"), XS_GET_ONLY); 
			slot = fxNextHostFunctionProperty(the, slot, _262_agent_receiveBroadcast, 1, xsID("receiveBroadcast"), XS_GET_ONLY); 
			slot = fxNextHostFunctionProperty(the, slot, _262_agent_report, 1, xsID("report"), XS_GET_ONLY); 
			slot = fxNextHostFunctionProperty(the, slot, _262_agent_sleep, 1, xsID("sleep"), XS_GET_ONLY); 
			fxSetHostData(the, the->stack, agent);
			
			mxPush(mxObjectPrototype);
			slot = fxLastProperty(the, fxNewObjectInstance(the));
			slot = fxNextSlotProperty(the, slot, the->stack + 1, xsID("agent"), XS_GET_ONLY);
			slot = fxGlobalSetProperty(the, mxGlobal.value.reference, xsID("$262"), XS_NO_ID, XS_OWN);
			slot->flag = XS_GET_ONLY;
			slot->kind = the->stack->kind;
			slot->value = the->stack->value;
			mxPop();
			
			mxPop();
			
			stream.buffer = agent->script;
			stream.offset = 0;
			stream.size = agent->scriptLength;
			fxRunScript(the, fxParseScript(the, &stream, fxStringCGetter, mxProgramFlag), mxThis, C_NULL, C_NULL, C_NULL, mxProgram.value.reference);
		}
		xsCatch {
		}
	}
	xsEndHost(machine);
	xsDeleteMachine(machine);
#ifdef mxUseFreeRTOSTasks
	vTaskDelete(NULL);
#elif mxWindows
	return 0;
#else
	return NULL;
#endif
}

void _262_agent_stop(txMachine* the)
{
#ifdef mxUseFreeRTOSTasks
	// will reboot
#else
	txAgent* agent = gxAgentCluster.firstAgent;
	if (agent) {
		while (agent) {
			txAgent* next = agent->next;
			if (agent->thread) {
				#if mxWindows
					WaitForSingleObject(agent->thread, INFINITE);
					CloseHandle(agent->thread);
				#else
					pthread_join(agent->thread, NULL);
				#endif
			}
			c_free(agent);
			agent = next;
		}
		if (gxAgentCluster.dataBuffer)
			c_free(gxAgentCluster.dataBuffer);
		gxAgentCluster.firstAgent = C_NULL;
		gxAgentCluster.lastAgent = C_NULL;
		gxAgentCluster.count = 0;
		gxAgentCluster.dataBuffer = C_NULL;
		gxAgentCluster.dataValue = 0;
		gxAgentCluster.firstReport = C_NULL;
		gxAgentCluster.lastReport = C_NULL;
	}
#endif
}

void _262_createRealm(txMachine* the)
{
	xsResult = xsThis;
}

void _262_detachArrayBuffer(txMachine* the)
{
	txSlot* slot = mxArgv(0);
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_ARRAY_BUFFER_KIND) && (instance != mxArrayBufferPrototype.value.reference)) {
			txSlot *bufferInfo = slot->next;
			if (bufferInfo && (bufferInfo->flag & XS_INTERNAL_FLAG) && (bufferInfo->kind == XS_BUFFER_INFO_KIND)) {
				slot->value.arrayBuffer.address = C_NULL;
				slot->value.arrayBuffer.detachKey = C_NULL;
				bufferInfo->value.bufferInfo.length = 0;
				if (bufferInfo->value.bufferInfo.maxLength > 0)
					bufferInfo->value.bufferInfo.maxLength = 0;
				return;
			}
		}
	}
	mxTypeError("this is no ArrayBuffer instance");
}

void _262_gc(txMachine* the)
{
	fxCollectGarbage(the);
}

void _262_evalScript(txMachine* the)
{
	txSlot* realm = mxProgram.value.reference->next->value.module.realm;
	txStringStream aStream;
	aStream.slot = mxArgv(0);
	aStream.offset = 0;
	aStream.size = c_strlen(fxToString(the, mxArgv(0)));
	fxRunScript(the, fxParseScript(the, &aStream, fxStringGetter, mxProgramFlag | mxDebugFlag), mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, mxProgram.value.reference);
	mxPullSlot(mxResult);
}


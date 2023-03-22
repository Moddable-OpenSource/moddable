/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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
#include "yaml.h"

#if mxWindows
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
	#define fxWakeAllCondition(CONDITION) WakeAllConditionVariable(CONDITION)
	#define fxWakeCondition(CONDITION) WakeConditionVariable(CONDITION)
#else
	#include <dirent.h>
	#include <pthread.h>
	#include <sys/stat.h>
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
	#define fxWakeAllCondition(CONDITION) pthread_cond_broadcast(CONDITION)
	#define fxWakeCondition(CONDITION) pthread_cond_signal(CONDITION)
#endif

typedef struct sxAgent txAgent;
typedef struct sxAgentCluster txAgentCluster;
typedef struct sxAgentReport txAgentReport;
typedef struct sxContext txContext;
typedef struct sxJob txJob;
typedef struct sxPool txPool;
typedef struct sxResult txResult;

#ifdef mxMultipleThreads
	#define mxPoolSize 3
#else
	#define mxPoolSize 1
#endif

struct sxAgent {
	txAgent* next;
#if mxWindows
    HANDLE thread;
#else
	pthread_t thread;
#endif
	txInteger scriptLength;
	char script[1];
};

struct sxAgentReport {
	txAgentReport* next;
	char message[1];
};

struct sxAgentCluster {
	txMutex mainMutex;

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
};

struct sxContext {
	txContext* next;
	txResult* result;
	yaml_document_t* document;
	yaml_node_t* includes;
	yaml_node_t* negative;
	char path[1];
};

struct sxJob {
	txJob* next;
	txMachine* the;
	txNumber when;
	txSlot self;
	txSlot function;
	txSlot argument;
	txNumber interval;
};

struct sxPool {
	txContext* first;
	txInteger count;
	txCondition countCondition;
	txMutex countMutex;
	txResult* current;
	txMutex resultMutex;
	txThread threads[mxPoolSize];
	char harnessPath[C_PATH_MAX];
	int testPathLength;
};

struct sxResult {
	txResult* next;
	txResult* parent;
	txResult* first;
	txResult* last;
	int testCount;
	int successCount;
	int pendingCount;
	char path[1];
};

static int main262(int argc, char* argv[]);
#if FUZZILLI
static int fuzz(int argc, char* argv[]);
#endif
#if OSSFUZZ
static int fuzz_oss(const uint8_t *Data, size_t script_size);
#endif
static void fxBuildAgent(xsMachine* the);
static void fxCountResult(txPool* pool, txContext* context, int success, int pending);
static yaml_node_t *fxGetMappingValue(yaml_document_t* document, yaml_node_t* mapping, char* name);
static void fxPopResult(txPool* pool);
#ifdef mxMultipleThreads
static void fxPrintBusy(txPool* pool);
static void fxPrintClear(txPool* pool);
#endif
static void fxPrintResult(txPool* pool, txResult* result, int c);
static void fxPrintUsage();
static void fxPushResult(txPool* pool, char* path);
static void fxRunDirectory(txPool* pool, char* path);
static void fxRunFile(txPool* pool, char* path);
#if mxWindows
static unsigned int __stdcall fxRunFileThread(void* it);
#else
static void* fxRunFileThread(void* it);
#endif
static void fxRunContext(txPool* pool, txContext* context);
static int fxRunTestCase(txPool* pool, txContext* context, char* path, txUnsigned flags, int async, char* message);
static int fxStringEndsWith(const char *string, const char *suffix);

static void fx_agent_get_safeBroadcast(xsMachine* the);
static void fx_agent_set_safeBroadcast(xsMachine* the);
static void fx_agent_broadcast(xsMachine* the);
static void fx_agent_getReport(xsMachine* the);
static void fx_agent_leaving(xsMachine* the);
static void fx_agent_monotonicNow(xsMachine* the);
static void fx_agent_receiveBroadcast(xsMachine* the);
static void fx_agent_report(xsMachine* the);
static void fx_agent_sleep(xsMachine* the);
static void fx_agent_start(xsMachine* the);
#if mxWindows
static unsigned int __stdcall fx_agent_start_aux(void* it);
#else
static void* fx_agent_start_aux(void* it);
#endif
static void fx_agent_stop(xsMachine* the);
static void fx_createRealm(xsMachine* the);
static void fx_detachArrayBuffer(xsMachine* the);
static void fx_done(xsMachine* the);
static void fx_evalScript(xsMachine* the);
#if FUZZING || FUZZILLI
static void fx_fillBuffer(txMachine *the);
void fx_nop(xsMachine *the);
void fx_assert_throws(xsMachine *the);
#endif
static void fx_gc(xsMachine* the);
static void fx_print(xsMachine* the);

extern void fx_clearTimer(txMachine* the);
static void fx_destroyTimer(void* data);
static void fx_markTimer(txMachine* the, void* it, txMarkRoot markRoot);
static void fx_setInterval(txMachine* the);
static void fx_setTimeout(txMachine* the);
static void fx_setTimer(txMachine* the, txNumber interval, txBoolean repeat);

static void fxFulfillModuleFile(txMachine* the);
static void fxRejectModuleFile(txMachine* the);
static void fxRunModuleFile(txMachine* the, txString path);
static void fxRunProgramFile(txMachine* the, txString path, txUnsigned flags);
static void fxRunLoop(txMachine* the);

static txScript* fxLoadScript(txMachine* the, txString path, txUnsigned flags);


static char *gxAbortStrings[] = {
	"debugger",
	"memory full",
	"stack overflow",
	"fatal",
	"dead strip",
	"unhandled exception",
	"not enough keys",
	"too much computation",
	"unhandled rejection"
};

static txAgentCluster gxAgentCluster;

static xsBooleanValue xsAlwaysWithinComputeLimit(xsMachine* machine, xsUnsignedValue index)
{
	return 1;
}

#if OSSFUZZ
int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    fuzz_oss(Data, Size);
    return 0;
}

int omain(int argc, char* argv[]) 
#else
int main(int argc, char* argv[]) 
#endif
{
	txAgentCluster* agentCluster = &gxAgentCluster;
	int argi;
	int option = 0;
	int profiling = 0;
	char path[C_PATH_MAX];
	char* dot;
#if mxWindows
    char* harnessPath = "..\\harness";
#else
    char* harnessPath = "../harness";
#endif
	int error = 0;
	
	c_memset(agentCluster, 0, sizeof(txAgentCluster));
	fxCreateMutex(&(agentCluster->mainMutex));
	fxCreateCondition(&(agentCluster->countCondition));
	fxCreateMutex(&(agentCluster->countMutex));
	fxCreateCondition(&(agentCluster->dataCondition));
	fxCreateMutex(&(agentCluster->dataMutex));
	fxCreateMutex(&(agentCluster->reportMutex));

	if (argc == 1) {
		fxPrintUsage();
		return 1;
	}
	for (argi = 1; argi < argc; argi++) {
		if (argv[argi][0] != '-')
			continue;
		if (!strcmp(argv[argi], "-h"))
			fxPrintUsage();
		else if (!strcmp(argv[argi], "-e"))
			option = 1;
		else if (!strcmp(argv[argi], "-m"))
			option = 2;
		else if (!strcmp(argv[argi], "-p"))
			profiling = 1;
		else if (!strcmp(argv[argi], "-s"))
			option = 3;
		else if (!strcmp(argv[argi], "-t"))
			option = 4;
#if FUZZILLI
		else if (!strcmp(argv[argi], "-f"))
			option = 5;
#endif
		else if (!strcmp(argv[argi], "-j"))
			option = 6;
		else if (!strcmp(argv[argi], "-v"))
			printf("XS %d.%d.%d %zu %zu\n", XS_MAJOR_VERSION, XS_MINOR_VERSION, XS_PATCH_VERSION, sizeof(txSlot), sizeof(txID));
		else {
			fxPrintUsage();
			return 1;
		}
	}
	if (option == 0) {
		if (c_realpath(harnessPath, path))
			option  = 4;
	}
	if (option == 4) {
		error = main262(argc, argv);
	}
#if FUZZILLI
 	else if (option == 5) {
 		error = fuzz(argc, argv);
 	}
#endif
	else {
		xsCreation _creation = {
			16 * 1024 * 1024, 	/* initialChunkSize */
			16 * 1024 * 1024, 	/* incrementalChunkSize */
			1 * 1024 * 1024, 	/* initialHeapCount */
			1 * 1024 * 1024, 	/* incrementalHeapCount */
			256 * 1024, 		/* stackCount */
			256 * 1024, 		/* initialKeyCount */
			0,					/* incrementalKeyCount */
			1993, 				/* nameModulo */
			127, 				/* symbolModulo */
			64 * 1024,			/* parserBufferSize */
			1993,				/* parserTableModulo */
		};
		xsCreation* creation = &_creation;
		xsMachine* machine;
		fxInitializeSharedCluster();
        machine = xsCreateMachine(creation, "xst", NULL);
 		fxBuildAgent(machine);
 		if (profiling)
			fxStartProfiling(machine);
		xsBeginMetering(machine, xsAlwaysWithinComputeLimit, 0x7FFFFFFF);
		{

		xsBeginHost(machine);
		{
			xsVars(2);
			xsTry {
#if FUZZING
				xsResult = xsNewHostFunction(fx_gc, 0);
				xsSet(xsGlobal, xsID("gc"), xsResult);
				xsResult = xsNewHostFunction(fx_fillBuffer, 2);
				xsSet(xsGlobal, xsID("fillBuffer"), xsResult);

				xsResult = xsNewHostFunction(fx_harden, 1);
				xsDefine(xsGlobal, xsID("harden"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_lockdown, 0);
				xsDefine(xsGlobal, xsID("lockdown"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_petrify, 1);
				xsDefine(xsGlobal, xsID("petrify"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_mutabilities, 1);
				xsDefine(xsGlobal, xsID("mutabilities"), xsResult, xsDontEnum);
#endif

				xsVar(0) = xsUndefined;
				the->rejection = &xsVar(0);
				for (argi = 1; argi < argc; argi++) {
					if (argv[argi][0] == '-')
						continue;
					if (option == 1) {
						xsVar(1) = xsGet(xsGlobal, xsID("$262"));
						xsResult = xsString(argv[argi]);
						xsCall1(xsVar(1), xsID("evalScript"), xsResult);
					}
					else {
						if (!c_realpath(argv[argi], path))
							xsURIError("file not found: %s", argv[argi]);
						dot = strrchr(path, '.');
						if (option == 6) {
							FILE* file = C_NULL;
							char *buffer = C_NULL;
							xsTry {
								file = fopen(path, "r");
								if (!file)
									xsUnknownError("can't open file");
								fseek(file, 0, SEEK_END);
								size_t size = ftell(file);
								fseek(file, 0, SEEK_SET);
								buffer = malloc(size + 1);
								if (!buffer)
									xsUnknownError("not enough memory");
								if (size != fread(buffer, 1, size, file))	
									xsUnknownError("can't read file");
								buffer[size] = 0;
								fclose(file);
								file = C_NULL;
								xsResult = xsArrayBuffer(buffer, size);
								c_free(buffer);
								buffer = C_NULL;
								xsVar(1) = xsNew0(xsGlobal, xsID("TextDecoder"));
								xsResult = xsCall1(xsVar(1), xsID("decode"), xsResult);
								xsVar(1) = xsGet(xsGlobal, xsID("JSON"));
								xsResult = xsCall1(xsVar(1), xsID("parse"), xsResult);
							}
							xsCatch {
								if (buffer)
									c_free(buffer);
								if (file)
									fclose(file);
							}
						}
						else
						if (((option == 0) && dot && !c_strcmp(dot, ".mjs")) || (option == 2))
							fxRunModuleFile(the, path);
						else
							fxRunProgramFile(the, path, mxProgramFlag | mxDebugFlag);
					}
				}
				fxRunLoop(the);
				if (xsTest(xsVar(0))) 
					xsThrow(xsVar(0));
			}
			xsCatch {
				fprintf(stderr, "%s\n", xsToString(xsException));
				error = 1;
			}
		}
		fxCheckUnhandledRejections(machine, 1);
		xsEndHost(machine);
		}
		xsEndMetering(machine);
 		if (profiling)
			fxStopProfiling(machine, C_NULL);
		if (machine->abortStatus) {
			char *why = (machine->abortStatus <= XS_UNHANDLED_REJECTION_EXIT) ? gxAbortStrings[machine->abortStatus] : "unknown";
			fprintf(stderr, "Error: %s\n", why);
			error = 1;
		}
		xsDeleteMachine(machine);
		fxTerminateSharedCluster();
	}
	return error;
}

int main262(int argc, char* argv[]) 
{
	txPool pool;
	char separator[2];
	char path[C_PATH_MAX];
	int error = 0;
	int argi = 1;
	int argj = 0;
	c_timeval from;
	c_timeval to;

	c_memset(&pool, 0, sizeof(txPool));
	fxCreateCondition(&(pool.countCondition));
	fxCreateMutex(&(pool.countMutex));
	fxCreateMutex(&(pool.resultMutex));
	{
	#if mxWindows
	#elif mxMacOSX
		pthread_attr_t attr; 
		pthread_t self = pthread_self();
   		size_t size = pthread_get_stacksize_np(self);
   		pthread_attr_init(&attr);
   		pthread_attr_setstacksize(&attr, size);
	#elif mxLinux
	#endif	
		for (argi = 0; argi < mxPoolSize; argi++) {
		#if mxWindows
			pool.threads[argi] = (HANDLE)_beginthreadex(NULL, 0, fxRunFileThread, &pool, 0, NULL);
		#elif mxMacOSX
			pthread_create(&(pool.threads[argi]), &attr, &fxRunFileThread, &pool);
		#else
			pthread_create(&(pool.threads[argi]), NULL, &fxRunFileThread, &pool);
		#endif
		}
	}
	fxInitializeSharedCluster();

	separator[0] = mxSeparator;
	separator[1] = 0;
	c_strcpy(path, "..");
	c_strcat(path, separator);
	c_strcat(path, "harness");
	if (!c_realpath(path, pool.harnessPath)) {
		fprintf(stderr, "### directory not found: %s\n", path);
		return 1;
	}
	c_strcat(pool.harnessPath, separator);
	if (!c_realpath(".", path)) {
		fprintf(stderr, "### directory not found: .\n");
		return 1;
	}
	c_strcat(path, separator);
	pool.testPathLength = mxStringLength(path);
	pool.current = NULL;
	fxPushResult(&pool, "");
	
	c_gettimeofday(&from, NULL);
	for (argi = 1; argi < argc; argi++) {
		if (argv[argi][0] == '-')
			continue;
		if (c_realpath(argv[argi], path)) {
			argj++;
#if mxWindows
			DWORD attributes = GetFileAttributes(path);
			if (attributes != 0xFFFFFFFF) {
				if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
#else		
			struct stat a_stat;
			if (stat(path, &a_stat) == 0) {
				if (S_ISDIR(a_stat.st_mode)) {
#endif
					fxPushResult(&pool, path + pool.testPathLength);
					fxRunDirectory(&pool, path);
					fxPopResult(&pool);
				}
				else if (fxStringEndsWith(path, ".js") && !fxStringEndsWith(path, "_FIXTURE.js"))
					fxRunFile(&pool, path);
			}
		}
		else {
			fprintf(stderr, "### test not found: %s\n", argv[argi]);
			error = 1;
		}
	}
    fxLockMutex(&(pool.countMutex));
    while (pool.count > 0)
		fxSleepCondition(&(pool.countCondition), &(pool.countMutex));
	pool.count = -1;
	fxWakeAllCondition(&(pool.countCondition));
    fxUnlockMutex(&(pool.countMutex));
	for (argi = 0; argi < mxPoolSize; argi++) {
	#if mxWindows
		WaitForSingleObject(pool.threads[argi], INFINITE);
		CloseHandle(pool.threads[argi]);
	#else
		pthread_join(pool.threads[argi], NULL);
	#endif
	}
	
	fxTerminateSharedCluster();
	
	c_gettimeofday(&to, NULL);
	if (argj) {
		int seconds = to.tv_sec - from.tv_sec;
		int minutes = seconds / 60;
		int hours = minutes / 60;
		txResult* result = pool.current;
		int value;
		
	#ifdef mxMultipleThreads	
		fxPrintClear(&pool);
	#endif
		fprintf(stderr, "# %d:%.2d:%.2d\n", hours, minutes % 60, seconds % 60);
		
		if (result->testCount) {
			value = (10000 * result->successCount) / result->testCount;
			fprintf(stderr, "# %d.%.2d%%", value / 100, value % 100);
			if (result->pendingCount) {
				if (result->successCount + result->pendingCount == result->testCount)
					value = 10000 - value;
				else
					value = (10000 * result->pendingCount) / result->testCount;
				fprintf(stderr, " (%d.%.2d%%)", value / 100, value % 100);
			}
		}
		else
			fprintf(stderr, "# 0.00%%");
		fxPrintResult(&pool, result, 0);
	}
	return error;
}

extern void modInstallTextDecoder(xsMachine *the);
extern void modInstallTextEncoder(xsMachine *the);
extern void modInstallBase64(xsMachine *the);

void fxBuildAgent(xsMachine* the) 
{
	txSlot* slot;
	txSlot* agent;
	txSlot* global;

	slot = fxLastProperty(the, fxNewHostObject(the, NULL));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_agent_get_safeBroadcast), mxCallback(fx_agent_set_safeBroadcast), xsID("safeBroadcast"), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_agent_broadcast, 2, xsID("broadcast"), XS_DONT_ENUM_FLAG); 
	slot = fxNextHostFunctionProperty(the, slot, fx_agent_getReport, 0, xsID("getReport"), XS_DONT_ENUM_FLAG); 
	slot = fxNextHostFunctionProperty(the, slot, fx_agent_sleep, 1, xsID("sleep"), XS_DONT_ENUM_FLAG); 
	slot = fxNextHostFunctionProperty(the, slot, fx_agent_start, 1, xsID("start"), XS_DONT_ENUM_FLAG); 
	slot = fxNextHostFunctionProperty(the, slot, fx_agent_stop, 1, xsID("stop"), XS_DONT_ENUM_FLAG);
	agent = the->stack;

	mxPush(mxGlobal);
	global = the->stack;

	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextSlotProperty(the, slot, agent, xsID("agent"), XS_GET_ONLY);
	slot = fxNextHostFunctionProperty(the, slot, fx_createRealm, 0, xsID("createRealm"), XS_DONT_ENUM_FLAG); 
	slot = fxNextHostFunctionProperty(the, slot, fx_detachArrayBuffer, 1, xsID("detachArrayBuffer"), XS_DONT_ENUM_FLAG); 
	slot = fxNextHostFunctionProperty(the, slot, fx_gc, 1, xsID("gc"), XS_DONT_ENUM_FLAG); 
	slot = fxNextHostFunctionProperty(the, slot, fx_evalScript, 1, xsID("evalScript"), XS_DONT_ENUM_FLAG); 
	slot = fxNextSlotProperty(the, slot, global, xsID("global"), XS_GET_ONLY);

	slot = fxLastProperty(the, fxToInstance(the, global));
	slot = fxNextSlotProperty(the, slot, the->stack, xsID("$262"), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_print, 1, xsID("print"), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_clearTimer, 1, xsID("clearInterval"), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_clearTimer, 1, xsID("clearTimeout"), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_setInterval, 1, xsID("setInterval"), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_setTimeout, 1, xsID("setTimeout"), XS_DONT_ENUM_FLAG);

	mxPop();
	mxPop();
	
	modInstallTextDecoder(the);
	modInstallTextEncoder(the);
	modInstallBase64(the);
}

void fxCountResult(txPool* pool, txContext* context, int success, int pending) 
{
	txResult* result = context->result;
    fxLockMutex(&(pool->resultMutex));
	while (result) {
		result->testCount++;
		result->successCount += success;
		result->pendingCount += pending;
		result = result->parent;
	}
    fxUnlockMutex(&(pool->resultMutex));
}

yaml_node_t *fxGetMappingValue(yaml_document_t* document, yaml_node_t* mapping, char* name)
{
	yaml_node_pair_t* pair = mapping->data.mapping.pairs.start;
	while (pair < mapping->data.mapping.pairs.top) {
		yaml_node_t* key = yaml_document_get_node(document, pair->key);
		if (!strcmp((char*)key->data.scalar.value, name)) {
			return yaml_document_get_node(document, pair->value);
		}
		pair++;
	}
	return NULL;
}

void fxPopResult(txPool* pool) 
{
	pool->current = pool->current->parent;
}

#ifdef mxMultipleThreads
static int c = 0;

void fxPrintBusy(txPool* pool)
{
	fprintf(stderr, "\b\b\b\b\b\b\b\b# %6.6d", c++);	
}

void fxPrintClear(txPool* pool)
{
	fprintf(stderr, "\b\b\b\b\b\b\b\b");
	c++;
}
#endif

void fxPrintResult(txPool* pool, txResult* result, int c)
{
	int i = 0;
	while (i < c) {
		fprintf(stderr, "    ");
		i++;
	}
	fprintf(stderr, " %d/%d", result->successCount, result->testCount);
	if (result->pendingCount)
		fprintf(stderr, " (%d)", result->pendingCount);
	fprintf(stderr, " %s\n", result->path);
	result = result->first;
	c++;
	while (result) {
		fxPrintResult(pool, result, c);
		result = result->next;
	}
}

void fxPrintUsage()
{
	printf("xst [-h] [-e] [-j] [-m] [-s] [-t] [-u] [-v] strings...\n");
	printf("\t-h: print this help message\n");
	printf("\t-e: eval strings\n");
	printf("\t-j: strings are paths to JSON\n");
	printf("\t-m: strings are paths to modules\n");
	printf("\t-s: strings are paths to scripts\n");
	printf("\t-t: strings are paths to test262 cases or directories\n");
	printf("\t-u: print unhandled exceptions and rejections\n");
	printf("\t-v: print XS version\n");
	printf("without -e, -j, -m, -s, or -t:\n");
	printf("\tif ../harness exists, strings are paths to test262 cases or directories\n");
	printf("\telse if the extension is .mjs, strings are paths to modules\n");
	printf("\telse strings are paths to scripts\n");
#if FUZZILLI
	printf("\t-f: fuzz with REPRL harness\n");
 #endif
}

void fxPushResult(txPool* pool, char* path) 
{
	txResult* parent = pool->current;
	txResult* result = c_malloc(sizeof(txResult) + mxStringLength(path));
	if (!result) {
		c_exit(1);
	}
	result->next = NULL;
	result->parent = parent;
	result->first = NULL;
	result->last = NULL;
	result->testCount = 0;
	result->successCount = 0;
	result->pendingCount = 0;
	c_strcpy(result->path, path);
	if (parent) {
		if (parent->last)
			parent->last->next = result;
		else
			parent->first = result;
		parent->last = result;
	}
	pool->current = result;
}

void fxRunDirectory(txPool* pool, char* path)
{
	typedef struct sxEntry txEntry;
	struct sxEntry {
		txEntry* nextEntry;
		char name[1];
	};

#if mxWindows
	size_t length;
	HANDLE findHandle = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA findData;
	length = strlen(path);
	path[length] = '\\';
	length++;
	path[length] = '*';
	path[length + 1] = 0;
	findHandle = FindFirstFile(path, &findData);
	if (findHandle != INVALID_HANDLE_VALUE) {
		txEntry* entry;
		txEntry* firstEntry = NULL;
		txEntry* nextEntry;
		txEntry** address;
		do {
			if ((findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ||
				!strcmp(findData.cFileName, ".") ||
				!strcmp(findData.cFileName, ".."))
				continue;
			entry = malloc(sizeof(txEntry) + strlen(findData.cFileName));
			if (!entry)
				break;
			strcpy(entry->name, findData.cFileName);
			address = &firstEntry;
			while ((nextEntry = *address)) {
				if (strcmp(entry->name, nextEntry->name) < 0)
					break;
				address = &nextEntry->nextEntry;
			}
			entry->nextEntry = nextEntry;
			*address = entry;
		} while (FindNextFile(findHandle, &findData));
		FindClose(findHandle);
		while (firstEntry) {
			DWORD attributes;
			strcpy(path + length, firstEntry->name);
			attributes = GetFileAttributes(path);
			if (attributes != 0xFFFFFFFF) {
				if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
					fxPushResult(pool, firstEntry->name);
					fxRunDirectory(pool, path);
					fxPopResult(pool);
				}
				else if (fxStringEndsWith(path, ".js") && !fxStringEndsWith(path, "_FIXTURE.js"))
					fxRunFile(pool, path);
			}
			nextEntry = firstEntry->nextEntry;
			free(firstEntry);
			firstEntry = nextEntry;
		}
	}
#else
    DIR* dir;
	int length;
	dir = opendir(path);
	length = strlen(path);
	path[length] = '/';
	length++;
	if (dir) {
		struct dirent *ent;
		txEntry* entry;
		txEntry* firstEntry = NULL;
		txEntry* nextEntry;
		txEntry** address;
		while ((ent = readdir(dir))) {
			if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
				continue;
			entry = malloc(sizeof(txEntry) + strlen(ent->d_name));
			if (!entry)
				break;
			strcpy(entry->name, ent->d_name);
			address = &firstEntry;
			while ((nextEntry = *address)) {
				if (strcmp(entry->name, nextEntry->name) < 0)
					break;
				address = &nextEntry->nextEntry;
			}
			entry->nextEntry = nextEntry;
			*address = entry;
		}
		closedir(dir);
		while (firstEntry) {
			struct stat a_stat;
			strcpy(path + length, firstEntry->name);
			if (stat(path, &a_stat) == 0) {
				if (S_ISDIR(a_stat.st_mode)) {
					fxPushResult(pool, firstEntry->name);
					fxRunDirectory(pool, path);
					fxPopResult(pool);
				}
				else if (fxStringEndsWith(path, ".js") && !fxStringEndsWith(path, "_FIXTURE.js"))
					fxRunFile(pool, path);
			}
			nextEntry = firstEntry->nextEntry;
			free(firstEntry);
			firstEntry = nextEntry;
		}
	}
#endif
}

void fxRunFile(txPool* pool, char* path)
{
	txContext* context = c_malloc(sizeof(txContext) + c_strlen(path));
	txContext** address;
	txContext* former;
	if (!context) return;
	c_memset(context, 0, sizeof(txContext));
	context->result = pool->current;
	c_strcpy(context->path, path);
    fxLockMutex(&(pool->countMutex));
    while (pool->count == mxPoolSize)
		fxSleepCondition(&(pool->countCondition), &(pool->countMutex));
	address = &(pool->first);	
	while ((former = *address))
		address = &(former->next);
	*address = context;
	pool->count++;
	fxWakeAllCondition(&(pool->countCondition));
    fxUnlockMutex(&(pool->countMutex));
}

#if mxWindows
unsigned int __stdcall fxRunFileThread(void* it)
#else
void* fxRunFileThread(void* it)
#endif
{
	txPool* pool = it;
	txContext* context;
	for (;;) {
    	fxLockMutex(&(pool->countMutex));
    	while (pool->count == 0)
			fxSleepCondition(&(pool->countCondition), &(pool->countMutex));
		if (pool->count > 0) {
			context = pool->first;
			pool->first = context->next;
			pool->count--;
			fxWakeAllCondition(&(pool->countCondition));
			fxUnlockMutex(&(pool->countMutex));
			
			fxRunContext(pool, context);
			free(context);
		}
		else if (pool->count < 0) {
			fxUnlockMutex(&(pool->countMutex));
			break;
		}
	}
#if mxWindows
	return 0;
#else
	return NULL;
#endif
}

void fxRunContext(txPool* pool, txContext* context)
{
	txAgentCluster* agentCluster = &gxAgentCluster;
	char* path = context->path;
	FILE* file = NULL;
	size_t size;
	char* buffer = NULL;
	char* begin;
	char* end;
	yaml_parser_t _parser;
	yaml_parser_t* parser = NULL;
	yaml_document_t _document;
	yaml_document_t* document = NULL;
	yaml_node_t* root;
	yaml_node_t* value;
	int async = 0;
	int atomics = 0;
	int sloppy = 1;
	int strict = 1;
	int module = 0;
	int pending = 0;
	char message[1024];
	
	file = fopen(path, "rb");
	if (!file) goto bail;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	if (!size) goto bail;
	fseek(file, 0, SEEK_SET);
	buffer = malloc(size + 1);
	if (!buffer) goto bail;
	if (fread(buffer, 1, size, file) != size) goto bail;	
	buffer[size] = 0;
	fclose(file);
	file = NULL;
	
	begin = strstr(buffer, "/*---");
	if (!begin) goto bail;
	begin += 5;
	end = strstr(begin, "---*/");
	if (!end) goto bail;

	if (!yaml_parser_initialize(&_parser)) goto bail;
	parser = &_parser;
	yaml_parser_set_input_string(parser, (unsigned char *)begin, end - begin);
	if (!yaml_parser_load(parser, &_document)) goto bail;
	document = &_document;
	root = yaml_document_get_root_node(document);
	if (!root) goto bail;
		
	context->document = document;
	context->includes = fxGetMappingValue(document, root, "includes");
	value = fxGetMappingValue(document, root, "negative");
	if (value)
		context->negative = fxGetMappingValue(document, value, "type");
	else
		context->negative = NULL;
	
	value = fxGetMappingValue(document, root, "flags");
	if (value) {
		yaml_node_item_t* item = value->data.sequence.items.start;
		while (item < value->data.sequence.items.top) {
			yaml_node_t* node = yaml_document_get_node(document, *item);
			if (!strcmp((char*)node->data.scalar.value, "async")) {
				async = 1;
			}
			else if (!strcmp((char*)node->data.scalar.value, "onlyStrict")) {
				sloppy = 0;
				strict = 1;
				module = 0;
			}
			else if (!strcmp((char*)node->data.scalar.value, "noStrict")) {
				sloppy = 1;
				strict = 0;
				module = 0;
			}
			else if (!strcmp((char*)node->data.scalar.value, "module")) {
				sloppy = 0;
				strict = 0;
				module = 1;
			}
			else if (!strcmp((char*)node->data.scalar.value, "raw")) {
				sloppy = 1;
				strict = 0;
				module = 0;
			}
			else if (!strcmp((char*)node->data.scalar.value, "CanBlockIsTrue")) {
				sloppy = 0;
				strict = 0;
				module = 0;
				pending = 1;
			}
			item++;
		}
	}

	value = fxGetMappingValue(document, root, "features");
	if (value) {
		yaml_node_item_t* item = value->data.sequence.items.start;
		while (item < value->data.sequence.items.top) {
			yaml_node_t* node = yaml_document_get_node(document, *item);
			if (0
 			||	!strcmp((char*)node->data.scalar.value, "Atomics.waitAsync")
  			||	!strcmp((char*)node->data.scalar.value, "ShadowRealm")
 			||	!strcmp((char*)node->data.scalar.value, "Temporal")
 			||	!strcmp((char*)node->data.scalar.value, "arbitrary-module-namespace-names")
 			||	!strcmp((char*)node->data.scalar.value, "array-grouping")
 			||	!strcmp((char*)node->data.scalar.value, "decorators")
 			||	!strcmp((char*)node->data.scalar.value, "import-assertions")
 			||	!strcmp((char*)node->data.scalar.value, "json-modules")
#ifndef mxRegExpUnicodePropertyEscapes
 			||	!strcmp((char*)node->data.scalar.value, "regexp-unicode-property-escapes")
#endif
			||	!strcmp((char*)node->data.scalar.value, "regexp-v-flag")
			) {
				sloppy = 0;
				strict = 0;
				module = 0;
				pending = 1;
			}
			if (!strcmp((char*)node->data.scalar.value, "Atomics")) {
				atomics = 1;
			}
			item++;
		}
	}
	if (atomics) {
		fxLockMutex(&(agentCluster->mainMutex));
	}

	if (sloppy) {
#ifdef mxMultipleThreads
		if (!fxRunTestCase(pool, context, path, mxProgramFlag | mxDebugFlag, async, message)) {
			fxPrintClear(pool);
			fprintf(stderr, "### %s (sloppy): %s\n", path + pool->testPathLength, message);
		}
		else
			fxPrintBusy(pool);
#else
		fprintf(stderr, "### %s (sloppy): ", path + pool->testPathLength);
		fxRunTestCase(pool, context, path, mxProgramFlag | mxDebugFlag, async, message);
		fprintf(stderr, "%s\n", message);
#endif
	}
	if (strict) {
#ifdef mxMultipleThreads
		if (!fxRunTestCase(pool, context, path, mxProgramFlag | mxDebugFlag | mxStrictFlag, async, message)) {
			fxPrintClear(pool);
			fprintf(stderr, "### %s (strict): %s\n", path + pool->testPathLength, message);
		}
		else
			fxPrintBusy(pool);
#else
		fprintf(stderr, "### %s (strict): ", path + pool->testPathLength);
		fxRunTestCase(pool, context, path, mxProgramFlag | mxDebugFlag | mxStrictFlag, async, message);
		fprintf(stderr, "%s\n", message);
#endif
	}
	if (module) {
#ifdef mxMultipleThreads
		if (!fxRunTestCase(pool, context, path, 0, async, message)) {
			fxPrintClear(pool);
			fprintf(stderr, "### %s (module): %s\n", path + pool->testPathLength, message);
		}
		else
			fxPrintBusy(pool);
#else
		fprintf(stderr, "### %s (module): ", path + pool->testPathLength);
		fxRunTestCase(pool, context, path, 0, async, message);
		fprintf(stderr, "%s\n", message);
#endif
	}
	if (atomics) {
		fxUnlockMutex(&(agentCluster->mainMutex));
	}

	if (pending) {
		fxCountResult(pool, context, 0, 1);
#ifdef mxMultipleThreads
		fxPrintBusy(pool);
#else
		fprintf(stderr, "### %s: SKIP\n", path + pool->testPathLength);
#endif
	}
bail:	
	context->negative = NULL;
	context->includes = NULL;
	context->document = NULL;
	if (document)
		yaml_document_delete(document);
	if (parser)
		yaml_parser_delete(parser);
	if (buffer)
		free(buffer);
	if (file)
		fclose(file);
}

int fxRunTestCase(txPool* pool, txContext* context, char* path, txUnsigned flags, int async, char* message)
{
	xsCreation _creation = {
		16 * 1024 * 1024, 	/* initialChunkSize */
		16 * 1024 * 1024, 	/* incrementalChunkSize */
		1 * 1024 * 1024, 	/* initialHeapCount */
		1 * 1024 * 1024, 	/* incrementalHeapCount */
		256 * 1024, 		/* stackCount */
		256 * 1024, 		/* initialKeyCount */
		0,					/* incrementalKeyCount */
		1993, 				/* nameModulo */
		127,				/* symbolModulo */
		64 * 1024,			/* parserBufferSize */
		1993,				/* parserTableModulo */
	};
	xsCreation* creation = &_creation;
	xsMachine* machine;
	char buffer[C_PATH_MAX];
	int success = 0;	
	machine = xsCreateMachine(creation, "xst", NULL);
	xsBeginHost(machine);
	{
		xsVars(1);
		xsTry {
			fxBuildAgent(the);
			c_strcpy(buffer, pool->harnessPath);
			c_strcat(buffer, "sta.js");
			fxRunProgramFile(the, buffer, mxProgramFlag | mxDebugFlag);
			c_strcpy(buffer, pool->harnessPath);
			c_strcat(buffer, "assert.js");
			fxRunProgramFile(the, buffer, mxProgramFlag | mxDebugFlag);
			if (async) {
				xsResult = xsNewHostFunction(fx_done, 1);
				xsSet(xsGlobal, xsID("$DONE"), xsResult);
				xsVar(0) = xsString("Test did not run to completion");
			}
			else
				xsVar(0) = xsUndefined;
			if (context->includes) {
				yaml_node_item_t* item = context->includes->data.sequence.items.start;
				while (item < context->includes->data.sequence.items.top) {
					yaml_node_t* node = yaml_document_get_node(context->document, *item);
					c_strcpy(buffer, pool->harnessPath);
					c_strcat(buffer, (char*)node->data.scalar.value);
					fxRunProgramFile(the, buffer, mxProgramFlag | mxDebugFlag);
					item++;
				}
			}
			mxPop();
			the->rejection = &xsVar(0);
			if (flags)
				fxRunProgramFile(the, path, flags);
			else
				fxRunModuleFile(the, path);
			fxRunLoop(the);
			if (xsTest(xsVar(0))) 
				xsThrow(xsVar(0));
			if (context->negative) {
				snprintf(message, 1024, "# Expected a %s but got no errors", context->negative->data.scalar.value);
			}
			else {
				snprintf(message, 1024, "OK");
				success = 1;
			}
		}
		xsCatch {
			if (context->negative) {
				txString name;
				xsResult = xsGet(xsException, xsID("constructor"));
				name = xsToString(xsGet(xsResult, xsID("name")));
				if (strcmp(name, (char*)context->negative->data.scalar.value))
					snprintf(message, 1024, "# Expected a %s but got a %s", context->negative->data.scalar.value, name);
				else {
					snprintf(message, 1024, "OK");
					success = 1;
				}
			}
			else {
				snprintf(message, 1024, "# %s", xsToString(xsException));
			}
			xsResult = xsGet(xsGlobal, xsID("$262"));
			xsResult = xsGet(xsResult, xsID("agent"));
			xsCall0(xsResult, xsID("stop"));
		}
	}
	xsEndHost(machine);
	if (machine->abortStatus) {
		success = 0;
		if ((machine->abortStatus == XS_NOT_ENOUGH_MEMORY_EXIT) || (machine->abortStatus == XS_STACK_OVERFLOW_EXIT)) {
			if (context->negative) {
				if (!strcmp("RangeError", (char*)context->negative->data.scalar.value)) {
					snprintf(message, 1024, "OK");
					success = 1;
				}
			}
		}
		if (!success) {
			char *why = (machine->abortStatus <= XS_UNHANDLED_REJECTION_EXIT) ? gxAbortStrings[machine->abortStatus] : "unknown";
			snprintf(message, 1024, "# %s", why);
			success = 0;
		}
	}
	fx_agent_stop(machine);
	xsDeleteMachine(machine);

	fxCountResult(pool, context, success, 0);
	return success;
}

int fxStringEndsWith(const char *string, const char *suffix)
{
	size_t stringLength = strlen(string);
	size_t suffixLength = strlen(suffix);
	return (stringLength >= suffixLength) && (0 == strcmp(string + (stringLength - suffixLength), suffix));
}

/* $262 */

void fx_agent_get_safeBroadcast(xsMachine* the)
{
	xsResult = xsGet(xsThis, xsID("broadcast"));
}

void fx_agent_set_safeBroadcast(xsMachine* the)
{
}

void fx_agent_broadcast(xsMachine* the)
{
	txAgentCluster* agentCluster = &gxAgentCluster;
	if (xsIsInstanceOf(xsArg(0), xsTypedArrayPrototype)) {
		xsArg(0) = xsGet(xsArg(0), xsID("buffer"));
	}
    fxLockMutex(&(agentCluster->dataMutex));
	agentCluster->dataBuffer = xsMarshallAlien(xsArg(0));
	if (mxArgc > 1)
		agentCluster->dataValue = xsToInteger(xsArg(1));
	fxWakeAllCondition(&(agentCluster->dataCondition));
    fxUnlockMutex(&(agentCluster->dataMutex));
    
    fxLockMutex(&(agentCluster->countMutex));
    while (agentCluster->count > 0)
		fxSleepCondition(&(agentCluster->countCondition), &(agentCluster->countMutex));
    fxUnlockMutex(&(agentCluster->countMutex));
}

void fx_agent_getReport(xsMachine* the)
{
	txAgentCluster* agentCluster = &gxAgentCluster;
	txAgentReport* report = C_NULL;
    fxLockMutex(&(agentCluster->reportMutex));
	report = agentCluster->firstReport;
	if (report)
		agentCluster->firstReport = report->next;
    fxUnlockMutex(&(agentCluster->reportMutex));
    if (report) {
    	xsResult = xsString(report->message);
    	c_free(report);
    }
    else
    	xsResult = xsNull;
}

void fx_agent_leaving(xsMachine* the)
{
}

void fx_agent_monotonicNow(xsMachine* the)
{
	xsResult = xsNumber(fxDateNow());
// #if mxWindows
//     xsResult = xsNumber((txNumber)GetTickCount64());
// #else	
// 	struct timespec now;
// 	clock_gettime(CLOCK_MONOTONIC, &now);
//     xsResult = xsNumber(((txNumber)(now.tv_sec) * 1000.0) + ((txNumber)(now.tv_nsec / 1000000)));
// #endif
}

void fx_agent_receiveBroadcast(xsMachine* the)
{
 	txAgentCluster* agentCluster = &gxAgentCluster;
   fxLockMutex(&(agentCluster->dataMutex));
	while (agentCluster->dataBuffer == NULL)
		fxSleepCondition(&(agentCluster->dataCondition), &(agentCluster->dataMutex));
	xsResult = xsDemarshallAlien(agentCluster->dataBuffer);
    fxUnlockMutex(&(agentCluster->dataMutex));
	
    fxLockMutex(&(agentCluster->countMutex));
    agentCluster->count--;
    fxWakeCondition(&(agentCluster->countCondition));
    fxUnlockMutex(&(agentCluster->countMutex));
		
	xsCallFunction2(xsArg(0), xsGlobal, xsResult, xsInteger(agentCluster->dataValue));
}

void fx_agent_report(xsMachine* the)
{
	txAgentCluster* agentCluster = &gxAgentCluster;
	xsStringValue message = xsToString(xsArg(0));
	xsIntegerValue messageLength = mxStringLength(message);
	txAgentReport* report = c_malloc(sizeof(txAgentReport) + messageLength);
	if (!report) xsUnknownError("not enough memory");
    report->next = C_NULL;
	c_memcpy(&(report->message[0]), message, messageLength + 1);
    fxLockMutex(&(agentCluster->reportMutex));
    if (agentCluster->firstReport)
		agentCluster->lastReport->next = report;
    else
		agentCluster->firstReport = report;
	agentCluster->lastReport = report;
    fxUnlockMutex(&(agentCluster->reportMutex));
}

void fx_agent_sleep(xsMachine* the)
{
	xsIntegerValue delay = xsToInteger(xsArg(0));
#if mxWindows
	Sleep(delay);
#else	
	usleep(delay * 1000);
#endif
}

void fx_agent_start(xsMachine* the)
{
	xsStringValue script = xsToString(xsArg(0));
	xsIntegerValue scriptLength = mxStringLength(script);
	txAgentCluster* agentCluster = &gxAgentCluster;
	txAgent* agent = c_malloc(sizeof(txAgent) + scriptLength);
	if (!agent) xsUnknownError("not enough memory");
	c_memset(agent, 0, sizeof(txAgent));
	if (agentCluster->firstAgent)
		agentCluster->lastAgent->next = agent;
	else
		agentCluster->firstAgent = agent;
	agentCluster->lastAgent = agent;
	agentCluster->count++;
	agent->scriptLength = scriptLength;
	c_memcpy(&(agent->script[0]), script, scriptLength + 1);
#if mxWindows
	agent->thread = (HANDLE)_beginthreadex(NULL, 0, fx_agent_start_aux, agent, 0, NULL);
#else	
    pthread_create(&(agent->thread), NULL, &fx_agent_start_aux, agent);
#endif
}

#if mxWindows
unsigned int __stdcall fx_agent_start_aux(void* it)
#else
void* fx_agent_start_aux(void* it)
#endif
{
	xsCreation creation = {
		16 * 1024 * 1024, 	/* initialChunkSize */
		16 * 1024 * 1024, 	/* incrementalChunkSize */
		1 * 1024 * 1024, 	/* initialHeapCount */
		1 * 1024 * 1024, 	/* incrementalHeapCount */
		4096, 				/* stackCount */
		4096*3, 			/* initialKeyCount */
		0,					/* incrementalKeyCount */
		1993, 				/* nameModulo */
		127, 				/* symbolModulo */
		64 * 1024,			/* parserBufferSize */
		1993,				/* parserTableModulo */
	};
	txAgent* agent = it;
	xsMachine* machine = xsCreateMachine(&creation, "xst-agent", NULL);
	xsBeginHost(machine);
	{
		xsTry {
			txSlot* slot;
			txSlot* global;
			txStringCStream stream;
			
			mxPush(mxGlobal);
			global = the->stack;
			
			slot = fxLastProperty(the, fxNewHostObject(the, NULL));
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_leaving, 0, xsID("leaving"), XS_DONT_ENUM_FLAG); 
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_monotonicNow, 0, xsID("monotonicNow"), XS_DONT_ENUM_FLAG); 
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_receiveBroadcast, 1, xsID("receiveBroadcast"), XS_DONT_ENUM_FLAG); 
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_report, 1, xsID("report"), XS_DONT_ENUM_FLAG); 
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_sleep, 1, xsID("sleep"), XS_DONT_ENUM_FLAG); 
			fxSetHostData(the, the->stack, agent);
			
			mxPush(mxObjectPrototype);
			slot = fxLastProperty(the, fxNewObjectInstance(the));
			slot = fxNextSlotProperty(the, slot, the->stack + 1, xsID("agent"), XS_GET_ONLY);
			
			slot = fxLastProperty(the, fxToInstance(the, global));
			slot = fxNextSlotProperty(the, slot, the->stack, xsID("$262"), XS_GET_ONLY);
			
			mxPop();
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
	xsEndHost(the);
	xsDeleteMachine(machine);
#if mxWindows
	return 0;
#else
	return NULL;
#endif
}

void fx_agent_stop(xsMachine* the)
{
	txAgentCluster* agentCluster = &gxAgentCluster;
	txAgent* agent = agentCluster->firstAgent;
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
		if (agentCluster->dataBuffer)
			c_free(agentCluster->dataBuffer);
		agentCluster->firstAgent = C_NULL;
		agentCluster->lastAgent = C_NULL;
		agentCluster->count = 0;
		agentCluster->dataBuffer = C_NULL;
		agentCluster->dataValue = 0;
		agentCluster->firstReport = C_NULL;
		agentCluster->lastReport = C_NULL;
	}
}

void fx_createRealm(xsMachine* the)
{
	xsResult = xsThis;
}

void fx_detachArrayBuffer(xsMachine* the)
{
	txSlot* slot = mxArgv(0);
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_ARRAY_BUFFER_KIND) && (instance != mxArrayBufferPrototype.value.reference)) {
			slot->value.arrayBuffer.address = C_NULL;
			slot->next->value.bufferInfo.length = 0;
			return;
		}
	}
	mxTypeError("this is no ArrayBuffer instance");
}

void fx_done(xsMachine* the)
{
	if ((xsToInteger(xsArgc) > 0) && (xsTest(xsArg(0))))
		*((txSlot*)the->rejection) = xsArg(0);
	else
		*((txSlot*)the->rejection) = xsUndefined;
}

void fx_gc(xsMachine* the)
{
#if !FUZZING
	xsCollectGarbage();
#else
	extern int gxStress;
	xsResult = xsInteger(gxStress);

	xsIntegerValue c = xsToInteger(xsArgc);
	if (!c) {
		xsCollectGarbage();
		return;
	}
	
	int count = xsToInteger(xsArg(0));
	gxStress = (count < 0) ? count : -count;
#endif
}

void fx_evalScript(xsMachine* the)
{
	txSlot* realm = mxProgram.value.reference->next->value.module.realm;
	txStringStream aStream;
	aStream.slot = mxArgv(0);
	aStream.offset = 0;
	aStream.size = mxStringLength(fxToString(the, mxArgv(0)));
	fxRunScript(the, fxParseScript(the, &aStream, fxStringGetter, mxProgramFlag | mxDebugFlag), mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, mxProgram.value.reference);
	mxPullSlot(mxResult);
}

void fx_print(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	xsStringValue string, p, q;
	xsVars(1);
	xsVar(0) = xsGet(xsGlobal, xsID("String"));
	for (i = 0; i < c; i++) {
		if (i)
			fprintf(stdout, " ");
		xsArg(i) = xsCallFunction1(xsVar(0), xsUndefined, xsArg(i));
		p = string = xsToString(xsArg(i));
	#if mxCESU8
		for (;;) {
			xsIntegerValue character;
			q = fxUTF8Decode(p, &character);
		again:
			if (character == C_EOF)
				break;
			if (character == 0) {
				if (p > string) {
					char c = *p;
					*p = 0;
					fprintf(stdout, "%s", string);
					*p = c;
				}
				string = q;
			}
			else if ((0x0000D800 <= character) && (character <= 0x0000DBFF)) {
				xsStringValue r = q;
				xsIntegerValue surrogate;
				q = fxUTF8Decode(r, &surrogate);
				if ((0x0000DC00 <= surrogate) && (surrogate <= 0x0000DFFF)) {
					char buffer[5];
					character = (txInteger)(0x00010000 + ((character & 0x03FF) << 10) + (surrogate & 0x03FF));
					if (p > string) {
						char c = *p;
						*p = 0;
						fprintf(stdout, "%s", string);
						*p = c;
					}
					p = fxUTF8Encode(buffer, character);
					*p = 0;
					fprintf(stdout, "%s", buffer);
					string = q;
				}
				else {
					p = r;
					character = surrogate;
					goto again;
				}
			}
			p = q;
		}
	#endif	
		fprintf(stdout, "%s", string);
	}
	fprintf(stdout, "\n");
}

/* TIMER */

static txHostHooks gxTimerHooks = {
	fx_destroyTimer,
	fx_markTimer
};

void fx_clearTimer(txMachine* the)
{
	txHostHooks* hooks = fxGetHostHooks(the, mxArgv(0));
	if (hooks == &gxTimerHooks) {
		txJob* job = fxGetHostData(the, mxArgv(0));
		if (job) {
			fxForget(the, &job->self);
			fxSetHostData(the, mxArgv(0), NULL);
			job->the = NULL;
		}
	}
	else
		mxTypeError("no timer");
}

void fx_destroyTimer(void* data)
{
}

void fx_markTimer(txMachine* the, void* it, txMarkRoot markRoot)
{
	txJob* job = it;
	if (job) {
		(*markRoot)(the, &job->function);
		(*markRoot)(the, &job->argument);
	}
}

void fx_setInterval(txMachine* the)
{
	fx_setTimer(the, fxToNumber(the, mxArgv(1)), 1);
}

void fx_setTimeout(txMachine* the)
{
	fx_setTimer(the, fxToNumber(the, mxArgv(1)), 0);
}

void fx_setTimer(txMachine* the, txNumber interval, txBoolean repeat)
{
	c_timeval tv;
	txJob* job;
	txJob** address = (txJob**)&(the->timerJobs);
	while ((job = *address))
		address = &(job->next);
	job = *address = malloc(sizeof(txJob));
	c_memset(job, 0, sizeof(txJob));
	job->the = the;
	c_gettimeofday(&tv, NULL);
	if (repeat)
		job->interval = interval;
	job->when = ((txNumber)(tv.tv_sec) * 1000.0) + ((txNumber)(tv.tv_usec) / 1000.0) + interval;
	fxNewHostObject(the, NULL);
    mxPull(job->self);
	job->function = *mxArgv(0);
	if (mxArgc > 2)
		job->argument = *mxArgv(2);
	else
		job->argument = mxUndefined;
	fxSetHostData(the, &job->self, job);
	fxSetHostHooks(the, &job->self, &gxTimerHooks);
	fxRemember(the, &job->self);
	fxAccess(the, &job->self);
	*mxResult = the->scratch;
}

/* FUZZILLI */

#if FUZZILLI
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#define SHM_SIZE 0x100000
#define MAX_EDGES ((SHM_SIZE - 4) * 8)

struct shmem_data {
	uint32_t num_edges;
	unsigned char edges[];
};

struct shmem_data* __shmem;
uint32_t *__edges_start, *__edges_stop;

void __sanitizer_cov_reset_edgeguards()
{
	uint64_t N = 0;
	for (uint32_t *x = __edges_start; x < __edges_stop && N < MAX_EDGES; x++)
		*x = ++N;
}
#ifndef __linux__
void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop)
{
	// Avoid duplicate initialization
	if (start == stop || *start)
		return;

	if (__edges_start != NULL || __edges_stop != NULL) {
		fprintf(stderr, "Coverage instrumentation is only supported for a single module\n");
		c_exit(-1);
	}

	__edges_start = start;
	__edges_stop = stop;

	// Map the shared memory region
	const char* shm_key = getenv("SHM_ID");
	if (!shm_key) {
		puts("[COV] no shared memory bitmap available, skipping");
		__shmem = (struct shmem_data*) malloc(SHM_SIZE);
	} else {
		int fd = shm_open(shm_key, O_RDWR, S_IREAD | S_IWRITE);
		if (fd <= -1) {
			fprintf(stderr, "Failed to open shared memory region: %s\n", strerror(errno));
			c_exit(-1);
		}

		__shmem = (struct shmem_data*) mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (__shmem == MAP_FAILED) {
			fprintf(stderr, "Failed to mmap shared memory region\n");
			c_exit(-1);
		}
	}

	__sanitizer_cov_reset_edgeguards();

	__shmem->num_edges = stop - start;
	printf("[COV] edge counters initialized. Shared memory: %s with %u edges\n", shm_key, __shmem->num_edges);
}

void __sanitizer_cov_trace_pc_guard(uint32_t *guard)
{
	// There's a small race condition here: if this function executes in two threads for the same
	// edge at the same time, the first thread might disable the edge (by setting the guard to zero)
	// before the second thread fetches the guard value (and thus the index). However, our
	// instrumentation ignores the first edge (see libcoverage.c) and so the race is unproblematic.
	uint32_t index = *guard;
	// If this function is called before coverage instrumentation is properly initialized we want to return early.
	if (!index) return;
	__shmem->edges[index / 8] |= 1 << (index % 8);
	*guard = 0;
}
#endif

#define REPRL_CRFD 100
#define REPRL_CWFD 101
#define REPRL_DRFD 102
#define REPRL_DWFD 103

void fx_fuzzilli(xsMachine* the)
{
	const char* str = xsToString(xsArg(0));
	if (!strcmp(str, "FUZZILLI_CRASH")) {
 		switch (xsToInteger(xsArg(1))) {
 			case 0:
				// check crash
				*((volatile char *)0) = 0;
				break;
 			case 1: {
				// check ASAN
				char *data = malloc(64);
				free(data);
				data[0]++;
				} break;
 			case 2:
				// check assert
				assert(0);
				break;
		}
 	}
	else if (!strcmp(str, "FUZZILLI_PRINT")) {
		const char* print_str = xsToString(xsArg(1));
		FILE* fzliout = fdopen(REPRL_DWFD, "w");
		if (!fzliout) {
			fprintf(stderr, "Fuzzer output channel not available, printing to stdout instead\n");
			fzliout = stdout;
		}
		fprintf(fzliout, "%s\n", print_str);
		fflush(fzliout);
	}
}

int fuzz(int argc, char* argv[])
{
	char helo[] = "HELO";
	if (4 != write(REPRL_CWFD, helo, 4)) {
		fprintf(stderr, "Error writing HELO\n");
		c_exit(-1);
	}
	if (4 != read(REPRL_CRFD, helo, 4)) {
		fprintf(stderr, "Error reading HELO\n");
		c_exit(-1);
	}
	if (0 != memcmp(helo, "HELO", 4)) {
		fprintf(stderr, "Invalid response from parent\n");
		c_exit(-1);
	}
	xsCreation _creation = {
		1 * 1024 * 1024, 	/* initialChunkSize */
		1 * 1024 * 1024, 	/* incrementalChunkSize */
		32768, 				/* initialHeapCount */
		32768,			 	/* incrementalHeapCount */
		64 * 1024,	 		/* stackCount */
		8 * 1024,			/* initialKeyCount */
		0,					/* incrementalKeyCount */
		1993, 				/* nameModulo */
		127, 				/* symbolModulo */
		64 * 1024,			/* parserBufferSize */
		1993,				/* parserTableModulo */
	};

	fxInitializeSharedCluster();

	while (1) {
		int error = 0;
		char action[4];
		ssize_t nread = read(REPRL_CRFD, action, 4);
		fflush(0);		//@@
		if (nread != 4 || memcmp(action, "exec", 4) != 0) {
			fprintf(stderr, "Unknown action: %s\n", action);
			c_exit(-1);
		}

		size_t script_size = 0;
		read(REPRL_CRFD, &script_size, 8);

		ssize_t remaining = (ssize_t)script_size;
		char* buffer = (char *)malloc(script_size + 1);
		ssize_t rv = read(REPRL_DRFD, buffer, (size_t) remaining);
		if (rv <= 0) {
			fprintf(stderr, "Failed to load script\n");
			c_exit(-1);
		}
		buffer[script_size] = 0;	// required when debugger active

		xsMachine* machine = xsCreateMachine(&_creation, "xst", NULL);
		xsBeginMetering(machine, xsAlwaysWithinComputeLimit, 0x7FFFFFFF);
		{
		xsBeginHost(machine);
		{
			xsTry {
				xsVars(1);

				// hardened javascript
				xsResult = xsNewHostFunction(fx_harden, 1);
				xsDefine(xsGlobal, xsID("harden"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_lockdown, 0);
				xsDefine(xsGlobal, xsID("lockdown"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_petrify, 1);
				xsDefine(xsGlobal, xsID("petrify"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_mutabilities, 1);
				xsDefine(xsGlobal, xsID("mutabilities"), xsResult, xsDontEnum);

				// fuzzilli
				xsResult = xsNewHostFunction(fx_fuzzilli, 2);
				xsSet(xsGlobal, xsID("fuzzilli"), xsResult);
				xsResult = xsNewHostFunction(fx_gc, 0);
				xsSet(xsGlobal, xsID("gc"), xsResult);
				xsResult = xsNewHostFunction(fx_print, 1);
				xsSet(xsGlobal, xsID("print"), xsResult);
				xsResult = xsNewHostFunction(fx_fillBuffer, 2);
				xsSet(xsGlobal, xsID("fillBuffer"), xsResult);

				txSlot* realm = mxProgram.value.reference->next->value.module.realm;
				txStringCStream aStream;
				aStream.buffer = buffer;
				aStream.offset = 0;
				aStream.size = script_size;
				the->script = fxParseScript(the, &aStream, fxStringCGetter, mxProgramFlag | mxDebugFlag);
				fxRunScript(the, the->script, mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, mxProgram.value.reference);
				the->script = NULL;
				mxPullSlot(mxResult);

				fxRunLoop(the);
			}
			xsCatch {
				the->script = NULL;
				error = 1;
			}
		}
		fxCheckUnhandledRejections(machine, 1);
		xsEndHost(machine);
		}
		xsEndMetering(machine);
		fxDeleteScript(machine->script);
		int status = (machine->abortStatus & 0xff) << 8;
		if (!status && error)
			status = XS_UNHANDLED_EXCEPTION_EXIT << 8;
		if (write(REPRL_CWFD, &status, 4) != 4) {
			fprintf(stderr, "Erroring writing return value over REPRL_CWFD\n");
			exit(-1);
		}

		xsDeleteMachine(machine);

		free(buffer);

		__sanitizer_cov_reset_edgeguards();
	}

	fxTerminateSharedCluster();

	return 0;
}
#endif 
#if OSSFUZZ

#if mxMetering
#ifndef mxFuzzMeter
	// highest rate for test262 corpus was 2147483800
	#define mxFuzzMeter (214748380)
#endif

static int lsan_disabled;

// allow toggling ASAN leak-checking
__attribute__((used)) int __lsan_is_turned_off()
{
	return lsan_disabled;
}

static xsBooleanValue xsWithinComputeLimit(xsMachine* machine, xsUnsignedValue index)
{
	// may be useful to print current index for debugging
//	fprintf(stderr, "Current index: %u\n", index);
	if (index > mxFuzzMeter) {
//		fprintf(stderr, "Computation limits reached (index %u). Exiting...\n", index);
		return 0;
	}
	return 1;
}
#endif

int fuzz_oss(const uint8_t *Data, size_t script_size)
{
	lsan_disabled = 0;

	xsCreation _creation = {
		1 * 1024 * 1024, 	/* initialChunkSize */
		1 * 1024 * 1024, 	/* incrementalChunkSize */
		32768, 				/* initialHeapCount */
		32768,			 	/* incrementalHeapCount */
		64 * 1024,	 		/* stackCount */
		8 * 1024,			/* initialKeyCount */
		0,					/* incrementalKeyCount */
		1993, 				/* nameModulo */
		127, 				/* symbolModulo */
		64 * 1024,			/* parserBufferSize */
		1993,				/* parserTableModulo */
	};
	size_t buffer_size = script_size + script_size + script_size + 1;			// (massively) over-allocate to have space if UTF-8 encoding expands (1 byte invalid byte becomes a 3-byte UTF-8 sequence)
	char* buffer = (char *)malloc(buffer_size);
	memcpy(buffer, Data, script_size);

	buffer[script_size] = 0;	// required when debugger active

	xsCreation* creation = &_creation;
	xsMachine* machine;
	fxInitializeSharedCluster();
	machine = xsCreateMachine(creation, "xst", NULL);

	xsBeginMetering(machine, xsWithinComputeLimit, 1);
	{
		xsBeginHost(machine);
		{
			xsTry {
				xsVars(2);
				modInstallTextDecoder(the);
				xsResult = xsArrayBuffer(buffer, script_size);
				xsVar(0) = xsNew0(xsGlobal, xsID("TextDecoder"));
				xsResult = xsCall1(xsVar(0), xsID("decode"), xsResult);
	#ifdef OSSFUZZ_JSONPARSE
				xsVar(0) = xsGet(xsGlobal, xsID("JSON"));
				xsResult = xsCall1(xsVar(0), xsID("parse"), xsResult);
	#else
				xsToStringBuffer(xsResult, buffer, buffer_size);

				// hardened javascript
				xsResult = xsNewHostFunction(fx_harden, 1);
				xsDefine(xsGlobal, xsID("harden"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_lockdown, 0);
				xsDefine(xsGlobal, xsID("lockdown"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_petrify, 1);
				xsDefine(xsGlobal, xsID("petrify"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_mutabilities, 1);
				xsDefine(xsGlobal, xsID("mutabilities"), xsResult, xsDontEnum);

				xsResult = xsNewHostFunction(fx_gc, 0);
				xsSet(xsGlobal, xsID("gc"), xsResult);
				xsResult = xsNewHostFunction(fx_print, 1);
				xsSet(xsGlobal, xsID("print"), xsResult);

				// test262 stubs
				xsVar(0) = xsNewHostFunction(fx_nop, 1);
				xsDefine(xsGlobal, xsID("assert"), xsVar(0), xsDontEnum);
				xsDefine(xsVar(0), xsID("sameValue"), xsVar(0), xsDontEnum);
				xsDefine(xsVar(0), xsID("notSameValue"), xsVar(0), xsDontEnum);
				xsVar(1) = xsNewHostFunction(fx_assert_throws, 1);
				xsDefine(xsVar(0), xsID("throws"), xsVar(1), xsDontEnum);
				
				txStringCStream aStream;
				aStream.buffer = buffer;
				aStream.offset = 0;
				aStream.size = strlen(buffer);
				// run script
				txSlot* realm = mxProgram.value.reference->next->value.module.realm;
				the->script = fxParseScript(the, &aStream, fxStringCGetter, mxProgramFlag | mxDebugFlag);
				fxRunScript(the, the->script, mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, mxProgram.value.reference);
				the->script = NULL;
				mxPullSlot(mxResult);
				fxRunLoop(the);
	#endif
			}
			xsCatch {
				the->script = NULL;
			}
		}
		xsEndHost(machine);
	}
	xsEndMetering(machine);
	fxDeleteScript(machine->script);
	xsDeleteMachine(machine);
	fxTerminateSharedCluster();
	free(buffer);
	return 0;
}

#endif 

#if 1 || FUZZING || FUZZILLI

void fx_fillBuffer(txMachine *the)
{
	xsIntegerValue seed = xsToInteger(xsArg(1));
	xsIntegerValue length = xsGetArrayBufferLength(xsArg(0)), i;
	uint8_t *buffer = xsToArrayBuffer(xsArg(0));
	
	for (i = 0; i < length; i++) {
		seed = (uint64_t)seed * 48271 % 0x7fffffff;
		*buffer++ = (uint8_t)seed;
	}
}

void fx_nop(xsMachine *the)
{
}

void fx_assert_throws(xsMachine *the)
{
	mxTry(the) {
		if (xsToInteger(xsArgc) >= 2)
			xsCallFunction0(xsArg(1), xsGlobal);
	}
	mxCatch(the) {
	}
}

#endif

/* PLATFORM */

void fxCreateMachinePlatform(txMachine* the)
{
#ifdef mxDebug
	the->connection = mxNoSocket;
#endif
}

void fxDeleteMachinePlatform(txMachine* the)
{
}

void fxQueuePromiseJobs(txMachine* the)
{
	the->promiseJobs = 1;
}

void fxRunLoop(txMachine* the)
{
	c_timeval tv;
	txNumber when;
	txJob* job;
	txJob** address;
	
	for (;;) {
		fxEndJob(the);
		while (the->promiseJobs) {
			while (the->promiseJobs) {
				the->promiseJobs = 0;
				fxRunPromiseJobs(the);
			}
			fxEndJob(the);
		}
		c_gettimeofday(&tv, NULL);
		when = ((txNumber)(tv.tv_sec) * 1000.0) + ((txNumber)(tv.tv_usec) / 1000.0);
		address = (txJob**)&(the->timerJobs);
		if (!*address)
			break;
		while ((job = *address)) {
			txMachine* the = job->the;
			if (the) {
				if (job->when <= when) {
					fxBeginHost(the);
					mxTry(the) {
						mxPushUndefined();
						mxPush(job->function);
						mxCall();
						mxPush(job->argument);
						mxRunCount(1);
						mxPop();
						if (job->the) {
							if (job->interval) {
								job->when += job->interval;
							}
							else {
								fxAccess(the, &job->self);
								*mxResult = the->scratch;
								fxForget(the, &job->self);
								fxSetHostData(the, mxResult, NULL);
								job->the = NULL;
							}
						}
					}
					mxCatch(the) {
						*((txSlot*)the->rejection) = mxException;
						fxAccess(the, &job->self);
						*mxResult = the->scratch;
						fxForget(the, &job->self);
						fxSetHostData(the, mxResult, NULL);
						job->the = NULL;
					}
					fxEndHost(the);
					break; // to run promise jobs queued by the timer in the same "tick"
				}
				address = &(job->next);
			}
			else {
				*address = job->next;
				c_free(job);
			}
		}
	}
}

void fxFulfillModuleFile(txMachine* the)
{
}

void fxRejectModuleFile(txMachine* the)
{
	*((txSlot*)the->rejection) = xsArg(0);
}

void fxRunModuleFile(txMachine* the, txString path)
{
	txSlot* realm = mxProgram.value.reference->next->value.module.realm;
	mxPushStringC(path);
	fxRunImport(the, realm, XS_NO_ID);
	mxDub();
	fxGetID(the, mxID(_then));
	mxCall();
	fxNewHostFunction(the, fxFulfillModuleFile, 1, XS_NO_ID, XS_NO_ID);
	fxNewHostFunction(the, fxRejectModuleFile, 1, XS_NO_ID, XS_NO_ID);
	mxRunCount(2);
	mxPop();
}

void fxRunProgramFile(txMachine* the, txString path, txUnsigned flags)
{
	txSlot* realm = mxProgram.value.reference->next->value.module.realm;
	txScript* script = fxLoadScript(the, path, flags);
	mxModuleInstanceInternal(mxProgram.value.reference)->value.module.id = fxID(the, path);
	fxRunScript(the, script, mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, mxProgram.value.reference);
	mxPullSlot(mxResult);
}

void fxAbort(txMachine* the, int status)
{
	if (XS_DEBUGGER_EXIT == status)
		c_exit(1);
	if (the->abortStatus) // xsEndHost calls fxAbort!
		return;
		
	the->abortStatus = status;
 #if OSSFUZZ
	lsan_disabled = 1;		// disable leak checking
 #endif

	fxExitToHost(the);
}

txID fxFindModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* slot)
{
	char name[C_PATH_MAX];
	char path[C_PATH_MAX];
	txInteger dot = 0;
	txString slash;
	fxToStringBuffer(the, slot, name, sizeof(name));
	if (name[0] == '.') {
		if (name[1] == '/') {
			dot = 1;
		}
		else if ((name[1] == '.') && (name[2] == '/')) {
			dot = 2;
		}
	}
	if (dot) {
		if (moduleID == XS_NO_ID)
			return XS_NO_ID;
		c_strncpy(path, fxGetKeyName(the, moduleID), C_PATH_MAX - 1);
		path[C_PATH_MAX - 1] = 0;
		slash = c_strrchr(path, mxSeparator);
		if (!slash)
			return XS_NO_ID;
		if (dot == 2) {
			*slash = 0;
			slash = c_strrchr(path, mxSeparator);
			if (!slash)
				return XS_NO_ID;
		}
#if mxWindows
		{
			char c;
			char* s = name;
			while ((c = *s)) {
				if (c == '/')
					*s = '\\';
				s++;
			}
		}
#endif
	}
	else
		slash = path;
	*slash = 0;
	if ((c_strlen(path) + c_strlen(name + dot)) >= sizeof(path))
		xsRangeError("path too long");
	c_strcat(path, name + dot);
	return fxNewNameC(the, path);
}

void fxLoadModule(txMachine* the, txSlot* module, txID moduleID)
{
	char path[C_PATH_MAX];
	char real[C_PATH_MAX];
	txScript* script;
#ifdef mxDebug
	txUnsigned flags = mxDebugFlag;
#else
	txUnsigned flags = 0;
#endif
	c_strncpy(path, fxGetKeyName(the, moduleID), C_PATH_MAX - 1);
	path[C_PATH_MAX - 1] = 0;
	if (c_realpath(path, real)) {
		script = fxLoadScript(the, real, flags);
		if (script)
			fxResolveModule(the, module, moduleID, script, C_NULL, C_NULL);
	}
}

txScript* fxLoadScript(txMachine* the, txString path, txUnsigned flags)
{
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	FILE* file = NULL;
	txString name = NULL;
	char map[C_PATH_MAX];
	txScript* script = NULL;
	fxInitializeParser(parser, the, the->parserBufferSize, the->parserTableModulo);
	parser->firstJump = &jump;
	file = fopen(path, "r");
	if (c_setjmp(jump.jmp_buf) == 0) {
		mxParserThrowElse(file);
		parser->path = fxNewParserSymbol(parser, path);
		fxParserTree(parser, file, (txGetter)fgetc, flags, &name);
		fclose(file);
		file = NULL;
		if (name) {
			mxParserThrowElse(c_realpath(fxCombinePath(parser, path, name), map));
			parser->path = fxNewParserSymbol(parser, map);
			file = fopen(map, "r");
			mxParserThrowElse(file);
			fxParserSourceMap(parser, file, (txGetter)fgetc, flags, &name);
			fclose(file);
			file = NULL;
			if ((parser->errorCount == 0) && name) {
				mxParserThrowElse(c_realpath(fxCombinePath(parser, map, name), map));
				parser->path = fxNewParserSymbol(parser, map);
			}
		}
		fxParserHoist(parser);
		fxParserBind(parser);
		script = fxParserCode(parser);
	}
	if (file)
		fclose(file);
#ifdef mxInstrument
	if (the->peakParserSize < parser->total)
		the->peakParserSize = parser->total;
#endif
	fxTerminateParser(parser);
	return script;
}

/* DEBUG */

#ifdef mxDebug

void fxConnect(txMachine* the)
{
#if FUZZING || FUZZILLI || OSSFUZZ
	return;
#endif
#ifdef mxMultipleThreads
#else
	char name[256];
	char* colon;
	int port;
	struct sockaddr_in address;
#if mxWindows
	if (GetEnvironmentVariable("XSBUG_HOST", name, sizeof(name))) {
#else
	colon = getenv("XSBUG_HOST");
	if ((colon) && (c_strlen(colon) + 1 < sizeof(name))) {
		c_strcpy(name, colon);
#endif		
		colon = strchr(name, ':');
		if (colon == NULL)
			port = 5002;
		else {
			*colon = 0;
			colon++;
			port = strtol(colon, NULL, 10);
		}
	}
	else {
		strcpy(name, "localhost");
		port = 5002;
	}
	memset(&address, 0, sizeof(address));
  	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(name);
	if (address.sin_addr.s_addr == INADDR_NONE) {
		struct hostent *host = gethostbyname(name);
		if (!host)
			return;
		memcpy(&(address.sin_addr), host->h_addr, host->h_length);
	}
  	address.sin_port = htons(port);
#if mxWindows
{  	
	WSADATA wsaData;
	unsigned long flag;
	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
		return;
	the->connection = socket(AF_INET, SOCK_STREAM, 0);
	if (the->connection == INVALID_SOCKET)
		return;
  	flag = 1;
  	ioctlsocket(the->connection, FIONBIO, &flag);
	if (connect(the->connection, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
		if (WSAEWOULDBLOCK == WSAGetLastError()) {
			fd_set fds;
			struct timeval timeout = { 2, 0 }; // 2 seconds, 0 micro-seconds
			FD_ZERO(&fds);
			FD_SET(the->connection, &fds);
			if (select(0, NULL, &fds, NULL, &timeout) == 0)
				goto bail;
			if (!FD_ISSET(the->connection, &fds))
				goto bail;
		}
		else
			goto bail;
	}
 	flag = 0;
 	ioctlsocket(the->connection, FIONBIO, &flag);
}
#else
{  	
	int	flag;
	the->connection = socket(AF_INET, SOCK_STREAM, 0);
	if (the->connection <= 0)
		goto bail;
	c_signal(SIGPIPE, SIG_IGN);
#if mxMacOSX
	{
		int set = 1;
		setsockopt(the->connection, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
	}
#endif
	flag = fcntl(the->connection, F_GETFL, 0);
	fcntl(the->connection, F_SETFL, flag | O_NONBLOCK);
	if (connect(the->connection, (struct sockaddr*)&address, sizeof(address)) < 0) {
    	 if (errno == EINPROGRESS) { 
			fd_set fds;
			struct timeval timeout = { 2, 0 }; // 2 seconds, 0 micro-seconds
			int error = 0;
			unsigned int length = sizeof(error);
			FD_ZERO(&fds);
			FD_SET(the->connection, &fds);
			if (select(the->connection + 1, NULL, &fds, NULL, &timeout) == 0)
				goto bail;
			if (!FD_ISSET(the->connection, &fds))
				goto bail;
			if (getsockopt(the->connection, SOL_SOCKET, SO_ERROR, &error, &length) < 0)
				goto bail;
			if (error)
				goto bail;
		}
		else
			goto bail;
	}
	fcntl(the->connection, F_SETFL, flag);
	c_signal(SIGPIPE, SIG_DFL);
}
#endif
	return;
bail:
	fxDisconnect(the);
#endif		
}

void fxDisconnect(txMachine* the)
{
#if mxWindows
	if (the->connection != INVALID_SOCKET) {
		closesocket(the->connection);
		the->connection = INVALID_SOCKET;
	}
	WSACleanup();
#else
	if (the->connection >= 0) {
		close(the->connection);
		the->connection = -1;
	}
#endif
}

txBoolean fxIsConnected(txMachine* the)
{
	return (the->connection != mxNoSocket) ? 1 : 0;
}

txBoolean fxIsReadable(txMachine* the)
{
	return 0;
}

void fxReceive(txMachine* the)
{
	int count;
	if (the->connection != mxNoSocket) {
#if mxWindows
		count = recv(the->connection, the->debugBuffer, sizeof(the->debugBuffer) - 1, 0);
		if (count < 0)
			fxDisconnect(the);
		else
			the->debugOffset = count;
#else
	again:
		count = read(the->connection, the->debugBuffer, sizeof(the->debugBuffer) - 1);
		if (count < 0) {
			if (errno == EINTR)
				goto again;
			else
				fxDisconnect(the);
		}
		else
			the->debugOffset = count;
#endif
	}
	the->debugBuffer[the->debugOffset] = 0;
}

void fxSend(txMachine* the, txBoolean more)
{
	if (the->connection != mxNoSocket) {
#if mxWindows
		if (send(the->connection, the->echoBuffer, the->echoOffset, 0) <= 0)
			fxDisconnect(the);
#else
	again:
		if (write(the->connection, the->echoBuffer, the->echoOffset) <= 0) {
			if (errno == EINTR)
				goto again;
			else
				fxDisconnect(the);
		}
#endif
	}
}

#endif /* mxDebug */






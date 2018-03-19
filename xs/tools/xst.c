/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

extern txScript* fxLoadScript(txMachine* the, txString path, txUnsigned flags);

#if mxWindows
	#include <direct.h>
	#include <errno.h>
	#include <process.h>
	typedef CONDITION_VARIABLE txCondition;
	typedef CRITICAL_SECTION txMutex;
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
	#define fxCreateCondition(CONDITION) pthread_cond_init(CONDITION,NULL)
	#define fxCreateMutex(MUTEX) pthread_mutex_init(MUTEX,NULL)
	#define fxDeleteCondition(CONDITION) pthread_cond_destroy(CONDITION)
	#define fxDeleteMutex(MUTEX) pthread_mutex_destroy(MUTEX)
	#define fxLockMutex(MUTEX) pthread_mutex_lock(MUTEX)
	#define fxUnlockMutex(MUTEX) pthread_mutex_unlock(MUTEX)
	#define fxSleepCondition(CONDITION,MUTEX) pthread_cond_wait(CONDITION,MUTEX)
	#define fxWakeCondition(CONDITION) pthread_cond_signal(CONDITION)
#endif

typedef struct sxAgent txAgent;
typedef struct sxAgentCluster txAgentCluster;
typedef struct sxAgentReport txAgentReport;
typedef struct sxContext txContext;
typedef struct sxJob txJob;
typedef void (*txJobCallback)(txJob*);
typedef struct sxResult txResult;

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
	char harnessPath[C_PATH_MAX];
	int testPathLength;
	txResult* current;
	yaml_document_t* document;
	yaml_node_t* includes;
	yaml_node_t* negative;
#ifdef mxInstrument
	txSize peakChunksSize;
	txSize peakHeapCount;
	txSize peakStackCount;
	txSize parserTotal;
#endif
};

struct sxJob {
	txJob* next;
	txMachine* the;
	txNumber when;
	txJobCallback callback;
	txSlot function;
	txSlot argument;
	txNumber interval;
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

static void fxCountResult(txContext* context, int success, int pending);
static yaml_node_t *fxGetMappingValue(yaml_document_t* document, yaml_node_t* mapping, char* name);
static void fxPopResult(txContext* context);
static void fxPrintResult(txContext* context, txResult* result, int c);
static void fxPrintUsage();
static void fxPushResult(txContext* context, char* path);
static void fxRunDirectory(txContext* context, char* path);
static void fxRunFile(txContext* context, char* path);
static int fxRunTestCase(txContext* context, char* path, txUnsigned flags, char* message);
static int fxStringEndsWith(const char *string, const char *suffix);

static void fx_agent_broadcast(xsMachine* the);
static void fx_agent_getReport(xsMachine* the);
static void fx_agent_leaving(xsMachine* the);
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
extern void fx_clearTimer(txMachine* the);
static void fx_createRealm(xsMachine* the);
static void fx_createRealmException(xsMachine* the);
static void fx_detachArrayBuffer(xsMachine* the);
static void fx_done(xsMachine* the);
static void fx_evalScript(xsMachine* the);
static void fx_print(xsMachine* the);
static void fx_setInterval(txMachine* the);
static void fx_setTimeout(txMachine* the);

static void fxQueuePromiseJobsCallback(txJob* job);
static void fxRunModule(txMachine* the, txString path);
static void fxRunProgram(txMachine* the, txString path, txUnsigned flags);
static void fxRunLoop(txMachine* the);

static void fx_setTimer(txMachine* the, txNumber interval, txBoolean repeat);
static void fx_destroyTimer(void* data);
static void fx_markTimer(txMachine* the, void* it, txMarkRoot markRoot);
static void fx_setTimerCallback(txJob* job);


static txAgentCluster gxAgentCluster;

int main(int argc, char* argv[]) 
{
	int argi;
	int error = 0;
	int option = 0;
	if (argc == 1) {
		fxPrintUsage();
		return 0;
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
		else if (!strcmp(argv[argi], "-s"))
			option = 3;
		else if (!strcmp(argv[argi], "-v"))
			printf("XS %d.%d.%d\n", XS_MAJOR_VERSION, XS_MINOR_VERSION, XS_PATCH_VERSION);
		else {
			fxPrintUsage();
			return 1;
		}
	}
	if (option == 0)
		error = main262(argc, argv);
	else {
		xsCreation _creation = {
			128 * 1024 * 1024, 	/* initialChunkSize */
			16 * 1024 * 1024, 	/* incrementalChunkSize */
			8 * 1024 * 1024, 		/* initialHeapCount */
			1 * 1024 * 1024, 		/* incrementalHeapCount */
			4096, 		/* stackCount */
			4096*3, 		/* keyCount */
			1993, 		/* nameModulo */
			127 		/* symbolModulo */
		};
		xsCreation* creation = &_creation;
		xsMachine* machine;
		char path[C_PATH_MAX];
		fxInitializeSharedCluster();
		machine = xsCreateMachine(creation, "xsr", NULL);
		xsBeginHost(machine);
		{
			xsTry {
				fxNewHostFunctionGlobal(the, fx_clearTimer, 1, xsID("clearInterval"), XS_DONT_ENUM_FLAG);
				the->stack++;
				fxNewHostFunctionGlobal(the, fx_clearTimer, 1, xsID("clearTimeout"), XS_DONT_ENUM_FLAG);
				the->stack++;
				fxNewHostFunctionGlobal(the, fx_createRealmException, 1, xsID("createRealm"), XS_DONT_ENUM_FLAG);
				the->stack++;
				fxNewHostFunctionGlobal(the, fx_evalScript, 1, xsID("evalScript"), XS_DONT_ENUM_FLAG);
				the->stack++;
				fxNewHostFunctionGlobal(the, fx_print, 1, xsID("print"), XS_DONT_ENUM_FLAG);
				the->stack++;
				fxNewHostFunctionGlobal(the, fx_setInterval, 1, xsID("setInterval"), XS_DONT_ENUM_FLAG);
				the->stack++;
				fxNewHostFunctionGlobal(the, fx_setTimeout, 1, xsID("setTimeout"), XS_DONT_ENUM_FLAG);
				the->stack++;

				for (argi = 1; argi < argc; argi++) {
					if (argv[argi][0] == '-')
						continue;
					if (option == 1) {
						xsResult = xsString(argv[argi]);
						xsCall1(xsGlobal, xsID("evalScript"), xsResult);
					}
					else {	
						if (!c_realpath(argv[argi], path))
							xsURIError("file not found: %s", argv[argi]);
						if (option == 2) 
							fxRunModule(the, path);
						else
							fxRunProgram(the, path, mxProgramFlag | mxDebugFlag);
					}
				}
				fxRunLoop(the);
			}
			xsCatch {
				fprintf(stderr, "%s\n", xsToString(xsException));
				error = 1;
			}
		}
		xsEndHost(the);
		xsDeleteMachine(machine);
		fxTerminateSharedCluster();
	}
	return error;
}

int main262(int argc, char* argv[]) 
{
	txContext context;
	char separator[2];
	char path[C_PATH_MAX];
	int error = 0;
	int argi = 1;
	int argj = 0;
	
	c_memset(&context, 0, sizeof(txContext));
	
	c_memset(&gxAgentCluster, 0, sizeof(txAgentCluster));
	fxCreateCondition(&(gxAgentCluster.countCondition));
	fxCreateMutex(&(gxAgentCluster.countMutex));
	fxCreateCondition(&(gxAgentCluster.dataCondition));
	fxCreateMutex(&(gxAgentCluster.dataMutex));
	fxCreateMutex(&(gxAgentCluster.reportMutex));

	separator[0] = mxSeparator;
	separator[1] = 0;
	c_strcpy(path, "..");
	c_strcat(path, separator);
	c_strcat(path, "harness");
	if (!c_realpath(path, context.harnessPath)) {
		fprintf(stderr, "### directory not found: %s\n", path);
		return 1;
	}
	c_strcat(context.harnessPath, separator);
	if (!c_realpath(".", path)) {
		fprintf(stderr, "### directory not found: .\n");
		return 1;
	}
	c_strcat(path, separator);
	context.testPathLength = c_strlen(path);
	context.current = NULL;
	fxPushResult(&context, "");
	
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
					fxPushResult(&context, path + context.testPathLength);
					fxRunDirectory(&context, path);
					fxPopResult(&context);
				}
				else if (fxStringEndsWith(path, ".js") && !fxStringEndsWith(path, "_FIXTURE.js"))
					fxRunFile(&context, path);
			}
		}
		else {
			fprintf(stderr, "### test not found: %s\n", argv[argi]);
			error = 1;
		}
	}
	if (argj)
		fxPrintResult(&context, context.current, 0);
#ifdef mxInstrument
	fprintf(stderr, "# parser chunks: %d bytes\n", context.parserTotal);
	fprintf(stderr, "# heap chunks: %d bytes\n", context.peakChunksSize);
	fprintf(stderr, "# heap slots: %lu bytes\n", context.peakHeapCount * sizeof(txSlot));
	fprintf(stderr, "# stack slots: %lu bytes\n", context.peakStackCount * sizeof(txSlot));
#endif
	return error;
}

void fxCountResult(txContext* context, int success, int pending) 
{
	txResult* result = context->current;
	while (result) {
		result->testCount++;
		result->successCount += success;
		result->pendingCount += pending;
		result = result->parent;
	}
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

void fxPopResult(txContext* context) 
{
	context->current = context->current->parent;
}

void fxPrintResult(txContext* context, txResult* result, int c)
{
	int i = 0;
	while (i < c) {
		fprintf(stderr, "    ");
		i++;
	}
	if (result->testCount > result->pendingCount)
		fprintf(stderr, "%3d%%", 100 * result->successCount / (result->testCount - result->pendingCount));
	else
		fprintf(stderr, "  0%%");
	fprintf(stderr, " %d/%d", result->successCount, result->testCount - result->pendingCount);
	if (result->pendingCount)
		fprintf(stderr, " (%d)", result->pendingCount);
	fprintf(stderr, " %s\n", result->path);
	result = result->first;
	c++;
	while (result) {
		fxPrintResult(context, result, c);
		result = result->next;
	}
}

void fxPrintUsage()
{
	printf("xst [-h] [-e] [-m] [-s] [-v] strings...\n");
	printf("\t-h: print this help message\n");
	printf("\t-e: eval strings\n");
	printf("\t-m: strings are paths to modules\n");
	printf("\t-s: strings are paths to scripts\n");
	printf("\t-v: print XS version\n");
	printf("without -e, -m or -s, strings are paths to test262 cases or directories\n");
}

void fxPushResult(txContext* context, char* path) 
{
	txResult* parent = context->current;
	txResult* result = c_malloc(sizeof(txResult) + c_strlen(path));
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
	context->current = result;
}

void fxRunDirectory(txContext* context, char* path)
{
	typedef struct sxEntry txEntry;
	struct sxEntry {
		txEntry* nextEntry;
		char name[1];
	};

#if mxWindows
	UINT32 length;
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
					fxPushResult(context, firstEntry->name);
					fxRunDirectory(context, path);
					fxPopResult(context);
				}
				else if (fxStringEndsWith(path, ".js") && !fxStringEndsWith(path, "_FIXTURE.js"))
					fxRunFile(context, path);
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
					fxPushResult(context, firstEntry->name);
					fxRunDirectory(context, path);
					fxPopResult(context);
				}
				else if (fxStringEndsWith(path, ".js") && !fxStringEndsWith(path, "_FIXTURE.js"))
					fxRunFile(context, path);
			}
			nextEntry = firstEntry->nextEntry;
			free(firstEntry);
			firstEntry = nextEntry;
		}
	}
#endif
}

void fxRunFile(txContext* context, char* path)
{
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
	int sloppy = 1;
	int strict = 1;
	int module = 0;
	int async = 0;
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
			if (!strcmp((char*)node->data.scalar.value, "onlyStrict")) {
				sloppy = 0;
			}
			else if (!strcmp((char*)node->data.scalar.value, "noStrict")) {
				strict = 0;
			}
			else if (!strcmp((char*)node->data.scalar.value, "module")) {
				sloppy = 0;
				strict = 0;
				module = 1;
			}
			else if (!strcmp((char*)node->data.scalar.value, "raw")) {
				strict = 0;
			}
			else if (!strcmp((char*)node->data.scalar.value, "async")) {
				async = 1;
			}
			item++;
		}
	}

	// @@ skip test cases beyond 8th...
	value = fxGetMappingValue(document, root, "features");
	if (value) {
		yaml_node_item_t* item = value->data.sequence.items.start;
		while (item < value->data.sequence.items.top) {
			yaml_node_t* node = yaml_document_get_node(document, *item);
			if (!strcmp((char*)node->data.scalar.value, "BigInt")
			||	!strcmp((char*)node->data.scalar.value, "class-fields")
			||	!strcmp((char*)node->data.scalar.value, "class-fields-private")
			||	!strcmp((char*)node->data.scalar.value, "class-fields-public")
			||	!strcmp((char*)node->data.scalar.value, "regexp-unicode-property-escapes")
			||	!strcmp((char*)node->data.scalar.value, "Array.prototype.flatten")
			||	!strcmp((char*)node->data.scalar.value, "Array.prototype.flatMap")
			||	!strcmp((char*)node->data.scalar.value, "numeric-separator-literal")
			||	!strcmp((char*)node->data.scalar.value, "string-trimming")
			||	!strcmp((char*)node->data.scalar.value, "String.prototype.trimEnd")
			||	!strcmp((char*)node->data.scalar.value, "String.prototype.trimStart")
			) {
				sloppy = 0;
				strict = 0;
				module = 0;
				pending = 1;
			}
			item++;
		}
	}

	if (sloppy) {
		fprintf(stderr, "### %s (sloppy): ", path + context->testPathLength);
		if (fxRunTestCase(context, path, mxProgramFlag | mxDebugFlag, message))
			fprintf(stderr, "%s\n", message);
		else
			fprintf(stderr, "%s\n", message);
	}
	if (strict) {
		fprintf(stderr, "### %s (strict): ", path + context->testPathLength);
		if (fxRunTestCase(context, path, mxProgramFlag | mxDebugFlag | mxStrictFlag, message))
			fprintf(stderr, "%s\n", message);
		else
			fprintf(stderr, "%s\n", message);
	}
	if (module) {
		fprintf(stderr, "### %s (module): ", path + context->testPathLength);
		if (fxRunTestCase(context, path, 0, message))
			fprintf(stderr, "%s\n", message);
		else
			fprintf(stderr, "%s\n", message);
	}
	if (pending) {
		fprintf(stderr, "### %s: SKIP\n", path + context->testPathLength);
		fxCountResult(context, 0, 1);
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

int fxRunTestCase(txContext* context, char* path, txUnsigned flags, char* message)
{
	xsCreation _creation = {
		128 * 1024 * 1024, 	/* initialChunkSize */
		16 * 1024 * 1024, 	/* incrementalChunkSize */
		8 * 1024 * 1024, 		/* initialHeapCount */
		1 * 1024 * 1024, 		/* incrementalHeapCount */
		4096, 		/* stackCount */
		4096*3, 		/* keyCount */
		1993, 		/* nameModulo */
		127 		/* symbolModulo */
	};
	xsCreation* creation = &_creation;
	xsMachine* machine;
	char buffer[C_PATH_MAX];
	int success = 0;
	fxInitializeSharedCluster();
	machine = xsCreateMachine(creation, "xst", NULL);
	xsBeginHost(machine);
	{
		xsTry {
			txSlot* slot;

			slot = fxLastProperty(the, fxNewHostObject(the, NULL));
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_broadcast, 2, xsID("broadcast"), XS_GET_ONLY); 
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_getReport, 0, xsID("getReport"), XS_GET_ONLY); 
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_sleep, 1, xsID("sleep"), XS_GET_ONLY); 
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_start, 1, xsID("start"), XS_GET_ONLY); 
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_stop, 1, xsID("stop"), XS_GET_ONLY); 
			
			mxPush(mxObjectPrototype);
			slot = fxLastProperty(the, fxNewObjectInstance(the));
			slot = fxNextSlotProperty(the, slot, the->stack + 1, xsID("agent"), XS_GET_ONLY);
			slot = fxNextHostFunctionProperty(the, slot, fx_createRealm, 0, xsID("createRealm"), XS_GET_ONLY); 
			slot = fxNextHostFunctionProperty(the, slot, fx_detachArrayBuffer, 1, xsID("detachArrayBuffer"), XS_GET_ONLY); 
			slot = fxNextHostFunctionProperty(the, slot, fx_evalScript, 1, xsID("evalScript"), XS_GET_ONLY); 
			slot = fxNextSlotProperty(the, slot, &mxGlobal, xsID("global"), XS_GET_ONLY);
			slot = fxGlobalSetProperty(the, mxGlobal.value.reference, xsID("$262"), XS_NO_ID, XS_OWN);
			slot->flag = XS_GET_ONLY;
			slot->kind = the->stack->kind;
			slot->value = the->stack->value;
			the->stack++;
			
			the->stack++;
		
			fxNewHostFunctionGlobal(the, fx_print, 1, xsID("print"), XS_DONT_ENUM_FLAG);
			the->stack++;
			fxNewHostFunctionGlobal(the, fx_clearTimer, 1, xsID("clearInterval"), XS_DONT_ENUM_FLAG);
			the->stack++;
			fxNewHostFunctionGlobal(the, fx_clearTimer, 1, xsID("clearTimeout"), XS_DONT_ENUM_FLAG);
			the->stack++;
			fxNewHostFunctionGlobal(the, fx_setInterval, 1, xsID("setInterval"), XS_DONT_ENUM_FLAG);
			the->stack++;
			fxNewHostFunctionGlobal(the, fx_setTimeout, 1, xsID("setTimeout"), XS_DONT_ENUM_FLAG);
			the->stack++;
			fxNewHostFunctionGlobal(the, fx_done, 1, xsID("$DONE"), XS_DONT_ENUM_FLAG);
			the->stack++;

			c_strcpy(buffer, context->harnessPath);
			c_strcat(buffer, "sta.js");
			fxRunProgram(the, buffer, mxProgramFlag | mxDebugFlag);
			c_strcpy(buffer, context->harnessPath);
			c_strcat(buffer, "assert.js");
			fxRunProgram(the, buffer, mxProgramFlag | mxDebugFlag);
			if (context->includes) {
				yaml_node_item_t* item = context->includes->data.sequence.items.start;
				while (item < context->includes->data.sequence.items.top) {
					yaml_node_t* node = yaml_document_get_node(context->document, *item);
					c_strcpy(buffer, context->harnessPath);
					c_strcat(buffer, (char*)node->data.scalar.value);
					fxRunProgram(the, buffer, mxProgramFlag | mxDebugFlag);
					item++;
				}
			}
			if (flags)
				fxRunProgram(the, path, flags);
			else
				fxRunModule(the, path);
			fxRunLoop(the);
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
				xsToStringBuffer(xsException, message, 1024);
			}
		}
		xsResult = xsGet(xsGlobal, xsID("$262"));
		xsResult = xsGet(xsResult, xsID("agent"));
		xsCall0(xsResult, xsID("stop"));
	}
#ifdef mxInstrument
	fxCollectGarbage(the);
	snprintf(buffer, sizeof(buffer), " # %d %lu %lu %d", 
		the->peakChunksSize, 
		the->peakHeapCount * sizeof(txSlot), 
		(the->stackTop - the->stackPeak) * sizeof(txSlot),
		the->parserTotal);
	c_strcat(message, buffer);
	if (context->peakChunksSize < the->peakChunksSize)
		context->peakChunksSize = the->peakChunksSize;
	if (context->peakHeapCount < the->peakHeapCount)
		context->peakHeapCount = the->peakHeapCount;
	if (context->peakStackCount < (the->stackTop - the->stackPeak))
		context->peakStackCount = (the->stackTop - the->stackPeak);
	if (context->parserTotal < the->parserTotal)
		context->parserTotal = the->parserTotal;
#endif
	xsEndHost(the);
	xsDeleteMachine(machine);
	fxTerminateSharedCluster();
	fxCountResult(context, success, 0);
	return success;
}

int fxStringEndsWith(const char *string, const char *suffix)
{
	int stringLength = strlen(string);
	int suffixLength = strlen(suffix);
	return (stringLength >= suffixLength) && (0 == strcmp(string + (stringLength - suffixLength), suffix));
}

/* $262 */

void fx_agent_broadcast(xsMachine* the)
{
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
    
    fxLockMutex(&(gxAgentCluster.countMutex));
    while (gxAgentCluster.count > 0)
		fxSleepCondition(&(gxAgentCluster.countCondition), &(gxAgentCluster.countMutex));
    fxUnlockMutex(&(gxAgentCluster.countMutex));
}

void fx_agent_getReport(xsMachine* the)
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

void fx_agent_leaving(xsMachine* the)
{
}

void fx_agent_receiveBroadcast(xsMachine* the)
{
    fxLockMutex(&(gxAgentCluster.dataMutex));
	while (gxAgentCluster.dataBuffer == NULL)
		fxSleepCondition(&(gxAgentCluster.dataCondition), &(gxAgentCluster.dataMutex));
	xsResult = xsDemarshallAlien(gxAgentCluster.dataBuffer);
    fxUnlockMutex(&(gxAgentCluster.dataMutex));
	
    fxLockMutex(&(gxAgentCluster.countMutex));
    gxAgentCluster.count--;
    fxWakeCondition(&(gxAgentCluster.countCondition));
    fxUnlockMutex(&(gxAgentCluster.countMutex));
		
	xsCallFunction2(xsArg(0), xsGlobal, xsResult, xsInteger(gxAgentCluster.dataValue));
}

void fx_agent_report(xsMachine* the)
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
		1 * 1024 * 1024, 		/* initialHeapCount */
		1 * 1024 * 1024, 		/* incrementalHeapCount */
		4096, 		/* stackCount */
		4096*3, 		/* keyCount */
		1993, 		/* nameModulo */
		127 		/* symbolModulo */
	};
	txAgent* agent = it;
	xsMachine* machine = xsCreateMachine(&creation, "xst-agent", NULL);
	machine->host = it;
	xsBeginHost(machine);
	{
		xsTry {
			txSlot* slot;
			txStringCStream stream;
			
			slot = fxLastProperty(the, fxNewHostObject(the, NULL));
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_leaving, 0, xsID("leaving"), XS_GET_ONLY); 
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_receiveBroadcast, 1, xsID("receiveBroadcast"), XS_GET_ONLY); 
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_report, 1, xsID("report"), XS_GET_ONLY); 
			slot = fxNextHostFunctionProperty(the, slot, fx_agent_sleep, 1, xsID("sleep"), XS_GET_ONLY); 
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
			fxRunScript(the, fxParseScript(the, &stream, fxStringCGetter, mxProgramFlag), mxThis, C_NULL, C_NULL, C_NULL, C_NULL);
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
	txAgent* agent = gxAgentCluster.firstAgent;
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

void fx_createRealm(xsMachine* the)
{
	xsResult = xsThis;
}

void fx_createRealmException(xsMachine* the)
{
	mxUnknownError("one realm only");
}

void fx_detachArrayBuffer(xsMachine* the)
{
	txSlot* slot = mxArgv(0);
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_ARRAY_BUFFER_KIND) && (instance != mxArrayBufferPrototype.value.reference)) {
			slot->value.arrayBuffer.address = C_NULL;
			slot->value.arrayBuffer.length = 0;
			return;
		}
	}
	mxTypeError("this is no ArrayBuffer instance");
}

void fx_done(xsMachine* the)
{
}

void fx_evalScript(xsMachine* the)
{
	txStringStream aStream;
	aStream.slot = mxArgv(0);
	aStream.offset = 0;
	aStream.size = c_strlen(fxToString(the, mxArgv(0)));
	fxRunScript(the, fxParseScript(the, &aStream, fxStringGetter, mxProgramFlag | mxDebugFlag), &mxGlobal, C_NULL, mxClosures.value.reference, C_NULL, C_NULL);
	mxPullSlot(mxResult);
}

void fx_print(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	for (i = 0; i < c; i++) {
		if (i)
			fprintf(stdout, " ");
		fprintf(stdout, "%s", xsToString(xsArg(i)));
	}
	fprintf(stdout, "\n");
}

void fx_setInterval(txMachine* the)
{
	fx_setTimer(the, fxToNumber(the, mxArgv(1)), 1);
}

void fx_setTimeout(txMachine* the)
{
	fx_setTimer(the, fxToNumber(the, mxArgv(1)), 0);
}

static txHostHooks gxTimerHooks = {
	fx_destroyTimer,
	fx_markTimer
};

void fx_clearTimer(txMachine* the)
{
	txJob* data = fxGetHostData(the, mxThis);
	if (data) {
		txJob* job;
		txJob** address;
		address = (txJob**)&(the->context);
		while ((job = *address)) {
			if (job == data) {
				*address = job->next;
				c_free(job);
				break;
			}
			address = &(job->next);
		}
		fxSetHostData(the, mxThis, NULL);
	}
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

void fx_setTimer(txMachine* the, txNumber interval, txBoolean repeat)
{
	c_timeval tv;
	txJob* job;
	txJob** address = (txJob**)&(the->context);
	while ((job = *address))
		address = &(job->next);
	job = *address = malloc(sizeof(txJob));
	c_memset(job, 0, sizeof(txJob));
	job->the = the;
	job->callback = fx_setTimerCallback;
	c_gettimeofday(&tv, NULL);
	if (repeat)
		job->interval = interval;
	job->when = ((txNumber)(tv.tv_sec) * 1000.0) + ((txNumber)(tv.tv_usec) / 1000.0) + interval;
	job->function.kind = mxArgv(0)->kind;
	job->function.value = mxArgv(0)->value;
	if (mxArgc > 2) {
		job->argument.kind = mxArgv(2)->kind;
		job->argument.value = mxArgv(2)->value;
	}
	fxNewHostObject(the, C_NULL);
	fxSetHostData(the, the->stack, job);
	fxSetHostHooks(the, the->stack, &gxTimerHooks);
	mxPullSlot(mxResult);
}

void fx_setTimerCallback(txJob* job)
{
	txMachine* the = job->the;
	fxBeginHost(the);
	{
		mxPush(job->argument);
		/* ARGC */
		mxPushInteger(1);
		/* THIS */
		mxPushUndefined();
		/* FUNCTION */
		mxPush(job->function);
		fxCall(the);
		the->stack++;
	}
	fxEndHost(the);
}

/* PLATFORM */

void fxCreateMachinePlatform(txMachine* the)
{
#ifdef mxDebug
	the->connection = mxNoSocket;
#endif
	the->host = NULL;
}

void fxDeleteMachinePlatform(txMachine* the)
{
}

void fxQueuePromiseJobs(txMachine* the)
{
	c_timeval tv;
	txJob* job;
	txJob** address = (txJob**)&(the->context);
	while ((job = *address))
		address = &(job->next);
	job = *address = malloc(sizeof(txJob));
    c_memset(job, 0, sizeof(txJob));
    job->the = the;
    job->callback = fxQueuePromiseJobsCallback;
	c_gettimeofday(&tv, NULL);
	job->when = ((txNumber)(tv.tv_sec) * 1000.0) + ((txNumber)(tv.tv_usec) / 1000.0);
}

void fxQueuePromiseJobsCallback(txJob* job) 
{
	txMachine* the = job->the;
	fxRunPromiseJobs(the);
}

void fxRunLoop(txMachine* the)
{
	c_timeval tv;
	txNumber when;
	txJob* job;
	txJob** address;
	for (;;) {
		c_gettimeofday(&tv, NULL);
		when = ((txNumber)(tv.tv_sec) * 1000.0) + ((txNumber)(tv.tv_usec) / 1000.0);
		address = (txJob**)&(the->context);
		if (!*address)
			break;
		while ((job = *address)) {
			if (job->when <= when) {
				(*job->callback)(job);	
				if (job->interval) {
					job->when += job->interval;
				}
				else {
					*address = job->next;
					c_free(job);
				}
			}
			else
				address = &(job->next);
		}
	}
}

void fxRunModule(txMachine* the, txString path)
{
	mxPushStringC(path);
	fxRequireModule(the, XS_NO_ID, the->stack);
	the->stack++;
}

void fxRunProgram(txMachine* the, txString path, txUnsigned flags)
{
	txScript* script = fxLoadScript(the, path, flags);
	fxRunScript(the, script, &mxGlobal, C_NULL, mxClosures.value.reference, C_NULL, C_NULL);
	mxPullSlot(mxResult);
}

/* DEBUG */

#ifdef mxDebug

void fxAbort(txMachine* the)
{
	c_exit(0);
}

void fxConnect(txMachine* the)
{
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
	signal(SIGPIPE, SIG_IGN);
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
	signal(SIGPIPE, SIG_DFL);
}
#endif
	return;
bail:
	fxDisconnect(the);
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






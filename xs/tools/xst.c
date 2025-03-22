/*
 * Copyright (c) 2016-2024  Moddable Tech, Inc.
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

extern int fuzz(int argc, char* argv[]);
extern void fx_print(xsMachine* the);
extern void fxBuildAgent(xsMachine* the);
extern void fxBuildFuzz(xsMachine* the);
extern txScript* fxLoadScript(txMachine* the, txString path, txUnsigned flags);
extern void fxFulfillModuleFile(txMachine* the);
extern void fxRejectModuleFile(txMachine* the);
extern void fxRunLoop(txMachine* the);
extern void fxRunModuleFile(txMachine* the, txString path);
extern void fxRunProgramFile(txMachine* the, txString path, txUnsigned flags);
extern int main262(int argc, char* argv[]);

struct sxJob {
	txMachine* the;
	txSlot self;
	txSlot function;
	txSlot argument;
};

struct sxSharedTimer {
	txSharedTimer* next;
	txThread thread;
	txNumber when;
	txNumber interval;
	txSharedTimerCallback callback;
	txInteger refconSize;
	char refcon[1];
};

static void fxPrintUsage();

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
static void fx_createRealm(xsMachine* the);
static void fx_detachArrayBuffer(xsMachine* the);
static void fx_evalScript(xsMachine* the);
static void fx_gc(xsMachine* the);
static void fx_isLockedDown(xsMachine* the);
static void fx_metering(xsMachine* the);
static void fx_runScript(xsMachine* the);

extern void fx_clearTimer(txMachine* the);
static void fx_destroyTimer(void* data);
static void fx_markTimer(txMachine* the, void* it, txMarkRoot markRoot);
static void fx_setInterval(txMachine* the);
static void fx_setTimeout(txMachine* the);
static void fx_setTimer(txMachine* the, txNumber interval, txBoolean repeat);


char *gxAbortStrings[] = {
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

txAgentCluster gxAgentCluster;

#if OSSFUZZ
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
	
		if (!strcmp(argv[argi], "-c"))
			option = 4;
		else if (!strcmp(argv[argi], "-i")) {
			argi++;
			option = 4;
		}
		else if (!strcmp(argv[argi], "-l"))
			option = 4;
		else if (!strcmp(argv[argi], "-lc"))
			option = 4;
		else if (!strcmp(argv[argi], "-o")) {
			argi++;
			option = 4;
		}
		else if (!strcmp(argv[argi], "-t"))
			option = 4;

		else if (!strcmp(argv[argi], "-h"))
			fxPrintUsage();
		else if (!strcmp(argv[argi], "-b"))
			option = 7;
		else if (!strcmp(argv[argi], "-e"))
			option = 1;
		else if (!strcmp(argv[argi], "-f"))
			option = 5;
		else if (!strcmp(argv[argi], "-j"))
			option = 6;
		else if (!strcmp(argv[argi], "-m"))
			option = 2;
		else if (!strcmp(argv[argi], "-p"))
			profiling = 1;
		else if (!strcmp(argv[argi], "-s"))
			option = 3;
		else if (!strcmp(argv[argi], "-v"))
			printf("XS %d.%d.%d, slot %zu bytes, ID %zu bytes\n", XS_MAJOR_VERSION, XS_MINOR_VERSION, XS_PATCH_VERSION, sizeof(txSlot), sizeof(txID));
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
 	else if (option == 5) {
 		error = fuzz(argc, argv);
 	}
	else {
		xsCreation _creation = {
			16 * 1024 * 1024, 	/* initialChunkSize */
			16 * 1024 * 1024, 	/* incrementalChunkSize */
			1 * 1024 * 1024, 	/* initialHeapCount */
			1 * 1024 * 1024, 	/* incrementalHeapCount */
			256 * 1024, 		/* stackCount */
			1024, 				/* initialKeyCount */
			1024,				/* incrementalKeyCount */
			1993, 				/* nameModulo */
			127, 				/* symbolModulo */
			64 * 1024,			/* parserBufferSize */
			1993,				/* parserTableModulo */
		};
		xsCreation* creation = &_creation;
		xsMachine* machine;
        machine = xsCreateMachine(creation, "xst", NULL);
		xsBeginMetering(machine, NULL, 0);
		{
 		if (profiling)
			fxStartProfiling(machine);

		xsBeginHost(machine);
		{
			xsVars(2);
			xsTry {
 				fxBuildAgent(machine);
 				fxBuildFuzz(machine);

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
						if ((option == 6) || (option == 7)) {
							FILE* file = C_NULL;
							char *buffer = C_NULL;
							xsTry {
								file = fopen(path, "r");
								if (!file)
									xsUnknownError("can't open file");
								fseek(file, 0, SEEK_END);
								size_t size = ftell(file);
								fseek(file, 0, SEEK_SET);
								buffer = c_malloc(size + 1);
								if (!buffer)
									xsUnknownError("not enough memory");
								if (size != fread(buffer, 1, size, file))	
									xsUnknownError("can't read file");
								buffer[size] = 0;
								fclose(file);
								file = C_NULL;
								xsResult = xsArrayBuffer(buffer, (txInteger)size);
								c_free(buffer);
								buffer = C_NULL;
								xsVar(1) = xsNew0(xsGlobal, xsID("TextDecoder"));
								xsResult = xsCall1(xsVar(1), xsID("decode"), xsResult);
								if (option == 6) {
									xsVar(1) = xsGet(xsGlobal, xsID("JSON"));
									xsResult = xsCall1(xsVar(1), xsID("parse"), xsResult);
								}
								else {
									xsResult = xsCall1(xsGlobal, xsID("eval"), xsResult);
								}
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
 		if (profiling)
			fxStopProfiling(machine, C_NULL);
		}
		xsEndMetering(machine);
		if (machine->exitStatus) {
			char *why = (machine->exitStatus <= XS_UNHANDLED_REJECTION_EXIT) ? gxAbortStrings[machine->exitStatus] : "unknown";
			fprintf(stderr, "Error: %s\n", why);
			error = 1;
		}
		xsDeleteMachine(machine);
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
	slot = fxNextHostFunctionProperty(the, slot, fx_agent_broadcast, 2, xsID("broadcast"), XS_DONT_ENUM_FLAG); 
	slot = fxNextHostFunctionProperty(the, slot, fx_agent_getReport, 0, xsID("getReport"), XS_DONT_ENUM_FLAG); 
	slot = fxNextHostFunctionProperty(the, slot, fx_agent_monotonicNow, 0, xsID("monotonicNow"), XS_DONT_ENUM_FLAG); 
	slot = fxNextHostFunctionProperty(the, slot, fx_agent_sleep, 1, xsID("sleep"), XS_DONT_ENUM_FLAG); 
	slot = fxNextHostFunctionProperty(the, slot, fx_agent_start, 1, xsID("start"), XS_DONT_ENUM_FLAG); 
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
	slot = fxNextHostFunctionProperty(the, slot, fx_isLockedDown, 1, xsID("isLockedDown"), XS_DONT_ENUM_FLAG); 
	slot = fxNextHostFunctionProperty(the, slot, fx_metering, 1, xsID("metering"), XS_DONT_ENUM_FLAG); 
	slot = fxNextSlotProperty(the, slot, global, xsID("global"), XS_GET_ONLY);

	slot = fxLastProperty(the, fxToInstance(the, global));
	slot = fxNextSlotProperty(the, slot, the->stack, xsID("$262"), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_print, 1, xsID("print"), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_runScript, 1, xsID("runScript"), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_clearTimer, 1, xsID("clearInterval"), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_clearTimer, 1, xsID("clearTimeout"), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_setInterval, 1, xsID("setInterval"), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_setTimeout, 1, xsID("setTimeout"), XS_DONT_ENUM_FLAG);
	
	slot = fxNextHostFunctionProperty(the, slot, fx_harden, 1, xsID("harden"), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_lockdown, 0, xsID("lockdown"), XS_DONT_ENUM_FLAG);
	
	slot = fxNextHostFunctionProperty(the, slot, fx_unicodeCompare, 2, xsID("unicodeCompare"), XS_DONT_ENUM_FLAG);

	mxPop();
	mxPop();
	
	modInstallTextDecoder(the);
	modInstallTextEncoder(the);
	modInstallBase64(the);
}

void fxPrintUsage()
{
	printf("xst [-h] [-e] [-f] [-j] [-m] [-p] [-s] [-t] [-v] strings...\n");
	printf("\t-b: strings are paths to script buffers\n");
	printf("\t-e: eval strings\n");
	printf("\t-f: fuzz with REPRL harness\n");
	printf("\t-h: print this help message\n");
	printf("\t-j: strings are paths to JSON buffers\n");
	printf("\t-m: strings are paths to modules\n");
	printf("\t-p: profile\n");
	printf("\t-s: strings are paths to scripts\n");
	printf("\t-t: strings are paths to test262 cases or directories\n");
	printf("\t-v: print XS version\n");
	printf("without -b, -e, -f, -j, -m, -s, or -t:\n");
	printf("\tif ../harness exists, strings are paths to test262 cases or directories\n");
	printf("\telse if the extension is .mjs, strings are paths to modules\n");
	printf("\telse strings are paths to scripts\n");
}

void fx_evalScript(xsMachine* the)
{
	txSlot* realm;
	txSlot* module = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.module;
	if (!module) module = mxProgram.value.reference;
	realm = mxModuleInstanceInternal(module)->value.module.realm;
	txStringStream aStream;
	aStream.slot = mxArgv(0);
	aStream.offset = 0;
	aStream.size = mxStringLength(fxToString(the, mxArgv(0)));
	fxRunScript(the, fxParseScript(the, &aStream, fxStringGetter, mxProgramFlag | mxDebugFlag), mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, module);
	mxPullSlot(mxResult);
}

void fx_gc(xsMachine* the)
{
	xsCollectGarbage();
}

void fx_isLockedDown(xsMachine* the)
{
	xsResult = (mxProgram.value.reference->flag & XS_DONT_MARSHALL_FLAG) ? xsTrue : xsFalse;
}

void fx_metering(xsMachine* the)
{
#ifdef mxMetering
	xsResult = xsNumber(the->meterIndex);
#endif	
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

void fx_runScript(xsMachine* the)
{
	txSlot* realm = mxProgram.value.reference->next->value.module.realm;
	char path[C_PATH_MAX];
	txUnsigned flags = mxProgramFlag | mxDebugFlag;
	if (!c_realpath(fxToString(the, mxArgv(0)), path))
		xsURIError("file not found");
	txScript* script = fxLoadScript(the, path, flags);
	mxModuleInstanceInternal(mxProgram.value.reference)->value.module.id = fxID(the, path);
	fxRunScript(the, script, mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, mxProgram.value.reference);
	mxPullSlot(mxResult);
}

/* $262 */

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
	xsResult = xsNumber(mxMonotonicNow());
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
#elif mxMacOSX
	pthread_attr_t attr; 
	pthread_t self = pthread_self();
	size_t size = pthread_get_stacksize_np(self);
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, size);
    pthread_create(&(agent->thread), &attr, &fx_agent_start_aux, agent);
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
		1024, 				/* initialKeyCount */
		1024,				/* incrementalKeyCount */
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
			fxRunLoop(the);
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

/* TIMER */

static txHostHooks gxTimerHooks = {
	fx_destroyTimer,
	fx_markTimer
};

void fx_callbackTimer(txSharedTimer* timer, void* refcon, txInteger refconSize)
{
	txJob* job = (txJob*)refcon;
	txMachine* the = job->the;
	fxBeginHost(the);
	mxTry(the) {
		mxPushUndefined();
		mxPush(job->function);
		mxCall();
		mxPush(job->argument);
		mxRunCount(1);
		mxPop();
	}
	mxCatch(the) {
		*((txSlot*)the->rejection) = mxException;
		timer->interval = 0;
	}
	if (timer->interval == 0) {
		fxAccess(the, &job->self);
		*mxResult = the->scratch;
		fxForget(the, &job->self);
		fxSetHostData(the, mxResult, NULL);
	}
	fxEndHost(the);
}

void fx_clearTimer(txMachine* the)
{
	if (mxIsNull(mxArgv(0)))
		return;
	txHostHooks* hooks = fxGetHostHooks(the, mxArgv(0));
	if (hooks == &gxTimerHooks) {
		txSharedTimer* timer = fxGetHostData(the, mxArgv(0));
		if (timer) {
			txJob* job = (txJob*)&(timer->refcon[0]);
			fxForget(the, &job->self);
			fxSetHostData(the, mxArgv(0), NULL);
			fxUnscheduleSharedTimer(timer);
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
	txJob _job;
	txJob* job;
	txSharedTimer* timer;
	if (c_isnan(interval) || (interval < 0))
		interval = 0;
	c_memset(&_job, 0, sizeof(txJob));
	timer = fxScheduleSharedTimer(interval, (repeat) ? interval : 0, fx_callbackTimer, &_job, sizeof(txJob));	
	if (!timer)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	job = (txJob*)&(timer->refcon[0]);
	job->the = the;
	fxNewHostObject(the, NULL);
    mxPull(job->self);
	job->function = *mxArgv(0);
	if (mxArgc > 2)
		job->argument = *mxArgv(2);
	else
		job->argument = mxUndefined;
	fxSetHostData(the, &job->self, timer);
	fxSetHostHooks(the, &job->self, &gxTimerHooks);
	fxRemember(the, &job->self);
	fxAccess(the, &job->self);
	*mxResult = the->scratch;
}

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

/* SHARED TIMERS */

typedef struct sxSharedTimers txSharedTimers;
struct sxSharedTimers {
	txSharedTimer* first;
	txMutex mutex;
};
static txSharedTimers gxSharedTimers;

void fxInitializeSharedTimers()
{
	c_memset(&gxSharedTimers, 0, sizeof(txSharedTimers));
	fxCreateMutex(&(gxSharedTimers.mutex));
}

void fxTerminateSharedTimers()
{
	fxDeleteMutex(&(gxSharedTimers.mutex));
	if (gxSharedTimers.first != C_NULL) {
		fprintf(stderr, "# shared timers mismatch!\n");
		exit(1);
	}
}

void fxRescheduleSharedTimer(txSharedTimer* timer, txNumber timeout, txNumber interval)
{
    fxLockMutex(&(gxSharedTimers.mutex));
	timer->when = mxMonotonicNow() + timeout;
	timer->interval = interval;
    fxUnlockMutex(&(gxSharedTimers.mutex));
}

void* fxScheduleSharedTimer(txNumber timeout, txNumber interval, txSharedTimerCallback callback, void* refcon, txInteger refconSize)
{
	txSharedTimer* timer;
	txSharedTimer** address;
	txSharedTimer* link;
	timer = c_calloc(1, sizeof(txSharedTimer) + refconSize - 1);
	if (timer) {
		timer->thread = mxCurrentThread();
		timer->when = mxMonotonicNow() + timeout;
		timer->interval = interval;
		timer->callback = callback;
		timer->refconSize = refconSize;
		c_memcpy(timer->refcon, refcon, refconSize);
		
		fxLockMutex(&(gxSharedTimers.mutex));
		address = (txSharedTimer**)&(gxSharedTimers.first);
		while ((link = *address))
			address = &(link->next);
		*address = timer;
		fxUnlockMutex(&(gxSharedTimers.mutex));
    }
    return timer;
}

void fxUnscheduleSharedTimer(txSharedTimer* timer)
{
	txSharedTimer** address;
	txSharedTimer* link;
    fxLockMutex(&(gxSharedTimers.mutex));
    address = (txSharedTimer**)&(gxSharedTimers.first);
	while ((link = *address)) {
		if (link == timer) {
			*address = link->next;
			c_free(timer);
			break;
		}
		address = &(link->next);
	}
    fxUnlockMutex(&(gxSharedTimers.mutex));
}

void fxRunLoop(txMachine* the)
{
	txThread thread = mxCurrentThread();
	txNumber when;
	txInteger count;
	txSharedTimer* timer;
	
	for (;;) {
		fxEndJob(the);
		while (the->promiseJobs) {
			while (the->promiseJobs) {
				the->promiseJobs = 0;
				fxRunPromiseJobs(the);
			}
			fxEndJob(the);
		}
		when = mxMonotonicNow();
		fxLockMutex(&(gxSharedTimers.mutex));
		count = 0;
		timer = gxSharedTimers.first;
		while (timer) {
			if (timer->thread == thread) {
				count++;
				if (timer->when <= when)
					break; // one timer at time to run promise jobs queued by the timer in the same "tick"
			}
			timer = timer->next;
		}
		fxUnlockMutex(&(gxSharedTimers.mutex));
		if (timer) {
			(timer->callback)(timer, timer->refcon, timer->refconSize);
			if (timer->interval == 0)
				fxUnscheduleSharedTimer(timer);
			else
				timer->when += timer->interval;
			continue;
		}
		if (count == 0)
			break;
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
	mxPushUndefined();
	fxRunImport(the, realm, C_NULL);
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
	if (the->exitStatus) // xsEndHost calls fxAbort!
		return;
	the->exitStatus = status;
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
	txString dot;
	txScript* script;
#ifdef mxDebug
	txUnsigned flags = mxDebugFlag;
#else
	txUnsigned flags = 0;
#endif
	c_strncpy(path, fxGetKeyName(the, moduleID), C_PATH_MAX - 1);
	path[C_PATH_MAX - 1] = 0;
	if (c_realpath(path, real)) {
#if mxWindows
		DWORD attributes;
		attributes = GetFileAttributes(path);
		if (attributes != 0xFFFFFFFF) {
			if (attributes & FILE_ATTRIBUTE_DIRECTORY)
				return;
		}
#else
		struct stat a_stat;
		if (stat(path, &a_stat) == 0) {
			if (S_ISDIR(a_stat.st_mode)) 
				return;
		}
#endif	
		dot = c_strrchr(real, '.');
		if (dot && !c_strcmp(dot, ".json"))
			flags |= mxJSONModuleFlag;
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
	if (!c_strcmp(the->name, "xst_fuzz"))
		return;
	if (!c_strcmp(the->name, "xst_fuzz_oss"))
		return;
#ifdef mxMultipleThreads
	if (!c_strcmp(the->name, "xst262"))
		return;
	if (!c_strcmp(the->name, "xst-agent"))
		return;
#endif		
	char name[256];
	char* colon;
	int port;
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
#if mxWindows
{  	
	WSADATA wsaData;
	struct hostent *host;
	struct sockaddr_in address;
	unsigned long flag;
	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
		return;
	host = gethostbyname(name);
	if (!host)
		goto bail;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	memcpy(&(address.sin_addr), host->h_addr, host->h_length);
  	address.sin_port = htons(port);
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
	struct sockaddr_in address;
	int	flag;
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






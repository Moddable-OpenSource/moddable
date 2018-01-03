/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

#ifndef __XSALL__
#define __XSALL__

#include "xsCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define mxFrequency 1
#ifndef mxRegExp
	#define mxRegExp 1
#endif
#ifndef mxMachinePlatform
	#define mxMachinePlatform \
		void* host;
#endif

typedef struct sxMachine txMachine;
typedef struct sxSlot txSlot;
typedef struct sxBlock txBlock;
typedef struct sxChunk txChunk;
typedef struct sxJump txJump;
typedef struct sxProfileRecord txProfileRecord;
typedef struct sxCreation txCreation;
typedef struct sxPreparation txPreparation;
typedef struct sxHostFunctionBuilder txHostFunctionBuilder;
typedef struct sxHostHooks txHostHooks;
typedef struct sxInspectorNameLink txInspectorNameLink;
typedef struct sxInspectorNameList txInspectorNameList;

typedef txBoolean (*txArchiveRead)(void* src, size_t offset, void* buffer, size_t size);
typedef txBoolean (*txArchiveWrite)(void* dst, size_t offset, void* buffer, size_t size);
typedef void (*txCallback)(txMachine*);
typedef txCallback (*txCallbackAt)(txID index);
typedef void (*txDestructor)(void*);
typedef void (*txMarkRoot)(txMachine*, txSlot*);
typedef void (*txMarker)(txMachine*, void*, txMarkRoot);
typedef void (*txStep)(txMachine*, txSlot*, txID, txIndex, txSlot*);
typedef void (*txSweepRoot)(txMachine*, txSlot*);
typedef void (*txSweeper)(txMachine*, void*, txSweepRoot);
typedef void (*txTypeCallback)(txMachine*, txSlot*, txInteger, txSlot*, int);
typedef int (*txTypeCompare)(const void*, const void*);
typedef void (*txTypeOperator)(txMachine*, txSlot*, txInteger, txSlot*, int, int);

typedef void (*txBehaviorCall)(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments);
typedef void (*txBehaviorConstruct)(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target);
typedef txBoolean (*txBehaviorDefineOwnProperty)(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask);
typedef txBoolean (*txBehaviorDeleteProperty)(txMachine* the, txSlot* instance, txID id, txIndex index);
typedef txBoolean (*txBehaviorGetOwnProperty)(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot);
typedef txSlot* (*txBehaviorGetProperty)(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
typedef txBoolean (*txBehaviorGetPropertyValue)(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* receiver, txSlot* value);
typedef txBoolean (*txBehaviorGetPrototype)(txMachine* the, txSlot* instance, txSlot* result);
typedef txBoolean (*txBehaviorHasProperty)(txMachine* the, txSlot* instance, txID id, txIndex index);
typedef txBoolean (*txBehaviorIsExtensible)(txMachine* the, txSlot* instance);
typedef void (*txBehaviorOwnKeys)(txMachine* the, txSlot* instance, txFlag flag, txSlot* keys);
typedef txBoolean (*txBehaviorPreventExtensions)(txMachine* the, txSlot* instance);
typedef txSlot* (*txBehaviorSetProperty)(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
typedef txBoolean (*txBehaviorSetPropertyValue)(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver);
typedef txBoolean (*txBehaviorSetPrototype)(txMachine* the, txSlot* instance, txSlot* prototype);

typedef struct {
	txSlot* (*newAsyncInstance)(txMachine*);
	void (*runAsync)(txMachine*, txSlot*);
	void (*runAwait)(txMachine*, txSlot*);
	txSlot* (*newGeneratorInstance)(txMachine*);
	txSlot* (*newGeneratorFunctionInstance)(txMachine*, txID name);
} txDefaults;

enum {
	XS_ORDINARY_BEHAVIOR = 0,
	XS_ARGUMENTS_SLOPPY_BEHAVIOR,
	XS_ARGUMENTS_STRICT_BEHAVIOR,
	XS_ARRAY_BEHAVIOR,
	XS_ENVIRONMENT_BEHAVIOR,
	XS_GLOBAL_BEHAVIOR,
	XS_MODULE_BEHAVIOR,
	XS_PROXY_BEHAVIOR,
	XS_STRING_BEHAVIOR,
	XS_TYPED_ARRAY_BEHAVIOR,
	XS_BEHAVIOR_COUNT
};

typedef struct {
	txBehaviorGetProperty getProperty;
	txBehaviorSetProperty setProperty;
    txBehaviorCall call;
    txBehaviorConstruct construct;
    txBehaviorDefineOwnProperty defineOwnProperty;
    txBehaviorDeleteProperty deleteProperty;
    txBehaviorGetOwnProperty getOwnProperty;
    txBehaviorGetPropertyValue getPropertyValue;
    txBehaviorGetPrototype getPrototype;
    txBehaviorHasProperty hasProperty;
    txBehaviorIsExtensible isExtensible;
    txBehaviorOwnKeys ownKeys;
    txBehaviorPreventExtensions preventExtensions;
    txBehaviorSetPropertyValue setPropertyValue;
    txBehaviorSetPrototype setPrototype;
} txBehavior;

typedef struct {
    txInteger lo;
    txInteger hi;
} txSortPartition;
#define mxSortThreshold 4
#define mxSortStackSize 8 * sizeof(txUnsigned)

#define mxTypeArrayCount 9

typedef struct {
	txInteger size;
	txTypeCallback getter;
	txTypeCallback setter;
	txTypeCompare compare;
#ifndef __ets__
	txID getID;
	txID setID;
	txID constructorID;
#else
	// 4-byte aligned, and always read as 32-bits (avoid 16-bit read optimzagtion in gcc)
	volatile txInteger getID;
	volatile txInteger setID;
	volatile txInteger constructorID;
#endif
} txTypeDispatch;

typedef struct {
	txTypeCallback add;
	txTypeCallback and;
	txTypeCallback compareExchange;
	txTypeCallback exchange;
	txTypeCallback load;
	txTypeCallback or;
	txTypeCallback store;
	txTypeCallback sub;
	txTypeCallback xor;
} txTypeAtomics;

struct sxHostHooks {
	txDestructor destructor;
	txMarker marker;
	txSweeper sweeper;
};

typedef union {
	txBoolean boolean;
	txInteger integer;
	txNumber number;
	txString string;
	txID symbol;

	txSlot* reference;

	txSlot* closure;
	
	struct { union { txSlot* reference; txInteger count; } variable; txInteger line; } environment; // id == file
	struct { txByte* code; txSlot* scope; } frame;

	struct { txSlot* garbage; txSlot* prototype; } instance;
	
	struct { txSlot* address; txIndex length; } array;
	struct { txByte* address; txInteger length; } arrayBuffer;
	struct { txCallback address; txID* IDs; } callback;
	struct { txByte* address; txSlot* closures; } code;
	struct { txInteger offset; txInteger size; } dataView;
	struct { void* data; union { txDestructor destructor; txHostHooks* hooks; } variant; } host;
	struct { txSlot* handler; txSlot* target; } proxy;
	struct { void* code; void* data; } regexp;
	struct { txSlot* address; txIndex length; } stack;
	struct { txSlot** address; txSize length; } table;
	struct { txTypeDispatch* dispatch; txTypeAtomics* atomics; } typedArray;
	
	struct { txSlot* getter; txSlot* setter; } accessor;
	struct { txU4 index; txID id; } at;
	struct { txSlot* slot; txU4 sum; } entry;
	struct { txSlot* object; txSlot* module; } home;
	struct { txString string; txU4 sum; } key;
	struct { txSlot* first; txSlot* last; } list;
#ifdef mxHostFunctionPrimitive
	struct { const txHostFunctionBuilder* builder; txID* IDs; } hostFunction;
#endif
	struct { txSlot* cache; txSlot* instance; } hostInspector;
	struct { txSlot* slot; txInspectorNameLink* link; } instanceInspector;
	struct { txSlot* closure; txSlot* module; } export;
} txValue;

struct sxBlock {
	txBlock* nextBlock;
	txByte* current;
	txByte* limit;
	txByte* temporary;
};

struct sxChunk {
	txSize size;
	txByte* temporary;
};

struct sxJump {
	c_jmp_buf buffer; /* xs.h */
	txJump* nextJump; /* xs.h */
	txSlot* stack; /* xs.h */
	txSlot* scope; /* xs.h */
	txSlot* frame; /* xs.h */
	txSlot* environment; /* xs.h */
	txByte* code; /* xs.h */
	txBoolean flag; /* xs.h */
};

struct sxSlot {
	txSlot* next;
	txID ID;
	txFlag flag;
	txKind kind;
#if (!defined(linux)) && ((defined(__GNUC__) && defined(__LP64__)) || (defined(_MSC_VER) && defined(_M_X64)))
	// Made it alinged and consistent on all platforms
	txInteger dummy;
#endif
	txValue value;
};

struct sxInspectorNameLink {
	txInspectorNameLink* previous;
	txInspectorNameLink* next;
	txString name;
};

struct sxInspectorNameList {
	txInspectorNameLink *first;
	txInspectorNameLink* last;
};

struct sxProfileRecord {
	txInteger delta;
	txInteger profileID;
};

struct sxMachine {
	txSlot* stack; /* xs.h */
	txSlot* scope; /* xs.h */
	txSlot* frame; /* xs.h */
	txByte* code; /* xs.h */
	txSlot* stackBottom; /* xs.h */
	txSlot* stackTop; /* xs.h */
	txSlot* stackPrototypes; /* xs.h */
	txJump* firstJump; /* xs.h */
	void* context; /* xs.h */
	void* archive; /* xs.h */
	txSlot scratch; /* xs.h */
	mxMachinePlatform /* xs.h */
	txFlag status;
	
	txSlot* cRoot;

	txBlock* firstBlock;

	txSlot* freeHeap;
	txSlot* firstHeap;

	txSize nameModulo;
	txSlot** nameTable;
	txSize symbolModulo;
	txSlot** symbolTable;

	txSlot** keyArray;
	txID keyCount;
	txID keyIndex;
	int	keyOffset;
	txSlot** keyArrayHost;

	txID aliasCount;
	txID aliasIndex;
	txSlot** aliasArray;
	
	txSlot* firstWeakMapTable;
	txSlot* firstWeakSetTable;

	txSize currentChunksSize;
	txSize peakChunksSize;
	txSize maximumChunksSize;
	txSize minimumChunksSize;

	txSize currentHeapCount;
	txSize peakHeapCount;
	txSize maximumHeapCount;
	txSize minimumHeapCount;

	txBoolean shared;
	txMachine* sharedMachine;
	txSlot* sharedModules;

	txBoolean collectFlag;
	txFlag requireFlag;
	void* dtoa;
	void* preparation;

	char nameBuffer[256];
#ifdef mxDebug
	txString name;
	txFlag breakOnExceptionsFlag;
	txFlag breakOnStartFlag;
	txFlag debugAttribute;
	txFlag debugExit;
	txFlag debugState;
	txFlag debugTag;
	txFlag nameIndex;
	txFlag pathIndex;
	unsigned long idValue;
	txInteger lineValue;
	char pathValue[256];
	txInteger debugOffset;
	char debugBuffer[256];
	txInteger echoOffset;
	char echoBuffer[256];
#endif
#ifdef mxFrequency
	txNumber exits[XS_CODE_COUNT];
	txNumber frequencies[XS_CODE_COUNT];
#endif
#ifdef mxInstrument
	txSize garbageCollectionCount;
	txSize loadedModulesCount;
	txSize parserTotal;
	txSlot* stackPeak;
	void (*onBreak)(txMachine*, txU1 stop);
#endif
#ifdef mxProfile
	txString profileDirectory;
	void* profileFile;
	txInteger profileID;
	#if mxWindows
		LARGE_INTEGER profileFrequency;
		LARGE_INTEGER profileCounter;
	#else
		c_timeval profileTV;
	#endif
	txProfileRecord* profileBottom;
	txProfileRecord* profileCurrent;
	txProfileRecord* profileTop;
#endif
};

struct sxCreation {
	txSize initialChunkSize; /* xs.h */
	txSize incrementalChunkSize; /* xs.h */
	txSize initialHeapCount; /* xs.h */
	txSize incrementalHeapCount; /* xs.h */
	txSize stackCount; /* xs.h */
	txSize keyCount; /* xs.h */
	txSize nameModulo; /* xs.h */
	txSize symbolModulo; /* xs.h */
	txSize staticSize; /* xs.h */
};

struct sxPreparation {
	txS1 version[4];
	
	txSize aliasCount;
	txSize heapCount;
	txSlot* heap;
	txSize stackCount;
	txSlot* stack;

	txSize keyCount;
	txSlot** keys;
	txSize nameModulo;
	txSlot** names;
	txSize symbolModulo;
	txSlot** symbols;

	txSize baseLength;
	txString base;
	txSize scriptCount;
	txScript* scripts;
	
	txCreation creation;
	txString main;
	
	txU1 checksum[16];
};

struct sxHostFunctionBuilder {
	txCallback callback;
	txID length;
	txID id;
};

typedef struct {
	txSlot* slot;
	txSize offset;
	txSize size;
} txStringStream;

typedef struct {
	txString buffer;
	txSize offset;
	txSize size;
} txStringCStream;

/* xsAPI.c */

mxExport txKind fxTypeOf(txMachine*, txSlot*);

mxExport void fxPushCount(txMachine*, txInteger);
mxExport void fxUndefined(txMachine*, txSlot*);
mxExport void fxNull(txMachine*, txSlot*);
mxExport void fxBoolean(txMachine*, txSlot*, txS1);
mxExport txBoolean fxToBoolean(txMachine*, txSlot*);
mxExport void fxInteger(txMachine*, txSlot*, txInteger);
mxExport txInteger fxToInteger(txMachine*, txSlot*);
mxExport void fxNumber(txMachine*, txSlot*, txNumber);
mxExport txNumber fxToNumber(txMachine*, txSlot*);
mxExport void fxString(txMachine*, txSlot*, txString);
mxExport void fxStringBuffer(txMachine* the, txSlot* theSlot, txString theValue, txSize theSize);
mxExport txString fxToString(txMachine*, txSlot*);
mxExport txString fxToStringBuffer(txMachine*, txSlot*, txString, txSize);
mxExport txString fxToStringCopy(txMachine*, txSlot*);
mxExport void fxStringX(txMachine* the, txSlot* theSlot, txString theValue);
mxExport void fxUnsigned(txMachine*, txSlot*, txUnsigned);
mxExport txUnsigned fxToUnsigned(txMachine*, txSlot*);

mxExport void fxClosure(txMachine* the, txSlot* theSlot, txSlot* theClosure);
mxExport txSlot* fxToClosure(txMachine* the, txSlot* theSlot);
mxExport void fxReference(txMachine* the, txSlot* theSlot, txSlot* theReference);
mxExport txSlot* fxToReference(txMachine* the, txSlot* theSlot);

mxExport txSlot* fxNewArray(txMachine*, txInteger);
mxExport txSlot* fxNewObject(txMachine*);
mxExport txBoolean fxIsInstanceOf(txMachine*);
mxExport void fxArrayCacheBegin(txMachine*, txSlot*);
mxExport void fxArrayCacheEnd(txMachine*, txSlot*);
mxExport void fxArrayCacheItem(txMachine*, txSlot*, txSlot*);

mxExport void fxBuildHosts(txMachine*, txInteger, const txHostFunctionBuilder*);
mxExport txSlot* fxNewHostConstructor(txMachine*, txCallback, txInteger, txInteger);
mxExport txSlot* fxNewHostFunction(txMachine*, txCallback, txInteger, txInteger);
mxExport txSlot* fxNewHostInstance(txMachine* the);
mxExport txSlot* fxNewHostObject(txMachine*, txDestructor);
mxExport void* fxGetHostChunk(txMachine*, txSlot*);
mxExport void* fxGetHostChunkIf(txMachine*, txSlot*);
mxExport void* fxGetHostData(txMachine*, txSlot*);
mxExport void* fxGetHostDataIf(txMachine*, txSlot*);
mxExport void* fxGetHostHandle(txMachine*, txSlot*);
mxExport txDestructor fxGetHostDestructor(txMachine*, txSlot*);
mxExport txHostHooks* fxGetHostHooks(txMachine*, txSlot*);
mxExport void *fxSetHostChunk(txMachine* the, txSlot* slot, void* theValue, txSize theSize);
mxExport void fxSetHostData(txMachine*, txSlot*, void*);
mxExport void fxSetHostDestructor(txMachine*, txSlot*, txDestructor);
mxExport void fxSetHostHooks(txMachine*, txSlot*, txHostHooks*);
mxExport void* fxToHostHandle(txMachine* the, txSlot* slot);

mxExport txID fxID(txMachine*, txString);
mxExport txID fxFindID(txMachine* the, txString theName);
mxExport txS1 fxIsID(txMachine*, txString);
mxExport txID fxToID(txMachine* the, txSlot* theSlot);
mxExport txString fxName(txMachine*, txID);

mxExport void fxEnumerate(txMachine* the);
mxExport txBoolean fxHasAt(txMachine* the);
mxExport txBoolean fxHasID(txMachine*, txInteger);
mxExport txBoolean fxHasIndex(txMachine* the, txIndex index);
mxExport void fxGetAll(txMachine* the, txInteger id, txIndex index);
mxExport void fxGetAt(txMachine*);
mxExport void fxGetID(txMachine*, txInteger);
mxExport void fxGetIndex(txMachine*, txIndex);
mxExport void fxSetAll(txMachine* the, txInteger id, txIndex index);
mxExport void fxSetAt(txMachine*);
mxExport void fxSetID(txMachine*, txInteger);
mxExport void fxSetIndex(txMachine*, txIndex);
mxExport void fxDefineAll(txMachine* the, txID id, txIndex index, txFlag flag, txFlag mask);
mxExport void fxDefineAt(txMachine* the, txFlag flag, txFlag mask);
mxExport void fxDefineID(txMachine* the, txID id, txFlag flag, txFlag mask);
mxExport void fxDefineIndex(txMachine* the, txIndex index, txFlag flag, txFlag mask);
mxExport void fxDeleteAll(txMachine*, txInteger, txIndex);
mxExport void fxDeleteAt(txMachine*);
mxExport void fxDeleteID(txMachine*, txInteger);
mxExport void fxDeleteIndex(txMachine*, txIndex);
mxExport void fxCall(txMachine*);
mxExport void fxCallID(txMachine*, txInteger);
mxExport void fxNew(txMachine*);
mxExport void fxNewID(txMachine*, txInteger);
mxExport txBoolean fxRunTest(txMachine* the);

mxExport void fxVars(txMachine*, txInteger);

mxExport txInteger fxCheckArg(txMachine*, txInteger);
mxExport txInteger fxCheckVar(txMachine*, txInteger);
mxExport void fxOverflow(txMachine*, txInteger, txString thePath, txInteger theLine);
mxExport void fxThrow(txMachine* the, txString thePath, txInteger theLine) XS_FUNCTION_NORETURN;
mxExport void fxThrowMessage(txMachine* the, txString thePath, txInteger theLine, txError theError, txString theMessage, ...) XS_FUNCTION_NORETURN;
mxExport void fxDebugger(txMachine* the, txString thePath, txInteger theLine);

mxExport const txByte gxNoCode[] ICACHE_FLASH_ATTR;
mxExport txMachine* fxCreateMachine(txCreation* theCreation, txString theName, void* theContext);
mxExport void fxDeleteMachine(txMachine*);
mxExport txMachine* fxCloneMachine(txCreation* theCreation, txMachine* theMachine, txString theName, void* theContext);
mxExport void fxShareMachine(txMachine* the);

mxExport txMachine* fxBeginHost(txMachine*);
mxExport void fxEndHost(txMachine*);

mxExport void fxCollectGarbage(txMachine*);
mxExport void fxEnableGarbageCollection(txMachine* the, txBoolean enableIt);
mxExport void fxRemember(txMachine*, txSlot*);
mxExport void fxForget(txMachine*, txSlot*);
mxExport void fxAccess(txMachine*, txSlot*);

mxExport void fxCopyObject(txMachine* the);
mxExport void fxDemarshall(txMachine* the, void* theData, txBoolean alien);
mxExport void* fxMarshall(txMachine* the, txBoolean alien);
mxExport void fxModulePaths(txMachine* the);

mxExport void fxBuildArchiveKeys(txMachine* the);
mxExport void* fxGetArchiveCode(txMachine* the, txString path, txSize* size);
mxExport void* fxGetArchiveData(txMachine* the, txString path, txSize* size);
mxExport void* fxMapArchive(txPreparation* preparation, void* archive, void* stage, size_t bufferSize, txArchiveRead read, txArchiveWrite write);

/* xsmc.c */
mxExport void _xsNewArray(txMachine *the, txSlot *res, txInteger length);
mxExport void _xsNewObject(txMachine *the, txSlot *res);
mxExport void _xsNewHostInstance(txMachine*, txSlot*, txSlot*);
mxExport txBoolean _xsIsInstanceOf(txMachine*, txSlot*, txSlot*);
mxExport txBoolean _xsHas(txMachine*, txSlot*, txInteger);
mxExport void _xsGet(txMachine*, txSlot*, txSlot*, txInteger);
mxExport void _xsGetAt(txMachine*, txSlot*, txSlot*, txSlot*);
mxExport void _xsSet(txMachine*, txSlot*, txInteger, txSlot*);
mxExport void _xsSetAt(txMachine*, txSlot*, txSlot*, txSlot*);
mxExport void _xsDelete(txMachine*, txSlot*, txInteger);
mxExport void _xsDeleteAt(txMachine*, txSlot*, txSlot*);
mxExport void _xsCall(txMachine*, txSlot*, txSlot*, txInteger, ...);
mxExport void _xsNew(txMachine*, txSlot*, txSlot*, txInteger, ...);
mxExport txBoolean _xsTest(txMachine*, txSlot*);
mxExport txInteger fxIncrementalVars(txMachine*, txInteger);

/* xsPlatforms.c */
extern void* fxAllocateChunks(txMachine* the, txSize theSize);
extern txSlot* fxAllocateSlots(txMachine* the, txSize theCount);
extern void fxBuildKeys(txMachine* the);
extern void fxCreateMachinePlatform(txMachine* the);
extern void fxDeleteMachinePlatform(txMachine* the);
extern txID fxFindModule(txMachine* the, txID moduleID, txSlot* name);
extern void fxFreeChunks(txMachine* the, void* theChunks);
extern void fxFreeSlots(txMachine* the, void* theSlots);
extern void fxLoadModule(txMachine* the, txID moduleID);
extern void fxMarkHost(txMachine* the, txMarkRoot markRoot);
extern txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags);
extern void fxQueuePromiseJobs(txMachine* the);
extern void fxSweepHost(txMachine* the);
mxExport void fxInitializeSharedCluster();
mxExport void fxTerminateSharedCluster();
extern void* fxCreateSharedChunk(txInteger byteLength);
extern void fxLockSharedChunk(void* data);
extern txInteger fxMeasureSharedChunk(void* data);
extern void fxReleaseSharedChunk(void* data);
extern void* fxRetainSharedChunk(void* data);
extern void fxUnlockSharedChunk(void* data);
extern txInteger fxWaitSharedChunk(txMachine* the, void* data, txInteger offset, txInteger value, txNumber timeout);
extern txInteger fxWakeSharedChunk(txMachine* the, void* data, txInteger offset, txInteger count);
#ifdef mxDebug
extern void fxAbort(txMachine* the);
extern void fxConnect(txMachine* the);
extern void fxDisconnect(txMachine* the);
extern txBoolean fxIsConnected(txMachine* the);
extern txBoolean fxIsReadable(txMachine* the);
extern void fxReceive(txMachine* the);
extern void fxSend(txMachine* the, txBoolean more);
#endif
#ifdef mxProfile
extern void fxCloseProfileFile(txMachine* the);
extern void fxOpenProfileFile(txMachine* the, char* theName);
extern void fxWriteProfileFile(txMachine* the, void* theBuffer, txInteger theSize);
#endif

/* xsDefaults.c */
extern const txDefaults gxDefaults;
extern const txBehavior* gxBehaviors[];

/* xsAll.c */
extern txString fxAdornStringC(txMachine* the, txString prefix, txSlot* string, txString suffix);
extern txNumber fxArgToInteger(txMachine* the, txInteger i, txNumber value);
extern txSlot* fxArgToCallback(txMachine* the, txInteger argi);
extern void fxBufferFrameName(txMachine* the, txString buffer, txSize size, txSlot* frame, txString suffix);
extern void fxBufferFunctionName(txMachine* the, txString buffer, txSize size, txSlot* function, txString suffix);
extern void fxBufferObjectName(txMachine* the, txString buffer, txSize size, txSlot* object, txString suffix);
extern txString fxConcatString(txMachine* the, txSlot* a, txSlot* b);
extern txString fxConcatStringC(txMachine* the, txSlot* a, txString b);
extern txString fxCopyString(txMachine* the, txSlot* a, txSlot* b);
extern txString fxCopyStringC(txMachine* the, txSlot* a, txString b);
extern txBoolean fxIsCanonicalIndex(txMachine* the, txID id);
extern txString fxResizeString(txMachine* the, txSlot* a, txSize theSize);

extern int fxStringGetter(void*);
extern int fxStringCGetter(void*);
extern void fxJump(txMachine*) XS_FUNCTION_NORETURN;

/* xsRun.c */
extern void fxRunID(txMachine* the, txSlot* generator, txID theID);
extern void fxRunScript(txMachine* the, txScript* script, txSlot* _this, txSlot* _target, txSlot* environment, txSlot* object, txSlot* module);
extern txBoolean fxIsSameSlot(txMachine* the, txSlot* a, txSlot* b);
extern txBoolean fxIsSameValue(txMachine* the, txSlot* a, txSlot* b, txBoolean zero);

/* xsMemory.c */
extern void fxCheckStack(txMachine* the, txSlot* slot);
extern void fxAllocate(txMachine* the, txCreation* theCreation);
extern void fxCollect(txMachine* the, txBoolean theFlag);
mxExport txSlot* fxDuplicateSlot(txMachine* the, txSlot* theSlot);
extern void fxFree(txMachine* the);
mxExport void* fxNewChunk(txMachine* the, txSize theSize);
extern txSlot* fxNewSlot(txMachine* the);
mxExport void* fxRenewChunk(txMachine* the, void* theData, txSize theSize);
extern void fxShare(txMachine* the);

/* xsDebug.c */
#ifdef mxDebug
mxExport void fxCheck(txMachine* the, txString thePath, txInteger theLine);
extern void fxDebugCommand(txMachine* the);
extern void fxDebugLine(txMachine* the);
extern void fxDebugLoop(txMachine* the, txString thePath, txInteger theLine, txString message);
extern void fxDebugThrow(txMachine* the, txString thePath, txInteger theLine, txString message);
mxExport void fxLogin(txMachine* the);
mxExport void fxLogout(txMachine* the);
#endif
mxExport void fxReport(txMachine* the, txString theFormat, ...);
mxExport void fxReportError(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...);
mxExport void fxReportWarning(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...);
#ifdef mxInstrument	
extern void fxDescribeInstrumentation(txMachine* the, txInteger count, txString* names, txString* units);
extern void fxSampleInstrumentation(txMachine* the, txInteger count, txInteger* values);
#endif

/* xsType.c */
extern const txBehavior gxOrdinaryBehavior;

extern txSlot* fxNewEnvironmentInstance(txMachine* the, txSlot* environment);

extern txSlot* fxGetInstance(txMachine* the, txSlot* theSlot);
extern void fxPushSpeciesConstructor(txMachine* the, txSlot* constructor);

extern txSlot* fxNewInstance(txMachine* the);
extern txSlot* fxToInstance(txMachine* the, txSlot* theSlot);
extern void fxToPrimitive(txMachine* the, txSlot* theSlot, txBoolean theHint);
extern txFlag fxDescriptorToSlot(txMachine* the, txSlot* descriptor);
extern void fxDescribeProperty(txMachine* the, txSlot* property, txFlag mask);
extern txBoolean fxIsPropertyCompatible(txMachine* the, txSlot* property, txSlot* slot, txFlag mask);
mxExport void fx_species_get(txMachine* the);

extern void fxOrdinaryCall(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments);
extern void fxOrdinaryConstruct(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target);
extern txBoolean fxOrdinaryDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask);
extern txBoolean fxOrdinaryDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
extern txBoolean fxOrdinaryGetOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot);
extern txSlot* fxOrdinaryGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
extern txBoolean fxOrdinaryGetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* receiver, txSlot* value);
extern txBoolean fxOrdinaryGetPrototype(txMachine* the, txSlot* instance, txSlot* result);
extern txBoolean fxOrdinaryHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
extern txSlot* fxOrdinarySetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
extern txBoolean fxOrdinarySetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver);
extern txBoolean fxOrdinaryIsExtensible(txMachine* the, txSlot* instance);
extern void fxOrdinaryOwnKeys(txMachine* the, txSlot* target, txFlag flag, txSlot* keys);
extern txBoolean fxOrdinaryPreventExtensions(txMachine* the, txSlot* instance);
extern txBoolean fxOrdinarySetPrototype(txMachine* the, txSlot* instance, txSlot* prototype);

/* xsProperty.c */
extern txSlot* fxNextHostAccessorProperty(txMachine* the, txSlot* property, txCallback get, txCallback set, txID id, txFlag flag);
extern txSlot* fxNextHostFunctionProperty(txMachine* the, txSlot* property, txCallback call, txInteger length, txID id, txFlag flag);
extern txSlot* fxNewHostFunctionGlobal(txMachine* the, txCallback call, txInteger length, txID id, txFlag flag);
extern txSlot* fxNewHostConstructorGlobal(txMachine* the, txCallback call, txInteger length, txID id, txFlag flag);

extern txSlot* fxLastProperty(txMachine* the, txSlot* slot);
extern txSlot* fxNextUndefinedProperty(txMachine* the, txSlot* property, txID id, txFlag flag);
extern txSlot* fxNextNullProperty(txMachine* the, txSlot* property, txID id, txFlag flag);
extern txSlot* fxNextBooleanProperty(txMachine* the, txSlot* property, txBoolean boolean, txID id, txFlag flag);
extern txSlot* fxNextIntegerProperty(txMachine* the, txSlot* property, txInteger integer, txID id, txFlag flag);
extern txSlot* fxNextNumberProperty(txMachine* the, txSlot* property, txNumber number, txID id, txFlag flag);
extern txSlot* fxNextSlotProperty(txMachine* the, txSlot* property, txSlot* slot, txID id, txFlag flag);
extern txSlot* fxNextStringProperty(txMachine* the, txSlot* property, txString string, txID id, txFlag flag);
extern txSlot* fxNextStringXProperty(txMachine* the, txSlot* property, txString string, txID id, txFlag flag);
extern txSlot* fxNextSymbolProperty(txMachine* the, txSlot* property, txID symbol, txID id, txFlag flag);
extern txSlot* fxNextTypeDispatchProperty(txMachine* the, txSlot* property, txTypeDispatch* dispatch, txTypeAtomics* atomics, txID id, txFlag flag);

extern txSlot* fxQueueIDKeys(txMachine* the, txSlot* first, txFlag flag, txSlot* keys);
extern txSlot* fxQueueIndexKeys(txMachine* the, txSlot* array, txFlag flag, txSlot* keys);
extern txSlot* fxQueueKey(txMachine* the, txID id, txIndex index, txSlot* keys);

extern txBoolean fxDeleteIndexProperty(txMachine* the, txSlot* array, txIndex index);
extern txSlot* fxGetIndexProperty(txMachine* the, txSlot* array, txIndex index);
extern txIndex fxGetIndexSize(txMachine* the, txSlot* array);
extern txSlot* fxSetIndexProperty(txMachine* the, txSlot* instance, txSlot* array, txIndex index);
extern void fxSetIndexSize(txMachine* the, txSlot* array, txIndex target);

/* xsGlobal.c */
extern const txBehavior gxEnvironmentBehavior;
extern const txBehavior gxGlobalBehavior;
extern void fxBuildGlobal(txMachine* the);

extern txBoolean fxGlobalDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
extern txSlot* fxGlobalGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
extern txSlot* fxGlobalSetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);

mxExport void fx_Iterator_iterator(txMachine* the);
mxExport void fx_Enumerator(txMachine* the);
mxExport void fx_Enumerator_next(txMachine* the);
mxExport void fx_decodeURI(txMachine* the);
mxExport void fx_decodeURIComponent(txMachine* the);
mxExport void fx_encodeURI(txMachine* the);
mxExport void fx_encodeURIComponent(txMachine* the);
mxExport void fx_escape(txMachine* the);
mxExport void fx_eval(txMachine* the);
mxExport void fx_trace(txMachine* the);
mxExport void fx_unescape(txMachine* the);

extern txSlot* fxCheckIteratorInstance(txMachine* the, txSlot* slot);
extern void fxCloseIterator(txMachine* the, txSlot* iterator);
extern txSlot* fxNewIteratorInstance(txMachine* the, txSlot* iterable);
mxExport void fxDecodeURI(txMachine* the, txString theSet);
mxExport void fxEncodeURI(txMachine* the, txString theSet);

/* xsObject.c */
mxExport void fx_Object(txMachine* the);
mxExport void fx_Object_prototype___proto__get(txMachine* the);
mxExport void fx_Object_prototype___proto__set(txMachine* the);
mxExport void fx_Object_prototype_hasOwnProperty(txMachine* the);
mxExport void fx_Object_prototype_isPrototypeOf(txMachine* the);
mxExport void fx_Object_prototype_propertyIsEnumerable(txMachine* the);
mxExport void fx_Object_prototype_propertyIsScriptable(txMachine* the);
mxExport void fx_Object_prototype_toLocaleString(txMachine* the);
mxExport void fx_Object_prototype_toPrimitive(txMachine* the);
mxExport void fx_Object_prototype_toString(txMachine* the);
mxExport void fx_Object_prototype_valueOf(txMachine* the);
mxExport void fx_Object_assign(txMachine* the);
mxExport void fx_Object_create(txMachine* the);
mxExport void fx_Object_defineProperties(txMachine* the);
mxExport void fx_Object_defineProperty(txMachine* the);
mxExport void fx_Object_entries(txMachine* the);
mxExport void fx_Object_freeze(txMachine* the);
mxExport void fx_Object_getOwnPropertyDescriptor(txMachine* the);
mxExport void fx_Object_getOwnPropertyDescriptors(txMachine* the);
mxExport void fx_Object_getOwnPropertyNames(txMachine* the);
mxExport void fx_Object_getOwnPropertySymbols(txMachine* the);
mxExport void fx_Object_getPrototypeOf(txMachine* the);
mxExport void fx_Object_is(txMachine* the);
mxExport void fx_Object_isExtensible(txMachine* the);
mxExport void fx_Object_isFrozen(txMachine* the);
mxExport void fx_Object_isSealed(txMachine* the);
mxExport void fx_Object_keys(txMachine* the);
mxExport void fx_Object_preventExtensions(txMachine* the);
mxExport void fx_Object_seal(txMachine* the);
mxExport void fx_Object_setPrototypeOf(txMachine* the);
mxExport void fx_Object_values(txMachine* the);

extern void fxBuildObject(txMachine* the);
extern txSlot* fxNewObjectInstance(txMachine* the);
extern void fxFreezePropertyStep(txMachine* the, txSlot* context, txID id, txIndex index, txSlot* property);
extern void fxIsPropertyFrozenStep(txMachine* the, txSlot* context, txID id, txIndex index, txSlot* property);

/* xsFunction.c */
mxExport void fx_Function(txMachine* the);
mxExport void fx_Function_prototype_apply(txMachine* the);
mxExport void fx_Function_prototype_bind(txMachine* the);
mxExport void fx_Function_prototype_bound(txMachine* the);
mxExport void fx_Function_prototype_call(txMachine* the);
mxExport void fx_Function_prototype_hasInstance(txMachine* the);
mxExport void fx_Function_prototype_toString(txMachine* the);

extern void fxBuildFunction(txMachine* the);
extern void fxDefaultFunctionPrototype(txMachine* the);
extern txSlot* fxGetPrototypeFromConstructor(txMachine* the, txSlot* defaultPrototype);
extern txBoolean fxIsCallable(txMachine* the, txSlot* slot);
extern txBoolean fxIsFunction(txMachine* the, txSlot* slot);
extern txSlot* fxNewFunctionInstance(txMachine* the, txID name);
extern void fxRenameFunction(txMachine* the, txSlot* function, txInteger id, txInteger former, txString prefix);

mxExport void fx_AsyncFunction(txMachine* the);

extern txSlot* fxNewAsyncInstance(txMachine* the);
extern void fxRunAsync(txMachine* the, txSlot* instance);
extern void fxRunAwait(txMachine* the, txSlot* instance);

/* xsBoolean.c */
mxExport void fx_Boolean(txMachine* the);
mxExport void fx_Boolean_prototype_toString(txMachine* the);
mxExport void fx_Boolean_prototype_valueOf(txMachine* the);

extern void fxBuildBoolean(txMachine* the);
extern txSlot* fxNewBooleanInstance(txMachine* the);

/* xsSymbol.c */
mxExport void fx_Symbol(txMachine* the);
mxExport void fx_Symbol_for(txMachine* the);
mxExport void fx_Symbol_keyFor(txMachine* the);
mxExport void fx_Symbol_prototype_toPrimitive(txMachine* the);
mxExport void fx_Symbol_prototype_toString(txMachine* the);
mxExport void fx_Symbol_prototype_valueOf(txMachine* the);

extern void fxBuildSymbol(txMachine* the);
extern txSlot* fxNewSymbolInstance(txMachine* the);
extern void fxSymbolToString(txMachine* the, txSlot* slot);
extern txSlot* fxGetKey(txMachine* the, txID theID);
extern char* fxGetKeyName(txMachine* the, txID theID);
extern txID fxFindName(txMachine* the, txString theString);
extern txBoolean fxIsKeyName(txMachine* the, txID theID);
extern txBoolean fxIsKeySymbol(txMachine* the, txID theID);
extern txID fxNewName(txMachine* the, txSlot* theSlot);
extern txID fxNewNameC(txMachine* the, txString theString);
extern txID fxNewNameX(txMachine* the, txString theString);
extern txSlot* fxAt(txMachine* the, txSlot* slot);
extern void fxKeyAt(txMachine* the, txID id, txIndex index, txSlot* slot);
extern void fxIDToString(txMachine* the, txInteger id, txString theBuffer, txSize theSize);

/* xsError.c */
mxExport void fx_Error(txMachine* the);
mxExport void fx_Error_toString(txMachine* the);
mxExport void fx_EvalError(txMachine* the);
mxExport void fx_RangeError(txMachine* the);
mxExport void fx_ReferenceError(txMachine* the);
mxExport void fx_SyntaxError(txMachine* the);
mxExport void fx_TypeError(txMachine* the);
mxExport void fx_URIError(txMachine* the);

extern void fxBuildError(txMachine* the);

/* xsNumber.c */
mxExport void fx_isFinite(txMachine* the);
mxExport void fx_isNaN(txMachine* the);
mxExport void fx_parseFloat(txMachine* the);
mxExport void fx_parseInt(txMachine* the);
mxExport void fx_Number(txMachine* the);
mxExport void fx_Number_isFinite(txMachine* the);
mxExport void fx_Number_isInteger(txMachine* the);
mxExport void fx_Number_isNaN(txMachine* the);
mxExport void fx_Number_isSafeInteger(txMachine* the);
mxExport void fx_Number_prototype_toExponential(txMachine* the);
mxExport void fx_Number_prototype_toFixed(txMachine* the);
mxExport void fx_Number_prototype_toPrecision(txMachine* the);
mxExport void fx_Number_prototype_toString(txMachine* the);
mxExport void fx_Number_prototype_valueOf(txMachine* the);

extern void fxBuildNumber(txMachine* the);
extern txSlot* fxNewNumberInstance(txMachine* the);

/* xsMath.c */
mxExport void fx_Math_abs(txMachine* the);
mxExport void fx_Math_acos(txMachine* the);
mxExport void fx_Math_acosh(txMachine* the);
mxExport void fx_Math_asin(txMachine* the);
mxExport void fx_Math_asinh(txMachine* the);
mxExport void fx_Math_atan(txMachine* the);
mxExport void fx_Math_atanh(txMachine* the);
mxExport void fx_Math_atan2(txMachine* the);
mxExport void fx_Math_cbrt(txMachine* the);
mxExport void fx_Math_ceil(txMachine* the);
mxExport void fx_Math_clz32(txMachine* the);
mxExport void fx_Math_cos(txMachine* the);
mxExport void fx_Math_cosh(txMachine* the);
mxExport void fx_Math_exp(txMachine* the);
mxExport void fx_Math_expm1(txMachine* the);
mxExport void fx_Math_floor(txMachine* the);
mxExport void fx_Math_fround(txMachine* the);
mxExport void fx_Math_hypot(txMachine* the);
mxExport void fx_Math_imul(txMachine* the);
mxExport void fx_Math_log(txMachine* the);
mxExport void fx_Math_log1p(txMachine* the);
mxExport void fx_Math_log10(txMachine* the);
mxExport void fx_Math_log2(txMachine* the);
mxExport void fx_Math_max(txMachine* the);
mxExport void fx_Math_min(txMachine* the);
mxExport void fx_Math_pow(txMachine* the);
mxExport void fx_Math_random(txMachine* the);
mxExport void fx_Math_round(txMachine* the);
mxExport void fx_Math_sign(txMachine* the);
mxExport void fx_Math_sin(txMachine* the);
mxExport void fx_Math_sinh(txMachine* the);
mxExport void fx_Math_sqrt(txMachine* the);
mxExport void fx_Math_tan(txMachine* the);
mxExport void fx_Math_tanh(txMachine* the);
mxExport void fx_Math_trunc(txMachine* the);
mxExport void fx_Math_toInteger(txMachine* the);

extern void fxBuildMath(txMachine* the);

/* xsDate.c */
mxExport void fx_Date(txMachine* the);
mxExport void fx_Date_now(txMachine* the);
mxExport void fx_Date_parse(txMachine* the);
mxExport void fx_Date_UTC(txMachine* the);
mxExport void fx_Date_prototype_getMilliseconds(txMachine* the);
mxExport void fx_Date_prototype_getSeconds(txMachine* the);
mxExport void fx_Date_prototype_getMinutes(txMachine* the);
mxExport void fx_Date_prototype_getHours(txMachine* the);
mxExport void fx_Date_prototype_getDay(txMachine* the);
mxExport void fx_Date_prototype_getDate(txMachine* the);
mxExport void fx_Date_prototype_getMonth(txMachine* the);
mxExport void fx_Date_prototype_getYear(txMachine* the);
mxExport void fx_Date_prototype_getFullYear(txMachine* the);
mxExport void fx_Date_prototype_getUTCMilliseconds(txMachine* the);
mxExport void fx_Date_prototype_getUTCSeconds(txMachine* the);
mxExport void fx_Date_prototype_getUTCMinutes(txMachine* the);
mxExport void fx_Date_prototype_getUTCHours(txMachine* the);
mxExport void fx_Date_prototype_getUTCDay(txMachine* the);
mxExport void fx_Date_prototype_getUTCDate(txMachine* the);
mxExport void fx_Date_prototype_getUTCMonth(txMachine* the);
mxExport void fx_Date_prototype_getUTCFullYear(txMachine* the);
mxExport void fx_Date_prototype_getTimezoneOffset(txMachine* the);
mxExport void fx_Date_prototype_setMilliseconds(txMachine* the);
mxExport void fx_Date_prototype_setSeconds(txMachine* the);
mxExport void fx_Date_prototype_setMinutes(txMachine* the);
mxExport void fx_Date_prototype_setHours(txMachine* the);
mxExport void fx_Date_prototype_setDate(txMachine* the);
mxExport void fx_Date_prototype_setMonth(txMachine* the);
mxExport void fx_Date_prototype_setYear(txMachine* the);
mxExport void fx_Date_prototype_setFullYear(txMachine* the);
mxExport void fx_Date_prototype_setTime(txMachine* the);
mxExport void fx_Date_prototype_setUTCMilliseconds(txMachine* the);
mxExport void fx_Date_prototype_setUTCSeconds(txMachine* the);
mxExport void fx_Date_prototype_setUTCMinutes(txMachine* the);
mxExport void fx_Date_prototype_setUTCHours(txMachine* the);
mxExport void fx_Date_prototype_setUTCDate(txMachine* the);
mxExport void fx_Date_prototype_setUTCMonth(txMachine* the);
mxExport void fx_Date_prototype_setUTCFullYear(txMachine* the);
mxExport void fx_Date_prototype_toDateString(txMachine* the);
mxExport void fx_Date_prototype_toISOString(txMachine* the);
mxExport void fx_Date_prototype_toJSON(txMachine* the);
mxExport void fx_Date_prototype_toPrimitive(txMachine* the);
mxExport void fx_Date_prototype_toString(txMachine* the);
mxExport void fx_Date_prototype_toTimeString(txMachine* the);
mxExport void fx_Date_prototype_toUTCString(txMachine* the);
mxExport void fx_Date_prototype_valueOf(txMachine* the);

extern void fxBuildDate(txMachine* the);
extern txNumber fxDateNow();

/* xsString.c */
extern const txBehavior gxStringBehavior;

mxExport void fx_String(txMachine* the);
mxExport void fx_String_fromArrayBuffer(txMachine* the);
mxExport void fx_String_fromCharCode(txMachine* the);
mxExport void fx_String_fromCodePoint(txMachine* the);
mxExport void fx_String_raw(txMachine* the);
mxExport void fx_String_prototype_charAt(txMachine* the);
mxExport void fx_String_prototype_charCodeAt(txMachine* the);
mxExport void fx_String_prototype_codePointAt(txMachine* the);
mxExport void fx_String_prototype_compare(txMachine* the);
mxExport void fx_String_prototype_concat(txMachine* the);
mxExport void fx_String_prototype_endsWith(txMachine* the);
mxExport void fx_String_prototype_includes(txMachine* the);
mxExport void fx_String_prototype_indexOf(txMachine* the);
mxExport void fx_String_prototype_lastIndexOf(txMachine* the);
mxExport void fx_String_prototype_match(txMachine* the);
mxExport void fx_String_prototype_normalize(txMachine* the);
mxExport void fx_String_prototype_padEnd(txMachine* the);
mxExport void fx_String_prototype_padStart(txMachine* the);
mxExport void fx_String_prototype_repeat(txMachine* the);
mxExport void fx_String_prototype_replace(txMachine* the);
mxExport void fx_String_prototype_search(txMachine* the);
mxExport void fx_String_prototype_slice(txMachine* the);
mxExport void fx_String_prototype_split(txMachine* the);
mxExport void fx_String_prototype_startsWith(txMachine* the);
mxExport void fx_String_prototype_substr(txMachine* the);
mxExport void fx_String_prototype_substring(txMachine* the);
mxExport void fx_String_prototype_toLowerCase(txMachine* the);
mxExport void fx_String_prototype_toUpperCase(txMachine* the);
mxExport void fx_String_prototype_trim(txMachine* the);
mxExport void fx_String_prototype_valueOf(txMachine* the);
mxExport void fx_String_prototype_iterator(txMachine* the);
mxExport void fx_String_prototype_iterator_next(txMachine* the);
mxExport void fxStringAccessorGetter(txMachine* the);
mxExport void fxStringAccessorSetter(txMachine* the);

extern void fxBuildString(txMachine* the);
extern txSlot* fxNewStringInstance(txMachine* the);
extern txSlot* fxAccessStringProperty(txMachine* the, txSlot* instance, txInteger index);
extern void fxPushSubstitutionString(txMachine* the, txSlot* string, txInteger size, txInteger offset, txSlot* match, txInteger length, txInteger count, txSlot* captures, txSlot* replace);

/* xsRegExp.c */
mxExport void fx_RegExp(txMachine* the);
mxExport void fx_RegExp_prototype_get_dotAll(txMachine* the);
mxExport void fx_RegExp_prototype_get_flags(txMachine* the);
mxExport void fx_RegExp_prototype_get_global(txMachine* the);
mxExport void fx_RegExp_prototype_get_ignoreCase(txMachine* the);
mxExport void fx_RegExp_prototype_get_multiline(txMachine* the);
mxExport void fx_RegExp_prototype_get_source(txMachine* the);
mxExport void fx_RegExp_prototype_get_sticky(txMachine* the);
mxExport void fx_RegExp_prototype_get_unicode(txMachine* the);
mxExport void fx_RegExp_prototype_compile(txMachine* the);
mxExport void fx_RegExp_prototype_exec(txMachine* the);
mxExport void fx_RegExp_prototype_match(txMachine* the);
mxExport void fx_RegExp_prototype_replace(txMachine* the);
mxExport void fx_RegExp_prototype_search(txMachine* the);
mxExport void fx_RegExp_prototype_split(txMachine* the);
mxExport void fx_RegExp_prototype_test(txMachine* the);
mxExport void fx_RegExp_prototype_toString(txMachine* the);
mxExport void fxInitializeRegExp(txMachine* the);

extern void fxBuildRegExp(txMachine* the);
extern txBoolean fxIsRegExp(txMachine* the, txSlot* slot);
extern txSlot* fxNewRegExpInstance(txMachine* the);

/* xsArray.c */
extern const txBehavior gxArrayBehavior;

extern void fxBuildArray(txMachine* the);

mxExport void fxArrayLengthGetter(txMachine* the);
mxExport void fxArrayLengthSetter(txMachine* the);

mxExport void fx_Array(txMachine* the);
mxExport void fx_Array_from(txMachine* the);
mxExport void fx_Array_isArray(txMachine* the);
mxExport void fx_Array_of(txMachine* the);
mxExport void fx_Array_prototype_concat(txMachine* the);
mxExport void fx_Array_prototype_copyWithin(txMachine* the);
mxExport void fx_Array_prototype_entries(txMachine* the);
mxExport void fx_Array_prototype_every(txMachine* the);
mxExport void fx_Array_prototype_fill(txMachine* the);
mxExport void fx_Array_prototype_filter(txMachine* the);
mxExport void fx_Array_prototype_find(txMachine* the);
mxExport void fx_Array_prototype_findIndex(txMachine* the);
mxExport void fx_Array_prototype_forEach(txMachine* the);
mxExport void fx_Array_prototype_includes(txMachine* the);
mxExport void fx_Array_prototype_indexOf(txMachine* the);
mxExport void fx_Array_prototype_join(txMachine* the);
mxExport void fx_Array_prototype_keys(txMachine* the);
mxExport void fx_Array_prototype_lastIndexOf(txMachine* the);
mxExport void fx_Array_prototype_map(txMachine* the);
mxExport void fx_Array_prototype_pop(txMachine* the);
mxExport void fx_Array_prototype_push(txMachine* the);
mxExport void fx_Array_prototype_reduce(txMachine* the);
mxExport void fx_Array_prototype_reduceRight(txMachine* the);
mxExport void fx_Array_prototype_reverse(txMachine* the);
mxExport void fx_Array_prototype_shift(txMachine* the);
mxExport void fx_Array_prototype_slice(txMachine* the);
mxExport void fx_Array_prototype_some(txMachine* the);
mxExport void fx_Array_prototype_sort(txMachine* the);
mxExport void fx_Array_prototype_splice(txMachine* the);
mxExport void fx_Array_prototype_toLocaleString(txMachine* the);
mxExport void fx_Array_prototype_toString(txMachine* the);
mxExport void fx_Array_prototype_unshift(txMachine* the);
mxExport void fx_Array_prototype_values(txMachine* the);
mxExport void fx_ArrayIterator_prototype_next(txMachine* the);

extern txNumber fxArgToIndex(txMachine* the, txInteger argi, txNumber index, txNumber length);
extern txNumber fxArgToLastIndex(txMachine* the, txInteger argi, txNumber index, txNumber length);
extern txNumber fxArgToRange(txMachine* the, txInteger argi, txNumber index, txNumber min, txNumber max);
extern void fxCacheArray(txMachine* the, txSlot* theArray);
extern void fxConstructArrayEntry(txMachine* the, txSlot* entry);
extern txBoolean fxIsArray(txMachine* the, txSlot* instance);
extern txSlot* fxNewArrayInstance(txMachine* the);
extern txNumber fxToLength(txMachine* the, txSlot* slot);

extern const txBehavior gxArgumentsSloppyBehavior;

extern txSlot* fxNewArgumentsSloppyInstance(txMachine* the, txIndex count);
extern const txBehavior gxArgumentsStrictBehavior;

extern txSlot* fxNewArgumentsStrictInstance(txMachine* the, txIndex count);
mxExport void fxThrowTypeError(txMachine* the);

/* xsDataView.c */
extern int fxFloat32Compare(const void* p, const void* q);
extern void fxFloat32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern void fxFloat32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern int fxFloat64Compare(const void* p, const void* q);
extern void fxFloat64Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern void fxFloat64Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern int fxInt8Compare(const void* p, const void* q);
extern void fxInt8Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern void fxInt8Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern int fxInt16Compare(const void* p, const void* q);
extern void fxInt16Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern int fxInt32Compare(const void* p, const void* q);
extern void fxInt32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern int fxUint8Compare(const void* p, const void* q);
extern void fxUint8Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern int fxUint16Compare(const void* p, const void* q);
extern void fxUint16Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern int fxUint32Compare(const void* p, const void* q);
extern void fxUint32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern void fxUint8ClampedSetter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);

mxExport void fxArrayBuffer(txMachine* the, txSlot* slot, void* data, txInteger byteLength);
mxExport void fxGetArrayBufferData(txMachine* the, txSlot* slot, txInteger byteOffset, void* data, txInteger byteLength);
mxExport txInteger fxGetArrayBufferLength(txMachine* the, txSlot* slot);
mxExport void fxSetArrayBufferData(txMachine* the, txSlot* slot, txInteger byteOffset, void* data, txInteger byteLength);
mxExport void fxSetArrayBufferLength(txMachine* the, txSlot* slot, txInteger byteLength);
mxExport void* fxToArrayBuffer(txMachine* the, txSlot* slot);


mxExport void fx_ArrayBuffer(txMachine* the);
mxExport void fx_ArrayBuffer_fromString(txMachine* the);
mxExport void fx_ArrayBuffer_isView(txMachine* the);
mxExport void fx_ArrayBuffer_prototype_get_byteLength(txMachine* the);
mxExport void fx_ArrayBuffer_prototype_concat(txMachine* the);
mxExport void fx_ArrayBuffer_prototype_slice(txMachine* the);

mxExport void fx_DataView(txMachine* the);
mxExport void fx_DataView_prototype_buffer_get(txMachine* the);
mxExport void fx_DataView_prototype_byteLength_get(txMachine* the);
mxExport void fx_DataView_prototype_byteOffset_get(txMachine* the);
mxExport void fx_DataView_prototype_getFloat32(txMachine* the);
mxExport void fx_DataView_prototype_getFloat64(txMachine* the);
mxExport void fx_DataView_prototype_getInt8(txMachine* the);
mxExport void fx_DataView_prototype_getInt16(txMachine* the);
mxExport void fx_DataView_prototype_getInt32(txMachine* the);
mxExport void fx_DataView_prototype_getUint8(txMachine* the);
mxExport void fx_DataView_prototype_getUint16(txMachine* the);
mxExport void fx_DataView_prototype_getUint32(txMachine* the);
mxExport void fx_DataView_prototype_setFloat32(txMachine* the);
mxExport void fx_DataView_prototype_setFloat64(txMachine* the);
mxExport void fx_DataView_prototype_setInt8(txMachine* the);
mxExport void fx_DataView_prototype_setInt16(txMachine* the);
mxExport void fx_DataView_prototype_setInt32(txMachine* the);
mxExport void fx_DataView_prototype_setUint8(txMachine* the);
mxExport void fx_DataView_prototype_setUint16(txMachine* the);
mxExport void fx_DataView_prototype_setUint32(txMachine* the);
mxExport void fx_DataView_prototype_setUint8Clamped(txMachine* the);

mxExport const txTypeDispatch gxTypeDispatches[];
mxExport const txBehavior gxTypedArrayBehavior;

mxExport void fxTypedArrayGetter(txMachine* the);
mxExport void fxTypedArraySetter(txMachine* the);

mxExport void fx_TypedArray(txMachine* the);
mxExport void fx_TypedArray_from(txMachine* the);
mxExport void fx_TypedArray_of(txMachine* the);
mxExport void fx_TypedArray_prototype_buffer_get(txMachine* the);
mxExport void fx_TypedArray_prototype_byteLength_get(txMachine* the);
mxExport void fx_TypedArray_prototype_byteOffset_get(txMachine* the);
mxExport void fx_TypedArray_prototype_copyWithin(txMachine* the);
mxExport void fx_TypedArray_prototype_entries(txMachine* the);
mxExport void fx_TypedArray_prototype_every(txMachine* the);
mxExport void fx_TypedArray_prototype_fill(txMachine* the);
mxExport void fx_TypedArray_prototype_filter(txMachine* the);
mxExport void fx_TypedArray_prototype_find(txMachine* the);
mxExport void fx_TypedArray_prototype_findIndex(txMachine* the);
mxExport void fx_TypedArray_prototype_forEach(txMachine* the);
mxExport void fx_TypedArray_prototype_includes(txMachine* the);
mxExport void fx_TypedArray_prototype_indexOf(txMachine* the);
mxExport void fx_TypedArray_prototype_join(txMachine* the);
mxExport void fx_TypedArray_prototype_keys(txMachine* the);
mxExport void fx_TypedArray_prototype_lastIndexOf(txMachine* the);
mxExport void fx_TypedArray_prototype_length_get(txMachine* the);
mxExport void fx_TypedArray_prototype_map(txMachine* the);
mxExport void fx_TypedArray_prototype_reduce(txMachine* the);
mxExport void fx_TypedArray_prototype_reduceRight(txMachine* the);
mxExport void fx_TypedArray_prototype_reverse(txMachine* the);
mxExport void fx_TypedArray_prototype_set(txMachine* the);
mxExport void fx_TypedArray_prototype_slice(txMachine* the);
mxExport void fx_TypedArray_prototype_some(txMachine* the);
mxExport void fx_TypedArray_prototype_sort(txMachine* the);
mxExport void fx_TypedArray_prototype_subarray(txMachine* the);
mxExport void fx_TypedArray_prototype_toLocaleString(txMachine* the);
mxExport void fx_TypedArray_prototype_toStringTag_get(txMachine* the);
mxExport void fx_TypedArray_prototype_values(txMachine* the);

extern void fxBuildDataView(txMachine* the);

/* xsAtomics.c */
extern void fxInt8Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

mxExport const txTypeAtomics gxTypeAtomics[];
extern void fxBuildAtomics(txMachine* the);

mxExport void fx_SharedArrayBuffer(txMachine* the);
mxExport void fx_SharedArrayBuffer_prototype_get_byteLength(txMachine* the);
mxExport void fx_SharedArrayBuffer_prototype_slice(txMachine* the);
mxExport void fx_Atomics_add(txMachine* the);
mxExport void fx_Atomics_and(txMachine* the);
mxExport void fx_Atomics_compareExchange(txMachine* the);
mxExport void fx_Atomics_exchange(txMachine* the);
mxExport void fx_Atomics_isLockFree(txMachine* the);
mxExport void fx_Atomics_load(txMachine* the);
mxExport void fx_Atomics_or(txMachine* the);
mxExport void fx_Atomics_store(txMachine* the);
mxExport void fx_Atomics_sub(txMachine* the);
mxExport void fx_Atomics_wait(txMachine* the);
mxExport void fx_Atomics_wake(txMachine* the);
mxExport void fx_Atomics_xor(txMachine* the);

/* xsMapSet.c */
mxExport void fx_Map(txMachine* the);
mxExport void fx_Map_prototype_clear(txMachine* the);
mxExport void fx_Map_prototype_delete(txMachine* the);
mxExport void fx_Map_prototype_entries(txMachine* the);
mxExport void fx_Map_prototype_entries_next(txMachine* the);
mxExport void fx_Map_prototype_forEach(txMachine* the);
mxExport void fx_Map_prototype_get(txMachine* the);
mxExport void fx_Map_prototype_has(txMachine* the);
mxExport void fx_Map_prototype_keys(txMachine* the);
mxExport void fx_Map_prototype_keys_next(txMachine* the);
mxExport void fx_Map_prototype_set(txMachine* the);
mxExport void fx_Map_prototype_size(txMachine* the);
mxExport void fx_Map_prototype_values(txMachine* the);
mxExport void fx_Map_prototype_values_next(txMachine* the);
mxExport void fx_Set(txMachine* the);
mxExport void fx_Set_prototype_add(txMachine* the);
mxExport void fx_Set_prototype_clear(txMachine* the);
mxExport void fx_Set_prototype_delete(txMachine* the);
mxExport void fx_Set_prototype_entries(txMachine* the);
mxExport void fx_Set_prototype_entries_next(txMachine* the);
mxExport void fx_Set_prototype_forEach(txMachine* the);
mxExport void fx_Set_prototype_has(txMachine* the);
mxExport void fx_Set_prototype_size(txMachine* the);
mxExport void fx_Set_prototype_values(txMachine* the);
mxExport void fx_Set_prototype_values_next(txMachine* the);
mxExport void fx_WeakMap(txMachine* the);
mxExport void fx_WeakMap_prototype_delete(txMachine* the);
mxExport void fx_WeakMap_prototype_get(txMachine* the);
mxExport void fx_WeakMap_prototype_has(txMachine* the);
mxExport void fx_WeakMap_prototype_set(txMachine* the);
mxExport void fx_WeakSet(txMachine* the);
mxExport void fx_WeakSet_prototype_add(txMachine* the);
mxExport void fx_WeakSet_prototype_delete(txMachine* the);
mxExport void fx_WeakSet_prototype_has(txMachine* the);

extern void fxBuildMapSet(txMachine* the);
extern txSlot* fxNewMapInstance(txMachine* the);
extern txSlot* fxNewSetInstance(txMachine* the);
extern txSlot* fxNewWeakMapInstance(txMachine* the);
extern txSlot* fxNewWeakSetInstance(txMachine* the);

/* xsJSON.c */
mxExport void fx_JSON_parse(txMachine* the);
mxExport void fx_JSON_stringify(txMachine* the);

extern void fxBuildJSON(txMachine* the);

/* xsGenerator.c */
mxExport void fx_Generator(txMachine* the);
mxExport void fx_Generator_prototype_next(txMachine* the);
mxExport void fx_Generator_prototype_return(txMachine* the);
mxExport void fx_Generator_prototype_throw(txMachine* the);
mxExport void fx_GeneratorFunction(txMachine* the);

extern void fxBuildGenerator(txMachine* the);
extern txSlot* fxNewGeneratorInstance(txMachine* the);
extern txSlot* fxNewGeneratorFunctionInstance(txMachine* the, txID name);

/* xsPromise.c */
mxExport void fx_Promise(txMachine* the);
mxExport void fx_Promise_all(txMachine* the);
mxExport void fx_Promise_race(txMachine* the);
mxExport void fx_Promise_reject(txMachine* the);
mxExport void fx_Promise_resolve(txMachine* the);
mxExport void fx_Promise_prototype_catch(txMachine* the);
mxExport void fx_Promise_prototype_then(txMachine* the);
mxExport void fxOnRejectedPromise(txMachine* the);
mxExport void fxOnResolvedPromise(txMachine* the);
mxExport void fxRejectPromise(txMachine* the);
mxExport void fxResolvePromise(txMachine* the);

extern void fxBuildPromise(txMachine* the);
extern txSlot* fxNewPromiseAlready(txMachine* the);
extern txSlot* fxNewPromiseFunction(txMachine* the, txSlot* already, txSlot* promise, txSlot* function);
extern txSlot* fxNewPromiseInstance(txMachine* the);
extern void fxRunPromiseJobs(txMachine* the);

/* xsProxy.c */
extern void fxBuildProxy(txMachine* the);

mxExport const txBehavior gxProxyBehavior;

mxExport void fxProxyGetter(txMachine* the);
mxExport void fxProxySetter(txMachine* the);

mxExport void fx_Proxy(txMachine* the);
mxExport void fx_Proxy_revocable(txMachine* the);
mxExport void fx_Proxy_revoke(txMachine* the);
mxExport void fx_Reflect_apply(txMachine* the);
mxExport void fx_Reflect_construct(txMachine* the);
mxExport void fx_Reflect_defineProperty(txMachine* the);
mxExport void fx_Reflect_deleteProperty(txMachine* the);
mxExport void fx_Reflect_get(txMachine* the);
mxExport void fx_Reflect_getOwnPropertyDescriptor(txMachine* the);
mxExport void fx_Reflect_getPrototypeOf(txMachine* the);
mxExport void fx_Reflect_has(txMachine* the);
mxExport void fx_Reflect_isExtensible(txMachine* the);
mxExport void fx_Reflect_ownKeys(txMachine* the);
mxExport void fx_Reflect_preventExtensions(txMachine* the);
mxExport void fx_Reflect_set(txMachine* the);
mxExport void fx_Reflect_setPrototypeOf(txMachine* the);

/* xsModule.c */
extern const txBehavior gxModuleBehavior;

extern void fxBuildModule(txMachine* the);

extern txID fxCurrentModuleID(txMachine* the);
extern txSlot* fxRequireModule(txMachine* the, txID moduleID, txSlot* name);
extern void fxResolveModule(txMachine* the, txID moduleID, txScript* script, void* data, txDestructor destructor);
extern void fxSetModule(txMachine* the, txID moduleID, txSlot* module);

mxExport void fx_require(txMachine* the);
mxExport void fx_require_get_busy(txMachine* the);
mxExport void fx_require_set_busy(txMachine* the);
mxExport void fx_require_get_cache(txMachine* the);
mxExport void fx_require_get_uri(txMachine* the);
mxExport void fx_require_resolve(txMachine* the);
mxExport void fx_require_weak(txMachine* the);
mxExport void fx_Module(txMachine* the);
mxExport void fx_Transfer(txMachine* the);

/* xsProfile.c */
#ifdef mxProfile
extern void fxBeginFunction(txMachine* the, txSlot* function);
extern void fxBeginGC(txMachine* the);
extern void fxEndFunction(txMachine* the, txSlot* function);
extern void fxEndGC(txMachine* the);
extern void fxJumpFrames(txMachine* the, txSlot* from, txSlot* to);
#endif
mxExport txS1 fxIsProfiling(txMachine* the);
mxExport void fxStartProfiling(txMachine* the);
mxExport void fxStopProfiling(txMachine* the);

enum {
	XS_NO_ERROR = 0,
	XS_UNKNOWN_ERROR,
	XS_EVAL_ERROR,
	XS_RANGE_ERROR,
	XS_REFERENCE_ERROR,
	XS_SYNTAX_ERROR,
	XS_TYPE_ERROR,
	XS_URI_ERROR,
	XS_ERROR_COUNT
};

enum {
	XS_NO_STATUS = 0,
	XS_RETURN_STATUS = 1,
	XS_THROW_STATUS = 2,

	XS_NO_HINT = 0,
	XS_NUMBER_HINT = 1,
	XS_STRING_HINT = 2,

	XS_NO_FLAG = 0,
	XS_REQUIRE_FLAG = 1,
	XS_REQUIRE_WEAK_FLAG = 2,
	
	XS_OWN = 0,
	XS_ANY = 1,

	/* frame flags */
	/* ? = 1, */
	XS_C_FLAG = 2,
	/* ? =  = 4, */
	XS_STEP_INTO_FLAG = 8,
	XS_STEP_OVER_FLAG = 16,
	XS_STRICT_FLAG = 32,
	XS_DEBUG_FLAG = 64,
	XS_MARK_FLAG = 128,

	/* instance flags */
	XS_EXOTIC_FLAG = 1,
	XS_CAN_CONSTRUCT_FLAG = 2,
	XS_BASE_FLAG = 4,
	XS_DERIVED_FLAG = 8,
	XS_DONT_PATCH_FLAG = 16,
	XS_LEVEL_FLAG = 32,
	/* ? = 64, */
	/* XS_MARK_FLAG = 128, */

	/* property flags */
	XS_INTERNAL_FLAG = 1,
	/* XS_DONT_DELETE_FLAG = 2, */
	/* XS_DONT_ENUM_FLAG = 4, */
	/* XS_DONT_SET_FLAG = 8 ,  */
	XS_INSPECTOR_FLAG = 16,
	/* ? = 32, */
	/* ? = 64, */
	/* XS_MARK_FLAG = 128, */

	/* mxBehaviorOwnKeys flags */
	XS_EACH_NAME_FLAG = 1,
	XS_EACH_SYMBOL_FLAG = 2,
	
	/* mxBehaviorDefineOwnProperty flags */
	/* XS_DONT_DELETE_FLAG = 2, */
	/* XS_DONT_ENUM_FLAG = 4, */
	/* XS_DONT_SET_FLAG = 8, */
	/* XS_METHOD_FLAG = 16, */
	/* XS_GETTER_FLAG = 32, */
	/* XS_SETTER_FLAG = 64, */
	XS_ACCESSOR_FLAG = XS_GETTER_FLAG | XS_SETTER_FLAG,
	XS_GET_ONLY = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG,

	/* collect flags */
	XS_COLLECTING_FLAG = 1,
	XS_TRASHING_FLAG = 2,
	XS_SKIPPED_COLLECT_FLAG = 4,
	XS_HOST_CHUNK_FLAG = 32,
	XS_HOST_HOOKS_FLAG = 64
};

enum {
	XS_UNINITIALIZED_KIND = -1,
	XS_UNDEFINED_KIND = 0,
	XS_NULL_KIND,
	XS_BOOLEAN_KIND,
	XS_INTEGER_KIND,
	XS_NUMBER_KIND,
	XS_STRING_KIND, // 5
	XS_STRING_X_KIND,
	XS_SYMBOL_KIND,
	
	XS_REFERENCE_KIND,
	
	XS_CLOSURE_KIND, 
	XS_FRAME_KIND, //10

	XS_INSTANCE_KIND,
	
	XS_ARGUMENTS_SLOPPY_KIND,
	XS_ARGUMENTS_STRICT_KIND,
	XS_ARRAY_KIND,
	XS_ARRAY_BUFFER_KIND, // 15
	XS_CALLBACK_KIND,
	XS_CODE_KIND,
	XS_CODE_X_KIND,
	XS_DATE_KIND,
	XS_DATA_VIEW_KIND, // 20
	XS_GLOBAL_KIND,
	XS_HOST_KIND,
	XS_MAP_KIND,
	XS_MODULE_KIND,
	XS_PROMISE_KIND, // 25
	XS_PROXY_KIND,
	XS_REGEXP_KIND,
	XS_SET_KIND,
	XS_TYPED_ARRAY_KIND,
	XS_WEAK_MAP_KIND, // 30
	XS_WEAK_SET_KIND,
	XS_WITH_KIND,

	XS_ACCESSOR_KIND,
	XS_AT_KIND,
	XS_ENTRY_KIND,
	XS_ERROR_KIND,
	XS_HOME_KIND,
	XS_KEY_KIND,
	XS_KEY_X_KIND,
	XS_LIST_KIND, //40
	XS_STACK_KIND,
	XS_VAR_KIND,
	XS_CALLBACK_X_KIND,
#ifdef mxHostFunctionPrimitive
	XS_HOST_FUNCTION_KIND,
#endif
	XS_HOST_INSPECTOR_KIND,
	XS_INSTANCE_INSPECTOR_KIND,
	XS_EXPORT_KIND,
};

#define mxTry(THE_MACHINE) \
	txJump __JUMP__; \
	__JUMP__.nextJump = (THE_MACHINE)->firstJump; \
	__JUMP__.stack = the->stack; \
	__JUMP__.scope = the->scope; \
	__JUMP__.frame = the->frame; \
	__JUMP__.code = the->code; \
	__JUMP__.flag = 0; \
	(THE_MACHINE)->firstJump = &__JUMP__; \
	if (c_setjmp(__JUMP__.buffer) == 0) {

#define mxCatch(THE_MACHINE) \
		(THE_MACHINE)->firstJump = __JUMP__.nextJump; \
	} \
	else for ( \
		the->stack = __JUMP__.stack, \
		the->scope = __JUMP__.scope, \
		the->frame = __JUMP__.frame, \
		the->code = __JUMP__.code, \
		(THE_MACHINE)->firstJump = __JUMP__.nextJump; \
		(__JUMP__.stack); \
		__JUMP__.stack = NULL)


#ifdef mxDebug
#define mxCheck(THE, THE_ASSERTION) \
	if (!(THE_ASSERTION)) \
		fxCheck(THE, __FILE__,__LINE__)
#else
#define mxCheck(THE, THE_ASSERTION)
#endif

#define mxThrowMessage(_CODE,...) fxThrowMessage(the, C_NULL, 0, _CODE, __VA_ARGS__)

#ifdef mxDebug
#define mxUnknownError(...) fxThrowMessage(the, NULL, 0, XS_UNKNOWN_ERROR, __VA_ARGS__)
#define mxEvalError(...) fxThrowMessage(the, NULL, 0, XS_EVAL_ERROR, __VA_ARGS__)
#define mxRangeError(...) fxThrowMessage(the, NULL, 0, XS_RANGE_ERROR, __VA_ARGS__)
#define mxReferenceError(...) fxThrowMessage(the, NULL, 0, XS_REFERENCE_ERROR, __VA_ARGS__)
#define mxSyntaxError(...) fxThrowMessage(the, NULL, 0, XS_SYNTAX_ERROR, __VA_ARGS__)
#define mxTypeError(...) fxThrowMessage(the, NULL, 0, XS_TYPE_ERROR, __VA_ARGS__)
#define mxURIError(...) fxThrowMessage(the, NULL, 0, XS_URI_ERROR, __VA_ARGS__)
#define mxDebugID(THE_ERROR, THE_FORMAT, THE_ID) ( \
	fxIDToString(the, THE_ID, the->nameBuffer, sizeof(the->nameBuffer)), \
	fxThrowMessage(the, NULL, 0, THE_ERROR, THE_FORMAT, the->nameBuffer) \
)
#else
#define mxUnknownError(...) fxThrowMessage(the, NULL, 0, XS_UNKNOWN_ERROR, __VA_ARGS__)
#define mxEvalError(...) fxThrowMessage(the, NULL, 0, XS_EVAL_ERROR, __VA_ARGS__)
#define mxRangeError(...) fxThrowMessage(the, NULL, 0, XS_RANGE_ERROR, __VA_ARGS__)
#define mxReferenceError(...) fxThrowMessage(the, NULL, 0, XS_REFERENCE_ERROR, __VA_ARGS__)
#define mxSyntaxError(...) fxThrowMessage(the, NULL, 0, XS_SYNTAX_ERROR, __VA_ARGS__)
#define mxTypeError(...) fxThrowMessage(the, NULL, 0, XS_TYPE_ERROR, __VA_ARGS__)
#define mxURIError(...) fxThrowMessage(the, NULL, 0, XS_URI_ERROR, __VA_ARGS__)
#define mxDebugID(THE_ERROR, THE_FORMAT, THE_ID) ( \
	fxIDToString(the, THE_ID, the->nameBuffer, sizeof(the->nameBuffer)), \
	fxThrowMessage(the, NULL, 0, THE_ERROR, THE_FORMAT, the->nameBuffer) \
)
#endif

#define mxIsUndefined(THE_SLOT) \
	((THE_SLOT)->kind == XS_UNDEFINED_KIND)
#define mxIsNull(THE_SLOT) \
	((THE_SLOT)->kind == XS_NULL_KIND)
#define mxIsReference(THE_SLOT) \
	((THE_SLOT)->kind == XS_REFERENCE_KIND)
#define mxIsFunction(THE_SLOT) \
	( (THE_SLOT) &&  ((THE_SLOT)->next) && ((THE_SLOT)->next->flag && XS_INTERNAL_FLAG) && (((THE_SLOT)->next->kind == XS_CALLBACK_KIND) || ((THE_SLOT)->next->kind == XS_CALLBACK_X_KIND) || ((THE_SLOT)->next->kind == XS_CODE_KIND) || ((THE_SLOT)->next->kind == XS_CODE_X_KIND)))
#define mxIsString(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag && XS_INTERNAL_FLAG) && (((THE_SLOT)->next->kind == XS_STRING_KIND) || ((THE_SLOT)->next->kind == XS_STRING_X_KIND)))
#define mxIsBoolean(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag && XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_BOOLEAN_KIND))
#define mxIsNumber(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag && XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_NUMBER_KIND))
#define mxIsDate(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag && XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_DATE_KIND))
#define mxIsRegExp(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag && XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_REGEXP_KIND))
#define mxIsSymbol(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag && XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_SYMBOL_KIND))
#define mxIsHost(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag && XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_HOST_KIND))
#define mxIsProxy(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag && XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_PROXY_KIND))
#define mxIsCallable(THE_SLOT) \
	( (THE_SLOT) &&  ((THE_SLOT)->next) && (((THE_SLOT)->next->kind == XS_CALLBACK_KIND) || ((THE_SLOT)->next->kind == XS_CALLBACK_X_KIND) || ((THE_SLOT)->next->kind == XS_CODE_KIND) || ((THE_SLOT)->next->kind == XS_CODE_X_KIND) || ((THE_SLOT)->next->kind == XS_PROXY_KIND)))
#define mxIsConstructor(THE_SLOT) \
	((THE_SLOT) && ((THE_SLOT)->flag & XS_CAN_CONSTRUCT_FLAG))

#define mxIsStringPrimitive(THE_SLOT) \
	(((THE_SLOT)->kind == XS_STRING_KIND) || ((THE_SLOT)->kind == XS_STRING_X_KIND))
	
#ifdef mxDebug

#define mxPush(THE_SLOT) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = (THE_SLOT).kind, \
	the->stack->value = (THE_SLOT).value)
#define mxPushSlot(THE_SLOT) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = (THE_SLOT)->kind, \
	the->stack->value = (THE_SLOT)->value)
	
#define mxPushBoolean(THE_BOOLEAN) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_BOOLEAN_KIND, \
	the->stack->value.boolean = (THE_BOOLEAN))
#define mxPushClosure(THE_SLOT) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_CLOSURE_KIND, \
	the->stack->value.closure = (THE_SLOT))
#define mxPushInteger(THE_NUMBER) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_INTEGER_KIND, \
	the->stack->value.integer = (THE_NUMBER))
#define mxPushList() \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_LIST_KIND, \
	the->stack->value.list.first = C_NULL, \
	the->stack->value.list.last = C_NULL)
#define mxPushNull() \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_NULL_KIND)
#define mxPushNumber(THE_NUMBER) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_NUMBER_KIND, \
	the->stack->value.number = (THE_NUMBER))
#define mxPushReference(THE_SLOT) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_REFERENCE_KIND, \
	the->stack->value.reference = (THE_SLOT))
#define mxPushString(THE_STRING) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_STRING_KIND, \
	the->stack->value.string = (THE_STRING))
#define mxPushStringC(THE_STRING) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_NULL_KIND, \
	fxCopyStringC(the, the->stack, THE_STRING))
#define mxPushStringX(THE_STRING) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_STRING_X_KIND, \
	the->stack->value.string = (THE_STRING))
#define mxPushUndefined() \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_UNDEFINED_KIND)
#define mxPushUninitialized() \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_UNINITIALIZED_KIND)
#define mxPushUnsigned(THE_NUMBER) \
	(fxOverflow(the, -1, C_NULL, 0), \
	(--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	(THE_NUMBER < 0x7FFFFFFF) ? \
		(the->stack->kind = XS_INTEGER_KIND, \
		the->stack->value.integer = (txInteger)(THE_NUMBER)) \
	: \
		(the->stack->kind = XS_NUMBER_KIND, \
		the->stack->value.number = (txNumber)(THE_NUMBER)) \
	)
#else

#define mxPush(THE_SLOT) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = (THE_SLOT).kind, \
	the->stack->value = (THE_SLOT).value)
#define mxPushSlot(THE_SLOT) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = (THE_SLOT)->kind, \
	the->stack->value = (THE_SLOT)->value)
	
#define mxPushBoolean(THE_BOOLEAN) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_BOOLEAN_KIND, \
	the->stack->value.boolean = (THE_BOOLEAN))
#define mxPushClosure(THE_SLOT) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_CLOSURE_KIND, \
	the->stack->value.closure = (THE_SLOT))
#define mxPushInteger(THE_NUMBER) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_INTEGER_KIND, \
	the->stack->value.integer = (THE_NUMBER))
#define mxPushList() \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_LIST_KIND, \
	the->stack->value.list.first = C_NULL, \
	the->stack->value.list.last = C_NULL)
#define mxPushNull() \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_NULL_KIND)
#define mxPushNumber(THE_NUMBER) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_NUMBER_KIND, \
	the->stack->value.number = (THE_NUMBER))
#define mxPushReference(THE_SLOT) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_REFERENCE_KIND, \
	the->stack->value.reference = (THE_SLOT))
#define mxPushString(THE_STRING) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_STRING_KIND, \
	the->stack->value.string = (THE_STRING))
#define mxPushStringC(THE_STRING) \
	(fxCopyStringC(the, --the->stack, THE_STRING))
#define mxPushStringX(THE_STRING) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_STRING_X_KIND, \
	the->stack->value.string = (THE_STRING))
#define mxPushUndefined() \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_UNDEFINED_KIND)
#define mxPushUninitialized() \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	the->stack->kind = XS_UNINITIALIZED_KIND)
#define mxPushUnsigned(THE_NUMBER) \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	(THE_NUMBER < 0x7FFFFFFF) ? \
		(the->stack->kind = XS_INTEGER_KIND, \
		the->stack->value.integer = (txInteger)(THE_NUMBER)) \
	: \
		(the->stack->kind = XS_NUMBER_KIND, \
		the->stack->value.number = (txNumber)(THE_NUMBER)) \
	)
	
#endif	

#define mxPull(THE_SLOT) \
	((THE_SLOT).value = the->stack->value, \
	(THE_SLOT).kind = (the->stack++)->kind)
#define mxPullSlot(THE_SLOT) \
	((THE_SLOT)->value = the->stack->value, \
	(THE_SLOT)->kind = (the->stack++)->kind)

#define mxPop() \
	(the->stack++)

#define mxArgv(THE_INDEX) (the->frame + 5 + ((the->frame + 5)->value.integer) - (THE_INDEX))
#define mxArgc ((the->frame + 5)->value.integer)
#define mxThis (the->frame + 4)
#define mxFunction (the->frame + 3)
#define mxTarget (the->frame + 2)
#define mxResult (the->frame + 1)
#define mxVarc ((the->frame - 1)->value.environment.variable.count)
#define mxVarv(THE_INDEX) (the->frame -2 - THE_INDEX)

#define mxFunctionInstanceCode(INSTANCE) 		((INSTANCE)->next)
#define mxFunctionInstanceHome(INSTANCE) 		((INSTANCE)->next->next)
#ifdef mxProfile
#define mxFunctionInstanceProfile(INSTANCE) 	((INSTANCE)->next->next->next)
#ifndef mxNoFunctionLength
#define mxFunctionInstanceLength(INSTANCE)		((INSTANCE)->next->next->next->next)
#endif
#else
#ifndef mxNoFunctionLength
#define mxFunctionInstanceLength(INSTANCE)		((INSTANCE)->next->next->next)
#endif
#endif

#define mxModuleInstanceInternal(MODULE)		((MODULE)->next)
#define mxModuleInstanceExports(MODULE)		((MODULE)->next->next)
#define mxModuleInstanceTransers(MODULE)	((MODULE)->next->next->next)
#define mxModuleInstanceFunction(MODULE)	((MODULE)->next->next->next->next)
#define mxModuleInstanceHosts(MODULE)		((MODULE)->next->next->next->next->next)
#define mxModuleInternal(MODULE) mxModuleInstanceInternal((MODULE)->value.reference)
#define mxModuleExports(MODULE) mxModuleInstanceExports((MODULE)->value.reference)
#define mxModuleTransfers(MODULE) mxModuleInstanceTransers((MODULE)->value.reference)
#define mxModuleFunction(MODULE) mxModuleInstanceFunction((MODULE)->value.reference)
#define mxModuleHosts(MODULE) mxModuleInstanceHosts((MODULE)->value.reference)

#define mxPromiseStatus(INSTANCE) ((INSTANCE)->next)
#define mxPromiseThens(INSTANCE) ((INSTANCE)->next->next)
#define mxPromiseResult(INSTANCE) ((INSTANCE)->next->next->next)

enum {
	mxUndefinedStatus,
	mxPendingStatus,
	mxFulfilledStatus,
	mxRejectedStatus
};

#define mxTransferLocal(TRANSFER)	(TRANSFER)->value.reference->next
#define mxTransferFrom(TRANSFER) 	(TRANSFER)->value.reference->next->next
#define mxTransferImport(TRANSFER) 	(TRANSFER)->value.reference->next->next->next
#define mxTransferAliases(TRANSFER)	(TRANSFER)->value.reference->next->next->next->next
#define mxTransferClosure(TRANSFER)	(TRANSFER)->value.reference->next->next->next->next->next

#define mxBehavior(INSTANCE) (gxBehaviors[((INSTANCE)->flag & XS_EXOTIC_FLAG) ? (INSTANCE)->next->ID : 0])
#define mxBehaviorCall(THE, INSTANCE, THIS, ARGUMENTS) \
	(*mxBehavior(INSTANCE)->call)(THE, INSTANCE, THIS, ARGUMENTS)
#define mxBehaviorConstruct(THE, INSTANCE, ARGUMENTS, TARGET) \
	(*mxBehavior(INSTANCE)->construct)(THE, INSTANCE, ARGUMENTS, TARGET)
#define mxBehaviorDefineOwnProperty(THE, INSTANCE, ID, INDEX, VALUE, MASK) \
	(*mxBehavior(INSTANCE)->defineOwnProperty)(THE, INSTANCE, ID, INDEX, VALUE, MASK)
#define mxBehaviorDeleteProperty(THE, INSTANCE, ID, INDEX) \
	(*mxBehavior(INSTANCE)->deleteProperty)(THE, INSTANCE, ID, INDEX)
#define mxBehaviorGetOwnProperty(THE, INSTANCE, ID, INDEX, VALUE) \
	(*mxBehavior(INSTANCE)->getOwnProperty)(THE, INSTANCE, ID, INDEX, VALUE)
#define mxBehaviorGetProperty(THE, INSTANCE, ID, INDEX, FLAG) \
	(*mxBehavior(INSTANCE)->getProperty)(THE, INSTANCE, ID, INDEX, FLAG)
#define mxBehaviorGetPropertyValue(THE, INSTANCE, ID, INDEX, RECEIVER, VALUE) \
	(*mxBehavior(INSTANCE)->getPropertyValue)(THE, INSTANCE, ID, INDEX, RECEIVER, VALUE)
#define mxBehaviorGetPrototype(THE, INSTANCE, PROTOTYPE) \
	(*mxBehavior(INSTANCE)->getPrototype)(THE, INSTANCE, PROTOTYPE)
#define mxBehaviorHasProperty(THE, INSTANCE, ID, INDEX) \
	(*mxBehavior(INSTANCE)->hasProperty)(THE, INSTANCE, ID, INDEX)
#define mxBehaviorIsExtensible(THE, INSTANCE) \
	(*mxBehavior(INSTANCE)->isExtensible)(THE, INSTANCE)
#define mxBehaviorOwnKeys(THE, INSTANCE, FLAG, KEYS) \
	(*mxBehavior(INSTANCE)->ownKeys)(THE, INSTANCE, FLAG, KEYS)
#define mxBehaviorPreventExtensions(THE, INSTANCE) \
	(*mxBehavior(INSTANCE)->preventExtensions)(THE, INSTANCE)
#define mxBehaviorSetProperty(THE, INSTANCE, ID, INDEX, FLAG) \
	(*mxBehavior(INSTANCE)->setProperty)(THE, INSTANCE, ID, INDEX, FLAG)
#define mxBehaviorSetPropertyValue(THE, INSTANCE, ID, INDEX, VALUE, RECEIVER) \
	(*mxBehavior(INSTANCE)->setPropertyValue)(THE, INSTANCE, ID, INDEX, VALUE, RECEIVER)
#define mxBehaviorSetPrototype(THE, INSTANCE, PROTOTYPE) \
	(*mxBehavior(INSTANCE)->setPrototype)(THE, INSTANCE, PROTOTYPE)

#define mxCall(_FUNCTION,_THIS,_COUNT) \
	mxPushInteger(_COUNT); \
	mxPushSlot(_THIS); \
	mxPushSlot(_FUNCTION); \
	fxCall(the)

#define mxCallID(_THIS,_ID,_COUNT) \
	mxPushInteger(_COUNT); \
	mxPushSlot(_THIS); \
	fxCallID(the, _ID)
	
#define mxGetID(_THIS,_ID) \
	mxPushSlot(_THIS); \
	fxGetID(the, _ID)


enum {
	mxGlobalStackIndex,
	mxExceptionStackIndex,
	mxHostsStackIndex,
	mxClosuresStackIndex,

	mxModulePathsStackIndex,
	mxImportingModulesStackIndex,
	mxLoadingModulesStackIndex,
	mxLoadedModulesStackIndex,
	mxResolvingModulesStackIndex,
	mxRunningModulesStackIndex,
	mxRequiredModulesStackIndex,
	mxModulesStackIndex,
	mxPendingJobsStackIndex,
	mxRunningJobsStackIndex,
	mxBreakpointsStackIndex,
	mxHostInspectorsStackIndex,
	mxInstanceInspectorsStackIndex,

	mxObjectPrototypeStackIndex,
	mxFunctionPrototypeStackIndex,
	mxArrayPrototypeStackIndex,
	mxStringPrototypeStackIndex,
	mxBooleanPrototypeStackIndex,
	mxNumberPrototypeStackIndex,
	mxDatePrototypeStackIndex,
	mxRegExpPrototypeStackIndex,
	mxHostPrototypeStackIndex,
	mxErrorPrototypeStackIndex,
	mxEvalErrorPrototypeStackIndex,
	mxRangeErrorPrototypeStackIndex,
	mxReferenceErrorPrototypeStackIndex,
	mxSyntaxErrorPrototypeStackIndex,
	mxTypeErrorPrototypeStackIndex,
	mxURIErrorPrototypeStackIndex,
	mxSymbolPrototypeStackIndex,
	mxArrayBufferPrototypeStackIndex,
	mxDataViewPrototypeStackIndex,
	mxTypedArrayPrototypeStackIndex,
	mxMapPrototypeStackIndex,
	mxSetPrototypeStackIndex,
	mxWeakMapPrototypeStackIndex,
	mxWeakSetPrototypeStackIndex,
	mxPromisePrototypeStackIndex,
	mxProxyPrototypeStackIndex,
	mxSharedArrayBufferPrototypeStackIndex,

	mxEnumeratorFunctionStackIndex,
	mxAsyncFunctionPrototypeStackIndex,
	mxGeneratorPrototypeStackIndex,
	mxGeneratorFunctionPrototypeStackIndex,
	mxModulePrototypeStackIndex,
	mxModuleConstructorStackIndex,
	mxTransferPrototypeStackIndex,
	mxTransferConstructorStackIndex,
	mxOnRejectedPromiseFunctionStackIndex,
	mxOnResolvedPromiseFunctionStackIndex,
	mxRejectPromiseFunctionStackIndex,
	mxResolvePromiseFunctionStackIndex,
	mxArrayLengthAccessorStackIndex,
	mxProxyAccessorStackIndex,
	mxStringAccessorStackIndex,
	mxTypedArrayAccessorStackIndex,
	mxUndefinedStackIndex,
	
	mxIteratorPrototypeStackIndex,
	mxArrayIteratorPrototypeStackIndex,
	mxMapEntriesIteratorPrototypeStackIndex,
	mxMapKeysIteratorPrototypeStackIndex,
	mxMapValuesIteratorPrototypeStackIndex,
	mxSetEntriesIteratorPrototypeStackIndex,
	mxSetKeysIteratorPrototypeStackIndex,
	mxSetValuesIteratorPrototypeStackIndex,
	mxStringIteratorPrototypeStackIndex,
	
	mxArgumentsSloppyPrototypeStackIndex,
	mxArgumentsStrictPrototypeStackIndex,
	mxThrowTypeErrorFunctionStackIndex,

	mxHookInstanceIndex,
	
	mxInitializeRegExpFunctionIndex,
	mxArrayIteratorFunctionIndex,
	mxArrayConstructorIndex,
	mxPromiseConstructorIndex,

	mxEmptyCodeStackIndex,
	mxEmptyStringStackIndex,
	mxEmptyRegExpStackIndex,
	mxBooleanStringStackIndex,
	mxDefaultStringStackIndex,
	mxFunctionStringStackIndex,
	mxNumberStringStackIndex,
	mxObjectStringStackIndex,
	mxStringStringStackIndex,
	mxSymbolStringStackIndex,
	mxUndefinedStringStackIndex,

	mxStackIndexCount
};

#define mxGlobal the->stackTop[-1 - mxGlobalStackIndex]
#define mxException the->stackTop[-1 - mxExceptionStackIndex]
#define mxHosts the->stackTop[-1 - mxHostsStackIndex]
#define mxClosures the->stackTop[-1 - mxClosuresStackIndex]
#define mxModulePaths the->stackTop[-1 - mxModulePathsStackIndex]
#define mxImportingModules the->stackTop[-1 - mxImportingModulesStackIndex]
#define mxLoadingModules the->stackTop[-1 - mxLoadingModulesStackIndex]
#define mxLoadedModules the->stackTop[-1 - mxLoadedModulesStackIndex]
#define mxResolvingModules the->stackTop[-1 - mxResolvingModulesStackIndex]
#define mxRunningModules the->stackTop[-1 - mxRunningModulesStackIndex]
#define mxRequiredModules the->stackTop[-1 - mxRequiredModulesStackIndex]
#define mxModules the->stackTop[-1 - mxModulesStackIndex]
#define mxPendingJobs the->stackTop[-1 - mxPendingJobsStackIndex]
#define mxRunningJobs the->stackTop[-1 - mxRunningJobsStackIndex]
#define mxBreakpoints the->stackTop[-1 - mxBreakpointsStackIndex]
#define mxHostInspectors the->stackTop[-1 - mxHostInspectorsStackIndex]
#define mxInstanceInspectors the->stackTop[-1 - mxInstanceInspectorsStackIndex]

#define mxObjectPrototype the->stackPrototypes[-1 - mxObjectPrototypeStackIndex]
#define mxFunctionPrototype the->stackPrototypes[-1 - mxFunctionPrototypeStackIndex]
#define mxArrayPrototype the->stackPrototypes[-1 - mxArrayPrototypeStackIndex]
#define mxStringPrototype the->stackPrototypes[-1 - mxStringPrototypeStackIndex]
#define mxBooleanPrototype the->stackPrototypes[-1 - mxBooleanPrototypeStackIndex]
#define mxNumberPrototype the->stackPrototypes[-1 - mxNumberPrototypeStackIndex]
#define mxDatePrototype the->stackPrototypes[-1 - mxDatePrototypeStackIndex]
#define mxRegExpPrototype the->stackPrototypes[-1 - mxRegExpPrototypeStackIndex]
#define mxHostPrototype the->stackPrototypes[-1 - mxHostPrototypeStackIndex]
#define mxErrorPrototype the->stackPrototypes[-1 - mxErrorPrototypeStackIndex]
#define mxEvalErrorPrototype the->stackPrototypes[-1 - mxEvalErrorPrototypeStackIndex]
#define mxRangeErrorPrototype the->stackPrototypes[-1 - mxRangeErrorPrototypeStackIndex]
#define mxReferenceErrorPrototype the->stackPrototypes[-1 - mxReferenceErrorPrototypeStackIndex]
#define mxSyntaxErrorPrototype the->stackPrototypes[-1 - mxSyntaxErrorPrototypeStackIndex]
#define mxTypeErrorPrototype the->stackPrototypes[-1 - mxTypeErrorPrototypeStackIndex]
#define mxURIErrorPrototype the->stackPrototypes[-1 - mxURIErrorPrototypeStackIndex]
#define mxSymbolPrototype the->stackPrototypes[-1 - mxSymbolPrototypeStackIndex]
#define mxArrayBufferPrototype the->stackPrototypes[-1 - mxArrayBufferPrototypeStackIndex]
#define mxDataViewPrototype the->stackPrototypes[-1 - mxDataViewPrototypeStackIndex]
#define mxTypedArrayPrototype the->stackPrototypes[-1 - mxTypedArrayPrototypeStackIndex]
#define mxMapPrototype the->stackPrototypes[-1 - mxMapPrototypeStackIndex]
#define mxSetPrototype the->stackPrototypes[-1 - mxSetPrototypeStackIndex]
#define mxWeakMapPrototype the->stackPrototypes[-1 - mxWeakMapPrototypeStackIndex]
#define mxWeakSetPrototype the->stackPrototypes[-1 - mxWeakSetPrototypeStackIndex]
#define mxPromisePrototype the->stackPrototypes[-1 - mxPromisePrototypeStackIndex]
#define mxProxyPrototype the->stackPrototypes[-1 - mxProxyPrototypeStackIndex]
#define mxSharedArrayBufferPrototype the->stackPrototypes[-1 - mxSharedArrayBufferPrototypeStackIndex]

#define mxEmptyCode the->stackPrototypes[-1 - mxEmptyCodeStackIndex]
#define mxEmptyString the->stackPrototypes[-1 - mxEmptyStringStackIndex]
#define mxEmptyRegExp the->stackPrototypes[-1 - mxEmptyRegExpStackIndex]
#define mxBooleanString the->stackPrototypes[-1 - mxBooleanStringStackIndex]
#define mxDefaultString the->stackPrototypes[-1 - mxDefaultStringStackIndex]
#define mxFunctionString the->stackPrototypes[-1 - mxFunctionStringStackIndex]
#define mxNumberString the->stackPrototypes[-1 - mxNumberStringStackIndex]
#define mxObjectString the->stackPrototypes[-1 - mxObjectStringStackIndex]
#define mxStringString the->stackPrototypes[-1 - mxStringStringStackIndex]
#define mxSymbolString the->stackPrototypes[-1 - mxSymbolStringStackIndex]
#define mxUndefinedString the->stackPrototypes[-1 - mxUndefinedStringStackIndex]

#define mxEnumeratorFunction the->stackPrototypes[-1 - mxEnumeratorFunctionStackIndex]
#define mxAsyncFunctionPrototype the->stackPrototypes[-1 - mxAsyncFunctionPrototypeStackIndex]
#define mxGeneratorPrototype the->stackPrototypes[-1 - mxGeneratorPrototypeStackIndex]
#define mxGeneratorFunctionPrototype the->stackPrototypes[-1 - mxGeneratorFunctionPrototypeStackIndex]
#define mxModulePrototype the->stackPrototypes[-1 - mxModulePrototypeStackIndex]
#define mxModuleConstructor the->stackPrototypes[-1 - mxModuleConstructorStackIndex]
#define mxTransferPrototype the->stackPrototypes[-1 - mxTransferPrototypeStackIndex]
#define mxTransferConstructor the->stackPrototypes[-1 - mxTransferConstructorStackIndex]
#define mxOnRejectedPromiseFunction the->stackPrototypes[-1 - mxOnRejectedPromiseFunctionStackIndex]
#define mxOnResolvedPromiseFunction the->stackPrototypes[-1 - mxOnResolvedPromiseFunctionStackIndex]
#define mxRejectPromiseFunction the->stackPrototypes[-1 - mxRejectPromiseFunctionStackIndex]
#define mxResolvePromiseFunction the->stackPrototypes[-1 - mxResolvePromiseFunctionStackIndex]
#define mxArrayLengthAccessor the->stackPrototypes[-1 - mxArrayLengthAccessorStackIndex]
#define mxProxyAccessor the->stackPrototypes[-1 - mxProxyAccessorStackIndex]
#define mxStringAccessor the->stackPrototypes[-1 - mxStringAccessorStackIndex]
#define mxTypedArrayAccessor the->stackPrototypes[-1 - mxTypedArrayAccessorStackIndex]
#define mxUndefined the->stackPrototypes[-1 - mxUndefinedStackIndex]

#define mxIteratorPrototype the->stackPrototypes[-1 - mxIteratorPrototypeStackIndex]
#define mxArrayIteratorPrototype the->stackPrototypes[-1 - mxArrayIteratorPrototypeStackIndex]
#define mxMapEntriesIteratorPrototype the->stackPrototypes[-1 - mxMapEntriesIteratorPrototypeStackIndex]
#define mxMapKeysIteratorPrototype the->stackPrototypes[-1 - mxMapKeysIteratorPrototypeStackIndex]
#define mxMapValuesIteratorPrototype the->stackPrototypes[-1 - mxMapValuesIteratorPrototypeStackIndex]
#define mxSetEntriesIteratorPrototype the->stackPrototypes[-1 - mxSetEntriesIteratorPrototypeStackIndex]
#define mxSetKeysIteratorPrototype the->stackPrototypes[-1 - mxSetKeysIteratorPrototypeStackIndex]
#define mxSetValuesIteratorPrototype the->stackPrototypes[-1 - mxSetValuesIteratorPrototypeStackIndex]
#define mxStringIteratorPrototype the->stackPrototypes[-1 - mxStringIteratorPrototypeStackIndex]

#define mxArgumentsSloppyPrototype the->stackPrototypes[-1 - mxArgumentsSloppyPrototypeStackIndex]
#define mxArgumentsStrictPrototype the->stackPrototypes[-1 - mxArgumentsStrictPrototypeStackIndex]
#define mxThrowTypeErrorFunction the->stackPrototypes[-1 - mxThrowTypeErrorFunctionStackIndex]

#define mxHookInstance the->stackPrototypes[-1 - mxHookInstanceIndex]
#define  mxInitializeRegExpFunction the->stackPrototypes[-1 - mxInitializeRegExpFunctionIndex]
#define  mxArrayIteratorFunction the->stackPrototypes[-1 - mxArrayIteratorFunctionIndex]
#define  mxArrayConstructor the->stackPrototypes[-1 - mxArrayConstructorIndex]
#define  mxPromiseConstructor the->stackPrototypes[-1 - mxPromiseConstructorIndex]

#define mxErrorPrototypes(THE_ERROR) (the->stackPrototypes[-mxErrorPrototypeStackIndex-(THE_ERROR)])

#define mxID(ID) ((ID) - 32768)

#ifdef mxLink
extern txCallback fxNewLinkerCallback(txMachine*, txCallback, txString);
#define mxCallback(CALLBACK) fxNewLinkerCallback(the, CALLBACK, #CALLBACK)
#else
#define mxCallback(CALLBACK) CALLBACK
#endif

#ifdef __cplusplus
}
#endif

#endif /* __XSALL__ */

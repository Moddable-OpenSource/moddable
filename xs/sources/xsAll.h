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

#ifndef __XSALL__
#define __XSALL__

#include "xsCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define mxFrequency 1
#ifndef mxBoundsCheck
	#ifdef mxDebug
		#define mxBoundsCheck 1
	#else
		#define mxBoundsCheck 0
	#endif
#endif
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
typedef void (*txTypeCoerce)(txMachine*, txSlot*);
typedef int (*txTypeCompare)(const void*, const void*);
typedef void (*txTypeOperator)(txMachine*, txSlot*, txInteger, txSlot*, int, int);
typedef txInteger (*txTypeWait)(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, txNumber timeout);

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
	txSlot* (*newGeneratorInstance)(txMachine*);
	txSlot* (*newGeneratorFunctionInstance)(txMachine*, txID name);
	txSlot* (*newAsyncGeneratorInstance)(txMachine*);
	txSlot* (*newAsyncGeneratorFunctionInstance)(txMachine*, txID name);
	void (*runForAwaitOf)(txMachine*);
	txSlot* (*newArgumentsSloppyInstance)(txMachine*, txIndex count);
	txSlot* (*newArgumentsStrictInstance)(txMachine*, txIndex count);
	void (*runEval)(txMachine*);
	void (*runEvalEnvironment)(txMachine*);
	void (*runProgramEnvironment)(txMachine*);
	void (*initializeSharedCluster)();
	void (*terminateSharedCluster)();
	txSlot* (*newFunctionLength)(txMachine* the, txSlot* instance, txNumber length);
	txSlot* (*newFunctionName)(txMachine* the, txSlot* instance, txID id, txIndex index, txID former, txString prefix);
    void (*runImport)(txMachine* the, txSlot* realm, txID id);
	txBoolean (*definePrivateProperty)(txMachine* the, txSlot* instance, txSlot* check, txID id, txSlot* slot, txFlag mask);
	txSlot* (*getPrivateProperty)(txMachine* the, txSlot* instance, txSlot* check, txID id);
	txSlot* (*setPrivateProperty)(txMachine* the, txSlot* instance, txSlot* check, txID id);
	void (*cleanupFinalizationRegistries)(txMachine*);
	void (*captureErrorStack)(txMachine* the, txSlot* internal, txSlot* frame);
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
#define mxSortPartitionCount 30

#define mxTypeArrayCount 11

typedef struct {
	txU2 size;
	txU2 shift;		// (1 << shift) == size
	txTypeCallback getter;
	txTypeCallback setter;
	txTypeCoerce coerce;
	txTypeCompare compare;
	txID getID;
	txID setID;
	txID constructorID;
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
	txTypeWait wait;
} txTypeAtomics;

typedef txBigInt* (*txBigIntBinary)(txMachine*, txBigInt* r, txBigInt* a, txBigInt* b);
typedef txBoolean (*txBigIntCompare)(txMachine*, txBoolean less, txBoolean equal, txBoolean more, txSlot*, txSlot*);
typedef void (*txBigIntDecode)(txMachine*, txSize);
typedef void (*txBigIntToArrayBuffer)(txMachine* the, txSlot* slot, txU4 minBytes, txBoolean sign, int endian);
typedef txSlot* (*txBigIntToInstance)(txMachine* the, txSlot* slot);
typedef void (*txBigintToString)(txMachine* the, txSlot* slot, txU4 radix);
typedef txNumber (*txBigIntToNumber)(txMachine* the, txSlot* slot);
typedef txBigInt* (*txBigIntUnary)(txMachine*, txBigInt* r, txBigInt* a);

typedef struct {
	txBigIntCompare compare;
	txBigIntDecode decode;
	txBigIntToArrayBuffer toArrayBuffer;
	txBigIntToInstance toInstance;
	txBigIntToNumber toNumber;
	txBigintToString toString;
	txBigIntBinary _add;
	txBigIntBinary _and;
	txBigIntUnary _dec;
	txBigIntBinary _div;
	txBigIntBinary _exp;
	txBigIntUnary _inc;
	txBigIntBinary _lsl;
	txBigIntBinary _lsr;
	txBigIntBinary _mul;
	txBigIntUnary _neg;
	txBigIntBinary _nop;
	txBigIntUnary _not;
	txBigIntBinary _or;
	txBigIntBinary _rem;
	txBigIntBinary _sub;
	txBigIntBinary _xor;
} txTypeBigInt;


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
	txBigInt bigint;

	txSlot* reference;

	txSlot* closure;
	
	struct { union { txSlot* reference; txInteger count; } variable; txInteger line; } environment; // id == file
	struct { txByte* code; txSlot* scope; } frame;

	struct { txSlot* garbage; txSlot* prototype; } instance;
	
	struct { txSlot* address; txIndex length; } array;
	struct { txByte* address; void* detachKey; } arrayBuffer;
	struct { txInteger length; txInteger maxLength; } bufferInfo;
	struct { txCallback address; txSlot* closures; } callback;
	struct { txByte* address; txSlot* closures; } code;
	struct { txInteger offset; txInteger size; } dataView;
	struct { txSlot* info; txError which; } error;
	struct { void* data; union { txDestructor destructor; txHostHooks* hooks; } variant; } host;
	struct { txSlot* handler; txSlot* target; } proxy;
	struct { txInteger* code; txInteger* data; } regexp;
	struct { txSlot* address; txIndex length; } stack;
	struct { txSlot** address; txSize length; } table;
	struct { txTypeDispatch* dispatch; txTypeAtomics* atomics; } typedArray;
	struct { txSlot* check; txSlot* value; } weakEntry;
	struct { txSlot* first; txSlot* link; } weakList;
	struct { txSlot* target; txSlot* link; } weakRef;
	struct { txSlot* callback; txUnsigned flags; } finalizationRegistry;
	struct { txSlot* target; txSlot* token; } finalizationCell;
	
	struct { txSlot* getter; txSlot* setter; } accessor;
	struct { txU4 index; txID id; } at;
	struct { txSlot* slot; txU4 sum; } entry;
	struct { txSlot* first; txIndex length; } errors;
	struct { txSlot* object; txSlot* module; } home;
	struct { txString string; txU4 sum; } key;
	struct { txSlot* first; txSlot* last; } list;
	struct { txSlot* realm; txID id; } module;
#ifdef mxHostFunctionPrimitive
	struct { const txHostFunctionBuilder* builder; txID profileID; } hostFunction;
#endif
	struct { txSlot* cache; txSlot* instance; } hostInspector;
	struct { txSlot* slot; txInspectorNameLink* link; } instanceInspector;
	struct { txSlot* closure; txSlot* module; } export;
	struct { txSlot* check; txSlot* first; } private;
	
	txID* IDs;
} txValue;

struct sxBlock {
	txBlock* nextBlock;
	txByte* current;
	txByte* limit;
	txByte* temporary;
};

struct sxChunk {
	txSize size;
#if INTPTR_MAX == INT64_MAX
	txS4 dummy;
#endif
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
#if mx32bitID
	#if mxBigEndian
		union {
			struct {
				txID ID;
				txS2 dummy;
				txFlag flag;
				txKind kind;
			};
			txS8 ID_FLAG_KIND;
		};
	#else
		union {
			struct {
				txKind kind;
				txFlag flag;
				txS2 dummy;
				txID ID;
			};
			txS8 KIND_FLAG_ID;
		};
	#endif
#else
	#if mxBigEndian
		union {
			struct {
				txID ID;
				txFlag flag;
				txKind kind;
			};
			txS4 ID_FLAG_KIND;
		};
	#else
		union {
			struct {
				txKind kind;
				txFlag flag;
				txID ID;
			};
			txS4 KIND_FLAG_ID;
		};
	#endif
	#if INTPTR_MAX == INT64_MAX
		txS4 dummy;
	#endif
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

	txID* colors;
	txSlot** keyArray;
	txID keyCount;
	txID keyDelta;
	txID keyIndex;
	txID keyOffset;
	txSlot** keyArrayHost;

	txID aliasCount;
	txID aliasIndex;
	txSlot** aliasArray;
	
	char* stackLimit;
	
	txSlot* firstWeakListLink;
	txSlot* firstWeakRefLink;
	
	txSize currentChunksSize;
	txSize peakChunksSize;
	txSize maximumChunksSize;
	txSize minimumChunksSize;

	txSize currentHeapCount;
	txSize peakHeapCount;
	txSize maximumHeapCount;
	txSize minimumHeapCount;
	
	txSize parserBufferSize;
	txSize parserTableModulo;

	txBoolean shared;
	txMachine* sharedMachine;

	txBoolean collectFlag;
	void* dtoa;
	void* preparation;

	txInteger tag;
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
	txSlot* debugModule;
	size_t idValue;
	txInteger lineValue;
	char pathValue[256];
	txSize debugOffset;
	char debugBuffer[256];
	txSize echoOffset;
	char echoBuffer[256];
#endif
#ifdef mxFrequency
	txNumber exits[XS_CODE_COUNT];
	txNumber frequencies[XS_CODE_COUNT];
#endif
#ifdef mxInstrument
	txU2 garbageCollectionCount;
	txU2 loadedModulesCount;
	txSize peakParserSize;
	txSlot* stackPeak;
	txSize floatingPointOps;
	void (*onBreak)(txMachine*, txU1 stop);
#endif
#ifdef mxMetering
	txBoolean (*meterCallback)(txMachine*, txU4);
	txU4 meterCount;
	txU4 meterIndex;
	txU4 meterInterval;
#endif
	txID profileID;
#if defined(mxInstrument) || defined(mxProfile)
	void* profiler;
#endif
};

struct sxCreation {
	txSize initialChunkSize; /* xs.h */
	txSize incrementalChunkSize; /* xs.h */
	txSize initialHeapCount; /* xs.h */
	txSize incrementalHeapCount; /* xs.h */
	txSize stackCount; /* xs.h */
	txSize initialKeyCount; /* xs.h */
	txSize incrementalKeyCount; /* xs.h */
	txSize nameModulo; /* xs.h */
	txSize symbolModulo; /* xs.h */
	txSize parserBufferSize; /* xs.h */
	txSize parserTableModulo; /* xs.h */
	txSize staticSize; /* xs.h */
};

struct sxPreparation {
	txS1 version[4];
	
	txSize aliasCount;
	txSize heapCount;
	txSlot* heap;
	txSize stackCount;
	txSlot* stack;

	txID* colors;
	txSize keyCount;
	txSlot** keys;
	txSize nameModulo;
	txSlot** names;
	txSize symbolModulo;
	txSlot** symbols;

	txSize scriptCount;
	txScript* scripts;
	
	txID profileID;
	
	txCreation creation;
	txString main;
	
	txU1 checksum[16];
};

struct sxHostFunctionBuilder {
	txCallback callback;
	txInteger length;
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
mxExport txSlot* fxNewHostFunction(txMachine*, txCallback, txInteger, txInteger, txInteger);
mxExport txSlot* fxNewHostInstance(txMachine* the);
mxExport txSlot* fxNewHostObject(txMachine*, txDestructor);
mxExport txInteger fxGetHostBufferLength(txMachine* the, txSlot* slot);
mxExport void* fxGetHostChunk(txMachine*, txSlot*);
mxExport void* fxGetHostChunkIf(txMachine*, txSlot*);
mxExport void* fxGetHostData(txMachine*, txSlot*);
mxExport void* fxGetHostDataIf(txMachine*, txSlot*);
mxExport void* fxGetHostHandle(txMachine*, txSlot*);
mxExport txDestructor fxGetHostDestructor(txMachine*, txSlot*);
mxExport txHostHooks* fxGetHostHooks(txMachine*, txSlot*);
mxExport void fxPetrifyHostBuffer(txMachine* the, txSlot* slot);
mxExport void fxSetHostBuffer(txMachine* the, txSlot* slot, void* theData, txSize theSize);
mxExport void *fxSetHostChunk(txMachine* the, txSlot* slot, void* theValue, txSize theSize);
mxExport void fxSetHostData(txMachine*, txSlot*, void*);
mxExport void fxSetHostDestructor(txMachine*, txSlot*, txDestructor);
mxExport void fxSetHostHooks(txMachine*, txSlot*, const txHostHooks*);
mxExport void* fxToHostHandle(txMachine* the, txSlot* slot);

mxExport txID fxID(txMachine*, txString);
mxExport txID fxFindID(txMachine* the, txString theName);
mxExport txS1 fxIsID(txMachine*, txString);
mxExport txID fxToID(txMachine* the, txSlot* theSlot);
mxExport txString fxName(txMachine*, txID);

mxExport void fxEnumerate(txMachine* the);
mxExport txBoolean fxHasAll(txMachine* the, txID id, txIndex index);
mxExport txBoolean fxHasAt(txMachine* the);
mxExport txBoolean fxHasID(txMachine*, txID);
mxExport txBoolean fxHasIndex(txMachine* the, txIndex index);
mxExport void fxGetAll(txMachine* the, txID id, txIndex index);
mxExport void fxGetAt(txMachine*);
mxExport void fxGetID(txMachine*, txID);
mxExport void fxGetIndex(txMachine*, txIndex);
mxExport void fxSetAll(txMachine* the, txID id, txIndex index);
mxExport void fxSetAt(txMachine*);
mxExport void fxSetID(txMachine*, txID);
mxExport void fxSetIndex(txMachine*, txIndex);
mxExport void fxDefineAll(txMachine* the, txID id, txIndex index, txFlag flag, txFlag mask);
mxExport void fxDefineAt(txMachine* the, txFlag flag, txFlag mask);
mxExport void fxDefineID(txMachine* the, txID id, txFlag flag, txFlag mask);
mxExport void fxDefineIndex(txMachine* the, txIndex index, txFlag flag, txFlag mask);
mxExport void fxDeleteAll(txMachine*, txID, txIndex);
mxExport void fxDeleteAt(txMachine*);
mxExport void fxDeleteID(txMachine*, txID);
mxExport void fxDeleteIndex(txMachine*, txIndex);
mxExport void fxCall(txMachine*);
mxExport void fxCallID(txMachine*, txID);
mxExport void fxNew(txMachine*);
mxExport void fxNewID(txMachine*, txID);
mxExport void fxRunCount(txMachine*, txInteger);
mxExport txBoolean fxRunTest(txMachine* the);

mxExport void fxCallFrame(txMachine*);
mxExport void fxNewFrame(txMachine* the);
mxExport void fxVars(txMachine*, txInteger);

mxExport txInteger fxCheckArg(txMachine*, txInteger);
mxExport txInteger fxCheckVar(txMachine*, txInteger);
mxExport void fxOverflow(txMachine*, txInteger, txString thePath, txInteger theLine);
mxExport void fxThrow(txMachine* the, txString thePath, txInteger theLine) XS_FUNCTION_NORETURN;
mxExport void fxThrowMessage(txMachine* the, txString thePath, txInteger theLine, txError theError, txString theMessage, ...) XS_FUNCTION_NORETURN;
mxExport void fxDebugger(txMachine* the, txString thePath, txInteger theLine);

mxExport const txByte gxNoCode[] ICACHE_FLASH_ATTR;
mxExport txMachine* fxCreateMachine(txCreation* theCreation, txString theName, void* theContext, txID profileID);
mxExport void fxDeleteMachine(txMachine*);
mxExport txMachine* fxCloneMachine(txCreation* theCreation, txMachine* theMachine, txString theName, void* theContext);
mxExport txMachine* fxPrepareMachine(txCreation* creation, txPreparation* preparation, txString name, void* context, void* archive);
mxExport void fxShareMachine(txMachine* the);

mxExport txMachine* fxBeginHost(txMachine*);
mxExport void fxEndHost(txMachine*);
mxExport void fxEndJob(txMachine* the);
mxExport void fxExitToHost(txMachine*) XS_FUNCTION_NORETURN;

mxExport void fxCollectGarbage(txMachine*);
mxExport void fxEnableGarbageCollection(txMachine* the, txBoolean enableIt);
mxExport void fxRemember(txMachine*, txSlot*);
mxExport void fxForget(txMachine*, txSlot*);
mxExport void fxAccess(txMachine*, txSlot*);

mxExport void fxDemarshall(txMachine* the, void* theData, txBoolean alien);
mxExport void* fxMarshall(txMachine* the, txBoolean alien);

mxExport void fxBuildArchiveKeys(txMachine* the);
mxExport void* fxGetArchiveCode(txMachine* the, void* archive, txString path, size_t* size);
mxExport txInteger fxGetArchiveCodeCount(txMachine* the, void* archive);
mxExport void* fxGetArchiveCodeName(txMachine* the, void* archive, txInteger index);
mxExport void* fxGetArchiveData(txMachine* the, void* archive, txString path, size_t* size);
mxExport txInteger fxGetArchiveDataCount(txMachine* the, void* archive);
mxExport void* fxGetArchiveDataName(txMachine* the, void* archive, txInteger index);
mxExport void* fxGetArchiveName(txMachine* the, void* archive);
mxExport void* fxMapArchive(txMachine* the, txPreparation* preparation, void* archive, size_t bufferSize, txArchiveRead read, txArchiveWrite write);

mxExport txBoolean fxIsProfiling(txMachine* the);
mxExport void fxStartProfiling(txMachine* the);
mxExport void fxStopProfiling(txMachine* the, void* stream);

mxExport void fxAwaitImport(txMachine*, txBoolean defaultFlag);

#ifdef mxMetering
mxExport void fxBeginMetering(txMachine* the, txBoolean (*callback)(txMachine*, txU4), txU4 interval);
mxExport void fxCheckMetering(txMachine* the);
mxExport void fxEndMetering(txMachine* the);
#endif

/* xsmc.c */
mxExport void _xsNewArray(txMachine *the, txSlot *res, txInteger length);
mxExport void _xsNewObject(txMachine *the, txSlot *res);
mxExport void _xsNewHostInstance(txMachine*, txSlot*, txSlot*);
mxExport txBoolean _xsIsInstanceOf(txMachine*, txSlot*, txSlot*);
mxExport txBoolean _xsHas(txMachine*, txSlot*, txID);
mxExport txBoolean _xsHasIndex(txMachine*, txSlot*, txIndex);
mxExport void _xsGet(txMachine*, txSlot*, txSlot*, txID);
mxExport void _xsGetAt(txMachine*, txSlot*, txSlot*, txSlot*);
mxExport void _xsGetIndex(txMachine*, txSlot*, txSlot*, txIndex);
mxExport void _xsSet(txMachine*, txSlot*, txID, txSlot*);
mxExport void _xsSetAt(txMachine*, txSlot*, txSlot*, txSlot*);
mxExport void _xsSetIndex(txMachine*, txSlot*, txIndex, txSlot*);
mxExport void _xsDelete(txMachine*, txSlot*, txID);
mxExport void _xsDeleteAt(txMachine*, txSlot*, txSlot*);
mxExport void _xsCall(txMachine*, txSlot*, txSlot*, txUnsigned, ...);
mxExport void _xsNew(txMachine*, txSlot*, txSlot*, txUnsigned, ...);
mxExport txBoolean _xsTest(txMachine*, txSlot*);
mxExport txInteger fxIncrementalVars(txMachine*, txInteger);

/* xsPlatforms.c */
extern void* fxAllocateChunks(txMachine* the, txSize theSize);
extern txSlot* fxAllocateSlots(txMachine* the, txSize theCount);
extern void fxBuildKeys(txMachine* the);
extern void fxCreateMachinePlatform(txMachine* the);
extern void fxDeleteMachinePlatform(txMachine* the);
extern void fxFreeChunks(txMachine* the, void* theChunks);
extern void fxFreeSlots(txMachine* the, void* theSlots);
extern txID fxFindModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* slot);
extern void fxLoadModule(txMachine* the, txSlot* module, txID moduleID);
extern txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags);
extern void fxQueuePromiseJobs(txMachine* the);
extern void fxInitializeSharedCluster();
extern void fxTerminateSharedCluster();
extern void* fxCreateSharedChunk(txInteger byteLength);
extern void fxLockSharedChunk(void* data);
extern void fxLinkSharedChunk(txMachine* the);
extern txInteger fxMeasureSharedChunk(void* data);
extern txInteger fxNotifySharedChunk(txMachine* the, void* data, txInteger offset, txInteger count);
extern void fxReleaseSharedChunk(void* data);
extern void* fxRetainSharedChunk(void* data);
extern void fxUnlinkSharedChunk(txMachine* the);
extern void fxUnlockSharedChunk(void* data);
extern txInteger fxWaitSharedChunk(txMachine* the, void* address, txNumber timeout);
extern void fxAbort(txMachine* the, int status);
#ifdef mxDebug
extern void fxConnect(txMachine* the);
extern void fxDisconnect(txMachine* the);
extern txBoolean fxIsConnected(txMachine* the);
extern txBoolean fxIsReadable(txMachine* the);
extern void fxReceive(txMachine* the);
extern void fxSend(txMachine* the, txBoolean more);
#endif

/* xsDefaults.c */
extern const txDefaults gxDefaults;
extern const txBehavior* gxBehaviors[];
extern const txTypeBigInt gxTypeBigInt;

/* xsAll.c */
extern txString fxAdornStringC(txMachine* the, txString prefix, txSlot* string, txString suffix);
extern txSlot* fxArgToCallback(txMachine* the, txInteger argi);
extern void fxBufferFrameName(txMachine* the, txString buffer, txSize size, txSlot* frame, txString suffix);
extern void fxBufferFunctionName(txMachine* the, txString buffer, txSize size, txSlot* function, txString suffix);
extern void fxBufferObjectName(txMachine* the, txString buffer, txSize size, txSlot* object, txString suffix);
extern txString fxConcatString(txMachine* the, txSlot* a, txSlot* b);
extern txString fxConcatStringC(txMachine* the, txSlot* a, txString b);
extern txString fxCopyString(txMachine* the, txSlot* a, txSlot* b);
extern txString fxCopyStringC(txMachine* the, txSlot* a, txString b);
extern txBoolean fxIsCanonicalIndex(txMachine* the, txID id);

extern int fxStringGetter(void*);
extern int fxStringCGetter(void*);
extern void fxJump(txMachine*) XS_FUNCTION_NORETURN;

/* xsRun.c */
extern void fxRunEval(txMachine* the);
extern void fxRunForAwaitOf(txMachine* the);
extern void fxRunID(txMachine* the, txSlot* generator, txInteger count);
extern void fxRunScript(txMachine* the, txScript* script, txSlot* _this, txSlot* _target, txSlot* environment, txSlot* object, txSlot* module);
extern txBoolean fxIsSameReference(txMachine* the, txSlot* a, txSlot* b);
extern txBoolean fxIsSameSlot(txMachine* the, txSlot* a, txSlot* b);
extern txBoolean fxIsSameValue(txMachine* the, txSlot* a, txSlot* b, txBoolean zero);

/* xsMemory.c */
extern txSize fxAddChunkSizes(txMachine* the, txSize a, txSize b);
extern void fxCheckCStack(txMachine* the);
extern void fxAllocate(txMachine* the, txCreation* theCreation);
extern void fxCollect(txMachine* the, txFlag theFlag);
mxExport txSlot* fxDuplicateSlot(txMachine* the, txSlot* theSlot);
extern void fxFree(txMachine* the);
extern void fxGrowKeys(txMachine* the, txID theCount);
extern txSize fxMultiplyChunkSizes(txMachine* the, txSize a, txSize b);
mxExport void* fxNewChunk(txMachine* the, txSize theSize);
mxExport void* fxNewGrowableChunk(txMachine* the, txSize size, txSize overflow);
extern txSlot* fxNewSlot(txMachine* the);
mxExport void* fxRenewChunk(txMachine* the, void* theData, txSize theSize);
extern void fxShare(txMachine* the);

/* xsDebug.c */
#ifdef mxDebug
mxExport void fxCheck(txMachine* the, txString thePath, txInteger theLine);
extern void fxDebugCommand(txMachine* the);
extern void fxDebugLine(txMachine* the, txID id, txInteger line);
extern void fxDebugLoop(txMachine* the, txString thePath, txInteger theLine, txString message);
extern void fxDebugThrow(txMachine* the, txString thePath, txInteger theLine, txString message);
mxExport void fxLogin(txMachine* the);
mxExport void fxLogout(txMachine* the);
#endif
mxExport void fxBubble(txMachine* the, txInteger flags, void* message, txInteger length, txString conversation);
mxExport void fxFileEvalString(txMachine* the, txString string, txString tag);
mxExport void fxReport(txMachine* the, txString theFormat, ...);
mxExport void fxReportError(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...);
mxExport void fxReportWarning(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...);
#ifdef mxInstrument	
extern void fxDescribeInstrumentation(txMachine* the, txInteger count, txString* names, txString* units);
extern void fxSampleInstrumentation(txMachine* the, txInteger count, txInteger* values);
extern void fxCheckProfiler(txMachine* the, txSlot* frame);
extern void fxCreateProfiler(txMachine* the);
extern void fxDeleteProfiler(txMachine* the, void* stream);
extern void fxResumeProfiler(txMachine* the);
extern void fxSuspendProfiler(txMachine* the);
#define mxFloatingPointOp(operation) \
		/* fprintf(stderr, "float: %s\n", operation); */ \
		the->floatingPointOps += 1
#else
#define mxFloatingPointOp(operation)
#endif

/* xsType.c */

extern txSlot* fxAliasInstance(txMachine* the, txSlot* instance);
extern txSlot* fxDuplicateInstance(txMachine* the, txSlot* instance);
extern txSlot* fxGetInstance(txMachine* the, txSlot* theSlot);
extern txSlot* fxGetPrototype(txMachine* the, txSlot* instance);
extern txBoolean fxIsSameInstance(txMachine* the, txSlot* a, txSlot* b);
extern txSlot* fxNewInstance(txMachine* the);
extern txSlot* fxToInstance(txMachine* the, txSlot* theSlot);
extern void fxToPrimitive(txMachine* the, txSlot* theSlot, txBoolean theHint);
extern void fxToSpeciesConstructor(txMachine* the, txSlot* constructor);
extern txFlag fxDescriptorToSlot(txMachine* the, txSlot* descriptor);
extern void fxDescribeProperty(txMachine* the, txSlot* property, txFlag mask);
extern txBoolean fxIsPropertyCompatible(txMachine* the, txSlot* property, txSlot* slot, txFlag mask);
mxExport void fx_species_get(txMachine* the);

extern const txBehavior gxOrdinaryBehavior;
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
mxExport void fxOrdinaryToPrimitive(txMachine* the);

extern const txBehavior gxEnvironmentBehavior;
extern txSlot* fxNewEnvironmentInstance(txMachine* the, txSlot* environment);
extern void fxRunEvalEnvironment(txMachine* the);
extern void fxRunProgramEnvironment(txMachine* the);

extern txSlot* fxNewRealmInstance(txMachine* the);
extern txSlot* fxNewProgramInstance(txMachine* the);

/* xsProperty.c */
extern txSlot* fxNextHostAccessorProperty(txMachine* the, txSlot* property, txCallback get, txCallback set, txID id, txFlag flag);
extern txSlot* fxNextHostFunctionProperty(txMachine* the, txSlot* property, txCallback call, txInteger length, txID id, txFlag flag);

extern txSlot* fxLastProperty(txMachine* the, txSlot* slot);
extern txSlot* fxNextUndefinedProperty(txMachine* the, txSlot* property, txID id, txFlag flag);
extern txSlot* fxNextNullProperty(txMachine* the, txSlot* property, txID id, txFlag flag);
extern txSlot* fxNextBooleanProperty(txMachine* the, txSlot* property, txBoolean boolean, txID id, txFlag flag);
extern txSlot* fxNextIntegerProperty(txMachine* the, txSlot* property, txInteger integer, txID id, txFlag flag);
extern txSlot* fxNextNumberProperty(txMachine* the, txSlot* property, txNumber number, txID id, txFlag flag);
extern txSlot* fxNextReferenceProperty(txMachine* the, txSlot* property, txSlot* slot, txID id, txFlag flag);
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
extern void fxSetIndexSize(txMachine* the, txSlot* array, txIndex target, txBoolean growable);

extern txBoolean fxDefinePrivateProperty(txMachine* the, txSlot* instance, txSlot* check, txID id, txSlot* slot, txFlag mask);
extern txSlot* fxGetPrivateProperty(txMachine* the, txSlot* instance, txSlot* check, txID id);
extern txSlot* fxSetPrivateProperty(txMachine* the, txSlot* instance, txSlot* check, txID id);

/* xsGlobal.c */
extern const txBehavior gxGlobalBehavior;
extern void fxBuildGlobal(txMachine* the);
extern txSlot* fxNewGlobalInstance(txMachine* the);

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
mxExport void fx_trace_center(txMachine* the);
mxExport void fx_trace_left(txMachine* the);
mxExport void fx_trace_right(txMachine* the);
mxExport void fx_unescape(txMachine* the);

extern txSlot* fxCheckIteratorInstance(txMachine* the, txSlot* slot, txID id);
extern txSlot* fxCheckIteratorResult(txMachine* the, txSlot* result);
extern txBoolean fxGetIterator(txMachine* the, txSlot* iterable, txSlot* iterator, txSlot* next, txBoolean optional);
extern txBoolean fxIteratorNext(txMachine* the, txSlot* iterator, txSlot* next, txSlot* value);
extern void fxIteratorReturn(txMachine* the, txSlot* iterator);
extern txSlot* fxNewIteratorInstance(txMachine* the, txSlot* iterable, txID id);
mxExport void fxDecodeURI(txMachine* the, txString theSet);
mxExport void fxEncodeURI(txMachine* the, txString theSet);

/* xsObject.c */
mxExport void fx_Object(txMachine* the);
mxExport void fx_Object_prototype___proto__get(txMachine* the);
mxExport void fx_Object_prototype___proto__set(txMachine* the);
mxExport void fx_Object_prototype___defineGetter__(txMachine* the);
mxExport void fx_Object_prototype___defineSetter__(txMachine* the);
mxExport void fx_Object_prototype___lookupGetter__(txMachine* the);
mxExport void fx_Object_prototype___lookupSetter__(txMachine* the);
mxExport void fx_Object_prototype_hasOwnProperty(txMachine* the);
mxExport void fx_Object_prototype_isPrototypeOf(txMachine* the);
mxExport void fx_Object_prototype_propertyIsEnumerable(txMachine* the);
mxExport void fx_Object_prototype_toLocaleString(txMachine* the);
mxExport void fx_Object_prototype_toString(txMachine* the);
mxExport void fx_Object_prototype_valueOf(txMachine* the);
mxExport void fx_Object_assign(txMachine* the);
mxExport void fx_Object_copy(txMachine* the);
mxExport void fx_Object_create(txMachine* the);
mxExport void fx_Object_defineProperties(txMachine* the);
mxExport void fx_Object_defineProperty(txMachine* the);
mxExport void fx_Object_entries(txMachine* the);
mxExport void fx_Object_freeze(txMachine* the);
mxExport void fx_Object_fromEntries(txMachine* the);
mxExport void fx_Object_getOwnPropertyDescriptor(txMachine* the);
mxExport void fx_Object_getOwnPropertyDescriptors(txMachine* the);
mxExport void fx_Object_getOwnPropertyNames(txMachine* the);
mxExport void fx_Object_getOwnPropertySymbols(txMachine* the);
mxExport void fx_Object_getPrototypeOf(txMachine* the);
mxExport void fx_Object_hasOwn(txMachine* the);
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
mxExport txBoolean fxIsCallable(txMachine* the, txSlot* slot);
extern txBoolean fxIsFunction(txMachine* the, txSlot* slot);
extern txSlot* fxNewFunctionInstance(txMachine* the, txID name);
extern txSlot* fxNewFunctionLength(txMachine* the, txSlot* instance, txNumber length);
extern txSlot* fxNewFunctionName(txMachine* the, txSlot* instance, txID id, txIndex index, txID former, txString prefix);
extern void fxRenameFunction(txMachine* the, txSlot* function, txID id, txIndex index, txID former, txString prefix);

mxExport void fx_AsyncFunction(txMachine* the);

extern txSlot* fxNewAsyncInstance(txMachine* the);
extern void fxResolveAwait(txMachine* the);
extern void fxRejectAwait(txMachine* the);
extern void fxRunAsync(txMachine* the, txSlot* instance);

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
mxExport void fx_Symbol_prototype_get_description(txMachine* the);
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
extern void fxIDToString(txMachine* the, txID id, txString theBuffer, txSize theSize);

/* xsError.c */
mxExport void fx_Error(txMachine* the);
mxExport void fx_Error_toString(txMachine* the);
mxExport void fx_AggregateError(txMachine* the);
mxExport void fx_EvalError(txMachine* the);
mxExport void fx_RangeError(txMachine* the);
mxExport void fx_ReferenceError(txMachine* the);
mxExport void fx_SyntaxError(txMachine* the);
mxExport void fx_TypeError(txMachine* the);
mxExport void fx_URIError(txMachine* the);
mxExport void fx_Error_prototype_get_stack(txMachine* the);

extern void fxBuildError(txMachine* the);
extern void fxCaptureErrorStack(txMachine* the, txSlot* internal, txSlot* frame);

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
mxExport void fx_Number_prototype_toLocaleString(txMachine* the);
mxExport void fx_Number_prototype_toPrecision(txMachine* the);
mxExport void fx_Number_prototype_toString(txMachine* the);
mxExport void fx_Number_prototype_valueOf(txMachine* the);

extern void fxBuildNumber(txMachine* the);
extern txSlot* fxNewNumberInstance(txMachine* the);
extern void fxNumberCoerce(txMachine* the, txSlot* slot);
extern void fxIntCoerce(txMachine* the, txSlot* slot);
extern void fxUintCoerce(txMachine* the, txSlot* slot);

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
mxExport void fx_Math_idiv(txMachine* the);
mxExport void fx_Math_idivmod(txMachine* the);
mxExport void fx_Math_imod(txMachine* the);
mxExport void fx_Math_imul(txMachine* the);
mxExport void fx_Math_imuldiv(txMachine* the);
mxExport void fx_Math_irem(txMachine* the);
mxExport void fx_Math_log(txMachine* the);
mxExport void fx_Math_log1p(txMachine* the);
mxExport void fx_Math_log10(txMachine* the);
mxExport void fx_Math_log2(txMachine* the);
mxExport void fx_Math_max(txMachine* the);
mxExport void fx_Math_min(txMachine* the);
mxExport void fx_Math_mod(txMachine* the);
mxExport void fx_Math_pow(txMachine* the);
mxExport void fx_Math_random(txMachine* the);
mxExport void fx_Math_random_secure(txMachine* the);
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
extern txNumber fx_pow(txNumber x, txNumber y);

/* xsBigInt.c */
mxExport void fx_BigInt(txMachine* the);
mxExport void fx_BigInt_asIntN(txMachine* the);
mxExport void fx_BigInt_asUintN(txMachine* the);
mxExport void fx_BigInt_bitLength(txMachine* the);
mxExport void fx_BigInt_fromArrayBuffer(txMachine* the);
mxExport void fx_BigInt_prototype_toString(txMachine* the);
mxExport void fx_BigInt_prototype_valueOf(txMachine* the);

extern void fxBuildBigInt(txMachine* the);
extern void fxBigIntCoerce(txMachine* the, txSlot* slot);
extern txBoolean fxBigIntCompare(txMachine* the, txBoolean less, txBoolean equal, txBoolean more, txSlot* left, txSlot* right);
extern void fxBigIntDecode(txMachine* the, txSize size);
extern void fxBigintToArrayBuffer(txMachine* the, txSlot* slot, txU4 minBytes, txBoolean sign, int endian);
extern txSlot* fxBigIntToInstance(txMachine* the, txSlot* slot);
extern txNumber fxBigIntToNumber(txMachine* the, txSlot* slot);
extern void fxBigintToString(txMachine* the, txSlot* slot, txU4 radix);
extern txS8 fxToBigInt64(txMachine* the, txSlot* slot);
extern txU8 fxToBigUint64(txMachine* the, txSlot* slot);

mxExport void fxBigInt(txMachine* the, txSlot* slot, txU1 sign, txU2 size, txU4* data);
mxExport void fxBigIntX(txMachine* the, txSlot* slot, txU1 sign, txU2 size, txU4* data);
mxExport txBigInt* fxToBigInt(txMachine* the, txSlot* slot, txFlag strict);
mxExport void fxFromBigInt64(txMachine* the, txSlot* slot, txS8 value);
mxExport void fxFromBigUint64(txMachine* the, txSlot* slot, txU8 value);

mxExport txBigInt* fxBigInt_add(txMachine* the, txBigInt* r, txBigInt* a, txBigInt* b);
mxExport txBigInt* fxBigInt_and(txMachine* the, txBigInt* r, txBigInt* a, txBigInt* b);
mxExport txBigInt* fxBigInt_dec(txMachine* the, txBigInt* r, txBigInt* a);
mxExport txBigInt* fxBigInt_div(txMachine* the, txBigInt* r, txBigInt* a, txBigInt* b);
mxExport txBigInt* fxBigInt_exp(txMachine* the, txBigInt* r, txBigInt* a, txBigInt* b);
mxExport txBigInt* fxBigInt_inc(txMachine* the, txBigInt* r, txBigInt* a);
mxExport txBigInt* fxBigInt_lsl(txMachine* the, txBigInt* r, txBigInt* a, txBigInt* b);
mxExport txBigInt* fxBigInt_lsr(txMachine* the, txBigInt* r, txBigInt* a, txBigInt* b);
mxExport txBigInt* fxBigInt_mul(txMachine* the, txBigInt* r, txBigInt* a, txBigInt* b);
mxExport txBigInt* fxBigInt_neg(txMachine* the, txBigInt* r, txBigInt* a);
mxExport txBigInt* fxBigInt_nop(txMachine* the, txBigInt* r, txBigInt* a, txBigInt* b);
mxExport txBigInt* fxBigInt_not(txMachine* the, txBigInt* r, txBigInt* a);
mxExport txBigInt* fxBigInt_or(txMachine* the, txBigInt* r, txBigInt* a, txBigInt* b);
mxExport txBigInt* fxBigInt_rem(txMachine* the, txBigInt* r, txBigInt* a, txBigInt* b);
mxExport txBigInt* fxBigInt_sub(txMachine* the, txBigInt* r, txBigInt* a, txBigInt* b);
mxExport txBigInt* fxBigInt_xor(txMachine* the, txBigInt* r, txBigInt* a, txBigInt* b);

mxExport txBigInt *fxBigInt_alloc(txMachine* the, txU4 size);
mxExport void fxBigInt_free(txMachine* the, txBigInt*);

mxExport int fxBigInt_comp(txBigInt* a, txBigInt* b);
mxExport void fxBigInt_copy(txBigInt* a, txBigInt* b);
mxExport txBigInt* fxBigInt_mod(txMachine* the, txBigInt* r, txBigInt* a, txBigInt* b);
mxExport txBigInt* fxBigInt_sqr(txMachine* the, txBigInt* r, txBigInt* a);

mxExport int fxBigInt_iszero(txBigInt *a);
mxExport txBigInt *fxBigInt_dup(txMachine* the, txBigInt *a);
mxExport int fxBigInt_bitsize(txBigInt *e);

mxExport int fxBigInt_ucomp(txBigInt *a, txBigInt *b);

mxExport txBigInt *fxBigInt_uand(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b);
mxExport txBigInt *fxBigInt_uor(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b);
mxExport txBigInt *fxBigInt_uxor(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b);

mxExport txBigInt *fxBigInt_ulsl1(txMachine* the, txBigInt *r, txBigInt *a, txU4 sw);
mxExport txBigInt *fxBigInt_ulsr1(txMachine* the, txBigInt *r, txBigInt *a, txU4 sw);

mxExport txBigInt *fxBigInt_uadd(txMachine* the, txBigInt *rr, txBigInt *aa, txBigInt *bb);
mxExport txBigInt *fxBigInt_usub(txMachine* the, txBigInt *rr, txBigInt *aa, txBigInt *bb);

mxExport txBigInt *fxBigInt_umul(txMachine* the, txBigInt *rr, txBigInt *aa, txBigInt *bb);
mxExport txBigInt *fxBigInt_umul1(txMachine* the, txBigInt *r, txBigInt *a, txU4 b);
mxExport txBigInt *fxBigInt_udiv(txMachine* the, txBigInt *q, txBigInt *a, txBigInt *b, txBigInt **r);

mxExport int fxBigInt_isset(txBigInt *e, txU4 i);
mxExport void fxBigInt_fill0(txBigInt *r);

/* xsDate.c */
mxExport void fx_Date(txMachine* the);
mxExport void fx_Date_secure(txMachine* the);
mxExport void fx_Date_now(txMachine* the);
mxExport void fx_Date_now_secure(txMachine* the);
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
#ifndef mxCESU8
mxExport void fx_String_fromArrayBuffer(txMachine* the);
#endif
mxExport void fx_String_fromCharCode(txMachine* the);
mxExport void fx_String_fromCodePoint(txMachine* the);
mxExport void fx_String_raw(txMachine* the);
mxExport void fx_String_prototype_at(txMachine* the);
mxExport void fx_String_prototype_charAt(txMachine* the);
mxExport void fx_String_prototype_charCodeAt(txMachine* the);
mxExport void fx_String_prototype_codePointAt(txMachine* the);
mxExport void fx_String_prototype_compare(txMachine* the);
mxExport void fx_String_prototype_concat(txMachine* the);
mxExport void fx_String_prototype_endsWith(txMachine* the);
mxExport void fx_String_prototype_includes(txMachine* the);
mxExport void fx_String_prototype_indexOf(txMachine* the);
mxExport void fx_String_prototype_lastIndexOf(txMachine* the);
mxExport void fx_String_prototype_localeCompare(txMachine* the);
mxExport void fx_String_prototype_match(txMachine* the);
mxExport void fx_String_prototype_matchAll(txMachine* the);
mxExport void fx_String_prototype_normalize(txMachine* the);
mxExport void fx_String_prototype_padEnd(txMachine* the);
mxExport void fx_String_prototype_padStart(txMachine* the);
mxExport void fx_String_prototype_repeat(txMachine* the);
mxExport void fx_String_prototype_replace(txMachine* the);
mxExport void fx_String_prototype_replaceAll(txMachine* the);
mxExport void fx_String_prototype_search(txMachine* the);
mxExport void fx_String_prototype_slice(txMachine* the);
mxExport void fx_String_prototype_split(txMachine* the);
mxExport void fx_String_prototype_startsWith(txMachine* the);
mxExport void fx_String_prototype_substr(txMachine* the);
mxExport void fx_String_prototype_substring(txMachine* the);
mxExport void fx_String_prototype_toLowerCase(txMachine* the);
mxExport void fx_String_prototype_toUpperCase(txMachine* the);
mxExport void fx_String_prototype_trim(txMachine* the);
mxExport void fx_String_prototype_trimEnd(txMachine* the);
mxExport void fx_String_prototype_trimStart(txMachine* the);
mxExport void fx_String_prototype_valueOf(txMachine* the);
mxExport void fx_String_prototype_iterator(txMachine* the);
mxExport void fx_String_prototype_iterator_next(txMachine* the);
mxExport void fxStringAccessorGetter(txMachine* the);
mxExport void fxStringAccessorSetter(txMachine* the);

extern void fxBuildString(txMachine* the);
extern txSlot* fxNewStringInstance(txMachine* the);
extern txSlot* fxAccessStringProperty(txMachine* the, txSlot* instance, txInteger index);
extern void fxPushSubstitutionString(txMachine* the, txSlot* string, txInteger size, txInteger offset, txSlot* match, txInteger length, txInteger count, txSlot* captures, txSlot* groups, txSlot* replace);

/* xsRegExp.c */
mxExport void fx_RegExp(txMachine* the);
mxExport void fx_RegExp_prototype_get_dotAll(txMachine* the);
mxExport void fx_RegExp_prototype_get_flags(txMachine* the);
mxExport void fx_RegExp_prototype_get_global(txMachine* the);
mxExport void fx_RegExp_prototype_get_hasIndices(txMachine* the);
mxExport void fx_RegExp_prototype_get_ignoreCase(txMachine* the);
mxExport void fx_RegExp_prototype_get_multiline(txMachine* the);
mxExport void fx_RegExp_prototype_get_source(txMachine* the);
mxExport void fx_RegExp_prototype_get_sticky(txMachine* the);
mxExport void fx_RegExp_prototype_get_unicode(txMachine* the);
mxExport void fx_RegExp_prototype_compile(txMachine* the);
mxExport void fx_RegExp_prototype_exec(txMachine* the);
mxExport void fx_RegExp_prototype_match(txMachine* the);
mxExport void fx_RegExp_prototype_matchAll(txMachine* the);
mxExport void fx_RegExp_prototype_matchAll_next(txMachine* the);
mxExport void fx_RegExp_prototype_replace(txMachine* the);
mxExport void fx_RegExp_prototype_search(txMachine* the);
mxExport void fx_RegExp_prototype_split(txMachine* the);
mxExport void fx_RegExp_prototype_test(txMachine* the);
mxExport void fx_RegExp_prototype_toString(txMachine* the);
mxExport void fxInitializeRegExp(txMachine* the);

extern void fxBuildRegExp(txMachine* the);
extern txBoolean fxIsRegExp(txMachine* the, txSlot* slot);
extern txSlot* fxNewRegExpInstance(txMachine* the);
extern void fx_String_prototype_toCase(txMachine* the, txBoolean flag);

/* xsArguments.c */
extern const txBehavior gxArgumentsSloppyBehavior;

extern void fxBuildArguments(txMachine* the);

extern txSlot* fxNewArgumentsSloppyInstance(txMachine* the, txIndex count);
extern txSlot* fxNewArgumentsStrictInstance(txMachine* the, txIndex count);
mxExport void fxThrowTypeError(txMachine* the);

/* xsArray.c */
extern const txBehavior gxArrayBehavior;

extern void fxBuildArray(txMachine* the);

mxExport void fxArrayLengthGetter(txMachine* the);
mxExport void fxArrayLengthSetter(txMachine* the);

mxExport void fx_Array(txMachine* the);
mxExport void fx_Array_from(txMachine* the);
mxExport void fx_Array_isArray(txMachine* the);
mxExport void fx_Array_of(txMachine* the);
mxExport void fx_Array_prototype_at(txMachine* the);
mxExport void fx_Array_prototype_concat(txMachine* the);
mxExport void fx_Array_prototype_copyWithin(txMachine* the);
mxExport void fx_Array_prototype_entries(txMachine* the);
mxExport void fx_Array_prototype_every(txMachine* the);
mxExport void fx_Array_prototype_fill(txMachine* the);
mxExport void fx_Array_prototype_filter(txMachine* the);
mxExport void fx_Array_prototype_find(txMachine* the);
mxExport void fx_Array_prototype_findIndex(txMachine* the);
mxExport void fx_Array_prototype_findLast(txMachine* the);
mxExport void fx_Array_prototype_findLastIndex(txMachine* the);
mxExport void fx_Array_prototype_flat(txMachine* the);
mxExport void fx_Array_prototype_flatMap(txMachine* the);
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
extern void fxSortArrayItems(txMachine* the, txSlot* function, txSlot* array, txNumber LENGTH);
extern txNumber fxToLength(txMachine* the, txSlot* slot);

/* xsDataView.c */

enum {
	EndianNative = 0,
	EndianLittle = 1,
	EndianBig = 2
};

extern int fxBigInt64Compare(const void* p, const void* q);
extern void fxBigInt64Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern void fxBigInt64Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern int fxBigUint64Compare(const void* p, const void* q);
extern void fxBigUint64Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
extern void fxBigUint64Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
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

mxExport void *fxArrayBuffer(txMachine* the, txSlot* slot, void* data, txInteger byteLength, txInteger maxByteLength);
mxExport void fxGetArrayBufferData(txMachine* the, txSlot* slot, txInteger byteOffset, void* data, txInteger byteLength);
mxExport txInteger fxGetArrayBufferLength(txMachine* the, txSlot* slot);
mxExport txInteger fxGetArrayBufferMaxLength(txMachine* the, txSlot* slot);
mxExport void fxSetArrayBufferData(txMachine* the, txSlot* slot, txInteger byteOffset, void* data, txInteger byteLength);
mxExport void fxSetArrayBufferLength(txMachine* the, txSlot* slot, txInteger byteLength);
mxExport void* fxToArrayBuffer(txMachine* the, txSlot* slot);

mxExport void fx_ArrayBuffer(txMachine* the);
mxExport void fx_ArrayBuffer_fromBigInt(txMachine* the);
#ifndef mxCESU8
mxExport void fx_ArrayBuffer_fromString(txMachine* the);
#endif
mxExport void fx_ArrayBuffer_isView(txMachine* the);
mxExport void fx_ArrayBuffer_prototype_get_byteLength(txMachine* the);
mxExport void fx_ArrayBuffer_prototype_get_maxByteLength(txMachine* the);
mxExport void fx_ArrayBuffer_prototype_get_resizable(txMachine* the);
mxExport void fx_ArrayBuffer_prototype_concat(txMachine* the);
mxExport void fx_ArrayBuffer_prototype_resize(txMachine* the);
mxExport void fx_ArrayBuffer_prototype_slice(txMachine* the);
mxExport void fx_ArrayBuffer_prototype_transfer(txMachine* the);

mxExport void fx_DataView(txMachine* the);
mxExport void fx_DataView_prototype_buffer_get(txMachine* the);
mxExport void fx_DataView_prototype_byteLength_get(txMachine* the);
mxExport void fx_DataView_prototype_byteOffset_get(txMachine* the);
mxExport void fx_DataView_prototype_getBigInt64(txMachine* the);
mxExport void fx_DataView_prototype_getBigUint64(txMachine* the);
mxExport void fx_DataView_prototype_getFloat32(txMachine* the);
mxExport void fx_DataView_prototype_getFloat64(txMachine* the);
mxExport void fx_DataView_prototype_getInt8(txMachine* the);
mxExport void fx_DataView_prototype_getInt16(txMachine* the);
mxExport void fx_DataView_prototype_getInt32(txMachine* the);
mxExport void fx_DataView_prototype_getUint8(txMachine* the);
mxExport void fx_DataView_prototype_getUint16(txMachine* the);
mxExport void fx_DataView_prototype_getUint32(txMachine* the);
mxExport void fx_DataView_prototype_setBigInt64(txMachine* the);
mxExport void fx_DataView_prototype_setBigUint64(txMachine* the);
mxExport void fx_DataView_prototype_setFloat32(txMachine* the);
mxExport void fx_DataView_prototype_setFloat64(txMachine* the);
mxExport void fx_DataView_prototype_setInt8(txMachine* the);
mxExport void fx_DataView_prototype_setInt16(txMachine* the);
mxExport void fx_DataView_prototype_setInt32(txMachine* the);
mxExport void fx_DataView_prototype_setUint8(txMachine* the);
mxExport void fx_DataView_prototype_setUint16(txMachine* the);
mxExport void fx_DataView_prototype_setUint32(txMachine* the);

mxExport const txTypeDispatch gxTypeDispatches[];
mxExport const txBehavior gxTypedArrayBehavior;

mxExport void fxTypedArrayGetter(txMachine* the);
mxExport void fxTypedArraySetter(txMachine* the);

mxExport void fx_TypedArray(txMachine* the);
mxExport void fx_TypedArray_from(txMachine* the);
mxExport void fx_TypedArray_of(txMachine* the);
mxExport void fx_TypedArray_prototype_at(txMachine* the);
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
mxExport void fx_TypedArray_prototype_findLast(txMachine* the);
mxExport void fx_TypedArray_prototype_findLastIndex(txMachine* the);
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
extern void fxConstructArrayBufferResult(txMachine* the, txSlot* constructor, txInteger length);

extern txInteger fxArgToByteLength(txMachine* the, txInteger argi, txInteger length);
extern txInteger fxGetDataViewSize(txMachine* the, txSlot* view, txSlot* buffer);

/* xsAtomics.c */
extern void fxInt8Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt64Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint64Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt64And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint64And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt64CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint64CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt64Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint64Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt64Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint64Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt64Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint64Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt64Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint64Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt64Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint64Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern void fxInt8Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt16Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt32Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxInt64Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint8Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint16Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint32Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);
extern void fxUint64Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian);

extern txInteger fxInt32Wait(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, txNumber timeout);
extern txInteger fxInt64Wait(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, txNumber timeout);

mxExport const txTypeAtomics gxTypeAtomics[];
extern void fxBuildAtomics(txMachine* the);

mxExport void fx_SharedArrayBuffer(txMachine* the);
mxExport void fx_SharedArrayBuffer_prototype_get_byteLength(txMachine* the);
mxExport void fx_SharedArrayBuffer_prototype_get_growable(txMachine* the);
mxExport void fx_SharedArrayBuffer_prototype_get_maxByteLength(txMachine* the);
mxExport void fx_SharedArrayBuffer_prototype_grow(txMachine* the);
mxExport void fx_SharedArrayBuffer_prototype_slice(txMachine* the);
mxExport void fx_Atomics_add(txMachine* the);
mxExport void fx_Atomics_and(txMachine* the);
mxExport void fx_Atomics_compareExchange(txMachine* the);
mxExport void fx_Atomics_exchange(txMachine* the);
mxExport void fx_Atomics_isLockFree(txMachine* the);
mxExport void fx_Atomics_load(txMachine* the);
mxExport void fx_Atomics_or(txMachine* the);
mxExport void fx_Atomics_notify(txMachine* the);
mxExport void fx_Atomics_store(txMachine* the);
mxExport void fx_Atomics_sub(txMachine* the);
mxExport void fx_Atomics_wait(txMachine* the);
mxExport void fx_Atomics_xor(txMachine* the);

/* xsMapSet.c */
mxExport void fx_Map(txMachine* the);
mxExport void fx_Map_prototype_clear(txMachine* the);
mxExport void fx_Map_prototype_delete(txMachine* the);
mxExport void fx_Map_prototype_entries(txMachine* the);
mxExport void fx_Map_prototype_forEach(txMachine* the);
mxExport void fx_Map_prototype_get(txMachine* the);
mxExport void fx_Map_prototype_has(txMachine* the);
mxExport void fx_Map_prototype_keys(txMachine* the);
mxExport void fx_Map_prototype_set(txMachine* the);
mxExport void fx_Map_prototype_size(txMachine* the);
mxExport void fx_Map_prototype_values(txMachine* the);
mxExport void fx_MapIterator_prototype_next(txMachine* the);
mxExport void fx_Set(txMachine* the);
mxExport void fx_Set_prototype_add(txMachine* the);
mxExport void fx_Set_prototype_clear(txMachine* the);
mxExport void fx_Set_prototype_delete(txMachine* the);
mxExport void fx_Set_prototype_entries(txMachine* the);
mxExport void fx_Set_prototype_forEach(txMachine* the);
mxExport void fx_Set_prototype_has(txMachine* the);
mxExport void fx_Set_prototype_size(txMachine* the);
mxExport void fx_Set_prototype_values(txMachine* the);
mxExport void fx_SetIterator_prototype_next(txMachine* the);
mxExport void fx_WeakMap(txMachine* the);
mxExport void fx_WeakMap_prototype_delete(txMachine* the);
mxExport void fx_WeakMap_prototype_get(txMachine* the);
mxExport void fx_WeakMap_prototype_has(txMachine* the);
mxExport void fx_WeakMap_prototype_set(txMachine* the);
mxExport void fx_WeakSet(txMachine* the);
mxExport void fx_WeakSet_prototype_add(txMachine* the);
mxExport void fx_WeakSet_prototype_delete(txMachine* the);
mxExport void fx_WeakSet_prototype_has(txMachine* the);
mxExport void fx_WeakRef(txMachine* the);
mxExport void fx_WeakRef_prototype_deref(txMachine* the);
mxExport void fx_FinalizationRegistry(txMachine* the);
//mxExport void fx_FinalizationRegistry_prototype_cleanupSome(txMachine* the);
mxExport void fx_FinalizationRegistry_prototype_register(txMachine* the);
mxExport void fx_FinalizationRegistry_prototype_unregister(txMachine* the);

extern void fxBuildMapSet(txMachine* the);
extern txSlot* fxNewMapInstance(txMachine* the);
extern txSlot* fxNewSetInstance(txMachine* the);
extern txSlot* fxNewWeakMapInstance(txMachine* the);
extern txSlot* fxNewWeakSetInstance(txMachine* the);
extern void fxCleanupFinalizationRegistries(txMachine* the);
extern txU4 fxSumEntry(txMachine* the, txSlot* slot); 

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

mxExport void fx_AsyncGenerator(txMachine* the);
mxExport void fx_AsyncGenerator_prototype_next(txMachine* the);
mxExport void fx_AsyncGenerator_prototype_return(txMachine* the);
mxExport void fx_AsyncGenerator_prototype_throw(txMachine* the);
mxExport void fx_AsyncGeneratorFunction(txMachine* the);

mxExport void fx_AsyncIterator_asyncIterator(txMachine* the);
mxExport void fx_AsyncFromSyncIterator_prototype_next(txMachine* the);
mxExport void fx_AsyncFromSyncIterator_prototype_return(txMachine* the);
mxExport void fx_AsyncFromSyncIterator_prototype_throw(txMachine* the);

extern void fxBuildGenerator(txMachine* the);
extern txSlot* fxNewGeneratorInstance(txMachine* the);
extern txSlot* fxNewGeneratorFunctionInstance(txMachine* the, txID name);
extern txSlot* fxNewAsyncGeneratorInstance(txMachine* the);
extern txSlot* fxNewAsyncGeneratorFunctionInstance(txMachine* the, txID name);
extern txSlot* fxNewAsyncFromSyncIteratorInstance(txMachine* the);
extern void fxAsyncGeneratorRejectAwait(txMachine* the);
extern void fxAsyncGeneratorRejectYield(txMachine* the);
extern void fxAsyncGeneratorResolveAwait(txMachine* the);
extern void fxAsyncGeneratorResolveYield(txMachine* the);
extern void fxAsyncFromSyncIteratorDone(txMachine* the);

/* xsPromise.c */
mxExport void fx_Promise(txMachine* the);
mxExport void fx_Promise_all(txMachine* the);
mxExport void fx_Promise_allSettled(txMachine* the);
mxExport void fx_Promise_any(txMachine* the);
mxExport void fx_Promise_race(txMachine* the);
mxExport void fx_Promise_reject(txMachine* the);
mxExport void fx_Promise_resolve(txMachine* the);
mxExport void fx_Promise_prototype_catch(txMachine* the);
mxExport void fx_Promise_prototype_finally(txMachine* the);
mxExport void fx_Promise_prototype_then(txMachine* the);
mxExport void fxOnRejectedPromise(txMachine* the);
mxExport void fxOnResolvedPromise(txMachine* the);
mxExport void fxOnThenable(txMachine* the);

extern void fx_Promise_resolveAux(txMachine* the);
extern void fx_Promise_prototype_finallyAux(txMachine* the);
extern void fx_Promise_prototype_finallyReturn(txMachine* the);
extern void fx_Promise_prototype_finallyThrow(txMachine* the);

extern void fxBuildPromise(txMachine* the);
extern void fxCheckUnhandledRejections(txMachine* the, txBoolean atExit);
extern void fxCombinePromisesCallback(txMachine* the);
extern txSlot* fxNewPromiseCapability(txMachine* the, txSlot* resolveFunction, txSlot* rejectFunction);
extern void fxNewPromiseCapabilityCallback(txMachine* the);
extern txSlot* fxNewPromiseInstance(txMachine* the);
extern void fxPromiseThen(txMachine* the, txSlot* promise, txSlot* onFullfilled, txSlot* onRejected, txSlot* resolveFunction, txSlot* rejectFunction);
extern void fxPushPromiseFunctions(txMachine* the, txSlot* promise);
extern void fxQueueJob(txMachine* the, txInteger count, txSlot* promise);
extern void fxRejectException(txMachine* the, txSlot* rejectFunction);
extern void fxRejectPromise(txMachine* the);
extern void fxResolvePromise(txMachine* the);
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

extern txBoolean fxIsLoadingModule(txMachine* the, txSlot* realm, txID moduleID);
extern void fxRunModule(txMachine* the, txSlot* realm, txID moduleID, txScript* script);

extern void fxExecuteModulesFulfilled(txMachine* the);
extern void fxExecuteModulesRejected(txMachine* the);
extern void fxLoadModulesFulfilled(txMachine* the);
extern void fxLoadModulesRejected(txMachine* the);
extern void fxPrepareModule(txMachine* the, txFlag flag);
extern void fxPrepareTransfer(txMachine* the);
extern void fxResolveModule(txMachine* the, txSlot* module, txID moduleID, txScript* script, void* data, txDestructor destructor);
extern void fxRunImport(txMachine* the, txSlot* realm, txID id);

mxExport void fxModuleGetter(txMachine* the);

mxExport void fx_Compartment(txMachine* the);
mxExport void fx_Compartment_prototype_get_globalThis(txMachine* the);
mxExport void fx_Compartment_prototype_evaluate(txMachine* the);
mxExport void fx_Compartment_prototype_import(txMachine* the);
mxExport void fx_Compartment_prototype_importNow(txMachine* the);

mxExport void fx_ModuleSource(txMachine* the);
mxExport void fx_ModuleSource_prototype_get_bindings(txMachine* the);
mxExport void fx_ModuleSource_prototype_get_needsImport(txMachine* the);
mxExport void fx_ModuleSource_prototype_get_needsImportMeta(txMachine* the);

mxExport void fxExecuteVirtualModuleSource(txMachine* the);
mxExport void fxExecuteVirtualModuleSourceImport(txMachine* the);

/* xsLockdown.c */
#ifdef mxLockdown
mxExport void fx_harden(txMachine* the);
mxExport void fx_lockdown(txMachine* the);
mxExport void fx_petrify(txMachine* the);
mxExport void fx_mutabilities(txMachine* the);
#endif

/* xsProfile.c */
#ifdef mxProfile
extern void fxCheckProfiler(txMachine* the, txSlot* frame);
extern void fxCreateProfiler(txMachine* the);
extern void fxDeleteProfiler(txMachine* the, void* stream);
extern void fxResumeProfiler(txMachine* the);
extern void fxSuspendProfiler(txMachine* the);
#endif

enum {
	XS_NO_ERROR = 0,
	XS_UNKNOWN_ERROR,
	XS_EVAL_ERROR,
	XS_RANGE_ERROR,
	XS_REFERENCE_ERROR,
	XS_SYNTAX_ERROR,
	XS_TYPE_ERROR,
	XS_URI_ERROR,
	XS_AGGREGATE_ERROR,
	XS_ERROR_COUNT
};

enum {
	XS_IMMUTABLE = 0,
	XS_MUTABLE = 1,
};

enum {
	XS_CHUNK = 0,
	XS_GROWABLE_CHUNK = 1,
};

enum {
	XS_NO_STATUS = 0,
	XS_RETURN_STATUS = 1,
	XS_THROW_STATUS = 2,

	XS_NO_HINT = 0,
	XS_NUMBER_HINT = 1,
	XS_STRING_HINT = 2,

	XS_NO_FLAG = 0,
	XS_ASYNC_FLAG = 1,
	
	XS_IMPORT_NAMESPACE = 0,
	XS_IMPORT_DEFAULT = 1,
	XS_IMPORT_PREFLIGHT = 2,
	XS_IMPORT_ASYNC = 4,

	XS_OWN = 0,
	XS_ANY = 1,

	/* frame flags */
	/* ? = 1, */
	XS_C_FLAG = 2,
	XS_FIELD_FLAG = 4,
	XS_STEP_INTO_FLAG = 8,
	XS_STEP_OVER_FLAG = 16,
	XS_STRICT_FLAG = 32,
	XS_DEBUG_FLAG = 64,
	XS_MARK_FLAG = 128,

	/* instance flags */
	XS_EXOTIC_FLAG = 1,
	XS_CAN_CALL_FLAG = 2,
	XS_CAN_CONSTRUCT_FLAG = 4,
	XS_DONT_MODIFY_FLAG = 8,
	XS_DONT_PATCH_FLAG = 16,
	XS_LEVEL_FLAG = 32,
	XS_DONT_MARSHALL_FLAG = 64,
	/* XS_MARK_FLAG = 128, */

	/* property flags */
	XS_INTERNAL_FLAG = 1,
	/* XS_DONT_DELETE_FLAG = 2, */
	/* XS_DONT_ENUM_FLAG = 4, */
	/* XS_DONT_SET_FLAG = 8 ,  */
	XS_INSPECTOR_FLAG = 16,
	XS_BASE_FLAG = 32,
	XS_DERIVED_FLAG = 64,
	/* XS_MARK_FLAG = 128, */

	/* mxBehaviorOwnKeys flags */
	XS_EACH_NAME_FLAG = 1,
	XS_EACH_SYMBOL_FLAG = 2,
	
	/* mxBehaviorDefineOwnProperty flags */
	/* XS_NAME_FLAG = 1, */
	/* XS_DONT_DELETE_FLAG = 2, */
	/* XS_DONT_ENUM_FLAG = 4, */
	/* XS_DONT_SET_FLAG = 8, */
	/* XS_METHOD_FLAG = 16, */
	/* XS_GETTER_FLAG = 32, */
	/* XS_SETTER_FLAG = 64, */
	XS_ACCESSOR_FLAG = XS_GETTER_FLAG | XS_SETTER_FLAG,
	XS_GET_ONLY = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG,

	/* collect flags */
	XS_COMPACT_FLAG = 1,
	XS_ORGANIC_FLAG = 2,
	XS_COLLECTING_FLAG = 4,
	XS_TRASHING_FLAG = 8,
	XS_SKIPPED_COLLECT_FLAG = 16,
	XS_HOST_CHUNK_FLAG = 32,
	XS_HOST_HOOKS_FLAG = 64,
	
	/* finalization registry flags */
	XS_FINALIZATION_REGISTRY_CHANGED = 1,
	
	/* bubble flags */
	XS_BUBBLE_LEFT = 1,
	XS_BUBBLE_RIGHT = 2,
	XS_BUBBLE_BINARY = 4,
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
	XS_BIGINT_KIND,
	XS_BIGINT_X_KIND,
	
	XS_REFERENCE_KIND, //10
	
	XS_CLOSURE_KIND, 
	XS_FRAME_KIND,

	XS_INSTANCE_KIND,
	
	XS_ARGUMENTS_SLOPPY_KIND,
	XS_ARGUMENTS_STRICT_KIND, // 15
	XS_ARRAY_KIND,
	XS_ARRAY_BUFFER_KIND,
	XS_CALLBACK_KIND,
	XS_CODE_KIND,
	XS_CODE_X_KIND, // 20
	XS_DATE_KIND,
	XS_DATA_VIEW_KIND,
	XS_FINALIZATION_CELL_KIND,
	XS_FINALIZATION_REGISTRY_KIND,
	XS_GLOBAL_KIND, // 25
	XS_HOST_KIND,
	XS_MAP_KIND,
	XS_MODULE_KIND,
	XS_PROGRAM_KIND,
	XS_PROMISE_KIND, // 30
	XS_PROXY_KIND,
	XS_REGEXP_KIND,
	XS_SET_KIND,
	XS_TYPED_ARRAY_KIND,
	XS_WEAK_MAP_KIND, // 35
	XS_WEAK_REF_KIND,
	XS_WEAK_SET_KIND,

	XS_ACCESSOR_KIND,
	XS_AT_KIND,
	XS_ENTRY_KIND, //40
	XS_ERROR_KIND,
	XS_HOME_KIND,
	XS_KEY_KIND,
	XS_KEY_X_KIND,
	XS_LIST_KIND,  // 45
	XS_PRIVATE_KIND,
	XS_STACK_KIND,
	XS_VAR_KIND,
	XS_CALLBACK_X_KIND,
#ifdef mxHostFunctionPrimitive
	XS_HOST_FUNCTION_KIND,
#endif
	XS_HOST_INSPECTOR_KIND,
	XS_INSTANCE_INSPECTOR_KIND,
	XS_EXPORT_KIND,
	XS_WEAK_ENTRY_KIND,
	XS_BUFFER_INFO_KIND,
	XS_MODULE_SOURCE_KIND,
	XS_IDS_KIND,
};
enum {
	XS_DEBUGGER_EXIT = 0,
	XS_NOT_ENOUGH_MEMORY_EXIT,
	XS_STACK_OVERFLOW_EXIT,
	XS_FATAL_CHECK_EXIT,
	XS_DEAD_STRIP_EXIT,
	XS_UNHANDLED_EXCEPTION_EXIT,
	XS_NO_MORE_KEYS_EXIT,
	XS_TOO_MUCH_COMPUTATION_EXIT,
	XS_UNHANDLED_REJECTION_EXIT,
};

#if mxBigEndian
#define mxInitSlotKind(SLOT,KIND) ((SLOT)->ID_FLAG_KIND = (uint8_t)(KIND))
#else
#define mxInitSlotKind(SLOT,KIND) ((SLOT)->KIND_FLAG_ID = (uint8_t)(KIND))
#endif

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
#define mxIsBigInt(THE_SLOT) \
	(((THE_SLOT)->kind == XS_BIGINT_KIND) || ((THE_SLOT)->kind == XS_BIGINT_X_KIND))
#define mxIsReference(THE_SLOT) \
	((THE_SLOT)->kind == XS_REFERENCE_KIND)
#define mxIsFunction(THE_SLOT) \
	( (THE_SLOT) &&  ((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && (((THE_SLOT)->next->kind == XS_CALLBACK_KIND) || ((THE_SLOT)->next->kind == XS_CALLBACK_X_KIND) || ((THE_SLOT)->next->kind == XS_CODE_KIND) || ((THE_SLOT)->next->kind == XS_CODE_X_KIND)))
#define mxIsString(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && (((THE_SLOT)->next->kind == XS_STRING_KIND) || ((THE_SLOT)->next->kind == XS_STRING_X_KIND)))
#define mxIsBoolean(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_BOOLEAN_KIND))
#define mxIsNumber(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_NUMBER_KIND))
#define mxIsDate(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_DATE_KIND))
#define mxIsRegExp(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_REGEXP_KIND))
#define mxIsSymbol(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_SYMBOL_KIND))
#define mxIsHost(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_HOST_KIND))
#define mxIsPromise(THE_SLOT) \
	((THE_SLOT) && ((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_PROMISE_KIND) && (THE_SLOT != mxPromisePrototype.value.reference))
#define mxIsProxy(THE_SLOT) \
	(/* (THE_SLOT) && */ ((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_PROXY_KIND))
#define mxIsCallable(THE_SLOT) \
	( (THE_SLOT) &&  ((THE_SLOT)->next) && (((THE_SLOT)->next->kind == XS_CALLBACK_KIND) || ((THE_SLOT)->next->kind == XS_CALLBACK_X_KIND) || ((THE_SLOT)->next->kind == XS_CODE_KIND) || ((THE_SLOT)->next->kind == XS_CODE_X_KIND) || ((THE_SLOT)->next->kind == XS_PROXY_KIND)))
#define mxIsConstructor(THE_SLOT) \
	((THE_SLOT) && ((THE_SLOT)->flag & XS_CAN_CONSTRUCT_FLAG))

#define mxIsStringPrimitive(THE_SLOT) \
	(((THE_SLOT)->kind == XS_STRING_KIND) || ((THE_SLOT)->kind == XS_STRING_X_KIND))
	
#ifdef mxMetering
#define mxMeterOne() \
	(the->meterIndex++)
#define mxMeterSome(_COUNT) \
	(the->meterIndex += _COUNT)
#else
#define mxMeterOne() \
	((void)0)
#define mxMeterSome(_COUNT) \
	((void)0)
#endif

#if mxBoundsCheck
#define mxCheckCStack() \
	(fxCheckCStack(the))
#define mxOverflow(_COUNT) \
	(mxMeterOne(), fxOverflow(the,_COUNT,C_NULL, 0))
#else
#define mxCheckCStack() \
	((void)0)
#define mxOverflow(_COUNT) \
	((void)0)
#endif

#define mxCall() \
	(mxOverflow(-4), \
	fxCall(the))
	
#define mxDefineAll(ID, INDEX, FLAG, MASK) \
	(mxMeterOne(), fxDefineAll(the, ID, INDEX, FLAG, MASK))
#define mxDefineAt(FLAG, MASK) \
	(mxMeterOne(), fxDefineAt(the, FLAG, MASK))
#define mxDefineID(ID, FLAG, MASK) \
	(mxMeterOne(), fxDefineAll(the, ID, 0, FLAG, MASK))
#define mxDefineIndex(INDEX, FLAG, MASK) \
	(mxMeterOne(), fxDefineAll(the, XS_NO_ID, INDEX, FLAG, MASK))
	
#define mxDeleteAll(ID, INDEX) \
	(mxMeterOne(), fxDeleteAll(the, ID, INDEX))
#define mxDeleteAt() \
	(mxMeterOne(), fxDeleteAt(the))
#define mxDeleteID(ID) \
	(mxMeterOne(), fxDeleteAll(the, ID, 0))
#define mxDeleteIndex(INDEX) \
	(mxMeterOne(), fxDeleteAll(the, XS_NO_ID, INDEX))
	
#define mxDub() \
	(mxOverflow(-1), \
	((--the->stack)->next = C_NULL, \
	the->stack->flag = XS_NO_FLAG, \
	mxInitSlotKind(the->stack, (the->stack + 1)->kind), \
	the->stack->value = (the->stack + 1)->value))
	
#define mxGetAll(ID, INDEX) \
	(mxMeterOne(), fxGetAll(the, ID, INDEX))
#define mxGetAt() \
	(mxMeterOne(), fxGetAt(the))
#define mxGetID(ID) \
	(mxMeterOne(), fxGetAll(the, ID, 0))
#define mxGetIndex(INDEX) \
	(mxMeterOne(), fxGetAll(the, XS_NO_ID, INDEX))
	
#define mxHasAll(ID, INDEX) \
	(mxMeterOne(), fxHasAll(the, ID, INDEX))
#define mxHasAt() \
	(mxMeterOne(), fxHasAt(the))
#define mxHasID(ID) \
	(mxMeterOne(), fxHasAll(the, ID, 0))
#define mxHasIndex(INDEX) \
	(mxMeterOne(), fxHasAll(the, XS_NO_ID, INDEX))
	
#define mxNew() \
	(mxOverflow(-5), \
	fxNew(the))
	
#define mxRunCount(_COUNT) \
	(mxMeterOne(), fxRunID(the, C_NULL, _COUNT))
	
#define mxSetAll(ID, INDEX) \
	(mxMeterOne(), fxSetAll(the, ID, INDEX))
#define mxSetAt() \
	(mxMeterOne(), fxSetAt(the))
#define mxSetID(ID) \
	(mxMeterOne(), fxSetAll(the, ID, 0))
#define mxSetIndex(INDEX) \
	(mxMeterOne(), fxSetAll(the, XS_NO_ID, INDEX))

#define mxPush(THE_SLOT) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, (THE_SLOT).kind), \
	the->stack->value = (THE_SLOT).value)
#define mxPushSlot(THE_SLOT) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, (THE_SLOT)->kind), \
	the->stack->value = (THE_SLOT)->value)

#define mxPushAt(ID,INDEX) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_AT_KIND), \
	the->stack->value.at.index = (INDEX), \
	the->stack->value.at.id = (ID))
#define mxPushBigInt(THE_BIGINT) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_BIGINT_KIND), \
	the->stack->value.bigint = (THE_BIGINT))
#define mxPushBoolean(THE_BOOLEAN) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_BOOLEAN_KIND), \
	the->stack->value.boolean = (THE_BOOLEAN))
#define mxPushClosure(THE_SLOT) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_CLOSURE_KIND), \
	the->stack->value.closure = (THE_SLOT))
#define mxPushInteger(THE_NUMBER) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_INTEGER_KIND), \
	the->stack->value.integer = (THE_NUMBER))
#define mxPushList() \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_LIST_KIND), \
	the->stack->value.list.first = C_NULL, \
	the->stack->value.list.last = C_NULL)
#define mxPushNull() \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_NULL_KIND))
#define mxPushNumber(THE_NUMBER) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_NUMBER_KIND), \
	the->stack->value.number = (THE_NUMBER))
#define mxPushReference(THE_SLOT) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_REFERENCE_KIND), \
	the->stack->value.reference = (THE_SLOT))
#define mxPushString(THE_STRING) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_STRING_KIND), \
	the->stack->value.string = (THE_STRING))
#define mxPushStringC(THE_STRING) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_UNDEFINED_KIND), \
	fxCopyStringC(the, the->stack, THE_STRING))
#ifdef mxSnapshot
#define mxPushStringX(THE_STRING) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_UNDEFINED_KIND), \
	fxCopyStringC(the, the->stack, THE_STRING))
#else
#define mxPushStringX(THE_STRING) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_STRING_X_KIND), \
	the->stack->value.string = (THE_STRING))
#endif

#define mxPushSymbol(THE_SYMBOL) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_SYMBOL_KIND), \
	the->stack->value.symbol = (THE_SYMBOL))
#define mxPushUndefined() \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_UNDEFINED_KIND))
#define mxPushUninitialized() \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	mxInitSlotKind(the->stack, XS_UNINITIALIZED_KIND))
#define mxPushUnsigned(THE_NUMBER) \
	(mxOverflow(-1), \
	(--the->stack)->next = C_NULL, \
	(THE_NUMBER < 0x7FFFFFFF) ? \
		(mxInitSlotKind(the->stack, XS_INTEGER_KIND), \
		the->stack->value.integer = (txInteger)(THE_NUMBER)) \
	: \
		(mxInitSlotKind(the->stack, XS_NUMBER_KIND), \
		the->stack->value.number = (txNumber)(THE_NUMBER)) \
	)
#define mxTemporary(_SLOT) \
	(mxOverflow(-1), \
	_SLOT = --the->stack, \
	mxInitSlotKind(the->stack, XS_UNDEFINED_KIND))

#define mxPop() \
	(mxMeterOne(), the->stack++)
#define mxPull(THE_SLOT) \
	(mxMeterOne(), \
	(THE_SLOT).value = the->stack->value, \
	(THE_SLOT).kind = (the->stack++)->kind)
#define mxPullSlot(THE_SLOT) \
	(mxMeterOne(), \
	(THE_SLOT)->value = the->stack->value, \
	(THE_SLOT)->kind = (the->stack++)->kind)



#define mxThis (the->frame + 4)
#define mxFunction (the->frame + 3)
#define mxTarget (the->frame + 2)
#define mxResult (the->frame + 1)
#define mxArgc ((the->frame - 1)->value.integer)
#define mxArgv(THE_INDEX) (the->frame - 2 - (THE_INDEX))
#define mxVarc (the->scope->value.environment.variable.count)
#define mxVarv(THE_INDEX) (the->scope - 1 - (THE_INDEX))

#define mxFrameToEnvironment(FRAME) ((FRAME) - 1 - ((FRAME) - 1)->value.integer - 1)

#define mxFunctionInstanceCode(INSTANCE) 		((INSTANCE)->next)
#define mxFunctionInstanceHome(INSTANCE) 		((INSTANCE)->next->next)

#define mxRealmGlobal(REALM)			((REALM)->next)
#define mxRealmClosures(REALM)			((REALM)->next->next)
#define mxRealmTemplateCache(REALM)		((REALM)->next->next->next)
#define mxOwnModules(REALM)				((REALM)->next->next->next->next)
#define mxResolveHook(REALM)			((REALM)->next->next->next->next->next)
#define mxModuleMap(REALM)				((REALM)->next->next->next->next->next->next)
#define mxModuleMapHook(REALM)			((REALM)->next->next->next->next->next->next->next)
#define mxLoadHook(REALM)				((REALM)->next->next->next->next->next->next->next->next)
#define mxLoadNowHook(REALM)			((REALM)->next->next->next->next->next->next->next->next->next)
#define mxImportMetaHook(REALM)			((REALM)->next->next->next->next->next->next->next->next->next->next)
#define mxRealmParent(REALM)			((REALM)->next->next->next->next->next->next->next->next->next->next->next)

#define mxModuleInstanceInternal(MODULE)		((MODULE)->next)
#define mxModuleInstanceExports(MODULE)		((MODULE)->next->next)
#define mxModuleInstanceMeta(MODULE)			((MODULE)->next->next->next)
#define mxModuleInstanceTransfers(MODULE)		((MODULE)->next->next->next->next)
#define mxModuleInstanceInitialize(MODULE)		((MODULE)->next->next->next->next->next)
#define mxModuleInstanceExecute(MODULE)		((MODULE)->next->next->next->next->next->next)
#define mxModuleInstanceHosts(MODULE)			((MODULE)->next->next->next->next->next->next->next)
#define mxModuleInstanceLoader(MODULE)			((MODULE)->next->next->next->next->next->next->next->next)
#define mxModuleInstanceFulfill(MODULE)		((MODULE)->next->next->next->next->next->next->next->next->next)
#define mxModuleInstanceReject(MODULE)			((MODULE)->next->next->next->next->next->next->next->next->next->next)

#define mxModuleInternal(MODULE) 	mxModuleInstanceInternal((MODULE)->value.reference)
#define mxModuleExports(MODULE) 	mxModuleInstanceExports((MODULE)->value.reference)
#define mxModuleMeta(MODULE) 		mxModuleInstanceMeta((MODULE)->value.reference)
#define mxModuleTransfers(MODULE) 	mxModuleInstanceTransfers((MODULE)->value.reference)
#define mxModuleInitialize(MODULE) 	mxModuleInstanceInitialize((MODULE)->value.reference)
#define mxModuleExecute(MODULE) 	mxModuleInstanceExecute((MODULE)->value.reference)
#define mxModuleHosts(MODULE) 		mxModuleInstanceHosts((MODULE)->value.reference)
#define mxModuleLoader(MODULE) 		mxModuleInstanceLoader((MODULE)->value.reference)
#define mxModuleFulfill(MODULE) 	mxModuleInstanceFulfill((MODULE)->value.reference)
#define mxModuleReject(MODULE) 		mxModuleInstanceReject((MODULE)->value.reference)

#define mxTransferLocal(TRANSFER)	(TRANSFER)->value.reference->next
#define mxTransferFrom(TRANSFER) 	(TRANSFER)->value.reference->next->next
#define mxTransferImport(TRANSFER) 	(TRANSFER)->value.reference->next->next->next
#define mxTransferAliases(TRANSFER)	(TRANSFER)->value.reference->next->next->next->next
#define mxTransferClosure(TRANSFER)	(TRANSFER)->value.reference->next->next->next->next->next

#define mxPromiseStatus(INSTANCE) ((INSTANCE)->next)
#define mxPromiseThens(INSTANCE) ((INSTANCE)->next->next)
#define mxPromiseResult(INSTANCE) ((INSTANCE)->next->next->next)
#define mxPromiseEnvironment(INSTANCE) ((INSTANCE)->next->next->next->next)

enum {
	mxUndefinedStatus,
	mxPendingStatus,
	mxFulfilledStatus,
	mxRejectedStatus
};

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

enum {
	mxGlobalStackIndex,
	mxExceptionStackIndex,
	mxProgramStackIndex,
	mxHostsStackIndex,
	mxModuleQueueStackIndex,

	mxUnhandledPromisesStackIndex,
	mxDuringJobsStackIndex,
	mxFinalizationRegistriesStackIndex,
	mxPendingJobsStackIndex,
	mxRunningJobsStackIndex,
	mxBreakpointsStackIndex,
	mxHostInspectorsStackIndex,
	mxInstanceInspectorsStackIndex,

	mxObjectPrototypeStackIndex = XS_INTRINSICS_COUNT,
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
	mxAggregateErrorPrototypeStackIndex,
	
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
	mxBigIntPrototypeStackIndex,
	mxCompartmentPrototypeStackIndex,
	mxModuleSourcePrototypeStackIndex,
	mxWeakRefPrototypeStackIndex,
	mxFinalizationRegistryPrototypeStackIndex,

	mxEnumeratorFunctionStackIndex,
	mxAssignObjectFunctionStackIndex,
	mxCopyObjectFunctionStackIndex,
	
	mxAsyncFunctionPrototypeStackIndex,
	mxGeneratorPrototypeStackIndex,
	mxGeneratorFunctionPrototypeStackIndex,
	mxModulePrototypeStackIndex,
	mxTransferPrototypeStackIndex,
	mxOnRejectedPromiseFunctionStackIndex,
	mxOnResolvedPromiseFunctionStackIndex,
	mxOnThenableFunctionStackIndex,
	mxArrayLengthAccessorStackIndex,
	mxModuleAccessorStackIndex,
	mxProxyAccessorStackIndex,
	mxStringAccessorStackIndex,
	mxTypedArrayAccessorStackIndex,
	
	mxIteratorPrototypeStackIndex,
	mxArrayIteratorPrototypeStackIndex,
	mxMapIteratorPrototypeStackIndex,
	mxRegExpStringIteratorPrototypeStackIndex,
	mxSetIteratorPrototypeStackIndex,
	mxStringIteratorPrototypeStackIndex,
	
	mxAsyncIteratorPrototypeStackIndex,
	mxAsyncFromSyncIteratorPrototypeStackIndex,
	mxAsyncGeneratorPrototypeStackIndex,
	mxAsyncGeneratorFunctionPrototypeStackIndex,
	
	mxArgumentsSloppyPrototypeStackIndex,
	mxArgumentsStrictPrototypeStackIndex,
	mxThrowTypeErrorFunctionStackIndex,

	mxHookInstanceIndex,
	
	mxExecuteRegExpFunctionIndex,
	mxInitializeRegExpFunctionIndex,
	mxArrayIteratorFunctionIndex,
	mxOrdinaryToPrimitiveFunctionStackIndex,
	mxCompartmentGlobalStackIndex,

	mxEmptyCodeStackIndex,
	mxEmptyStringStackIndex,
	mxEmptyRegExpStackIndex,
	mxBigIntStringStackIndex,
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
#define mxProgram the->stackTop[-1 - mxProgramStackIndex]
#define mxHosts the->stackTop[-1 - mxHostsStackIndex]
#define mxModuleQueue the->stackTop[-1 - mxModuleQueueStackIndex]
#define mxUnhandledPromises the->stackTop[-1 - mxUnhandledPromisesStackIndex]
#define mxDuringJobs the->stackTop[-1 - mxDuringJobsStackIndex]
#define mxFinalizationRegistries the->stackTop[-1 - mxFinalizationRegistriesStackIndex]
#define mxPendingJobs the->stackTop[-1 - mxPendingJobsStackIndex]
#define mxRunningJobs the->stackTop[-1 - mxRunningJobsStackIndex]
#define mxBreakpoints the->stackTop[-1 - mxBreakpointsStackIndex]
#define mxHostInspectors the->stackTop[-1 - mxHostInspectorsStackIndex]
#define mxInstanceInspectors the->stackTop[-1 - mxInstanceInspectorsStackIndex]

#define mxAggregateErrorConstructor the->stackPrototypes[-1 - _AggregateError]
#define mxArrayConstructor the->stackPrototypes[-1 - _Array]
#define mxArrayBufferConstructor the->stackPrototypes[-1 - _ArrayBuffer]
#define mxAtomicsObject the->stackPrototypes[-1 - _Atomics]
#define mxBigIntConstructor the->stackPrototypes[-1 - _BigInt]
#define mxBigInt64ArrayConstructor the->stackPrototypes[-1 - _BigInt64Array]
#define mxBigUint64ArrayConstructor the->stackPrototypes[-1 - _BigUint64Array]
#define mxBooleanConstructor the->stackPrototypes[-1 - _Boolean]
#define mxCompartmentConstructor the->stackPrototypes[-1 - _Compartment]
#define mxDataViewConstructor the->stackPrototypes[-1 - _DataView]
#define mxDateConstructor the->stackPrototypes[-1 - _Date]
#define mxErrorConstructor the->stackPrototypes[-1 - _Error]
#define mxEvalErrorConstructor the->stackPrototypes[-1 - _EvalError]
#define mxFinalizationRegistryConstructor the->stackPrototypes[-1 - _FinalizationRegistry]
#define mxFloat32ArrayConstructor the->stackPrototypes[-1 - _Float32Array]
#define mxFloat64ArrayConstructor the->stackPrototypes[-1 - _Float64Array]
#define mxFunctionConstructor the->stackPrototypes[-1 - _Function]
#define mxInfinity the->stackPrototypes[-1 - _Infinity]
#define mxInt16ArrayConstructor the->stackPrototypes[-1 - _Int16Array]
#define mxInt32ArrayConstructor the->stackPrototypes[-1 - _Int32Array]
#define mxInt8ArrayConstructor the->stackPrototypes[-1 - _Int8Array]
#define mxJSONObject the->stackPrototypes[-1 - _JSON]
#define mxMapConstructor the->stackPrototypes[-1 - _Map]
#define mxMathObject the->stackPrototypes[-1 - _Math]
#define mxNaN the->stackPrototypes[-1 - _NaN]
#define mxNumberConstructor the->stackPrototypes[-1 - _Number]
#define mxObjectConstructor the->stackPrototypes[-1 - _Object]
#define mxPromiseConstructor the->stackPrototypes[-1 - _Promise]
#define mxProxyConstructor the->stackPrototypes[-1 - _Proxy]
#define mxRangeErrorConstructor the->stackPrototypes[-1 - _RangeError]
#define mxReferenceErrorConstructor the->stackPrototypes[-1 - _ReferenceError]
#define mxReflectObject the->stackPrototypes[-1 - _Reflect]
#define mxRegExpConstructor the->stackPrototypes[-1 - _RegExp]
#define mxSetConstructor the->stackPrototypes[-1 - _Set]
#define mxSharedArrayBufferConstructor the->stackPrototypes[-1 - _SharedArrayBuffer]
#define mxModuleSourceConstructor the->stackPrototypes[-1 - _ModuleSource]
#define mxStringConstructor the->stackPrototypes[-1 - _String]
#define mxSymbolConstructor the->stackPrototypes[-1 - _Symbol]
#define mxSyntaxErrorConstructor the->stackPrototypes[-1 - _SyntaxError]
#define mxTypeErrorConstructor the->stackPrototypes[-1 - _TypeError]
#define mxTypedArrayConstructor the->stackPrototypes[-1 - _TypedArray]
#define mxURIErrorConstructor the->stackPrototypes[-1 - _URIError]
#define mxUint16ArrayConstructor the->stackPrototypes[-1 - _Uint16Array]
#define mxUint32ArrayConstructor the->stackPrototypes[-1 - _Uint32Array]
#define mxUint8ArrayConstructor the->stackPrototypes[-1 - _Uint8Array]
#define mxUint8ClampedArrayConstructor the->stackPrototypes[-1 - _Uint8ClampedArray]
#define mxWeakMapConstructor the->stackPrototypes[-1 - _WeakMap]
#define mxWeakRefConstructor the->stackPrototypes[-1 - _WeakRef]
#define mxWeakSetConstructor the->stackPrototypes[-1 - _WeakSet]
#define mxDecodeURIFunction the->stackPrototypes[-1 - _decodeURI]
#define mxDecodeURIComponentFunction the->stackPrototypes[-1 - _decodeURIComponent]
#define mxEncodeURIFunction the->stackPrototypes[-1 - _encodeURI]
#define mxEncodeURIComponentFunction the->stackPrototypes[-1 - _encodeURIComponent]
#define mxEscapeFunction the->stackPrototypes[-1 - _escape]
#define mxEvalFunction the->stackPrototypes[-1 - _eval]
#define mxIsFiniteFunction the->stackPrototypes[-1 - _isFinite]
#define mxIsNaNFunction the->stackPrototypes[-1 - _isNaN]
#define mxParseFloatFunction the->stackPrototypes[-1 - _parseFloat]
#define mxParseIntFunction the->stackPrototypes[-1 - _parseInt]
#define mxTraceFunction the->stackPrototypes[-1 - _trace]
#define mxUndefined the->stackPrototypes[-1 - _undefined]
#define mxUnescapeFunction the->stackPrototypes[-1 - _unescape]

#define mxObjectPrototype the->stackPrototypes[-1 - mxObjectPrototypeStackIndex]
#define mxFunctionPrototype the->stackPrototypes[-1 - mxFunctionPrototypeStackIndex]
#define mxArrayPrototype the->stackPrototypes[-1 - mxArrayPrototypeStackIndex]
#define mxStringPrototype the->stackPrototypes[-1 - mxStringPrototypeStackIndex]
#define mxBooleanPrototype the->stackPrototypes[-1 - mxBooleanPrototypeStackIndex]
#define mxNumberPrototype the->stackPrototypes[-1 - mxNumberPrototypeStackIndex]
#define mxDatePrototype the->stackPrototypes[-1 - mxDatePrototypeStackIndex]
#define mxRegExpPrototype the->stackPrototypes[-1 - mxRegExpPrototypeStackIndex]
#define mxHostPrototype the->stackPrototypes[-1 - mxHostPrototypeStackIndex]

#define mxErrorPrototypes(THE_ERROR) (the->stackPrototypes[-mxErrorPrototypeStackIndex-(THE_ERROR)])
#define mxErrorPrototype the->stackPrototypes[-1 - mxErrorPrototypeStackIndex]
#define mxEvalErrorPrototype the->stackPrototypes[-1 - mxEvalErrorPrototypeStackIndex]
#define mxRangeErrorPrototype the->stackPrototypes[-1 - mxRangeErrorPrototypeStackIndex]
#define mxReferenceErrorPrototype the->stackPrototypes[-1 - mxReferenceErrorPrototypeStackIndex]
#define mxSyntaxErrorPrototype the->stackPrototypes[-1 - mxSyntaxErrorPrototypeStackIndex]
#define mxTypeErrorPrototype the->stackPrototypes[-1 - mxTypeErrorPrototypeStackIndex]
#define mxURIErrorPrototype the->stackPrototypes[-1 - mxURIErrorPrototypeStackIndex]
#define mxAggregateErrorPrototype the->stackPrototypes[-1 - mxAggregateErrorPrototypeStackIndex]

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
#define mxBigIntPrototype the->stackPrototypes[-1 - mxBigIntPrototypeStackIndex]
#define mxCompartmentPrototype the->stackPrototypes[-1 - mxCompartmentPrototypeStackIndex]
#define mxModuleSourcePrototype the->stackPrototypes[-1 - mxModuleSourcePrototypeStackIndex]
#define mxWeakRefPrototype the->stackPrototypes[-1 - mxWeakRefPrototypeStackIndex]
#define mxFinalizationRegistryPrototype the->stackPrototypes[-1 - mxFinalizationRegistryPrototypeStackIndex]

#define mxEmptyCode the->stackPrototypes[-1 - mxEmptyCodeStackIndex]
#define mxEmptyString the->stackPrototypes[-1 - mxEmptyStringStackIndex]
#define mxEmptyRegExp the->stackPrototypes[-1 - mxEmptyRegExpStackIndex]
#define mxBigIntString the->stackPrototypes[-1 - mxBigIntStringStackIndex]
#define mxBooleanString the->stackPrototypes[-1 - mxBooleanStringStackIndex]
#define mxDefaultString the->stackPrototypes[-1 - mxDefaultStringStackIndex]
#define mxFunctionString the->stackPrototypes[-1 - mxFunctionStringStackIndex]
#define mxNumberString the->stackPrototypes[-1 - mxNumberStringStackIndex]
#define mxObjectString the->stackPrototypes[-1 - mxObjectStringStackIndex]
#define mxStringString the->stackPrototypes[-1 - mxStringStringStackIndex]
#define mxSymbolString the->stackPrototypes[-1 - mxSymbolStringStackIndex]
#define mxUndefinedString the->stackPrototypes[-1 - mxUndefinedStringStackIndex]

#define mxEnumeratorFunction the->stackPrototypes[-1 - mxEnumeratorFunctionStackIndex]
#define mxAssignObjectFunction the->stackPrototypes[-1 - mxAssignObjectFunctionStackIndex]
#define mxCopyObjectFunction the->stackPrototypes[-1 - mxCopyObjectFunctionStackIndex]

#define mxAsyncFunctionPrototype the->stackPrototypes[-1 - mxAsyncFunctionPrototypeStackIndex]
#define mxGeneratorPrototype the->stackPrototypes[-1 - mxGeneratorPrototypeStackIndex]
#define mxGeneratorFunctionPrototype the->stackPrototypes[-1 - mxGeneratorFunctionPrototypeStackIndex]
#define mxModulePrototype the->stackPrototypes[-1 - mxModulePrototypeStackIndex]
#define mxTransferPrototype the->stackPrototypes[-1 - mxTransferPrototypeStackIndex]
#define mxOnRejectedPromiseFunction the->stackPrototypes[-1 - mxOnRejectedPromiseFunctionStackIndex]
#define mxOnResolvedPromiseFunction the->stackPrototypes[-1 - mxOnResolvedPromiseFunctionStackIndex]
#define mxOnThenableFunction the->stackPrototypes[-1 - mxOnThenableFunctionStackIndex]
#define mxArrayLengthAccessor the->stackPrototypes[-1 - mxArrayLengthAccessorStackIndex]
#define mxModuleAccessor the->stackPrototypes[-1 - mxModuleAccessorStackIndex]
#define mxProxyAccessor the->stackPrototypes[-1 - mxProxyAccessorStackIndex]
#define mxStringAccessor the->stackPrototypes[-1 - mxStringAccessorStackIndex]
#define mxTypedArrayAccessor the->stackPrototypes[-1 - mxTypedArrayAccessorStackIndex]

#define mxIteratorPrototype the->stackPrototypes[-1 - mxIteratorPrototypeStackIndex]
#define mxArrayIteratorPrototype the->stackPrototypes[-1 - mxArrayIteratorPrototypeStackIndex]
#define mxMapIteratorPrototype the->stackPrototypes[-1 - mxMapIteratorPrototypeStackIndex]
#define mxRegExpStringIteratorPrototype the->stackPrototypes[-1 - mxRegExpStringIteratorPrototypeStackIndex]
#define mxSetIteratorPrototype the->stackPrototypes[-1 - mxSetIteratorPrototypeStackIndex]
#define mxStringIteratorPrototype the->stackPrototypes[-1 - mxStringIteratorPrototypeStackIndex]

#define mxAsyncIteratorPrototype the->stackPrototypes[-1 - mxAsyncIteratorPrototypeStackIndex]
#define mxAsyncFromSyncIteratorPrototype the->stackPrototypes[-1 - mxAsyncFromSyncIteratorPrototypeStackIndex]
#define mxAsyncGeneratorPrototype the->stackPrototypes[-1 - mxAsyncGeneratorPrototypeStackIndex]
#define mxAsyncGeneratorFunctionPrototype the->stackPrototypes[-1 - mxAsyncGeneratorFunctionPrototypeStackIndex]

#define mxArgumentsSloppyPrototype the->stackPrototypes[-1 - mxArgumentsSloppyPrototypeStackIndex]
#define mxArgumentsStrictPrototype the->stackPrototypes[-1 - mxArgumentsStrictPrototypeStackIndex]
#define mxThrowTypeErrorFunction the->stackPrototypes[-1 - mxThrowTypeErrorFunctionStackIndex]

#define mxHookInstance the->stackPrototypes[-1 - mxHookInstanceIndex]
#define  mxExecuteRegExpFunction the->stackPrototypes[-1 - mxExecuteRegExpFunctionIndex]
#define  mxInitializeRegExpFunction the->stackPrototypes[-1 - mxInitializeRegExpFunctionIndex]
#define  mxArrayIteratorFunction the->stackPrototypes[-1 - mxArrayIteratorFunctionIndex]
#define mxOrdinaryToPrimitiveFunction the->stackPrototypes[-1 - mxOrdinaryToPrimitiveFunctionStackIndex]
#define mxCompartmentGlobal the->stackPrototypes[-1 - mxCompartmentGlobalStackIndex]

#define mxID(ID) ((txID)(ID))

#ifdef mxLink
extern txCallback fxNewLinkerCallback(txMachine*, txCallback, txString);
#define mxCallback(CALLBACK) fxNewLinkerCallback(the, CALLBACK, #CALLBACK)
extern txSlot* fxBuildHostConstructor(txMachine* the, txCallback theCallback, txInteger theLength, txInteger name);
extern txSlot* fxBuildHostFunction(txMachine* the, txCallback theCallback, txInteger theLength, txInteger name);
#else
#define mxCallback(CALLBACK) CALLBACK
#define fxBuildHostConstructor(THE, CALLBACK, LENGTH, NAME) fxNewHostConstructor(THE, CALLBACK, LENGTH, NAME)
#define fxBuildHostFunction(THE, CALLBACK, LENGTH, NAME) fxNewHostFunction(THE, CALLBACK, LENGTH, NAME, XS_NO_ID)
#endif

enum {
	mxHostProfileID,
	mxGarbageCollectorProfileID,
	mx_Promise_prototype_finallyAuxProfileID,
	mx_Promise_prototype_finallyReturnProfileID,
	mx_Promise_prototype_finallyThrowProfileID,
	mx_Proxy_revokeProfileID,
	mxAsyncGeneratorRejectAwaitProfileID,
	mxAsyncGeneratorRejectYieldProfileID,
	mxAsyncGeneratorResolveAwaitProfileID,
	mxAsyncGeneratorResolveYieldProfileID,
	mxAsyncFromSyncIteratorDoneProfileID,
	mxCombinePromisesCallbackProfileID,
	mxExecuteModulesFulfilledProfileID,
	mxExecuteModulesRejectedProfileID,
	mxExecuteVirtualModuleSourceProfileID,
	mxExecuteVirtualModuleSourceImportProfileID,
	mxLoadModulesFulfilledProfileID,
	mxLoadModulesRejectedProfileID,
	mxNewPromiseCapabilityCallbackProfileID,
	mxRejectAwaitProfileID,
	mxRejectPromiseProfileID,
	mxResolveAwaitProfileID,
	mxResolvePromiseProfileID,
	mxBaseProfileID
};

#ifdef __cplusplus
}
#endif

#endif /* __XSALL__ */

/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

#define xsBuildKeys(THE) fxBuildKeys(THE)
#define xsFindModule(THE,ID,NAME) fxFindModule(THE,ID,NAME)
#define xsLoadModule(THE,ID) fxLoadModule(THE,ID)
#define xsQueuePromiseJobs(THE) fxQueuePromiseJobs(THE)
#define xsParseScript(THE,STREAM,GETTER,FLAGS) fxParseScript(THE,STREAM,GETTER,FLAGS)

#include "xsAll.h"
#include "xs.h"

#define XS_ATOM_INCREMENTAL 0x58535F49 /* 'XS_I' */

typedef struct sxLinker txLinker;
typedef struct sxLinkerBuilder txLinkerBuilder;
typedef struct sxLinkerCallback txLinkerCallback;
typedef struct sxLinkerChunk txLinkerChunk;
typedef struct sxLinkerInclude txLinkerInclude;
typedef struct sxLinkerPreload txLinkerPreload;
typedef struct sxLinkerProjection txLinkerProjection;
typedef struct sxLinkerResource txLinkerResource;
typedef struct sxLinkerScript txLinkerScript;
typedef struct sxLinkerStrip txLinkerStrip;
typedef struct sxLinkerSymbol txLinkerSymbol;

typedef struct {
	txCallback callback;
	txString name;
	txFlag flag;
} txCallbackName;

struct sxLinker {
	c_jmp_buf jmp_buf;
	void* dtoa;
	int error;
	int twice;
	txLinkerChunk* firstChunk;
	
	txLinkerCallback* firstCallback;
	txLinkerBuilder* firstBuilder;
	txLinkerBuilder* lastBuilder;
	txInteger builderCount;
	
	txLinkerInclude* firstInclude;
	txLinkerPreload* firstPreload;
	txLinkerProjection* firstProjection;
	
	txLinkerResource* firstResource;
	
	txLinkerScript* firstScript;
	txID scriptCount;
	txID hostsCount;
	
	txLinkerStrip* firstStrip;
	
	txSize symbolModulo;
	txLinkerSymbol** symbolTable;
	txID symbolCount;
	txID symbolIndex;
	txLinkerSymbol** symbolArray;
	
	txS1* symbolsBuffer;
	txU1 symbolsChecksum[16];
	txSize symbolsSize;
	
	txString base;
	txSize baseLength;
	
	txCreation creation;
	
	txFlag intrinsicFlags[mxIntrinsicCount];

	char main[1024];
};

struct sxLinkerBuilder {
	txLinkerBuilder* nextBuilder;
	txInteger builderIndex;
	txHostFunctionBuilder host;
};

struct sxLinkerCallback {
	txLinkerCallback* nextCallback;
	txCallback callback;
	txString name;
	txFlag flag;
};

struct sxLinkerChunk {
	txLinkerChunk* nextChunk;
};

struct sxLinkerInclude {
	txLinkerInclude* nextInclude;
	txString path;
};

struct sxLinkerPreload {
	txLinkerPreload* nextPreload;
	txString name;
};

struct sxLinkerProjection {
	txLinkerProjection* nextProjection;
	txSlot* heap;
	txSlot* limit;
	txInteger indexes[1];
};

struct sxLinkerResource {
	txLinkerResource* nextResource;
	txS1* dataBuffer;
	txSize dataSize;
	txString path;
	txSize pathSize;
	txSize padSize;
};

struct sxLinkerScript {
	txLinkerScript* nextScript;
	txS1* codeBuffer;
	txSize codeSize;
	txS1* hostsBuffer;
	txSize hostsSize;
	txS1* symbolsBuffer;
	txSize symbolsSize;
	txString path;
	txSize pathSize;
	txID scriptIndex;
	txHostFunctionBuilder* builders;
	txCallbackName* callbackNames;
	txSize hostsCount;
};

struct sxLinkerStrip {
	txLinkerStrip* nextStrip;
	txString name;
};

struct sxLinkerSymbol {
	txLinkerSymbol* next;
	txID ID;
	txInteger length;
	txString string;
	txSize sum;
	txFlag flag;
};

/* xslBase.c */
extern void fxBaseResource(txLinker* linker, txLinkerResource* resource, txString base, txInteger baseLength);
extern void fxBaseScript(txLinker* linker, txLinkerScript* script, txString base, txInteger baseLength);
extern void fxBufferPaths(txLinker* linker);
extern void fxBufferSymbols(txLinker* linker);
extern void fxDefaultSymbols(txLinker* linker);
extern txLinkerCallback* fxGetLinkerCallbackByAddress(txLinker* linker, txCallback which);
extern txLinkerCallback* fxGetLinkerCallbackByName(txLinker* linker, txString name);
extern void fxInitializeLinker(txLinker* linker);
extern txBoolean fxIsCodeUsed(txU1 code);
extern void fxMapScript(txLinker* linker, txLinkerScript* script);
extern txHostFunctionBuilder* fxNewLinkerBuilder(txLinker* linker, txCallback callback, txInteger length, txID id);
extern void* fxNewLinkerChunk(txLinker* linker, txSize size);
extern void* fxNewLinkerChunkClear(txLinker* linker, txSize size);
extern txLinkerInclude* fxNewLinkerInclude(txLinker* linker, txString path);
extern txLinkerPreload* fxNewLinkerPreload(txLinker* linker, txString name);
extern txLinkerResource* fxNewLinkerResource(txLinker* linker, txString path, FILE** fileAddress);
extern txLinkerScript* fxNewLinkerScript(txLinker* linker, txString path, FILE** fileAddress);
extern txLinkerStrip* fxNewLinkerStrip(txLinker* linker, txString name);
extern void fxReadSymbols(txLinker* linker, txString path, txFlag flag, FILE** fileAddress);
extern txString fxRealDirectoryPath(txLinker* linker, txString path);
extern txString fxRealFilePath(txLinker* linker, txString path);
extern void fxReportLinkerError(txLinker* linker, txString theFormat, ...);
extern void fxSlashPath(txString path, char from, char to);
extern void fxTerminateLinker(txLinker* linker);
extern void fxWriteArchive(txLinker* linker, txString path, FILE** fileAddress);
extern void fxWriteCString(FILE* file, txString string);
extern void fxWriteDefines(txLinker* linker, FILE* file);
extern void fxWriteScriptCode(txLinkerScript* script, FILE* file);
extern void fxWriteScriptExterns(txLinkerScript* script, FILE* file);
extern void fxWriteScriptHosts(txLinkerScript* script, FILE* file);
extern void fxWriteScriptRecord(txLinkerScript* script, FILE* file);
extern void fxWriteSymbols(txLinker* linker, txString path, FILE** fileAddress);
extern void fxWriteStrips(txLinker* linker, FILE* file);

/* xslSlot.c */
extern void fxLinkerScriptCallback(txMachine* the);
extern txInteger fxPrepareHeap(txMachine* the);
extern void fxPrintBuilders(txMachine* the, FILE* file);
extern void fxPrintHeap(txMachine* the, FILE* file, txInteger count, txBoolean dummy);
extern void fxPrintStack(txMachine* the, FILE* file, txBoolean dummy);
extern void fxPrintTable(txMachine* the, FILE* file, txSize modulo, txSlot** table);

/* xslStrip.c */
extern void fxReferenceLinkerSymbol(txLinker* linker, txID id);
extern void fxStripCallbacks(txLinker* linker, txMachine* the);
extern void fxStripDefaults(txLinker* linker, FILE* file);
extern void fxStripName(txLinker* linker, txString name);
extern void fxUnstripCallbacks(txLinker* linker);

#define mxAssert(_ASSERTION) { if (!(_ASSERTION)) { fprintf(stderr, "### '%s': invalid file\n", path); linker->error = C_EINVAL; c_longjmp(linker->jmp_buf, 1); } }
#define mxThrowElse(_ASSERTION) { if (!(_ASSERTION)) { linker->error = errno; c_longjmp(linker->jmp_buf, 1); } }


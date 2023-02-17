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

#include "xsl.h"

#define MD5_NUMSTATE 4
#define MD5_DGSTSIZE (MD5_NUMSTATE * 4)
#define MD5_BLKSIZE	64

typedef struct sxMD5 txMD5;

struct sxMD5 {
	uint64_t len;
	uint32_t state[MD5_NUMSTATE];
	uint8_t buf[MD5_BLKSIZE];
};

static txString fxBasePrefix(txLinker* linker, txString path);
static void fxMapCode(txLinker* linker, txLinkerScript* script, txID* theIDs);
static void fxMapHosts(txLinker* linker, txLinkerScript* script, txID* theIDs);
static txID* fxMapSymbols(txLinker* linker, txS1* symbolsBuffer, txFlag flag);
static void fxReferenceLinkerSymbol(txLinker* linker, txID id);
static void fxRemapCode(txLinker* linker, txLinkerScript* script);
static void md5_create(txMD5 *s);
static void md5_update(txMD5 *s, const void *data, uint32_t size);
static void md5_fin(txMD5 *s, uint8_t *dgst);

static txInteger gxCodeUsages[XS_CODE_COUNT];

static txCallback gxTypeCallbacks[1 + mxTypeArrayCount] = {
	fx_TypedArray,
	fx_BigInt64Array,
	fx_BigUint64Array,
	fx_Float32Array,
	fx_Float64Array,
	fx_Int8Array,
	fx_Int16Array,
	fx_Int32Array,
	fx_Uint8Array,
	fx_Uint16Array,
	fx_Uint32Array,
	fx_Uint8ClampedArray,
};
static int gxTypeCallbacksIndex = 0;

void fx_BigInt64Array(txMachine* the) { fx_TypedArray(the); }
void fx_BigUint64Array(txMachine* the) { fx_TypedArray(the); }
void fx_Float32Array(txMachine* the) { fx_TypedArray(the); }
void fx_Float64Array(txMachine* the) { fx_TypedArray(the); }
void fx_Int8Array(txMachine* the) { fx_TypedArray(the); }
void fx_Int16Array(txMachine* the) { fx_TypedArray(the); }
void fx_Int32Array(txMachine* the) { fx_TypedArray(the); }
void fx_Uint8Array(txMachine* the) { fx_TypedArray(the); }
void fx_Uint16Array(txMachine* the) { fx_TypedArray(the); }
void fx_Uint32Array(txMachine* the) { fx_TypedArray(the); }
void fx_Uint8ClampedArray(txMachine* the) { fx_TypedArray(the); }

txString fxBasePrefix(txLinker* linker, txString path)
{
	if ((path[0] == '~') && (path[1] == '.')) {
		txString slash = c_strchr(path + 2, mxSeparator);
		if (slash) {
			*slash = ':';
			path += 2;
		}
	}
	return path;
}

void fxBaseResource(txLinker* linker, txLinkerResource* resource, txString base, txInteger baseLength)
{
	if (c_strncmp(resource->path, base, baseLength))
		fxReportLinkerError(linker, "'%s': not relative to '%s'", resource->path, base);
	resource->path += baseLength;
	resource->pathSize = mxStringLength(resource->path) + 1;
}

void fxBaseScript(txLinker* linker, txLinkerScript* script, txString base, txInteger baseLength)
{
	if (c_strncmp(script->path, base, baseLength))
		fxReportLinkerError(linker, "'%s': not relative to '%s'", script->path, base);
	script->path = fxBasePrefix(linker, script->path + baseLength);
	script->pathSize = mxStringLength(script->path) - 4;
	script->path[script->pathSize] = 0;
	script->pathSize++;
	script->scriptIndex = linker->scriptCount;
	linker->scriptCount++;
}

void fxBufferMaps(txLinker* linker)
{
	txSize size;
	txLinkerScript* script;
	
	size = linker->mapIndex * sizeof(txID);
	linker->mapsBuffer = fxNewLinkerChunk(linker, size);
	linker->mapsSize = size;
	linker->map = linker->mapsBuffer;
	script = linker->firstScript;
	while (script) {
		fxRemapCode(linker, script);
		script = script->nextScript;
	}
}

void fxBufferSymbols(txLinker* linker)
{
	txByte* p;
	txS2 c, i;
	txLinkerSymbol** address;
	txInteger size;

	c = (txS2)(linker->symbolIndex);
	size = 2;
	address = &linker->symbolArray[0];
	for (i = 0; i < c; i++) {
		size += (*address)->length;
		address++;
	}

	linker->symbolsBuffer = fxNewLinkerChunk(linker, size);
	linker->symbolsSize = size;

	p = linker->symbolsBuffer;
	mxEncode2(p, c);
	address = &(linker->symbolArray[0]);
	for (i = 0; i < c; i++) {
		c_memcpy(p, (*address)->string, (*address)->length);
		p += (*address)->length;
		address++;
	}
}

void fxDefaultSymbols(txLinker* linker)
{
	int i;
	for (i = 0; i < XS_ID_COUNT; i++) {
		fxNewLinkerSymbol(linker, gxIDStrings[i], 0);
	}
}

txLinkerCallback* fxGetLinkerCallbackByAddress(txLinker* linker, txCallback which) 
{
	txLinkerCallback* linkerCallback = linker->firstCallback;
	while (linkerCallback) {
		if (linkerCallback->callback == which)
			return linkerCallback;
		linkerCallback = linkerCallback->nextCallback;
	}
	return NULL;
}	

txLinkerCallback* fxGetLinkerCallbackByName(txLinker* linker, txString name) 
{
	txLinkerCallback* linkerCallback = linker->firstCallback;
	while (linkerCallback) {
		if (!c_strcmp(linkerCallback->name, name))
			return linkerCallback;
		linkerCallback = linkerCallback->nextCallback;
	}
	return NULL;
}	

void fxInitializeLinker(txLinker* linker)
{
	c_memset(gxCodeUsages, 0, sizeof(gxCodeUsages));
	
	c_memset(linker, 0, sizeof(txLinker));
	linker->dtoa = fxNew_dtoa(NULL);
	linker->symbolModulo = 1993;
	linker->symbolCount = 0x10000;
	linker->symbolArray = fxNewLinkerChunkClear(linker, linker->symbolCount * sizeof(txLinkerSymbol*));
	linker->symbolIndex = 0;
	
	linker->creation.initialChunkSize = 32768;
	linker->creation.incrementalChunkSize = 1024;
	linker->creation.initialHeapCount = 2048;
	linker->creation.incrementalHeapCount = 64;
	linker->creation.stackCount = 512;
	linker->creation.initialKeyCount = 256;
	linker->creation.incrementalKeyCount = 0;
	linker->creation.nameModulo = 127;
	linker->creation.symbolModulo = 127;
	c_strcpy(linker->main, "main");
}

txBoolean fxIsCIdentifier(txLinker* linker, txString string)
{
	static char gxIdentifierSet[128] = {
	  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
		 0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
		 1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
		 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
		 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
		 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
		 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0 	/* 7X  pqrstuvwxyz{|}~   */
	};
	txU1 c;
	while ((c = *((txU1*)string))) {
		if ((c > 127) || (gxIdentifierSet[c] == 0))
			return 0;
		string++;
	}
	return 1;
}

txBoolean fxIsCodeUsed(txU1 code)
{
	return (gxCodeUsages[code] > 0) ? 1 : 0;
}

void fxMapCode(txLinker* linker, txLinkerScript* script, txID* theIDs)
{
	register const txS1* sizes = gxCodeSizes;
	register txByte* p = script->codeBuffer;
	register txByte* q = p + script->codeSize;
	register txS1 offset;
	txU1 code;
	txU2 index;
	txID id;
	while (p < q) {
		code = *((txU1*)p);
		if (script->preload == C_NULL) {
			if (XS_CODE_CODE_1 == code)
				*((txU1*)p) = code = XS_CODE_CODE_ARCHIVE_1;
			else if (XS_CODE_CODE_2 == code)
				*((txU1*)p) = code = XS_CODE_CODE_ARCHIVE_2;
			else if (XS_CODE_CODE_4 == code)
				*((txU1*)p) = code = XS_CODE_CODE_ARCHIVE_4;
			else if (XS_CODE_STRING_1 == code)
				*((txU1*)p) = code = XS_CODE_STRING_ARCHIVE_1;
			else if (XS_CODE_STRING_2 == code)
				*((txU1*)p) = code = XS_CODE_STRING_ARCHIVE_2;
			else if (XS_CODE_STRING_4 == code)
				*((txU1*)p) = code = XS_CODE_STRING_ARCHIVE_4;
		}
		gxCodeUsages[code]++;	
		offset = (txS1)sizes[code];
		if (0 < offset) {
			if (linker->profileID && (XS_CODE_PROFILE == code)) {
				p++;
				id = linker->profileID;
				linker->profileID++;
				mxEncodeID(p, id);
			}
			else
				p += offset;
		}
		else if (0 == offset) {
			p++;
			mxDecodeID(p, id);
			id = theIDs[id];
			p -= sizeof(txID);
			mxEncodeID(p, id);
			linker->mapIndex++;
			if ((XS_CODE_GET_PROPERTY == code) || (XS_CODE_GET_SUPER == code) || (XS_CODE_GET_THIS_VARIABLE == code) || (XS_CODE_GET_VARIABLE == code))
				fxReferenceLinkerSymbol(linker, id);
		}
		else if (-1 == offset) {
			p++;
			index = *((txU1*)p);
			p += 1 + index;
		}
		else if (-2 == offset) {
			p++;
			mxDecode2(p, index);
			p += index;
		}
		else if (-4 == offset) {
			p++;
			mxDecode4(p, index);
			p += index;
		}
	}
}

void fxMapHosts(txLinker* linker, txLinkerScript* script, txID* theIDs)
{
	txByte* p = script->hostsBuffer;
	if (p) {
		txID c, i, id;
		mxDecodeID(p, c);
		for (i = 0; i < c; i++) {
			p++;
			mxDecodeID(p, id);
			if (id != XS_NO_ID) {
				id = theIDs[id];
				p -= sizeof(txID);
				mxEncodeID(p, id);
			}
			p += mxStringLength((char*)p) + 1;
		}
		linker->hostsCount += c;
	}
}

void fxMapScript(txLinker* linker, txLinkerScript* script)
{
	txID* symbols = fxMapSymbols(linker, script->symbolsBuffer, 0);
	fxMapCode(linker, script, symbols);
	fxMapHosts(linker, script, symbols);
}

txID* fxMapSymbols(txLinker* linker, txS1* symbolsBuffer, txFlag flag)
{
	txID* symbols = C_NULL;
	txByte* p = symbolsBuffer;
	txID c, i;
	mxDecodeID(p, c);
	symbols = fxNewLinkerChunk(linker, c * sizeof(txID*));
	symbols[0] = XS_NO_ID;
	for (i = 1; i < c; i++) {
		txLinkerSymbol* symbol = fxNewLinkerSymbol(linker, (txString)p, flag);
		symbols[i] = symbol->ID;
		p += mxStringLength((char*)p) + 1;
	}
	return symbols;
}

txHostFunctionBuilder* fxNewLinkerBuilder(txLinker* linker, txCallback callback, txInteger length, txID id)
{
	txLinkerBuilder* result = fxNewLinkerChunkClear(linker, sizeof(txLinkerBuilder));
	result->builderIndex = linker->builderCount;
	linker->builderCount++;
	if (linker->lastBuilder)
		linker->lastBuilder->nextBuilder = result;
	else
		linker->firstBuilder = result;
	linker->lastBuilder = result;
	result->host.callback = callback;
	result->host.length = length;
	result->host.id = id;
	return &(result->host);
}

txCallback fxNewLinkerCallback(txMachine* the, txCallback callback, txString name)
{
	txLinker* linker = (txLinker*)(the->context);
	txLinkerCallback* result = fxNewLinkerChunkClear(linker, sizeof(txLinkerCallback));
	result->nextCallback = linker->firstCallback;
	linker->firstCallback = result;
	if (callback == fx_TypedArray)
		result->callback = gxTypeCallbacks[gxTypeCallbacksIndex++];
	else
		result->callback = callback;
	result->name = name;
	return result->callback;
}

void* fxNewLinkerChunk(txLinker* linker, txSize size)
{
	txLinkerChunk* block = c_malloc(sizeof(txLinkerChunk) + size);
	mxThrowElse(block);
	block->nextChunk = linker->firstChunk;
	linker->firstChunk = block;
	return block + 1;
}

void* fxNewLinkerChunkClear(txLinker* linker, txSize size)
{
	void* result = fxNewLinkerChunk(linker, size);
    c_memset(result, 0, size);
	return result;
}

txLinkerInclude* fxNewLinkerInclude(txLinker* linker, txString path)
{
	txLinkerInclude* result = fxNewLinkerChunkClear(linker, sizeof(txLinkerInclude));
	result->path = fxNewLinkerString(linker, path, mxStringLength(path));
	return result;
}

txLinkerPreload* fxNewLinkerPreload(txLinker* linker, txString name)
{
	txLinkerPreload* result = fxNewLinkerChunkClear(linker, sizeof(txLinkerPreload));
	result->name = fxNewLinkerString(linker, name, mxStringLength(name));
	result->name = fxBasePrefix(linker, result->name);
	name = c_strrchr(result->name, '.');
	if (name && (!c_strcmp(name, ".js") || !c_strcmp(name, ".mjs") || !c_strcmp(name, ".xsb")))
		*name = 0;
	return result;
}

txLinkerResource* fxNewLinkerResource(txLinker* linker, txString path, FILE** fileAddress)
{
	txLinkerResource* result = fxNewLinkerChunkClear(linker, sizeof(txLinkerResource));
	FILE* aFile = NULL;
	result->path = fxNewLinkerString(linker, path, mxStringLength(path));
	aFile = fopen(path, "rb");
	mxThrowElse(aFile);
	*fileAddress = aFile;
	fseek(aFile, 0, SEEK_END);
	result->dataSize = ftell(aFile);
	fseek(aFile, 0, SEEK_SET);
	result->dataBuffer = fxNewLinkerChunk(linker, result->dataSize);;
	mxThrowElse(fread(result->dataBuffer, result->dataSize, 1, aFile) == 1);
	fclose(aFile);
	*fileAddress = NULL;
	return result;
}

txLinkerScript* fxNewLinkerScript(txLinker* linker, txString path, FILE** fileAddress)
{
	txLinkerScript* script = NULL;
	FILE* aFile = NULL;
	Atom atom;
	txByte version[4];
	script = fxNewLinkerChunkClear(linker, sizeof(txLinkerScript));
	script->path = fxNewLinkerString(linker, path, mxStringLength(path));
	aFile = fopen(path, "rb");
	mxThrowElse(aFile);
	*fileAddress = aFile;
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
	atom.atomSize = ntohl(atom.atomSize);
	atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_BINARY);
	
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
	atom.atomSize = ntohl(atom.atomSize);
	atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_VERSION);
	mxThrowElse(fread(version, sizeof(version), 1, aFile) == 1);	
	mxAssert(version[0] == XS_MAJOR_VERSION);
	mxAssert(version[1] == XS_MINOR_VERSION);
	mxAssert(version[3] != 1);
	
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
	atom.atomSize = ntohl(atom.atomSize);
	atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_SYMBOLS);
	script->symbolsSize = atom.atomSize - sizeof(atom);
	script->symbolsBuffer = fxNewLinkerChunk(linker, script->symbolsSize);
	mxThrowElse(fread(script->symbolsBuffer, script->symbolsSize, 1, aFile) == 1);
		
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
    atom.atomSize = ntohl(atom.atomSize);
    atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_CODE);
	script->codeSize = atom.atomSize - sizeof(atom);
	script->codeBuffer = fxNewLinkerChunk(linker, script->codeSize);
	mxThrowElse(fread(script->codeBuffer, script->codeSize, 1, aFile) == 1);
	
	if (version[3] == -1) {
		mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
		atom.atomSize = ntohl(atom.atomSize);
		atom.atomType = ntohl(atom.atomType);
		mxAssert(atom.atomType == XS_ATOM_HOSTS);
		script->hostsSize = atom.atomSize - sizeof(atom);
		script->hostsBuffer = fxNewLinkerChunk(linker, script->hostsSize);
		mxThrowElse(fread(script->hostsBuffer, script->hostsSize, 1, aFile) == 1);	
	}
	
	fclose(aFile);
	*fileAddress = NULL;
	return script;
}

txString fxNewLinkerString(txLinker* linker, txString buffer, txSize size)
{
	txString result = fxNewLinkerChunk(linker, size + 1);
	c_memcpy(result, buffer, size);
	result[size] = 0;
	return result;
}

txLinkerStrip* fxNewLinkerStrip(txLinker* linker, txString name)
{
	txLinkerStrip* result = fxNewLinkerChunkClear(linker, sizeof(txLinkerStrip));
	result->name = fxNewLinkerString(linker, name, mxStringLength(name));
	return result;
}

txLinkerSymbol* fxNewLinkerSymbol(txLinker* linker, txString theString, txFlag flag)
{
	txString aString;
	txSize aLength;
	txSize aSum;
	txSize aModulo;
	txLinkerSymbol* aSymbol;
	txID anID;
	
	aString = theString;
	aLength = 0;
	aSum = 0;
	while(*aString != 0) {
		aLength++;
		aSum = (aSum << 1) + *aString++;
	}
	aSum &= 0x7FFFFFFF;
	aModulo = aSum % linker->symbolModulo;
	aSymbol = linker->symbolTable[aModulo];
	while (aSymbol != C_NULL) {
		if (aSymbol->sum == aSum)
			if (c_strcmp(aSymbol->string, theString) == 0)
				break;
		aSymbol = aSymbol->next;
	}
	if (aSymbol == C_NULL) {
		anID = linker->symbolIndex;
		if (anID == linker->symbolCount) {
			exit(1);
		}
		aSymbol = fxNewLinkerChunk(linker, sizeof(txLinkerSymbol));
		aSymbol->next = linker->symbolTable[aModulo];
		aSymbol->ID = anID;
		aSymbol->length = aLength + 1;
		aSymbol->string = fxNewLinkerString(linker, theString, aLength);
		aSymbol->sum = aSum;
		aSymbol->flag = flag;
		linker->symbolArray[anID] = aSymbol;
		linker->symbolTable[aModulo] = aSymbol;
		linker->symbolIndex++;
	}
	else
		aSymbol->flag |= flag;
	return aSymbol;
}

void fxReadSymbols(txLinker* linker, txString path, txFlag flag, FILE** fileAddress)
{
	FILE* aFile = NULL;
	Atom atom;
	txByte version[4];
	txSize symbolsSize;
	txS1* symbolsBuffer;
	aFile = fopen(path, "rb");
	mxThrowElse(aFile);
	*fileAddress = aFile;
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
	atom.atomSize = ntohl(atom.atomSize);
	atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_INCREMENTAL);
	
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
	atom.atomSize = ntohl(atom.atomSize);
	atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_VERSION);
	mxThrowElse(fread(version, sizeof(version), 1, aFile) == 1);	
	mxAssert(version[0] == XS_MAJOR_VERSION);
	mxAssert(version[1] == XS_MINOR_VERSION);
	
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
	atom.atomSize = ntohl(atom.atomSize);
	atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_SYMBOLS);
	symbolsSize = atom.atomSize - sizeof(atom);
	symbolsBuffer = fxNewLinkerChunk(linker, symbolsSize);
	mxThrowElse(fread(symbolsBuffer, symbolsSize, 1, aFile) == 1);
	fxMapSymbols(linker, symbolsBuffer, flag);
	
	fclose(aFile);
	*fileAddress = NULL;
}

txString fxRealDirectoryPath(txLinker* linker, txString path)
{
#if mxWindows
	char buffer[C_PATH_MAX];
	DWORD attributes;
	if (_fullpath(buffer, path, C_PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if ((attributes != 0xFFFFFFFF) && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
  	 		strcat(buffer, "\\");
			return fxNewLinkerString(linker, buffer, mxStringLength(buffer));
		}
	}
#else
	char buffer[C_PATH_MAX];
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(buffer, &a_stat) == 0) {
			if (S_ISDIR(a_stat.st_mode)) {
  	 			strcat(buffer, "/");
				return fxNewLinkerString(linker, buffer, mxStringLength(buffer));
			}
		}
	}
#endif
	return NULL;
}

txString fxRealFilePath(txLinker* linker, txString path)
{
#if mxWindows
	char buffer[C_PATH_MAX];
	DWORD attributes;
	if (_fullpath(buffer, path, C_PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if ((attributes != 0xFFFFFFFF) && (!(attributes & FILE_ATTRIBUTE_DIRECTORY))) {
			return fxNewLinkerString(linker, buffer, mxStringLength(buffer));
		}
	}
#else
	char buffer[C_PATH_MAX];
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(buffer, &a_stat) == 0) {
			if (S_ISREG(a_stat.st_mode)) {
				return fxNewLinkerString(linker, buffer, mxStringLength(buffer));
			}
		}
	}
#endif
	return NULL;
}

void fxReferenceLinkerSymbol(txLinker* linker, txID id)
{
	txLinkerSymbol* linkerSymbol = linker->symbolArray[id];
	linkerSymbol->flag |= 1;
}

void fxRemapCode(txLinker* linker, txLinkerScript* script)
{
	register const txS1* sizes = gxCodeSizes;
	register txByte* p = script->codeBuffer;
	register txByte* q = p + script->codeSize;
	register txByte* r = (txByte*)linker->map;
	register txS1 offset;
	txU1 code;
	txU2 index;
	txID id;
	while (p < q) {
		code = *((txU1*)p);
		offset = (txS1)sizes[code];
		if (0 < offset) {
			p += offset;
		}
		else if (0 == offset) {
			p++;
			mxDecodeID(p, id);
			mxEncodeID(r, id);
			p -= sizeof(txID);
			id = XS_NO_ID;
			mxEncodeID(p, id);
		}
		else if (-1 == offset) {
			p++;
			index = *((txU1*)p);
			p += 1 + index;
		}
		else if (-2 == offset) {
			p++;
			mxDecode2(p, index);
			p += index;
		}
		else if (-4 == offset) {
			p++;
			mxDecode4(p, index);
			p += index;
		}
	}
	linker->map = (txID*)r;
}

void fxReportLinkerError(txLinker* linker, txString theFormat, ...)
{
	c_va_list arguments;
	fprintf(stderr, "### ");
	c_va_start(arguments, theFormat);
	vfprintf(stderr, theFormat, arguments);
	c_va_end(arguments);
	fprintf(stderr, "!\n");
	linker->error = C_EINVAL; 
	c_longjmp(linker->jmp_buf, 1); 
}

void fxSlashPath(txString path, char from, char to)
{
	txString s = path;
	while (*s) {
		if (*s == from)
			*s = to;
		s++;
	}
}

void fxTerminateLinker(txLinker* linker)
{
	txLinkerChunk* block = linker->firstChunk;
	while (block) {
		txLinkerChunk* nextChunk = block->nextChunk;
		c_free(block);
		block = nextChunk;
	}
	if (linker->dtoa)
		fxDelete_dtoa(linker->dtoa);
// 	{
// 		txU1 code;
// 		for (code = 0; code < XS_CODE_COUNT; code++) {
// 			fprintf(stderr, "%s %d\n", gxCodeNames[code], gxCodeUsages[code]);
// 		}
// 	
// 	}
}

void fxUnuseCode(txU1 code)
{
	gxCodeUsages[code] = 0;
}

void fxUseCodes()
{
	c_memset(gxCodeUsages, 1, sizeof(gxCodeUsages));
}

void fxWriteArchive(txLinker* linker, txString path, FILE** fileAddress)
{
	txSize identifiersSize;
	void* identifiersBuffer;
	FILE* file = NULL;
	txMD5 md5;
	txSize modsSize;
	txSize padSize;
	txLinkerScript* script;
	txSize rsrcSize;
	txLinkerResource* resource;
	uint8_t signature[MD5_DGSTSIZE];
	txSize size;
	txByte byte;
	
	identifiersSize = linker->symbolIndex * sizeof(txID);
	identifiersBuffer = fxNewLinkerChunkClear(linker, identifiersSize);
	
	file = fopen(path, "wb");
	mxThrowElse(file);
	*fileAddress = file;
	
	md5_create(&md5);
	md5_update(&md5, linker->name, linker->nameSize);
	md5_update(&md5, linker->symbolsBuffer, linker->symbolsSize);
	md5_update(&md5, linker->mapsBuffer, linker->mapsSize);
	modsSize = 0;
	script = linker->firstScript;
	while (script) {
		modsSize += 8 + script->pathSize;
		md5_update(&md5, script->path, script->pathSize);
		modsSize += 8 + script->codeSize;
		md5_update(&md5, script->codeBuffer, script->codeSize);
		script = script->nextScript;
	}
	size = 8 
		+ 8 + 4 
		+ 8 + sizeof(signature) 
		+ 8 + linker->nameSize
		+ 8 + linker->symbolsSize 
		+ 8 + identifiersSize
		+ 8 + linker->mapsSize 
		+ 8 + modsSize
		+ 8;
	rsrcSize = 0;
	resource = linker->firstResource;
	while (resource) {
		rsrcSize += 8 + resource->pathSize;
		size += 8 + resource->pathSize;
		md5_update(&md5, resource->path, resource->pathSize);
		if ((padSize = size % 4)) {
			resource->padSize = 4 - padSize;
			rsrcSize += resource->padSize;
			size += resource->padSize;
		}
		rsrcSize += 8 + resource->dataSize;
		size += 8 + resource->dataSize;
		md5_update(&md5, resource->dataBuffer, resource->dataSize);
		resource = resource->nextResource;
	}
	md5_fin(&md5, signature);
	
	size = htonl(size);
	mxThrowElse(fwrite(&size, 4, 1, file) == 1);
	mxThrowElse(fwrite("XS_A", 4, 1, file) == 1);

	size = 8 + 4;
	size = htonl(size);
	mxThrowElse(fwrite(&size, 4, 1, file) == 1);
	mxThrowElse(fwrite("VERS", 4, 1, file) == 1);
	byte = XS_MAJOR_VERSION;
	mxThrowElse(fwrite(&byte, 1, 1, file) == 1);
	byte = XS_MINOR_VERSION;
	mxThrowElse(fwrite(&byte, 1, 1, file) == 1);
	byte = XS_PATCH_VERSION;
	mxThrowElse(fwrite(&byte, 1, 1, file) == 1);
	byte = 0;
	mxThrowElse(fwrite(&byte, 1, 1, file) == 1);

	size = 8 + sizeof(signature);
	size = htonl(size);
	mxThrowElse(fwrite(&size, 4, 1, file) == 1);
	mxThrowElse(fwrite("SIGN", 4, 1, file) == 1);
	mxThrowElse(fwrite(signature, sizeof(signature), 1, file) == 1);
	
	size = 8 + linker->nameSize;
	size = htonl(size);
	mxThrowElse(fwrite(&size, 4, 1, file) == 1);
	mxThrowElse(fwrite("NAME", 4, 1, file) == 1);
	mxThrowElse(fwrite(linker->name, linker->nameSize, 1, file) == 1);

	size = 8 + linker->symbolsSize;
	size = htonl(size);
	mxThrowElse(fwrite(&size, 4, 1, file) == 1);
	mxThrowElse(fwrite("SYMB", 4, 1, file) == 1);
	mxThrowElse(fwrite(linker->symbolsBuffer, linker->symbolsSize, 1, file) == 1);

	size = 8 + identifiersSize;
	size = htonl(size);
	mxThrowElse(fwrite(&size, 4, 1, file) == 1);
	mxThrowElse(fwrite("IDEN", 4, 1, file) == 1);
	mxThrowElse(fwrite(identifiersBuffer, identifiersSize, 1, file) == 1);

	size = 8 + linker->mapsSize;
	size = htonl(size);
	mxThrowElse(fwrite(&size, 4, 1, file) == 1);
	mxThrowElse(fwrite("MAPS", 4, 1, file) == 1);
	mxThrowElse(fwrite(linker->mapsBuffer, linker->mapsSize, 1, file) == 1);

	size = 8 + modsSize;
	size = htonl(size);
	mxThrowElse(fwrite(&size, 4, 1, file) == 1);
	mxThrowElse(fwrite("MODS", 4, 1, file) == 1);
	script = linker->firstScript;
	while (script) {
		size = 8 + script->pathSize;
		size = htonl(size);
		mxThrowElse(fwrite(&size, 4, 1, file) == 1);
		mxThrowElse(fwrite("PATH", 4, 1, file) == 1);
		mxThrowElse(fwrite(script->path, script->pathSize, 1, file) == 1);
		size = 8 + script->codeSize;
		modsSize += size;
		size = htonl(size);
		mxThrowElse(fwrite(&size, 4, 1, file) == 1);
		mxThrowElse(fwrite("CODE", 4, 1, file) == 1);
		mxThrowElse(fwrite(script->codeBuffer, script->codeSize, 1, file) == 1);
		script = script->nextScript;
	}
	
	size = 8 + rsrcSize;
	size = htonl(size);
	mxThrowElse(fwrite(&size, 4, 1, file) == 1);
	mxThrowElse(fwrite("RSRC", 4, 1, file) == 1);
	resource = linker->firstResource;
	while (resource) {
		size = 8 + resource->pathSize + resource->padSize;
		size = htonl(size);
		mxThrowElse(fwrite(&size, 4, 1, file) == 1);
		mxThrowElse(fwrite("PATH", 4, 1, file) == 1);
		mxThrowElse(fwrite(resource->path, resource->pathSize, 1, file) == 1);
		if (resource->padSize)
			mxThrowElse(fwrite(signature, resource->padSize, 1, file) == 1);
		size = 8 + resource->dataSize;
		size = htonl(size);
		mxThrowElse(fwrite(&size, 4, 1, file) == 1);
		mxThrowElse(fwrite("DATA", 4, 1, file) == 1);
		mxThrowElse(fwrite(resource->dataBuffer, resource->dataSize, 1, file) == 1);
		resource = resource->nextResource;
	}
	
	fclose(file);
	*fileAddress = NULL;
}

void fxWriteCData(FILE* file, void* data, txSize size) 
{
	unsigned char* p = data;
	unsigned char c;
	fprintf(file, "\"");
	while (size) {
		c = *p;
		fprintf(file, "\\x%.2x", c);
		p++;
		size--;
	}
	fprintf(file, "\"");
}

void fxWriteCString(FILE* file, txString string) 
{
	unsigned char c;
	fprintf(file, "\"");
	while ((c = *((unsigned char *)string))) {
		if (c == '\\')
			fprintf(file, "\\\\");
		else if (c == '"')
			fprintf(file, "\\\"");
		else if ((32 <= c) && (c < 128))
			fprintf(file, "%c", (char)c);
		else
			fprintf(file, "\\x%.2x\"\"", c);
		string++;
	}
	fprintf(file, "\"");
}

void fxWriteDefines(txLinker* linker, FILE* file)
{
	txID c, i;
	txLinkerSymbol** address;
	
	c = linker->symbolIndex;
	address = &(linker->symbolArray[0]);
	for (i = 0; i < c; i++) {
		if (fxIsCIdentifier(linker, (*address)->string))
			fprintf(file, "#define xsID_%s %d\n", (*address)->string, (*address)->ID);
		address++;
	}
}

void fxWriteScriptCode(txLinkerScript* script, FILE* file)
{
	if (script->preload == C_NULL) {
		fprintf(file, "static const txU1 gxCode%d[%d] = {\n", script->scriptIndex, script->codeSize);
		{
			txU1* buffer = (txU1*)(script->codeBuffer);
			txSize c = script->codeSize, i = 0;
			fprintf(file, "\t");
			for (;;) {
				fprintf(file, "0x%02x", *buffer);
				buffer++;
				i++;
				if (i == c)
					break;
				if (i % 16)
					fprintf(file, ", ");
				else
					fprintf(file, ",\n\t");
			}
			fprintf(file, "\n");
		}
		fprintf(file, "};\n\n");
	}
}

void fxWriteScriptExterns(txLinkerScript* script, FILE* file)
{
	txByte* p = script->hostsBuffer;
	if (p) {
		txID c, i;
		mxDecode2(p, c);
		for (i = 0; i < c; i++) {
			txS1 length = *p++;
			p += 2;
			if (length < 0)
				fprintf(file, "extern void %s(void* data);\n", p);
			else
				fprintf(file, "extern void %s(xsMachine* the);\n", p);
			p += mxStringLength((char*)p) + 1;
		}
	}
}

void fxWriteScriptHosts(txLinkerScript* script, FILE* file)
{
	txByte* p = script->hostsBuffer;
	if (p) {
		txID c, i, id;
		mxDecode2(p, c);
		fprintf(file, "static const xsHostBuilder gxBuilders%d[%d] = {\n", script->scriptIndex, c);
		for (i = 0; i < c; i++) {
			txS1 length = *p++;
			mxDecode2(p, id);
			if (length < 0)
				fprintf(file, "\t{ (xsCallback)%s, -1, -1 },\n", p);
			else
				fprintf(file, "\t{ %s, %d, %d },\n", p, length, id);
			p += mxStringLength((char*)p) + 1;
		}
		fprintf(file, "};\n");
		fprintf(file, "void xsHostModule%d(xsMachine* the)\n", script->scriptIndex);
		fprintf(file, "{\n");
		fprintf(file, "\txsResult = xsBuildHosts(%d, (xsHostBuilder*)gxBuilders%d);\n", c, script->scriptIndex);
		fprintf(file, "}\n\n");
	}
}

void fxWriteScriptRecord(txLinkerScript* script, FILE* file)
{
	fprintf(file, "\t{ ");
	if (script->hostsBuffer)
		fprintf(file, "xsHostModule%d, ", script->scriptIndex);
	else
		fprintf(file, "NULL, ");
	fprintf(file, "NULL, 0, ");
	if (script->preload == C_NULL)
		fprintf(file, "(txS1*)gxCode%d, %d, ", script->scriptIndex, script->codeSize);
	else
		fprintf(file, "NULL, 0, ");
	fprintf(file, "NULL, 0, ");
	fxWriteCString(file, script->path);
	fprintf(file, ", ");
	fprintf(file, "{ XS_MAJOR_VERSION, XS_MINOR_VERSION, XS_PATCH_VERSION, 0 } ");
	fprintf(file, "}");
}

void fxWriteSymbols(txLinker* linker, txString path, FILE** fileAddress)
{
	FILE* file = NULL;
	txSize size;
	txByte byte;
	txMD5 md5;
	
	file = fopen(path, "wb");
	mxThrowElse(file);
	*fileAddress = file;
	size = 8 + 8 + 4 + 8 + linker->symbolsSize;
	size = htonl(size);
	mxThrowElse(fwrite(&size, 4, 1, file) == 1);
	mxThrowElse(fwrite("XS_I", 4, 1, file) == 1);

	size = 8 + 4;
	size = htonl(size);
	mxThrowElse(fwrite(&size, 4, 1, file) == 1);
	mxThrowElse(fwrite("VERS", 4, 1, file) == 1);
	byte = XS_MAJOR_VERSION;
	mxThrowElse(fwrite(&byte, 1, 1, file) == 1);
	byte = XS_MINOR_VERSION;
	mxThrowElse(fwrite(&byte, 1, 1, file) == 1);
	byte = XS_PATCH_VERSION;
	mxThrowElse(fwrite(&byte, 1, 1, file) == 1);
	byte = (linker->hostsCount) ? 1 : 0;
	mxThrowElse(fwrite(&byte, 1, 1, file) == 1);

	size = 8 + linker->symbolsSize;
	size = htonl(size);
	mxThrowElse(fwrite(&size, 4, 1, file) == 1);
	mxThrowElse(fwrite("SYMB", 4, 1, file) == 1);
	mxThrowElse(fwrite(linker->symbolsBuffer, linker->symbolsSize, 1, file) == 1);

	fclose(file);
	*fileAddress = NULL;
	
	md5_create(&md5);
	md5_update(&md5, linker->symbolsBuffer, linker->symbolsSize);
	md5_fin(&md5, linker->symbolsChecksum);
}

static const uint32_t md5_k[] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static const uint32_t md5_iv[] = {
	0x67452301,
	0xefcdab89,
	0x98badcfe,
	0x10325476,
};

static const uint32_t md5_rotate[][4] = {
	{7, 12, 17, 22},
	{5, 9, 14, 20},
	{4, 11, 16, 23},
	{6, 10, 15, 21},
};

static inline uint32_t rotl(uint32_t n, int k)
{
	return (n << k) | (n >> (32-k));
}

#define F(x,y,z) (z ^ (x & (y ^ z)))
#define G(x,y,z) (y ^ (z & (y ^ x)))
#define H(x,y,z) (x ^ y ^ z)
#define I(x,y,z) (y ^ (x | ~z))
#define R1(a, b, c, d, i)	(a = b + rotl(a + F(b, c, d) + W[i] + md5_k[i], md5_rotate[0][i % 4]))
#define R2(a, b, c, d, i)	(a = b + rotl(a + G(b, c, d) + W[(5*i + 1) % 16] + md5_k[i], md5_rotate[1][i % 4]))
#define R3(a, b, c, d, i)	(a = b + rotl(a + H(b, c, d) + W[(3*i + 5) % 16] + md5_k[i], md5_rotate[2][i % 4]))
#define R4(a, b, c, d, i)	(a = b + rotl(a + I(b, c, d) + W[7*i % 16] + md5_k[i], md5_rotate[3][i % 4]))

void md5_create(txMD5 *s)
{
	int i;

	s->len = 0;
	for (i = 0; i < MD5_NUMSTATE; i++)
		s->state[i] = md5_iv[i];
}

static void md5_process(txMD5 *s, const uint8_t *blk)
{
	uint32_t W[16], a, b, c, d, w, t;
	int i;

	for (i = 0; i < 16; i++) {
		w = *blk++;
		w |= *blk++ << 8;
		w |= *blk++ << 16;
		w |= *blk++ << 24;
		W[i] = w;
	}
	a = s->state[0];
	b = s->state[1];
	c = s->state[2];
	d = s->state[3];

	i = 0;
	for (; i < 16; i++) {
		R1(a, b, c, d, i);
		t = d; d = c; c = b; b = a; a = t;
	}
	for (; i < 32; i++) {
		R2(a, b, c, d, i);
		t = d; d = c; c = b; b = a; a = t;
	}
	for (; i < 48; i++) {
		R3(a, b, c, d, i);
		t = d; d = c; c = b; b = a; a = t;
	}
	for (; i < 64; i++) {
		R4(a, b, c, d, i);
		t = d; d = c; c = b; b = a; a = t;
	}
	s->state[0] += a;
	s->state[1] += b;
	s->state[2] += c;
	s->state[3] += d;
}

void md5_update(txMD5 *s, const void *data, uint32_t size)
{
	const uint8_t *p = data;
	uint32_t r = s->len % MD5_BLKSIZE;

	s->len += size;
	if (r > 0) {
		uint32_t n = MD5_BLKSIZE - r;
		if (size < n) {
			memcpy(s->buf + r, p, size);
			return;
		}
		memcpy(s->buf + r, p, n);
		size -= n;
		p += n;
		md5_process(s, s->buf);
	}
	for (; size >= MD5_BLKSIZE; size -= MD5_BLKSIZE, p += MD5_BLKSIZE)
		md5_process(s, p);
	memcpy(s->buf, p, size);
}

void md5_fin(txMD5 *s, uint8_t *dgst)
{
	uint32_t r = s->len % MD5_BLKSIZE;
	uint64_t l;
	uint8_t *p;
	int i;

	s->buf[r++] = 0x80;
	if (r > MD5_BLKSIZE - 8) {
		memset(s->buf + r, 0, MD5_BLKSIZE - r);
		md5_process(s, s->buf);
		r = 0;
	}
	memset(s->buf + r, 0, MD5_BLKSIZE - 8 - r);
	l = s->len * 8;
	p = &s->buf[MD5_BLKSIZE - 8];
	*p++ = (uint8_t)l;
	*p++ = (uint8_t)(l >> 8);
	*p++ = (uint8_t)(l >> 16);
	*p++ = (uint8_t)(l >> 24);
	*p++ = (uint8_t)(l >> 32);
	*p++ = (uint8_t)(l >> 40);
	*p++ = (uint8_t)(l >> 48);
	*p++ = (uint8_t)(l >> 56);
	md5_process(s, s->buf);

	for (i = 0; i < MD5_NUMSTATE; i++) {
		uint32_t w = s->state[i];
		*dgst++ = w;
		*dgst++ = w >> 8;
		*dgst++ = w >> 16;
		*dgst++ = w >> 24;
	}
}


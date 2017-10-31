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

#include "xsAll.h"

/* old programming interface defaults to 0 */

#ifndef mxUseDefaultMachinePlatform
	#define mxUseDefaultMachinePlatform 0
#endif
#ifndef mxUseDefaultBuildKeys
	#define mxUseDefaultBuildKeys 0
#endif
#ifndef mxUseDefaultChunkAllocation
	#define mxUseDefaultChunkAllocation 0
#endif
#ifndef mxUseDefaultSlotAllocation
	#define mxUseDefaultSlotAllocation 0
#endif
#ifndef mxUseDefaultHostCollection
	#define mxUseDefaultHostCollection 0
#endif
#ifndef mxUseDefaultFindModule
	#define mxUseDefaultFindModule 0
#endif
#ifndef mxUseDefaultLoadModule
	#define mxUseDefaultLoadModule 0
#endif
#ifndef mxUseDefaultParseScript
	#define mxUseDefaultParseScript 0
#endif
#ifndef mxUseDefaultQueuePromiseJobs
	#define mxUseDefaultQueuePromiseJobs 0
#endif
#ifndef mxUseDefaultDebug
	#define mxUseDefaultDebug 0
#endif

#ifndef mxLink

#if mxUseDefaultFindModule
	static txBoolean fxFindPreparation(txMachine* the, txString path, txID* id);
#endif

#ifdef mxParse
#include "xsScript.h"
	#if mxUseDefaultFindModule
		static txBoolean fxFindScript(txMachine* the, txString path, txID* id);
	#endif
	#if mxUseDefaultLoadModule
		extern txScript* fxLoadScript(txMachine* the, txString path, txUnsigned flags);
	#endif
#endif

#endif /* mxLink*/

#if mxUseDefaultMachinePlatform

void fxCreateMachinePlatform(txMachine* the)
{
}

void fxDeleteMachinePlatform(txMachine* the)
{
}

#endif /* mxUseDefaultMachinePlatform */

#ifndef mxLink

#if mxUseDefaultBuildKeys

void fxBuildKeys(txMachine* the)
{
	int i;
	for (i = 0; i < XS_SYMBOL_ID_COUNT; i++) {
		txID id = the->keyIndex;
		txSlot* description = fxNewSlot(the);
		fxCopyStringC(the, description, gxIDStrings[i]);
		the->keyArray[id] = description;
		the->keyIndex++;
	}
	for (; i < XS_ID_COUNT; i++) {
		fxID(the, gxIDStrings[i]);
	}
}

#endif	/* mxUseDefaultBuildKeys */ 

#endif


#if mxUseDefaultChunkAllocation

void* fxAllocateChunks(txMachine* the, txSize theSize)
{
	return c_malloc(theSize);
}

void fxFreeChunks(txMachine* the, void* theChunks)
{
	c_free(theChunks);
}

#endif /* mxUseDefaultChunkAllocation */ 


#if mxUseDefaultSlotAllocation

txSlot* fxAllocateSlots(txMachine* the, txSize theCount)
{
	return(txSlot*)c_malloc(theCount * sizeof(txSlot));
}

void fxFreeSlots(txMachine* the, void* theSlots)
{
	c_free(theSlots);
}

#endif /* mxUseDefaultSlotAllocation */ 


#if mxUseDefaultHostCollection

void fxMarkHost(txMachine* the, txMarkRoot markRoot)
{
}

void fxSweepHost(txMachine* the)
{
}

#endif /* mxUseDefaultHostCollection */ 


#ifndef mxLink

#if mxUseDefaultFindModule

txID fxFindModule(txMachine* the, txID moduleID, txSlot* slot)
{
	txPreparation* preparation = the->preparation;
	char name[C_PATH_MAX];
	char path[C_PATH_MAX];
	txBoolean absolute = 0, relative = 0, search = 0;
	txInteger dot = 0;
	txString slash;
	txID id;
	if (preparation)
		fxToStringBuffer(the, slot, name, sizeof(name) - preparation->baseLength - 4);
	else
		fxToStringBuffer(the, slot, name, sizeof(name));
	if (name[0] == '/') {
		absolute = 1;
	}	
	else if ((name[0] == '.') && (name[1] == '/')) {
		dot = 1;
		relative = 1;
	}	
	else if ((name[0] == '.') && (name[1] == '.') && (name[2] == '/')) {
		dot = 2;
		relative = 1;
	}
#if mxWindows
	else if (('A' <= name[0]) && (name[0] <= 'Z') && (name[1] == ':') && (name[2] == '\\')) {
		absolute = 1;
	}	
#endif
	else {
		relative = 1;
		search = 1;
	}
#if mxWindows
	{
		char c;
		slash = name;
		while ((c = *slash)) {
			if (c == '/')
				*slash = '\\';
			slash++;
		}
	}
#endif
	if (absolute) {
		if (preparation) {
			c_strcpy(path, preparation->base);
			c_strcat(path, name + 1);
			c_strcat(path, ".xsb");
			if (fxFindPreparation(the, path, &id))
				return id;
		}
#ifdef mxParse
		if (fxFindScript(the, name, &id))
			return id;
#endif
	}
	if (relative && (moduleID != XS_NO_ID)) {
		c_strcpy(path, fxGetKeyName(the, moduleID));
		slash = c_strrchr(path, mxSeparator);
		if (!slash)
			return XS_NO_ID;
		if (dot == 0)
			slash++;
		else if (dot == 2) {
			*slash = 0;
			slash = c_strrchr(path, mxSeparator);
			if (!slash)
				return XS_NO_ID;
		}
		if (preparation) {
			if (!c_strncmp(path, preparation->base, preparation->baseLength)) {
				*slash = 0;
				c_strcat(path, name + dot);
				c_strcat(path, ".xsb");
				if (fxFindPreparation(the, path, &id))
					return id;
			}
		}
#ifdef mxParse
		*slash = 0;
		c_strcat(path, name + dot);
		if (fxFindScript(the, path, &id))
			return id;
#endif
	}
	if (search) {
#ifdef mxParse
		txSlot *iterator, *result;
#endif
		if (preparation) {
			c_strcpy(path, preparation->base);
			c_strcat(path, name);
			c_strcat(path, ".xsb");
			if (fxFindPreparation(the, path, &id))
				return id;
		}
#ifdef mxParse
		mxCallID(&mxModulePaths, mxID(_Symbol_iterator), 0);
		iterator = the->stack;
		for (;;) {
			mxCallID(iterator, mxID(_next), 0);
			result = the->stack;
			mxGetID(result, mxID(_done));
			if (fxToBoolean(the, the->stack))
				break;
			the->stack++;
			mxGetID(result, mxID(_value));
			fxToStringBuffer(the, the->stack++, path, sizeof(path));
 			c_strcat(path, name);
			if (fxFindScript(the, path, &id))
				return id;
		}
#endif
	}
	return XS_NO_ID;
}

txBoolean fxFindPreparation(txMachine* the, txString path, txID* id)
{
	txSize size;
	txByte* code = fxGetArchiveCode(the, path, &size);
	if (!code) {
		txPreparation* preparation = the->preparation;
		txInteger c = preparation->scriptCount;
		txScript* script = preparation->scripts;
		path += preparation->baseLength;
		while (c > 0) {
			if (!c_strcmp(path, script->path)) {
				path -= preparation->baseLength;
				break;
			}
			c--;
			script++;
		}
		if (c == 0) {
			*id = XS_NO_ID;
			return 0;
		}
	}
	*id = fxNewNameC(the, path);
	return 1;
}

#ifdef mxParse
txBoolean fxFindScript(txMachine* the, txString path, txID* id)
{
	txString slash;
	txString dot;
	char real[C_PATH_MAX];
	slash = c_strrchr(path, mxSeparator);
	if (!slash)
		slash = path;
	dot = c_strrchr(slash, '.');
	if (!dot)
		c_strcat(path, ".js");
	if (c_realpath(path, real)) {
		*id = fxNewNameC(the, real);
		return 1;
	}
	return 0;
}	
#endif

#endif /* mxUseDefaultFindModule */

#if mxUseDefaultLoadModule

void fxLoadModule(txMachine* the, txID moduleID)
{
	char path[C_PATH_MAX];
	txByte* code;
	txSize size;
	c_strcpy(path, fxGetKeyName(the, moduleID));
	code = fxGetArchiveCode(the, path, &size);
	if (code) {
		txScript script;
		script.callback = NULL;
		script.symbolsBuffer = NULL;
		script.symbolsSize = 0;
		script.codeBuffer = code;
		script.codeSize = size;
		script.hostsBuffer = NULL;
		script.hostsSize = 0;
		script.path = path;
		script.version[0] = XS_MAJOR_VERSION;
		script.version[1] = XS_MINOR_VERSION;
		script.version[2] = XS_PATCH_VERSION;
		script.version[3] = 0;
		fxResolveModule(the, moduleID, &script, C_NULL, C_NULL);
	}
	else {
 		txPreparation* preparation = the->preparation;
		if (preparation) {
			txInteger c = preparation->scriptCount;
			txScript* script = preparation->scripts;
			while (c > 0) {
				if (!c_strcmp(path + preparation->baseLength, script->path)) {
					fxResolveModule(the, moduleID, script, C_NULL, C_NULL);
					return;
				}
				c--;
				script++;
			}
		}
	}
#ifdef mxParse
	{
	#ifdef mxDebug
		txUnsigned flags = mxDebugFlag;
	#else
		txUnsigned flags = 0;
	#endif
 		txScript* script = fxLoadScript(the, path, flags);
 		if (script)
 			fxResolveModule(the, moduleID, script, C_NULL, C_NULL);
 	}
#endif
}

#ifdef mxParse
txScript* fxLoadScript(txMachine* the, txString path, txUnsigned flags)
{
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	FILE* file = NULL;
	txString name = NULL;
	char map[C_PATH_MAX];
	txScript* script = NULL;
	fxInitializeParser(parser, the, 32*1024, 1993);
	parser->firstJump = &jump;
	if (c_setjmp(jump.jmp_buf) == 0) {
		parser->path = fxNewParserSymbol(parser, path);
		file = fopen(path, "r");
		mxParserThrowElse(file);
		fxParserTree(parser, file, (txGetter)fgetc, flags, &name);
		fclose(file);
		file = NULL;
		if (name) {
			txString slash = c_strrchr(path, mxSeparator);
			if (slash) *slash = 0;
			c_strcat(path, name);
			mxParserThrowElse(c_realpath(path, map));
			parser->path = fxNewParserSymbol(parser, map);
			file = fopen(map, "r");
			mxParserThrowElse(file);
			fxParserSourceMap(parser, file, (txGetter)fgetc, flags, &name);
			fclose(file);
			file = NULL;
			if (slash) *slash = 0;
			c_strcat(path, name);
			mxParserThrowElse(c_realpath(path, map));
			parser->path = fxNewParserSymbol(parser, map);
		}
		fxParserHoist(parser);
		fxParserBind(parser);
		script = fxParserCode(parser);
	}
	if (file)
		fclose(file);
#ifdef mxInstrument
	if (the->parserTotal < parser->total)
		the->parserTotal = parser->total;
#endif
	fxTerminateParser(parser);
	return script;
}
#endif

#endif  /* mxUseDefaultLoadModule */

#if mxUseDefaultParseScript

txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags)
{
#ifdef mxParse
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	txScript* script = NULL;
	fxInitializeParser(parser, the, 32*1024, 1993);
	parser->firstJump = &jump;
	if (c_setjmp(jump.jmp_buf) == 0) {
		fxParserTree(parser, stream, getter, flags, NULL);
		fxParserHoist(parser);
		fxParserBind(parser);
		script = fxParserCode(parser);
	}
#ifdef mxInstrument
	if (the->parserTotal < parser->total)
		the->parserTotal = parser->total;
#endif
	fxTerminateParser(parser);
	return script;
#else
	return C_NULL;
#endif
}

#endif  /* mxUseDefaultParseScript */

#endif /* mxLink */

#if mxUseDefaultQueuePromiseJobs

void fxQueuePromiseJobs(txMachine* the)
{
	mxUnknownError("promise: no queue");
}

#endif  /* mxUseDefaultQueuePromiseJobs */

#ifdef mxDebug

#if mxUseDefaultDebug

void fxAbort(txMachine* the)
{
	c_exit(0);
}

void fxConnect(txMachine* the)
{
}

void fxDisconnect(txMachine* the)
{
}

txBoolean fxIsConnected(txMachine* the)
{
	return 0;
}

txBoolean fxIsReadable(txMachine* the)
{
	return 0;
}

void fxReceive(txMachine* the)
{
}

void fxSend(txMachine* the, txBoolean more)
{
}

#endif /* mxUseDefaultDebug */

#endif

#if mxWindows

#if _MSC_VER < 1800

unsigned long c_nan[2]={0xffffffff, 0x7fffffff};
unsigned long c_infinity[2]={0x00000000, 0x7ff00000};

int c_fpclassify(double x)
{
	int result = FP_NORMAL;
	switch (_fpclass(x)) {
	case _FPCLASS_SNAN:
	case _FPCLASS_QNAN:
		result = FP_NAN;
		break;
	case _FPCLASS_NINF:
	case _FPCLASS_PINF:
		result = FP_INFINITE;
		break;
	case _FPCLASS_NZ:
	case _FPCLASS_PZ:
		result = FP_ZERO;
		break;
	case _FPCLASS_ND:
	case _FPCLASS_PD:
		result = FP_SUBNORMAL;
		break;
	}
	return result;
}

#endif /* _MSC_VER < 1800 */

int c_gettimeofday(c_timeval *tp, struct c_timezone *tzp)
{
  struct _timeb tb;

  _ftime(&tb);
  if (tp != 0) {
    tp->tv_sec = (long)tb.time;
    tp->tv_usec = tb.millitm * 1000;
  }
  if (tzp != 0) {
    tzp->tz_minuteswest = tb.timezone;
    tzp->tz_dsttime = tb.dstflag;
  }
  return (0);
}

#ifdef mxParse
char *c_realpath(const char *path, char *real)
{
	if (_fullpath(real, path, C_PATH_MAX) != NULL) {
		DWORD attributes = GetFileAttributes(real);
		if (attributes != 0xFFFFFFFF) {
			return real;
		}
	}
	return C_NULL;
}
#endif

#endif

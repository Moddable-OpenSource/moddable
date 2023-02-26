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
#include "xsScript.h"

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
#ifndef mxUseDefaultAbort
	#define mxUseDefaultAbort 0
#endif
#ifndef mxUseDefaultDebug
	#define mxUseDefaultDebug 0
#endif

#if mxUseDefaultMachinePlatform

void fxCreateMachinePlatform(txMachine* the)
{
}

void fxDeleteMachinePlatform(txMachine* the)
{
}

#endif /* mxUseDefaultMachinePlatform */

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

#if mxUseDefaultFindModule

txID fxFindModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* slot)
{
	txPreparation* preparation = the->preparation;
	char name[C_PATH_MAX];
#if MODDEF_XS_TEST
	char extension[5] = "";
#endif
	char buffer[C_PATH_MAX];
	txInteger dot = 0;
	txString slash;
	txString path;
	fxToStringBuffer(the, slot, name, sizeof(name));
	if (name[0] == '.') {
		if (name[1] == '/') {
			dot = 1;
		}
		else if ((name[1] == '.') && (name[2] == '/')) {
			dot = 2;
		}
	}
#if mxWindows
	{
		char c;
		slash = name;
		if (!c_strncmp(name, "xsbug://", 8))
			slash += 8;
		while ((c = *slash)) {
			if (c == '/')
				*slash = '\\';
			slash++;
		}
	}
#endif
	slash = c_strrchr(name, mxSeparator);
	if (!slash)
		slash = name;
	slash = c_strrchr(slash, '.');
	if (slash && (!c_strcmp(slash, ".js") || !c_strcmp(slash, ".mjs"))) {
#if MODDEF_XS_TEST
		c_strcpy(extension, slash);
#endif
		*slash = 0;
	}
	if (dot) {
		if (moduleID == XS_NO_ID)
			return XS_NO_ID;
		buffer[0] = mxSeparator;
		path = buffer + 1;
		c_strcpy(path, fxGetKeyName(the, moduleID));
		slash = c_strrchr(buffer, mxSeparator);
		if (!slash)
			return XS_NO_ID;
		if (dot == 2) {
			*slash = 0;
			slash = c_strrchr(buffer, mxSeparator);
			if (!slash)
				return XS_NO_ID;
		}
		*slash = 0;
		if ((c_strlen(buffer) + c_strlen(name + dot)) >= sizeof(buffer))
			mxRangeError("path too long");
		c_strcat(buffer, name + dot);
	}
	else
		path = name;
	if (preparation) {
		txInteger c = preparation->scriptCount;
		txScript* script = preparation->scripts;
		size_t size;
		if (fxGetArchiveCode(the, the->archive, path, &size))
			return fxNewNameC(the, path);
		while (c > 0) {
			if (!c_strcmp(path, script->path))
				return fxNewNameC(the, path);
			c--;
			script++;
		}
	}
#if MODDEF_XS_TEST
	if (!c_strncmp(path, "xsbug://", 8)) {
		c_strcat(path, extension);
		return fxNewNameC(the, path);
	}
#endif
	return XS_NO_ID;
}

#endif /* mxUseDefaultFindModule */

#if mxUseDefaultLoadModule

#if MODDEF_XS_TEST
extern void fxDebugImport(txMachine* the, txSlot* module, txString path);
#endif

void fxLoadModule(txMachine* the, txSlot* module, txID moduleID)
{
	txString path = fxGetKeyName(the, moduleID);
	txByte* code;
	size_t size;
	code = fxGetArchiveCode(the, the->archive, path, &size);
	if (code) {
		txScript script;
		script.callback = NULL;
		script.symbolsBuffer = NULL;
		script.symbolsSize = 0;
		script.codeBuffer = code;
		script.codeSize = (txSize)size;
		script.hostsBuffer = NULL;
		script.hostsSize = 0;
		script.path = path;
		script.version[0] = XS_MAJOR_VERSION;
		script.version[1] = XS_MINOR_VERSION;
		script.version[2] = XS_PATCH_VERSION;
		script.version[3] = 0;
		fxResolveModule(the, module, moduleID, &script, C_NULL, C_NULL);
	}
	else {
 		txPreparation* preparation = the->preparation;
		if (preparation) {
			txInteger c = preparation->scriptCount;
			txScript* script = preparation->scripts;
			while (c > 0) {
				if (!c_strcmp(path, script->path)) {
					fxResolveModule(the, module, moduleID, script, C_NULL, C_NULL);
					return;
				}
				c--;
				script++;
			}
		}
	}
#if MODDEF_XS_TEST
	if (!c_strncmp(path, "xsbug://", 8)) {
		fxDebugImport(the, module, path);
		return;
	}
#endif
}

#endif  /* mxUseDefaultLoadModule */

#if mxUseDefaultParseScript

txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags)
{
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	txScript* script = NULL;
	fxInitializeParser(parser, the, the->parserBufferSize, the->parserTableModulo);
	parser->firstJump = &jump;
	if (c_setjmp(jump.jmp_buf) == 0) {
		fxParserTree(parser, stream, getter, flags, NULL);
#ifdef mxDebug
		parser->flags |= mxDebugFlag;
		if (!parser->source) {
			char tag[16];
			parser->flags |= mxDebugFlag;
			fxGenerateTag(the, tag, sizeof(tag), C_NULL);
			parser->source = fxNewParserSymbol(parser, tag);
		}
		if (fxIsConnected(the)) {
			if (getter == fxStringGetter)
				fxFileEvalString(the, ((txStringStream*)stream)->slot->value.string, parser->source->string);
			else if (getter == fxStringCGetter)
				fxFileEvalString(the, ((txStringCStream*)stream)->buffer, parser->source->string);
		}
#endif
		fxParserHoist(parser);
		fxParserBind(parser);
		script = fxParserCode(parser);
	}
#ifdef mxInstrument
	if (the->peakParserSize < parser->total)
		the->peakParserSize = parser->total;
#endif
	fxTerminateParser(parser);
	return script;
}

#endif  /* mxUseDefaultParseScript */

#if mxUseDefaultQueuePromiseJobs

void fxQueuePromiseJobs(txMachine* the)
{
	mxUnknownError("promise: no queue");
}

#endif  /* mxUseDefaultQueuePromiseJobs */

#if mxUseDefaultAbort

void fxAbort(txMachine* the, int status)
{
	txString why = C_NULL;
	switch (status) {
	case XS_STACK_OVERFLOW_EXIT:
		why = "stack overflow";
		break;
	case XS_NOT_ENOUGH_MEMORY_EXIT:
		why = "memory full";
		break;
	case XS_NO_MORE_KEYS_EXIT:
		why = "not enough keys";
		break;
	case XS_DEAD_STRIP_EXIT:
		why = "dead strip";
		break;
	case XS_DEBUGGER_EXIT:
		break;
	case XS_FATAL_CHECK_EXIT:
		break;
	case XS_UNHANDLED_EXCEPTION_EXIT:
		why = "unhandled exception";
		break;
	case XS_UNHANDLED_REJECTION_EXIT:
		why = "unhandled rejection";
		break;
	case XS_TOO_MUCH_COMPUTATION_EXIT:
		why = "too much computation";
		break;
	default:
		why = "unknown";
		break;
	}
	if (why)
		fprintf(stderr, "Error: %s\n", why);
	c_exit(status);
}

#endif  /* mxUseDefaultAbort */

#ifdef mxDebug

#if mxUseDefaultDebug

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

#if mxWindows || mxMacOSX || mxLinux || mxWasm
uint32_t modMilliseconds()
{
	c_timeval tv;
	c_gettimeofday(&tv, NULL);
// #if (mxWasm || mxWindows || mxMacOSX)
	return (uint32_t)(uint64_t)(((double)(tv.tv_sec) * 1000.0) + ((double)(tv.tv_usec) / 1000.0));
// #else
// 	return (uint32_t)(((double)(tv.tv_sec) * 1000.0) + ((double)(tv.tv_usec) / 1000.0));
// #endif
}
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

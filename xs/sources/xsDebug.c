/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

#if MODDEF_XS_TEST
	#undef MODDEF_XS_XSBUG_HOOKS
	#define MODDEF_XS_XSBUG_HOOKS 1
#endif

#if defined(DEBUG_EFM)
char _lastDebugStrBuffer[256];
char _debugStrBuffer[256];
#endif
static void fxVReportException(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments);

#if defined(mxInstrument) || defined (mxDebug)	
static const char gxHexaDigits[] ICACHE_FLASH_ATTR = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#endif

#ifdef mxDebug
static void fxClearAllBreakpoints(txMachine* the);
static void fxClearBreakpoint(txMachine* the, txString thePath, txInteger theLine, size_t theID);
static void fxDebugEval(txMachine* the, txSlot* frame, txString buffer, txInteger index);
static txU1* fxDebugEvalAtom(txMachine* the, txU1* p, Atom* atom, txString t);
static void fxDebugEvalBuffer(txMachine* the, txString buffer, txSlot* expression);
static txBoolean fxDebugEvalExpression(txMachine* the, txSlot* frame, txSlot* expression, txSlot* result);
static void fxDebugParse(txMachine* the);
static void fxDebugParseTag(txMachine* the, txString name);
static void fxDebugPopTag(txMachine* the);
static void fxDebugPushTag(txMachine* the);
static void fxDebugScriptCDATA(txMachine* the, char c);
static void fxEcho(txMachine* the, txString theString);
static void fxEchoAddress(txMachine* the, txSlot* theSlot);
static void fxEchoArrayBuffer(txMachine* the, txSlot* theInstance, txInspectorNameList* theList);
static void fxEchoBigInt(txMachine* the, txBigInt* bigint);
static void fxEchoCharacter(txMachine* the, char theCharacter);
static void fxEchoException(txMachine* the, txSlot* exception);
static void fxEchoFlags(txMachine* the, txString state, txFlag flag);
static void fxEchoFormat(txMachine* the, txString theFormat, c_va_list theArguments);
static void fxEchoFrameName(txMachine* the, txSlot* theFrame);
static void fxEchoFramePathLine(txMachine* the, txSlot* theFrame);
static void fxEchoInteger(txMachine* the, txInteger theInteger);
static void fxEchoInstance(txMachine* the, txSlot* theInstance, txInspectorNameList* theList);
static void fxEchoModule(txMachine* the, txSlot* module, txInspectorNameList* list);
static void fxEchoNumber(txMachine* the, txNumber theNumber);
static void fxEchoPathLine(txMachine* the, txString thePath, txInteger theLine);
static void fxEchoProperty(txMachine* the, txSlot* theProperty, txInspectorNameList* theList, txString thePrefix, txIndex theIndex, txString theSuffix);
static void fxEchoPropertyHost(txMachine* the, txInspectorNameList* theList, txSlot* theInstance, txSlot* theHost);
static void fxEchoPropertyInstance(txMachine* the, txInspectorNameList* theList, txString thePrefix, txIndex theIndex, txString theSuffix, txID theID, txFlag theFlag, txSlot* theInstance);
static void fxEchoPropertyName(txMachine* the, txString thePrefix, txIndex theIndex, txString theSuffix, txID theID);
static void fxEchoStart(txMachine* the);
static void fxEchoStop(txMachine* the);
static void fxEchoString(txMachine* the, txString theString);
static void fxEchoTypedArray(txMachine* the, txSlot* theInstance, txInspectorNameList* theList);
static txSlot* fxFindFrame(txMachine* the);
static txSlot* fxFindRealm(txMachine* the);
static void fxGo(txMachine* the);
static void fxIndexToString(txMachine* the, txIndex theIndex, txString theBuffer, txSize theSize);
static void fxListFrames(txMachine* the);
static void fxListGlobal(txMachine* the);
static void fxListLocal(txMachine* the);
static void fxListModules(txMachine* the);
static void fxSetBreakpoint(txMachine* the, txString thePath, txInteger theLine, size_t theID);
static void fxSetBreakpointCondition(txMachine* the, txSlot* reference, txString it);
static void fxSetBreakpointHitCount(txMachine* the, txSlot* reference, txString it);
static void fxSetBreakpointTrace(txMachine* the, txSlot* reference, txString it);
static void fxSelect(txMachine* the, txSlot* slot);
static void fxStep(txMachine* the);
static void fxStepInside(txMachine* the);
static void fxStepOutside(txMachine* the);
static txSlot* fxToInstanceInspector(txMachine* the, txSlot* instance);
static void fxToggle(txMachine* the, txSlot* slot);
static void fxReportException(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...);

#define mxIsDigit(c) \
	(('0' <= c) && (c <= '9'))
#define mxIsFirstLetter(c) \
	((('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '_') || (c == ':'))
#define mxIsNextLetter(c) \
	((('0' <= c) && (c <= '9')) || (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '.') || (c == '-') || (c == '_') || (c == ':'))
#define mxIsSpace(c) \
	((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t'))
	
enum {
	XS_BODY_STATE = 0,
	XS_CR_STATE,
	XS_LF_STATE,
	XS_TAG_STATE,
	XS_START_TAG_NAME_STATE,
	XS_START_TAG_SPACE_STATE,
	XS_ATTRIBUTE_NAME_STATE,
	XS_ATTRIBUTE_SPACE_STATE,
	XS_ATTRIBUTE_EQUAL_STATE,
	XS_ATTRIBUTE_VALUE_STATE,
	XS_EMPTY_TAG_STATE,
	XS_END_TAG_STATE,
	XS_END_TAG_NAME_STATE,
	XS_END_TAG_SPACE_STATE,
	XS_PROCESSING_INSTRUCTION_STATE,
	XS_PROCESSING_INSTRUCTION_SPACE_STATE,
	XS_START_CDATA_STATE_1,
	XS_START_CDATA_STATE_2,
	XS_START_CDATA_STATE_3,
	XS_START_CDATA_STATE_4,
	XS_START_CDATA_STATE_5,
	XS_START_CDATA_STATE_6,
	XS_START_CDATA_STATE_7,
	XS_CDATA_STATE,
	XS_END_CDATA_STATE_1,
	XS_END_CDATA_STATE_2,
	XS_END_CDATA_STATE_3,
	XS_ERROR_STATE
};

enum {
	XS_ABORT_TAG = 0,
	XS_BREAKPOINT_TAG,
	XS_BREAKPOINT_CONDITION_TAG,
	XS_BREAKPOINT_HIT_COUNT_TAG,
	XS_BREAKPOINT_TRACE_TAG,
	XS_CLEAR_ALL_BREAKPOINTS_TAG,
	XS_CLEAR_BREAKPOINTS_TAG,
	XS_EVAL_TAG,
	XS_GO_TAG,
	XS_IMPORT_TAG,
	XS_LOGOUT_TAG,
	XS_MODULE_TAG,
	XS_SCRIPT_TAG,
	XS_SET_BREAKPOINT_TAG,
	XS_SET_ALL_BREAKPOINTS_TAG,
	XS_SELECT_TAG,
	XS_STEP_TAG,
	XS_STEP_INSIDE_TAG,
	XS_STEP_OUTSIDE_TAG,
	XS_TOGGLE_TAG,
	XS_START_PROFILING_TAG,
	XS_STOP_PROFILING_TAG,
	XS_UNKNOWN_TAG
};

enum {
	XS_ID_ATTRIBUTE = 0,
	XS_LINE_ATTRIBUTE,
	XS_PATH_ATTRIBUTE,
	XS_UNKNOWN_ATTRIBUTE
};

void fxCheck(txMachine* the, txString thePath, txInteger theLine)
{
#if mxWindows
	printf("%s(%ld): fatal!\n", thePath, (int)theLine);
#else
	c_printf("%s:%d: fatal!\n", thePath, (int)theLine);
#endif
	fxAbort(the, XS_FATAL_CHECK_EXIT);
}

void fxClearAllBreakpoints(txMachine* the)
{
	mxBreakpoints.value.list.first = C_NULL;
}

void fxClearBreakpoint(txMachine* the, txString thePath, txInteger theLine, size_t theID)
{
	txID path;
	txSlot** breakpointAddress;
	txSlot* breakpoint;

	if (!thePath)
		return;
	if ((theID == 0) && (theLine == 0)) { 
		if (!c_strcmp(thePath, "exceptions")) {
			the->breakOnExceptionsFlag = 0;
			return;
		}	
		if (!c_strcmp(thePath, "start")) {
			the->breakOnStartFlag = 0;
			return;
		}	
	}
	path = fxFindName(the, thePath);
	if (!path)
		return;
	breakpointAddress = &(mxBreakpoints.value.list.first);
	while ((breakpoint = *breakpointAddress)) {
		if ((breakpoint->ID == path) && (breakpoint->value.breakpoint.line == theLine)) {
			*breakpointAddress = breakpoint->next;
			break;
		}
		breakpointAddress = &(breakpoint->next);
	}
}

void fxDebugCommand(txMachine* the)
{
	the->debugExit = 0;
	the->debugModule = C_NULL;
	while (fxIsConnected(the)) {
		fxReceive(the);
		fxDebugParse(the);
		if ((the->debugState == XS_LF_STATE) && (the->debugExit > 0))
			break;
	}
	mxHostInspectors.value.list.first = C_NULL;
	mxHostInspectors.value.list.last = C_NULL;
#if MODDEF_XS_XSBUG_HOOKS
	if (the->debugTag == XS_IMPORT_TAG)
		fxQueueJob(the, 2, C_NULL);
	else if (the->debugTag == XS_SCRIPT_TAG)
		fxQueueJob(the, 6, C_NULL);
#endif
}

void fxDebugEval(txMachine* the, txSlot* frame, txString buffer, txInteger index)
{
// #if mxDebugEval
	txSlot* result;
	txSlot* expression;
	mxHostInspectors.value.list.first = C_NULL;
	mxHostInspectors.value.list.last = C_NULL;
	mxTemporary(result);
	mxTemporary(expression);
	fxDebugEvalBuffer(the, buffer, expression);
	if (fxDebugEvalExpression(the, frame, expression, result)) {
		txInspectorNameList aList = { C_NULL, C_NULL };
		fxEchoStart(the);
		fxEcho(the, "<result line=\"");
		fxEchoInteger(the, index);
		fxEcho(the, "\">");
		if (result->kind == XS_REFERENCE_KIND) {
			txSlot* instance = result->value.reference;
			txSlot* instanceInspector = fxToInstanceInspector(the, instance);
			if (!instanceInspector)
				fxToggle(the, instance);
			fxEchoProperty(the, result, &aList, C_NULL, -1, C_NULL);
			if (!instanceInspector)
				fxToggle(the, instance);
		}
		else
			fxEchoProperty(the, result, &aList, C_NULL, -1, C_NULL);
		fxEcho(the, "</result>");
		fxListLocal(the);
		fxListGlobal(the);
		fxListModules(the);
		fxEchoStop(the);
	}
	else {
		fxEchoStart(the);
		fxEcho(the, "<result line=\"");
		fxEchoInteger(the, index);
		fxEcho(the, "\"># ");
		fxEchoException(the, result);
		fxEcho(the, "</result>");
		fxListLocal(the);
		fxListGlobal(the);
		fxListModules(the);
		fxEchoStop(the);
	}
	mxPop();
	mxPop();
// #else
// 	fxEchoStart(the);
// 	fxEcho(the, "<result line=\"");
// 	fxEchoInteger(the, index);
// 	fxEcho(the, "\">not available</result>");
// 	fxEchoStop(the);
// #endif
}

txU1* fxDebugEvalAtom(txMachine* the, txU1* p, Atom* atom, txString t)
{
	atom->atomSize = (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
	if ((t[0] != p[4]) || (t[1] != p[5]) || (t[2] != p[6]) || (t[3] != p[7]))
		mxUnknownError("invalid buffer");
	return p + 8;
}

void fxDebugEvalBuffer(txMachine* the, txString buffer, txSlot* expression)
{
	mxTry(the) {
		txU1* src = (txU1*)buffer;
		txU1* dst = src;
		Atom atom;
		txScript script;
		txSlot* instance;
		txSlot* code;
		txSlot* home;
		for (;;) {
			txU4 byte = 0;
			txU1 c = *src++;
			if (c == 0)
				break;
			if (!fxParseHex(c, &byte))
				mxUnknownError("invalid buffer");
			c = *src++;
			if (!fxParseHex(c, &byte))
				mxUnknownError("invalid buffer");
			*dst++ = (txU1)byte;
		}
		src = fxDebugEvalAtom(the, (txU1*)buffer, &atom, "XS_B");
		if (atom.atomSize != (txSize)(dst - (txU1*)buffer)) mxUnknownError("invalid buffer");
		src = fxDebugEvalAtom(the, src, &atom, "VERS");
		if (atom.atomSize != 12) mxUnknownError("invalid buffer");
		if (src[0] != XS_MAJOR_VERSION) mxUnknownError("invalid buffer");
		if (src[1] != XS_MINOR_VERSION) mxUnknownError("invalid buffer");
		src = fxDebugEvalAtom(the, src + 4, &atom, "SYMB");
		script.symbolsSize = atom.atomSize - sizeof(atom);
		script.symbolsBuffer = (txByte*)src;
		src = fxDebugEvalAtom(the, src + script.symbolsSize, &atom, "CODE");
		script.codeSize = atom.atomSize - sizeof(atom);
		script.codeBuffer = (txByte*)src;
		fxRemapScript(the, &script);
		mxPop();
		mxPush(mxFunctionPrototype);
		instance = fxNewFunctionInstance(the, XS_NO_ID);
		code = instance->next;
		code->value.code.address = fxNewChunk(the, script.codeSize);
		c_memcpy(code->value.code.address, script.codeBuffer, script.codeSize);
		code->kind = XS_CODE_KIND;
		home = mxFunctionInstanceHome(instance);
		home->ID = fxGenerateProfileID(the);
		mxPullSlot(expression);
	}
	mxCatch(the) {
		mxPush(mxException);
		mxException = mxUndefined;
		mxPullSlot(expression);
	}
}

txBoolean fxDebugEvalExpression(txMachine* the, txSlot* frame, txSlot* expression, txSlot* result)
{
	txBoolean success = 0;
	if (mxIsFunction(expression->value.reference)) {
	// #if mxDebugEval
		
		txSlot* scope = scope;
		if (frame == the->frame)
			scope = the->scope;
		else {
			txSlot* current = the->frame;
			while (current->next != frame)
				current = current->next;
			if (current)
				scope = current->value.frame.scope;
			else
				scope = C_NULL;
		}
	
		txSlot* _this = frame + 4;
		txSlot* environment = mxFrameToEnvironment(frame);
		txSlot* function = frame + 3;
		txSlot* home = (function->kind == XS_REFERENCE_KIND) ? mxFunctionInstanceHome(function->value.reference) : C_NULL;
		txSlot* target = frame + 2;
		txSlot* closures;
		txSlot* property;
	
		the->debugEval = 1;
	
		mxOverflow(-7);
		/* THIS */
		mxPushSlot(_this);
		/* FUNCTION */
		mxPushSlot(function);
		/* TARGET */
		mxPushSlot(target);
		/* RESULT */
		mxPushUndefined();
		/* FRAME */
		(--the->stack)->next = the->frame;
		the->stack->ID = XS_NO_ID;
		the->stack->flag = XS_C_FLAG | (frame->flag & (XS_STRICT_FLAG | XS_FIELD_FLAG));
		the->stack->kind = XS_FRAME_KIND;
		the->stack->value.frame.code = the->code;
		the->stack->value.frame.scope = the->scope;
		the->frame = the->stack;
		/* COUNT */
		mxPushInteger(0);
		/* ENVIRONMENT */
		mxPushUndefined();
		closures = fxNewEnvironmentInstance(the, C_NULL);
		if (scope) {
			txSlot* local = environment;
			txID id;
			property = closures->next;
			while (local > scope) {
				local--;
				id = local->ID;
				if ((0 < id) && (id < the->keyCount)) {
					property = fxNextSlotProperty(the, property, local, id, local->flag);
				}
			}
		}
		the->scope = the->stack;
		the->code = C_NULL;
				
		property = mxFunctionInstanceCode(expression->value.reference);
		property->value.code.closures = closures;
		property = mxFunctionInstanceHome(expression->value.reference);
		if (home) {
			property->value.home.object = home->value.home.object;
			property->value.home.module = home->value.home.module;
		}
		if (property->value.home.module == C_NULL)
			property->value.home.module = mxProgram.value.reference;
	
		{
			mxTry(the) {
				/* THIS */
				mxPushSlot(mxThis);
				the->stack->ID = XS_NO_ID;
				/* FUNCTION */
				mxPushSlot(expression);
				/* TARGET */
				mxPushSlot(mxTarget);
				/* RESULT */
				mxPushUndefined();
				/* FRAME */
				mxPushUninitialized();
				/* COUNT */
				mxPushUninitialized();
				mxRunCount(0);
				mxPullSlot(result);
				success = 1;
			}
			mxCatch(the) {
				mxPush(mxException);
				mxException = mxUndefined;
				mxPullSlot(result);
			}
		}
	
		property = mxFunctionInstanceCode(expression->value.reference);
		property->value.code.closures = C_NULL;
		property = mxFunctionInstanceHome(expression->value.reference);
		property->value.home.object = C_NULL;
		property->value.home.module = C_NULL;
		
		if (scope) {
			txSlot* local = environment;
			txID id;
			property = closures->next->next;
			while (local > scope) {
				local--;
				id = local->ID;
				if ((0 < id) && (id < the->keyCount)) {
					if (property->kind != XS_CLOSURE_KIND) {
						local->kind = property->kind;
						local->value = property->value;
					}
					property = property->next;
				}
			}		
		}
	
		fxEndHost(the);
	
		the->debugEval = 0;
	// #endif
	}
	else {
		mxPushSlot(expression);
		mxPullSlot(result);
	}
	return success;
}

#if MODDEF_XS_XSBUG_HOOKS
void fxDebugImport(txMachine* the, txSlot* module, txString path)
{
	if (!fxIsConnected(the))
		return;
	fxEchoStart(the);
	fxEcho(the, "<import path=\"");
	fxEcho(the, path + 8);
	fxEcho(the, "\"/>");
	fxEchoStop(the);
	the->debugExit = 0;
	the->debugModule = module;
	while (fxIsConnected(the)) {
		fxReceive(the);
		fxDebugParse(the);
		if ((the->debugState == XS_LF_STATE) && (the->debugExit > 1))
			break;
	}
    if (the->debugTag == XS_MODULE_TAG){
        mxRunCount(6);
        mxPop();
    }
}
#endif

void fxDebugLine(txMachine* the, txID path, txInteger line, txID function)
{
	txSlot* breakpoint = C_NULL;
	breakpoint = mxBreakpoints.value.list.first;
	while (breakpoint) {
		if (breakpoint->value.breakpoint.line == line) {
			if (breakpoint->ID == path)
				break;
		}
		else if (breakpoint->value.breakpoint.line == 0) {
			if (breakpoint->ID == function)
				break;
		}
		breakpoint = breakpoint->next;
	}
	if (breakpoint) {
		txSlot* instance = breakpoint->value.breakpoint.info;
		if (instance) {
			txSlot* property = instance->next;
			if (!mxIsUndefined(property)) {
				txSlot* result;
				txBoolean skip = 1;
				mxTemporary(result);
				if (fxDebugEvalExpression(the, the->frame, property, result)) {
					mxPushSlot(result);
					skip = (fxRunTest(the)) ? 0 : 1;
				}
				else {
					fxEchoStart(the);
					fxEcho(the, "<log");
					fxEchoPathLine(the, fxGetKeyName(the, path), line);
					fxEcho(the, "># ");
					fxEchoException(the, result);
					fxEcho(the, "\n</log>");
					fxEchoStop(the);
				}
				mxPop();
				if (skip)
					breakpoint = C_NULL;
			}
			if (breakpoint) {
				property = property->next;
				if (!mxIsUndefined(property)) {
					txInteger offset = property->value.dataView.offset + 1;
					txInteger size = property->value.dataView.size;
					txBoolean skip = 1;
					switch (property->ID) {
					case XS_CODE_EQUAL: if (offset == size) skip = 0; break;
					case XS_CODE_LESS: if (offset < size) skip = 0; break;
					case XS_CODE_LESS_EQUAL: if (offset <= size) skip = 0; break;
					case XS_CODE_MODULO: if ((offset % size) == 0) skip = 0; break;
					case XS_CODE_MORE: if (offset > size) skip = 0; break;
					case XS_CODE_MORE_EQUAL: if (offset >= size) skip = 0; break;
					}
					property->value.dataView.offset = offset;
					if (skip)
						breakpoint = C_NULL;
				}
			}
			if (breakpoint) {
				property = property->next;
				if (!mxIsUndefined(property)) {
					txSlot* result;
					mxTemporary(result);
					if (fxDebugEvalExpression(the, the->frame, property, result)) {
						fxToString(the, result);
						fxEchoStart(the);
						fxEcho(the, "<log");
						fxEchoPathLine(the, fxGetKeyName(the, path), line);
						fxEcho(the, ">");
						fxEchoString(the, result->value.string);
						fxEcho(the, "\n</log>");
						fxEchoStop(the);
					}
					else {
						fxEchoStart(the);
						fxEcho(the, "<log");
						fxEchoPathLine(the, fxGetKeyName(the, path), line);
						fxEcho(the, "># ");
						fxEchoException(the, result);
						fxEcho(the, "\n</log>");
						fxEchoStop(the);
					}
					mxPop();
					breakpoint = C_NULL;
				}
			}
		}
	}
	if (breakpoint)
		fxDebugLoop(the, C_NULL, 0, "breakpoint");
	else if ((the->frame->flag & XS_STEP_OVER_FLAG))
		fxDebugLoop(the, C_NULL, 0, "step");
}

void fxDebugLoop(txMachine* the, txString path, txInteger line, txString message)
{
	txSlot* frame;
	if (!fxIsConnected(the))
		return;

#ifdef mxInstrument
	if (the->onBreak)
		(the->onBreak)(the, 1);
#endif
#if defined(mxInstrument) || defined(mxProfile)
	fxSuspendProfiler(the);
#endif

	fxEchoStart(the);
	frame = the->frame;
	do {
		frame->flag &= ~XS_DEBUG_FLAG;
		frame = frame->next;
	} while (frame);
	the->frame->flag |= XS_DEBUG_FLAG;
	fxListFrames(the);
	fxListLocal(the);
	fxListGlobal(the);
	fxListModules(the);
	fxEcho(the, "<break");
	frame = the->frame;
	while (frame && !path) {
		txSlot* environment = mxFrameToEnvironment(frame);
		if (environment->ID != XS_NO_ID) {
			path = fxGetKeyName(the, environment->ID);
			line = environment->value.environment.line;
		}
		frame = frame->next;
	}
	if (path)
		fxEchoPathLine(the, path, line);
	fxEcho(the, "># Break: ");
	if (!c_strcmp(message, "throw"))
		fxEchoException(the, &mxException);
	else
		fxEchoString(the, message);
	fxEcho(the, "!\n</break>");
	fxEchoStop(the);

	the->debugExit = 0;
	the->debugModule = C_NULL;
	while (fxIsConnected(the)) {
		fxReceive(the);
		fxDebugParse(the);
		if ((the->debugState == XS_LF_STATE) && (the->debugExit > 1))
			break;
	}
	mxHostInspectors.value.list.first = C_NULL;
	mxHostInspectors.value.list.last = C_NULL;

#if defined(mxInstrument) || defined(mxProfile)
	fxResumeProfiler(the);
#endif
#ifdef mxInstrument
	if (the->onBreak)
		(the->onBreak)(the, 0);
#endif
}

void fxDebugParse(txMachine* the)
{
	txString string = the->debugBuffer;
	txString limit = string + the->debugOffset;
	char c;
	while (string < limit) {
		c = *string++;
		switch (the->debugState) {
		case XS_BODY_STATE:
			if (c == '<') 
				the->debugState = XS_TAG_STATE;
			else if (c == '\r') 
				the->debugState = XS_CR_STATE;
			break;
			
		case XS_CR_STATE:
			if (c == '\n') 
				the->debugState = XS_LF_STATE;
			else
				the->debugState = XS_ERROR_STATE;
			break;
			
		case XS_LF_STATE:
			if (c == '<') 
				the->debugState = XS_TAG_STATE;
			else if (c == '\r') 
				the->debugState = XS_CR_STATE;
			break;
			
		case XS_TAG_STATE:
			if (c == '/')
				the->debugState = XS_END_TAG_STATE;
			else if (c == '!')
				the->debugState = XS_START_CDATA_STATE_1;
			else if (c == '?')
				the->debugState = XS_PROCESSING_INSTRUCTION_STATE;
			else if (mxIsFirstLetter(c)) {
				the->debugState = XS_START_TAG_NAME_STATE;
				the->nameBuffer[0] = c;
				the->nameIndex = 1;
			}
			else
				the->debugState = XS_ERROR_STATE;
			break;
		case XS_START_TAG_NAME_STATE:
			if (mxIsNextLetter(c)) {
				if (the->nameIndex < 255) {
					the->nameBuffer[the->nameIndex] = c;
					the->nameIndex++;
				}
				else
					the->debugState = XS_ERROR_STATE;
				break;
			}
			the->debugState = XS_START_TAG_SPACE_STATE;
			the->nameBuffer[the->nameIndex] = 0;
			fxDebugParseTag(the, the->nameBuffer);
			/* continue */
		case XS_START_TAG_SPACE_STATE:
			if (mxIsFirstLetter(c)) {
				the->debugState = XS_ATTRIBUTE_NAME_STATE;
				the->nameBuffer[0] = c;
				the->nameIndex = 1;
			}
			else if (c == '/')
				the->debugState = XS_EMPTY_TAG_STATE;
			else if (c == '>') {
				the->debugState = XS_BODY_STATE;
				fxDebugPushTag(the);
			}
			else if (!mxIsSpace(c))
				the->debugState = XS_ERROR_STATE;
			break;
			
		case XS_ATTRIBUTE_NAME_STATE:
			if (mxIsNextLetter(c)) {
				if (the->nameIndex < 255) {
					the->nameBuffer[the->nameIndex] = c;
					the->nameIndex++;
				}
				else
					the->debugState = XS_ERROR_STATE;
				break;
			}
			the->nameBuffer[the->nameIndex] = 0;
			if (!c_strcmp(the->nameBuffer, "path")) {
				the->debugAttribute = XS_PATH_ATTRIBUTE;
				the->pathIndex = 0;
			}
			else if (!c_strcmp(the->nameBuffer, "line")) {
				the->debugAttribute = XS_LINE_ATTRIBUTE;
				the->lineValue = 0;
			}
			else if (!c_strcmp(the->nameBuffer, "id")) {
				the->debugAttribute = XS_ID_ATTRIBUTE;
				the->idValue = 0;
			}
			else
				the->debugAttribute = XS_UNKNOWN_ATTRIBUTE;
			/* continue */
		case XS_ATTRIBUTE_SPACE_STATE:
			if (c == '=')
				the->debugState = XS_ATTRIBUTE_EQUAL_STATE;
			else if (!mxIsSpace(c))
				the->debugState = XS_ERROR_STATE;
			break;
		case XS_ATTRIBUTE_EQUAL_STATE:
			if (c == '"')
				the->debugState = XS_ATTRIBUTE_VALUE_STATE;
			else if (!mxIsSpace(c))
				the->debugState = XS_ERROR_STATE;
			break;
		case XS_ATTRIBUTE_VALUE_STATE:
			if (the->debugAttribute == XS_PATH_ATTRIBUTE) {
				if (the->pathIndex == the->pathCount) {
					the->pathCount += 256;
					the->pathValue = c_realloc(the->pathValue, the->pathCount);
					if (!the->pathValue)
						fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
				}
				if (c == '"') {
					the->debugState = XS_START_TAG_SPACE_STATE;
					the->pathValue[the->pathIndex] = 0;
				}
				else {
					the->pathValue[the->pathIndex] = c;
					the->pathIndex++;
				}
			}
			else if (the->debugAttribute == XS_LINE_ATTRIBUTE) {
				if (c == '"')
					the->debugState = XS_START_TAG_SPACE_STATE;
				else if (('0' <= c) && (c <= '9'))
					the->lineValue = (the->lineValue * 10) + (c - '0');
				else
					the->debugState = XS_ERROR_STATE;
			}
			else if (the->debugAttribute == XS_ID_ATTRIBUTE) {
				if (c == '"')
					the->debugState = XS_START_TAG_SPACE_STATE;
				else if (('0' <= c) && (c <= '9'))
					the->idValue = (the->idValue << 4) | (c - '0');
				else if (('A' <= c) && (c <= 'F'))
					the->idValue = (the->idValue << 4) | (c - 'A' + 10);
				else if ((c != '@') || (the->idValue != 0))
					the->debugState = XS_ERROR_STATE;
			}
			else {
				if (c == '"')
					the->debugState = XS_START_TAG_SPACE_STATE;
			}
			break;
			
		case XS_EMPTY_TAG_STATE:
			if (c == '>') {
				the->debugState = XS_BODY_STATE;
				fxDebugPushTag(the);
				fxDebugPopTag(the);
			}
			else
				the->debugState = XS_ERROR_STATE;
			break;
			
		case XS_END_TAG_STATE:
			if (mxIsFirstLetter(c))	{
				the->debugState = XS_END_TAG_NAME_STATE;
				the->nameBuffer[0] = c;
				the->nameIndex = 1;
			}
			else
				the->debugState = XS_ERROR_STATE;
			break;
		case XS_END_TAG_NAME_STATE:
			if (mxIsNextLetter(c)) {
				if (the->nameIndex < 255) {
					the->nameBuffer[the->nameIndex] = c;
					the->nameIndex++;
				}
				else 
					the->debugState = XS_ERROR_STATE;
				break;
			}
			the->nameBuffer[the->nameIndex] = 0;
			the->debugState = XS_END_TAG_SPACE_STATE;
			fxDebugParseTag(the, the->nameBuffer);
			/* continue */
		case XS_END_TAG_SPACE_STATE:
			if (c == '>') {
				the->debugState = XS_BODY_STATE;
				fxDebugPopTag(the);
			}
			else if (!mxIsSpace(c))
				the->debugState = XS_ERROR_STATE;
			break;

		case XS_PROCESSING_INSTRUCTION_STATE:
			if (c == '?')
				the->debugState = XS_PROCESSING_INSTRUCTION_SPACE_STATE;
			break;
		case XS_PROCESSING_INSTRUCTION_SPACE_STATE:
			if (c == '>')
				the->debugState = XS_BODY_STATE;
			else
				the->debugState = XS_ERROR_STATE;
			break;
			
		case XS_START_CDATA_STATE_1:
			if (c == '[') 
				the->debugState = XS_START_CDATA_STATE_2;
			else
				the->debugState = XS_ERROR_STATE;
			break;
		case XS_START_CDATA_STATE_2:
			if (c == 'C') 
				the->debugState = XS_START_CDATA_STATE_3;
			else
				the->debugState = XS_ERROR_STATE;
			break;
		case XS_START_CDATA_STATE_3:
			if (c == 'D') 
				the->debugState = XS_START_CDATA_STATE_4;
			else
				the->debugState = XS_ERROR_STATE;
			break;
		case XS_START_CDATA_STATE_4:
			if (c == 'A') 
				the->debugState = XS_START_CDATA_STATE_5;
			else
				the->debugState = XS_ERROR_STATE;
			break;
		case XS_START_CDATA_STATE_5:
			if (c == 'T') 
				the->debugState = XS_START_CDATA_STATE_6;
			else
				the->debugState = XS_ERROR_STATE;
			break;
		case XS_START_CDATA_STATE_6:
			if (c == 'A') 
				the->debugState = XS_START_CDATA_STATE_7;
			else
				the->debugState = XS_ERROR_STATE;
			break;
		case XS_START_CDATA_STATE_7:
			if (c == '[')
				the->debugState = XS_CDATA_STATE;
			else
				the->debugState = XS_ERROR_STATE;
			break;
		case XS_CDATA_STATE:
			if (c == ']') 
				the->debugState = XS_END_CDATA_STATE_1;
			else if (c == 0) {
				fxDebugScriptCDATA(the, 0xF4);
				fxDebugScriptCDATA(the, 0x90);
				fxDebugScriptCDATA(the, 0x80);
				fxDebugScriptCDATA(the, 0x80);
			}
			else
				fxDebugScriptCDATA(the, c);
			break;
		case XS_END_CDATA_STATE_1:
			if (c == ']') 
				the->debugState = XS_END_CDATA_STATE_2;
			else {
				fxDebugScriptCDATA(the, ']');
				fxDebugScriptCDATA(the, c);
				the->debugState = XS_CDATA_STATE;
			}
			break;
		case XS_END_CDATA_STATE_2:
			if (c == '>') {
				fxDebugScriptCDATA(the, 0);
				the->debugState = XS_BODY_STATE;
			}
			else {
				fxDebugScriptCDATA(the, ']');
				fxDebugScriptCDATA(the, ']');
				fxDebugScriptCDATA(the, c);
				the->debugState = XS_CDATA_STATE;
			}
			break;
			
		case XS_ERROR_STATE:
// 			fprintf(stderr, "\nERROR: %c\n", c);
			break;
		}
	}
	the->debugOffset = 0;
}

void fxDebugParseTag(txMachine* the, txString name)
{
	if (!c_strcmp(name, "abort"))
		the->debugTag = XS_ABORT_TAG;
	else if (!c_strcmp(name, "breakpoint")) {
		the->debugTag = XS_BREAKPOINT_TAG;
		the->idValue = 0;
	}
	else if (!c_strcmp(name, "breakpoint-condition"))
		the->debugTag = XS_BREAKPOINT_CONDITION_TAG;
	else if (!c_strcmp(name, "breakpoint-hit-count"))
		the->debugTag = XS_BREAKPOINT_HIT_COUNT_TAG;
	else if (!c_strcmp(name, "breakpoint-trace"))
		the->debugTag = XS_BREAKPOINT_TRACE_TAG;
	else if (!c_strcmp(name, "clear-all-breakpoints"))
		the->debugTag = XS_CLEAR_ALL_BREAKPOINTS_TAG;
	else if (!c_strcmp(name, "clear-breakpoint"))
		the->debugTag = XS_CLEAR_BREAKPOINTS_TAG;
	else if (!c_strcmp(name, "eval"))
		the->debugTag = XS_EVAL_TAG;
	else if (!c_strcmp(name, "go"))
		the->debugTag = XS_GO_TAG;
	else if (!c_strcmp(name, "logout"))
		the->debugTag = XS_LOGOUT_TAG;
	else if (!c_strcmp(name, "select"))
		the->debugTag = XS_SELECT_TAG;
	else if (!c_strcmp(name, "set-all-breakpoints"))
		the->debugTag = XS_SET_ALL_BREAKPOINTS_TAG;
	else if (!c_strcmp(name, "set-breakpoint")) {
		the->debugTag = XS_SET_BREAKPOINT_TAG;
		the->idValue = 0;
	}
	else if (!c_strcmp(name, "start-profiling"))
		the->debugTag = XS_START_PROFILING_TAG;
	else if (!c_strcmp(name, "step"))
		the->debugTag = XS_STEP_TAG;
	else if (!c_strcmp(name, "step-inside"))
		the->debugTag = XS_STEP_INSIDE_TAG;
	else if (!c_strcmp(name, "step-outside"))
		the->debugTag = XS_STEP_OUTSIDE_TAG;
	else if (!c_strcmp(name, "stop-profiling"))
		the->debugTag = XS_STOP_PROFILING_TAG;
	else if (!c_strcmp(name, "toggle"))
		the->debugTag = XS_TOGGLE_TAG;
#if MODDEF_XS_XSBUG_HOOKS
	else if (!c_strcmp(name, "import"))
		the->debugTag = XS_IMPORT_TAG;
	else if (!c_strcmp(name, "module"))
		the->debugTag = XS_MODULE_TAG;
	else if (!c_strcmp(name, "script"))
		the->debugTag = XS_SCRIPT_TAG;
#endif
	else
		the->debugTag = XS_UNKNOWN_TAG;
}

void fxDebugPopTag(txMachine* the)
{
	switch (the->debugTag) {
	case XS_ABORT_TAG:
		the->debugExit |= 2;
		break;
	case XS_BREAKPOINT_TAG:
		mxPop();
		break;
	case XS_BREAKPOINT_CONDITION_TAG:
		break;
	case XS_BREAKPOINT_HIT_COUNT_TAG:
		break;
	case XS_BREAKPOINT_TRACE_TAG:
		break;
	case XS_CLEAR_ALL_BREAKPOINTS_TAG:
		the->debugExit |= 1;
		break;
	case XS_CLEAR_BREAKPOINTS_TAG:
		the->debugExit |= 1;
		break;
	case XS_EVAL_TAG:
		break;
	case XS_GO_TAG:
		the->debugExit |= 2;
		break;
	case XS_LOGOUT_TAG:
		the->debugExit |= 2;
		break;
	case XS_SELECT_TAG:
		break;
	case XS_SET_ALL_BREAKPOINTS_TAG:
		the->debugExit |= 1;
		break;
	case XS_SET_BREAKPOINT_TAG:
		mxPop();
		the->debugExit |= 1;
		break;
	case XS_START_PROFILING_TAG:
		the->debugExit |= 1;
		break;
	case XS_STEP_TAG:
		the->debugExit |= 2;
		break;
	case XS_STEP_INSIDE_TAG:
		the->debugExit |= 2;
		break;
	case XS_STEP_OUTSIDE_TAG:
		the->debugExit |= 2;
		break;
	case XS_STOP_PROFILING_TAG:
		the->debugExit |= 1;
		break;
	case XS_TOGGLE_TAG:
		break;
#if MODDEF_XS_XSBUG_HOOKS
	case XS_IMPORT_TAG:
		the->debugExit |= 2;
		break;
	case XS_MODULE_TAG:
	case XS_SCRIPT_TAG:
		the->debugExit |= 2;
		break;
#endif
	}
}

void fxDebugPushTag(txMachine* the)
{
	switch (the->debugTag) {
	case XS_ABORT_TAG:
		fxLogout(the);
		fxGo(the);
		fxAbort(the, XS_DEBUGGER_EXIT);
		break;
	case XS_BREAKPOINT_TAG:
		fxSetBreakpoint(the, the->pathValue, the->lineValue, the->idValue);
		break;
	case XS_BREAKPOINT_CONDITION_TAG:
		fxSetBreakpointCondition(the, the->stack, the->pathValue);
		break;
	case XS_BREAKPOINT_HIT_COUNT_TAG:
		fxSetBreakpointHitCount(the, the->stack, the->pathValue);
		break;
	case XS_BREAKPOINT_TRACE_TAG:
		fxSetBreakpointTrace(the, the->stack, the->pathValue);
		break;
	case XS_CLEAR_ALL_BREAKPOINTS_TAG:
		fxClearAllBreakpoints(the);
		break;
	case XS_CLEAR_BREAKPOINTS_TAG:
		fxClearBreakpoint(the, the->pathValue, the->lineValue, the->idValue);
		break;
	case XS_EVAL_TAG:
		fxDebugEval(the, (txSlot*)the->idValue, the->pathValue, the->lineValue);
		break;
	case XS_GO_TAG:
		fxGo(the);
		break;
	case XS_LOGOUT_TAG:
		fxLogout(the);
		fxGo(the);
		break;
	case XS_SELECT_TAG:
		fxSelect(the, (txSlot*)the->idValue);
		fxEchoStart(the);
		fxListLocal(the);
		fxListGlobal(the);
		fxListModules(the);
		fxEchoStop(the);
		break;
	case XS_SET_ALL_BREAKPOINTS_TAG:
		break;
	case XS_SET_BREAKPOINT_TAG:
		fxSetBreakpoint(the, the->pathValue, the->lineValue, the->idValue);
		break;
	case XS_START_PROFILING_TAG:
		fxStartProfiling(the);
		break;
	case XS_STEP_TAG:
		fxStep(the);
		break;
	case XS_STEP_INSIDE_TAG:
		fxStepInside(the);
		break;
	case XS_STEP_OUTSIDE_TAG:
		fxStepOutside(the);
		break;
	case XS_STOP_PROFILING_TAG:
		fxStopProfiling(the, C_NULL);
		break;
	case XS_TOGGLE_TAG:
		fxToggle(the, (txSlot*)the->idValue);
		fxEchoStart(the);
		fxListLocal(the);
		fxListGlobal(the);
		fxListModules(the);
		fxEchoStop(the);
		break;
#if MODDEF_XS_XSBUG_HOOKS
	case XS_IMPORT_TAG: 
		/* THIS */
		mxPushUndefined();
		/* FUNCTION */
		mxPush(mxGlobal);
		mxGetID(fxID(the, "<xsbug:import>"));
        mxCall();
		mxPushStringC("xsbug://");
		fxConcatStringC(the, the->stack, the->pathValue);
		mxPushInteger(the->lineValue);
		break;
	case XS_MODULE_TAG:
	case XS_SCRIPT_TAG:
		fxCollectGarbage(the);
		/* THIS */
		mxPushUndefined();
		/* FUNCTION */
		mxPush(mxGlobal);
		if (the->debugTag == XS_MODULE_TAG)
			mxGetID(fxID(the, "<xsbug:module>"));
		else
			mxGetID(mxID(__xsbug_script_));
		mxCall();
		if (the->debugModule)
			mxPushSlot(the->debugModule);
		else
			mxPushUndefined();
		mxPushStringC(the->pathValue);
		mxPushInteger(the->lineValue);
		mxPushUndefined();
		fxStringBuffer(the, the->stack, C_NULL, 256);
		mxPushInteger(256);
		mxPushInteger(0);
		break;
#endif
	}
}

void fxDebugScriptCDATA(txMachine* the, char c)
{
#if MODDEF_XS_XSBUG_HOOKS
	if ((the->debugTag == XS_MODULE_TAG) || (the->debugTag == XS_SCRIPT_TAG)) {
		txString string = the->stack[2].value.string;
		txInteger size = the->stack[1].value.integer;
		txInteger offset = the->stack[0].value.integer;
		if (offset == size) {
			txString result = (txString)fxRenewChunk(the, string, size + 256);
			if (!result) {
				result = (txString)fxNewChunk(the, size + 256);
				string = the->stack[2].value.string;
				c_memcpy(result, string, size);
			}
			the->stack[2].value.string = string = result;
			the->stack[1].value.integer = size + 256;
		}
		string[offset++] = c;
		the->stack[0].value.integer = offset;
	}
#endif
}

void fxDebugThrow(txMachine* the, txString path, txInteger line, txString message)
{
	if (the->debugEval)
		return;
	if (fxIsConnected(the) && (the->breakOnExceptionsFlag))
		fxDebugLoop(the, path, line, message);
	else {
		txSlot* frame = the->frame;
		while (frame && !path) {
			txSlot* environment = mxFrameToEnvironment(frame);
			if (environment->ID != XS_NO_ID) {
				path = fxGetKeyName(the, environment->ID);
				line = environment->value.environment.line;
			}
			frame = frame->next;
		}
		fxReportException(the, path, line, "%s", message);
	}
}

void fxEcho(txMachine* the, txString theString)
{
	txInteger srcLength = mxStringLength(theString);
	txInteger dstLength = sizeof(the->echoBuffer) - the->echoOffset;
	while (srcLength > dstLength) {
		c_memcpy(the->echoBuffer + the->echoOffset, theString, dstLength);
		theString += dstLength;
		srcLength -= dstLength;
		the->echoOffset = sizeof(the->echoBuffer);
		fxSend(the, 1);
		the->echoOffset = 0;
		dstLength = sizeof(the->echoBuffer);
	}
	c_memcpy(the->echoBuffer + the->echoOffset, theString, srcLength);
	the->echoOffset += srcLength;
}

void fxEchoAddress(txMachine* the, txSlot* theSlot)
{
	uintptr_t aValue = (uintptr_t)theSlot;
	int aShift;

	fxEcho(the, " value=\"@");
	aShift = (8 * sizeof(aValue)) - 4;
	while (aShift >= 0) {
		fxEchoCharacter(the, c_read8(gxHexaDigits + ((aValue >> aShift) & 0x0F)));
		aShift -= 4;
	}
	fxEcho(the, "\"");
}

void fxEchoArrayBuffer(txMachine* the, txSlot* theInstance, txInspectorNameList* theList)
{
	txSlot* arrayBuffer = theInstance->next;
	txSlot* bufferInfo = arrayBuffer->next;
	txU1* address = (txU1*)(arrayBuffer->value.arrayBuffer.address);
	txInteger size = bufferInfo->value.bufferInfo.length;
	txInteger offset = 0, index;
	if (size > 1024)
		size = 1024;
	while (offset < size) {
		fxEcho(the, "<property");
		fxEchoFlags(the, " ", 0);
		fxEcho(the, " name=\"");
		fxEchoCharacter(the, c_read8(gxHexaDigits + ((offset >> 12) & 0xF)));
		fxEchoCharacter(the, c_read8(gxHexaDigits + ((offset >> 8) & 0xF)));
		fxEchoCharacter(the, c_read8(gxHexaDigits + ((offset >> 4) & 0xF)));
		fxEchoCharacter(the, c_read8(gxHexaDigits + (offset & 0xF)));
		fxEcho(the, "\"");
		fxEcho(the, " value=\"");
		index = 0;
		while (index < 16) {
			txByte byte = *address++;
			fxEchoCharacter(the, c_read8(gxHexaDigits + ((byte >> 4) & 0xF)));
			fxEchoCharacter(the, c_read8(gxHexaDigits + (byte & 0xF)));
			fxEcho(the, " ");
			index++;
			offset++;
			if (offset == size)
				break;
		}
		address -= index;
		offset -= index;
		while (index < 16) {
			fxEcho(the, "   ");
			index++;
		}		
		index = 0;
		while (index < 16) {
			txByte byte = *address++;
			if ((32 <= byte) && (byte < 127))
				fxEchoCharacter(the, byte);
			else
				fxEchoCharacter(the, '.');
			index++;
			offset++;
			if (offset == size)
				break;
		}
		fxEcho(the, " \"/>");
	}
}

void fxEchoBigInt(txMachine* the, txBigInt* bigint)
{
	int i = bigint->size - 1;
	if (i < 0) {
		fxEchoCharacter(the, 'N');
		fxEchoCharacter(the, 'a');
		fxEchoCharacter(the, 'N');
	}
	else {
		int echo = 0;
		if (bigint->sign)
			fxEchoCharacter(the, '-');
		fxEchoCharacter(the, '0');
		fxEchoCharacter(the, 'x');
		while (i >= 0) {
			txU4 value = bigint->data[i];
			txU4 mask = 0xF;
			int shift = 28;
			while (shift >= 0) {
				char digit = c_read8(gxHexaDigits + ((value & (mask << shift)) >> shift));
				if (echo || (digit != '0')) {
					echo = 1;
					fxEchoCharacter(the, digit);
				}
				shift -= 4;
			}
			i--;
		}
		if (!echo)
			fxEchoCharacter(the, '0');
		fxEchoCharacter(the, 'n');
	}
}

void fxEchoCharacter(txMachine* the, char theCharacter)
{
	char c[2];
	c[0] = theCharacter;
	c[1] = 0;
	fxEchoString(the, c);
}

void fxEchoException(txMachine* the, txSlot* exception)
{
	switch (exception->kind) {
	case XS_REFERENCE_KIND: {
		txSlot* instance = exception->value.reference;
		txSlot* internal = instance->next;
		if (internal && (internal->kind == XS_ERROR_KIND)) {
			switch (internal->value.error.which) {
			case XS_UNKNOWN_ERROR: fxEcho(the, "Error"); break;
			case XS_EVAL_ERROR: fxEcho(the, "EvalError"); break;
			case XS_RANGE_ERROR: fxEcho(the, "RangeError"); break;
			case XS_REFERENCE_ERROR: fxEcho(the, "ReferenceError"); break;
			case XS_SYNTAX_ERROR: fxEcho(the, "SyntaxError"); break;
			case XS_TYPE_ERROR: fxEcho(the, "TypeError"); break;
			case XS_URI_ERROR: fxEcho(the, "URIError"); break;
			case XS_AGGREGATE_ERROR: fxEcho(the, "AggregateError"); break;
			case XS_SUPPRESSED_ERROR: fxEcho(the, "SuppressedError"); break;
			}
			fxEcho(the, ": ");
			internal = internal->next;
			if (internal && ((internal->kind == XS_STRING_KIND) || (internal->kind == XS_STRING_X_KIND))) {
				fxEchoString(the, internal->value.string);
			}
		}
		else {
			fxEcho(the, "(");
			if (instance->flag & XS_CAN_CALL_FLAG)
				fxEcho(the, mxFunctionString.value.string);
			else
				fxEcho(the, mxObjectString.value.string);
			fxEcho(the, ")");
		}
		} break;
	case XS_UNDEFINED_KIND:
		fxEcho(the, "undefined");
		break;
	case XS_NULL_KIND:
		fxEcho(the, "null");
		break;
	case XS_BOOLEAN_KIND:
		if (exception->value.boolean)
			fxEcho(the, "true");
		else
			fxEcho(the, "false");
		break;
	case XS_INTEGER_KIND:
		fxEchoInteger(the, exception->value.integer);
		break;
	case XS_NUMBER_KIND:
		fxEchoNumber(the, exception->value.number);
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		fxEchoString(the, exception->value.string);
		break;
	case XS_SYMBOL_KIND:
		fxEcho(the, "Symbol(");
		fxEchoString(the, fxGetKeyString(the, exception->value.symbol, C_NULL));
		fxEcho(the, ")\"/>");
		break;
	case XS_BIGINT_KIND:
	case XS_BIGINT_X_KIND:
		fxEchoBigInt(the, &exception->value.bigint);
		break;
	}
}

void fxEchoFlags(txMachine* the, txString state, txFlag flag)
{
	fxEcho(the, " flags=\"");
	fxEcho(the, state);
	if (flag & XS_DONT_DELETE_FLAG)
		fxEcho(the, "C");
	else
		fxEcho(the, "c");
	if (flag & XS_DONT_ENUM_FLAG)
		fxEcho(the, "E");
	else
		fxEcho(the, "e");
	if (flag & XS_DONT_SET_FLAG)
		fxEcho(the, "W");
	else
		fxEcho(the, "w");
	if (flag & XS_INSPECTOR_FLAG)
		fxEcho(the, "I");
	else if (flag & XS_MARK_FLAG)
		fxEcho(the, "M");
	else
		fxEcho(the, "_");
	fxEcho(the, "\"");
}

void fxEchoFormat(txMachine* the, txString theFormat, c_va_list theArguments)
{
	char *p, c;

	p = theFormat;
	while ((c = c_read8(p++))) {
		if (c != '%')
			fxEchoCharacter(the, c);
		else {
			if (c_strncmp(p, "c", 1) == 0) {
				fxEchoCharacter(the, c_va_arg(theArguments, int));
				p++;
			}
			else if (c_strncmp(p, "hd", 2) == 0) {
				fxEchoInteger(the, c_va_arg(theArguments, int));
				p += 2;
			}
			else if (c_strncmp(p, "d", 1) == 0) {
				fxEchoInteger(the, c_va_arg(theArguments, int));
				p++;
			}
			else if (c_strncmp(p, "ld", 2) == 0) {
				fxEchoInteger(the, c_va_arg(theArguments, long));
				p += 2;
			}
			else if (c_strncmp(p, "g", 1) == 0) {
				fxEchoNumber(the, c_va_arg(theArguments, double));
				p++;
			}
			else if (c_strncmp(p, "s", 1) == 0) {
				char *s = c_va_arg(theArguments, char *);
				fxEchoString(the, s);
				p++;
			}
			else {
				fxEchoCharacter(the, c);
				p++;
			}
		}
	}
}

void fxEchoFrameName(txMachine* the, txSlot* theFrame)
{
	char buffer[128] = "";
	fxBufferFrameName(the, buffer, sizeof(buffer), theFrame, "");
	fxEcho(the, buffer);
}

void fxEchoFramePathLine(txMachine* the, txSlot* theFrame)
{
	if (theFrame) {
		txSlot* environment = mxFrameToEnvironment(theFrame);
		if (environment->ID != XS_NO_ID)
			fxEchoPathLine(the, fxGetKeyName(the, environment->ID), environment->value.environment.line);
	}
}

void fxEchoInteger(txMachine* the, txInteger theInteger)
{
	char aBuffer[256];

	fxIntegerToString(the->dtoa, theInteger, aBuffer, sizeof(aBuffer));
	fxEcho(the, aBuffer);
}

void fxEchoInstance(txMachine* the, txSlot* theInstance, txInspectorNameList* theList)
{
	txSlot* aParent;
	txSlot* aProperty;
	txSlot* aSlot;
	txInteger anIndex;

#if mxAliasInstance
	if (theInstance->ID) {
		txSlot* aliasInstance = the->aliasArray[theInstance->ID];
		if (aliasInstance)
			theInstance = aliasInstance;
	}
#endif
	aParent = fxGetPrototype(the, theInstance);
	if (aParent)
		fxEchoPropertyInstance(the, theList, "(..)", -1, C_NULL, XS_NO_ID, theInstance->flag & XS_MARK_FLAG, aParent);
	aProperty = theInstance->next;
	if (aProperty && (aProperty->flag & XS_INTERNAL_FLAG) && (aProperty->ID == XS_ARRAY_BEHAVIOR)) {
		fxEchoProperty(the, aProperty, theList, "(array)", -1, C_NULL);
	}
	else if (aProperty && (aProperty->flag & XS_INTERNAL_FLAG)) {
		switch (aProperty->kind) {
		case XS_CALLBACK_KIND:
		case XS_CALLBACK_X_KIND:
		case XS_CODE_KIND:
		case XS_CODE_X_KIND:
			if (aProperty->value.code.closures)
				fxEchoPropertyInstance(the, theList, "(closures)", -1, C_NULL, XS_NO_ID, aProperty->flag & (XS_GET_ONLY | XS_INSPECTOR_FLAG | XS_MARK_FLAG), aProperty->value.code.closures);
			fxEchoProperty(the, aProperty, theList, "(function)", -1, C_NULL);
			aProperty = aProperty->next;
			if ((aProperty->kind == XS_HOME_KIND) && (aProperty->value.home.object))
				fxEchoPropertyInstance(the, theList, "(home)", -1, C_NULL, XS_NO_ID, aProperty->flag, aProperty->value.home.object);
			aProperty = aProperty->next;
			break;
		case XS_ARRAY_BUFFER_KIND:
			fxEchoArrayBuffer(the, theInstance, theList);
			aProperty = aProperty->next;
			fxEchoProperty(the, aProperty, theList, "(buffer)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_STRING_KIND:
		case XS_STRING_X_KIND:
			fxEchoProperty(the, aProperty, theList, "(string)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_BOOLEAN_KIND:
			fxEchoProperty(the, aProperty, theList, "(boolean)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_NUMBER_KIND:
			fxEchoProperty(the, aProperty, theList, "(number)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_BIGINT_KIND:
		case XS_BIGINT_X_KIND:
			fxEchoProperty(the, aProperty, theList, "(bigint)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_DATA_VIEW_KIND:
			fxEchoProperty(the, aProperty, theList, "(view)", -1, C_NULL);
			aProperty = aProperty->next;
			fxEchoProperty(the, aProperty, theList, "(data)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_DATE_KIND:
			fxEchoProperty(the, aProperty, theList, "(date)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_REGEXP_KIND:
			fxEchoProperty(the, aProperty, theList, "(regexp)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_HOST_KIND:
           	fxEchoProperty(the, aProperty, theList, "(host)", -1, C_NULL);
			if (aProperty->value.host.data)
				fxEchoPropertyHost(the, theList, theInstance, aProperty);
			aProperty = aProperty->next;
			break;
		case XS_GLOBAL_KIND:
			aProperty = aProperty->next;
			break;
		case XS_PROMISE_KIND:
           	fxEchoProperty(the, aProperty, theList, "(promise)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_MAP_KIND:
			aProperty = aProperty->next;
			anIndex = 0;
			aSlot = aProperty->value.list.first;
			while (aSlot) {
				if (!(aSlot->flag & XS_DONT_ENUM_FLAG)) {
					fxEchoProperty(the, aSlot, theList, "(", anIndex, ".0)");
					fxEchoProperty(the, aSlot->next, theList, "(", anIndex, ".1)");
				}
				anIndex++;
				aSlot = aSlot->next->next;
			}
			aProperty = aProperty->next;
			break;
		case XS_MODULE_KIND:
			aProperty = aProperty->next;
			fxEchoProperty(the, aProperty, theList, "(export)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_PROGRAM_KIND:
			aSlot = aProperty->value.module.realm;
			fxEchoProperty(the, mxRealmGlobal(aSlot), theList, "(globals)", -1, C_NULL);
			fxEchoProperty(the, mxOwnModules(aSlot), theList, "(modules)", -1, C_NULL);
			break;
		case XS_SET_KIND:
			aProperty = aProperty->next;
			anIndex = 0;
			aSlot = aProperty->value.list.first;
			while (aSlot) {
				if (!(aSlot->flag & XS_DONT_ENUM_FLAG))
					fxEchoProperty(the, aSlot, theList, "(", anIndex, ")");
				anIndex++;
				aSlot = aSlot->next;
			}
			aProperty = aProperty->next;
			break;
		case XS_TYPED_ARRAY_KIND:
			fxEchoTypedArray(the, theInstance, theList);
			aProperty = aProperty->next;
			fxEchoProperty(the, aProperty, theList, "(view)", -1, C_NULL);
			aProperty = aProperty->next;
			fxEchoProperty(the, aProperty, theList, "(buffer)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_PROXY_KIND:
			if (aProperty->value.proxy.target)
				fxEchoPropertyInstance(the, theList, "(target)", -1, C_NULL, XS_NO_ID, aProperty->flag, aProperty->value.proxy.target);
			if (aProperty->value.proxy.handler)
				fxEchoPropertyInstance(the, theList, "(handler)", -1, C_NULL, XS_NO_ID, aProperty->flag, aProperty->value.proxy.handler);
			aProperty = aProperty->next;
			break;
		}
	}
	while (aProperty) {
		if (aProperty->flag & XS_INTERNAL_FLAG) {
			if (aProperty->kind == XS_ARRAY_KIND) {
				txSlot* item = aProperty->value.array.address;
				txIndex c = fxGetIndexSize(the, aProperty), i;
				if (c > 1024)
					c = 1024;
				for (i = 0; i < c; i++) {
					txIndex index = *((txIndex*)item);
					fxEchoProperty(the, item, theList, "[", index, "]");
					item++;
				}
			}
			else if (aProperty->kind == XS_PRIVATE_KIND) {
				txSlot* instanceInspector = fxToInstanceInspector(the, aProperty);
				char buffer[128] = "(";
				txSlot* check = aProperty->value.private.check;
				txSlot* item = aProperty->value.private.first;
				fxBufferFunctionName(the, &buffer[1], sizeof(buffer) - 1, check, ")");
				fxEcho(the, "<property");
				if (instanceInspector) {
					if (instanceInspector->value.instanceInspector.link)
						fxEchoFlags(the, " ", aProperty->flag);
					else
						fxEchoFlags(the, "-", aProperty->flag);
				}
				else
					fxEchoFlags(the, "+", aProperty->flag);
				fxEcho(the, " name=\"");
				fxEchoString(the, buffer);
				fxEcho(the, "\"");
				if (instanceInspector) {
					if (instanceInspector->value.instanceInspector.link) {
						txInspectorNameLink* link = theList->first;
						fxEcho(the, " value=\"");
						while (link) {
							fxEchoPropertyName(the, link->prefix, link->index, link->suffix, link->id);
							if (link == instanceInspector->value.instanceInspector.link)
								break;
							fxEcho(the, ".");
							link = link->next;
						}
						fxEcho(the, "\"/>");
					}
					else {
						txInspectorNameLink link;
						link.previous = theList->last;
						link.next = C_NULL;
						link.prefix = buffer;
						link.index = 0;
						link.suffix = C_NULL;
						link.id = XS_NO_ID;
						if (theList->first)
							theList->last->next = &link;
						else
							theList->first = &link;
						theList->last = &link;
						instanceInspector->value.instanceInspector.link = &link;
						fxEchoAddress(the, aProperty);
						fxEcho(the, ">");
						while (item) {
							fxEchoProperty(the, item, theList, C_NULL, -1, C_NULL);
							item = item->next;
						}
						fxEcho(the, "</property>");
						instanceInspector->value.instanceInspector.link = C_NULL;
						if (link.previous)
							link.previous->next = C_NULL;
						else
							theList->first = C_NULL;
						theList->last = link.previous;
					}
				}
				else {
					fxEchoAddress(the, aProperty);
					fxEcho(the, "/>");
				}
			}
		}
		else {
			fxEchoProperty(the, aProperty, theList, C_NULL, -1, C_NULL);
		}
		aProperty = aProperty->next;
	}
}

void fxEchoModule(txMachine* the, txSlot* module, txInspectorNameList* list)
{
	txSlot* exports = mxModuleExports(module);
	txSlot* instanceInspector = fxToInstanceInspector(the, module);
	txSlot* slot;
	fxEcho(the, "<node");
	if (instanceInspector)
		fxEchoFlags(the, "-", exports->flag);
	else
		fxEchoFlags(the, "+", exports->flag);
	fxEcho(the, " name=\"");
	slot = mxModuleInternal(module);
	fxEcho(the, fxGetKeyName(the, slot->value.module.id));
	fxEcho(the, "\"");
	fxEchoAddress(the, module);
	if (instanceInspector) {
		fxEcho(the, ">");
		fxEchoInstance(the, fxGetInstance(the, exports), list);
		fxEcho(the, "</node>");
	}
	else
		fxEcho(the, "/>");
}

void fxEchoNumber(txMachine* the, txNumber theNumber)
{
	char aBuffer[256];

	fxNumberToString(the->dtoa, theNumber, aBuffer, sizeof(aBuffer), 0, 0);
	fxEcho(the, aBuffer);
}

void fxEchoPathLine(txMachine* the, txString thePath, txInteger theLine)
{
	if (thePath && theLine) {
		fxEcho(the, " path=\"");
		fxEchoString(the, thePath);
		fxEcho(the, "\"");
		fxEcho(the, " line=\"");
		fxEchoInteger(the, theLine);
		fxEcho(the, "\"");
	}
}

void fxEchoProperty(txMachine* the, txSlot* theProperty, txInspectorNameList* theList, txString thePrefix, txIndex theIndex, txString theSuffix)
{
	txID ID = theProperty->ID;
	txFlag flag = theProperty->flag;
	txSlot* instance;
	if ((theProperty->kind == XS_CLOSURE_KIND) || (theProperty->kind == XS_EXPORT_KIND)) {
		theProperty = theProperty->value.closure;
        if (!theProperty)
            return;
#if mxAliasInstance
		if (theProperty->ID) {
			txSlot* slot = the->aliasArray[theProperty->ID];
			if (slot)
				theProperty = slot;
		}
#endif
	}
	if (theProperty->kind == XS_REFERENCE_KIND) {
 		instance = fxGetInstance(the, theProperty);
		if (instance)
			fxEchoPropertyInstance(the, theList, thePrefix, theIndex, theSuffix, ID, flag, instance);
	}
	else if (theProperty->kind == XS_ACCESSOR_KIND) {
		instance = theProperty->value.accessor.getter;
		if (instance)
			fxEchoPropertyInstance(the, theList, thePrefix, theIndex, theSuffix, ID, flag | XS_GETTER_FLAG, instance);
		instance = theProperty->value.accessor.setter;
		if (instance)
			fxEchoPropertyInstance(the, theList, thePrefix, theIndex, theSuffix, ID, flag | XS_SETTER_FLAG, instance);
	}
	else {
		fxEcho(the, "<property");
		fxEchoFlags(the, " ", flag);
		fxEcho(the, " name=\"");
		fxEchoPropertyName(the, thePrefix, theIndex, theSuffix, ID);
		fxEcho(the, "\"");
	
		switch (theProperty->kind) {
		case XS_UNDEFINED_KIND:
			fxEcho(the, " value=\"undefined\"/>");
			break;
		case XS_NULL_KIND:
			fxEcho(the, " value=\"null\"/>");
			break;
		case XS_CALLBACK_KIND:
		case XS_CALLBACK_X_KIND:
			fxEcho(the, " value=\"(C code)\"/>");
			break;
		case XS_CODE_KIND:
		case XS_CODE_X_KIND:
			fxEcho(the, " value=\"(XS code)\"/>");
			break;
	#ifdef mxHostFunctionPrimitive
		case XS_HOST_FUNCTION_KIND:
			fxEcho(the, " value=\"(host function)\"/>");
			break;
	#endif
		case XS_ARRAY_KIND:
			fxEcho(the, " value=\"");
			fxEchoInteger(the, theProperty->value.array.length);
			fxEcho(the, " items\"/>");
			break;
		case XS_BUFFER_INFO_KIND:
			fxEcho(the, " value=\"");
			fxEchoInteger(the, theProperty->value.bufferInfo.length);
			if (theProperty->value.bufferInfo.maxLength >= 0) {
				fxEcho(the, " bytes <= ");
				fxEchoInteger(the, theProperty->value.bufferInfo.maxLength);
			}
			fxEcho(the, " bytes\"/>");
			break;
		case XS_STRING_KIND:
		case XS_STRING_X_KIND:
			fxEcho(the, " value=\"'");
			fxEchoString(the, theProperty->value.string);
			fxEcho(the, "'\"/>");
			break;
		case XS_BOOLEAN_KIND:
			fxEcho(the, " value=\"");
			if (theProperty->value.boolean)
				fxEcho(the, "true");
			else
				fxEcho(the, "false");
			fxEcho(the, "\"/>");
			break;
		case XS_INTEGER_KIND:
			fxEcho(the, " value=\"");
			fxEchoInteger(the, theProperty->value.integer);
			fxEcho(the, "\"/>");
			break;
		case XS_NUMBER_KIND:
			fxEcho(the, " value=\"");
			fxEchoNumber(the, theProperty->value.number);
			fxEcho(the, "\"/>");
			break;
		case XS_BIGINT_KIND:
		case XS_BIGINT_X_KIND:
			fxEcho(the, " value=\"");
			fxEchoBigInt(the, &theProperty->value.bigint);
			fxEcho(the, "\"/>");
			break;
		case XS_DATE_KIND:
			fxEcho(the, " value=\"");
			fxEchoNumber(the, theProperty->value.number);
			fxEcho(the, "\"/>");
			break;
		case XS_REGEXP_KIND:
			fxEcho(the, " value=\"\"/>");
			break;
		case XS_HOST_KIND:
			if (theProperty->value.host.data) {
				if (theProperty->flag & XS_HOST_CHUNK_FLAG)
					fxEcho(the, " value=\"XS data\"/>");
				else
					fxEcho(the, " value=\"C data\"/>");
			}
			else
					fxEcho(the, " value=\"NULL\"/>");
			break;
		case XS_PROMISE_KIND:
			switch (theProperty->value.integer) {
			case mxUndefinedStatus: fxEcho(the, " value=\"?\"/>"); break;
			case mxPendingStatus: fxEcho(the, " value=\"pending\"/>"); break;
			case mxFulfilledStatus: fxEcho(the, " value=\"fulfilled\"/>"); break;
			case mxRejectedStatus: fxEcho(the, " value=\"rejected\"/>"); break;
			}
			break;
		case XS_KEY_KIND:
		case XS_KEY_X_KIND:
			fxEcho(the, " value=\"'");
			fxEchoString(the, theProperty->value.key.string);
			fxEcho(the, "'\"/>");
			break;
		case XS_SYMBOL_KIND:
			fxEcho(the, " value=\"Symbol(");
			fxEchoString(the, fxGetKeyString(the, theProperty->value.symbol, C_NULL));
			fxEcho(the, ")\"/>");
			break;
		case XS_DATA_VIEW_KIND:
			fxEcho(the, " value=\"");
			fxEchoInteger(the, theProperty->value.dataView.offset);
			fxEcho(the, ", ");
			fxEchoInteger(the, fxGetDataViewSize(the, theProperty, theProperty->next));
			fxEcho(the, " bytes\"/>");
			break;
		default:
			fxEcho(the, "/>");
			break;
		}
	}
}

void fxEchoPropertyHost(txMachine* the, txInspectorNameList* theList, txSlot* theInstance, txSlot* theHost)
{
	txSlot* instanceInspector = fxToInstanceInspector(the, theInstance);
	if (instanceInspector) {
		txSlot* hostInspectors = &mxHostInspectors;
		txSlot* hostInspector;
		txSlot* cache;
		txSlot* cacheProperty;
		hostInspector = hostInspectors->value.list.first;
		while (hostInspector) {
			if (hostInspector->value.hostInspector.instance == theInstance) {
				break;
			}
			hostInspector = hostInspector->next;
		}
		if (!hostInspector) {
			txSlot* aParent;
			cache = fxNewInstance(the);
			hostInspector = fxNewSlot(the);
			hostInspector->kind = XS_HOST_INSPECTOR_KIND;
			hostInspector->value.hostInspector.cache = cache;
			hostInspector->value.hostInspector.instance = theInstance;
			if (hostInspectors->value.list.first) 
				hostInspectors->value.list.last->next = hostInspector;
			else
				hostInspectors->value.list.first = hostInspector;
			hostInspectors->value.list.last = hostInspector;
			mxPop();
			
			aParent = theInstance;
			while (aParent && (aParent->next->kind == XS_HOST_KIND)) {
				txSlot* aParentProperty = aParent->next;
				while (aParentProperty) {
					if ((aParentProperty->kind == XS_ACCESSOR_KIND) && (aParentProperty->value.accessor.getter)) {
						cacheProperty = mxBehaviorGetProperty(the, cache, aParentProperty->ID, 0, XS_ANY);
						if (!cacheProperty) {
							txSlot* aFunction = aParentProperty->value.accessor.getter;
							if (mxIsFunction(aFunction)) {
								fxBeginHost(the);
								/* THIS */
								mxPushReference(theInstance);
								/* FUNCTION */
								mxPushReference(aFunction);
								mxCall();
								mxRunCount(0);
								cacheProperty = mxBehaviorSetProperty(the, cache, aParentProperty->ID, 0, XS_ANY);
								cacheProperty->flag |= XS_INSPECTOR_FLAG;
								cacheProperty->kind = the->stack->kind;
								cacheProperty->value = the->stack->value;
								mxPop();
								fxEndHost(the);
							}
						}
					}
					aParentProperty = aParentProperty->next;
				}
				aParent = fxGetPrototype(the, aParent);
			}
		}
		cache = hostInspector->value.hostInspector.cache;
		cacheProperty = cache->next;
		while (cacheProperty) {
			if (cacheProperty->ID)
				fxEchoProperty(the, cacheProperty, theList, C_NULL, -1, C_NULL);
			cacheProperty = cacheProperty->next;
		}
	}
}


void fxEchoPropertyInstance(txMachine* the, txInspectorNameList* theList, txString thePrefix, txIndex theIndex, txString theSuffix, txID theID, txFlag theFlag, txSlot* theInstance)
{
	txSlot* instanceInspector = fxToInstanceInspector(the, theInstance);
	char buffer[128];
	txString p = buffer;
	txString q = p + sizeof(buffer);

	if (theFlag & XS_GETTER_FLAG) {
		c_strcpy(p, "get ");
		p += 4;
	}
	else if (theFlag & XS_SETTER_FLAG) {
		c_strcpy(p, "set ");
		p += 4;
	}
	if (thePrefix) {
		c_strcpy(p, thePrefix);
		p += mxStringLength(thePrefix);
		if (theSuffix) {
			fxIndexToString(the, theIndex, p, mxPtrDiff(q - p - 1)); // assume mxStringLength(theSuffix) == 1;
			c_strcat(p, theSuffix);
		}
	}
	else
		fxIDToString(the, theID, p, mxPtrDiff(q - p));

	fxEcho(the, "<property");
	if (instanceInspector) {
		if (instanceInspector->value.instanceInspector.link)
			fxEchoFlags(the, " ", theFlag);
		else
			fxEchoFlags(the, "-", theFlag);
	}
	else
		fxEchoFlags(the, "+", theFlag);
	fxEcho(the, " name=\"");
	if (theFlag & XS_GETTER_FLAG)
		fxEcho(the, "get ");
	else if (theFlag & XS_SETTER_FLAG)
		fxEcho(the, "set ");
	fxEchoPropertyName(the, thePrefix, theIndex, theSuffix, theID);
	fxEcho(the, "\"");
	
	if (instanceInspector) {
		if (instanceInspector->value.instanceInspector.link) {
			txInspectorNameLink* link = theList->first;
			fxEcho(the, " value=\"");
			while (link) {
				fxEchoPropertyName(the, link->prefix, link->index, link->suffix, link->id);
				if (link == instanceInspector->value.instanceInspector.link)
					break;
				fxEcho(the, ".");
				link = link->next;
			}
			fxEcho(the, "\"/>");
		}
		else {
			txInspectorNameLink link;
			link.previous = theList->last;
			link.next = C_NULL;
			link.prefix = thePrefix;
			link.index = theIndex;
			link.suffix = theSuffix;
			link.id = theID;
			if (theList->first)
				theList->last->next = &link;
			else
				theList->first = &link;
			theList->last = &link;
			instanceInspector->value.instanceInspector.link = &link;
			
			fxEchoAddress(the, theInstance);
			fxEcho(the, ">");
			fxEchoInstance(the, theInstance, theList);
			fxEcho(the, "</property>");
			
			instanceInspector->value.instanceInspector.link = C_NULL;
			if (link.previous)
				link.previous->next = C_NULL;
			else
				theList->first = C_NULL;
			theList->last = link.previous;
		}
	}
	else {
		fxEchoAddress(the, theInstance);
		fxEcho(the, "/>");
	}
}

void fxEchoPropertyName(txMachine* the, txString thePrefix, txIndex theIndex, txString theSuffix, txID theID)
{
	if (thePrefix) {
		fxEchoString(the, thePrefix);
		if (theSuffix) {
			fxIndexToString(the, theIndex, the->nameBuffer, sizeof(the->nameBuffer));
			fxEchoString(the, the->nameBuffer);
			fxEchoString(the, theSuffix);
		}
	}
	else {
		if (theID != XS_NO_ID) {
			txBoolean adorn;
			txString string = fxGetKeyString(the, theID, &adorn);
			if (adorn) {
				fxEcho(the, "Symbol(");
				fxEchoString(the, string);
				fxEcho(the, ")");
			}
			else
				fxEchoString(the, string);
		}
		else
			fxEcho(the, "?");
	}
}

void fxEchoStart(txMachine* the)
{
	the->echoOffset = 0;
	fxEcho(the, "\15\12<xsbug>");
}

void fxEchoStop(txMachine* the)
{
	fxEcho(the, "</xsbug>\15\12");
	fxSend(the, 0);
	the->echoOffset = 0;
}

void fxEchoString(txMachine* the, txString theString)
{
	static const txByte gxEscape[256] ICACHE_FLASH_ATTR = {
	/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
		 1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,	/* 2x   !"#$%&'()*+,-./  */
		 1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,	/* 3x  0123456789:;<=>?  */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0, 	/* 7X  pqrstuvwxyz{|}~   */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 8X                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 9X                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* AX                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* BX                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* CX                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* FX                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* EX                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 	/* FX                    */
	};
	txU1 tmp;
	txU1* src;
	txU1* dst;
	txU1* start;
	txU1* stop;

	src = (txU1*)theString;
	dst = (txU1*)the->echoBuffer + the->echoOffset;
	start = (txU1*)the->echoBuffer;
	stop = (txU1*)the->echoBuffer + sizeof(the->echoBuffer) - 1;
	while ((tmp = c_read8(src))) {
		src++;
		if (dst + 6 > stop) {
			the->echoOffset = mxPtrDiff(dst - start);
			fxSend(the, 1);
			dst = start;
		}
#if mxCESU8
		if (tmp & 0x80) {
			txInteger character;
			src = (txU1*)fxCESU8Decode((txString)src - 1, &character);
			if (character > 128) {
				dst = (txU1*)fxUTF8Encode((txString)dst, character);
				continue;
			}
			tmp = (txU1)character;
		}
#endif
		if (c_read8(gxEscape + tmp))
			*dst++ = tmp;
		else {
			*(dst++) = '&';
			*(dst++) = '#';
			if (tmp >= 100) {
				*(dst++) = '0' + (tmp / 100);
				tmp %= 100;
				*(dst++) = '0' + (tmp / 10);
				tmp %= 10;
				*(dst++) = '0' + tmp;
			}
			else if (tmp >= 10) {
				*(dst++) = '0' + (tmp / 10);
				tmp %= 10;
				*(dst++) = '0' + tmp;
			}
			else {
				*(dst++) = '0' + tmp;
			}
			*(dst++) = ';';
		}
	}
	the->echoOffset = mxPtrDiff(dst - start);
}

void fxEchoTypedArray(txMachine* the, txSlot* theInstance, txInspectorNameList* theList)
{
	txSlot* dispatch = theInstance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txU2 shift = dispatch->value.typedArray.dispatch->shift;
	txInteger size = fxGetDataViewSize(the, view, buffer) >> shift;
	fxEcho(the, "<property");
	fxEchoFlags(the, " ", dispatch->flag);
	fxEcho(the, " name=\"(");
	fxIDToString(the, dispatch->value.typedArray.dispatch->constructorID, the->nameBuffer, sizeof(the->nameBuffer));
	fxEchoString(the, the->nameBuffer);
	fxEcho(the, ")\"");
	fxEcho(the, " value=\"");
	fxEchoInteger(the, size);
	fxEcho(the, " items\"/>");
	if (size > 0) {
		txInteger index = 0;
		if (size > 1024)
			size = 1024;
		mxPushUndefined();
		while (index < size) {
			(*dispatch->value.typedArray.dispatch->getter)(the, buffer->value.reference->next, view->value.dataView.offset + (index << shift), the->stack, EndianNative);
			fxEchoProperty(the, the->stack, theList, "[", index, "]");
			index++;
		}
		mxPop();
	}
}

txSlot* fxFindFrame(txMachine* the)
{
	txSlot* frame = the->frame;
	while (frame) {
		if (frame->flag & XS_DEBUG_FLAG)
			break;
		frame = frame->next;
	}
	return frame;
}

txSlot* fxFindRealm(txMachine* the)
{
	txSlot* frame = fxFindFrame(the);
	txSlot* realm = C_NULL;
	if (frame && (!(frame->flag & XS_C_FLAG))) {
		txSlot* function = frame + 3;
		if (mxIsReference(function)) {
            txSlot* instance = function->value.reference;
            txSlot* home = mxFunctionInstanceHome(instance);
			txSlot* module = home->value.home.module;
            if (module)
                realm = mxModuleInstanceInternal(module)->value.module.realm;
		}
	}
	if (!realm)
		realm = mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm;
	return realm;
}

void fxGo(txMachine* the)
{
	txSlot* aSlot = the->frame;
	while (aSlot) {
		aSlot->flag &= ~(XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG);
		aSlot = aSlot->next;
	}
}

void fxIndexToString(txMachine* the, txIndex theIndex, txString theBuffer, txSize theSize)
{
	c_snprintf(theBuffer, theSize, "%u", theIndex);
}

void fxListFrames(txMachine* the)
{
	txSlot* aFrame;

	fxEcho(the, "<frames>");
	aFrame = the->frame;
	while (aFrame) {
		fxEcho(the, "<frame");
		fxEcho(the, " name=\"");
		fxEchoFrameName(the, aFrame);
		fxEcho(the, "\"");
		fxEchoAddress(the, aFrame);
		fxEchoFramePathLine(the, aFrame);
		fxEcho(the, "/>");
		aFrame = aFrame->next;
	}
	fxEcho(the, "</frames>");
}

void fxListGlobal(txMachine* the)
{
	txInspectorNameList aList = { C_NULL, C_NULL };
	txSlot* realm = fxFindRealm(the);
	txSlot* global = mxRealmGlobal(realm)->value.reference;
	txSlot* slot = fxGetPrototype(the, global);
	fxEcho(the, "<global>");
	if (slot != mxObjectPrototype.value.reference) {
		fxEchoPropertyInstance(the, &aList, "(..)", -1, C_NULL, XS_NO_ID, global->flag & XS_MARK_FLAG, slot);
	}
	slot = global->next;
	while (slot->flag & XS_INTERNAL_FLAG) {
		slot = slot->next;
	}
	while (slot) {
		fxEchoProperty(the, slot, &aList, C_NULL, -1, C_NULL);
		slot = slot->next;
	}
	global = mxRealmClosures(realm)->value.reference;
	slot = global->next;
	while (slot) {
		fxEchoProperty(the, slot, &aList, C_NULL, -1, C_NULL);
		slot = slot->next;
	}
	fxEcho(the, "</global>");
}

void fxListLocal(txMachine* the)
{
	txInspectorNameList aList = { C_NULL, C_NULL };
	txSlot* frame = fxFindFrame(the);
	txSlot* scope = C_NULL;
	if (!frame) // @@
		return;
	fxEcho(the, "<local");
	fxEcho(the, " name=\"");
	fxEchoFrameName(the, frame);
	fxEcho(the, "\"");
	fxEchoAddress(the, frame);
	fxEchoFramePathLine(the, frame);
	fxEcho(the, ">");
	fxEchoProperty(the, frame + 1, &aList, "(return)", -1, C_NULL);
	fxEchoProperty(the, frame + 2, &aList, "new.target", -1, C_NULL);
	fxEchoProperty(the, frame + 3, &aList, "(function)", -1, C_NULL);
	fxEchoProperty(the, frame + 4, &aList, "this", -1, C_NULL);
	if (frame == the->frame)
		scope = the->scope;
	else {
		txSlot* current = the->frame;
		while (current->next != frame)
			current = current->next;
		if (current)
			scope = current->value.frame.scope;
	}
	if (frame->flag & XS_C_FLAG) {
		txInteger aCount, anIndex;
		aCount = (frame - 1)->value.integer;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			fxEchoProperty(the, (frame - 2 - anIndex), &aList, "arg(", anIndex, ")");
		}
		if (scope) {
			aCount = scope->value.environment.variable.count;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				fxEchoProperty(the, (scope - 1 - anIndex), &aList, "var(", anIndex, ")");
			}
		}
	}
	else {
		if (scope) {
			txSlot* aSlot = mxFrameToEnvironment(frame);
			txID id;
			while (aSlot > scope) {
				aSlot--;
				id = aSlot->ID;
				if ((0 < id) && (id < the->keyCount)) {
					txSlot* key;
					if (id < the->keyOffset)
						key = the->keyArrayHost[id];
					else
						key = the->keyArray[id - the->keyOffset];
					if (key) {
						txKind kind = mxGetKeySlotKind(key);
						if ((kind == XS_KEY_KIND) || (kind == XS_KEY_X_KIND)) {
							 if (key->value.key.string[0] != '#')
								fxEchoProperty(the, aSlot, &aList, C_NULL, -1, C_NULL);
						}
						else
							fxEchoProperty(the, aSlot, &aList, C_NULL, -1, C_NULL);
					}
				}
			}
		}
	}
	fxEcho(the, "</local>");
}

void fxListModules(txMachine* the)
{
	txInspectorNameList aList = { C_NULL, C_NULL };
	txSlot* realm = fxFindRealm(the);
	txSlot* moduleMap = mxModuleMap(realm);
	txSlot* instance = fxGetInstance(the, moduleMap);
	txSlot* instanceInspector = fxToInstanceInspector(the, instance);
	txSlot* modules = mxOwnModules(realm)->value.reference;
	txSlot* module;
	fxEcho(the, "<grammar>");
	if (instance->next) {
		fxEcho(the, "<node");
		if (instanceInspector)
			fxEchoFlags(the, "-", moduleMap->flag);
		else
			fxEchoFlags(the, "+", moduleMap->flag);
		fxEcho(the, " name=\"(map)\"");
		fxEchoAddress(the, instance);
		if (instanceInspector) {
			fxEcho(the, ">");
			fxEchoInstance(the, instance, &aList);
			fxEcho(the, "</node>");
		}
		else
			fxEcho(the, "/>");
	}
	module = modules->next;
	while (module) {
        if (mxIsReference(module))
            fxEchoModule(the, module, &aList);
		module = module->next;
	}
	modules = modules->value.instance.prototype;
	if (modules) {
		module = modules->next;
		while (module) {
			if (mxIsReference(module))
				fxEchoModule(the, module, &aList);
			module = module->next;
		}
	}
	fxEcho(the, "</grammar>");
}

void fxLogin(txMachine* the)
{
	if (!fxIsConnected(the)) {
		fxConnect(the);
		if (!fxIsConnected(the))
			return;
	}
	fxEchoStart(the);
	fxEcho(the, "<login name=\"");
	if (the->name)
		fxEchoString(the, the->name);
	else
		fxEchoString(the, "xslib");
	fxEcho(the, "\" value=\"");
	fxEcho(the, "XS ");
	fxEchoInteger(the, XS_MAJOR_VERSION);
	fxEcho(the, ".");
	fxEchoInteger(the, XS_MINOR_VERSION);
	fxEcho(the, ".");
	fxEchoInteger(the, XS_PATCH_VERSION);
	fxEcho(the, " ");
	fxEchoInteger(the, (txInteger)(sizeof(txSlot*)*8));
	fxEcho(the, "-bit ");
#if mxBigEndian
	fxEcho(the, "BE ");
#else
	fxEcho(the, "LE ");
#endif
	fxEchoInteger(the, (txInteger)(sizeof(txID)*8));
	fxEcho(the, "-bit ID\" flags=\"");
#if mxDebugEval
	fxEcho(the, "e");
#else	
	fxEcho(the, "E");
#endif
	fxEcho(the, "\"/>");
	fxEchoStop(the);
	if (the->sharedMachine) {
		fxToggle(the, the->sharedMachine->stackTop[-1 - mxGlobalStackIndex].value.reference);
		fxToggle(the, the->sharedMachine->stackTop[-1 - mxCompartmentGlobalStackIndex].value.reference);
	}
	fxDebugCommand(the);
}

void fxLogout(txMachine* the)
{
	if (!fxIsConnected(the))
		return;
	fxStopProfiling(the, C_NULL);
	fxDisconnect(the);
}

void fxSelect(txMachine* the, txSlot* slot)
{
	txSlot* frame = the->frame;
	while (frame) {
		if (frame == slot)
			frame->flag |= XS_DEBUG_FLAG;
		else
			frame->flag &= ~XS_DEBUG_FLAG;
		frame = frame->next;
	}
}

void fxSetBreakpoint(txMachine* the, txString thePath, txInteger theLine, size_t theID)
{
	txID path;
	txSlot* breakpoint;

	if (!thePath)
		return;
	if ((theID == 0) && (theLine == 0)) { 
		if (!c_strcmp(thePath, "exceptions")) {
			the->breakOnExceptionsFlag = 1;
			mxPushUndefined();
			return;
		}	
		if (!c_strcmp(thePath, "start")) {
			the->breakOnStartFlag = 1;
			mxPushUndefined();
			return;
		}	
	}
	path = fxNewNameC(the, thePath);
	if (!path)
		return;
	breakpoint = mxBreakpoints.value.list.first;
	while (breakpoint)	{
		if ((breakpoint->ID == path) && (breakpoint->value.breakpoint.line == theLine)) {
			break;
		}
		breakpoint = breakpoint->next;
	}
	if (!breakpoint) {
		breakpoint = fxNewSlot(the);
		breakpoint->next = mxBreakpoints.value.list.first;
		breakpoint->ID = path;
		breakpoint->kind = XS_BREAKPOINT_KIND;
		breakpoint->value.breakpoint.line = theLine;
		mxBreakpoints.value.list.first = breakpoint;
	}
	if (theID == 0) {
		mxPushUndefined();
		breakpoint->value.breakpoint.info = C_NULL;
	}
	else {
		txSlot* instance = fxNewInstance(the);
		txSlot* property = fxLastProperty(the, instance);
		property = fxNextUndefinedProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG);
		property = fxNextUndefinedProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG);
		property = fxNextUndefinedProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG);
		breakpoint->value.breakpoint.info = instance;
	}
}

void fxSetBreakpointCondition(txMachine* the, txSlot* reference, txString it)
{
	txSlot* instance = fxToInstance(the, reference);
	txSlot* property = instance->next;
	fxDebugEvalBuffer(the, it, property);
}

void fxSetBreakpointHitCount(txMachine* the, txSlot* reference, txString it)
{
	txSlot* instance = fxToInstance(the, reference);
	txSlot* property = instance->next->next;
	char c = *it;
	txID op = XS_CODE_MORE_EQUAL;
	txInteger count = 0;
	if (c == '%') {
		it++;
		op = XS_CODE_MODULO;
	}
	else if (c == '<') {
		it++;
		if (*it == '=') {
			it++;
			op = XS_CODE_LESS_EQUAL;
		}
		else
			op = XS_CODE_LESS;
	}
	else if (*it == '=') {
		op = XS_CODE_EQUAL;
		it++;
	}
	else if (*it == '>') {
		it++;
		if (*it == '=') {
			it++;
			op = XS_CODE_MORE_EQUAL;
		}
		else
			op = XS_CODE_MORE;
	}
	it = fxSkipSpaces(it);
	while ((c = *it++)) {
		if (('0' <= c) && (c <= '9'))
			count = (count * 10) + (c - '0');
		else
			break;
	}
	property->ID = op;
	property->kind = XS_DATA_VIEW_KIND;
	property->value.dataView.offset = 0;
	property->value.dataView.size = count;
}

void fxSetBreakpointTrace(txMachine* the, txSlot* reference, txString it)
{
	txSlot* instance = fxToInstance(the, reference);
	txSlot* property = instance->next->next->next;
	fxDebugEvalBuffer(the, it, property);
}

void fxStep(txMachine* the)
{
	txSlot* aSlot = the->frame;
	if (aSlot) {
		while (aSlot) {
			aSlot->flag &= ~XS_STEP_INTO_FLAG;
			aSlot->flag |= XS_STEP_OVER_FLAG;
			aSlot = aSlot->next;
		}
	}
	else {
		the->breakOnStartFlag = 1;
	}
}

void fxStepInside(txMachine* the)
{
	txSlot* aSlot = the->frame;
	while (aSlot) {
		aSlot->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
		aSlot = aSlot->next;
	}
}

void fxStepOutside(txMachine* the)
{
	txSlot* aSlot = the->frame;
	if (aSlot) {
		aSlot->flag &= ~(XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG);
		aSlot = aSlot->next;
		while (aSlot) {
			aSlot->flag &= ~XS_STEP_INTO_FLAG;
			aSlot->flag |= XS_STEP_OVER_FLAG;
			aSlot = aSlot->next;
		}
	}
}

txSlot* fxToInstanceInspector(txMachine* the, txSlot* slot)
{
	txSlot* instanceInspector = mxInstanceInspectors.value.list.first;
	while (instanceInspector) {
		if (instanceInspector->value.instanceInspector.slot == slot)
			return instanceInspector;
		if (instanceInspector->value.instanceInspector.slot > slot)
			break;
		instanceInspector = instanceInspector->next;
	}
	return C_NULL;
}

void fxToggle(txMachine* the, txSlot* slot)
{
	txSlot** instanceInspectorAddress = &(mxInstanceInspectors.value.list.first);
	txSlot* instanceInspector;
	while ((instanceInspector = *instanceInspectorAddress)) {
		if (instanceInspector->value.instanceInspector.slot == slot) {
			*instanceInspectorAddress = instanceInspector->next;
			return;
		}
		if (instanceInspector->value.instanceInspector.slot > slot)
			break;
		instanceInspectorAddress = &(instanceInspector->next);
	}
	instanceInspector = fxNewSlot(the);
	instanceInspector->next = *instanceInspectorAddress;
	instanceInspector->kind = XS_INSTANCE_INSPECTOR_KIND;
	instanceInspector->value.instanceInspector.slot = slot;
	instanceInspector->value.instanceInspector.link = C_NULL;
	*instanceInspectorAddress = instanceInspector;
}

#endif

void fxBubble(txMachine* the, txInteger flags, void* message, txInteger length, txString conversation)
{
#ifdef mxDebug
	if (fxIsConnected(the)) {
		txString path = C_NULL;
		txInteger line = 0;
		txSlot* frame = the->frame;
		while (frame && !path) {
			txSlot* environment = mxFrameToEnvironment(frame);
			if (environment->ID != XS_NO_ID) {
				path = fxGetKeyName(the, environment->ID);
				line = environment->value.environment.line;
			}
			frame = frame->next;
		}
		fxEchoStart(the);
		fxEcho(the, "<bubble name=\"");
		if (conversation)
			fxEchoString(the, conversation);
		fxEcho(the, "\" value=\"");
		fxEchoInteger(the, flags);
		fxEcho(the, "\"");
		fxEchoPathLine(the, path, line);
		fxEcho(the, ">");
		if (flags & XS_BUBBLE_BINARY) {
			txU1* bytes = message;
			txInteger byteLength = length;
			while (byteLength) {
				txU1 byte = c_read8(bytes);
				fxEchoCharacter(the, gxHexaDigits[(byte & 0xF0) >> 4]);
				fxEchoCharacter(the, gxHexaDigits[(byte & 0x0F)]);	
				bytes++;
				byteLength--;
			}
		}
		else
			fxEchoString(the, message);
		fxEcho(the, "</bubble>");
		fxEchoStop(the);
	}
#elif defined(mxInstrument)
	if (!(flags & XS_BUBBLE_BINARY)) {
		if (conversation)
			fxReport(the, "%s: %s\n", conversation, message);
		else
			fxReport(the, "%s\n", message);
	}
#endif
}

void fxFileEvalString(txMachine* the, txString string, txString tag)
{
	#ifdef mxDebug
	if (fxIsConnected(the)) {
		fxEchoStart(the);
		fxEcho(the, "<eval path=\"");
		fxEchoString(the, tag);
		fxEcho(the, "\"");
		fxEcho(the, ">");
		fxEchoString(the, string);
		fxEcho(the, "</eval>");
		fxEchoStop(the);
	}
#endif
}

void fxReport(txMachine* the, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxVReport(the, theFormat, arguments);
	c_va_end(arguments);
#ifndef mxNoConsole
	c_va_start(arguments, theFormat);
	c_vprintf(theFormat, arguments);
	c_va_end(arguments);
#endif
}

void fxReportException(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxVReportException(the, thePath, theLine, theFormat, arguments);
	c_va_end(arguments);
#ifndef mxNoConsole
	if (thePath && theLine)
#if mxWindows
		printf("%s(%d): exception: ", thePath, (int)theLine);
#else
		c_printf("%s:%d: exception: ", thePath, (int)theLine);
#endif
	else
		c_printf("# exception: ");
	c_va_start(arguments, theFormat);
	c_vprintf(theFormat, arguments);
	c_va_end(arguments);
	c_printf("!\n");
#endif
}

void fxReportError(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxVReportError(the, thePath, theLine, theFormat, arguments);
	c_va_end(arguments);
#ifndef mxNoConsole
	if (thePath && theLine)
#if mxWindows
		printf("%s(%d): error: ", thePath, (int)theLine);
#else
		c_printf("%s:%d: error: ", thePath, (int)theLine);
#endif
	else
		c_printf("# error: ");
	c_va_start(arguments, theFormat);
	c_vprintf(theFormat, arguments);
	c_va_end(arguments);
	c_printf("!\n");
#endif
}

void fxReportWarning(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxVReportWarning(the, thePath, theLine, theFormat, arguments);
	c_va_end(arguments);
#ifndef mxNoConsole
	if (thePath && theLine)
#if mxWindows
		printf("%s(%d): warning: ", thePath, (int)theLine);
#else
		c_printf("%s:%d: warning: ", thePath, (int)theLine);
#endif
	else
		c_printf("# warning: ");
	c_va_start(arguments, theFormat);
	c_vprintf(theFormat, arguments);
	c_va_end(arguments);
	c_printf("!\n");
#endif
}

txID fxGenerateProfileID(void* console)
{
	txMachine* the = console;
	txID id = the->profileID;
	the->profileID++;
	return id;
}

void fxGenerateTag(void* console, txString buffer, txInteger bufferSize, txString path)
{
	txMachine* the = console;
	if (path)
		c_snprintf(buffer, bufferSize, "#%d@%s", the->tag, path);
	else
		c_snprintf(buffer, bufferSize, "#%d", the->tag);
	the->tag++;
}

void fxVReport(void* console, txString theFormat, c_va_list theArguments)
{
#ifdef mxDebug
	txMachine* the = console;
	if (fxIsConnected(the)) {
		fxEchoStart(the);
		fxEcho(the, "<log>");
		fxEchoFormat(the, theFormat, theArguments);
		fxEcho(the, "</log>");
		fxEchoStop(the);
	}
#endif
#if defined(DEBUG_EFM)
	memmove(_lastDebugStrBuffer, _debugStrBuffer, 256);
	vsprintf(_debugStrBuffer, theFormat, theArguments);
	_debugStrBuffer[255] = '\0';
#endif
}

void fxVReportException(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
{
#ifdef mxDebug
	txMachine* the = console;
	if (fxIsConnected(the)) {
		fxEchoStart(the);
		fxEcho(the, "<log");
		fxEchoPathLine(the, thePath, theLine);
		fxEcho(the, "># Exception: ");
		fxEchoFormat(the, theFormat, theArguments);
		fxEcho(the, "!\n</log>");
		fxEchoStop(the);
	}
#endif
#if defined(DEBUG_EFM)
	if (thePath && theLine)
		sprintf(_debugStrBuffer, "%s:%d: exception: ", thePath, (int)theLine);
	else
		sprintf(_debugStrBuffer, "# exception: ");
	memmove(_lastDebugStrBuffer, _debugStrBuffer, 256);
	vsprintf(_debugStrBuffer, theFormat, theArguments);
	_debugStrBuffer[255] = '\0';
#endif
}

void fxVReportError(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
{
#ifdef mxDebug
	txMachine* the = console;
	if (fxIsConnected(the)) {
		fxEchoStart(the);
		fxEcho(the, "<log");
		fxEchoPathLine(the, thePath, theLine);
		fxEcho(the, "># Error: ");
		fxEchoFormat(the, theFormat, theArguments);
		fxEcho(the, "!\n</log>");
		fxEchoStop(the);
	}
#endif
#if defined(DEBUG_EFM)
	if (thePath && theLine)
		sprintf(_debugStrBuffer, "%s:%d: error: ", thePath, (int)theLine);
	else
		sprintf(_debugStrBuffer, "# error: ");
	vsprintf(_debugStrBuffer, theFormat, theArguments);
	_debugStrBuffer[255] = '\0';
#endif
}

void fxVReportWarning(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
{
#ifdef mxDebug
	txMachine* the = console;
	if (fxIsConnected(the)) {
		fxEchoStart(the);
		fxEcho(the, "<log");
		fxEchoPathLine(the, thePath, theLine);
		fxEcho(the, "># Warning: ");
		fxEchoFormat(the, theFormat, theArguments);
		fxEcho(the, "!\n</log>");
		fxEchoStop(the);
	}
#endif
#if defined(DEBUG_EFM)
	if (thePath && theLine)
		sprintf(_debugStrBuffer, "%s:%d: warning: ", thePath, (int)theLine);
	else
		sprintf(_debugStrBuffer, "# warning: ");
	vsprintf(_debugStrBuffer, theFormat, theArguments);
	_debugStrBuffer[255] = '\0';
#endif
}

#ifdef mxInstrument	
#if kCPUESP32C6 || kCPUESP32H2
#define ICACHE_XS6STRING_ATTR
#endif
#define xsInstrumentCount 12
static char* const xsInstrumentNames[xsInstrumentCount] ICACHE_XS6STRING_ATTR = {
	"Chunk used",
	"Chunk available",
	"Slot used",
	"Slot available",
	"Stack used",
	"Stack available",
	"Garbage collections",
	"Keys used",
	"Modules loaded",
	"Parser used",
	"Floating Point",
	"Promises settled"
};
static char* const xsInstrumentUnits[xsInstrumentCount] ICACHE_XS6STRING_ATTR = {
	" / ",
	" bytes",
	" / ",
	" bytes",
	" / ",
	" bytes",
	" times",
	" keys",
	" modules",
	" bytes",
	" operations",
	" promises",
};

void fxDescribeInstrumentation(txMachine* the, txInteger count, txString* names, txString* units)
{
	txInteger i, j = 0;
#ifdef mxDebug
	if (fxIsConnected(the)) {
		fxEchoStart(the);
		fxEcho(the, "<instruments>");
		for (i = 0; i < count; i++, j++) {
			fxEcho(the, "<instrument name=\"");
			fxEchoString(the, names[i]);
			fxEcho(the, "\" value=\"");
			fxEchoString(the, units[i]);
			fxEcho(the, "\"/>");
		}
		for (i = 0; i < xsInstrumentCount; i++, j++) {
			fxEcho(the, "<instrument name=\"");
			fxEchoString(the, (txString) xsInstrumentNames[i]);
			fxEcho(the, "\" value=\"");
			fxEchoString(the, (txString) xsInstrumentUnits[i]);
			fxEcho(the, "\"/>");
		}
		fxEcho(the, "</instruments>");
		fxEchoStop(the);
		//fxReceive(the);
		return;
	}
#endif
#ifndef mxNoConsole
	j = 0;
	c_printf("instruments key: ");
	for (i = 0; i < count; i++, j++) {
		if (j)
			c_printf(",");
		c_printf("%s", names[i]);
	}
	for (i = 0; i < xsInstrumentCount; i++, j++) {
		if (j)
			c_printf(",");
		c_printf("%s", xsInstrumentNames[i]);
	}
	c_printf("\n");
#endif
}

void fxSampleInstrumentation(txMachine* the, txInteger count, txInteger* values)
{
	txInteger xsInstrumentValues[xsInstrumentCount];
	xsInstrumentValues[0] = the->currentChunksSize;
	xsInstrumentValues[1] = the->maximumChunksSize;
	xsInstrumentValues[2] = the->currentHeapCount * sizeof(txSlot);
	xsInstrumentValues[3] = the->maximumHeapCount * sizeof(txSlot);
	xsInstrumentValues[4] = (mxPtrDiff(the->stackTop - the->stackPeak)) * sizeof(txSlot);
	xsInstrumentValues[5] = (mxPtrDiff(the->stackTop - the->stackBottom)) * sizeof(txSlot);
	xsInstrumentValues[6] = the->garbageCollectionCount;
	xsInstrumentValues[7] = the->keyIndex - the->keyOffset - the->keyholeCount;
	xsInstrumentValues[8] = the->loadedModulesCount;
	xsInstrumentValues[9] = the->peakParserSize;
	xsInstrumentValues[10] = the->floatingPointOps;
	xsInstrumentValues[11] = the->promisesSettledCount;

	txInteger i, j = 0;
#ifdef mxDebug
	if (fxIsConnected(the)) {
		fxEchoStart(the);
		fxEcho(the, "<samples>");
		for (i = 0; i < count; i++, j++) {
			if (j)
				fxEcho(the, ",");
			fxEchoInteger(the, values[i]);
		}
		for (i = 0; i < xsInstrumentCount; i++, j++) {
			if (j)
				fxEcho(the, ",");
			fxEchoInteger(the, xsInstrumentValues[i]);
		}
		fxEcho(the, "</samples>");
		fxEchoStop(the);
		return;
	}
#endif
#ifndef mxNoConsole
	j = 0;
	c_printf("instruments: ");
	for (i = 0; i < count; i++, j++) {
		if (j)
			c_printf(",");
		c_printf("%d", values[i]);
	}
	for (i = 0; i < xsInstrumentCount; i++, j++) {
		if (j)
			c_printf(",");
		c_printf("%d", xsInstrumentValues[i]);
	}
	c_printf("\n");
#endif
}

#if defined(modMicrosecondsInstrumentation) || defined(modMicroseconds)
	typedef txU4 txMicroseconds;
#else
	typedef txU8 txMicroseconds;
#endif

#define mxProfilerSampleCount 8

typedef struct sxProfiler txProfiler;
struct sxProfiler {
	txMicroseconds when;
	txMicroseconds former;
//	txMicroseconds start;
	txMicroseconds stop;
	txU4 interval;
	txSize recordCount;
	txByte* records;
	txSize sampleIndex;
	txSize sampleSize;
	txID* samples;
	txU4 deltas[mxProfilerSampleCount];
};

static void fxEchoUnsigned(txMachine* the, txUnsigned value, txInteger radix);
static txID fxFrameToProfilerID(txMachine* the, txSlot* frame);
static txMicroseconds fxGetMicroSeconds();
static void fxSendProfilerRecord(txMachine* the, txSlot* frame, txID id, txSlot* code);
static void fxSendProfilerSamples(txMachine* the, txProfiler* profiler);
static void fxSendProfilerTime(txMachine* the, txString name, txMicroseconds when);

void fxCheckProfiler(txMachine* the, txSlot* frame)
{
	txProfiler* profiler = the->profiler;
	if (!profiler)
		return;
	txMicroseconds when = profiler->when;
	txMicroseconds time = fxGetMicroSeconds();
	if (when <= time) {
		txSize sampleIndex = profiler->sampleIndex;
		txSize sampleSize = profiler->sampleSize;
		txID* samples = profiler->samples + (sampleIndex * sampleSize);
		txU4 interval = profiler->interval;
		profiler->deltas[sampleIndex] = (txU4)(time - profiler->former);
		profiler->former = time;
		profiler->when = time + interval - (time % interval);
		if (!frame) {
			frame = the->frame;
			if (frame)
				*samples++ = 1;
		}
		while (frame) {
			txID id = fxFrameToProfilerID(the, frame);
			if (id)
				*samples++ = id;
			frame = frame->next;
		}
		*samples++ = 0;
		sampleIndex++;
		if (sampleIndex == mxProfilerSampleCount) {
			fxSendProfilerSamples(the, profiler);
			sampleIndex = 0;
		}
		profiler->sampleIndex = sampleIndex;
	}
}

void fxCreateProfiler(txMachine* the)
{
	txProfiler* profiler = the->profiler = c_malloc(sizeof(txProfiler));
	if (profiler == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	profiler->interval = 1250;
	profiler->former = fxGetMicroSeconds();
	profiler->when = profiler->former + profiler->interval;
	
	profiler->recordCount = 128;
	profiler->records = c_calloc(1, profiler->recordCount);
	if (profiler->records == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);

	profiler->sampleIndex = 0;
	profiler->sampleSize = (the->stackTop - the->stackBottom) >> 3;
	profiler->samples = (txID*)c_malloc((size_t)(mxProfilerSampleCount * profiler->sampleSize * sizeof(txID)));
	if (profiler->samples == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);

	fxSendProfilerTime(the, "start", profiler->former);
	fxSendProfilerRecord(the, C_NULL, 0, C_NULL);
	fxSendProfilerRecord(the, C_NULL, 1, C_NULL);
	profiler->records[0] = 0x03;
}

void fxDeleteProfiler(txMachine* the, void* stream)
{
	txProfiler* profiler = the->profiler;
	fxSendProfilerTime(the, "stop", fxGetMicroSeconds());
	c_free(profiler->samples);
	c_free(profiler->records);
	c_free(profiler);
	the->profiler = C_NULL;
}

#ifdef mxDebug
void fxEchoUnsigned(txMachine* the, txUnsigned value, txInteger radix)
{
	char buffer[256];
	char *p = &buffer[sizeof(buffer) - 1];
	*p-- = 0;
	do {
		*p-- = c_read8(gxHexaDigits + (value % radix));
		value /= radix;
	} while (value);
	fxEcho(the, p + 1);
}
#endif

txID fxFrameToProfilerID(txMachine* the, txSlot* frame)
{
	txProfiler* profiler = the->profiler;
	txSlot* function = frame + 3;
	txSlot* code = C_NULL;
	txID id = XS_NO_ID;
	if (function->kind == XS_REFERENCE_KIND) {
		function = function->value.reference;
		if (mxIsFunction(function)) {
			code = mxFunctionInstanceCode(function);
			id = mxFunctionInstanceHome(function)->ID;
		}
	}
#ifdef mxHostFunctionPrimitive
	else if (function->kind == XS_HOST_FUNCTION_KIND)
		id = function->value.hostFunction.profileID;
#endif
	if (id != XS_NO_ID) {		
		txInteger recordIndex = id >> 3;
		txInteger recordMask = 1 << (id & 0x07);
		txInteger recordCount = profiler->recordCount;
		if (recordIndex >= recordCount) {
			while (recordIndex >= recordCount)
				recordCount += 128;
			profiler->records = c_realloc(profiler->records, recordCount);
			if (profiler->records == C_NULL)
				fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
			c_memset(profiler->records + profiler->recordCount, 0, (recordCount - profiler->recordCount));
			profiler->recordCount = recordCount;
		}
		else if (profiler->records[recordIndex] & recordMask)
			return id;
		profiler->records[recordIndex] |= recordMask;
		fxSendProfilerRecord(the, frame, id, code);
		return id;
	}
	return 0;
}

txMicroseconds fxGetMicroSeconds()
{
#if defined(modMicrosecondsInstrumentation)
	return modMicrosecondsInstrumentation();
#elif defined(modMicroseconds)
	return modMicroseconds();
#else
	c_timeval tv;
	c_gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000ULL) + tv.tv_usec;
#endif
}

void fxResumeProfiler(txMachine* the)
{
	txProfiler* profiler = the->profiler;
	if (!profiler)
		return;
	txMicroseconds delta = fxGetMicroSeconds();
	fxSendProfilerTime(the, "resume", delta);
	delta -= profiler->stop;
	profiler->when += delta;
	profiler->former += delta;
}

void fxSendProfilerRecord(txMachine* the, txSlot* frame, txID id, txSlot* code)
{
#ifdef mxDebug
	if (fxIsConnected(the)) {
		fxEchoStart(the);
		if (id == 0) {
			fxEcho(the, "<pr name=\"(host)\" value=\"0\"");
		}
		else if (id == 1) {
			fxEcho(the, "<pr name=\"(gc)\" value=\"1\"");
		}
		else {
			fxEcho(the, "<pr name=\"");
			fxEchoFrameName(the, frame);
			fxEcho(the, "\" value=\"");
			fxEchoInteger(the, id);
			fxEcho(the, "\"");
		}
		if (code) {
			if ((code->kind == XS_CODE_KIND) || (code->kind == XS_CODE_X_KIND)) {
				txByte* p = code->value.code.address + 2;
				if (*p == XS_CODE_FILE) {
					txID file;
					txS2 line;
					p++;
					mxDecodeID(p, file);
					p++;
					mxDecode2(p, line);
					fxEchoPathLine(the, fxGetKeyName(the, file), line);
				}
			}
		}
		fxEcho(the, "/>");
		fxEchoStop(the);
	}
#endif
}

void fxSendProfilerSamples(txMachine* the, txProfiler* profiler)
{
#ifdef mxDebug
	if (fxIsConnected(the)) {
		txID* samples = profiler->samples;
		txSize sampleSize = profiler->sampleSize;
		txSize sampleIndex = 0;
		txID* ids;
		txID id;
		fxEchoStart(the);
		fxEcho(the, "<ps>");
		for (;;) {
			fxEchoUnsigned(the, profiler->deltas[sampleIndex], 36);
			ids = samples;
			while ((id = *ids++)) {
				fxEcho(the, ",");
				fxEchoUnsigned(the, id, 36);
			}
			sampleIndex++;
			if (sampleIndex < mxProfilerSampleCount) 
				fxEcho(the, ",0.");
			else {
				fxEcho(the, ",0</ps>");
				break;
			}
			samples += sampleSize;
		}
		fxEchoStop(the);
	}
#endif
}

void fxSendProfilerTime(txMachine* the, txString name, txMicroseconds when)
{
#ifdef mxDebug
	if (fxIsConnected(the)) {
		int shift;
		fxEchoStart(the);
		fxEcho(the, "<pt name=\"");
		fxEchoString(the, name);
		fxEcho(the, "\" value=\"@");
		shift = (8 * sizeof(when)) - 4;
		while (shift >= 0) {
			fxEchoCharacter(the, c_read8(gxHexaDigits + ((when >> shift) & 0x0F)));
			shift -= 4;
		}
		fxEcho(the, "\"/>");
		fxEchoStop(the);
	}
#endif
}

void fxSuspendProfiler(txMachine* the)
{
	txProfiler* profiler = the->profiler;
	if (!profiler)
		return;
	profiler->stop = fxGetMicroSeconds();
	fxSendProfilerTime(the, "suspend", profiler->stop);
}

#endif /* mxInstrument */

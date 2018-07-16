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

#if defined(_RENESAS_SYNERGY_) || defined(DEBUG_EFM)
char lastDebugStr[256];
char synergyDebugStr[256];
#endif
static void fxVReportException(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments);

#ifdef mxDebug
static void fxClearAllBreakpoints(txMachine* the);
static void fxClearBreakpoint(txMachine* the, txString thePath, txInteger theLine);
static void fxDebugParse(txMachine* the);
static void fxDebugParseTag(txMachine* the, txString name);
static void fxDebugPopTag(txMachine* the);
static void fxDebugPushTag(txMachine* the);
static void fxDebugScriptCDATA(txMachine* the, char c);
static void fxEcho(txMachine* the, txString theString);
static void fxEchoAddress(txMachine* the, txSlot* theSlot);
static void fxEchoCharacter(txMachine* the, char theCharacter);
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
static void fxEchoStart(txMachine* the);
static void fxEchoStop(txMachine* the);
static void fxEchoString(txMachine* the, txString theString);
static void fxGo(txMachine* the);
static void fxListFrames(txMachine* the);
static void fxListGlobal(txMachine* the);
static void fxListLocal(txMachine* the);
static void fxListModules(txMachine* the);
static void fxSetBreakpoint(txMachine* the, txString thePath, txInteger theLine);
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
	XS_CLEAR_ALL_BREAKPOINTS_TAG,
	XS_CLEAR_BREAKPOINTS_TAG,
	XS_GO_TAG,
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
	XS_UNKNOWN_TAG
};

enum {
	XS_ID_ATTRIBUTE = 0,
	XS_LINE_ATTRIBUTE,
	XS_PATH_ATTRIBUTE,
	XS_UNKNOWN_ATTRIBUTE
};

static const char gxHexaDigits[] ICACHE_FLASH_ATTR = "0123456789ABCDEF";

void fxCheck(txMachine* the, txString thePath, txInteger theLine)
{
#if mxWindows
	printf("%s(%ld): fatal!\n", thePath, (int)theLine);
#else
	c_printf("%s:%d: fatal!\n", thePath, (int)theLine);
#endif
	c_exit(0);
}

void fxClearAllBreakpoints(txMachine* the)
{
	mxBreakpoints.value.list.first = C_NULL;
}

void fxClearBreakpoint(txMachine* the, txString thePath, txInteger theLine)
{
	txID path;
	txSlot** breakpointAddress;
	txSlot* breakpoint;

	if (!thePath)
		return;
	if (!c_strcmp(thePath, "exceptions")) {
		the->breakOnExceptionsFlag = 0;
		return;
	}	
	if (!c_strcmp(thePath, "start")) {
		the->breakOnStartFlag = 0;
		return;
	}	
	if ((theLine <= 0) || (0x00007FFF < theLine))
		return;
	path = fxFindName(the, thePath);
	if (!path)
		return;
	breakpointAddress = &(mxBreakpoints.value.list.first);
	while ((breakpoint = *breakpointAddress)) {
		if ((breakpoint->ID == path) && (breakpoint->value.integer == theLine)) {
			*breakpointAddress = breakpoint->next;
			break;
		}
		breakpointAddress = &(breakpoint->next);
	}
}

void fxDebugCommand(txMachine* the)
{
	if (!fxIsConnected(the))
		return;
	the->debugExit = 0;
	for (;;) {
		fxReceive(the);
		fxDebugParse(the);
		if ((the->debugState == XS_LF_STATE) && (the->debugExit > 0))
			break;
	}
	mxHostInspectors.value.list.first = C_NULL;
	mxHostInspectors.value.list.last = C_NULL;
	if ((the->debugTag == XS_MODULE_TAG) || (the->debugTag == XS_SCRIPT_TAG))
		fxQueueJob(the, XS_NO_ID);
}

void fxDebugImport(txMachine* the, txString path)
{
	if (!fxIsConnected(the))
		return;
	fxEchoStart(the);
	fxEcho(the, "<import path=\"");
	fxEcho(the, path + 8);
	fxEcho(the, "\"/>");
	fxEchoStop(the);
	the->debugExit = 0;
	for (;;) {
		fxReceive(the);
		fxDebugParse(the);
		if ((the->debugState == XS_LF_STATE) && (the->debugExit > 1))
			break;
	}
    if (the->debugTag == XS_MODULE_TAG){
        /* RESULT */
        mxPushUndefined();
        fxRunID(the, C_NULL, XS_NO_ID);
        mxPop();
    }
}

void fxDebugLine(txMachine* the)
{
	txSlot* environment = the->frame - 1;
	if (environment->ID != XS_NO_ID) {
		txSlot* breakpoint = C_NULL;
		breakpoint = mxBreakpoints.value.list.first;
		while (breakpoint) {
			if ((breakpoint->ID == environment->ID) && (breakpoint->value.integer == environment->value.environment.line))
				break;
			breakpoint = breakpoint->next;
		}
		if (breakpoint)
			fxDebugLoop(the, C_NULL, 0, "breakpoint");
		else if ((the->frame->flag & XS_STEP_OVER_FLAG))
			fxDebugLoop(the, C_NULL, 0, "step");
	}
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
		txSlot* environment = frame - 1;
		if (environment->ID != XS_NO_ID) {
			path = fxGetKeyName(the, environment->ID);
			line = environment->value.environment.line;
		}
		frame = frame->next;
	}
	if (path)
		fxEchoPathLine(the, path, line);
	fxEcho(the, "># Break: ");
	fxEchoString(the, message);
	fxEcho(the, "!\n</break>");
	fxEchoStop(the);

	the->debugExit = 0;
	for (;;) {
		fxReceive(the);
		fxDebugParse(the);
		if ((the->debugState == XS_LF_STATE) && (the->debugExit > 1))
			break;
	}
	mxHostInspectors.value.list.first = C_NULL;
	mxHostInspectors.value.list.last = C_NULL;

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
				if (c == '"') {
					the->debugState = XS_START_TAG_SPACE_STATE;
					the->pathValue[the->pathIndex] = 0;
				}
				else if (the->pathIndex < 255) {
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
	else if (!c_strcmp(name, "breakpoint"))
		the->debugTag = XS_BREAKPOINT_TAG;
	else if (!c_strcmp(name, "clear-all-breakpoints"))
		the->debugTag = XS_CLEAR_ALL_BREAKPOINTS_TAG;
	else if (!c_strcmp(name, "clear-breakpoint"))
		the->debugTag = XS_CLEAR_BREAKPOINTS_TAG;
	else if (!c_strcmp(name, "go"))
		the->debugTag = XS_GO_TAG;
	else if (!c_strcmp(name, "logout"))
		the->debugTag = XS_LOGOUT_TAG;
	else if (!c_strcmp(name, "module"))
		the->debugTag = XS_MODULE_TAG;
	else if (!c_strcmp(name, "script"))
		the->debugTag = XS_SCRIPT_TAG;
	else if (!c_strcmp(name, "select"))
		the->debugTag = XS_SELECT_TAG;
	else if (!c_strcmp(name, "set-all-breakpoints"))
		the->debugTag = XS_SET_ALL_BREAKPOINTS_TAG;
	else if (!c_strcmp(name, "set-breakpoint"))
		the->debugTag = XS_SET_BREAKPOINT_TAG;
	else if (!c_strcmp(name, "step"))
		the->debugTag = XS_STEP_TAG;
	else if (!c_strcmp(name, "step-inside"))
		the->debugTag = XS_STEP_INSIDE_TAG;
	else if (!c_strcmp(name, "step-outside"))
		the->debugTag = XS_STEP_OUTSIDE_TAG;
	else if (!c_strcmp(name, "toggle"))
		the->debugTag = XS_TOGGLE_TAG;
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
		break;
	case XS_CLEAR_ALL_BREAKPOINTS_TAG:
		the->debugExit |= 1;
		break;
	case XS_CLEAR_BREAKPOINTS_TAG:
		the->debugExit |= 1;
		break;
	case XS_GO_TAG:
		the->debugExit |= 2;
		break;
	case XS_LOGOUT_TAG:
		the->debugExit |= 2;
		break;
	case XS_MODULE_TAG:
	case XS_SCRIPT_TAG:
		mxPop();
		mxPop();
		/* COUNT */
		mxPushInteger(3);
		/* THIS */
		mxPush(mxGlobal);
		/* FUNCTION */
		mxPush(mxGlobal);
		if (the->debugTag == XS_MODULE_TAG)
			fxGetID(the, fxID(the, "<xsbug:module>"));
		else
			fxGetID(the, mxID(__xsbug_script_));
		/* TARGET */
		mxPushUndefined();
		the->debugExit |= 2;
		break;
	case XS_SELECT_TAG:
		break;
	case XS_SET_ALL_BREAKPOINTS_TAG:
		the->debugExit |= 1;
		break;
	case XS_SET_BREAKPOINT_TAG:
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
	case XS_TOGGLE_TAG:
		break;
	}
}

void fxDebugPushTag(txMachine* the)
{
	switch (the->debugTag) {
	case XS_ABORT_TAG:
		fxLogout(the);
		fxGo(the);
		fxAbort(the);
		break;
	case XS_BREAKPOINT_TAG:
		fxSetBreakpoint(the, the->pathValue, the->lineValue);
		break;
	case XS_CLEAR_ALL_BREAKPOINTS_TAG:
		fxClearAllBreakpoints(the);
		break;
	case XS_CLEAR_BREAKPOINTS_TAG:
		fxClearBreakpoint(the, the->pathValue, the->lineValue);
		break;
	case XS_GO_TAG:
		fxGo(the);
		break;
	case XS_LOGOUT_TAG:
		fxLogout(the);
		fxGo(the);
		break;
	case XS_MODULE_TAG:
	case XS_SCRIPT_TAG:
		mxPushUndefined();
		fxStringBuffer(the, the->stack, C_NULL, 256);
		mxPushStringC(the->pathValue);
		mxPushInteger(the->lineValue);
		mxPushInteger(256);
		mxPushInteger(0);
		break;
	case XS_SELECT_TAG:
		fxSelect(the, (txSlot*)the->idValue);
		fxEchoStart(the);
		fxListLocal(the);
		fxEchoStop(the);
		break;
	case XS_SET_ALL_BREAKPOINTS_TAG:
		break;
	case XS_SET_BREAKPOINT_TAG:
		fxSetBreakpoint(the, the->pathValue, the->lineValue);
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
	case XS_TOGGLE_TAG:
		fxToggle(the, (txSlot*)the->idValue);
		fxEchoStart(the);
		fxListLocal(the);
		fxListGlobal(the);
		fxListModules(the);
		fxEchoStop(the);
		break;
	}
}

void fxDebugScriptCDATA(txMachine* the, char c)
{
	if ((the->debugTag == XS_MODULE_TAG) || (the->debugTag == XS_SCRIPT_TAG)) {
		txString string = the->stack[4].value.string;
		txInteger size = the->stack[1].value.integer;
		txInteger offset = the->stack[0].value.integer;
		if (offset == size) {
			txString result = (txString)fxRenewChunk(the, string, size + 256);
			if (!result) {
				result = (txString)fxNewChunk(the, size + 256);
				string = the->stack[4].value.string;
				c_memcpy(result, string, size);
			}
			the->stack[4].value.string = string = result;
			the->stack[1].value.integer = size + 256;
		}
		string[offset++] = c;
		the->stack[0].value.integer = offset;
	}
}

void fxDebugThrow(txMachine* the, txString path, txInteger line, txString message)
{
	if (fxIsConnected(the) && (the->breakOnExceptionsFlag))
		fxDebugLoop(the, path, line, message);
	else {
		txSlot* frame = the->frame;
		while (frame && !path) {
			txSlot* environment = frame - 1;
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
	txInteger srcLength = c_strlen(theString);
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
	unsigned long aValue = (unsigned long)theSlot;
	unsigned long aMask = 0xF;
	int aShift;

	fxEcho(the, " value=\"@");
	aShift = (8 * sizeof(aValue)) - 4;
	while (aShift >= 0) {
		fxEchoCharacter(the, c_read8(gxHexaDigits + ((aValue & aMask << aShift) >> aShift)));
		aShift -= 4;
	}
	/*
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0xF0000000) >> 28]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x0F000000) >> 24]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x00F00000) >> 20]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x000F0000) >> 16]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x0000F000) >> 12]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x00000F00) >> 8]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x000000F0) >> 4]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x0000000F)]);
	*/
	fxEcho(the, "\"");
}

void fxEchoCharacter(txMachine* the, char theCharacter)
{
	char c[2];
	c[0] = theCharacter;
	c[1] = 0;
	fxEchoString(the, c);
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
		txSlot* environment = theFrame - 1;
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

	if (theInstance->ID >= 0) {
		txSlot* aliasInstance = the->aliasArray[theInstance->ID];
		if (aliasInstance)
			theInstance = aliasInstance;
	}
	aParent = theInstance->value.instance.prototype;
	if (aParent)
		fxEchoPropertyInstance(the, theList, "(..)", -1, C_NULL, XS_NO_ID, theInstance->flag & XS_MARK_FLAG, aParent);
	aProperty = theInstance->next;
	if (aProperty && (aProperty->ID == XS_ARRAY_BEHAVIOR)) {
		fxEchoProperty(the, aProperty, theList, "(array)", -1, C_NULL);
	}
	else if (aProperty && (aProperty->flag & XS_INTERNAL_FLAG)) {
		switch (aProperty->kind) {
		case XS_CALLBACK_KIND:
		case XS_CALLBACK_X_KIND:
		case XS_CODE_KIND:
		case XS_CODE_X_KIND:
			fxEchoProperty(the, aProperty, theList, "(function)", -1, C_NULL);
			aProperty = aProperty->next;
			if ((aProperty->kind == XS_HOME_KIND) && (aProperty->value.home.object))
				fxEchoPropertyInstance(the, theList, "(home)", -1, C_NULL, XS_NO_ID, aProperty->flag, aProperty->value.home.object);
			aProperty = aProperty->next;
		#ifdef mxProfile
			aProperty = aProperty->next;
		#endif
			break;
		case XS_ARRAY_BUFFER_KIND:
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
			break;
		case XS_PROMISE_KIND:
			break;
		case XS_MAP_KIND:
			aProperty = aProperty->next;
			anIndex = 0;
			aSlot = aProperty->value.list.first;
			while (aSlot) {
				if (!(aSlot->flag & XS_DONT_ENUM_FLAG)) {
					fxEchoProperty(the, aSlot, theList, "(.", anIndex, ")");
					fxEchoProperty(the, aSlot->next, theList, "(..", anIndex, ")");
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
		case XS_SET_KIND:
			aProperty = aProperty->next;
			anIndex = 0;
			aSlot = aProperty->value.list.first;
			while (aSlot) {
				if (!(aSlot->flag & XS_DONT_ENUM_FLAG))
					fxEchoProperty(the, aSlot, theList, "(.", anIndex, ")");
				anIndex++;
				aSlot = aSlot->next;
			}
			aProperty = aProperty->next;
			break;
		case XS_TYPED_ARRAY_KIND:
			fxEchoProperty(the, aProperty, theList, "(per item)", -1, C_NULL);
			aProperty = aProperty->next;
			fxEchoProperty(the, aProperty, theList, "(view)", -1, C_NULL);
			aProperty = aProperty->next;
			fxEchoProperty(the, aProperty, theList, "(items)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_PROXY_KIND:
			if (aProperty->value.proxy.target)
				fxEchoPropertyInstance(the, theList, "(target)", -1, C_NULL, XS_NO_ID, aProperty->flag, aProperty->value.proxy.target);
			if (aProperty->value.proxy.handler)
				fxEchoPropertyInstance(the, theList, "(handler)", -1, C_NULL, XS_NO_ID, aProperty->flag, aProperty->value.proxy.handler);
			break;
		}
	}
	while (aProperty) {
		if (aProperty->ID < -1) {
			fxEchoProperty(the, aProperty, theList, C_NULL, -1, C_NULL);
		}
		else {
			if (aProperty->kind == XS_ARRAY_KIND) {
				txSlot* item = aProperty->value.array.address;
				txIndex c = fxGetIndexSize(the, aProperty), i;
				for (i = 0; i < c; i++) {
					txIndex index = *((txIndex*)item);
					fxEchoProperty(the, item, theList, "[", index, "]");
					item++;
				}
			}
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
	fxEcho(the, fxGetKeyName(the, slot->value.symbol));
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
	char buffer[256];
	txString name;
	if ((theProperty->kind == XS_CLOSURE_KIND) || (theProperty->kind == XS_EXPORT_KIND))
		theProperty = theProperty->value.closure;
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
		if (thePrefix) {
			fxEchoString(the, thePrefix);
			if (theSuffix) {
				fxNumberToString(the->dtoa, theIndex, buffer, sizeof(buffer), 0, 0);
				fxEchoString(the, buffer);
				fxEchoString(the, theSuffix);
			}
		}
		else {
			fxIDToString(the, ID, buffer, sizeof(buffer));
			fxEchoString(the, buffer);
		}
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
		case XS_ARRAY_BUFFER_KIND:
			fxEcho(the, " value=\"");
			fxEchoInteger(the, theProperty->value.arrayBuffer.length);
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
		case XS_KEY_KIND:
		case XS_KEY_X_KIND:
			fxEcho(the, " value=\"'");
			fxEchoString(the, theProperty->value.key.string);
			fxEcho(the, "'\"/>");
			break;
		case XS_SYMBOL_KIND:
			name = fxGetKeyName(the, theProperty->value.symbol);
			fxEcho(the, " value=\"Symbol(");
			if (name)
				fxEchoString(the, name);
			fxEcho(the, ")\"/>");
			break;
		case XS_DATA_VIEW_KIND:
			fxEcho(the, " value=\"");
			fxEchoInteger(the, theProperty->value.dataView.offset);
			fxEcho(the, ", ");
			fxEchoInteger(the, theProperty->value.dataView.size);
			fxEcho(the, " bytes\"/>");
			break;
		case XS_TYPED_ARRAY_KIND:
			fxEcho(the, " value=\"");
			fxEchoInteger(the, theProperty->value.typedArray.dispatch->size);
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
			the->stack++;
			
			aParent = theInstance;
			while (aParent && (aParent->next->kind == XS_HOST_KIND)) {
				txSlot* aParentProperty = aParent->next;
				while (aParentProperty) {
					if ((aParentProperty->kind == XS_ACCESSOR_KIND) && (aParentProperty->value.accessor.getter)) {
						cacheProperty = mxBehaviorGetProperty(the, cache, aParentProperty->ID, XS_NO_ID, XS_ANY);
						if (!cacheProperty) {
							txSlot* aFunction = aParentProperty->value.accessor.getter;
							if (mxIsFunction(aFunction)) {
								fxBeginHost(the);
								mxPushInteger(0);
								/* THIS */
								mxPushReference(theInstance);
								/* FUNCTION */
								mxPushReference(aFunction);
								fxCall(the);
								cacheProperty = mxBehaviorSetProperty(the, cache, aParentProperty->ID, XS_NO_ID, XS_ANY);
								cacheProperty->flag |= XS_INSPECTOR_FLAG;
								cacheProperty->kind = the->stack->kind;
								cacheProperty->value = the->stack->value;
								the->stack++;
								fxEndHost(the);
							}
						}
					}
					aParentProperty = aParentProperty->next;
				}
				aParent = aParent->value.instance.prototype;
			}
		}
		cache = hostInspector->value.hostInspector.cache;
		cacheProperty = cache->next;
		while (cacheProperty) {
			if (cacheProperty->ID < -1)
				fxEchoProperty(the, cacheProperty, theList, C_NULL, -1, C_NULL);
			cacheProperty = cacheProperty->next;
		}
	}
}

void fxEchoPropertyInstance(txMachine* the, txInspectorNameList* theList, txString thePrefix, txIndex theIndex, txString theSuffix, txID theID, txFlag theFlag, txSlot* theInstance)
{
	txSlot* instanceInspector = fxToInstanceInspector(the, theInstance);
	char buffer[256];
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
		p += c_strlen(thePrefix);
		if (theSuffix) {
			fxNumberToString(the->dtoa, theIndex, p, q - p - 1, 0, 0); // assume c_strlen(theSuffix) == 1;
			c_strcat(p, theSuffix);
		}
	}
	else
		fxIDToString(the, theID, p, q - p);

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
	fxEchoString(the, buffer);
	fxEcho(the, "\"");
	
	if (instanceInspector) {
		if (instanceInspector->value.instanceInspector.link) {
			txInspectorNameLink* link = theList->first;
			fxEcho(the, " value=\"");
			while (link) {
				fxEchoString(the, link->name);
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
			link.name = buffer;
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
	unsigned char tmp;
	unsigned char* src;
	unsigned char* dst;
	unsigned char* start;
	unsigned char* stop;

	src = (unsigned char*)theString;
	dst = (unsigned char*)the->echoBuffer + the->echoOffset;
	start = (unsigned char*)the->echoBuffer;
	stop = (unsigned char*)the->echoBuffer + sizeof(the->echoBuffer) - 1;
	while ((tmp = c_read8(src))) {
		src++;
		if (dst + 6 > stop) {
			the->echoOffset = dst - start;
			fxSend(the, 1);
			dst = start;
		}
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
	the->echoOffset = dst - start;
}

void fxGo(txMachine* the)
{
	txSlot* aSlot = the->frame;
	while (aSlot) {
		aSlot->flag &= ~(XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG);
		aSlot = aSlot->next;
	}
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
	txSlot* aProperty = mxGlobal.value.reference->next->next;
	fxEcho(the, "<global>");
	while (aProperty) {
		fxEchoProperty(the, aProperty, &aList, C_NULL, -1, C_NULL);
		aProperty = aProperty->next;
	}
	fxEcho(the, "</global>");
}

void fxListLocal(txMachine* the)
{
	txInspectorNameList aList = { C_NULL, C_NULL };
	txSlot* frame  = the->frame;
	while (frame) {
		if (frame->flag & XS_DEBUG_FLAG)
			break;
		frame = frame->next;
	}
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
	if (frame->flag & XS_C_FLAG) {
		txInteger aCount, anIndex;
		aCount = (frame + 5)->value.integer;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			fxEchoProperty(the, frame + 5 + aCount - anIndex, &aList, "arg(", anIndex, ")");
		}
		aCount = (frame - 1)->value.environment.variable.count;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			fxEchoProperty(the, frame - 2 - anIndex, &aList, "var(", anIndex, ")");
		}
	}
	else {
		txSlot* current = the->frame;
		txSlot* aScope = C_NULL;
		if (current == frame)
			aScope = the->scope;
		else {
			current = the->frame;
			while (current->next != frame)
				current = current->next;
			if (current)
				aScope = current->value.frame.scope;
		}
		if (aScope) {
			txSlot* aSlot = frame - 1;
			while (aSlot > aScope) {
				aSlot--;
				if (aSlot->ID)
					fxEchoProperty(the, aSlot, &aList, C_NULL, -1, C_NULL);
			}
		}
	}
	fxEcho(the, "</local>");
}

void fxListModules(txMachine* the)
{
	txInspectorNameList aList = { C_NULL, C_NULL };
	txSlot* table = mxModules.value.reference->next;
	txSlot** address = table->value.table.address;
	txInteger modulo = table->value.table.length;
	txSlot* module;
	fxEcho(the, "<grammar>");
	while (modulo) {
		txSlot* entry = *address;
		while (entry) {
			module = entry->value.entry.slot;
			fxEchoModule(the, module, &aList);
			entry = entry->next;
		}
		address++;
		modulo--;
	}
	module = the->sharedModules;
	while (module) {
		fxEchoModule(the, module, &aList);
		module = module->next;
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
	fxEcho(the, "XS");
	fxEcho(the, "\"/>");
	fxEchoStop(the);
	fxDebugCommand(the);
}

void fxLogout(txMachine* the)
{
	if (!fxIsConnected(the))
		return;
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

void fxSetBreakpoint(txMachine* the, txString thePath, txInteger theLine)
{
	txID path;
	txSlot* breakpoint;

	if (!thePath)
		return;
	if (!c_strcmp(thePath, "exceptions")) {
		the->breakOnExceptionsFlag = 1;
		return;
	}	
	if (!c_strcmp(thePath, "start")) {
		the->breakOnStartFlag = 1;
		return;
	}	
	if ((theLine <= 0) || (0x00007FFF < theLine))
		return;
	path = fxNewNameC(the, thePath);
	if (!path)
		return;
	breakpoint = mxBreakpoints.value.list.first;
	while (breakpoint)	{
		if ((breakpoint->ID == path) && (breakpoint->value.integer == theLine))
			return;
		breakpoint = breakpoint->next;
	}
	breakpoint = fxNewSlot(the);
	breakpoint->next = mxBreakpoints.value.list.first;
	breakpoint->ID = path;
	breakpoint->kind = XS_INTEGER_KIND;
	breakpoint->value.integer = theLine;
	mxBreakpoints.value.list.first = breakpoint;
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
			txSlot* environment = frame - 1;
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
#endif
}

void fxReport(txMachine* the, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxVReport(the, theFormat, arguments);
	c_va_end(arguments);
}

void fxReportException(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxVReportException(the, thePath, theLine, theFormat, arguments);
	c_va_end(arguments);
}

void fxReportError(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxVReportError(the, thePath, theLine, theFormat, arguments);
	c_va_end(arguments);
}

void fxReportWarning(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxVReportWarning(the, thePath, theLine, theFormat, arguments);
	c_va_end(arguments);
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
#ifndef mxNoConsole
	c_vprintf(theFormat, theArguments);
#endif
#if defined(_RENESAS_SYNERGY_) || defined(DEBUG_EFM)
	memmove(lastDebugStr, synergyDebugStr, 256);
	vsprintf(synergyDebugStr, theFormat, theArguments);
	synergyDebugStr[255] = '\0';
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
#ifndef mxNoConsole
	if (thePath && theLine)
#if mxWindows
		printf("%s(%d): exception: ", thePath, (int)theLine);
#else
		c_printf("%s:%d: exception: ", thePath, (int)theLine);
#endif
	else
		c_printf("# exception: ");
	c_vprintf(theFormat, theArguments);
	c_printf("!\n");
#endif
#if defined(_RENESAS_SYNERGY_) || defined(DEBUG_EFM)
	if (thePath && theLine)
		sprintf(synergyDebugStr, "%s:%d: exception: ", thePath, (int)theLine);
	else
		sprintf(synergyDebugStr, "# exception: ");
	memmove(lastDebugStr, synergyDebugStr, 256);
	vsprintf(synergyDebugStr, theFormat, theArguments);
	synergyDebugStr[255] = '\0';
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
#ifndef mxNoConsole
	if (thePath && theLine)
#if mxWindows
		printf("%s(%d): error: ", thePath, (int)theLine);
#else
		c_printf("%s:%d: error: ", thePath, (int)theLine);
#endif
	else
		c_printf("# error: ");
	c_vprintf(theFormat, theArguments);
	c_printf("!\n");
#endif
#if defined(_RENESAS_SYNERGY_) || defined(DEBUG_EFM)
	if (thePath && theLine)
		sprintf(synergyDebugStr, "%s:%d: error: ", thePath, (int)theLine);
	else
		sprintf(synergyDebugStr, "# error: ");
	vsprintf(synergyDebugStr, theFormat, theArguments);
	synergyDebugStr[255] = '\0';
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
#ifndef mxNoConsole
	if (thePath && theLine)
#if mxWindows
		printf("%s(%d): warning: ", thePath, (int)theLine);
#else
		c_printf("%s:%d: warning: ", thePath, (int)theLine);
#endif
	else
		c_printf("# warning: ");
	c_vprintf(theFormat, theArguments);
	c_printf("!\n");
#endif
#if defined(_RENESAS_SYNERGY_) || defined(DEBUG_EFM)
	if (thePath && theLine)
		sprintf(synergyDebugStr, "%s:%d: warning: ", thePath, (int)theLine);
	else
		sprintf(synergyDebugStr, "# warning: ");
	vsprintf(synergyDebugStr, theFormat, theArguments);
	synergyDebugStr[255] = '\0';
#endif
}

#ifdef mxInstrument	
#define xsInstrumentCount 10
static char* xsInstrumentNames[xsInstrumentCount] ICACHE_XS6STRING_ATTR = {
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
};
static char* xsInstrumentUnits[xsInstrumentCount] ICACHE_XS6STRING_ATTR = {
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
			fxEchoString(the, xsInstrumentNames[i]);
			fxEcho(the, "\" value=\"");
			fxEchoString(the, xsInstrumentUnits[i]);
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
	for (i = 0; i < count; i++, j++) {
		if (j)
			c_printf(",");
		c_printf(",%s", names[i]);
	}
	for (i = 0; i < xsInstrumentCount; i++, j++) {
		if (j)
			c_printf(",");
		c_printf(",%s", xsInstrumentNames[i]);
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
	xsInstrumentValues[4] = (the->stackTop - the->stackPeak) * sizeof(txSlot);
	xsInstrumentValues[5] = (the->stackTop - the->stackBottom) * sizeof(txSlot);
	xsInstrumentValues[6] = the->garbageCollectionCount;
	xsInstrumentValues[7] = the->keyIndex - the->keyOffset;
	xsInstrumentValues[8] = the->loadedModulesCount;
	xsInstrumentValues[9] = the->peakParserSize;

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
#endif

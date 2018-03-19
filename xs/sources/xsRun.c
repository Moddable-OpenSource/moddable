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

//#define mxTrace 1
//#define mxTraceCall 1

#define c_iszero(NUMBER) (FP_ZERO == c_fpclassify(NUMBER))

extern void fxRemapIDs(txMachine* the, txByte* codeBuffer, txSize codeSize, txID* theIDs);
static void fxRunBase(txMachine* the);
static void fxRunConstructor(txMachine* the);
static txBoolean fxRunDefine(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask);
static txBoolean fxRunDelete(txMachine* the, txSlot* instance, txID id, txIndex index);
static void fxRunDerived(txMachine* the);
static void fxRunExtends(txMachine* the);
static void fxRunEval(txMachine* the);
static void fxRunForAwaitOf(txMachine* the);
static void fxRunForOf(txMachine* the);
static void fxRunIn(txMachine* the);
static void fxRunProxy(txMachine* the, txSlot* instance);
static void fxRunInstanceOf(txMachine* the);
static txBoolean fxIsScopableSlot(txMachine* the, txSlot* instance, txID id);

#if defined(__GNUC__) && defined(__OPTIMIZE__)
	#if defined(mxFrequency)
		#define mxBreak \
			the->frequencies[byte]++; \
			goto *bytes[byte]
	#elif defined(mxTrace)
		#define mxBreak \
			if (gxDoTrace) fxTraceCode(the, byte); \
			goto *bytes[byte]
	#else
		#define mxBreak \
			goto *bytes[byte]
	#endif
	#define mxCase(OPCODE) OPCODE:
	#define mxSwitch(OPCODE) mxBreak;
#else
	#define mxBreak \
		break
	#define mxCase(OPCODE) \
		case OPCODE:
	#if defined(mxFrequency)
		#define mxSwitch(OPCODE) \
			the->frequencies[OPCODE]++; \
			switch(OPCODE)
	#else
		#define mxSwitch(OPCODE) \
			switch(OPCODE)
	#endif
#endif

#define mxCode code
#define mxFrame frame
#define mxScope scope
#define mxStack stack

#define mxFrameArgv(THE_INDEX) (mxFrame + 5 + ((mxFrame + 5)->value.integer) - (THE_INDEX))
#define mxFrameArgc ((mxFrame + 5)->value.integer)
#define mxFrameThis (mxFrame + 4)
#define mxFrameFunction (mxFrame + 3)
#define mxFrameTarget (mxFrame + 2)
#define mxFrameResult (mxFrame + 1)
#define mxFrameEnvironment (mxFrame - 1)

#define mxRestoreState { \
	txSlot** it = (txSlot**)the; \
	mxStack = *it++; \
	mxScope = *it++; \
	mxFrame = *it++; \
	mxCode = *((txByte**)it); \
}
#if defined(mxFrequency)
	#define mxSaveState { \
		the->exits[byte]++; \
		the->stack = mxStack; \
		the->scope = mxScope; \
		the->frame = mxFrame; \
		the->code = mxCode; \
	}
#else
	#define mxSaveState { \
		txSlot** it = (txSlot**)the; \
		*it++ = mxStack; \
		*it++ =	mxScope; \
		*it++ = mxFrame; \
		*it = (txSlot*)mxCode; \
	}
#endif

#ifdef mxDebug
#define mxOverflow(_COUNT) \
	if ((mxStack - _COUNT) < the->stackBottom) { \
		mxSaveState; \
		fxReport(the, "stack overflow (%ld)!\n", ((the->stack - _COUNT) - the->stackBottom)); \
		fxJump(the); \
	} \
	mxStack -= _COUNT
#else
#define mxOverflow(_COUNT) \
	mxStack -= _COUNT
#endif

#define mxPushKind(_KIND) { \
	mxOverflow(1); \
	mxStack->next = C_NULL;  \
	mxStack->flag = XS_NO_FLAG;  \
	mxStack->kind = _KIND; \
}

#define mxRunDebug(_ERROR, ...) { \
	mxSaveState; \
	fxThrowMessage(the, NULL, 0, _ERROR, __VA_ARGS__); \
}

#define mxRunDebugID(_ERROR, _MESSAGE, _ID) { \
	mxSaveState; \
	fxIDToString(the, _ID, the->nameBuffer, sizeof(the->nameBuffer)); \
	fxThrowMessage(the, NULL, 0, _ERROR, _MESSAGE, the->nameBuffer); \
}

#define mxToBoolean(SLOT) \
	if (XS_BOOLEAN_KIND != (SLOT)->kind) { \
		if (XS_SYMBOL_KIND <= (SLOT)->kind) { \
			mxSaveState; \
			fxToBoolean(the, SLOT); \
			mxRestoreState; \
		} \
		else \
			fxToBoolean(the, SLOT); \
	}

#define mxToInteger(SLOT) \
	if (XS_INTEGER_KIND != (SLOT)->kind) { \
		if (XS_SYMBOL_KIND <= (SLOT)->kind) { \
			mxSaveState; \
			fxToInteger(the, SLOT); \
			mxRestoreState; \
		} \
		else \
			fxToInteger(the, SLOT); \
	}

#define mxToNumber(SLOT) \
	if (XS_NUMBER_KIND != (SLOT)->kind) { \
		if (XS_SYMBOL_KIND <= (SLOT)->kind) { \
			mxSaveState; \
			fxToNumber(the, SLOT); \
			mxRestoreState; \
		} \
		else \
			fxToNumber(the, SLOT); \
	}
	
#define mxToString(SLOT) \
	if ((XS_STRING_KIND != (SLOT)->kind) && (XS_STRING_X_KIND != (SLOT)->kind)) { \
		mxSaveState; \
		fxToString(the, SLOT); \
		mxRestoreState; \
	}
	
#define mxToInstance(SLOT) \
	if (XS_REFERENCE_KIND != (SLOT)->kind) { \
		mxSaveState; \
		variable = fxToInstance(the, SLOT); \
		mxRestoreState; \
	} \
	else { \
		variable = (SLOT)->value.reference; \
	}
	
#ifdef __ets__
	#define mxNextCode(OFFSET) { \
		mxCode += OFFSET; \
		byte = c_read8(mxCode); \
	}
	#define mxSkipCode(OFFSET) { \
		mxCode += OFFSET; \
	}
	#define mxRunS1(OFFSET) ((txS1)c_read8(mxCode + OFFSET))
	#define mxRunS2(OFFSET) ((txS2)c_read16be(mxCode + OFFSET))
	#define mxRunS4(OFFSET) ((txS4)c_read32be(mxCode + OFFSET))

	#define mxRunU1(OFFSET) ((txU1)c_read8(mxCode + OFFSET))
	#define mxRunU2(OFFSET) ((txU2)c_read16be(mxCode + OFFSET))

	#undef mxDecode8
	#if mxLittleEndian
		#define mxDecode8(THE_CODE, THE_VALUE) { \
			txS1* dst = (txS1*)&(THE_VALUE); \
			dst[7] = mxRunS1(0); \
			dst[6] = mxRunS1(1); \
			dst[5] = mxRunS1(2); \
			dst[4] = mxRunS1(3); \
			dst[3] = mxRunS1(4); \
			dst[2] = mxRunS1(5); \
			dst[1] = mxRunS1(6); \
			dst[0] = mxRunS1(7); \
			mxSkipCode(8); \
		}
	#else
		#define mxDecode8(THE_CODE, THE_VALUE)	{ \
			txS1* dst = (txS1*)&(THE_VALUE); \
			dst[0] = mxRunS1(0); \
			dst[1] = mxRunS1(1); \
			dst[2] = mxRunS1(2); \
			dst[3] = mxRunS1(3); \
			dst[4] = mxRunS1(4); \
			dst[5] = mxRunS1(5); \
			dst[6] = mxRunS1(6); \
			dst[7] = mxRunS1(7); \
			mxSkipCode(8); \
			}
	#endif
#else
	#define mxNextCode(OFFSET) { \
		mxCode += OFFSET; \
		byte = *((txU1*)mxCode); \
	}
	#define mxSkipCode(OFFSET) { \
		mxCode += OFFSET; \
	}
	#define mxRunS1(OFFSET) ((txS1*)mxCode)[OFFSET+0]
	#define mxRunS2(OFFSET) (((txS1*)mxCode)[OFFSET+0] << 8) | ((txU1*)mxCode)[OFFSET+1]
	#define mxRunS4(OFFSET) (((txS1*)mxCode)[OFFSET+0] << 24) | (((txU1*)mxCode)[OFFSET+1] << 16) | (((txU1*)mxCode)[OFFSET+2] << 8) | ((txU1*)mxCode)[OFFSET+3]

	#define mxRunU1(OFFSET) ((txU1*)mxCode)[OFFSET]
	#define mxRunU2(OFFSET) (((txU1*)mxCode)[OFFSET+0] << 8) | ((txU1*)mxCode)[OFFSET+1]
#endif

#ifdef mxTrace
short gxDoTrace = 1;

static void fxTraceCode(txMachine* the, txU1 theCode) 
{
	if (((XS_NO_CODE < theCode) && (theCode < XS_CODE_COUNT)))
		fprintf(stderr, "\n%s", gxCodeNames[theCode]);
	else
		fprintf(stderr, "\n?");
}

static void fxTraceID(txMachine* the, txInteger id) 
{
	if (id >= 0)
		fprintf(stderr, " [%ld]", id);
	else {	
		char* key = fxGetKeyName(the, id);
		if (key)
			fprintf(stderr, " [%s]", key);
		else
			fprintf(stderr, " [?]");
	}
}

static void fxTraceIndex(txMachine* the, txU2 theIndex) 
{
	fprintf(stderr, " %d", theIndex);
}

static void fxTraceInteger(txMachine* the, txInteger theInteger) 
{
	fprintf(stderr, " %ld", theInteger);
}

static void fxTraceNumber(txMachine* the, txNumber theNumber) 
{
	fprintf(stderr, " %lf", theNumber);
}

static void fxTraceReturn(txMachine* the) 
{
	fprintf(stderr, "\n");
}

static void fxTraceString(txMachine* the, txString theString) 
{
	fprintf(stderr, " \"%s\"", theString);
}
#endif

#ifdef mxTraceCall
int depth = 0;

static void fxTraceCallBegin(txMachine* the, txSlot* function)
{
	txSlot* slot = mxBehaviorGetProperty(the, function->value.reference, mxID(_name), XS_NO_ID, XS_ANY);
	int i;
	for (i = 0; i < depth; i++)
		fprintf(stderr, "\t");
	if (slot && (slot->kind == XS_STRING_KIND) &&  (slot->kind == XS_STRING_X_KIND))
		fprintf(stderr, " [%s]\n", slot->value.string);
	else
		fprintf(stderr, " [?]\n");
	depth++;
}
static void fxTraceCallEnd(txMachine* the, txSlot* function)
{
	depth--;
}
#endif

#ifdef mxLink
extern txCallback gxFakeCallback;
#endif

void fxRunID(txMachine* the, txSlot* generator, txID id)
{
	register txSlot* stack = the->stack;
	register txSlot* scope = the->scope;
	register txSlot* frame = the->frame;
	register txByte* code = the->code;
	register txSlot* variable;
	register txSlot* slot;
	register txU1 byte = 0;
	register txU4 index;
	register txS4 offset;
#if defined(__GNUC__) && defined(__OPTIMIZE__)
	static void *const ICACHE_RAM_ATTR gxBytes[] = {
		&&XS_NO_CODE,
		&&XS_CODE_ADD,
		&&XS_CODE_ARGUMENT,
		&&XS_CODE_ARGUMENTS,
		&&XS_CODE_ARGUMENTS_SLOPPY,
		&&XS_CODE_ARGUMENTS_STRICT,
		&&XS_CODE_ARRAY,
		&&XS_CODE_ASYNC_FUNCTION,
		&&XS_CODE_ASYNC_GENERATOR_FUNCTION,
		&&XS_CODE_AT,
		&&XS_CODE_AWAIT,
		&&XS_CODE_BEGIN_SLOPPY,
		&&XS_CODE_BEGIN_STRICT,
		&&XS_CODE_BEGIN_STRICT_BASE,
		&&XS_CODE_BEGIN_STRICT_DERIVED,
		&&XS_CODE_BIT_AND,
		&&XS_CODE_BIT_NOT,
		&&XS_CODE_BIT_OR,
		&&XS_CODE_BIT_XOR,
		&&XS_CODE_BRANCH_1,
		&&XS_CODE_BRANCH_2,
		&&XS_CODE_BRANCH_4,
		&&XS_CODE_BRANCH_ELSE_1,
		&&XS_CODE_BRANCH_ELSE_2,
		&&XS_CODE_BRANCH_ELSE_4,
		&&XS_CODE_BRANCH_IF_1,
		&&XS_CODE_BRANCH_IF_2,
		&&XS_CODE_BRANCH_IF_4,
		&&XS_CODE_BRANCH_STATUS_1,
		&&XS_CODE_BRANCH_STATUS_2,
		&&XS_CODE_BRANCH_STATUS_4,
		&&XS_CODE_CALL,
		&&XS_CODE_CALL_TAIL,
		&&XS_CODE_CATCH_1,
		&&XS_CODE_CATCH_2,
		&&XS_CODE_CATCH_4,
		&&XS_CODE_CHECK_INSTANCE,
		&&XS_CODE_CLASS,
		&&XS_CODE_CODE_1,
		&&XS_CODE_CODE_2,
		&&XS_CODE_CODE_4,
		&&XS_CODE_CODE_ARCHIVE_1,
		&&XS_CODE_CODE_ARCHIVE_2,
		&&XS_CODE_CODE_ARCHIVE_4,
		&&XS_CODE_CONST_CLOSURE_1,
		&&XS_CODE_CONST_CLOSURE_2,
		&&XS_CODE_CONST_LOCAL_1,
		&&XS_CODE_CONST_LOCAL_2,
		&&XS_CODE_CONSTRUCTOR_FUNCTION,
		&&XS_CODE_CURRENT,
		&&XS_CODE_DEBUGGER,
		&&XS_CODE_DECREMENT,
		&&XS_CODE_DELETE_PROPERTY,
		&&XS_CODE_DELETE_PROPERTY_AT,
		&&XS_CODE_DELETE_SUPER,
		&&XS_CODE_DELETE_SUPER_AT,
		&&XS_CODE_DIVIDE,
		&&XS_CODE_DUB,
		&&XS_CODE_DUB_AT,
		&&XS_CODE_END,
		&&XS_CODE_END_ARROW,
		&&XS_CODE_END_BASE,
		&&XS_CODE_END_DERIVED,
		&&XS_CODE_ENVIRONMENT,
		&&XS_CODE_EQUAL,
		&&XS_CODE_EVAL,
		&&XS_CODE_EVAL_ENVIRONMENT,
		&&XS_CODE_EVAL_REFERENCE,
		&&XS_CODE_EXCEPTION,
		&&XS_CODE_EXPONENTIATION,
		&&XS_CODE_EXTEND,
		&&XS_CODE_FALSE,
		&&XS_CODE_FILE,
		&&XS_CODE_FOR_AWAIT_OF,
		&&XS_CODE_FOR_IN,
		&&XS_CODE_FOR_OF,
		&&XS_CODE_FUNCTION,
		&&XS_CODE_GENERATOR_FUNCTION,
		&&XS_CODE_GET_CLOSURE_1,
		&&XS_CODE_GET_CLOSURE_2,
		&&XS_CODE_GET_LOCAL_1,
		&&XS_CODE_GET_LOCAL_2,
		&&XS_CODE_GET_PROPERTY,
		&&XS_CODE_GET_PROPERTY_AT,
		&&XS_CODE_GET_SUPER,
		&&XS_CODE_GET_SUPER_AT,
		&&XS_CODE_GET_THIS,
		&&XS_CODE_GET_VARIABLE,
		&&XS_CODE_GLOBAL,
		&&XS_CODE_HOST,
		&&XS_CODE_IN,
		&&XS_CODE_INCREMENT,
		&&XS_CODE_INSTANCEOF,
		&&XS_CODE_INSTANTIATE,
		&&XS_CODE_INTEGER_1,
		&&XS_CODE_INTEGER_2,
		&&XS_CODE_INTEGER_4,
		&&XS_CODE_INTRINSIC,
		&&XS_CODE_LEFT_SHIFT,
		&&XS_CODE_LESS,
		&&XS_CODE_LESS_EQUAL,
		&&XS_CODE_LET_CLOSURE_1,
		&&XS_CODE_LET_CLOSURE_2,
		&&XS_CODE_LET_LOCAL_1,
		&&XS_CODE_LET_LOCAL_2,
		&&XS_CODE_LINE,
		&&XS_CODE_MINUS,
		&&XS_CODE_MODULE,
		&&XS_CODE_MODULO,
		&&XS_CODE_MORE,
		&&XS_CODE_MORE_EQUAL,
		&&XS_CODE_MULTIPLY,
		&&XS_CODE_NAME,
		&&XS_CODE_NEW,
		&&XS_CODE_NEW_CLOSURE,
		&&XS_CODE_NEW_LOCAL,
		&&XS_CODE_NEW_PROPERTY,
		&&XS_CODE_NEW_TEMPORARY,
		&&XS_CODE_NOT,
		&&XS_CODE_NOT_EQUAL,
		&&XS_CODE_NULL,
		&&XS_CODE_NUMBER,
		&&XS_CODE_OBJECT,
		&&XS_CODE_PLUS,
		&&XS_CODE_POP,
		&&XS_CODE_PROGRAM_ENVIRONMENT,
		&&XS_CODE_PROGRAM_REFERENCE,
		&&XS_CODE_PULL_CLOSURE_1,
		&&XS_CODE_PULL_CLOSURE_2,
		&&XS_CODE_PULL_LOCAL_1,
		&&XS_CODE_PULL_LOCAL_2,
		&&XS_CODE_REFRESH_CLOSURE_1,
		&&XS_CODE_REFRESH_CLOSURE_2,
		&&XS_CODE_REFRESH_LOCAL_1,
		&&XS_CODE_REFRESH_LOCAL_2,
		&&XS_CODE_RESERVE_1,
		&&XS_CODE_RESERVE_2,
		&&XS_CODE_RESET_CLOSURE_1,
		&&XS_CODE_RESET_CLOSURE_2,
		&&XS_CODE_RESET_LOCAL_1,
		&&XS_CODE_RESET_LOCAL_2,
		&&XS_CODE_RESULT,
		&&XS_CODE_RETHROW,
		&&XS_CODE_RETRIEVE_1,
		&&XS_CODE_RETRIEVE_2,
		&&XS_CODE_RETRIEVE_TARGET,
		&&XS_CODE_RETRIEVE_THIS,
		&&XS_CODE_RETURN,
		&&XS_CODE_SET_CLOSURE_1,
		&&XS_CODE_SET_CLOSURE_2,
		&&XS_CODE_SET_LOCAL_1,
		&&XS_CODE_SET_LOCAL_2,
		&&XS_CODE_SET_PROPERTY,
		&&XS_CODE_SET_PROPERTY_AT,
		&&XS_CODE_SET_SUPER,
		&&XS_CODE_SET_SUPER_AT,
		&&XS_CODE_SET_THIS,
		&&XS_CODE_SET_VARIABLE,
		&&XS_CODE_SIGNED_RIGHT_SHIFT,
		&&XS_CODE_START_ASYNC,
		&&XS_CODE_START_ASYNC_GENERATOR,
		&&XS_CODE_START_GENERATOR,
		&&XS_CODE_STORE_1,
		&&XS_CODE_STORE_2,
		&&XS_CODE_STORE_ARROW,
		&&XS_CODE_STRICT_EQUAL,
		&&XS_CODE_STRICT_NOT_EQUAL,
		&&XS_CODE_STRING_1,
		&&XS_CODE_STRING_2,
		&&XS_CODE_STRING_ARCHIVE_1,
		&&XS_CODE_STRING_ARCHIVE_2,
		&&XS_CODE_SUBTRACT,
		&&XS_CODE_SUPER,
		&&XS_CODE_SWAP,
		&&XS_CODE_SYMBOL,
		&&XS_CODE_TARGET,
		&&XS_CODE_TEMPLATE,
		&&XS_CODE_THIS,
		&&XS_CODE_THROW,
		&&XS_CODE_THROW_STATUS,
		&&XS_CODE_TO_INSTANCE,
		&&XS_CODE_TRANSFER,
		&&XS_CODE_TRUE,
		&&XS_CODE_TYPEOF,
		&&XS_CODE_UNCATCH,
		&&XS_CODE_UNDEFINED,
		&&XS_CODE_UNSIGNED_RIGHT_SHIFT,
		&&XS_CODE_UNWIND_1,
		&&XS_CODE_UNWIND_2,
		&&XS_CODE_VAR_CLOSURE_1,
		&&XS_CODE_VAR_CLOSURE_2,
		&&XS_CODE_VAR_LOCAL_1,
		&&XS_CODE_VAR_LOCAL_2,
		&&XS_CODE_VOID,
		&&XS_CODE_WITH,
		&&XS_CODE_WITHOUT,
		&&XS_CODE_YIELD,
	};
	register void * const *bytes = gxBytes;
#endif
	txJump* jump = C_NULL;
	txSlot scratch;
	txSlot** address;
	txJump* yieldJump = the->firstJump;

	if (generator) {
		slot = mxStack;
		variable = generator->next;
		offset = variable->value.stack.length;
		mxOverflow(offset);
		c_memcpy(mxStack, variable->value.stack.address, offset * sizeof(txSlot));
		mxCode = (mxStack++)->value.reference->next->value.code.address;
		offset = (mxStack++)->value.integer;
		while (offset) {
			jump = c_malloc(sizeof(txJump));
			if (jump) {
				jump->nextJump = the->firstJump;
				jump->stack = slot - (mxStack++)->value.integer;
				jump->frame = slot - (mxStack++)->value.integer;
				jump->scope = slot - (mxStack++)->value.integer;
				jump->code = mxCode + (mxStack++)->value.integer;
				jump->flag = 1;
                the->firstJump = jump;
				if (c_setjmp(jump->buffer) == 1) {
					jump = the->firstJump;
                    the->firstJump = jump->nextJump;
					mxStack = jump->stack;
					mxFrame = jump->frame;
					mxScope = jump->scope;
					mxCode = jump->code;
					c_free(jump);
                    goto XS_CODE_JUMP;
				}
			}
			else {
				mxSaveState;
				fxJump(the);
			}
			offset--;
		}
		variable = slot - (mxStack++)->value.integer;
		variable->next = mxFrame;
		variable->flag = XS_NO_FLAG;
#ifdef mxDebug
		if (mxFrame && (mxFrame->flag & XS_STEP_INTO_FLAG))
			variable->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
		variable->value.frame.code = the->code;
		variable->value.frame.scope = mxScope;
		mxFrame = variable;
		mxScope = slot - (mxStack++)->value.integer;
		mxCode = mxCode + (mxStack++)->value.integer;
		mxStack->kind = the->scratch.kind;
		mxStack->value = the->scratch.value;
		mxFrameResult->kind = XS_UNDEFINED_KIND;
#ifdef mxTraceCall
		fxTraceCallBegin(the, mxFrameFunction);
#endif
#ifdef mxProfile
		fxBeginFunction(the, mxFrameFunction);
#endif
XS_CODE_JUMP:
		mxNextCode(0);
	}
	else {
#ifdef mxHostFunctionPrimitive
		slot = mxStack + 2;
		if (slot->kind != XS_HOST_FUNCTION_KIND)
			slot = slot->value.reference;
#else
		slot = (mxStack + 2)->value.reference;
#endif
		offset = XS_NO_ID;
		goto XS_CODE_CALL_ALL;
	}
	for (;;) {

#ifdef mxTrace
		if (gxDoTrace) fxTraceCode(the, byte);
#endif
		//fxCheckStack(the, mxStack);
		
		mxSwitch(byte) {
		mxCase(XS_NO_CODE)
			mxBreak;
			
		mxCase(XS_CODE_BEGIN_SLOPPY)
		#ifdef mxSloppy
            if (mxFrameTarget->kind != XS_UNDEFINED_KIND) {
				mxSaveState;
				fxRunConstructor(the);
				mxRestoreState;
			}
			else {
				index = mxFrameThis->kind;
				if (index < XS_REFERENCE_KIND) {
					if ((index == XS_UNDEFINED_KIND) || (index == XS_NULL_KIND))
						*mxFrameThis = mxGlobal;
					else {
						mxSaveState;
						fxToInstance(the, mxFrameThis);
						mxRestoreState;
					}
				}
			}
			mxNextCode(2);
		#endif
			mxBreak;
		mxCase(XS_CODE_BEGIN_STRICT)
			mxFrame->flag |= XS_STRICT_FLAG;
            if (mxFrameTarget->kind != XS_UNDEFINED_KIND) {
				mxSaveState;
				fxRunConstructor(the);
				mxRestoreState;
			}
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_BEGIN_STRICT_BASE)
			mxFrame->flag |= XS_STRICT_FLAG;
			mxSaveState;
			fxRunBase(the);
			mxRestoreState;
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_BEGIN_STRICT_DERIVED)
			mxFrame->flag |= XS_STRICT_FLAG;
			mxSaveState;
			fxRunDerived(the);
			mxRestoreState;
			mxNextCode(2);
			mxBreak;
			
		mxCase(XS_CODE_CALL)
			mxSkipCode(1);
		#ifdef mxHostFunctionPrimitive
            slot = mxStack;
			if (slot->kind != XS_HOST_FUNCTION_KIND) {
				slot = fxGetInstance(the, slot);
				if (!mxIsCallable(slot))
					mxRunDebugID(XS_TYPE_ERROR, "call %s: no function", offset);
			}
		#else
			slot = fxGetInstance(the, mxStack);
			if (!mxIsCallable(slot)) 
				mxRunDebugID(XS_TYPE_ERROR, "call %s: no function", offset);
		#endif
			/* TARGET */
			mxPushKind(XS_UNDEFINED_KIND);
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
		XS_CODE_CALL_ALL:
			/* FRAME */
			mxOverflow(1);
			mxStack->kind = XS_FRAME_KIND;
			mxStack->next = mxFrame;
			mxStack->flag = XS_NO_FLAG;
#ifdef mxDebug
			if (mxFrame && (mxFrame->flag & XS_STEP_INTO_FLAG))
				mxStack->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
			mxStack->value.frame.code = mxCode;
			mxStack->value.frame.scope = mxScope;
		XS_CODE_CALL_STACK:
			mxFrame = mxStack;
#ifdef mxTraceCall
		fxTraceCallBegin(the, mxFrameFunction);
#endif
#ifdef mxProfile
			fxBeginFunction(the, mxFrameFunction);
#endif
		#ifdef mxHostFunctionPrimitive
			if (slot->kind == XS_HOST_FUNCTION_KIND) {
				mxFrame->flag |= XS_C_FLAG;
				mxPushKind(XS_VAR_KIND);
				mxStack->value.environment.variable.count = 0;
#ifdef mxDebug
				mxStack->ID = XS_NO_ID;
				mxStack->value.environment.line = 0;
#endif
				mxScope = mxStack;
				mxCode = (txByte*)slot->value.hostFunction.IDs;
				byte = XS_CODE_CALL;
				mxSaveState;
#ifdef mxLink
				if ((txU1*)slot->value.hostFunction.builder->callback - (txU1*)gxFakeCallback < 0)
					mxRunDebugID(XS_TYPE_ERROR, "call %s: no function", offset);
#endif
				(*(slot->value.hostFunction.builder->callback))(the);
				mxRestoreState;
				slot = mxFrameResult;
				goto XS_CODE_END_ALL;
			}
		#endif
			if ((slot->next->kind == XS_CALLBACK_KIND) || (slot->next->kind == XS_CALLBACK_X_KIND)) {
				mxFrame->flag |= XS_C_FLAG;
				mxPushKind(XS_VAR_KIND);
				mxStack->value.environment.variable.count = 0;
#ifdef mxDebug
				mxStack->ID = XS_NO_ID;
				mxStack->value.environment.line = 0;
#endif
				mxScope = mxStack;
				mxCode = (txByte*)slot->next->value.callback.IDs;
				byte = XS_CODE_CALL;
				mxSaveState;
#ifdef mxLink
				if ((txU1*)slot->next->value.callback.address - (txU1*)gxFakeCallback < 0)
					mxRunDebugID(XS_TYPE_ERROR, "call %s: no function", offset);
#endif
				if (slot->flag & XS_BASE_FLAG)
					fxRunBase(the);
				else if (slot->flag & XS_DERIVED_FLAG)
					fxRunDerived(the);
				(*(slot->next->value.callback.address))(the);
				mxRestoreState;
				if (slot->flag & XS_BASE_FLAG)
					goto XS_CODE_END_BASE_ALL;
				if (slot->flag & XS_DERIVED_FLAG)
					goto XS_CODE_END_DERIVED_ALL;
				slot = mxFrameResult;
				goto XS_CODE_END_ALL;
			}
			if (slot->next->kind == XS_PROXY_KIND) {
				mxFrame->flag |= XS_C_FLAG;
				mxPushKind(XS_VAR_KIND);
				mxStack->value.environment.variable.count = 0;
#ifdef mxDebug
				mxStack->ID = XS_NO_ID;
				mxStack->value.environment.line = 0;
#endif
				mxScope = mxStack;
				mxCode = C_NULL;
				byte = XS_CODE_CALL;
				mxSaveState;
				fxRunProxy(the, slot);
				mxRestoreState;
				slot = mxFrameResult;
				goto XS_CODE_END_ALL;
			}
			variable = slot->next->value.code.closures;
			if (variable) {
				mxPushKind(XS_REFERENCE_KIND);
				mxStack->value.environment.variable.reference = variable;
			}
			else {
				mxPushKind(XS_NULL_KIND);
				mxStack->value.environment.variable.reference = C_NULL;
			}
#ifdef mxDebug
			mxStack->ID = XS_NO_ID;
			mxStack->value.environment.line = 0;
#endif
			mxScope = mxStack;
			mxCode = slot->next->value.code.address;
			mxNextCode(0);
			mxBreak;
		mxCase(XS_CODE_CALL_TAIL)
			mxSkipCode(1);
		#ifdef mxHostFunctionPrimitive
            slot = mxStack;
			if (slot->kind != XS_HOST_FUNCTION_KIND) {
				slot = fxGetInstance(the, slot);
				if (!mxIsCallable(slot))
					mxRunDebugID(XS_TYPE_ERROR, "call %s: no function", offset);
			}
		#else
			slot = fxGetInstance(the, mxStack);
			if (!mxIsCallable(slot)) 
				mxRunDebugID(XS_TYPE_ERROR, "call %s: no function", offset);
		#endif
			/* TARGET */
			mxPushKind(XS_UNDEFINED_KIND);
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
			if (mxFrameTarget->kind)
				goto XS_CODE_CALL_ALL;
			/* FRAME */
			mxOverflow(1);
			mxStack->kind = XS_FRAME_KIND;
			mxStack->next = mxFrame->next;
			mxStack->flag = XS_NO_FLAG;
#ifdef mxDebug
			if (mxFrame && (mxFrame->flag & XS_STEP_INTO_FLAG))
				mxStack->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
			mxStack->value.frame.code = mxFrame->value.frame.code;
			mxStack->value.frame.scope = mxFrame->value.frame.scope;
#ifdef mxTraceCall
			fxTraceCallEnd(the, mxFrameFunction);
#endif
#ifdef mxProfile
			fxEndFunction(the, mxFrameFunction);
#endif
			offset = 6 + ((mxStack + 5)->value.integer);
			variable = mxFrame + 6 + ((mxFrame + 5)->value.integer) - offset;
			c_memmove(variable, mxStack, offset * sizeof(txSlot));
			mxStack = variable;
			goto XS_CODE_CALL_STACK;
			
		mxCase(XS_CODE_END_ARROW)
			slot = mxFrameResult;
			goto XS_CODE_END_ALL;
		mxCase(XS_CODE_END_BASE)
		XS_CODE_END_BASE_ALL:
			slot = mxFrameResult;
			if (slot->kind != XS_REFERENCE_KIND)
				slot = mxFrameThis;
			goto XS_CODE_END_ALL;
		mxCase(XS_CODE_END_DERIVED)
		XS_CODE_END_DERIVED_ALL:
			slot = mxFrameResult;
			if (slot->kind != XS_REFERENCE_KIND) {
				if ((slot->kind != XS_UNDEFINED_KIND) && (slot->kind != XS_CLOSURE_KIND))
					mxRunDebug(XS_TYPE_ERROR, "invalid result");
				slot = mxFrameThis->value.closure;
				if (slot->kind < 0)
					mxRunDebug(XS_REFERENCE_ERROR, "this is not initialized");
			}
			goto XS_CODE_END_ALL;
		mxCase(XS_CODE_END)
			slot = mxFrameResult;
			if (mxFrameTarget->kind) {
				if (slot->kind != XS_REFERENCE_KIND)
					slot = mxFrameThis;
			}
		XS_CODE_END_ALL:
#ifdef mxTraceCall
			fxTraceCallEnd(the, mxFrameFunction);
#endif
#ifdef mxProfile
			fxEndFunction(the, mxFrameFunction);
#endif
#ifdef mxInstrument
			if (the->stackPeak > mxStack)
				the->stackPeak = mxStack;
#endif
			mxStack = mxFrameArgv(-1);
			mxScope = mxFrame->value.frame.scope;
			mxCode = mxFrame->value.frame.code;
			mxOverflow(1);
			*mxStack = *slot;
			mxFrame = mxFrame->next;
			if (!mxFrame || (mxFrame->flag & XS_C_FLAG)) {
#ifdef mxTrace
				if (gxDoTrace) fxTraceReturn(the);
#endif
				byte = XS_CODE_END;
				mxSaveState;
				return;	
			}
			mxNextCode(0);
			mxBreak;
		mxCase(XS_CODE_RETURN)
#ifdef mxProfile
			fxEndFunction(the, mxFrameFunction);
#endif
			mxStack = mxFrameArgv(-1);
			mxScope = mxFrame->value.frame.scope;
			mxCode = mxFrame->value.frame.code;
			mxOverflow(1);
			*mxStack = *mxFrameResult;
			mxFrame = mxFrame->next;
#ifdef mxTrace
			if (gxDoTrace)
				if (gxDoTrace) fxTraceReturn(the);
#endif
			mxSaveState;
			return;
			
		mxCase(XS_CODE_START_ASYNC)
			mxSkipCode(1);
            if (mxFrameTarget->kind != XS_UNDEFINED_KIND)
				mxRunDebug(XS_TYPE_ERROR, "new async");
			mxSaveState;
			variable = gxDefaults.newAsyncInstance(the);
			mxRestoreState;
			*mxFrameResult = *mxStack;
			slot = mxFrameArgv(-1);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxCode - mxFrameFunction->value.reference->next->value.code.address;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = slot - mxScope;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = slot - mxFrame;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = 0;
			mxPushKind(mxFrameFunction->kind);
			mxStack->value = mxFrameFunction->value;
			index = slot - mxStack;
			slot = variable->next;
			variable = slot->value.stack.address;
			if (slot->value.stack.length < index) {
				mxSaveState;
				if (variable)
					variable = (txSlot *)fxRenewChunk(the, variable, index * sizeof(txSlot));
				if (!variable)
					variable = (txSlot *)fxNewChunk(the, index * sizeof(txSlot));
				mxRestoreState;
				slot->value.stack.address = variable;
			}
			slot->value.stack.length = index;
			c_memcpy(variable, mxStack, index * sizeof(txSlot));
			mxSaveState;
			gxDefaults.runAsync(the, mxFrameResult->value.reference);
			mxRestoreState;
 			slot = mxFrameResult;
 			goto XS_CODE_END_ALL;
 			
		mxCase(XS_CODE_START_ASYNC_GENERATOR)
			mxSkipCode(1);
             if (mxFrameTarget->kind != XS_UNDEFINED_KIND)
				mxRunDebug(XS_TYPE_ERROR, "new async generator");
			slot = mxBehaviorGetProperty(the, mxFrameFunction->value.reference, mxID(_prototype), XS_NO_ID, XS_ANY);
			mxPushKind(slot->kind);
			mxStack->value = slot->value;
			mxSaveState;
			variable = gxDefaults.newAsyncGeneratorInstance(the);
			mxRestoreState;
			
			*mxFrameResult = *mxStack;
			slot = mxFrameArgv(-1);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxCode - mxFrameFunction->value.reference->next->value.code.address;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = slot - mxScope;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = slot - mxFrame;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = 0;
			mxPushKind(mxFrameFunction->kind);
			mxStack->value = mxFrameFunction->value;
			index = slot - mxStack;
			slot = variable->next;
			variable = slot->value.stack.address;
			if (slot->value.stack.length < index) {
				mxSaveState;
				if (variable)
					variable = (txSlot *)fxRenewChunk(the, variable, index * sizeof(txSlot));
				if (!variable)
					variable = (txSlot *)fxNewChunk(the, index * sizeof(txSlot));
				mxRestoreState;
				slot->value.stack.address = variable;
			}
			slot->value.stack.length = index;
			c_memcpy(variable, mxStack, index * sizeof(txSlot));
 			slot = mxFrameResult;
			goto XS_CODE_END_ALL;
		
		mxCase(XS_CODE_START_GENERATOR)
			mxSkipCode(1);
            if (mxFrameTarget->kind != XS_UNDEFINED_KIND)
				mxRunDebug(XS_TYPE_ERROR, "new generator");
			slot = mxBehaviorGetProperty(the, mxFrameFunction->value.reference, mxID(_prototype), XS_NO_ID, XS_ANY);
			mxPushKind(slot->kind);
			mxStack->value = slot->value;
			mxSaveState;
			variable = gxDefaults.newGeneratorInstance(the);
			mxRestoreState;
			
			*mxFrameResult = *mxStack;
			slot = mxFrameArgv(-1);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxCode - mxFrameFunction->value.reference->next->value.code.address;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = slot - mxScope;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = slot - mxFrame;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = 0;
			mxPushKind(mxFrameFunction->kind);
			mxStack->value = mxFrameFunction->value;
			index = slot - mxStack;
			slot = variable->next;
			variable = slot->value.stack.address;
			if (slot->value.stack.length < index) {
				mxSaveState;
				if (variable)
					variable = (txSlot *)fxRenewChunk(the, variable, index * sizeof(txSlot));
				if (!variable)
					variable = (txSlot *)fxNewChunk(the, index * sizeof(txSlot));
				mxRestoreState;
				slot->value.stack.address = variable;
			}
			slot->value.stack.length = index;
			c_memcpy(variable, mxStack, index * sizeof(txSlot));
 			slot = mxFrameResult;
 			goto XS_CODE_END_ALL;
 			
		mxCase(XS_CODE_AWAIT)
		mxCase(XS_CODE_YIELD)
			generator->next->next->value.integer = byte;
			mxSkipCode(1);
			*mxFrameResult = *mxStack;
			slot = mxFrameArgv(-1);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxCode - mxFrameFunction->value.reference->next->value.code.address;;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = slot - mxScope;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = slot - mxFrame;
			jump = the->firstJump;
			offset = 0;
			while (jump != yieldJump) {
				txJump* nextJump = jump->nextJump;
				mxPushKind(XS_INTEGER_KIND);
				mxStack->value.integer = jump->code - mxFrameFunction->value.reference->next->value.code.address;
				mxPushKind(XS_INTEGER_KIND);
				mxStack->value.integer = slot - jump->scope;
				mxPushKind(XS_INTEGER_KIND);
				mxStack->value.integer = slot - jump->frame;
				mxPushKind(XS_INTEGER_KIND);
				mxStack->value.integer = slot - jump->stack;
				c_free(jump);
				jump = nextJump;
				offset++;
			}
			the->firstJump = yieldJump;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = offset;
			mxPushKind(mxFrameFunction->kind);
			mxStack->value = mxFrameFunction->value;
			index = slot - mxStack;
			slot = generator->next;
			variable = slot->value.stack.address;
			if (slot->value.stack.length < index) {
				mxSaveState;
				if (variable)
					variable = (txSlot *)fxRenewChunk(the, variable, index * sizeof(txSlot));
				if (!variable)
					variable = (txSlot *)fxNewChunk(the, index * sizeof(txSlot));
				mxRestoreState;
				slot->value.stack.address = variable;
			}
			slot->value.stack.length = index;
			c_memcpy(variable, mxStack, index * sizeof(txSlot));
			slot = mxFrameResult;
			goto XS_CODE_END_ALL;
			
	/* FRAMES */		
		mxCase(XS_CODE_ARGUMENT)
			offset = mxRunS1(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, offset);
#endif
			offset = mxFrameArgc - offset;
			if (0 < offset) {
				slot = mxFrame + 5 + offset;
				mxPushKind(slot->kind);
				mxStack->value = slot->value;
			}
			else {
				mxPushKind(XS_UNDEFINED_KIND);
			}
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_ARGUMENTS)
			offset = mxRunS1(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, offset);
#endif
			mxOverflow(1);
			*mxStack = mxArrayPrototype;
			mxSaveState;
			fxNewArrayInstance(the);
			mxRestoreState;
			offset = mxFrameArgc - offset;
			if (0 < offset) {
				variable = mxFrame + 5 + offset;
				for (id = 0; id < offset; id++) {
					slot = mxBehaviorSetProperty(the, mxStack->value.reference, 0, id, XS_OWN);
					slot->kind = variable->kind;
					slot->value = variable->value;
					variable--;
				}
			}
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_ARGUMENTS_SLOPPY)
		#ifdef mxSloppy
			offset = mxRunS1(1);
			mxOverflow(1);
			*mxStack = mxArgumentsSloppyPrototype;
			mxSaveState;
			fxNewArgumentsSloppyInstance(the, offset);
			mxRestoreState;
			mxNextCode(2);
		#endif
			mxBreak;
		mxCase(XS_CODE_ARGUMENTS_STRICT)
			offset = mxRunS1(1);
			mxOverflow(1);
			*mxStack = mxArgumentsStrictPrototype;
			mxSaveState;
			fxNewArgumentsStrictInstance(the, offset);
			mxRestoreState;
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_CURRENT)
			mxOverflow(1);
			*mxStack = *mxFrameFunction;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_RESULT)
			*mxFrameResult = *(mxStack++);
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_TARGET)
			mxOverflow(1);
			*mxStack = *mxFrameTarget;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_THIS)
			mxOverflow(1);
			*mxStack = *mxFrameThis;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_GET_THIS)
			slot = mxFrameThis;
			variable = slot->value.closure;
			if (variable->kind < 0)
				mxRunDebug(XS_REFERENCE_ERROR, "this is not initialized yet");
			mxPushKind(variable->kind);
			mxStack->value = variable->value;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SET_THIS)
			slot = mxFrameThis;
			variable = slot->value.closure;
			if (variable->kind >= 0)
				mxRunDebug(XS_REFERENCE_ERROR, "this is already initialized");
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxNextCode(1);
			mxBreak;
		
	/* EXCEPTIONS */	
		mxCase(XS_CODE_EXCEPTION)
			mxOverflow(1);
			*mxStack = mxException;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_CATCH_1)
			offset = mxRunS1(1);
			mxNextCode(2);
			goto XS_CODE_CATCH;
		mxCase(XS_CODE_CATCH_2)
			offset = mxRunS2(1);
			mxNextCode(3);
			goto XS_CODE_CATCH;
		mxCase(XS_CODE_CATCH_4)
			offset = mxRunS4(1);
			mxNextCode(5);
		XS_CODE_CATCH:
			jump = c_malloc(sizeof(txJump));
			if (jump) {
				jump->nextJump = the->firstJump;
				jump->stack = mxStack;
				jump->scope = mxScope;
				jump->frame = mxFrame;
				jump->environment = mxFrameEnvironment->value.reference;
				jump->code = mxCode + offset;
				jump->flag = 1;
                the->firstJump = jump;
				if (c_setjmp(jump->buffer) != 0) {
					jump = the->firstJump;
                    the->firstJump = jump->nextJump;
					mxStack = jump->stack;
					mxScope = jump->scope;
					mxFrame = jump->frame;
					mxFrameEnvironment->value.reference = jump->environment;
					mxCode = jump->code;
					c_free(jump);
					mxNextCode(0);
				}
			}
			else {
				mxSaveState;
				fxJump(the);
			}
			mxBreak;
		mxCase(XS_CODE_RETHROW)
			mxSaveState;
			fxJump(the);
			mxBreak;
		mxCase(XS_CODE_THROW)
			mxException = *mxStack;
		#ifdef mxDebug
			mxSaveState;
			fxDebugThrow(the, C_NULL, 0, "throw");
			mxRestoreState;
		#endif
			mxSaveState;
			fxJump(the);
			mxBreak;
		mxCase(XS_CODE_THROW_STATUS)
			if (the->status & XS_THROW_STATUS) {
				mxException = *mxStack;
			#ifdef mxDebug
				mxSaveState;
				fxDebugThrow(the, C_NULL, 0, "throw");
				mxRestoreState;
			#endif
				mxSaveState;
				fxJump(the);
			}
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_UNCATCH)
			jump = the->firstJump;
			the->firstJump = jump->nextJump;
			c_free(jump);
			mxNextCode(1);
			mxBreak;
			
	/* BRANCHES */	
		mxCase(XS_CODE_BRANCH_1)
			offset = mxRunS1(1);
			mxNextCode(2 + offset);
			mxBreak;
		mxCase(XS_CODE_BRANCH_2)
			offset = mxRunS2(1);
			mxNextCode(3 + offset);
			mxBreak;
		mxCase(XS_CODE_BRANCH_4)
			offset = mxRunS4(1);
			mxNextCode(5 + offset);
			mxBreak;
		mxCase(XS_CODE_BRANCH_ELSE_1)
			offset = mxRunS1(1);
			index = 2;
			goto XS_CODE_BRANCH_ELSE;
		mxCase(XS_CODE_BRANCH_ELSE_2)
			offset = mxRunS2(1);
			index = 3;
			goto XS_CODE_BRANCH_ELSE;
		mxCase(XS_CODE_BRANCH_ELSE_4)
			offset = mxRunS4(1);
			index = 5;
		XS_CODE_BRANCH_ELSE:
			byte = mxStack->kind;
			if (XS_BOOLEAN_KIND == byte)
				mxNextCode(mxStack->value.boolean ? index : index + offset)
			else if (XS_INTEGER_KIND == byte)
				mxNextCode(mxStack->value.integer ? index : index + offset)
			else if (XS_NUMBER_KIND == byte)
				mxNextCode((c_isnan(mxStack->value.number) || c_iszero(mxStack->value.number)) ? index + offset : index)
			else if ((XS_STRING_KIND == byte) || (XS_STRING_X_KIND == byte))
				mxNextCode(c_isEmpty(mxStack->value.string) ? index + offset : index)
			else
				mxNextCode((XS_UNDEFINED_KIND == byte) || (XS_NULL_KIND == byte) ? index + offset : index)
			mxStack++;
			mxBreak;
		mxCase(XS_CODE_BRANCH_IF_1)
			offset = mxRunS1(1);
			index = 2;
			goto XS_CODE_BRANCH_IF;
		mxCase(XS_CODE_BRANCH_IF_2)
			offset = mxRunS2(1);
			index = 3;
			goto XS_CODE_BRANCH_IF;
		mxCase(XS_CODE_BRANCH_IF_4)
			offset = mxRunS4(1);
			index = 5;
		XS_CODE_BRANCH_IF:
			byte = mxStack->kind;
			if (XS_BOOLEAN_KIND == byte)
				mxNextCode(mxStack->value.boolean ? index + offset : index)
			else if (XS_INTEGER_KIND == byte)
				mxNextCode(mxStack->value.integer ? index + offset : index)
			else if (XS_NUMBER_KIND == byte)
				mxNextCode((c_isnan(mxStack->value.number) || c_iszero(mxStack->value.number)) ? index : index + offset)
			else if ((XS_STRING_KIND == byte) || (XS_STRING_X_KIND == byte))
				mxNextCode(c_isEmpty(mxStack->value.string) ? index : index + offset)
			else
				mxNextCode((XS_UNDEFINED_KIND == byte) || (XS_NULL_KIND == byte) ? index : index + offset)
			mxStack++;
			mxBreak;
		mxCase(XS_CODE_BRANCH_STATUS_1)
			offset = mxRunS1(1);
			index = 2;
			goto XS_CODE_BRANCH_STATUS;
		mxCase(XS_CODE_BRANCH_STATUS_2)
			offset = mxRunS2(1);
			index = 3;
			goto XS_CODE_BRANCH_STATUS;
		mxCase(XS_CODE_BRANCH_STATUS_4)
			offset = mxRunS4(1);
			index = 5;
		XS_CODE_BRANCH_STATUS:
			if (the->status & XS_THROW_STATUS) {
				mxException = *mxStack;
			#ifdef mxDebug
				mxSaveState;
				fxDebugThrow(the, C_NULL, 0, "throw");
				mxRestoreState;
			#endif
				mxSaveState;
				fxJump(the);
			}
			mxNextCode((the->status & XS_RETURN_STATUS) ? index : index + offset);
			mxBreak;
			
	/* STACK */	
		mxCase(XS_CODE_DUB)
			mxOverflow(1);
			*mxStack = *(mxStack + 1);
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_DUB_AT)
			mxOverflow(2);
			*(mxStack + 1) = *(mxStack + 3);
			*mxStack = *(mxStack + 2);
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_POP)
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SWAP)
			scratch = *(mxStack);
			*(mxStack) = *(mxStack + 1);
			*(mxStack + 1) = scratch;
			mxNextCode(1);
			mxBreak;

	/* SCOPE */		
		mxCase(XS_CODE_CONST_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_CONST_CLOSURE;
		mxCase(XS_CODE_CONST_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_CONST_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			slot = mxFrame - index;
			variable = slot->value.closure;
			if (variable->kind >= 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: already initialized", slot->ID);
			slot->flag |= XS_DONT_SET_FLAG;
			variable->flag |= XS_DONT_SET_FLAG;
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
		mxCase(XS_CODE_CONST_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_CONST_LOCAL;
		mxCase(XS_CODE_CONST_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_CONST_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = mxFrame - index;
			if (variable->kind >= 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: already initialized", variable->ID);
			variable->flag |= XS_DONT_SET_FLAG;
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
						
		mxCase(XS_CODE_GET_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_GET_CLOSURE;
		mxCase(XS_CODE_GET_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_GET_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			slot = mxFrame - index;
			#ifdef mxDebug
				offset = slot->ID;
			#endif
			variable = slot->value.closure;
			if (variable->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "get %s: not initialized yet", slot->ID);
			mxPushKind(variable->kind);
			mxStack->value = variable->value;
			mxBreak;
		mxCase(XS_CODE_GET_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_GET_LOCAL;
		mxCase(XS_CODE_GET_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_GET_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = mxFrame - index;
			#ifdef mxDebug
				offset = variable->ID;
			#endif
			if (variable->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "get %s: not initialized yet", variable->ID);
			mxPushKind(variable->kind);
			mxStack->value = variable->value;
			mxBreak;
			
		mxCase(XS_CODE_LET_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_LET_CLOSURE;
		mxCase(XS_CODE_LET_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_LET_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			slot = mxFrame - index;
			variable = slot->value.closure;
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
		mxCase(XS_CODE_LET_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_LET_LOCAL;
		mxCase(XS_CODE_LET_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_LET_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = mxFrame - index;
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
			
		mxCase(XS_CODE_NEW_CLOSURE)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
#endif
			mxNextCode(3);
			slot = --mxScope;
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, mxFrame - mxScope - 2);
#endif
			mxSaveState;
			variable = fxNewSlot(the);
			mxRestoreState;
			slot->flag |= XS_DONT_DELETE_FLAG;
			slot->ID = (txID)offset;
			slot->kind = XS_CLOSURE_KIND;
			slot->value.closure = variable;
			variable->ID = (txID)offset;
			variable->kind = XS_UNINITIALIZED_KIND;
			mxBreak;
		mxCase(XS_CODE_NEW_LOCAL)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
#endif
			mxNextCode(3);
			variable = --mxScope;
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, mxFrame - mxScope - 2);
#endif
			variable->flag = XS_DONT_DELETE_FLAG;
			variable->ID = (txID)offset;
			variable->kind = XS_UNINITIALIZED_KIND;
			mxBreak;
		mxCase(XS_CODE_NEW_TEMPORARY)
			variable = --mxScope;
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, mxFrame - mxScope - 2);
#endif
			variable->flag = XS_NO_FLAG;
			variable->ID = XS_NO_ID;
			variable->kind = XS_UNDEFINED_KIND;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_PULL_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_PULL_CLOSURE;
		mxCase(XS_CODE_PULL_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_PULL_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			slot = mxFrame - index;
			if (slot->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", slot->ID);
			variable = slot->value.closure;
			if (variable->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: not initialized yet", slot->ID);
			if (variable->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", slot->ID);
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxStack++;
			mxBreak;
		mxCase(XS_CODE_PULL_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_PULL_LOCAL;
		mxCase(XS_CODE_PULL_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_PULL_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = mxFrame - index;
			if (variable->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: not initialized yet", variable->ID);
			if (variable->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", variable->ID);
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxStack++;
			mxBreak;
			
		mxCase(XS_CODE_REFRESH_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_REFRESH_CLOSURE;
		mxCase(XS_CODE_REFRESH_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_REFRESH_CLOSURE:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = mxFrame - index;
			mxSaveState;
			slot = fxNewSlot(the);
			mxRestoreState;
			slot->flag = variable->value.closure->flag;
			slot->ID = variable->value.closure->ID;
			slot->kind = variable->value.closure->kind;
			slot->value = variable->value.closure->value;
			variable->value.closure = slot;
			mxBreak;
		mxCase(XS_CODE_REFRESH_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_REFRESH_LOCAL;
		mxCase(XS_CODE_REFRESH_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_REFRESH_LOCAL:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = mxFrame - index;
			mxBreak;
			
		mxCase(XS_CODE_RESERVE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_RESERVE;
		mxCase(XS_CODE_RESERVE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_RESERVE:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			mxOverflow(index);		
			c_memset(mxStack, 0, index * sizeof(txSlot));
			mxBreak;
			
		mxCase(XS_CODE_RESET_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_RESET_CLOSURE;
		mxCase(XS_CODE_RESET_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_RESET_CLOSURE:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			slot = mxFrame - index;
			variable = slot->value.closure;
//			if (variable->flag & XS_DONT_SET_FLAG)
//				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", slot->ID);
			mxSaveState;
			variable = fxNewSlot(the);
			mxRestoreState;
			variable->ID = slot->value.closure->ID;
			variable->kind = XS_UNINITIALIZED_KIND;
			slot->value.closure = variable;
			mxBreak;
		mxCase(XS_CODE_RESET_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_RESET_LOCAL;
		mxCase(XS_CODE_RESET_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_RESET_LOCAL:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			variable = mxFrame - index;
//			if (variable->flag & XS_DONT_SET_FLAG)
//				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", variable->ID);
			variable->flag = XS_NO_FLAG;
			variable->kind = XS_UNINITIALIZED_KIND;
			mxBreak;
			
		mxCase(XS_CODE_SET_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_SET_CLOSURE;
		mxCase(XS_CODE_SET_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_SET_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			slot = mxFrame - index;
			if (slot->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", slot->ID);
			variable = slot->value.closure;
			if (variable->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: not initialized yet", slot->ID);
			if (variable->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", slot->ID);
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
		mxCase(XS_CODE_SET_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_SET_LOCAL;
		mxCase(XS_CODE_SET_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_SET_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = mxFrame - index;
			if (variable->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: not initialized yet", variable->ID);
			if (variable->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", variable->ID);
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;

		mxCase(XS_CODE_VAR_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_VAR_CLOSURE;
		mxCase(XS_CODE_VAR_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_VAR_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = (mxFrame - index)->value.closure;
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
		mxCase(XS_CODE_VAR_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_VAR_LOCAL;
		mxCase(XS_CODE_VAR_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_VAR_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = mxFrame - index;
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
			
		mxCase(XS_CODE_UNWIND_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_UNWIND;
		mxCase(XS_CODE_UNWIND_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_UNWIND:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			slot = mxScope;
			mxScope += index;
			while (slot < mxScope)
				(slot++)->kind = XS_UNDEFINED_KIND;
			mxBreak;
			
		mxCase(XS_CODE_RETRIEVE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_RETRIEVE;
		mxCase(XS_CODE_RETRIEVE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_RETRIEVE:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			slot = mxFrameEnvironment->value.reference->next->next;
			variable = mxScope;
			while (index) {
				--variable;
				offset = variable->ID = slot->ID;
                variable->flag = slot->flag;
                variable->kind = slot->kind;
				variable->value = slot->value;
#ifdef mxTrace
				if (gxDoTrace) fxTraceID(the, (txID)offset);
#endif
				index--;
				slot = slot->next;
			}
			mxScope = variable;
			mxBreak;
		mxCase(XS_CODE_RETRIEVE_TARGET)
			variable = mxFrameTarget;
			variable->kind = slot->kind;
			variable->value = slot->value;
			slot = slot->next;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_RETRIEVE_THIS)
			variable = mxFrameThis;
			variable->kind = slot->kind;
			variable->value = slot->value;
			slot = slot->next;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_ENVIRONMENT)	
			variable = mxFrameEnvironment;
			mxPushKind(XS_UNDEFINED_KIND);
			mxSaveState;
			slot = fxNewEnvironmentInstance(the, variable);
			mxRestoreState;
			variable = mxFunctionInstanceCode((mxStack + 1)->value.reference);
			variable->value.code.closures = slot;
			mxNextCode(1);
			mxBreak;

		mxCase(XS_CODE_STORE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_STORE;
		mxCase(XS_CODE_STORE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_STORE:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			address = &(mxStack->value.reference->next);
			while ((slot = *address)) {
				address = &slot->next;
			}
			mxSaveState;
			*address = slot = fxNewSlot(the);
			mxRestoreState;
			variable = mxFrame - index;
			offset = slot->ID = variable->ID;
            slot->flag = variable->flag;
            slot->kind = variable->kind;
			slot->value = variable->value;
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
#endif
			mxBreak;
		mxCase(XS_CODE_STORE_ARROW)
			// super
			variable = mxFunctionInstanceHome(mxFrameFunction->value.reference);
			slot = mxFunctionInstanceHome((mxStack + 1)->value.reference);
			slot->value.home.object = variable->value.home.object;
			// target
			address = &(mxStack->value.reference->next);
			while ((slot = *address)) {
				address = &slot->next;
			}
			mxSaveState;
			*address = slot = fxNewSlot(the);
			mxRestoreState;
			variable = mxFrameTarget;
			slot->ID = mxID(_new_target);
			slot->kind = variable->kind;
			slot->value = variable->value;
			// this
			address = &slot->next;
			mxSaveState;
			*address = slot = fxNewSlot(the);
			mxRestoreState;
			variable = mxFrameThis;
			slot->ID = mxID(_this);
			slot->kind = variable->kind;
			slot->value = variable->value;
			mxNextCode(1);
			mxBreak;
				
	/* PROPERTIES */	
		mxCase(XS_CODE_INTRINSIC)
			offset = mxRunS2(1);
			mxNextCode(3);
			mxOverflow(1);
			*mxStack = mxIntrinsics[-offset];
			mxBreak;
		
		mxCase(XS_CODE_CHECK_INSTANCE)
			if (mxStack->kind != XS_REFERENCE_KIND)
				mxRunDebug(XS_TYPE_ERROR, "result: no instance");
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_TO_INSTANCE)
			mxToInstance(mxStack);
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_AT)
			mxToInstance(mxStack + 1);
			if ((mxStack->kind == XS_INTEGER_KIND) && fxIntegerToIndex(the->dtoa, mxStack->value.integer, &(scratch.value.at.index))) {
				mxStack->kind = XS_AT_KIND;
				mxStack->value.at.id = 0;
				mxStack->value.at.index = scratch.value.at.index;
			}
			else if ((mxStack->kind == XS_NUMBER_KIND) && fxNumberToIndex(the->dtoa, mxStack->value.number, &(scratch.value.at.index))) {
				mxStack->kind = XS_AT_KIND;
				mxStack->value.at.id = 0;
				mxStack->value.at.index = scratch.value.at.index;
			}
			else if (mxStack->kind == XS_SYMBOL_KIND) {
				mxStack->kind = XS_AT_KIND;
				mxStack->value.at.id = mxStack->value.symbol;
				mxStack->value.at.index = XS_NO_ID;
			}
			else {
				txFlag flag;

				mxToString(mxStack);
				mxSaveState;
				flag = fxStringToIndex(the->dtoa, mxStack->value.string, &(scratch.value.at.index));
				mxRestoreState;
				if (flag) {
					mxStack->kind = XS_AT_KIND;
					mxStack->value.at.id = 0;
					mxStack->value.at.index = scratch.value.at.index;
				}
				else {
					txID id;
					mxSaveState;
					if (mxStack->kind == XS_STRING_X_KIND)
						id = fxNewNameX(the, mxStack->value.string);
					else
						id = fxNewName(the, mxStack);
					mxRestoreState;
					mxStack->kind = XS_AT_KIND;
					mxStack->value.at.id = id;
					mxStack->value.at.index = XS_NO_ID;
				}
			}
			mxNextCode(1);
			mxBreak;

		mxCase(XS_CODE_DELETE_PROPERTY_AT)
		mxCase(XS_CODE_DELETE_SUPER_AT)
			variable = (mxStack + 1)->value.reference;
			offset = mxStack->value.at.id;
			index = mxStack->value.at.index;
			mxStack++;
			mxNextCode(1);
			goto XS_CODE_DELETE_PROPERTY_ALL;
		mxCase(XS_CODE_DELETE_PROPERTY)
		mxCase(XS_CODE_DELETE_SUPER)
			mxToInstance(mxStack);
			offset = mxRunS2(1);
			index = XS_NO_ID;
			mxNextCode(3);
			/* continue */
		XS_CODE_DELETE_PROPERTY_ALL:	
			mxSaveState;
			index = (txU4)fxRunDelete(the, variable, (txID)offset, index);
			mxRestoreState;
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
#endif
			if (!index && (mxFrame->flag & XS_STRICT_FLAG))
				mxRunDebugID(XS_TYPE_ERROR, "delete %s: no permission (strict mode)", (txID)offset);
			mxStack->kind = XS_BOOLEAN_KIND;
			mxStack->value.boolean = index;
			mxBreak;
			
		mxCase(XS_CODE_GET_VARIABLE)
			mxToInstance(mxStack);
			offset = mxRunS2(1);
			index = XS_NO_ID;
			mxNextCode(3);
			slot = mxBehaviorGetProperty(the, variable, (txID)offset, index, XS_ANY);
			if (!slot && (byte != XS_CODE_TYPEOF))
				mxRunDebugID(XS_REFERENCE_ERROR, "get %s: undefined variable", (txID)offset);
			goto XS_CODE_GET_ALL;
		mxCase(XS_CODE_GET_SUPER_AT)
			variable = (mxStack + 1)->value.reference;
			offset = mxStack->value.at.id;
			index = mxStack->value.at.index;
			mxStack++;
			mxNextCode(1);
			goto XS_CODE_GET_SUPER_ALL;
		mxCase(XS_CODE_GET_SUPER)
			mxToInstance(mxStack);
			offset = mxRunS2(1);
			index = XS_NO_ID;
			mxNextCode(3);
			/* continue */
		XS_CODE_GET_SUPER_ALL:	
			slot = mxFunctionInstanceHome(mxFrameFunction->value.reference);
			if (!slot->value.home.object)
				mxRunDebugID(XS_TYPE_ERROR, "get super %s: no home", (txID)offset);
			slot = slot->value.home.object->value.instance.prototype;
			if (!slot)
				mxRunDebugID(XS_TYPE_ERROR, "get super %s: no prototype", (txID)offset);
			slot = mxBehaviorGetProperty(the, slot, (txID)offset, index, XS_ANY);
			goto XS_CODE_GET_ALL;
		mxCase(XS_CODE_GET_PROPERTY_AT)
			variable = (mxStack + 1)->value.reference;
			offset = mxStack->value.at.id;
			index = mxStack->value.at.index;
			mxStack++;
			mxNextCode(1);
			goto XS_CODE_GET_PROPERTY_ALL;
		mxCase(XS_CODE_GET_PROPERTY)
			mxToInstance(mxStack);
			offset = mxRunS2(1);
			index = XS_NO_ID;
			mxNextCode(3);
			/* continue */
		XS_CODE_GET_PROPERTY_ALL:	
			slot = mxBehaviorGetProperty(the, variable, (txID)offset, index, XS_ANY);
		XS_CODE_GET_ALL:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
#endif
			if (!slot) {
				mxStack->kind = XS_UNDEFINED_KIND;
				mxBreak;
			}
			if (slot->kind == XS_ACCESSOR_KIND) {
				slot = slot->value.accessor.getter;
				if (!mxIsFunction(slot)) {
					mxStack->kind = XS_UNDEFINED_KIND;
					mxBreak;
				}
				mxStack->kind = XS_INTEGER_KIND;
				mxStack->value.integer = 0;
				/* THIS */
				mxPushKind(XS_REFERENCE_KIND);
				mxStack->value.reference = variable;
				/* FUNCTION */
				mxPushKind(XS_REFERENCE_KIND);
				mxStack->value.reference = slot;
				/* TARGET */
				mxPushKind(XS_UNDEFINED_KIND);
				/* RESULT */
				mxPushKind(XS_UNDEFINED_KIND);
				goto XS_CODE_CALL_ALL;
			}
			mxStack->kind = slot->kind;
			mxStack->value = slot->value;
			mxBreak;
			
		mxCase(XS_CODE_NEW_PROPERTY)
			byte = mxRunU1(1);
			variable = (mxStack + 2)->value.reference;
			offset = (mxStack + 1)->value.at.id;
			index = (mxStack + 1)->value.at.index;
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
#endif
			mxSaveState;
			if (byte & XS_GETTER_FLAG) {
				mxStack->value.accessor.getter = fxToInstance(the, mxStack);
				mxStack->value.accessor.setter = C_NULL;
				mxStack->kind = XS_ACCESSOR_KIND;
			}
			else if (byte & XS_SETTER_FLAG) {
				mxStack->value.accessor.setter = fxToInstance(the, mxStack);
				mxStack->value.accessor.getter = C_NULL;
				mxStack->kind = XS_ACCESSOR_KIND;
			}
			mxStack->flag = byte & XS_GET_ONLY;
			index = fxRunDefine(the, variable, (txID)offset, index, mxStack, byte | XS_GET_ONLY);
			mxRestoreState;
			mxStack += 3;
			if (!index)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", (txID)offset);
			mxNextCode(2);
			mxBreak;
		
		mxCase(XS_CODE_SET_VARIABLE)
			mxToInstance(mxStack + 1);
			offset = mxRunS2(1);
			index = XS_NO_ID;
			mxNextCode(3);
			if (mxFrame->flag & XS_STRICT_FLAG) {
				if (!mxBehaviorHasProperty(the, variable, (txID)offset, index))
					mxRunDebugID(XS_REFERENCE_ERROR, "set %s: undefined variable", (txID)offset);
			}
			goto XS_CODE_SET_PROPERTY_ALL;
		mxCase(XS_CODE_SET_SUPER_AT)
			variable = (mxStack + 2)->value.reference;
			offset = (mxStack + 1)->value.at.id;
			index = (mxStack + 1)->value.at.index;
			*(mxStack + 1) = *mxStack;
			mxStack++;
			mxNextCode(1);
			goto XS_CODE_SET_SUPER_ALL;
		mxCase(XS_CODE_SET_SUPER)
			mxToInstance(mxStack + 1);
			offset = mxRunS2(1);
			index = XS_NO_ID;
			mxNextCode(3);
			/* continue */
		XS_CODE_SET_SUPER_ALL:
			slot = mxFunctionInstanceHome(mxFrameFunction->value.reference);
			if (!slot->value.home.object)
				mxRunDebugID(XS_TYPE_ERROR, "set super %s: no home", (txID)offset);
			slot = slot->value.home.object->value.instance.prototype;
			if (!slot)
				mxRunDebugID(XS_TYPE_ERROR, "set super %s: no prototype", (txID)offset);
			slot = mxBehaviorGetProperty(the, slot, (txID)offset, index, XS_ANY);
			if (!slot || (slot->kind != XS_ACCESSOR_KIND)) {
				mxSaveState;
				slot = mxBehaviorSetProperty(the, variable, (txID)offset, index, XS_OWN);
				mxRestoreState;
			}
			goto XS_CODE_SET_ALL;
		mxCase(XS_CODE_SET_PROPERTY_AT)
			variable = (mxStack + 2)->value.reference;
			offset = (mxStack + 1)->value.at.id;
			index = (mxStack + 1)->value.at.index;
			*(mxStack + 1) = *mxStack;
			mxStack++;
			mxNextCode(1);
			goto XS_CODE_SET_PROPERTY_ALL;
		mxCase(XS_CODE_SET_PROPERTY)
			mxToInstance(mxStack + 1);
			offset = mxRunS2(1);
			index = XS_NO_ID;
			mxNextCode(3);
			/* continue */
		XS_CODE_SET_PROPERTY_ALL:	
			mxSaveState;
			slot = mxBehaviorSetProperty(the, variable, (txID)offset, index, XS_ANY);
			mxRestoreState;
		XS_CODE_SET_ALL:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
#endif
			if (!slot) {
				if (mxFrame->flag & XS_STRICT_FLAG) {
					mxRunDebugID(XS_TYPE_ERROR, "set %s: not extensible", (txID)offset);
				}
				goto XS_CODE_SET_SKIP;
			}
			if (slot->kind == XS_ACCESSOR_KIND) {
				slot = slot->value.accessor.setter;
				if (!mxIsFunction(slot)) {
					if (mxFrame->flag & XS_STRICT_FLAG) {
						mxRunDebugID(XS_TYPE_ERROR, "set %s: no setter", (txID)offset);
					}
					goto XS_CODE_SET_SKIP;
				}
				*(mxStack + 1) = *mxStack;
				mxStack->kind = XS_INTEGER_KIND;
				mxStack->value.integer = 1;
				/* THIS */
				mxPushKind(XS_REFERENCE_KIND);
				mxStack->value.reference = variable;
				/* FUNCTION */
				mxPushKind(XS_REFERENCE_KIND);
				mxStack->value.reference = slot;
				/* TARGET */
				mxPushKind(XS_UNDEFINED_KIND);
				/* RESULT */
				mxStack--;
				*(mxStack) = *(mxStack + 5);
				goto XS_CODE_CALL_ALL;
			}
			if (slot->flag & XS_DONT_SET_FLAG) {
				if (mxFrame->flag & XS_STRICT_FLAG) {
					mxRunDebugID(XS_TYPE_ERROR, "set %s: not writable", (txID)offset);
				}
				goto XS_CODE_SET_SKIP;
			}
			slot->kind = mxStack->kind;
			slot->value = mxStack->value;
		XS_CODE_SET_SKIP:	
			*(mxStack + 1) = *mxStack;
			mxStack++;
			mxBreak;
			
	/* INSTANCES */	
		mxCase(XS_CODE_ARRAY)
// 			mxOverflow(1);
			mxSaveState;
			fxNewArray(the, 0);
			mxRestoreState;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_CLASS)
			variable = fxToInstance(the, mxStack);
			slot = mxStack + 2;
			if (slot->kind == XS_NULL_KIND)
				variable->flag = XS_CAN_CONSTRUCT_FLAG | XS_BASE_FLAG;
			else {
				variable->flag = XS_CAN_CONSTRUCT_FLAG | XS_DERIVED_FLAG;
				variable->value.instance.prototype = slot->value.reference;
			}
			slot = mxStack + 1;
			mxFunctionInstanceHome(variable)->value.home.object = slot->value.reference;
			mxSaveState;
			slot->flag = XS_GET_ONLY;
			fxRunDefine(the, variable, mxID(_prototype), XS_NO_ID, slot, XS_GET_ONLY);
			slot = mxBehaviorSetProperty(the, slot->value.reference, mxID(_constructor), XS_NO_ID, XS_OWN);
			mxRestoreState;
			slot->flag |= XS_DONT_ENUM_FLAG;
			slot->kind = mxStack->kind;
			slot->value = mxStack->value;
			mxStack += 3;
            mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_EXTEND)
			if (mxStack->kind == XS_NULL_KIND) {
				mxSaveState;
				fxNewInstance(the);
				mxRestoreState;
			}
			else {
				mxSaveState;
				fxRunExtends(the);
				mxRestoreState;
			}
            mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_GLOBAL)
			mxPushKind(XS_REFERENCE_KIND);
			mxStack->value.reference = mxGlobal.value.reference;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_HOST)
			index = mxRunU2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			mxOverflow(1);
			*mxStack = mxHosts;
			slot = mxBehaviorGetProperty(the, mxStack->value.reference, 0, index, XS_OWN);
			mxStack->kind = slot->kind;
			mxStack->value = slot->value;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_INSTANTIATE)
			mxSaveState;
			fxNewHostInstance(the);
			mxRestoreState;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_NEW)
			mxSkipCode(1);
			slot = fxGetInstance(the, mxStack);
            if (!mxIsConstructor(slot))
				mxRunDebugID(XS_TYPE_ERROR, "new %s: no constructor", (txID)offset);
			/* THIS */
			mxPushKind(XS_UNINITIALIZED_KIND);
			/* FUNCTION */
			scratch = *(mxStack);
			*(mxStack) = *(mxStack + 1);
			*(mxStack + 1) = scratch;
			/* TARGET */
			mxOverflow(1);
			*mxStack = *(mxStack + 1);
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
			goto XS_CODE_CALL_ALL;
		mxCase(XS_CODE_OBJECT)
// 			mxOverflow(1);
			mxSaveState;
			fxNewObject(the);
			mxRestoreState;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SUPER)
			mxSkipCode(1);
			slot = mxFunctionInstanceHome(mxFrameFunction->value.reference);
			slot = mxBehaviorGetProperty(the, slot->value.home.object, mxID(_constructor), XS_NO_ID, XS_ANY);
			slot = slot->value.reference->value.instance.prototype;
            if (!mxIsConstructor(slot))
				mxRunDebug(XS_TYPE_ERROR, "super: no constructor");
			/* THIS */
			mxPushKind(XS_UNINITIALIZED_KIND);
			/* FUNCTION */
			mxPushKind(XS_REFERENCE_KIND);
			mxStack->value.reference = slot;
			/* TARGET */
			mxPushKind(mxFrameTarget->kind);
			mxStack->value = mxFrameTarget->value;
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
			goto XS_CODE_CALL_ALL;
		mxCase(XS_CODE_TEMPLATE)
			mxNextCode(1);
			variable = mxStack->value.reference;
			slot = mxBehaviorGetProperty(the, variable, mxID(_raw), XS_NO_ID, XS_OWN);
			variable->flag |= XS_DONT_PATCH_FLAG;
			variable->next->flag |= XS_DONT_SET_FLAG;
			slot->flag |= XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
			variable = slot->value.reference;
			variable->flag |= XS_DONT_PATCH_FLAG;
			variable->next->flag |= XS_DONT_SET_FLAG;
			mxBreak;
			
	/* FUNCTIONS */		
		mxCase(XS_CODE_ASYNC_FUNCTION)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
#endif
			mxOverflow(1);
			*mxStack = mxAsyncFunctionPrototype;
			mxSaveState;
			fxNewFunctionInstance(the, (txID)offset);
			mxRestoreState;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_ASYNC_GENERATOR_FUNCTION)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
#endif
			mxOverflow(1);
			*mxStack = mxAsyncGeneratorFunctionPrototype;
			mxSaveState;
			gxDefaults.newAsyncGeneratorFunctionInstance(the,(txID) offset);
			mxRestoreState;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_CONSTRUCTOR_FUNCTION)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
#endif
			mxOverflow(1);
			*mxStack = mxFunctionPrototype;
			mxSaveState;
			fxNewFunctionInstance(the, (txID)offset);
			fxDefaultFunctionPrototype(the);
			mxRestoreState;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_FUNCTION)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
#endif
			mxOverflow(1);
			*mxStack = mxFunctionPrototype;
			mxSaveState;
			fxNewFunctionInstance(the, (txID)offset);
			mxRestoreState;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_GENERATOR_FUNCTION)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
#endif
			mxOverflow(1);
			*mxStack = mxGeneratorFunctionPrototype;
			mxSaveState;
			gxDefaults.newGeneratorFunctionInstance(the,(txID) offset);
			mxRestoreState;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_NAME)
			offset = mxRunS2(1);
			mxSaveState;
			fxRenameFunction(the, mxStack->value.reference, (txID)offset, XS_NO_ID, C_NULL);
			mxRestoreState;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_CODE_1)
			offset = mxRunS1(1);
			mxSkipCode(2);
			goto XS_CODE_CODE;
		mxCase(XS_CODE_CODE_2)
			offset = mxRunS2(1);
			mxSkipCode(3);
			goto XS_CODE_CODE;
		mxCase(XS_CODE_CODE_4)
			offset = mxRunS4(1);
			mxSkipCode(5);
		XS_CODE_CODE:
			mxSaveState;
			scratch.value.code.address = (txByte*)fxNewChunk(the, (txID)offset);
			mxRestoreState;
			c_memcpy(scratch.value.code.address, mxCode, offset);
			variable = mxStack->value.reference;
			slot = mxFunctionInstanceCode(variable);
			slot->kind = XS_CODE_KIND;
			slot->value.code.address = scratch.value.code.address;
#ifndef mxNoFunctionLength
			slot = mxFunctionInstanceLength(variable);
			slot->value.integer = *(scratch.value.code.address + 1);
#endif
			mxNextCode(offset);
			mxBreak;
		mxCase(XS_CODE_CODE_ARCHIVE_1)
			offset = mxRunS1(1);
			mxSkipCode(2);
			goto XS_CODE_CODE_ARCHIVE;
		mxCase(XS_CODE_CODE_ARCHIVE_2)
			offset = mxRunS2(1);
			mxSkipCode(3);
			goto XS_CODE_CODE_ARCHIVE;
		mxCase(XS_CODE_CODE_ARCHIVE_4)
			offset = mxRunS4(1);
			mxSkipCode(5);
		XS_CODE_CODE_ARCHIVE:
			variable = mxStack->value.reference;
			slot = mxFunctionInstanceCode(variable);
			slot->kind = XS_CODE_X_KIND;
			slot->value.code.address = mxCode;
#ifndef mxNoFunctionLength
			slot = mxFunctionInstanceLength(variable);
			slot->value.integer = mxRunS1(1);		// slot->value.integer = *(mxCode + 1);		//@@ should be mxRunU1?
#endif
			mxNextCode(offset);
			mxBreak;

	/* VALUES */		
		mxCase(XS_CODE_UNDEFINED)
			mxPushKind(XS_UNDEFINED_KIND);
			mxNextCode(1);
			mxBreak; 
		mxCase(XS_CODE_NULL)
			mxPushKind(XS_NULL_KIND);
			mxNextCode(1);
			mxBreak; 
		mxCase(XS_CODE_FALSE)
			mxPushKind(XS_BOOLEAN_KIND);
			mxStack->value.boolean = 0;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_TRUE)
			mxPushKind(XS_BOOLEAN_KIND);
			mxStack->value.boolean = 1;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_INTEGER_1)
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxRunS1(1);
			mxNextCode(2);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, mxStack->value.integer);
#endif
			mxBreak;
		mxCase(XS_CODE_INTEGER_2)
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxRunS2(1);
			mxNextCode(3);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, mxStack->value.integer);
#endif
			mxBreak;
		mxCase(XS_CODE_INTEGER_4)
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxRunS4(1);
			mxNextCode(5);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, mxStack->value.integer);
#endif
			mxBreak;
		mxCase(XS_CODE_NUMBER)
			mxPushKind(XS_NUMBER_KIND);
			mxCode++;
			mxDecode8(mxCode, mxStack->value.number);
			mxNextCode(0);
#ifdef mxTrace
			if (gxDoTrace) fxTraceNumber(the, mxStack->value.number);
#endif
			mxBreak;
			
		mxCase(XS_CODE_STRING_1)
			index = mxRunU1(1);
			mxSkipCode(2);
			goto XS_CODE_STRING;
		mxCase(XS_CODE_STRING_2)
			index = mxRunU2(1);
			mxSkipCode(3);
		XS_CODE_STRING:
			mxSaveState;
			scratch.value.string = (txString)fxNewChunk(the, index);
			mxRestoreState;
			c_memcpy(scratch.value.string, mxCode, index);
			mxPushKind(XS_STRING_KIND);
			mxStack->value.string = scratch.value.string;
			mxNextCode(index);
#ifdef mxTrace
			if (gxDoTrace) fxTraceString(the, mxStack->value.string);
#endif
			mxBreak;
		mxCase(XS_CODE_STRING_ARCHIVE_1)
			index = mxRunU1(1);
			mxSkipCode(2);
			goto XS_CODE_STRING_ARCHIVE;
		mxCase(XS_CODE_STRING_ARCHIVE_2)
			index = mxRunU2(1);
			mxSkipCode(3);
		XS_CODE_STRING_ARCHIVE:
			mxPushKind(XS_STRING_X_KIND);
			mxStack->value.string = (txString)mxCode;
			mxNextCode(index);
#ifdef mxTrace
			if (gxDoTrace) fxTraceString(the, mxStack->value.string);
#endif
			mxBreak;
			
		mxCase(XS_CODE_SYMBOL)
			mxPushKind(XS_SYMBOL_KIND);
			mxStack->value.symbol = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, mxStack->value.symbol);
#endif
			mxNextCode(3);
			mxBreak;
			

	/* EXPRESSIONS */ 
		mxCase(XS_CODE_BIT_NOT)
			mxToInteger(mxStack);
			mxStack->value.integer = ~mxStack->value.integer;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_BIT_AND)
			slot = mxStack + 1;
			mxToInteger(slot);
			mxToInteger(mxStack);
			slot->value.integer &= mxStack->value.integer;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_BIT_OR)
			slot = mxStack + 1;
			mxToInteger(slot);
			mxToInteger(mxStack);
			slot->value.integer |= mxStack->value.integer;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_BIT_XOR)
			slot = mxStack + 1;
			mxToInteger(slot);
			mxToInteger(mxStack);
			slot->value.integer ^= mxStack->value.integer;
			mxStack++;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_LEFT_SHIFT)
			slot = mxStack + 1;
			mxToInteger(slot);
			mxToInteger(mxStack);
			slot->value.integer <<= mxStack->value.integer & 0x1f;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SIGNED_RIGHT_SHIFT)
			slot = mxStack + 1;
			mxToInteger(slot);
			mxToInteger(mxStack);
			slot->value.integer >>= mxStack->value.integer & 0x1f;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_UNSIGNED_RIGHT_SHIFT)
			slot = mxStack + 1;
			mxSaveState;
			fxUnsigned(the, slot, fxToUnsigned(the, slot) >> (fxToUnsigned(the, mxStack) & 0x1F));
			mxRestoreState;
			mxStack++;
			mxNextCode(1);
			mxBreak;
				
		mxCase(XS_CODE_MINUS)
			if ((mxStack->kind == XS_INTEGER_KIND) && mxStack->value.integer)
				mxStack->value.integer = -mxStack->value.integer;
			else {
				mxToNumber(mxStack);
				mxStack->value.number = -mxStack->value.number;
			}
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_PLUS)
			if (mxStack->kind != XS_INTEGER_KIND) {
				mxToNumber(mxStack);
			}
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_DECREMENT)
			if (mxStack->kind == XS_INTEGER_KIND) {
				mxStack->value.integer--;
			}
			else {
				mxToNumber(mxStack);
				mxStack->value.number--;
			}
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_INCREMENT)
			if (mxStack->kind == XS_INTEGER_KIND)
				mxStack->value.integer++;
			else {
				mxToNumber(mxStack);
				mxStack->value.number++;
			}
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_ADD)
			slot = mxStack + 1;
			if (slot->kind == mxStack->kind) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					#if __has_builtin(__builtin_add_overflow)
						if (__builtin_add_overflow(slot->value.integer, mxStack->value.integer, &slot->value.integer)) {
							fxToNumber(the, slot);
							fxToNumber(the, mxStack);
							slot->value.number += mxStack->value.number;
						}
					#else
						txInteger a = slot->value.integer;
						txInteger b = mxStack->value.integer;
						txInteger c = a + b;
						if (((a ^ c) & (b ^ c)) < 0) {
							fxToNumber(the, slot);
							fxToNumber(the, mxStack);
							slot->value.number += mxStack->value.number;
						}
						else
							slot->value.integer = c;
					#endif
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->value.number += mxStack->value.number;
				}
				else
					goto XS_CODE_ADD_GENERAL;
			}
			else if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_NUMBER_KIND)) {
				fxToNumber(the, slot);
				slot->value.number += mxStack->value.number;
			} 			
			else if ((slot->kind == XS_NUMBER_KIND) && (mxStack->kind == XS_INTEGER_KIND)) {
				fxToNumber(the, mxStack);
				slot->value.number += mxStack->value.number;
			} 			
			else {
		XS_CODE_ADD_GENERAL:
				mxSaveState;
				fxToPrimitive(the, slot, XS_NO_HINT);
				fxToPrimitive(the, mxStack, XS_NO_HINT);
				if ((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND) || (mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)) {
					fxToString(the, slot);
					fxToString(the, mxStack);
					fxConcatString(the, slot, mxStack);
				}
				else {
					mxToNumber(slot);
					mxToNumber(mxStack);
					slot->value.number += mxStack->value.number;
				}
				mxRestoreState;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SUBTRACT)
			slot = mxStack + 1;
			if (slot->kind == mxStack->kind) {
				if (slot->kind == XS_INTEGER_KIND) {
					#if __has_builtin(__builtin_sub_overflow)
						if (__builtin_sub_overflow(slot->value.integer, mxStack->value.integer, &slot->value.integer)) {
							fxToNumber(the, slot);
							fxToNumber(the, mxStack);
							slot->value.number -= mxStack->value.number;
						}
					#else
						slot->value.integer -= mxStack->value.integer; // @@ overflow?
					#endif
				}
				else if (slot->kind == XS_NUMBER_KIND)
					slot->value.number -= mxStack->value.number;
				else
					goto XS_CODE_SUBTRACT_GENERAL;
			}
			else {
		XS_CODE_SUBTRACT_GENERAL:
				mxToNumber(slot);
				mxToNumber(mxStack);
				slot->value.number -= mxStack->value.number;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_DIVIDE)
			slot = mxStack + 1;
			mxToNumber(slot);
			mxToNumber(mxStack);
			slot->value.number /= mxStack->value.number;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_EXPONENTIATION)
			slot = mxStack + 1;
			mxToNumber(slot);
			mxToNumber(mxStack);
			if (!c_isfinite(mxStack->value.number) && (c_fabs(slot->value.number) == 1.0))
				slot->value.number = C_NAN;
			else
				slot->value.number = c_pow(slot->value.number, mxStack->value.number);
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_MULTIPLY)
			slot = mxStack + 1;
			if (slot->kind == mxStack->kind) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					#if __has_builtin(__builtin_mul_overflow)
						variable = mxStack;
						mxPushKind(XS_INTEGER_KIND);
						if (__builtin_mul_overflow(slot->value.integer, variable->value.integer, &mxStack->value.integer)) {
							fxToNumber(the, slot);
							fxToNumber(the, variable);
							slot->value.number *= variable->value.number;
						}
						else
							slot->value.integer = mxStack->value.integer;
						mxStack++;
					#else
						fxToNumber(the, slot);
						fxToNumber(the, mxStack);
						slot->value.number *= mxStack->value.number;
					#endif
				}
				else if (mxStack->kind == XS_NUMBER_KIND)
					slot->value.number *= mxStack->value.number;
				else
					goto XS_CODE_MULTIPLY_GENERAL;
			}
			else {
		XS_CODE_MULTIPLY_GENERAL:
				mxToNumber(slot);
				mxToNumber(mxStack);
				slot->value.number *= mxStack->value.number;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_MODULO)
			slot = mxStack + 1;
			if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_INTEGER_KIND) && (mxStack->value.integer != 0)) {
				slot->value.integer %= mxStack->value.integer;
			} 
			else {
				mxToNumber(slot);
				mxToNumber(mxStack);
				slot->value.number = c_fmod(slot->value.number, mxStack->value.number);
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_NOT)
			mxToBoolean(mxStack);
			mxStack->value.boolean = !mxStack->value.boolean;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_LESS)
			slot = mxStack + 1;
			if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_INTEGER_KIND))
				offset = slot->value.integer < mxStack->value.integer;
			else if ((slot->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_NUMBER_KIND))
				goto XS_CODE_LESS_ALL;
			else {
				mxSaveState; 
				fxToPrimitive(the, slot, XS_NUMBER_HINT); 
				fxToPrimitive(the, mxStack, XS_NUMBER_HINT); 
				mxRestoreState;
				if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)))
					offset = c_strcmp(slot->value.string, mxStack->value.string) < 0;
				else {
			XS_CODE_LESS_ALL:
					mxToNumber(slot);
					mxToNumber(mxStack);
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number < mxStack->value.number);
				}
			}
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_LESS_EQUAL)
			slot = mxStack + 1;
			if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_INTEGER_KIND))
				offset = slot->value.integer <= mxStack->value.integer;
			else if ((slot->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_NUMBER_KIND))
				goto XS_CODE_LESS_EQUAL_ALL;
			else {
				mxSaveState; 
				fxToPrimitive(the, slot, XS_NUMBER_HINT); 
				fxToPrimitive(the, mxStack, XS_NUMBER_HINT); 
				mxRestoreState;
				if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)))
					offset = c_strcmp(slot->value.string, mxStack->value.string) <= 0;
				else {
			XS_CODE_LESS_EQUAL_ALL:
					mxToNumber(slot);
					mxToNumber(mxStack);
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number <= mxStack->value.number);
				}
			}
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_MORE)
			slot = mxStack + 1;
			if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_INTEGER_KIND))
				offset = slot->value.integer > mxStack->value.integer;
			else if ((slot->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_NUMBER_KIND))
				goto XS_CODE_MORE_ALL;
			else {
				mxSaveState; 
				fxToPrimitive(the, slot, XS_NUMBER_HINT); 
				fxToPrimitive(the, mxStack, XS_NUMBER_HINT); 
				mxRestoreState;
				if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)))
					offset = c_strcmp(slot->value.string, mxStack->value.string) > 0;
				else {
			XS_CODE_MORE_ALL:
					mxToNumber(slot);
					mxToNumber(mxStack);
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number > mxStack->value.number);
				}
			}
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_MORE_EQUAL)
			slot = mxStack + 1;
			if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_INTEGER_KIND))
				offset = slot->value.integer >= mxStack->value.integer;
			else if ((slot->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_NUMBER_KIND))
				goto XS_CODE_MORE_EQUAL_ALL;
			else {
				mxSaveState; 
				fxToPrimitive(the, slot, XS_NUMBER_HINT); 
				fxToPrimitive(the, mxStack, XS_NUMBER_HINT); 
				mxRestoreState;
				if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)))
					offset = c_strcmp(slot->value.string, mxStack->value.string) >= 0;
				else {
			XS_CODE_MORE_EQUAL_ALL:
					mxToNumber(slot);
					mxToNumber(mxStack);
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number >= mxStack->value.number);
				}
			}
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_EQUAL)
		mxCase(XS_CODE_STRICT_EQUAL)
			slot = mxStack + 1;
		XS_CODE_EQUAL_AGAIN:
			if (slot->kind == mxStack->kind) {
				if ((XS_UNDEFINED_KIND == slot->kind) || (XS_NULL_KIND == slot->kind))
					offset = 1;
				else if (XS_BOOLEAN_KIND == slot->kind)
					offset = slot->value.boolean == stack->value.boolean;
				else if (XS_INTEGER_KIND == slot->kind)
					offset = slot->value.integer == stack->value.integer;
				else if (XS_NUMBER_KIND == slot->kind)
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number == mxStack->value.number);
				else if ((XS_STRING_KIND == slot->kind) || (XS_STRING_X_KIND == slot->kind))
					offset = c_strcmp(slot->value.string, mxStack->value.string) == 0;
				else if (XS_SYMBOL_KIND == slot->kind)
					offset = slot->value.symbol == mxStack->value.symbol;
				else if (XS_REFERENCE_KIND == slot->kind)
					offset = slot->value.reference == mxStack->value.reference;
			#ifdef mxHostFunctionPrimitive
				else if (XS_HOST_FUNCTION_KIND == slot->kind)
					offset = slot->value.hostFunction.builder == mxStack->value.hostFunction.builder;
			#endif
                else
                    offset = 0;
			}
			else if ((XS_INTEGER_KIND == slot->kind) && (XS_NUMBER_KIND == mxStack->kind))
				offset = (!c_isnan(mxStack->value.number)) && ((txNumber)(slot->value.integer) == stack->value.number);
			else if ((XS_NUMBER_KIND == slot->kind) && (XS_INTEGER_KIND == mxStack->kind))
				offset = (!c_isnan(slot->value.number)) && (slot->value.number == (txNumber)(mxStack->value.integer));
			else if ((XS_STRING_KIND == slot->kind) && (XS_STRING_X_KIND == mxStack->kind))
				offset = c_strcmp(slot->value.string, mxStack->value.string) == 0;
			else if ((XS_STRING_X_KIND == slot->kind) && (XS_STRING_KIND == mxStack->kind))
				offset = c_strcmp(slot->value.string, mxStack->value.string) == 0;
			else if (XS_CODE_EQUAL == byte) {
				if ((slot->kind == XS_UNDEFINED_KIND) && (mxStack->kind == XS_NULL_KIND))
					offset = 1;
				else if ((slot->kind == XS_NULL_KIND) && (mxStack->kind == XS_UNDEFINED_KIND))
					offset = 1;
				else if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((XS_INTEGER_KIND == mxStack->kind) || (XS_NUMBER_KIND == mxStack->kind))) {
					fxToNumber(the, slot);
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)) && ((XS_INTEGER_KIND == slot->kind) || (XS_NUMBER_KIND == slot->kind))) {
					fxToNumber(the, mxStack); 
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (XS_BOOLEAN_KIND == slot->kind) {
					fxToNumber(the, slot);
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (XS_BOOLEAN_KIND == mxStack->kind) {
					fxToNumber(the, mxStack);
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (((slot->kind == XS_INTEGER_KIND) || (slot->kind == XS_NUMBER_KIND) || (slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND) || (slot->kind == XS_SYMBOL_KIND)) && mxIsReference(mxStack)) {
					mxSaveState;
					fxToPrimitive(the, mxStack, XS_NO_HINT);
					mxRestoreState;
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (((mxStack->kind == XS_INTEGER_KIND) || (mxStack->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND) || (mxStack->kind == XS_SYMBOL_KIND)) && mxIsReference(slot)) {
					mxSaveState;
					fxToPrimitive(the, slot, XS_NO_HINT);
					mxRestoreState;
					goto XS_CODE_EQUAL_AGAIN;
				}
                else
                    offset = 0;
			}
			else 
				offset = 0;
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_NOT_EQUAL)
		mxCase(XS_CODE_STRICT_NOT_EQUAL)
			slot = mxStack + 1;
		XS_CODE_NOT_EQUAL_AGAIN:
			if (slot->kind == mxStack->kind) {
				if ((XS_UNDEFINED_KIND == slot->kind) || (XS_NULL_KIND == slot->kind))
					offset = 0;
				else if (XS_BOOLEAN_KIND == slot->kind)
					offset = slot->value.boolean != stack->value.boolean;
				else if (XS_INTEGER_KIND == slot->kind)
					offset = slot->value.integer != stack->value.integer;
				else if (XS_NUMBER_KIND == slot->kind)
					offset = (c_isnan(slot->value.number)) || (c_isnan(mxStack->value.number)) || (slot->value.number != mxStack->value.number);
				else if ((XS_STRING_KIND == slot->kind) || (XS_STRING_X_KIND == slot->kind))
					offset = c_strcmp(slot->value.string, mxStack->value.string) != 0;
				else if (XS_SYMBOL_KIND == slot->kind)
					offset = slot->value.symbol != mxStack->value.symbol;
				else if (XS_REFERENCE_KIND == slot->kind)
					offset = slot->value.reference != mxStack->value.reference;
			#ifdef mxHostFunctionPrimitive
				else if (XS_HOST_FUNCTION_KIND == slot->kind)
					offset = slot->value.hostFunction.builder != mxStack->value.hostFunction.builder;
			#endif
                else
                    offset = 1;
			}
			else if ((XS_INTEGER_KIND == slot->kind) && (XS_NUMBER_KIND == mxStack->kind))
				offset = (c_isnan(mxStack->value.number)) || ((txNumber)(slot->value.integer) != stack->value.number);
			else if ((XS_NUMBER_KIND == slot->kind) && (XS_INTEGER_KIND == mxStack->kind))
				offset = (c_isnan(slot->value.number)) || (slot->value.number != (txNumber)(mxStack->value.integer));
			else if ((XS_STRING_KIND == slot->kind) && (XS_STRING_X_KIND == mxStack->kind))
				offset = c_strcmp(slot->value.string, mxStack->value.string) != 0;
			else if ((XS_STRING_X_KIND == slot->kind) && (XS_STRING_KIND == mxStack->kind))
				offset = c_strcmp(slot->value.string, mxStack->value.string) != 0;
			else if (XS_CODE_NOT_EQUAL == byte) {
				if ((slot->kind == XS_UNDEFINED_KIND) && (mxStack->kind == XS_NULL_KIND))
					offset = 0;
				else if ((slot->kind == XS_NULL_KIND) && (mxStack->kind == XS_UNDEFINED_KIND))
					offset = 0;
				else if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((XS_INTEGER_KIND == mxStack->kind) || (XS_NUMBER_KIND == mxStack->kind))) {
					fxToNumber(the, slot);
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)) && ((XS_INTEGER_KIND == slot->kind) || (XS_NUMBER_KIND == slot->kind))) {
					fxToNumber(the, mxStack); 
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (XS_BOOLEAN_KIND == slot->kind) {
					fxToNumber(the, slot);
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (XS_BOOLEAN_KIND == mxStack->kind) {
					fxToNumber(the, mxStack);
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (((slot->kind == XS_INTEGER_KIND) || (slot->kind == XS_NUMBER_KIND) || (slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND) || (slot->kind == XS_SYMBOL_KIND)) && mxIsReference(mxStack)) {
					mxSaveState;
					fxToPrimitive(the, mxStack, XS_NO_HINT);
					mxRestoreState;
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (((mxStack->kind == XS_INTEGER_KIND) || (mxStack->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND) || (mxStack->kind == XS_SYMBOL_KIND)) && mxIsReference(slot)) {
					mxSaveState;
					fxToPrimitive(the, slot, XS_NO_HINT);
					mxRestoreState;
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
                else
                    offset = 1;
			}
			else 
				offset = 1;
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_FOR_AWAIT_OF)
			mxNextCode(1);
			mxSaveState;
			fxRunForAwaitOf(the);
			mxRestoreState;
			mxBreak;
		mxCase(XS_CODE_FOR_IN)
			mxSkipCode(1);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = 0;
			/* SWAP THIS */
			scratch = *(mxStack);
			*(mxStack) = *(mxStack + 1);
			*(mxStack + 1) = scratch;
			/* FUNCTION */
			mxOverflow(1);
			*mxStack = mxEnumeratorFunction;
			slot = fxGetInstance(the, mxStack);
			/* TARGET */
			mxPushKind(XS_UNDEFINED_KIND);
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
			goto XS_CODE_CALL_ALL;

		mxCase(XS_CODE_FOR_OF)
			mxNextCode(1);
			mxSaveState;
			fxRunForOf(the);
			mxRestoreState;
			mxBreak;
			
		mxCase(XS_CODE_IN)
			mxNextCode(1);
			if (!mxIsReference(mxStack))
				mxRunDebug(XS_TYPE_ERROR, "in: no reference");
			mxSaveState;
			fxRunIn(the);
			mxRestoreState;
			mxBreak;
		mxCase(XS_CODE_INSTANCEOF)
			mxNextCode(1);
			mxSaveState;
			fxRunInstanceOf(the);
			mxRestoreState;
			mxBreak;
		mxCase(XS_CODE_TYPEOF)
			byte = mxStack->kind;
			if (XS_UNDEFINED_KIND == byte)
				*mxStack = mxUndefinedString;
			else if (XS_NULL_KIND == byte)
				*mxStack = mxObjectString;
			else if (XS_BOOLEAN_KIND == byte)
				*mxStack = mxBooleanString;
			else if ((XS_INTEGER_KIND == byte) || (XS_NUMBER_KIND == byte))
				*mxStack = mxNumberString;
			else if ((XS_STRING_KIND == byte) || (XS_STRING_X_KIND == byte))
				*mxStack = mxStringString;
			else if (XS_SYMBOL_KIND == byte)
				*mxStack = mxSymbolString;
			else if (XS_REFERENCE_KIND == byte) {
				slot = fxGetInstance(the, mxStack);
				if (fxIsFunction(the, slot))
					*mxStack = mxFunctionString;
				else
					*mxStack = mxObjectString;
			}
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_VOID)
			mxNextCode(1);
			mxStack->kind = XS_UNDEFINED_KIND;
			mxBreak;

	/* DEBUG */		
		mxCase(XS_CODE_DEBUGGER)
			mxNextCode(1);
		#ifdef mxDebug
			mxSaveState;
			fxDebugLoop(the, C_NULL, 0, "debugger");
			mxRestoreState;
		#endif
			mxBreak;
		mxCase(XS_CODE_FILE)
		#ifdef mxDebug
			id = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, id);
#endif
			mxFrameEnvironment->ID = id;
		#endif
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_LINE)
		#ifdef mxDebug
			id = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, id);
#endif
			mxFrameEnvironment->value.environment.line = id;
			if (fxIsReadable(the)) {
				mxSaveState;
				fxDebugCommand(the);
				mxRestoreState;
			}
			if ((mxFrame->flag & XS_STEP_OVER_FLAG) || mxBreakpoints.value.list.first) {
				mxSaveState;
				fxDebugLine(the);
				mxRestoreState;
			}
		#endif
			mxNextCode(3);
			mxBreak;

	/* MODULE */		
		mxCase(XS_CODE_TRANSFER)
			mxSkipCode(1);
			slot = mxTransferConstructor.value.reference;
			/* THIS */
			mxPushKind(XS_UNDEFINED_KIND);
			/* FUNCTION */
			mxOverflow(1);
			*mxStack = mxTransferConstructor;
			/* TARGET */
			mxOverflow(1);
			*mxStack = mxTransferConstructor;
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
			goto XS_CODE_CALL_ALL;
			mxBreak;
		mxCase(XS_CODE_MODULE)
			mxSkipCode(1);
			slot = mxModuleConstructor.value.reference;
			/* THIS */
			variable = mxFunctionInstanceHome(mxFrameFunction->value.reference);
			mxPushKind(XS_REFERENCE_KIND);
			mxStack->value.reference = variable->value.home.module;
			/* FUNCTION */
			mxOverflow(1);
			*mxStack = mxModuleConstructor;
			/* TARGET */
			mxPushKind(XS_UNDEFINED_KIND);
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
			goto XS_CODE_CALL_ALL;
			mxBreak;
			
	/* EVAL, PROGRAM & WITH */
		mxCase(XS_CODE_EVAL)
			mxSaveState;
			fxRunEval(the);
			mxRestoreState;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_EVAL_ENVIRONMENT)
			mxNextCode(1);
			mxSaveState;
			fxRunEvalEnvironment(the);
			mxRestoreState;
			mxBreak;
		mxCase(XS_CODE_EVAL_REFERENCE)
			offset = mxRunS2(1);
		#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
		#endif
			mxNextCode(3);
			variable = mxFrameEnvironment;
			if (variable->kind == XS_REFERENCE_KIND) {
				variable = variable->value.reference;
		XS_CODE_EVAL_REFERENCE_AGAIN:
				slot = variable->next;
				if (slot->kind == XS_REFERENCE_KIND) {
					slot = slot->value.reference;
					mxSaveState;
					index = fxIsScopableSlot(the, slot, (txID)offset);
					mxRestoreState;
					if (index) {
						if (XS_CODE_GET_VARIABLE == byte) {
							mxNextCode(3);
							if ((XS_CODE_CALL == byte) || (XS_CODE_CALL_TAIL == byte)) {
								mxStack->kind = XS_REFERENCE_KIND;
								mxStack->value.reference = slot;
							}
							mxNextCode(-3);
						}
						mxPushKind(XS_REFERENCE_KIND);
						mxStack->value.reference = slot;
						mxBreak;
					}
				}
				else if (mxBehaviorHasProperty(the, variable, (txID)offset, XS_NO_ID)) {
					mxPushKind(XS_REFERENCE_KIND);
					mxStack->value.reference = variable;
					mxBreak;
				}
				variable = variable->value.instance.prototype;
				if (variable)
					goto XS_CODE_EVAL_REFERENCE_AGAIN;
			}
			mxPushKind(XS_REFERENCE_KIND);
			mxStack->value.reference = mxGlobal.value.reference;
			mxBreak;
		mxCase(XS_CODE_PROGRAM_ENVIRONMENT)
			mxNextCode(1);
			mxSaveState;
			fxRunProgramEnvironment(the);
			mxRestoreState;
			mxBreak;
		mxCase(XS_CODE_PROGRAM_REFERENCE)
			offset = mxRunS2(1);
		#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset);
		#endif
			mxNextCode(3);
			variable = mxClosures.value.reference;
			if (mxBehaviorHasProperty(the, variable, (txID)offset, XS_NO_ID)) {
				mxPushKind(XS_REFERENCE_KIND);
				mxStack->value.reference = variable;
				mxBreak;
			}
			mxPushKind(XS_REFERENCE_KIND);
			mxStack->value.reference = mxGlobal.value.reference;
			mxBreak;
		mxCase(XS_CODE_WITH)
			variable = mxFrameEnvironment;
			mxSaveState;
			slot = fxNewEnvironmentInstance(the, variable);
			mxRestoreState;
			variable->kind = XS_REFERENCE_KIND;
			variable->value.reference = slot;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_WITHOUT)
			variable = mxFrameEnvironment;
			slot = variable->value.reference->value.instance.prototype;
			if (slot) {
				variable->kind = XS_REFERENCE_KIND;
				variable->value.reference = slot;
			}
			else
				variable->kind = XS_NULL_KIND;
			mxNextCode(1);
			mxBreak;
		}
	}
}

void fxRunBase(txMachine* the)
{
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: class");
	fxRunConstructor(the);
}

void fxRunConstructor(txMachine* the)
{
	txSlot* target;
	txSlot* prototype;
	target = mxTarget;
	mxPushUndefined();
	prototype = the->stack;
	fxBeginHost(the);
	mxPushSlot(target);
	fxGetPrototypeFromConstructor(the, &mxObjectPrototype);
	*prototype = *the->stack;
	fxEndHost(the);
	fxNewHostInstance(the);
	mxPullSlot(mxThis);
}

txBoolean fxRunDefine(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask) 
{
	txBoolean result;
	fxBeginHost(the);
	mxCheck(the, instance->kind == XS_INSTANCE_KIND);
	result = mxBehaviorDefineOwnProperty(the, instance, id, index, slot, mask);
	fxEndHost(the);
	return result;
}


txBoolean fxRunDelete(txMachine* the, txSlot* instance, txID id, txIndex index) 
{
	txBoolean result;
	fxBeginHost(the);
	mxCheck(the, instance->kind == XS_INSTANCE_KIND);
	result = mxBehaviorDeleteProperty(the, instance, id, index);
	fxEndHost(the);
	return result;
}

void fxRunDerived(txMachine* the)
{
	txSlot* slot;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: class");
	slot = fxNewSlot(the);
	slot->kind = XS_UNINITIALIZED_KIND;
	mxPushClosure(slot);
	mxPullSlot(mxThis);
}

void fxRunExtends(txMachine* the)
{
	txSlot* constructor;
	txSlot* prototype;
	
	constructor = fxGetInstance(the, the->stack);
	if (!constructor)
		mxTypeError("extends: no function");
	mxPushUndefined();
	prototype = the->stack;
	fxBeginHost(the);
	mxPushReference(constructor);
	fxGetID(the, mxID(_prototype));
	*prototype = *the->stack;
	fxEndHost(the);
	if (the->stack->kind == XS_NULL_KIND) {
		the->stack++;
		fxNewInstance(the);
	}
	else if (the->stack->kind == XS_REFERENCE_KIND) {
		if (the->stack->value.reference->value.instance.prototype == mxGeneratorPrototype.value.reference)
			mxTypeError("extends: generator");
		fxNewHostInstance(the);
	}
	else
		mxTypeError("extends: no prototype");
}

void fxRunEval(txMachine* the)
{	
#ifdef mxParse
	txStringStream aStream;
	txUnsigned flags;
	txSlot* function;
	txSlot* home;
	txSlot* closures;
	if (the->stack->value.integer == 0)
		the->stack->kind = XS_UNDEFINED_KIND;
	else {
		the->stack += the->stack->value.integer;
		if ((the->stack->kind == XS_STRING_KIND) || (the->stack->kind == XS_STRING_X_KIND)) {
			aStream.slot = the->stack;
			aStream.offset = 0;
			aStream.size = c_strlen(fxToString(the, aStream.slot));
			flags = mxProgramFlag | mxEvalFlag;
			if (the->frame->flag & XS_STRICT_FLAG)
				flags |= mxStrictFlag;
			function = mxFunction->value.reference;
			if (function->flag & XS_CAN_CONSTRUCT_FLAG)
				flags |= mxTargetFlag;
			home = mxFunctionInstanceHome(function);
			if (home->value.home.object)
				flags |= mxSuperFlag;
			closures = the->frame - 1;
			if (closures->kind == XS_REFERENCE_KIND)
				closures = closures->value.reference;
			else
				closures = C_NULL;
			fxRunScript(the, fxParseScript(the, &aStream, fxStringGetter, flags), mxThis, mxTarget, closures, home->value.home.object, C_NULL);
			aStream.slot->kind = the->stack->kind;
			aStream.slot->value = the->stack->value;
			the->stack++;
		}
	}
#else
	mxUnknownError("not built-in");
#endif
}

void fxRunForAwaitOf(txMachine* the)
{
	txSlot* slot = the->stack;
	fxBeginHost(the);
	mxPushInteger(0);
	mxPushSlot(slot);
	mxPushSlot(slot);
	fxGetID(the, mxID(_Symbol_asyncIterator));
	if (mxIsUndefined(the->stack)) {
		mxPop();
		fxCallID(the, mxID(_Symbol_iterator));
		fxNewAsyncFromSyncIteratorInstance(the);
	}
	else
		fxCall(the);
	mxPullSlot(slot);
	fxEndHost(the);
}

void fxRunForOf(txMachine* the)
{
	txSlot* slot = the->stack;
	fxBeginHost(the);
	mxPushInteger(0);
	mxPushSlot(slot);
	fxCallID(the, mxID(_Symbol_iterator));
	mxPullSlot(slot);
	fxEndHost(the);
}

void fxRunIn(txMachine* the)
{
	txSlot* left = the->stack + 1;
	txSlot* right = the->stack;
	fxBeginHost(the);
	mxPushSlot(right);
	mxPushSlot(left);
	left->value.boolean = fxHasAt(the);
	left->kind = XS_BOOLEAN_KIND;
	fxEndHost(the);
	the->stack++;
}

void fxRunInstanceOf(txMachine* the)
{
	txSlot* left = the->stack + 1;
	txSlot* right = the->stack;
	fxBeginHost(the);
	mxPushSlot(left);
	mxPushInteger(1);
	mxPushSlot(right);
	fxCallID(the, mxID(_Symbol_hasInstance));
	fxToBoolean(the, the->stack);
	mxPullSlot(left);
	fxEndHost(the);
	the->stack++;
}

void fxRunProxy(txMachine* the, txSlot* instance)
{
	txSlot* array;
	txIndex length = (txIndex)mxArgc;
	txIndex index;
	txSlot* address;
	mxPush(mxArrayPrototype);
	array = fxNewArrayInstance(the)->next;
	fxSetIndexSize(the, array, length);
	index = 0;
	address = array->value.array.address;
	while (index < length) {
		txSlot* property = mxArgv(index);
		*((txIndex*)address) = index;
		address->ID = XS_NO_ID;
		address->kind = property->kind;
		address->value = property->value;
		index++;
		address++;
	}
	if (mxTarget->kind == XS_UNDEFINED_KIND)
		mxBehaviorCall(the, instance, mxThis, the->stack);
	else if (instance->flag & XS_CAN_CONSTRUCT_FLAG)
		mxBehaviorConstruct(the, instance, the->stack, mxTarget);
	else
		mxTypeError("no constructor");
	the->stack++;
}

txBoolean fxIsSameSlot(txMachine* the, txSlot* a, txSlot* b)
{	
	txBoolean result = 0;
	if (a->kind == b->kind) {
		if ((XS_UNDEFINED_KIND == a->kind) || (XS_NULL_KIND == a->kind))
			result = 1;
		else if (XS_BOOLEAN_KIND == a->kind)
			result = a->value.boolean == b->value.boolean;
		else if (XS_INTEGER_KIND == a->kind)
			result = a->value.integer == b->value.integer;
        else if (XS_NUMBER_KIND == a->kind)
			result = (!c_isnan(a->value.number)) && (!c_isnan(b->value.number)) && (a->value.number == b->value.number);
		else if ((XS_STRING_KIND == a->kind) || (XS_STRING_X_KIND == a->kind))
			result = c_strcmp(a->value.string, b->value.string) == 0;
		else if (XS_SYMBOL_KIND == a->kind)
			result = a->value.symbol == b->value.symbol;
		else if (XS_REFERENCE_KIND == a->kind)
			result = a->value.reference == b->value.reference;
	}
	else if ((XS_INTEGER_KIND == a->kind) && (XS_NUMBER_KIND == b->kind))
		result = (!c_isnan(b->value.number)) && ((txNumber)(a->value.integer) == b->value.number);
	else if ((XS_NUMBER_KIND == a->kind) && (XS_INTEGER_KIND == b->kind))
		result = (!c_isnan(a->value.number)) && (a->value.number == (txNumber)(b->value.integer));
	else if ((XS_STRING_KIND == a->kind) && (XS_STRING_X_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	else if ((XS_STRING_X_KIND == a->kind) && (XS_STRING_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	return result;
}

txBoolean fxIsSameValue(txMachine* the, txSlot* a, txSlot* b, txBoolean zero)
{
	txBoolean result = 0;
	if (a->kind == b->kind) {
		if ((XS_UNDEFINED_KIND == a->kind) || (XS_NULL_KIND == a->kind))
			result = 1;
		else if (XS_BOOLEAN_KIND == a->kind)
			result = a->value.boolean == b->value.boolean;
		else if (XS_INTEGER_KIND == a->kind)
			result = a->value.integer == b->value.integer;
        else if (XS_NUMBER_KIND == a->kind)
			result = ((c_isnan(a->value.number) && c_isnan(b->value.number)) || ((a->value.number == b->value.number) && (zero || (c_signbit(a->value.number) == c_signbit(b->value.number)))));
		else if ((XS_STRING_KIND == a->kind) || (XS_STRING_X_KIND == a->kind))
			result = c_strcmp(a->value.string, b->value.string) == 0;
		else if (XS_SYMBOL_KIND == a->kind)
			result = a->value.symbol == b->value.symbol;
		else if (XS_REFERENCE_KIND == a->kind)
			result = a->value.reference == b->value.reference;
	}
	else if ((XS_INTEGER_KIND == a->kind) && (XS_NUMBER_KIND == b->kind)) {
		txNumber aNumber = a->value.integer;
		result = (aNumber == b->value.number) && (zero || (signbit(aNumber) == signbit(b->value.number)));
	}
	else if ((XS_NUMBER_KIND == a->kind) && (XS_INTEGER_KIND == b->kind)) {
		txNumber bNumber = b->value.integer;
		result = (a->value.number == bNumber) && (zero || (signbit(a->value.number) == signbit(bNumber)));
	}
	else if ((XS_STRING_KIND == a->kind) && (XS_STRING_X_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	else if ((XS_STRING_X_KIND == a->kind) && (XS_STRING_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	return result;
}

txBoolean fxIsScopableSlot(txMachine* the, txSlot* instance, txID id)
{	
	txBoolean result;
	fxBeginHost(the);
	mxPushReference(instance);
	result = fxHasID(the, id);
	if (result) {
		mxPushReference(instance);
		fxGetID(the, mxID(_Symbol_unscopables));
		if (mxIsReference(the->stack)) {
			fxGetID(the, id);
			result = fxToBoolean(the, the->stack) ? 0 : 1;
		}
		the->stack++;
	}
	fxEndHost(the);
	return result;
}

void fxRemapIDs(txMachine* the, txByte* codeBuffer, txSize codeSize, txID* theIDs)
{
	register const txS1* bytes = gxCodeSizes;
	register txByte* p = codeBuffer;
	register txByte* q = codeBuffer + codeSize;
	register txS1 offset;
	txU2 index;
	txID id;
	while (p < q) {
		//fprintf(stderr, "%s", gxCodeNames[*((txU1*)p)]);
		offset = (txS1)c_read8(bytes + c_read8(p));
		if (0 < offset)
			p += offset;
		else if (0 == offset) {
			p++;
			mxDecode2(p, id);
			if (id != XS_NO_ID) {
				id &= 0x7FFF;
				id = theIDs[id];
				p -= 2;
				mxEncode2(p, id);
			}
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
		//fprintf(stderr, "\n");
	}
}


void fxRunScript(txMachine* the, txScript* script, txSlot* _this, txSlot* _target, txSlot* closures, txSlot* object, txSlot* module)
{
	if (script) {
		mxTry(the) {
			txSlot* instance;
			txSlot* property;
			mxPushUndefined();
			if (script->symbolsBuffer) {
				txByte* p = script->symbolsBuffer;
				txInteger c = *((txU1*)p), i;
				p++;
				c <<= 8;
				c += *((txU1*)p);
				p++;
				the->stack->value.callback.address = C_NULL;
				the->stack->value.callback.IDs = (txID*)fxNewChunk(the, c * sizeof(txID));
				the->stack->kind = XS_CALLBACK_KIND;
				for (i = 0; i < c; i++) {
					txID id = fxNewNameC(the, (txString)p);
					the->stack->value.callback.IDs[i] = id;
					p += c_strlen((char*)p) + 1;
				}
				fxRemapIDs(the, script->codeBuffer, script->codeSize, the->stack->value.callback.IDs);
			}	
			else {
				the->stack->value.callback.address = C_NULL;
				the->stack->value.callback.IDs = C_NULL;
				the->stack->kind = XS_CALLBACK_KIND;
			}
			if (script->callback) {
				property = the->stack;
				/* ARGC */
				mxPushInteger(0);
				/* THIS */
				if (module)
					mxPushReference(module);
				else
					mxPush(mxGlobal);
				the->stack->ID = XS_NO_ID;
				/* FUNCTION */
				mxPush(mxFunctionPrototype);
				instance = fxNewFunctionInstance(the, closures ? mxID(_eval) : XS_NO_ID);
				instance->next->kind = XS_CALLBACK_KIND;
				instance->next->value.callback.address = script->callback;
				instance->next->value.callback.IDs = property->value.callback.IDs;
				property = mxFunctionInstanceHome(instance);
				property->value.home.object = object;
				property->value.home.module = module;
				/* TARGET */
				mxPushUndefined();
				/* RESULT */
				mxPushUndefined();
				fxRunID(the, C_NULL, XS_NO_ID);
			}
			else {
				mxPushUndefined();
			}
			if (module)
				mxPullSlot(mxModuleInstanceHosts(module));
			else
				mxPull(mxHosts);
			the->stack++;

			/* ARGC */
			mxPushInteger(0);
			/* THIS */
			if (_this)
				mxPushSlot(_this);
			else
				mxPushUndefined();
			the->stack->ID = XS_NO_ID;
			/* FUNCTION */
			mxPush(mxFunctionPrototype);
			instance = fxNewFunctionInstance(the, XS_NO_ID);
			instance->next->kind = XS_CODE_X_KIND;
			instance->next->value.code.address = script->codeBuffer;
			instance->next->value.code.closures = closures;
			property = mxFunctionInstanceHome(instance);
			property->value.home.object = object;
			property->value.home.module = module;
			/* TARGET */
			if (_target)
				mxPushSlot(_target);
			else
				mxPushUndefined();
			/* RESULT */
			mxPushUndefined();
			fxRunID(the, C_NULL, XS_NO_ID);

			mxPushUndefined();
			mxPull(mxHosts);
			if (!module)
				fxGlobalDeleteProperty(the, mxGlobal.value.reference, mxID(0), XS_NO_ID);		
			if (script->symbolsBuffer)
				fxDeleteScript(script);
		}
		mxCatch(the) {
			if (!module)
				fxGlobalDeleteProperty(the, mxGlobal.value.reference, mxID(0), XS_NO_ID);		
			if (script->symbolsBuffer)
				fxDeleteScript(script);
			fxJump(the);
		}
	}
	else {
		mxSyntaxError("invalid script");
	}
}













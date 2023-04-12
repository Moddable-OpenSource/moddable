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
static void fxRunArguments(txMachine* the, txIndex offset);
static void fxRunBase(txMachine* the);
static void fxRunConstructor(txMachine* the);
static txBoolean fxRunDefine(txMachine* the, txSlot* instance, txSlot* check, txID id, txIndex index, txSlot* slot, txFlag mask);
static txBoolean fxRunDelete(txMachine* the, txSlot* instance, txID id, txIndex index);
static void fxRunDerived(txMachine* the);
static void fxRunExtends(txMachine* the);
static void fxRunForOf(txMachine* the);
static txBoolean fxRunHas(txMachine* the, txSlot* instance, txID id, txIndex index);
static void fxRunIn(txMachine* the);
static void fxRunInstantiate(txMachine* the);
static void fxRunProxy(txMachine* the, txSlot* instance);
static void fxRunInstanceOf(txMachine* the);
static txBoolean fxIsScopableSlot(txMachine* the, txSlot* instance, txID id);
static txBoolean fxToNumericInteger(txMachine* the, txSlot* theSlot);
static txBoolean fxToNumericIntegerUnary(txMachine* the, txSlot* theSlot, txBigIntUnary op);
static txBoolean fxToNumericIntegerBinary(txMachine* the, txSlot* a, txSlot* b, txBigIntBinary op);
static txBoolean fxToNumericNumber(txMachine* the, txSlot* theSlot);
static txBoolean fxToNumericNumberUnary(txMachine* the, txSlot* theSlot, txBigIntUnary op);
static txBoolean fxToNumericNumberBinary(txMachine* the, txSlot* a, txSlot* b, txBigIntBinary op);

#if defined(__GNUC__) && defined(__OPTIMIZE__)
	#if defined(mxFrequency)
		#define mxBreak \
			the->frequencies[byte]++; \
			goto *bytes[byte]
	#elif defined(mxTrace)
		#define mxBreak \
			if (gxDoTrace) fxTraceCode(the, stack, byte); \
			goto *bytes[byte]
	#elif defined(mxMetering)
		#define mxBreak \
			the->meterIndex++; \
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
#define mxEnvironment environment
#define mxScope scope
#define mxStack stack

#define mxFrameEnd (mxFrame + 5)
#define mxFrameThis (mxFrame + 4)
#define mxFrameFunction (mxFrame + 3)
#define mxFrameTarget (mxFrame + 2)
#define mxFrameResult (mxFrame + 1)
#define mxFrameArgc ((mxFrame - 1)->value.integer)
#define mxFrameArgv(THE_INDEX) (mxFrame - 2 - (THE_INDEX))

#define mxRestoreState { \
	mxStack = the->stack; \
	mxScope = the->scope; \
	mxFrame = the->frame; \
	mxEnvironment = mxFrameToEnvironment(mxFrame); \
	mxCode = the->code; \
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
		the->stack = mxStack; \
		the->scope = mxScope; \
		the->frame = mxFrame; \
		the->code = mxCode; \
	}
#endif

#if mxBoundsCheck
#define mxAllocStack(_COUNT) \
	if ((mxStack - _COUNT) < the->stackBottom) { \
		mxSaveState; \
		fxAbort(the, XS_STACK_OVERFLOW_EXIT); \
	} \
	mxStack -= _COUNT
#else
#define mxAllocStack(_COUNT) \
	mxStack -= _COUNT
#endif

#define mxPushKind(_KIND) { \
	mxAllocStack(1); \
	mxStack->next = C_NULL;  \
	mxInitSlotKind(mxStack, _KIND); \
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
		if (XS_STRING_KIND <= (SLOT)->kind) { \
			mxSaveState; \
			fxToInteger(the, SLOT); \
			mxRestoreState; \
		} \
		else \
			fxToInteger(the, SLOT); \
	}
	
#define mxToNumber(SLOT) \
	if (XS_NUMBER_KIND != (SLOT)->kind) { \
		if (XS_STRING_KIND <= (SLOT)->kind) { \
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
		primitive = 1; \
	} \
	else { \
		variable = (SLOT)->value.reference; \
		primitive = 0; \
	}

#ifndef mxUnalignedAccess
	#define mxUnalignedAccess 1
#endif

#define mxRunS1(OFFSET) ((txS1*)mxCode)[OFFSET]
#define mxRunU1(OFFSET) ((txU1*)mxCode)[OFFSET]
#if mxBigEndian
	#define mxRunS2(OFFSET) (((txS1*)mxCode)[OFFSET+0] << 8) | ((txU1*)mxCode)[OFFSET+1]
	#define mxRunS4(OFFSET) (((txS1*)mxCode)[OFFSET+0] << 24) | (((txU1*)mxCode)[OFFSET+1] << 16) | (((txU1*)mxCode)[OFFSET+2] << 8) | ((txU1*)mxCode)[OFFSET+3]
	#define mxRunU2(OFFSET) (((txU1*)mxCode)[OFFSET+0] << 8) | ((txU1*)mxCode)[OFFSET+1]
#else
	#if mxUnalignedAccess
		#define mxRunS2(OFFSET) *((txS2*)(mxCode + OFFSET))
		#define mxRunS4(OFFSET) *((txS4*)(mxCode + OFFSET))
		#define mxRunU2(OFFSET) *((txU2*)(mxCode + OFFSET))
	#else	
		#define mxRunS2(OFFSET) (((txS1*)mxCode)[OFFSET+1] << 8) | ((txU1*)mxCode)[OFFSET+0]
		#define mxRunS4(OFFSET) (((txS1*)mxCode)[OFFSET+3] << 24) | (((txU1*)mxCode)[OFFSET+2] << 16) | (((txU1*)mxCode)[OFFSET+1] << 8) | ((txU1*)mxCode)[OFFSET+0]
		#define mxRunU2(OFFSET) (((txU1*)mxCode)[OFFSET+1] << 8) | ((txU1*)mxCode)[OFFSET+0]
	#endif
#endif
#ifdef mx32bitID
	#define mxRunID(OFFSET) mxRunS4(OFFSET)
#else
	#define mxRunID(OFFSET) mxRunS2(OFFSET)
#endif

#define mxNextCode(OFFSET) { \
	mxCode += OFFSET; \
	byte = *((txU1*)mxCode); \
}
#define mxSkipCode(OFFSET) { \
	mxCode += OFFSET; \
}

#ifdef mxMetering
	#define mxCheckMeter() \
		if (the->meterInterval && (the->meterIndex > the->meterCount)) { \
			mxSaveState; \
			fxCheckMetering(the); \
			mxRestoreState; \
		}
	#define mxBranch(INDEX, OFFSET) \
		if ((OFFSET) < 0) { \
			mxCheckMeter(); \
		} \
		mxNextCode((txS4)INDEX + OFFSET)
	#define mxBranchElse(TEST, INDEX, OFFSET) \
		if (TEST) { \
			mxNextCode((txS4)INDEX); \
		} \
		else { \
			mxBranch(INDEX, OFFSET); \
		}
	#define mxBranchIf(TEST, INDEX, OFFSET) \
		if (TEST) { \
			mxBranch(INDEX, OFFSET); \
		} \
		else { \
			mxNextCode((txS4)index); \
		}
	#define mxFirstCode() \
		mxCheckMeter(); \
		byte = *((txU1*)mxCode)
#else
	#define mxBranch(INDEX, OFFSET) \
		mxNextCode((txS4)INDEX + OFFSET)
	#define mxBranchElse(TEST, INDEX, OFFSET) \
		mxNextCode((TEST) ? (txS4)INDEX : (txS4)INDEX + OFFSET)
	#define mxBranchIf(TEST, INDEX, OFFSET) \
		mxNextCode((TEST) ? (txS4)INDEX + OFFSET : (txS4)INDEX)
	#define mxFirstCode() \
		byte = *((txU1*)mxCode)
#endif

#ifdef mxTrace
short gxDoTrace = 1;

#ifdef mxMetering
static void fxTraceCode(txMachine* the, txSlot* stack, txU1 theCode) 
{
	if (((XS_NO_CODE < theCode) && (theCode < XS_CODE_COUNT)))
		fprintf(stderr, "\n%u %ld: %s", the->meterIndex, the->stackTop - stack, gxCodeNames[theCode]);
	else
		fprintf(stderr, "\n%u %ld: ?", the->meterIndex, the->stackTop - stack);
}
#else
static void fxTraceCode(txMachine* the, txSlot* stack, txU1 theCode) 
{
	if (((XS_NO_CODE < theCode) && (theCode < XS_CODE_COUNT)))
		fprintf(stderr, "\n%ld: %s", the->stackTop - stack, gxCodeNames[theCode]);
	else
		fprintf(stderr, "\n%ld: ?", the->stackTop - stack);
}
#endif

static void fxTraceID(txMachine* the, txID id, txIndex index) 
{
	if (id) {
		char* key = fxGetKeyName(the, id);
		if (key)
			fprintf(stderr, " [%s]", key);
		else
			fprintf(stderr, " [?]");
	}
	else
		fprintf(stderr, " [%d]", index);
}

static void fxTraceIndex(txMachine* the, txU2 theIndex) 
{
	fprintf(stderr, " %d", theIndex);
}

static void fxTraceInteger(txMachine* the, txInteger theInteger) 
{
	fprintf(stderr, " %ld", (long)theInteger);
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
int gxTraceCall = 0;
static int depth = 0;
static void fxTraceCallBegin(txMachine* the, txSlot* function)
{
	if (gxTraceCall) {
		txSlot* slot = mxBehaviorGetProperty(the, function->value.reference, mxID(_name), 0, XS_ANY);
		int i;
		for (i = 0; i < depth; i++)
			fprintf(stderr, "\t");
		if (slot && ((slot->kind == XS_STRING_KIND) ||  (slot->kind == XS_STRING_X_KIND)))
			fprintf(stderr, " [%s]\n", slot->value.string);
		else
			fprintf(stderr, " [?]\n");
		depth++;
	}
}
static void fxTraceCallEnd(txMachine* the, txSlot* function)
{
	if (gxTraceCall) {
		depth--;
		if (depth < 0)
			depth = 0;
	}
}
#endif

void fxRunID(txMachine* the, txSlot* generator, txInteger count)
{
	register txSlot* stack = the->stack;
	register txSlot* scope = the->scope;
	register txSlot* frame = the->frame;
	register txSlot* environment;
	register txByte* code = the->code;
	register txSlot* variable;
	register txSlot* slot;
	register txU1 byte = 0;
	register txU4 index;
	register txS4 offset;
	txU1 primitive = 0;
#if defined(__GNUC__) && defined(__OPTIMIZE__)
	static void *const
	#if !defined(__ets__) || ESP32
		ICACHE_RAM_ATTR
	#endif
		gxBytes[] = {
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
		&&XS_CODE_BEGIN_STRICT_FIELD,
		&&XS_CODE_BIGINT_1,
		&&XS_CODE_BIGINT_2,
		&&XS_CODE_BIT_AND,
		&&XS_CODE_BIT_NOT,
		&&XS_CODE_BIT_OR,
		&&XS_CODE_BIT_XOR,
		&&XS_CODE_BRANCH_1,
		&&XS_CODE_BRANCH_2,
		&&XS_CODE_BRANCH_4,
		&&XS_CODE_BRANCH_CHAIN_1,
		&&XS_CODE_BRANCH_CHAIN_2,
		&&XS_CODE_BRANCH_CHAIN_4,
		&&XS_CODE_BRANCH_COALESCE_1,
		&&XS_CODE_BRANCH_COALESCE_2,
		&&XS_CODE_BRANCH_COALESCE_4,
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
		&&XS_CODE_COPY_OBJECT,
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
		&&XS_CODE_EVAL_PRIVATE,
		&&XS_CODE_EVAL_REFERENCE,
		&&XS_CODE_EVAL_TAIL,
		&&XS_CODE_EXCEPTION,
		&&XS_CODE_EXPONENTIATION,
		&&XS_CODE_EXTEND,
		&&XS_CODE_FALSE,
		&&XS_CODE_FILE,
		&&XS_CODE_FOR_AWAIT_OF,
		&&XS_CODE_FOR_IN,
		&&XS_CODE_FOR_OF,
		&&XS_CODE_FUNCTION,
		&&XS_CODE_FUNCTION_ENVIRONMENT,
		&&XS_CODE_GENERATOR_FUNCTION,
		&&XS_CODE_GET_CLOSURE_1,
		&&XS_CODE_GET_CLOSURE_2,
		&&XS_CODE_GET_LOCAL_1,
		&&XS_CODE_GET_LOCAL_2,
		&&XS_CODE_GET_PRIVATE_1,
		&&XS_CODE_GET_PRIVATE_2,
		&&XS_CODE_GET_PROPERTY,
		&&XS_CODE_GET_PROPERTY_AT,
		&&XS_CODE_GET_RESULT,
		&&XS_CODE_GET_SUPER,
		&&XS_CODE_GET_SUPER_AT,
		&&XS_CODE_GET_THIS,
		&&XS_CODE_GET_VARIABLE,
		&&XS_CODE_GET_THIS_VARIABLE,
		&&XS_CODE_GLOBAL,
		&&XS_CODE_HAS_PRIVATE_1,
		&&XS_CODE_HAS_PRIVATE_2,
		&&XS_CODE_HOST,
		&&XS_CODE_IMPORT,
		&&XS_CODE_IMPORT_META,
		&&XS_CODE_IN,
		&&XS_CODE_INCREMENT,
		&&XS_CODE_INSTANCEOF,
		&&XS_CODE_INSTANTIATE,
		&&XS_CODE_INTEGER_1,
		&&XS_CODE_INTEGER_2,
		&&XS_CODE_INTEGER_4,
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
		&&XS_CODE_NEW_PRIVATE_1,
		&&XS_CODE_NEW_PRIVATE_2,
		&&XS_CODE_NEW_PROPERTY,
		&&XS_CODE_NEW_PROPERTY_AT,
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
		&&XS_CODE_REGEXP,
		&&XS_CODE_RESERVE_1,
		&&XS_CODE_RESERVE_2,
		&&XS_CODE_RESET_CLOSURE_1,
		&&XS_CODE_RESET_CLOSURE_2,
		&&XS_CODE_RESET_LOCAL_1,
		&&XS_CODE_RESET_LOCAL_2,
		&&XS_CODE_RETHROW,
		&&XS_CODE_RETRIEVE_1,
		&&XS_CODE_RETRIEVE_2,
		&&XS_CODE_RETRIEVE_TARGET,
		&&XS_CODE_RETRIEVE_THIS,
		&&XS_CODE_RETURN,
		&&XS_CODE_RUN,
		&&XS_CODE_RUN_1,
		&&XS_CODE_RUN_2,
		&&XS_CODE_RUN_4,
		&&XS_CODE_RUN_TAIL,
		&&XS_CODE_RUN_TAIL_1,
		&&XS_CODE_RUN_TAIL_2,
		&&XS_CODE_RUN_TAIL_4,
		&&XS_CODE_SET_CLOSURE_1,
		&&XS_CODE_SET_CLOSURE_2,
		&&XS_CODE_SET_HOME,
		&&XS_CODE_SET_LOCAL_1,
		&&XS_CODE_SET_LOCAL_2,
		&&XS_CODE_SET_PRIVATE_1,
		&&XS_CODE_SET_PRIVATE_2,
		&&XS_CODE_SET_PROPERTY,
		&&XS_CODE_SET_PROPERTY_AT,
		&&XS_CODE_SET_RESULT,
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
		&&XS_CODE_STRING_4,
		&&XS_CODE_STRING_ARCHIVE_1,
		&&XS_CODE_STRING_ARCHIVE_2,
		&&XS_CODE_STRING_ARCHIVE_4,
		&&XS_CODE_SUBTRACT,
		&&XS_CODE_SUPER,
		&&XS_CODE_SWAP,
		&&XS_CODE_SYMBOL,
		&&XS_CODE_TARGET,
		&&XS_CODE_TEMPLATE,
		&&XS_CODE_TEMPLATE_CACHE,
		&&XS_CODE_THIS,
		&&XS_CODE_THROW,
		&&XS_CODE_THROW_STATUS,
		&&XS_CODE_TO_INSTANCE,
		&&XS_CODE_TO_NUMERIC,
		&&XS_CODE_TO_STRING,
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
		&&XS_CODE_PROFILE,
	};
	register void * const *bytes = gxBytes;
#endif
	txJump* jump = C_NULL;
	txSlot scratch;
	txSlot** address;
	txJump* yieldJump = the->firstJump;

	mxCheckCStack();
	if (generator) {
		slot = mxStack;
		variable = generator->next;
		offset = variable->value.stack.length;
		mxAllocStack(offset);
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
					mxScope = jump->scope;
					mxFrame = jump->frame;
					mxEnvironment = mxFrameToEnvironment(mxFrame);
					mxCode = jump->code;
					c_free(jump);
					goto XS_CODE_JUMP;
				}
			}
			else {
				mxSaveState;
				fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
			}
			offset--;
		}
		variable = slot - (mxStack++)->value.integer;
		variable->next = mxFrame;
		variable->flag &= XS_STRICT_FLAG;
#ifdef mxDebug
		if (mxFrame && (mxFrame->flag & XS_STEP_INTO_FLAG))
			variable->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
		variable->value.frame.code = the->code;
		variable->value.frame.scope = mxScope;
		mxFrame = variable;
		mxEnvironment = mxFrameToEnvironment(mxFrame);
		mxScope = slot - (mxStack++)->value.integer;
		mxCode = mxCode + (mxStack++)->value.integer;
		mxStack->kind = the->scratch.kind;
		mxStack->value = the->scratch.value;
#ifdef mxTraceCall
		fxTraceCallBegin(the, mxFrameFunction);
#endif
XS_CODE_JUMP:
		mxFirstCode();
	}
	else {
		offset = count;
		goto XS_CODE_RUN_ALL;
	}
	for (;;) {

#ifdef mxTrace
		if (gxDoTrace) fxTraceCode(the, stack, byte);
#endif
#ifdef mxMetering
		the->meterIndex++;
#endif
		
		mxSwitch(byte) {
		mxCase(XS_NO_CODE)
			mxBreak;

		mxCase(XS_CODE_RUN_TAIL)
			mxSkipCode(1);
			offset = mxStack->value.integer;
			mxStack++;
			goto XS_CODE_RUN_TAIL_ALL;
		mxCase(XS_CODE_RUN_TAIL_4)
			offset = mxRunS4(1);
			mxSkipCode(5);
			goto XS_CODE_RUN_TAIL_ALL;
		mxCase(XS_CODE_RUN_TAIL_2)
			offset = mxRunS2(1);
			mxSkipCode(3);
			goto XS_CODE_RUN_TAIL_ALL;
		mxCase(XS_CODE_RUN_TAIL_1)
			offset = mxRunS1(1);
			mxSkipCode(2);
		XS_CODE_RUN_TAIL_ALL:
			if (mxFrameTarget->kind)
				goto XS_CODE_RUN_ALL;
#ifdef mxDebug
			slot = mxStack + offset + 4;
			if (!fxIsCallable(the, slot))
				goto XS_CODE_RUN_ALL;
#endif
			variable = mxFrameEnd - 6 - offset;
			mxScope = mxFrame->value.frame.scope;
			mxCode = mxFrame->value.frame.code;
			mxFrame = mxFrame->next;
			c_memmove(variable, mxStack, (6 + offset) * sizeof(txSlot));
			mxStack = variable;
			goto XS_CODE_RUN_ALL;

		mxCase(XS_CODE_RUN)
			mxSkipCode(1);
			offset = mxStack->value.integer;
			mxStack++;
			goto XS_CODE_RUN_ALL;
		mxCase(XS_CODE_RUN_4)
			offset = mxRunS4(1);
			mxSkipCode(5);
			goto XS_CODE_RUN_ALL;
		mxCase(XS_CODE_RUN_2)
			offset = mxRunS2(1);
			mxSkipCode(3);
			goto XS_CODE_RUN_ALL;
		mxCase(XS_CODE_RUN_1)
			offset = mxRunS1(1);
			mxSkipCode(2);
		XS_CODE_RUN_ALL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, offset);
#endif
			// COUNT
			slot = mxStack + offset;
			slot->kind = XS_INTEGER_KIND;
			slot->value.integer = offset;
			slot++;
			// FRAME
			slot->kind = XS_FRAME_KIND;
			slot->next = mxFrame;
			slot->flag = XS_NO_FLAG;
#ifdef mxDebug
			if (mxFrame && (mxFrame->flag & XS_STEP_INTO_FLAG))
				slot->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
			slot->value.frame.code = mxCode;
			slot->value.frame.scope = mxScope;
			slot++;
			// RESULT
			slot++;
			// TARGET
			slot++;
			// FUNCTION
			byte = ((slot + 1)->kind == XS_UNINITIALIZED_KIND) ? 1 : 0;
			if (slot->kind == XS_REFERENCE_KIND) {
				variable = slot->value.reference;
				slot = variable->next;
				if (slot && (slot->flag & XS_INTERNAL_FLAG)) {
					if ((slot->kind == XS_CODE_KIND) || (slot->kind == XS_CODE_X_KIND)) {
						if (byte && !mxIsConstructor(variable))
							mxRunDebug(XS_TYPE_ERROR, "no constructor");
						variable = slot->value.code.closures;
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
						mxFrame = mxStack + 1 + offset + 1;
						mxEnvironment = mxStack;
						mxScope = mxStack;
						mxCode = slot->value.code.address;
			#ifdef mxTraceCall
						fxTraceCallBegin(the, mxFrameFunction);
			#endif
						mxFirstCode();
						mxBreak;
					}
					if ((slot->kind == XS_CALLBACK_KIND) || (slot->kind == XS_CALLBACK_X_KIND)) {
						if (byte && !mxIsConstructor(variable))
							mxRunDebug(XS_TYPE_ERROR, "no constructor");
						mxPushKind(XS_VAR_KIND);
						mxStack->value.environment.variable.count = 0;
			#ifdef mxDebug
						mxStack->ID = XS_NO_ID;
						mxStack->value.environment.line = 0;
			#endif
						mxFrame = mxStack + 1 + offset + 1;
						mxFrame->flag |= XS_C_FLAG;
						mxScope = mxStack;
						mxCode = C_NULL;
						byte = XS_CODE_CALL;
			#ifdef mxTraceCall
						fxTraceCallBegin(the, mxFrameFunction);
			#endif
						mxSaveState;
			#ifdef mxLink
						if ((txU1*)slot->value.callback.address - (txU1*)the->fakeCallback < 0)
							mxRunDebug(XS_TYPE_ERROR, "not available");
			#endif
						if (slot->flag & XS_BASE_FLAG)
							fxRunBase(the);
						else if (slot->flag & XS_DERIVED_FLAG)
							fxRunDerived(the);
						(*(slot->value.callback.address))(the);
						mxRestoreState;
			#if defined(mxInstrument) || defined(mxProfile)
						fxCheckProfiler(the, mxFrame);
			#endif
						if (slot->flag & XS_BASE_FLAG)
							goto XS_CODE_END_BASE_ALL;
						if (slot->flag & XS_DERIVED_FLAG)
							goto XS_CODE_END_DERIVED_ALL;
						slot = mxFrameResult;
						goto XS_CODE_END_ALL;
					}
					if (slot->kind == XS_PROXY_KIND) {
						mxPushKind(XS_VAR_KIND);
						mxStack->value.environment.variable.count = 0;
			#ifdef mxDebug
						mxStack->ID = XS_NO_ID;
						mxStack->value.environment.line = 0;
			#endif
						mxFrame = mxStack + 1 + offset + 1;
						mxFrame->flag |= XS_C_FLAG;
						mxScope = mxStack;
						mxCode = C_NULL;
						byte = XS_CODE_CALL;
			#ifdef mxTraceCall
						fxTraceCallBegin(the, mxFrameFunction);
			#endif
						mxSaveState;
						fxRunProxy(the, variable);
						mxRestoreState;
						slot = mxFrameResult;
						goto XS_CODE_END_ALL;
					}
				}
			}
#ifdef mxHostFunctionPrimitive
			else if (slot->kind == XS_HOST_FUNCTION_KIND) {
				if (byte)
					mxRunDebug(XS_TYPE_ERROR, "no constructor");
				mxPushKind(XS_VAR_KIND);
				mxStack->value.environment.variable.count = 0;
#ifdef mxDebug
				mxStack->ID = XS_NO_ID;
				mxStack->value.environment.line = 0;
#endif
				mxFrame = mxStack + 1 + offset + 1;
				mxFrame->flag |= XS_C_FLAG;
				mxScope = mxStack;
				mxCode = C_NULL;
				byte = XS_CODE_CALL;
#ifdef mxTraceCall
				fxTraceCallBegin(the, mxFrameFunction);
#endif
				mxSaveState;
#ifdef mxLink
				if ((txU1*)slot->value.hostFunction.builder->callback - (txU1*)the->fakeCallback < 0)
					mxRunDebug(XS_TYPE_ERROR, "not available");
#endif
				(*(slot->value.hostFunction.builder->callback))(the);
				mxRestoreState;
				slot = mxFrameResult;
				goto XS_CODE_END_ALL;
			}
#endif
			mxRunDebug(XS_TYPE_ERROR, "no function");
			mxBreak;
			
		mxCase(XS_CODE_BEGIN_SLOPPY)
            if (mxFrameTarget->kind != XS_UNDEFINED_KIND) {
				mxSaveState;
				fxRunConstructor(the);
				mxRestoreState;
			}
			else {
				index = mxFrameThis->kind;
				if (index < XS_REFERENCE_KIND) {
					if ((index == XS_UNDEFINED_KIND) || (index == XS_NULL_KIND)) {
						mxFrameThis->kind = XS_REFERENCE_KIND;
						variable = mxFunctionInstanceHome(mxFrameFunction->value.reference)->value.home.module;
						variable = mxModuleInstanceInternal(variable)->value.module.realm;
						if (!variable) variable = mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm;
						mxFrameThis->value.reference = mxRealmGlobal(variable)->value.reference;
					}
					else {
						mxSaveState;
						fxToInstance(the, mxFrameThis);
						mxRestoreState;
					}
				}
			}
			mxNextCode(2);
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
		mxCase(XS_CODE_BEGIN_STRICT_FIELD)
			mxFrame->flag |= XS_STRICT_FLAG | XS_FIELD_FLAG;
            if (mxFrameTarget->kind != XS_UNDEFINED_KIND) {
				mxSaveState;
				fxRunConstructor(the);
				mxRestoreState;
			}
			mxNextCode(2);
			mxBreak;
					
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
#ifdef mxInstrument
			if (the->stackPeak > mxStack)
				the->stackPeak = mxStack;
#endif
			mxStack = mxFrameEnd;
			mxScope = mxFrame->value.frame.scope;
			mxCode = mxFrame->value.frame.code;
			mxAllocStack(1);
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
			mxEnvironment = mxFrameToEnvironment(mxFrame);
			mxFirstCode();
			mxBreak;
		mxCase(XS_CODE_RETURN)
			mxStack = mxFrameEnd;
			mxScope = mxFrame->value.frame.scope;
			mxCode = mxFrame->value.frame.code;
			mxAllocStack(1);
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
			mxSaveState;
			variable = gxDefaults.newAsyncInstance(the);
			mxRestoreState;
			slot = mxFrameEnd;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxPtrDiff(mxCode - mxFrameFunction->value.reference->next->value.code.address);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxPtrDiff(slot - mxScope);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxPtrDiff(slot - mxFrame);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = 0;
			mxPushKind(mxFrameFunction->kind);
			mxStack->value = mxFrameFunction->value;
			index = mxPtrDiff(slot - mxStack);
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
			mxStack += 5;
			mxSaveState;
			gxDefaults.runAsync(the, mxStack->value.reference);
			mxRestoreState;
			slot = mxFrameResult;
 			goto XS_CODE_END_ALL;
 			
		mxCase(XS_CODE_START_ASYNC_GENERATOR)
			mxSkipCode(1);
            if (mxFrameTarget->kind != XS_UNDEFINED_KIND)
				mxRunDebug(XS_TYPE_ERROR, "new async generator");
			slot = mxBehaviorGetProperty(the, mxFrameFunction->value.reference, mxID(_prototype), 0, XS_ANY);
			mxPushKind(slot->kind);
			mxStack->value = slot->value;
			mxSaveState;
			variable = gxDefaults.newAsyncGeneratorInstance(the);
			mxRestoreState;
			mxFrameResult->kind = XS_UNINITIALIZED_KIND;
			slot = mxFrameEnd;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxPtrDiff(mxCode - mxFrameFunction->value.reference->next->value.code.address);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxPtrDiff(slot - mxScope);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxPtrDiff(slot - mxFrame);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = 0;
			mxPushKind(mxFrameFunction->kind);
			mxStack->value = mxFrameFunction->value;
			index = mxPtrDiff(slot - mxStack);
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
			mxStack += 5;
			*mxFrameResult = *(mxStack++);
			slot = mxFrameResult;
			goto XS_CODE_END_ALL;
		
		mxCase(XS_CODE_START_GENERATOR)
			mxSkipCode(1);
            if (mxFrameTarget->kind != XS_UNDEFINED_KIND)
				mxRunDebug(XS_TYPE_ERROR, "new generator");
			slot = mxBehaviorGetProperty(the, mxFrameFunction->value.reference, mxID(_prototype), 0, XS_ANY);
			mxPushKind(slot->kind);
			mxStack->value = slot->value;
			mxSaveState;
			variable = gxDefaults.newGeneratorInstance(the);
			mxRestoreState;
			slot = mxFrameEnd;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxPtrDiff(mxCode - mxFrameFunction->value.reference->next->value.code.address);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxPtrDiff(slot - mxScope);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxPtrDiff(slot - mxFrame);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = 0;
			mxPushKind(mxFrameFunction->kind);
			mxStack->value = mxFrameFunction->value;
			index = mxPtrDiff(slot - mxStack);
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
			mxStack += 5;
			*mxFrameResult = *(mxStack++);
			slot = mxFrameResult;
 			goto XS_CODE_END_ALL;
 			
		mxCase(XS_CODE_AWAIT)
		mxCase(XS_CODE_YIELD)
			generator->next->next->value.integer = byte;
			mxSkipCode(1);
			slot = mxFrameEnd;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxPtrDiff(mxCode - mxFrameFunction->value.reference->next->value.code.address);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxPtrDiff(slot - mxScope);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxPtrDiff(slot - mxFrame);
			jump = the->firstJump;
			offset = 0;
			while (jump != yieldJump) {
				txJump* nextJump = jump->nextJump;
				mxPushKind(XS_INTEGER_KIND);
				mxStack->value.integer = mxPtrDiff(jump->code - mxFrameFunction->value.reference->next->value.code.address);
				mxPushKind(XS_INTEGER_KIND);
				mxStack->value.integer = mxPtrDiff(slot - jump->scope);
				mxPushKind(XS_INTEGER_KIND);
				mxStack->value.integer = mxPtrDiff(slot - jump->frame);
				mxPushKind(XS_INTEGER_KIND);
				mxStack->value.integer = mxPtrDiff(slot - jump->stack);
				c_free(jump);
				jump = nextJump;
				offset++;
			}
			the->firstJump = yieldJump;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = offset;
			mxPushKind(mxFrameFunction->kind);
			mxStack->value = mxFrameFunction->value;
			index = mxPtrDiff(slot - mxStack);
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
			mxStack += 5 + (offset * 4);
			*mxFrameResult = *(mxStack++);
			slot = mxFrameResult;
			goto XS_CODE_END_ALL;
			
	/* FRAMES */		
		mxCase(XS_CODE_ARGUMENT)
			offset = mxRunU1(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, offset);
#endif
			if (offset < mxFrameArgc) {
				slot = mxFrameArgv(offset);
				mxPushKind(slot->kind);
				mxStack->value = slot->value;
			}
			else {
				mxPushKind(XS_UNDEFINED_KIND);
			}
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_ARGUMENTS)
			offset = mxRunU1(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, offset);
#endif
			mxSaveState;
			fxRunArguments(the, offset);
			mxRestoreState;
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_ARGUMENTS_SLOPPY)
			offset = mxRunU1(1);
			mxAllocStack(1);
			*mxStack = mxArgumentsSloppyPrototype;
			mxSaveState;
			gxDefaults.newArgumentsSloppyInstance(the, offset);
			mxRestoreState;
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_ARGUMENTS_STRICT)
			offset = mxRunU1(1);
			mxAllocStack(1);
			*mxStack = mxArgumentsStrictPrototype;
			mxSaveState;
			gxDefaults.newArgumentsStrictInstance(the, offset);
			mxRestoreState;
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_CURRENT)
			mxAllocStack(1);
			*mxStack = *mxFrameFunction;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_GET_RESULT)
			mxAllocStack(1);
			*mxStack = *mxFrameResult;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SET_RESULT)
			*mxFrameResult = *(mxStack++);
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_TARGET)
			mxAllocStack(1);
			*mxStack = *mxFrameTarget;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_THIS)
			mxAllocStack(1);
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
			mxAllocStack(1);
			*mxStack = mxException;
			mxException = mxUndefined;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_CATCH_4)
			offset = mxRunS4(1);
			mxNextCode(5);
			goto XS_CODE_CATCH;
		mxCase(XS_CODE_CATCH_2)
			offset = mxRunS2(1);
			mxNextCode(3);
			goto XS_CODE_CATCH;
		mxCase(XS_CODE_CATCH_1)
			offset = mxRunS1(1);
			mxNextCode(2);
		XS_CODE_CATCH:
			jump = c_malloc(sizeof(txJump));
			if (jump) {
				jump->nextJump = the->firstJump;
				jump->stack = mxStack;
				jump->scope = mxScope;
				jump->frame = mxFrame;
				jump->environment = mxEnvironment->value.reference;
				jump->code = mxCode + offset;
				jump->flag = 1;
                the->firstJump = jump;
				if (c_setjmp(jump->buffer) != 0) {
					jump = the->firstJump;
                    the->firstJump = jump->nextJump;
					mxStack = jump->stack;
					mxScope = jump->scope;
					mxFrame = jump->frame;
					mxEnvironment = mxFrameToEnvironment(mxFrame);
					mxEnvironment->value.reference = jump->environment;
					mxCode = jump->code;
					c_free(jump);
					mxFirstCode();
				}
			}
			else {
				mxSaveState;
				fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
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
			mxBranch(2, offset);
			mxBreak;
		mxCase(XS_CODE_BRANCH_2)
			offset = mxRunS2(1);
			mxBranch(3, offset);
			mxBreak;
		mxCase(XS_CODE_BRANCH_4)
			offset = mxRunS4(1);
			mxBranch(5, offset);
			mxBreak;
		mxCase(XS_CODE_BRANCH_CHAIN_4)
			offset = mxRunS4(1);
			index = 5;
			goto XS_CODE_BRANCH_CHAIN;
		mxCase(XS_CODE_BRANCH_CHAIN_2)
			offset = mxRunS2(1);
			index = 3;
			goto XS_CODE_BRANCH_CHAIN;
		mxCase(XS_CODE_BRANCH_CHAIN_1)
			offset = mxRunS1(1);
			index = 2;
		XS_CODE_BRANCH_CHAIN:
			byte = mxStack->kind;
			if ((XS_UNDEFINED_KIND == byte) || (XS_NULL_KIND == byte)) {
				mxStack->kind = XS_UNDEFINED_KIND;
				mxBranch(index, offset);
			}
			else
				mxNextCode((txS4)index);
			mxBreak;
		mxCase(XS_CODE_BRANCH_COALESCE_4)
			offset = mxRunS4(1);
			index = 5;
			goto XS_CODE_BRANCH_COALESCE;
		mxCase(XS_CODE_BRANCH_COALESCE_2)
			offset = mxRunS2(1);
			index = 3;
			goto XS_CODE_BRANCH_COALESCE;
		mxCase(XS_CODE_BRANCH_COALESCE_1)
			offset = mxRunS1(1);
			index = 2;
		XS_CODE_BRANCH_COALESCE:
			byte = mxStack->kind;
			if ((XS_UNDEFINED_KIND == byte) || (XS_NULL_KIND == byte)) {
				mxStack++;
				mxNextCode((txS4)index);
			}
			else {
				mxBranch(index, offset);
			}
			mxBreak;
		mxCase(XS_CODE_BRANCH_ELSE_4)
			offset = mxRunS4(1);
			index = 5;
			goto XS_CODE_BRANCH_ELSE;
		mxCase(XS_CODE_BRANCH_ELSE_2)
			offset = mxRunS2(1);
			index = 3;
			goto XS_CODE_BRANCH_ELSE;
		mxCase(XS_CODE_BRANCH_ELSE_1)
			offset = mxRunS1(1);
			index = 2;
		XS_CODE_BRANCH_ELSE:
			byte = mxStack->kind;
			if (XS_BOOLEAN_KIND == byte) {
				mxBranchElse(mxStack->value.boolean, index, offset);
			}
			else if (XS_INTEGER_KIND == byte) {
				mxBranchElse(mxStack->value.integer, index, offset);
			}
			else if (XS_NUMBER_KIND == byte) {
				mxBranchIf((c_isnan(mxStack->value.number) || c_iszero(mxStack->value.number)), index, offset);
				mxFloatingPointOp("else");
			}
			else if ((XS_BIGINT_KIND == byte) || (XS_BIGINT_X_KIND == byte)) {
				mxBranchIf(((mxStack->value.bigint.size == 1) && (mxStack->value.bigint.data[0] == 0)), index, offset);
			}
			else if ((XS_STRING_KIND == byte) || (XS_STRING_X_KIND == byte)) {
				mxBranchIf(c_isEmpty(mxStack->value.string), index, offset);
			}
			else {
				mxBranchIf((XS_UNDEFINED_KIND == byte) || (XS_NULL_KIND == byte), index, offset);
			}
			mxStack++;
			mxBreak;
		mxCase(XS_CODE_BRANCH_IF_4)
			offset = mxRunS4(1);
			index = 5;
			goto XS_CODE_BRANCH_IF;
		mxCase(XS_CODE_BRANCH_IF_2)
			offset = mxRunS2(1);
			index = 3;
			goto XS_CODE_BRANCH_IF;
		mxCase(XS_CODE_BRANCH_IF_1)
			offset = mxRunS1(1);
			index = 2;
		XS_CODE_BRANCH_IF:
			byte = mxStack->kind;
			if (XS_BOOLEAN_KIND == byte) {
				mxBranchIf(mxStack->value.boolean, index, offset);
			}
			else if (XS_INTEGER_KIND == byte) {
				mxBranchIf(mxStack->value.integer, index, offset);
			}
			else if (XS_NUMBER_KIND == byte) {
				mxBranchElse(c_isnan(mxStack->value.number) || c_iszero(mxStack->value.number), index, offset);
				mxFloatingPointOp("if");
			}
			else if ((XS_BIGINT_KIND == byte) || (XS_BIGINT_X_KIND == byte)) {
				mxBranchElse((mxStack->value.bigint.size == 1) && (mxStack->value.bigint.data[0] == 0), index, offset);
			}
			else if ((XS_STRING_KIND == byte) || (XS_STRING_X_KIND == byte)) {
				mxBranchElse(c_isEmpty(mxStack->value.string), index, offset);
			}
			else {
				mxBranchElse((XS_UNDEFINED_KIND == byte) || (XS_NULL_KIND == byte), index, offset);
			}
			mxStack++;
			mxBreak;
		mxCase(XS_CODE_BRANCH_STATUS_4)
			offset = mxRunS4(1);
			index = 5;
			goto XS_CODE_BRANCH_STATUS;
		mxCase(XS_CODE_BRANCH_STATUS_2)
			offset = mxRunS2(1);
			index = 3;
			goto XS_CODE_BRANCH_STATUS;
		mxCase(XS_CODE_BRANCH_STATUS_1)
			offset = mxRunS1(1);
			index = 2;
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
			mxAllocStack(1);
			*mxStack = *(mxStack + 1);
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_DUB_AT)
			mxAllocStack(2);
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
		mxCase(XS_CODE_CONST_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_CONST_CLOSURE;
		mxCase(XS_CODE_CONST_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_CONST_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			slot = mxEnvironment - index;
			variable = slot->value.closure;
			if (variable->kind >= 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: already initialized", slot->ID);
			slot->flag |= XS_DONT_SET_FLAG; //@@
			variable->flag |= XS_DONT_SET_FLAG;
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
		mxCase(XS_CODE_CONST_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_CONST_LOCAL;
		mxCase(XS_CODE_CONST_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_CONST_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			variable = mxEnvironment - index;
			if (variable->kind >= 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: already initialized", variable->ID);
			variable->flag |= XS_DONT_SET_FLAG;
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
						
		mxCase(XS_CODE_GET_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_GET_CLOSURE;
		mxCase(XS_CODE_GET_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_GET_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			slot = mxEnvironment - index;
			#ifdef mxDebug
				offset = slot->ID;
			#endif
			variable = slot->value.closure;
			if (variable->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "get %s: not initialized yet", slot->ID);
			offset = variable->ID;
			if (offset) {
				slot = the->aliasArray[offset];
				if (slot)
					variable = slot;
			}	
			mxPushKind(variable->kind);
			mxStack->value = variable->value;
			mxBreak;
		mxCase(XS_CODE_GET_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_GET_LOCAL;
		mxCase(XS_CODE_GET_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_GET_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			variable = mxEnvironment - index;
			#ifdef mxDebug
				offset = variable->ID;
			#endif
			if (variable->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "get %s: not initialized yet", variable->ID);
			mxPushKind(variable->kind);
			mxStack->value = variable->value;
			mxBreak;
			
		mxCase(XS_CODE_LET_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_LET_CLOSURE;
		mxCase(XS_CODE_LET_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_LET_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			slot = mxEnvironment - index;
			variable = slot->value.closure;
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
		mxCase(XS_CODE_LET_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_LET_LOCAL;
		mxCase(XS_CODE_LET_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_LET_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			variable = mxEnvironment - index;
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
			
		mxCase(XS_CODE_NEW_CLOSURE)
			offset = mxRunID(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, 0);
#endif
			mxNextCode(1 + sizeof(txID));
			slot = --mxScope;
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, mxEnvironment - mxScope - 1);
#endif
			mxSaveState;
			variable = fxNewSlot(the);
			mxRestoreState;
			slot->flag = XS_DONT_DELETE_FLAG;
			slot->ID = (txID)offset;
			slot->kind = XS_CLOSURE_KIND;
			slot->value.closure = variable;
			variable->kind = XS_UNINITIALIZED_KIND;
			mxBreak;
		mxCase(XS_CODE_NEW_LOCAL)
			offset = mxRunID(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, 0);
#endif
			mxNextCode(1 + sizeof(txID));
			variable = --mxScope;
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, mxEnvironment - mxScope - 1);
#endif
			variable->flag = XS_DONT_DELETE_FLAG;
			variable->ID = (txID)offset;
			variable->kind = XS_UNINITIALIZED_KIND;
			mxBreak;
		mxCase(XS_CODE_NEW_TEMPORARY)
			variable = --mxScope;
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, mxEnvironment - mxScope - 1);
#endif
			mxInitSlotKind(variable, XS_UNDEFINED_KIND);
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_PULL_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_PULL_CLOSURE;
		mxCase(XS_CODE_PULL_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_PULL_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			slot = mxEnvironment - index;
			if (slot->flag & XS_DONT_SET_FLAG) // import
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", slot->ID);
			variable = slot->value.closure;
			if (variable->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: not initialized yet", slot->ID);
			if (variable->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", slot->ID);
			offset = variable->ID;
			if (offset) {
				variable = the->aliasArray[offset];
				if (!variable) {
					mxSaveState;
					variable = fxNewSlot(the);
					mxRestoreState;
					the->aliasArray[offset] = variable;
				}
			}	
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxStack++;
			mxBreak;
		mxCase(XS_CODE_PULL_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_PULL_LOCAL;
		mxCase(XS_CODE_PULL_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_PULL_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			variable = mxEnvironment - index;
			if (variable->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: not initialized yet", variable->ID);
			if (variable->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", variable->ID);
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxStack++;
			mxBreak;
			
		mxCase(XS_CODE_REFRESH_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_REFRESH_CLOSURE;
		mxCase(XS_CODE_REFRESH_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_REFRESH_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			variable = mxEnvironment - index;
			mxSaveState;
			slot = fxNewSlot(the);
			mxRestoreState;
			slot->flag = variable->value.closure->flag;
			slot->kind = variable->value.closure->kind;
			slot->value = variable->value.closure->value;
			variable->value.closure = slot;
			mxBreak;
		mxCase(XS_CODE_REFRESH_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_REFRESH_LOCAL;
		mxCase(XS_CODE_REFRESH_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_REFRESH_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			variable = mxEnvironment - index;
			mxBreak;
			
		mxCase(XS_CODE_RESERVE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_RESERVE;
		mxCase(XS_CODE_RESERVE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_RESERVE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			mxAllocStack(index);		
			c_memset(mxStack, 0, index * sizeof(txSlot));
			mxBreak;
			
		mxCase(XS_CODE_RESET_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_RESET_CLOSURE;
		mxCase(XS_CODE_RESET_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_RESET_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			slot = mxEnvironment - index;
			mxSaveState;
			variable = fxNewSlot(the);
			mxRestoreState;
			variable->kind = XS_UNINITIALIZED_KIND;
			slot->value.closure = variable;
			mxBreak;
		mxCase(XS_CODE_RESET_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_RESET_LOCAL;
		mxCase(XS_CODE_RESET_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_RESET_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			variable = mxEnvironment - index;
			variable->flag = XS_NO_FLAG;
			variable->kind = XS_UNINITIALIZED_KIND;
			mxBreak;
			
		mxCase(XS_CODE_SET_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_SET_CLOSURE;
		mxCase(XS_CODE_SET_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_SET_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			slot = mxEnvironment - index;
			if (slot->flag & XS_DONT_SET_FLAG) // import
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", slot->ID);
			variable = slot->value.closure;
			if (variable->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: not initialized yet", slot->ID);
			if (variable->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", slot->ID);
			offset = variable->ID;
			if (offset > 0) {
				variable = the->aliasArray[offset];
				if (!variable) {
					mxSaveState;
					variable = fxNewSlot(the);
					mxRestoreState;
					the->aliasArray[offset] = variable;
				}
			}	
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
		mxCase(XS_CODE_SET_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_SET_LOCAL;
		mxCase(XS_CODE_SET_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_SET_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			variable = mxEnvironment - index;
			if (variable->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: not initialized yet", variable->ID);
			if (variable->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", variable->ID);
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;

		mxCase(XS_CODE_VAR_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_VAR_CLOSURE;
		mxCase(XS_CODE_VAR_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_VAR_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			variable = (mxEnvironment - index)->value.closure;
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
		mxCase(XS_CODE_VAR_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_VAR_LOCAL;
		mxCase(XS_CODE_VAR_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_VAR_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			variable = mxEnvironment - index;
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
			
		mxCase(XS_CODE_UNWIND_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_UNWIND;
		mxCase(XS_CODE_UNWIND_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_UNWIND:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			slot = mxScope;
			mxScope += index;
			while (slot < mxScope)
				(slot++)->kind = XS_UNDEFINED_KIND;
			mxBreak;
			
		mxCase(XS_CODE_RETRIEVE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_RETRIEVE;
		mxCase(XS_CODE_RETRIEVE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_RETRIEVE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			slot = mxEnvironment->value.reference->next->next;
			variable = mxScope;
			while (index) {
				--variable;
				offset = variable->ID = slot->ID;
                variable->flag = slot->flag;
                variable->kind = slot->kind;
				variable->value = slot->value;
#ifdef mxTrace
				if (gxDoTrace) fxTraceID(the, (txID)offset, 0);
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
			mxPushKind(XS_UNDEFINED_KIND);
			mxSaveState;
			slot = fxNewEnvironmentInstance(the, C_NULL);
			mxRestoreState;
			variable = mxFunctionInstanceCode((mxStack + 1)->value.reference);
			variable->value.code.closures = slot;
			mxNextCode(1);
			mxBreak;

		mxCase(XS_CODE_STORE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_STORE;
		mxCase(XS_CODE_STORE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_STORE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			address = &(mxStack->value.reference->next);
			while ((slot = *address)) {
				address = &slot->next;
			}
			mxSaveState;
			*address = slot = fxNewSlot(the);
			mxRestoreState;
			variable = mxEnvironment - index;
			offset = slot->ID = variable->ID;
            slot->flag = variable->flag;
            slot->kind = variable->kind;
			slot->value = variable->value;
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, 0);
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
			if (mxStack->kind == XS_REFERENCE_KIND) {
				mxSaveState;
				fxToPrimitive(the, mxStack, XS_STRING_HINT);
				mxRestoreState;
			}
			if ((mxStack->kind == XS_INTEGER_KIND) && fxIntegerToIndex(the->dtoa, mxStack->value.integer, &(scratch.value.at.index))) {
				mxStack->kind = XS_AT_KIND;
				mxStack->value.at.id = XS_NO_ID;
				mxStack->value.at.index = scratch.value.at.index;
			}
			else if ((mxStack->kind == XS_NUMBER_KIND) && fxNumberToIndex(the->dtoa, mxStack->value.number, &(scratch.value.at.index))) {
				mxStack->kind = XS_AT_KIND;
				mxStack->value.at.id = XS_NO_ID;
				mxStack->value.at.index = scratch.value.at.index;
			}
			else if (mxStack->kind == XS_SYMBOL_KIND) {
				mxStack->kind = XS_AT_KIND;
				mxStack->value.at.id = mxStack->value.symbol;
				mxStack->value.at.index = 0;
			}
			else {
				txFlag flag;

				mxToString(mxStack);
				mxSaveState;
				flag = fxStringToIndex(the->dtoa, mxStack->value.string, &(scratch.value.at.index));
				mxRestoreState;
				if (flag) {
#ifdef mxMetering
					the->meterIndex += 2;
#endif
					mxStack->kind = XS_AT_KIND;
					mxStack->value.at.id = XS_NO_ID;
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
					mxStack->value.at.index = 0;
				}
			}
			mxNextCode(1);
			mxBreak;

		mxCase(XS_CODE_DELETE_SUPER_AT)
			variable = (mxStack + 1)->value.reference;
			offset = mxStack->value.at.id;
			index = mxStack->value.at.index;
			mxStack++;
			mxNextCode(1);
			goto XS_CODE_DELETE_SUPER_ALL;
		mxCase(XS_CODE_DELETE_SUPER)
			mxToInstance(mxStack);
			offset = mxRunID(1);
			index = 0;
			mxNextCode(1 + sizeof(txID));
			/* continue */
		XS_CODE_DELETE_SUPER_ALL:	
			mxRunDebugID(XS_REFERENCE_ERROR, "delete super.%s", (txID)offset);
			mxBreak;

		mxCase(XS_CODE_DELETE_PROPERTY_AT)
			variable = (mxStack + 1)->value.reference;
			offset = mxStack->value.at.id;
			index = mxStack->value.at.index;
			mxStack++;
			mxNextCode(1);
			goto XS_CODE_DELETE_PROPERTY_ALL;
		mxCase(XS_CODE_DELETE_PROPERTY)
			mxToInstance(mxStack);
			offset = mxRunID(1);
			index = 0;
			mxNextCode(1 + sizeof(txID));
			/* continue */
		XS_CODE_DELETE_PROPERTY_ALL:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, index);
#endif
			mxSaveState;
			index = (txU4)fxRunDelete(the, variable, (txID)offset, index);
			mxRestoreState;
			if (!index && (mxFrame->flag & XS_STRICT_FLAG))
				mxRunDebugID(XS_TYPE_ERROR, "delete %s: no permission (strict mode)", (txID)offset);
			mxStack->kind = XS_BOOLEAN_KIND;
			mxStack->value.boolean = index;
			mxBreak;
			
		mxCase(XS_CODE_GET_THIS_VARIABLE)
		mxCase(XS_CODE_GET_VARIABLE)
			mxToInstance(mxStack);
			offset = mxRunID(1);
			index = 0;
			mxNextCode(1 + sizeof(txID));
			slot = mxBehaviorGetProperty(the, variable, (txID)offset, index, XS_ANY);
			if (slot) {
				if (slot->kind < 0)
					mxRunDebugID(XS_REFERENCE_ERROR, "get %s: not initialized yet", (txID)offset);
			}
			else {
				if (byte != XS_CODE_TYPEOF)
					mxRunDebugID(XS_REFERENCE_ERROR, "get %s: undefined variable", (txID)offset);
			}
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
			offset = mxRunID(1);
			index = 0;
			mxNextCode(1 + sizeof(txID));
			/* continue */
		XS_CODE_GET_SUPER_ALL:	
			slot = mxFunctionInstanceHome(mxFrameFunction->value.reference);
			if (!slot->value.home.object)
				mxRunDebugID(XS_TYPE_ERROR, "get super %s: no home", (txID)offset);
			slot = fxGetPrototype(the, slot->value.home.object);
			if (!slot)
				mxRunDebugID(XS_TYPE_ERROR, "get super %s: no prototype", (txID)offset);
			slot = mxBehaviorGetProperty(the, slot, (txID)offset, index, XS_ANY);
			goto XS_CODE_GET_ALL;
		mxCase(XS_CODE_GET_PRIVATE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_GET_PRIVATE;
		mxCase(XS_CODE_GET_PRIVATE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_GET_PRIVATE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			slot = (mxEnvironment - index);
			mxToInstance(mxStack);
			offset = slot->ID;
			index = 0;
			if (slot->value.closure->kind < 0)
				mxRunDebugID(XS_TYPE_ERROR, "get %s: undefined private property", (txID)offset);
			slot = gxDefaults.getPrivateProperty(the, variable, slot->value.closure->value.reference, (txID)offset);
			if (!slot)
				mxRunDebugID(XS_TYPE_ERROR, "get %s: undefined private property", (txID)offset);
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
			offset = mxRunID(1);
			index = 0;
			mxNextCode(1 + sizeof(txID));
			/* continue */
		XS_CODE_GET_PROPERTY_ALL:	
			slot = mxBehaviorGetProperty(the, variable, (txID)offset, index, XS_ANY);
		XS_CODE_GET_ALL:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, index);
#endif
			if (!slot) {
				mxStack->kind = XS_UNDEFINED_KIND;
				mxBreak;
			}
			if (slot->kind == XS_ACCESSOR_KIND) {
				variable = slot->value.accessor.getter;
				if (!mxIsFunction(variable)) {
					mxStack->kind = XS_UNDEFINED_KIND;
					mxBreak;
				}
				mxAllocStack(5);
				slot = mxStack;
				mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
				mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
				mxInitSlotKind(slot++, XS_UNDEFINED_KIND);
				mxInitSlotKind(slot++, XS_UNDEFINED_KIND);
				slot->value.reference = variable;
				mxInitSlotKind(slot++, XS_REFERENCE_KIND);
				if (primitive) {
					variable = slot->value.reference->next;
					slot->value = variable->value;
					mxInitSlotKind(slot, variable->kind);
				}
				offset = 0;
				goto XS_CODE_RUN_ALL;
			}
			mxStack->kind = slot->kind;
			mxStack->value = slot->value;
			mxBreak;
			
		mxCase(XS_CODE_NEW_PRIVATE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_NEW_PRIVATE;
		mxCase(XS_CODE_NEW_PRIVATE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_NEW_PRIVATE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			slot = (mxEnvironment - index);
			mxToInstance(mxStack + 1);
			offset = slot->ID;
			index = 0;
			slot = slot->value.closure->value.reference;
			goto XS_CODE_NEW_PROPERTY_ALL;
		mxCase(XS_CODE_NEW_PROPERTY_AT)
			mxToInstance(mxStack + 2);
			offset = (mxStack + 1)->value.at.id;
			index = (mxStack + 1)->value.at.index;
			slot = C_NULL;
			*(mxStack + 1) = *mxStack;
			mxStack++;
			mxNextCode(1);
			goto XS_CODE_NEW_PROPERTY_ALL;
		mxCase(XS_CODE_NEW_PROPERTY)
			mxToInstance(mxStack + 1);
			offset = mxRunID(1);
			index = 0;
			slot = C_NULL;
			mxNextCode(1 + sizeof(txID));
		XS_CODE_NEW_PROPERTY_ALL:
			byte = mxRunU1(1);
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
			index = fxRunDefine(the, variable, slot, (txID)offset, index, mxStack, byte | XS_GET_ONLY);
			mxRestoreState;
			mxStack += 2;
			if (!index)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", (txID)offset);
			mxNextCode(2);
			mxBreak;
			
		mxCase(XS_CODE_SET_VARIABLE)
			mxToInstance(mxStack + 1);
			offset = mxRunID(1);
			index = 0;
			mxNextCode(1 + sizeof(txID));
			if (mxFrame->flag & XS_STRICT_FLAG) {
            	mxSaveState;
				if (!fxRunHas(the, variable, (txID)offset, index))
					mxRunDebugID(XS_REFERENCE_ERROR, "set %s: undefined variable", (txID)offset);
				mxRestoreState;
			}
            mxSaveState;
			slot = mxBehaviorSetProperty(the, variable, (txID)offset, index, XS_ANY);
			mxRestoreState;
			if (slot && (slot->kind < 0))
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: not initialized yet", (txID)offset);
			goto XS_CODE_SET_ALL;
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
			offset = mxRunID(1);
			index = 0;
			mxNextCode(1 + sizeof(txID));
			/* continue */
		XS_CODE_SET_SUPER_ALL:
			slot = mxFunctionInstanceHome(mxFrameFunction->value.reference);
			if (!slot->value.home.object)
				mxRunDebugID(XS_TYPE_ERROR, "set super %s: no home", (txID)offset);
			slot = fxGetPrototype(the, slot->value.home.object);
			if (!slot)
				mxRunDebugID(XS_TYPE_ERROR, "set super %s: no prototype", (txID)offset);
			slot = mxBehaviorGetProperty(the, slot, (txID)offset, index, XS_ANY);
			if (!slot || (slot->kind != XS_ACCESSOR_KIND)) {
				mxSaveState;
				slot = mxBehaviorSetProperty(the, variable, (txID)offset, index, XS_OWN);
				mxRestoreState;
			}
			goto XS_CODE_SET_ALL;
		mxCase(XS_CODE_SET_PRIVATE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_SET_PRIVATE;
		mxCase(XS_CODE_SET_PRIVATE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_SET_PRIVATE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			slot = (mxEnvironment - index);
			mxToInstance(mxStack + 1);
			offset = slot->ID;
			index = 0;
			if (slot->value.closure->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "set %s: undefined private property", (txID)offset);
			slot = gxDefaults.setPrivateProperty(the, variable, slot->value.closure->value.reference, (txID)offset);
			if (!slot)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: undefined private property", (txID)offset);
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
			offset = mxRunID(1);
			index = 0;
			mxNextCode(1 + sizeof(txID));
			/* continue */
		XS_CODE_SET_PROPERTY_ALL:	
			mxSaveState;
			slot = mxBehaviorSetProperty(the, variable, (txID)offset, index, XS_ANY);
			mxRestoreState;
		XS_CODE_SET_ALL:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, index);
#endif
			if (!slot) {
				if (mxFrame->flag & XS_STRICT_FLAG) {
					mxRunDebugID(XS_TYPE_ERROR, "set %s: not extensible", (txID)offset);
				}
				goto XS_CODE_SET_SKIP;
			}
			if (slot->kind == XS_ACCESSOR_KIND) {
				variable = slot->value.accessor.setter;
				if (!mxIsFunction(variable)) {
					if (mxFrame->flag & XS_STRICT_FLAG) {
						mxRunDebugID(XS_TYPE_ERROR, "set %s: no setter", (txID)offset);
					}
					goto XS_CODE_SET_SKIP;
				}
				slot = mxStack;
				mxAllocStack(5);
				mxStack->value = slot->value;
				mxInitSlotKind(mxStack, slot->kind);
				slot = mxStack + 1;
				mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
				mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
				slot->value = mxStack->value;
				mxInitSlotKind(slot++, mxStack->kind);
				mxInitSlotKind(slot++, XS_UNDEFINED_KIND);
				slot->value.reference = variable;
				mxInitSlotKind(slot++, XS_REFERENCE_KIND);
				offset = 1;
				goto XS_CODE_RUN_ALL;
			}
			if (slot->flag & (XS_DONT_SET_FLAG | XS_MARK_FLAG)) {
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
			
		mxCase(XS_CODE_HAS_PRIVATE_2)
			index = mxRunU2(1);
			mxNextCode(3);
			goto XS_CODE_HAS_PRIVATE;
		mxCase(XS_CODE_HAS_PRIVATE_1)
			index = mxRunU1(1);
			mxNextCode(2);
		XS_CODE_HAS_PRIVATE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 1);
#endif
			slot = (mxEnvironment - index);
			if (mxStack->kind == XS_REFERENCE_KIND)
				variable = mxStack->value.reference; 
			else
				mxRunDebug(XS_TYPE_ERROR, "in: no instance");
			offset = slot->ID;
			index = 0;
			if (slot->value.closure->kind < 0)
				mxRunDebugID(XS_TYPE_ERROR, "get %s: undefined private property", (txID)offset);
			slot = gxDefaults.getPrivateProperty(the, variable, slot->value.closure->value.reference, (txID)offset);
			mxStack->kind = XS_BOOLEAN_KIND;
			mxStack->value.boolean = (slot) ? 1 : 0;
			mxBreak;
						
	/* INSTANCES */	
		mxCase(XS_CODE_ARRAY)
// 			mxAllocStack(1);
			mxSaveState;
			fxNewArray(the, 0);
			mxRestoreState;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_CLASS)
			variable = fxToInstance(the, mxStack);
			slot = mxStack + 2;
			variable->flag |= XS_CAN_CONSTRUCT_FLAG;
			if (slot->kind == XS_NULL_KIND)
				variable->next->flag |= XS_BASE_FLAG;
			else {
				variable->next->flag |= XS_DERIVED_FLAG;
				variable->value.instance.prototype = slot->value.reference;
			}
			slot = mxStack + 1;
			mxFunctionInstanceHome(variable)->value.home.object = slot->value.reference;
			mxSaveState;
			slot->flag = XS_GET_ONLY;
			fxRunDefine(the, variable, C_NULL, mxID(_prototype), 0, slot, XS_GET_ONLY);
			slot = mxBehaviorSetProperty(the, slot->value.reference, mxID(_constructor), 0, XS_OWN);
			mxRestoreState;
			slot->flag |= XS_DONT_ENUM_FLAG;
			slot->kind = mxStack->kind;
			slot->value = mxStack->value;
			mxStack += 3;
            mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_COPY_OBJECT)
			mxNextCode(1);
			mxAllocStack(1);
			*mxStack = mxCopyObjectFunction;
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
			variable = mxFunctionInstanceHome(mxFrameFunction->value.reference)->value.home.module;
			variable = mxModuleInstanceInternal(variable)->value.module.realm;
			if (!variable) variable = mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm;
			mxStack->value.reference = mxRealmGlobal(variable)->value.reference;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_HOST)
			index = mxRunU2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			mxAllocStack(1);
			*mxStack = mxHosts;
			slot = mxBehaviorGetProperty(the, mxStack->value.reference, 0, index, XS_OWN);
			mxStack->kind = slot->kind;
			mxStack->value = slot->value;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_INSTANTIATE)
			mxSaveState;
			fxRunInstantiate(the);
			mxRestoreState;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_CALL)
			mxNextCode(1);
			mxAllocStack(4);
			slot = mxStack;
			mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
			mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
			mxInitSlotKind(slot++, XS_UNDEFINED_KIND);
			mxInitSlotKind(slot, XS_UNDEFINED_KIND);
			mxBreak;
		mxCase(XS_CODE_NEW)
			mxNextCode(1);
			variable = mxStack;
			mxAllocStack(5);
			slot = mxStack;
			mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
			mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
			mxInitSlotKind(slot++, XS_UNDEFINED_KIND);
			slot->value = variable->value;
			mxInitSlotKind(slot++, variable->kind);
			slot->value = variable->value;
			mxInitSlotKind(slot++, variable->kind);
			mxInitSlotKind(slot, XS_UNINITIALIZED_KIND);
			mxBreak;
		mxCase(XS_CODE_OBJECT)
// 			mxAllocStack(1);
			mxSaveState;
			fxNewObject(the);
			mxRestoreState;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_REGEXP)
			mxNextCode(1);
			mxAllocStack(1);
			*mxStack = mxRegExpConstructor;
			mxBreak;
		mxCase(XS_CODE_SUPER)
			mxNextCode(1);
			variable = mxFrameFunction->value.reference;
            if (mxIsConstructor(variable))
				variable = fxGetPrototype(the, variable);
			else {
				variable = mxFunctionInstanceHome(variable);
				variable = mxBehaviorGetProperty(the, variable->value.home.object, mxID(_constructor), 0, XS_ANY);
				variable = fxGetPrototype(the, variable->value.reference);
			}
            if (!mxIsConstructor(variable))
				mxRunDebug(XS_TYPE_ERROR, "super: no constructor");
			mxAllocStack(6);
			slot = mxStack;
			mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
			mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
			mxInitSlotKind(slot++, XS_UNDEFINED_KIND);
			slot->value = mxFrameTarget->value;
			mxInitSlotKind(slot++, mxFrameTarget->kind);
			slot->value.reference = variable;
			mxInitSlotKind(slot++, XS_REFERENCE_KIND);
			mxInitSlotKind(slot, XS_UNINITIALIZED_KIND);
			mxBreak;
		mxCase(XS_CODE_TEMPLATE)
			mxNextCode(1);
			variable = mxStack->value.reference;
			slot = mxBehaviorGetProperty(the, variable, mxID(_raw), 0, XS_OWN);
            if (!slot)
				mxRunDebug(XS_TYPE_ERROR, "template: no raw");
			variable->flag |= XS_DONT_PATCH_FLAG;
			variable->next->flag |= XS_DONT_SET_FLAG;
			slot->flag |= XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
			variable = slot->value.reference;
			variable->flag |= XS_DONT_PATCH_FLAG;
			variable->next->flag |= XS_DONT_SET_FLAG;
			mxBreak;
		mxCase(XS_CODE_TEMPLATE_CACHE)
			mxNextCode(1);
            variable = mxFunctionInstanceHome(mxFrameFunction->value.reference)->value.home.module;
            variable = mxModuleInstanceInternal(variable)->value.module.realm;
            if (!variable) variable = mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm;
            slot = mxRealmTemplateCache(variable);
			mxPushKind(XS_REFERENCE_KIND);
			mxStack->value.reference = slot->value.reference;
			mxBreak;
			
	/* FUNCTIONS */		
		mxCase(XS_CODE_ASYNC_FUNCTION)
			offset = mxRunID(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, 0);
#endif
			mxAllocStack(1);
			*mxStack = mxAsyncFunctionPrototype;
			mxSaveState;
			fxNewFunctionInstance(the, (txID)offset);
			mxRestoreState;
			mxNextCode(1 + sizeof(txID));
			mxBreak;
		mxCase(XS_CODE_ASYNC_GENERATOR_FUNCTION)
			offset = mxRunID(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, 0);
#endif
			mxAllocStack(1);
			*mxStack = mxAsyncGeneratorFunctionPrototype;
			mxSaveState;
			gxDefaults.newAsyncGeneratorFunctionInstance(the,(txID) offset);
			mxRestoreState;
			mxNextCode(1 + sizeof(txID));
			mxBreak;
		mxCase(XS_CODE_CONSTRUCTOR_FUNCTION)
			offset = mxRunID(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, 0);
#endif
			mxAllocStack(1);
			*mxStack = mxFunctionPrototype;
			mxSaveState;
			fxNewFunctionInstance(the, (txID)offset);
			fxDefaultFunctionPrototype(the);
			mxRestoreState;
			mxNextCode(1 + sizeof(txID));
			mxBreak;
		mxCase(XS_CODE_FUNCTION)
			offset = mxRunID(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, 0);
#endif
			mxAllocStack(1);
			*mxStack = mxFunctionPrototype;
			mxSaveState;
			fxNewFunctionInstance(the, (txID)offset);
			mxRestoreState;
			mxNextCode(1 + sizeof(txID));
			mxBreak;
		mxCase(XS_CODE_GENERATOR_FUNCTION)
			offset = mxRunID(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, 0);
#endif
			mxAllocStack(1);
			*mxStack = mxGeneratorFunctionPrototype;
			mxSaveState;
			gxDefaults.newGeneratorFunctionInstance(the,(txID) offset);
			mxRestoreState;
			mxNextCode(1 + sizeof(txID));
			mxBreak;
		mxCase(XS_CODE_PROFILE)
			offset = mxRunID(1);
			variable = mxFunctionInstanceHome(mxStack->value.reference);
			variable->ID = offset;
			mxNextCode(1 + sizeof(txID));
			mxBreak;
		mxCase(XS_CODE_NAME)
			offset = mxRunID(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, 0);
#endif
			mxSaveState;
			fxRenameFunction(the, mxStack->value.reference, (txID)offset, 0, XS_NO_ID, C_NULL);
			mxRestoreState;
			mxNextCode(1 + sizeof(txID));
			mxBreak;
		mxCase(XS_CODE_SET_HOME)
			slot = mxFunctionInstanceHome((mxStack + 1)->value.reference);
			slot->value.home.object = mxStack->value.reference;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_CODE_4)
			offset = mxRunS4(1);
			mxSkipCode(5);
			goto XS_CODE_CODE;
		mxCase(XS_CODE_CODE_2)
			offset = mxRunS2(1);
			mxSkipCode(3);
			goto XS_CODE_CODE;
		mxCase(XS_CODE_CODE_1)
			offset = mxRunS1(1);
			mxSkipCode(2);
		XS_CODE_CODE:
			mxSaveState;
			scratch.value.code.address = (txByte*)fxNewChunk(the, (txSize)offset);
			mxRestoreState;
			c_memcpy(scratch.value.code.address, mxCode, offset);
			variable = mxStack->value.reference;
			slot = mxFunctionInstanceCode(variable);
			slot->kind = XS_CODE_KIND;
			slot->value.code.address = scratch.value.code.address;
			if (gxDefaults.newFunctionLength) {
				gxDefaults.newFunctionLength(the, variable, *(((txU1*)scratch.value.code.address + 1)));
			}
			mxNextCode(offset);
			mxBreak;
		mxCase(XS_CODE_CODE_ARCHIVE_4)
			offset = mxRunS4(1);
			mxSkipCode(5);
			goto XS_CODE_CODE_ARCHIVE;
		mxCase(XS_CODE_CODE_ARCHIVE_2)
			offset = mxRunS2(1);
			mxSkipCode(3);
			goto XS_CODE_CODE_ARCHIVE;
		mxCase(XS_CODE_CODE_ARCHIVE_1)
			offset = mxRunS1(1);
			mxSkipCode(2);
		XS_CODE_CODE_ARCHIVE:
			variable = mxStack->value.reference;
			slot = mxFunctionInstanceCode(variable);
			slot->kind = XS_CODE_X_KIND;
			slot->value.code.address = mxCode;
			if (gxDefaults.newFunctionLength) {
				gxDefaults.newFunctionLength(the, variable, mxRunU1(1));
			}
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
			{
				txByte* number = (txByte*)&(mxStack->value.number);
			#if mxBigEndian
				number[7] = mxCode[1];
				number[6] = mxCode[2];
				number[5] = mxCode[3];
				number[4] = mxCode[4];
				number[3] = mxCode[5];
				number[2] = mxCode[6];
				number[1] = mxCode[7];
				number[0] = mxCode[8];
			#else
				number[0] = mxCode[1];
				number[1] = mxCode[2];
				number[2] = mxCode[3];
				number[3] = mxCode[4];
				number[4] = mxCode[5];
				number[5] = mxCode[6];
				number[6] = mxCode[7];
				number[7] = mxCode[8];
			#endif
			}
			mxNextCode(9);
#ifdef mxTrace
			if (gxDoTrace) fxTraceNumber(the, mxStack->value.number);
#endif
			mxBreak;
			
		mxCase(XS_CODE_STRING_4)
			index = mxRunS4(1);
			mxSkipCode(5);
			goto XS_CODE_STRING;
		mxCase(XS_CODE_STRING_2)
			index = mxRunU2(1);
			mxSkipCode(3);
			goto XS_CODE_STRING;
		mxCase(XS_CODE_STRING_1)
			index = mxRunU1(1);
			mxSkipCode(2);
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
		mxCase(XS_CODE_STRING_ARCHIVE_4)
			index = mxRunS4(1);
			mxSkipCode(5);
			goto XS_CODE_STRING_ARCHIVE;
		mxCase(XS_CODE_STRING_ARCHIVE_2)
			index = mxRunU2(1);
			mxSkipCode(3);
			goto XS_CODE_STRING_ARCHIVE;
		mxCase(XS_CODE_STRING_ARCHIVE_1)
			index = mxRunU1(1);
			mxSkipCode(2);
		XS_CODE_STRING_ARCHIVE:
			mxPushKind(XS_STRING_X_KIND);
			mxStack->value.string = (txString)mxCode;
			mxNextCode(index);
#ifdef mxTrace
			if (gxDoTrace) fxTraceString(the, mxStack->value.string);
#endif
			mxBreak;
		mxCase(XS_CODE_TO_STRING)
			mxToString(mxStack);
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_SYMBOL)
			mxPushKind(XS_SYMBOL_KIND);
			mxStack->value.symbol = mxRunID(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, mxStack->value.symbol, 0);
#endif
			mxNextCode(1 + sizeof(txID));
			mxBreak;
			
		mxCase(XS_CODE_BIGINT_2)
			index = mxRunU2(1);
			mxSkipCode(3);
			goto XS_CODE_BIGINT;
		mxCase(XS_CODE_BIGINT_1)
			index = mxRunU1(1);
			mxSkipCode(2);
		XS_CODE_BIGINT:
			mxSaveState;
			gxTypeBigInt.decode(the, index);
			mxRestoreState;
			mxNextCode(index);
			mxBreak;

	/* EXPRESSIONS */ 
		mxCase(XS_CODE_BIT_NOT)
			if (mxStack->kind == XS_INTEGER_KIND)
				mxStack->value.integer = ~mxStack->value.integer;
			else if (mxStack->kind == XS_NUMBER_KIND) {
				mxStack->kind = XS_INTEGER_KIND;
				mxStack->value.integer = ~fxNumberToInteger(mxStack->value.number);
				mxFloatingPointOp("not");
			}
			else {
				mxSaveState;
				if (fxToNumericIntegerUnary(the, mxStack, gxTypeBigInt._not))
					mxStack->value.integer = ~mxStack->value.integer;
				mxRestoreState;
			}
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_BIT_AND)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND)
					slot->value.integer &= mxStack->value.integer;
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->value.integer &= fxNumberToInteger(mxStack->value.number);
					mxFloatingPointOp("and");
				}
				else
					goto XS_CODE_BIT_AND_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					slot->kind = XS_INTEGER_KIND;
					slot->value.integer = fxNumberToInteger(slot->value.number) & mxStack->value.integer;
					mxFloatingPointOp("and");
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->kind = XS_INTEGER_KIND;
					slot->value.integer = fxNumberToInteger(slot->value.number) & fxNumberToInteger(mxStack->value.number);
					mxFloatingPointOp("and");
				}
				else
					goto XS_CODE_BIT_AND_GENERAL;
			}
			else {
		XS_CODE_BIT_AND_GENERAL:
				mxSaveState;
				if (fxToNumericIntegerBinary(the, slot, mxStack, gxTypeBigInt._and))
					slot->value.integer &= mxStack->value.integer;
				mxRestoreState;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_BIT_OR)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND)
					slot->value.integer |= mxStack->value.integer;
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->value.integer |= fxNumberToInteger(mxStack->value.number);
					mxFloatingPointOp("or");
				}
				else
					goto XS_CODE_BIT_OR_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					slot->kind = XS_INTEGER_KIND;
					slot->value.integer = fxNumberToInteger(slot->value.number) | mxStack->value.integer;
					mxFloatingPointOp("or");
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->kind = XS_INTEGER_KIND;
					slot->value.integer = fxNumberToInteger(slot->value.number) | fxNumberToInteger(mxStack->value.number);
					mxFloatingPointOp("or");
				}
				else
					goto XS_CODE_BIT_OR_GENERAL;
			}
			else {
		XS_CODE_BIT_OR_GENERAL:
				mxSaveState;
				if (fxToNumericIntegerBinary(the, slot, mxStack, gxTypeBigInt._or))
					slot->value.integer |= mxStack->value.integer;
				mxRestoreState;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_BIT_XOR)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND)
					slot->value.integer ^= mxStack->value.integer;
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->value.integer ^= fxNumberToInteger(mxStack->value.number);
					mxFloatingPointOp("xor");
				}
				else
					goto XS_CODE_BIT_XOR_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					slot->kind = XS_INTEGER_KIND;
					slot->value.integer = fxNumberToInteger(slot->value.number) ^ mxStack->value.integer;
					mxFloatingPointOp("xor");
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->kind = XS_INTEGER_KIND;
					slot->value.integer = fxNumberToInteger(slot->value.number) ^ fxNumberToInteger(mxStack->value.number);
					mxFloatingPointOp("xor");
				}
				else
					goto XS_CODE_BIT_XOR_GENERAL;
			}
			else {
		XS_CODE_BIT_XOR_GENERAL:
				mxSaveState;
				if (fxToNumericIntegerBinary(the, slot, mxStack, gxTypeBigInt._xor))
					slot->value.integer ^= mxStack->value.integer;
				mxRestoreState;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_LEFT_SHIFT)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND)
					slot->value.integer <<= mxStack->value.integer & 0x1f;
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->value.integer <<= fxNumberToInteger(mxStack->value.number) & 0x1f;
					mxFloatingPointOp("left shift");
				}
				else
					goto XS_CODE_LEFT_SHIFT_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					slot->kind = XS_INTEGER_KIND;
					slot->value.integer = fxNumberToInteger(slot->value.number) << (mxStack->value.integer & 0x1f);
					mxFloatingPointOp("left shift");
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->kind = XS_INTEGER_KIND;
					slot->value.integer = fxNumberToInteger(slot->value.number) << (fxNumberToInteger(mxStack->value.number) & 0x1f);
					mxFloatingPointOp("left shift");
				}
				else
					goto XS_CODE_LEFT_SHIFT_GENERAL;
			}
			else {
		XS_CODE_LEFT_SHIFT_GENERAL:
				mxSaveState;
				if (fxToNumericIntegerBinary(the, slot, mxStack, gxTypeBigInt._lsl))
					slot->value.integer <<= mxStack->value.integer & 0x1f;
				mxRestoreState;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SIGNED_RIGHT_SHIFT)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND)
					slot->value.integer >>= mxStack->value.integer & 0x1f;
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->value.integer >>= fxNumberToInteger(mxStack->value.number) & 0x1f;
					mxFloatingPointOp("signed right shift");
				}
				else
					goto XS_CODE_SIGNED_RIGHT_SHIFT_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					slot->kind = XS_INTEGER_KIND;
					slot->value.integer = fxNumberToInteger(slot->value.number) >> (mxStack->value.integer & 0x1f);
					mxFloatingPointOp("signed right shift");
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->kind = XS_INTEGER_KIND;
					slot->value.integer = fxNumberToInteger(slot->value.number) >> (fxNumberToInteger(mxStack->value.number) & 0x1f);
					mxFloatingPointOp("signed right shift");
				}
				else
					goto XS_CODE_SIGNED_RIGHT_SHIFT_GENERAL;
			}
			else {
		XS_CODE_SIGNED_RIGHT_SHIFT_GENERAL:
				mxSaveState;
				if (fxToNumericIntegerBinary(the, slot, mxStack, gxTypeBigInt._lsr))
					slot->value.integer >>= mxStack->value.integer & 0x1f;
				mxRestoreState;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_UNSIGNED_RIGHT_SHIFT)
			slot = mxStack + 1;
			if (((slot->kind == XS_INTEGER_KIND) || (slot->kind == XS_NUMBER_KIND)) && ((mxStack->kind == XS_INTEGER_KIND) || (mxStack->kind == XS_NUMBER_KIND))) {
				fxUnsigned(the, slot, fxToUnsigned(the, slot) >> (fxToUnsigned(the, mxStack) & 0x1F));
			}
			else {
				mxSaveState;
				if (fxToNumericNumberBinary(the, slot, mxStack, gxTypeBigInt._nop))
					fxUnsigned(the, slot, fxToUnsigned(the, slot) >> (fxToUnsigned(the, mxStack) & 0x1F));
				mxRestoreState;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
				
		mxCase(XS_CODE_MINUS)
			if (mxStack->kind == XS_INTEGER_KIND) {
				if (mxStack->value.integer & 0x7FFFFFFF)
					mxStack->value.integer = -mxStack->value.integer;
				else {
					mxStack->kind = XS_NUMBER_KIND;
					mxStack->value.number = -((txNumber)(mxStack->value.integer));
					mxFloatingPointOp("minus");
				}
			}
			else if (mxStack->kind == XS_NUMBER_KIND) {
				mxStack->value.number = -mxStack->value.number;
				mxFloatingPointOp("minus");
			}
			else {
				mxSaveState;
				if (fxToNumericNumberUnary(the, mxStack, gxTypeBigInt._neg)) {
					mxStack->value.number = -mxStack->value.number;
					mxFloatingPointOp("minus");
				}
				mxRestoreState;
			}
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_PLUS)
			if (mxStack->kind != XS_INTEGER_KIND) {
				mxToNumber(mxStack);
			}
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_TO_NUMERIC)
			if ((mxStack->kind != XS_INTEGER_KIND) && (mxStack->kind != XS_NUMBER_KIND)) {
				mxSaveState;
				fxToNumericNumber(the, mxStack);
				mxRestoreState;
			}
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_DECREMENT)
			if (mxStack->kind == XS_INTEGER_KIND) {
				if (mxStack->value.integer != -2147483647)
					mxStack->value.integer--;
				else {
					mxStack->kind = XS_NUMBER_KIND;
					mxStack->value.number = mxStack->value.integer;
					mxStack->value.number--;
					mxFloatingPointOp("decrement");
				}
			}
			else if (mxStack->kind == XS_NUMBER_KIND) {
				mxStack->value.number--;
				mxFloatingPointOp("decrement");
			}
			else {
				mxSaveState;
				if (fxToNumericNumberUnary(the, mxStack, gxTypeBigInt._dec)) {
					mxStack->value.number--;
					mxFloatingPointOp("decrement");
				}
				mxRestoreState;
			}
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_INCREMENT)
			if (mxStack->kind == XS_INTEGER_KIND) {
				if (mxStack->value.integer != 2147483647)
					mxStack->value.integer++;
				else {
					mxStack->kind = XS_NUMBER_KIND;
					mxStack->value.number = mxStack->value.integer;
					mxStack->value.number++;
					mxFloatingPointOp("increment");
				}
			}
			else if (mxStack->kind == XS_NUMBER_KIND) {
				mxStack->value.number++;
				mxFloatingPointOp("increment");
			}
			else {
				mxSaveState;
				if (fxToNumericNumberUnary(the, mxStack, gxTypeBigInt._inc)) {
					mxStack->value.number++;
					mxFloatingPointOp("increment");
				}
				mxRestoreState;
			}
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_ADD)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					#if __has_builtin(__builtin_add_overflow)
						if (__builtin_add_overflow(slot->value.integer, mxStack->value.integer, &scratch.value.integer)) {
							slot->kind = XS_NUMBER_KIND;
							slot->value.number = (txNumber)(slot->value.integer) + (txNumber)(mxStack->value.integer);
						}
						else
							slot->value.integer = scratch.value.integer;
					#else
						txInteger a = slot->value.integer;
						txInteger b = mxStack->value.integer;
						txInteger c = a + b;
						if (((a ^ c) & (b ^ c)) < 0) {
							slot->kind = XS_NUMBER_KIND;
							slot->value.number = (txNumber)(slot->value.integer) + (txNumber)(mxStack->value.integer);
							mxFloatingPointOp("add");
						}
						else
							slot->value.integer = c;
					#endif
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->kind = XS_NUMBER_KIND;
					slot->value.number = (txNumber)(slot->value.integer) + mxStack->value.number;
					mxFloatingPointOp("add");
				}
				else
					goto XS_CODE_ADD_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					slot->value.number += (txNumber)(mxStack->value.integer);
					mxFloatingPointOp("add");
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->value.number += mxStack->value.number;
					mxFloatingPointOp("add");
				}
				else
					goto XS_CODE_ADD_GENERAL;
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
					if (fxToNumericNumberBinary(the, slot, mxStack, gxTypeBigInt._add)) {
						slot->value.number += mxStack->value.number;
						mxFloatingPointOp("add");
					}
				}
				mxRestoreState;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SUBTRACT)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					#if __has_builtin(__builtin_sub_overflow)
						if (__builtin_sub_overflow(slot->value.integer, mxStack->value.integer, &scratch.value.integer)) {
							slot->kind = XS_NUMBER_KIND;
							slot->value.number = (txNumber)(slot->value.integer) - (txNumber)(mxStack->value.integer);
						}
						else
							slot->value.integer = scratch.value.integer;
					#else
						txInteger a = slot->value.integer;
						txInteger b = -mxStack->value.integer;
						txInteger c = a + b;
						if (((a ^ c) & (b ^ c)) < 0) {
							slot->kind = XS_NUMBER_KIND;
							slot->value.number = (txNumber)(slot->value.integer) - (txNumber)(mxStack->value.integer);
						}
						else
							slot->value.integer = c;
					#endif
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->kind = XS_NUMBER_KIND;
					slot->value.number = (txNumber)(slot->value.integer) - mxStack->value.number;
					mxFloatingPointOp("subtract");
				}
				else
					goto XS_CODE_SUBTRACT_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					slot->value.number -= (txNumber)(mxStack->value.integer);
					mxFloatingPointOp("subtract");
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->value.number -= mxStack->value.number;
					mxFloatingPointOp("subtract");
				}
				else
					goto XS_CODE_SUBTRACT_GENERAL;
			}
			else {
		XS_CODE_SUBTRACT_GENERAL:
				mxSaveState;
				if (fxToNumericNumberBinary(the, slot, mxStack, gxTypeBigInt._sub)) {
					slot->value.number -= mxStack->value.number;
					mxFloatingPointOp("subtract");
				}
				mxRestoreState;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_DIVIDE)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					slot->kind = XS_NUMBER_KIND;
					slot->value.number = (txNumber)(slot->value.integer) / (txNumber)(mxStack->value.integer);
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->kind = XS_NUMBER_KIND;
					slot->value.number = (txNumber)(slot->value.integer) / mxStack->value.number;
				}
				else
					goto XS_CODE_DIVIDE_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND)
					slot->value.number /= (txNumber)(mxStack->value.integer);
				else if (mxStack->kind == XS_NUMBER_KIND)
					slot->value.number /= mxStack->value.number;
				else
					goto XS_CODE_DIVIDE_GENERAL;
			}
			else {
		XS_CODE_DIVIDE_GENERAL:
				mxSaveState;
				if (fxToNumericNumberBinary(the, slot, mxStack, gxTypeBigInt._div))
					slot->value.number /= mxStack->value.number;
				mxRestoreState;
			}
			mxFloatingPointOp("divide");
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_EXPONENTIATION)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					slot->kind = XS_NUMBER_KIND;
					slot->value.number = fx_pow((txNumber)(slot->value.integer), (txNumber)(mxStack->value.integer));
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->kind = XS_NUMBER_KIND;
					slot->value.number = fx_pow((txNumber)(slot->value.integer), mxStack->value.number);
				}
				else
					goto XS_CODE_EXPONENTIATION_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND)
					slot->value.number = fx_pow(slot->value.number, (txNumber)(mxStack->value.integer));
				else if (mxStack->kind == XS_NUMBER_KIND)
					slot->value.number = fx_pow(slot->value.number, mxStack->value.number);
				else
					goto XS_CODE_EXPONENTIATION_GENERAL;
			}
			else {
		XS_CODE_EXPONENTIATION_GENERAL:
				mxSaveState;
				if (fxToNumericNumberBinary(the, slot, mxStack, gxTypeBigInt._exp))
					slot->value.number = fx_pow(slot->value.number, mxStack->value.number);
				mxRestoreState;
			}
			mxFloatingPointOp("exponent");
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_MULTIPLY)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
				#ifdef mxMinusZero
					if (slot->value.integer == 0) {
						if (mxStack->value.integer < 0) {
							slot->kind = XS_NUMBER_KIND;
							slot->value.number = -0.0;
						}
					}
					else if (mxStack->value.integer == 0) {
						if (slot->value.integer < 0) {
							slot->kind = XS_NUMBER_KIND;
							slot->value.number = -0.0;
						}
						else
							slot->value.integer = 0;
					}
					else {
				#endif
					#if __has_builtin(__builtin_mul_overflow)
						if (__builtin_mul_overflow(slot->value.integer, mxStack->value.integer, &scratch.value.integer)) {
							slot->kind = XS_NUMBER_KIND;
							slot->value.number = (txNumber)(slot->value.integer) * (txNumber)(mxStack->value.integer);
						}
						else
							slot->value.integer = scratch.value.integer;
					#else
						slot->kind = XS_NUMBER_KIND;
						slot->value.number = (txNumber)(slot->value.integer) * (txNumber)(mxStack->value.integer);
					#endif
				#ifdef mxMinusZero
					}
				#endif
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->kind = XS_NUMBER_KIND;
					slot->value.number = (txNumber)(slot->value.integer) * mxStack->value.number;
					mxFloatingPointOp("multiply");
				}
				else
					goto XS_CODE_MULTIPLY_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					slot->value.number *= (txNumber)(mxStack->value.integer);
					mxFloatingPointOp("multiply");
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->value.number *= mxStack->value.number;
					mxFloatingPointOp("multiply");
				}
				else
					goto XS_CODE_MULTIPLY_GENERAL;
			}
			else {
		XS_CODE_MULTIPLY_GENERAL:
				mxSaveState;
				if (fxToNumericNumberBinary(the, slot, mxStack, gxTypeBigInt._mul))
					slot->value.number *= mxStack->value.number;
				mxRestoreState;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_MODULO)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					if (mxStack->value.integer == 0) {
						slot->kind = XS_NUMBER_KIND;
						slot->value.number = C_NAN;
					}
				#if mxIntegerDivideOverflowException
					else if ((-1 == mxStack->value.integer) && ((txInteger)0x80000000 == slot->value.integer)) {
						slot->kind = XS_NUMBER_KIND;
						slot->value.number = -0.0;
					}
				#endif 
				#ifdef mxMinusZero
					else if (slot->value.integer < 0) {
						slot->value.integer %= mxStack->value.integer;
						if (slot->value.integer == 0) {
							slot->kind = XS_NUMBER_KIND;
							slot->value.number = -0.0;
						}
					}
				#endif
					else
						slot->value.integer %= mxStack->value.integer;
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->kind = XS_NUMBER_KIND;
					slot->value.number = c_fmod((txNumber)(slot->value.integer), mxStack->value.number);
					mxFloatingPointOp("modulo");
				}
				else
					goto XS_CODE_MODULO_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					slot->value.number = c_fmod(slot->value.number, (txNumber)(mxStack->value.integer));
					mxFloatingPointOp("modulo");
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					slot->value.number = c_fmod(slot->value.number, mxStack->value.number);
					mxFloatingPointOp("modulo");
				}
				else
					goto XS_CODE_MODULO_GENERAL;
			}
			else {
		XS_CODE_MODULO_GENERAL:
				mxSaveState;
				if (fxToNumericNumberBinary(the, slot, mxStack, gxTypeBigInt._rem)) {
					slot->value.number = c_fmod(slot->value.number, mxStack->value.number);
					mxFloatingPointOp("modulo");
				}
				mxRestoreState;
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
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND)
					offset = slot->value.integer < mxStack->value.integer;
				else if (mxStack->kind == XS_NUMBER_KIND)  {
					offset = (!c_isnan(mxStack->value.number)) && ((txNumber)(slot->value.integer) < mxStack->value.number);
					mxFloatingPointOp("less");
				}
				else
					goto XS_CODE_LESS_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					offset = (!c_isnan(slot->value.number)) && (slot->value.number < (txNumber)(mxStack->value.integer));
					mxFloatingPointOp("less");
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number < mxStack->value.number);
					mxFloatingPointOp("less");
				}
				else
					goto XS_CODE_LESS_GENERAL;
			}
			else {
		XS_CODE_LESS_GENERAL:
				mxSaveState; 
				fxToPrimitive(the, slot, XS_NUMBER_HINT); 
				fxToPrimitive(the, mxStack, XS_NUMBER_HINT); 
				if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)))
					offset = fxUTF8Compare(slot->value.string, mxStack->value.string) < 0;
				else if ((slot->kind == XS_BIGINT_KIND) || (slot->kind == XS_BIGINT_X_KIND))
					offset = gxTypeBigInt.compare(the, 1, 0, 0, slot, mxStack);
				else if ((mxStack->kind == XS_BIGINT_KIND) || (mxStack->kind == XS_BIGINT_X_KIND))
					offset = gxTypeBigInt.compare(the, 0, 0, 1, mxStack, slot);
				else {
					fxToNumber(the, slot);
					fxToNumber(the, mxStack);
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number < mxStack->value.number);
					mxFloatingPointOp("less");
				}
				mxRestoreState;
			}
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_LESS_EQUAL)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND)
					offset = slot->value.integer <= mxStack->value.integer;
				else if (mxStack->kind == XS_NUMBER_KIND) {
					offset = (!c_isnan(mxStack->value.number)) && ((txNumber)(slot->value.integer) <= mxStack->value.number);
					mxFloatingPointOp("less or equal");
				}
				else
					goto XS_CODE_LESS_EQUAL_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					offset = (!c_isnan(slot->value.number)) && (slot->value.number <= (txNumber)(mxStack->value.integer));
					mxFloatingPointOp("less or equal");
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number <= mxStack->value.number);
					mxFloatingPointOp("less or equal");
				}
				else
					goto XS_CODE_LESS_EQUAL_GENERAL;
			}
			else {
		XS_CODE_LESS_EQUAL_GENERAL:
				mxSaveState; 
				fxToPrimitive(the, slot, XS_NUMBER_HINT); 
				fxToPrimitive(the, mxStack, XS_NUMBER_HINT); 
				if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)))
					offset = fxUTF8Compare(slot->value.string, mxStack->value.string) <= 0;
				else if ((slot->kind == XS_BIGINT_KIND) || (slot->kind == XS_BIGINT_X_KIND))
					offset = gxTypeBigInt.compare(the, 1, 1, 0, slot, mxStack);
				else if ((mxStack->kind == XS_BIGINT_KIND) || (mxStack->kind == XS_BIGINT_X_KIND))
					offset = gxTypeBigInt.compare(the, 0, 1, 1, mxStack, slot);
				else {
					fxToNumber(the, slot);
					fxToNumber(the, mxStack);
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number <= mxStack->value.number);
					mxFloatingPointOp("less or equal");
				}
				mxRestoreState;
			}
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_MORE)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND)
					offset = slot->value.integer > mxStack->value.integer;
				else if (mxStack->kind == XS_NUMBER_KIND) {
					offset = (!c_isnan(mxStack->value.number)) && ((txNumber)(slot->value.integer) > mxStack->value.number);
					mxFloatingPointOp("more");
				}
				else
					goto XS_CODE_MORE_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					offset = (!c_isnan(slot->value.number)) && (slot->value.number > (txNumber)(mxStack->value.integer));
					mxFloatingPointOp("more");
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number > mxStack->value.number);
					mxFloatingPointOp("more");
				}
				else
					goto XS_CODE_MORE_GENERAL;
			}
			else {
		XS_CODE_MORE_GENERAL:
				mxSaveState; 
				fxToPrimitive(the, slot, XS_NUMBER_HINT); 
				fxToPrimitive(the, mxStack, XS_NUMBER_HINT); 
				if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)))
					offset = fxUTF8Compare(slot->value.string, mxStack->value.string) > 0;
				else if ((slot->kind == XS_BIGINT_KIND) || (slot->kind == XS_BIGINT_X_KIND))
					offset = gxTypeBigInt.compare(the, 0, 0, 1, slot, mxStack);
				else if ((mxStack->kind == XS_BIGINT_KIND) || (mxStack->kind == XS_BIGINT_X_KIND))
					offset = gxTypeBigInt.compare(the, 1, 0, 0, mxStack, slot);
				else {
					fxToNumber(the, slot);
					fxToNumber(the, mxStack);
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number > mxStack->value.number);
					mxFloatingPointOp("more");
				}
				mxRestoreState;
			}
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_MORE_EQUAL)
			slot = mxStack + 1;
			if (slot->kind == XS_INTEGER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND)
					offset = slot->value.integer >= mxStack->value.integer;
				else if (mxStack->kind == XS_NUMBER_KIND) {
					offset = (!c_isnan(mxStack->value.number)) && ((txNumber)(slot->value.integer) >= mxStack->value.number);
					mxFloatingPointOp("more or equal");
				}
				else
					goto XS_CODE_MORE_EQUAL_GENERAL;
			}
			else if (slot->kind == XS_NUMBER_KIND) {
				if (mxStack->kind == XS_INTEGER_KIND) {
					offset = (!c_isnan(slot->value.number)) && (slot->value.number >= (txNumber)(mxStack->value.integer));
					mxFloatingPointOp("more or equal");
				}
				else if (mxStack->kind == XS_NUMBER_KIND) {
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number >= mxStack->value.number);
					mxFloatingPointOp("more or equal");
				}
				else
					goto XS_CODE_MORE_EQUAL_GENERAL;
			}
			else {
		XS_CODE_MORE_EQUAL_GENERAL:
				mxSaveState; 
				fxToPrimitive(the, slot, XS_NUMBER_HINT); 
				fxToPrimitive(the, mxStack, XS_NUMBER_HINT); 
				if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)))
					offset = fxUTF8Compare(slot->value.string, mxStack->value.string) >= 0;
				else if ((slot->kind == XS_BIGINT_KIND) || (slot->kind == XS_BIGINT_X_KIND))
					offset = gxTypeBigInt.compare(the, 0, 1, 1, slot, mxStack);
				else if ((mxStack->kind == XS_BIGINT_KIND) || (mxStack->kind == XS_BIGINT_X_KIND))
					offset = gxTypeBigInt.compare(the, 1, 1, 0, mxStack, slot);
				else {
					fxToNumber(the, slot);
					fxToNumber(the, mxStack);
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number >= mxStack->value.number);
					mxFloatingPointOp("more or equal");
				}
				mxRestoreState;
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
				else if (XS_NUMBER_KIND == slot->kind) {
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number == mxStack->value.number);
					mxFloatingPointOp("equal");
				}
				else if ((XS_STRING_KIND == slot->kind) || (XS_STRING_X_KIND == slot->kind))
					offset = c_strcmp(slot->value.string, mxStack->value.string) == 0;
				else if (XS_SYMBOL_KIND == slot->kind)
					offset = slot->value.symbol == mxStack->value.symbol;
				else if (XS_REFERENCE_KIND == slot->kind)
					offset = fxIsSameReference(the, slot, mxStack);
			#ifdef mxHostFunctionPrimitive
				else if (XS_HOST_FUNCTION_KIND == slot->kind)
					offset = slot->value.hostFunction.builder == mxStack->value.hostFunction.builder;
			#endif
 				else if ((XS_BIGINT_KIND == slot->kind) || (XS_BIGINT_X_KIND == slot->kind))
					offset = gxTypeBigInt.compare(the, 0, 1, 0, slot, mxStack);
				else
                    offset = 0;
			}
			else if ((XS_INTEGER_KIND == slot->kind) && (XS_NUMBER_KIND == mxStack->kind)) {
				offset = (!c_isnan(mxStack->value.number)) && ((txNumber)(slot->value.integer) == stack->value.number);
				mxFloatingPointOp("equal");
			}
			else if ((XS_NUMBER_KIND == slot->kind) && (XS_INTEGER_KIND == mxStack->kind)) {
				offset = (!c_isnan(slot->value.number)) && (slot->value.number == (txNumber)(mxStack->value.integer));
				mxFloatingPointOp("equal");
			}
			else if ((XS_STRING_KIND == slot->kind) && (XS_STRING_X_KIND == mxStack->kind))
				offset = c_strcmp(slot->value.string, mxStack->value.string) == 0;
			else if ((XS_STRING_X_KIND == slot->kind) && (XS_STRING_KIND == mxStack->kind))
				offset = c_strcmp(slot->value.string, mxStack->value.string) == 0;
			else if (XS_CODE_EQUAL == byte) {
				if ((slot->kind == XS_UNDEFINED_KIND) && (mxStack->kind == XS_NULL_KIND))
					offset = 1;
				else if ((slot->kind == XS_NULL_KIND) && (mxStack->kind == XS_UNDEFINED_KIND))
					offset = 1;
				else if (((XS_INTEGER_KIND == slot->kind) || (XS_NUMBER_KIND == slot->kind)) && ((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND))) {
					mxToNumber(mxStack); 
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((XS_INTEGER_KIND == mxStack->kind) || (XS_NUMBER_KIND == mxStack->kind))) {
					mxToNumber(slot);
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (XS_BOOLEAN_KIND == slot->kind) {
					mxToNumber(slot);
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (XS_BOOLEAN_KIND == mxStack->kind) {
					mxToNumber(mxStack);
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (((slot->kind == XS_INTEGER_KIND) || (slot->kind == XS_NUMBER_KIND) || (slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND) || (slot->kind == XS_SYMBOL_KIND) || (slot->kind == XS_BIGINT_KIND) || (slot->kind == XS_BIGINT_X_KIND)) && mxIsReference(mxStack)) {
					mxSaveState;
					fxToPrimitive(the, mxStack, XS_NO_HINT);
					mxRestoreState;
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (mxIsReference(slot) && ((mxStack->kind == XS_INTEGER_KIND) || (mxStack->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND) || (mxStack->kind == XS_SYMBOL_KIND) || (mxStack->kind == XS_BIGINT_KIND) || (mxStack->kind == XS_BIGINT_X_KIND))) {
					mxSaveState;
					fxToPrimitive(the, slot, XS_NO_HINT);
					mxRestoreState;
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (((XS_BIGINT_KIND == slot->kind) || (XS_BIGINT_X_KIND == slot->kind)) && ((mxStack->kind == XS_INTEGER_KIND) || (mxStack->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND))) {
					mxSaveState;
					offset = gxTypeBigInt.compare(the, 0, 1, 0, slot, mxStack);
					mxRestoreState;
				}
				else if (((slot->kind == XS_INTEGER_KIND) || (slot->kind == XS_NUMBER_KIND) || (slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((XS_BIGINT_KIND == mxStack->kind) || (XS_BIGINT_X_KIND == mxStack->kind))) {
					mxSaveState;
					offset = gxTypeBigInt.compare(the, 0, 1, 0, mxStack, slot);
					mxRestoreState;
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
				else if (XS_NUMBER_KIND == slot->kind) {
					offset = (c_isnan(slot->value.number)) || (c_isnan(mxStack->value.number)) || (slot->value.number != mxStack->value.number);
					mxFloatingPointOp("not equal");
				}
				else if ((XS_STRING_KIND == slot->kind) || (XS_STRING_X_KIND == slot->kind))
					offset = c_strcmp(slot->value.string, mxStack->value.string) != 0;
				else if (XS_SYMBOL_KIND == slot->kind)
					offset = slot->value.symbol != mxStack->value.symbol;
				else if (XS_REFERENCE_KIND == slot->kind)
					offset = !fxIsSameReference(the, slot, mxStack);
			#ifdef mxHostFunctionPrimitive
				else if (XS_HOST_FUNCTION_KIND == slot->kind)
					offset = slot->value.hostFunction.builder != mxStack->value.hostFunction.builder;
			#endif
 				else if ((XS_BIGINT_KIND == slot->kind) || (XS_BIGINT_X_KIND == slot->kind))
					offset = gxTypeBigInt.compare(the, 1, 0, 1, slot, mxStack);
                else
                	offset = 1;
			}
			else if ((XS_INTEGER_KIND == slot->kind) && (XS_NUMBER_KIND == mxStack->kind)) {
				offset = (c_isnan(mxStack->value.number)) || ((txNumber)(slot->value.integer) != stack->value.number);
				mxFloatingPointOp("not equal");
			}
			else if ((XS_NUMBER_KIND == slot->kind) && (XS_INTEGER_KIND == mxStack->kind)) {
				offset = (c_isnan(slot->value.number)) || (slot->value.number != (txNumber)(mxStack->value.integer));
				mxFloatingPointOp("not equal");
			}
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
					mxToNumber(slot);
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)) && ((XS_INTEGER_KIND == slot->kind) || (XS_NUMBER_KIND == slot->kind))) {
					mxToNumber(mxStack); 
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (XS_BOOLEAN_KIND == slot->kind) {
					mxToNumber(slot);
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (XS_BOOLEAN_KIND == mxStack->kind) {
					mxToNumber(mxStack);
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (((slot->kind == XS_INTEGER_KIND) || (slot->kind == XS_NUMBER_KIND) || (slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND) || (slot->kind == XS_SYMBOL_KIND) || (slot->kind == XS_BIGINT_KIND) || (slot->kind == XS_BIGINT_X_KIND)) && mxIsReference(mxStack)) {
					mxSaveState;
					fxToPrimitive(the, mxStack, XS_NO_HINT);
					mxRestoreState;
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (mxIsReference(slot) && ((mxStack->kind == XS_INTEGER_KIND) || (mxStack->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND) || (mxStack->kind == XS_SYMBOL_KIND) || (mxStack->kind == XS_BIGINT_KIND) || (mxStack->kind == XS_BIGINT_X_KIND))) {
					mxSaveState;
					fxToPrimitive(the, slot, XS_NO_HINT);
					mxRestoreState;
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (((XS_BIGINT_KIND == slot->kind) || (XS_BIGINT_X_KIND == slot->kind)) && ((mxStack->kind == XS_INTEGER_KIND) || (mxStack->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND))) {
					mxSaveState;
					offset = gxTypeBigInt.compare(the, 1, 0, 1, slot, mxStack);
					mxRestoreState;
				}
				else if (((slot->kind == XS_INTEGER_KIND) || (slot->kind == XS_NUMBER_KIND) || (slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((XS_BIGINT_KIND == mxStack->kind) || (XS_BIGINT_X_KIND == mxStack->kind))) {
					mxSaveState;
					offset = gxTypeBigInt.compare(the, 1, 0, 1, mxStack, slot);
					mxRestoreState;
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
			gxDefaults.runForAwaitOf(the);
			mxRestoreState;
			mxBreak;
		mxCase(XS_CODE_FOR_IN)
			mxSkipCode(1);
			/* FUNCTION */
			mxAllocStack(1);
			*mxStack = mxEnumeratorFunction;
			slot = fxGetInstance(the, mxStack);
			/* TARGET */
			mxPushKind(XS_UNDEFINED_KIND);
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
			mxPushKind(XS_UNDEFINED_KIND);
			mxPushKind(XS_UNDEFINED_KIND);
			offset = 0;
			goto XS_CODE_RUN_ALL;

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
			else if ((XS_BIGINT_KIND == byte) || (XS_BIGINT_X_KIND == byte))
				*mxStack = mxBigIntString;
			else if (XS_REFERENCE_KIND == byte) {
				slot = fxGetInstance(the, mxStack);
				if (slot->flag & XS_CAN_CALL_FLAG)
					*mxStack = mxFunctionString;
				else
					*mxStack = mxObjectString;
			}
		#ifdef mxHostFunctionPrimitive
			else if (slot->kind == XS_HOST_FUNCTION_KIND)
				*mxStack = mxFunctionString;
		#endif
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
			count = mxRunID(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, count, 0);
#endif
			mxEnvironment->ID = count;
		#endif
			mxNextCode(1 + sizeof(txID));
			mxBreak;
		mxCase(XS_CODE_LINE)
		#ifdef mxDebug
			count = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, count);
#endif
			mxEnvironment->value.environment.line = count;
			if (fxIsReadable(the)) {
				mxSaveState;
				fxDebugCommand(the);
				mxRestoreState;
			}
			if ((mxEnvironment->ID != XS_NO_ID) && ((mxFrame->flag & XS_STEP_OVER_FLAG) || mxBreakpoints.value.list.first)) {
				mxSaveState;
				fxDebugLine(the, mxEnvironment->ID, count);
				mxRestoreState;
			}
		#endif
		#if defined(mxInstrument) || defined(mxProfile)
			fxCheckProfiler(the, mxFrame);
		#endif
			mxNextCode(3);
			mxBreak;

	/* MODULE */		
		mxCase(XS_CODE_IMPORT)
			slot = mxFunctionInstanceHome(mxFrameFunction->value.reference)->value.home.module;
			slot = mxModuleInstanceInternal(slot);
			variable = slot->value.module.realm;
			if (!variable) variable = mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm;
			mxSaveState;
			gxDefaults.runImport(the, variable, slot->value.module.id);
			mxRestoreState;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_IMPORT_META)
			variable = mxFunctionInstanceHome(mxFrameFunction->value.reference);
			slot = mxModuleInstanceMeta(variable->value.home.module);
			mxPushKind(XS_REFERENCE_KIND);
			mxStack->value.reference = slot->value.reference;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_TRANSFER)
			mxSaveState;
			fxPrepareTransfer(the);
			mxRestoreState;
            mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_MODULE)
			byte = mxRunU1(1);
			mxSaveState;
			fxPrepareModule(the, byte);
			mxRestoreState;
            mxNextCode(2);
			mxBreak;
			
	/* EVAL, PROGRAM & WITH */
		mxCase(XS_CODE_EVAL)
			offset = mxStack->value.integer;
			slot = mxStack + 1 + offset + 4;
			if (slot->value.reference == mxEvalFunction.value.reference) {
				mxSaveState;
				gxDefaults.runEval(the);
				mxRestoreState;
				mxNextCode(1);
				mxBreak;
			}
			mxSkipCode(1);
			mxStack++;
			goto XS_CODE_RUN_ALL;
		mxCase(XS_CODE_EVAL_TAIL)
			offset = mxStack->value.integer;
			slot = mxStack + 1 + offset + 4;
			if (slot->value.reference == mxEvalFunction.value.reference) {
				mxSaveState;
				gxDefaults.runEval(the);
				mxRestoreState;
				mxNextCode(1);
				mxBreak;
			}
			mxSkipCode(1);
			mxStack++;
			goto XS_CODE_RUN_TAIL_ALL;
		mxCase(XS_CODE_EVAL_ENVIRONMENT)
			mxNextCode(1);
			mxSaveState;
			gxDefaults.runEvalEnvironment(the);
			mxRestoreState;
			mxBreak;
		mxCase(XS_CODE_EVAL_PRIVATE)
			offset = mxRunID(1);
		#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, 0);
		#endif
			mxNextCode(1 + sizeof(txID));
			variable = mxEnvironment;
			if (variable->kind == XS_REFERENCE_KIND) {
				variable = variable->value.reference;
		XS_CODE_EVAL_PRIVATE_AGAIN:
				if (variable) {
					slot = mxBehaviorGetProperty(the, variable, (txID)offset, 0, XS_OWN);
					if (slot) {
						mxPushKind(slot->kind);
						mxStack->value = slot->value;
						mxBreak;
					}
					variable = variable->value.instance.prototype;
					goto XS_CODE_EVAL_PRIVATE_AGAIN;
				}
			}
			mxRunDebugID(XS_SYNTAX_ERROR, "eval %s: undefined private property", (txID)offset);
			mxBreak;
		mxCase(XS_CODE_EVAL_REFERENCE)
			offset = mxRunID(1);
		#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, 0);
		#endif
			mxNextCode(1 + sizeof(txID));
			variable = mxEnvironment;
			if (variable->kind == XS_REFERENCE_KIND) {
				variable = variable->value.reference;
		XS_CODE_EVAL_REFERENCE_AGAIN:
				if (variable->value.instance.prototype) {
					slot = variable->next;
					if (slot->kind == XS_REFERENCE_KIND) {
						slot = slot->value.reference;
						mxSaveState;
						index = fxIsScopableSlot(the, slot, (txID)offset);
						mxRestoreState;
						if (index) {
							if (XS_CODE_GET_THIS_VARIABLE == byte) {
								mxStack->kind = XS_REFERENCE_KIND;
								mxStack->value.reference = slot;
							}
							mxPushKind(XS_REFERENCE_KIND);
							mxStack->value.reference = slot;
							mxBreak;
						}
					}
					else if (mxBehaviorHasProperty(the, variable, (txID)offset, 0)) {
						mxPushKind(XS_REFERENCE_KIND);
						mxStack->value.reference = variable;
						mxBreak;
					}
					variable = variable->value.instance.prototype;
					goto XS_CODE_EVAL_REFERENCE_AGAIN;
				}
				if (mxBehaviorHasProperty(the, variable, (txID)offset, 0)) {
					mxPushKind(XS_REFERENCE_KIND);
					mxStack->value.reference = variable;
					mxBreak;
				}
			}
			mxPushKind(XS_REFERENCE_KIND);
			variable = mxFunctionInstanceHome(mxFrameFunction->value.reference)->value.home.module;
			variable = mxModuleInstanceInternal(variable)->value.module.realm;
			if (!variable) variable = mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm;
			mxStack->value.reference = mxRealmGlobal(variable)->value.reference;
			mxBreak;
		mxCase(XS_CODE_FUNCTION_ENVIRONMENT)	
			variable = mxEnvironment;
			mxPushKind(XS_UNDEFINED_KIND);
			mxSaveState;
			slot = fxNewEnvironmentInstance(the, variable);
			mxRestoreState;
			variable = mxFunctionInstanceCode((mxStack + 1)->value.reference);
			variable->value.code.closures = slot;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_PROGRAM_ENVIRONMENT)
			mxNextCode(1);
			mxSaveState;
			gxDefaults.runProgramEnvironment(the);
			mxRestoreState;
			mxBreak;
		mxCase(XS_CODE_PROGRAM_REFERENCE)
			offset = mxRunID(1);
		#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, (txID)offset, 0);
		#endif
			mxNextCode(1 + sizeof(txID));
            variable = mxFunctionInstanceHome(mxFrameFunction->value.reference)->value.home.module;
            variable = mxModuleInstanceInternal(variable)->value.module.realm;
            if (!variable) variable = mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm;
            slot = mxRealmClosures(variable)->value.reference;
            if (mxBehaviorHasProperty(the, slot, (txID)offset, 0)) {
                mxPushKind(XS_REFERENCE_KIND);
                mxStack->value.reference = slot;
                mxBreak;
            }
			mxPushKind(XS_REFERENCE_KIND);
			mxStack->value.reference = mxRealmGlobal(variable)->value.reference;
			mxBreak;
		mxCase(XS_CODE_WITH)
			variable = mxEnvironment;
			mxSaveState;
			slot = fxNewEnvironmentInstance(the, variable);
			mxRestoreState;
			variable->kind = XS_REFERENCE_KIND;
			variable->value.reference = slot;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_WITHOUT)
			variable = mxEnvironment;
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

#ifdef mxMetering

void fxBeginMetering(txMachine* the, txBoolean (*callback)(txMachine*, txU4), txU4 interval)
{
	the->meterCallback = callback;
	the->meterCount = interval;
	the->meterIndex = 0;
	the->meterInterval = interval;
}

void fxEndMetering(txMachine* the)
{
	the->meterCallback = C_NULL;
	the->meterIndex = 0;
	the->meterInterval = 0;
	the->meterCount = 0;
}

void fxCheckMetering(txMachine* the)
{
	txU4 interval = the->meterInterval;
	the->meterInterval = 0;
	if ((*the->meterCallback)(the, the->meterIndex)) {
		the->meterCount = the->meterIndex + interval;
		if (the->meterCount < the->meterIndex) {
			the->meterIndex = 0;
			the->meterCount = interval;
		}
		the->meterInterval = interval;
	}
	else {
		fxAbort(the, XS_TOO_MUCH_COMPUTATION_EXIT);
	}
}
#endif

void fxRunArguments(txMachine* the, txIndex offset)
{
	txSlot* array;
	txIndex length = (txIndex)mxArgc;
	txIndex index;
	txSlot* address;
	mxPush(mxArrayPrototype);
	array = fxNewArrayInstance(the)->next;
	if (offset < length) {
		length -= offset;
		fxSetIndexSize(the, array, length, XS_CHUNK);
		index = 0;
		address = array->value.array.address;
		while (index < length) {
			txSlot* property = mxArgv(offset + index);
			*((txIndex*)address) = index;
			address->ID = XS_NO_ID;
			address->kind = property->kind;
			address->value = property->value;
			index++;
			address++;
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

txBoolean fxRunDefine(txMachine* the, txSlot* instance, txSlot* check, txID id, txIndex index, txSlot* slot, txFlag mask) 
{
	txBoolean result;
	if (check) {
		result = gxDefaults.definePrivateProperty(the, instance, check, id, slot, mask);
	}
	else {
		fxBeginHost(the);
		mxCheck(the, instance->kind == XS_INSTANCE_KIND);
		result = mxBehaviorDefineOwnProperty(the, instance, id, index, slot, mask);
		fxEndHost(the);
	}
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
	if (!mxIsConstructor(constructor))
		mxTypeError("extends: no constructor");
	mxPushUndefined();
	prototype = the->stack;
	fxBeginHost(the);
	mxPushReference(constructor);
	mxGetID(mxID(_prototype));
	*prototype = *the->stack;
	fxEndHost(the);
	if (the->stack->kind == XS_NULL_KIND) {
		mxPop();
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
	txStringStream aStream;
	txUnsigned flags;
	txSlot* function;
	txSlot* home;
	txSlot* closures;
	if (the->stack->value.integer == 0)
		the->stack->kind = XS_UNDEFINED_KIND;
	else
		the->stack += the->stack->value.integer;
	the->stack[6] = the->stack[0];
	the->stack += 6;
	if ((the->stack->kind == XS_STRING_KIND) || (the->stack->kind == XS_STRING_X_KIND)) {
		aStream.slot = the->stack;
		aStream.offset = 0;
		aStream.size = mxStringLength(fxToString(the, aStream.slot));
		flags = mxProgramFlag | mxEvalFlag;
		if (the->frame->flag & XS_STRICT_FLAG)
			flags |= mxStrictFlag;
		if (the->frame->flag & XS_FIELD_FLAG)
			flags |= mxFieldFlag;
		function = mxFunction->value.reference;
		if (function->flag & XS_CAN_CONSTRUCT_FLAG)
			flags |= mxTargetFlag;
		home = mxFunctionInstanceHome(function);
		if (home->value.home.object)
			flags |= mxSuperFlag;
		closures = mxFrameToEnvironment(the->frame);
		if (closures->kind == XS_REFERENCE_KIND)
			closures = closures->value.reference;
		else
			closures = C_NULL;
		fxRunScript(the, fxParseScript(the, &aStream, fxStringGetter, flags), mxThis, mxTarget, closures, home->value.home.object, home->value.home.module);
		aStream.slot->kind = the->stack->kind;
		aStream.slot->value = the->stack->value;
		mxPop();
	}
}

void fxRunForAwaitOf(txMachine* the)
{
	txSlot* slot = the->stack;
	fxBeginHost(the);
	mxPushSlot(slot);
	mxDub();
	mxGetID(mxID(_Symbol_asyncIterator));
	if (mxIsUndefined(the->stack) || mxIsNull(the->stack)) {
		mxPop();
		mxPop();
		fxGetIterator(the, slot, the->stack, C_NULL, 0);
		fxNewAsyncFromSyncIteratorInstance(the);
	}
	else {
		mxCall();
		mxRunCount(0);
	}
	mxPullSlot(slot);
	fxEndHost(the);
}

void fxRunForOf(txMachine* the)
{
	txSlot* slot = the->stack;
	fxBeginHost(the);
	fxGetIterator(the, slot, slot, C_NULL, 0);
	fxEndHost(the);
}

txBoolean fxRunHas(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	txBoolean result;
	fxBeginHost(the);
	result = mxBehaviorHasProperty(the, instance, id, index);
	fxEndHost(the);
	return result;
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
	mxPop();
}

void fxRunInstanceOf(txMachine* the)
{
	txSlot* left = the->stack + 1;
	txSlot* right = the->stack;
	fxBeginHost(the);
	mxPushSlot(right);
	mxDub();
	mxGetID(mxID(_Symbol_hasInstance));
	mxCall();
	mxPushSlot(left);
	mxRunCount(1);
	fxToBoolean(the, the->stack);
	mxPullSlot(left);
	fxEndHost(the);
	mxPop();
}

void fxRunInstantiate(txMachine* the)
{
	if (the->stack->kind == XS_NULL_KIND) {
		mxPop();
		fxNewInstance(the);
	}
	else if (the->stack->kind == XS_REFERENCE_KIND) {
		fxNewHostInstance(the);
	}
	else {
		mxPop();
		mxPush(mxObjectPrototype);
		fxNewObjectInstance(the);
	}
}

void fxRunProxy(txMachine* the, txSlot* instance)
{
	txSlot* array;
	txIndex length = (txIndex)mxArgc;
	txIndex index;
	txSlot* address;
	mxPush(mxArrayPrototype);
	array = fxNewArrayInstance(the)->next;
	fxSetIndexSize(the, array, length, XS_CHUNK);
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
	mxPop();
}

txBoolean fxIsSameReference(txMachine* the, txSlot* a, txSlot* b)
{	
	a = a->value.reference;
	b = b->value.reference;
	if (a == b)
		return 1;
	if (a->ID) {
		txSlot* alias = the->aliasArray[a->ID];
		if (alias) {
			a = alias;
			if (a == b)
				return 1;
		}
	}
	if (b->ID) {
		txSlot* alias = the->aliasArray[b->ID];
		if (alias) {
			b = alias;
			if (a == b)
				return 1;
		}
	}
	return 0;
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
        else if (XS_NUMBER_KIND == a->kind) {
			result = (!c_isnan(a->value.number)) && (!c_isnan(b->value.number)) && (a->value.number == b->value.number);
			mxFloatingPointOp("same slot");
		}
		else if ((XS_STRING_KIND == a->kind) || (XS_STRING_X_KIND == a->kind))
			result = c_strcmp(a->value.string, b->value.string) == 0;
		else if (XS_SYMBOL_KIND == a->kind)
			result = a->value.symbol == b->value.symbol;
		else if ((XS_BIGINT_KIND == a->kind) || (XS_BIGINT_X_KIND == a->kind))
			result = gxTypeBigInt.compare(the, 0, 1, 0, a, b);
		else if (XS_REFERENCE_KIND == a->kind)
			result = fxIsSameReference(the, a, b);
	}
	else if ((XS_INTEGER_KIND == a->kind) && (XS_NUMBER_KIND == b->kind)) {
		result = (!c_isnan(b->value.number)) && ((txNumber)(a->value.integer) == b->value.number);
		mxFloatingPointOp("same slot");
	}
	else if ((XS_NUMBER_KIND == a->kind) && (XS_INTEGER_KIND == b->kind)) {
		result = (!c_isnan(a->value.number)) && (a->value.number == (txNumber)(b->value.integer));
		mxFloatingPointOp("same slot");
	}
	else if ((XS_STRING_KIND == a->kind) && (XS_STRING_X_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	else if ((XS_STRING_X_KIND == a->kind) && (XS_STRING_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	else if ((XS_BIGINT_KIND == a->kind) && (XS_BIGINT_X_KIND == b->kind))
		result = gxTypeBigInt.compare(the, 0, 1, 0, a, b);
	else if ((XS_BIGINT_X_KIND == a->kind) && (XS_BIGINT_KIND == b->kind))
		result = gxTypeBigInt.compare(the, 0, 1, 0, a, b);
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
        else if (XS_NUMBER_KIND == a->kind) {
			result = ((c_isnan(a->value.number) && c_isnan(b->value.number)) || ((a->value.number == b->value.number) && (zero || (c_signbit(a->value.number) == c_signbit(b->value.number)))));
			mxFloatingPointOp("same value");
		}
		else if ((XS_STRING_KIND == a->kind) || (XS_STRING_X_KIND == a->kind))
			result = c_strcmp(a->value.string, b->value.string) == 0;
		else if (XS_SYMBOL_KIND == a->kind)
			result = a->value.symbol == b->value.symbol;
		else if ((XS_BIGINT_KIND == a->kind) || (XS_BIGINT_X_KIND == a->kind))
			result = gxTypeBigInt.compare(the, 0, 1, 0, a, b);
		else if (XS_REFERENCE_KIND == a->kind)
			result = fxIsSameReference(the, a, b);
	}
	else if ((XS_INTEGER_KIND == a->kind) && (XS_NUMBER_KIND == b->kind)) {
		txNumber aNumber = a->value.integer;
		result = (aNumber == b->value.number) && (zero || (signbit(aNumber) == signbit(b->value.number)));
		mxFloatingPointOp("same value");
	}
	else if ((XS_NUMBER_KIND == a->kind) && (XS_INTEGER_KIND == b->kind)) {
		txNumber bNumber = b->value.integer;
		result = (a->value.number == bNumber) && (zero || (signbit(a->value.number) == signbit(bNumber)));
		mxFloatingPointOp("same value");
	}
	else if ((XS_STRING_KIND == a->kind) && (XS_STRING_X_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	else if ((XS_STRING_X_KIND == a->kind) && (XS_STRING_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	else if ((XS_BIGINT_KIND == a->kind) && (XS_BIGINT_X_KIND == b->kind))
		result = gxTypeBigInt.compare(the, 0, 1, 0, a, b);
	else if ((XS_BIGINT_X_KIND == a->kind) && (XS_BIGINT_KIND == b->kind))
		result = gxTypeBigInt.compare(the, 0, 1, 0, a, b);
	return result;
}

txBoolean fxIsScopableSlot(txMachine* the, txSlot* instance, txID id)
{	
	txBoolean result;
	fxBeginHost(the);
	mxPushReference(instance);
	result = mxHasID(id);
	if (result) {
		mxPushReference(instance);
		mxGetID(mxID(_Symbol_unscopables));
		if (mxIsReference(the->stack)) {
			mxGetID(id);
			result = fxToBoolean(the, the->stack) ? 0 : 1;
		}
		mxPop();
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
	txID id;
	while (p < q) {
		//fprintf(stderr, "%s", gxCodeNames[*((txU1*)p)]);
		offset = (txS1)c_read8(bytes + c_read8(p));
		if (0 < offset)
			p += offset;
		else if (0 == offset) {
			p++;
			mxDecodeID(p, id);
			if (id != XS_NO_ID) {
				id = theIDs[id];
				p -= sizeof(txID);
				mxEncodeID(p, id);
			}
		}
		else if (-1 == offset) {
			txU1 index;
			p++;
			index = *((txU1*)p);
			p += 1 + index;
		}
        else if (-2 == offset) {
			txU2 index;
            p++;
            mxDecode2(p, index);
            p += index;
        }
        else if (-4 == offset) {
			txS4 index;
            p++;
            mxDecode4(p, index);
            p += index;
        }
		//fprintf(stderr, "\n");
	}
}

txBoolean fxToNumericInteger(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_REFERENCE_KIND)
		fxToPrimitive(the, theSlot, XS_NUMBER_HINT);
	if ((theSlot->kind == XS_BIGINT_KIND) || (theSlot->kind == XS_BIGINT_X_KIND))
		return 0;
	fxToInteger(the, theSlot);
	return 1;
}

txBoolean fxToNumericIntegerUnary(txMachine* the, txSlot* a, txBigIntUnary op)
{
	if (fxToNumericInteger(the, a))
		return 1;
	a->value.bigint = *(*op)(the, C_NULL, &a->value.bigint);
	a->kind = XS_BIGINT_KIND;
	the->stack = a;
	return 0;		
}

txBoolean fxToNumericIntegerBinary(txMachine* the, txSlot* a, txSlot* b, txBigIntBinary op)
{
	txBoolean ra = fxToNumericInteger(the, a);
	txBoolean rb = fxToNumericInteger(the, b);
	if (ra) {
		if (rb)
			return 1;
		mxTypeError("Cannot coerce left operand to bigint");
	}
	else if (rb)
		mxTypeError("Cannot coerce right operand to bigint");
	a->value.bigint = *(*op)(the, C_NULL, &a->value.bigint, &b->value.bigint);
	a->kind = XS_BIGINT_KIND;
	the->stack = b;
	return 0;		
}

txBoolean fxToNumericNumber(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_REFERENCE_KIND)
		fxToPrimitive(the, theSlot, XS_NUMBER_HINT);
	if ((theSlot->kind == XS_BIGINT_KIND) || (theSlot->kind == XS_BIGINT_X_KIND))
		return 0;
	fxToNumber(the, theSlot);
	return 1;
}

txBoolean fxToNumericNumberUnary(txMachine* the, txSlot* a, txBigIntUnary op)
{
	if (fxToNumericNumber(the, a))
		return 1;
	a->value.bigint = *(*op)(the, C_NULL, &a->value.bigint);
	a->kind = XS_BIGINT_KIND;
	the->stack = a;
	return 0;		
}

txBoolean fxToNumericNumberBinary(txMachine* the, txSlot* a, txSlot* b, txBigIntBinary op)
{
	txBoolean ra = fxToNumericNumber(the, a);
	txBoolean rb = fxToNumericNumber(the, b);
	if (ra) {
		if (rb)
			return 1;
		mxTypeError("Cannot coerce left operand to bigint");
	}
	else if (rb)
		mxTypeError("Cannot coerce right operand to bigint");
	a->value.bigint = *(*op)(the, C_NULL, &a->value.bigint, &b->value.bigint);
	a->kind = XS_BIGINT_KIND;
	the->stack = b;
	return 0;		
}

void fxRunScript(txMachine* the, txScript* script, txSlot* _this, txSlot* _target, txSlot* closures, txSlot* object, txSlot* module)
{
	if (script) {
		mxTry(the) {
			txSlot* instance;
			txSlot* property;
			__JUMP__.code = C_NULL;
			mxPushUndefined();
			if (script->symbolsBuffer) {
				txByte* p = script->symbolsBuffer;
				txID c, i;
				mxDecodeID(p, c);
				the->stack->value.callback.address = C_NULL;
				the->stack->value.IDs = (txID*)fxNewChunk(the, c * sizeof(txID));
				the->stack->kind = XS_IDS_KIND;
				the->stack->value.IDs[0] = XS_NO_ID;
				for (i = 1; i < c; i++) {
					txID id = fxNewNameC(the, (txString)p);
					the->stack->value.IDs[i] = id;
					p += mxStringLength((char*)p) + 1;
				}
				fxRemapIDs(the, script->codeBuffer, script->codeSize, the->stack->value.IDs);
				the->stack->value.IDs = C_NULL;
			}	
			else {
				the->stack->value.IDs = C_NULL;
				the->stack->kind = XS_IDS_KIND;
			}
			if (script->callback) {
				property = the->stack;
				/* THIS */
				if (_this)
					mxPushSlot(_this);
				else
					mxPushUndefined();
				the->stack->ID = XS_NO_ID;
				/* FUNCTION */
				mxPush(mxFunctionPrototype);
				instance = fxNewFunctionInstance(the, closures ? mxID(_eval) : XS_NO_ID);
				instance->next->kind = XS_CALLBACK_KIND;
				instance->next->value.callback.address = script->callback;
				instance->next->value.callback.closures = C_NULL;
				property = mxFunctionInstanceHome(instance);
				property->value.home.object = object;
				property->value.home.module = module;
				/* TARGET */
				mxPushUndefined();
				/* RESULT */
				mxPushUndefined();
				/* FRAME */
				mxPushUninitialized();
				/* COUNT */
				mxPushUninitialized();
				mxRunCount(0);
			}
			else {
				mxPushUndefined();
			}
			mxPull(mxHosts);
			mxPop();

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
			property->ID = fxGenerateProfileID(the);
			property->value.home.object = object;
			property->value.home.module = module;
			/* TARGET */
			if (_target)
				mxPushSlot(_target);
			else
				mxPushUndefined();
			/* RESULT */
            mxPushUndefined();
 			/* FRAME */
			mxPushUninitialized();
			/* COUNT */
			mxPushUninitialized();
			mxRunCount(0);

			mxPushUndefined();
			mxPull(mxHosts);
			if (script->symbolsBuffer)
				fxDeleteScript(script);
		}
		mxCatch(the) {
			if (script->symbolsBuffer)
				fxDeleteScript(script);
			fxJump(the);
		}
	}
	else {
		mxSyntaxError("invalid script");
	}
}













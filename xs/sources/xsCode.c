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

#include "xsScript.h"

//#define mxCodePrint 1

#define mxByteCodePart\
	txByteCode* nextCode;\
	txInteger id;\
	txInteger stackLevel
	
struct sxByteCode {
	mxByteCodePart;
};
	
struct sxBranchCode {
	mxByteCodePart;
	txTargetCode* target;
};

struct sxFlagCode {
	mxByteCodePart;
	txFlag flag;
};
	
struct sxIndexCode {
	mxByteCodePart;
	txInteger index;
};
	
struct sxIntegerCode {
	mxByteCodePart;
	txInteger integer;
};
	
struct sxNumberCode {
	mxByteCodePart;
	txNumber number;
};
	
struct sxStringCode {
	mxByteCodePart;
	txInteger length;
	txString string;
};
	
struct sxSymbolCode {
	mxByteCodePart;
	txSymbol* symbol;
};

struct sxTargetCode {
	mxByteCodePart;
	txInteger index;
	txLabelNode* label;
	txTargetCode* nextTarget;
	txInteger environmentLevel;
	txInteger scopeLevel;
	txInteger offset;
	txTargetCode* original;
};
	
struct sxVariableCode {
	mxByteCodePart;
	txSymbol* symbol;
	txInteger index;
};

struct sxCoder {
	txParser* parser;
	txByteCode* firstCode;
	txByteCode* lastCode;
	txTargetCode* firstBreakTarget;
	txTargetCode* firstContinueTarget;
	txTargetCode* returnTarget;
	txInteger environmentLevel;
	txInteger scopeLevel;
	txInteger stackLevel;
	txInteger targetIndex;
	txSymbol* path;
	txInteger line;
	txBoolean programFlag;
	txBoolean evalFlag;
};

static void fxCoderAdd(txCoder* self, txInteger delta, void* it);
static void fxCoderAddBranch(txCoder* self, txInteger delta, txInteger id, txTargetCode* target);
static void fxCoderAddByte(txCoder* self, txInteger delta, txInteger id);
static void fxCoderAddFlag(txCoder* self, txInteger delta, txInteger id, txFlag flag);
static void fxCoderAddIndex(txCoder* self, txInteger delta, txInteger id, txInteger index);
static void fxCoderAddInteger(txCoder* self, txInteger delta, txInteger id, txInteger integer);
static void fxCoderAddLine(txCoder* self, txInteger delta, txInteger id, txNode* node);
static void fxCoderAddNumber(txCoder* self, txInteger delta, txInteger id, txNumber number);
static void fxCoderAddString(txCoder* self, txInteger delta, txInteger id, txInteger length, txString string);
static void fxCoderAddSymbol(txCoder* self, txInteger delta, txInteger id, txSymbol* symbol);
static void fxCoderAddVariable(txCoder* self, txInteger delta, txInteger id, txSymbol* symbol, txInteger index);
static void fxCoderAdjustEnvironment(txCoder* self, txTargetCode* target);
static void fxCoderAdjustScope(txCoder* self, txTargetCode* target);
static txTargetCode* fxCoderAliasTargets(txCoder* self, txTargetCode* target);
static txInteger fxCoderCountParameters(txCoder* self, txNode* it);
static txTargetCode* fxCoderCreateTarget(txCoder* self);
static txTargetCode* fxCoderFinalizeTargets(txCoder* self, txTargetCode* alias, txInteger selector, txInteger* address, txTargetCode* finallyTarget);
static void fxCoderJumpTargets(txCoder* self, txTargetCode* target, txInteger selector, txInteger* address);
static txInteger fxCoderUseTemporaryVariable(txCoder* self);
static void fxCoderUnuseTemporaryVariables(txCoder* self, txInteger count);

static void fxScopeCoded(txScope* self, txCoder* coder);
static void fxScopeCodedBody(txScope* self, txCoder* coder);
static void fxScopeCodingBlock(txScope* self, txCoder* coder);
static void fxScopeCodingBody(txScope* self, txCoder* coder);
static void fxScopeCodingParams(txScope* self, txCoder* coder);
static void fxScopeCodingProgram(txScope* self, txCoder* coder);
static void fxScopeCodeDefineNodes(txScope* self, txCoder* coder);
static void fxScopeCodeRefresh(txScope* self, txCoder* coder);
static void fxScopeCodeReset(txScope* self, txCoder* coder);
static void fxScopeCodeRetrieve(txScope* self, txCoder* coder);
static void fxScopeCodeStore(txScope* self, txCoder* coder);
static void fxScopeCodeStoreAll(txScope* self, txCoder* coder);

static void fxNodeDispatchCode(void* it, void* param);
static void fxNodeDispatchCodeAssign(void* it, void* param);

static void fxAccessNodeCodeCompound(void* it, void* param, txAssignNode* compound);
static void fxAccessNodeCodeDelete(void* it, void* param);
static void fxAccessNodeCodePostfix(void* it, void* param, txPostfixExpressionNode* compound);
static void fxBindingNodeCodeDefault(void* it, void* param);
static void fxMemberNodeCodeCall(void* it, void* param);
static void fxMemberNodeCodeCompound(void* it, void* param, txAssignNode* compound);
static void fxMemberNodeCodeDelete(void* it, void* param);
static void fxMemberNodeCodePostfix(void* it, void* param, txPostfixExpressionNode* compound);
static void fxMemberAtNodeCodeCall(void* it, void* param);
static void fxMemberAtNodeCodeCompound(void* it, void* param, txAssignNode* compound);
static void fxMemberAtNodeCodeDelete(void* it, void* param);
static void fxMemberAtNodeCodePostfix(void* it, void* param, txPostfixExpressionNode* compound);
static txInteger fxParamsNodeCode(void* it, void* param);
static void fxSpreadNodeCode(void* it, void* param, txInteger counter);

txScript* fxParserCode(txParser* parser)
{
	txCoder coder;
	txByteCode* code;
	txScript* script;
	txInteger size, delta, offset;
	txSymbol* symbol;
	txSymbol** address;
	txSize c, i;
	txID id;
	txSize total;
	txByte* p;
	txHostNode* node;

    c_memset(&coder, 0, sizeof(txCoder));
	coder.parser = parser;
	if (parser->errorCount == 0)
		fxNodeDispatchCode(parser->root, &coder);
	if (parser->errorCount) {
		coder.firstCode = NULL;
		coder.lastCode = NULL;
		fxCoderAddString(&coder, 1, XS_CODE_STRING_1, c_strlen(parser->errorMessage), parser->errorMessage);
		fxCoderAddInteger(&coder, 1, XS_CODE_INTEGER_1, 1);
		fxCoderAddByte(&coder, 1, XS_CODE_GLOBAL);
		fxCoderAddSymbol(&coder, 0, XS_CODE_GET_VARIABLE, parser->errorSymbol);
		fxCoderAddByte(&coder, -2, XS_CODE_NEW);
		fxCoderAddByte(&coder, -1, XS_CODE_THROW);
	}
	
	script = c_malloc(sizeof(txScript));
	if (!script) goto bail;
	c_memset(script, 0, sizeof(txScript));
	
	code = coder.firstCode;
	size = 0;
	delta = 0;
	while (code) {
		txInteger value;
		switch (code->id) {
		case XS_NO_CODE:
			((txTargetCode*)code)->offset = size;
			break;
		case XS_CODE_BRANCH_1:
		case XS_CODE_BRANCH_ELSE_1:
		case XS_CODE_BRANCH_IF_1:
		case XS_CODE_BRANCH_STATUS_1:
		case XS_CODE_CATCH_1:
		case XS_CODE_CODE_1:
			size += 2;
			delta += 3;
			break;
			
		case XS_CODE_ARGUMENT:
		case XS_CODE_ARGUMENTS:
		case XS_CODE_ARGUMENTS_SLOPPY:
		case XS_CODE_ARGUMENTS_STRICT:
		case XS_CODE_BEGIN_SLOPPY:
		case XS_CODE_BEGIN_STRICT:
		case XS_CODE_BEGIN_STRICT_BASE:
		case XS_CODE_BEGIN_STRICT_DERIVED:
		case XS_CODE_NEW_PROPERTY:
			size += 2;
			break;

		case XS_CODE_ASYNC_FUNCTION:
		case XS_CODE_CONSTRUCTOR_FUNCTION:
		case XS_CODE_DELETE_PROPERTY:
		case XS_CODE_DELETE_SUPER:
		case XS_CODE_FILE:
		case XS_CODE_FUNCTION:
		case XS_CODE_GENERATOR_FUNCTION:
		case XS_CODE_GET_PROPERTY:
		case XS_CODE_GET_SUPER:
		case XS_CODE_GET_VARIABLE:
		case XS_CODE_EVAL_REFERENCE:
		case XS_CODE_INTRINSIC:
		case XS_CODE_LINE:
		case XS_CODE_NAME:
		case XS_CODE_NEW_CLOSURE:
		case XS_CODE_NEW_LOCAL:
		case XS_CODE_PROGRAM_REFERENCE:
		case XS_CODE_SET_PROPERTY:
		case XS_CODE_SET_SUPER:
		case XS_CODE_SET_VARIABLE:
		case XS_CODE_SYMBOL:
			size += 3;
			break;
	
		case XS_CODE_STRING_1:
			size += ((txStringCode*)code)->length;
			// continue
		case XS_CODE_RESERVE_1:
		case XS_CODE_RETRIEVE_1:
		case XS_CODE_UNWIND_1:
			value = ((txIndexCode*)code)->index;
			if (value > 65535) {
				code->id += 2;
				size += 5;
			}
			else if (value > 255) {
				code->id += 1;
				size += 3;
			}
			else
				size += 2;
			break;
			
		case XS_CODE_CONST_CLOSURE_1:
		case XS_CODE_CONST_LOCAL_1:
		case XS_CODE_GET_CLOSURE_1:
		case XS_CODE_GET_LOCAL_1:
		case XS_CODE_LET_CLOSURE_1:
		case XS_CODE_LET_LOCAL_1:
		case XS_CODE_PULL_CLOSURE_1:
		case XS_CODE_PULL_LOCAL_1:
		case XS_CODE_REFRESH_CLOSURE_1:
		case XS_CODE_REFRESH_LOCAL_1:
		case XS_CODE_RESET_CLOSURE_1:
		case XS_CODE_RESET_LOCAL_1:
		case XS_CODE_SET_CLOSURE_1:
		case XS_CODE_SET_LOCAL_1:
		case XS_CODE_STORE_1:
		case XS_CODE_VAR_CLOSURE_1:
		case XS_CODE_VAR_LOCAL_1:
			value = ((txIndexCode*)code)->index + 2;
			if (value > 65535) {
				code->id += 2;
				size += 5;
			}
			else if (value > 255) {
				code->id += 1;
				size += 3;
			}
			else
				size += 2;
			break;
			
		case XS_CODE_INTEGER_1: 
			value = ((txIntegerCode*)code)->integer;
			if ((value < -32768) || (value > 32767)) {
				code->id += 2;
				size += 5;
			}
			else if ((value < -128) || (value > 127)) {
				code->id += 1;
				size += 3;
			}
			else
				size += 2;
			break;
		case XS_CODE_NUMBER:
			size += 9;
			break;
			
		case XS_CODE_HOST:
			size += 3;
			break;
			
		default:
			size++;
			break;
		}
		code = code->nextCode;
	}	
	
	code = coder.firstCode;
	size = 0;
	while (code) {
		switch (code->id) {
		case XS_NO_CODE:
			((txTargetCode*)code)->offset = size;
			break;
		case XS_CODE_BRANCH_1:
		case XS_CODE_BRANCH_ELSE_1:
		case XS_CODE_BRANCH_IF_1:
		case XS_CODE_BRANCH_STATUS_1:
		case XS_CODE_CATCH_1:
		case XS_CODE_CODE_1:
			offset = ((txBranchCode*)code)->target->offset - (size + 5);
			if ((offset < -32768) || (offset + delta > 32767)) {
				code->id += 2;
				size += 5;
			}
			else if ((offset < -128) || (offset + delta > 127)) {
				code->id += 1;
				delta -= 2;
				size += 3;
			}
			else {
				delta -= 3;
				size += 2;
			}
			break;
			
		case XS_CODE_ARGUMENT:
		case XS_CODE_ARGUMENTS:
		case XS_CODE_ARGUMENTS_SLOPPY:
		case XS_CODE_ARGUMENTS_STRICT:
		case XS_CODE_BEGIN_SLOPPY:
		case XS_CODE_BEGIN_STRICT:
		case XS_CODE_BEGIN_STRICT_BASE:
		case XS_CODE_BEGIN_STRICT_DERIVED:
		case XS_CODE_NEW_PROPERTY:
			size += 2;
			break;
		case XS_CODE_INTRINSIC:
		case XS_CODE_LINE:
			size += 3;
			break;
		case XS_CODE_ASYNC_FUNCTION:
		case XS_CODE_CONSTRUCTOR_FUNCTION:
		case XS_CODE_DELETE_PROPERTY:
		case XS_CODE_DELETE_SUPER:
		case XS_CODE_FILE:
		case XS_CODE_FUNCTION:
		case XS_CODE_GENERATOR_FUNCTION:
		case XS_CODE_GET_PROPERTY:
		case XS_CODE_GET_SUPER:
		case XS_CODE_GET_VARIABLE:
		case XS_CODE_EVAL_REFERENCE:
		case XS_CODE_NAME:
		case XS_CODE_NEW_CLOSURE:
		case XS_CODE_NEW_LOCAL:
		case XS_CODE_PROGRAM_REFERENCE:
		case XS_CODE_SET_PROPERTY:
		case XS_CODE_SET_SUPER:
		case XS_CODE_SET_VARIABLE:
		case XS_CODE_SYMBOL:
			symbol = ((txSymbolCode*)code)->symbol;
			if (symbol)
				symbol->usage++;
			size += 3;
			break;
			
		case XS_CODE_CONST_CLOSURE_1:
		case XS_CODE_CONST_LOCAL_1:
		case XS_CODE_GET_CLOSURE_1:
		case XS_CODE_GET_LOCAL_1:
		case XS_CODE_LET_CLOSURE_1:
		case XS_CODE_LET_LOCAL_1:
		case XS_CODE_PULL_CLOSURE_1:
		case XS_CODE_PULL_LOCAL_1:
		case XS_CODE_REFRESH_CLOSURE_1:
		case XS_CODE_REFRESH_LOCAL_1:
		case XS_CODE_RESERVE_1:
		case XS_CODE_RESET_CLOSURE_1:
		case XS_CODE_RESET_LOCAL_1:
		case XS_CODE_RETRIEVE_1:
		case XS_CODE_SET_CLOSURE_1:
		case XS_CODE_SET_LOCAL_1:
		case XS_CODE_STORE_1:
		case XS_CODE_VAR_CLOSURE_1:
		case XS_CODE_VAR_LOCAL_1:
		case XS_CODE_UNWIND_1:
			size += 2;
			break;
		case XS_CODE_CONST_CLOSURE_2:
		case XS_CODE_CONST_LOCAL_2:
		case XS_CODE_GET_CLOSURE_2:
		case XS_CODE_GET_LOCAL_2:
		case XS_CODE_LET_CLOSURE_2:
		case XS_CODE_LET_LOCAL_2:
		case XS_CODE_PULL_CLOSURE_2:
		case XS_CODE_PULL_LOCAL_2:
		case XS_CODE_REFRESH_CLOSURE_2:
		case XS_CODE_REFRESH_LOCAL_2:
		case XS_CODE_RESERVE_2:
		case XS_CODE_RESET_CLOSURE_2:
		case XS_CODE_RESET_LOCAL_2:
		case XS_CODE_RETRIEVE_2:
		case XS_CODE_SET_CLOSURE_2:
		case XS_CODE_SET_LOCAL_2:
		case XS_CODE_STORE_2:
		case XS_CODE_VAR_CLOSURE_2:
		case XS_CODE_VAR_LOCAL_2:
		case XS_CODE_UNWIND_2:
			size += 3;
			break;
		
		case XS_CODE_INTEGER_1: 
			size += 2;
			break;
		case XS_CODE_INTEGER_2: 
			size += 3;
			break;
		case XS_CODE_INTEGER_4: 
			size += 5;
			break;
		case XS_CODE_NUMBER:
			size += 9;
			break;
		case XS_CODE_STRING_1:
			size += 2 + ((txStringCode*)code)->length;
			break;
		case XS_CODE_STRING_2:
			size += 3 + ((txStringCode*)code)->length;
			break;
			
		case XS_CODE_HOST:
			size += 3;
			break;
		
		default:
			size++;
			break;
		}
		code = code->nextCode;
	}	
	
	node = parser->firstHostNode;
	while (node) {
		if (node->symbol)
			node->symbol->usage++;
		node = node->nextHostNode;
	}
	
	address = parser->symbolTable;
	c = parser->symbolModulo;
	id = 0;
	total = 2;
	for (i = 0; i < c; i++) {
		txSymbol* symbol = *address;
		while (symbol) {
			symbol->ID = id;
			if (symbol->usage) {
				id++;
				total += symbol->length;
			}
			symbol = symbol->next;
		}
		address++;
	}
		
	script->codeBuffer = c_malloc(size);
	if (!script->codeBuffer) goto bail;
	script->codeSize = size;
	
	code = coder.firstCode;
	p = script->codeBuffer;
	while (code) {
		txS1 s1; txS2 s2; txS4 s4; 
		txU1 u1; txU2 u2;
		txNumber n;
		if (code->id)
			*p++ = (txS1)(code->id);
		switch (code->id) {
		case XS_CODE_BRANCH_1:
		case XS_CODE_BRANCH_ELSE_1:
		case XS_CODE_BRANCH_IF_1:
		case XS_CODE_BRANCH_STATUS_1:
		case XS_CODE_CATCH_1:
		case XS_CODE_CODE_1:
			offset = p + 1 - script->codeBuffer;
			s1 = (txS1)(((txBranchCode*)code)->target->offset - offset);
			*p++ = s1;
			break;
		case XS_CODE_BRANCH_2:
		case XS_CODE_BRANCH_ELSE_2:
		case XS_CODE_BRANCH_IF_2:
		case XS_CODE_BRANCH_STATUS_2:
		case XS_CODE_CATCH_2:
		case XS_CODE_CODE_2:
			offset = p + 2 - script->codeBuffer;
			s2 = (txS2)(((txBranchCode*)code)->target->offset - offset);
			mxEncode2(p, s2);
			break;
		case XS_CODE_BRANCH_4:
		case XS_CODE_BRANCH_ELSE_4:
		case XS_CODE_BRANCH_IF_4:
		case XS_CODE_BRANCH_STATUS_4:
		case XS_CODE_CATCH_4:
		case XS_CODE_CODE_4:
			offset = p + 4 - script->codeBuffer;
			s4 = (txS4)(((txBranchCode*)code)->target->offset  - offset);
			mxEncode4(p, s4);
			break;
			
		case XS_CODE_ASYNC_FUNCTION:
		case XS_CODE_CONSTRUCTOR_FUNCTION:
		case XS_CODE_DELETE_PROPERTY:
		case XS_CODE_DELETE_SUPER:
		case XS_CODE_FILE:
		case XS_CODE_FUNCTION:
		case XS_CODE_GENERATOR_FUNCTION:
		case XS_CODE_GET_PROPERTY:
		case XS_CODE_GET_SUPER:
		case XS_CODE_GET_VARIABLE:
		case XS_CODE_EVAL_REFERENCE:
		case XS_CODE_NAME:
		case XS_CODE_NEW_CLOSURE:
		case XS_CODE_NEW_LOCAL:
		case XS_CODE_PROGRAM_REFERENCE:
		case XS_CODE_SET_PROPERTY:
		case XS_CODE_SET_SUPER:
		case XS_CODE_SET_VARIABLE:
		case XS_CODE_SYMBOL:
			symbol = ((txSymbolCode*)code)->symbol;
			if (symbol)
				s2 = (txS2)symbol->ID;
			else
				s2 = -1;
			mxEncode2(p, s2);
			break;
			
		case XS_CODE_NEW_PROPERTY:
			u1 = ((txFlagCode*)code)->flag;
			*((txU1*)p++) = u1;
			break;
			
		case XS_CODE_ARGUMENT:
		case XS_CODE_ARGUMENTS:
		case XS_CODE_ARGUMENTS_SLOPPY:
		case XS_CODE_ARGUMENTS_STRICT:
		case XS_CODE_BEGIN_SLOPPY:
		case XS_CODE_BEGIN_STRICT:
		case XS_CODE_BEGIN_STRICT_BASE:
		case XS_CODE_BEGIN_STRICT_DERIVED:
		case XS_CODE_RESERVE_1:
		case XS_CODE_RETRIEVE_1:
		case XS_CODE_UNWIND_1:
			u1 = (txU1)(((txIndexCode*)code)->index);
			*((txU1*)p++) = u1;
			break;
		case XS_CODE_INTRINSIC:
		case XS_CODE_LINE:
		case XS_CODE_RESERVE_2:
		case XS_CODE_RETRIEVE_2:
		case XS_CODE_UNWIND_2:
			u2 = (txU2)(((txIndexCode*)code)->index);
			mxEncode2(p, u2);
			break;

		case XS_CODE_CONST_CLOSURE_1:
		case XS_CODE_CONST_LOCAL_1:
		case XS_CODE_GET_CLOSURE_1:
		case XS_CODE_GET_LOCAL_1:
		case XS_CODE_LET_CLOSURE_1:
		case XS_CODE_LET_LOCAL_1:
		case XS_CODE_PULL_CLOSURE_1:
		case XS_CODE_PULL_LOCAL_1:
		case XS_CODE_REFRESH_CLOSURE_1:
		case XS_CODE_REFRESH_LOCAL_1:
		case XS_CODE_RESET_CLOSURE_1:
		case XS_CODE_RESET_LOCAL_1:
		case XS_CODE_SET_CLOSURE_1:
		case XS_CODE_SET_LOCAL_1:
		case XS_CODE_STORE_1:
		case XS_CODE_VAR_CLOSURE_1:
		case XS_CODE_VAR_LOCAL_1:
			u1 = (txU1)(((txIndexCode*)code)->index + 2);
			*((txU1*)p++) = u1;
			break;

		case XS_CODE_CONST_CLOSURE_2:
		case XS_CODE_CONST_LOCAL_2:
		case XS_CODE_GET_CLOSURE_2:
		case XS_CODE_GET_LOCAL_2:
		case XS_CODE_LET_CLOSURE_2:
		case XS_CODE_LET_LOCAL_2:
		case XS_CODE_PULL_CLOSURE_2:
		case XS_CODE_PULL_LOCAL_2:
		case XS_CODE_REFRESH_CLOSURE_2:
		case XS_CODE_REFRESH_LOCAL_2:
		case XS_CODE_RESET_CLOSURE_2:
		case XS_CODE_RESET_LOCAL_2:
		case XS_CODE_SET_CLOSURE_2:
		case XS_CODE_SET_LOCAL_2:
		case XS_CODE_STORE_2:
		case XS_CODE_VAR_CLOSURE_2:
		case XS_CODE_VAR_LOCAL_2:
			u2 = (txU2)(((txIndexCode*)code)->index + 2);
			mxEncode2(p, u2);
			break;
	
		case XS_CODE_INTEGER_1: 
			s1 = (txS1)(((txIntegerCode*)code)->integer);
			*p++ = s1;
			break;
		case XS_CODE_INTEGER_2: 
			s2 = (txS2)(((txIntegerCode*)code)->integer);
			mxEncode2(p, s2);
			break;
		case XS_CODE_INTEGER_4: 
			s4 = (txS4)(((txIntegerCode*)code)->integer);
			mxEncode4(p, s4);
			break;
		case XS_CODE_NUMBER:
			n = ((txNumberCode*)code)->number;
			mxEncode8(p, n);
			break;
		case XS_CODE_STRING_1:
			u1 = (txU1)(((txStringCode*)code)->length);
			*((txU1*)p++) = u1;
			c_memcpy(p, ((txStringCode*)code)->string, u1);
			p += u1;
			break;
		case XS_CODE_STRING_2:
			u2 = (txU2)(((txStringCode*)code)->length);
			mxEncode2(p, u2);
			c_memcpy(p, ((txStringCode*)code)->string, u2);
			p += u2;
			break;
			
		case XS_CODE_HOST:
			u2 = (txU2)(((txIndexCode*)code)->index);
			mxEncode2(p, u2);
			break;
		}
		code = code->nextCode;
	}	
	
#ifdef mxCodePrint
	fprintf(stderr, "\n");
	code = coder.firstCode;
	while (code) {
		txInteger tab;
		for (tab = 0; tab < code->stackLevel; tab++)
			fprintf(stderr, "\t");
		switch (code->id) {
		case XS_NO_CODE:
			fprintf(stderr, "_%d\n", ((txTargetCode*)code)->index);
			break;
		
		case XS_CODE_BRANCH_1:
		case XS_CODE_BRANCH_2:
		case XS_CODE_BRANCH_4:
		case XS_CODE_BRANCH_ELSE_1:
		case XS_CODE_BRANCH_ELSE_2:
		case XS_CODE_BRANCH_ELSE_4:
		case XS_CODE_BRANCH_IF_1:
		case XS_CODE_BRANCH_IF_2:
		case XS_CODE_BRANCH_IF_4:
		case XS_CODE_BRANCH_STATUS_1:
		case XS_CODE_BRANCH_STATUS_2:
		case XS_CODE_BRANCH_STATUS_4:
		case XS_CODE_CATCH_1:
		case XS_CODE_CATCH_2:
		case XS_CODE_CATCH_4:
		case XS_CODE_CODE_1:
		case XS_CODE_CODE_2:
		case XS_CODE_CODE_4:
			fprintf(stderr, "%s _%d\n", gxCodeNames[code->id], ((txBranchCode*)code)->target->index);
			break;
		
		case XS_CODE_ARGUMENT:
		case XS_CODE_ARGUMENTS:
		case XS_CODE_ARGUMENTS_SLOPPY:
		case XS_CODE_ARGUMENTS_STRICT:
		case XS_CODE_BEGIN_SLOPPY:
		case XS_CODE_BEGIN_STRICT:
		case XS_CODE_BEGIN_STRICT_BASE:
		case XS_CODE_BEGIN_STRICT_DERIVED:
		case XS_CODE_INTRINSIC:
		case XS_CODE_LINE:
			fprintf(stderr, "%s %d\n", gxCodeNames[code->id], ((txIndexCode*)code)->index);
			break;
			
		case XS_CODE_ASYNC_FUNCTION:
		case XS_CODE_CONSTRUCTOR_FUNCTION:
		case XS_CODE_DELETE_PROPERTY:
		case XS_CODE_DELETE_SUPER:
		case XS_CODE_FILE:
		case XS_CODE_FUNCTION:
		case XS_CODE_GENERATOR_FUNCTION:
		case XS_CODE_GET_PROPERTY:
		case XS_CODE_GET_SUPER:
		case XS_CODE_GET_VARIABLE:
		case XS_CODE_EVAL_REFERENCE:
		case XS_CODE_NAME:
		case XS_CODE_PROGRAM_REFERENCE:
		case XS_CODE_SET_PROPERTY:
		case XS_CODE_SET_SUPER:
		case XS_CODE_SET_VARIABLE:
		case XS_CODE_SYMBOL:
			fprintf(stderr, "%s %s\n", gxCodeNames[code->id], ((txSymbolCode*)code)->symbol ? ((txSymbolCode*)code)->symbol->string : "?");
			break;
		
		case XS_CODE_NEW_PROPERTY:
			fprintf(stderr, "%s ", gxCodeNames[code->id]);
			if (((txFlagCode*)code)->flag & XS_DONT_DELETE_FLAG)
				fprintf(stderr, "C");
			else
				fprintf(stderr, "c");
			if (((txFlagCode*)code)->flag  & XS_DONT_ENUM_FLAG)
				fprintf(stderr, "E");
			else
				fprintf(stderr, "e");
			if (((txFlagCode*)code)->flag  & XS_DONT_SET_FLAG)
				fprintf(stderr, "W");
			else
				fprintf(stderr, "w");
			if (((txFlagCode*)code)->flag  & XS_GETTER_FLAG)
				fprintf(stderr, " getter");
			if (((txFlagCode*)code)->flag  & XS_SETTER_FLAG)
				fprintf(stderr, " setter");
			fprintf(stderr, "\n");
			break;
		
		case XS_CODE_CONST_CLOSURE_1:
		case XS_CODE_CONST_CLOSURE_2:
		case XS_CODE_CONST_LOCAL_1:
		case XS_CODE_CONST_LOCAL_2:
		case XS_CODE_GET_CLOSURE_1:
		case XS_CODE_GET_CLOSURE_2:
		case XS_CODE_GET_LOCAL_1:
		case XS_CODE_GET_LOCAL_2:
		case XS_CODE_LET_CLOSURE_1:
		case XS_CODE_LET_CLOSURE_2:
		case XS_CODE_LET_LOCAL_1:
		case XS_CODE_LET_LOCAL_2:
		case XS_CODE_PULL_CLOSURE_1:
		case XS_CODE_PULL_CLOSURE_2:
		case XS_CODE_PULL_LOCAL_1:
		case XS_CODE_PULL_LOCAL_2:
		case XS_CODE_REFRESH_CLOSURE_1:
		case XS_CODE_REFRESH_CLOSURE_2:
		case XS_CODE_REFRESH_LOCAL_1:
		case XS_CODE_REFRESH_LOCAL_2:
		case XS_CODE_RESET_CLOSURE_1:
		case XS_CODE_RESET_CLOSURE_2:
		case XS_CODE_RESET_LOCAL_1:
		case XS_CODE_RESET_LOCAL_2:
		case XS_CODE_SET_CLOSURE_1:
		case XS_CODE_SET_CLOSURE_2:
		case XS_CODE_SET_LOCAL_1:
		case XS_CODE_SET_LOCAL_2:
		case XS_CODE_STORE_1:
		case XS_CODE_STORE_2:
		case XS_CODE_VAR_CLOSURE_1:
		case XS_CODE_VAR_CLOSURE_2:
		case XS_CODE_VAR_LOCAL_1:
		case XS_CODE_VAR_LOCAL_2:
			fprintf(stderr, "%s [%d]\n", gxCodeNames[code->id], ((txIndexCode*)code)->index);
			break;
		
		case XS_CODE_RESERVE_1:
		case XS_CODE_RESERVE_2:
		case XS_CODE_UNWIND_1:
		case XS_CODE_UNWIND_2:
			fprintf(stderr, "%s #%d\n", gxCodeNames[code->id], ((txIndexCode*)code)->index);
			break;
		case XS_CODE_NEW_CLOSURE:
		case XS_CODE_NEW_LOCAL:
			fprintf(stderr, "[%d] %s %s\n", ((txVariableCode*)code)->index, gxCodeNames[code->id], ((txSymbolCode*)code)->symbol->string);
			break;
		case XS_CODE_NEW_TEMPORARY:
			fprintf(stderr, "[%d] %s\n", ((txIndexCode*)code)->index, gxCodeNames[code->id]);
			break;
		case XS_CODE_RETRIEVE_1:
		case XS_CODE_RETRIEVE_2:
			{
				txInteger i, c = ((txIndexCode*)code)->index;
				fprintf(stderr, "[0");
				for (i = 1; i < c; i++)
					fprintf(stderr, ",%d", i);
				fprintf(stderr, "] %s\n", gxCodeNames[code->id]);
			}
			break;
		
		case XS_CODE_INTEGER_1: 
		case XS_CODE_INTEGER_2: 
		case XS_CODE_INTEGER_4: 
			fprintf(stderr, "%s %d\n", gxCodeNames[code->id], ((txIntegerCode*)code)->integer);
			break;
		case XS_CODE_NUMBER:
			fprintf(stderr, "%s %lf\n", gxCodeNames[code->id], ((txNumberCode*)code)->number);
			break;
		case XS_CODE_STRING_1:
		case XS_CODE_STRING_2:
			fprintf(stderr, "%s %d \"%s\"\n", gxCodeNames[code->id], ((txStringCode*)code)->length, ((txStringCode*)code)->string);
			break;
			
		case XS_CODE_HOST:
			fprintf(stderr, "%s %d\n", gxCodeNames[code->id], ((txIndexCode*)code)->index);
			break;
	
		default:
			fprintf(stderr, "%s\n", gxCodeNames[code->id]);
			break;
		}
		code = code->nextCode;
	}
#endif
	script->symbolsBuffer = c_malloc(total);
	if (!script->symbolsBuffer) goto bail;
	script->symbolsSize = total;
	
	p = script->symbolsBuffer;
	mxEncode2(p, id);
	
	address = parser->symbolTable;
	c = parser->symbolModulo;
	for (i = 0; i < c; i++) {
		txSymbol* symbol = *address;
		while (symbol) {
			if (symbol->usage) {
				c_memcpy(p, symbol->string, symbol->length);
				p += symbol->length;
			}
			symbol = symbol->next;
		}
		address++;
	}
	
	c = (txS2)(parser->hostNodeIndex);
	if (c) {
		size = 2;
		node = parser->firstHostNode;
		while (node) {
			size += 3 + node->at->length + 1;
			node = node->nextHostNode;
		}
	
		script->hostsBuffer = c_malloc(size);
		if (!script->hostsBuffer) goto bail;
		script->hostsSize = size;
	
		p = script->hostsBuffer;
		mxEncode2(p, c);
		node = parser->firstHostNode;
		while (node) {
			*p++ = (txS1)(node->paramsCount);
			if (node->symbol)
				c = (txS2)node->symbol->ID;
			else
				c = -1;
			mxEncode2(p, c);
			c_memcpy(p, node->at->value, node->at->length);
			p += node->at->length;
			*p++ = 0;
			node = node->nextHostNode;
		}
	}

	return script;
bail:
	fxDeleteScript(script);
	return C_NULL;
}

void fxCoderAdd(txCoder* self, txInteger delta, void* it)
{
	txByteCode* code = it;
	if (self->lastCode)
		self->lastCode->nextCode = code;
	else
		self->firstCode = code;
	self->lastCode = code;
	self->stackLevel += delta;
	code->stackLevel = self->stackLevel;
	if (self->stackLevel < 0)
		fprintf(stderr, "oops\n");
}

void fxCoderAddBranch(txCoder* self, txInteger delta, txInteger id, txTargetCode* target)
{
	txBranchCode* code = fxNewParserChunkClear(self->parser, sizeof(txBranchCode));
	fxCoderAdd(self, delta, code);
	code->id = id;
	code->target = target;
}

void fxCoderAddByte(txCoder* self, txInteger delta, txInteger id)
{
	txByteCode* code = fxNewParserChunkClear(self->parser, sizeof(txByteCode));
	fxCoderAdd(self, delta, code);
	code->id = id;
}

void fxCoderAddFlag(txCoder* self, txInteger delta, txInteger id, txFlag flag)
{
	txFlagCode* code = fxNewParserChunkClear(self->parser, sizeof(txFlagCode));
	fxCoderAdd(self, delta, code);
	code->id = id;
	code->flag = flag;
}

void fxCoderAddIndex(txCoder* self, txInteger delta, txInteger id, txInteger index)
{
	txIndexCode* code = fxNewParserChunkClear(self->parser, sizeof(txIndexCode));
	fxCoderAdd(self, delta, code);
	code->id = id;
	code->index = index;
}

void fxCoderAddInteger(txCoder* self, txInteger delta, txInteger id, txInteger integer)
{
	txIntegerCode* code = fxNewParserChunkClear(self->parser, sizeof(txIntegerCode));
	fxCoderAdd(self, delta, code);
	code->id = id;
	code->integer = integer;
}

void fxCoderAddLine(txCoder* self, txInteger delta, txInteger id, txNode* node)
{
	if (self->parser->flags & mxDebugFlag) {
		if (self->parser->lines) {
			node->path = self->parser->path;
			node->line = self->parser->lines[node->line];
		}
		if (self->path != node->path) {
			if (node->path) {
				fxCoderAddSymbol(self, 0, XS_CODE_FILE, node->path);
				fxCoderAddIndex(self, 0, id, node->line);
			}
			self->path = node->path;
			self->line = node->line;
		}
		else if (self->line != node->line) {
			if (self->path) {
				txIndexCode* code = (txIndexCode*)self->lastCode;
				if (code && (code->id == id))
					code->index = node->line;
				else
					fxCoderAddIndex(self, 0, id, node->line);
			}
			self->line = node->line;
		}
	}
}

void fxCoderAddNumber(txCoder* self, txInteger delta, txInteger id, txNumber number)
{
	txNumberCode* code = fxNewParserChunkClear(self->parser, sizeof(txNumberCode));
	fxCoderAdd(self, delta, code);
	code->id = id;
	code->number = number;
}

void fxCoderAddString(txCoder* self, txInteger delta, txInteger id, txInteger length, txString string)
{
	txStringCode* code = fxNewParserChunkClear(self->parser, sizeof(txStringCode));
	fxCoderAdd(self, delta, code);
	code->id = id;
	code->length = length + 1;
	code->string = string;
}

void fxCoderAddSymbol(txCoder* self, txInteger delta, txInteger id, txSymbol* symbol)
{
	txSymbolCode* code = fxNewParserChunkClear(self->parser, sizeof(txSymbolCode));
	fxCoderAdd(self, delta, code);
	code->id = id;
	code->symbol = symbol;
}

void fxCoderAddVariable(txCoder* self, txInteger delta, txInteger id, txSymbol* symbol, txInteger index)
{
	txVariableCode* code = fxNewParserChunkClear(self->parser, sizeof(txVariableCode));
	fxCoderAdd(self, delta, code);
	code->id = id;
	code->symbol = symbol;
	code->index = index;
}

void fxCoderAdjustEnvironment(txCoder* self, txTargetCode* target)
{
	txInteger count = self->environmentLevel - target->environmentLevel;
	while (count) {
		fxCoderAddByte(self, 0, XS_CODE_WITHOUT);
		count--;
	}
}

void fxCoderAdjustScope(txCoder* self, txTargetCode* target)
{
	txInteger count = self->scopeLevel - target->scopeLevel;
	if (count)
		fxCoderAddIndex(self, 0, XS_CODE_UNWIND_1, count);
}

txTargetCode* fxCoderAliasTargets(txCoder* self, txTargetCode* target)
{
	txTargetCode* result = NULL;
	if (target) {
		txTargetCode* alias = result = fxNewParserChunkClear(self->parser, sizeof(txTargetCode));
		alias->index = self->targetIndex++;
		alias->label = target->label;
		alias->environmentLevel = self->environmentLevel;
		alias->scopeLevel = self->scopeLevel;
		alias->stackLevel = self->stackLevel;
		alias->original = target;
		target = target->nextTarget;
		while (target) {
			alias = alias->nextTarget = fxNewParserChunkClear(self->parser, sizeof(txTargetCode));
			alias->index = self->targetIndex++;
			alias->label = target->label;
			alias->environmentLevel = self->environmentLevel;
			alias->scopeLevel = self->scopeLevel;
			alias->stackLevel = self->stackLevel;
			alias->original = target;
			target = target->nextTarget;
		}
	}
	return result;
}

txInteger fxCoderCountParameters(txCoder* self, txNode* params)
{
	txNode* item = ((txParamsBindingNode*)params)->items->first;
	txInteger count = 0;
	while (item) {
		if (item->description->token == XS_TOKEN_REST_BINDING)
			break;
		if ((item->description->token == XS_TOKEN_ARG) && ((txBindingNode*)item)->initializer)
			break;
		count++;
		item = item->next;
	}
	return count;
}

txTargetCode* fxCoderCreateTarget(txCoder* self)
{
	txTargetCode* result = fxNewParserChunkClear(self->parser, sizeof(txTargetCode));
	result->index = self->targetIndex++;
	result->environmentLevel = self->environmentLevel;
	result->scopeLevel = self->scopeLevel;
	result->stackLevel = self->stackLevel;
	return result;
}

txTargetCode* fxCoderFinalizeTargets(txCoder* self, txTargetCode* alias, txInteger selector, txInteger* address, txTargetCode* finallyTarget)
{
	txTargetCode* result = NULL;
	txInteger selection = *address;
	if (alias) {
		result = alias->original;
		while (alias) {
			fxCoderAdd(self, 0, alias);
			fxCoderAddInteger(self, 1, XS_CODE_INTEGER_1, selection);
			fxCoderAddIndex(self, 0, XS_CODE_SET_LOCAL_1, selector);
			fxCoderAddByte(self, -1, XS_CODE_POP);
			fxCoderAddBranch(self, 0, XS_CODE_BRANCH_1, finallyTarget);
			alias = alias->nextTarget;
			selection++;
		}
	}
	*address = selection;
	return result;
}

void fxCoderJumpTargets(txCoder* self, txTargetCode* target, txInteger selector, txInteger* address)
{
	txInteger selection = *address;
	while (target) {
		txTargetCode* elseTarget = fxCoderCreateTarget(self);
		fxCoderAddInteger(self, 1, XS_CODE_INTEGER_1, selection);
		fxCoderAddIndex(self, 1, XS_CODE_GET_LOCAL_1, selector);
		fxCoderAddByte(self, -1, XS_CODE_STRICT_EQUAL);
		fxCoderAddBranch(self, -1, XS_CODE_BRANCH_ELSE_1, elseTarget);
		fxCoderAdjustEnvironment(self, target);
		fxCoderAdjustScope(self, target);
		fxCoderAddBranch(self, 0, XS_CODE_BRANCH_1, target);
		fxCoderAdd(self, 0, elseTarget);
		target = target->nextTarget;
		selection++;
	}
	*address = selection;
}

txInteger fxCoderUseTemporaryVariable(txCoder* self)
{
	txInteger result = self->scopeLevel++;
	fxCoderAddIndex(self, 0, XS_CODE_NEW_TEMPORARY, result);
	return result;
}

void fxCoderUnuseTemporaryVariables(txCoder* self, txInteger count)
{
	fxCoderAddIndex(self, 0, XS_CODE_UNWIND_1, count);
	self->scopeLevel -= count;
}

void fxScopeCoded(txScope* self, txCoder* coder) 
{
	if (self->declareNodeCount) {
		if (self->flags & mxEvalFlag) {
			coder->environmentLevel--;
			fxCoderAddByte(coder, 0, XS_CODE_WITHOUT);
		}
		fxCoderAddIndex(coder, 0, XS_CODE_UNWIND_1, self->declareNodeCount);
		coder->scopeLevel -= self->declareNodeCount;
	}
}

void fxScopeCodedBody(txScope* self, txCoder* coder) 
{
	txScope* functionScope = self->scope;
	if ((functionScope->node->flags & mxEvalFlag) && !(functionScope->node->flags & mxStrictFlag)) {
		coder->environmentLevel--;
		fxCoderAddByte(coder, 0, XS_CODE_WITHOUT);
		coder->environmentLevel--;
		fxCoderAddByte(coder, 0, XS_CODE_WITHOUT);
		fxCoderAddIndex(coder, 0, XS_CODE_UNWIND_1, self->declareNodeCount);
		coder->scopeLevel -= self->declareNodeCount;
	}
	else 
		fxScopeCoded(self, coder);
}

void fxScopeCodingBlock(txScope* self, txCoder* coder) 
{
	if (self->declareNodeCount) {
		txDeclareNode* node = self->firstDeclareNode;
		while (node) {
			if (node->flags & mxDeclareNodeClosureFlag) {
				if (!(node->flags & mxDeclareNodeUseClosureFlag)) {
					node->index = coder->scopeLevel++;
					fxCoderAddVariable(coder, 0, XS_CODE_NEW_CLOSURE, node->symbol, node->index);
					if (node->description->token == XS_TOKEN_VAR) {
						fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
						fxCoderAddIndex(coder, 0, XS_CODE_VAR_CLOSURE_1, node->index);
						fxCoderAddByte(coder, -1, XS_CODE_POP);
					}
				}
			}
			else {
				node->index = coder->scopeLevel++;
				fxCoderAddVariable(coder, 0, XS_CODE_NEW_LOCAL, node->symbol, node->index);
				if (node->description->token == XS_TOKEN_VAR) {
					fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
					fxCoderAddIndex(coder, 0, XS_CODE_VAR_LOCAL_1, node->index);
					fxCoderAddByte(coder, -1, XS_CODE_POP);
				}
			}
			node = node->nextDeclareNode;
		}
		if (self->flags & mxEvalFlag) {
			fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
			fxCoderAddByte(coder, 0, XS_CODE_WITH);
			node = self->firstDeclareNode;
			while (node) {
				fxCoderAddIndex(coder, 0, XS_CODE_STORE_1, node->index);
				node = node->nextDeclareNode;
			}
			fxCoderAddByte(coder, -1, XS_CODE_POP);
			coder->environmentLevel++;
		}
	}
}

void fxScopeCodingBody(txScope* self, txCoder* coder) 
{
	if ((self->node->flags & mxEvalFlag) && !(self->node->flags & mxStrictFlag)) {
		txDeclareNode* node = self->firstDeclareNode;
		while (node) {
			if (node->description->token == XS_TOKEN_DEFINE) {
				node->index = coder->scopeLevel++;
				fxCoderAddVariable(coder, 0, XS_CODE_NEW_CLOSURE, node->symbol, node->index);
				fxCoderAddByte(coder, 1, XS_CODE_NULL);
				fxCoderAddIndex(coder, 0, XS_CODE_VAR_CLOSURE_1, node->index);
				fxCoderAddByte(coder, -1, XS_CODE_POP);
			}
			else if (node->description->token == XS_TOKEN_VAR) {
				node->index = coder->scopeLevel++;
				fxCoderAddVariable(coder, 0, XS_CODE_NEW_CLOSURE, node->symbol, node->index);
				fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
				fxCoderAddIndex(coder, 0, XS_CODE_VAR_CLOSURE_1, node->index);
				fxCoderAddByte(coder, -1, XS_CODE_POP);
			}
			node = node->nextDeclareNode;
		}
		fxCoderAddByte(coder, 1, XS_CODE_NULL);
		fxCoderAddByte(coder, 0, XS_CODE_WITH);
		node = self->scope->firstDeclareNode;
		while (node) {
			if ((node->description->token == XS_TOKEN_ARG) || (node->description->token == XS_TOKEN_VAR))
				fxCoderAddIndex(coder, 0, XS_CODE_STORE_1, node->index);
			node = node->nextDeclareNode;
		}
		node = self->firstDeclareNode;
		while (node) {
			if ((node->description->token == XS_TOKEN_DEFINE) || (node->description->token == XS_TOKEN_VAR))
				fxCoderAddIndex(coder, 0, XS_CODE_STORE_1, node->index);
			node = node->nextDeclareNode;
		}
		fxCoderAddByte(coder, -1, XS_CODE_POP);
		coder->environmentLevel++;
		node = self->firstDeclareNode;
		while (node) {
			if ((node->description->token != XS_TOKEN_DEFINE) && (node->description->token != XS_TOKEN_VAR)) {
				node->index = coder->scopeLevel++;
				fxCoderAddVariable(coder, 0, XS_CODE_NEW_CLOSURE, node->symbol, node->index);
			}
			node = node->nextDeclareNode;
		}
		fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(coder, 0, XS_CODE_WITH);
		node = self->firstDeclareNode;
		while (node) {
			if ((node->description->token != XS_TOKEN_DEFINE) && (node->description->token != XS_TOKEN_VAR))
				fxCoderAddIndex(coder, 0, XS_CODE_STORE_1, node->index);
			node = node->nextDeclareNode;
		}
		fxCoderAddByte(coder, -1, XS_CODE_POP);
		coder->environmentLevel++;
	}
	else 
		fxScopeCodingBlock(self, coder);
}

void fxScopeCodingEval(txScope* self, txCoder* coder) 
{
	txProgramNode* programNode = (txProgramNode*)self->node;
	txDeclareNode* node;
	if (self->flags & mxStrictFlag) {
		if (programNode->scopeCount) {
			fxCoderAddIndex(coder, 0, XS_CODE_RESERVE_1, programNode->scopeCount);
			fxScopeCodingBlock(self, coder);
		}
	}
	else {
		txInteger count = 0;
		node = self->firstDeclareNode;
		while (node) {
			if ((node->description->token == XS_TOKEN_DEFINE) || (node->description->token == XS_TOKEN_VAR))
				count++;
			node = node->nextDeclareNode;
		}
		if (count) {
			fxCoderAddIndex(coder, 0, XS_CODE_RESERVE_1, count);
			node = self->firstDeclareNode;
			while (node) {
				if (node->description->token == XS_TOKEN_DEFINE) {
					node->index = coder->scopeLevel++;
					fxCoderAddVariable(coder, 0, XS_CODE_NEW_LOCAL, node->symbol, node->index);
					fxCoderAddByte(coder, 1, XS_CODE_NULL);
					fxCoderAddIndex(coder, 0, XS_CODE_VAR_LOCAL_1, node->index);
					fxCoderAddByte(coder, -1, XS_CODE_POP);
				}
				else if (node->description->token == XS_TOKEN_VAR) {
					node->index = coder->scopeLevel++;
					fxCoderAddVariable(coder, 0, XS_CODE_NEW_LOCAL, node->symbol, node->index);
					fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
					fxCoderAddIndex(coder, 0, XS_CODE_VAR_LOCAL_1, node->index);
					fxCoderAddByte(coder, -1, XS_CODE_POP);
				}
				node = node->nextDeclareNode;
			}
		}
		fxCoderAddByte(coder, 0, XS_CODE_EVAL_ENVIRONMENT);
		coder->scopeLevel = 0;
		if (programNode->scopeCount) {
			fxCoderAddIndex(coder, 0, XS_CODE_RESERVE_1, programNode->scopeCount);
			if (self->declareNodeCount) {
				node = self->firstDeclareNode;
				while (node) {
					if ((node->description->token != XS_TOKEN_DEFINE) && (node->description->token != XS_TOKEN_VAR)) {
						node->index = coder->scopeLevel++;
						if (node->flags & mxDeclareNodeClosureFlag) {
							fxCoderAddVariable(coder, 0, XS_CODE_NEW_CLOSURE, node->symbol, node->index);
						}
						else {
							fxCoderAddVariable(coder, 0, XS_CODE_NEW_LOCAL, node->symbol, node->index);
						}
					}
					node = node->nextDeclareNode;
				}
				if (self->flags & mxEvalFlag) {
					fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
					fxCoderAddByte(coder, 0, XS_CODE_WITH);
					node = self->firstDeclareNode;
					while (node) {
						if ((node->description->token != XS_TOKEN_DEFINE) && (node->description->token != XS_TOKEN_VAR))
							fxCoderAddIndex(coder, 0, XS_CODE_STORE_1, node->index);
						node = node->nextDeclareNode;
					}
					fxCoderAddByte(coder, -1, XS_CODE_POP);
					coder->environmentLevel++;
				}
			}
		}
	}
}

void fxScopeCodingParams(txScope* self, txCoder* coder) 
{
	txDeclareNode* node = self->firstDeclareNode;
	while (node) {
		txToken token = node->description->token;
		if ((token == XS_TOKEN_ARG) || (token == XS_TOKEN_VAR) || (token == XS_TOKEN_CONST)) {
			if (node->flags & mxDeclareNodeClosureFlag) {
				if (node->flags & mxDeclareNodeUseClosureFlag) {
					fxReportLineError(self->parser, node->line, "argument %s use closure", node->symbol->string);
				}
				node->index = coder->scopeLevel++;
				fxCoderAddVariable(coder, 0, XS_CODE_NEW_CLOSURE, node->symbol, node->index);
			}
			else {
				node->index = coder->scopeLevel++;
				fxCoderAddVariable(coder, 0, XS_CODE_NEW_LOCAL, node->symbol, node->index);
			}
		}
		node = node->nextDeclareNode;
	}
	if (self->flags & mxEvalFlag) {
		fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(coder, 0, XS_CODE_WITH);
		node = self->firstDeclareNode;
		while (node) {
			txToken token = node->description->token;
			if ((token == XS_TOKEN_ARG) || (token == XS_TOKEN_VAR) || (token == XS_TOKEN_CONST))
				fxCoderAddIndex(coder, 0, XS_CODE_STORE_1, node->index);
			node = node->nextDeclareNode;
		}
		fxCoderAddByte(coder, -1, XS_CODE_POP);
		coder->environmentLevel++;
	}
}

void fxScopeCodingProgram(txScope* self, txCoder* coder) 
{
	txProgramNode* programNode = (txProgramNode*)self->node;
	txDeclareNode* node;
	txInteger count = 0;
	if (programNode->variableCount) {
		fxCoderAddIndex(coder, 0, XS_CODE_RESERVE_1, programNode->variableCount);
		node = self->firstDeclareNode;
		while (node) {
			if ((node->description->token != XS_TOKEN_DEFINE) && (node->description->token != XS_TOKEN_VAR)) {
				node->index = coder->scopeLevel++;
				fxCoderAddVariable(coder, 0, XS_CODE_NEW_CLOSURE, node->symbol, node->index);
			}
			node = node->nextDeclareNode;
		}
		count = coder->scopeLevel;
		node = self->firstDeclareNode;
		while (node) {
			if (node->description->token == XS_TOKEN_DEFINE) {
				// closure -> global property
				node->index = coder->scopeLevel++;
				fxCoderAddVariable(coder, 0, XS_CODE_NEW_LOCAL, node->symbol, node->index);
				fxCoderAddByte(coder, 1, XS_CODE_NULL);
				fxCoderAddIndex(coder, 0, XS_CODE_VAR_LOCAL_1, node->index);
				fxCoderAddByte(coder, -1, XS_CODE_POP);
			}
			else if (node->description->token == XS_TOKEN_VAR) {
				// closure -> global property
				node->index = coder->scopeLevel++;
				fxCoderAddVariable(coder, 0, XS_CODE_NEW_LOCAL, node->symbol, node->index);
				fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
				fxCoderAddIndex(coder, 0, XS_CODE_VAR_LOCAL_1, node->index);
				fxCoderAddByte(coder, -1, XS_CODE_POP);
			}
			node = node->nextDeclareNode;
		}
		fxCoderAddByte(coder, 0, XS_CODE_PROGRAM_ENVIRONMENT);
		coder->scopeLevel = count;
	}
	if (programNode->scopeCount > count)
		fxCoderAddIndex(coder, 0, XS_CODE_RESERVE_1, programNode->scopeCount - count);
}

void fxScopeCodeDefineNodes(txScope* self, txCoder* coder) 
{
	txDefineNode* node = self->firstDefineNode;
	while (node) {
		fxDefineNodeCode(node, coder);
		node = node->nextDefineNode;
	}
}

txInteger fxScopeCodeSpecifierNodes(txScope* self, txCoder* coder) 
{
	txDeclareNode* node = self->firstDeclareNode;
	txInteger count = 0;
	while (node) {
		if (node->flags & mxDeclareNodeUseClosureFlag) {
			txSpecifierNode* specifier = node->importSpecifier;
			txInteger index = 3;
			if (node->symbol)
				fxCoderAddSymbol(coder, 1, XS_CODE_SYMBOL, node->symbol);
			else
				fxCoderAddByte(coder, 1, XS_CODE_NULL);
			if (specifier) {
				if (coder->parser->flags & mxDebugFlag) {
					fxCoderAddLine(coder, 0, XS_CODE_LINE, (txNode*)specifier);
				}
				fxStringNodeCode(specifier->from, coder);
				if (specifier->symbol)
					fxCoderAddSymbol(coder, 1, XS_CODE_SYMBOL, specifier->symbol);
				else
					fxCoderAddByte(coder, 1, XS_CODE_NULL);
			}
			else {
				fxCoderAddByte(coder, 1, XS_CODE_NULL);
				fxCoderAddByte(coder, 1, XS_CODE_NULL);
			}
			specifier = node->firstExportSpecifier;
			while (specifier) {
				if (specifier->asSymbol) {
					fxCoderAddSymbol(coder, 1, XS_CODE_SYMBOL, specifier->asSymbol);
					index++;
				}
				else if (specifier->symbol) {
					fxCoderAddSymbol(coder, 1, XS_CODE_SYMBOL, specifier->symbol);
					index++;
				}
				specifier = specifier->nextSpecifier;
			}
			fxCoderAddInteger(coder, 1, XS_CODE_INTEGER_1, index);
			fxCoderAddByte(coder, 0 - index, XS_CODE_TRANSFER);
			count++;
		}
		node = node->nextDeclareNode;
	}
	return count;
}

void fxScopeCodeRefresh(txScope* self, txCoder* coder) 
{
	txDeclareNode* node = self->firstDeclareNode;
	while (node) {
		if (node->flags & mxDeclareNodeClosureFlag)
			fxCoderAddIndex(coder, 0, XS_CODE_REFRESH_CLOSURE_1, node->index);
		else
			fxCoderAddIndex(coder, 0, XS_CODE_REFRESH_LOCAL_1, node->index);
		node = node->nextDeclareNode;
	}
}

void fxScopeCodeReset(txScope* self, txCoder* coder) 
{
	txDeclareNode* node = self->firstDeclareNode;
	while (node) {
		if (node->flags & mxDeclareNodeClosureFlag)
			fxCoderAddIndex(coder, 0, XS_CODE_RESET_CLOSURE_1, node->index);
		else
			fxCoderAddIndex(coder, 0, XS_CODE_RESET_LOCAL_1, node->index);
		node = node->nextDeclareNode;
	}
}

void fxScopeCodeRetrieve(txScope* self, txCoder* coder) 
{
	txDeclareNode* node;
	txInteger count = 0;
	node = self->firstDeclareNode;
	while (node) {
		if ((node->flags & mxDeclareNodeUseClosureFlag) && node->symbol) {
			node->index = coder->scopeLevel++;
			count++;
		}
		node = node->nextDeclareNode;
	}
	if (self->node->flags & mxArrowFlag) {
		fxCoderAddIndex(coder, 0, XS_CODE_RETRIEVE_1, count);
		fxCoderAddByte(coder, 0, XS_CODE_RETRIEVE_TARGET);
		fxCoderAddByte(coder, 0, XS_CODE_RETRIEVE_THIS);
	}
	else if (count)
		fxCoderAddIndex(coder, 0, XS_CODE_RETRIEVE_1, count);
	self->closureNodeCount = count;
}

void fxScopeCodeStore(txScope* self, txCoder* coder) 
{
	txDeclareNode* node = self->firstDeclareNode;
	txUnsigned flags = self->flags & mxEvalFlag;
	while (node) {
		if (node->flags & mxDeclareNodeUseClosureFlag) {
			fxCoderAddIndex(coder, 0, XS_CODE_STORE_1, node->declaration->index);
			node->declaration->flags |= flags;
		}
		node = node->nextDeclareNode;
	}
	if (self->node->flags & mxArrowFlag)
		fxCoderAddByte(coder, 0, XS_CODE_STORE_ARROW);
	if (self->flags & mxEvalFlag)
		fxScopeCodeStoreAll(self->scope, coder);
}

void fxScopeCodeStoreAll(txScope* self, txCoder* coder) 
{
	txScope* scope = self;
	while (scope) {
		txDeclareNode* node;
		if (scope->token == XS_TOKEN_WITH)
			break;
		node = scope->firstDeclareNode;
		while (node) {
			if (node->flags & mxEvalFlag)
				node->flags &= ~mxEvalFlag;
			else if ((!(node->flags & mxDeclareNodeUseClosureFlag)) && node->declaration)
				fxCoderAddIndex(coder, 0, XS_CODE_STORE_1, node->declaration->index);
			node = node->nextDeclareNode;
		}
		if ((scope->token == XS_TOKEN_FUNCTION) || (scope->token == XS_TOKEN_MODULE))
			break;
		scope = scope->scope;
	}
}

void fxNodeDispatchCode(void* it, void* param)
{
	txNode* self = it;
	txCoder* coder = param;
	if (self->line >= 0)
		fxCoderAddLine(coder, 0, XS_CODE_LINE, self); 
	(*self->description->dispatch->code)(it, param);
}

void fxNodeDispatchCodeAssign(void* it, void* param)
{
	txNode* self = it;
	txCoder* coder = param;
	if (self->line >= 0)
		fxCoderAddLine(coder, 0, XS_CODE_LINE, self); 
	(*self->description->dispatch->codeAssign)(self, param);
}

void fxNodeDispatchCodeReference(void* it, void* param)
{
	txNode* self = it;
	txCoder* coder = param;
	if (self->line >= 0)
		fxCoderAddLine(coder, 0, XS_CODE_LINE, self); 
	(*self->description->dispatch->codeReference)(self, param);
}

void fxNodeCode(void* it, void* param) 
{
	txNode* self = it;
	txCoder* coder = param;
	fxReportLineError(coder->parser, self->line, "no value");
	fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
}

void fxNodeCodeAssign(void* it, void* param) 
{
	txNode* self = it;
	txCoder* coder = param;
	fxReportLineError(coder->parser, self->line, "no reference");
}

void fxNodeCodeReference(void* it, void* param) 
{
}

void fxAccessNodeCode(void* it, void* param) 
{
	txAccessNode* self = it;
	txDeclareNode* declaration = self->declaration;
	if (!declaration) {
		fxAccessNodeCodeReference(it, param);
		fxCoderAddSymbol(param, 0, XS_CODE_GET_VARIABLE, self->symbol);
	}
	else
		fxCoderAddIndex(param, 1, (declaration->flags & mxDeclareNodeClosureFlag) ? XS_CODE_GET_CLOSURE_1 : XS_CODE_GET_LOCAL_1, declaration->index);
}

void fxAccessNodeCodeAssign(void* it, void* param) 
{
	txAccessNode* self = it;
	txDeclareNode* declaration = self->declaration;
	if (!declaration)
		fxCoderAddSymbol(param, -1, XS_CODE_SET_VARIABLE, self->symbol);
	else
		fxCoderAddIndex(param, 0, (declaration->flags & mxDeclareNodeClosureFlag) ? XS_CODE_SET_CLOSURE_1 : XS_CODE_SET_LOCAL_1, declaration->index);
}

void fxAccessNodeCodeCompound(void* it, void* param, txAssignNode* compound) 
{
	txAccessNode* self = it;
	txDeclareNode* declaration = self->declaration;
	if (!declaration) {
		fxAccessNodeCodeReference(it, param);
		fxCoderAddByte(param, 1, XS_CODE_DUB);
		fxCoderAddSymbol(param, 0, XS_CODE_GET_VARIABLE, self->symbol);
	}
	else
		fxCoderAddIndex(param, 1, (declaration->flags & mxDeclareNodeClosureFlag) ? XS_CODE_GET_CLOSURE_1 : XS_CODE_GET_LOCAL_1, declaration->index);
	fxNodeDispatchCode(compound->value, param);
	fxCoderAddByte(param, -1, compound->description->code);
	if (!declaration)
		fxCoderAddSymbol(param, -1, XS_CODE_SET_PROPERTY, self->symbol);
	else
		fxCoderAddIndex(param, 0, (declaration->flags & mxDeclareNodeClosureFlag) ? XS_CODE_SET_CLOSURE_1 : XS_CODE_SET_LOCAL_1, declaration->index);
}

void fxAccessNodeCodeDelete(void* it, void* param) 
{
	txAccessNode* self = it;
	txCoder* coder = param;
	txDeclareNode* declaration = self->declaration;
	if (self->flags & mxStrictFlag)
		fxReportLineError(coder->parser, self->line, "delete identifier (strict code)");
	if (!declaration) {
		fxAccessNodeCodeReference(it, param);
		fxCoderAddSymbol(param, 0, XS_CODE_DELETE_PROPERTY, self->symbol);
	}
	else
		fxCoderAddByte(param, 1, XS_CODE_FALSE);
}

void fxAccessNodeCodePostfix(void* it, void* param, txPostfixExpressionNode* compound) 
{
	txAccessNode* self = it;
	txDeclareNode* declaration = self->declaration;
	txInteger value;
	if (!declaration) {
		fxAccessNodeCodeReference(it, param);
		fxCoderAddByte(param, 1, XS_CODE_DUB);
		fxCoderAddSymbol(param, 0, XS_CODE_GET_VARIABLE, self->symbol);
	}
	else
		fxCoderAddIndex(param, 1, (declaration->flags & mxDeclareNodeClosureFlag) ? XS_CODE_GET_CLOSURE_1 : XS_CODE_GET_LOCAL_1, declaration->index);
	if (!(compound->flags & mxExpressionNoValue)) {
		value = fxCoderUseTemporaryVariable(param);
		fxCoderAddByte(param, 0, XS_CODE_PLUS);
		fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, value);
	}
	fxCoderAddByte(param, 0, compound->description->code);
	if (!declaration)
		fxCoderAddSymbol(param, -1, XS_CODE_SET_PROPERTY, self->symbol);
	else
		fxCoderAddIndex(param, 0, (declaration->flags & mxDeclareNodeClosureFlag) ? XS_CODE_SET_CLOSURE_1 : XS_CODE_SET_LOCAL_1, declaration->index);
	if (!(compound->flags & mxExpressionNoValue)) {
		fxCoderAddByte(param, -1, XS_CODE_POP);
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, value);
		fxCoderUnuseTemporaryVariables(param, 1);
	}
}

void fxAccessNodeCodeReference(void* it, void* param) 
{
	txAccessNode* self = it;
	txCoder* coder = param;
	txDeclareNode* declaration = self->declaration;
	if (!declaration) {
		if (coder->evalFlag)
			fxCoderAddSymbol(param, 1, XS_CODE_EVAL_REFERENCE, self->symbol);
		else
			fxCoderAddSymbol(param, 1, XS_CODE_PROGRAM_REFERENCE, self->symbol);
	}
}

void fxAndExpressionNodeCode(void* it, void* param) 
{
	txBinaryExpressionNode* self = it;
	txTargetCode* endTarget = fxCoderCreateTarget(param);
	self->right->flags |= (self->flags & mxTailRecursionFlag);
	fxNodeDispatchCode(self->left, param);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, endTarget);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxNodeDispatchCode(self->right, param);
	fxCoderAdd(param, 0, endTarget);
}

void fxArgumentsNodeCode(void* it, void* param) 
{
	fxCoderAddIndex(param, 1, XS_CODE_ARGUMENTS, 0);
}

void fxArrayNodeCode(void* it, void* param) 
{
	txArrayNode* self = it;
	txCoder* coder = param;
	txInteger array = fxCoderUseTemporaryVariable(param);
	fxCoderAddByte(param, 1, XS_CODE_ARRAY);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, array);
	if (self->items) {
		txNode* item = self->items->first;
		if (self->flags & mxSpreadFlag) {
			txInteger counter = fxCoderUseTemporaryVariable(param);
			fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
			fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, counter);
			while (item) {
				if (item->description->token == XS_TOKEN_SPREAD) {
					txInteger iterator = fxCoderUseTemporaryVariable(param);
					txInteger result = fxCoderUseTemporaryVariable(param);
					txTargetCode* nextTarget = fxCoderCreateTarget(param);
					txTargetCode* doneTarget = fxCoderCreateTarget(param);
					fxNodeDispatchCode(((txSpreadNode*)item)->expression, param);
					fxCoderAddByte(param, 0, XS_CODE_FOR_OF);
					fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, iterator);
					fxCoderAddByte(param, -1, XS_CODE_POP);
					fxCoderAdd(param, 0, nextTarget);
					fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
					fxCoderAddByte(param, 1, XS_CODE_DUB);
					fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->nextSymbol);
					fxCoderAddByte(param, -2, XS_CODE_CALL);
					fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, result);
					fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->doneSymbol);
					fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, doneTarget);
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, array);
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, counter);
					fxCoderAddByte(param, 0, XS_CODE_AT);
					fxCoderAddIndex(param, 0, XS_CODE_GET_LOCAL_1, result);
					fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->valueSymbol);
					fxCoderAddByte(param, -2, XS_CODE_SET_PROPERTY_AT);
					fxCoderAddByte(param, -1, XS_CODE_POP);
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, counter);
					fxCoderAddByte(param, 0, XS_CODE_INCREMENT);
					fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, counter);
					fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, nextTarget);
					fxCoderAdd(param, 1, doneTarget);
					fxCoderUnuseTemporaryVariables(param, 1);
				}
				else {
					if (item->description->token != XS_TOKEN_ELISION) {
						fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, array);
						fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, counter);
						fxCoderAddByte(param, 0, XS_CODE_AT);
						fxNodeDispatchCode(item, param);
						fxCoderAddByte(param, -2, XS_CODE_SET_PROPERTY_AT);
						fxCoderAddByte(param, -1, XS_CODE_POP);
					}
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, counter);
					fxCoderAddByte(param, 0, XS_CODE_INCREMENT);
					fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, counter);
				}
				item = item->next;
			}
			fxCoderUnuseTemporaryVariables(param, 1);
		}
		else {
			txInteger index = 0;
			txInteger count = self->items->length;
			fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, array);
			fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, count);
			fxCoderAddSymbol(param, -1, XS_CODE_SET_PROPERTY, coder->parser->lengthSymbol);
			fxCoderAddByte(param, -1, XS_CODE_POP);
			while (item) {
				if (item->description->token == XS_TOKEN_ELISION)
					break;
				item = item->next;
			}
			if (!item) {
				fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
				fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, array);
				fxCoderAddByte(param, 1, XS_CODE_DUB);
				fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->fillSymbol);
				fxCoderAddByte(param, -2, XS_CODE_CALL);
				fxCoderAddByte(param, -1, XS_CODE_POP);
			}
			item = self->items->first;
			while (item) {
				if (item->description->token != XS_TOKEN_ELISION) {
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, array);
					fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, index);
					fxCoderAddByte(param, 0, XS_CODE_AT);
					fxNodeDispatchCode(item, param);
					fxCoderAddByte(param, -2, XS_CODE_SET_PROPERTY_AT);
					fxCoderAddByte(param, -1, XS_CODE_POP);
				}
				item = item->next;
				index++;
			}
		}
	}
	fxCoderUnuseTemporaryVariables(param, 1);
}

void fxArrayBindingNodeCodeAssign(void* it, void* param) 
{
	txArrayBindingNode* self = it;
	txCoder* coder = param;
	txNode* item = self->items->first;
	txInteger iterator;
	txInteger done;
	txInteger rest;
	txInteger result;
	txInteger selector;
	txInteger selection;
	txTargetCode* catchTarget;
	txTargetCode* normalTarget;
	txTargetCode* finallyTarget;
	
	txTargetCode* doneTarget;
	txTargetCode* nextTarget;
	
	fxBindingNodeCodeDefault(it, param);
	
	iterator = fxCoderUseTemporaryVariable(param);
	done = fxCoderUseTemporaryVariable(param);
	selector = fxCoderUseTemporaryVariable(param);
	rest = fxCoderUseTemporaryVariable(param);
	result = fxCoderUseTemporaryVariable(param);
	
	coder->returnTarget = fxCoderAliasTargets(param, coder->returnTarget);
	catchTarget = fxCoderCreateTarget(param);
	normalTarget = fxCoderCreateTarget(param);
	finallyTarget = fxCoderCreateTarget(param);
	
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddByte(param, 0, XS_CODE_FOR_OF);
	fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, iterator);
	fxCoderAddByte(param, 1, XS_CODE_FALSE);
	fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, done);
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
	fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, selector);
	fxCoderAddBranch(param, 0, XS_CODE_CATCH_1, catchTarget);
	
	if (item) {
		while (item && (item->description->token != XS_TOKEN_REST_BINDING)) {
			doneTarget = fxCoderCreateTarget(param);
			nextTarget = fxCoderCreateTarget(param);
			
			if (item->description->token != XS_TOKEN_SKIP_BINDING)
				fxNodeDispatchCodeReference(item, param);
			fxCoderAddByte(param, 1, XS_CODE_TRUE);
			fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, done);
			fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
			fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
			fxCoderAddByte(param, 1, XS_CODE_DUB);
			fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->nextSymbol);
			fxCoderAddByte(param, -2, XS_CODE_CALL);
			fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
			if (item->description->token != XS_TOKEN_SKIP_BINDING) {
				fxCoderAddByte(param, 1, XS_CODE_DUB);
				fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->doneSymbol);
				fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, done);
				fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, doneTarget);
				fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->valueSymbol);
				fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, nextTarget);
				fxCoderAdd(param, 1, doneTarget);
				fxCoderAddByte(param, -1, XS_CODE_POP);
				fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
				fxCoderAdd(param, 1, nextTarget);
				fxNodeDispatchCodeAssign(item, param);
				fxCoderAddByte(param, -1, XS_CODE_POP);
			}
			else {
				fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->doneSymbol);
				fxCoderAddIndex(param, 0, XS_CODE_PULL_LOCAL_1, done);
			}
			item = item->next;
		}
		if (item) {
			nextTarget = fxCoderCreateTarget(param);
			doneTarget = fxCoderCreateTarget(param);
		
			fxNodeDispatchCodeReference(((txRestBindingNode*)item)->binding, param);
			fxCoderAddByte(param, 1, XS_CODE_ARRAY);
			fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, rest);

			fxCoderAdd(param, 0, nextTarget);
			fxCoderAddByte(param, 1, XS_CODE_TRUE);
			fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, done);
			fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
			fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
			fxCoderAddByte(param, 1, XS_CODE_DUB);
			fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->nextSymbol);
			fxCoderAddByte(param, -2, XS_CODE_CALL);
			fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
			fxCoderAddIndex(param, 1, XS_CODE_SET_LOCAL_1, result);
			fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->doneSymbol);
			fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, done);
			
			fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, doneTarget);
		
			fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, rest);
			fxCoderAddByte(param, 1, XS_CODE_DUB);
			fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->lengthSymbol);
			fxCoderAddByte(param, 0, XS_CODE_AT);
			fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, result);
			fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->valueSymbol);
			fxCoderAddByte(param, -2, XS_CODE_SET_PROPERTY_AT);
			fxCoderAddByte(param, -1, XS_CODE_POP);
	
			fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, nextTarget);
			fxCoderAdd(param, 1, doneTarget);
		
			fxCoderAddIndex(param, 0, XS_CODE_GET_LOCAL_1, rest);
			fxNodeDispatchCodeAssign(((txRestBindingNode*)item)->binding, param);
			fxCoderAddByte(param, -1, XS_CODE_POP);
		}
	
	}
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, normalTarget);
	
	selection = 1;
	coder->returnTarget = fxCoderFinalizeTargets(param, coder->returnTarget, selector, &selection, finallyTarget);
	fxCoderAdd(param, 0, normalTarget);
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, selection);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, selector);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAdd(param, 0, finallyTarget);
	fxCoderAddByte(param, 0, XS_CODE_UNCATCH);
	fxCoderAdd(param, 0, catchTarget);
	
	nextTarget = fxCoderCreateTarget(param);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, selector);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, nextTarget);
	fxCoderAddByte(param, 1, XS_CODE_EXCEPTION);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, result);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	catchTarget = fxCoderCreateTarget(param);
	fxCoderAddBranch(param, 0, XS_CODE_CATCH_1, catchTarget);
	fxCoderAdd(param, 0, nextTarget);
	
	doneTarget = fxCoderCreateTarget(param);
 	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, done);
 	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, doneTarget);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->returnSymbol);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, rest);
	fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
	fxCoderAddByte(param, -1, XS_CODE_STRICT_EQUAL);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, doneTarget);
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, rest);
	fxCoderAddByte(param, -2, XS_CODE_CALL);
	fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAdd(param, 0, doneTarget);
	
	nextTarget = fxCoderCreateTarget(param);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, selector);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, nextTarget);
	fxCoderAddByte(param, 0, XS_CODE_UNCATCH);
	fxCoderAdd(param, 0, catchTarget);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, result);
	fxCoderAddByte(param, -1, XS_CODE_THROW);
	fxCoderAdd(param, 0, nextTarget);

	selection = 1;
	fxCoderJumpTargets(param, coder->returnTarget, selector, &selection);

	fxCoderUnuseTemporaryVariables(param, 5);
}

void fxAssignNodeCode(void* it, void* param) 
{
	txAssignNode* self = it;
	fxNodeDispatchCodeReference(self->reference, param);
	fxNodeDispatchCode(self->value, param);
	fxNodeDispatchCodeAssign(self->reference, param);
}

void fxAwaitNodeCode(void* it, void* param)
{
	txStatementNode* self = it;
	txCoder* coder = param;
	txTargetCode* target = fxCoderCreateTarget(coder);
	fxNodeDispatchCode(self->expression, param);
	fxCoderAddByte(param, 0, XS_CODE_AWAIT);
	fxCoderAddBranch(coder, 1, XS_CODE_BRANCH_STATUS_1, target);
	fxCoderAddByte(param, -1, XS_CODE_RESULT);
	fxCoderAdjustEnvironment(coder, coder->returnTarget);
	fxCoderAdjustScope(coder, coder->returnTarget);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, coder->returnTarget);
	fxCoderAdd(coder, 0, target);
}

void fxBinaryExpressionNodeCode(void* it, void* param) 
{
	txBinaryExpressionNode* self = it;
	fxNodeDispatchCode(self->left, param);
	fxNodeDispatchCode(self->right, param);
	fxCoderAddByte(param, -1, self->description->code);
}

void fxBindingNodeCode(void* it, void* param) 
{
	txBindingNode* self = it;
	txCoder* coder = param;
	if ((self->description->token != XS_TOKEN_VAR) || self->initializer) {
		fxNodeDispatchCodeReference(self, param);
		if (self->initializer)
			fxNodeDispatchCode(self->initializer, param);
		else {
			if (self->description->token == XS_TOKEN_CONST)
				fxReportLineError(coder->parser, self->line, "invalid const");
			fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
		}
		self->initializer = C_NULL;
		fxNodeDispatchCodeAssign(self, param);
		fxCoderAddByte(param, -1, XS_CODE_POP);
	}
}

void fxBindingNodeCodeAssign(void* it, void* param) 
{
	txBindingNode* self = it;
	txDeclareNode* declaration = self->declaration;
	fxBindingNodeCodeDefault(it, param);
	if (!self->declaration)
		fxCoderAddSymbol(param, -1, XS_CODE_SET_VARIABLE, self->symbol);
	else
		fxCoderAddIndex(param, 0, (declaration->flags & mxDeclareNodeClosureFlag) ? XS_CODE_SET_CLOSURE_1 : XS_CODE_SET_LOCAL_1, declaration->index);
}

void fxBindingNodeCodeReference(void* it, void* param) 
{
	txAccessNode* self = it;
	txCoder* coder = param;
	txDeclareNode* declaration = self->declaration;
	if (!declaration) {
		if (coder->evalFlag)
			fxCoderAddSymbol(param, 1, XS_CODE_EVAL_REFERENCE, self->symbol);
		else
			fxCoderAddSymbol(param, 1, XS_CODE_PROGRAM_REFERENCE, self->symbol);
	}
}

void fxBindingNodeCodeDefault(void* it, void* param) 
{
	txBindingNode* self = it;
	if (self->initializer) {
		txTargetCode* target = fxCoderCreateTarget(param);
		fxCoderAddByte(param, 1, XS_CODE_DUB);
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_STRICT_NOT_EQUAL);
		fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, target);
		fxCoderAddByte(param, -1, XS_CODE_POP);
		fxNodeDispatchCode(self->initializer, param);
		fxCoderAdd(param, 0, target);
	}
}

void fxBlockNodeCode(void* it, void* param) 
{
	txBlockNode* self = it;
	fxScopeCodingBlock(self->scope, param);
	fxScopeCodeDefineNodes(self->scope, param);
	fxNodeDispatchCode(self->statement, param);
	fxScopeCoded(self->scope, param);
}

void fxBodyNodeCode(void* it, void* param) 
{
	txBlockNode* self = it;
	txCoder* coder = param;
	txBoolean evalFlag = coder->evalFlag;
	if ((self->flags & mxEvalFlag) && !(self->flags & mxStrictFlag))
		coder->evalFlag = 1;
	fxScopeCodingBody(self->scope, param);
	fxScopeCodeDefineNodes(self->scope, param);
	fxNodeDispatchCode(self->statement, param);
	fxScopeCodedBody(self->scope, param);
	if ((self->flags & mxEvalFlag) && !(self->flags & mxStrictFlag))
		coder->evalFlag = evalFlag;
}

void fxBreakContinueNodeCode(void* it, void* param) 
{
	txBreakContinueNode* self = it;
	txCoder* coder = param;
	txTargetCode* target = (self->description->token == XS_TOKEN_BREAK) ? coder->firstBreakTarget : coder->firstContinueTarget;
	while (target) {
		txLabelNode* label = target->label;
		while (label) {
			if (label->symbol == self->symbol) {
				fxCoderAdjustEnvironment(coder, target);
				fxCoderAdjustScope(coder, target);
				fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, target);
				return;
			}
			label = label->nextLabel;
		}
		target = target->nextTarget;
	}
	if (self->description->token == XS_TOKEN_BREAK)
		fxReportLineError(coder->parser, self->line, "invalid break");
	else
		fxReportLineError(coder->parser, self->line, "invalid continue");
}

void fxCallNodeCode(void* it, void* param) 
{
	txCallNewNode* self = it;
	txCoder* coder = param;
	txInteger c;
	switch (self->reference->description->token) {
	case XS_TOKEN_INCLUDE:
		fxNodeDispatchCode(self->reference, param);
		break;
	case XS_TOKEN_MEMBER: 
		c = fxParamsNodeCode(self->params, param);
		fxMemberNodeCodeCall(self->reference, param);
		fxCoderAddByte(param, -2 - c, (self->flags & mxTailRecursionFlag) ? XS_CODE_CALL_TAIL : XS_CODE_CALL);
		break;
	case XS_TOKEN_MEMBER_AT: 
		c = fxParamsNodeCode(self->params, param);
		fxMemberAtNodeCodeCall(self->reference, param);
		fxCoderAddByte(param, -2 - c, (self->flags & mxTailRecursionFlag) ? XS_CODE_CALL_TAIL : XS_CODE_CALL);
		break;
	default: 	
		c = fxParamsNodeCode(self->params, param);
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxNodeDispatchCode(self->reference, param);
		if (self->scope) {
			txTargetCode* elseTarget = fxCoderCreateTarget(param);
			txTargetCode* endTarget = fxCoderCreateTarget(param);
			fxCoderAddByte(param, 1, XS_CODE_DUB);
			fxCoderAddIndex(param, 1, XS_CODE_INTRINSIC, mxEvalIntrinsic);
			fxCoderAddByte(param, -1, XS_CODE_STRICT_EQUAL);
			fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, elseTarget);
			fxCoderAddByte(param, -1, XS_CODE_POP);
			fxCoderAddByte(param, -1, XS_CODE_POP);
			fxCoderAddByte(param, 0 - c, XS_CODE_EVAL);
			fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, endTarget);
			fxCoderAdd(coder, 2 + c, elseTarget);
			fxCoderAddByte(param, -2 - c, (self->flags & mxTailRecursionFlag) ? XS_CODE_CALL_TAIL : XS_CODE_CALL);
			fxCoderAdd(coder, 0, endTarget);
		}
		else {
			fxCoderAddByte(param, -2 - c, (self->flags & mxTailRecursionFlag) ? XS_CODE_CALL_TAIL : XS_CODE_CALL);
		}
		break;
	}
}

void fxCatchNodeCode(void* it, void* param) 
{
	txCatchNode* self = it;
	if (self->parameter) {
		fxScopeCodingBlock(self->scope, param);
		fxNodeDispatchCodeReference(self->parameter, param);
		fxCoderAddByte(param, 1, XS_CODE_EXCEPTION);
		fxNodeDispatchCodeAssign(self->parameter, param);
		fxCoderAddByte(param, -1, XS_CODE_POP);
		fxScopeCodingBlock(self->statementScope, param);
		fxScopeCodeDefineNodes(self->statementScope, param);
		fxNodeDispatchCode(self->statement, param);
		fxScopeCoded(self->statementScope, param);
		fxScopeCoded(self->scope, param);
	}
	else {
		fxScopeCodingBlock(self->statementScope, param);
		fxScopeCodeDefineNodes(self->statementScope, param);
		fxNodeDispatchCode(self->statement, param);
		fxScopeCoded(self->statementScope, param);
	}
}

void fxClassNodeCode(void* it, void* param) 
{
	txClassNode* self = it;
	txCoder* coder = param;
	txFlag flag;
	txInteger prototype = fxCoderUseTemporaryVariable(coder);
	txInteger constructor = fxCoderUseTemporaryVariable(coder);
	txNode* item = self->items->first;
	if (self->scope)
		fxScopeCodingBlock(self->scope, param);
	if (self->heritage) {
		if (self->heritage->description->token == XS_TOKEN_HOST) {
			fxCoderAddByte(param, 1, XS_CODE_NULL);
			fxNodeDispatchCode(self->heritage, param);
		}
		else {
			fxNodeDispatchCode(self->heritage, param);
			fxCoderAddByte(param, 1, XS_CODE_EXTEND);
		}
	}
	else {
		fxCoderAddByte(param, 1, XS_CODE_NULL);
		fxCoderAddByte(param, 1, XS_CODE_OBJECT);
	}
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, prototype);
	fxNodeDispatchCode(self->constructor, param);
	fxCoderAddByte(param, 0, XS_CODE_TO_INSTANCE);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, constructor);
	fxCoderAddByte(param, -3, XS_CODE_CLASS);
	
	while (item) {
		txNode* value;
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, (item->flags & mxStaticFlag) ? constructor : prototype);
		if (item->description->token == XS_TOKEN_PROPERTY) {
			fxCoderAddSymbol(param, 1, XS_CODE_SYMBOL, ((txPropertyNode*)item)->symbol);
			value = ((txPropertyNode*)item)->value;
		}
		else {
			fxNodeDispatchCode(((txPropertyAtNode*)item)->at, param);
			value = ((txPropertyAtNode*)item)->value;
		}
		fxCoderAddByte(param, 0, XS_CODE_AT);
		fxNodeDispatchCode(value, param);
		flag = XS_DONT_ENUM_FLAG;
		if (item->flags & mxMethodFlag)
			flag |= XS_METHOD_FLAG;
		else if (item->flags & mxGetterFlag)
			flag |= XS_GETTER_FLAG;
		else if (item->flags & mxSetterFlag)
			flag |= XS_SETTER_FLAG;
		fxCoderAddFlag(param, -3, XS_CODE_NEW_PROPERTY, flag);
		item = item->next;
	}
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, constructor);
	if (self->symbol)
		fxCoderAddSymbol(param, 0, XS_CODE_NAME, self->symbol);
	if (self->scope) {
		txDeclareNode* declaration = self->scope->firstDeclareNode;
		fxCoderAddIndex(param, 0, XS_CODE_CONST_CLOSURE_1, declaration->index);
		fxScopeCoded(self->scope, param);
	}
	fxCoderUnuseTemporaryVariables(coder, 2);
}

void fxCompoundExpressionNodeCode(void* it, void* param) 
{
	txAssignNode* self = it;
	txCoder* coder = param;
	switch (self->reference->description->token) {
	case XS_TOKEN_ACCESS: fxAccessNodeCodeCompound(self->reference, param, self); break;
	case XS_TOKEN_MEMBER: fxMemberNodeCodeCompound(self->reference, param, self); break;
	case XS_TOKEN_MEMBER_AT: fxMemberAtNodeCodeCompound(self->reference, param, self); break;
	default: fxReportLineError(coder->parser, self->line, "no reference");
	}
}

void fxDebuggerNodeCode(void* it, void* param) 
{
	fxCoderAddByte(param, 0, XS_CODE_DEBUGGER);
}

void fxDeclareNodeCodeAssign(void* it, void* param) 
{
	txDeclareNode* self = it;
	txDeclareNode* declaration = self->declaration;
	fxBindingNodeCodeDefault(it, param);
	if (!declaration)
		fxCoderAddSymbol(param, -1, XS_CODE_SET_VARIABLE, self->symbol);
	else {
		if (self->description->token == XS_TOKEN_CONST)
			fxCoderAddIndex(param, 0, (declaration->flags & mxDeclareNodeClosureFlag) ? XS_CODE_CONST_CLOSURE_1: XS_CODE_CONST_LOCAL_1, declaration->index);
		else if (self->description->token == XS_TOKEN_LET)
			fxCoderAddIndex(param, 0, (declaration->flags & mxDeclareNodeClosureFlag) ? XS_CODE_LET_CLOSURE_1: XS_CODE_LET_LOCAL_1, declaration->index);
		else
			fxCoderAddIndex(param, 0, (declaration->flags & mxDeclareNodeClosureFlag) ? XS_CODE_VAR_CLOSURE_1 : XS_CODE_VAR_LOCAL_1, declaration->index);
	}
}

void fxDeclareNodeCodeReference(void* it, void* param) 
{
	txAccessNode* self = it;
	txCoder* coder = param;
	txDeclareNode* declaration = self->declaration;
	if (!declaration) {
		if (coder->evalFlag)
			fxCoderAddSymbol(param, 1, XS_CODE_EVAL_REFERENCE, self->symbol);
		else
			fxCoderAddSymbol(param, 1, XS_CODE_PROGRAM_REFERENCE, self->symbol);
	}
}

void fxDefineNodeCode(void* it, void* param) 
{
	txDefineNode* self = it;
	txCoder* coder = param;
	if (self->flags & mxDefineNodeCodedFlag)
		return;
	self->flags |= mxDefineNodeCodedFlag;
	fxDeclareNodeCodeReference(it, param);
	fxNodeDispatchCode(self->initializer, coder);
    self->initializer = C_NULL;
	fxDeclareNodeCodeAssign(it, param);
	fxCoderAddByte(coder, -1, XS_CODE_POP);
}

void fxDelegateNodeCode(void* it, void* param)
{
	txStatementNode* self = it;
	txCoder* coder = param;
	txInteger iterator;
	txInteger method;
	txInteger result;

	txTargetCode* nextTarget = fxCoderCreateTarget(param);
	txTargetCode* catchTarget = fxCoderCreateTarget(param);
	txTargetCode* rethrowTarget = fxCoderCreateTarget(param);
	txTargetCode* returnTarget = fxCoderCreateTarget(param);
	txTargetCode* normalTarget = fxCoderCreateTarget(param);
	txTargetCode* doneTarget = fxCoderCreateTarget(param);
	
	iterator = fxCoderUseTemporaryVariable(param);
	method = fxCoderUseTemporaryVariable(param);
	result = fxCoderUseTemporaryVariable(param);
	
	fxNodeDispatchCode(self->expression, param);
	fxCoderAddByte(param, 0, XS_CODE_FOR_OF);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, iterator);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	
	fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, result);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAddBranch(param, 0, XS_CODE_CATCH_1, catchTarget);
	fxCoderAddBranch(coder, 0, XS_CODE_BRANCH_1, normalTarget);
	
// LOOP	
	fxCoderAdd(param, 0, nextTarget);
	fxCoderAddByte(coder, 0, XS_CODE_YIELD);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, result);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAddBranch(param, 0, XS_CODE_CATCH_1, catchTarget);
	fxCoderAddBranch(coder, 1, XS_CODE_BRANCH_STATUS_1, normalTarget);
	
// RETURN	
	fxCoderAddByte(param, 0, XS_CODE_UNCATCH);
	
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->returnSymbol);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, method);
	fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
	fxCoderAddByte(param, -1, XS_CODE_STRICT_EQUAL);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, returnTarget);
	
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, result);
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 1);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, method);
	fxCoderAddByte(param, -3, XS_CODE_CALL);
	fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->doneSymbol);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, nextTarget);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->valueSymbol);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, result);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	
	fxCoderAdd(coder, 0, returnTarget);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, result);
	fxCoderAddByte(param, -1, XS_CODE_RESULT);
	fxCoderAdjustEnvironment(coder, coder->returnTarget);
	fxCoderAdjustScope(coder, coder->returnTarget);
	fxCoderAddBranch(coder, 0, XS_CODE_BRANCH_1, coder->returnTarget);
	
// THROW	
	fxCoderAdd(coder, 0, catchTarget);

	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->throwSymbol);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, method);
	fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
	fxCoderAddByte(param, -1, XS_CODE_STRICT_EQUAL);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, doneTarget);
	
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->returnSymbol);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, method);
	fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
	fxCoderAddByte(param, -1, XS_CODE_STRICT_EQUAL);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, rethrowTarget);
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, method);
	fxCoderAddByte(param, -2, XS_CODE_CALL);
	fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	
	fxCoderAdd(coder, 0, rethrowTarget);
	fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
	fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	
// NORMAL	
	fxCoderAdd(coder, 0, normalTarget);
	fxCoderAddByte(param, 0, XS_CODE_UNCATCH);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->nextSymbol);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, method);
	fxCoderAddByte(param, -1, XS_CODE_POP);

	fxCoderAdd(param, 1, doneTarget);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, result);
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 1);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, method);
	fxCoderAddByte(param, -3, XS_CODE_CALL);
	fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->doneSymbol);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, nextTarget);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->valueSymbol);

	fxCoderUnuseTemporaryVariables(param, 3);
}

void fxDeleteNodeCode(void* it, void* param) 
{
	txDeleteNode* self = it;
	txCoder* coder = param;
	switch (self->reference->description->token) {
	case XS_TOKEN_ACCESS: fxAccessNodeCodeDelete(self->reference, param); break;
	case XS_TOKEN_MEMBER: fxMemberNodeCodeDelete(self->reference, param); break;
	case XS_TOKEN_MEMBER_AT: fxMemberAtNodeCodeDelete(self->reference, param); break;
	case XS_TOKEN_UNDEFINED:
		if (self->flags & mxStrictFlag)
			fxReportLineError(coder->parser, self->line, "delete identifier (strict code)");
		fxCoderAddByte(param, 1, XS_CODE_FALSE);
		break;
	default: 
		fxNodeDispatchCode(self->reference, param);
		fxCoderAddByte(param, -1, XS_CODE_POP);
		fxCoderAddByte(param, 1, XS_CODE_TRUE);
		break;
	}
}

void fxDoNodeCode(void* it, void* param) 
{
	txDoNode* self = it;
	txCoder* coder = param;
	txTargetCode* loopTarget = fxCoderCreateTarget(param);
	if (coder->programFlag) {
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_RESULT);
	}
	fxCoderAdd(param, 0, loopTarget);
	fxNodeDispatchCode(self->statement, param);
	fxCoderAdd(param, 0, coder->firstContinueTarget);
	fxNodeDispatchCode(self->expression, param);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, loopTarget);
}

void fxExportNodeCode(void* it, void* param) 
{
}

void fxExpressionsNodeCode(void* it, void* param) 
{
	txExpressionsNode* self = it;
	if (self->items) {
		txNode* item = self->items->first;
		txNode* previous = NULL;
		txNode* next;
		while (item) {
			next = item->next;
			if (previous)
				fxCoderAddByte(param, -1, XS_CODE_POP);
			if (!next)
				item->flags |= (self->flags & mxTailRecursionFlag);
			fxNodeDispatchCode(item, param);
			previous = item;
			item = next;
		}
	}
}

void fxForNodeCode(void* it, void* param) 
{
	txForNode* self = it;
	txCoder* coder = param;
	txTargetCode* nextTarget;
	txTargetCode* doneTarget;
	fxScopeCodingBlock(self->scope, param);
	fxScopeCodeDefineNodes(self->scope, param);
	nextTarget = fxCoderCreateTarget(param);
	doneTarget = fxCoderCreateTarget(param);
	if (self->initialization)
		fxNodeDispatchCode(self->initialization, param);
	if (coder->programFlag) {
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_RESULT);
	}
	fxScopeCodeRefresh(self->scope, param);
	fxCoderAdd(param, 0, nextTarget);
	if (self->expression) {
		fxNodeDispatchCode(self->expression, param);
		fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, doneTarget);
	}
	coder->firstContinueTarget->scopeLevel = coder->scopeLevel;
	fxNodeDispatchCode(self->statement, param);
	fxCoderAdd(param, 0, coder->firstContinueTarget);
	if (self->iteration) {
		fxScopeCodeRefresh(self->scope, param);
		self->iteration->flags |= mxExpressionNoValue;
		fxNodeDispatchCode(self->iteration, param);
		fxCoderAddByte(param, -1, XS_CODE_POP);
	}
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, nextTarget);
	fxCoderAdd(param, 0, doneTarget);
	fxScopeCoded(self->scope, param);
}

void fxForInForOfNodeCode(void* it, void* param) 
{
	txForInForOfNode* self = it;
	txCoder* coder = param;
	txInteger iterator;
	txInteger next;
	txInteger done;
	txInteger result;
	txInteger selector;
	txInteger selection;
	txInteger _return;
	txTargetCode* nextTarget;
	txTargetCode* doneTarget;
	txTargetCode* catchTarget;
	txTargetCode* normalTarget;
	txTargetCode* finallyTarget;
	
	iterator = fxCoderUseTemporaryVariable(param);
	next = fxCoderUseTemporaryVariable(param);
	done = fxCoderUseTemporaryVariable(param);
	result = fxCoderUseTemporaryVariable(param);
	selector = fxCoderUseTemporaryVariable(coder);
	_return = fxCoderUseTemporaryVariable(coder);
	coder->firstBreakTarget = fxCoderAliasTargets(param, coder->firstBreakTarget);
	coder->firstContinueTarget->nextTarget = fxCoderAliasTargets(param, coder->firstContinueTarget->nextTarget);
	coder->returnTarget = fxCoderAliasTargets(param, coder->returnTarget);

	fxScopeCodingBlock(self->scope, param);
	fxScopeCodeDefineNodes(self->scope, param);
	if (coder->programFlag) {
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_RESULT);
	}
	fxNodeDispatchCode(self->expression, param);
	fxCoderAddByte(param, 0, self->description->code);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->nextSymbol);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, next);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	
	catchTarget = fxCoderCreateTarget(param);
	normalTarget = fxCoderCreateTarget(param);
	finallyTarget = fxCoderCreateTarget(param);
	nextTarget = fxCoderCreateTarget(param);
	
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
	fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, selector);
	fxCoderAddBranch(param, 0, XS_CODE_CATCH_1, catchTarget);
	
	fxCoderAdd(param, 0, nextTarget);
	fxCoderAddByte(param, 1, XS_CODE_TRUE);
	fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, done);
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, next);
	fxCoderAddByte(param, -2, XS_CODE_CALL);
	fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, result);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->doneSymbol);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, done);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, normalTarget);
	
	fxScopeCodeReset(self->scope, param);
	fxNodeDispatchCodeReference(self->reference, param);
	fxCoderAddByte(param, 1, XS_CODE_TRUE);
	fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, done);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, result);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->valueSymbol);
	fxCoderAddByte(param, 1, XS_CODE_FALSE);
	fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, done);
	fxNodeDispatchCodeAssign(self->reference, param);
	fxCoderAddByte(param, -1, XS_CODE_POP);

	coder->firstContinueTarget->scopeLevel = coder->scopeLevel;
	fxNodeDispatchCode(self->statement, param);
	
	fxCoderAdd(param, 0, coder->firstContinueTarget);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, nextTarget);

	selection = 1;
	coder->firstBreakTarget = fxCoderFinalizeTargets(param, coder->firstBreakTarget, selector, &selection, finallyTarget);
	coder->firstContinueTarget->nextTarget = fxCoderFinalizeTargets(param, coder->firstContinueTarget->nextTarget, selector, &selection, finallyTarget);
	coder->returnTarget = fxCoderFinalizeTargets(param, coder->returnTarget, selector, &selection, finallyTarget);
	fxCoderAdd(param, 0, normalTarget);
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, selection);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, selector);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAdd(param, 0, finallyTarget);
	fxCoderAddByte(param, 0, XS_CODE_UNCATCH);
	fxCoderAdd(param, 0, catchTarget);
	
	nextTarget = fxCoderCreateTarget(param);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, selector);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, nextTarget);
	fxCoderAddByte(param, 1, XS_CODE_EXCEPTION);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, result);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	catchTarget = fxCoderCreateTarget(param);
	fxCoderAddBranch(param, 0, XS_CODE_CATCH_1, catchTarget);
	fxCoderAdd(param, 0, nextTarget);
	
	doneTarget = fxCoderCreateTarget(param);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, done);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, doneTarget);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->returnSymbol);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, _return);
	fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
	fxCoderAddByte(param, -1, XS_CODE_STRICT_EQUAL);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, doneTarget);
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, _return);
	fxCoderAddByte(param, -2, XS_CODE_CALL);
 	fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAdd(param, 0, doneTarget);

	nextTarget = fxCoderCreateTarget(param);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, selector);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, nextTarget);
	fxCoderAddByte(param, 0, XS_CODE_UNCATCH);
	fxCoderAdd(param, 0, catchTarget);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, result);
	fxCoderAddByte(param, -1, XS_CODE_THROW);
	fxCoderAdd(param, 0, nextTarget);
	
	selection = 1;
	fxCoderJumpTargets(param, coder->firstBreakTarget, selector, &selection);
	fxCoderJumpTargets(param, coder->firstContinueTarget->nextTarget, selector, &selection);
	fxCoderJumpTargets(param, coder->returnTarget, selector, &selection);
	
	fxScopeCoded(self->scope, param);
	fxCoderUnuseTemporaryVariables(param, 6);
}

void fxFunctionNodeCode(void* it, void* param) 
{
	txFunctionNode* self = it;
	txCoder* coder = param;
	txInteger environmentLevel = coder->environmentLevel;
	txInteger line = coder->line;
	txBoolean programFlag = coder->programFlag;
	txInteger scopeLevel = coder->scopeLevel;
	txTargetCode* firstBreakTarget = coder->firstBreakTarget;
	txTargetCode* firstContinueTarget = coder->firstContinueTarget;
	txTargetCode* returnTarget = coder->returnTarget;
	txSymbol* name = self->symbol;
	txTargetCode* target = fxCoderCreateTarget(param);
	
	coder->line = -1;
	coder->programFlag = 0;
	coder->scopeLevel = 0;
	coder->firstBreakTarget = NULL;
	coder->firstContinueTarget = NULL;

    if (name) {
        if (self->flags & mxGetterFlag) {
            txString buffer = coder->parser->buffer;
            c_strcpy(buffer, "get ");
            c_strcat(buffer, name->string);
            name = fxNewParserSymbol(coder->parser, buffer);
        }
        else if (self->flags & mxSetterFlag) {
            txString buffer = coder->parser->buffer;
            c_strcpy(buffer, "set ");
            c_strcat(buffer, name->string);
            name = fxNewParserSymbol(coder->parser, buffer);
        }
    }
	
	if (self->flags & mxGeneratorFlag)
		fxCoderAddSymbol(param, 1, XS_CODE_GENERATOR_FUNCTION, name);
	else if (self->flags & mxAsyncFlag)
		fxCoderAddSymbol(param, 1, XS_CODE_ASYNC_FUNCTION, name);
	else if (self->flags & (mxArrowFlag | mxMethodFlag | mxGetterFlag | mxSetterFlag))
		fxCoderAddSymbol(param, 1, XS_CODE_FUNCTION, name);
	else
		fxCoderAddSymbol(param, 1, XS_CODE_CONSTRUCTOR_FUNCTION, name);
	fxCoderAddBranch(param, 0, XS_CODE_CODE_1, target);
	if (self->flags & mxDerivedFlag)
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_STRICT_DERIVED, fxCoderCountParameters(coder, self->params));
	else if (self->flags & mxBaseFlag)
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_STRICT_BASE, fxCoderCountParameters(coder, self->params));
	else if (self->flags & mxStrictFlag)
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_STRICT, fxCoderCountParameters(coder, self->params));
	else
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_SLOPPY, fxCoderCountParameters(coder, self->params));
	if (self->scopeCount)
		fxCoderAddIndex(param, 0, XS_CODE_RESERVE_1, self->scopeCount);
	coder->path = C_NULL;
	fxScopeCodeRetrieve(self->scope, param);
	fxScopeCodingParams(self->scope, param);
	if (self->flags & mxAsyncFlag)
		fxCoderAddByte(param, 0, XS_CODE_START_ASYNC);
	fxNodeDispatchCode(self->params, param);
	fxScopeCodeDefineNodes(self->scope, param);
	coder->returnTarget = fxCoderCreateTarget(param);
	if (self->flags & mxGeneratorFlag)
		fxCoderAddByte(param, 0, XS_CODE_START_GENERATOR);
	fxNodeDispatchCode(self->body, param);
	fxCoderAdd(param, 0, coder->returnTarget);
	if (self->flags & mxArrowFlag)
		fxCoderAddByte(param, 0, XS_CODE_END_ARROW);
	else if (self->flags & mxBaseFlag)
		fxCoderAddByte(param, 0, XS_CODE_END_BASE);
	else if (self->flags & mxDerivedFlag)
		fxCoderAddByte(param, 0, XS_CODE_END_DERIVED);
	else
		fxCoderAddByte(param, 0, XS_CODE_END);
	fxCoderAdd(param, 0, target);
	
	if (self->scope->closureNodeCount || (self->flags & mxArrowFlag) || (self->scope->flags & mxEvalFlag) || (coder->evalFlag)) {
		fxCoderAddByte(coder, 1, XS_CODE_ENVIRONMENT);
		fxScopeCodeStore(self->scope, param);
		fxCoderAddByte(coder, -1, XS_CODE_POP);
	}
	if ((self->flags & (mxArrowFlag | mxBaseFlag | mxDerivedFlag | mxGeneratorFlag | mxStrictFlag)) == 0) {
		fxCoderAddByte(param, 1, XS_CODE_DUB);
		fxCoderAddSymbol(param, 1, XS_CODE_SYMBOL, coder->parser->callerSymbol);
		fxCoderAddByte(param, 0, XS_CODE_AT);
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddFlag(param, -3, XS_CODE_NEW_PROPERTY, XS_DONT_ENUM_FLAG);
	}
	
	coder->returnTarget = returnTarget;
	coder->firstContinueTarget = firstContinueTarget;
	coder->firstBreakTarget = firstBreakTarget;
	coder->scopeLevel = scopeLevel;
	coder->programFlag = programFlag;
	coder->line = line;
	coder->environmentLevel = environmentLevel;
}

void fxHostNodeCode(void* it, void* param) 
{
	txHostNode* self = it;
	txCoder* coder = param;
	txParser* parser = coder->parser;
    if (self->symbol) {
        if (self->flags & mxGetterFlag) {
            txString buffer = coder->parser->buffer;
            c_strcpy(buffer, "get ");
            c_strcat(buffer, self->symbol->string);
            self->symbol = fxNewParserSymbol(coder->parser, buffer);
        }
        else if (self->flags & mxSetterFlag) {
            txString buffer = coder->parser->buffer;
            c_strcpy(buffer, "set ");
            c_strcat(buffer, self->symbol->string);
            self->symbol = fxNewParserSymbol(coder->parser, buffer);
        }
    }
	if (self->params)
		self->paramsCount = fxCoderCountParameters(coder, self->params);
	else
		self->paramsCount = -1;	
	if (parser->firstHostNode)
		parser->lastHostNode->nextHostNode = self;
	else
		parser->firstHostNode = self;
	parser->lastHostNode = self;
	fxCoderAddIndex(param, 1, XS_CODE_HOST, parser->hostNodeIndex);
	parser->hostNodeIndex++;
}

void fxIfNodeCode(void* it, void* param) 
{
	txIfNode* self = it;
	txCoder* coder = param;
	fxNodeDispatchCode(self->expression, param);
	if (coder->programFlag) {
		txTargetCode* elseTarget = fxCoderCreateTarget(param);
		txTargetCode* endTarget = fxCoderCreateTarget(param);
		fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, elseTarget);
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_RESULT);
		fxNodeDispatchCode(self->thenStatement, param);
		fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, endTarget);
		fxCoderAdd(param, 0, elseTarget);
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_RESULT);
		if (self->elseStatement)
			fxNodeDispatchCode(self->elseStatement, param);
		fxCoderAdd(param, 0, endTarget);
	}
	else {
		if (self->elseStatement) {
			txTargetCode* elseTarget = fxCoderCreateTarget(param);
			txTargetCode* endTarget = fxCoderCreateTarget(param);
			fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, elseTarget);
			fxNodeDispatchCode(self->thenStatement, param);
			fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, endTarget);
			fxCoderAdd(param, 0, elseTarget);
			fxNodeDispatchCode(self->elseStatement, param);
			fxCoderAdd(param, 0, endTarget);
		}
		else {
			txTargetCode* endTarget = fxCoderCreateTarget(param);
			fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, endTarget);
			fxNodeDispatchCode(self->thenStatement, param);
			fxCoderAdd(param, 0, endTarget);
		}
	}
}

void fxImportNodeCode(void* it, void* param) 
{
}

void fxIncludeNodeCode(void* it, void* param) 
{
	txIncludeNode* self = it;
	fxNodeDispatchCode(self->body, param);
}

void fxIntegerNodeCode(void* it, void* param) 
{
	txIntegerNode* self = it;
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, self->value);
}

void fxLabelNodeCode(void* it, void* param) 
{
	txLabelNode* self = it;
	txCoder* coder = param;
	txNode* statement = self->statement;
	txTargetCode* breakTarget;
	while (statement->description->token == XS_TOKEN_LABEL) {
		((txLabelNode*)statement)->nextLabel = self;
		self = (txLabelNode*)statement;
		statement = self->statement;
	}
	breakTarget = coder->firstBreakTarget;
	while (breakTarget) {
		txLabelNode* former = breakTarget->label;
		if (former) {
			txLabelNode* current = self;
			while (current) {
				if (former->symbol && current->symbol && (former->symbol == current->symbol)) {
					fxReportLineError(coder->parser, current->line, "duplicate label %s", current->symbol->string);
				}
				current = current->nextLabel;
			}
		}
		breakTarget = breakTarget->nextTarget;
	}
	breakTarget = fxCoderCreateTarget(coder);
	breakTarget->nextTarget = coder->firstBreakTarget;
	coder->firstBreakTarget = breakTarget;
	breakTarget->label = self;
	if (self->symbol)
		fxNodeDispatchCode(statement, param);
	else {
		txTargetCode* continueTarget = fxCoderCreateTarget(coder);
		continueTarget->nextTarget = coder->firstContinueTarget;
		coder->firstContinueTarget = continueTarget;
		continueTarget->label = self;
		fxNodeDispatchCode(statement, param);
		coder->firstContinueTarget = continueTarget->nextTarget;
	}
	fxCoderAdd(param, 0, coder->firstBreakTarget);
	coder->firstBreakTarget = breakTarget->nextTarget;
}

void fxMemberNodeCode(void* it, void* param) 
{
	txMemberNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxCoderAddSymbol(param, 0, (self->reference->flags & mxSuperFlag) ? XS_CODE_GET_SUPER : XS_CODE_GET_PROPERTY, self->symbol);
}

void fxMemberNodeCodeAssign(void* it, void* param) 
{
	txMemberNode* self = it;
	fxCoderAddSymbol(param, -1, (self->reference->flags & mxSuperFlag) ? XS_CODE_SET_SUPER : XS_CODE_SET_PROPERTY, self->symbol);
}

void fxMemberNodeCodeCall(void* it, void* param) 
{
	txMemberNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddSymbol(param, 0, (self->reference->flags & mxSuperFlag) ? XS_CODE_GET_SUPER : XS_CODE_GET_PROPERTY, self->symbol);
}

void fxMemberNodeCodeCompound(void* it, void* param, txAssignNode* compound) 
{
	txMemberNode* self = it;
	fxMemberNodeCodeReference(it, param);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddSymbol(param, 0, (self->reference->flags & mxSuperFlag) ? XS_CODE_GET_SUPER : XS_CODE_GET_PROPERTY, self->symbol);
	fxNodeDispatchCode(compound->value, param);
	fxCoderAddByte(param, -1, compound->description->code);
	fxMemberNodeCodeAssign(it, param);
}

void fxMemberNodeCodeDelete(void* it, void* param) 
{
	txMemberNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxCoderAddSymbol(param, 0, (self->reference->flags & mxSuperFlag) ? XS_CODE_DELETE_SUPER : XS_CODE_DELETE_PROPERTY, self->symbol);
}

void fxMemberNodeCodePostfix(void* it, void* param, txPostfixExpressionNode* compound) 
{
	txMemberNode* self = it;
	txInteger value;
	fxMemberNodeCodeReference(it, param);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddSymbol(param, 0, (self->reference->flags & mxSuperFlag) ? XS_CODE_GET_SUPER : XS_CODE_GET_PROPERTY, self->symbol);
	if (!(compound->flags & mxExpressionNoValue)) {
		value = fxCoderUseTemporaryVariable(param);
		fxCoderAddByte(param, 0, XS_CODE_PLUS);
		fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, value);
	}
	fxCoderAddByte(param, 0, compound->description->code);
	fxMemberNodeCodeAssign(it, param);
	if (!(compound->flags & mxExpressionNoValue)) {
		fxCoderAddByte(param, -1, XS_CODE_POP);
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, value);
		fxCoderUnuseTemporaryVariables(param, 1);
	}
}

void fxMemberNodeCodeReference(void* it, void* param) 
{
	txMemberNode* self = it;
	fxNodeDispatchCode(self->reference, param);
}

void fxMemberAtNodeCode(void* it, void* param) 
{
	txMemberAtNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxNodeDispatchCode(self->at, param);
	fxCoderAddByte(param, 0, XS_CODE_AT);
	fxCoderAddByte(param, -1, (self->reference->flags & mxSuperFlag) ? XS_CODE_GET_SUPER_AT : XS_CODE_GET_PROPERTY_AT);
}

void fxMemberAtNodeCodeAssign(void* it, void* param) 
{
	txMemberAtNode* self = it;
	fxCoderAddByte(param, -2, (self->reference->flags & mxSuperFlag) ? XS_CODE_SET_SUPER_AT : XS_CODE_SET_PROPERTY_AT);
}

void fxMemberAtNodeCodeCall(void* it, void* param) 
{
	txMemberAtNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxNodeDispatchCode(self->at, param);
	fxCoderAddByte(param, 0, XS_CODE_AT);
	fxCoderAddByte(param, -1, (self->reference->flags & mxSuperFlag) ? XS_CODE_GET_SUPER_AT : XS_CODE_GET_PROPERTY_AT);
}

void fxMemberAtNodeCodeCompound(void* it, void* param, txAssignNode* compound) 
{
	txMemberAtNode* self = it;
	fxMemberAtNodeCodeReference(it, param);
	fxCoderAddByte(param, 2, XS_CODE_DUB_AT);
	fxCoderAddByte(param, -1, (self->reference->flags & mxSuperFlag) ? XS_CODE_GET_SUPER_AT : XS_CODE_GET_PROPERTY_AT);
	fxNodeDispatchCode(compound->value, param);
	fxCoderAddByte(param, -1, compound->description->code);
	fxMemberAtNodeCodeAssign(it, param);
}

void fxMemberAtNodeCodeDelete(void* it, void* param) 
{
	txMemberAtNode* self = it;
	fxMemberAtNodeCodeReference(it, param);
	fxCoderAddByte(param, -1, (self->reference->flags & mxSuperFlag) ? XS_CODE_DELETE_SUPER_AT : XS_CODE_DELETE_PROPERTY_AT);
}

void fxMemberAtNodeCodePostfix(void* it, void* param, txPostfixExpressionNode* compound) 
{
	txMemberAtNode* self = it;
	fxMemberAtNodeCodeReference(it, param);
	txInteger value;
	fxCoderAddByte(param, 2, XS_CODE_DUB_AT);
	fxCoderAddByte(param, -1, (self->reference->flags & mxSuperFlag) ? XS_CODE_GET_SUPER_AT : XS_CODE_GET_PROPERTY_AT);
	if (!(compound->flags & mxExpressionNoValue)) {
		value = fxCoderUseTemporaryVariable(param);
		fxCoderAddByte(param, 0, XS_CODE_PLUS);
		fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, value);
	}
	fxCoderAddByte(param, 0, compound->description->code);
	fxMemberAtNodeCodeAssign(it, param);
	if (!(compound->flags & mxExpressionNoValue)) {
		fxCoderAddByte(param, -1, XS_CODE_POP);
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, value);
		fxCoderUnuseTemporaryVariables(param, 1);
	}
}

void fxMemberAtNodeCodeReference(void* it, void* param) 
{
	txMemberAtNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxNodeDispatchCode(self->at, param);
	fxCoderAddByte(param, 0, XS_CODE_AT);
}

void fxModuleNodeCode(void* it, void* param) 
{
	txModuleNode* self = it;
	txCoder* coder = param;
	txTargetCode* target = fxCoderCreateTarget(param);
	txDeclareNode* declaration;
	txInteger count;
	
	coder->line = -1;
	coder->programFlag = 0;
	coder->scopeLevel = 0;
	coder->firstBreakTarget = NULL;
	coder->firstContinueTarget = NULL;
	
	fxCoderAddSymbol(param, 1, XS_CODE_FUNCTION, (coder->parser->flags & mxDebugFlag) ? self->path : C_NULL);
	fxCoderAddBranch(param, 0, XS_CODE_CODE_1, target);
	if (self->flags & mxStrictFlag)
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_STRICT, 0);
	else
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_SLOPPY, 0);
	if (self->scopeCount)
		fxCoderAddIndex(param, 0, XS_CODE_RESERVE_1, self->scopeCount);
	coder->path = C_NULL;
	fxScopeCodeRetrieve(self->scope, param);

	coder->returnTarget = fxCoderCreateTarget(param);
	declaration = self->scope->firstDeclareNode;
	while (declaration) {
		if (declaration->description->token == XS_TOKEN_VAR) {
			fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
			fxCoderAddIndex(coder, 0, XS_CODE_VAR_CLOSURE_1, declaration->index);
			fxCoderAddByte(coder, -1, XS_CODE_POP);
		}
		declaration = declaration->nextDeclareNode;
	}
	fxScopeCodeDefineNodes(self->scope, param);
	
	fxNodeDispatchCode(self->body, param);
	
	fxCoderAdd(param, 0, coder->returnTarget);
	fxCoderAddByte(param, 0, XS_CODE_END);
	fxCoderAdd(param, 0, target);
	
	fxCoderAddByte(param, 1, XS_CODE_ENVIRONMENT);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	
	count = 1 + fxScopeCodeSpecifierNodes(self->scope, coder);
	fxCoderAddInteger(coder, 1, XS_CODE_INTEGER_1, count);
	fxCoderAddByte(coder, 0 - count, XS_CODE_MODULE);
	fxCoderAddByte(coder, -1, XS_CODE_RESULT);
	fxCoderAddByte(coder, 0, XS_CODE_END);
}

void fxNewNodeCode(void* it, void* param) 
{
	txCallNewNode* self = it;
	txInteger c = fxParamsNodeCode(self->params, param);
	fxNodeDispatchCode(self->reference, param);
	fxCoderAddByte(param, -1 - c, XS_CODE_NEW);
}

void fxNumberNodeCode(void* it, void* param) 
{
	txNumberNode* self = it;
	fxCoderAddNumber(param, 1, XS_CODE_NUMBER, self->value);
}

void fxObjectNodeCode(void* it, void* param) 
{
	txObjectNode* self = it;
	txCoder* coder = param;
	txInteger object = fxCoderUseTemporaryVariable(param);
	txNode* item;
	txFlag flag = 0;
	if (self->items) {
		item = self->items->first;
		while (item) {
			if (item->description->token == XS_TOKEN_PROPERTY) {
				if (!(item->flags & mxShorthandFlag) && (((txPropertyNode*)item)->symbol == coder->parser->__proto__Symbol)) {
					if (flag)
						fxReportLineError(coder->parser, item->line, "invalid __proto__");
					flag = 1;
					fxNodeDispatchCode(((txPropertyNode*)item)->value, param);
					fxCoderAddByte(param, 0, XS_CODE_INSTANTIATE);
				}
			}
			item = item->next;
		}
	}
	if (!flag)
		fxCoderAddByte(param, 1, XS_CODE_OBJECT);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, object);
	if (self->items) {
		item = self->items->first;
		while (item) {
			txNode* value;
			if (item->description->token == XS_TOKEN_SPREAD) {
				fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, object);
				fxNodeDispatchCode(((txSpreadNode*)item)->expression, param);
				fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 2);
				fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
				fxCoderAddIndex(param, 1, XS_CODE_INTRINSIC, mxCopyObjectIntrinsic);
				fxCoderAddByte(param, -4, XS_CODE_CALL);
				fxCoderAddByte(param, -1, XS_CODE_POP);
			}
			else {
				if (item->description->token == XS_TOKEN_PROPERTY) {
					if (!(item->flags & mxShorthandFlag) && (((txPropertyNode*)item)->symbol == coder->parser->__proto__Symbol)) {
						item = item->next;
						continue;
					}
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, object);
					fxCoderAddSymbol(param, 1, XS_CODE_SYMBOL, ((txPropertyNode*)item)->symbol);
					value = ((txPropertyNode*)item)->value;
				}
				else {
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, object);
					fxNodeDispatchCode(((txPropertyAtNode*)item)->at, param);
					value = ((txPropertyAtNode*)item)->value;
				}
				fxCoderAddByte(param, 0, XS_CODE_AT);
				fxNodeDispatchCode(value, param);
				flag = 0;
				if (item->flags & mxMethodFlag)
					flag |= XS_METHOD_FLAG;
				else if (item->flags & mxGetterFlag)
					flag |= XS_GETTER_FLAG;
				else if (item->flags & mxSetterFlag)
					flag |= XS_SETTER_FLAG;
				fxCoderAddFlag(param, -3, XS_CODE_NEW_PROPERTY, flag);
			}
			item = item->next;
		}
	}
	fxCoderUnuseTemporaryVariables(param, 1);
}

void fxObjectBindingNodeCodeAssign(void* it, void* param) 
{
	txObjectBindingNode* self = it;
	txNode* item = self->items->first;
	txInteger object;
	txInteger at;
	txInteger c = 0;
	fxBindingNodeCodeDefault(it, param);
	object = fxCoderUseTemporaryVariable(param);
	at = fxCoderUseTemporaryVariable(param);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddByte(param, 0, XS_CODE_TO_INSTANCE);
	fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, object);
	if (self->flags & mxSpreadFlag) {
		fxCoderAddByte(param, 1, XS_CODE_OBJECT);
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, object);
		c = 2;
	}
	while (item && (item->description->token != XS_TOKEN_REST_BINDING)) {
		if (item->description->token == XS_TOKEN_PROPERTY_BINDING) {
			if (self->flags & mxSpreadFlag) {
				fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, object);
				fxCoderAddSymbol(param, 1, XS_CODE_SYMBOL, ((txPropertyBindingNode*)item)->symbol);
				fxCoderAddByte(param, 0, XS_CODE_AT);
				fxCoderAddByte(param, 0, XS_CODE_SWAP);
				fxCoderAddByte(param, -1, XS_CODE_POP);
				c++;
			}
			fxNodeDispatchCodeReference(((txPropertyBindingNode*)item)->binding, param);
			fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, object);
			fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, ((txPropertyBindingNode*)item)->symbol);
			fxNodeDispatchCodeAssign(((txPropertyBindingNode*)item)->binding, param);
		}
		else {
			fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, object);
			fxNodeDispatchCode(((txPropertyBindingAtNode*)item)->at, param);
			fxCoderAddByte(param, 0, XS_CODE_AT);
			if (self->flags & mxSpreadFlag) {
				fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, at);
				fxCoderAddByte(param, 0, XS_CODE_SWAP);
				fxCoderAddByte(param, -1, XS_CODE_POP);
				c++;
			}
			else {
				fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, at);
				fxCoderAddByte(param, -1, XS_CODE_POP);
			}
			fxNodeDispatchCodeReference(((txPropertyBindingAtNode*)item)->binding, param);
			fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, object);
			fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, at);
			fxCoderAddByte(param, -1, XS_CODE_GET_PROPERTY_AT);
			fxNodeDispatchCodeAssign(((txPropertyBindingAtNode*)item)->binding, param);
		}
		fxCoderAddByte(param, -1, XS_CODE_POP);
		item = item->next;
	}
	if (self->flags & mxSpreadFlag) {
		fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, c);
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddIndex(param, 1, XS_CODE_INTRINSIC, mxCopyObjectIntrinsic);
		fxCoderAddByte(param, -2 - c, XS_CODE_CALL);
		fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, object);
		fxNodeDispatchCodeReference(((txRestBindingNode*)item)->binding, param);
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, object);
		fxNodeDispatchCodeAssign(((txRestBindingNode*)item)->binding, param);
        fxCoderAddByte(param, -1, XS_CODE_POP);
	}
	fxCoderUnuseTemporaryVariables(param, 2);
}

void fxOrExpressionNodeCode(void* it, void* param) 
{
	txBinaryExpressionNode* self = it;
	txTargetCode* endTarget = fxCoderCreateTarget(param);
	self->right->flags |= (self->flags & mxTailRecursionFlag);
	fxNodeDispatchCode(self->left, param);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, endTarget);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxNodeDispatchCode(self->right, param);
	fxCoderAdd(param, 0, endTarget);
}

txInteger fxParamsNodeCode(void* it, void* param) 
{
	txParamsNode* self = it;
	txInteger c = 0;
	if (self->flags & mxSpreadFlag) {
		txInteger counter = fxCoderUseTemporaryVariable(param);
		fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
		fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, counter);
		fxCoderAddByte(param, -1, XS_CODE_POP);
		if (self->items) {
			txNode* item = self->items->first;
			while (item) {
				if (item->description->token == XS_TOKEN_SPREAD) {
					fxSpreadNodeCode(item, param, counter);
				}
				else {
					c++;
					fxNodeDispatchCode(item, param);
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, counter);
					fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 1);
					fxCoderAddByte(param, -1, XS_CODE_ADD);
					fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, counter);
					fxCoderAddByte(param, -1, XS_CODE_POP);
				}
				item = item->next;
			}
		}
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, counter);
		fxCoderUnuseTemporaryVariables(param, 1);
	}
	else {
		if (self->items) {
			txNode* item = self->items->first;
			while (item) {
				fxNodeDispatchCode(item, param);
				c++;
				item = item->next;
			}
			fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, c);
		}
	}
	return c;
}

void fxParamsBindingNodeCode(void* it, void* param) 
{
	txParamsBindingNode* self = it;
	txCoder* coder = param;
	txBoolean evalFlag = coder->evalFlag;
	txNode* item = self->items->first;
	txInteger index = 0;
	if (self->declaration) {
		if (self->mapped)
			fxCoderAddIndex(param, 1, XS_CODE_ARGUMENTS_SLOPPY, self->items->length);
		else
			fxCoderAddIndex(param, 1, XS_CODE_ARGUMENTS_STRICT, self->items->length);
		if (self->declaration->flags & mxDeclareNodeClosureFlag)
			fxCoderAddIndex(param, 0, XS_CODE_VAR_CLOSURE_1, self->declaration->index);
		else 
			fxCoderAddIndex(param, 0, XS_CODE_VAR_LOCAL_1, self->declaration->index);
		fxCoderAddByte(param, -1, XS_CODE_POP);
	}
	while (item) {
		if ((item->flags & mxEvalFlag) && !(item->flags & mxStrictFlag)) {
			fxCoderAddByte(param, 1, XS_CODE_NULL);
			fxCoderAddByte(param, 0, XS_CODE_WITH);
			fxCoderAddByte(param, -1, XS_CODE_POP);
			coder->evalFlag = 1;
		}
		if (item->description->token == XS_TOKEN_REST_BINDING) {
			fxNodeDispatchCodeReference(((txRestBindingNode*)item)->binding, param);
			fxCoderAddIndex(param, 1, XS_CODE_ARGUMENTS, index);
			fxNodeDispatchCodeAssign(((txRestBindingNode*)item)->binding, param);
		}
		else {
			fxNodeDispatchCodeReference(item, param);
			fxCoderAddIndex(param, 1, XS_CODE_ARGUMENT, index);
			fxNodeDispatchCodeAssign(item, param);
		}
		fxCoderAddByte(param, -1, XS_CODE_POP);
		if ((item->flags & mxEvalFlag) && !(item->flags & mxStrictFlag)) {
			coder->evalFlag = evalFlag;
			fxCoderAddByte(param, 0, XS_CODE_WITHOUT);
		}
		item = item->next;
		index++;
	}
}

void fxPostfixExpressionNodeCode(void* it, void* param) 
{
	txPostfixExpressionNode* self = it;
	txCoder* coder = param;
	switch (self->left->description->token) {
	case XS_TOKEN_ACCESS: fxAccessNodeCodePostfix(self->left, param, self); break;
	case XS_TOKEN_MEMBER: fxMemberNodeCodePostfix(self->left, param, self); break;
	case XS_TOKEN_MEMBER_AT: fxMemberAtNodeCodePostfix(self->left, param, self); break;
	default: fxReportLineError(coder->parser, self->line, "no reference");
	}
}

void fxProgramNodeCode(void* it, void* param) 
{
	txProgramNode* self = it;
	txCoder* coder = param;
	
	coder->line = -1;
	coder->programFlag = 1;
	coder->scopeLevel = 0;
	coder->firstBreakTarget = NULL;
	coder->firstContinueTarget = NULL;
	
	if (self->flags & mxStrictFlag)
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_STRICT, 0);
	else
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_SLOPPY, 0);
	coder->path = C_NULL;
	if (coder->parser->flags & mxEvalFlag) {
		coder->evalFlag = 1;
		fxScopeCodingEval(self->scope, param);
	}
	else
		fxScopeCodingProgram(self->scope, param);
	coder->returnTarget = fxCoderCreateTarget(param);
	fxScopeCodeDefineNodes(self->scope, param);
	fxNodeDispatchCode(self->body, param);
	fxCoderAdd(param, 0, coder->returnTarget);
	fxCoderAddByte(param, 0, XS_CODE_RETURN);
}

void fxQuestionMarkNodeCode(void* it, void* param) 
{
	txQuestionMarkNode* self = it;
	txTargetCode* elseTarget = fxCoderCreateTarget(param);
	txTargetCode* endTarget = fxCoderCreateTarget(param);
	self->thenExpression->flags |= (self->flags & mxTailRecursionFlag);
	self->elseExpression->flags |= (self->flags & mxTailRecursionFlag);
	fxNodeDispatchCode(self->expression, param);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, elseTarget);
	fxNodeDispatchCode(self->thenExpression, param);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, endTarget);
	fxCoderAdd(param, -1, elseTarget);
	fxNodeDispatchCode(self->elseExpression, param);
	fxCoderAdd(param, 0, endTarget);
}

void fxRegexpNodeCode(void* it, void* param) 
{
	txRegexpNode* self = it;
	txCoder* coder = param;
	fxNodeDispatchCode(self->modifier, param);
	fxNodeDispatchCode(self->value, param);
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 2);
	fxCoderAddByte(param, 1, XS_CODE_GLOBAL);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_VARIABLE, coder->parser->RegExpSymbol);
	fxCoderAddByte(param, -3, XS_CODE_NEW);
}

void fxReturnNodeCode(void* it, void* param) 
{
	txStatementNode* self = it;
	txCoder* coder = param;
	if (coder->programFlag)
		fxReportLineError(coder->parser, self->line, "invalid return");
	if (((self->flags & (mxStrictFlag | mxGeneratorFlag)) == mxStrictFlag) && (coder->returnTarget->original == NULL))
		self->expression->flags |= mxTailRecursionFlag;
	fxNodeDispatchCode(self->expression, param);
	fxCoderAddByte(param, -1, XS_CODE_RESULT);
	fxCoderAdjustEnvironment(coder, coder->returnTarget);
	fxCoderAdjustScope(coder, coder->returnTarget);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, coder->returnTarget);
}

void fxSpreadNodeCode(void* it, void* param, txInteger counter) 
{
	txSpreadNode* self = it;
	txCoder* coder = param;
	txTargetCode* nextTarget = fxCoderCreateTarget(param);
	txTargetCode* doneTarget = fxCoderCreateTarget(param);
	txInteger iterator;
	fxNodeDispatchCode(self->expression, param);
	fxCoderAddByte(param, 0, XS_CODE_FOR_OF);
	iterator = fxCoderUseTemporaryVariable(param);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, iterator);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAdd(param, 0, nextTarget);
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->nextSymbol);
	fxCoderAddByte(param, -2, XS_CODE_CALL);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->doneSymbol);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, doneTarget);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->valueSymbol);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, counter);
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 1);
	fxCoderAddByte(param, -1, XS_CODE_ADD);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, counter);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, nextTarget);
	fxCoderAdd(param, 1, doneTarget);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderUnuseTemporaryVariables(param, 1);
}

void fxStatementNodeCode(void* it, void* param) 
{
	txStatementNode* self = it;
	txCoder* coder = param;
	if (coder->programFlag) {
		fxNodeDispatchCode(self->expression, param);
		fxCoderAddByte(param, -1, XS_CODE_RESULT);
	}
	else {
		self->expression->flags |= mxExpressionNoValue;
		fxNodeDispatchCode(self->expression, param);
		if (coder->lastCode->id == XS_CODE_SET_CLOSURE_1) {
			coder->lastCode->id = XS_CODE_PULL_CLOSURE_1;
			coder->stackLevel--;
			coder->lastCode->stackLevel = coder->stackLevel;
		}
		else if (coder->lastCode->id == XS_CODE_SET_LOCAL_1) {
			coder->lastCode->id = XS_CODE_PULL_LOCAL_1;
			coder->stackLevel--;
			coder->lastCode->stackLevel = coder->stackLevel;
		}
		else
			fxCoderAddByte(param, -1, XS_CODE_POP);
	}
}

void fxStatementsNodeCode(void* it, void* param) 
{
	txStatementsNode* self = it;
	txNode* item = self->items->first;
	while (item) {
		fxNodeDispatchCode(item, param);
		item = item->next;
	}
}

void fxStringNodeCode(void* it, void* param) 
{
	txStringNode* self = it;
	fxCoderAddString(param, 1, XS_CODE_STRING_1, self->length, self->value);
}

void fxSuperNodeCode(void* it, void* param)
{
	txSuperNode* self = it;
	txInteger c = fxParamsNodeCode(self->params, param);
	fxCoderAddByte(param, 0 - c, XS_CODE_SUPER);
	fxCoderAddByte(param, 0, XS_CODE_SET_THIS);
}

void fxSwitchNodeCode(void* it, void* param) 
{
	txSwitchNode* self = it;
	txCoder* coder = param;
	txTargetCode* breakTarget;
	txCaseNode* caseNode;
	txCaseNode* defaultNode = NULL;
	fxNodeDispatchCode(self->expression, param);
	fxScopeCodingBlock(self->scope, param);
	breakTarget = fxCoderCreateTarget(coder);
	breakTarget->label = fxNewParserChunkClear(coder->parser, sizeof(txLabelNode));
	breakTarget->nextTarget = coder->firstBreakTarget;
	coder->firstBreakTarget = breakTarget;
	if (coder->programFlag) {
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_RESULT);
	}
	caseNode = (txCaseNode*)self->items->first;
	while (caseNode) {
		caseNode->target = fxCoderCreateTarget(param);
		if (caseNode->expression) {
			fxCoderAddByte(param, 1, XS_CODE_DUB);
			fxNodeDispatchCode(caseNode->expression, param);
			fxCoderAddByte(param, -1, XS_CODE_STRICT_EQUAL);
			fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, caseNode->target);
		}
		else
			defaultNode = caseNode;
		caseNode = (txCaseNode*)caseNode->next;
	}
	if (defaultNode)
		fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, defaultNode->target);
	else
		fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, coder->firstBreakTarget);
	caseNode = (txCaseNode*)self->items->first;
	while (caseNode) {
		fxCoderAdd(param, 0, caseNode->target);
		if (caseNode->statement)
			fxNodeDispatchCode(caseNode->statement, param);
		caseNode = (txCaseNode*)caseNode->next;
	}
	fxCoderAdd(param, 0, coder->firstBreakTarget);
	coder->firstBreakTarget = breakTarget->nextTarget;
	fxScopeCoded(self->scope, param);
	fxCoderAddByte(param, -1, XS_CODE_POP);
}

void fxTemplateNodeCode(void* it, void* param) 
{
	txTemplateNode* self = it;
	txCoder* coder = param;
	txParser* parser = coder->parser;
	txNode* item = self->items->first;
	
	if (self->reference) {
		txInteger i = (self->items->length / 2) + 1;
		txInteger raws = fxCoderUseTemporaryVariable(param);
		txInteger strings = fxCoderUseTemporaryVariable(param);
		txFlag flag = XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;

		fxCoderAddByte(param, 1, XS_CODE_ARRAY);
		fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, strings);
		fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, i);
		fxCoderAddSymbol(param, -1, XS_CODE_SET_PROPERTY, coder->parser->lengthSymbol);
		fxCoderAddByte(param, -1, XS_CODE_POP);
		fxCoderAddByte(param, 1, XS_CODE_ARRAY);
		fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, raws);
		fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, i);
		fxCoderAddSymbol(param, -1, XS_CODE_SET_PROPERTY, coder->parser->lengthSymbol);
		fxCoderAddByte(param, -1, XS_CODE_POP);
		i = 0;
		while (item) {
			if (item->description->token == XS_TOKEN_TEMPLATE_MIDDLE) {
		
				fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, strings);
				fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, i);
				fxCoderAddByte(param, 0, XS_CODE_AT);
				if (((txTemplateItemNode*)item)->string->flags & mxStringErrorFlag)
					fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
				else
					fxNodeDispatchCode(((txTemplateItemNode*)item)->string, param);
				fxCoderAddFlag(param, -3, XS_CODE_NEW_PROPERTY, flag);
			
				fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, raws);
				fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, i);
				fxCoderAddByte(param, 0, XS_CODE_AT);
				fxNodeDispatchCode(((txTemplateItemNode*)item)->raw, param);
				fxCoderAddFlag(param, -3, XS_CODE_NEW_PROPERTY, flag);
			
				i++;
			}
			item = item->next;
		}
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, strings);
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, raws);
		fxCoderAddSymbol(param, -1, XS_CODE_SET_PROPERTY, parser->rawSymbol);
		fxCoderAddByte(param, -1, XS_CODE_POP);
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, strings);
		fxCoderAddByte(param, 0, XS_CODE_TEMPLATE);
		i = 1;
		item = self->items->first;
		while (item) {
			if (item->description->token != XS_TOKEN_TEMPLATE_MIDDLE) {
				fxNodeDispatchCode(item, param);
				i++;
			}
			item = item->next;
		}
		fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, i);
		switch (self->reference->description->token) {
		case XS_TOKEN_MEMBER: 
			fxMemberNodeCodeCall(self->reference, param);
			fxCoderAddByte(param, -2 - i, (self->flags & mxTailRecursionFlag) ? XS_CODE_CALL_TAIL : XS_CODE_CALL);
			break;
		case XS_TOKEN_MEMBER_AT: 
			fxMemberAtNodeCodeCall(self->reference, param);
			fxCoderAddByte(param, -2 - i, (self->flags & mxTailRecursionFlag) ? XS_CODE_CALL_TAIL : XS_CODE_CALL);
			break;
		default: 	
			fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
			fxNodeDispatchCode(self->reference, param);
			fxCoderAddByte(param, -2 - i, (self->flags & mxTailRecursionFlag) ? XS_CODE_CALL_TAIL : XS_CODE_CALL);
			break;
		}
		fxCoderUnuseTemporaryVariables(coder, 2);
	}
	else {
        if (((txTemplateItemNode*)item)->string->flags & mxStringErrorFlag)
            fxReportLineError(parser, item->line, "invalid escape sequence");
		fxNodeDispatchCode(((txTemplateItemNode*)item)->string, param);
		item = item->next;
		while (item) {
			if (item->description->token == XS_TOKEN_TEMPLATE_MIDDLE) {
				if (((txTemplateItemNode*)item)->string->flags & mxStringErrorFlag)
					fxReportLineError(parser, item->line, "invalid escape sequence");
				fxNodeDispatchCode(((txTemplateItemNode*)item)->string, param);
			}
			else {
				fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
				fxNodeDispatchCode(item, param);
				fxCoderAddByte(param, 1, XS_CODE_DUB);
				fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, parser->toStringSymbol);
				fxCoderAddByte(param, -2, XS_CODE_CALL);
			}
			fxCoderAddByte(param, -1, XS_CODE_ADD);
			item = item->next;
		}
	}
}

void fxThisNodeCode(void* it, void* param) 
{
	txNode* self = it;
	if (self->flags & mxDerivedFlag)
		fxCoderAddByte(param, 1, XS_CODE_GET_THIS);
	else
		fxCoderAddByte(param, 1, self->description->code);
}

void fxThrowNodeCode(void* it, void* param) 
{
	txStatementNode* self = it;
	fxNodeDispatchCode(self->expression, param);
	fxCoderAddByte(param, -1, XS_CODE_THROW);
}

void fxTryNodeCode(void* it, void* param) 
{
	txTryNode* self = it;
	txCoder* coder = param;
	txInteger exception;
	txInteger selector;
	txInteger selection;
	txTargetCode* catchTarget;
	txTargetCode* normalTarget;
	txTargetCode* finallyTarget;

	exception = fxCoderUseTemporaryVariable(coder);
	selector = fxCoderUseTemporaryVariable(coder);

	coder->firstBreakTarget = fxCoderAliasTargets(param, coder->firstBreakTarget);
	coder->firstContinueTarget = fxCoderAliasTargets(param, coder->firstContinueTarget);
	coder->returnTarget = fxCoderAliasTargets(param, coder->returnTarget);
	catchTarget = fxCoderCreateTarget(param);
	normalTarget = fxCoderCreateTarget(param);
	finallyTarget = fxCoderCreateTarget(param);

	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, 0);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, selector);
	fxCoderAddByte(param, -1, XS_CODE_POP);

	fxCoderAddBranch(param, 0, XS_CODE_CATCH_1, catchTarget);
	fxNodeDispatchCode(self->tryBlock, param);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, normalTarget);
	if (self->catchBlock) {
		fxCoderAddByte(param, 0, XS_CODE_UNCATCH);
		fxCoderAdd(param, 0, catchTarget);
		catchTarget = fxCoderCreateTarget(param);
		fxCoderAddBranch(param, 0, XS_CODE_CATCH_1, catchTarget);
		fxNodeDispatchCode(self->catchBlock, param);
		fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, normalTarget);
	}
	
	selection = 1;
	coder->firstBreakTarget = fxCoderFinalizeTargets(param, coder->firstBreakTarget, selector, &selection, finallyTarget);
	coder->firstContinueTarget = fxCoderFinalizeTargets(param, coder->firstContinueTarget, selector, &selection, finallyTarget);
	coder->returnTarget = fxCoderFinalizeTargets(param, coder->returnTarget, selector, &selection, finallyTarget);
	fxCoderAdd(param, 0, normalTarget);
	fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, selection);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, selector);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAdd(param, 0, finallyTarget);
	fxCoderAddByte(param, 0, XS_CODE_UNCATCH);
	fxCoderAdd(param, 0, catchTarget);
	fxCoderAddByte(param, 1, XS_CODE_EXCEPTION);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, exception);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	if (self->finallyBlock) {
		txBoolean programFlag = coder->programFlag;
		coder->programFlag = 0; // no result
		fxNodeDispatchCode(self->finallyBlock, param);
		coder->programFlag = programFlag;
	}
	catchTarget = fxCoderCreateTarget(param);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, selector);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, catchTarget);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, exception);
	fxCoderAddByte(param, -1, XS_CODE_THROW);
	fxCoderAdd(param, 0, catchTarget);
	selection = 1;
	fxCoderJumpTargets(param, coder->firstBreakTarget, selector, &selection);
	fxCoderJumpTargets(param, coder->firstContinueTarget, selector, &selection);
	fxCoderJumpTargets(param, coder->returnTarget, selector, &selection);
	fxCoderUnuseTemporaryVariables(coder, 2);
}

void fxUnaryExpressionNodeCode(void* it, void* param) 
{
	txUnaryExpressionNode* self = it;
	fxNodeDispatchCode(self->right, param);
	fxCoderAddByte(param, 0, self->description->code);
}

void fxUndefinedNodeCodeAssign(void* it, void* param) 
{
	txCoder* coder = param;
	fxCoderAddSymbol(param, -1, XS_CODE_SET_VARIABLE, coder->parser->undefinedSymbol);
}

void fxUndefinedNodeCodeReference(void* it, void* param) 
{
	txCoder* coder = param;
	if (coder->evalFlag)
		fxCoderAddSymbol(param, 1, XS_CODE_EVAL_REFERENCE, coder->parser->undefinedSymbol);
	else
		fxCoderAddSymbol(param, 1, XS_CODE_PROGRAM_REFERENCE, coder->parser->undefinedSymbol);
}

void fxValueNodeCode(void* it, void* param) 
{
	txNode* self = it;
	fxCoderAddByte(param, 1, self->description->code);
}

void fxWhileNodeCode(void* it, void* param) 
{
	txWhileNode* self = it;
	txCoder* coder = param;
	if (coder->programFlag) {
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_RESULT);
	}
	fxCoderAdd(param, 0, coder->firstContinueTarget);
	fxNodeDispatchCode(self->expression, param);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, coder->firstBreakTarget);
	fxNodeDispatchCode(self->statement, param);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, coder->firstContinueTarget);
}

void fxWithNodeCode(void* it, void* param)
{
	txWithNode* self = it;
	txCoder* coder = param;
	txBoolean evalFlag;
	fxNodeDispatchCode(self->expression, param);
	fxCoderAddByte(param, 0, XS_CODE_TO_INSTANCE);
	fxCoderAddByte(param, 0, XS_CODE_WITH);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	evalFlag = coder->evalFlag;
	coder->environmentLevel++;
	coder->evalFlag = 1;
	fxNodeDispatchCode(self->statement, param);
	coder->evalFlag = evalFlag;
	coder->environmentLevel--;
	fxCoderAddByte(param, 0, XS_CODE_WITHOUT);
}

void fxYieldNodeCode(void* it, void* param) 
{
	txStatementNode* self = it;
	txCoder* coder = param;
	txTargetCode* target = fxCoderCreateTarget(coder);
	fxCoderAddByte(param, 1, XS_CODE_OBJECT);
	
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddSymbol(param, 1, XS_CODE_SYMBOL, coder->parser->valueSymbol);
	fxCoderAddByte(param, 0, XS_CODE_AT);
	fxNodeDispatchCode(self->expression, param);
	fxCoderAddFlag(param, -3, XS_CODE_NEW_PROPERTY, 0);
	
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddSymbol(param, 1, XS_CODE_SYMBOL, coder->parser->doneSymbol);
	fxCoderAddByte(param, 0, XS_CODE_AT);
	fxCoderAddByte(param, 1, XS_CODE_FALSE);
	fxCoderAddFlag(param, -3, XS_CODE_NEW_PROPERTY, 0);
	
	fxCoderAddByte(coder, 0, XS_CODE_YIELD);
	fxCoderAddBranch(coder, 1, XS_CODE_BRANCH_STATUS_1, target);
	fxCoderAddByte(param, -1, XS_CODE_RESULT);
	fxCoderAdjustEnvironment(coder, coder->returnTarget);
	fxCoderAdjustScope(coder, coder->returnTarget);
	fxCoderAddBranch(coder, 0, XS_CODE_BRANCH_1, coder->returnTarget);
	fxCoderAdd(coder, 0, target);
}


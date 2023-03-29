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
	
struct sxBigIntCode {
	mxByteCodePart;
	txBigInt bigint;
};
	
struct sxBranchCode {
	mxByteCodePart;
	txTargetCode* target;
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
	txBoolean importFlag;
	txBoolean importMetaFlag;
	txClassNode* classNode;
	txTargetCode* chainTarget;
};

typedef void (*txCompound)(void* it, void* param, txByte step);

static void fxCoderAdd(txCoder* self, txInteger delta, void* it);
static void fxCoderAddBigInt(txCoder* self, txInteger delta, txInteger id, txBigInt* bigint);
static void fxCoderAddBranch(txCoder* self, txInteger delta, txInteger id, txTargetCode* target);
static void fxCoderAddByte(txCoder* self, txInteger delta, txInteger id);
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
static void fxCoderOptimize(txCoder* self);
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
static void fxNodeDispatchCodeAssign(void* it, void* param, txFlag flag);
static void fxNodeDispatchCodeDelete(void* it, void* param);
static void fxNodeDispatchCodeReference(void* it, void* param);
static txFlag fxNodeDispatchCodeThis(void* it, void* param, txFlag flag);

static txFlag fxNodeCodeName(txNode* value);
static void fxCompoundExpressionNodeCodeName(void* it, void* param);
static void fxSpreadNodeCode(void* it, void* param, txInteger counter);

txScript* fxParserCode(txParser* parser)
{
	txCoder coder;
	txByteCode* code;
	txScript* script;
	txSize size, delta, offset;
	txSymbol* symbol;
	txSymbol** address;
	txSize c, i;
	txID id, count;
	txSize total;
	txByte* p;
	txHostNode* node;

    c_memset(&coder, 0, sizeof(txCoder));
	coder.parser = parser;
	if (parser->errorCount == 0) {
		mxTryParser(parser) {
			txNode* self = parser->root;
			(*self->description->dispatch->code)(parser->root, &coder);
		}
		mxCatchParser(parser) {
		}
	}
	if (parser->errorCount) {
		if (parser->console) {
			coder.firstCode = NULL;
			coder.lastCode = NULL;
			fxCoderAddByte(&coder, 1, XS_CODE_GLOBAL);
			fxCoderAddSymbol(&coder, 0, XS_CODE_GET_VARIABLE, parser->errorSymbol);
			fxCoderAddByte(&coder, 2, XS_CODE_NEW);
			fxCoderAddString(&coder, 1, XS_CODE_STRING_1, mxStringLength(parser->errorMessage), parser->errorMessage);
			fxCoderAddInteger(&coder, -3, XS_CODE_RUN_1, 1);
			fxCoderAddByte(&coder, -1, XS_CODE_THROW);
		}
		else
			return C_NULL;
	}
	
	fxCoderOptimize(&coder);
	
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
		case XS_CODE_BRANCH_CHAIN_1:
		case XS_CODE_BRANCH_COALESCE_1:
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
		case XS_CODE_BEGIN_STRICT_FIELD:
		case XS_CODE_MODULE:
			size += 2;
			break;

		case XS_CODE_LINE:
			size += 3;
			break;
		case XS_CODE_ASYNC_FUNCTION:
		case XS_CODE_ASYNC_GENERATOR_FUNCTION:
		case XS_CODE_CONSTRUCTOR_FUNCTION:
		case XS_CODE_DELETE_PROPERTY:
		case XS_CODE_DELETE_SUPER:
		case XS_CODE_FILE:
		case XS_CODE_FUNCTION:
		case XS_CODE_GENERATOR_FUNCTION:
		case XS_CODE_GET_PROPERTY:
		case XS_CODE_GET_SUPER:
		case XS_CODE_GET_THIS_VARIABLE:
		case XS_CODE_GET_VARIABLE:
		case XS_CODE_EVAL_PRIVATE:
		case XS_CODE_EVAL_REFERENCE:
		case XS_CODE_NAME:
		case XS_CODE_NEW_CLOSURE:
		case XS_CODE_NEW_LOCAL:
		case XS_CODE_NEW_PROPERTY:
		case XS_CODE_PROGRAM_REFERENCE:
		case XS_CODE_SET_PROPERTY:
		case XS_CODE_SET_SUPER:
		case XS_CODE_SET_VARIABLE:
		case XS_CODE_SYMBOL:
		case XS_CODE_PROFILE:
			size += 1 + sizeof(txID);
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
		case XS_CODE_BIGINT_1:
			value = fxBigIntMeasure(&((txBigIntCode*)code)->bigint);
			if (value > 255) {
				code->id += 1;
				size += 3;
			}
			else
				size += 2;
			size += value;
			break;
			
		case XS_CODE_CONST_CLOSURE_1:
		case XS_CODE_CONST_LOCAL_1:
		case XS_CODE_GET_CLOSURE_1:
		case XS_CODE_GET_LOCAL_1:
		case XS_CODE_GET_PRIVATE_1:
		case XS_CODE_HAS_PRIVATE_1:
		case XS_CODE_LET_CLOSURE_1:
		case XS_CODE_LET_LOCAL_1:
		case XS_CODE_NEW_PRIVATE_1:
		case XS_CODE_PULL_CLOSURE_1:
		case XS_CODE_PULL_LOCAL_1:
		case XS_CODE_REFRESH_CLOSURE_1:
		case XS_CODE_REFRESH_LOCAL_1:
		case XS_CODE_RESET_CLOSURE_1:
		case XS_CODE_RESET_LOCAL_1:
		case XS_CODE_SET_CLOSURE_1:
		case XS_CODE_SET_LOCAL_1:
		case XS_CODE_SET_PRIVATE_1:
		case XS_CODE_STORE_1:
		case XS_CODE_VAR_CLOSURE_1:
		case XS_CODE_VAR_LOCAL_1:
			value = ((txIndexCode*)code)->index + 1;
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
		case XS_CODE_RUN_1: 
		case XS_CODE_RUN_TAIL_1: 
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
		case XS_CODE_BRANCH_CHAIN_1:
		case XS_CODE_BRANCH_COALESCE_1:
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
		case XS_CODE_BEGIN_STRICT_FIELD:
		case XS_CODE_MODULE:
			size += 2;
			break;
		case XS_CODE_LINE:
			size += 3;
			break;
		case XS_CODE_ASYNC_FUNCTION:
		case XS_CODE_ASYNC_GENERATOR_FUNCTION:
		case XS_CODE_CONSTRUCTOR_FUNCTION:
		case XS_CODE_DELETE_PROPERTY:
		case XS_CODE_DELETE_SUPER:
		case XS_CODE_FILE:
		case XS_CODE_FUNCTION:
		case XS_CODE_GENERATOR_FUNCTION:
		case XS_CODE_GET_PROPERTY:
		case XS_CODE_GET_SUPER:
		case XS_CODE_GET_THIS_VARIABLE:
		case XS_CODE_GET_VARIABLE:
		case XS_CODE_EVAL_PRIVATE:
		case XS_CODE_EVAL_REFERENCE:
		case XS_CODE_NAME:
		case XS_CODE_NEW_CLOSURE:
		case XS_CODE_NEW_LOCAL:
		case XS_CODE_NEW_PROPERTY:
		case XS_CODE_PROGRAM_REFERENCE:
		case XS_CODE_SET_PROPERTY:
		case XS_CODE_SET_SUPER:
		case XS_CODE_SET_VARIABLE:
		case XS_CODE_SYMBOL:
			symbol = ((txSymbolCode*)code)->symbol;
			if (symbol && symbol->string)
				symbol->usage++;
			size += 1 + sizeof(txID);
			break;
		case XS_CODE_PROFILE:
			size += 1 + sizeof(txID);
			break;
			
		case XS_CODE_CONST_CLOSURE_1:
		case XS_CODE_CONST_LOCAL_1:
		case XS_CODE_GET_CLOSURE_1:
		case XS_CODE_GET_LOCAL_1:
		case XS_CODE_GET_PRIVATE_1:
		case XS_CODE_HAS_PRIVATE_1:
		case XS_CODE_LET_CLOSURE_1:
		case XS_CODE_LET_LOCAL_1:
		case XS_CODE_NEW_PRIVATE_1:
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
		case XS_CODE_SET_PRIVATE_1:
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
		case XS_CODE_GET_PRIVATE_2:
		case XS_CODE_HAS_PRIVATE_2:
		case XS_CODE_LET_CLOSURE_2:
		case XS_CODE_LET_LOCAL_2:
		case XS_CODE_NEW_PRIVATE_2:
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
		case XS_CODE_SET_PRIVATE_2:
		case XS_CODE_STORE_2:
		case XS_CODE_VAR_CLOSURE_2:
		case XS_CODE_VAR_LOCAL_2:
		case XS_CODE_UNWIND_2:
			size += 3;
			break;
		
		case XS_CODE_INTEGER_1: 
		case XS_CODE_RUN_1: 
		case XS_CODE_RUN_TAIL_1: 
			size += 2;
			break;
		case XS_CODE_INTEGER_2: 
		case XS_CODE_RUN_2: 
		case XS_CODE_RUN_TAIL_2: 
			size += 3;
			break;
		case XS_CODE_INTEGER_4: 
		case XS_CODE_RUN_4: 
		case XS_CODE_RUN_TAIL_4: 
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
		case XS_CODE_STRING_4:
			size += 5 + ((txStringCode*)code)->length;
			break;
		case XS_CODE_BIGINT_1:
			size += 2 + fxBigIntMeasure(&((txBigIntCode*)code)->bigint);
			break;
		case XS_CODE_BIGINT_2:
			size += 3 + fxBigIntMeasure(&((txBigIntCode*)code)->bigint);
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
	id = 1;
	total = sizeof(txID);
	for (i = 0; i < c; i++) {
		txSymbol* symbol = *address;
		while (symbol) {
			if (symbol->usage) {
				symbol->ID = id;
				id++;
				total += symbol->length;
			}
			symbol = symbol->next;
		}
		address++;
	}
	count = id;
		
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
		case XS_CODE_BRANCH_CHAIN_1:
		case XS_CODE_BRANCH_COALESCE_1:
		case XS_CODE_BRANCH_ELSE_1:
		case XS_CODE_BRANCH_IF_1:
		case XS_CODE_BRANCH_STATUS_1:
		case XS_CODE_CATCH_1:
		case XS_CODE_CODE_1:
			offset = mxPtrDiff(p + 1 - script->codeBuffer);
			s1 = (txS1)(((txBranchCode*)code)->target->offset - offset);
			*p++ = s1;
			break;
		case XS_CODE_BRANCH_2:
		case XS_CODE_BRANCH_CHAIN_2:
		case XS_CODE_BRANCH_COALESCE_2:
		case XS_CODE_BRANCH_ELSE_2:
		case XS_CODE_BRANCH_IF_2:
		case XS_CODE_BRANCH_STATUS_2:
		case XS_CODE_CATCH_2:
		case XS_CODE_CODE_2:
			offset = mxPtrDiff(p + 2 - script->codeBuffer);
			s2 = (txS2)(((txBranchCode*)code)->target->offset - offset);
			mxEncode2(p, s2);
			break;
		case XS_CODE_BRANCH_4:
		case XS_CODE_BRANCH_CHAIN_4:
		case XS_CODE_BRANCH_COALESCE_4:
		case XS_CODE_BRANCH_ELSE_4:
		case XS_CODE_BRANCH_IF_4:
		case XS_CODE_BRANCH_STATUS_4:
		case XS_CODE_CATCH_4:
		case XS_CODE_CODE_4:
			offset = mxPtrDiff(p + 4 - script->codeBuffer);
			s4 = (txS4)(((txBranchCode*)code)->target->offset  - offset);
			mxEncode4(p, s4);
			break;
			
		case XS_CODE_ASYNC_FUNCTION:
		case XS_CODE_ASYNC_GENERATOR_FUNCTION:
		case XS_CODE_CONSTRUCTOR_FUNCTION:
		case XS_CODE_DELETE_PROPERTY:
		case XS_CODE_DELETE_SUPER:
		case XS_CODE_FILE:
		case XS_CODE_FUNCTION:
		case XS_CODE_GENERATOR_FUNCTION:
		case XS_CODE_GET_PROPERTY:
		case XS_CODE_GET_SUPER:
		case XS_CODE_GET_THIS_VARIABLE:
		case XS_CODE_GET_VARIABLE:
		case XS_CODE_EVAL_PRIVATE:
		case XS_CODE_EVAL_REFERENCE:
		case XS_CODE_NAME:
		case XS_CODE_NEW_CLOSURE:
		case XS_CODE_NEW_LOCAL:
		case XS_CODE_NEW_PROPERTY:
		case XS_CODE_PROGRAM_REFERENCE:
		case XS_CODE_SET_PROPERTY:
		case XS_CODE_SET_SUPER:
		case XS_CODE_SET_VARIABLE:
		case XS_CODE_SYMBOL:
			symbol = ((txSymbolCode*)code)->symbol;
			if (symbol && symbol->string)
				id = symbol->ID;
			else
				id = XS_NO_ID;
			mxEncodeID(p, id);
			break;
		case XS_CODE_PROFILE:
			id = fxGenerateProfileID(parser->console);
			mxEncodeID(p, id);
			break;
			
		case XS_CODE_ARGUMENT:
		case XS_CODE_ARGUMENTS:
		case XS_CODE_ARGUMENTS_SLOPPY:
		case XS_CODE_ARGUMENTS_STRICT:
		case XS_CODE_BEGIN_SLOPPY:
		case XS_CODE_BEGIN_STRICT:
		case XS_CODE_BEGIN_STRICT_BASE:
		case XS_CODE_BEGIN_STRICT_DERIVED:
		case XS_CODE_BEGIN_STRICT_FIELD:
		case XS_CODE_MODULE:
		case XS_CODE_RESERVE_1:
		case XS_CODE_RETRIEVE_1:
		case XS_CODE_UNWIND_1:
			u1 = (txU1)(((txIndexCode*)code)->index);
			*((txU1*)p++) = u1;
			break;
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
		case XS_CODE_GET_PRIVATE_1:
		case XS_CODE_HAS_PRIVATE_1:
		case XS_CODE_LET_CLOSURE_1:
		case XS_CODE_LET_LOCAL_1:
		case XS_CODE_NEW_PRIVATE_1:
		case XS_CODE_PULL_CLOSURE_1:
		case XS_CODE_PULL_LOCAL_1:
		case XS_CODE_REFRESH_CLOSURE_1:
		case XS_CODE_REFRESH_LOCAL_1:
		case XS_CODE_RESET_CLOSURE_1:
		case XS_CODE_RESET_LOCAL_1:
		case XS_CODE_SET_CLOSURE_1:
		case XS_CODE_SET_LOCAL_1:
		case XS_CODE_SET_PRIVATE_1:
		case XS_CODE_STORE_1:
		case XS_CODE_VAR_CLOSURE_1:
		case XS_CODE_VAR_LOCAL_1:
			u1 = (txU1)(((txIndexCode*)code)->index + 1);
			*((txU1*)p++) = u1;
			break;

		case XS_CODE_CONST_CLOSURE_2:
		case XS_CODE_CONST_LOCAL_2:
		case XS_CODE_GET_CLOSURE_2:
		case XS_CODE_GET_LOCAL_2:
		case XS_CODE_GET_PRIVATE_2:
		case XS_CODE_HAS_PRIVATE_2:
		case XS_CODE_LET_CLOSURE_2:
		case XS_CODE_LET_LOCAL_2:
		case XS_CODE_NEW_PRIVATE_2:
		case XS_CODE_PULL_CLOSURE_2:
		case XS_CODE_PULL_LOCAL_2:
		case XS_CODE_REFRESH_CLOSURE_2:
		case XS_CODE_REFRESH_LOCAL_2:
		case XS_CODE_RESET_CLOSURE_2:
		case XS_CODE_RESET_LOCAL_2:
		case XS_CODE_SET_CLOSURE_2:
		case XS_CODE_SET_LOCAL_2:
		case XS_CODE_SET_PRIVATE_2:
		case XS_CODE_STORE_2:
		case XS_CODE_VAR_CLOSURE_2:
		case XS_CODE_VAR_LOCAL_2:
			u2 = (txU2)(((txIndexCode*)code)->index + 1);
			mxEncode2(p, u2);
			break;
	
		case XS_CODE_INTEGER_1: 
		case XS_CODE_RUN_1: 
		case XS_CODE_RUN_TAIL_1: 
			s1 = (txS1)(((txIntegerCode*)code)->integer);
			*p++ = s1;
			break;
		case XS_CODE_INTEGER_2: 
		case XS_CODE_RUN_2: 
		case XS_CODE_RUN_TAIL_2: 
			s2 = (txS2)(((txIntegerCode*)code)->integer);
			mxEncode2(p, s2);
			break;
		case XS_CODE_INTEGER_4: 
		case XS_CODE_RUN_4: 
		case XS_CODE_RUN_TAIL_4: 
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
		case XS_CODE_STRING_4:
			s4 = (txS4)(((txStringCode*)code)->length);
			mxEncode4(p, s4);
			c_memcpy(p, ((txStringCode*)code)->string, s4);
			p += s4;
			break;
		case XS_CODE_BIGINT_1:
			u1 = (txU1)fxBigIntMeasure(&((txBigIntCode*)code)->bigint);
			*((txU1*)p++) = u1;
			fxBigIntEncode(p, &((txBigIntCode*)code)->bigint, u1);
			p += u1;
			break;
		case XS_CODE_BIGINT_2:
			u2 = (txU2)fxBigIntMeasure(&((txBigIntCode*)code)->bigint);
            mxEncode2(p, u2);
			fxBigIntEncode(p, &((txBigIntCode*)code)->bigint, u2);
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
		case XS_CODE_BRANCH_CHAIN_1:
		case XS_CODE_BRANCH_CHAIN_2:
		case XS_CODE_BRANCH_CHAIN_4:
		case XS_CODE_BRANCH_COALESCE_1:
		case XS_CODE_BRANCH_COALESCE_2:
		case XS_CODE_BRANCH_COALESCE_4:
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
		case XS_CODE_BEGIN_STRICT_FIELD:
		case XS_CODE_LINE:
		case XS_CODE_MODULE:
			fprintf(stderr, "%s %d\n", gxCodeNames[code->id], ((txIndexCode*)code)->index);
			break;
			
		case XS_CODE_ASYNC_FUNCTION:
		case XS_CODE_ASYNC_GENERATOR_FUNCTION:
		case XS_CODE_CONSTRUCTOR_FUNCTION:
		case XS_CODE_DELETE_PROPERTY:
		case XS_CODE_DELETE_SUPER:
		case XS_CODE_FILE:
		case XS_CODE_FUNCTION:
		case XS_CODE_GENERATOR_FUNCTION:
		case XS_CODE_GET_PROPERTY:
		case XS_CODE_GET_SUPER:
		case XS_CODE_GET_THIS_VARIABLE:
		case XS_CODE_GET_VARIABLE:
		case XS_CODE_EVAL_PRIVATE:
		case XS_CODE_EVAL_REFERENCE:
		case XS_CODE_NAME:
		case XS_CODE_NEW_PROPERTY:
		case XS_CODE_PROGRAM_REFERENCE:
		case XS_CODE_SET_PROPERTY:
		case XS_CODE_SET_SUPER:
		case XS_CODE_SET_VARIABLE:
		case XS_CODE_SYMBOL:
			symbol = ((txSymbolCode*)code)->symbol;
			if (symbol && symbol->string)
				fprintf(stderr, "%s %s\n", gxCodeNames[code->id], symbol->string);
			else
				fprintf(stderr, "%s ?\n", gxCodeNames[code->id]);
			break;
		
		case XS_CODE_CONST_CLOSURE_1:
		case XS_CODE_CONST_CLOSURE_2:
		case XS_CODE_CONST_LOCAL_1:
		case XS_CODE_CONST_LOCAL_2:
		case XS_CODE_GET_CLOSURE_1:
		case XS_CODE_GET_CLOSURE_2:
		case XS_CODE_GET_LOCAL_1:
		case XS_CODE_GET_LOCAL_2:
		case XS_CODE_GET_PRIVATE_1:
		case XS_CODE_GET_PRIVATE_2:
		case XS_CODE_HAS_PRIVATE_1:
		case XS_CODE_HAS_PRIVATE_2:
		case XS_CODE_LET_CLOSURE_1:
		case XS_CODE_LET_CLOSURE_2:
		case XS_CODE_LET_LOCAL_1:
		case XS_CODE_LET_LOCAL_2:
		case XS_CODE_NEW_PRIVATE_1:
		case XS_CODE_NEW_PRIVATE_2:
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
		case XS_CODE_SET_PRIVATE_1:
		case XS_CODE_SET_PRIVATE_2:
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
		case XS_CODE_RUN_1: 
		case XS_CODE_RUN_2: 
		case XS_CODE_RUN_4: 
		case XS_CODE_RUN_TAIL_1: 
		case XS_CODE_RUN_TAIL_2: 
		case XS_CODE_RUN_TAIL_4: 
			fprintf(stderr, "%s %d\n", gxCodeNames[code->id], ((txIntegerCode*)code)->integer);
			break;
		case XS_CODE_NUMBER:
			fprintf(stderr, "%s %lf\n", gxCodeNames[code->id], ((txNumberCode*)code)->number);
			break;
		case XS_CODE_STRING_1:
		case XS_CODE_STRING_2:
		case XS_CODE_STRING_4:
			fprintf(stderr, "%s %d \"%s\"\n", gxCodeNames[code->id], ((txStringCode*)code)->length, ((txStringCode*)code)->string);
			break;
		case XS_CODE_BIGINT_1:
		case XS_CODE_BIGINT_2:
			fprintf(stderr, "%s %d\n", gxCodeNames[code->id], fxBigIntMeasure(&((txBigIntCode*)code)->bigint));
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
	mxEncodeID(p, count);
	
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
		size = sizeof(txID);
		node = parser->firstHostNode;
		while (node) {
			size += 1 + sizeof(txID) + node->at->length + 1;
			node = node->nextHostNode;
		}
	
		script->hostsBuffer = c_malloc(size);
		if (!script->hostsBuffer) goto bail;
		script->hostsSize = size;
	
		p = script->hostsBuffer;
		mxEncodeID(p, c);
		node = parser->firstHostNode;
		while (node) {
			*p++ = (txS1)(node->paramsCount);
			if (node->symbol)
				c = node->symbol->ID;
			else
				c = XS_NO_ID;
			mxEncodeID(p, c);
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
		c_fprintf(stderr, "# oops %d\n", code->id);		//@@
}

void fxCoderAddBigInt(txCoder* self, txInteger delta, txInteger id, txBigInt* bigint)
{
	txBigIntCode* code = fxNewParserChunkClear(self->parser, sizeof(txBigIntCode));
	fxCoderAdd(self, delta, code);
	code->id = id;
	code->bigint = *bigint;
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
			if (node->path != self->parser->path) {
				node->path = self->parser->path;
				node->line = self->parser->lines[node->line];
			}
		}
		else if (self->parser->source) {
			node->path = self->parser->source;
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
		if ((item->description->token != XS_TOKEN_ARG) && (item->description->token != XS_TOKEN_ARRAY_BINDING) && (item->description->token != XS_TOKEN_OBJECT_BINDING))
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

void fxCoderOptimize(txCoder* self)
{
	txByteCode** address;
	txByteCode* code;
	
	// branch to (target | unwind)* end => end
	address = &self->firstCode;
	while ((code = *address)) {
		if (code->id == XS_CODE_BRANCH_1) {
			txByteCode* nextCode = ((txBranchCode*)code)->target->nextCode;
			while ((nextCode->id == XS_NO_CODE) || (nextCode->id == XS_CODE_UNWIND_1))
				nextCode = nextCode->nextCode;
			if ((XS_CODE_END <= nextCode->id) && (nextCode->id <= XS_CODE_END_DERIVED)) {
				txByteCode* end = fxNewParserChunkClear(self->parser, sizeof(txByteCode));
				end->nextCode = code->nextCode;
				end->id = nextCode->id;
				end->stackLevel = code->stackLevel;
				*address = end;
			}
			else
				address = &code->nextCode;
		}
		else
			address = &code->nextCode;
	}
	// unwind (target | unwind)* end => (target | unwind)* end
	address = &self->firstCode;
	while ((code = *address)) {
		if (code->id == XS_CODE_UNWIND_1) {
			txByteCode* nextCode = code->nextCode;
			while ((nextCode->id == XS_NO_CODE) || (nextCode->id == XS_CODE_UNWIND_1))
				nextCode = nextCode->nextCode;
			if ((XS_CODE_END <= nextCode->id) && (nextCode->id <= XS_CODE_END_DERIVED))
				*address = code->nextCode;
			else
				address = &code->nextCode;
		}
		else
			address = &code->nextCode;
	}
	// end target* end => target* end
	address = &self->firstCode;
	while ((code = *address)) {
		if ((XS_CODE_END <= code->id) && (code->id <= XS_CODE_END_DERIVED)) {
			txByteCode* nextCode = code->nextCode;
			if (!nextCode)
				break;
			while (nextCode->id == XS_NO_CODE)
				nextCode = nextCode->nextCode;
			if (nextCode->id == code->id)
				*address = code->nextCode;
			else
				address = &code->nextCode;
		}
		else
			address = &code->nextCode;
	}
	// branch to next =>
	address = &self->firstCode;
	while ((code = *address)) {
		if (code->id == XS_CODE_BRANCH_1) {
			if (code->nextCode == (txByteCode*)(((txBranchCode*)code)->target))
				*address = code->nextCode;
			else
				address = &code->nextCode;
		}
		else
			address = &code->nextCode;
	}
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
			node = self->firstDeclareNode;
			while (node) {
				if (node->description->token == XS_TOKEN_PRIVATE) {
					fxCoderAddSymbol(coder, 1, XS_CODE_EVAL_PRIVATE, node->symbol);
					fxCoderAddIndex(coder, 0, XS_CODE_CONST_CLOSURE_1, node->index);
					fxCoderAddByte(coder, -1, XS_CODE_POP);
				}
				node = node->nextDeclareNode;
			}
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
					fxReportParserError(self->parser, node->line, "argument %s use closure", node->symbol->string);
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
		if (!(self->node->flags & mxStrictFlag)) {	
			fxCoderAddByte(coder, 1, XS_CODE_NULL);
			fxCoderAddByte(coder, 0, XS_CODE_WITH);
			fxCoderAddByte(coder, -1, XS_CODE_POP);
			coder->environmentLevel++;
		}
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
	fxCheckParserStack(coder->parser, self->line);
	if (self->line >= 0)
		fxCoderAddLine(coder, 0, XS_CODE_LINE, self); 
	(*self->description->dispatch->code)(it, param);
}

void fxNodeDispatchCodeAssign(void* it, void* param, txFlag flag)
{
	txNode* self = it;
	txCoder* coder = param;
	fxCheckParserStack(coder->parser, self->line);
	if (self->line >= 0)
		fxCoderAddLine(coder, 0, XS_CODE_LINE, self); 
	(*self->description->dispatch->codeAssign)(self, param, flag);
}

void fxNodeDispatchCodeDelete(void* it, void* param)
{
	txNode* self = it;
	txCoder* coder = param;
	fxCheckParserStack(coder->parser, self->line);
	if (self->line >= 0)
		fxCoderAddLine(coder, 0, XS_CODE_LINE, self); 
	(*self->description->dispatch->codeDelete)(self, param);
}

void fxNodeDispatchCodeReference(void* it, void* param)
{
	txNode* self = it;
	txCoder* coder = param;
	fxCheckParserStack(coder->parser, self->line);
	if (self->line >= 0)
		fxCoderAddLine(coder, 0, XS_CODE_LINE, self); 
	(*self->description->dispatch->codeReference)(self, param);
}

txFlag fxNodeDispatchCodeThis(void* it, void* param, txFlag flag) 
{
	txNode* self = it;
	txCoder* coder = param;
	fxCheckParserStack(coder->parser, self->line);
	if (self->line >= 0)
		fxCoderAddLine(coder, 0, XS_CODE_LINE, self); 
	return (*self->description->dispatch->codeThis)(self, param, flag);
}

void fxNodeCode(void* it, void* param) 
{
	txNode* self = it;
	txCoder* coder = param;
	fxReportParserError(coder->parser, self->line, "no value");
	fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
}

void fxNodeCodeAssign(void* it, void* param, txFlag flag) 
{
	txNode* self = it;
	txCoder* coder = param;
	fxReportParserError(coder->parser, self->line, "no reference");
}

void fxNodeCodeDelete(void* it, void* param) 
{
	fxNodeDispatchCode(it, param);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAddByte(param, 1, XS_CODE_TRUE);
}

void fxNodeCodeReference(void* it, void* param) 
{
}

txFlag fxNodeCodeName(txNode* value)
{
	txToken token = value->description->token;
	if (token == XS_TOKEN_EXPRESSIONS) {
		value = ((txExpressionsNode*)value)->items->first;
		if (value->next)
			return 0;
		token = value->description->token;
	}
	if (token == XS_TOKEN_CLASS) {
		txClassNode* node = (txClassNode*)value;
		if (node->symbol)
			return 0;
	}
	else if ((token == XS_TOKEN_FUNCTION) || (token == XS_TOKEN_GENERATOR) || (token == XS_TOKEN_HOST)) {
		txFunctionNode* node = (txFunctionNode*)value;
		if (node->symbol)
			return 0;
	}
	else
		return 0;
	return 1;
}

txFlag fxNodeCodeThis(void* it, void* param, txFlag flag) 
{
	fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
	fxNodeDispatchCode(it, param);
	return 1;
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

void fxAccessNodeCodeAssign(void* it, void* param, txFlag flag) 
{
	txAccessNode* self = it;
	txDeclareNode* declaration = self->declaration;
	if (!declaration)
		fxCoderAddSymbol(param, -1, XS_CODE_SET_VARIABLE, self->symbol);
	else
		fxCoderAddIndex(param, 0, (declaration->flags & mxDeclareNodeClosureFlag) ? XS_CODE_SET_CLOSURE_1 : XS_CODE_SET_LOCAL_1, declaration->index);
}

void fxAccessNodeCodeDelete(void* it, void* param) 
{
	txAccessNode* self = it;
	txCoder* coder = param;
	txDeclareNode* declaration = self->declaration;
	if (self->flags & mxStrictFlag)
		fxReportParserError(coder->parser, self->line, "delete identifier (strict code)");
	if (!declaration) {
		fxAccessNodeCodeReference(it, param);
		fxCoderAddSymbol(param, 0, XS_CODE_DELETE_PROPERTY, self->symbol);
	}
	else
		fxCoderAddByte(param, 1, XS_CODE_FALSE);
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

txFlag fxAccessNodeCodeThis(void* it, void* param, txFlag flag) 
{
	txAccessNode* self = it;
	txDeclareNode* declaration = self->declaration;
	if (!flag)
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
	if (!declaration) {
		fxAccessNodeCodeReference(it, param);
		if (flag)
			fxCoderAddByte(param, 1, XS_CODE_DUB);
		fxCoderAddSymbol(param, 0, XS_CODE_GET_THIS_VARIABLE, self->symbol);
	}
	else {
		fxCoderAddIndex(param, 1, (declaration->flags & mxDeclareNodeClosureFlag) ? XS_CODE_GET_CLOSURE_1 : XS_CODE_GET_LOCAL_1, declaration->index);
		flag = 0;
	}
	return flag;
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
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
					fxCoderAddByte(param, 1, XS_CODE_DUB);
					fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->nextSymbol);
					fxCoderAddByte(param, 1, XS_CODE_CALL);
					fxCoderAddInteger(param, -2, XS_CODE_RUN_1, 0);
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
					fxCoderUnuseTemporaryVariables(param, 2);
				}
				else {
					if (item->description->token != XS_TOKEN_ELISION) {
						fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, array);
						fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, counter);
						fxCoderAddByte(param, 0, XS_CODE_AT);
						fxNodeDispatchCode(item, param);
						fxCoderAddByte(param, -2, XS_CODE_SET_PROPERTY_AT);
						fxCoderAddByte(param, -1, XS_CODE_POP);
						fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, counter);
						fxCoderAddByte(param, 0, XS_CODE_INCREMENT);
						fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, counter);
					}
					else {
						fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, array);
						fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, counter);
						fxCoderAddByte(param, 0, XS_CODE_INCREMENT);
						fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, counter);
						fxCoderAddSymbol(param, -1, XS_CODE_SET_PROPERTY, coder->parser->lengthSymbol);
						fxCoderAddByte(param, -1, XS_CODE_POP);
					}
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
				fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, array);
				fxCoderAddByte(param, 1, XS_CODE_DUB);
				fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->fillSymbol);
				fxCoderAddByte(param, 1, XS_CODE_CALL);
				fxCoderAddInteger(param, -2, XS_CODE_RUN_1, 0);
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

void fxArrayBindingNodeCode(void* it, void* param)
{
	txArrayBindingNode* self = it;
	txCoder* coder = param;
	fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
	fxArrayBindingNodeCodeAssign(self, param, 0);
}

void fxArrayBindingNodeCodeAssign(void* it, void* param, txFlag flag) 
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
	
	txTargetCode* returnTarget;
	txTargetCode* stepTarget;
	txTargetCode* doneTarget;
	txTargetCode* nextTarget;
	
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
			stepTarget = fxCoderCreateTarget(param);
			
			if (item->description->token == XS_TOKEN_SKIP_BINDING) {
				fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, done);
				fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, stepTarget);
				fxCoderAddByte(param, 1, XS_CODE_TRUE);
				fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, done);
				fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
				fxCoderAddByte(param, 1, XS_CODE_DUB);
				fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->nextSymbol);
				fxCoderAddByte(param, 1, XS_CODE_CALL);
				fxCoderAddInteger(param, -2, XS_CODE_RUN_1, 0);
				fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
				fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->doneSymbol);
				fxCoderAddIndex(param, 0, XS_CODE_PULL_LOCAL_1, done);
				fxCoderAdd(param, 1, stepTarget);
			}
			else {
				doneTarget = fxCoderCreateTarget(param);
				nextTarget = fxCoderCreateTarget(param);
				fxNodeDispatchCodeReference(item, param);
				
				fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, done);
				fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, stepTarget);
				fxCoderAddByte(param, 1, XS_CODE_TRUE);
				fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, done);
				fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
				fxCoderAddByte(param, 1, XS_CODE_DUB);
				fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->nextSymbol);
				fxCoderAddByte(param, 1, XS_CODE_CALL);
				fxCoderAddInteger(param, -2, XS_CODE_RUN_1, 0);
				fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
				fxCoderAddByte(param, 1, XS_CODE_DUB);
				fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->doneSymbol);
				fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, done);
				fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, doneTarget);
				fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->valueSymbol);
				fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, nextTarget);
				fxCoderAdd(param, 1, doneTarget);
				fxCoderAddByte(param, -1, XS_CODE_POP);
				fxCoderAdd(param, 1, stepTarget);
				fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
				fxCoderAdd(param, 1, nextTarget);
				fxNodeDispatchCodeAssign(item, param, 0);
				fxCoderAddByte(param, -1, XS_CODE_POP);
			}
			item = item->next;
		}
		if (item) {
			nextTarget = fxCoderCreateTarget(param);
			doneTarget = fxCoderCreateTarget(param);
		
			fxNodeDispatchCodeReference(((txRestBindingNode*)item)->binding, param);
			fxCoderAddByte(param, 1, XS_CODE_ARRAY);
			fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, rest);
			
			fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, done);
			fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, doneTarget);

			fxCoderAdd(param, 0, nextTarget);
			fxCoderAddByte(param, 1, XS_CODE_TRUE);
			fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, done);
			fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
			fxCoderAddByte(param, 1, XS_CODE_DUB);
			fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->nextSymbol);
			fxCoderAddByte(param, 1, XS_CODE_CALL);
			fxCoderAddInteger(param, -2, XS_CODE_RUN_1, 0);
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
			fxNodeDispatchCodeAssign(((txRestBindingNode*)item)->binding, param, 0);
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
	returnTarget = fxCoderCreateTarget(param);
 	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, done);
 	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, doneTarget);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->returnSymbol);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_CHAIN_1, returnTarget);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddByte(param, 0, XS_CODE_SWAP);
	fxCoderAddByte(param, 1, XS_CODE_CALL);
	fxCoderAddInteger(param, -2, XS_CODE_RUN_1, 0);
	fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
	fxCoderAdd(param, 0, returnTarget);
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
	fxNodeDispatchCodeAssign(self->reference, param, 0);
}

void fxAwaitNodeCode(void* it, void* param)
{
	txStatementNode* self = it;
	txCoder* coder = param;
	txTargetCode* target = fxCoderCreateTarget(coder);
	fxNodeDispatchCode(self->expression, param);
	fxCoderAddByte(param, 0, XS_CODE_AWAIT);
	fxCoderAddBranch(coder, 1, XS_CODE_BRANCH_STATUS_1, target);
	fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
	fxCoderAdjustEnvironment(coder, coder->returnTarget);
	fxCoderAdjustScope(coder, coder->returnTarget);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, coder->returnTarget);
	fxCoderAdd(coder, 0, target);
}

void fxBigIntNodeCode(void* it, void* param) 
{
	txBigIntNode* self = it;
	fxCoderAddBigInt(param, 1, XS_CODE_BIGINT_1, &self->value);
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
	
	if (self->target->description->token == XS_TOKEN_ACCESS) {
		fxReportParserError(coder->parser, self->line, "invalid initializer");
		fxNodeDispatchCode(self->initializer, param);
		return;
	}
	
	fxNodeDispatchCodeReference(self->target, param);
	fxNodeDispatchCode(self->initializer, param);
	fxNodeDispatchCodeAssign(self->target, param, 0);
	fxCoderAddByte(coder, -1, XS_CODE_POP);
}

void fxBindingNodeCodeAssign(void* it, void* param, txFlag flag) 
{
	txBindingNode* self = it;
	txTargetCode* target = fxCoderCreateTarget(param);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
	fxCoderAddByte(param, -1, XS_CODE_STRICT_NOT_EQUAL);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, target);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxNodeDispatchCode(self->initializer, param);
	fxCoderAdd(param, 0, target);
	fxNodeDispatchCodeAssign(self->target, param, flag);
}

void fxBindingNodeCodeReference(void* it, void* param) 
{
	txBindingNode* self = it;
	fxNodeDispatchCodeReference(self->target, param);
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
		fxReportParserError(coder->parser, self->line, "invalid break");
	else
		fxReportParserError(coder->parser, self->line, "invalid continue");
}

void fxCallNodeCode(void* it, void* param) 
{
	txCallNewNode* self = it;
	fxNodeDispatchCodeThis(self->reference, param, 0);
	fxCoderAddByte(param, 1, XS_CODE_CALL);
	self->params->flags |= self->flags & mxTailRecursionFlag;
	fxNodeDispatchCode(self->params, param);
}

void fxCatchNodeCode(void* it, void* param) 
{
	txCatchNode* self = it;
	if (self->parameter) {
		fxScopeCodingBlock(self->scope, param);
		fxNodeDispatchCodeReference(self->parameter, param);
		fxCoderAddByte(param, 1, XS_CODE_EXCEPTION);
		fxNodeDispatchCodeAssign(self->parameter, param, 0);
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

void fxChainNodeCode(void* it, void* param)
{
	txUnaryExpressionNode* self = it;
	txCoder* coder = param;
	txTargetCode* chainTarget = coder->chainTarget;
	coder->chainTarget = fxCoderCreateTarget(param);
	fxNodeDispatchCode(self->right, param);
	fxCoderAdd(param, 0, coder->chainTarget);
	coder->chainTarget = chainTarget;
}

txFlag fxChainNodeCodeThis(void* it, void* param, txFlag flag)
{
	txUnaryExpressionNode* self = it;
	txCoder* coder = param;
	txTargetCode* chainTarget = coder->chainTarget;
	coder->chainTarget = fxCoderCreateTarget(param);
	flag = fxNodeDispatchCodeThis(self->right, param, flag);
    fxCoderAdd(param, 0, coder->chainTarget);
	coder->chainTarget = chainTarget;
	return flag;
}

void fxClassNodeCode(void* it, void* param) 
{
	txClassNode* self = it;
	txCoder* coder = param;
	txClassNode* former = coder->classNode;
	txFlag flag;
	txInteger prototype = fxCoderUseTemporaryVariable(coder);
	txInteger constructor = fxCoderUseTemporaryVariable(coder);
	txDeclareNode* declaration = self->scope->firstDeclareNode;
	txNode* item = self->items->first;
	if (self->symbol)
		fxScopeCodingBlock(self->symbolScope, param);
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
	fxScopeCodingBlock(self->scope, param);
	
	coder->classNode = self;
	fxNodeDispatchCode(self->constructor, param);
	
	fxCoderAddByte(param, 0, XS_CODE_TO_INSTANCE);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, constructor);
	fxCoderAddByte(param, -3, XS_CODE_CLASS);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, constructor);
	if (self->symbol)
		fxCoderAddSymbol(param, 0, XS_CODE_NAME, self->symbol);
		
	while (item) {
		if (item->description->token == XS_TOKEN_PROPERTY) {
			txPropertyNode* property = (txPropertyNode*)item;
			if (item->flags & (mxMethodFlag | mxGetterFlag | mxSetterFlag)) {
				if (item->flags & mxStaticFlag)
					fxCoderAddByte(param, 1, XS_CODE_DUB);
				else
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, prototype);
				fxNodeDispatchCode(property->value, param);
				fxCoderAddSymbol(param, -2, XS_CODE_NEW_PROPERTY, property->symbol);
				flag = XS_DONT_ENUM_FLAG;
				if (item->flags & mxMethodFlag)
					flag |= XS_NAME_FLAG | XS_METHOD_FLAG;
				else if (item->flags & mxGetterFlag)
					flag |= XS_NAME_FLAG | XS_METHOD_FLAG | XS_GETTER_FLAG;
				else if (item->flags & mxSetterFlag)
					flag |= XS_NAME_FLAG | XS_METHOD_FLAG | XS_SETTER_FLAG;
				fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, flag);
			}
		}
		else if (item->description->token == XS_TOKEN_PROPERTY_AT) {
			txPropertyAtNode* property = (txPropertyAtNode*)item;
			if (item->flags & (mxMethodFlag | mxGetterFlag | mxSetterFlag)) {
				if (item->flags & mxStaticFlag)
					fxCoderAddByte(param, 1, XS_CODE_DUB);
				else
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, prototype);
				fxNodeDispatchCode(property->at, param);
				fxCoderAddByte(param, 0, XS_CODE_AT);
				fxNodeDispatchCode(property->value, param);
				fxCoderAddByte(param, -3, XS_CODE_NEW_PROPERTY_AT);
				flag = XS_DONT_ENUM_FLAG;
				if (item->flags & mxMethodFlag)
					flag |= XS_NAME_FLAG | XS_METHOD_FLAG;
				else if (item->flags & mxGetterFlag)
					flag |= XS_NAME_FLAG | XS_METHOD_FLAG | XS_GETTER_FLAG;
				else if (item->flags & mxSetterFlag)
					flag |= XS_NAME_FLAG | XS_METHOD_FLAG | XS_SETTER_FLAG;
				fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, flag);
			}
			else {
				fxNodeDispatchCode(property->at, param);
				fxCoderAddByte(param, 0, XS_CODE_AT);
				fxCoderAddIndex(param, 0, XS_CODE_CONST_CLOSURE_1, declaration->index);
				fxCoderAddByte(param, -1, XS_CODE_POP);
				declaration = declaration->nextDeclareNode;
			}
		}
		else  {
			txPrivatePropertyNode* property = (txPrivatePropertyNode*)item;
			fxCoderAddIndex(param, 0, XS_CODE_CONST_CLOSURE_1, declaration->index);
			declaration = declaration->nextDeclareNode;
			if (item->flags & (mxMethodFlag | mxGetterFlag | mxSetterFlag)) {
				fxNodeDispatchCode(property->value, param);
				fxCoderAddIndex(param, 0, XS_CODE_CONST_CLOSURE_1, declaration->index);
				fxCoderAddByte(param, -1, XS_CODE_POP);
				declaration = declaration->nextDeclareNode;
			}
		}
		item = item->next;
	}
	if (self->symbol)
		fxCoderAddIndex(param, 0, XS_CODE_CONST_CLOSURE_1, self->symbolScope->firstDeclareNode->index);
	if (self->constructorInit) {
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, constructor);
		fxNodeDispatchCode(self->constructorInit, param);
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, constructor);
		fxCoderAddByte(param, -1, XS_CODE_SET_HOME);
		fxCoderAddByte(param, 1, XS_CODE_CALL);
		fxCoderAddInteger(param, -2, XS_CODE_RUN_1, 0);
		fxCoderAddByte(param, -1, XS_CODE_POP);
	}
	if (self->instanceInit) {
		fxNodeDispatchCode(self->instanceInit, param);
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, prototype);
		fxCoderAddByte(param, -1, XS_CODE_SET_HOME);
		fxCoderAddIndex(param, 0, XS_CODE_CONST_CLOSURE_1, declaration->index);
		fxCoderAddByte(param, -1, XS_CODE_POP);
	}
	coder->classNode = former;
	fxScopeCoded(self->scope, param);
	if (self->symbol)
		fxScopeCoded(self->symbolScope, param);
	fxCoderUnuseTemporaryVariables(coder, 2);
}

void fxCoalesceExpressionNodeCode(void* it, void* param) 
{
	txBinaryExpressionNode* self = it;
	txTargetCode* endTarget = fxCoderCreateTarget(param);
	self->right->flags |= (self->flags & mxTailRecursionFlag);
	fxNodeDispatchCode(self->left, param);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_COALESCE_1, endTarget);
	fxNodeDispatchCode(self->right, param);
	fxCoderAdd(param, 0, endTarget);
}

void fxCompoundExpressionNodeCode(void* it, void* param) 
{
	txAssignNode* self = it;
	txCoder* coder = param;
	txToken token = self->description->token;
	txFlag shortcut = ((token == XS_TOKEN_AND_ASSIGN) || (token == XS_TOKEN_COALESCE_ASSIGN) || (token == XS_TOKEN_OR_ASSIGN)) ? 1 : 0;
	txTargetCode* elseTarget = (shortcut) ? fxCoderCreateTarget(param) : C_NULL;
	txTargetCode* endTarget = (shortcut) ? fxCoderCreateTarget(param) : C_NULL;
	txInteger stackLevel;
	txFlag swap = fxNodeDispatchCodeThis(self->reference, param, 1);
	switch (self->description->token) {
	case XS_TOKEN_AND_ASSIGN:
		fxCoderAddByte(param, 1, XS_CODE_DUB);
		fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, elseTarget);
		fxCoderAddByte(param, -1, XS_CODE_POP);
		fxNodeDispatchCode(self->value, param);
		fxCompoundExpressionNodeCodeName(it, param);
		stackLevel = coder->stackLevel;
		break;
	case XS_TOKEN_COALESCE_ASSIGN:
		fxCoderAddBranch(param, -1, XS_CODE_BRANCH_COALESCE_1, elseTarget);
		fxNodeDispatchCode(self->value, param);
		fxCompoundExpressionNodeCodeName(it, param);
		stackLevel = coder->stackLevel;
		break;
	case XS_TOKEN_OR_ASSIGN:
		fxCoderAddByte(param, 1, XS_CODE_DUB);
		fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, elseTarget);
		fxCoderAddByte(param, -1, XS_CODE_POP);
		fxNodeDispatchCode(self->value, param);
		fxCompoundExpressionNodeCodeName(it, param);
		stackLevel = coder->stackLevel;
		break;
	default:
		fxNodeDispatchCode(self->value, param);
		fxCoderAddByte(param, -1, self->description->code);
		break;
	}
	fxNodeDispatchCodeAssign(self->reference, param, 1);
	if (shortcut) {
		fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, endTarget);
		coder->stackLevel = stackLevel;
		fxCoderAdd(param, 0, elseTarget);
		while (swap > 0) {
			if (!(self->flags & mxExpressionNoValue))
				fxCoderAddByte(param, 0, XS_CODE_SWAP);
			fxCoderAddByte(param, -1, XS_CODE_POP);
			swap--;
		}
		fxCoderAdd(param, 0, endTarget);
	}
}

void fxCompoundExpressionNodeCodeName(void* it, void* param) 
{
	txAssignNode* self = it;
	txAccessNode* reference = (txAccessNode*)(self->reference);
	txToken token = reference->description->token;
	txNode* value = self->value;
	if (token != XS_TOKEN_ACCESS)
		return;
	if (fxNodeCodeName(value))
		fxCoderAddSymbol(param, 0, XS_CODE_NAME, reference->symbol);
}

void fxDebuggerNodeCode(void* it, void* param) 
{
	fxCoderAddByte(param, 0, XS_CODE_DEBUGGER);
}

void fxDeclareNodeCode(void* it, void* param) 
{
	txDeclareNode* self = it;
	txCoder* coder = param;
	if (self->description->token == XS_TOKEN_CONST)
		fxReportParserError(coder->parser, self->line, "invalid const");
	else if (self->description->token == XS_TOKEN_LET) {
		fxNodeDispatchCodeReference(self, param);
		fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
		fxNodeDispatchCodeAssign(self, param, 0);
		fxCoderAddByte(coder, -1, XS_CODE_POP);
	}
}

void fxDeclareNodeCodeAssign(void* it, void* param, txFlag flag) 
{
	txDeclareNode* self = it;
	txDeclareNode* declaration = self->declaration;
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
	fxDeclareNodeCodeAssign(it, param, 0);
	fxCoderAddByte(coder, -1, XS_CODE_POP);
}

void fxDelegateNodeCode(void* it, void* param)
{
	txStatementNode* self = it;
	txBoolean async = (self->flags & mxAsyncFlag) ? 1 : 0;
	txCoder* coder = param;
	txInteger iterator;
	txInteger method;
	txInteger next;
	txInteger result;

	txTargetCode* nextTarget = fxCoderCreateTarget(param);
	txTargetCode* catchTarget = fxCoderCreateTarget(param);
	txTargetCode* rethrowTarget = fxCoderCreateTarget(param);
	txTargetCode* returnTarget = fxCoderCreateTarget(param);
	txTargetCode* normalTarget = fxCoderCreateTarget(param);
	txTargetCode* doneTarget = fxCoderCreateTarget(param);
	
	iterator = fxCoderUseTemporaryVariable(param);
	method = fxCoderUseTemporaryVariable(param);
	next = fxCoderUseTemporaryVariable(param);
	result = fxCoderUseTemporaryVariable(param);
	
	fxNodeDispatchCode(self->expression, param);
	if (async)
		fxCoderAddByte(param, 0, XS_CODE_FOR_AWAIT_OF);
	else
		fxCoderAddByte(param, 0, XS_CODE_FOR_OF);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->nextSymbol);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, next);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	
	fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, result);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAddBranch(param, 0, XS_CODE_CATCH_1, catchTarget);
	fxCoderAddBranch(coder, 0, XS_CODE_BRANCH_1, normalTarget);
	
// LOOP	
	fxCoderAdd(param, 0, nextTarget);
	if (async)
		fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->valueSymbol);
	fxCoderAddByte(coder, 0, XS_CODE_YIELD);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, result);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAddBranch(param, 0, XS_CODE_CATCH_1, catchTarget);
	fxCoderAddBranch(coder, 1, XS_CODE_BRANCH_STATUS_1, normalTarget);
	
// RETURN	
	fxCoderAddByte(param, 0, XS_CODE_UNCATCH);
	if (async) {
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, result);
		fxCoderAddByte(param, 0, XS_CODE_AWAIT);
		fxCoderAddByte(param, 0, XS_CODE_THROW_STATUS);
		fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, result);
		fxCoderAddByte(param, -1, XS_CODE_POP);
	}	
	
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->returnSymbol);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_CHAIN_1, returnTarget);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddByte(param, 0, XS_CODE_SWAP);
	fxCoderAddByte(param, 1, XS_CODE_CALL);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, result);
	fxCoderAddInteger(param, -3, XS_CODE_RUN_1, 1);
	if (async) {
		fxCoderAddByte(param, 0, XS_CODE_AWAIT);
		fxCoderAddByte(param, 0, XS_CODE_THROW_STATUS);
	}
	fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->doneSymbol);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, nextTarget);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->valueSymbol);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, result);
	fxCoderAdd(coder, 0, returnTarget);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, result);
	if (async) {
		fxCoderAddByte(param, 0, XS_CODE_AWAIT);
		fxCoderAddByte(param, 0, XS_CODE_THROW_STATUS);
	}	
	fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
	fxCoderAdjustEnvironment(coder, coder->returnTarget);
	fxCoderAdjustScope(coder, coder->returnTarget);
	fxCoderAddBranch(coder, 0, XS_CODE_BRANCH_1, coder->returnTarget);
	
// THROW	
	fxCoderAdd(coder, 0, catchTarget);

	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->throwSymbol);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, method);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_COALESCE_1, doneTarget);
	
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->returnSymbol);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_CHAIN_1, rethrowTarget);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddByte(param, 0, XS_CODE_SWAP);
	fxCoderAddByte(param, 1, XS_CODE_CALL);
	fxCoderAddInteger(param, -2, XS_CODE_RUN_1, 0);
	if (async) {
		fxCoderAddByte(param, 0, XS_CODE_AWAIT);
		fxCoderAddByte(param, 0, XS_CODE_THROW_STATUS);
	}
	fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
	fxCoderAdd(coder, 0, rethrowTarget);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	
	fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
	fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	
// NORMAL	
	fxCoderAdd(coder, 0, normalTarget);
	fxCoderAddByte(param, 0, XS_CODE_UNCATCH);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, next);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, method);
	fxCoderAdd(param, 1, doneTarget);
	fxCoderAddByte(param, -1, XS_CODE_POP);

	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, method);
	fxCoderAddByte(param, 1, XS_CODE_CALL);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, result);
	fxCoderAddInteger(param, -3, XS_CODE_RUN_1, 1);
	if (async) {
		fxCoderAddByte(param, 0, XS_CODE_AWAIT);
		fxCoderAddByte(param, 0, XS_CODE_THROW_STATUS);
	}
	fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->doneSymbol);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, nextTarget);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->valueSymbol);

	fxCoderUnuseTemporaryVariables(param, 4);
}

void fxDeleteNodeCode(void* it, void* param) 
{
	txDeleteNode* self = it;
	fxNodeDispatchCodeDelete(self->reference, param);
}

void fxDoNodeCode(void* it, void* param) 
{
	txDoNode* self = it;
	txCoder* coder = param;
	txTargetCode* loopTarget = fxCoderCreateTarget(param);
	if (coder->programFlag) {
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
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

txFlag fxExpressionsNodeCodeThis(void* it, void* param, txFlag flag) 
{
	txExpressionsNode* self = it;
 	if (self->items) {
 		txNode* item = self->items->first;
 		if (item->next == C_NULL) {
 			return fxNodeDispatchCodeThis(item, param, flag);
 		}
 	}
	return fxNodeCodeThis(it, param, flag);
}

void fxExpressionsNodeCodeDelete(void* it, void* param) 
 {
 	txExpressionsNode* self = it;
 	if (self->items) {
 		txNode* item = self->items->first;
 		if (item->next == C_NULL) {
			fxNodeDispatchCodeDelete(item, param);
 			return;
 		}
 	}
	fxNodeCodeDelete(it, param);
 }

void fxFieldNodeCode(void* it, void* param) 
{
	txFieldNode* self = it;
	txNode* item = self->item;
	fxCoderAddByte(param, 1, XS_CODE_THIS);
	if (item->description->token == XS_TOKEN_PROPERTY) {
		fxNodeDispatchCode(self->value, param);
		fxCoderAddSymbol(param, -2, XS_CODE_NEW_PROPERTY, ((txPropertyNode*)item)->symbol);
		fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, fxNodeCodeName(self->value) ? XS_NAME_FLAG : 0);
	}
	else if (item->description->token == XS_TOKEN_PROPERTY_AT) {
		fxCoderAddIndex(param, 1, XS_CODE_GET_CLOSURE_1, ((txPropertyAtNode*)item)->atAccess->declaration->index);
		fxNodeDispatchCode(self->value, param);
		fxCoderAddByte(param, -3, XS_CODE_NEW_PROPERTY_AT);
		fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, fxNodeCodeName(self->value) ? XS_NAME_FLAG : 0);
	}
	else {
		if (item->flags & (mxMethodFlag | mxGetterFlag | mxSetterFlag))
			fxCoderAddIndex(param, 1, XS_CODE_GET_CLOSURE_1, ((txPrivatePropertyNode*)item)->valueAccess->declaration->index);
		else
			fxNodeDispatchCode(self->value, param);
		fxCoderAddIndex(param, -2, XS_CODE_NEW_PRIVATE_1, ((txPrivatePropertyNode*)item)->symbolAccess->declaration->index);
		if (item->flags & mxMethodFlag)
			fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, XS_NAME_FLAG | XS_METHOD_FLAG);
		else if (item->flags & mxGetterFlag)
			fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, XS_NAME_FLAG | XS_METHOD_FLAG | XS_GETTER_FLAG);
		else if (item->flags & mxSetterFlag)
			fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, XS_NAME_FLAG | XS_METHOD_FLAG | XS_SETTER_FLAG);
		else
			fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, fxNodeCodeName(self->value) ? XS_NAME_FLAG : 0);
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
		fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
	}
	fxScopeCodeRefresh(self->scope, param);
	fxCoderAdd(param, 0, nextTarget);
	if (self->expression) {
		fxNodeDispatchCode(self->expression, param);
		fxCoderAddBranch(param, -1, XS_CODE_BRANCH_ELSE_1, doneTarget);
	}
	coder->firstContinueTarget->environmentLevel = coder->environmentLevel;
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
	txBoolean async = (self->description->code == XS_CODE_FOR_AWAIT_OF) ? 1 : 0;
	txInteger iterator;
	txInteger next;
	txInteger done;
	txInteger result;
	txInteger selector;
	txInteger selection;
	txTargetCode* nextTarget;
	txTargetCode* returnTarget;
	txTargetCode* doneTarget;
	txTargetCode* catchTarget;
	txTargetCode* normalTarget;
	txTargetCode* finallyTarget;
	
	iterator = fxCoderUseTemporaryVariable(param);
	next = fxCoderUseTemporaryVariable(param);
	done = fxCoderUseTemporaryVariable(param);
	result = fxCoderUseTemporaryVariable(param);
	selector = fxCoderUseTemporaryVariable(coder);
	fxScopeCodingBlock(self->scope, param);

	fxScopeCodeDefineNodes(self->scope, param);
	if (coder->programFlag) {
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
	}
	fxNodeDispatchCode(self->expression, param);
	fxCoderAddByte(param, 0, self->description->code);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->nextSymbol);
	fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, next);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	
	coder->firstBreakTarget = fxCoderAliasTargets(param, coder->firstBreakTarget);
	coder->firstContinueTarget->nextTarget = fxCoderAliasTargets(param, coder->firstContinueTarget->nextTarget);
	coder->returnTarget = fxCoderAliasTargets(param, coder->returnTarget);
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
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, next);
	fxCoderAddByte(param, 1, XS_CODE_CALL);
	fxCoderAddInteger(param, -2, XS_CODE_RUN_1, 0);
	if (async) {
		fxCoderAddByte(param, 0, XS_CODE_AWAIT);
		fxCoderAddByte(param, 0, XS_CODE_THROW_STATUS);
	}
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
	fxNodeDispatchCodeAssign(self->reference, param, 0);
	fxCoderAddByte(param, -1, XS_CODE_POP);

	coder->firstContinueTarget->environmentLevel = coder->environmentLevel;
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
	returnTarget = fxCoderCreateTarget(param);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, done);
	fxCoderAddBranch(param, -1, XS_CODE_BRANCH_IF_1, doneTarget);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->returnSymbol);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_CHAIN_1, returnTarget);
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddByte(param, 0, XS_CODE_SWAP);
	fxCoderAddByte(param, 1, XS_CODE_CALL);
	fxCoderAddInteger(param, -2, XS_CODE_RUN_1, 0);
	if (async) {
		fxCoderAddByte(param, 0, XS_CODE_AWAIT);
		fxCoderAddByte(param, 0, XS_CODE_THROW_STATUS);
	}
 	fxCoderAddByte(param, 0, XS_CODE_CHECK_INSTANCE);
	fxCoderAdd(param, 0, returnTarget);
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
	fxCoderUnuseTemporaryVariables(param, 5);
}

void fxFunctionNodeCode(void* it, void* param) 
{
	txFunctionNode* self = it;
	txCoder* coder = param;
	txInteger environmentLevel = coder->environmentLevel;
	txBoolean evalFlag = coder->evalFlag;
	txInteger line = coder->line;
	txBoolean programFlag = coder->programFlag;
	txInteger scopeLevel = coder->scopeLevel;
	txTargetCode* firstBreakTarget = coder->firstBreakTarget;
	txTargetCode* firstContinueTarget = coder->firstContinueTarget;
	txTargetCode* returnTarget = coder->returnTarget;
	txSymbol* name = self->symbol;
	txTargetCode* target = fxCoderCreateTarget(param);
	
	if ((self->flags & mxEvalFlag) && !(self->flags & mxStrictFlag))
		coder->evalFlag = 1;
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
    
	if (self->flags & mxAsyncFlag) {
		if (self->flags & mxGeneratorFlag)
			fxCoderAddSymbol(param, 1, XS_CODE_ASYNC_GENERATOR_FUNCTION, name);
		else
			fxCoderAddSymbol(param, 1, XS_CODE_ASYNC_FUNCTION, name);
	}
	else if (self->flags & mxGeneratorFlag)
		fxCoderAddSymbol(param, 1, XS_CODE_GENERATOR_FUNCTION, name);
	else if (self->flags & (mxArrowFlag | mxMethodFlag | mxGetterFlag | mxSetterFlag))
		fxCoderAddSymbol(param, 1, XS_CODE_FUNCTION, name);
	else
		fxCoderAddSymbol(param, 1, XS_CODE_CONSTRUCTOR_FUNCTION, name);
	if (coder->parser->flags & mxDebugFlag)
		fxCoderAddByte(param, 0, XS_CODE_PROFILE);
	fxCoderAddBranch(param, 0, XS_CODE_CODE_1, target);
	if (self->flags & mxFieldFlag)
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_STRICT_FIELD, fxCoderCountParameters(coder, self->params));
	else if (self->flags & mxDerivedFlag)
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_STRICT_DERIVED, fxCoderCountParameters(coder, self->params));
	else if (self->flags & mxBaseFlag)
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_STRICT_BASE, fxCoderCountParameters(coder, self->params));
	else if (self->flags & mxStrictFlag)
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_STRICT, fxCoderCountParameters(coder, self->params));
	else
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_SLOPPY, fxCoderCountParameters(coder, self->params));
	coder->path = C_NULL;
	if (self->line >= 0)
		fxCoderAddLine(coder, 0, XS_CODE_LINE, it); 
	if (self->scopeCount)
		fxCoderAddIndex(param, 0, XS_CODE_RESERVE_1, self->scopeCount);
	fxScopeCodeRetrieve(self->scope, param);
	fxScopeCodingParams(self->scope, param);
	if (self->flags & mxBaseFlag) {
		if (coder->classNode->instanceInitAccess) {
			fxCoderAddByte(param, 1, XS_CODE_THIS);
			fxCoderAddIndex(param, 1, XS_CODE_GET_CLOSURE_1, coder->classNode->instanceInitAccess->declaration->index);
			fxCoderAddByte(param, 1, XS_CODE_CALL);
			fxCoderAddInteger(param, -2, XS_CODE_RUN_1, 0);
			fxCoderAddByte(param, -1, XS_CODE_POP);
		}
	}
	if ((self->flags & mxAsyncFlag) && !(self->flags & mxGeneratorFlag))
		fxCoderAddByte(param, 0, XS_CODE_START_ASYNC);
	fxNodeDispatchCode(self->params, param);
	fxScopeCodeDefineNodes(self->scope, param);
	coder->returnTarget = fxCoderCreateTarget(param);
	if (self->flags & mxGeneratorFlag) {
		if (self->flags & mxAsyncFlag)
			fxCoderAddByte(param, 0, XS_CODE_START_ASYNC_GENERATOR);
		else
			fxCoderAddByte(param, 0, XS_CODE_START_GENERATOR);
	}
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
	
	if ((self->scope->flags & mxEvalFlag) || coder->evalFlag) {
		fxCoderAddByte(coder, 1, XS_CODE_FUNCTION_ENVIRONMENT);
		fxScopeCodeStore(self->scope, param);
		fxCoderAddByte(coder, -1, XS_CODE_POP);
	}
	else if (self->scope->closureNodeCount || (self->flags & mxArrowFlag)) {
		fxCoderAddByte(coder, 1, XS_CODE_ENVIRONMENT);
		fxScopeCodeStore(self->scope, param);
		fxCoderAddByte(coder, -1, XS_CODE_POP);
	}
	if ((self->flags & (mxArrowFlag | mxBaseFlag | mxDerivedFlag | mxGeneratorFlag | mxStrictFlag | mxMethodFlag)) == 0) {
		fxCoderAddByte(param, 1, XS_CODE_DUB);
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddSymbol(param, -2, XS_CODE_NEW_PROPERTY, coder->parser->callerSymbol);
		fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, XS_DONT_ENUM_FLAG);
	}
	
	coder->returnTarget = returnTarget;
	coder->firstContinueTarget = firstContinueTarget;
	coder->firstBreakTarget = firstBreakTarget;
	coder->scopeLevel = scopeLevel;
	coder->programFlag = programFlag;
	coder->line = line;
	coder->evalFlag = evalFlag;
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
		fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
		fxNodeDispatchCode(self->thenStatement, param);
		fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, endTarget);
		fxCoderAdd(param, 0, elseTarget);
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
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

void fxImportCallNodeCode(void* it, void* param)
{
	txCoder* coder = param;
	txStatementNode* self = it;
	fxNodeDispatchCode(self->expression, param);
	coder->importFlag = 1;
	fxCoderAddByte(param, 0, XS_CODE_IMPORT);
}

void fxImportMetaNodeCode(void* it, void* param)
{
	txCoder* coder = param;
	coder->importMetaFlag = 1;
	fxCoderAddByte(param, 1, XS_CODE_IMPORT_META);
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
		txLabelNode* former = (txLabelNode*)statement;
		txLabelNode* current = self;
		while (current) {
			if (former->symbol && current->symbol && (former->symbol == current->symbol)) {
				fxReportParserError(coder->parser, current->line, "duplicate label %s", current->symbol->string);
			}
			current = current->nextLabel;
		}
		former->nextLabel = self;
		self = former;
		statement = self->statement;
	}
	breakTarget = coder->firstBreakTarget;
	while (breakTarget) {
		txLabelNode* former = breakTarget->label;
		if (former) {
			txLabelNode* current = self;
			while (current) {
				if (former->symbol && current->symbol && (former->symbol == current->symbol)) {
					fxReportParserError(coder->parser, current->line, "duplicate label %s", current->symbol->string);
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

void fxMemberNodeCodeAssign(void* it, void* param, txFlag flag) 
{
	txMemberNode* self = it;
	fxCoderAddSymbol(param, -1, (self->reference->flags & mxSuperFlag) ? XS_CODE_SET_SUPER : XS_CODE_SET_PROPERTY, self->symbol);
}

void fxMemberNodeCodeDelete(void* it, void* param) 
{
	txMemberNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxCoderAddSymbol(param, 0, (self->reference->flags & mxSuperFlag) ? XS_CODE_DELETE_SUPER : XS_CODE_DELETE_PROPERTY, self->symbol);
}

void fxMemberNodeCodeReference(void* it, void* param) 
{
	txMemberNode* self = it;
	fxNodeDispatchCode(self->reference, param);
}

txFlag fxMemberNodeCodeThis(void* it, void* param, txFlag flag) 
{
	txMemberNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddSymbol(param, 0, (self->reference->flags & mxSuperFlag) ? XS_CODE_GET_SUPER : XS_CODE_GET_PROPERTY, self->symbol);
	return 1;
}

void fxMemberAtNodeCode(void* it, void* param) 
{
	txMemberAtNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxNodeDispatchCode(self->at, param);
	fxCoderAddByte(param, 0, XS_CODE_AT);
	fxCoderAddByte(param, -1, (self->reference->flags & mxSuperFlag) ? XS_CODE_GET_SUPER_AT : XS_CODE_GET_PROPERTY_AT);
}

void fxMemberAtNodeCodeAssign(void* it, void* param, txFlag flag) 
{
	txMemberAtNode* self = it;
	fxCoderAddByte(param, -2, (self->reference->flags & mxSuperFlag) ? XS_CODE_SET_SUPER_AT : XS_CODE_SET_PROPERTY_AT);
}

void fxMemberAtNodeCodeDelete(void* it, void* param) 
{
	txMemberAtNode* self = it;
	fxMemberAtNodeCodeReference(it, param);
	fxCoderAddByte(param, -1, (self->reference->flags & mxSuperFlag) ? XS_CODE_DELETE_SUPER_AT : XS_CODE_DELETE_PROPERTY_AT);
}

void fxMemberAtNodeCodeReference(void* it, void* param) 
{
	txMemberAtNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxNodeDispatchCode(self->at, param);
	fxCoderAddByte(param, 0, XS_CODE_AT);
}

txFlag fxMemberAtNodeCodeThis(void* it, void* param, txFlag flag) 
{
	txMemberAtNode* self = it;
	if (flag) {
		fxMemberAtNodeCodeReference(it, param);
		fxCoderAddByte(param, 2, XS_CODE_DUB_AT);
		flag = 2;
	}
	else {
		fxNodeDispatchCode(self->reference, param);
		fxCoderAddByte(param, 1, XS_CODE_DUB);
		fxNodeDispatchCode(self->at, param);
		fxCoderAddByte(param, 0, XS_CODE_AT);
	}
	fxCoderAddByte(param, -1, (self->reference->flags & mxSuperFlag) ? XS_CODE_GET_SUPER_AT : XS_CODE_GET_PROPERTY_AT);
	return flag;
}

void fxModuleNodeCode(void* it, void* param) 
{
	txModuleNode* self = it;
	txCoder* coder = param;
	txTargetCode* target = fxCoderCreateTarget(param);
	txDeclareNode* declaration;
	txInteger count;
	txSymbol* name = /*(coder->parser->flags & mxDebugFlag) ? self->path :*/ C_NULL;
	txFlag flag = 0;
	
	coder->line = -1;
	coder->programFlag = 0;
	coder->scopeLevel = 0;
	coder->firstBreakTarget = NULL;
	coder->firstContinueTarget = NULL;

	count = 0;
	declaration = self->scope->firstDeclareNode;
	while (declaration) {
		if ((declaration->description->token == XS_TOKEN_DEFINE) || (declaration->description->token == XS_TOKEN_VAR))
			count++;
		declaration = declaration->nextDeclareNode;
	}
	if (count) {
		fxCoderAddSymbol(param, 1, XS_CODE_FUNCTION, name);
		if (coder->parser->flags & mxDebugFlag)
			fxCoderAddByte(param, 0, XS_CODE_PROFILE);
		fxCoderAddBranch(param, 0, XS_CODE_CODE_1, target);
		fxCoderAddIndex(param, 0, XS_CODE_BEGIN_STRICT, 0);
		coder->path = C_NULL;
		if (self->line >= 0)
			fxCoderAddLine(coder, 0, XS_CODE_LINE, it); 
		if (self->scopeCount)
			fxCoderAddIndex(param, 0, XS_CODE_RESERVE_1, self->scopeCount);
		fxScopeCodeRetrieve(self->scope, param);
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
		fxCoderAddByte(param, 0, XS_CODE_END);
		fxCoderAdd(param, 0, target);
		fxCoderAddByte(param, 1, XS_CODE_ENVIRONMENT);
		fxCoderAddByte(param, -1, XS_CODE_POP);
	
		target = fxCoderCreateTarget(param);
		coder->line = -1;
		coder->programFlag = 0;
		coder->scopeLevel = 0;
		coder->firstBreakTarget = NULL;
		coder->firstContinueTarget = NULL;
	}
	else {
		fxCoderAddByte(coder, 1, XS_CODE_NULL);
	}
	
	if (self->flags & mxAwaitingFlag)
		fxCoderAddSymbol(param, 1, XS_CODE_ASYNC_FUNCTION, name);
	else
		fxCoderAddSymbol(param, 1, XS_CODE_FUNCTION, name);
	if (coder->parser->flags & mxDebugFlag)
		fxCoderAddByte(param, 0, XS_CODE_PROFILE);
	fxCoderAddBranch(param, 0, XS_CODE_CODE_1, target);
	fxCoderAddIndex(param, 0, XS_CODE_BEGIN_STRICT, 0);
	coder->path = C_NULL;
	if (self->line >= 0)
		fxCoderAddLine(coder, 0, XS_CODE_LINE, it); 
	if (self->scopeCount)
		fxCoderAddIndex(param, 0, XS_CODE_RESERVE_1, self->scopeCount);
	fxScopeCodeRetrieve(self->scope, param);
	
	if (self->flags & mxAwaitingFlag)
		fxCoderAddByte(param, 0, XS_CODE_START_ASYNC);

	coder->returnTarget = fxCoderCreateTarget(param);
	
	fxNodeDispatchCode(self->body, param);
	
	fxCoderAdd(param, 0, coder->returnTarget);
	fxCoderAddByte(param, 0, XS_CODE_END);
	fxCoderAdd(param, 0, target);
	
	fxCoderAddByte(param, 1, XS_CODE_ENVIRONMENT);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	
	count = 2 + fxScopeCodeSpecifierNodes(self->scope, coder);
	fxCoderAddInteger(coder, 1, XS_CODE_INTEGER_1, count);
	if (coder->importFlag)
		flag |= XS_IMPORT_FLAG;
	if (coder->importMetaFlag)
		flag |= XS_IMPORT_META_FLAG;
	fxCoderAddIndex(coder, 0 - count, XS_CODE_MODULE, flag);
	fxCoderAddByte(coder, -1, XS_CODE_SET_RESULT);
	fxCoderAddByte(coder, 0, XS_CODE_END);
}

void fxNewNodeCode(void* it, void* param) 
{
	txCallNewNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxCoderAddByte(param, 2, XS_CODE_NEW);
	fxNodeDispatchCode(self->params, param);
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
						fxReportParserError(coder->parser, item->line, "invalid __proto__");
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
			if (item->description->token == XS_TOKEN_SPREAD) {
				fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
				fxCoderAddByte(param, 1, XS_CODE_COPY_OBJECT);
				fxCoderAddByte(param, 1, XS_CODE_CALL);
				fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, object);
				fxNodeDispatchCode(((txSpreadNode*)item)->expression, param);
				fxCoderAddInteger(param, -4, XS_CODE_RUN_1, 2);
				fxCoderAddByte(param, -1, XS_CODE_POP);
			}
			else {
				txNode* value;
				if (item->description->token == XS_TOKEN_PROPERTY) {
					if (!(item->flags & mxShorthandFlag) && (((txPropertyNode*)item)->symbol == coder->parser->__proto__Symbol)) {
						item = item->next;
						continue;
					}
					value = ((txPropertyNode*)item)->value;
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, object);
					fxNodeDispatchCode(value, param);
					fxCoderAddSymbol(param, -2, XS_CODE_NEW_PROPERTY, ((txPropertyNode*)item)->symbol);
				}
				else {
					value = ((txPropertyAtNode*)item)->value;
					fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, object);
					fxNodeDispatchCode(((txPropertyAtNode*)item)->at, param);
					fxCoderAddByte(param, 0, XS_CODE_AT);
					fxNodeDispatchCode(value, param);
					fxCoderAddByte(param, -3, XS_CODE_NEW_PROPERTY_AT);
				}
				flag = 0;
				if (item->flags & mxMethodFlag)
					flag |= XS_NAME_FLAG | XS_METHOD_FLAG;
				else if (item->flags & mxGetterFlag)
					flag |= XS_NAME_FLAG | XS_METHOD_FLAG | XS_GETTER_FLAG;
				else if (item->flags & mxSetterFlag)
					flag |= XS_NAME_FLAG | XS_METHOD_FLAG | XS_SETTER_FLAG;
				else if (fxNodeCodeName(value))
					flag |= XS_NAME_FLAG;
				fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, flag);
			}
			item = item->next;
		}
	}
	fxCoderUnuseTemporaryVariables(param, 1);
}

void fxObjectBindingNodeCode(void* it, void* param)
{
	txObjectBindingNode* self = it;
	txCoder* coder = param;
	fxCoderAddByte(coder, 1, XS_CODE_UNDEFINED);
	fxObjectBindingNodeCodeAssign(self, param, 0);
}

void fxObjectBindingNodeCodeAssign(void* it, void* param, txFlag flag) 
{
	txObjectBindingNode* self = it;
	txNode* item = self->items->first;
	txInteger object;
	txInteger at;
	txInteger c = 0;
	object = fxCoderUseTemporaryVariable(param);
	at = fxCoderUseTemporaryVariable(param);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddByte(param, 0, XS_CODE_TO_INSTANCE);
	fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, object);
	if (self->flags & mxSpreadFlag) {
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, 1, XS_CODE_COPY_OBJECT);
		fxCoderAddByte(param, 1, XS_CODE_CALL);
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
			fxNodeDispatchCodeAssign(((txPropertyBindingNode*)item)->binding, param, 0);
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
			fxNodeDispatchCodeAssign(((txPropertyBindingAtNode*)item)->binding, param, 0);
		}
		fxCoderAddByte(param, -1, XS_CODE_POP);
		item = item->next;
	}
	if (self->flags & mxSpreadFlag) {
		fxCoderAddInteger(param, -2 - c, XS_CODE_RUN_1, c);
		fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, object);
		fxNodeDispatchCodeReference(((txRestBindingNode*)item)->binding, param);
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, object);
		fxNodeDispatchCodeAssign(((txRestBindingNode*)item)->binding, param, 0);
        fxCoderAddByte(param, -1, XS_CODE_POP);
	}
	fxCoderUnuseTemporaryVariables(param, 2);
}

void fxOptionNodeCode(void* it, void* param) 
{
	txUnaryExpressionNode* self = it;
	txCoder* coder = param;
	self->right->flags |= (self->flags & mxTailRecursionFlag);
	fxNodeDispatchCode(self->right, param);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_CHAIN_1, coder->chainTarget);
}

txFlag fxOptionNodeCodeThis(void* it, void* param, txFlag flag) 
{
	txUnaryExpressionNode* self = it;
	txCoder* coder = param;
	txTargetCode* swapTarget = fxCoderCreateTarget(param);
	txTargetCode* skipTarget = fxCoderCreateTarget(param);
	self->right->flags |= (self->flags & mxTailRecursionFlag);
	flag = fxNodeDispatchCodeThis(self->right, param, flag);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_CHAIN_1, swapTarget);
	fxCoderAddBranch(param, 1, XS_CODE_BRANCH_1, skipTarget);
	fxCoderAdd(param, 0, swapTarget);
	fxCoderAddByte(param, 0, XS_CODE_SWAP);
	fxCoderAddByte(param, -1, XS_CODE_POP);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, coder->chainTarget);
	fxCoderAdd(param, 0, skipTarget);
	return flag;
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

void fxParamsNodeCode(void* it, void* param) 
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
		if (self->flags & mxEvalParametersFlag)
			fxCoderAddByte(param, -3 - c, (self->flags & mxTailRecursionFlag) ? XS_CODE_EVAL_TAIL : XS_CODE_EVAL);
		else
			fxCoderAddByte(param, -3 - c, (self->flags & mxTailRecursionFlag) ? XS_CODE_RUN_TAIL : XS_CODE_RUN);
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
			if (self->flags & mxEvalParametersFlag) {
				fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, c);
				fxCoderAddByte(param, -3 - c, (self->flags & mxTailRecursionFlag) ? XS_CODE_EVAL_TAIL : XS_CODE_EVAL);
			}
			else
				fxCoderAddInteger(param, -2 - c, (self->flags & mxTailRecursionFlag) ? XS_CODE_RUN_TAIL_1 : XS_CODE_RUN_1, c);
		}
	}
}

void fxParamsBindingNodeCode(void* it, void* param) 
{
	txParamsBindingNode* self = it;
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
		if (item->description->token == XS_TOKEN_REST_BINDING) {
			fxNodeDispatchCodeReference(((txRestBindingNode*)item)->binding, param);
			fxCoderAddIndex(param, 1, XS_CODE_ARGUMENTS, index);
			fxNodeDispatchCodeAssign(((txRestBindingNode*)item)->binding, param, 0);
		}
		else {
			fxNodeDispatchCodeReference(item, param);
			fxCoderAddIndex(param, 1, XS_CODE_ARGUMENT, index);
			fxNodeDispatchCodeAssign(item, param, 0);
		}
		fxCoderAddByte(param, -1, XS_CODE_POP);
		item = item->next;
		index++;
	}
}

void fxPostfixExpressionNodeCode(void* it, void* param) 
{
	txPostfixExpressionNode* self = it;
	txInteger value;
	fxNodeDispatchCodeThis(self->left, param, 1);
	if (!(self->flags & mxExpressionNoValue)) {
		value = fxCoderUseTemporaryVariable(param);
		fxCoderAddByte(param, 0, XS_CODE_TO_NUMERIC);
		fxCoderAddIndex(param, 0, XS_CODE_SET_LOCAL_1, value);
	}
	fxCoderAddByte(param, 0, self->description->code);
	fxNodeDispatchCodeAssign(self->left, param, 1);
	if (!(self->flags & mxExpressionNoValue)) {
		fxCoderAddByte(param, -1, XS_CODE_POP);
		fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, value);
		fxCoderUnuseTemporaryVariables(param, 1);
	}
}

void fxPrivateIdentifierNodeCode(void* it, void* param) 
{
	txPrivateMemberNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxCoderAddIndex(param, 0,  XS_CODE_HAS_PRIVATE_1, self->declaration->index);
}

void fxPrivateMemberNodeCode(void* it, void* param) 
{
	txPrivateMemberNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxCoderAddIndex(param, 0,  XS_CODE_GET_PRIVATE_1, self->declaration->index);
}

void fxPrivateMemberNodeCodeAssign(void* it, void* param, txFlag flag) 
{
	txPrivateMemberNode* self = it;
	fxCoderAddIndex(param, -1, XS_CODE_SET_PRIVATE_1, self->declaration->index);
}

void fxPrivateMemberNodeCodeDelete(void* it, void* param) 
{
	txPrivateMemberNode* self = it;
	txCoder* coder = param;
	fxNodeDispatchCode(self->reference, param);
	fxReportParserError(coder->parser, self->line, "delete private property");
}

void fxPrivateMemberNodeCodeReference(void* it, void* param) 
{
	txPrivateMemberNode* self = it;
	fxNodeDispatchCode(self->reference, param);
}

txFlag fxPrivateMemberNodeCodeThis(void* it, void* param, txFlag flag) 
{
	txPrivateMemberNode* self = it;
	fxNodeDispatchCode(self->reference, param);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddIndex(param, 0, XS_CODE_GET_PRIVATE_1, self->declaration->index);
	return 1;
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
	if (self->line >= 0)
		fxCoderAddLine(coder, 0, XS_CODE_LINE, it); 
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
	fxCoderAddByte(param, 1, XS_CODE_REGEXP);
	fxCoderAddByte(param, 2, XS_CODE_NEW);
	fxNodeDispatchCode(self->modifier, param);
	fxNodeDispatchCode(self->value, param);
	fxCoderAddInteger(param, -4, XS_CODE_RUN_1, 2);
}

void fxReturnNodeCode(void* it, void* param) 
{
	txStatementNode* self = it;
	txCoder* coder = param;
	if (coder->programFlag)
		fxReportParserError(coder->parser, self->line, "invalid return");
	if (self->expression) {	
		if (((self->flags & (mxStrictFlag | mxGeneratorFlag)) == mxStrictFlag) && (coder->returnTarget->original == NULL))
			self->expression->flags |= mxTailRecursionFlag;
		fxNodeDispatchCode(self->expression, param);
		if ((self->flags & (mxAsyncFlag | mxGeneratorFlag)) == (mxAsyncFlag | mxGeneratorFlag)) {
			fxCoderAddByte(param, 0, XS_CODE_AWAIT);
			fxCoderAddByte(coder, 0, XS_CODE_THROW_STATUS);
		}
		fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
	}
	else if ((self->flags & (mxAsyncFlag | mxGeneratorFlag)) != (mxAsyncFlag | mxGeneratorFlag)) {
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
	}
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
	fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, iterator);
	fxCoderAddByte(param, 1, XS_CODE_DUB);
	fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, coder->parser->nextSymbol);
	fxCoderAddByte(param, 1, XS_CODE_CALL);
	fxCoderAddInteger(param, -2, XS_CODE_RUN_1, 0);
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
		fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
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
	txCoder* coder = param;
	txParser* parser = coder->parser;
	if (self->flags & mxStringErrorFlag)
		fxReportParserError(parser, self->line, "invalid escape sequence");
	fxCoderAddString(param, 1, XS_CODE_STRING_1, self->length, self->value);
}

void fxSuperNodeCode(void* it, void* param)
{
	txSuperNode* self = it;
	txCoder* coder = param;
	fxCoderAddByte(param, 3, XS_CODE_SUPER);
	fxNodeDispatchCode(self->params, param);
	fxCoderAddByte(param, 0, XS_CODE_SET_THIS);
	if (coder->classNode->instanceInitAccess) {
		fxCoderAddByte(param, 1, XS_CODE_GET_THIS);
		fxCoderAddIndex(param, 1, XS_CODE_GET_CLOSURE_1, coder->classNode->instanceInitAccess->declaration->index);
		fxCoderAddByte(param, 1, XS_CODE_CALL);
		fxCoderAddInteger(param, -2, XS_CODE_RUN_1, 0);
		fxCoderAddByte(param, -1, XS_CODE_POP);
	}
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
		fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
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
		txSymbol* symbol;
		txTargetCode* cacheTarget = fxCoderCreateTarget(param);
		txInteger i = (self->items->length / 2) + 1;
		txInteger raws = fxCoderUseTemporaryVariable(param);
		txInteger strings = fxCoderUseTemporaryVariable(param);
		txFlag flag = XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;

		fxNodeDispatchCodeThis(self->reference, param, 0);
		fxCoderAddByte(param, 1, XS_CODE_CALL);

		fxGenerateTag(parser->console, parser->buffer, parser->bufferSize, (parser->path) ? parser->path->string : C_NULL);
		symbol = fxNewParserSymbol(parser, parser->buffer);
		fxCoderAddByte(param, 1, XS_CODE_TEMPLATE_CACHE);
		fxCoderAddSymbol(param, 0, XS_CODE_GET_PROPERTY, symbol);
		fxCoderAddBranch(param, 0, XS_CODE_BRANCH_COALESCE_1, cacheTarget);
		fxCoderAddByte(param, 1, XS_CODE_TEMPLATE_CACHE);

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
				fxCoderAddByte(param, -3, XS_CODE_NEW_PROPERTY_AT);
				fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, flag);
			
				fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, raws);
				fxCoderAddInteger(param, 1, XS_CODE_INTEGER_1, i);
				fxCoderAddByte(param, 0, XS_CODE_AT);
				fxNodeDispatchCode(((txTemplateItemNode*)item)->raw, param);
				fxCoderAddByte(param, -3, XS_CODE_NEW_PROPERTY_AT);
				fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, flag);
			
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
		
		fxCoderAddSymbol(param, -1, XS_CODE_SET_PROPERTY, symbol);
		
		fxCoderAdd(param, 0, cacheTarget);
		
		i = 1;
		item = self->items->first;
		while (item) {
			if (item->description->token != XS_TOKEN_TEMPLATE_MIDDLE) {
				fxNodeDispatchCode(item, param);
				i++;
			}
			item = item->next;
		}
		fxCoderAddInteger(param, -2 - i, (self->flags & mxTailRecursionFlag) ? XS_CODE_RUN_TAIL_1 : XS_CODE_RUN_1, i);
		fxCoderUnuseTemporaryVariables(coder, 2);
	}
	else {
		fxNodeDispatchCode(((txTemplateItemNode*)item)->string, param);
		item = item->next;
		while (item) {
			if (item->description->token == XS_TOKEN_TEMPLATE_MIDDLE) {
				fxNodeDispatchCode(((txTemplateItemNode*)item)->string, param);
			}
			else {
				fxNodeDispatchCode(item, param);
				fxCoderAddByte(param, 1, XS_CODE_TO_STRING);
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
	txInteger result;
	txInteger selection;
	txTargetCode* catchTarget;
	txTargetCode* normalTarget;
	txTargetCode* finallyTarget;

	exception = fxCoderUseTemporaryVariable(coder);
	selector = fxCoderUseTemporaryVariable(coder);
	result = fxCoderUseTemporaryVariable(coder);

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
	if (coder->programFlag) {
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
	}
	fxNodeDispatchCode(self->tryBlock, param);
	fxCoderAddBranch(param, 0, XS_CODE_BRANCH_1, normalTarget);
	if (self->catchBlock) {
		fxCoderAddByte(param, 0, XS_CODE_UNCATCH);
		fxCoderAdd(param, 0, catchTarget);
		catchTarget = fxCoderCreateTarget(param);
		fxCoderAddBranch(param, 0, XS_CODE_CATCH_1, catchTarget);
		if (coder->programFlag) {
			fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
			fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
		}
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
		if (coder->programFlag) {
			fxCoderAddByte(param, 1, XS_CODE_GET_RESULT);
			fxCoderAddIndex(param, -1, XS_CODE_PULL_LOCAL_1, result);
			fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
			fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
		}
		fxNodeDispatchCode(self->finallyBlock, param);
		if (coder->programFlag) {
			fxCoderAddIndex(param, 1, XS_CODE_GET_LOCAL_1, result);
			fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
		}		
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
	fxCoderUnuseTemporaryVariables(coder, 3);
}

void fxUnaryExpressionNodeCode(void* it, void* param) 
{
	txUnaryExpressionNode* self = it;
	fxNodeDispatchCode(self->right, param);
	fxCoderAddByte(param, 0, self->description->code);
}

void fxUndefinedNodeCodeAssign(void* it, void* param, txFlag flag) 
{
	txCoder* coder = param;
	fxCoderAddSymbol(param, -1, XS_CODE_SET_VARIABLE, coder->parser->undefinedSymbol);
}

void fxUndefinedNodeCodeDelete(void* it, void* param) 
{
	txNode* self = it;
	txCoder* coder = param;
	if (self->flags & mxStrictFlag)
		fxReportParserError(coder->parser, self->line, "delete identifier (strict code)");
	fxCoderAddByte(param, 1, XS_CODE_FALSE);
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
		fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
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
	if (coder->programFlag) {
		fxCoderAddByte(param, 1, XS_CODE_UNDEFINED);
		fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
	}
	fxNodeDispatchCode(self->statement, param);
	coder->evalFlag = evalFlag;
	coder->environmentLevel--;
	fxCoderAddByte(param, 0, XS_CODE_WITHOUT);
}

void fxYieldNodeCode(void* it, void* param) 
{
	txStatementNode* self = it;
	txBoolean async = (self->flags & mxAsyncFlag) ? 1 : 0;
	txCoder* coder = param;
	txTargetCode* target = fxCoderCreateTarget(coder);
	
	if (!async) {
		fxCoderAddByte(param, 1, XS_CODE_OBJECT);
		fxCoderAddByte(param, 1, XS_CODE_DUB);
	}
	fxNodeDispatchCode(self->expression, param);
	if (!async) {
		fxCoderAddSymbol(param, -2, XS_CODE_NEW_PROPERTY, coder->parser->valueSymbol);
		fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, 0);
		fxCoderAddByte(param, 1, XS_CODE_DUB);
		fxCoderAddByte(param, 1, XS_CODE_FALSE);
		fxCoderAddSymbol(param, -2, XS_CODE_NEW_PROPERTY, coder->parser->doneSymbol);
		fxCoderAddInteger(param, 0, XS_CODE_INTEGER_1, 0);
	}
	fxCoderAddByte(coder, 0, XS_CODE_YIELD);
	fxCoderAddBranch(coder, 1, XS_CODE_BRANCH_STATUS_1, target);
	if (async) {
		fxCoderAddByte(param, 0, XS_CODE_AWAIT);
		fxCoderAddByte(coder, 0, XS_CODE_THROW_STATUS);
	}
	fxCoderAddByte(param, -1, XS_CODE_SET_RESULT);
	fxCoderAdjustEnvironment(coder, coder->returnTarget);
	fxCoderAdjustScope(coder, coder->returnTarget);
	fxCoderAddBranch(coder, 0, XS_CODE_BRANCH_1, coder->returnTarget);
	fxCoderAdd(coder, 0, target);
}


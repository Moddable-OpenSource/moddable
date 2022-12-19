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

//#define mxTreePrint 1

#ifdef mxTreePrint
typedef struct {
	txInteger tabs;
} txPrinter;
#endif

static void fxNodeDistribute(void* it, txNodeCall call, void* param);

static void fxCheckFunction(txParser* parser);
static void fxCheckGenerator(txParser* parser);

static void fxArrayNodeDistribute(void* it, txNodeCall call, void* param);
static void fxArrayBindingNodeDistribute(void* it, txNodeCall call, void* param);
static void fxAssignNodeDistribute(void* it, txNodeCall call, void* param);
static void fxBinaryExpressionNodeDistribute(void* it, txNodeCall call, void* param);
static void fxBindingNodeDistribute(void* it, txNodeCall call, void* param);
static void fxBlockNodeDistribute(void* it, txNodeCall call, void* param);
static void fxBodyNodeDistribute(void* it, txNodeCall call, void* param);
static void fxCallNewNodeDistribute(void* it, txNodeCall call, void* param);
static void fxCaseNodeDistribute(void* it, txNodeCall call, void* param);
static void fxCatchNodeDistribute(void* it, txNodeCall call, void* param);
static void fxClassNodeDistribute(void* it, txNodeCall call, void* param);
static void fxDeleteNodeDistribute(void* it, txNodeCall call, void* param);
static void fxDoNodeDistribute(void* it, txNodeCall call, void* param);
static void fxExportNodeDistribute(void* it, txNodeCall call, void* param);
static void fxExpressionsNodeDistribute(void* it, txNodeCall call, void* param);
static void fxFieldNodeDistribute(void* it, txNodeCall call, void* param);
static void fxFunctionNodeDistribute(void* it, txNodeCall call, void* param);
static void fxForNodeDistribute(void* it, txNodeCall call, void* param);
static void fxForInForOfNodeDistribute(void* it, txNodeCall call, void* param);
static void fxIfNodeDistribute(void* it, txNodeCall call, void* param);
static void fxImportNodeDistribute(void* it, txNodeCall call, void* param);
static void fxIncludeNodeDistribute(void* it, txNodeCall call, void* param);
static void fxLabelNodeDistribute(void* it, txNodeCall call, void* param);
static void fxMemberNodeDistribute(void* it, txNodeCall call, void* param);
static void fxMemberAtNodeDistribute(void* it, txNodeCall call, void* param);
static void fxModuleNodeDistribute(void* it, txNodeCall call, void* param);
static void fxObjectNodeDistribute(void* it, txNodeCall call, void* param);
static void fxObjectBindingNodeDistribute(void* it, txNodeCall call, void* param);
static void fxParamsNodeDistribute(void* it, txNodeCall call, void* param);
static void fxParamsBindingNodeDistribute(void* it, txNodeCall call, void* param);
static void fxPostfixExpressionNodeDistribute(void* it, txNodeCall call, void* param);
static void fxProgramNodeDistribute(void* it, txNodeCall call, void* param);
static void fxPropertyNodeDistribute(void* it, txNodeCall call, void* param);
static void fxPropertyAtNodeDistribute(void* it, txNodeCall call, void* param);
static void fxPropertyBindingNodeDistribute(void* it, txNodeCall call, void* param);
static void fxPropertyBindingAtNodeDistribute(void* it, txNodeCall call, void* param);
static void fxQuestionMarkNodeDistribute(void* it, txNodeCall call, void* param);
static void fxRegexpNodeDistribute(void* it, txNodeCall call, void* param);
static void fxRestBindingNodeDistribute(void* it, txNodeCall call, void* param);
static void fxReturnNodeDistribute(void* it, txNodeCall call, void* param);
static void fxSpreadNodeDistribute(void* it, txNodeCall call, void* param);
static void fxStatementNodeDistribute(void* it, txNodeCall call, void* param);
static void fxStatementsNodeDistribute(void* it, txNodeCall call, void* param);
static void fxSuperNodeDistribute(void* it, txNodeCall call, void* param);
static void fxSwitchNodeDistribute(void* it, txNodeCall call, void* param);
static void fxTemplateNodeDistribute(void* it, txNodeCall call, void* param);
static void fxTryNodeDistribute(void* it, txNodeCall call, void* param);
static void fxUnaryExpressionNodeDistribute(void* it, txNodeCall call, void* param);
static void fxWhileNodeDistribute(void* it, txNodeCall call, void* param);
static void fxWithNodeDistribute(void* it, txNodeCall call, void* param);

#ifdef mxTreePrint
static void fxTreePrint(txParser* parser, txNode* node);

static void fxNodePrintNode(void* it);
static void fxNodePrintTree(void* it, void* param);

static void fxAccessNodePrintNode(void* it);
static void fxDeclareDefineNodePrintNode(void* it);
static void fxExportNodePrintNode(void* it);
static void fxFunctionNodePrintNode(void* it);
static void fxImportNodePrintNode(void* it);
static void fxIntegerNodePrintNode(void* it);
static void fxLabelNodePrintNode(void* it);
static void fxMemberNodePrintNode(void* it);
static void fxNumberNodePrintNode(void* it);
static void fxPropertyNodePrintNode(void* it);
static void fxPropertyBindingNodePrintNode(void* it);
static void fxSpecifierNodePrintNode(void* it);
static void fxStringNodePrintNode(void* it);
#endif

void fxIncludeTree(txParser* parser, void* stream, txGetter getter, txUnsigned flags, txString path)
{
	txParserJump jump;
	txSymbol* symbol = parser->path;
	jump.nextJump = parser->firstJump;
	parser->firstJump = &jump;
	if (c_setjmp(jump.jmp_buf) == 0) {
		parser->path = fxNewParserSymbol(parser, path);
		fxParserTree(parser, stream, getter, flags, C_NULL);
	}
	parser->firstJump = jump.nextJump;
	parser->path = symbol;
}

void fxCheckFunction(txParser* parser)
{
	txNode* node = parser->root;
	if (node->description->token == XS_TOKEN_PROGRAM) {
		node = ((txProgramNode*)node)->body;
		if (node->description->token == XS_TOKEN_STATEMENT) {
			node = ((txStatementNode*)node)->expression;
			if (node->description->token == XS_TOKEN_EXPRESSIONS) {
				txNodeList* list = ((txExpressionsNode*)node)->items;
				if (list->length == 1) {
					node = list->first;
					if (node->description->token == XS_TOKEN_FUNCTION)
						return;
				}
			}
		}
	}
	fxReportParserError(parser, parser->line, "no function");
}

void fxCheckGenerator(txParser* parser)
{
	txNode* node = parser->root;
	if (node->description->token == XS_TOKEN_PROGRAM) {
		node = ((txProgramNode*)node)->body;
		if (node->description->token == XS_TOKEN_STATEMENT) {
			node = ((txStatementNode*)node)->expression;
			if (node->description->token == XS_TOKEN_EXPRESSIONS) {
				txNodeList* list = ((txExpressionsNode*)node)->items;
				if (list->length == 1) {
					node = list->first;
					if (node->description->token == XS_TOKEN_GENERATOR)
						return;
				}
			}
		}
	}
	fxReportParserError(parser, parser->line, "no generator function");
}

void fxParserTree(txParser* parser, void* theStream, txGetter theGetter, txUnsigned flags, txString* name)
{
	mxTryParser(parser) {
		parser->stream = theStream;
		parser->getter = theGetter;
		parser->line = 1;
		parser->flags = flags;
		parser->modifier = parser->emptyString;
		parser->string = parser->emptyString;
		parser->line2 = 1;
		parser->modifier2 = parser->emptyString;
		parser->string2 = parser->emptyString;
	
		parser->root = NULL;
	
		parser->flags &= ~(mxEvalFlag | mxFunctionFlag | mxGeneratorFlag);
		if (!(parser->flags & mxProgramFlag))
			parser->flags |= mxStrictFlag | mxAsyncFlag;
		fxGetNextCharacter(parser);
		fxGetNextCharacter(parser);
		if (parser->character == '#') {
			fxGetNextCharacter(parser);
			if (parser->character == '!') {
				fxGetNextCharacter(parser);
				while ((parser->character != (txU4)C_EOF) && (parser->character != 10) && (parser->character != 13) && (parser->character != 0x2028) && (parser->character != 0x2029)) {
					fxGetNextCharacter(parser);
				}	
			}
			else
				fxReportParserError(parser, parser->line, "invalid character %d", parser->character);
		}
		fxGetNextToken(parser);
		if (parser->flags & mxProgramFlag) {
			fxProgram(parser);
			if (flags & mxFunctionFlag)
				fxCheckFunction(parser);
			else if (flags & mxGeneratorFlag)
				fxCheckGenerator(parser);
		}
		else {
			fxModule(parser);
		}
		parser->flags &= ~mxEvalFlag;
	
		parser->flags |= flags & mxEvalFlag;
	
	#ifdef mxTreePrint
		fxTreePrint(parser, parser->root);
	#endif
		if ((parser->errorCount == 0) && name)
			*name = parser->name;
	}
	mxCatchParser(parser) {
	}
}

void fxNodeListDistribute(txNodeList* list, txNodeCall call, void* param)
{
	txNode* item = list->first;
	while (item) {
		(*call)(item, param);
		item = item->next;
	}
}

txAccessNode* fxAccessNodeNew(txParser* parser, txToken token, txSymbol* symbol)
{
	txAccessNode* node = fxNewParserChunkClear(parser, sizeof(txAccessNode));
	node->description = &gxTokenDescriptions[token];
	node->symbol = symbol;
	return node;
}

txDeclareNode* fxDeclareNodeNew(txParser* parser, txToken token, txSymbol* symbol)
{
	txDeclareNode* node = fxNewParserChunkClear(parser, sizeof(txDeclareNode));
	node->description = &gxTokenDescriptions[token];
	node->symbol = symbol;
	return node;
}

txDefineNode* fxDefineNodeNew(txParser* parser, txToken token, txSymbol* symbol)
{
	txDefineNode* node = fxNewParserChunkClear(parser, sizeof(txDefineNode));
	node->description = &gxTokenDescriptions[token];
	node->symbol = symbol;
	return node;
}

txFieldNode* fxFieldNodeNew(txParser* parser, txToken token)
{
	txFieldNode* node = fxNewParserChunkClear(parser, sizeof(txFieldNode));
	node->description = &gxTokenDescriptions[token];
	return node;
}

txSpecifierNode* fxSpecifierNodeNew(txParser* parser, txToken token)
{
	txSpecifierNode* node = fxNewParserChunkClear(parser, sizeof(txSpecifierNode));
	node->description = &gxTokenDescriptions[token];
	return node;
}

txNode* fxValueNodeNew(txParser* parser, txToken token)
{
	txNode* node = fxNewParserChunkClear(parser, sizeof(txNode));
	node->description = &gxTokenDescriptions[token];
	return node;
}

void fxNodeDistribute(void* it, txNodeCall call, void* param)
{
}

void fxArrayNodeDistribute(void* it, txNodeCall call, void* param)
{
	txArrayNode* self = it;
	fxNodeListDistribute(self->items, call, param);
}

void fxArrayBindingNodeDistribute(void* it, txNodeCall call, void* param)
{
	txArrayBindingNode* self = it;
	fxNodeListDistribute(self->items, call, param);
}

void fxAssignNodeDistribute(void* it, txNodeCall call, void* param)
{
	txAssignNode* self = it;
	(*call)(self->reference, param);
	(*call)(self->value, param);
}

void fxBinaryExpressionNodeDistribute(void* it, txNodeCall call, void* param)
{
	txBinaryExpressionNode* self = it;
	(*call)(self->left, param);
	(*call)(self->right, param);
}

void fxBindingNodeDistribute(void* it, txNodeCall call, void* param)
{
	txBindingNode* self = it;
	(*call)(self->target, param);
	(*call)(self->initializer, param);
}

void fxBlockNodeDistribute(void* it, txNodeCall call, void* param)
{
	txBlockNode* self = it;
	(*call)(self->statement, param);
}

void fxBodyNodeDistribute(void* it, txNodeCall call, void* param)
{
	txBlockNode* self = it;
	(*call)(self->statement, param);
}

void fxCallNewNodeDistribute(void* it, txNodeCall call, void* param)
{
	txCallNewNode* self = it;
	(*call)(self->reference, param);
	(*call)(self->params, param);
}

void fxCaseNodeDistribute(void* it, txNodeCall call, void* param)
{
	txCaseNode* self = it;
	if (self->expression)
		(*call)(self->expression, param);
	if (self->statement)
		(*call)(self->statement, param);
}

void fxClassNodeDistribute(void* it, txNodeCall call, void* param)
{
	txClassNode* self = it;
	if (self->heritage)
		(*call)(self->heritage, param);
	(*call)(self->constructor, param);
	fxNodeListDistribute(self->items, call, param);
	if (self->constructorInit)
		(*call)(self->constructorInit, param);
	if (self->instanceInit)
		(*call)(self->instanceInit, param);
}

void fxCatchNodeDistribute(void* it, txNodeCall call, void* param)
{
	txCatchNode* self = it;
	(*call)(self->parameter, param);
	(*call)(self->statement, param);
}

void fxDefineNodeDistribute(void* it, txNodeCall call, void* param)
{
	txDefineNode* self = it;
	(*call)(self->initializer, param);
}

void fxDeleteNodeDistribute(void* it, txNodeCall call, void* param)
{
	txDeleteNode* self = it;
	(*call)(self->reference, param);
}

void fxDoNodeDistribute(void* it, txNodeCall call, void* param)
{
	txDoNode* self = it;
	(*call)(self->statement, param);
	(*call)(self->expression, param);
}

void fxExportNodeDistribute(void* it, txNodeCall call, void* param)
{
	txExportNode* self = it;
	if (self->specifiers)
		fxNodeListDistribute(self->specifiers, call, param);
}

void fxExpressionsNodeDistribute(void* it, txNodeCall call, void* param)
{
	txExpressionsNode* self = it;
	fxNodeListDistribute(self->items, call, param);
}

void fxFieldNodeDistribute(void* it, txNodeCall call, void* param)
{
	txFieldNode* self = it;
	if (self->value)
		(*call)(self->value, param);
}

void fxForNodeDistribute(void* it, txNodeCall call, void* param)
{
	txForNode* self = it;
	if (self->initialization)
		(*call)(self->initialization, param);
	if (self->expression)
		(*call)(self->expression, param);
	if (self->iteration)
		(*call)(self->iteration, param);
	(*call)(self->statement, param);
}

void fxForInForOfNodeDistribute(void* it, txNodeCall call, void* param)
{
	txForInForOfNode* self = it;
	(*call)(self->reference, param);
	(*call)(self->expression, param);
	(*call)(self->statement, param);
}

void fxFunctionNodeDistribute(void* it, txNodeCall call, void* param)
{
	txFunctionNode* self = it;
	(*call)(self->params, param);
	(*call)(self->body, param);
}

void fxIfNodeDistribute(void* it, txNodeCall call, void* param)
{
	txIfNode* self = it;
	(*call)(self->expression, param);
	(*call)(self->thenStatement, param);
	if (self->elseStatement)
		(*call)(self->elseStatement, param);
}

void fxImportNodeDistribute(void* it, txNodeCall call, void* param)
{
	txImportNode* self = it;
	if (self->specifiers)
		fxNodeListDistribute(self->specifiers, call, param);
}

void fxIncludeNodeDistribute(void* it, txNodeCall call, void* param)
{
	txIncludeNode* self = it;
	(*call)(self->body, param);
}

txLabelNode* fxLabelNodeNew(txParser* parser)
{
	txLabelNode* node = fxNewParserChunkClear(parser, sizeof(txLabelNode));
	return node;
}

void fxLabelNodeDistribute(void* it, txNodeCall call, void* param)
{
	txLabelNode* self = it;
	(*call)(self->statement, param);
}

void fxMemberNodeDistribute(void* it, txNodeCall call, void* param)
{
	txMemberNode* self = it;
	(*call)(self->reference, param);
}

void fxMemberAtNodeDistribute(void* it, txNodeCall call, void* param)
{
	txMemberAtNode* self = it;
	(*call)(self->reference, param);
	(*call)(self->at, param);
}

void fxModuleNodeDistribute(void* it, txNodeCall call, void* param)
{
	txProgramNode* self = it;
	(*call)(self->body, param);
}

void fxObjectNodeDistribute(void* it, txNodeCall call, void* param)
{
	txObjectNode* self = it;
	fxNodeListDistribute(self->items, call, param);
}

void fxObjectBindingNodeDistribute(void* it, txNodeCall call, void* param)
{
	txObjectBindingNode* self = it;
	fxNodeListDistribute(self->items, call, param);
}

void fxParamsNodeDistribute(void* it, txNodeCall call, void* param)
{
	txParamsNode* self = it;
	fxNodeListDistribute(self->items, call, param);
}

void fxParamsBindingNodeDistribute(void* it, txNodeCall call, void* param)
{
	txParamsBindingNode* self = it;
	fxNodeListDistribute(self->items, call, param);
}

void fxPostfixExpressionNodeDistribute(void* it, txNodeCall call, void* param)
{
	txPostfixExpressionNode* self = it;
	(*call)(self->left, param);
}

void fxPrivateMemberNodeDistribute(void* it, txNodeCall call, void* param)
{
	txPrivateMemberNode* self = it;
	(*call)(self->reference, param);
}

void fxPrivatePropertyNodeDistribute(void* it, txNodeCall call, void* param)
{
	txPrivatePropertyNode* self = it;
	if (self->value)
		(*call)(self->value, param);
}

void fxProgramNodeDistribute(void* it, txNodeCall call, void* param)
{
	txProgramNode* self = it;
	(*call)(self->body, param);
}

void fxPropertyNodeDistribute(void* it, txNodeCall call, void* param)
{
	txPropertyNode* self = it;
	if (self->value)
		(*call)(self->value, param);
}

void fxPropertyAtNodeDistribute(void* it, txNodeCall call, void* param)
{
	txPropertyAtNode* self = it;
	(*call)(self->at, param);
	if (self->value)
		(*call)(self->value, param);
}

void fxPropertyBindingNodeDistribute(void* it, txNodeCall call, void* param)
{
	txPropertyBindingNode* self = it;
	(*call)(self->binding, param);
}

void fxPropertyBindingAtNodeDistribute(void* it, txNodeCall call, void* param)
{
	txPropertyBindingAtNode* self = it;
	(*call)(self->at, param);
	(*call)(self->binding, param);
}

void fxQuestionMarkNodeDistribute(void* it, txNodeCall call, void* param)
{
	txQuestionMarkNode* self = it;
	(*call)(self->expression, param);
	(*call)(self->thenExpression, param);
	(*call)(self->elseExpression, param);
}

void fxRegexpNodeDistribute(void* it, txNodeCall call, void* param)
{
	txRegexpNode* self = it;
	(*call)(self->value, param);
	(*call)(self->modifier, param);
}

void fxRestBindingNodeDistribute(void* it, txNodeCall call, void* param)
{
	txRestBindingNode* self = it;
	(*call)(self->binding, param);
}

void fxReturnNodeDistribute(void* it, txNodeCall call, void* param)
{
	txStatementNode* self = it;
	if (self->expression)
		(*call)(self->expression, param);
}

void fxSpreadNodeDistribute(void* it, txNodeCall call, void* param)
{
	txSpreadNode* self = it;
	(*call)(self->expression, param);
}

void fxStatementNodeDistribute(void* it, txNodeCall call, void* param)
{
	txStatementNode* self = it;
	(*call)(self->expression, param);
}

void fxStatementsNodeDistribute(void* it, txNodeCall call, void* param)
{
	txStatementsNode* self = it;
	fxNodeListDistribute(self->items, call, param);
}

void fxSuperNodeDistribute(void* it, txNodeCall call, void* param)
{
	txSuperNode* self = it;
	(*call)(self->params, param);
}

void fxSwitchNodeDistribute(void* it, txNodeCall call, void* param)
{
	txSwitchNode* self = it;
	(*call)(self->expression, param);
	fxNodeListDistribute(self->items, call, param);
}

void fxTemplateNodeDistribute(void* it, txNodeCall call, void* param)
{
	txTemplateNode* self = it;
	if (self->reference)
		(*call)(self->reference, param);
	fxNodeListDistribute(self->items, call, param);
}

void fxTemplateItemNodeDistribute(void* it, txNodeCall call, void* param)
{
	txTemplateItemNode* self = it;
	(*call)(self->string, param);
	(*call)(self->raw, param);
}

void fxTryNodeDistribute(void* it, txNodeCall call, void* param)
{
	txTryNode* self = it;
	(*call)(self->tryBlock, param);
	if (self->catchBlock)
		(*call)(self->catchBlock, param);
	if (self->finallyBlock)
		(*call)(self->finallyBlock, param);
}

void fxUnaryExpressionNodeDistribute(void* it, txNodeCall call, void* param)
{
	txUnaryExpressionNode* self = it;
	(*call)(self->right, param);
}

void fxWhileNodeDistribute(void* it, txNodeCall call, void* param)
{
	txWhileNode* self = it;
	(*call)(self->expression, param);
	(*call)(self->statement, param);
}

void fxWithNodeDistribute(void* it, txNodeCall call, void* param)
{
	txWithNode* self = it;
	(*call)(self->expression, param);
	(*call)(self->statement, param);
}

static const txNodeDispatch gxAccessNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxAccessNodeBind,
	fxNodeHoist,
	fxAccessNodeCode,
	fxAccessNodeCodeAssign,
	fxAccessNodeCodeDelete,
	fxAccessNodeCodeReference,
	fxAccessNodeCodeThis
};
static const txNodeDispatch gxAndExpressionNodeDispatch ICACHE_FLASH_ATTR = {
	fxBinaryExpressionNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxAndExpressionNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxArgumentsNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxArgumentsNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxArrayNodeDispatch ICACHE_FLASH_ATTR = {
	fxArrayNodeDistribute,
	fxArrayNodeBind,
	fxNodeHoist,
	fxArrayNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxArrayBindingNodeDispatch ICACHE_FLASH_ATTR = {
	fxArrayBindingNodeDistribute,
	fxArrayBindingNodeBind,
	fxNodeHoist,
	fxArrayBindingNodeCode,
	fxArrayBindingNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxAssignNodeDispatch ICACHE_FLASH_ATTR = {
	fxAssignNodeDistribute,
	fxAssignNodeBind,
	fxNodeHoist,
	fxAssignNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxAwaitNodeDispatch ICACHE_FLASH_ATTR = {
	fxStatementNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxAwaitNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxBigIntNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxBigIntNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxBinaryExpressionNodeDispatch ICACHE_FLASH_ATTR = {
	fxBinaryExpressionNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxBinaryExpressionNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxBindingNodeDispatch ICACHE_FLASH_ATTR = {
	fxBindingNodeDistribute,
	fxBindingNodeBind,
	fxNodeHoist,
	fxBindingNodeCode,
	fxBindingNodeCodeAssign,
	fxNodeCodeDelete,
	fxBindingNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxBlockNodeDispatch ICACHE_FLASH_ATTR = {
	fxBlockNodeDistribute,
	fxBlockNodeBind,
	fxBlockNodeHoist,
	fxBlockNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxBodyNodeDispatch ICACHE_FLASH_ATTR = {
	fxBodyNodeDistribute,
	fxBlockNodeBind,
	fxBodyNodeHoist,
	fxBodyNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxBreakContinueNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxBreakContinueNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxCallNodeDispatch ICACHE_FLASH_ATTR = {
	fxCallNewNodeDistribute,
	fxNodeBind,
	fxCallNodeHoist,
	fxCallNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxCaseNodeDispatch ICACHE_FLASH_ATTR = {
	fxCaseNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxCatchNodeDispatch ICACHE_FLASH_ATTR = {
	fxCatchNodeDistribute,
	fxCatchNodeBind,
	fxCatchNodeHoist,
	fxCatchNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxChainNodeDispatch ICACHE_FLASH_ATTR = {
	fxUnaryExpressionNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxChainNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxChainNodeCodeThis
};
static const txNodeDispatch gxClassNodeDispatch ICACHE_FLASH_ATTR = {
	fxClassNodeDistribute,
	fxClassNodeBind,
	fxClassNodeHoist,
	fxClassNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxCoalesceExpressionNodeDispatch ICACHE_FLASH_ATTR = {
	fxBinaryExpressionNodeDistribute,
	fxNodeBind,
	fxCoalesceExpressionNodeHoist,
	fxCoalesceExpressionNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxCompoundExpressionNodeDispatch ICACHE_FLASH_ATTR = {
	fxAssignNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxCompoundExpressionNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxDebuggerNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxDebuggerNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxDeclareNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxDeclareNodeBind,
	fxDeclareNodeHoist,
	fxDeclareNodeCode,
	fxDeclareNodeCodeAssign,
	fxNodeCodeDelete,
	fxDeclareNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxDefineNodeDispatch ICACHE_FLASH_ATTR = {
	fxDefineNodeDistribute,
	fxDefineNodeBind,
	fxDefineNodeHoist,
	fxDefineNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxDelegateNodeDispatch ICACHE_FLASH_ATTR = {
	fxStatementNodeDistribute,
	fxDelegateNodeBind,
	fxNodeHoist,
	fxDelegateNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxDeleteNodeDispatch ICACHE_FLASH_ATTR = {
	fxDeleteNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxDeleteNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxDoNodeDispatch ICACHE_FLASH_ATTR = {
	fxDoNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxDoNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxExportNodeDispatch ICACHE_FLASH_ATTR = {
	fxExportNodeDistribute,
	fxExportNodeBind,
	fxExportNodeHoist,
	fxExportNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxExpressionsNodeDispatch ICACHE_FLASH_ATTR = {
	fxExpressionsNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxExpressionsNodeCode,
	fxNodeCodeAssign,
	fxExpressionsNodeCodeDelete,
	fxNodeCodeReference,
	fxExpressionsNodeCodeThis
};
static const txNodeDispatch gxFieldNodeDispatch ICACHE_FLASH_ATTR = {
	fxFieldNodeDistribute,
	fxFieldNodeBind,
	fxNodeHoist,
	fxFieldNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxForNodeDispatch ICACHE_FLASH_ATTR = {
	fxForNodeDistribute,
	fxForNodeBind,
	fxForNodeHoist,
	fxForNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxForInForOfNodeDispatch ICACHE_FLASH_ATTR = {
	fxForInForOfNodeDistribute,
	fxForInForOfNodeBind,
	fxForInForOfNodeHoist,
	fxForInForOfNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxFunctionNodeDispatch ICACHE_FLASH_ATTR = {
	fxFunctionNodeDistribute,
	fxFunctionNodeBind,
	fxFunctionNodeHoist,
	fxFunctionNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxHostNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxHostNodeBind,
	fxHostNodeHoist,
	fxHostNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxIfNodeDispatch ICACHE_FLASH_ATTR = {
	fxIfNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxIfNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxImportNodeDispatch ICACHE_FLASH_ATTR = {
	fxImportNodeDistribute,
	fxNodeBind,
	fxImportNodeHoist,
	fxImportNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxImportCallNodeDispatch ICACHE_FLASH_ATTR = {
	fxStatementNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxImportCallNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxImportMetaNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxImportMetaNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxIncludeNodeDispatch ICACHE_FLASH_ATTR = {
	fxIncludeNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxIncludeNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxIntegerNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxIntegerNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxLabelNodeDispatch ICACHE_FLASH_ATTR = {
	fxLabelNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxLabelNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxMemberNodeDispatch ICACHE_FLASH_ATTR = {
	fxMemberNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxMemberNodeCode,
	fxMemberNodeCodeAssign,
	fxMemberNodeCodeDelete,
	fxMemberNodeCodeReference,
	fxMemberNodeCodeThis
};
static const txNodeDispatch gxMemberAtNodeDispatch ICACHE_FLASH_ATTR = {
	fxMemberAtNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxMemberAtNodeCode,
	fxMemberAtNodeCodeAssign,
	fxMemberAtNodeCodeDelete,
	fxMemberAtNodeCodeReference,
	fxMemberAtNodeCodeThis
};
static const txNodeDispatch gxModuleNodeDispatch ICACHE_FLASH_ATTR = {
	fxModuleNodeDistribute,
	fxModuleNodeBind,
	fxModuleNodeHoist,
	fxModuleNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxNewNodeDispatch ICACHE_FLASH_ATTR = {
	fxCallNewNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxNewNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxNumberNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxNumberNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxObjectNodeDispatch ICACHE_FLASH_ATTR = {
	fxObjectNodeDistribute,
	fxObjectNodeBind,
	fxNodeHoist,
	fxObjectNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxObjectBindingNodeDispatch ICACHE_FLASH_ATTR = {
	fxObjectBindingNodeDistribute,
	fxObjectBindingNodeBind,
	fxNodeHoist,
	fxObjectBindingNodeCode,
	fxObjectBindingNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxOptionNodeDispatch ICACHE_FLASH_ATTR = {
	fxUnaryExpressionNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxOptionNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxOptionNodeCodeThis
};
static const txNodeDispatch gxOrExpressionNodeDispatch ICACHE_FLASH_ATTR = {
	fxBinaryExpressionNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxOrExpressionNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxParamsNodeDispatch ICACHE_FLASH_ATTR = {
	fxParamsNodeDistribute,
	fxParamsNodeBind,
	fxNodeHoist,
	fxParamsNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxParamsBindingNodeDispatch ICACHE_FLASH_ATTR = {
	fxParamsBindingNodeDistribute,
	fxParamsBindingNodeBind,
	fxParamsBindingNodeHoist,
	fxParamsBindingNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxPostfixExpressionNodeDispatch ICACHE_FLASH_ATTR = {
	fxPostfixExpressionNodeDistribute,
	fxPostfixExpressionNodeBind,
	fxNodeHoist,
	fxPostfixExpressionNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxPrivateIdentifierNodeDispatch ICACHE_FLASH_ATTR = {
	fxPrivateMemberNodeDistribute,
	fxPrivateMemberNodeBind,
	fxNodeHoist,
	fxPrivateIdentifierNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxPrivateMemberNodeDispatch ICACHE_FLASH_ATTR = {
	fxPrivateMemberNodeDistribute,
	fxPrivateMemberNodeBind,
	fxNodeHoist,
	fxPrivateMemberNodeCode,
	fxPrivateMemberNodeCodeAssign,
	fxPrivateMemberNodeCodeDelete,
	fxPrivateMemberNodeCodeReference,
	fxPrivateMemberNodeCodeThis
};
static const txNodeDispatch gxPrivatePropertyNodeDispatch ICACHE_FLASH_ATTR = {
	fxPrivatePropertyNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxProgramNodeDispatch ICACHE_FLASH_ATTR = {
	fxProgramNodeDistribute,
	fxProgramNodeBind,
	fxProgramNodeHoist,
	fxProgramNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxPropertyNodeDispatch ICACHE_FLASH_ATTR = {
	fxPropertyNodeDistribute,
	fxNodeBind,
	fxPropertyNodeHoist,
	fxNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxPropertyAtNodeDispatch ICACHE_FLASH_ATTR = {
	fxPropertyAtNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxPropertyBindingNodeDispatch ICACHE_FLASH_ATTR = {
	fxPropertyBindingNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxPropertyBindingAtNodeDispatch ICACHE_FLASH_ATTR = {
	fxPropertyBindingAtNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxQuestionMarkNodeDispatch ICACHE_FLASH_ATTR = {
	fxQuestionMarkNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxQuestionMarkNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxRegexpNodeDispatch ICACHE_FLASH_ATTR = {
	fxRegexpNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxRegexpNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxRestBindingNodeDispatch ICACHE_FLASH_ATTR = {
	fxRestBindingNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxReturnNodeDispatch ICACHE_FLASH_ATTR = {
	fxReturnNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxReturnNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxSkipBindingNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxSpecifierNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxSpreadNodeDispatch ICACHE_FLASH_ATTR = {
	fxSpreadNodeDistribute,
	fxSpreadNodeBind,
	fxNodeHoist,
	fxNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxStatementNodeDispatch ICACHE_FLASH_ATTR = {
	fxStatementNodeDistribute,
	fxNodeBind,
	fxStatementNodeHoist,
	fxStatementNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxStatementsNodeDispatch ICACHE_FLASH_ATTR = {
	fxStatementsNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxStatementsNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxStringNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxNodeBind,
	fxStringNodeHoist,
	fxStringNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxSuperNodeDispatch ICACHE_FLASH_ATTR = {
	fxSuperNodeDistribute,
	fxSuperNodeBind,
	fxNodeHoist,
	fxSuperNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxSwitchNodeDispatch ICACHE_FLASH_ATTR = {
	fxSwitchNodeDistribute,
	fxSwitchNodeBind,
	fxSwitchNodeHoist,
	fxSwitchNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxTemplateNodeDispatch ICACHE_FLASH_ATTR = {
	fxTemplateNodeDistribute,
	fxTemplateNodeBind,
	fxNodeHoist,
	fxTemplateNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxTemplateItemNodeDispatch ICACHE_FLASH_ATTR = {
	fxTemplateItemNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxThisNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxThisNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxThrowNodeDispatch ICACHE_FLASH_ATTR = {
	fxStatementNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxThrowNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxTryNodeDispatch ICACHE_FLASH_ATTR = {
	fxTryNodeDistribute,
	fxTryNodeBind,
	fxNodeHoist,
	fxTryNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxUnaryExpressionNodeDispatch ICACHE_FLASH_ATTR = {
	fxUnaryExpressionNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxUnaryExpressionNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxUndefinedNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxValueNodeCode,
	fxUndefinedNodeCodeAssign,
	fxUndefinedNodeCodeDelete,
	fxUndefinedNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxValueNodeDispatch ICACHE_FLASH_ATTR = {
	fxNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxValueNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxWhileNodeDispatch ICACHE_FLASH_ATTR = {
	fxWhileNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxWhileNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch gxWithNodeDispatch ICACHE_FLASH_ATTR = {
	fxWithNodeDistribute,
	fxWithNodeBind,
	fxWithNodeHoist,
	fxWithNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};
static const txNodeDispatch  gxYieldNodeDispatch ICACHE_FLASH_ATTR = {
	fxStatementNodeDistribute,
	fxNodeBind,
	fxNodeHoist,
	fxYieldNodeCode,
	fxNodeCodeAssign,
	fxNodeCodeDelete,
	fxNodeCodeReference,
	fxNodeCodeThis
};

const txNodeDescription gxTokenDescriptions[XS_TOKEN_COUNT] ICACHE_FLASH_ATTR = {
	{ XS_NO_CODE, XS_NO_TOKEN, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_ACCESS, "Access", sizeof(txAccessNode), &gxAccessNodeDispatch },
	{ XS_CODE_ADD, XS_TOKEN_ADD, "Add", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_ADD, XS_TOKEN_ADD_ASSIGN, "AddAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_AND, "And", sizeof(txBinaryExpressionNode), &gxAndExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_AND_ASSIGN, "AndAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_ARG, "Arg", sizeof(txDeclareNode), &gxDeclareNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_ARGUMENTS, "Arguments", sizeof(txNode), &gxArgumentsNodeDispatch },
	{ XS_CODE_ARGUMENTS_SLOPPY, XS_TOKEN_ARGUMENTS_SLOPPY, "Arguments", sizeof(txNode), &gxValueNodeDispatch },
	{ XS_CODE_ARGUMENTS_STRICT, XS_TOKEN_ARGUMENTS_STRICT, "Arguments", sizeof(txNode), &gxValueNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_ARRAY, "Array", sizeof(txArrayNode), &gxArrayNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_ARRAY_BINDING, "ArrayBinding", sizeof(txArrayBindingNode), &gxArrayBindingNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_ARROW, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_ASSIGN, "Assign", sizeof(txAssignNode), &gxAssignNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_AWAIT, "Await", sizeof(txStatementNode), &gxAwaitNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_BIGINT, "BigInt", sizeof(txBigIntNode), &gxBigIntNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_BINDING, "Binding", sizeof(txBindingNode), &gxBindingNodeDispatch },
	{ XS_CODE_BIT_AND, XS_TOKEN_BIT_AND, "BitAnd", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_BIT_AND, XS_TOKEN_BIT_AND_ASSIGN, "BitAndAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_CODE_BIT_NOT, XS_TOKEN_BIT_NOT, "BitNot", sizeof(txUnaryExpressionNode), &gxUnaryExpressionNodeDispatch },
	{ XS_CODE_BIT_OR, XS_TOKEN_BIT_OR, "BitOr", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_BIT_OR, XS_TOKEN_BIT_OR_ASSIGN, "BitOrAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_CODE_BIT_XOR, XS_TOKEN_BIT_XOR, "BitXor", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_BIT_XOR, XS_TOKEN_BIT_XOR_ASSIGN, "BitXorAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_BLOCK, "Block", sizeof(txBlockNode), &gxBlockNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_BODY, "Body", sizeof(txBodyNode), &gxBodyNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_BREAK, "Break", sizeof(txBreakContinueNode), &gxBreakContinueNodeDispatch },
	{ XS_CODE_CALL, XS_TOKEN_CALL, "Call", sizeof(txCallNewNode), &gxCallNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_CASE, "Case", sizeof(txCaseNode), &gxCaseNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_CATCH, "Catch", sizeof(txCatchNode), &gxCatchNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_CHAIN, "Chain", sizeof(txUnaryExpressionNode), &gxChainNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_CLASS, "Class", sizeof(txClassNode), &gxClassNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_COALESCE, "Coalesce", sizeof(txBinaryExpressionNode), &gxCoalesceExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_COALESCE_ASSIGN, "CoalesceAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_COLON, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_COMMA, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_CONST, "Const", sizeof(txDeclareNode), &gxDeclareNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_CONTINUE, "Continue", sizeof(txBreakContinueNode), &gxBreakContinueNodeDispatch },
	{ XS_CODE_CURRENT, XS_TOKEN_CURRENT, "Current", sizeof(txNode), &gxValueNodeDispatch },
	{ XS_CODE_DEBUGGER, XS_TOKEN_DEBUGGER, "Debugger", sizeof(txNode), &gxDebuggerNodeDispatch },
	{ XS_CODE_DECREMENT, XS_TOKEN_DECREMENT, "Decrement", sizeof(txPostfixExpressionNode), &gxPostfixExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_DEFAULT, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_DEFINE, "Define", sizeof(txDefineNode), &gxDefineNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_DELEGATE, "Delegate", sizeof(txStatementNode), &gxDelegateNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_DELETE, "Delete", sizeof(txDeleteNode), &gxDeleteNodeDispatch },
	{ XS_CODE_DIVIDE, XS_TOKEN_DIVIDE, "Divide", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_DIVIDE, XS_TOKEN_DIVIDE_ASSIGN, "DivideAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_DO, "Do", sizeof(txDoNode), &gxDoNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_DOT, "", 0, NULL },
	{ XS_CODE_UNDEFINED, XS_TOKEN_ELISION, "Elision", sizeof(txNode), &gxValueNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_ELSE, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_ENUM, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_EOF, "", 0, NULL },
	{ XS_CODE_EQUAL, XS_TOKEN_EQUAL, "Equal", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_EVAL, XS_TOKEN_EVAL, "", 0, NULL },
	{ XS_CODE_EXPONENTIATION, XS_TOKEN_EXPONENTIATION, "Exponent", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_EXPONENTIATION, XS_TOKEN_EXPONENTIATION_ASSIGN, "ExponentAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_EXPORT, "Export", sizeof(txExportNode), &gxExportNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_EXPRESSIONS, "Expressions", sizeof(txExpressionsNode), &gxExpressionsNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_EXTENDS, "", 0, NULL },
	{ XS_CODE_FALSE, XS_TOKEN_FALSE, "False", sizeof(txNode), &gxValueNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_FIELD, "Field", sizeof(txFieldNode), &gxFieldNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_FINALLY, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_FOR, "For", sizeof(txForNode), &gxForNodeDispatch },
	{ XS_CODE_FOR_AWAIT_OF, XS_TOKEN_FOR_AWAIT_OF, "ForAwaitOf", sizeof(txForInForOfNode), &gxForInForOfNodeDispatch },
	{ XS_CODE_FOR_IN, XS_TOKEN_FOR_IN, "ForIn", sizeof(txForInForOfNode), &gxForInForOfNodeDispatch },
	{ XS_CODE_FOR_OF, XS_TOKEN_FOR_OF, "ForOf", sizeof(txForInForOfNode), &gxForInForOfNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_FUNCTION, "Function", sizeof(txFunctionNode), &gxFunctionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_GENERATOR, "Generator", sizeof(txFunctionNode), &gxFunctionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_GETTER, "Getter", sizeof(txFunctionNode), &gxFunctionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_HOST, "Host", sizeof(txHostNode), &gxHostNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_IDENTIFIER, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_IF, "If", sizeof(txIfNode), &gxIfNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_IMPLEMENTS, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_IMPORT, "Import", sizeof(txImportNode), &gxImportNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_IMPORT_CALL, "ImportCall", sizeof(txStatementNode), &gxImportCallNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_IMPORT_META, "ImportMeta", sizeof(txNode), &gxImportMetaNodeDispatch },
	{ XS_CODE_IN, XS_TOKEN_IN, "In", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_INCLUDE, "include", sizeof(txIncludeNode), &gxIncludeNodeDispatch },
	{ XS_CODE_INCREMENT, XS_TOKEN_INCREMENT, "Increment", sizeof(txPostfixExpressionNode), &gxPostfixExpressionNodeDispatch },
	{ XS_CODE_INSTANCEOF, XS_TOKEN_INSTANCEOF, "Instanceof", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_INTEGER, "Integer", sizeof(txIntegerNode), &gxIntegerNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_INTERFACE, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_ITEMS, "Items", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_LABEL, "Label", sizeof(txLabelNode), &gxLabelNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_LEFT_BRACE, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_LEFT_BRACKET, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_LEFT_PARENTHESIS, "", 0, NULL },
	{ XS_CODE_LEFT_SHIFT, XS_TOKEN_LEFT_SHIFT, "LeftShift", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_LEFT_SHIFT, XS_TOKEN_LEFT_SHIFT_ASSIGN, "LeftShiftAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_CODE_LESS, XS_TOKEN_LESS, "Less", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_LESS_EQUAL, XS_TOKEN_LESS_EQUAL, "LessEqual", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_LET, "Let", sizeof(txDeclareNode), &gxDeclareNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_MEMBER, "Member", sizeof(txMemberNode), &gxMemberNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_MEMBER_AT, "MemberAt", sizeof(txMemberAtNode), &gxMemberAtNodeDispatch },
	{ XS_CODE_MINUS, XS_TOKEN_MINUS, "Minus", sizeof(txUnaryExpressionNode), &gxUnaryExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_MODULE, "Module", sizeof(txModuleNode), &gxModuleNodeDispatch },
	{ XS_CODE_MODULO, XS_TOKEN_MODULO, "Modulo", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_MODULO, XS_TOKEN_MODULO_ASSIGN, "ModuloAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_CODE_MORE, XS_TOKEN_MORE, "More", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_MORE_EQUAL, XS_TOKEN_MORE_EQUAL, "MoreEqual", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_MULTIPLY, XS_TOKEN_MULTIPLY, "Multiply", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_MULTIPLY, XS_TOKEN_MULTIPLY_ASSIGN, "MultiplyAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_CODE_NEW, XS_TOKEN_NEW, "New", sizeof(txCallNewNode), &gxNewNodeDispatch }, 
	{ XS_CODE_NOT, XS_TOKEN_NOT, "Not", sizeof(txUnaryExpressionNode), &gxUnaryExpressionNodeDispatch },
	{ XS_CODE_NOT_EQUAL, XS_TOKEN_NOT_EQUAL, "NotEqual", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_NULL, XS_TOKEN_NULL, "Null", sizeof(txNode), &gxValueNodeDispatch }, 
	{ XS_NO_CODE, XS_TOKEN_NUMBER, "Number", sizeof(txNumberNode), &gxNumberNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_OBJECT, "Object", sizeof(txObjectNode), &gxObjectNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_OBJECT_BINDING, "ObjectBinding", sizeof(txObjectBindingNode), &gxObjectBindingNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_OPTION, "Option", sizeof(txUnaryExpressionNode), &gxOptionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_OR, "Or", sizeof(txBinaryExpressionNode), &gxOrExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_OR_ASSIGN, "OrAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_PACKAGE, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_PARAMS, "Params", sizeof(txParamsNode), &gxParamsNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_PARAMS_BINDING, "ParamsBinding", sizeof(txParamsBindingNode), &gxParamsBindingNodeDispatch },
	{ XS_CODE_PLUS, XS_TOKEN_PLUS, "Plus", sizeof(txUnaryExpressionNode), &gxUnaryExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_PRIVATE, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_PRIVATE_IDENTIFIER, "PrivateIdenfifier", sizeof(txPrivateMemberNode), &gxPrivateIdentifierNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_PRIVATE_MEMBER, "PrivateMember", sizeof(txPrivateMemberNode), &gxPrivateMemberNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_PRIVATE_PROPERTY, "PrivateProperty", sizeof(txPrivatePropertyNode), &gxPrivatePropertyNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_PROGRAM, "Program", sizeof(txProgramNode), &gxProgramNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_PROPERTY, "Property", sizeof(txPropertyNode), &gxPropertyNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_PROPERTY_AT, "PropertyAt", sizeof(txPropertyAtNode), &gxPropertyAtNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_PROPERTY_BINDING, "PropertyBinding", sizeof(txPropertyBindingNode), &gxPropertyBindingNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_PROPERTY_BINDING_AT, "PropertyBindingAt", sizeof(txPropertyBindingAtNode), &gxPropertyBindingAtNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_PROTECTED, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_PUBLIC, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_QUESTION_MARK, "QuestionMark", sizeof(txQuestionMarkNode), &gxQuestionMarkNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_REGEXP, "Regexp", sizeof(txRegexpNode), &gxRegexpNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_REST_BINDING, "RestBinding", sizeof(txRestBindingNode), &gxRestBindingNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_RETURN, "Return", sizeof(txStatementNode), &gxReturnNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_RIGHT_BRACE, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_RIGHT_BRACKET, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_RIGHT_PARENTHESIS, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_SEMICOLON, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_SETTER, "Setter", sizeof(txFunctionNode), &gxFunctionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_SHORT, "", 0, NULL },
	{ XS_CODE_SIGNED_RIGHT_SHIFT, XS_TOKEN_SIGNED_RIGHT_SHIFT, "SignedRightShift", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_SIGNED_RIGHT_SHIFT, XS_TOKEN_SIGNED_RIGHT_SHIFT_ASSIGN, "SignedRightShiftAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_SKIP_BINDING, "SkipBinding", sizeof(txSkipBindingNode), &gxSkipBindingNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_SPECIFIER, "Specifier", sizeof(txSpecifierNode), &gxSpecifierNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_SPREAD, "Spread", sizeof(txSpreadNode), &gxSpreadNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_STATEMENT, "Statement", sizeof(txStatementNode), &gxStatementNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_STATEMENTS, "Statements", sizeof(txStatementsNode), &gxStatementsNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_STATIC, "", 0, NULL },
	{ XS_CODE_STRICT_EQUAL, XS_TOKEN_STRICT_EQUAL, "StrictEqual", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_STRICT_NOT_EQUAL, XS_TOKEN_STRICT_NOT_EQUAL, "StrictNotEqual", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_STRING, "String", sizeof(txStringNode), &gxStringNodeDispatch },
	{ XS_CODE_SUBTRACT, XS_TOKEN_SUBTRACT, "Subtract", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_SUBTRACT, XS_TOKEN_SUBTRACT_ASSIGN, "SubtractAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_CODE_SUPER, XS_TOKEN_SUPER, "Super", sizeof(txSuperNode), &gxSuperNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_SWITCH, "Switch", sizeof(txSwitchNode), &gxSwitchNodeDispatch },
	{ XS_CODE_TARGET, XS_TOKEN_TARGET, "Target", sizeof(txNode), &gxValueNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_TEMPLATE, "Template", sizeof(txTemplateNode), &gxTemplateNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_TEMPLATE_HEAD, "", 0, NULL },
	{ XS_NO_CODE, XS_TOKEN_TEMPLATE_MIDDLE, "TemplateItem", sizeof(txTemplateItemNode), &gxTemplateItemNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_TEMPLATE_TAIL, "", 0, NULL },
	{ XS_CODE_THIS, XS_TOKEN_THIS, "This", sizeof(txNode), &gxThisNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_THROW, "Throw", sizeof(txStatementNode), &gxThrowNodeDispatch },
	{ XS_CODE_TRUE, XS_TOKEN_TRUE, "True", sizeof(txNode), &gxValueNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_TRY, "Try", sizeof(txTryNode), &gxTryNodeDispatch },
	{ XS_CODE_TYPEOF, XS_TOKEN_TYPEOF, "Typeof", sizeof(txUnaryExpressionNode), &gxUnaryExpressionNodeDispatch },
	{ XS_CODE_UNDEFINED, XS_TOKEN_UNDEFINED, "Undefined", sizeof(txNode), &gxUndefinedNodeDispatch },
	{ XS_CODE_UNSIGNED_RIGHT_SHIFT, XS_TOKEN_UNSIGNED_RIGHT_SHIFT, "UnsignedRightShift", sizeof(txBinaryExpressionNode), &gxBinaryExpressionNodeDispatch },
	{ XS_CODE_UNSIGNED_RIGHT_SHIFT, XS_TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN, "UnsignedRightShiftAssign", sizeof(txAssignNode), &gxCompoundExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_VAR, "Var", sizeof(txDeclareNode), &gxDeclareNodeDispatch },
	{ XS_CODE_VOID, XS_TOKEN_VOID, "Void", sizeof(txUnaryExpressionNode), &gxUnaryExpressionNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_WHILE, "While", sizeof(txWhileNode), &gxWhileNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_WITH, "With", sizeof(txWithNode), &gxWithNodeDispatch },
	{ XS_NO_CODE, XS_TOKEN_YIELD, "Yield", sizeof(txStatementNode), &gxYieldNodeDispatch },
};

#ifdef mxTreePrint

void fxTreePrint(txParser* parser, txNode* node)
{
	txPrinter printer;
	printer.tabs = 0;
	fxNodePrintTree(node, &printer);
	fprintf(stderr, "\n");
}

void fxNodePrintTree(void* it, void* param) 
{
	txNode* node = it;
	txPrinter* printer = param;
	txInteger tabs = printer->tabs;
	fprintf(stderr, "\n");
	while (tabs) {
		fprintf(stderr, "\t");
		tabs--;
	}
	if (node) {
		switch (node->description->token) {
		case XS_TOKEN_ACCESS: 
			fxAccessNodePrintNode(it); 
			break;
		case XS_TOKEN_ARG: 
		case XS_TOKEN_CONST: 
		case XS_TOKEN_DEFINE: 
		case XS_TOKEN_LET: 
		case XS_TOKEN_VAR: 
			fxDeclareDefineNodePrintNode(it); 
			break;
		case XS_TOKEN_EXPORT: 
			fxExportNodePrintNode(it); 
			break;
		case XS_TOKEN_IMPORT: 
			fxImportNodePrintNode(it); 
			break;
		case XS_TOKEN_INTEGER: 
			fxIntegerNodePrintNode(it); 
			break;
		case XS_TOKEN_FUNCTION: 
			fxFunctionNodePrintNode(it); 
			break;
		case XS_TOKEN_LABEL: 
			fxLabelNodePrintNode(it); 
			break;
		case XS_TOKEN_MEMBER: 
			fxMemberNodePrintNode(it); 
			break;
		case XS_TOKEN_NUMBER: 
			fxNumberNodePrintNode(it); 
			break;
		case XS_TOKEN_PROPERTY: 
			fxPropertyNodePrintNode(it); 
			break;
		case XS_TOKEN_PROPERTY_BINDING: 
			fxPropertyBindingNodePrintNode(it); 
			break;
		case XS_TOKEN_SPECIFIER: 
			fxSpecifierNodePrintNode(it); 
			break;
		case XS_TOKEN_STRING: 
			fxStringNodePrintNode(it); 
			break;
		default: 
			fxNodePrintNode(it); 
			break;
		}
		if (node->flags) {
			fprintf(stderr, " [");
			if (node->flags & mxArgumentsFlag)
				fprintf(stderr, " arguments");
			if (node->flags & mxArrowFlag)
				fprintf(stderr, " arrow");
			if (node->flags & mxAsyncFlag)
				fprintf(stderr, " async");
			if (node->flags & mxAwaitingFlag)
				fprintf(stderr, " await");
			if (node->flags & mxBaseFlag)
				fprintf(stderr, " base");
			if (node->flags & mxDerivedFlag)
				fprintf(stderr, " derived");
			if (node->flags & mxEvalFlag)
				fprintf(stderr, " eval");
			if (node->flags & mxFieldFlag)
				fprintf(stderr, " field");
			if (node->flags & mxTargetFlag)
				fprintf(stderr, " function");
			if (node->flags & mxGeneratorFlag)
				fprintf(stderr, " generator");
			if (node->flags & mxGetterFlag)
				fprintf(stderr, " getter");
			if (node->flags & mxMethodFlag)
				fprintf(stderr, " method");
			if (node->flags & mxSetterFlag)
				fprintf(stderr, " setter");
			if (node->flags & mxStaticFlag)
				fprintf(stderr, " static");
			if (node->flags & mxStrictFlag)
				fprintf(stderr, " strict");
			if (node->flags & mxSuperFlag)
				fprintf(stderr, " super");
			fprintf(stderr, " ]");
		}
		printer->tabs++;
		(*node->description->dispatch->distribute)(node, fxNodePrintTree, param);
		printer->tabs--;
	}
	else
		fprintf(stderr, "NULL");
}


void fxNodePrintNode(void* it) 
{
	txNode* node = it;
	fprintf(stderr, "%s", node->description->name);
}

void fxAccessNodePrintNode(void* it) 
{
	txAccessNode* node = it;
	fprintf(stderr, "%s %s", node->description->name, node->symbol->string);
}

void fxDeclareDefineNodePrintNode(void* it) 
{
	txDeclareNode* node = it;
	fprintf(stderr, "%s %s", node->description->name, node->symbol->string);
}

void fxExportNodePrintNode(void* it) 
{
	txExportNode* node = it;
	fprintf(stderr, "%s ", node->description->name);
	if (node->from)
		fprintf(stderr, "\"%s\" ", ((txStringNode*)node->from)->value);
	if (!node->specifiers)
		fprintf(stderr, "* ");
}

void fxFunctionNodePrintNode(void* it) 
{
	txFunctionNode* node = it;
    fprintf(stderr, "%s", node->description->name);
    if (node->symbol)
        fprintf(stderr, " %s", node->symbol->string);
}

void fxImportNodePrintNode(void* it) 
{
	txImportNode* node = it;
	fprintf(stderr, "%s ", node->description->name);
	if (node->from)
		fprintf(stderr, "\"%s\" ", ((txStringNode*)node->from)->value);
	else
		fprintf(stderr, "\"?\" ");
}

void fxIntegerNodePrintNode(void* it) 
{
	txIntegerNode* node = it;
	fprintf(stderr, "%s %d", node->description->name, node->value);
}

void fxLabelNodePrintNode(void* it) 
{
	txLabelNode* node = it;
    fprintf(stderr, "%s", node->description->name);
    if (node->symbol)
        fprintf(stderr, " %s", node->symbol->string);
}

void fxMemberNodePrintNode(void* it) 
{
	txMemberNode* node = it;
	fprintf(stderr, "%s %s", node->description->name, node->symbol->string);
}

void fxNumberNodePrintNode(void* it) 
{
	txNumberNode* node = it;
	fprintf(stderr, "%s %lf", node->description->name, node->value);
}

void fxPropertyNodePrintNode(void* it) 
{
	txPropertyNode* node = it;
	fprintf(stderr, "%s %s", node->description->name, node->symbol->string);
}

void fxPropertyBindingNodePrintNode(void* it) 
{
	txPropertyBindingNode* node = it;
	fprintf(stderr, "%s %s", node->description->name, node->symbol->string);
}

void fxSpecifierNodePrintNode(void* it) 
{
	txSpecifierNode* node = it;
	fprintf(stderr, "%s", node->description->name);
	if (node->symbol)
		fprintf(stderr, " %s", node->symbol->string);
	if (node->asSymbol)
		fprintf(stderr, " as %s", node->asSymbol->string);
}

void fxStringNodePrintNode(void* it) 
{
	txStringNode* node = it;
	fprintf(stderr, "%s \"%s\"", node->description->name, node->value);
}

#endif

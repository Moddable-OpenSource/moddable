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

#define mxBindHoistPart\
	txParser* parser;\
	txScope* scope

typedef struct sxExportlink txExportLink;

typedef struct {
	mxBindHoistPart;
	txInteger scopeLevel;
	txInteger scopeMaximum;
} txBinder;

typedef struct {
	mxBindHoistPart;
	txScope* functionScope;
	txScope* bodyScope;
	txExportLink* firstExportLink;
} txHoister;

struct sxExportlink {
	txExportLink* next;
	txSymbol* symbol;
};

static void fxHoisterAddExportLink(txHoister* self, txSpecifierNode* specifier);
static void fxBinderPopVariables(txBinder* self, txInteger count);
static void fxBinderPushVariables(txBinder* self, txInteger count);
static txScope* fxScopeNew(txHoister* hoister, txNode* node, txToken token);
static void fxScopeAddDeclareNode(txScope* self, txDeclareNode* node);
static void fxScopeAddDefineNode(txScope* self, txDefineNode* node);
static void fxScopeBindDefineNodes(txScope* self, void* param);
static void fxScopeBinding(txScope* self, txBinder* binder);
static void fxScopeBound(txScope* self, txBinder* binder);
static void fxScopeEval(txScope* self);
static txDeclareNode* fxScopeGetDeclareNode(txScope* self, txSymbol* symbol);
static void fxScopeHoisted(txScope* self, txHoister* hoister);
static void fxScopeLookup(txScope* self, txAccessNode* access, txBoolean closureFlag);

static void fxNodeDispatchBind(void* it, void* param);
static void fxNodeDispatchHoist(void* it, void* param);

static void fxAccessNodeBindCompound(void* it, void* param, txAssignNode* compound);
static void fxAccessNodeBindPostfix(void* it, void* param, txPostfixExpressionNode* compound);
static void fxFunctionNodeRename(void* it, txSymbol* symbol);
static void fxMemberNodeBindCompound(void* it, void* param, txAssignNode* compound);
static void fxMemberNodeBindPostfix(void* it, void* param, txPostfixExpressionNode* compound);
static void fxMemberAtNodeBindCompound(void* it, void* param, txAssignNode* compound);
static void fxMemberAtNodeBindPostfix(void* it, void* param, txPostfixExpressionNode* compound);

void fxParserBind(txParser* parser)
{
	txBinder binder;
	if (parser->errorCount == 0) {
		c_memset(&binder, 0, sizeof(txBinder));
		binder.parser = parser;
		fxNodeDispatchBind(parser->root, &binder);
	}
}

void fxParserHoist(txParser* parser)
{
	txHoister hoister;
	if (parser->errorCount == 0) {
		c_memset(&hoister, 0, sizeof(txHoister));
		hoister.parser = parser;
		fxNodeDispatchHoist(parser->root, &hoister);
	}
}

void fxHoisterAddExportLink(txHoister* self, txSpecifierNode* specifier)
{
	txExportLink* link = self->firstExportLink;
	txSymbol* symbol = specifier->asSymbol ? specifier->asSymbol : specifier->symbol;
    if (symbol) {
        while (link) {
            if (link->symbol == symbol) {
                fxReportLineError(self->parser, specifier->line, "duplicate export %s", symbol->string);
                return;
            }
            link = link->next;
        }
        link = fxNewParserChunk(self->parser, sizeof(txExportLink));
        link->next = self->firstExportLink;
        link->symbol = symbol;
        self->firstExportLink = link;
    }
}

void fxBinderPopVariables(txBinder* self, txInteger count)
{
	self->scopeLevel -= count;
}

void fxBinderPushVariables(txBinder* self, txInteger count)
{
	self->scopeLevel += count;
	if (self->scopeMaximum < self->scopeLevel)
		self->scopeMaximum = self->scopeLevel;
}

txScope* fxScopeNew(txHoister* hoister, txNode* node, txToken token) 
{
	txScope* scope = fxNewParserChunkClear(hoister->parser, sizeof(txScope));
	scope->parser = hoister->parser;
	scope->scope = hoister->scope;
	scope->token = token;
	scope->flags = node->flags & mxStrictFlag;
	scope->node = node;
	hoister->scope = scope;
	return scope;
}

void fxScopeAddDeclareNode(txScope* self, txDeclareNode* node) 
{
	self->declareNodeCount++;
	if (self->token == XS_TOKEN_EVAL) {
		if (self->lastDeclareNode)
			node->nextDeclareNode = self->firstDeclareNode;
		else
			self->lastDeclareNode = node;
		self->firstDeclareNode = node;
	}
	else {
		if (self->lastDeclareNode)
			self->lastDeclareNode->nextDeclareNode = node;
		else
			self->firstDeclareNode = node;
		self->lastDeclareNode = node;
	}
}

void fxScopeAddDefineNode(txScope* self, txDefineNode* node) 
{
	self->defineNodeCount++;
	if (self->lastDefineNode)
		self->lastDefineNode->nextDefineNode = node;
	else
		self->firstDefineNode = node;
	self->lastDefineNode = node;
}

void fxScopeBindDefineNodes(txScope* self, void* param) 
{
	txDefineNode* node = self->firstDefineNode;
	while (node) {
		fxNodeDispatchBind(node, param);
		node = node->nextDefineNode;
	}
}

void fxScopeBinding(txScope* self, txBinder* binder) 
{
	self->scope = binder->scope;
	binder->scope = self;
	fxBinderPushVariables(binder, self->declareNodeCount);
}

void fxScopeBound(txScope* self, txBinder* binder) 
{
	if (self->flags & mxEvalFlag) {
		txDeclareNode* node = self->firstDeclareNode;
		while (node) {
			node->flags |= mxDeclareNodeClosureFlag;
			node = node->nextDeclareNode;
		}
	}
	if (self->token == XS_TOKEN_MODULE) {
		txDeclareNode* node = self->firstDeclareNode;
		while (node) {
			node->flags |= mxDeclareNodeClosureFlag |  mxDeclareNodeUseClosureFlag;
			node = node->nextDeclareNode;
		}
	}
	else if (self->token == XS_TOKEN_PROGRAM) {
		txDeclareNode* node = self->firstDeclareNode;
		while (node) {
			if ((node->description->token != XS_TOKEN_VAR) && (node->description->token != XS_TOKEN_DEFINE))
				node->flags |= mxDeclareNodeClosureFlag |  mxDeclareNodeUseClosureFlag;
			node = node->nextDeclareNode;
		}
	}
	binder->scopeLevel += self->closureNodeCount;
	binder->scopeMaximum += self->closureNodeCount;
	if (self->token != XS_TOKEN_PROGRAM)
		fxBinderPopVariables(binder, self->declareNodeCount);
	binder->scope = self->scope;
}

void fxScopeEval(txScope* self) 
{
	while (self) {
		self->flags |= mxEvalFlag;
		self = self->scope;
	}
}

txDeclareNode* fxScopeGetDeclareNode(txScope* self, txSymbol* symbol) 
{
	txDeclareNode* node = self->firstDeclareNode;
	while (node) {
		if (node->symbol == symbol)
			return node;
		node = node->nextDeclareNode;
	}
	return NULL;
}

void fxScopeHoisted(txScope* self, txHoister* hoister) 
{
	if (self->token == XS_TOKEN_EVAL) {
		if (!(self->flags & mxStrictFlag)) {
			txDeclareNode* node = self->firstDeclareNode;
			while (node) {
				if (node->description->token == XS_TOKEN_VAR)
					self->declareNodeCount--;
				node = node->nextDeclareNode;
			}
		}
	}
	else if (self->token == XS_TOKEN_FUNCTION) {
	}
	else if (self->token == XS_TOKEN_MODULE) {
	}
	else if (self->token == XS_TOKEN_PROGRAM) {
		txDeclareNode* node = self->firstDeclareNode;
		while (node) {
			if ((node->description->token == XS_TOKEN_VAR) || (node->description->token == XS_TOKEN_DEFINE))
				self->declareNodeCount--;
			node = node->nextDeclareNode;
		}
	}
	else if (self->token == XS_TOKEN_WITH) {
	}
	else {
		txDeclareNode** address = &self->firstDeclareNode;
		txDeclareNode* node;
		txDeclareNode* last = C_NULL;
		while ((node = *address)) {
			if (node->description->token == XS_NO_TOKEN) {
				self->declareNodeCount--;
				*address = node->nextDeclareNode;
			}
			else {
				address = &node->nextDeclareNode;
				last = node;
			}
		}
		self->lastDeclareNode = last;
	}
	hoister->scope = self->scope;
}

void fxScopeLookup(txScope* self, txAccessNode* access, txBoolean closureFlag) 
{
	txDeclareNode* declaration;
	if (self->token == XS_TOKEN_EVAL) {
		declaration = fxScopeGetDeclareNode(self, access->symbol);
		if (declaration) {
			if (((declaration->description->token == XS_TOKEN_VAR) || (declaration->description->token == XS_TOKEN_DEFINE)) && (!(self->flags & mxStrictFlag)))
				access->declaration = C_NULL;
			else
				access->declaration = declaration;
		}
		else {
			access->declaration = C_NULL;
		}
	}
	else if (self->token == XS_TOKEN_PROGRAM) {
		declaration = fxScopeGetDeclareNode(self, access->symbol);
		if (declaration) {
			if ((declaration->description->token == XS_TOKEN_VAR) || (declaration->description->token == XS_TOKEN_DEFINE))
				access->declaration = C_NULL;
			else {
				if (closureFlag)
					declaration->flags |= mxDeclareNodeClosureFlag;
				access->declaration = declaration;
			}
		}
		else {
			access->declaration = C_NULL;
		}
	}
	else if (self->token == XS_TOKEN_WITH) {
		fxScopeLookup(self->scope, access, 1);
		if (access->declaration && (!(access->declaration->flags & mxDeclareNodeUseClosureFlag))) {
			// cache variables declared by the blocks and the function containing the with
			txDeclareNode* closureNode = fxScopeGetDeclareNode(self, access->symbol);
			if (!closureNode) {
				closureNode = fxDeclareNodeNew(self->parser, XS_NO_TOKEN, access->symbol);
				closureNode->flags |= mxDeclareNodeUseClosureFlag;
				closureNode->line = access->declaration->line;
				closureNode->declaration = access->declaration;
				fxScopeAddDeclareNode(self, closureNode);
				self->closureNodeCount++;
			}
		}
		// with can override variables 
		access->declaration = C_NULL;
	}
	else {
		declaration = fxScopeGetDeclareNode(self, access->symbol);
		if (declaration) {
			if (closureFlag)
				declaration->flags |= mxDeclareNodeClosureFlag;
			access->declaration = declaration;
		}
		else if (self->scope) {
			if (self->token == XS_TOKEN_FUNCTION) {
				if ((self->node->flags & mxEvalFlag) && !(self->node->flags & mxStrictFlag)) {
					// eval can create variables that override closures 
					access->declaration = C_NULL;
				}
				else {
					fxScopeLookup(self->scope, access, 1);
					if (access->declaration) {
						txDeclareNode* closureNode = fxDeclareNodeNew(self->parser, XS_NO_TOKEN, access->symbol);
						closureNode->flags |= mxDeclareNodeClosureFlag | mxDeclareNodeUseClosureFlag;
						closureNode->line = access->declaration->line;
						closureNode->declaration = access->declaration;
						fxScopeAddDeclareNode(self, closureNode);
						self->closureNodeCount++;
						access->declaration = closureNode;
					}
				}
			}
			else
				fxScopeLookup(self->scope, access, closureFlag);
		}
		else {
			access->declaration = C_NULL;
		}
	}
}


void fxNodeHoist(void* it, void* param) 
{
	txNode* node = it;
	(*node->description->dispatch->distribute)(node, fxNodeDispatchHoist, param);
}

void fxNodeDispatchHoist(void* it, void* param)
{
	txNode* node = it;
	(*node->description->dispatch->hoist)(it, param);
}

void fxBlockNodeHoist(void* it, void* param) 
{
	txBlockNode* self = it;
	self->scope = fxScopeNew(param, it, XS_TOKEN_BLOCK);
	fxNodeDispatchHoist(self->statement, param);
	fxScopeHoisted(self->scope, param);
}

void fxBodyNodeHoist(void* it, void* param) 
{
	txBlockNode* self = it;
	txHoister* hoister = param;
	hoister->bodyScope = self->scope = fxScopeNew(param, it, XS_TOKEN_BLOCK);
	fxNodeDispatchHoist(self->statement, param);
	fxScopeHoisted(self->scope, param);
}

void fxCallNodeHoist(void* it, void* param) 
{
	txCallNewNode* self = it;
	txHoister* hoister = param;
	txParser* parser = hoister->parser;
	if (self->reference->description->token == XS_TOKEN_ACCESS) {
		txAccessNode* access = (txAccessNode*)self->reference;
		if (access->symbol == parser->evalSymbol) {
			fxScopeEval(hoister->scope);
			hoister->functionScope->node->flags |= mxArgumentsFlag | mxEvalFlag;
			self->scope = hoister->scope;
		}
	}
	fxNodeDispatchHoist(self->reference, param);
	fxNodeDispatchHoist(self->params, param);
}

void fxCatchNodeHoist(void* it, void* param) 
{
	txCatchNode* self = it;
	txHoister* hoister = param;
	txDeclareNode* node;
	if (self->parameter) {
		self->scope = fxScopeNew(param, it, XS_TOKEN_BLOCK);
		fxNodeDispatchHoist(self->parameter, param);
		self->statementScope = fxScopeNew(param, it, XS_TOKEN_BLOCK);
		fxNodeDispatchHoist(self->statement, param);
		node = self->statementScope->firstDeclareNode;
		while (node) {
			if (fxScopeGetDeclareNode(self->scope, node->symbol))
				fxReportLineError(hoister->parser, node->line, "duplicate variable %s", node->symbol->string);
			node = node->nextDeclareNode;
		}
		fxScopeHoisted(self->statementScope, param);
		fxScopeHoisted(self->scope, param);
	}
	else {
		self->statementScope = fxScopeNew(param, it, XS_TOKEN_BLOCK);
		fxNodeDispatchHoist(self->statement, param);
		fxScopeHoisted(self->statementScope, param);
	}
}

void fxClassNodeHoist(void* it, void* param) 
{
	txClassNode* self = it;
	txHoister* hoister = param;
	txSymbol* symbol = self->symbol;
	if (symbol) {
		txDeclareNode* node = fxDeclareNodeNew(hoister->parser, XS_TOKEN_CONST, symbol);
		node->flags |= mxDeclareNodeClosureFlag;
		self->scope = fxScopeNew(hoister, it, XS_TOKEN_BLOCK);
		fxScopeAddDeclareNode(self->scope, node);
	}	
	if (self->heritage)
		fxNodeDispatchHoist(self->heritage, param);
	fxNodeDispatchHoist(self->constructor, param);
	fxNodeListDistribute(self->items, fxNodeDispatchHoist, param);
	if (symbol)
		fxScopeHoisted(self->scope, param);
}

void fxDeclareNodeHoist(void* it, void* param) 
{
	txDeclareNode* self = it;
	txHoister* hoister = param;
	txDeclareNode* node;
	txScope* scope;
	if (self->description->token == XS_TOKEN_ARG) {
		self->flags |= mxDeclareNodeInitializedFlag;
		node = fxScopeGetDeclareNode(hoister->functionScope, self->symbol);
		if (node) {
			if ((node->description->token == XS_TOKEN_ARG) && (hoister->functionScope->node->flags & (mxArrowFlag | mxAsyncFlag | mxNotSimpleParametersFlag | mxStrictFlag)))
				fxReportLineError(hoister->parser, self->line, "duplicate argument %s", self->symbol->string);
		}
		else {
			fxScopeAddDeclareNode(hoister->functionScope, self);
		}
	}
	else if ((self->description->token == XS_TOKEN_CONST) || (self->description->token == XS_TOKEN_LET)) {
		node = fxScopeGetDeclareNode(hoister->scope, self->symbol);
		if (!node && (hoister->scope == hoister->bodyScope)) {
			node = fxScopeGetDeclareNode(hoister->functionScope, self->symbol);
			if (node && (node->description->token != XS_TOKEN_ARG))
				node = C_NULL;
		}
		if (node)
			fxReportLineError(hoister->parser, self->line, "duplicate variable %s", self->symbol->string);
		else
			fxScopeAddDeclareNode(hoister->scope, self);
	}
	else {
		self->flags |= mxDeclareNodeInitializedFlag;
		scope = hoister->scope;
		node = C_NULL;
		while (scope != hoister->bodyScope) {
			node = fxScopeGetDeclareNode(scope, self->symbol);
			if (node) {
				if ((node->description->token == XS_TOKEN_CONST) || (node->description->token == XS_TOKEN_LET) || (node->description->token == XS_TOKEN_DEFINE))
					break;
				node = C_NULL;
			}
			scope = scope->scope;
		}
		if (!node) {
			node = fxScopeGetDeclareNode(scope, self->symbol);
			if (node) {
				if ((node->description->token != XS_TOKEN_CONST) && (node->description->token != XS_TOKEN_LET))
					node = C_NULL;
			}
		}
		if (node)
			fxReportLineError(hoister->parser, self->line, "duplicate variable %s", self->symbol->string);
		else {
			node = fxScopeGetDeclareNode(hoister->functionScope, self->symbol);
			if (!node || ((node->description->token != XS_TOKEN_ARG) && (node->description->token != XS_TOKEN_VAR)))
				fxScopeAddDeclareNode(hoister->bodyScope, self);
			if (hoister->scope != hoister->bodyScope)
				fxScopeAddDeclareNode(hoister->scope, fxDeclareNodeNew(hoister->parser, XS_NO_TOKEN, self->symbol));
		}
	}
	if (self->initializer)
		fxNodeDispatchHoist(self->initializer, param);
}

void fxDefineNodeHoist(void* it, void* param) 
{
	txDefineNode* self = it;
	txHoister* hoister = param;
	txDeclareNode* node;
	if (self->flags & mxStrictFlag) {
		if ((self->symbol == hoister->parser->argumentsSymbol) || (self->symbol == hoister->parser->evalSymbol) || (self->symbol == hoister->parser->yieldSymbol))
			fxReportLineError(hoister->parser, self->line, "invalid definition %s", self->symbol->string);
	}
	if ((hoister->scope == hoister->bodyScope) && (hoister->scope->token != XS_TOKEN_MODULE)) {
		node = fxScopeGetDeclareNode(hoister->bodyScope, self->symbol);
		if (node) {
			if ((node->description->token == XS_TOKEN_CONST) || (node->description->token == XS_TOKEN_LET))
				fxReportLineError(hoister->parser, self->line, "duplicate variable %s", self->symbol->string);
		}
		else {
			if (hoister->functionScope != hoister->bodyScope)
				node = fxScopeGetDeclareNode(hoister->functionScope, self->symbol);
			if (!node)
				fxScopeAddDeclareNode(hoister->bodyScope, (txDeclareNode*)self);
		}
		fxScopeAddDefineNode(hoister->bodyScope, self);
	}
	else {
		node = fxScopeGetDeclareNode(hoister->scope, self->symbol);
		if (node)
			fxReportLineError(hoister->parser, self->line, "duplicate variable %s", self->symbol->string);
		else
			fxScopeAddDeclareNode(hoister->scope, (txDeclareNode*)self);
		fxScopeAddDefineNode(hoister->scope, self);
	}
	self->flags |= mxDeclareNodeInitializedFlag;
	((txFunctionNode*)(self->initializer))->symbol = C_NULL;
	fxNodeDispatchHoist(self->initializer, param);
	((txFunctionNode*)(self->initializer))->symbol = self->symbol;
}

void fxExportNodeHoist(void* it, void* param)
{
	txExportNode* self = it;
	txHoister* hoister = param;
	if (self->from) {
		if (self->specifiers && self->specifiers->length) {
			txSpecifierNode* specifier = (txSpecifierNode*)self->specifiers->first;
			while (specifier) {
				txDeclareNode* node = fxDeclareNodeNew(hoister->parser, XS_TOKEN_LET, C_NULL);
				specifier->from = self->from;
				node->flags |= mxDeclareNodeClosureFlag | mxDeclareNodeUseClosureFlag;
				node->line = self->line;
				node->importSpecifier = specifier;
				node->firstExportSpecifier = specifier;
				fxScopeAddDeclareNode(hoister->scope, node);
				specifier = (txSpecifierNode*)specifier->next;
			}
		}
		else {
			txSpecifierNode* specifier = fxSpecifierNodeNew(hoister->parser, XS_TOKEN_SPECIFIER);
			txDeclareNode* node = fxDeclareNodeNew(hoister->parser, XS_TOKEN_LET, C_NULL);
			specifier->from = self->from;
			node->flags |= mxDeclareNodeClosureFlag | mxDeclareNodeUseClosureFlag;
			node->line = self->line;
			node->importSpecifier = specifier;
			fxScopeAddDeclareNode(hoister->scope, node);
		}
		return;
	}
	if (self->specifiers && self->specifiers->length) {
		txSpecifierNode* specifier = (txSpecifierNode*)self->specifiers->first;
		while (specifier) {
			fxHoisterAddExportLink(hoister, specifier);
			fxNodeDispatchHoist(specifier, param);
			specifier = (txSpecifierNode*)specifier->next;
		}
	}
}

void fxForNodeHoist(void* it, void* param) 
{
	txForNode* self = it;
	self->scope = fxScopeNew(param, it, XS_TOKEN_BLOCK);
	if (self->initialization)
		fxNodeDispatchHoist(self->initialization, param);
	if (self->expression)
		fxNodeDispatchHoist(self->expression, param);
	if (self->iteration)
		fxNodeDispatchHoist(self->iteration, param);
	fxNodeDispatchHoist(self->statement, param);
	fxScopeHoisted(self->scope, param);
}

void fxForInForOfNodeHoist(void* it, void* param) 
{
	txForInForOfNode* self = it;
	self->scope = fxScopeNew(param, it, XS_TOKEN_BLOCK);
	fxNodeDispatchHoist(self->reference, param);
	fxNodeDispatchHoist(self->expression, param);
	fxNodeDispatchHoist(self->statement, param);
	fxScopeHoisted(self->scope, param);
}

void fxFunctionNodeHoist(void* it, void* param) 
{
	txFunctionNode* self = it;
	txHoister* hoister = param;
	txScope* functionScope = hoister->functionScope;
	txScope* bodyScope = hoister->bodyScope;
	hoister->functionScope = self->scope = fxScopeNew(param, it, XS_TOKEN_FUNCTION);
	hoister->bodyScope = C_NULL;
	if (self->symbol) {
		txDefineNode* node = fxDefineNodeNew(hoister->parser, XS_TOKEN_CONST, self->symbol);
		node->initializer = fxValueNodeNew(hoister->parser, XS_TOKEN_CURRENT);
		fxScopeAddDeclareNode(hoister->functionScope, (txDeclareNode*)node);
		fxScopeAddDefineNode(hoister->functionScope, node);
	}
	fxNodeDispatchHoist(self->params, param);
	if (self->flags & mxArgumentsFlag) {
		txDeclareNode* declaration = fxDeclareNodeNew(hoister->parser, XS_TOKEN_VAR, hoister->parser->argumentsSymbol);
		declaration->flags |= mxDeclareNodeBoundFlag | mxDeclareNodeInitializedFlag;
		fxScopeAddDeclareNode(hoister->functionScope, declaration);
	}
	fxNodeDispatchHoist(self->body, param);
	fxScopeHoisted(self->scope, param);
	hoister->bodyScope = bodyScope;
	hoister->functionScope = functionScope;
}

void fxHostNodeHoist(void* it, void* param) 
{
	txHostNode* self = it;
	txHoister* hoister = param;
	txScope* scope = hoister->bodyScope;
	if ((scope->token != XS_TOKEN_MODULE) && (scope->token != XS_TOKEN_PROGRAM))
		fxReportLineError(hoister->parser, self->line, "invalid host");
	else {
		// @@ check simple parameters
	}
}

void fxImportNodeHoist(void* it, void* param) 
{
	txImportNode* self = it;
	txHoister* hoister = param;
	if (self->specifiers && self->specifiers->length) {
		txSpecifierNode* specifier = (txSpecifierNode*)self->specifiers->first;
		while (specifier) {
			txDeclareNode* node;
			txSymbol* symbol = specifier->asSymbol ? specifier->asSymbol : specifier->symbol;
			if (self->flags & mxStrictFlag) {
				if ((symbol == hoister->parser->argumentsSymbol) || (symbol == hoister->parser->evalSymbol))
					fxReportLineError(hoister->parser, self->line, "invalid import %s", symbol->string);
			}
			node = fxScopeGetDeclareNode(hoister->scope, symbol);
			if (node)
				fxReportLineError(hoister->parser, self->line, "duplicate variable %s", symbol->string);
			else {
				specifier->declaration = node = fxDeclareNodeNew(hoister->parser, XS_TOKEN_LET, symbol);
				specifier->from = self->from;
				node->flags |= mxDeclareNodeClosureFlag | mxDeclareNodeUseClosureFlag;
				node->line = self->line;
				node->importSpecifier = specifier;
				fxScopeAddDeclareNode(hoister->scope, node);
			}
			specifier = (txSpecifierNode*)specifier->next;
		}
	}
	else {
		txSpecifierNode* specifier = fxSpecifierNodeNew(hoister->parser, XS_TOKEN_SPECIFIER);
		txDeclareNode* node = fxDeclareNodeNew(hoister->parser, XS_TOKEN_LET, C_NULL);
		specifier->from = self->from;
		node->flags |= mxDeclareNodeClosureFlag | mxDeclareNodeUseClosureFlag;
		node->line = self->line;
		node->importSpecifier = specifier;
		fxScopeAddDeclareNode(hoister->scope, node);
	}
}

void fxModuleNodeHoist(void* it, void* param) 
{
	txModuleNode* self = it;
	txHoister* hoister = param;
	hoister->functionScope = hoister->bodyScope = self->scope = fxScopeNew(param, it, XS_TOKEN_MODULE); // @@
	fxNodeDispatchHoist(self->body, param);
	fxScopeHoisted(self->scope, param);
}

void fxParamsBindingNodeHoist(void* it, void* param)
{
	txParamsNode* self = it;
	fxNodeListDistribute(self->items, fxNodeDispatchHoist, param);
}

void fxProgramNodeHoist(void* it, void* param) 
{
	txProgramNode* self = it;
	txHoister* hoister = param;
	hoister->functionScope = hoister->bodyScope = self->scope = fxScopeNew(param, it, (hoister->parser->flags & mxEvalFlag) ? XS_TOKEN_EVAL : XS_TOKEN_PROGRAM);
	fxNodeDispatchHoist(self->body, param);
	fxScopeHoisted(self->scope, param);
}

void fxStatementNodeHoist(void* it, void* param) 
{
	txStatementNode* self = it;
	fxNodeDispatchHoist(self->expression, param);
}

void fxSwitchNodeHoist(void* it, void* param) 
{
	txSwitchNode* self = it;
	fxNodeDispatchHoist(self->expression, param);
	self->scope = fxScopeNew(param, it, XS_TOKEN_BLOCK);
	fxNodeListDistribute(self->items, fxNodeDispatchHoist, param);
	fxScopeHoisted(self->scope, param);
}


void fxNodeDispatchBind(void* it, void* param)
{
	txNode* node = it;
	(*node->description->dispatch->bind)(it, param);
}

void fxNodeBind(void* it, void* param) 
{
	txNode* node = it;
	(*node->description->dispatch->distribute)(node, fxNodeDispatchBind, param);
}

void fxAccessNodeBind(void* it, void* param) 
{
	txAccessNode* self = it;
	txBinder* binder = param;
	fxScopeLookup(binder->scope, (txAccessNode*)self, 0);
}

void fxAccessNodeBindCompound(void* it, void* param, txAssignNode* compound) 
{
	fxAccessNodeBind(it, param);
	fxNodeDispatchBind(compound->value, param);
}

void fxAccessNodeBindPostfix(void* it, void* param, txPostfixExpressionNode* compound) 
{
	fxAccessNodeBind(it, param);
	fxBinderPushVariables(param, 1);
	fxBinderPopVariables(param, 1);
}

void fxArrayNodeBind(void* it, void* param) 
{
	txArrayNode* self = it;
	fxBinderPushVariables(param, 1);
	if (self->flags & mxSpreadFlag) {
		fxBinderPushVariables(param, 2);
		fxNodeListDistribute(self->items, fxNodeDispatchBind, param);
		fxBinderPopVariables(param, 2);
	}
	else
		fxNodeListDistribute(self->items, fxNodeDispatchBind, param);
	fxBinderPopVariables(param, 1);
}

void fxArrayBindingNodeBind(void* it, void* param) 
{
	txArrayBindingNode* self = it;
	if (self->initializer)
		fxNodeDispatchBind(self->initializer, param);
	fxBinderPushVariables(param, 6);
	fxNodeListDistribute(self->items, fxNodeDispatchBind, param);
	fxBinderPopVariables(param, 6);
}

void fxAssignNodeBind(void* it, void* param) 
{
	txAssignNode* self = it;
	txToken referenceToken = self->reference->description->token;
	txSymbol* symbol = C_NULL;
	fxNodeDispatchBind(self->reference, param);
	if (referenceToken == XS_TOKEN_ACCESS)
		symbol = ((txAccessNode*)self->reference)->symbol;
	else if ((referenceToken == XS_TOKEN_CONST) || (referenceToken == XS_TOKEN_LET) || (referenceToken == XS_TOKEN_VAR)) 
		symbol = ((txDeclareNode*)self->reference)->symbol;
	if (symbol)
		fxFunctionNodeRename(self->value, symbol);
	fxNodeDispatchBind(self->value, param);
}

void fxBindingNodeBind(void* it, void* param) 
{
	txBindingNode* self = it;
	txBinder* binder = param;
	self->flags |= mxDeclareNodeBoundFlag;
	fxScopeLookup(binder->scope, (txAccessNode*)self, 0);
	if (self->initializer) {
		fxFunctionNodeRename(self->initializer, self->symbol);
		fxNodeDispatchBind(self->initializer, param);
	}
}

void fxBlockNodeBind(void* it, void* param) 
{
	txBlockNode* self = it;
	fxScopeBinding(self->scope, param);
	fxScopeBindDefineNodes(self->scope, param);
	fxNodeDispatchBind(self->statement, param);
	fxScopeBound(self->scope, param);
}

void fxCatchNodeBind(void* it, void* param) 
{
	txCatchNode* self = it;
	if (self->parameter) {
		fxScopeBinding(self->scope, param);
		fxNodeDispatchBind(self->parameter, param);
		fxScopeBinding(self->statementScope, param);
		fxScopeBindDefineNodes(self->statementScope, param);
		fxNodeDispatchBind(self->statement, param);
		fxScopeBound(self->statementScope, param);
		fxScopeBound(self->scope, param);
	}
	else {
		fxScopeBinding(self->statementScope, param);
		fxScopeBindDefineNodes(self->statementScope, param);
		fxNodeDispatchBind(self->statement, param);
		fxScopeBound(self->statementScope, param);
	}
}

void fxClassNodeBind(void* it, void* param) 
{
	txClassNode* self = it;
	txNode* item = self->items->first;
	fxBinderPushVariables(param, 2);
	if (self->scope)
		fxScopeBinding(self->scope, param);
	if (self->heritage)
		fxNodeDispatchBind(self->heritage, param);
	fxNodeDispatchBind(self->constructor, param);
// 	if (self->symbol)
// 		((txFunctionNode*)self->constructor)->symbol = self->symbol;
	while (item) {
		txNode* value;
		if (item->description->token == XS_TOKEN_PROPERTY) {
			value = ((txPropertyNode*)item)->value;
		}
		else {
			value = ((txPropertyAtNode*)item)->value;
		}
		if ((value->description->token == XS_TOKEN_FUNCTION) || (value->description->token == XS_TOKEN_GENERATOR) || (value->description->token == XS_TOKEN_HOST)) {
			txFunctionNode* node = (txFunctionNode*)value;
			node->flags |= item->flags & (mxMethodFlag | mxGetterFlag | mxSetterFlag);
		}
		fxNodeDispatchBind(item, param);
		item = item->next;
	}
	if (self->scope) 
		fxScopeBound(self->scope, param);
	fxBinderPopVariables(param, 2);
}

void fxCompoundExpressionNodeBind(void* it, void* param) 
{
	txAssignNode* self = it;
	switch (self->reference->description->token) {
	case XS_TOKEN_ACCESS: fxAccessNodeBindCompound(self->reference, param, self); break;
	case XS_TOKEN_MEMBER: fxMemberNodeBindCompound(self->reference, param, self); break;
	case XS_TOKEN_MEMBER_AT: fxMemberAtNodeBindCompound(self->reference, param, self); break;
	}
}

void fxDeclareNodeBind(void* it, void* param) 
{
	txBindingNode* self = it;
	txBinder* binder = param;
	self->flags |= mxDeclareNodeBoundFlag | mxDeclareNodeInitializedFlag;
	fxScopeLookup(binder->scope, (txAccessNode*)self, 0);
	if (self->initializer) {
		fxFunctionNodeRename(self->initializer, self->symbol);
		fxNodeDispatchBind(self->initializer, param);
	}
}

void fxDefineNodeBind(void* it, void* param) 
{
	txDefineNode* self = it;
	txBinder* binder = param;
	if (self->flags & mxDefineNodeBoundFlag)
		return;
	self->flags |= mxDefineNodeBoundFlag;
	fxScopeLookup(binder->scope, (txAccessNode*)self, 0);
	fxNodeDispatchBind(self->initializer, param);
}

void fxDelegateNodeBind(void* it, void* param) 
{
	txStatementNode* self = it;
	fxBinderPushVariables(param, 4);
	fxNodeDispatchBind(self->expression, param);
	fxBinderPopVariables(param, 4);
}

void fxExportNodeBind(void* it, void* param) 
{
	txExportNode* self = it;
	txBinder* binder = param;
	if (self->from)
		return;
	if (self->specifiers) {
		txSpecifierNode* specifier = (txSpecifierNode*)self->specifiers->first;
		while (specifier) {
			txAccessNode* node = fxAccessNodeNew(binder->parser, XS_TOKEN_ACCESS, specifier->symbol);
			fxScopeLookup(binder->scope, node, 0);
			if (node->declaration) {
				specifier->declaration = node->declaration;
				specifier->declaration->flags |= mxDeclareNodeClosureFlag | mxDeclareNodeUseClosureFlag;
				specifier->nextSpecifier = specifier->declaration->firstExportSpecifier;
				specifier->declaration->firstExportSpecifier = specifier;
			}
			else
				fxReportLineError(binder->parser, specifier->line, "unknown variable %s", specifier->symbol->string);
			specifier = (txSpecifierNode*)specifier->next;
		}
	}
}

void fxForNodeBind(void* it, void* param) 
{
	txForNode* self = it;
	fxScopeBinding(self->scope, param);
	fxScopeBindDefineNodes(self->scope, param);
	if (self->initialization)
		fxNodeDispatchBind(self->initialization, param);
	if (self->expression)
		fxNodeDispatchBind(self->expression, param);
	if (self->iteration)
		fxNodeDispatchBind(self->iteration, param);
	fxNodeDispatchBind(self->statement, param);
	fxScopeBound(self->scope, param);
}

void fxForInForOfNodeBind(void* it, void* param) 
{
	txForInForOfNode* self = it;
	fxBinderPushVariables(param, 6);
	fxScopeBinding(self->scope, param);
	fxScopeBindDefineNodes(self->scope, param);
	fxNodeDispatchBind(self->reference, param);
	fxNodeDispatchBind(self->expression, param);
	fxNodeDispatchBind(self->statement, param);
	fxScopeBound(self->scope, param);
	fxBinderPopVariables(param, 6);
}

void fxFunctionNodeBind(void* it, void* param) 
{
	txFunctionNode* self = it;
	txBinder* binder = param;
	txInteger scopeLevel = binder->scopeLevel;
	txInteger scopeMaximum = binder->scopeMaximum;
	scopeLevel = binder->scopeLevel;
	scopeMaximum = binder->scopeMaximum;
	binder->scopeLevel = 0;
	binder->scopeMaximum = 0;
	fxScopeBinding(self->scope, param);
	fxNodeDispatchBind(self->params, param);
	fxScopeBindDefineNodes(self->scope, param);
	fxNodeDispatchBind(self->body, param);
	fxScopeBound(self->scope, param);
	self->variableCount = binder->scopeMaximum;
	binder->scopeMaximum = scopeMaximum;
	binder->scopeLevel = scopeLevel;
}

void fxFunctionNodeRename(void* it, txSymbol* symbol)
{
	txNode* self = it;
	txToken token = self->description->token;
	if (token == XS_TOKEN_EXPRESSIONS) {
		self = ((txExpressionsNode*)self)->items->first;
		if (self->next)
			return;
		token = self->description->token;
	}
	if (token == XS_TOKEN_CLASS) {
		txClassNode* node = (txClassNode*)self;
		if (!node->symbol)
			node->symbol = symbol;
	}
	else if ((token == XS_TOKEN_FUNCTION) || (token == XS_TOKEN_GENERATOR) || (token == XS_TOKEN_HOST)) {
		txFunctionNode* node = (txFunctionNode*)self;
		if (!node->symbol)
			node->symbol = symbol;
	}
}

void fxHostNodeBind(void* it, void* param) 
{
}

void fxMemberNodeBindCompound(void* it, void* param, txAssignNode* compound) 
{
	txMemberNode* self = it;
	fxNodeDispatchBind(self->reference, param);
	fxNodeDispatchBind(compound->value, param);
}

void fxMemberNodeBindPostfix(void* it, void* param, txPostfixExpressionNode* compound) 
{
	txMemberNode* self = it;
	fxNodeDispatchBind(self->reference, param);
	fxBinderPushVariables(param, 1);
	fxBinderPopVariables(param, 1);
}

void fxMemberAtNodeBindCompound(void* it, void* param, txAssignNode* compound) 
{
	txMemberAtNode* self = it;
	fxNodeDispatchBind(self->reference, param);
	fxNodeDispatchBind(self->at, param);
	fxNodeDispatchBind(compound->value, param);
}

void fxMemberAtNodeBindPostfix(void* it, void* param, txPostfixExpressionNode* compound) 
{
	txMemberAtNode* self = it;
	fxNodeDispatchBind(self->reference, param);
	fxNodeDispatchBind(self->at, param);
	fxBinderPushVariables(param, 1);
	fxBinderPopVariables(param, 1);
}

void fxModuleNodeBind(void* it, void* param) 
{
	txModuleNode* self = it;
	txBinder* binder = param;
	fxScopeBinding(self->scope, param);
	fxScopeBindDefineNodes(self->scope, param);
	fxNodeDispatchBind(self->body, param);
	fxScopeBound(self->scope, param);
	self->variableCount = binder->scopeMaximum;
}

void fxObjectNodeBind(void* it, void* param) 
{
	txObjectNode* self = it;
	txNode* item = self->items->first;
	fxBinderPushVariables(param, 1);
	while (item) {
		txNode* value;
		if (item->description->token == XS_TOKEN_SPREAD) {
		}
		else {
			if (item->description->token == XS_TOKEN_PROPERTY) {
				value = ((txPropertyNode*)item)->value;
			}
			else {
				value = ((txPropertyAtNode*)item)->value;
			}
			if ((value->description->token == XS_TOKEN_FUNCTION) || (value->description->token == XS_TOKEN_GENERATOR) || (value->description->token == XS_TOKEN_HOST)) {
				txFunctionNode* node = (txFunctionNode*)value;
				node->flags |= item->flags & (mxMethodFlag | mxGetterFlag | mxSetterFlag);
			}
			else if (value->description->token == XS_TOKEN_CLASS) {
	//			txFunctionNode* node = (txFunctionNode*)(((txClassNode*)value)->constructor);
			}
		}
		fxNodeDispatchBind(item, param);
		item = item->next;
	}
	fxBinderPopVariables(param, 1);
}

void fxObjectBindingNodeBind(void* it, void* param) 
{
	txArrayBindingNode* self = it;
	if (self->initializer)
		fxNodeDispatchBind(self->initializer, param);
	fxBinderPushVariables(param, 2);
	fxNodeListDistribute(self->items, fxNodeDispatchBind, param);
	fxBinderPopVariables(param, 2);
}

void fxParamsNodeBind(void* it, void* param) 
{
	txParamsNode* self = it;
	if (self->flags & mxSpreadFlag) {
		fxBinderPushVariables(param, 1);
		fxNodeListDistribute(self->items, fxNodeDispatchBind, param);
		fxBinderPopVariables(param, 1);
	}
	else
		fxNodeListDistribute(self->items, fxNodeDispatchBind, param);
}

void fxParamsBindingNodeBind(void* it, void* param) 
{
	txParamsBindingNode* self = it;
	txBinder* binder = param;
	txScope* functionScope = binder->scope;
	txFunctionNode* functionNode = (txFunctionNode*)(functionScope->node);
	if (functionNode->flags & mxGetterFlag) {
		txInteger count = self->items->length;
		if (count != 0)
			fxReportLineError(binder->parser, self->line, "invalid getter arguments");
	}
	else if (functionNode->flags & mxSetterFlag) {
		txInteger count = self->items->length;
		if ((count != 1) || (self->items->first->description->token == XS_TOKEN_REST_BINDING))
			fxReportLineError(binder->parser, self->line, "invalid setter arguments");
	}
	if (functionNode->flags & mxArgumentsFlag) {
		txNode* item;
		self->declaration = fxScopeGetDeclareNode(functionScope, binder->parser->argumentsSymbol);
		if (functionNode->flags & mxStrictFlag)
			goto bail;
		item = self->items->first;
		while (item) {
			if (item->description->token != XS_TOKEN_ARG)
				goto bail;
			if (((txBindingNode*)item)->initializer)
				goto bail;
			item = item->next;
		}
		item = self->items->first;
		while (item) {
			item->flags |= mxDeclareNodeClosureFlag;
			item = item->next;
		}
		self->mapped = 1;
	}
bail:
	fxNodeListDistribute(self->items, fxNodeDispatchBind, param);
}

void fxPostfixExpressionNodeBind(void* it, void* param) 
{
	txPostfixExpressionNode* self = it;
	switch (self->left->description->token) {
	case XS_TOKEN_ACCESS: fxAccessNodeBindPostfix(self->left, param, self); break;
	case XS_TOKEN_MEMBER: fxMemberNodeBindPostfix(self->left, param, self); break;
	case XS_TOKEN_MEMBER_AT: fxMemberAtNodeBindPostfix(self->left, param, self); break;
	}
}

void fxProgramNodeBind(void* it, void* param) 
{
	txProgramNode* self = it;
	txBinder* binder = param;
	fxScopeBinding(self->scope, param);
	fxScopeBindDefineNodes(self->scope, param);
	fxNodeDispatchBind(self->body, param);
	fxScopeBound(self->scope, param);
	self->variableCount = binder->scopeMaximum;
}

void fxSpreadNodeBind(void* it, void* param) 
{
	txSpreadNode* self = it;
	fxBinderPushVariables(param, 1);
	fxNodeDispatchBind(self->expression, param);
	fxBinderPopVariables(param, 1);
}

void fxSwitchNodeBind(void* it, void* param) 
{
	txSwitchNode* self = it;
	fxNodeDispatchBind(self->expression, param);
	fxScopeBinding(self->scope, param);
	fxScopeBindDefineNodes(self->scope, param);
	fxNodeListDistribute(self->items, fxNodeDispatchBind, param);
	fxScopeBound(self->scope, param);
}

void fxTemplateNodeBind(void* it, void* param) 
{
	txTemplateNode* self = it;
	fxBinderPushVariables(param, 2);
	if (self->reference)
		fxNodeDispatchBind(self->reference, param);
	fxNodeListDistribute(self->items, fxNodeDispatchBind, param);
	fxBinderPopVariables(param, 2);
}

void fxTryNodeBind(void* it, void* param) 
{
	txTryNode* self = it;
	fxBinderPushVariables(param, 2);
	fxNodeDispatchBind(self->tryBlock, param);
	if (self->catchBlock)
		fxNodeDispatchBind(self->catchBlock, param);
	if (self->finallyBlock)
		fxNodeDispatchBind(self->finallyBlock, param);
	fxBinderPopVariables(param, 2);
}

void fxWithNodeBind(void* it, void* param) 
{
	txWithNode* self = it;
	fxNodeDispatchBind(self->expression, param);
	fxScopeBinding(self->scope, param);
	fxNodeDispatchBind(self->statement, param);
	fxScopeBound(self->scope, param);
}

void fxWithNodeHoist(void* it, void* param) 
{
	txWithNode* self = it;
	fxNodeDispatchHoist(self->expression, param);
	self->scope = fxScopeNew(param, it, XS_TOKEN_WITH);
	fxNodeDispatchHoist(self->statement, param);
	fxScopeHoisted(self->scope, param);
}




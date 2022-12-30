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

static txBoolean fxIsKeyword(txParser* parser, txSymbol* keyword);
static txBoolean fxIsToken(txParser* parser, txToken theToken);
static void fxMatchToken(txParser* parser, txToken theToken);
static txNode* fxPopNode(txParser* parser);
static void fxPushNode(txParser* parser, txNode* node);
static void fxPushBigIntNode(txParser* parser, txBigInt* value, txInteger line);
static void fxPushIndexNode(txParser* parser, txIndex value, txInteger line);
static void fxPushIntegerNode(txParser* parser, txInteger value, txInteger line);
static void fxPushNodeStruct(txParser* parser, txInteger count, txToken token, txInteger line);
static void fxPushNodeList(txParser* parser, txInteger count);
static void fxPushNULL(txParser* parser);
static void fxPushNumberNode(txParser* parser, txNumber value, txInteger line);
static void fxPushRawNode(txParser* parser, txInteger length, txString value, txInteger line);
static void fxPushStringNode(txParser* parser, txInteger length, txString value, txInteger line);
static void fxPushSymbol(txParser* parser, txSymbol* symbol);
static void fxSwapNodes(txParser* parser);

static void fxExport(txParser* parser);
static void fxExportBinding(txParser* parser, txNode* node);
static void fxImport(txParser* parser);
static void fxSpecifiers(txParser* parser);

static void fxBody(txParser* parser);
static void fxStatements(txParser* parser);
static void fxBlock(txParser* parser);
static void fxStatement(txParser* parser, txBoolean blockIt);
static void fxSemicolon(txParser* parser);
static void fxBreakStatement(txParser* parser);
static void fxContinueStatement(txParser* parser);
static void fxDebuggerStatement(txParser* parser);
static void fxDoStatement(txParser* parser);
static void fxForStatement(txParser* parser);
static void fxIfStatement(txParser* parser);
static void fxReturnStatement(txParser* parser);
static void fxSwitchStatement(txParser* parser);
static void fxThrowStatement(txParser* parser);
static void fxTryStatement(txParser* parser);
static void fxVariableStatement(txParser* parser, txToken theToken);
static void fxWhileStatement(txParser* parser);
static void fxWithStatement(txParser* parser);

static void fxCommaExpression(txParser* parser);
static void fxAssignmentExpression(txParser* parser);
static void fxConditionalExpression(txParser* parser);
static void fxOrExpression(txParser* parser);
static void fxAndExpression(txParser* parser);
static void fxCoalesceExpression(txParser* parser);
static void fxBitOrExpression(txParser* parser);
static void fxBitXorExpression(txParser* parser);
static void fxBitAndExpression(txParser* parser);
static void fxEqualExpression(txParser* parser);
static void fxRelationalExpression(txParser* parser);
static void fxShiftExpression(txParser* parser);
static void fxAdditiveExpression(txParser* parser);
static void fxMultiplicativeExpression(txParser* parser);
static void fxExponentiationExpression(txParser* parser);
static void fxUnaryExpression(txParser* parser);
static void fxPrefixExpression(txParser* parser);
static void fxPostfixExpression(txParser* parser);
static void fxCallExpression(txParser* parser);

static void fxLiteralExpression(txParser* parser, txUnsigned flag);
static void fxArrayExpression(txParser* parser);
static void fxArrowExpression(txParser* parser, txUnsigned flag);
static void fxClassExpression(txParser* parser, txInteger theLine, txSymbol** theSymbol);
static void fxFunctionExpression(txParser* parser, txInteger theLine, txSymbol** theSymbol, txUnsigned flag);
static void fxGeneratorExpression(txParser* parser, txInteger theLine, txSymbol** theSymbol, txUnsigned flag);
static void fxGroupExpression(txParser* parser, txUnsigned flag);
static void fxObjectExpression(txParser* parser);
static void fxNewExpression(txParser* parser);
static void fxTemplateExpression(txParser* parser);
static void fxYieldExpression(txParser* parser);

static void fxParameters(txParser* parser);
static void fxPropertyName(txParser* parser, txSymbol** theSymbol, txToken* theToken0, txToken* theToken1, txToken* theToken2, txUnsigned* flag);

static void fxBinding(txParser* parser, txToken theToken, txFlag initializeIt);
static txNode* fxBindingFromExpression(txParser* parser, txNode* theNode, txToken theToken);
static void fxArrayBinding(txParser* parser, txToken theToken);
static txNode* fxArrayBindingFromExpression(txParser* parser, txNode* theNode, txToken theToken);
static txUnsigned fxObjectBinding(txParser* parser, txToken theToken);
static txNode* fxObjectBindingFromExpression(txParser* parser, txNode* theNode, txToken theToken);
static void fxParametersBinding(txParser* parser);
static txNode* fxParametersBindingFromExpressions(txParser* parser, txNode* theNode);
static void fxRestBinding(txParser* parser, txToken theToken, txUnsigned flag);
static txNode* fxRestBindingFromExpression(txParser* parser, txNode* theNode, txToken theToken, txUnsigned flag);

static void fxCheckArrowFunction(txParser* parser, txInteger count);
static txBoolean fxCheckReference(txParser* parser, txToken theToken);
static void fxCheckStrictBinding(txParser* parser, txNode* node);
static void fxCheckStrictFunction(txParser* parser, txFunctionNode* function);
static void fxCheckStrictSymbol(txParser* parser, txSymbol* symbol);
static void fxCheckUniqueProperty(txParser* parser, txNode* base, txNode* current);
static void fxCheckUniquePropertyAux(txParser* parser, txNode* baseNode, txNode* currentNode);

static void fxJSONObject(txParser* parser);
static void fxJSONArray(txParser* parser);

static void fxJSXAttributeName(txParser* parser);
static void fxJSXAttributeValue(txParser* parser);
static void fxJSXElement(txParser* parser);
static void fxJSXElementName(txParser* parser);
static txBoolean fxJSXMatch(txParser* parser, txNode* opening, txNode* closing);
static txSymbol* fxJSXNamespace(txParser* parser, txSymbol* namespace, txSymbol* name);

#define XS_TOKEN_BEGIN_STATEMENT 1
#define XS_TOKEN_BEGIN_EXPRESSION 2
#define XS_TOKEN_ASSIGN_EXPRESSION 4
#define XS_TOKEN_EQUAL_EXPRESSION 8
#define XS_TOKEN_RELATIONAL_EXPRESSION 16
#define XS_TOKEN_SHIFT_EXPRESSION 32
#define XS_TOKEN_ADDITIVE_EXPRESSION 64
#define XS_TOKEN_MULTIPLICATIVE_EXPRESSION 128
#define XS_TOKEN_EXPONENTIATION_EXPRESSION 256
#define XS_TOKEN_PREFIX_EXPRESSION 512
#define XS_TOKEN_POSTFIX_EXPRESSION 1024
#define XS_TOKEN_END_STATEMENT 2048
#define XS_TOKEN_REFERENCE_EXPRESSION 4096
#define XS_TOKEN_BEGIN_BINDING 16384
#define XS_TOKEN_IDENTIFIER_NAME 32768
#define XS_TOKEN_UNARY_EXPRESSION 65536
#define XS_TOKEN_CALL_EXPRESSION 131072

static txTokenFlag gxTokenFlags[XS_TOKEN_COUNT] = {
	/* XS_NO_TOKEN */ 0,
	/* XS_TOKEN_ACCESS */ 0,
	/* XS_TOKEN_ADD */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_ADDITIVE_EXPRESSION | XS_TOKEN_UNARY_EXPRESSION,
	/* XS_TOKEN_ADD_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_AND */ 0,
	/* XS_TOKEN_AND_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_ARG */ 0,
	/* XS_TOKEN_ARGUMENTS */ 0,
	/* XS_TOKEN_ARGUMENTS_SLOPPY */ 0,
	/* XS_TOKEN_ARGUMENTS_STRICT */ 0,
	/* XS_TOKEN_ARRAY */ 0,
	/* XS_TOKEN_ARRAY_BINDING */ 0,
	/* XS_TOKEN_ARROW */ 0,
	/* XS_TOKEN_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_AWAIT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_UNARY_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_BIGINT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_BINDING */ 0,
	/* XS_TOKEN_BIT_AND */ 0,
	/* XS_TOKEN_BIT_AND_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_BIT_NOT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_UNARY_EXPRESSION,
	/* XS_TOKEN_BIT_OR */ 0,
	/* XS_TOKEN_BIT_OR_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_BIT_XOR */ 0,
	/* XS_TOKEN_BIT_XOR_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_BLOCK */ 0,
	/* XS_TOKEN_BODY */ 0,
	/* XS_TOKEN_BREAK */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_CALL */ 0,
	/* XS_TOKEN_CASE */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_CATCH */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_CHAIN */ XS_TOKEN_CALL_EXPRESSION,
	/* XS_TOKEN_CLASS */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_COALESCE */ 0,
	/* XS_TOKEN_COALESCE_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_COLON */ 0,
	/* XS_TOKEN_COMMA */ XS_TOKEN_END_STATEMENT,
	/* XS_TOKEN_CONST */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_CONTINUE */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_CURRENT */ 0,
	/* XS_TOKEN_DEBUGGER */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_DECREMENT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_PREFIX_EXPRESSION | XS_TOKEN_POSTFIX_EXPRESSION,
	/* XS_TOKEN_DEFAULT */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_DEFINE */ 0,
	/* XS_TOKEN_DELEGATE */ 0,
	/* XS_TOKEN_DELETE */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_UNARY_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_DIVIDE */ XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_MULTIPLICATIVE_EXPRESSION,
	/* XS_TOKEN_DIVIDE_ASSIGN */ XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_DO */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_DOT */ XS_TOKEN_CALL_EXPRESSION,
	/* XS_TOKEN_ELISION */ 0,
	/* XS_TOKEN_ELSE */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_ENUM */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_EOF */ XS_TOKEN_END_STATEMENT,
	/* XS_TOKEN_EQUAL */ XS_TOKEN_EQUAL_EXPRESSION,
	/* XS_TOKEN_EVAL */ 0,
	/* XS_TOKEN_EXPONENTIATION */ XS_TOKEN_EXPONENTIATION_EXPRESSION,
	/* XS_TOKEN_EXPONENTIATION_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_EXPORT */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_EXPRESSIONS */ 0,
	/* XS_TOKEN_EXTENDS */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_FALSE */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_FIELD */ 0,
	/* XS_TOKEN_FINALLY */ XS_TOKEN_IDENTIFIER_NAME | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_FOR */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_FOR_AWAIT_OF */ 0,
	/* XS_TOKEN_FOR_IN */ 0,
	/* XS_TOKEN_FOR_OF */ 0,
	/* XS_TOKEN_FUNCTION */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_GENERATOR */ 0,
	/* XS_TOKEN_GETTER */ 0,
	/* XS_TOKEN_HOST */ XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_IDENTIFIER */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_BEGIN_BINDING | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_IF */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_IMPLEMENTS */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_IMPORT */ XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_IMPORT_CALL */ 0,
	/* XS_TOKEN_IMPORT_META */ 0,
	/* XS_TOKEN_IN */ XS_TOKEN_RELATIONAL_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_INCLUDE */ 0,
	/* XS_TOKEN_INCREMENT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_PREFIX_EXPRESSION | XS_TOKEN_POSTFIX_EXPRESSION,
	/* XS_TOKEN_INSTANCEOF */ XS_TOKEN_RELATIONAL_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_INTEGER */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_INTERFACE */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_ITEMS */ 0,
	/* XS_TOKEN_LABEL */ 0,
	/* XS_TOKEN_LEFT_BRACE */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_BEGIN_BINDING,
	/* XS_TOKEN_LEFT_BRACKET */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_BEGIN_BINDING | XS_TOKEN_CALL_EXPRESSION,
	/* XS_TOKEN_LEFT_PARENTHESIS */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_CALL_EXPRESSION,
	/* XS_TOKEN_LEFT_SHIFT */ XS_TOKEN_SHIFT_EXPRESSION,
	/* XS_TOKEN_LEFT_SHIFT_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_LESS */ XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_RELATIONAL_EXPRESSION,
	/* XS_TOKEN_LESS_EQUAL */ XS_TOKEN_RELATIONAL_EXPRESSION,
	/* XS_TOKEN_LET */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_MEMBER */ 0,
	/* XS_TOKEN_MEMBER_AT */ 0,
	/* XS_TOKEN_MINUS */ 0,
	/* XS_TOKEN_MODULE */ 0,
	/* XS_TOKEN_MODULO */ XS_TOKEN_MULTIPLICATIVE_EXPRESSION,
	/* XS_TOKEN_MODULO_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_MORE */ XS_TOKEN_RELATIONAL_EXPRESSION,
	/* XS_TOKEN_MORE_EQUAL */ XS_TOKEN_RELATIONAL_EXPRESSION,
	/* XS_TOKEN_MULTIPLY */ XS_TOKEN_MULTIPLICATIVE_EXPRESSION,
	/* XS_TOKEN_MULTIPLY_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_NEW */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME, 
	/* XS_TOKEN_NOT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_UNARY_EXPRESSION,
	/* XS_TOKEN_NOT_EQUAL */ XS_TOKEN_EQUAL_EXPRESSION,
	/* XS_TOKEN_NULL */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME, 
	/* XS_TOKEN_NUMBER */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_OBJECT */ 0,
	/* XS_TOKEN_OBJECT_BINDING */ 0,
	/* XS_TOKEN_OPTION */ 0,
	/* XS_TOKEN_OR */ 0,
	/* XS_TOKEN_OR_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_PACKAGE */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_PARAMS */ 0,
	/* XS_TOKEN_PARAMS_BINDING */ 0,
	/* XS_TOKEN_PLUS */ 0,
	/* XS_TOKEN_PRIVATE */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_PRIVATE_IDENTIFIER */ XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_PRIVATE_MEMBER */ 0,
	/* XS_TOKEN_PRIVATE_PROPERTY */ 0,
	/* XS_TOKEN_PROGRAM */ 0,
	/* XS_TOKEN_PROPERTY */ 0,
	/* XS_TOKEN_PROPERTY_AT */ 0,
	/* XS_TOKEN_PROPERTY_BINDING */ 0,
	/* XS_TOKEN_PROPERTY_BINDING_AT */ 0,
	/* XS_TOKEN_PROTECTED */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_PUBLIC */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_QUESTION_MARK */ 0,
	/* XS_TOKEN_REGEXP */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_REST_BINDING */ 0,
	/* XS_TOKEN_RETURN */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_RIGHT_BRACE */ XS_TOKEN_END_STATEMENT,
	/* XS_TOKEN_RIGHT_BRACKET */ 0,
	/* XS_TOKEN_RIGHT_PARENTHESIS */ 0,
	/* XS_TOKEN_SEMICOLON */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_END_STATEMENT,
	/* XS_TOKEN_SETTER */ 0,
	/* XS_TOKEN_SHORT */ 0,
	/* XS_TOKEN_SIGNED_RIGHT_SHIFT */ XS_TOKEN_SHIFT_EXPRESSION,
	/* XS_TOKEN_SIGNED_RIGHT_SHIFT_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_SKIP_BINDING */ 0,
	/* XS_TOKEN_SPECIFIER */ 0,
	/* XS_TOKEN_SPREAD */ XS_TOKEN_BEGIN_BINDING,
	/* XS_TOKEN_STATEMENT */ 0,
	/* XS_TOKEN_STATEMENTS */ 0,
	/* XS_TOKEN_STATIC */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_STRICT_EQUAL */ XS_TOKEN_EQUAL_EXPRESSION,
	/* XS_TOKEN_STRICT_NOT_EQUAL */ XS_TOKEN_EQUAL_EXPRESSION,
	/* XS_TOKEN_STRING */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_SUBTRACT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_ADDITIVE_EXPRESSION | XS_TOKEN_UNARY_EXPRESSION,
	/* XS_TOKEN_SUBTRACT_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_SUPER */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_SWITCH */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_TARGET */ 0,
	/* XS_TOKEN_TEMPLATE */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_CALL_EXPRESSION,
	/* XS_TOKEN_TEMPLATE_HEAD */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_CALL_EXPRESSION,
	/* XS_TOKEN_TEMPLATE_MIDDLE */ 0,
	/* XS_TOKEN_TEMPLATE_TAIL */ 0,
	/* XS_TOKEN_THIS */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_THROW */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_TRUE */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_TRY */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_TYPEOF */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_UNARY_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_UNDEFINED */ 0,
	/* XS_TOKEN_UNSIGNED_RIGHT_SHIFT */ XS_TOKEN_SHIFT_EXPRESSION,
	/* XS_TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_VAR */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_VOID */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_UNARY_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_WHILE */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_WITH */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_YIELD */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME
};

static txString const gxTokenNames[XS_TOKEN_COUNT] ICACHE_FLASH_ATTR = {
	/* XS_NO_TOKEN */ "",
	/* XS_TOKEN_ACCESS */ "access",
	/* XS_TOKEN_ADD */ "+",
	/* XS_TOKEN_ADD_ASSIGN */ "+=",
	/* XS_TOKEN_AND */ "&&",
	/* XS_TOKEN_AND_ASSIGN */ "&&=",
	/* XS_TOKEN_ARG */ "arg",
	/* XS_TOKEN_ARGUMENTS */ "arguments",
	/* XS_TOKEN_ARGUMENTS_SLOPPY */ "arguments_sloppy",
	/* XS_TOKEN_ARGUMENTS_STRICT */ "arguments_strict",
	/* XS_TOKEN_ARRAY */ "array",
	/* XS_TOKEN_ARRAY_BINDING */ "array_binding",
	/* XS_TOKEN_ARROW */ "=>",
	/* XS_TOKEN_ASSIGN */ "=",
	/* XS_TOKEN_AWAIT */ "await",
	/* XS_TOKEN_BIGINT */ "bigint",
	/* XS_TOKEN_BINDING */ "binding",
	/* XS_TOKEN_BIT_AND */ "&",
	/* XS_TOKEN_BIT_AND_ASSIGN */ "&=",
	/* XS_TOKEN_BIT_NOT */ "~",
	/* XS_TOKEN_BIT_OR */ "|",
	/* XS_TOKEN_BIT_OR_ASSIGN */ "|=",
	/* XS_TOKEN_BIT_XOR */ "^",
	/* XS_TOKEN_BIT_XOR_ASSIGN */ "^=",
	/* XS_TOKEN_BLOCK */ "block",
	/* XS_TOKEN_BODY */ "body",
	/* XS_TOKEN_BREAK */ "break",
	/* XS_TOKEN_CALL */ "call",
	/* XS_TOKEN_CASE */ "case",
	/* XS_TOKEN_CATCH */ "catch",
	/* XS_TOKEN_CHAIN */ "?.",
	/* XS_TOKEN_CLASS */ "class",
	/* XS_TOKEN_COALESCE */ "??",
	/* XS_TOKEN_COALESCE_ASSIGN */ "\?\?=",
	/* XS_TOKEN_COLON */ ":",
	/* XS_TOKEN_COMMA */ ",",
	/* XS_TOKEN_CONST */ "const",
	/* XS_TOKEN_CONTINUE */ "continue",
	/* XS_TOKEN_CURRENT */ "current",
	/* XS_TOKEN_DEBUGGER */ "debugger",
	/* XS_TOKEN_DECREMENT */ "--",
	/* XS_TOKEN_DEFAULT */ "default",
	/* XS_TOKEN_DEFINE */ "define",
	/* XS_TOKEN_DELEGATE */ "delegate",
	/* XS_TOKEN_DELETE */ "delete",
	/* XS_TOKEN_DIVIDE */ "/",
	/* XS_TOKEN_DIVIDE_ASSIGN */ "/=",
	/* XS_TOKEN_DO */ "do",
	/* XS_TOKEN_DOT */ ".",
	/* XS_TOKEN_ELISION */ "elision",
	/* XS_TOKEN_ELSE */ "else",
	/* XS_TOKEN_ENUM */ "enum",
	/* XS_TOKEN_EOF */ "",
	/* XS_TOKEN_EQUAL */ "==",
	/* XS_TOKEN_EVAL */ "eval",
	/* XS_TOKEN_EXPONENTIATION */ "**",
	/* XS_TOKEN_EXPONENTIATION_ASSIGN */ "**=",
	/* XS_TOKEN_EXPORT */ "export",
	/* XS_TOKEN_EXPRESSIONS */ "expressions",
	/* XS_TOKEN_EXTENDS */ "extends",
	/* XS_TOKEN_FALSE */ "false",
	/* XS_TOKEN_FIELD */ "field",
	/* XS_TOKEN_FINALLY */ "finally",
	/* XS_TOKEN_FOR */ "for",
	/* XS_TOKEN_FOR_AWAIT_OF */ "for_await_of",
	/* XS_TOKEN_FOR_IN */ "for_in",
	/* XS_TOKEN_FOR_OF */ "for_of",
	/* XS_TOKEN_FUNCTION */ "function",
	/* XS_TOKEN_GENERATOR */ "generator",
	/* XS_TOKEN_GETTER */ "getter",
	/* XS_TOKEN_HOST */ "host", 
	/* XS_TOKEN_IDENTIFIER */ "identifier",
	/* XS_TOKEN_IF */ "if",
	/* XS_TOKEN_IMPLEMENTS */ "implements",
	/* XS_TOKEN_IMPORT */ "import",
	/* XS_TOKEN_IMPORT_CALL */ "import",
	/* XS_TOKEN_IMPORT_META */ "import.meta",
	/* XS_TOKEN_IN */ "in",
	/* XS_TOKEN_INCLUDE */ "include",
	/* XS_TOKEN_INCREMENT */ "++",
	/* XS_TOKEN_INSTANCEOF */ "instanceof",
	/* XS_TOKEN_INTEGER */ "integer",
	/* XS_TOKEN_INTERFACE */ "interface",
	/* XS_TOKEN_ITEMS */ "items",
	/* XS_TOKEN_LABEL */ "label",
	/* XS_TOKEN_LEFT_BRACE */ "{",
	/* XS_TOKEN_LEFT_BRACKET */ "[",
	/* XS_TOKEN_LEFT_PARENTHESIS */ "(",
	/* XS_TOKEN_LEFT_SHIFT */ "<<",
	/* XS_TOKEN_LEFT_SHIFT_ASSIGN */ "<<=",
	/* XS_TOKEN_LESS */ "<",
	/* XS_TOKEN_LESS_EQUAL */ "<=",
	/* XS_TOKEN_LET */ "let",
	/* XS_TOKEN_MEMBER */ "member",
	/* XS_TOKEN_MEMBER_AT */ "member_at",
	/* XS_TOKEN_MINUS */ "minus",
	/* XS_TOKEN_MODULE */ "module",
	/* XS_TOKEN_MODULO */ "%",
	/* XS_TOKEN_MODULO_ASSIGN */ "%=",
	/* XS_TOKEN_MORE */ ">",
	/* XS_TOKEN_MORE_EQUAL */ ">=",
	/* XS_TOKEN_MULTIPLY */ "*",
	/* XS_TOKEN_MULTIPLY_ASSIGN */ "*=",
	/* XS_TOKEN_NEW */ "new", 
	/* XS_TOKEN_NOT */ "!",
	/* XS_TOKEN_NOT_EQUAL */ "!=",
	/* XS_TOKEN_NULL */ "null", 
	/* XS_TOKEN_NUMBER */ "number",
	/* XS_TOKEN_OBJECT */ "object",
	/* XS_TOKEN_OBJECT_BINDING */ "object_binding",
	/* XS_TOKEN_OPTION */ "?.",
	/* XS_TOKEN_OR */ "||",
	/* XS_TOKEN_OR_ASSIGN */ "||=",
	/* XS_TOKEN_PACKAGE */ "package",
	/* XS_TOKEN_PARAMS */ "params",
	/* XS_TOKEN_PARAMS_BINDING */ "params_binding",
	/* XS_TOKEN_PLUS */ "plus",
	/* XS_TOKEN_PRIVATE */ "private",
	/* XS_TOKEN_PRIVATE_IDENTIFIER */ "private_identifier",
	/* XS_TOKEN_PRIVATE_MEMBER */ "private_member",
	/* XS_TOKEN_PRIVATE_PROPERTY */ "private_property",
	/* XS_TOKEN_PROGRAM */ "program",
	/* XS_TOKEN_PROPERTY */ "property",
	/* XS_TOKEN_PROPERTY_AT */ "property_at",
	/* XS_TOKEN_PROPERTY_BINDING */ "property_binding",
	/* XS_TOKEN_PROPERTY_BINDING_AT */ "property_binding_at",
	/* XS_TOKEN_PROTECTED */ "protected",
	/* XS_TOKEN_PUBLIC */ "public",
	/* XS_TOKEN_QUESTION_MARK */ "?",
	/* XS_TOKEN_REGEXP */ "regexp",
	/* XS_TOKEN_REST_BINDING */ "rest_binding",
	/* XS_TOKEN_RETURN */ "return",
	/* XS_TOKEN_RIGHT_BRACE */ "}",
	/* XS_TOKEN_RIGHT_BRACKET */ "]",
	/* XS_TOKEN_RIGHT_PARENTHESIS */ ")",
	/* XS_TOKEN_SEMICOLON */ ";",
	/* XS_TOKEN_SETTER */ "setter",
	/* XS_TOKEN_SHORT */ "short",
	/* XS_TOKEN_SIGNED_RIGHT_SHIFT */ ">>",
	/* XS_TOKEN_SIGNED_RIGHT_SHIFT_ASSIGN */ ">>=",
	/* XS_TOKEN_SKIP_BINDING */ "skip_binding",
	/* XS_TOKEN_SPECIFIER */ "specifier",
	/* XS_TOKEN_SPREAD */ "...",
	/* XS_TOKEN_STATEMENT */ "statement",
	/* XS_TOKEN_STATEMENTS */ "statements",
	/* XS_TOKEN_STATIC */ "static",
	/* XS_TOKEN_STRICT_EQUAL */ "===",
	/* XS_TOKEN_STRICT_NOT_EQUAL */ "!==",
	/* XS_TOKEN_STRING */ "string",
	/* XS_TOKEN_SUBTRACT */ "-",
	/* XS_TOKEN_SUBTRACT_ASSIGN */ "-=",
	/* XS_TOKEN_SUPER */ "super",
	/* XS_TOKEN_SWITCH */ "switch",
	/* XS_TOKEN_TARGET */ "target",
	/* XS_TOKEN_TEMPLATE */ "template",
	/* XS_TOKEN_TEMPLATE_HEAD */ "template_head",
	/* XS_TOKEN_TEMPLATE_MIDDLE */ "template_middle",
	/* XS_TOKEN_TEMPLATE_TAIL */ "template_tail",
	/* XS_TOKEN_THIS */ "this",
	/* XS_TOKEN_THROW */ "throw",
	/* XS_TOKEN_TRUE */ "true",
	/* XS_TOKEN_TRY */ "try",
	/* XS_TOKEN_TYPEOF */ "typeof",
	/* XS_TOKEN_UNDEFINED */ "undefined",
	/* XS_TOKEN_UNSIGNED_RIGHT_SHIFT */ ">>>",
	/* XS_TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN */ ">>>=",
	/* XS_TOKEN_VAR */ "var",
	/* XS_TOKEN_VOID */ "void",
	/* XS_TOKEN_WHILE */ "while",
	/* XS_TOKEN_WITH */ "with",
	/* XS_TOKEN_YIELD */ "yield",
};

txBoolean fxIsKeyword(txParser* parser, txSymbol* keyword)
{
	txBoolean result = ((parser->token == XS_TOKEN_IDENTIFIER) && (parser->symbol == keyword)) ? 1 : 0;
	if (result) {
		if (parser->escaped)
			fxReportParserError(parser, parser->line, "escaped keyword");
	}
	return result;
}

txBoolean fxIsToken(txParser* parser, txToken theToken)
{
	txBoolean result = (parser->token == theToken) ? 1 : 0;
	if (result) {
		if (parser->escaped)
			fxReportParserError(parser, parser->line, "escaped keyword");
	}
	return result;
}

void fxMatchToken(txParser* parser, txToken theToken)
{
	if (parser->token == theToken) {
		if (parser->escaped)
			fxReportParserError(parser, parser->line, "escaped keyword");
		fxGetNextToken(parser);
	}
	else
		fxReportParserError(parser, parser->line, "missing %s", gxTokenNames[theToken]);
}

txNode* fxPopNode(txParser* parser)
{
	txNode* node = parser->root;
	parser->root = node->next;
	node->next = NULL;
	parser->nodeCount--;
	return node;
}

void fxPushNode(txParser* parser, txNode* node)
{
	node->next = parser->root;
	parser->root = (txNode*)node;
	parser->nodeCount++;
}

void fxPushBigIntNode(txParser* parser, txBigInt* value, txInteger line)
{
	txBigIntNode* node = fxNewParserChunk(parser, sizeof(txBigIntNode));
	node->description = &gxTokenDescriptions[XS_TOKEN_BIGINT];
	node->path = parser->path;
	node->line = line;
	node->flags = 0;
	node->value = *value;
	fxPushNode(parser, (txNode*)node);
}

void fxPushIndexNode(txParser* parser, txIndex value, txInteger line)
{
	if (((txInteger)value) >= 0)
		fxPushIntegerNode(parser, (txInteger)value, line);
	else
		fxPushNumberNode(parser, value, line);
}

void fxPushIntegerNode(txParser* parser, txInteger value, txInteger line)
{
	txIntegerNode* node = fxNewParserChunk(parser, sizeof(txIntegerNode));
	node->description = &gxTokenDescriptions[XS_TOKEN_INTEGER];
	node->path = parser->path;
	node->line = line;
	node->flags = 0;
	node->value = value;
	fxPushNode(parser, (txNode*)node);
}

void fxPushNodeStruct(txParser* parser, txInteger count, txToken token, txInteger line)
{
	const txNodeDescription* description = &gxTokenDescriptions[token];
	txNode* node;
	if ((count > parser->nodeCount) || ((sizeof(txNode) + (count * sizeof(txNode*))) > (size_t)(description->size))) {
		fxReportParserError(parser, parser->line, "invalid %s", gxTokenNames[token]);
	}
    node = fxNewParserChunkClear(parser, description->size);
	node->description = description;
	node->flags |= parser->flags & (mxStrictFlag | mxGeneratorFlag | mxAsyncFlag);
	node->path = parser->path;
	node->line = line;
    parser->nodeCount -= count;
	if (count) {
		txNode** dst = (txNode**)&node[1];
		txNode* src = parser->root;
		while (count) {
			txNode* next = src->next;
			src->next = NULL;
			count--;
			if (src->description)
				dst[count] = src;
			else if (src->path)
				dst[count] = (txNode*)(src->path);
			src = next;
		}
		parser->root = src;
	}
	fxPushNode(parser, node);
}

void fxPushNodeList(txParser* parser, txInteger count)
{
	txNodeList* list = fxNewParserChunk(parser, sizeof(txNodeList));
	txNode* previous = NULL;
	txNode* current = parser->root;
	txNode* next;
    parser->nodeCount -= count;
	list->length = count;
	while (count) {
		next = current->next;
		current->next = previous;
		previous = current;
		current = next;
		count--;
	}
	parser->root = current;
	list->description = &gxTokenDescriptions[XS_NO_TOKEN];
	list->first = previous;
	fxPushNode(parser, (txNode*)list);
}

void fxPushNULL(txParser* parser)
{
    txNodeLink* node = fxNewParserChunkClear(parser, sizeof(txNodeLink));
	fxPushNode(parser, (txNode*)node);
}

void fxPushNumberNode(txParser* parser, txNumber value, txInteger line)
{
	txNumberNode* node = fxNewParserChunk(parser, sizeof(txNumberNode));
	node->description = &gxTokenDescriptions[XS_TOKEN_NUMBER];
	node->path = parser->path;
	node->line = line;
	node->flags = 0;
	node->value = value;
	fxPushNode(parser, (txNode*)node);
}

void fxPushRawNode(txParser* parser, txInteger length, txString value, txInteger line)
{
	txStringNode* node = fxNewParserChunk(parser, sizeof(txStringNode));
	node->description = &gxTokenDescriptions[XS_TOKEN_STRING];
	node->path = parser->path;
	node->line = line;
	node->flags = 0;
	node->length = length;
	node->value = value;
	fxPushNode(parser, (txNode*)node);
}

void fxPushStringNode(txParser* parser, txInteger length, txString value, txInteger line)
{
	txStringNode* node = fxNewParserChunk(parser, sizeof(txStringNode));
	node->description = &gxTokenDescriptions[XS_TOKEN_STRING];
	node->path = parser->path;
	node->line = line;
	node->flags = parser->escaped;
	node->length = length;
	node->value = value;
	fxPushNode(parser, (txNode*)node);
}

void fxPushSymbol(txParser* parser, txSymbol* symbol)
{
    txNodeLink* node = fxNewParserChunkClear(parser, sizeof(txNodeLink));
	node->symbol = symbol;	
	fxPushNode(parser, (txNode*)node);
}

void fxSwapNodes(txParser* parser)
{
	txNode* previous = parser->root;
	txNode* current = previous->next;
	previous->next = current->next;
	current->next = previous;
	parser->root = current;
}

void fxModule(txParser* parser)
{
	txInteger aCount = parser->nodeCount;
	txInteger aLine = parser->line;
	while ((parser->token != XS_TOKEN_EOF)) {
		if (parser->token == XS_TOKEN_EXPORT)
			fxExport(parser);
		else if (parser->token == XS_TOKEN_IMPORT) {
			fxGetNextToken2(parser);
			if ((parser->token2 == XS_TOKEN_DOT) || (parser->token2 == XS_TOKEN_LEFT_PARENTHESIS))
				fxStatement(parser, 1);
			else
				fxImport(parser);
		}
		else if (parser->token == XS_TOKEN_RETURN) {
			fxReportParserError(parser, parser->line, "invalid return");
			fxGetNextToken(parser);
		}
		else if (parser->token == XS_TOKEN_YIELD) {
			fxReportParserError(parser, parser->line, "invalid yield");
			fxGetNextToken(parser);
		}
		else {
			fxStatement(parser, 1);
		}
	}
	aCount = parser->nodeCount - aCount;
	if (aCount > 1) {
		fxPushNodeList(parser, aCount);
		fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENTS, aLine);
	}
	else if (aCount == 0) {
		fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aLine);
		fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, aLine);
	}
	fxPushNodeStruct(parser, 1, XS_TOKEN_MODULE, aLine);
	parser->root->flags = parser->flags & (mxStrictFlag | mxAwaitingFlag);
}

void fxExport(txParser* parser)
{
	txSymbol* symbol = C_NULL;
	txInteger count;
	txInteger line = parser->line;
	txUnsigned flag = 0;
	txToken aToken;
	fxMatchToken(parser, XS_TOKEN_EXPORT);
	switch (parser->token) {
	case XS_TOKEN_MULTIPLY:
		fxPushNULL(parser);
		fxGetNextToken(parser);
		if (fxIsKeyword(parser, parser->asSymbol)) {
			fxGetNextToken(parser);
			if (gxTokenFlags[parser->token] & XS_TOKEN_IDENTIFIER_NAME) {
				fxPushSymbol(parser, parser->symbol);
				fxGetNextToken(parser);
			}
			else {
				fxPushNULL(parser);
				fxReportParserError(parser, parser->line, "missing identifier");
			}
		}
		else {
			fxPushNULL(parser);
		}
		if (fxIsKeyword(parser, parser->fromSymbol)) {
			fxGetNextToken(parser);
			if (parser->token == XS_TOKEN_STRING) {
				fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, line);
				fxPushNodeList(parser, 1);
				fxPushStringNode(parser, parser->stringLength, parser->string, line);
				fxPushNodeStruct(parser, 2, XS_TOKEN_EXPORT, line);
				fxGetNextToken(parser);
				fxSemicolon(parser);
			}
			else
				fxReportParserError(parser, parser->line, "missing module");
		}
		else
			fxReportParserError(parser, parser->line, "missing from");
		break;
	case XS_TOKEN_DEFAULT:
		fxMatchToken(parser, XS_TOKEN_DEFAULT);
		if (parser->flags & mxDefaultFlag)
			fxReportParserError(parser, parser->line, "invalid default");
		parser->flags |= mxDefaultFlag;
		if (parser->token == XS_TOKEN_CLASS) {
			fxClassExpression(parser, line, &symbol);
			if (symbol)
				fxPushSymbol(parser, symbol);
			else
				fxPushSymbol(parser, parser->defaultSymbol);
			fxPushNodeStruct(parser, 1, XS_TOKEN_LET, line);
			fxSwapNodes(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_BINDING, line);
		}
		else if (parser->token == XS_TOKEN_FUNCTION) {
	again:
			fxMatchToken(parser, XS_TOKEN_FUNCTION);
			if (parser->token == XS_TOKEN_MULTIPLY) {
				fxGetNextToken(parser);
				fxGeneratorExpression(parser, line, &symbol, flag);
			}
			else
				fxFunctionExpression(parser, line, &symbol, flag);
			if (symbol) {
				txDefineNode* node = fxDefineNodeNew(parser, XS_TOKEN_DEFINE, symbol);
				node->initializer = fxPopNode(parser);
				fxPushNode(parser, (txNode*)node);
			}
			else {
				txDefineNode* node = fxDefineNodeNew(parser, XS_TOKEN_DEFINE, parser->defaultSymbol);
				node->initializer = fxPopNode(parser);
				fxPushNode(parser, (txNode*)node);
			}
		}
		else {
			if ((parser->token == XS_TOKEN_IDENTIFIER) && (parser->symbol == parser->asyncSymbol) && (!parser->escaped)) {
				fxGetNextToken2(parser);
				if ((!parser->crlf2) && (parser->token2 == XS_TOKEN_FUNCTION)) {
					fxGetNextToken(parser);
					flag = mxAsyncFlag;
					goto again;					
				}
			}
			fxAssignmentExpression(parser);
			fxSemicolon(parser);
			fxPushSymbol(parser, parser->defaultSymbol);
			fxPushNULL(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_CONST, line);
			fxSwapNodes(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_ASSIGN, line);
			fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, line);
		}
		if (symbol) {
			fxPushSymbol(parser, symbol);
			fxPushSymbol(parser, parser->defaultSymbol);
		}
		else {
			fxPushSymbol(parser, parser->defaultSymbol);
			fxPushNULL(parser);
		}
		fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, line);
		fxPushNodeList(parser, 1);
		fxPushNULL(parser);
		fxPushNodeStruct(parser, 2, XS_TOKEN_EXPORT, line);
		break;
	case XS_TOKEN_CLASS:
		fxClassExpression(parser, line, &symbol);
		if (symbol) {
			fxPushSymbol(parser, symbol);
			fxPushNodeStruct(parser, 1, XS_TOKEN_LET, line);
			fxSwapNodes(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_BINDING, line);
			
			fxPushSymbol(parser, symbol);
			fxPushNULL(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, line);
			fxPushNodeList(parser, 1);
			fxPushNULL(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_EXPORT, line);
		}
		else
			fxReportParserError(parser, parser->line, "missing identifier");
		break;
	case XS_TOKEN_FUNCTION:
	again2:
		fxMatchToken(parser, XS_TOKEN_FUNCTION);
		if (parser->token == XS_TOKEN_MULTIPLY) {
			fxGetNextToken(parser);
			fxGeneratorExpression(parser, line, &symbol, flag);
		}
		else
			fxFunctionExpression(parser, line, &symbol, flag);
		if (symbol) {
			txDefineNode* node = fxDefineNodeNew(parser, XS_TOKEN_DEFINE, symbol);
			node->initializer = fxPopNode(parser);
			fxPushNode(parser, (txNode*)node);
			fxPushSymbol(parser, symbol);
			fxPushNULL(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, line);
			fxPushNodeList(parser, 1);
			fxPushNULL(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_EXPORT, line);
		}
		else
			fxReportParserError(parser, parser->line, "missing identifier");
		break;
	case XS_TOKEN_CONST:
	case XS_TOKEN_LET:
	case XS_TOKEN_VAR:
		aToken = parser->token;
		fxVariableStatement(parser, aToken);
		count = parser->nodeCount;
		fxExportBinding(parser, parser->root);
		fxPushNodeList(parser, parser->nodeCount - count);
		fxPushNULL(parser);
		fxPushNodeStruct(parser, 2, XS_TOKEN_EXPORT, line);
		fxSemicolon(parser);
		break;
	case XS_TOKEN_LEFT_BRACE:
		count = parser->nodeCount;
		fxSpecifiers(parser);
		fxPushNodeList(parser, parser->nodeCount - count);
		if (fxIsKeyword(parser, parser->fromSymbol)) {
			fxGetNextToken(parser);
			if (parser->token == XS_TOKEN_STRING) {
				fxPushStringNode(parser, parser->stringLength, parser->string, line);
				fxGetNextToken(parser);
			}
			else {
				fxPushNULL(parser);
				fxReportParserError(parser, parser->line, "missing module");
			}
		}
		else
			fxPushNULL(parser);
		fxPushNodeStruct(parser, 2, XS_TOKEN_EXPORT, line);
		fxSemicolon(parser);
		break;
	default:
		if ((parser->token == XS_TOKEN_IDENTIFIER) && (parser->symbol == parser->asyncSymbol) && (!parser->escaped)) {
			fxGetNextToken2(parser);
			if ((!parser->crlf2) && (parser->token2 == XS_TOKEN_FUNCTION)) {
				fxGetNextToken(parser);
				flag = mxAsyncFlag;
				goto again2;					
			}
		}
		fxReportParserError(parser, parser->line, "invalid export %s", gxTokenNames[parser->token]);
		fxGetNextToken(parser);
		break;
	}
}

void fxExportBinding(txParser* parser, txNode* node)
{
	txToken token = node->description->token;
	if ((token == XS_TOKEN_CONST) || (token == XS_TOKEN_LET) || (token == XS_TOKEN_VAR)) {
		fxPushSymbol(parser, ((txDeclareNode*)node)->symbol);
		fxPushNULL(parser);
		fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, node->line);
	}
	else if (token == XS_TOKEN_BINDING) {
		fxExportBinding(parser, ((txBindingNode*)node)->target);
	}
	else if (token == XS_TOKEN_ARRAY_BINDING) {
		node = ((txArrayBindingNode*)node)->items->first;
		while (node) {
			fxExportBinding(parser, node);
			node = node->next;
		}
	}
	else if (token == XS_TOKEN_OBJECT_BINDING) {
		node = ((txObjectBindingNode*)node)->items->first;
		while (node) {
			fxExportBinding(parser, node);
			node = node->next;
		}
	}
	else if (token == XS_TOKEN_PROPERTY_BINDING)
		fxExportBinding(parser, ((txPropertyBindingNode*)node)->binding);
	else if (token == XS_TOKEN_PROPERTY_BINDING_AT)
		fxExportBinding(parser, ((txPropertyBindingAtNode*)node)->binding);
	else if (token == XS_TOKEN_REST_BINDING)
		fxExportBinding(parser, ((txRestBindingNode*)node)->binding);
	else if (token == XS_TOKEN_STATEMENTS) {
		node = ((txStatementsNode*)node)->items->first;
		while (node) {
			fxExportBinding(parser, node);
			node = node->next;
		}
	}
}


void fxImport(txParser* parser)
{
	txBoolean asFlag = 1;
	txBoolean fromFlag = 0;
	txInteger count = parser->nodeCount;
	fxMatchToken(parser, XS_TOKEN_IMPORT);
	if (parser->token == XS_TOKEN_IDENTIFIER) {
		fxPushSymbol(parser, parser->defaultSymbol);
		fxPushSymbol(parser, parser->symbol);
		fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, parser->line);
		fxGetNextToken(parser);
		if (parser->token == XS_TOKEN_COMMA)
			fxGetNextToken(parser);
		else
			asFlag = 0;
		fromFlag = 1;
	}
	if (asFlag) {
		if (parser->token == XS_TOKEN_MULTIPLY) {
			fxGetNextToken(parser);
			if (fxIsKeyword(parser, parser->asSymbol)) {
				fxGetNextToken(parser);
				if (parser->token == XS_TOKEN_IDENTIFIER) {
					fxPushNULL(parser);
					fxPushSymbol(parser, parser->symbol);
					fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, parser->line);
					fxGetNextToken(parser);
				}
				else {
					fxReportParserError(parser, parser->line, "missing identifier");
				}
			}
			else {
				fxReportParserError(parser, parser->line, "missing as");
			}
			fromFlag = 1;
		}
		else if (parser->token == XS_TOKEN_LEFT_BRACE) {
			fxSpecifiers(parser);
			fromFlag = 1;
		}
	}
	fxPushNodeList(parser, parser->nodeCount - count);
	if (fromFlag) {
		if (fxIsKeyword(parser, parser->fromSymbol)) {
			fxGetNextToken(parser);
			if (parser->token == XS_TOKEN_STRING) {
				fxPushStringNode(parser, parser->stringLength, parser->string, parser->line);
				fxGetNextToken(parser);
			}
			else {
				fxPushNULL(parser);
				fxReportParserError(parser, parser->line, "missing module");
			}
		}
		else {
			fxPushNULL(parser);
			fxReportParserError(parser, parser->line, "missing from");
		}
	}
	else if (parser->token == XS_TOKEN_STRING) {
		fxPushStringNode(parser, parser->stringLength, parser->string, parser->line);
		fxGetNextToken(parser);
	}
	else {
		fxPushNULL(parser);
		fxReportParserError(parser, parser->line, "missing module");
	}
	fxPushNodeStruct(parser, 2, XS_TOKEN_IMPORT, parser->line);
	fxSemicolon(parser);
}

void fxSpecifiers(txParser* parser)
{
	txInteger aCount = 0;
	fxMatchToken(parser, XS_TOKEN_LEFT_BRACE);
	while (gxTokenFlags[parser->token] & XS_TOKEN_IDENTIFIER_NAME) {
		fxPushSymbol(parser, parser->symbol);
		fxGetNextToken(parser);
		if (fxIsKeyword(parser, parser->asSymbol)) {
			fxGetNextToken(parser);
			if (gxTokenFlags[parser->token] & XS_TOKEN_IDENTIFIER_NAME) {
				fxPushSymbol(parser, parser->symbol);
				fxGetNextToken(parser);
			}
			else {
				fxPushNULL(parser);
				fxReportParserError(parser, parser->line, "missing identifier");
			}
		}
		else
			fxPushNULL(parser);
		fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, parser->line);
		aCount++;
		if (parser->token != XS_TOKEN_COMMA) 
			break;
		fxGetNextToken(parser);
	}
	fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
}

void fxProgram(txParser* parser)
{
	txInteger count = parser->nodeCount;
	txInteger line = parser->line;
	txNode* node;
	while (parser->token != XS_TOKEN_EOF) {
		fxStatement(parser, 1);
		node = parser->root;
		if (!node || !node->description || (node->description->token != XS_TOKEN_STATEMENT))
			break;
		node = ((txStatementNode*)node)->expression;
		if (!node || !node->description || (node->description->token != XS_TOKEN_STRING))
			break;
		if (!(node->flags & mxStringEscapeFlag) && (c_strcmp(((txStringNode*)node)->value, "use strict") == 0)) {
			if (!(parser->flags & mxStrictFlag)) {
				parser->flags |= mxStrictFlag;
				if (parser->token == XS_TOKEN_IDENTIFIER)
					fxCheckStrictKeyword(parser);
			}
		}
	}
	while (parser->token != XS_TOKEN_EOF) {
		fxStatement(parser, 1);
	}
	count = parser->nodeCount - count;
	if (count > 1) {
		fxPushNodeList(parser, count);
		fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENTS, line);
	}
	else if (count == 0) {
		fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, line);
		fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, line);
	}
	fxPushNodeStruct(parser, 1, XS_TOKEN_PROGRAM, line);
	if (parser->flags & mxFieldFlag)
		if (parser->flags & mxArgumentsFlag)
			fxReportParserError(parser, parser->line, "invalid arguments");
	parser->root->flags = parser->flags & mxStrictFlag;
}

void fxBody(txParser* parser)
{
	txInteger count = parser->nodeCount;
	txInteger line = parser->line;
	txNode* node;
    fxCheckParserStack(parser, line);
	while ((parser->token != XS_TOKEN_EOF) && (parser->token != XS_TOKEN_RIGHT_BRACE)) {
		fxStatement(parser, 1);
		node = parser->root;
		if (!node || !node->description || (node->description->token != XS_TOKEN_STATEMENT))
			break;
		node = ((txStatementNode*)node)->expression;
		if (!node || !node->description || (node->description->token != XS_TOKEN_STRING))
			break;
		if (!(node->flags & mxStringEscapeFlag) && (c_strcmp(((txStringNode*)node)->value, "use strict") == 0)) {
			if (parser->flags & mxNotSimpleParametersFlag)
				fxReportParserError(parser, parser->line, "invalid directive");
			if (!(parser->flags & mxStrictFlag)) {
				parser->flags |= mxStrictFlag;
				if (parser->token == XS_TOKEN_IDENTIFIER)
					fxCheckStrictKeyword(parser);
			}
		}
	}
	while ((parser->token != XS_TOKEN_EOF) && (parser->token != XS_TOKEN_RIGHT_BRACE)) {
		fxStatement(parser, 1);
	}
	count = parser->nodeCount - count;
	if (count > 1) {
		fxPushNodeList(parser, count);
		fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENTS, line);
	}
	else if (count == 0) {
		fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, line);
		fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, line);
	}
}

void fxBlock(txParser* parser)
{
	txInteger aLine = parser->line;
	fxCheckParserStack(parser, aLine);
	fxMatchToken(parser, XS_TOKEN_LEFT_BRACE);
	fxStatements(parser);
	fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
	fxPushNodeStruct(parser, 1, XS_TOKEN_BLOCK, aLine);
}

void fxStatements(txParser* parser)
{
	txInteger count = parser->nodeCount;
	txInteger line = parser->line;
	while ((parser->token != XS_TOKEN_EOF) && (parser->token != XS_TOKEN_RIGHT_BRACE)) {
		fxStatement(parser, 1);
	}
	count = parser->nodeCount - count;
	fxPushNodeList(parser, count);
	fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENTS, line);
}

void fxStatement(txParser* parser, txBoolean blockIt)
{
	txInteger line = parser->line;
	txSymbol* symbol = C_NULL;
	txUnsigned flag = 0;
	switch (parser->token) {
//	case XS_TOKEN_COMMA:
	case XS_TOKEN_SEMICOLON:
		fxGetNextToken(parser);
		if (!blockIt) {
			fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, line);
			fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, line);
		}
		break;
	case XS_TOKEN_BREAK:
		fxBreakStatement(parser);
		fxSemicolon(parser);
		break;
	case XS_TOKEN_CLASS:
		if (!blockIt)
			fxReportParserError(parser, parser->line, "no block");
		fxClassExpression(parser, line, &symbol);
		if (symbol) {
			fxPushSymbol(parser, symbol);
			fxPushNodeStruct(parser, 1, XS_TOKEN_LET, line);
			fxSwapNodes(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_BINDING, line);
		}
		else
			fxReportParserError(parser, parser->line, "missing identifier");
		break;
	case XS_TOKEN_CONST:
		if (!blockIt)
			fxReportParserError(parser, parser->line, "no block");
		fxVariableStatement(parser, XS_TOKEN_CONST);
		fxSemicolon(parser);
		break;
	case XS_TOKEN_CONTINUE:
		fxContinueStatement(parser);
		fxSemicolon(parser);
		break;
	case XS_TOKEN_DEBUGGER:
		fxDebuggerStatement(parser);
		fxSemicolon(parser);
		break;
	case XS_TOKEN_DO:
		fxDoStatement(parser);
		break;
	case XS_TOKEN_FOR:
		fxForStatement(parser);
		break;
	case XS_TOKEN_FUNCTION:
		//if ((parser->flags & mxStrictFlag) && !blockIt) BROWSER
	again:
		if (!blockIt)
			fxReportParserError(parser, parser->line, "no block (strict code)");
		fxMatchToken(parser, XS_TOKEN_FUNCTION);
		if (parser->token == XS_TOKEN_MULTIPLY) {
			fxGetNextToken(parser);
			fxGeneratorExpression(parser, line, &symbol, flag);
		}
		else
			fxFunctionExpression(parser, line, &symbol, flag);
		if (symbol) {
			txDefineNode* node = fxDefineNodeNew(parser, XS_TOKEN_DEFINE, symbol);
			node->initializer = fxPopNode(parser);
			fxPushNode(parser, (txNode*)node);
		}
		else
			fxReportParserError(parser, parser->line, "missing identifier");
		break;
	case XS_TOKEN_IF:
		fxIfStatement(parser);
		break;
	case XS_TOKEN_RETURN:
		if (!(parser->flags & (mxArrowFlag | mxFunctionFlag | mxGeneratorFlag)))
			fxReportParserError(parser, parser->line, "invalid return");
		fxReturnStatement(parser);
		fxSemicolon(parser);
		break;
	case XS_TOKEN_LEFT_BRACE:
		fxBlock(parser);
		break;
	case XS_TOKEN_LET:
		if (!blockIt)
			fxReportParserError(parser, parser->line, "no block");
		fxVariableStatement(parser, XS_TOKEN_LET);
		fxSemicolon(parser);
		break;
	case XS_TOKEN_SWITCH:
		fxSwitchStatement(parser);
		break;
	case XS_TOKEN_THROW:
		fxThrowStatement(parser);
		fxSemicolon(parser);
		break;
	case XS_TOKEN_TRY:
		fxTryStatement(parser);
		break;
	case XS_TOKEN_VAR:
		fxVariableStatement(parser, XS_TOKEN_VAR);
		fxSemicolon(parser);
		break;
	case XS_TOKEN_WHILE:
		fxWhileStatement(parser);
		break;
	case XS_TOKEN_WITH:
		if (parser->flags & mxStrictFlag)
			fxReportParserError(parser, parser->line, "with (strict code)");
		fxWithStatement(parser);
		break;
	case XS_TOKEN_IDENTIFIER:
		fxGetNextToken2(parser);
		if (parser->token2 == XS_TOKEN_COLON) {
			fxPushSymbol(parser, parser->symbol);
			fxGetNextToken(parser);
			fxMatchToken(parser, XS_TOKEN_COLON);
			//if ((parser->flags & mxStrictFlag) && (parser->token == XS_TOKEN_FUNCTION)) BROWSER
			//	fxReportParserError(parser, parser->line, "labeled function (strict code)");
			if (parser->token == XS_TOKEN_FUNCTION)
				fxReportParserError(parser, parser->line, "labeled function");
			fxCheckParserStack(parser, line);
			fxStatement(parser, 0);
			fxPushNodeStruct(parser, 2, XS_TOKEN_LABEL, line);
			break;
		}
		if ((parser->symbol == parser->asyncSymbol) && (!parser->escaped) 
				&& (!parser->crlf2) && (parser->token2 == XS_TOKEN_FUNCTION)) {
			fxGetNextToken(parser);
			flag = mxAsyncFlag;
			goto again;
		}
		if ((parser->symbol == parser->letSymbol) && (!parser->escaped) 
				&& ((gxTokenFlags[parser->token2] & XS_TOKEN_BEGIN_BINDING) || (parser->token2 == XS_TOKEN_AWAIT) || (parser->token2 == XS_TOKEN_YIELD))
				&& (blockIt || (!parser->crlf2) || (parser->token2 == XS_TOKEN_LEFT_BRACKET))) {
			parser->token = XS_TOKEN_LET;
			if (!blockIt)
				fxReportParserError(parser, parser->line, "no block");
			fxVariableStatement(parser, XS_TOKEN_LET);
			fxSemicolon(parser);
			break;
		}
		/* continue */
	default:
		if (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
			fxCommaExpression(parser);
			fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, line);
			fxSemicolon(parser);
		}
		else {
			fxReportParserError(parser, parser->line, "invalid token %s", gxTokenNames[parser->token]);
			fxPushNULL(parser);
			fxGetNextToken(parser);
		}
		break;
	}
}

void fxSemicolon(txParser* parser)
{
	if ((parser->crlf) || (gxTokenFlags[parser->token] & XS_TOKEN_END_STATEMENT)) {
		if (parser->token == XS_TOKEN_SEMICOLON)
			fxGetNextToken(parser);
	}
	else
		fxReportParserError(parser, parser->line, "missing ;");
}

void fxBreakStatement(txParser* parser)
{
	txInteger aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_BREAK);
	if ((!parser->crlf) && (parser->token == XS_TOKEN_IDENTIFIER)) {
		fxPushSymbol(parser, parser->symbol);
		fxGetNextToken(parser);
	}
	else {
		fxPushNULL(parser);
	}
	fxPushNodeStruct(parser, 1, XS_TOKEN_BREAK, aLine);
}

void fxContinueStatement(txParser* parser)
{
	txInteger aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_CONTINUE);
	if ((!parser->crlf) && (parser->token == XS_TOKEN_IDENTIFIER)) {
		fxPushSymbol(parser, parser->symbol);
		fxGetNextToken(parser);
	}
	else {
		fxPushNULL(parser);
	}
	fxPushNodeStruct(parser, 1, XS_TOKEN_CONTINUE, aLine);
}

void fxDebuggerStatement(txParser* parser)
{
	txInteger aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_DEBUGGER);
	fxPushNodeStruct(parser, 0, XS_TOKEN_DEBUGGER, aLine);
}

void fxDoStatement(txParser* parser)
{
	txInteger aLine = parser->line;
	fxCheckParserStack(parser, aLine);
	fxPushNULL(parser);
	fxMatchToken(parser, XS_TOKEN_DO);
	fxStatement(parser, 0);
	fxMatchToken(parser, XS_TOKEN_WHILE);
	fxMatchToken(parser, XS_TOKEN_LEFT_PARENTHESIS);
	fxCommaExpression(parser);
	fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
	if (parser->token == XS_TOKEN_SEMICOLON)
		fxGetNextToken(parser);
	fxPushNodeStruct(parser, 2, XS_TOKEN_DO, aLine);
	fxPushNodeStruct(parser, 2, XS_TOKEN_LABEL, aLine);
}

void fxForStatement(txParser* parser)
{
	txInteger aLine = parser->line;
	txBoolean awaitFlag = 0;
	txBoolean expressionFlag = 0;
	txToken aToken;
	fxPushNULL(parser);
	fxMatchToken(parser, XS_TOKEN_FOR);
	if (parser->token == XS_TOKEN_AWAIT) {
		awaitFlag = 1;
		fxMatchToken(parser, XS_TOKEN_AWAIT);
	}
	fxMatchToken(parser, XS_TOKEN_LEFT_PARENTHESIS);
	fxGetNextToken2(parser);
	parser->flags |= mxForFlag;
	if (parser->token == XS_TOKEN_SEMICOLON) {
		fxPushNULL(parser);
	}
	else if (parser->token == XS_TOKEN_CONST) {
		fxVariableStatement(parser, XS_TOKEN_CONST);
	}
	else if (parser->token == XS_TOKEN_LET) {
		fxVariableStatement(parser, XS_TOKEN_LET);
	}
	else if (fxIsKeyword(parser, parser->letSymbol) && (gxTokenFlags[parser->token2] & XS_TOKEN_BEGIN_BINDING)) {
		parser->token = XS_TOKEN_LET;
		fxVariableStatement(parser, XS_TOKEN_LET);
	}
	else if (parser->token == XS_TOKEN_VAR) {
		fxVariableStatement(parser, XS_TOKEN_VAR);
	}
	else {
		fxCommaExpression(parser);
		expressionFlag = 1;
	}
	parser->flags &= ~mxForFlag;
	if (awaitFlag && !fxIsKeyword(parser, parser->ofSymbol))
		fxReportParserError(parser, parser->line, "invalid for await");
	if (fxIsToken(parser, XS_TOKEN_IN) || fxIsKeyword(parser, parser->ofSymbol)) {
		if (expressionFlag) {
			if (!fxCheckReference(parser, XS_TOKEN_ASSIGN)) {
				fxReportParserError(parser, parser->line, "no reference");
			}
		}
		else {
			aToken = parser->root->description->token;
			if (aToken == XS_TOKEN_BINDING) {
				if (((txBindingNode*)(parser->root))->initializer)
					fxReportParserError(parser, parser->line, "invalid binding initializer");
			}
// 			else if (aToken == XS_TOKEN_ARRAY_BINDING) {
// 				if (((txArrayBindingNode*)(parser->root))->initializer)
// 					fxReportParserError(parser, parser->line, "invalid array binding initializer");
// 			}
// 			else if (aToken == XS_TOKEN_OBJECT_BINDING) {
// 				if (((txObjectBindingNode*)(parser->root))->initializer)
// 					fxReportParserError(parser, parser->line, "invalid object binding initializer");
// 			}
// 			else
// 				fxReportParserError(parser, parser->line, "no reference %s", gxTokenNames[aToken]);
		}
		aToken = parser->token;
		fxGetNextToken(parser);
		if (aToken == XS_TOKEN_IN)
			fxCommaExpression(parser);
		else
			fxAssignmentExpression(parser);
		fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
		fxStatement(parser, 0);
		if (awaitFlag)
			fxPushNodeStruct(parser, 3, XS_TOKEN_FOR_AWAIT_OF, aLine);
		else if (aToken == XS_TOKEN_IN)
			fxPushNodeStruct(parser, 3, XS_TOKEN_FOR_IN, aLine);
		else
			fxPushNodeStruct(parser, 3, XS_TOKEN_FOR_OF, aLine);
	}
	else {
		if (expressionFlag)
			fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, aLine);
		fxMatchToken(parser, XS_TOKEN_SEMICOLON);
		if (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
			fxCommaExpression(parser);
		}
		else {
			fxPushNULL(parser);
		}
		fxMatchToken(parser, XS_TOKEN_SEMICOLON);
		if (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
			fxCommaExpression(parser);
		}
		else {
			fxPushNULL(parser);
		}
		fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
		fxCheckParserStack(parser, aLine);
		fxStatement(parser, 0);
		fxPushNodeStruct(parser, 4, XS_TOKEN_FOR, aLine);
	}
	fxPushNodeStruct(parser, 2, XS_TOKEN_LABEL, aLine);
}

void fxIfStatement(txParser* parser)
{
	txInteger aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_IF);
	fxMatchToken(parser, XS_TOKEN_LEFT_PARENTHESIS);
	fxCommaExpression(parser);
	fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
	fxStatement(parser, 0);
	if (parser->token == XS_TOKEN_ELSE) {
		fxMatchToken(parser, XS_TOKEN_ELSE);
		fxStatement(parser, 0);
	}
	else {
		fxPushNULL(parser);
	}
	fxPushNodeStruct(parser, 3, XS_TOKEN_IF, aLine);
}

void fxReturnStatement(txParser* parser)
{
	txInteger aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_RETURN);
	if ((!parser->crlf) && (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION)) {
		fxCommaExpression(parser);
	}
	else {
		fxPushNULL(parser);
	}
	fxPushNodeStruct(parser, 1, XS_TOKEN_RETURN, aLine);
}

void fxSwitchStatement(txParser* parser)
{
	txInteger aCount = 0;
	txBoolean aDefaultFlag = 0;
	txInteger aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_SWITCH);
	fxMatchToken(parser, XS_TOKEN_LEFT_PARENTHESIS);
	fxCommaExpression(parser);
	fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
	fxMatchToken(parser, XS_TOKEN_LEFT_BRACE);
	while ((parser->token == XS_TOKEN_CASE) || (parser->token == XS_TOKEN_DEFAULT)) {
		txInteger aCaseCount;
		txInteger aCaseLine = parser->line;
		if (parser->token == XS_TOKEN_CASE) {
			fxMatchToken(parser, XS_TOKEN_CASE);
			fxCommaExpression(parser);
			fxMatchToken(parser, XS_TOKEN_COLON);
			aCaseCount = parser->nodeCount;
			while (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_STATEMENT)
				fxStatement(parser, 1);
			aCaseCount = parser->nodeCount - aCaseCount;
			if (aCaseCount > 1) {
				fxPushNodeList(parser, aCaseCount);
				fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENTS, aCaseLine);
			}
			else if (aCaseCount == 0) {
				fxPushNULL(parser);
			}
			fxPushNodeStruct(parser, 2, XS_TOKEN_CASE, aCaseLine);
		}
		else {
			fxMatchToken(parser, XS_TOKEN_DEFAULT);
			if (aDefaultFlag) 
				fxReportParserError(parser, parser->line, "invalid default");
			fxPushNULL(parser);
			fxMatchToken(parser, XS_TOKEN_COLON);
			aCaseCount = parser->nodeCount;
			while (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_STATEMENT)
				fxStatement(parser, 1);
			aCaseCount = parser->nodeCount - aCaseCount;
			if (aCaseCount > 1) {
				fxPushNodeList(parser, aCaseCount);
				fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENTS, aLine);
			}
			else if (aCaseCount == 0) {
				fxPushNULL(parser);
			}
			fxPushNodeStruct(parser, 2, XS_TOKEN_CASE, aCaseLine);
			aDefaultFlag = 1;
		}
		aCount++;
	}
	fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
	fxPushNodeList(parser, aCount);
	fxPushNodeStruct(parser, 2, XS_TOKEN_SWITCH, aLine);
}

void fxThrowStatement(txParser* parser)
{
	txInteger aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_THROW);
	if ((!parser->crlf) && (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION)) {
		fxCommaExpression(parser);
	}
	else {
		fxReportParserError(parser, parser->line, "missing expression");
		fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aLine);
	}
	fxPushNodeStruct(parser, 1, XS_TOKEN_THROW, aLine);
}

void fxTryStatement(txParser* parser)
{
	txInteger aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_TRY);
	fxBlock(parser);
	if (parser->token == XS_TOKEN_CATCH) {
		txInteger aCatchLine = parser->line;
		fxMatchToken(parser, XS_TOKEN_CATCH);
		if (parser->token == XS_TOKEN_LEFT_PARENTHESIS) {
			fxMatchToken(parser, XS_TOKEN_LEFT_PARENTHESIS);
			fxBinding(parser, XS_TOKEN_LET, 1);
			fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
		}
		else
			fxPushNULL(parser);
		fxMatchToken(parser, XS_TOKEN_LEFT_BRACE);
		fxStatements(parser);
		fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
		fxPushNodeStruct(parser, 2, XS_TOKEN_CATCH, aCatchLine);
	}
	else {
		fxPushNULL(parser);
	}
	if (parser->token == XS_TOKEN_FINALLY) {
		fxMatchToken(parser, XS_TOKEN_FINALLY);
		fxBlock(parser);
	}
	else {
		fxPushNULL(parser);
	}
	fxPushNodeStruct(parser, 3, XS_TOKEN_TRY, aLine);
}

void fxVariableStatement(txParser* parser, txToken theToken)
{
	txBoolean commaFlag = 0;
	txInteger aCount = 0;
	txInteger aLine = parser->line;
	fxMatchToken(parser, theToken);
	while (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_BINDING) {
		commaFlag = 0;
		fxBinding(parser, theToken, 1);
// 		if (parser->token == XS_TOKEN_ASSIGN) {
// 			parser->flags &= ~mxForFlag;
// 			fxGetNextToken(parser);
// 			fxAssignmentExpression(parser);
// 			fxPushNodeStruct(parser, 2, XS_TOKEN_ASSIGN, aLine);
// 			fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, aLine);
// 		}
		aCount++;
		if (parser->token == XS_TOKEN_COMMA) {
			parser->flags &= ~mxForFlag;
			fxGetNextToken(parser);
			commaFlag = 1;
		}
		else
			break;
	}
    if ((aCount == 0) || commaFlag) {
		fxPushNULL(parser);
		fxPushNULL(parser);
		fxPushNodeStruct(parser, 2, theToken, aLine);
		fxReportParserError(parser, parser->line, "missing identifier");
	}
	if (aCount > 1) {
		fxPushNodeList(parser, aCount);
		fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENTS, aLine);
	}
}

void fxWhileStatement(txParser* parser)
{
	txInteger aLine = parser->line;
	fxPushNULL(parser);
	fxMatchToken(parser, XS_TOKEN_WHILE);
	fxMatchToken(parser, XS_TOKEN_LEFT_PARENTHESIS);
	fxCommaExpression(parser);
	fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
	fxStatement(parser, 0);
	fxPushNodeStruct(parser, 2, XS_TOKEN_WHILE, aLine);
	fxPushNodeStruct(parser, 2, XS_TOKEN_LABEL, aLine);
}

void fxWithStatement(txParser* parser)
{
	txInteger aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_WITH);
	fxMatchToken(parser, XS_TOKEN_LEFT_PARENTHESIS);
	fxCommaExpression(parser);
	fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
	fxStatement(parser, 0);
	fxPushNodeStruct(parser, 2, XS_TOKEN_WITH, aLine);
}

void fxCommaExpression(txParser* parser)
{
	txInteger aCount = 0;
	txInteger aLine = parser->line;
	if (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
		fxAssignmentExpression(parser);
		aCount++;
		while (parser->token == XS_TOKEN_COMMA) {
			fxGetNextToken(parser);
			fxAssignmentExpression(parser);
			aCount++;
		}
	}
	if (aCount > 1) {
		fxPushNodeList(parser, aCount);
		fxPushNodeStruct(parser, 1, XS_TOKEN_EXPRESSIONS, aLine);
	}
	else if (aCount == 0) {
		fxPushNULL(parser);
		fxReportParserError(parser, parser->line, "missing expression");
	}
}

void fxAssignmentExpression(txParser* parser)
{
	if (parser->token == XS_TOKEN_YIELD)
		fxYieldExpression(parser);
	else {
		fxConditionalExpression(parser);
		while (gxTokenFlags[parser->token] & XS_TOKEN_ASSIGN_EXPRESSION) {
			txToken aToken = parser->token;
			txInteger aLine = parser->line;
			if (!fxCheckReference(parser, aToken)) 
				fxReportParserError(parser, parser->line, "no reference");
			fxGetNextToken(parser);
			fxAssignmentExpression(parser);
			fxPushNodeStruct(parser, 2, aToken, aLine);
		}
	}
}

void fxConditionalExpression(txParser* parser)
{
	fxCoalesceExpression(parser);
	if (parser->token == XS_TOKEN_QUESTION_MARK) {
		txInteger aLine = parser->line;
		txUnsigned flags;
		fxCheckArrowFunction(parser, 1);
		fxGetNextToken(parser);
		flags = parser->flags & mxForFlag;
		parser->flags &= ~mxForFlag;
		fxAssignmentExpression(parser);
		parser->flags |= flags;
		fxMatchToken(parser, XS_TOKEN_COLON);
		fxAssignmentExpression(parser);
		fxPushNodeStruct(parser, 3, XS_TOKEN_QUESTION_MARK, aLine);
	}
}

void fxCoalesceExpression(txParser* parser)
{
	fxOrExpression(parser);
	while (parser->token == XS_TOKEN_COALESCE) {
		txInteger aLine = parser->line;
		fxGetNextToken(parser);
		fxOrExpression(parser);
		fxCheckArrowFunction(parser, 2);
		fxPushNodeStruct(parser, 2, XS_TOKEN_COALESCE, aLine);
	}
}

void fxOrExpression(txParser* parser)
{
	fxAndExpression(parser);
	while (parser->token == XS_TOKEN_OR) {
		txInteger aLine = parser->line;
		fxGetNextToken(parser);
		fxAndExpression(parser);
		fxCheckArrowFunction(parser, 2);
		fxPushNodeStruct(parser, 2, XS_TOKEN_OR, aLine);
	}
}

void fxAndExpression(txParser* parser)
{
	fxBitOrExpression(parser);
	while (parser->token == XS_TOKEN_AND) {
		txInteger aLine = parser->line;
		fxGetNextToken(parser);
		fxBitOrExpression(parser);
		fxCheckArrowFunction(parser, 2);
		fxPushNodeStruct(parser, 2, XS_TOKEN_AND, aLine);
	}
}

void fxBitOrExpression(txParser* parser)
{
	fxBitXorExpression(parser);
	while (parser->token == XS_TOKEN_BIT_OR) {
		txInteger aLine = parser->line;
		fxGetNextToken(parser);
		fxBitXorExpression(parser);
		fxCheckArrowFunction(parser, 2);
		fxPushNodeStruct(parser, 2, XS_TOKEN_BIT_OR, aLine);
	}
}

void fxBitXorExpression(txParser* parser)
{
	fxBitAndExpression(parser);
	while (parser->token == XS_TOKEN_BIT_XOR) {
		txInteger aLine = parser->line;
		fxGetNextToken(parser);
		fxBitAndExpression(parser);
		fxCheckArrowFunction(parser, 2);
		fxPushNodeStruct(parser, 2, XS_TOKEN_BIT_XOR, aLine);
	}
}

void fxBitAndExpression(txParser* parser)
{
	fxEqualExpression(parser);
	while (parser->token == XS_TOKEN_BIT_AND) {
		txInteger aLine = parser->line;
		fxGetNextToken(parser);
		fxEqualExpression(parser);
		fxCheckArrowFunction(parser, 2);
		fxPushNodeStruct(parser, 2, XS_TOKEN_BIT_AND, aLine);
	}
}

void fxEqualExpression(txParser* parser)
{
	fxRelationalExpression(parser);
	while (gxTokenFlags[parser->token] & XS_TOKEN_EQUAL_EXPRESSION) {
		txToken aToken = parser->token;
		txInteger aLine = parser->line;
		fxGetNextToken(parser);
		fxRelationalExpression(parser);
		fxCheckArrowFunction(parser, 2);
		fxPushNodeStruct(parser, 2, aToken, aLine);
	}
}

void fxRelationalExpression(txParser* parser)
{
	if (parser->token == XS_TOKEN_PRIVATE_IDENTIFIER) {
		txInteger aLine = parser->line;
		fxPushSymbol(parser, parser->symbol);
		fxGetNextToken(parser);
		fxMatchToken(parser, XS_TOKEN_IN);
		if (parser->flags & mxForFlag)
			fxReportParserError(parser, parser->line, "invalid %s", gxTokenNames[XS_TOKEN_IN]);
		fxShiftExpression(parser);
		fxCheckArrowFunction(parser, 2);
		fxPushNodeStruct(parser, 2, XS_TOKEN_PRIVATE_IDENTIFIER, aLine);
	}
	else {
		fxShiftExpression(parser);
		if ((parser->flags & mxForFlag) && ((parser->token == XS_TOKEN_IN) || fxIsKeyword(parser, parser->ofSymbol)))
			return;
		while (gxTokenFlags[parser->token] & XS_TOKEN_RELATIONAL_EXPRESSION) {
			txToken aToken = parser->token;
			txInteger aLine = parser->line;
			fxMatchToken(parser, aToken);
			fxShiftExpression(parser);
			fxCheckArrowFunction(parser, 2);
			fxPushNodeStruct(parser, 2, aToken, aLine);
		}
	}
}

void fxShiftExpression(txParser* parser)
{
	fxAdditiveExpression(parser);
	while (gxTokenFlags[parser->token] & XS_TOKEN_SHIFT_EXPRESSION) {
		txToken aToken = parser->token;
		txInteger aLine = parser->line;
		fxGetNextToken(parser);
		fxAdditiveExpression(parser);
		fxCheckArrowFunction(parser, 2);
		fxPushNodeStruct(parser, 2, aToken, aLine);
	}
}

void fxAdditiveExpression(txParser* parser)
{
	fxMultiplicativeExpression(parser);
	while (gxTokenFlags[parser->token] & XS_TOKEN_ADDITIVE_EXPRESSION) {
		txToken aToken = parser->token;
		txInteger aLine = parser->line;
		fxGetNextToken(parser);
		fxMultiplicativeExpression(parser);
		fxCheckArrowFunction(parser, 2);
		fxPushNodeStruct(parser, 2, aToken, aLine);
	}
}

void fxMultiplicativeExpression(txParser* parser)
{
	fxExponentiationExpression(parser);
	while (gxTokenFlags[parser->token] & XS_TOKEN_MULTIPLICATIVE_EXPRESSION) {
		txToken aToken = parser->token;
		txInteger aLine = parser->line;
		fxGetNextToken(parser);
		fxExponentiationExpression(parser);
		fxCheckArrowFunction(parser, 2);
		fxPushNodeStruct(parser, 2, aToken, aLine);
	}
}

void fxExponentiationExpression(txParser* parser)
{
	if (gxTokenFlags[parser->token] & XS_TOKEN_UNARY_EXPRESSION)
		fxUnaryExpression(parser);
	else {
		fxPrefixExpression(parser);
		if (gxTokenFlags[parser->token] & XS_TOKEN_EXPONENTIATION_EXPRESSION) {
			txToken aToken = parser->token;
			txInteger aLine = parser->line;
			fxGetNextToken(parser);
            fxExponentiationExpression(parser);
			fxCheckArrowFunction(parser, 2);
			fxPushNodeStruct(parser, 2, aToken, aLine);
		}
	}
}

void fxUnaryExpression(txParser* parser)
{
	if (gxTokenFlags[parser->token] & XS_TOKEN_UNARY_EXPRESSION) {
		txToken aToken = parser->token;
		txInteger aLine = parser->line;
		fxCheckParserStack(parser, aLine);
		fxMatchToken(parser, aToken);
		fxUnaryExpression(parser);
		fxCheckArrowFunction(parser, 1);
		if (aToken == XS_TOKEN_ADD)
			fxPushNodeStruct(parser, 1, XS_TOKEN_PLUS, aLine);
		else if (aToken == XS_TOKEN_SUBTRACT)
			fxPushNodeStruct(parser, 1, XS_TOKEN_MINUS, aLine);
		else if (aToken == XS_TOKEN_DELETE) {
			//if ((parser->flags & mxStrictFlag) && (parser->root->description->token == XS_TOKEN_ACCESS)) {
			//	fxReportParserError(parser, parser->line, "no reference (strict mode)");
			//}
			if (!fxCheckReference(parser, aToken)) 
				fxReportParserError(parser, parser->line, "no reference");
			fxPushNodeStruct(parser, 1, aToken, aLine);
		}
		else if (aToken == XS_TOKEN_AWAIT) {
			if ((parser->flags & mxGeneratorFlag) && !(parser->flags & mxYieldFlag))
				fxReportParserError(parser, parser->line, "invalid await");
			else
				parser->flags |= mxAwaitingFlag;
			fxPushNodeStruct(parser, 1, aToken, aLine);
		}
		else
			fxPushNodeStruct(parser, 1, aToken, aLine);
			
	}
	else
		fxPrefixExpression(parser);
}

void fxPrefixExpression(txParser* parser)
{
	if (gxTokenFlags[parser->token] & XS_TOKEN_PREFIX_EXPRESSION) {
		txToken aToken = parser->token;
		txInteger aLine = parser->line;
		fxCheckParserStack(parser, aLine);
		fxGetNextToken(parser);
		fxPrefixExpression(parser);
		fxCheckArrowFunction(parser, 1);
		if (!fxCheckReference(parser, aToken)) 
			fxReportParserError(parser, parser->line, "no reference");
		fxPushNodeStruct(parser, 1, aToken, aLine);
		parser->root->flags = mxExpressionNoValue;
	}
	else
		fxPostfixExpression(parser);
}

void fxPostfixExpression(txParser* parser)
{
	fxCallExpression(parser);
	if ((!parser->crlf) && (gxTokenFlags[parser->token] & XS_TOKEN_POSTFIX_EXPRESSION)) {
		fxCheckArrowFunction(parser, 1);
		if (!fxCheckReference(parser, parser->token)) 
			fxReportParserError(parser, parser->line, "no reference");
		fxPushNodeStruct(parser, 1, parser->token, parser->line);
		fxGetNextToken(parser);
	}
}

void fxCallExpression(txParser* parser)
{
	txInteger chainLine = parser->line;
	fxLiteralExpression(parser, 0);
	if (gxTokenFlags[parser->token] & XS_TOKEN_CALL_EXPRESSION) {
		txBoolean chainFlag = 0;
		fxCheckArrowFunction(parser, 1);
		for (;;) {
			txInteger aLine = parser->line;
			if (parser->token == XS_TOKEN_DOT) {
				fxGetNextToken(parser);
				if (parser->token == XS_TOKEN_IDENTIFIER) {
					fxPushSymbol(parser, parser->symbol);
					fxPushNodeStruct(parser, 2, XS_TOKEN_MEMBER, aLine);
					fxGetNextToken(parser);
				}
				else if (parser->token == XS_TOKEN_PRIVATE_IDENTIFIER) {
					if (parser->root->flags & mxSuperFlag)
						fxReportParserError(parser, parser->line, "invalid super");
					fxPushSymbol(parser, parser->symbol);
					fxSwapNodes(parser);
					fxPushNodeStruct(parser, 2, XS_TOKEN_PRIVATE_MEMBER, aLine);
					fxGetNextToken(parser);
				}
				else
					fxReportParserError(parser, parser->line, "missing property");
			}
			//else if (parser->crlf)
			//	break;
			else if (parser->token == XS_TOKEN_LEFT_BRACKET) {
				fxGetNextToken(parser);
				fxCommaExpression(parser);
				fxPushNodeStruct(parser, 2, XS_TOKEN_MEMBER_AT, aLine);
				fxMatchToken(parser, XS_TOKEN_RIGHT_BRACKET);
			}
			else if (parser->token == XS_TOKEN_LEFT_PARENTHESIS) {
				if (parser->root->description && (parser->root->description->token == XS_TOKEN_ACCESS)) {
					txAccessNode* access = (txAccessNode*)parser->root;
					if (access->symbol == parser->evalSymbol) {
						parser->flags |= mxEvalFlag;
					}
				}
				fxParameters(parser);
				fxPushNodeStruct(parser, 2, XS_TOKEN_CALL, aLine);
			}
			else if (parser->token == XS_TOKEN_TEMPLATE) {
				if (chainFlag)
					fxReportParserError(parser, parser->line, "invalid template");
				fxPushStringNode(parser, parser->stringLength, parser->string, aLine);
				fxPushRawNode(parser, parser->rawLength, parser->raw, aLine);
				fxPushNodeStruct(parser, 2, XS_TOKEN_TEMPLATE_MIDDLE, aLine);
				fxGetNextToken(parser);
				fxPushNodeList(parser, 1);
				fxPushNodeStruct(parser, 2, XS_TOKEN_TEMPLATE, aLine);
			}
			else if (parser->token == XS_TOKEN_TEMPLATE_HEAD) {
				if (chainFlag)
					fxReportParserError(parser, parser->line, "invalid template");
				fxTemplateExpression(parser);
				fxPushNodeStruct(parser, 2, XS_TOKEN_TEMPLATE, aLine);
			}
			else if (parser->token == XS_TOKEN_CHAIN) {
				fxGetNextToken(parser);
				chainFlag = 1;
				if (parser->token == XS_TOKEN_IDENTIFIER) {
					fxPushNodeStruct(parser, 1, XS_TOKEN_OPTION, aLine);
					fxPushSymbol(parser, parser->symbol);
					fxPushNodeStruct(parser, 2, XS_TOKEN_MEMBER, aLine);
					fxGetNextToken(parser);
				}
				else if (parser->token == XS_TOKEN_PRIVATE_IDENTIFIER) {
					fxPushNodeStruct(parser, 1, XS_TOKEN_OPTION, aLine);
					fxPushSymbol(parser, parser->symbol);
					fxSwapNodes(parser);
					fxPushNodeStruct(parser, 2, XS_TOKEN_PRIVATE_MEMBER, aLine);
					fxGetNextToken(parser);
				}
				else if (parser->token == XS_TOKEN_LEFT_BRACKET) {
					fxPushNodeStruct(parser, 1, XS_TOKEN_OPTION, aLine);
					fxGetNextToken(parser);
					fxCommaExpression(parser);
					fxPushNodeStruct(parser, 2, XS_TOKEN_MEMBER_AT, aLine);
					fxMatchToken(parser, XS_TOKEN_RIGHT_BRACKET);
				}
				else if (parser->token == XS_TOKEN_LEFT_PARENTHESIS) {
					fxPushNodeStruct(parser, 1, XS_TOKEN_OPTION, aLine);
					fxParameters(parser);
					fxPushNodeStruct(parser, 2, XS_TOKEN_CALL, aLine);
				}
				else
					fxReportParserError(parser, parser->line, "invalid ?.");
			}
			else
				break;
		} 
		if (chainFlag)
			fxPushNodeStruct(parser, 1, XS_TOKEN_CHAIN, chainLine);
	} 
}

void fxLiteralExpression(txParser* parser, txUnsigned flag)
{
	int escaped;
	txSymbol* aSymbol;
	txInteger aLine = parser->line;
	txUnsigned flags = 0;
	char c = 0;
	fxCheckParserStack(parser, aLine);
	switch (parser->token) {
	case XS_TOKEN_NULL:
	case XS_TOKEN_TRUE:
	case XS_TOKEN_FALSE:
		fxPushNodeStruct(parser, 0, parser->token, aLine);
		fxMatchToken(parser, parser->token);
		break;
	case XS_TOKEN_IMPORT:
		fxMatchToken(parser, XS_TOKEN_IMPORT);
		if (!flag && (parser->token == XS_TOKEN_LEFT_PARENTHESIS)) {
			fxGetNextToken(parser);
			fxAssignmentExpression(parser);
			fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
			fxPushNodeStruct(parser, 1, XS_TOKEN_IMPORT_CALL, aLine);
		}
		else if (parser->token == XS_TOKEN_DOT) {
			fxGetNextToken(parser);
			if ((parser->token == XS_TOKEN_IDENTIFIER) && (parser->symbol == parser->metaSymbol) && (!parser->escaped)) {	
				fxGetNextToken(parser);
				if (parser->flags & mxProgramFlag)
					fxReportParserError(parser, parser->line, "invalid import.meta");
				else
					fxPushNodeStruct(parser, 0, XS_TOKEN_IMPORT_META, aLine);
			}
			else
				fxReportParserError(parser, parser->line, "invalid import.");
		}
		else
			fxReportParserError(parser, parser->line, "invalid import");
		break;
	case XS_TOKEN_SUPER:
		fxMatchToken(parser, XS_TOKEN_SUPER);
		if (parser->token == XS_TOKEN_LEFT_PARENTHESIS) {
			if (parser->flags & mxDerivedFlag) {
				fxParameters(parser);
				fxPushNodeStruct(parser, 1, XS_TOKEN_SUPER, aLine);
			}
			else {
				fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aLine);
				fxReportParserError(parser, parser->line, "invalid super");
			}
		}
		else if ((parser->token == XS_TOKEN_DOT) || (parser->token == XS_TOKEN_LEFT_BRACKET)) {
			if (parser->flags & mxSuperFlag) {
				fxPushNodeStruct(parser, 0, XS_TOKEN_THIS, aLine);
				parser->root->flags |= parser->flags & (mxDerivedFlag | mxSuperFlag);
			}
			else {
				fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aLine);
				fxReportParserError(parser, parser->line, "invalid super");
			}
		}
		else
			fxReportParserError(parser, parser->line, "invalid super");
		parser->flags |= mxSuperFlag;
		break;
	case XS_TOKEN_THIS:
		fxPushNodeStruct(parser, 0, parser->token, aLine);
		parser->root->flags |= parser->flags & mxDerivedFlag;
		fxMatchToken(parser, XS_TOKEN_THIS);
		break;
	case XS_TOKEN_INTEGER:
		fxPushIntegerNode(parser, parser->integer, aLine);
		fxGetNextToken(parser);
		break;
	case XS_TOKEN_NUMBER:
		fxPushNumberNode(parser, parser->number, aLine);
		fxGetNextToken(parser);
		break;
	case XS_TOKEN_BIGINT:
		fxPushBigIntNode(parser, &parser->bigint, aLine);
		fxGetNextToken(parser);
		break;
	case XS_TOKEN_DIVIDE_ASSIGN:
		c = '=';
		// continue
	case XS_TOKEN_DIVIDE:
		fxGetNextRegExp(parser, c);
		fxPushStringNode(parser, parser->modifierLength, parser->modifier, aLine);
		fxPushStringNode(parser, parser->stringLength, parser->string, aLine);
		fxPushNodeStruct(parser, 2, XS_TOKEN_REGEXP, aLine);
		fxGetNextToken(parser);
		break;
	case XS_TOKEN_STRING:
		fxPushStringNode(parser, parser->stringLength, parser->string, aLine);
		fxGetNextToken(parser);
		break;
	case XS_TOKEN_IDENTIFIER:
		escaped = parser->escaped;
		aSymbol = parser->symbol;
		fxGetNextToken(parser);
		if (aSymbol == parser->undefinedSymbol) {
			fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aLine);
			break;
		}
		flags = 0;
		if ((aSymbol == parser->asyncSymbol) && (!escaped) && (!parser->crlf)) {
			if (parser->token == XS_TOKEN_FUNCTION) {
				fxMatchToken(parser, XS_TOKEN_FUNCTION);
				if (parser->token == XS_TOKEN_MULTIPLY) {
					fxGetNextToken(parser);
					fxGeneratorExpression(parser, aLine, C_NULL, mxAsyncFlag);
				}
				else
					fxFunctionExpression(parser, aLine, C_NULL, mxAsyncFlag);
				break;
			}
			if (parser->token == XS_TOKEN_LEFT_PARENTHESIS) {
				fxGroupExpression(parser, mxAsyncFlag);
				break;
			}
			if (parser->token == XS_TOKEN_IDENTIFIER) {
// 				if (!(parser->flags & mxForFlag) || !fxIsKeyword(parser, parser->ofSymbol)) {
					aSymbol = parser->symbol;
					fxGetNextToken(parser);
					flags = mxAsyncFlag;
// 				}
			}
		}
		if (aSymbol == parser->awaitSymbol)
			parser->flags |= mxAwaitingFlag;
		if ((!parser->crlf) && (parser->token == XS_TOKEN_ARROW)) {
			fxCheckStrictSymbol(parser, aSymbol);
			if (flags && (aSymbol == parser->awaitSymbol))
				fxReportParserError(parser, parser->line, "invalid await");
			fxPushSymbol(parser, aSymbol);
			fxPushNULL(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_ARG, aLine);
			fxPushNodeList(parser, 1);
			fxPushNodeStruct(parser, 1, XS_TOKEN_PARAMS_BINDING, aLine);
			fxArrowExpression(parser, flags);
			break;
		}
		if (aSymbol == parser->argumentsSymbol)
			parser->flags |= mxArgumentsFlag;
		fxPushSymbol(parser, aSymbol);
		fxPushNodeStruct(parser, 1, XS_TOKEN_ACCESS, aLine);
		break;
	case XS_TOKEN_CLASS:
		flags = parser->flags & mxForFlag;
		parser->flags &= ~mxForFlag;
		fxClassExpression(parser, aLine, C_NULL);
		parser->flags |= flags;
		break;
	case XS_TOKEN_FUNCTION:
		fxMatchToken(parser, XS_TOKEN_FUNCTION);
		if (parser->token == XS_TOKEN_MULTIPLY) {
			fxGetNextToken(parser);
			fxGeneratorExpression(parser, aLine, C_NULL, 0);
		}
		else
			fxFunctionExpression(parser, aLine, C_NULL, 0);
		break;
	case XS_TOKEN_NEW:
		fxNewExpression(parser);
		break;
	case XS_TOKEN_LEFT_BRACE:
		flags = parser->flags & mxForFlag;
		parser->flags &= ~mxForFlag;
		fxObjectExpression(parser);
		parser->flags |= flags;
		break;
	case XS_TOKEN_LEFT_BRACKET:
		flags = parser->flags & mxForFlag;
		parser->flags &= ~mxForFlag;
		fxArrayExpression(parser);
		parser->flags |= flags;
		break;
	case XS_TOKEN_LEFT_PARENTHESIS:
		fxGroupExpression(parser, 0);
		break;
	case XS_TOKEN_TEMPLATE:
		fxPushNULL(parser);
		fxPushStringNode(parser, parser->stringLength, parser->string, aLine);
		fxPushRawNode(parser, parser->rawLength, parser->raw, aLine);
		fxPushNodeStruct(parser, 2, XS_TOKEN_TEMPLATE_MIDDLE, aLine);
		fxGetNextToken(parser);
		fxPushNodeList(parser, 1);
		fxPushNodeStruct(parser, 2, XS_TOKEN_TEMPLATE, aLine);
		break;
	case XS_TOKEN_TEMPLATE_HEAD:
		fxPushNULL(parser);
		fxTemplateExpression(parser);
		fxPushNodeStruct(parser, 2, XS_TOKEN_TEMPLATE, aLine);
		break;
	case XS_TOKEN_HOST:
		fxGetNextToken(parser);
		fxPushNULL(parser);
		fxPushNULL(parser);
		if (parser->token == XS_TOKEN_STRING) {
			fxPushStringNode(parser, parser->stringLength, parser->string, aLine);
			fxGetNextToken(parser);
		}
		else {
			fxReportParserError(parser, parser->line, "invalid host object");
			fxPushNULL(parser);
		}
		fxPushNodeStruct(parser, 3, XS_TOKEN_HOST, aLine);
		break;
	case XS_TOKEN_LESS:
		fxGetNextToken(parser);
		fxJSXElement(parser);
		fxGetNextToken(parser);
		break;
	default:
		fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aLine);
		fxReportParserError(parser, parser->line, "missing expression");
		break;
	}
}

void fxArrayExpression(txParser* parser)
{
	txInteger aCount = 0;
	int elision = 1;
	txInteger aLine = parser->line;
	txBoolean aSpreadFlag = 0;
	fxMatchToken(parser, XS_TOKEN_LEFT_BRACKET);
	while ((parser->token == XS_TOKEN_COMMA) || (parser->token == XS_TOKEN_SPREAD) || (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION)) {
		txInteger anItemLine = parser->line;
		if (parser->token == XS_TOKEN_COMMA) {
			fxGetNextToken(parser);
			if (elision) {
				fxPushNodeStruct(parser, 0, XS_TOKEN_ELISION, anItemLine);
				aCount++;
			}
			else
				elision = 1;
		}
		else if (parser->token == XS_TOKEN_SPREAD) {
			fxGetNextToken(parser);
			if (!elision)
				fxReportParserError(parser, parser->line, "missing ,");
			fxAssignmentExpression(parser);
			fxPushNodeStruct(parser, 1, XS_TOKEN_SPREAD, anItemLine);
			aCount++;
			elision = 0;
			aSpreadFlag = 1;
		}
		else {
			if (!elision)
				fxReportParserError(parser, parser->line, "missing ,");
			fxAssignmentExpression(parser);
			aCount++;
			elision = 0;
		}
	}
	fxMatchToken(parser, XS_TOKEN_RIGHT_BRACKET);
	fxPushNodeList(parser, aCount);
	fxPushNodeStruct(parser, 1, XS_TOKEN_ARRAY, aLine);
	if (aCount && elision)
		parser->root->flags |= mxElisionFlag;
	if (aSpreadFlag)
		parser->root->flags |= mxSpreadFlag;
}

void fxArrowExpression(txParser* parser, txUnsigned flag)
{
	txInteger aLine = parser->line;
	txUnsigned flags = parser->flags;
	parser->flags &= ~mxAsyncFlag;
	parser->flags |= mxArrowFlag | flag;
	fxMatchToken(parser, XS_TOKEN_ARROW);
	fxPushNULL(parser);
	fxSwapNodes(parser);	
	if (parser->token == XS_TOKEN_LEFT_BRACE) {
		fxMatchToken(parser, XS_TOKEN_LEFT_BRACE);
		fxBody(parser);
		fxPushNodeStruct(parser, 1, XS_TOKEN_BODY, aLine);
		fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
	}
	else {
		fxAssignmentExpression(parser);
		fxPushNodeStruct(parser, 1, XS_TOKEN_RETURN, aLine);
		fxPushNodeStruct(parser, 1, XS_TOKEN_BODY, aLine);
	}
	fxPushNodeStruct(parser, 3, XS_TOKEN_FUNCTION, aLine);
	parser->root->flags = parser->flags & (mxStrictFlag | mxFieldFlag | mxNotSimpleParametersFlag | mxArrowFlag | mxSuperFlag | flag);
	if (!(flags & mxStrictFlag) && (parser->flags & mxStrictFlag))
		fxCheckStrictFunction(parser, (txFunctionNode*)parser->root);
	parser->flags = flags | (parser->flags & (mxArgumentsFlag | mxEvalFlag));
}

void fxClassExpression(txParser* parser, txInteger theLine, txSymbol** theSymbol)
{
	txBoolean heritageFlag = 0;
//	txBoolean hostFlag = 0;
	txNode* constructor = NULL;
	txInteger aCount = 0;
	txInteger aLine = parser->line;
	txUnsigned flags = parser->flags;
	txInteger constructorInitCount = 0;
	txInteger instanceInitCount = 0;
	parser->flags |= mxStrictFlag;
	fxMatchToken(parser, XS_TOKEN_CLASS);
	if (parser->token == XS_TOKEN_IDENTIFIER) {
		fxPushSymbol(parser, parser->symbol);
		if (theSymbol)
			*theSymbol = parser->symbol;
		fxGetNextToken(parser);
	}
	else
		fxPushNULL(parser);
	if (parser->token == XS_TOKEN_EXTENDS) {
		fxMatchToken(parser, XS_TOKEN_EXTENDS);
		fxCallExpression(parser);
		fxCheckArrowFunction(parser, 1);
		flags |= parser->flags & mxAwaitingFlag;
		heritageFlag = 1;
	}
	else if (parser->token == XS_TOKEN_HOST) {
		fxGetNextToken(parser);
		fxPushNULL(parser);
		fxPushNULL(parser);
		if (parser->token == XS_TOKEN_STRING) {
			fxPushStringNode(parser, parser->stringLength, parser->string, aLine);
			fxGetNextToken(parser);
		}
		else {
			fxReportParserError(parser, parser->line, "invalid host class");
			fxPushNULL(parser);
		}
		fxPushNodeStruct(parser, 3, XS_TOKEN_HOST, aLine);
//		hostFlag = 1;
	}
	else {
		fxPushNULL(parser);
	}
	if (parser->token == XS_TOKEN_LEFT_BRACE) {
		fxMatchToken(parser, XS_TOKEN_LEFT_BRACE);
		for (;;) {
			txBoolean aStaticFlag;
			txInteger aPropertyLine = parser->line;
			txSymbol* aSymbol;
			txToken aToken0;
			txToken aToken1;
			txToken aToken2;
			txUnsigned flag;
			while (parser->token == XS_TOKEN_SEMICOLON)
				fxGetNextToken(parser);
			if (parser->token == XS_TOKEN_RIGHT_BRACE)
				break;
			aStaticFlag = 0;
			if ((parser->token == XS_TOKEN_STATIC) && (!parser->escaped)) {
				fxGetNextToken(parser);
				if ((parser->token == XS_TOKEN_ASSIGN) || parser->token == XS_TOKEN_SEMICOLON) {
					fxPushSymbol(parser, parser->staticSymbol);
					aToken1 = XS_TOKEN_PROPERTY;
					goto field;
				}
				if (parser->token == XS_TOKEN_LEFT_BRACE) {
					txUnsigned flags = parser->flags;
					parser->flags = (flags & (mxParserFlags | mxStrictFlag)) | mxSuperFlag | mxTargetFlag | mxFieldFlag | mxAsyncFlag;
					fxCheckParserStack(parser, aPropertyLine);
					fxGetNextToken(parser);
					fxStatements(parser);
					fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
					fxPushNodeStruct(parser, 1, XS_TOKEN_BODY, aPropertyLine);
					if (parser->flags & mxArgumentsFlag)
						fxReportParserError(parser, parser->line, "invalid arguments");
					if (parser->flags & mxAwaitingFlag)
						fxReportParserError(parser, parser->line, "invalid await");
					parser->flags = flags;
					parser->root->flags |= mxStaticFlag;
					constructorInitCount++;
					aCount++;
					continue;
				}
				aStaticFlag = 1;
			}
			fxPropertyName(parser, &aSymbol, &aToken0, &aToken1, &aToken2, &flag);
			if ((aStaticFlag == 0) && (aSymbol == parser->constructorSymbol)) {
				fxPopNode(parser); // symbol
				if (constructor || (aToken2 == XS_TOKEN_GENERATOR) || (aToken2 == XS_TOKEN_GETTER) || (aToken2 == XS_TOKEN_SETTER) || (flag & mxAsyncFlag)) 
					fxReportParserError(parser, parser->line, "invalid constructor");
				fxFunctionExpression(parser, aPropertyLine, C_NULL, mxSuperFlag | ((heritageFlag) ? mxDerivedFlag : mxBaseFlag));
				constructor = fxPopNode(parser);
			}
			else if (parser->token == XS_TOKEN_LEFT_PARENTHESIS) {
				if ((aToken1 == XS_TOKEN_PRIVATE_PROPERTY) && (aSymbol == parser->privateConstructorSymbol))
					fxReportParserError(parser, parser->line, "invalid method: #constructor");
				if (aStaticFlag && (aSymbol == parser->prototypeSymbol))
					fxReportParserError(parser, parser->line, "invalid static method: prototype");
				if (aStaticFlag)
					flag |= mxStaticFlag;
				if (aToken2 == XS_TOKEN_GETTER) 
					flag |= mxGetterFlag;
				else if (aToken2 == XS_TOKEN_SETTER) 
					flag |= mxSetterFlag;
				else
					flag |= mxMethodFlag;
				if (aToken2 == XS_TOKEN_GENERATOR)
					fxGeneratorExpression(parser, aPropertyLine, C_NULL, mxSuperFlag | flag);
				else
					fxFunctionExpression(parser, aPropertyLine, C_NULL, mxSuperFlag | flag);
				fxPushNodeStruct(parser, 2, aToken1, aPropertyLine);
				parser->root->flags |= flag & (mxStaticFlag | mxGetterFlag | mxSetterFlag | mxMethodFlag);
				if (aToken1 == XS_TOKEN_PRIVATE_PROPERTY) {
					if (aStaticFlag)
						constructorInitCount++;
					else
						instanceInitCount++;
				}
				aCount++;
			}
			else {
				if ((aToken1 == XS_TOKEN_PRIVATE_PROPERTY) && (aSymbol == parser->privateConstructorSymbol))
					fxReportParserError(parser, parser->line, "invalid field: #constructor");
				if (aSymbol == parser->constructorSymbol)
					fxReportParserError(parser, parser->line, "invalid field: constructor");
				if (aSymbol == parser->prototypeSymbol)
					fxReportParserError(parser, parser->line, "invalid field: prototype");
			field:
				if (parser->token == XS_TOKEN_ASSIGN) {
					txUnsigned flags = parser->flags;
					parser->flags = (flags & (mxParserFlags | mxStrictFlag)) | mxSuperFlag | mxTargetFlag | mxFieldFlag;
					fxGetNextToken(parser);
					fxAssignmentExpression(parser);
					if (parser->flags & mxArgumentsFlag)
						fxReportParserError(parser, parser->line, "invalid arguments");
					parser->flags = flags;
				}
				else {
					fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aLine);
				}
				fxPushNodeStruct(parser, 2, aToken1, aPropertyLine);
				if (aStaticFlag) {
					parser->root->flags |= mxStaticFlag;
					constructorInitCount++;
				}
				else
					instanceInitCount++;
				fxSemicolon(parser);
				aCount++;
			}
		}
	}
	fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
	fxPushNodeList(parser, aCount);
	
	if (constructorInitCount || instanceInitCount) {
		txNodeList* itemsList = (txNodeList*)(parser->root);
		txNodeList* constructorInitList = C_NULL;
		txNode** constructorInitAddress = C_NULL;
		txNodeList* instanceInitList = C_NULL;
		txNode** instanceInitAddress = C_NULL;
		txNode** address;
		txNode* item;
		if (constructorInitCount) {
			fxPushNULL(parser);
			fxPushNodeList(parser, 0);
			fxPushNodeStruct(parser, 1, XS_TOKEN_PARAMS_BINDING, aLine);
			fxPushNodeList(parser, 0);
			constructorInitList = (txNodeList*)(parser->root);
			constructorInitList->length = constructorInitCount;
			constructorInitAddress = &(((txNodeList*)(parser->root))->first);
			fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENTS, aLine);
			fxPushNodeStruct(parser, 1, XS_TOKEN_BODY, aLine);
			fxPushNodeStruct(parser, 3, XS_TOKEN_FUNCTION, aLine);
			parser->root->flags = mxStrictFlag | mxSuperFlag | mxFieldFlag;
		}
		else 
			fxPushNULL(parser);
		if (instanceInitCount) {
			fxPushNULL(parser);
			fxPushNodeList(parser, 0);
			fxPushNodeStruct(parser, 1, XS_TOKEN_PARAMS_BINDING, aLine);
			fxPushNodeList(parser, 0);
			instanceInitList = (txNodeList*)(parser->root);
			instanceInitList->length = instanceInitCount;
			instanceInitAddress = &(((txNodeList*)(parser->root))->first);
			fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENTS, aLine);
			fxPushNodeStruct(parser, 1, XS_TOKEN_BODY, aLine);
			fxPushNodeStruct(parser, 3, XS_TOKEN_FUNCTION, aLine);
			parser->root->flags = mxStrictFlag | mxSuperFlag | mxFieldFlag;
		}
		else 
			fxPushNULL(parser);
		address = &(itemsList->first);
		while ((item = *address)) {
			if (item->flags & (mxMethodFlag | mxGetterFlag | mxSetterFlag)) {
				if (item->description->token == XS_TOKEN_PRIVATE_PROPERTY) {
					txFieldNode* field = fxFieldNodeNew(parser, XS_TOKEN_FIELD);
					field->item = item;
					if (item->flags & mxStaticFlag) {
						*constructorInitAddress = (txNode*)field;
						constructorInitAddress = &field->next;
					}
					else {
						*instanceInitAddress = (txNode*)field;
						instanceInitAddress = &field->next;
					}
				}
			}
			address = &(item->next);
		}
		address = &(itemsList->first);
		while ((item = *address)) {
			if (item->description->token == XS_TOKEN_BODY) {
				*address = item->next;
				item->next = C_NULL;
				itemsList->length--;
				*constructorInitAddress = (txNode*)item;
				constructorInitAddress = &item->next;
			}
			else if (item->flags & (mxMethodFlag | mxGetterFlag | mxSetterFlag)) {
				address = &(item->next);
			}
			else {
				txFieldNode* field = fxFieldNodeNew(parser, XS_TOKEN_FIELD);
				field->item = item;
				if (item->description->token == XS_TOKEN_PROPERTY) {
					field->value = ((txPropertyNode*)item)->value;
					((txPropertyNode*)item)->value = C_NULL;
				}
				else if (item->description->token == XS_TOKEN_PROPERTY_AT) {
					field->value = ((txPropertyAtNode*)item)->value;
					((txPropertyAtNode*)item)->value = C_NULL;
				}
				else {
					field->value = ((txPrivatePropertyNode*)item)->value;
					((txPrivatePropertyNode*)item)->value = C_NULL;
				}
				if (item->flags & mxStaticFlag) {
					*constructorInitAddress = (txNode*)field;
					constructorInitAddress = &field->next;
				}
				else {
					*instanceInitAddress = (txNode*)field;
					instanceInitAddress = &field->next;
				}
				address = &(item->next);
			}
		}
	}
	else {
		fxPushNULL(parser);
		fxPushNULL(parser);
	}
	if (constructor) {
		fxPushNode(parser, constructor);
	}
	else {
		if (heritageFlag) {
			fxPushNULL(parser);
		
			fxPushSymbol(parser, parser->argsSymbol);
			fxPushNULL(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_ARG, aLine);
			fxPushNodeStruct(parser, 1, XS_TOKEN_REST_BINDING, aLine);
			fxPushNodeList(parser, 1);
			fxPushNodeStruct(parser, 1, XS_TOKEN_PARAMS_BINDING, aLine);
			
			fxPushSymbol(parser, parser->argsSymbol);
			fxPushNodeStruct(parser, 1, XS_TOKEN_ACCESS, aLine);
			fxPushNodeStruct(parser, 1, XS_TOKEN_SPREAD, aLine);
			fxPushNodeList(parser, 1);
			fxPushNodeStruct(parser, 1, XS_TOKEN_PARAMS, aLine);
			parser->root->flags |= mxSpreadFlag;
			fxPushNodeStruct(parser, 1, XS_TOKEN_SUPER, aLine);
			fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, aLine);
			fxPushNodeStruct(parser, 1, XS_TOKEN_BODY, aLine);
			
			fxPushNodeStruct(parser, 3, XS_TOKEN_FUNCTION, aLine);
			parser->root->flags = mxStrictFlag | mxDerivedFlag | mxMethodFlag | mxTargetFlag | mxSuperFlag;
		}
		else {
			fxPushNULL(parser);
		
			fxPushNodeList(parser, 0);
			fxPushNodeStruct(parser, 1, XS_TOKEN_PARAMS_BINDING, aLine);
			
			fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aLine);
			fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, aLine);
			fxPushNodeStruct(parser, 1, XS_TOKEN_BODY, aLine);
			
			fxPushNodeStruct(parser, 3, XS_TOKEN_FUNCTION, aLine);
			parser->root->flags = mxStrictFlag | mxBaseFlag | mxMethodFlag | mxTargetFlag;
		}
	}
	fxPushNodeStruct(parser, 6, XS_TOKEN_CLASS, aLine);
	parser->flags = flags | (parser->flags & (mxArgumentsFlag));
}

void fxFunctionExpression(txParser* parser, txInteger theLine, txSymbol** theSymbol, txUnsigned flag)
{
	txUnsigned flags = parser->flags;
	parser->flags = (flags & (mxParserFlags | mxStrictFlag)) | mxFunctionFlag | mxTargetFlag | flag;
	if ((parser->token == XS_TOKEN_IDENTIFIER)
			|| ((flags & mxGeneratorFlag) && !(flags & mxStrictFlag) && (parser->token == XS_TOKEN_YIELD))
			|| ((theSymbol == C_NULL) && (parser->token == XS_TOKEN_AWAIT))) {
		fxPushSymbol(parser, parser->symbol);
		if (theSymbol)
			*theSymbol = parser->symbol;
		fxCheckStrictSymbol(parser, parser->symbol);
		fxGetNextToken(parser);
	}
	else
		fxPushNULL(parser);
	fxParametersBinding(parser);
	if (parser->token == XS_TOKEN_HOST) {
		fxGetNextToken(parser);
		if (parser->token == XS_TOKEN_STRING) {
			fxPushStringNode(parser, parser->stringLength, parser->string, theLine);
			fxGetNextToken(parser);
		}
		else {
			fxReportParserError(parser, parser->line, "invalid host function");
			fxPushNULL(parser);
		}
		fxPushNodeStruct(parser, 3, XS_TOKEN_HOST, theLine);
        parser->root->flags = parser->flags & (mxStrictFlag | mxNotSimpleParametersFlag | mxTargetFlag | mxArgumentsFlag | mxEvalFlag | flag);
        if (!(flags & mxStrictFlag) && (parser->flags & mxStrictFlag))
            fxCheckStrictFunction(parser, (txFunctionNode*)parser->root);
        parser->flags = flags;
	}
	else {
		fxMatchToken(parser, XS_TOKEN_LEFT_BRACE);
		fxBody(parser);
		fxPushNodeStruct(parser, 1, XS_TOKEN_BODY, theLine);
		fxPushNodeStruct(parser, 3, XS_TOKEN_FUNCTION, theLine);
        parser->root->flags = parser->flags & (mxStrictFlag | mxNotSimpleParametersFlag | mxTargetFlag | mxArgumentsFlag | mxEvalFlag | flag);
        if (!(flags & mxStrictFlag) && (parser->flags & mxStrictFlag))
            fxCheckStrictFunction(parser, (txFunctionNode*)parser->root);
        parser->flags = flags;
        fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
	}
}

void fxGeneratorExpression(txParser* parser, txInteger theLine, txSymbol** theSymbol, txUnsigned flag)
{
	txUnsigned flags = parser->flags;
	parser->flags = (flags & (mxParserFlags | mxStrictFlag)) | mxGeneratorFlag | mxTargetFlag | flag;
	if ((parser->token == XS_TOKEN_IDENTIFIER)
			|| ((theSymbol == C_NULL) && (parser->token == XS_TOKEN_AWAIT))) {
		fxPushSymbol(parser, parser->symbol);
		if (theSymbol)
			*theSymbol = parser->symbol;
		else if (parser->symbol == parser->yieldSymbol)
 			fxReportParserError(parser, parser->line, "invalid yield");
		else if ((parser->flags & mxAsyncFlag) && (parser->symbol == parser->awaitSymbol))
 			fxReportParserError(parser, parser->line, "invalid await");
		fxCheckStrictSymbol(parser, parser->symbol);
		fxGetNextToken(parser);
	}
	else
		fxPushNULL(parser);
	fxParametersBinding(parser);
	fxMatchToken(parser, XS_TOKEN_LEFT_BRACE);
	parser->flags |= mxYieldFlag;
    fxBody(parser);
	parser->flags &= ~mxYieldFlag;
	fxPushNodeStruct(parser, 1, XS_TOKEN_BODY, theLine);
	fxPushNodeStruct(parser, 3, XS_TOKEN_GENERATOR, theLine);
	parser->root->flags = parser->flags & (mxStrictFlag | mxNotSimpleParametersFlag | mxGeneratorFlag | mxArgumentsFlag | mxEvalFlag | flag);
	if (!(flags & mxStrictFlag) && (parser->flags & mxStrictFlag))
		fxCheckStrictFunction(parser, (txFunctionNode*)parser->root);
	parser->flags = flags;
    fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
}

void fxGroupExpression(txParser* parser, txUnsigned flag)
{
	txBoolean commaFlag = 0;
	txBoolean spreadFlag = 0;
	txInteger aCount = 0;
	txInteger aLine;
	txUnsigned formerAwaitingYieldingFlags = parser->flags & (mxAwaitingFlag | mxYieldingFlag);
	parser->flags &= ~(mxAwaitingFlag | mxYieldingFlag);
	fxMatchToken(parser, XS_TOKEN_LEFT_PARENTHESIS);
	while ((parser->token == XS_TOKEN_SPREAD) || (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION)) {
		aLine = parser->line;
		commaFlag = 0;
		if (parser->token == XS_TOKEN_SPREAD) {
			fxGetNextToken(parser);
			fxAssignmentExpression(parser);
			fxPushNodeStruct(parser, 1, XS_TOKEN_SPREAD, aLine);
			spreadFlag = 1;
		}
		else
			fxAssignmentExpression(parser);
		aCount++;
		if (parser->token != XS_TOKEN_COMMA) 
			break;
		fxGetNextToken(parser);
		commaFlag = 1;
	}
	aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
	if ((!parser->crlf) && (parser->token == XS_TOKEN_ARROW)) {
		fxPushNodeList(parser, aCount);
		fxPushNodeStruct(parser, 1, XS_TOKEN_EXPRESSIONS, aLine);
		if (commaFlag && spreadFlag)
			fxReportParserError(parser, parser->line, "invalid parameters");
		if (!fxParametersBindingFromExpressions(parser, parser->root))
			fxReportParserError(parser, parser->line, "no parameters");
		fxCheckStrictBinding(parser, parser->root);
		parser->root->flags |= flag;
		if (parser->flags & mxAwaitingFlag) {
			if (flag || (parser->flags & mxAsyncFlag))
				fxReportParserError(parser, parser->line, "invalid await");
			else
				formerAwaitingYieldingFlags |= mxAwaitingFlag;
		}
		if (parser->flags & mxYieldingFlag)
			fxReportParserError(parser, parser->line, "invalid yield");
		fxArrowExpression(parser, flag);
	}
	else if (flag) {
		fxPushNodeList(parser, aCount);
		fxPushNodeStruct(parser, 1, XS_TOKEN_PARAMS, aLine);
		if (spreadFlag)
			parser->root->flags |= mxSpreadFlag;
		
		fxPushSymbol(parser, parser->asyncSymbol);
		fxPushNodeStruct(parser, 1, XS_TOKEN_ACCESS, aLine);
		fxSwapNodes(parser);
		fxPushNodeStruct(parser, 2, XS_TOKEN_CALL, aLine);
	}
	else {
        if ((aCount == 0) || commaFlag) {
            fxPushNULL(parser);
			fxReportParserError(parser, parser->line, "missing expression");
        }
		else /*if (aCount > 1)*/ {
			fxPushNodeList(parser, aCount);
			fxPushNodeStruct(parser, 1, XS_TOKEN_EXPRESSIONS, aLine);
		}
	}
	parser->flags |= formerAwaitingYieldingFlags;
}

void fxNewExpression(txParser* parser)
{
	txInteger aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_NEW);
	if (parser->token == XS_TOKEN_DOT) {
		fxGetNextToken(parser);
		if (fxIsKeyword(parser, parser->targetSymbol)) {
			if (!(parser->flags & mxTargetFlag))
				fxReportParserError(parser, parser->line, "invalid new.target");
			fxGetNextToken(parser);
			fxPushNodeStruct(parser, 0, XS_TOKEN_TARGET, aLine);
		}
		else
			fxReportParserError(parser, parser->line, "missing target");
		return;
	}
	fxLiteralExpression(parser, 1);
	fxCheckArrowFunction(parser, 1);
	for (;;) {
		txInteger aMemberLine = parser->line;
		if (parser->token == XS_TOKEN_DOT) {
			fxGetNextToken(parser);
			if (parser->token == XS_TOKEN_IDENTIFIER) {
				fxPushSymbol(parser, parser->symbol);
				fxPushNodeStruct(parser, 2, XS_TOKEN_MEMBER, aMemberLine);
				fxGetNextToken(parser);
			}
			else if (parser->token == XS_TOKEN_PRIVATE_IDENTIFIER) {
				fxPushSymbol(parser, parser->symbol);
				fxSwapNodes(parser);
				fxPushNodeStruct(parser, 2, XS_TOKEN_PRIVATE_MEMBER, aMemberLine);
				fxGetNextToken(parser);
			}
			else
				fxReportParserError(parser, parser->line, "missing property");
		}
		else if (parser->token == XS_TOKEN_LEFT_BRACKET) {
			fxGetNextToken(parser);
			fxCommaExpression(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_MEMBER_AT, aMemberLine);
			fxMatchToken(parser, XS_TOKEN_RIGHT_BRACKET);
		}
		else if (parser->token == XS_TOKEN_TEMPLATE) {
			fxPushStringNode(parser, parser->stringLength, parser->string, aLine);
			fxPushRawNode(parser, parser->rawLength, parser->raw, aLine);
			fxPushNodeStruct(parser, 2, XS_TOKEN_TEMPLATE_MIDDLE, aLine);
			fxGetNextToken(parser);
			fxPushNodeList(parser, 1);
			fxPushNodeStruct(parser, 2, XS_TOKEN_TEMPLATE, aLine);
		}
		else if (parser->token == XS_TOKEN_TEMPLATE_HEAD) {
			fxTemplateExpression(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_TEMPLATE, aLine);
		}
		else
			break;
	} 
	if (parser->token == XS_TOKEN_LEFT_PARENTHESIS)
		fxParameters(parser);
	else {
		fxPushNodeList(parser, 0);
		fxPushNodeStruct(parser, 1, XS_TOKEN_PARAMS, aLine);
	}
	fxPushNodeStruct(parser, 2, XS_TOKEN_NEW, aLine);
}

void fxObjectExpression(txParser* parser)
{
	txInteger aCount = 0;
	txSymbol* aSymbol;
	txToken aToken0;
	txToken aToken1;
	txToken aToken2;
	txInteger aLine = parser->line;
	txNode* base = parser->root;
	fxMatchToken(parser, XS_TOKEN_LEFT_BRACE);
	for (;;) {
		txInteger aPropertyLine = parser->line;
		txUnsigned flags = 0;
        if (parser->token == XS_TOKEN_RIGHT_BRACE)
            break;
        if (parser->token == XS_TOKEN_SPREAD) {
			fxGetNextToken(parser);
			fxAssignmentExpression(parser);
			fxPushNodeStruct(parser, 1, XS_TOKEN_SPREAD, aPropertyLine);
        }
        else {
			fxPropertyName(parser, &aSymbol, &aToken0, &aToken1, &aToken2, &flags);
			if (aToken1 == XS_TOKEN_PRIVATE_PROPERTY) {
				fxReportParserError(parser, parser->line, "invalid private property");
			}
			else if ((aToken2 == XS_TOKEN_GETTER) || (aToken2 == XS_TOKEN_SETTER)) {
				flags |= mxShorthandFlag;
				if (aToken2 == XS_TOKEN_GETTER)
					flags |= mxGetterFlag;
				else if (aToken2 == XS_TOKEN_SETTER)
					flags |= mxSetterFlag;
				if (parser->token == XS_TOKEN_LEFT_PARENTHESIS)
					fxFunctionExpression(parser, aPropertyLine, C_NULL, mxSuperFlag);
				else
					fxReportParserError(parser, parser->line, "missing (");
			}
			else if (aToken2 == XS_TOKEN_GENERATOR) {
				flags |= mxShorthandFlag | mxMethodFlag;
				if (parser->token == XS_TOKEN_LEFT_PARENTHESIS)
					fxGeneratorExpression(parser, aPropertyLine, C_NULL, mxSuperFlag | flags);
				else
					fxReportParserError(parser, parser->line, "missing (");
			}
			else if (aToken2 == XS_TOKEN_FUNCTION) {
				flags |= mxShorthandFlag | mxMethodFlag;
				if (parser->token == XS_TOKEN_LEFT_PARENTHESIS)
					fxFunctionExpression(parser, aPropertyLine, C_NULL, mxSuperFlag | flags);
				else
					fxReportParserError(parser, parser->line, "missing (");
			}
			else if (parser->token == XS_TOKEN_LEFT_PARENTHESIS) {
				flags |= mxShorthandFlag | mxMethodFlag;
				fxFunctionExpression(parser, aPropertyLine, C_NULL, mxSuperFlag | flags);
			}
			else if (parser->token == XS_TOKEN_COLON) {
				fxGetNextToken(parser);
				fxAssignmentExpression(parser);
			}
			else if (aToken1 == XS_TOKEN_PROPERTY) {
				flags |= mxShorthandFlag;
				fxPushSymbol(parser, aSymbol);
				if (parser->token == XS_TOKEN_ASSIGN) {
					fxPushNodeStruct(parser, 1, XS_TOKEN_ACCESS, aPropertyLine);
					fxGetNextToken(parser);
					fxAssignmentExpression(parser);
					fxPushNodeStruct(parser, 2, XS_TOKEN_BINDING, aPropertyLine);
				}
				else if (aToken0 == XS_TOKEN_IDENTIFIER) {
					fxPushNodeStruct(parser, 1, XS_TOKEN_ACCESS, aPropertyLine);
				}
				else {
					fxReportParserError(parser, parser->line, "invalid identifier");
					fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aPropertyLine);
				}
			}
			else {
				fxReportParserError(parser, parser->line, "missing :");
				fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aPropertyLine);
			}
			fxPushNodeStruct(parser, 2, aToken1, aPropertyLine);
			parser->root->flags |= flags;
			fxCheckUniqueProperty(parser, base, parser->root); 
		}
		aCount++;
        if (parser->token == XS_TOKEN_RIGHT_BRACE)
            break;
		fxMatchToken(parser, XS_TOKEN_COMMA);
	}
	fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
	fxPushNodeList(parser, aCount);
	fxPushNodeStruct(parser, 1, XS_TOKEN_OBJECT, aLine);
}

void fxTemplateExpression(txParser* parser)
{
	txInteger aCount = 0;
	txInteger aLine = parser->line;
	fxPushStringNode(parser, parser->stringLength, parser->string, aLine);
	fxPushRawNode(parser, parser->rawLength, parser->raw, aLine);
	fxPushNodeStruct(parser, 2, XS_TOKEN_TEMPLATE_MIDDLE, aLine);
	aCount++;
	for (;;) {
		fxGetNextToken(parser);
        if (parser->token != XS_TOKEN_RIGHT_BRACE) {
            fxCommaExpression(parser);
            aCount++;
        }
		if (parser->token != XS_TOKEN_RIGHT_BRACE) {
			fxReportParserError(parser, parser->line, "missing }");
		}
		fxGetNextTokenTemplate(parser);
		fxPushStringNode(parser, parser->stringLength, parser->string, aLine);
		fxPushRawNode(parser, parser->rawLength, parser->raw, aLine);
		fxPushNodeStruct(parser, 2, XS_TOKEN_TEMPLATE_MIDDLE, aLine);
		aCount++;
		if (parser->token == XS_TOKEN_TEMPLATE_TAIL) {
			fxGetNextToken(parser);
			break;
		}
	}
	fxPushNodeList(parser, aCount);
}

void fxYieldExpression(txParser* parser)
{
	txInteger aLine = parser->line;
	if (!(parser->flags & mxYieldFlag))
		fxReportParserError(parser, parser->line, "invalid yield");
	else
		parser->flags |= mxYieldingFlag;
	fxMatchToken(parser, XS_TOKEN_YIELD);
	if ((!parser->crlf) && (parser->token == XS_TOKEN_MULTIPLY)) {
		fxGetNextToken(parser);
		fxAssignmentExpression(parser);
		fxPushNodeStruct(parser, 1, XS_TOKEN_DELEGATE, aLine);
		return;
	}
	if ((!parser->crlf) && (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION)) {
		fxAssignmentExpression(parser);
	}
	else {
		fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aLine);
	}
	fxPushNodeStruct(parser, 1, XS_TOKEN_YIELD, aLine);
}

void fxParameters(txParser* parser)
{
	txInteger aCount = 0;
	txInteger aLine = parser->line;
	txBoolean aSpreadFlag = 0;
	fxMatchToken(parser, XS_TOKEN_LEFT_PARENTHESIS);
	while ((parser->token == XS_TOKEN_SPREAD) || (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION)) {
		txInteger aParamLine = parser->line;
		if (parser->token == XS_TOKEN_SPREAD) {
			fxGetNextToken(parser);
			fxAssignmentExpression(parser);
			fxPushNodeStruct(parser, 1, XS_TOKEN_SPREAD, aParamLine);
			aSpreadFlag = 1;
		}
		else
			fxAssignmentExpression(parser);
		aCount++;
		if (parser->token != XS_TOKEN_RIGHT_PARENTHESIS)
			fxMatchToken(parser, XS_TOKEN_COMMA);
	}
	fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
	fxPushNodeList(parser, aCount);
	fxPushNodeStruct(parser, 1, XS_TOKEN_PARAMS, aLine);
	if (aSpreadFlag)
		parser->root->flags |= mxSpreadFlag;
}

void fxPropertyName(txParser* parser, txSymbol** theSymbol, txToken* theToken0, txToken* theToken1, txToken* theToken2, txUnsigned* flag)
{
	txSymbol* aSymbol = C_NULL;
	txToken aToken0 = XS_NO_TOKEN;
	txToken aToken1 = XS_NO_TOKEN;
	txToken aToken2 = XS_NO_TOKEN;
	txInteger aLine = parser->line;
	txIndex index;
	*flag = 0;
	fxGetNextToken2(parser);
	aToken0 = parser->token;
	if ((gxTokenFlags[aToken0] & XS_TOKEN_IDENTIFIER_NAME)) {
		aSymbol = parser->symbol;
		if (parser->token2 == XS_TOKEN_COLON) {
			fxPushSymbol(parser, aSymbol);
			aToken1 = XS_TOKEN_PROPERTY;
		}
		else if (fxIsKeyword(parser, parser->asyncSymbol) && (!parser->crlf2)) {
			*flag = mxAsyncFlag;
			fxGetNextToken(parser);
			if (parser->token == XS_TOKEN_MULTIPLY) {
				aToken2 = XS_TOKEN_GENERATOR;
				fxGetNextToken(parser);
			}
			else
				aToken2 = XS_TOKEN_FUNCTION;
		}
		else if (fxIsKeyword(parser, parser->getSymbol)) {
			aToken2 = XS_TOKEN_GETTER;
			fxGetNextToken(parser);
		}
		else if (fxIsKeyword(parser, parser->setSymbol)) {
			aToken2 = XS_TOKEN_SETTER;
			fxGetNextToken(parser);
		}
		else {
			fxPushSymbol(parser, aSymbol);
			aToken1 = XS_TOKEN_PROPERTY;
		}
	}
	else if (parser->token == XS_TOKEN_MULTIPLY) {
		aToken2 = XS_TOKEN_GENERATOR;
		fxGetNextToken(parser);
	}
	else if (parser->token == XS_TOKEN_PRIVATE_IDENTIFIER) {
		aSymbol = parser->symbol;
		fxPushSymbol(parser, aSymbol);
		aToken1 = XS_TOKEN_PRIVATE_PROPERTY;
	}
	else if (parser->token == XS_TOKEN_INTEGER) {
		if (fxIntegerToIndex(parser->dtoa, parser->integer, &index)) {
			fxPushIndexNode(parser, index, aLine);
			aToken1 = XS_TOKEN_PROPERTY_AT;
		}
		else {
			aSymbol = fxNewParserSymbol(parser, fxIntegerToString(parser->dtoa, parser->integer, parser->buffer, parser->bufferSize));
			fxPushSymbol(parser, aSymbol);
			aToken1 = XS_TOKEN_PROPERTY;
		}
	}
	else if (parser->token == XS_TOKEN_NUMBER) {
		if (fxNumberToIndex(parser->dtoa, parser->number, &index)) {
			fxPushIndexNode(parser, index, aLine);
			aToken1 = XS_TOKEN_PROPERTY_AT;
		}
		else {
			aSymbol = fxNewParserSymbol(parser, fxNumberToString(parser->dtoa, parser->number, parser->buffer, parser->bufferSize, 0, 0));
			fxPushSymbol(parser, aSymbol);
			aToken1 = XS_TOKEN_PROPERTY;
		}
	}
	else if (parser->token == XS_TOKEN_STRING) {
		if (fxStringToIndex(parser->dtoa, parser->string, &index)) {
			fxPushIndexNode(parser, index, aLine);
			aToken1 = XS_TOKEN_PROPERTY_AT;
		}
		else {
			aSymbol = fxNewParserSymbol(parser, parser->string);
			fxPushSymbol(parser, aSymbol);
			aToken1 = XS_TOKEN_PROPERTY;
		}
	}
	else if (parser->token == XS_TOKEN_LEFT_BRACKET) {
		fxGetNextToken(parser);
		fxCommaExpression(parser);
		if (parser->token != XS_TOKEN_RIGHT_BRACKET) {
			fxReportParserError(parser, parser->line, "missing ]");
		}
		aToken1 = XS_TOKEN_PROPERTY_AT;
	}
	else {
		fxReportParserError(parser, parser->line, "missing identifier");
		fxPushNULL(parser);
	}
	if (aToken2 != XS_NO_TOKEN) {
		if ((gxTokenFlags[parser->token] & XS_TOKEN_IDENTIFIER_NAME)) {
			aSymbol = parser->symbol;
			fxPushSymbol(parser, aSymbol);
			aToken1 = XS_TOKEN_PROPERTY;
			fxGetNextToken(parser);
		}
		else if (parser->token == XS_TOKEN_PRIVATE_IDENTIFIER) {
			aSymbol = parser->symbol;
			fxPushSymbol(parser, aSymbol);
			aToken1 = XS_TOKEN_PRIVATE_PROPERTY;
			fxGetNextToken(parser);
		}
		else if (parser->token == XS_TOKEN_INTEGER) {
			if (fxIntegerToIndex(parser->dtoa, parser->integer, &index)) {
				fxPushIndexNode(parser, index, aLine);
				aToken1 = XS_TOKEN_PROPERTY_AT;
			}
			else {
				aSymbol = fxNewParserSymbol(parser, fxIntegerToString(parser->dtoa, parser->integer, parser->buffer, parser->bufferSize));
				fxPushSymbol(parser, aSymbol);
				aToken1 = XS_TOKEN_PROPERTY;
			}
			fxGetNextToken(parser);
		}
		else if (parser->token == XS_TOKEN_NUMBER) {
			if (fxNumberToIndex(parser->dtoa, parser->number, &index)) {
				fxPushIndexNode(parser, index, aLine);
				aToken1 = XS_TOKEN_PROPERTY_AT;
			}
			else {
				aSymbol = fxNewParserSymbol(parser, fxNumberToString(parser->dtoa, parser->number, parser->buffer, parser->bufferSize, 0, 0));
				fxPushSymbol(parser, aSymbol);
				aToken1 = XS_TOKEN_PROPERTY;
			}
			fxGetNextToken(parser);
		}
		else if (parser->token == XS_TOKEN_STRING) {
			if (fxStringToIndex(parser->dtoa, parser->string, &index)) {
				fxPushIndexNode(parser, index, aLine);
				aToken1 = XS_TOKEN_PROPERTY_AT;
			}
			else {
				aSymbol = fxNewParserSymbol(parser, parser->string);
				fxPushSymbol(parser, aSymbol);
				aToken1 = XS_TOKEN_PROPERTY;
			}
			fxGetNextToken(parser);
		}
		else if (parser->token == XS_TOKEN_LEFT_BRACKET) {
			fxGetNextToken(parser);
			fxCommaExpression(parser);
			if (parser->token != XS_TOKEN_RIGHT_BRACKET) {
				fxReportParserError(parser, parser->line, "missing ]");
			}
			aToken1 = XS_TOKEN_PROPERTY_AT;
			fxGetNextToken(parser);
		}
		else if (aToken2 == XS_TOKEN_GETTER) {
			fxPushSymbol(parser, aSymbol);
			aToken1 = XS_TOKEN_PROPERTY;
			aToken2 = XS_NO_TOKEN;
		}
		else if (aToken2 == XS_TOKEN_SETTER) {
			fxPushSymbol(parser, aSymbol);
			aToken1 = XS_TOKEN_PROPERTY;
			aToken2 = XS_NO_TOKEN;
		}
		else {
			fxReportParserError(parser, parser->line, "missing identifier");
			fxPushNULL(parser);
		}
	}
	else
		fxGetNextToken(parser);
	*theSymbol = aSymbol;
	*theToken0 = aToken0;
	*theToken1 = aToken1;
	*theToken2 = aToken2;
}

void fxBinding(txParser* parser, txToken theToken, txFlag initializeIt)
{
	txInteger aLine = parser->line;
	fxCheckParserStack(parser, aLine);
	if (parser->token == XS_TOKEN_IDENTIFIER) {
		fxCheckStrictSymbol(parser, parser->symbol);
		if (((theToken == XS_TOKEN_CONST) || (theToken == XS_TOKEN_LET)) && (parser->symbol == parser->letSymbol))
			fxReportParserError(parser, parser->line, "invalid identifier");
		fxPushSymbol(parser, parser->symbol);
		fxPushNodeStruct(parser, 1, theToken, aLine);
		fxGetNextToken(parser);
	}
	else if (parser->token == XS_TOKEN_LEFT_BRACE) {
		fxObjectBinding(parser, theToken);
	}
	else if (parser->token == XS_TOKEN_LEFT_BRACKET) {
		fxArrayBinding(parser, theToken);
	}
	else {
		fxReportParserError(parser, parser->line, "missing identifier");
		fxPushNULL(parser);
	}
	if (initializeIt && (parser->token == XS_TOKEN_ASSIGN)) {
		parser->flags &= ~mxForFlag;
		fxGetNextToken(parser);
		fxAssignmentExpression(parser);
		fxPushNodeStruct(parser, 2, XS_TOKEN_BINDING, aLine);
	}
}

txNode* fxBindingFromExpression(txParser* parser, txNode* theNode, txToken theToken)
{
	txToken aToken = (theNode && theNode->description) ? theNode->description->token : XS_NO_TOKEN;
	txNode* binding;
again:
	if (aToken == XS_TOKEN_EXPRESSIONS) {
		txNode* item = ((txExpressionsNode*)theNode)->items->first;
		if (item && !item->next) {
			aToken = item->description->token;
			if ((aToken == XS_TOKEN_ACCESS) || (aToken == XS_TOKEN_MEMBER) || (aToken == XS_TOKEN_MEMBER_AT) || (aToken == XS_TOKEN_PRIVATE_MEMBER) || (aToken == XS_TOKEN_UNDEFINED)) {
				item->next = theNode->next;
				theNode = item;
			}
			else if (aToken == XS_TOKEN_EXPRESSIONS) {
				item->next = theNode->next;
				theNode = item;
				goto again;
			}
			else
				return NULL;
		}
	}
	if (aToken == XS_TOKEN_BINDING) {
		binding = fxBindingFromExpression(parser, ((txBindingNode*)theNode)->target, theToken);
		((txBindingNode*)theNode)->target = binding;
// 		fxCheckStrictBinding(parser, theNode);
// 		fxCheckStrictSymbol(parser, ((txBindingNode*)theNode)->symbol);
// 		theNode->description = &gxTokenDescriptions[theToken];
		return theNode;
	}
	if (aToken == XS_TOKEN_ARRAY_BINDING) {
		txNodeList* list = ((txArrayBindingNode*)theNode)->items;
		txNode** address = &(list->first);
		txNode* item;
		while ((item = *address)) {
			txNode* binding = *address = fxBindingFromExpression(parser, item, theToken);
			binding->next = item->next;
			address = &(binding->next);
		}
		return theNode;
	}
	if (aToken == XS_TOKEN_OBJECT_BINDING) {
		txNodeList* list = ((txObjectBindingNode*)theNode)->items;
		txNode** address = &(list->first);
		txNode* item;
		while ((item = *address)) {
			txNode* binding = *address = fxBindingFromExpression(parser, item, theToken);
			binding->next = item->next;
			address = &(binding->next);
		}
		return theNode;
	}
	if (aToken == XS_TOKEN_PROPERTY_BINDING) {
		((txPropertyBindingNode*)theNode)->binding = fxBindingFromExpression(parser, ((txPropertyBindingNode*)theNode)->binding, theToken);
		return theNode;
	}
	if (aToken == XS_TOKEN_PROPERTY_BINDING_AT) {
		((txPropertyBindingAtNode*)theNode)->binding = fxBindingFromExpression(parser, ((txPropertyBindingAtNode*)theNode)->binding, theToken);
		return theNode;
	}
	if (aToken == XS_TOKEN_REST_BINDING) {
		((txRestBindingNode*)theNode)->binding = fxBindingFromExpression(parser, ((txRestBindingNode*)theNode)->binding, theToken);
		return theNode;
	}
	if (aToken == XS_TOKEN_SKIP_BINDING)
		return theNode;
	
	if (aToken == XS_TOKEN_ACCESS) {
		fxCheckStrictSymbol(parser, ((txAccessNode*)theNode)->symbol);
		if (theToken == XS_TOKEN_ACCESS)
			return theNode;
		fxPushSymbol(parser, ((txAccessNode*)theNode)->symbol);
		fxPushNodeStruct(parser, 1, theToken, ((txAccessNode*)theNode)->line);
		return fxPopNode(parser);
	}
	if ((aToken == XS_TOKEN_MEMBER) || (aToken == XS_TOKEN_MEMBER_AT) || (aToken == XS_TOKEN_PRIVATE_MEMBER) || (aToken == XS_TOKEN_UNDEFINED)) {
		return theNode;
	}
	if (aToken == XS_TOKEN_ASSIGN) {
		binding = fxBindingFromExpression(parser, ((txAssignNode*)theNode)->reference, theToken);
		if (!binding)
			return NULL;
		((txBindingNode*)theNode)->description = &gxTokenDescriptions[XS_TOKEN_BINDING];
		((txBindingNode*)theNode)->target = binding;
		((txBindingNode*)theNode)->initializer = ((txAssignNode*)theNode)->value;
		return theNode;
	}
	
	if (aToken == XS_TOKEN_ARRAY)
		return fxArrayBindingFromExpression(parser, theNode, theToken);
	if (aToken == XS_TOKEN_OBJECT)
		return fxObjectBindingFromExpression(parser, theNode, theToken);
	return NULL;
}

void fxArrayBinding(txParser* parser, txToken theToken)
{
	txInteger aCount = 0;
	txInteger aLine = parser->line;
	int elision = 1;
	fxCheckParserStack(parser, aLine);
	fxMatchToken(parser, XS_TOKEN_LEFT_BRACKET);
	while ((parser->token == XS_TOKEN_COMMA) || (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_BINDING)) {
		txInteger anItemLine = parser->line;
		if (parser->token == XS_TOKEN_COMMA) {
			fxGetNextToken(parser);
			if (elision) {
				fxPushNodeStruct(parser, 0, XS_TOKEN_SKIP_BINDING, anItemLine);
				aCount++;
			}
			else
				elision = 1;
		}
		else {
			if (!elision)
				fxReportParserError(parser, parser->line, "missing ,");
			if (parser->token == XS_TOKEN_SPREAD) {
				fxRestBinding(parser, theToken, 0);
				aCount++;
				break;
			}
			fxBinding(parser, theToken, 1);
			aCount++;
			elision = 0;
		}
	}
	fxMatchToken(parser, XS_TOKEN_RIGHT_BRACKET);
	fxPushNodeList(parser, aCount);
	fxPushNodeStruct(parser, 1, XS_TOKEN_ARRAY_BINDING, aLine);
}

txNode* fxArrayBindingFromExpression(txParser* parser, txNode* theNode, txToken theToken)
{
	txNodeList* list = ((txArrayNode*)theNode)->items;
	txNode** address = &(list->first);
	txNode* item;
	txNode* binding;
	while ((item = *address)) {
        if (!item->description) {
            parser->errorSymbol = parser->SyntaxErrorSymbol;
            return NULL;
        }
		if (item->description->token == XS_TOKEN_SPREAD) {
			if (theNode->flags & mxElisionFlag) {
				parser->errorSymbol = parser->SyntaxErrorSymbol;
				return NULL;
			}
			binding = fxRestBindingFromExpression(parser, item, theToken, 0);
			if (!binding) {
				parser->errorSymbol = parser->SyntaxErrorSymbol;
				return NULL;
			}
			break;
		}
		if (item->description->token == XS_TOKEN_ELISION) {
			item->description = &gxTokenDescriptions[XS_TOKEN_SKIP_BINDING];
		}
		else {
// 			if ((theToken == XS_TOKEN_BINDING) && ((item->description->token== XS_TOKEN_MEMBER) || (item->description->token == XS_TOKEN_MEMBER_AT)))
// 				binding = item;
// 			else {			
				binding = fxBindingFromExpression(parser, item, theToken);
				if (!binding) {
					parser->errorSymbol = parser->SyntaxErrorSymbol;
					return NULL;
				}
				binding->next = item->next;
				item = *address = binding;
// 			}
		}
		address = &(item->next);
	}
	fxPushNode(parser, (txNode*)list);
	fxPushNodeStruct(parser, 1, XS_TOKEN_ARRAY_BINDING, theNode->line);
	return fxPopNode(parser);
}

txUnsigned fxObjectBinding(txParser* parser, txToken theToken)
{
	txUnsigned flags = 0;
	txInteger aCount = 0;
	txInteger aLine = parser->line;
	txSymbol* aSymbol;
	txToken aToken;
	txIndex index;
	fxMatchToken(parser, XS_TOKEN_LEFT_BRACE);
	for (;;) {
		txInteger aPropertyLine = parser->line;
		if (parser->token == XS_TOKEN_RIGHT_BRACE)
			break;
		aSymbol = NULL;
		aToken = XS_TOKEN_PROPERTY_BINDING;
		if ((gxTokenFlags[parser->token] & XS_TOKEN_IDENTIFIER_NAME)) {
			aSymbol = parser->symbol;
			fxPushSymbol(parser, aSymbol);
			aToken = XS_TOKEN_PROPERTY_BINDING;
		}
		else if (parser->token == XS_TOKEN_INTEGER) {
			if (fxIntegerToIndex(parser->dtoa, parser->integer, &index)) {
				fxPushIndexNode(parser, index, aPropertyLine);
				aToken = XS_TOKEN_PROPERTY_BINDING_AT;
			}
			else {
				aSymbol = fxNewParserSymbol(parser, fxIntegerToString(parser->dtoa, parser->integer, parser->buffer, parser->bufferSize));
				fxPushSymbol(parser, aSymbol);
				aToken = XS_TOKEN_PROPERTY_BINDING;
			}
		}
		else if (parser->token == XS_TOKEN_NUMBER) {
			if (fxNumberToIndex(parser->dtoa, parser->number, &index)) {
				fxPushIndexNode(parser, index, aPropertyLine);
				aToken = XS_TOKEN_PROPERTY_BINDING_AT;
			}
			else {
				aSymbol = fxNewParserSymbol(parser, fxNumberToString(parser->dtoa, parser->number, parser->buffer, parser->bufferSize, 0, 0));
				fxPushSymbol(parser, aSymbol);
				aToken = XS_TOKEN_PROPERTY_BINDING;
			}
		}
		else if (parser->token == XS_TOKEN_STRING) {
			if (fxStringToIndex(parser->dtoa, parser->string, &index)) {
				fxPushIndexNode(parser, index, aPropertyLine);
				aToken = XS_TOKEN_PROPERTY_BINDING_AT;
			}
			else {
				aSymbol = fxNewParserSymbol(parser, parser->string);
				fxPushSymbol(parser, aSymbol);
				aToken = XS_TOKEN_PROPERTY_BINDING;
			}
		}
		else if (parser->token == XS_TOKEN_LEFT_BRACKET) {
			fxGetNextToken(parser);
			fxCommaExpression(parser);
			if (parser->token != XS_TOKEN_RIGHT_BRACKET) {
				fxReportParserError(parser, parser->line, "missing ]");
			}
			aToken = XS_TOKEN_PROPERTY_BINDING_AT;
		}
		else if (parser->token == XS_TOKEN_SPREAD) {
			flags |= mxSpreadFlag;
			fxRestBinding(parser, theToken, 1);
			aCount++;
            break;
		}
		else {
			fxReportParserError(parser, parser->line, "missing identifier");
			fxPushNULL(parser);
		}
		fxGetNextToken2(parser);
		if (parser->token2 == XS_TOKEN_COLON) {
			fxGetNextToken(parser);
			fxGetNextToken(parser);
			fxBinding(parser, theToken, 1);
		}
		else if (aSymbol) {
			fxBinding(parser, theToken, 1);
		}
		else {
			fxReportParserError(parser, parser->line, "missing :");
			fxPushNULL(parser);
		}
		fxPushNodeStruct(parser, 2, aToken, aPropertyLine);
		aCount++;
        if (parser->token == XS_TOKEN_RIGHT_BRACE)
            break;
		if (parser->token == XS_TOKEN_COMMA)
			fxGetNextToken(parser);
		else
            break;
	}
	fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
	fxPushNodeList(parser, aCount);
	fxPushNodeStruct(parser, 1, XS_TOKEN_OBJECT_BINDING, aLine);
	parser->root->flags |= flags;
	return flags;
}

txNode* fxObjectBindingFromExpression(txParser* parser, txNode* theNode, txToken theToken)
{
	txNodeList* list = ((txObjectNode*)theNode)->items;
	txNode* property = list->first;
	txNode* binding;
	txUnsigned flags = 0;
	while (property) {
		if (!property->description) {
			parser->errorSymbol = parser->SyntaxErrorSymbol;
			return NULL;
		}
		if (property->description->token == XS_TOKEN_PROPERTY) {
			binding = fxBindingFromExpression(parser, ((txPropertyNode*)property)->value, theToken);
			if (!binding) {
				parser->errorSymbol = parser->SyntaxErrorSymbol;
				return NULL;
			}
			property->description = &gxTokenDescriptions[XS_TOKEN_PROPERTY_BINDING];
			((txPropertyBindingNode*)property)->binding = binding;
		}
		else if (property->description->token == XS_TOKEN_PROPERTY_AT) {
			binding = fxBindingFromExpression(parser, ((txPropertyAtNode*)property)->value, theToken);
			if (!binding) {
				parser->errorSymbol = parser->SyntaxErrorSymbol;
				return NULL;
			}
			property->description = &gxTokenDescriptions[XS_TOKEN_PROPERTY_BINDING_AT];
			((txPropertyBindingAtNode*)property)->binding = binding;
		}
		else if (property->description->token == XS_TOKEN_SPREAD) {
			binding = fxRestBindingFromExpression(parser, property, theToken, 1);
			if (!binding) {
				parser->errorSymbol = parser->SyntaxErrorSymbol;
				return NULL;
			}
			flags |= mxSpreadFlag;
			break;
		}
		property = property->next;
	}
	fxPushNode(parser, (txNode*)list);
	fxPushNodeStruct(parser, 1, XS_TOKEN_OBJECT_BINDING, theNode->line);
	parser->root->flags |= flags;
	return fxPopNode(parser);
}

void fxParametersBinding(txParser* parser)
{
	txInteger aCount = 0;
	txInteger aLine = parser->line;
	txBindingNode* binding;
	if (parser->token == XS_TOKEN_LEFT_PARENTHESIS) {
		fxGetNextToken(parser);
		while (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_BINDING) {
			if (parser->token == XS_TOKEN_SPREAD) {
				parser->flags |= mxNotSimpleParametersFlag;
				fxRestBinding(parser, XS_TOKEN_ARG, 0);
				aCount++;
				break;
			}
			fxBinding(parser, XS_TOKEN_ARG, 1);
			binding = (txBindingNode*)parser->root;
			if (binding->description->token != XS_TOKEN_ARG)
				parser->flags |= mxNotSimpleParametersFlag;
			aCount++;
			if (parser->token != XS_TOKEN_RIGHT_PARENTHESIS)
				fxMatchToken(parser, XS_TOKEN_COMMA);
		}
		fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
	}
	else
		fxReportParserError(parser, parser->line, "missing (");
	fxPushNodeList(parser, aCount);
	fxPushNodeStruct(parser, 1, XS_TOKEN_PARAMS_BINDING, aLine);
}

txNode* fxParametersBindingFromExpressions(txParser* parser, txNode* theNode)
{
	txNodeList* list = ((txParamsNode*)theNode)->items;
	txNode** address = &(list->first);
	txNode* item;
	txBindingNode* binding;
	while ((item = *address)) {
		txToken aToken = (item && item->description) ? item->description->token : XS_NO_TOKEN;
		if (aToken == XS_TOKEN_SPREAD) {
			binding = (txBindingNode*)fxRestBindingFromExpression(parser, item, XS_TOKEN_ARG, 0);
			if (!binding)
				return NULL;
			parser->flags |= mxNotSimpleParametersFlag;
			break;
		}
		binding = (txBindingNode*)fxBindingFromExpression(parser, item, XS_TOKEN_ARG);
		if (!binding)
			return NULL;
		if (binding->description->token != XS_TOKEN_ARG)
			parser->flags |= mxNotSimpleParametersFlag;
		binding->next = item->next;
		item = *address = (txNode*)binding;
		address = &(item->next);
	}
	theNode->description = &gxTokenDescriptions[XS_TOKEN_PARAMS_BINDING];
	return theNode;
}

void fxRestBinding(txParser* parser, txToken theToken, txUnsigned flag)
{
	txInteger aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_SPREAD);
	fxBinding(parser, theToken, 0);
	if (flag && ((parser->root->description->token == XS_TOKEN_ARRAY_BINDING) || (parser->root->description->token == XS_TOKEN_OBJECT_BINDING))) {
		fxReportParserError(parser, parser->line, "invalid rest");
	}
	fxPushNodeStruct(parser, 1, XS_TOKEN_REST_BINDING, aLine);
}

txNode* fxRestBindingFromExpression(txParser* parser, txNode* theNode, txToken theToken, txUnsigned flag)
{
	txNode* expression = ((txSpreadNode*)theNode)->expression;
	txNode* binding;
	if (theNode->next) {
		parser->errorSymbol = parser->SyntaxErrorSymbol;
		return NULL;
	}
	if (!expression)
		return NULL;
// 	if ((theToken == XS_TOKEN_BINDING) && ((expression->description->token == XS_TOKEN_MEMBER) || (expression->description->token == XS_TOKEN_MEMBER_AT)))
// 		binding = expression;
// 	else {			
		binding = fxBindingFromExpression(parser, expression, theToken);
		if (!binding) {
			parser->errorSymbol = parser->SyntaxErrorSymbol;
			return NULL;
		}
		if (binding->description->token == XS_TOKEN_BINDING) {
			fxReportParserError(parser, parser->line, "invalid rest");
			return NULL;
		}
		if (flag && ((binding->description->token == XS_TOKEN_ARRAY_BINDING) || (binding->description->token == XS_TOKEN_OBJECT_BINDING))) {
			fxReportParserError(parser, parser->line, "invalid rest");
			return NULL;
		}
// 	}
	theNode->description = &gxTokenDescriptions[XS_TOKEN_REST_BINDING];
	((txRestBindingNode*)theNode)->binding = binding;
	return theNode;
}

void fxCheckArrowFunction(txParser* parser, txInteger count)
{
	txNode* node = parser->root;
	while (count) {
		if (node->flags & mxArrowFlag)
			fxReportParserError(parser, parser->line, "invalid arrow function");
		count--;
		node = node->next;
	}
}

txBoolean fxCheckReference(txParser* parser, txToken theToken)
{
	txNode* node = parser->root;
	txToken aToken = (node && node->description) ? node->description->token : XS_NO_TOKEN;
	if (aToken == XS_TOKEN_EXPRESSIONS) {
		txNode* item = ((txExpressionsNode*)node)->items->first;
		if (item && !item->next) {
			aToken = (item->description) ? item->description->token : XS_NO_TOKEN;
			if ((aToken == XS_TOKEN_ACCESS) || (aToken == XS_TOKEN_MEMBER) || (aToken == XS_TOKEN_MEMBER_AT) || (aToken == XS_TOKEN_PRIVATE_MEMBER) || (aToken == XS_TOKEN_UNDEFINED)) {
				item->next = node->next;
				node = parser->root = item;
			}
			else
				aToken = XS_TOKEN_EXPRESSIONS;
		}
	}
	if (aToken == XS_TOKEN_ACCESS) {
		fxCheckStrictSymbol(parser, ((txAccessNode*)node)->symbol);
		return 1;
	}
	if ((aToken == XS_TOKEN_MEMBER) || (aToken == XS_TOKEN_MEMBER_AT) || (aToken == XS_TOKEN_PRIVATE_MEMBER) || (aToken == XS_TOKEN_UNDEFINED))
		return 1;
		
	if (theToken == XS_TOKEN_ASSIGN) {
		if (aToken == XS_TOKEN_ARRAY) {
			txNode* binding = fxArrayBindingFromExpression(parser, node, XS_TOKEN_ACCESS);
            if (binding) {
                binding->next = node->next;
                parser->root = binding;
 				return 1;
           }
       	}
		else if (aToken == XS_TOKEN_OBJECT) {
			txNode* binding = fxObjectBindingFromExpression(parser, node, XS_TOKEN_ACCESS);
            if (binding) {
                binding->next = node->next;
                parser->root = binding;
 				return 1;
            }
        }
	}
	else if (theToken == XS_TOKEN_DELETE)
		return 1;
	return 0;
}

void fxCheckStrictBinding(txParser* parser, txNode* node)
{
	if (node && node->description) {
		if (node->description->token == XS_TOKEN_ACCESS) {
			fxCheckStrictSymbol(parser, ((txAccessNode*)node)->symbol);
		}
		else if (node->description->token == XS_TOKEN_ARG) {
			fxCheckStrictSymbol(parser, ((txDeclareNode*)node)->symbol);
		}
		else if (node->description->token == XS_TOKEN_CONST) {
			fxCheckStrictSymbol(parser, ((txDeclareNode*)node)->symbol);
		}
		else if (node->description->token == XS_TOKEN_LET) {
			fxCheckStrictSymbol(parser, ((txDeclareNode*)node)->symbol);
		}
		else if (node->description->token == XS_TOKEN_VAR) {
			fxCheckStrictSymbol(parser, ((txDeclareNode*)node)->symbol);
		}
		else if (node->description->token == XS_TOKEN_BINDING) {
			fxCheckStrictBinding(parser, ((txBindingNode*)node)->target);
		}
		else if (node->description->token == XS_TOKEN_ARRAY_BINDING) {
			node = ((txArrayBindingNode*)node)->items->first;
			while (node) {
				fxCheckStrictBinding(parser, node);
				node = node->next;
			}
		}
		else if (node->description->token == XS_TOKEN_OBJECT_BINDING) {
			node = ((txObjectBindingNode*)node)->items->first;
			while (node) {
				fxCheckStrictBinding(parser, node);
				node = node->next;
			}
		}
		else if (node->description->token == XS_TOKEN_PARAMS_BINDING) {
			node = ((txParamsBindingNode*)node)->items->first;
			while (node) {
				fxCheckStrictBinding(parser, node);
				node = node->next;
			}
		}
		else if (node->description->token == XS_TOKEN_PROPERTY_BINDING)
			fxCheckStrictBinding(parser, ((txPropertyBindingNode*)node)->binding);
		else if (node->description->token == XS_TOKEN_PROPERTY_BINDING_AT)
			fxCheckStrictBinding(parser, ((txPropertyBindingAtNode*)node)->binding);
		else if (node->description->token == XS_TOKEN_REST_BINDING)
			fxCheckStrictBinding(parser, ((txRestBindingNode*)node)->binding);
	}
}

void fxCheckStrictFunction(txParser* parser, txFunctionNode* function)
{
	parser->line = function->line;
	fxCheckStrictSymbol(parser, function->symbol);
	fxCheckStrictBinding(parser, function->params);
}

void fxCheckStrictSymbol(txParser* parser, txSymbol* symbol)
{
	if (parser->flags & mxStrictFlag) {
		if (symbol == parser->argumentsSymbol)
			fxReportParserError(parser, parser->line, "invalid arguments (strict mode)");
		else if (symbol == parser->evalSymbol)
			fxReportParserError(parser, parser->line, "invalid eval (strict mode)");
		else if (symbol == parser->yieldSymbol)
			fxReportParserError(parser, parser->line, "invalid yield (strict mode)");
	}
	else if (parser->flags & mxYieldFlag) {
		if (symbol == parser->yieldSymbol)
			fxReportParserError(parser, parser->line, "invalid yield");
	}
}

void fxCheckUniqueProperty(txParser* parser, txNode* base, txNode* current)
{
	return; // no more!
	if (current->description->token == XS_TOKEN_PROPERTY) {
		txPropertyNode* currentNode = (txPropertyNode*)current;
		while (base != current) {
			txPropertyNode* baseNode = (txPropertyNode*)base;
			if (baseNode->description->token == XS_TOKEN_PROPERTY) {
				if (baseNode->symbol == currentNode->symbol) {
					fxCheckUniquePropertyAux(parser, (txNode*)baseNode, (txNode*)currentNode);			
				}
			}
			base = base->next;
		}
	}
	else {
		txPropertyAtNode* currentNode = (txPropertyAtNode*)current;
		txIntegerNode* currentAt = (txIntegerNode*)(currentNode->at);
		if (currentAt->description->token == XS_TOKEN_INTEGER) {
			while (base != current) {
				txPropertyAtNode* baseNode = (txPropertyAtNode*)base;
				if (baseNode->description->token == XS_TOKEN_PROPERTY_AT) {
					txIntegerNode* baseAt = (txIntegerNode*)(baseNode->at);
					if (baseAt->description->token == XS_TOKEN_INTEGER) {
						if (baseAt->value == currentAt->value) {
							fxCheckUniquePropertyAux(parser, (txNode*)baseNode, (txNode*)currentNode);			
						}
					}
				}
				base = base->next;
			}
		}
	}
}

void fxCheckUniquePropertyAux(txParser* parser, txNode* baseNode, txNode* currentNode)
{
	if (currentNode->flags & mxGetterFlag) {
		if (baseNode->flags & mxGetterFlag)
			fxReportParserError(parser, parser->line, "getter already defined");
		else if (!(baseNode->flags & mxSetterFlag))
			fxReportParserError(parser, parser->line, "property already defined");
	}
	else if (currentNode->flags & mxSetterFlag) {
		if (baseNode->flags & mxSetterFlag)
			fxReportParserError(parser, parser->line, "setter already defined");
		else if (!(baseNode->flags & mxGetterFlag))
			fxReportParserError(parser, parser->line, "property already defined");
	}
	else {
		if (baseNode->flags & mxGetterFlag)
			fxReportParserError(parser, parser->line, "getter already defined");
		else if (baseNode->flags & mxSetterFlag)
			fxReportParserError(parser, parser->line, "setter already defined");
		else if (parser->flags & mxStrictFlag)
			fxReportParserError(parser, parser->line, "property already defined (strict mode)");
	}
}


void fxJSONValue(txParser* parser)
{
	switch (parser->token) {
	case XS_TOKEN_FALSE:
		fxPushNodeStruct(parser, 0, parser->token, parser->line);
		fxGetNextTokenJSON(parser);
		break;
	case XS_TOKEN_TRUE:
		fxPushNodeStruct(parser, 0, parser->token, parser->line);
		fxGetNextTokenJSON(parser);
		break;
	case XS_TOKEN_NULL:
		fxPushNodeStruct(parser, 0, parser->token, parser->line);
		fxGetNextTokenJSON(parser);
		break;
	case XS_TOKEN_INTEGER:
		fxPushIntegerNode(parser, parser->integer, parser->line);
		fxGetNextTokenJSON(parser);
		break;
	case XS_TOKEN_NUMBER:
		fxPushNumberNode(parser, parser->number, parser->line);
		fxGetNextTokenJSON(parser);
		break;
	case XS_TOKEN_STRING:
		fxPushStringNode(parser, parser->stringLength, parser->string, parser->line);
		fxGetNextTokenJSON(parser);
		break;
	case XS_TOKEN_LEFT_BRACE:
		fxJSONObject(parser);
		break;
	case XS_TOKEN_LEFT_BRACKET:
		fxJSONArray(parser);
		break;
	default:
		fxPushNULL(parser);
		fxReportParserError(parser, parser->line, "invalid value");
		break;
	}
}

void fxJSONObject(txParser* parser)
{
	txIndex aLength = 0;
	txInteger aLine = parser->line;
	fxGetNextTokenJSON(parser);
	for (;;) {
		if (parser->token == XS_TOKEN_RIGHT_BRACE)
			break;
		if (parser->token != XS_TOKEN_STRING) {
			fxReportParserError(parser, parser->line, "missing name");
			break;
		}
		fxPushStringNode(parser, parser->stringLength, parser->string, parser->line);
		fxGetNextTokenJSON(parser);
		if (parser->token != XS_TOKEN_COLON) {
			fxReportParserError(parser, parser->line, "missing :");
			break;
		}
		fxGetNextTokenJSON(parser);
		fxJSONValue(parser);
		fxPushNodeStruct(parser, 2, XS_TOKEN_PROPERTY_AT, parser->line);
		aLength++;
		if (parser->token != XS_TOKEN_COMMA)
			break;
		fxGetNextTokenJSON(parser);
	}
	if (parser->token != XS_TOKEN_RIGHT_BRACE)
		fxReportParserError(parser, parser->line, "missing }");
	fxGetNextTokenJSON(parser);
	fxPushNodeList(parser, aLength);
	fxPushNodeStruct(parser, 1, XS_TOKEN_OBJECT, aLine);
}

void fxJSONArray(txParser* parser)
{
	txIndex aLength = 0;
	txInteger aLine = parser->line;
	fxGetNextTokenJSON(parser);
	for (;;) {
		if (parser->token == XS_TOKEN_RIGHT_BRACKET)
			break;
		fxJSONValue(parser);
		aLength++;
		if (parser->token != XS_TOKEN_COMMA)
			break;
		fxGetNextTokenJSON(parser);
	}
	if (parser->token != XS_TOKEN_RIGHT_BRACKET)
		fxReportParserError(parser, parser->line, "missing ]");
	fxGetNextTokenJSON(parser);
	fxPushNodeList(parser, aLength);
	fxPushNodeStruct(parser, 1, XS_TOKEN_ARRAY, aLine);
}

void fxJSXAttributeName(txParser* parser)
{
	txSymbol* symbol = parser->symbol;
	fxGetNextToken(parser);
	if (parser->token == XS_TOKEN_COLON) {
		fxGetNextToken(parser);
		if (gxTokenFlags[parser->token] & XS_TOKEN_IDENTIFIER_NAME)
			symbol = fxJSXNamespace(parser, symbol, parser->symbol);
		else
			fxReportParserError(parser, parser->line, "missing name");
	}
	fxPushSymbol(parser, symbol);
}

void fxJSXAttributeValue(txParser* parser)
{
	txInteger line = parser->line;
	fxGetNextTokenJSXAttribute(parser);
	if (parser->token == XS_TOKEN_STRING) {
		fxPushStringNode(parser, parser->stringLength, parser->string, line);
		fxGetNextToken(parser);
	}
	else if (parser->token == XS_TOKEN_LEFT_BRACE) {
		fxGetNextToken(parser);
		if (parser->token == XS_TOKEN_RIGHT_BRACE) {
			fxGetNextToken(parser);
			fxReportParserError(parser, parser->line, "missing expression");
			fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, line);
		}
		else {
			fxAssignmentExpression(parser);
			if (parser->token == XS_TOKEN_RIGHT_BRACE)
				fxGetNextToken(parser);
			else
				fxReportParserError(parser, parser->line, "missing }");
		}
	}
	else {
		fxReportParserError(parser, parser->line, "invalid %s", gxTokenNames[parser->token]);
		fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, line);
	}
}

void fxJSXElement(txParser* parser)
{
	txBoolean closed = 0;
	txInteger line = parser->line;
	txNode* name = NULL;
	txInteger nodeCount = parser->nodeCount;
	txInteger propertyCount = 0;
	fxPushSymbol(parser, parser->__jsx__Symbol);
	fxPushNodeStruct(parser, 1, XS_TOKEN_ACCESS, parser->line);
	if (parser->token == XS_TOKEN_IDENTIFIER) {
		fxJSXElementName(parser);
		name = parser->root;
	}
	else {
		fxReportParserError(parser, parser->line, "missing identifier");
		fxPushNULL(parser);
	}
	for (;;) {
		if (parser->token == XS_TOKEN_MORE)
			break;
		if (parser->token == XS_TOKEN_DIVIDE) {
			fxGetNextToken(parser);
			if (parser->token != XS_TOKEN_MORE)
				fxReportParserError(parser, parser->line, "missing >");
			closed = 1;
			break;
		}
		if (gxTokenFlags[parser->token] & XS_TOKEN_IDENTIFIER_NAME) {
			fxJSXAttributeName(parser);
			if (parser->token == XS_TOKEN_ASSIGN)
				fxJSXAttributeValue(parser);
			else
				fxPushNodeStruct(parser, 0, XS_TOKEN_TRUE, parser->line);
			fxPushNodeStruct(parser, 2, XS_TOKEN_PROPERTY, parser->line);
			propertyCount++;
		}
		else if (parser->token == XS_TOKEN_LEFT_BRACE) {
			fxGetNextToken(parser);
			if (parser->token == XS_TOKEN_SPREAD) {
				fxGetNextToken(parser);
				fxAssignmentExpression(parser);
				if (parser->token == XS_TOKEN_RIGHT_BRACE)
					fxGetNextToken(parser);
				else
					fxReportParserError(parser, parser->line, "missing }");
				fxPushNodeStruct(parser, 1, XS_TOKEN_SPREAD, parser->line);
				propertyCount++;
			}
			else
				fxReportParserError(parser, parser->line, "invalid %s", gxTokenNames[parser->token]);
		}
		else {
			fxReportParserError(parser, parser->line, "invalid %s", gxTokenNames[parser->token]);
			break;
		}
	}
	if (propertyCount > 0) {
		fxPushNodeList(parser, propertyCount);
		fxPushNodeStruct(parser, 1, XS_TOKEN_OBJECT, parser->line);
	}
	else
		fxPushNodeStruct(parser, 0, XS_TOKEN_NULL, parser->line);
	if (!closed) {
		for (;;) {
			fxGetNextTokenJSXChild(parser);
			if (parser->stringLength)
				fxPushStringNode(parser, parser->stringLength, parser->string, parser->line);
			if (parser->token == XS_TOKEN_LEFT_BRACE) {
				fxGetNextToken(parser);
				if (parser->token == XS_TOKEN_RIGHT_BRACE)
					fxGetNextToken(parser);
				else {
					fxAssignmentExpression(parser);
					if (parser->token != XS_TOKEN_RIGHT_BRACE)
						fxReportParserError(parser, parser->line, "missing }");
				}
			}
			else if (parser->token == XS_TOKEN_LESS) {
				fxGetNextToken(parser);
				if (parser->token == XS_TOKEN_DIVIDE) {
					fxGetNextToken(parser);
					if (parser->token == XS_TOKEN_IDENTIFIER) {
						fxJSXElementName(parser);
						if (!fxJSXMatch(parser, name, parser->root)) {
							fxReportParserError(parser, parser->line, "invalid element");
							fxPushNULL(parser);
						}
					}
					else {
						fxReportParserError(parser, parser->line, "missing identifier");
						fxPushNULL(parser);
					}
					if (parser->token != XS_TOKEN_MORE)
						fxReportParserError(parser, parser->line, "missing >");
					fxPopNode(parser);
					break;
				}
				else
					fxJSXElement(parser);
			}
			else {
				fxReportParserError(parser, parser->line, "invalid %s", gxTokenNames[parser->token]);
				break;
			}
		}
	}
	fxPushNodeList(parser, parser->nodeCount - nodeCount - 1);
	fxPushNodeStruct(parser, 1, XS_TOKEN_PARAMS, line);
	fxPushNodeStruct(parser, 2, XS_TOKEN_CALL, line);
}

void fxJSXElementName(txParser* parser)
{
	txInteger line = parser->line;
	txSymbol* symbol = parser->symbol;
	fxGetNextToken(parser);
	if (parser->token == XS_TOKEN_COLON) {
		fxGetNextToken(parser);
		if (parser->token == XS_TOKEN_IDENTIFIER)
			symbol = fxJSXNamespace(parser, symbol, parser->symbol);
		else
			fxReportParserError(parser, parser->line, "missing name");
		fxPushSymbol(parser, symbol);
		fxPushNodeStruct(parser, 1, XS_TOKEN_ACCESS, line);
		fxGetNextToken(parser);
	}
	else {
		fxPushSymbol(parser, symbol);
		fxPushNodeStruct(parser, 1, XS_TOKEN_ACCESS, line);
		while (parser->token == XS_TOKEN_DOT) {
			fxGetNextToken(parser);
			if (parser->token == XS_TOKEN_IDENTIFIER) {
				fxPushSymbol(parser, parser->symbol);
				fxPushNodeStruct(parser, 2, XS_TOKEN_MEMBER, parser->line);
				fxGetNextToken(parser);
			}
			else
				fxReportParserError(parser, parser->line, "missing property");
		}
	}
}

txBoolean fxJSXMatch(txParser* parser, txNode* opening, txNode* closing)
{
	if (opening && closing) {
		while (opening->description->token == XS_TOKEN_MEMBER) {
			if (closing->description->token != XS_TOKEN_MEMBER)
				return 0;
			if (((txMemberNode*)opening)->symbol != ((txMemberNode*)closing)->symbol)
				return 0;
			opening = ((txMemberNode*)opening)->reference;
			closing = ((txMemberNode*)closing)->reference;
		}
		if (opening->description->token == XS_TOKEN_ACCESS) {
			if (closing->description->token != XS_TOKEN_ACCESS)
				return 0;
			if (((txAccessNode*)opening)->symbol != ((txAccessNode*)closing)->symbol)
				return 0;
			return 1;
		}
	}
	return 0;
}

txSymbol* fxJSXNamespace(txParser* parser, txSymbol* namespace, txSymbol* name)
{
	txSize namespaceLength = namespace->length;
	txSize nameLength = name->length;
	txSize length = namespaceLength + 1 + nameLength + 1;
	txString string = fxNewParserChunk(parser, length);
	snprintf(string, length, "%s:%s", namespace->string, name->string);
	return fxNewParserSymbol(parser, string);
}

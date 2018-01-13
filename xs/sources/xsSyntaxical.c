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
static void fxMatchToken(txParser* parser, txToken theToken);
static txNode* fxPopNode(txParser* parser);
static void fxPushNode(txParser* parser, txNode* node);
static void fxPushIndexNode(txParser* parser, txIndex value, txInteger line);
static void fxPushIntegerNode(txParser* parser, txInteger value, txInteger line);
static void fxPushNodeStruct(txParser* parser, txInteger count, txToken token, txInteger line);
static void fxPushNodeList(txParser* parser, txInteger count);
static void fxPushNULL(txParser* parser);
static void fxPushNumberNode(txParser* parser, txNumber value, txInteger line);
static void fxPushStringNode(txParser* parser, txInteger length, txString value, txInteger line);
static void fxPushSymbol(txParser* parser, txSymbol* symbol);
static void fxSwapNodes(txParser* parser);

static void fxExport(txParser* parser);
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

static void fxLiteralExpression(txParser* parser);
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
static void fxObjectBinding(txParser* parser, txToken theToken);
static txNode* fxObjectBindingFromExpression(txParser* parser, txNode* theNode, txToken theToken);
static void fxParametersBinding(txParser* parser);
static txNode* fxParametersBindingFromExpressions(txParser* parser, txNode* theNode);
static void fxRestBinding(txParser* parser, txToken theToken);

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

static txTokenFlag gxTokenFlags[XS_TOKEN_COUNT] = {
	/* XS_NO_TOKEN */ 0,
	/* XS_TOKEN_ACCESS */ 0,
	/* XS_TOKEN_ADD */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_ADDITIVE_EXPRESSION | XS_TOKEN_UNARY_EXPRESSION,
	/* XS_TOKEN_ADD_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_AND */ 0,
	/* XS_TOKEN_ARG */ 0,
	/* XS_TOKEN_ARGUMENTS */ 0,
	/* XS_TOKEN_ARGUMENTS_SLOPPY */ 0,
	/* XS_TOKEN_ARGUMENTS_STRICT */ 0,
	/* XS_TOKEN_ARRAY */ 0,
	/* XS_TOKEN_ARRAY_BINDING */ 0,
	/* XS_TOKEN_ARROW */ 0,
	/* XS_TOKEN_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_AWAIT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_UNARY_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
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
	/* XS_TOKEN_CLASS */ XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
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
	/* XS_TOKEN_DOT */ 0,
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
	/* XS_TOKEN_FINALLY */ XS_TOKEN_IDENTIFIER_NAME | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_FOR */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_FOR_IN */ 0,
	/* XS_TOKEN_FOR_OF */ 0,
	/* XS_TOKEN_FUNCTION */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_GENERATOR */ 0,
	/* XS_TOKEN_GETTER */ 0,
	/* XS_TOKEN_HOST */ XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_IDENTIFIER */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_BEGIN_BINDING | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_IF */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_IMPLEMENTS */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_IMPORT */ XS_TOKEN_IDENTIFIER_NAME | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_IN */ XS_TOKEN_RELATIONAL_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_INCLUDE */ 0,
	/* XS_TOKEN_INCREMENT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_PREFIX_EXPRESSION | XS_TOKEN_POSTFIX_EXPRESSION,
	/* XS_TOKEN_INSTANCEOF */ XS_TOKEN_RELATIONAL_EXPRESSION | XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_INTEGER */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_INTERFACE */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_ITEMS */ 0,
	/* XS_TOKEN_LABEL */ 0,
	/* XS_TOKEN_LEFT_BRACE */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_BEGIN_BINDING,
	/* XS_TOKEN_LEFT_BRACKET */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_BEGIN_BINDING,
	/* XS_TOKEN_LEFT_PARENTHESIS */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION,
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
	/* XS_TOKEN_OR */ 0,
	/* XS_TOKEN_PACKAGE */ XS_TOKEN_IDENTIFIER_NAME,
	/* XS_TOKEN_PARAMS */ 0,
	/* XS_TOKEN_PARAMS_BINDING */ 0,
	/* XS_TOKEN_PLUS */ 0,
	/* XS_TOKEN_PRIVATE */ XS_TOKEN_IDENTIFIER_NAME,
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
	/* XS_TOKEN_TEMPLATE */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_TEMPLATE_HEAD */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION,
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

static txString gxTokenNames[XS_TOKEN_COUNT] = {
	/* XS_NO_TOKEN */ "",
	/* XS_TOKEN_ACCESS */ "access",
	/* XS_TOKEN_ADD */ "+",
	/* XS_TOKEN_ADD_ASSIGN */ "+=",
	/* XS_TOKEN_AND */ "&&",
	/* XS_TOKEN_ARG */ "arg",
	/* XS_TOKEN_ARGUMENTS */ "arguments",
	/* XS_TOKEN_ARGUMENTS_SLOPPY */ "arguments_sloppy",
	/* XS_TOKEN_ARGUMENTS_STRICT */ "arguments_strict",
	/* XS_TOKEN_ARRAY */ "array",
	/* XS_TOKEN_ARRAY_BINDING */ "array_binding",
	/* XS_TOKEN_ARROW */ "=>",
	/* XS_TOKEN_ASSIGN */ "=",
	/* XS_TOKEN_AWAIT */ "await",
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
	/* XS_TOKEN_CLASS */ "class",
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
	/* XS_TOKEN_FINALLY */ "finally",
	/* XS_TOKEN_FOR */ "for",
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
	/* XS_TOKEN_OR */ "||",
	/* XS_TOKEN_PACKAGE */ "package",
	/* XS_TOKEN_PARAMS */ "params",
	/* XS_TOKEN_PARAMS_BINDING */ "params_binding",
	/* XS_TOKEN_PLUS */ "plus",
	/* XS_TOKEN_PRIVATE */ "private",
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
			fxReportParserError(parser, "escaped keyword");
	}
	return result;
}

void fxMatchToken(txParser* parser, txToken theToken)
{
	if (parser->token == theToken)
		fxGetNextToken(parser);
	else
		fxReportParserError(parser, "missing %s", gxTokenNames[theToken]);
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
		fxReportParserError(parser, "invalid %s", gxTokenNames[token]);
		fxThrowParserError(parser, parser->errorCount);
	}
    node = fxNewParserChunkClear(parser, description->size);
	node->description = description;
	node->flags |= parser->flags & (mxStrictFlag | mxGeneratorFlag);
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
    txNodeLink* node = fxNewParserChunk(parser, sizeof(txNodeLink));
	node->description = NULL;	
	node->symbol = NULL;	
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
    txNodeLink* node = fxNewParserChunk(parser, sizeof(txNodeLink));
	node->description = NULL;	
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
	parser->flags |= mxStrictFlag;
	while ((parser->token != XS_TOKEN_EOF) && (parser->token != XS_TOKEN_RIGHT_BRACE)) {
		if (parser->token == XS_TOKEN_EXPORT)
			fxExport(parser);
		else if (parser->token == XS_TOKEN_IMPORT)
			fxImport(parser);
		else if (parser->token == XS_TOKEN_RETURN) {
			fxReportParserError(parser, "invalid return");
			fxGetNextToken(parser);
		}
		else if (parser->token == XS_TOKEN_YIELD) {
			fxReportParserError(parser, "invalid yield");
			fxGetNextToken(parser);
		}
		else
			fxStatement(parser, 1);
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
	parser->root->flags = parser->flags & mxStrictFlag;
}

void fxExport(txParser* parser)
{
	txSymbol* symbol = C_NULL;
	txNode* node;
	txInteger count;
	txInteger line = parser->line;
	txUnsigned flag = 0;
	txToken aToken;
	fxMatchToken(parser, XS_TOKEN_EXPORT);
	switch (parser->token) {
	case XS_TOKEN_MULTIPLY:
		fxGetNextToken(parser);
		if (fxIsKeyword(parser, parser->fromSymbol)) {
			fxGetNextToken(parser);
			if (parser->token == XS_TOKEN_STRING) {
				fxPushNULL(parser);
				fxPushNULL(parser);
				fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, line);
				fxPushNodeList(parser, 1);
				fxPushStringNode(parser, parser->stringLength, parser->string, line);
				fxPushNodeStruct(parser, 2, XS_TOKEN_EXPORT, line);
				fxGetNextToken(parser);
				fxSemicolon(parser);
			}
			else
				fxReportParserError(parser, "missing module");
		}
		else
			fxReportParserError(parser, "missing from");
		break;
	case XS_TOKEN_DEFAULT:
		if (parser->flags & mxDefaultFlag)
			fxReportParserError(parser, "invalid default");
		parser->flags |= mxDefaultFlag;
		fxGetNextToken(parser);
		if (parser->token == XS_TOKEN_CLASS) {
			fxClassExpression(parser, line, &symbol);
			if (symbol)
				fxPushSymbol(parser, symbol);
			else
				fxPushSymbol(parser, parser->defaultSymbol);
			fxPushNULL(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_LET, line);
			fxSwapNodes(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_ASSIGN, line);
			fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, line);
		}
		else if (parser->token == XS_TOKEN_FUNCTION) {
	again:
			fxGetNextToken(parser);
			if (parser->token == XS_TOKEN_MULTIPLY) {
				fxGetNextToken(parser);
				fxGeneratorExpression(parser, line, &symbol, flag);
			}
			else
				fxFunctionExpression(parser, line, &symbol, flag);
			if (symbol)
				fxPushSymbol(parser, symbol);
			else
				fxPushSymbol(parser, parser->defaultSymbol);
			fxSwapNodes(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_DEFINE, line);
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
			fxPushNodeStruct(parser, 2, XS_TOKEN_LET, line);
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
			fxPushNULL(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_LET, line);
			fxSwapNodes(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_ASSIGN, line);
			fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, line);
			
			fxPushSymbol(parser, symbol);
			fxPushNULL(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, line);
			fxPushNodeList(parser, 1);
			fxPushNULL(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_EXPORT, line);
		}
		else
			fxReportParserError(parser, "missing identifier");
		break;
	case XS_TOKEN_FUNCTION:
		fxGetNextToken(parser);
		if (parser->token == XS_TOKEN_MULTIPLY) {
			fxGetNextToken(parser);
			fxGeneratorExpression(parser, line, &symbol, 0);
		}
		else
			fxFunctionExpression(parser, line, &symbol, 0);
		if (symbol) {
			fxPushSymbol(parser, symbol);
			fxSwapNodes(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_DEFINE, line);
			
			fxPushSymbol(parser, symbol);
			fxPushNULL(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, line);
			fxPushNodeList(parser, 1);
			fxPushNULL(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_EXPORT, line);
		}
		else
			fxReportParserError(parser, "missing identifier");
		break;
	case XS_TOKEN_CONST:
	case XS_TOKEN_LET:
	case XS_TOKEN_VAR:
		aToken = parser->token;
		fxVariableStatement(parser, aToken);
		node = parser->root;
		count = 0;
		if (node->description->token == XS_TOKEN_STATEMENTS) {
			node = ((txStatementsNode*)node)->items->first;
			while (node) {
				if (node->description->token == XS_TOKEN_STATEMENT) {
					node = ((txStatementNode*)node)->expression;
					if (node->description->token == XS_TOKEN_ASSIGN)
						node = ((txAssignNode*)node)->reference;
				}	
				if (node->description->token == aToken) {
					symbol = ((txDeclareNode*)node)->symbol;
					fxPushSymbol(parser, symbol);
					fxPushNULL(parser);
					fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, line);
					count++;
				}
				node = node->next;
			}
		}
		else {
			if (node->description->token == XS_TOKEN_STATEMENT) {
				node = ((txStatementNode*)node)->expression;
				if (node->description->token == XS_TOKEN_ASSIGN)
					node = ((txAssignNode*)node)->reference;
			}	
			if (node->description->token == aToken) {	
				symbol = ((txDeclareNode*)node)->symbol;
				fxPushSymbol(parser, symbol);
				fxPushNULL(parser);
				fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, line);
				count++;
			}
		}
		fxPushNodeList(parser, count);
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
				fxReportParserError(parser, "missing module");
			}
		}
		else
			fxPushNULL(parser);
		fxPushNodeStruct(parser, 2, XS_TOKEN_EXPORT, line);
		fxSemicolon(parser);
		break;
	default:
		fxReportParserError(parser, "invalid export %s", gxTokenNames[parser->token]);
		fxGetNextToken(parser);
		break;
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
					fxReportParserError(parser, "missing identifier");
				}
			}
			else {
				fxReportParserError(parser, "missing as");
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
				fxReportParserError(parser, "missing module");
			}
		}
		else {
			fxPushNULL(parser);
			fxReportParserError(parser, "missing from");
		}
	}
	else if (parser->token == XS_TOKEN_STRING) {
		fxPushStringNode(parser, parser->stringLength, parser->string, parser->line);
		fxGetNextToken(parser);
	}
	else {
		fxPushNULL(parser);
		fxReportParserError(parser, "missing module");
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
				fxReportParserError(parser, "missing identifier");
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

void fxCommonModule(txParser* parser)
{
	txInteger count = parser->nodeCount;
	txInteger line = parser->line;

	fxPushSymbol(parser, parser->exportsSymbol);
	fxPushNULL(parser);
	fxPushNodeStruct(parser, 2, XS_TOKEN_VAR, -1);
	fxPushNodeList(parser, 0);
	fxPushNodeStruct(parser, 1, XS_TOKEN_OBJECT, -1);
	fxPushNodeStruct(parser, 2, XS_TOKEN_ASSIGN, -1);
	fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, -1);
	
	fxPushSymbol(parser, parser->moduleSymbol);
	fxPushNULL(parser);
	fxPushNodeStruct(parser, 2, XS_TOKEN_VAR, -1);
	
	fxPushSymbol(parser, parser->exportsSymbol);
	
	fxPushSymbol(parser, parser->exportsSymbol);
	fxPushNodeStruct(parser, 1, XS_TOKEN_ACCESS, -1);
	fxPushNodeStruct(parser, 2, XS_TOKEN_PROPERTY, -1);
	
	fxPushNodeList(parser, 1);
	fxPushNodeStruct(parser, 1, XS_TOKEN_OBJECT, -1);
	fxPushNodeStruct(parser, 2, XS_TOKEN_ASSIGN, -1);
	fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, -1);

	txNode* node;
	while ((parser->token != XS_TOKEN_EOF) && (parser->token != XS_TOKEN_RIGHT_BRACE)) {
		fxStatement(parser, 1);
		node = parser->root;
		if (!node || !node->description || (node->description->token != XS_TOKEN_STATEMENT))
			break;
		node = ((txStatementNode*)node)->expression;
		if (!node || !node->description || (node->description->token != XS_TOKEN_STRING))
			break;
		if (!node->flags && (c_strcmp(((txStringNode*)node)->value, "use strict") == 0)) {
			if (parser->flags & mxNotSimpleParametersFlag)
				fxReportParserError(parser, "invalid directive");
			parser->flags |= mxStrictFlag;
		}
	}
	while ((parser->token != XS_TOKEN_EOF) && (parser->token != XS_TOKEN_RIGHT_BRACE)) {
		if (parser->token == XS_TOKEN_IMPORT)
			fxImport(parser);
		else 
			fxStatement(parser, 1);
	}
	

	fxPushSymbol(parser, parser->defaultSymbol);
	fxPushNULL(parser);
	fxPushNodeStruct(parser, 2, XS_TOKEN_VAR, -1);
	fxPushSymbol(parser, parser->moduleSymbol);
	fxPushNodeStruct(parser, 1, XS_TOKEN_ACCESS, -1);
	fxPushSymbol(parser, parser->exportsSymbol);
	fxPushNodeStruct(parser, 2, XS_TOKEN_MEMBER, -1);
	fxPushNodeStruct(parser, 2, XS_TOKEN_ASSIGN, -1);
	fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, -1);

	fxPushSymbol(parser, parser->defaultSymbol);
	fxPushNULL(parser);
	fxPushNodeStruct(parser, 2, XS_TOKEN_SPECIFIER, -1);
	fxPushNodeList(parser, 1);
	fxPushNULL(parser);
	fxPushNodeStruct(parser, 2, XS_TOKEN_EXPORT, -1);
	
	count = parser->nodeCount - count;
	if (count > 1) {
		fxPushNodeList(parser, count);
		fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENTS, line);
	}
	else if (count == 0) {
		fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, line);
		fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, line);
	}
	
	fxPushNodeStruct(parser, 1, XS_TOKEN_MODULE, line);
	parser->root->flags = parser->flags & mxStrictFlag;
}

void fxProgram(txParser* parser)
{
	txInteger aLine = parser->line;
	fxBody(parser);
	fxPushNodeStruct(parser, 1, XS_TOKEN_PROGRAM, aLine);
	parser->root->flags = parser->flags & mxStrictFlag;
}

void fxBody(txParser* parser)
{
	txInteger count = parser->nodeCount;
	txInteger line = parser->line;
	txNode* node;
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
				fxReportParserError(parser, "invalid directive");
			parser->flags |= mxStrictFlag;
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
	if (count > 1) {
		fxPushNodeList(parser, count);
		fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENTS, line);
	}
	else if (count == 0) {
		fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, line);
		fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, line);
	}
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
			fxReportParserError(parser, "no block");
		fxClassExpression(parser, line, &symbol);
		if (symbol) {
			fxPushSymbol(parser, symbol);
			fxSwapNodes(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_LET, line);
		}
		else
			fxReportParserError(parser, "missing identifier");
		break;
	case XS_TOKEN_CONST:
		if (!blockIt)
			fxReportParserError(parser, "no block");
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
			fxReportParserError(parser, "no block (strict code)");
		fxMatchToken(parser, XS_TOKEN_FUNCTION);
		if (parser->token == XS_TOKEN_MULTIPLY) {
			fxGetNextToken(parser);
			fxGeneratorExpression(parser, line, &symbol, flag);
		}
		else
			fxFunctionExpression(parser, line, &symbol, flag);
		if (symbol) {
			fxPushSymbol(parser, symbol);
			fxSwapNodes(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_DEFINE, line);
		}
		else
			fxReportParserError(parser, "missing identifier");
		break;
	case XS_TOKEN_IF:
		fxIfStatement(parser);
		break;
	case XS_TOKEN_RETURN:
		fxReturnStatement(parser);
		fxSemicolon(parser);
		break;
	case XS_TOKEN_LEFT_BRACE:
		fxBlock(parser);
		break;
	case XS_TOKEN_LET:
		if (!blockIt)
			fxReportParserError(parser, "no block");
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
			fxReportParserError(parser, "with (strict code)");
		fxWithStatement(parser);
		break;
	case XS_TOKEN_IDENTIFIER:
		fxGetNextToken2(parser);
		if (parser->token2 == XS_TOKEN_COLON) {
			fxPushSymbol(parser, parser->symbol);
			fxGetNextToken(parser);
			fxMatchToken(parser, XS_TOKEN_COLON);
			//if ((parser->flags & mxStrictFlag) && (parser->token == XS_TOKEN_FUNCTION)) BROWSER
			//	fxReportParserError(parser, "labeled function (strict code)");
			if (parser->token == XS_TOKEN_FUNCTION)
				fxReportParserError(parser, "labeled function");
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
				fxReportParserError(parser, "no block");
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
			fxReportParserError(parser, "invalid token %s", gxTokenNames[parser->token]);
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
		fxReportParserError(parser, "missing ;");
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
	fxPushNULL(parser);
	fxMatchToken(parser, XS_TOKEN_DO);
	fxStatement(parser, 0);
	fxMatchToken(parser, XS_TOKEN_WHILE);
	fxMatchToken(parser, XS_TOKEN_LEFT_PARENTHESIS);
	fxCommaExpression(parser);
	fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
	fxPushNodeStruct(parser, 2, XS_TOKEN_DO, aLine);
	fxPushNodeStruct(parser, 2, XS_TOKEN_LABEL, aLine);
}

void fxForStatement(txParser* parser)
{
	txInteger aLine = parser->line;
	txBoolean expressionFlag = 0;
	txToken aToken;
	fxPushNULL(parser);
	fxMatchToken(parser, XS_TOKEN_FOR);
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
	if ((parser->token == XS_TOKEN_IN) || fxIsKeyword(parser, parser->ofSymbol)) {
		if (expressionFlag) {
			if (!fxCheckReference(parser, XS_TOKEN_ASSIGN)) {
				fxReportParserError(parser, "no reference");
			}
		}
		else {
			aToken = parser->root->description->token;
			if ((aToken != XS_TOKEN_CONST) && (aToken != XS_TOKEN_LET) && (aToken != XS_TOKEN_VAR) && (aToken != XS_TOKEN_ARRAY_BINDING) && (aToken != XS_TOKEN_OBJECT_BINDING)) {
				fxReportParserError(parser, "no reference %s", gxTokenNames[aToken]);
			}
		}
		aToken = parser->token;
		fxGetNextToken(parser);
		if (aToken == XS_TOKEN_IN)
			fxCommaExpression(parser);
		else
			fxAssignmentExpression(parser);
		fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
		fxStatement(parser, 0);
		if (aToken == XS_TOKEN_IN)
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
	if (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION)
		fxCommaExpression(parser);
	else
		fxReportParserError(parser, "invalid if");
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
		fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aLine);
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
	if (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION)
		fxCommaExpression(parser);
	else
		fxReportParserError(parser, "invalid switch");
	fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
	fxMatchToken(parser, XS_TOKEN_LEFT_BRACE);
	while ((parser->token == XS_TOKEN_CASE) || (parser->token == XS_TOKEN_DEFAULT)) {
		txInteger aCaseCount;
		txInteger aCaseLine = parser->line;
		if (parser->token == XS_TOKEN_CASE) {
			fxGetNextToken(parser);
			if (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION)
				fxCommaExpression(parser);
			else
				fxReportParserError(parser, "invalid case");
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
			fxGetNextToken(parser);
			if (aDefaultFlag) 
				fxReportParserError(parser, "invalid default");
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
		fxReportParserError(parser, "missing expression");
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
		fxGetNextToken(parser);
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
		fxGetNextToken(parser);
		fxBlock(parser);
	}
	else {
		fxPushNULL(parser);
	}
	fxPushNodeStruct(parser, 3, XS_TOKEN_TRY, aLine);
}

void fxVariableStatement(txParser* parser, txToken theToken)
{
	txInteger aCount = 0;
	txInteger aLine = parser->line;
	fxMatchToken(parser, theToken);
	while (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_BINDING) {
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
		}
		else
			break;
	}
	if (aCount == 0) {
		fxPushNULL(parser);
		fxPushNULL(parser);
		fxPushNodeStruct(parser, 2, theToken, aLine);
		fxReportParserError(parser, "missing identifier");
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
	while (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
		fxAssignmentExpression(parser);
		aCount++;
		if (parser->token != XS_TOKEN_COMMA) 
			break;
		fxGetNextToken(parser);
	}
	if (aCount > 1) {
		fxPushNodeList(parser, aCount);
		fxPushNodeStruct(parser, 1, XS_TOKEN_EXPRESSIONS, aLine);
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
				fxReportReferenceError(parser, "no reference");
			fxGetNextToken(parser);
			fxAssignmentExpression(parser);
			fxPushNodeStruct(parser, 2, aToken, aLine);
		}
	}
}

void fxConditionalExpression(txParser* parser)
{
	fxOrExpression(parser);
	if (parser->token == XS_TOKEN_QUESTION_MARK) {
		txInteger aLine = parser->line;
		txUnsigned flags;
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

void fxOrExpression(txParser* parser)
{
	fxAndExpression(parser);
	while (parser->token == XS_TOKEN_OR) {
		txInteger aLine = parser->line;
		fxGetNextToken(parser);
		fxAndExpression(parser);
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
		fxPushNodeStruct(parser, 2, aToken, aLine);
	}
}

void fxRelationalExpression(txParser* parser)
{
	fxShiftExpression(parser);
	if ((parser->flags & mxForFlag) && ((parser->token == XS_TOKEN_IN) || fxIsKeyword(parser, parser->ofSymbol)))
		return;
	while (gxTokenFlags[parser->token] & XS_TOKEN_RELATIONAL_EXPRESSION) {
		txToken aToken = parser->token;
		txInteger aLine = parser->line;
		fxGetNextToken(parser);
		fxShiftExpression(parser);
		fxPushNodeStruct(parser, 2, aToken, aLine);
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
		fxPushNodeStruct(parser, 2, aToken, aLine);
	}
}

void fxExponentiationExpression(txParser* parser)
{
	if (gxTokenFlags[parser->token] & XS_TOKEN_UNARY_EXPRESSION)
		fxUnaryExpression(parser);
	else {
		fxPrefixExpression(parser);
		while (gxTokenFlags[parser->token] & XS_TOKEN_EXPONENTIATION_EXPRESSION) {
			txToken aToken = parser->token;
			txInteger aLine = parser->line;
			fxGetNextToken(parser);
			fxExponentiationExpression(parser);
			fxPushNodeStruct(parser, 2, aToken, aLine);
		}
	}
}

void fxUnaryExpression(txParser* parser)
{
	if (gxTokenFlags[parser->token] & XS_TOKEN_UNARY_EXPRESSION) {
		txToken aToken = parser->token;
		txInteger aLine = parser->line;
		fxGetNextToken(parser);
		fxUnaryExpression(parser);
		if (aToken == XS_TOKEN_ADD)
			fxPushNodeStruct(parser, 1, XS_TOKEN_PLUS, aLine);
		else if (aToken == XS_TOKEN_SUBTRACT)
			fxPushNodeStruct(parser, 1, XS_TOKEN_MINUS, aLine);
		else if (aToken == XS_TOKEN_DELETE) {
			//if ((parser->flags & mxStrictFlag) && (parser->root->description->token == XS_TOKEN_ACCESS)) {
			//	fxReportParserError(parser, "no reference (strict mode)");
			//}
			if (!fxCheckReference(parser, aToken)) 
				fxReportReferenceError(parser, "no reference");
			fxPushNodeStruct(parser, 1, aToken, aLine);
		}
		else if (aToken == XS_TOKEN_AWAIT) {
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
		fxGetNextToken(parser);
		fxPrefixExpression(parser);
		if (!fxCheckReference(parser, aToken)) 
			fxReportReferenceError(parser, "no reference");
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
		if (!fxCheckReference(parser, parser->token)) 
			fxReportReferenceError(parser, "no reference");
		fxPushNodeStruct(parser, 1, parser->token, parser->line);
		fxGetNextToken(parser);
	}
}

void fxCallExpression(txParser* parser)
{
	fxLiteralExpression(parser);
	for (;;) {
		txInteger aLine = parser->line;
		if (parser->token == XS_TOKEN_DOT) {
			fxGetNextToken(parser);
			if (parser->token == XS_TOKEN_IDENTIFIER) {
				fxPushSymbol(parser, parser->symbol);
				fxPushNodeStruct(parser, 2, XS_TOKEN_MEMBER, aLine);
				fxGetNextToken(parser);
			}
			else
				fxReportParserError(parser, "missing property");
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
			fxParameters(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_CALL, aLine);
		}
		else if (parser->token == XS_TOKEN_TEMPLATE) {
			fxPushStringNode(parser, parser->stringLength, parser->string, aLine);
			fxPushStringNode(parser, parser->rawLength, parser->raw, aLine);
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
}

void fxLiteralExpression(txParser* parser)
{
	int escaped;
	txSymbol* aSymbol;
	txInteger aLine = parser->line;
	txUnsigned flags = 0;
	char c = 0;
	switch (parser->token) {
	case XS_TOKEN_NULL:
	case XS_TOKEN_TRUE:
	case XS_TOKEN_FALSE:
		fxPushNodeStruct(parser, 0, parser->token, aLine);
		fxGetNextToken(parser);
		break;
	case XS_TOKEN_SUPER:
		fxGetNextToken(parser);
		if (parser->token == XS_TOKEN_LEFT_PARENTHESIS) {
			if (parser->flags & mxDerivedFlag) {
				fxParameters(parser);
				fxPushNodeStruct(parser, 1, XS_TOKEN_SUPER, aLine);
			}
			else {
				fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aLine);
				fxReportParserError(parser, "invalid super");
			}
		}
		else if ((parser->token == XS_TOKEN_DOT) || (parser->token == XS_TOKEN_LEFT_BRACKET)) {
			if (parser->flags & mxSuperFlag) {
				fxPushNodeStruct(parser, 0, XS_TOKEN_THIS, aLine);
				parser->root->flags |= parser->flags & (mxDerivedFlag | mxSuperFlag);
			}
			else {
				fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aLine);
				fxReportParserError(parser, "invalid super");
			}
		}
		else
			fxReportParserError(parser, "invalid super");
		parser->flags |= mxSuperFlag;
		break;
	case XS_TOKEN_THIS:
		fxPushNodeStruct(parser, 0, parser->token, aLine);
		parser->root->flags |= parser->flags & mxDerivedFlag;
		fxGetNextToken(parser);
		break;
	case XS_TOKEN_INTEGER:
		fxPushIntegerNode(parser, parser->integer, aLine);
		fxGetNextToken(parser);
		break;
	case XS_TOKEN_NUMBER:
		fxPushNumberNode(parser, parser->number, aLine);
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
				fxGetNextToken(parser);
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
				aSymbol = parser->symbol;
				fxGetNextToken(parser);
				flags = mxAsyncFlag;
			}
		}
		if ((!parser->crlf) && (parser->token == XS_TOKEN_ARROW)) {
			fxCheckStrictSymbol(parser, aSymbol);
			if (flags && (aSymbol == parser->awaitSymbol))
				fxReportParserError(parser, "invalid await");
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
		fxPushStringNode(parser, parser->rawLength, parser->raw, aLine);
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
			fxReportParserError(parser, "invalid host object");
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
		fxReportParserError(parser, "missing expression");
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
				fxReportParserError(parser, "missing ,");
			fxAssignmentExpression(parser);
			fxPushNodeStruct(parser, 1, XS_TOKEN_SPREAD, anItemLine);
			aCount++;
			elision = 0;
			aSpreadFlag = 1;
		}
		else {
			if (!elision)
				fxReportParserError(parser, "missing ,");
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
	parser->root->flags = parser->flags & (mxStrictFlag | mxNotSimpleParametersFlag | mxArrowFlag | mxSuperFlag | flag);
	if (!(flags & mxStrictFlag) && (parser->flags & mxStrictFlag))
		fxCheckStrictFunction(parser, (txFunctionNode*)parser->root);
	parser->flags = flags | (parser->flags & mxArgumentsFlag);
}

void fxClassExpression(txParser* parser, txInteger theLine, txSymbol** theSymbol)
{
	txBoolean heritageFlag = 0;
//	txBoolean hostFlag = 0;
	txNode* constructor = NULL;
	txInteger aCount = 0;
	txInteger aLine = parser->line;
	txUnsigned flags = parser->flags;
	fxMatchToken(parser, XS_TOKEN_CLASS);
	if (parser->token == XS_TOKEN_IDENTIFIER) {
		fxPushSymbol(parser, parser->symbol);
		if (theSymbol)
			*theSymbol = parser->symbol;
		fxGetNextToken(parser);
	}
	else
		fxPushNULL(parser);
	parser->flags |= mxStrictFlag;
	if (parser->token == XS_TOKEN_EXTENDS) {
		fxGetNextToken(parser);
		fxCallExpression(parser);
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
			fxReportParserError(parser, "invalid host class");
			fxPushNULL(parser);
		}
		fxPushNodeStruct(parser, 3, XS_TOKEN_HOST, aLine);
//		hostFlag = 1;
	}
	else {
		fxPushNULL(parser);
	}
	fxMatchToken(parser, XS_TOKEN_LEFT_BRACE);
	for (;;) {
		txBoolean aStaticFlag;
		txInteger aPropertyLine = parser->line;
		txSymbol* aSymbol;
		txToken aToken0;
		txToken aToken1;
		txToken aToken2;
		txUnsigned flag;
		if (parser->token == XS_TOKEN_RIGHT_BRACE)
			break;
		if (parser->token == XS_TOKEN_STATIC) {
			fxGetNextToken(parser);
			aStaticFlag = 1;
		}
		else
			aStaticFlag = 0;
		fxPropertyName(parser, &aSymbol, &aToken0, &aToken1, &aToken2, &flag);
		if (aStaticFlag && (aSymbol == parser->prototypeSymbol))
			fxReportParserError(parser, "invalid static prototype");
		if (aSymbol == parser->constructorSymbol) {
            fxPopNode(parser); // symbol
			if (constructor || aStaticFlag || (aToken2 == XS_TOKEN_GENERATOR) || (aToken2 == XS_TOKEN_GETTER) || (aToken2 == XS_TOKEN_SETTER)) 
				fxReportParserError(parser, "invalid constructor");
			fxFunctionExpression(parser, aPropertyLine, C_NULL, mxSuperFlag | ((heritageFlag) ? mxDerivedFlag : mxBaseFlag));
            constructor = fxPopNode(parser);
		}
		else {
			if (aToken2 == XS_TOKEN_GENERATOR)
				fxGeneratorExpression(parser, aPropertyLine, C_NULL, mxSuperFlag | flag);
			else
				fxFunctionExpression(parser, aPropertyLine, C_NULL, mxSuperFlag | flag);
			fxPushNodeStruct(parser, 2, aToken1, aPropertyLine);
			if (aStaticFlag)
				parser->root->flags |= mxStaticFlag;
			if (aToken2 == XS_TOKEN_GETTER) 
				parser->root->flags |= mxGetterFlag;
			else if (aToken2 == XS_TOKEN_SETTER) 
				parser->root->flags |= mxSetterFlag;
			else
				parser->root->flags |= mxMethodFlag;
			aCount++;
		}
		fxSemicolon(parser);
	}
	fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
	fxPushNodeList(parser, aCount);
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
			parser->root->flags = mxStrictFlag | mxDerivedFlag | mxMethodFlag | mxFunctionFlag | mxSuperFlag;
		}
		else {
			fxPushNULL(parser);
		
			fxPushNodeList(parser, 0);
			fxPushNodeStruct(parser, 1, XS_TOKEN_PARAMS_BINDING, aLine);
			
			fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aLine);
			fxPushNodeStruct(parser, 1, XS_TOKEN_STATEMENT, aLine);
			fxPushNodeStruct(parser, 1, XS_TOKEN_BODY, aLine);
			
			fxPushNodeStruct(parser, 3, XS_TOKEN_FUNCTION, aLine);
			parser->root->flags = mxStrictFlag | mxBaseFlag | mxMethodFlag | mxFunctionFlag;
		}
	}
	fxPushNodeStruct(parser, 4, XS_TOKEN_CLASS, aLine);
	parser->flags = flags;
}

void fxFunctionExpression(txParser* parser, txInteger theLine, txSymbol** theSymbol, txUnsigned flag)
{
	txUnsigned flags = parser->flags;
	parser->flags = (flags & (mxParserFlags | mxStrictFlag)) | mxFunctionFlag | flag;
	if ((parser->token == XS_TOKEN_IDENTIFIER) || ((flags & mxGeneratorFlag) && !(flags & mxStrictFlag) && (parser->token == XS_TOKEN_YIELD))) {
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
			fxReportParserError(parser, "invalid host function");
			fxPushNULL(parser);
		}
		fxPushNodeStruct(parser, 3, XS_TOKEN_HOST, theLine);
	}
	else {
		fxMatchToken(parser, XS_TOKEN_LEFT_BRACE);
		fxBody(parser);
		fxPushNodeStruct(parser, 1, XS_TOKEN_BODY, theLine);
		fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
		fxPushNodeStruct(parser, 3, XS_TOKEN_FUNCTION, theLine);
	}
	parser->root->flags = parser->flags & (mxStrictFlag | mxNotSimpleParametersFlag | mxFunctionFlag | mxArgumentsFlag | flag);
	if (!(flags & mxStrictFlag) && (parser->flags & mxStrictFlag))
		fxCheckStrictFunction(parser, (txFunctionNode*)parser->root);
	parser->flags = flags;
}

void fxGeneratorExpression(txParser* parser, txInteger theLine, txSymbol** theSymbol, txUnsigned flag)
{
	txUnsigned flags = parser->flags;
	parser->flags = (flags & (mxParserFlags | mxStrictFlag)) | mxGeneratorFlag | flag;
	if (parser->token == XS_TOKEN_IDENTIFIER) {
		fxPushSymbol(parser, parser->symbol);
		if (theSymbol)
			*theSymbol = parser->symbol;
		else if (parser->symbol == parser->yieldSymbol)
 			fxReportParserError(parser, "invalid yield");
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
	fxMatchToken(parser, XS_TOKEN_RIGHT_BRACE);
	fxPushNodeStruct(parser, 3, XS_TOKEN_GENERATOR, theLine);
	parser->root->flags = parser->flags & (mxStrictFlag | mxNotSimpleParametersFlag | mxGeneratorFlag | mxArgumentsFlag | flag);
	if (!(flags & mxStrictFlag) && (parser->flags & mxStrictFlag))
		fxCheckStrictFunction(parser, (txFunctionNode*)parser->root);
	parser->flags = flags;
}

void fxGroupExpression(txParser* parser, txUnsigned flag)
{
	txInteger aCount = 0;
	txInteger aLine;
	parser->flags &= ~(mxAwaitingFlag | mxYieldingFlag);
	parser->flags |= flag;
	fxMatchToken(parser, XS_TOKEN_LEFT_PARENTHESIS);
	while (gxTokenFlags[parser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
		fxAssignmentExpression(parser);
		aCount++;
		if (parser->token != XS_TOKEN_COMMA) 
			break;
		fxGetNextToken(parser);
	}
	if (parser->token == XS_TOKEN_SPREAD) {
		fxRestBinding(parser, XS_TOKEN_ARG);
		aCount++;
	}
	aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
	parser->flags &= ~flag;
	if ((!parser->crlf) && (parser->token == XS_TOKEN_ARROW)) {
		fxPushNodeList(parser, aCount);
		fxPushNodeStruct(parser, 1, XS_TOKEN_EXPRESSIONS, aLine);
		if (!fxParametersBindingFromExpressions(parser, parser->root))
			fxReportParserError(parser, "no parameters");
		fxCheckStrictBinding(parser, parser->root);
		if (parser->flags & mxAwaitingFlag)
			fxReportParserError(parser, "invalid await");
		if (parser->flags & mxYieldingFlag)
			fxReportParserError(parser, "invalid yield");
		fxArrowExpression(parser, flag);
	}
	else {
		if (aCount == 0)
			fxReportParserError(parser, "missing expression");
		else /*if (aCount > 1)*/ {
			fxPushNodeList(parser, aCount);
			fxPushNodeStruct(parser, 1, XS_TOKEN_EXPRESSIONS, aLine);
		}
	}
}

void fxNewExpression(txParser* parser)
{
	txInteger aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_NEW);
	if (parser->token == XS_TOKEN_DOT) {
		fxGetNextToken(parser);
		if (fxIsKeyword(parser, parser->targetSymbol)) {
			if (!(parser->flags & mxFunctionFlag))
				fxReportParserError(parser, "invalid new.target");
			fxGetNextToken(parser);
			fxPushNodeStruct(parser, 0, XS_TOKEN_TARGET, aLine);
		}
		else
			fxReportParserError(parser, "missing target");
		return;
	}
	fxLiteralExpression(parser);
	for (;;) {
		txInteger aMemberLine = parser->line;
		if (parser->token == XS_TOKEN_DOT) {
			fxGetNextToken(parser);
			if (parser->token == XS_TOKEN_IDENTIFIER) {
				fxPushSymbol(parser, parser->symbol);
				fxPushNodeStruct(parser, 2, XS_TOKEN_MEMBER, aMemberLine);
				fxGetNextToken(parser);
			}
			else
				fxReportParserError(parser, "missing property");
		}
		else if (parser->token == XS_TOKEN_LEFT_BRACKET) {
			fxGetNextToken(parser);
			fxCommaExpression(parser);
			fxPushNodeStruct(parser, 2, XS_TOKEN_MEMBER_AT, aMemberLine);
			fxMatchToken(parser, XS_TOKEN_RIGHT_BRACKET);
		}
		else if (parser->token == XS_TOKEN_TEMPLATE) {
			fxPushStringNode(parser, parser->stringLength, parser->string, aLine);
			fxPushStringNode(parser, parser->rawLength, parser->raw, aLine);
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
        fxPropertyName(parser, &aSymbol, &aToken0, &aToken1, &aToken2, &flags);
		if ((aToken2 == XS_TOKEN_GETTER) || (aToken2 == XS_TOKEN_SETTER)) {
			flags |= mxShorthandFlag;
			if (aToken2 == XS_TOKEN_GETTER)
				flags |= mxGetterFlag;
			else if (aToken2 == XS_TOKEN_SETTER)
				flags |= mxSetterFlag;
			if (parser->token == XS_TOKEN_LEFT_PARENTHESIS)
				fxFunctionExpression(parser, aPropertyLine, C_NULL, mxSuperFlag);
			else
				fxReportParserError(parser, "missing (");
		}
		else if (aToken2 == XS_TOKEN_GENERATOR) {
			flags |= mxShorthandFlag | mxMethodFlag;
			if (parser->token == XS_TOKEN_LEFT_PARENTHESIS)
				fxGeneratorExpression(parser, aPropertyLine, C_NULL, mxSuperFlag | flags);
			else
				fxReportParserError(parser, "missing (");
		}
		else if (aToken2 == XS_TOKEN_FUNCTION) {
			flags |= mxShorthandFlag | mxMethodFlag;
			if (parser->token == XS_TOKEN_LEFT_PARENTHESIS)
				fxFunctionExpression(parser, aPropertyLine, C_NULL, mxSuperFlag | flags);
			else
				fxReportParserError(parser, "missing (");
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
				fxGetNextToken(parser);
				fxAssignmentExpression(parser);
				fxPushNodeStruct(parser, 2, XS_TOKEN_BINDING, aPropertyLine);
			}
			else if (aToken0 == XS_TOKEN_IDENTIFIER) {
				fxPushNodeStruct(parser, 1, XS_TOKEN_ACCESS, aPropertyLine);
			}
			else {
				fxReportParserError(parser, "invalid identifier");
				fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aPropertyLine);
			}
		}
		else {
			fxReportParserError(parser, "missing :");
			fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, aPropertyLine);
		}
		fxPushNodeStruct(parser, 2, aToken1, aPropertyLine);
		parser->root->flags |= flags;
		fxCheckUniqueProperty(parser, base, parser->root); 
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
	fxPushStringNode(parser, parser->rawLength, parser->raw, aLine);
	fxPushNodeStruct(parser, 2, XS_TOKEN_TEMPLATE_MIDDLE, aLine);
	aCount++;
	for (;;) {
		fxGetNextToken(parser);
        if (parser->token != XS_TOKEN_RIGHT_BRACE) {
            fxCommaExpression(parser);
            aCount++;
        }
		if (parser->token != XS_TOKEN_RIGHT_BRACE) {
			fxReportParserError(parser, "missing }");
		}
		fxGetNextTokenTemplate(parser);
		fxPushStringNode(parser, parser->stringLength, parser->string, aLine);
		fxPushStringNode(parser, parser->rawLength, parser->raw, aLine);
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
		fxReportParserError(parser, "invalid yield");
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
			fxReportParserError(parser, "missing ]");
		}
		aToken1 = XS_TOKEN_PROPERTY_AT;
	}
	else {
		fxReportParserError(parser, "missing identifier");
		fxPushNULL(parser);
	}
	if (aToken2 != XS_NO_TOKEN) {
		if ((gxTokenFlags[parser->token] & XS_TOKEN_IDENTIFIER_NAME)) {
			aSymbol = parser->symbol;
			fxPushSymbol(parser, aSymbol);
			aToken1 = XS_TOKEN_PROPERTY;
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
				fxReportParserError(parser, "missing ]");
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
			fxReportParserError(parser, "missing identifier");
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
	txToken aToken;
	txInteger aLine = parser->line;
	if (parser->token == XS_TOKEN_IDENTIFIER) {
		fxCheckStrictSymbol(parser, parser->symbol);
		if (((theToken == XS_TOKEN_CONST) || (theToken == XS_TOKEN_LET)) && (parser->symbol == parser->letSymbol))
			fxReportParserError(parser, "invalid identifier");
		fxPushSymbol(parser, parser->symbol);
		fxGetNextToken(parser);
		aToken = theToken;
	}
	else if (parser->token == XS_TOKEN_LEFT_BRACE) {
		fxObjectBinding(parser, theToken);
		aToken = XS_TOKEN_OBJECT_BINDING;
	}
	else if (parser->token == XS_TOKEN_LEFT_BRACKET) {
		fxArrayBinding(parser, theToken);
		aToken = XS_TOKEN_ARRAY_BINDING;
	}
	else {
		fxReportParserError(parser, "missing identifier");
		fxPushNULL(parser);
		aToken = theToken;
	}
	if (initializeIt && (parser->token == XS_TOKEN_ASSIGN)) {
		parser->flags &= ~mxForFlag;
		fxGetNextToken(parser);
		fxAssignmentExpression(parser);
		fxPushNodeStruct(parser, 2, aToken, aLine);
	}
	else {
		fxPushNULL(parser);
		fxPushNodeStruct(parser, 2, aToken, aLine);
	}
}

txNode* fxBindingFromExpression(txParser* parser, txNode* theNode, txToken theToken)
{
	txToken aToken = theNode->description->token;
	txNode* binding;
	if (aToken == XS_TOKEN_BINDING) {
		fxCheckStrictSymbol(parser, ((txBindingNode*)theNode)->symbol);
		theNode->description = &gxTokenDescriptions[theToken];
		return theNode;
	}
	if (aToken == XS_TOKEN_ARRAY_BINDING) {
		txNodeList* list = ((txArrayBindingNode*)theNode)->items;
		txNode* item = list->first;
		while (item) {
			fxBindingFromExpression(parser, item, theToken);
			item = item->next;
		}
		return theNode;
	}
	if (aToken == XS_TOKEN_OBJECT_BINDING) {
		txNodeList* list = ((txObjectBindingNode*)theNode)->items;
		txNode* item = list->first;
		while (item) {
			fxBindingFromExpression(parser, item, theToken);
			item = item->next;
		}
		return theNode;
	}
	if (aToken == XS_TOKEN_PROPERTY_BINDING) {
		fxBindingFromExpression(parser, ((txPropertyBindingNode*)theNode)->binding, theToken);
		return theNode;
	}
	if (aToken == XS_TOKEN_PROPERTY_BINDING_AT) {
		fxBindingFromExpression(parser, ((txPropertyBindingAtNode*)theNode)->binding, theToken);
		return theNode;
	}
	if (aToken == XS_TOKEN_REST_BINDING) {
		fxBindingFromExpression(parser, ((txRestBindingNode*)theNode)->binding, theToken);
		return theNode;
	}
	if (aToken == XS_TOKEN_SKIP_BINDING)
		return theNode;
	if ((aToken== XS_TOKEN_MEMBER) || (aToken == XS_TOKEN_MEMBER_AT))
		return theNode;
	
	if (aToken == XS_TOKEN_ACCESS) {
		fxCheckStrictSymbol(parser, ((txAccessNode*)theNode)->symbol);
		fxPushSymbol(parser, ((txAccessNode*)theNode)->symbol);
		fxPushNULL(parser);
		fxPushNodeStruct(parser, 2, theToken, theNode->line);
		return fxPopNode(parser);
	}
	
	if (aToken == XS_TOKEN_ASSIGN) {
		binding = fxBindingFromExpression(parser, ((txAssignNode*)theNode)->reference, theToken);
		if (!binding)
			return NULL;
		((txBindingNode*)binding)->initializer = ((txAssignNode*)theNode)->value;
		return binding;
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
	int elision = 1;
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
				fxReportParserError(parser, "missing ,");
			if (parser->token == XS_TOKEN_SPREAD) {
				fxRestBinding(parser, theToken);
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
}

txNode* fxArrayBindingFromExpression(txParser* parser, txNode* theNode, txToken theToken)
{
	txNodeList* list = ((txArrayNode*)theNode)->items;
	txNode** address = &(list->first);
	txNode* item;
	txNode* binding;
	while ((item = *address)) {
		if (item->description->token == XS_TOKEN_SPREAD) {
			txNode* expression = ((txSpreadNode*)item)->expression;
			if ((item->next) || (theNode->flags & mxElisionFlag)) {
				parser->errorSymbol = parser->SyntaxErrorSymbol;
				return NULL;
			}
			if (!expression)
				return NULL;
			if ((theToken == XS_TOKEN_BINDING) && ((expression->description->token == XS_TOKEN_MEMBER) || (expression->description->token == XS_TOKEN_MEMBER_AT)))
				binding = expression;
			else {			
				binding = fxBindingFromExpression(parser, expression, theToken);
				if (!binding) {
					parser->errorSymbol = parser->SyntaxErrorSymbol;
					return NULL;
				}
				if (((txBindingNode*)binding)->initializer) {
					fxReportParserError(parser, "invalid rest");
					return NULL;
				}
			}
			item->description = &gxTokenDescriptions[XS_TOKEN_REST_BINDING];
			((txRestBindingNode*)item)->binding = binding;
			break;
		}
		if (item->description->token == XS_TOKEN_ELISION) {
			item->description = &gxTokenDescriptions[XS_TOKEN_SKIP_BINDING];
		}
		else {
			if ((theToken == XS_TOKEN_BINDING) && ((item->description->token== XS_TOKEN_MEMBER) || (item->description->token == XS_TOKEN_MEMBER_AT)))
				binding = item;
			else {			
				binding = fxBindingFromExpression(parser, item, theToken);
				if (!binding) {
					parser->errorSymbol = parser->SyntaxErrorSymbol;
					return NULL;
				}
				binding->next = item->next;
				item = *address = binding;
			}
		}
		address = &(item->next);
	}
	fxPushNode(parser, (txNode*)list);
	fxPushNULL(parser);
	fxPushNodeStruct(parser, 2, XS_TOKEN_ARRAY_BINDING, theNode->line);
	return fxPopNode(parser);
}

void fxObjectBinding(txParser* parser, txToken theToken)
{
	txInteger aCount = 0;
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
		if (parser->token == XS_TOKEN_IDENTIFIER) {
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
				fxReportParserError(parser, "missing ]");
			}
			aToken = XS_TOKEN_PROPERTY_BINDING_AT;
		}
		else {
			fxReportParserError(parser, "missing identifier");
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
			fxReportParserError(parser, "missing :");
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
}

txNode* fxObjectBindingFromExpression(txParser* parser, txNode* theNode, txToken theToken)
{
	txNodeList* list = ((txObjectNode*)theNode)->items;
	txNode* property = list->first;
	txNode* binding;
	while (property) {
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
		property = property->next;
	}
	fxPushNode(parser, (txNode*)list);
	fxPushNULL(parser);
	fxPushNodeStruct(parser, 2, XS_TOKEN_OBJECT_BINDING, theNode->line);
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
				fxRestBinding(parser, XS_TOKEN_ARG);
				aCount++;
				break;
			}
			fxBinding(parser, XS_TOKEN_ARG, 1);
			binding = (txBindingNode*)parser->root;
			if (binding->initializer)
				parser->flags |= mxNotSimpleParametersFlag;
			else if (binding->description->token != XS_TOKEN_ARG)
				parser->flags |= mxNotSimpleParametersFlag;
			aCount++;
			if (parser->token != XS_TOKEN_RIGHT_PARENTHESIS)
				fxMatchToken(parser, XS_TOKEN_COMMA);
		}
		fxMatchToken(parser, XS_TOKEN_RIGHT_PARENTHESIS);
	}
	else
		fxReportParserError(parser, "missing (");
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
		if (item->description->token == XS_TOKEN_SPREAD) {
			if ((!(item->next)) && ((txSpreadNode*)item)->expression && (((txSpreadNode*)item)->expression->description->token == XS_TOKEN_ACCESS)) {
				item->description = &gxTokenDescriptions[XS_TOKEN_REST_BINDING];
				((txRestBindingNode*)item)->binding = fxBindingFromExpression(parser, ((txSpreadNode*)item)->expression, XS_TOKEN_ARG);
				break;
			}
			return NULL;
		}
		if (item->description->token == XS_TOKEN_REST_BINDING) {
			break;
		}
		binding = (txBindingNode*)fxBindingFromExpression(parser, item, XS_TOKEN_ARG);
		if (!binding)
			return NULL;
		if (binding->initializer)
			parser->flags |= mxNotSimpleParametersFlag;
		else if (binding->description->token != XS_TOKEN_ARG)
			parser->flags |= mxNotSimpleParametersFlag;
		binding->next = item->next;
		item = *address = (txNode*)binding;
		address = &(item->next);
	}
	theNode->description = &gxTokenDescriptions[XS_TOKEN_PARAMS_BINDING];
	return theNode;
}

void fxRestBinding(txParser* parser, txToken theToken)
{
	txInteger aLine = parser->line;
	fxMatchToken(parser, XS_TOKEN_SPREAD);
	fxBinding(parser, theToken, 0);
	fxPushNodeStruct(parser, 1, XS_TOKEN_REST_BINDING, aLine);
}

txBoolean fxCheckReference(txParser* parser, txToken theToken)
{
	txNode* node = parser->root;
	txToken aToken = node ? node->description->token : XS_NO_TOKEN;
	if (aToken == XS_TOKEN_EXPRESSIONS) {
		txNode* item = ((txExpressionsNode*)node)->items->first;
		if (item && !item->next) {
			aToken = item->description->token;
			if ((aToken == XS_TOKEN_ACCESS) || (aToken == XS_TOKEN_MEMBER) || (aToken == XS_TOKEN_MEMBER_AT) || (aToken == XS_TOKEN_UNDEFINED)) {
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
	if ((aToken == XS_TOKEN_MEMBER) || (aToken == XS_TOKEN_MEMBER_AT) || (aToken == XS_TOKEN_UNDEFINED))
		return 1;
		
	if (theToken == XS_TOKEN_ASSIGN) {
		if (aToken == XS_TOKEN_ARRAY) {
			txNode* binding = fxArrayBindingFromExpression(parser, node, XS_TOKEN_BINDING);
            if (binding) {
                binding->next = node->next;
                parser->root = binding;
 				return 1;
           }
       	}
		else if (aToken == XS_TOKEN_OBJECT) {
			txNode* binding = fxObjectBindingFromExpression(parser, node, XS_TOKEN_BINDING);
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
	if (node->description->token == XS_TOKEN_ARRAY_BINDING) {
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
	else if (node->description->token != XS_TOKEN_SKIP_BINDING)
		fxCheckStrictSymbol(parser, ((txBindingNode*)node)->symbol);
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
			fxReportParserError(parser, "invalid arguments (strict mode)");
		else if (symbol == parser->evalSymbol)
			fxReportParserError(parser, "invalid eval (strict mode)");
		else if (symbol == parser->yieldSymbol)
			fxReportParserError(parser, "invalid yield (strict mode)");
	}
	else if (parser->flags & mxYieldFlag) {
		if (symbol == parser->yieldSymbol)
			fxReportParserError(parser, "invalid yield");
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
			fxReportParserError(parser, "getter already defined");
		else if (!(baseNode->flags & mxSetterFlag))
			fxReportParserError(parser, "property already defined");
	}
	else if (currentNode->flags & mxSetterFlag) {
		if (baseNode->flags & mxSetterFlag)
			fxReportParserError(parser, "setter already defined");
		else if (!(baseNode->flags & mxGetterFlag))
			fxReportParserError(parser, "property already defined");
	}
	else {
		if (baseNode->flags & mxGetterFlag)
			fxReportParserError(parser, "getter already defined");
		else if (baseNode->flags & mxSetterFlag)
			fxReportParserError(parser, "setter already defined");
		else if (parser->flags & mxStrictFlag)
			fxReportParserError(parser, "property already defined (strict mode)");
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
		fxReportParserError(parser, "invalid value");
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
			fxReportParserError(parser, "missing name");
			break;
		}
		fxPushStringNode(parser, parser->stringLength, parser->string, parser->line);
		fxGetNextTokenJSON(parser);
		if (parser->token != XS_TOKEN_COLON) {
			fxReportParserError(parser, "missing :");
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
		fxReportParserError(parser, "missing }");
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
		fxReportParserError(parser, "missing ]");
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
		if (parser->token == XS_TOKEN_IDENTIFIER)
			symbol = fxJSXNamespace(parser, symbol, parser->symbol);
		else
			fxReportParserError(parser, "missing name");
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
			fxReportParserError(parser, "missing expression");
			fxPushNodeStruct(parser, 0, XS_TOKEN_UNDEFINED, line);
		}
		else {
			fxAssignmentExpression(parser);
			if (parser->token == XS_TOKEN_RIGHT_BRACE)
				fxGetNextToken(parser);
			else
				fxReportParserError(parser, "missing }");
		}
	}
	else {
		fxReportParserError(parser, "invalid %s", gxTokenNames[parser->token]);
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
		fxReportParserError(parser, "missing identifier");
		fxPushNULL(parser);
	}
	for (;;) {
		if (parser->token == XS_TOKEN_MORE)
			break;
		if (parser->token == XS_TOKEN_DIVIDE) {
			fxGetNextToken(parser);
			if (parser->token != XS_TOKEN_MORE)
				fxReportParserError(parser, "missing >");
			closed = 1;
			break;
		}
		if (parser->token == XS_TOKEN_IDENTIFIER) {
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
					fxReportParserError(parser, "missing }");
				fxPushNodeStruct(parser, 1, XS_TOKEN_SPREAD, parser->line);
				propertyCount++;
			}
			else
				fxReportParserError(parser, "invalid %s", gxTokenNames[parser->token]);
		}
		else {
			fxReportParserError(parser, "invalid %s", gxTokenNames[parser->token]);
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
						fxReportParserError(parser, "missing }");
				}
			}
			else if (parser->token == XS_TOKEN_LESS) {
				fxGetNextToken(parser);
				if (parser->token == XS_TOKEN_DIVIDE) {
					fxGetNextToken(parser);
					if (parser->token == XS_TOKEN_IDENTIFIER) {
						fxJSXElementName(parser);
						fxJSXMatch(parser, name, parser->root);
					}
					else {
						fxReportParserError(parser, "missing identifier");
						fxPushNULL(parser);
					}
					if (parser->token != XS_TOKEN_MORE)
						fxReportParserError(parser, "missing >");
					fxPopNode(parser);
					break;
				}
				else
					fxJSXElement(parser);
			}
			else {
				fxReportParserError(parser, "invalid %s", gxTokenNames[parser->token]);
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
			fxReportParserError(parser, "missing name");
		fxPushSymbol(parser, symbol);
		fxPushNodeStruct(parser, 1, XS_TOKEN_ACCESS, line);
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
				fxReportParserError(parser, "missing property");
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
	txString string = fxNewParserChunk(parser, namespaceLength + 1 + nameLength + 1);
	c_memcpy(string, namespace, namespaceLength);
	string[namespaceLength] = ':';
	c_memcpy(string + namespaceLength + 1, name, nameLength);
	string[namespaceLength + 1 + nameLength] = 0;
	return fxNewParserSymbol(parser, string);
}

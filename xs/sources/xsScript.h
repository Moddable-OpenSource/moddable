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

#ifndef __XS6SCRIPT__
#define __XS6SCRIPT__

#include "xsCommon.h"

typedef void (*txReport)(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments);

typedef txS2 txToken;
typedef txS4 txTokenFlag;

typedef struct sxKeyword txKeyword;
typedef struct sxSymbol txSymbol;
typedef struct sxScope txScope;

typedef struct sxParser txParser;
typedef struct sxParserChunk txParserChunk;
typedef struct sxParserJump txParserJump;

typedef struct sxByteCode txByteCode;
typedef struct sxBranchCode txBranchCode;
typedef struct sxFlagCode txFlagCode;
typedef struct sxIndexCode txIndexCode;
typedef struct sxIntegerCode txIntegerCode;
typedef struct sxNumberCode txNumberCode;
typedef struct sxStringCode txStringCode;
typedef struct sxSymbolCode txSymbolCode;
typedef struct sxTargetCode txTargetCode;
typedef struct sxTargetAlias txTargetAlias;
typedef struct sxVariableCode txVariableCode;
typedef struct sxCoder txCoder;

struct sxKeyword {
	char text[30];
	txToken token;
};

struct sxSymbol {
	txSymbol* next;
	txID ID;
	txInteger length;
	txString string;
	txSize sum;
	txInteger usage;
};

typedef void (*txNodeCall)(void*, void*);
typedef void (*txNodeDistribute)(void*, txNodeCall, void*);

typedef struct {
	txNodeDistribute distribute;
	txNodeCall bind;
	txNodeCall hoist;
	txNodeCall code;
	txNodeCall codeAssign;
	txNodeCall codeReference;
} txNodeDispatch;

typedef struct {
	txToken code;
	txToken token;
	txString name;
	txInteger size;
	const txNodeDispatch* dispatch;
} txNodeDescription;

typedef struct sxNode txNode;

typedef struct sxDeclareNode txDeclareNode;
typedef struct sxDefineNode txDefineNode;
typedef struct sxExportNode txExportNode;
typedef struct sxHostNode txHostNode;
typedef struct sxImportNode txImportNode;
typedef struct sxLabelNode txLabelNode;
typedef struct sxNativeNode txNativeNode;
typedef struct sxSpecifierNode txSpecifierNode;
typedef struct sxStringNode txStringNode;

#define mxNodePart\
	txNode* next;\
	const txNodeDescription* description;\
	txSymbol* path;\
	txInteger line;\
	txUnsigned flags

#define mxAccessNodePart\
	txSymbol* symbol;\
	txNode* initializer;\
	txDeclareNode* declaration

#define mxDeclareNodePart\
	txDeclareNode* nextDeclareNode;\
	txInteger index;\
	txSpecifierNode* importSpecifier;\
	txSpecifierNode* firstExportSpecifier

struct sxNode {
	mxNodePart;
};

typedef struct {
	txNode* next;
	const txNodeDescription* description;
	txSymbol* symbol;
} txNodeLink;

typedef struct {
	txNode* next;
	const txNodeDescription* description;
	txNode* first;
	txInteger length;
} txNodeList;


typedef struct {
	mxNodePart;
	mxAccessNodePart;
} txAccessNode;

typedef struct {
	mxNodePart;
	txNodeList* items;
	txInteger count;
} txArrayNode;

typedef struct {
	mxNodePart;
	txNodeList* items;
	txNode* initializer;
} txArrayBindingNode;

typedef struct {
	mxNodePart;
	txNode* reference;
	txNode* value;
} txAssignNode;

typedef struct {
	mxNodePart;
	txNode* left;
	txNode* right;
} txBinaryExpressionNode;

typedef struct {
	mxNodePart;
	mxAccessNodePart;
	mxDeclareNodePart;
} txBindingNode;

typedef struct {
	mxNodePart;
	txNode* statement;
	txScope* scope;
} txBlockNode;

typedef struct {
	mxNodePart;
	txNode* statement;
	txScope* scope;
} txBodyNode;

typedef struct {
	mxNodePart;
	txSymbol* symbol;
} txBreakContinueNode;

typedef struct {
	mxNodePart;
	txNode* reference;
	txNode* params;
} txCallNewNode;

typedef struct {
	mxNodePart;
	txNode* expression;
	txNode* statement;
	txTargetCode* target;
} txCaseNode;

typedef struct {
	mxNodePart;
	txNode* parameter;
	txNode* statement;
	txScope* scope;
	txScope* statementScope;
} txCatchNode;

typedef struct {
	mxNodePart;
	txSymbol* symbol;
	txNode* heritage;
	txNodeList* items;
	txNode* constructor;
	txScope* scope;
} txClassNode;

struct sxDeclareNode {
	mxNodePart;
	mxAccessNodePart;
	mxDeclareNodePart;
};

struct sxDefineNode {
	mxNodePart;
	mxAccessNodePart;
	mxDeclareNodePart;
	txDefineNode* nextDefineNode;
};

typedef struct {
	mxNodePart;
	txNode* reference;
} txDeleteNode;

typedef struct {
	mxNodePart;
	txNode* statement;
	txNode* expression;
} txDoNode;

typedef struct {
	mxNodePart;
	txScope* scope;
} txEvalNode;

struct sxExportNode {
	mxNodePart;
	txNodeList* specifiers;
	txNode* from;
};

typedef struct {
	mxNodePart;
	txNodeList* items;
	txDeclareNode* declaration;
	txBoolean mapped;
} txExpressionsNode;

typedef struct {
	mxNodePart;
	txNode* initialization;
	txNode* expression;
	txNode* iteration;
	txNode* statement;
	txScope* scope;
} txForNode;

typedef struct {
	mxNodePart;
	txNode* reference;
	txNode* expression;
	txNode* statement;
	txScope* scope;
} txForInForOfNode;

typedef struct {
	mxNodePart;
	txSymbol* symbol;
	txNode* params;
	txNode* body;
	txScope* scope;
	txInteger variableCount;
} txFunctionNode;

struct sxHostNode {
	mxNodePart;
	txSymbol* symbol;
	txNode* params;
	txStringNode* at;
	txHostNode* nextHostNode;
	txInteger paramsCount;
};

typedef struct {
	mxNodePart;
	txNode* expression;
	txNode* thenStatement;
	txNode* elseStatement;
} txIfNode;

struct sxImportNode {
	mxNodePart;
	txNodeList* specifiers;
	txNode* from;
};

typedef struct {
	mxNodePart;
	txNode* body;
} txIncludeNode;

typedef struct {
	mxNodePart;
	txInteger value;
} txIntegerNode;

struct sxLabelNode {
	mxNodePart;
	txSymbol* symbol;
	txNode* statement;
	txLabelNode* nextLabel;
};

typedef struct {
	mxNodePart;
	txNode* reference;
	txSymbol* symbol;
} txMemberNode;

typedef struct {
	mxNodePart;
	txNode* reference;
	txNode* at;
} txMemberAtNode;

typedef struct {
	mxNodePart;
	txNode* body;
	txScope* scope;
	txInteger variableCount;
	txSpecifierNode* firstEmptySpecifier;
} txModuleNode;

typedef struct {
	mxNodePart;
	txNumber value;
} txNumberNode;

typedef struct {
	mxNodePart;
	txNodeList* items;
} txObjectNode;

typedef struct {
	mxNodePart;
	txNodeList* items;
	txNode* initializer;
} txObjectBindingNode;

typedef struct {
	mxNodePart;
	txNodeList* items;
} txParamsNode;

typedef struct {
	mxNodePart;
	txNodeList* items;
	txDeclareNode* declaration;
	txBoolean mapped;
} txParamsBindingNode;

typedef struct {
	mxNodePart;
	txNode* left;
} txPostfixExpressionNode;

typedef struct {
	mxNodePart;
	txNode* body;
	txScope* scope;
	txInteger variableCount;
} txProgramNode;

typedef struct {
	mxNodePart;
	txSymbol* symbol;
	txNode* value;
} txPropertyNode;

typedef struct {
	mxNodePart;
	txNode* at;
	txNode* value;
} txPropertyAtNode;

typedef struct {
	mxNodePart;
	txSymbol* symbol;
	txNode* binding;
} txPropertyBindingNode;

typedef struct {
	mxNodePart;
	txNode* at;
	txNode* binding;
} txPropertyBindingAtNode;

typedef struct {
	mxNodePart;
	txNode* expression;
	txNode* thenExpression;
	txNode* elseExpression;
} txQuestionMarkNode;

typedef struct {
	mxNodePart;
	txNode* value;
	txNode* modifier;
} txRegexpNode;

typedef struct {
	mxNodePart;
	txNode* binding;
} txRestBindingNode;

typedef struct {
	mxNodePart;
} txSkipBindingNode;

struct sxSpecifierNode {
	mxNodePart;
	txSymbol* symbol;
	txSymbol* asSymbol;
	txNode* from;
	txDeclareNode* declaration;
	txSpecifierNode* nextSpecifier;
};

typedef struct {
	mxNodePart;
	txNode* expression;
} txSpreadNode;

typedef struct {
	mxNodePart;
	txNode* expression;
} txStatementNode;

typedef struct {
	mxNodePart;
	txNodeList* items;
} txStatementsNode;

struct sxStringNode {
	mxNodePart;
	txInteger length;
	txString value;
};

typedef struct {
	mxNodePart;
	txNode* params;
} txSuperNode;

typedef struct {
	mxNodePart;
	txNode* expression;
	txNodeList* items;
	txScope* scope;
} txSwitchNode;

typedef struct {
	mxNodePart;
	txNode* reference;
	txNodeList* items;
} txTemplateNode;

typedef struct {
	mxNodePart;
	txNode* string;
	txNode* raw;
} txTemplateItemNode;

typedef struct {
	mxNodePart;
	txNode* tryBlock;
	txNode* catchBlock;
	txNode* finallyBlock;
} txTryNode;

typedef struct {
	mxNodePart;
	txNode* right;
} txUnaryExpressionNode;

typedef struct {
	mxNodePart;
	txNode* expression;
	txNode* statement;
} txWhileNode;

typedef struct {
	mxNodePart;
	txNode* expression;
	txNode* statement;
	txScope* scope;
	txDeclareNode* declaration;
} txWithNode;

struct sxParserChunk {
	txParserChunk* next;
};

struct sxScope {
	txParser* parser;
	txScope* scope;
	txInteger closureNodeCount;
	txInteger declareNodeCount;
	txInteger defineNodeCount;
	txInteger nativeNodeCount;
	txDeclareNode* firstDeclareNode;
	txDeclareNode* lastDeclareNode;
	txDefineNode* firstDefineNode;
	txDefineNode* lastDefineNode;
	txNativeNode* firstNativeNode;
	txNativeNode* lastNativeNode;
	txToken token;
	txUnsigned flags;
	txNode* node;
};

struct sxParserJump {
	c_jmp_buf jmp_buf;
	txParserJump* nextJump;
};

struct sxParser {
	txParserChunk* first;
	txParserJump* firstJump;
	void* console;
	int error;
	
	void* dtoa;
	
	txSize symbolModulo;
	txSymbol** symbolTable;
	
	txID hostNodeIndex;
	txHostNode* firstHostNode;
	txHostNode* lastHostNode;
	
	void* stream;
	txGetter getter;
	txSymbol* origin;
	txSymbol* path;
	txString name;
	
	int cFlag;
	txUnsigned flags;
	
	int ahead;
	txU4 character;

	int line;
	int crlf;
	int escaped;
	txInteger integer;
	txInteger modifierLength;
	txString modifier;
	txNumber number;
	txInteger rawLength;
	txString raw;
	txInteger stringLength;
	txString string;
	txSymbol* symbol;
	txToken token;

	int line2;
	int crlf2;
	int escaped2;
	txInteger integer2;
	txInteger modifierLength2;
	txString modifier2;
	txNumber number2;
	txInteger rawLength2;
	txString raw2;
	txInteger stringLength2;
	txString string2;
	txSymbol* symbol2;
	txToken token2;
	
	txSymbol* function;

	int errorCount;
	txSymbol* errorSymbol;
	int warningCount;
	txReport reportError;
	txReport reportWarning;
	
	txSymbol* symbolMapPath;
	int* lines;

	txNode* root;
	int nodeCount;
	
	txString emptyString;
	txSymbol* ObjectSymbol;
	txSymbol* __dirnameSymbol;
	txSymbol* __filenameSymbol;
	txSymbol* __jsx__Symbol;
	txSymbol* __proto__Symbol;
	txSymbol* allSymbol;
	txSymbol* argsSymbol;
	txSymbol* argumentsSymbol;
	txSymbol* arrowSymbol;
	txSymbol* asSymbol;
	txSymbol* assignSymbol;
	txSymbol* asyncSymbol;
	txSymbol* awaitSymbol;
	txSymbol* callerSymbol;
	txSymbol* constructorSymbol;
	txSymbol* defaultSymbol;
	txSymbol* doneSymbol;
	txSymbol* evalSymbol;
	txSymbol* exportsSymbol;
	txSymbol* fillSymbol;
	txSymbol* freezeSymbol;
	txSymbol* fromSymbol;
	txSymbol* getSymbol;
	txSymbol* idSymbol;
	txSymbol* includeSymbol;
	txSymbol* InfinitySymbol;
	txSymbol* lengthSymbol;
	txSymbol* letSymbol;
	txSymbol* moduleSymbol;
	txSymbol* nameSymbol;
	txSymbol* NaNSymbol;
	txSymbol* nextSymbol;
	txSymbol* newTargetSymbol;
	txSymbol* ofSymbol;
	txSymbol* prototypeSymbol;
	txSymbol* rawSymbol;
	txSymbol* ReferenceErrorSymbol;
	txSymbol* RegExpSymbol;
	txSymbol* returnSymbol;
	txSymbol* setSymbol;
	txSymbol* sliceSymbol;
	txSymbol* SyntaxErrorSymbol;
	txSymbol* StringSymbol;
	txSymbol* targetSymbol;
	txSymbol* thisSymbol;
	txSymbol* throwSymbol;
	txSymbol* toStringSymbol;
	txSymbol* undefinedSymbol;
	txSymbol* uriSymbol;
	txSymbol* valueSymbol;
	txSymbol* withSymbol;
	txSymbol* yieldSymbol;
	
	char* buffer;
	txSize bufferSize;
	txSize total;
};

enum {
	XS_NO_TOKEN = 0,
	XS_TOKEN_ACCESS,
	XS_TOKEN_ADD,
	XS_TOKEN_ADD_ASSIGN,
	XS_TOKEN_AND,
	XS_TOKEN_ARG,
	XS_TOKEN_ARGUMENTS,
	XS_TOKEN_ARGUMENTS_SLOPPY,
	XS_TOKEN_ARGUMENTS_STRICT,
	XS_TOKEN_ARRAY,
	XS_TOKEN_ARRAY_BINDING,
	XS_TOKEN_ARROW,
	XS_TOKEN_ASSIGN,
	XS_TOKEN_AWAIT,
	XS_TOKEN_BINDING,
	XS_TOKEN_BIT_AND,
	XS_TOKEN_BIT_AND_ASSIGN,
	XS_TOKEN_BIT_NOT,
	XS_TOKEN_BIT_OR,
	XS_TOKEN_BIT_OR_ASSIGN,
	XS_TOKEN_BIT_XOR,
	XS_TOKEN_BIT_XOR_ASSIGN,
	XS_TOKEN_BLOCK,
	XS_TOKEN_BODY,
	XS_TOKEN_BREAK,
	XS_TOKEN_CALL,
	XS_TOKEN_CASE,
	XS_TOKEN_CATCH,
	XS_TOKEN_CLASS,
	XS_TOKEN_COLON,
	XS_TOKEN_COMMA,
	XS_TOKEN_CONST,
	XS_TOKEN_CONTINUE,
	XS_TOKEN_CURRENT,
	XS_TOKEN_DEBUGGER,
	XS_TOKEN_DECREMENT,
	XS_TOKEN_DEFAULT,
	XS_TOKEN_DEFINE,
	XS_TOKEN_DELEGATE,
	XS_TOKEN_DELETE,
	XS_TOKEN_DIVIDE,
	XS_TOKEN_DIVIDE_ASSIGN,
	XS_TOKEN_DO,
	XS_TOKEN_DOT,
	XS_TOKEN_ELISION,
	XS_TOKEN_ELSE,
	XS_TOKEN_ENUM,
	XS_TOKEN_EOF,
	XS_TOKEN_EQUAL,
	XS_TOKEN_EVAL,
	XS_TOKEN_EXPONENTIATION,
	XS_TOKEN_EXPONENTIATION_ASSIGN,
	XS_TOKEN_EXPORT,
	XS_TOKEN_EXPRESSIONS,
	XS_TOKEN_EXTENDS,
	XS_TOKEN_FALSE,
	XS_TOKEN_FINALLY,
	XS_TOKEN_FOR,
	XS_TOKEN_FOR_IN,
	XS_TOKEN_FOR_OF,
	XS_TOKEN_FUNCTION,
	XS_TOKEN_GENERATOR,
	XS_TOKEN_GETTER,
	XS_TOKEN_HOST,
	XS_TOKEN_IDENTIFIER,
	XS_TOKEN_IF,
	XS_TOKEN_IMPLEMENTS,
	XS_TOKEN_IMPORT,
	XS_TOKEN_IN,
	XS_TOKEN_INCLUDE,
	XS_TOKEN_INCREMENT,
	XS_TOKEN_INSTANCEOF,
	XS_TOKEN_INTEGER,
	XS_TOKEN_INTERFACE,
	XS_TOKEN_ITEMS,
	XS_TOKEN_LABEL,
	XS_TOKEN_LEFT_BRACE,
	XS_TOKEN_LEFT_BRACKET,
	XS_TOKEN_LEFT_PARENTHESIS,
	XS_TOKEN_LEFT_SHIFT,
	XS_TOKEN_LEFT_SHIFT_ASSIGN,
	XS_TOKEN_LESS,
	XS_TOKEN_LESS_EQUAL,
	XS_TOKEN_LET,
	XS_TOKEN_MEMBER,
	XS_TOKEN_MEMBER_AT,
	XS_TOKEN_MINUS,
	XS_TOKEN_MODULE,
	XS_TOKEN_MODULO,
	XS_TOKEN_MODULO_ASSIGN,
	XS_TOKEN_MORE,
	XS_TOKEN_MORE_EQUAL,
	XS_TOKEN_MULTIPLY,
	XS_TOKEN_MULTIPLY_ASSIGN,
	XS_TOKEN_NEW, 
	XS_TOKEN_NOT,
	XS_TOKEN_NOT_EQUAL,
	XS_TOKEN_NULL, 
	XS_TOKEN_NUMBER,
	XS_TOKEN_OBJECT,
	XS_TOKEN_OBJECT_BINDING,
	XS_TOKEN_OR,
	XS_TOKEN_PACKAGE,
	XS_TOKEN_PARAMS,
	XS_TOKEN_PARAMS_BINDING,
	XS_TOKEN_PLUS,
	XS_TOKEN_PRIVATE,
	XS_TOKEN_PROGRAM,
	XS_TOKEN_PROPERTY,
	XS_TOKEN_PROPERTY_AT,
	XS_TOKEN_PROPERTY_BINDING,
	XS_TOKEN_PROPERTY_BINDING_AT,
	XS_TOKEN_PROTECTED,
	XS_TOKEN_PUBLIC,
	XS_TOKEN_QUESTION_MARK,
	XS_TOKEN_REGEXP,
	XS_TOKEN_REST_BINDING,
	XS_TOKEN_RETURN,
	XS_TOKEN_RIGHT_BRACE,
	XS_TOKEN_RIGHT_BRACKET,
	XS_TOKEN_RIGHT_PARENTHESIS,
	XS_TOKEN_SEMICOLON,
	XS_TOKEN_SETTER,
	XS_TOKEN_SHORT,
	XS_TOKEN_SIGNED_RIGHT_SHIFT,
	XS_TOKEN_SIGNED_RIGHT_SHIFT_ASSIGN,
	XS_TOKEN_SKIP_BINDING,
	XS_TOKEN_SPECIFIER,
	XS_TOKEN_SPREAD,
	XS_TOKEN_STATEMENT,
	XS_TOKEN_STATEMENTS,
	XS_TOKEN_STATIC,
	XS_TOKEN_STRICT_EQUAL,
	XS_TOKEN_STRICT_NOT_EQUAL,
	XS_TOKEN_STRING,
	XS_TOKEN_SUBTRACT,
	XS_TOKEN_SUBTRACT_ASSIGN,
	XS_TOKEN_SUPER,
	XS_TOKEN_SWITCH,
	XS_TOKEN_TARGET,
	XS_TOKEN_TEMPLATE,
	XS_TOKEN_TEMPLATE_HEAD,
	XS_TOKEN_TEMPLATE_MIDDLE,
	XS_TOKEN_TEMPLATE_TAIL,
	XS_TOKEN_THIS,
	XS_TOKEN_THROW,
	XS_TOKEN_TRUE,
	XS_TOKEN_TRY,
	XS_TOKEN_TYPEOF,
	XS_TOKEN_UNDEFINED,
	XS_TOKEN_UNSIGNED_RIGHT_SHIFT,
	XS_TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN,
	XS_TOKEN_VAR,
	XS_TOKEN_VOID,
	XS_TOKEN_WHILE,
	XS_TOKEN_WITH,
	XS_TOKEN_YIELD,
	XS_TOKEN_COUNT
};

/* xsScript.c */

extern void fxDisposeParserChunks(txParser* parser);
extern void fxInitializeParser(txParser* parser, void* console, txSize bufferSize, txSize symbolModulo);
extern void* fxNewParserChunk(txParser* parser, txSize size);
extern void* fxNewParserChunkClear(txParser* parser, txSize size);
extern txString fxNewParserString(txParser* parser, txString buffer, txSize size);
extern txSymbol* fxNewParserSymbol(txParser* parser, txString buffer);
extern void fxReportReferenceError(txParser* parser, txString theFormat, ...);
extern void fxReportParserError(txParser* parser, txString theFormat, ...);
extern void fxReportParserWarning(txParser* parser, txString theFormat, ...);
extern void fxReportLineReferenceError(txParser* parser, txInteger line, txString theFormat, ...);
extern void fxReportLineError(txParser* parser, txInteger line, txString theFormat, ...);
extern void fxTerminateParser(txParser* parser);
extern void fxThrowMemoryError(txParser* parser);
extern void fxThrowParserError(txParser* parser, txInteger count);

/* xsLexical.c */

extern void fxGetNextCharacter(txParser* parser);
extern void fxGetNextRegExp(txParser* parser, txU4 c);
extern void fxGetNextToken(txParser* parser);
extern void fxGetNextToken2(txParser* parser);
extern void fxGetNextTokenTemplate(txParser* parser);
extern void fxGetNextTokenJSON(txParser* parser);
extern void fxGetNextTokenJSXAttribute(txParser* parser);
extern void fxGetNextTokenJSXChild(txParser* parser);

/* xsSyntaxical.c */

extern void fxCommonModule(txParser* parser);
extern void fxProgram(txParser* parser);
extern void fxModule(txParser* parser);
extern void fxJSONValue(txParser* parser);

/* xsTree.c */

extern void fxIncludeTree(txParser* parser, void* stream, txGetter getter, txUnsigned flags, txString path);
extern void fxParserTree(txParser* parser, void* theStream, txGetter theGetter, txUnsigned flags, txString* name);

extern void fxNodeListDistribute(txNodeList* list, txNodeCall call, void* param);

extern txAccessNode* fxAccessNodeNew(txParser* parser, txToken token, txSymbol* symbol);
extern txDeclareNode* fxDeclareNodeNew(txParser* parser, txToken token, txSymbol* symbol);
extern txDefineNode* fxDefineNodeNew(txParser* parser, txToken token, txSymbol* symbol);
extern txEvalNode* fxEvalNodeNew(txParser* parser, txToken token, txScope* scope);
extern txSpecifierNode* fxSpecifierNodeNew(txParser* parser, txToken token);
extern txNode* fxValueNodeNew(txParser* parser, txToken token);

extern const txNodeDescription gxTokenDescriptions[XS_TOKEN_COUNT];

/* xsSourceMap.c */

extern void fxParserSourceMap(txParser* parser, void* theStream, txGetter theGetter, txUnsigned flags, txString* name);

/* xsScope.c */

extern void fxParserBind(txParser* parser);
extern void fxParserHoist(txParser* parser);

extern void fxNodeBind(void* it, void* param); 
extern void fxNodeHoist(void* it, void* param); 

extern void fxAccessNodeBind(void* it, void* param); 
extern void fxArrayNodeBind(void* it, void* param);
extern void fxArrayBindingNodeBind(void* it, void* param);
extern void fxAssignNodeBind(void* it, void* param);
extern void fxBindingNodeBind(void* it, void* param); 
extern void fxBlockNodeBind(void* it, void* param); 
extern void fxBlockNodeHoist(void* it, void* param); 
extern void fxBodyNodeHoist(void* it, void* param);
extern void fxCallNodeHoist(void* it, void* param);
extern void fxCatchNodeBind(void* it, void* param); 
extern void fxCatchNodeHoist(void* it, void* param); 
extern void fxClassNodeHoist(void* it, void* param);
extern void fxClassNodeBind(void* it, void* param);
extern void fxCompoundExpressionNodeBind(void* it, void* param);
extern void fxDeclareNodeBind(void* it, void* param);
extern void fxDeclareNodeHoist(void* it, void* param); 
extern void fxDefineNodeBind(void* it, void* param); 
extern void fxDefineNodeHoist(void* it, void* param); 
extern void fxDelegateNodeBind(void* it, void* param);
extern void fxExportNodeBind(void* it, void* param);
extern void fxExportNodeHoist(void* it, void* param);
extern void fxForNodeBind(void* it, void* param); 
extern void fxForNodeHoist(void* it, void* param); 
extern void fxForInForOfNodeBind(void* it, void* param); 
extern void fxForInForOfNodeHoist(void* it, void* param); 
extern void fxFunctionNodeBind(void* it, void* param); 
extern void fxFunctionNodeHoist(void* it, void* param); 
extern void fxHostNodeBind(void* it, void* param); 
extern void fxHostNodeHoist(void* it, void* param); 
extern void fxImportNodeHoist(void* it, void* param);
extern void fxModuleNodeBind(void* it, void* param); 
extern void fxModuleNodeHoist(void* it, void* param); 
extern void fxObjectNodeBind(void* it, void* param);
extern void fxObjectBindingNodeBind(void* it, void* param);
extern void fxParamsNodeBind(void* it, void* param);
extern void fxParamsBindingNodeBind(void* it, void* param);
extern void fxParamsBindingNodeHoist(void* it, void* param);
extern void fxPostfixExpressionNodeBind(void* it, void* param);
extern void fxProgramNodeBind(void* it, void* param); 
extern void fxProgramNodeHoist(void* it, void* param); 
extern void fxSpreadNodeBind(void* it, void* param);
extern void fxStatementNodeHoist(void* it, void* param);
extern void fxSwitchNodeBind(void* it, void* param); 
extern void fxSwitchNodeHoist(void* it, void* param); 
extern void fxTemplateNodeBind(void* it, void* param);
extern void fxTryNodeBind(void* it, void* param);
extern void fxWithNodeBind(void* it, void* param);
extern void fxWithNodeHoist(void* it, void* param);

/* xsCode.c */

extern txScript* fxParserCode(txParser* parser);

extern void fxNodeCode(void* it, void* param); 
extern void fxNodeCodeAssign(void* it, void* param); 
extern void fxNodeCodeReference(void* it, void* param); 

extern void fxAccessNodeCode(void* it, void* param); 
extern void fxAccessNodeCodeAssign(void* it, void* param); 
extern void fxAccessNodeCodeReference(void* it, void* param); 
extern void fxAndExpressionNodeCode(void* it, void* param); 
extern void fxArgumentsNodeCode(void* it, void* param);
extern void fxArrayNodeCode(void* it, void* param); 
extern void fxArrayBindingNodeCodeAssign(void* it, void* param); 
extern void fxAssignNodeCode(void* it, void* param); 
extern void fxAwaitNodeCode(void* it, void* param); 
extern void fxBinaryExpressionNodeCode(void* it, void* param); 
extern void fxBindingNodeCode(void* it, void* param);
extern void fxBindingNodeCodeAssign(void* it, void* param); 
extern void fxBindingNodeCodeReference(void* it, void* param);
extern void fxBlockNodeCode(void* it, void* param); 
extern void fxBreakContinueNodeCode(void* it, void* param); 
extern void fxCallNodeCode(void* it, void* param); 
extern void fxCatchNodeCode(void* it, void* param); 
extern void fxClassNodeCode(void* it, void* param) ;
extern void fxCompoundExpressionNodeCode(void* it, void* param); 
extern void fxDebuggerNodeCode(void* it, void* param);
extern void fxDeclareNodeCodeAssign(void* it, void* param);
extern void fxDeclareNodeCodeReference(void* it, void* param);
extern void fxDefineNodeCode(void* it, void* param);
extern void fxDelegateNodeCode(void* it, void* param); 
extern void fxDeleteNodeCode(void* it, void* param); 
extern void fxDoNodeCode(void* it, void* param); 
extern void fxEvalNodeCode(void* it, void* param);
extern void fxExportNodeCode(void* it, void* param); 
extern void fxExpressionsNodeCode(void* it, void* param); 
extern void fxForNodeCode(void* it, void* param); 
extern void fxForInForOfNodeCode(void* it, void* param); 
extern void fxFunctionNodeCode(void* it, void* param); 
extern void fxHostNodeCode(void* it, void* param);
extern void fxIfNodeCode(void* it, void* param); 
extern void fxIncludeNodeCode(void* it, void* param);
extern void fxImportNodeCode(void* it, void* param); 
extern void fxIntegerNodeCode(void* it, void* param); 
extern void fxLabelNodeCode(void* it, void* param); 
extern void fxMemberNodeCode(void* it, void* param); 
extern void fxMemberNodeCodeAssign(void* it, void* param); 
extern void fxMemberNodeCodeReference(void* it, void* param); 
extern void fxMemberAtNodeCode(void* it, void* param); 
extern void fxMemberAtNodeCodeAssign(void* it, void* param); 
extern void fxMemberAtNodeCodeReference(void* it, void* param); 
extern void fxModuleNodeCode(void* it, void* param); 
extern void fxNewNodeCode(void* it, void* param); 
extern void fxNumberNodeCode(void* it, void* param); 
extern void fxObjectNodeCode(void* it, void* param); 
extern void fxObjectBindingNodeCodeAssign(void* it, void* param); 
extern void fxOrExpressionNodeCode(void* it, void* param); 
extern void fxParamsBindingNodeCode(void* it, void* param); 
extern void fxPostfixExpressionNodeCode(void* it, void* param); 
extern void fxProgramNodeCode(void* it, void* param); 
extern void fxQuestionMarkNodeCode(void* it, void* param); 
extern void fxRegexpNodeCode(void* it, void* param);
extern void fxReturnNodeCode(void* it, void* param); 
extern void fxStatementNodeCode(void* it, void* param); 
extern void fxStatementsNodeCode(void* it, void* param); 
extern void fxStringNodeCode(void* it, void* param); 
extern void fxSuperNodeCode(void* it, void* param); 
extern void fxSwitchNodeCode(void* it, void* param); 
extern void fxTemplateNodeCode(void* it, void* param);
extern void fxThisNodeCode(void* it, void* param);
extern void fxThrowNodeCode(void* it, void* param); 
extern void fxTryNodeCode(void* it, void* param); 
extern void fxUnaryExpressionNodeCode(void* it, void* param);
extern void fxUndefinedNodeCodeAssign(void* it, void* param);
extern void fxUndefinedNodeCodeReference(void* it, void* param);
extern void fxValueNodeCode(void* it, void* param);
extern void fxWhileNodeCode(void* it, void* param); 
extern void fxWithNodeCode(void* it, void* param);
extern void fxYieldNodeCode(void* it, void* param); 

#endif /* __XS6SCRIPT__ */















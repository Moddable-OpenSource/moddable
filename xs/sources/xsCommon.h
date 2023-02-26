/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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

#ifndef __XS6COMMON__
#define __XS6COMMON__

#include "xsPlatform.h"

#ifndef mx_dtoa
	#define mx_dtoa 1
#endif
#if __GNUC__ >= 5
	#if ESP32
		#undef __has_builtin
		#define __has_builtin(x) 1
	#endif
#endif
#if !defined(__has_builtin)
	#define __has_builtin(x) 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef txS1 txByte;
typedef txU1 txFlag;
#ifdef mx32bitID
	typedef txU4 txID;
#else
	typedef txU2 txID;
#endif
typedef txU4 txIndex;
typedef txS1 txKind;
typedef txS4 txSize;
typedef txS4 txError;

typedef txS4 txBoolean;
typedef txS4 txInteger;
typedef double txNumber;
typedef char* txString;
typedef txU4 txUnsigned;
typedef int (*txGetter)(void*);
typedef int (*txPutter)(txString, void*);

typedef struct {
	txU4* data;
	txU2 size;
	txU1 sign;
} txBigInt;

#define mxBigIntIsNaN(x) ((x)->size == 0)
#define mxBigIntHighWord(x)		((txU4)((x) >> 32))
#define mxBigIntLowWord(x)		((txU4)(x))
#define mxBigIntWordSize		(sizeof(txU4) * 8)

#define XS_ATOM_ARCHIVE 0x58535F41 /* 'XS_A' */
#define XS_ATOM_BINARY 0x58535F42 /* 'XS_B' */
#define XS_ATOM_ERROR 0x58535F45 /* 'XS_E' */
#define XS_ATOM_CHECKSUM 0x43484B53 /* 'CHKS' */
#define XS_ATOM_CODE 0x434F4445 /* 'CODE' */
#define XS_ATOM_DATA 0x44415441 /* 'DATA' */
#define XS_ATOM_HOSTS 0x484F5354 /* 'HOST' */
#define XS_ATOM_IDENTIFIERS 0x4944454E /* 'IDEN' */
#define XS_ATOM_MAPS 0x4D415053 /* 'MAPS' */
#define XS_ATOM_MODULES 0x4D4F4453 /* 'MODS' */
#define XS_ATOM_NAME 0x4E414D45 /* 'NAME' */
#define XS_ATOM_PATH 0x50415448 /* 'PATH' */
#define XS_ATOM_RESOURCES 0x52535243 /* 'RSRC' */
#define XS_ATOM_SIGNATURE 0x5349474E /* 'SIGN' */
#define XS_ATOM_SYMBOLS 0x53594D42 /* 'SYMB' */
#define XS_ATOM_VERSION 0x56455253 /* 'VERS' */
#define XS_MAJOR_VERSION 13
#define XS_MINOR_VERSION 3
#define XS_PATCH_VERSION 0

#define XS_DIGEST_SIZE 16
#define XS_VERSION_SIZE 4

typedef struct {
	txS4 atomSize;
	txU4 atomType;
} Atom;

typedef struct {
	void* callback;
	txS1* symbolsBuffer;
	txSize symbolsSize;
	txS1* codeBuffer;
	txSize codeSize;
	txS1* hostsBuffer;
	txSize hostsSize;
	txString path;
	txS1 version[4];
} txScript;

typedef struct {
	txS4 size;
	txU4 cmask;
	txU4 cval;
	txS4 shift;
	txU4 lmask;
	txU4 lval;
} txUTF8Sequence;

enum {
	XS_NO_CODE = 0,
	XS_CODE_ADD,
	XS_CODE_ARGUMENT,
	XS_CODE_ARGUMENTS,
	XS_CODE_ARGUMENTS_SLOPPY,
	XS_CODE_ARGUMENTS_STRICT,
	XS_CODE_ARRAY,
	XS_CODE_ASYNC_FUNCTION,
	XS_CODE_ASYNC_GENERATOR_FUNCTION,
	XS_CODE_AT,
	XS_CODE_AWAIT,
	XS_CODE_BEGIN_SLOPPY,
	XS_CODE_BEGIN_STRICT,
	XS_CODE_BEGIN_STRICT_BASE,
	XS_CODE_BEGIN_STRICT_DERIVED,
	XS_CODE_BEGIN_STRICT_FIELD,
	XS_CODE_BIGINT_1,
	XS_CODE_BIGINT_2,
	XS_CODE_BIT_AND,
	XS_CODE_BIT_NOT,
	XS_CODE_BIT_OR,
	XS_CODE_BIT_XOR,
	XS_CODE_BRANCH_1,
	XS_CODE_BRANCH_2,
	XS_CODE_BRANCH_4,
	XS_CODE_BRANCH_CHAIN_1,
	XS_CODE_BRANCH_CHAIN_2,
	XS_CODE_BRANCH_CHAIN_4,
	XS_CODE_BRANCH_COALESCE_1,
	XS_CODE_BRANCH_COALESCE_2,
	XS_CODE_BRANCH_COALESCE_4,
	XS_CODE_BRANCH_ELSE_1,
	XS_CODE_BRANCH_ELSE_2,
	XS_CODE_BRANCH_ELSE_4,
	XS_CODE_BRANCH_IF_1,
	XS_CODE_BRANCH_IF_2,
	XS_CODE_BRANCH_IF_4,
	XS_CODE_BRANCH_STATUS_1,
	XS_CODE_BRANCH_STATUS_2,
	XS_CODE_BRANCH_STATUS_4,
	XS_CODE_CALL,
	XS_CODE_CATCH_1,
	XS_CODE_CATCH_2,
	XS_CODE_CATCH_4,
	XS_CODE_CHECK_INSTANCE,
	XS_CODE_CLASS,
	XS_CODE_CODE_1,
	XS_CODE_CODE_2,
	XS_CODE_CODE_4,
	XS_CODE_CODE_ARCHIVE_1,
	XS_CODE_CODE_ARCHIVE_2,
	XS_CODE_CODE_ARCHIVE_4,
	XS_CODE_CONST_CLOSURE_1,
	XS_CODE_CONST_CLOSURE_2,
	XS_CODE_CONST_LOCAL_1,
	XS_CODE_CONST_LOCAL_2,
	XS_CODE_CONSTRUCTOR_FUNCTION,
	XS_CODE_COPY_OBJECT,
	XS_CODE_CURRENT,
	XS_CODE_DEBUGGER,
	XS_CODE_DECREMENT,
	XS_CODE_DELETE_PROPERTY,
	XS_CODE_DELETE_PROPERTY_AT,
	XS_CODE_DELETE_SUPER,
	XS_CODE_DELETE_SUPER_AT,
	XS_CODE_DIVIDE,
	XS_CODE_DUB,
	XS_CODE_DUB_AT,
	XS_CODE_END,
	XS_CODE_END_ARROW,
	XS_CODE_END_BASE,
	XS_CODE_END_DERIVED,
	XS_CODE_ENVIRONMENT,
	XS_CODE_EQUAL,
	XS_CODE_EVAL,
	XS_CODE_EVAL_ENVIRONMENT,
	XS_CODE_EVAL_PRIVATE,
	XS_CODE_EVAL_REFERENCE,
	XS_CODE_EVAL_TAIL,
	XS_CODE_EXCEPTION,
	XS_CODE_EXPONENTIATION,
	XS_CODE_EXTEND,
	XS_CODE_FALSE,
	XS_CODE_FILE,
	XS_CODE_FOR_AWAIT_OF,
	XS_CODE_FOR_IN,
	XS_CODE_FOR_OF,
	XS_CODE_FUNCTION,
	XS_CODE_FUNCTION_ENVIRONMENT,
	XS_CODE_GENERATOR_FUNCTION,
	XS_CODE_GET_CLOSURE_1,
	XS_CODE_GET_CLOSURE_2,
	XS_CODE_GET_LOCAL_1,
	XS_CODE_GET_LOCAL_2,
	XS_CODE_GET_PRIVATE_1,
	XS_CODE_GET_PRIVATE_2,
	XS_CODE_GET_PROPERTY,
	XS_CODE_GET_PROPERTY_AT,
	XS_CODE_GET_RESULT,
	XS_CODE_GET_SUPER,
	XS_CODE_GET_SUPER_AT,
	XS_CODE_GET_THIS,
	XS_CODE_GET_THIS_VARIABLE,
	XS_CODE_GET_VARIABLE,
	XS_CODE_GLOBAL,
	XS_CODE_HAS_PRIVATE_1,
	XS_CODE_HAS_PRIVATE_2,
	XS_CODE_HOST,
	XS_CODE_IMPORT,
	XS_CODE_IMPORT_META,
	XS_CODE_IN,
	XS_CODE_INCREMENT,
	XS_CODE_INSTANCEOF,
	XS_CODE_INSTANTIATE,
	XS_CODE_INTEGER_1,
	XS_CODE_INTEGER_2,
	XS_CODE_INTEGER_4,
	XS_CODE_LEFT_SHIFT,
	XS_CODE_LESS,
	XS_CODE_LESS_EQUAL,
	XS_CODE_LET_CLOSURE_1,
	XS_CODE_LET_CLOSURE_2,
	XS_CODE_LET_LOCAL_1,
	XS_CODE_LET_LOCAL_2,
	XS_CODE_LINE,
	XS_CODE_MINUS,
	XS_CODE_MODULE,
	XS_CODE_MODULO,
	XS_CODE_MORE,
	XS_CODE_MORE_EQUAL,
	XS_CODE_MULTIPLY,
	XS_CODE_NAME,
	XS_CODE_NEW,
	XS_CODE_NEW_CLOSURE,
	XS_CODE_NEW_LOCAL,
	XS_CODE_NEW_PRIVATE_1,
	XS_CODE_NEW_PRIVATE_2,
	XS_CODE_NEW_PROPERTY,
	XS_CODE_NEW_PROPERTY_AT,
	XS_CODE_NEW_TEMPORARY,
	XS_CODE_NOT,
	XS_CODE_NOT_EQUAL,
	XS_CODE_NULL,
	XS_CODE_NUMBER,
	XS_CODE_OBJECT,
	XS_CODE_PLUS,
	XS_CODE_POP,
	XS_CODE_PROGRAM_ENVIRONMENT,
	XS_CODE_PROGRAM_REFERENCE,
	XS_CODE_PULL_CLOSURE_1,
	XS_CODE_PULL_CLOSURE_2,
	XS_CODE_PULL_LOCAL_1,
	XS_CODE_PULL_LOCAL_2,
	XS_CODE_REFRESH_CLOSURE_1,
	XS_CODE_REFRESH_CLOSURE_2,
	XS_CODE_REFRESH_LOCAL_1,
	XS_CODE_REFRESH_LOCAL_2,
	XS_CODE_REGEXP,
	XS_CODE_RESERVE_1,
	XS_CODE_RESERVE_2,
	XS_CODE_RESET_CLOSURE_1,
	XS_CODE_RESET_CLOSURE_2,
	XS_CODE_RESET_LOCAL_1,
	XS_CODE_RESET_LOCAL_2,
	XS_CODE_RETHROW,
	XS_CODE_RETRIEVE_1,
	XS_CODE_RETRIEVE_2,
	XS_CODE_RETRIEVE_TARGET,
	XS_CODE_RETRIEVE_THIS,
	XS_CODE_RETURN,
	XS_CODE_RUN,
	XS_CODE_RUN_1,
	XS_CODE_RUN_2,
	XS_CODE_RUN_4,
	XS_CODE_RUN_TAIL,
	XS_CODE_RUN_TAIL_1,
	XS_CODE_RUN_TAIL_2,
	XS_CODE_RUN_TAIL_4,
	XS_CODE_SET_CLOSURE_1,
	XS_CODE_SET_CLOSURE_2,
	XS_CODE_SET_HOME,
	XS_CODE_SET_LOCAL_1,
	XS_CODE_SET_LOCAL_2,
	XS_CODE_SET_PRIVATE_1,
	XS_CODE_SET_PRIVATE_2,
	XS_CODE_SET_PROPERTY,
	XS_CODE_SET_PROPERTY_AT,
	XS_CODE_SET_RESULT,
	XS_CODE_SET_SUPER,
	XS_CODE_SET_SUPER_AT,
	XS_CODE_SET_THIS,
	XS_CODE_SET_VARIABLE,
	XS_CODE_SIGNED_RIGHT_SHIFT,
	XS_CODE_START_ASYNC,
	XS_CODE_START_ASYNC_GENERATOR,
	XS_CODE_START_GENERATOR,
	XS_CODE_STORE_1,
	XS_CODE_STORE_2,
	XS_CODE_STORE_ARROW,
	XS_CODE_STRICT_EQUAL,
	XS_CODE_STRICT_NOT_EQUAL,
	XS_CODE_STRING_1,
	XS_CODE_STRING_2,
	XS_CODE_STRING_4,
	XS_CODE_STRING_ARCHIVE_1,
	XS_CODE_STRING_ARCHIVE_2,
	XS_CODE_STRING_ARCHIVE_4,
	XS_CODE_SUBTRACT,
	XS_CODE_SUPER,
	XS_CODE_SWAP,
	XS_CODE_SYMBOL,
	XS_CODE_TARGET,
	XS_CODE_TEMPLATE,
	XS_CODE_TEMPLATE_CACHE,
	XS_CODE_THIS,
	XS_CODE_THROW,
	XS_CODE_THROW_STATUS,
	XS_CODE_TO_INSTANCE,
	XS_CODE_TO_NUMERIC,
	XS_CODE_TO_STRING,
	XS_CODE_TRANSFER,
	XS_CODE_TRUE,
	XS_CODE_TYPEOF,
	XS_CODE_UNCATCH,
	XS_CODE_UNDEFINED,
	XS_CODE_UNSIGNED_RIGHT_SHIFT,
	XS_CODE_UNWIND_1,
	XS_CODE_UNWIND_2,
	XS_CODE_VAR_CLOSURE_1,
	XS_CODE_VAR_CLOSURE_2,
	XS_CODE_VAR_LOCAL_1,
	XS_CODE_VAR_LOCAL_2,
	XS_CODE_VOID,
	XS_CODE_WITH,
	XS_CODE_WITHOUT,
	XS_CODE_YIELD,
	XS_CODE_PROFILE,
	XS_CODE_COUNT
};

extern const txString gxCodeNames[XS_CODE_COUNT];
extern const txS1 gxCodeSizes[XS_CODE_COUNT] ICACHE_FLASH_ATTR;

enum {
	XS_NAME_FLAG = 1,
	XS_DONT_DELETE_FLAG = 2,
	XS_DONT_ENUM_FLAG = 4,
	XS_DONT_SET_FLAG = 8,
	XS_METHOD_FLAG = 16,
	XS_GETTER_FLAG = 32,
	XS_SETTER_FLAG = 64,
	XS_IMPORT_FLAG = 32,
	XS_IMPORT_META_FLAG = 64,
};

enum {
	mxEnumeratorIntrinsic = 0,
	mxCopyObjectIntrinsic,
	mxIntrinsicCount,
};

enum {
	mxCFlag = 1 << 0,
	mxDebugFlag = 1 << 1,
	mxEvalFlag = 1 << 2,
	mxProgramFlag = 1 << 3,
	mxStrictFlag = 1 << 4,
	mxSuperFlag = 1 << 5,
	mxTargetFlag = 1 << 6,
	mxFieldFlag = 1 << 15,
	mxFunctionFlag = 1 << 16,
	mxGeneratorFlag = 1 << 21,
};

extern void fxDeleteScript(txScript* script);

extern const txUTF8Sequence gxUTF8Sequences[];

extern txBoolean fxIsIdentifierFirst(txU4 c);
extern txBoolean fxIsIdentifierNext(txU4 c);
extern txBoolean fxIsSpace(txInteger character);
extern txString fxSkipSpaces(txString string);

extern txBoolean fxParseHexEscape(txString* string, txInteger* character);
extern txBoolean fxParseUnicodeEscape(txString* string, txInteger* character, txInteger braces, txInteger separator);
extern txString fxStringifyHexEscape(txString string, txInteger character);
extern txString fxStringifyUnicodeEscape(txString string, txInteger character, txInteger separator);

mxExport int fxUTF8Compare(txString p1, txString p2);
mxExport txString fxUTF8Decode(txString string, txInteger* character);
mxExport txString fxUTF8Encode(txString string, txInteger character);
mxExport txSize fxUTF8Length(txInteger character);

#if mxCESU8
mxExport txString fxCESU8Decode(txString string, txInteger* character);
mxExport txString fxCESU8Encode(txString string, txInteger character);
mxExport txSize fxCESU8Length(txInteger character);
#define mxStringByteDecode fxCESU8Decode
#define mxStringByteEncode fxCESU8Encode
#define mxStringByteLength fxCESU8Length
#else
#define mxStringByteDecode fxUTF8Decode
#define mxStringByteEncode fxUTF8Encode
#define mxStringByteLength fxUTF8Length
#endif

mxExport txSize fxUTF8ToUnicodeOffset(txString theString, txSize theOffset);
mxExport txSize fxUnicodeLength(txString theString);
mxExport txSize fxUnicodeToUTF8Offset(txString theString, txSize theOffset);

txFlag fxIntegerToIndex(void* dtoa, txInteger theInteger, txIndex* theIndex);
txFlag fxNumberToIndex(void* dtoa, txNumber theNumber, txIndex* theIndex);
txFlag fxStringToIndex(void* dtoa, txString theString, txIndex* theIndex);

/* ? */
mxExport char* fxCStackLimit();
mxExport txID fxGenerateProfileID(void* console);
mxExport void fxGenerateTag(void* console, txString buffer, txInteger bufferSize, txString path);
mxExport void fxVReport(void* console, txString theFormat, c_va_list theArguments);
mxExport void fxVReportError(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments);
mxExport void fxVReportWarning(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments);

/* xsdtoa.c */
extern void* fxNew_dtoa(void*);
extern void fxDelete_dtoa(void*);
mxExport txString fxIntegerToString(void* dtoa, txInteger theValue, txString theBuffer, txSize theSize);
mxExport txInteger fxNumberToInteger(txNumber theValue);
mxExport txString fxNumberToString(void* dtoa, txNumber theValue, txString theBuffer, txSize theSize, txByte theMode, txInteger thePrecision);
mxExport txNumber fxStringToNumber(void* dtoa, txString theString, txFlag whole);

/* xsre.c */
enum {
	XS_REGEXP_G = 1 << 0,
	XS_REGEXP_I = 1 << 1,
	XS_REGEXP_M = 1 << 2,
	XS_REGEXP_N = 1 << 3,
	XS_REGEXP_S = 1 << 4,
	XS_REGEXP_U = 1 << 5,
	XS_REGEXP_Y = 1 << 6,
	XS_REGEXP_D = 1 << 7,
};
mxExport txInteger* fxAllocateRegExpData(void* the, txInteger* code);
mxExport txBoolean fxCompileRegExp(void* the, txString pattern, txString modifier, txInteger** code, txInteger** data, txString errorBuffer, txInteger errorSize);
mxExport void fxDeleteRegExp(void* the, txInteger* code, txInteger* data);
mxExport txInteger fxMatchRegExp(void* the, txInteger* code, txInteger* data, txString subject, txInteger offset);

/* xsBigInt.c */

extern void fxBigIntEncode(txByte* code, txBigInt* bigint, txSize size);
extern txSize fxBigIntMaximum(txSize length);
extern txSize fxBigIntMaximumB(txSize length);
extern txSize fxBigIntMaximumO(txSize length);
extern txSize fxBigIntMaximumX(txSize length);
extern txSize fxBigIntMeasure(txBigInt* bigint);
extern void fxBigIntParse(txBigInt* bigint, txString string, txSize length, txInteger sign);
extern void fxBigIntParseB(txBigInt* bigint, txString string, txSize length);
extern void fxBigIntParseO(txBigInt* bigint, txString string, txSize length);
extern void fxBigIntParseX(txBigInt* bigint, txString string, txSize length);

#if mxBigEndian
#define mxDecode2(THE_CODE, THE_VALUE)	{ \
	txS1* src = (txS1*)(THE_CODE); \
	txS1* dst = (txS1*)&(THE_VALUE) + 1; \
	*dst-- = *src++; \
	*dst = *src++; \
	(THE_CODE) = (void *)src; \
	}
#else
#define mxDecode2(THE_CODE, THE_VALUE)	{ \
	txS1* src = (txS1*)(THE_CODE); \
	txS1* dst = (txS1*)&(THE_VALUE); \
	*dst++ = *src++; \
	*dst = *src++; \
	(THE_CODE) = (void *)src; \
	}
#endif

#if mxBigEndian
#define mxDecode4(THE_CODE, THE_VALUE)	{ \
	txS1* src = (THE_CODE); \
	txS1* dst = (txS1*)&(THE_VALUE) + 3; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst = *src++; \
	(THE_CODE) = src; \
	}
#else
#define mxDecode4(THE_CODE, THE_VALUE)	{ \
	txS1* src = (THE_CODE); \
	txS1* dst = (txS1*)&(THE_VALUE); \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst = *src++; \
	(THE_CODE) = src; \
	}
#endif

#if mxBigEndian
#define mxDecode8(THE_CODE, THE_VALUE)	{ \
	txS1* src = (THE_CODE); \
	txS1* dst = (txS1*)&(THE_VALUE) + 7; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst-- = *src++; \
	*dst = *src++; \
	(THE_CODE) = src; \
}
#else
#define mxDecode8(THE_CODE, THE_VALUE)	{ \
	txS1* src = (THE_CODE); \
	txS1* dst = (txS1*)&(THE_VALUE); \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst = *src++; \
	(THE_CODE) = src; \
	}
#endif

#ifdef mx32bitID
#define mxDecodeID(THE_CODE, THE_VALUE) mxDecode4(THE_CODE, THE_VALUE)	
#else
#define mxDecodeID(THE_CODE, THE_VALUE) mxDecode2(THE_CODE, THE_VALUE)	
#endif	

#if mxBigEndian
#define mxEncode2(THE_CODE, THE_VALUE)	{ \
	txByte* dst = (THE_CODE); \
	txByte* src = (txByte*)&(THE_VALUE) + 1; \
	*dst++ = *src--; \
	*dst++ = *src; \
	(THE_CODE) = dst; \
	}
#else
#define mxEncode2(THE_CODE, THE_VALUE)	{ \
	txByte* dst = (THE_CODE); \
	txByte* src = (txByte*)&(THE_VALUE); \
	*dst++ = *src++; \
	*dst++ = *src; \
	(THE_CODE) = dst; \
	}
#endif

#if mxBigEndian
#define mxEncode4(THE_CODE, THE_VALUE)	{ \
	txByte* dst = (THE_CODE); \
	txByte* src = (txByte*)&(THE_VALUE) + 3; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src; \
	(THE_CODE) = dst; \
	}
#else
#define mxEncode4(THE_CODE, THE_VALUE)	{ \
	txByte* dst = (THE_CODE); \
	txByte* src = (txByte*)&(THE_VALUE); \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src; \
	(THE_CODE) = dst; \
	}
#endif

#if mxBigEndian
#define mxEncode8(THE_CODE, THE_VALUE)	{ \
	txByte* dst = (THE_CODE); \
	txByte* src = (txByte*)&(THE_VALUE) + 7; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src--; \
	*dst++ = *src; \
	(THE_CODE) = dst; \
	}
#else
#define mxEncode8(THE_CODE, THE_VALUE)	{ \
	txByte* dst = (THE_CODE); \
	txByte* src = (txByte*)&(THE_VALUE); \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src++; \
	*dst++ = *src; \
	(THE_CODE) = dst; \
	}
#endif

#ifdef mx32bitID
#define mxEncodeID(THE_CODE, THE_VALUE) mxEncode4(THE_CODE, THE_VALUE)	
#else
#define mxEncodeID(THE_CODE, THE_VALUE) mxEncode2(THE_CODE, THE_VALUE)	
#endif	

enum {
	XS_NO_ID = 0,
	_Symbol_asyncIterator = 1,
	_Symbol_hasInstance,
	_Symbol_isConcatSpreadable,
	_Symbol_iterator,
	_Symbol_match,
	_Symbol_matchAll,
	_Symbol_replace,
	_Symbol_search,
	_Symbol_species,
	_Symbol_split,
	_Symbol_toPrimitive,
	_Symbol_toStringTag,
	_Symbol_unscopables,
	_AggregateError,
	_Array,
	_ArrayBuffer,
	_Atomics,
	_BigInt,
	_BigInt64Array,
	_BigUint64Array,
	_Boolean,
	_DataView,
	_Date,
	_Error,
	_EvalError,
	_FinalizationRegistry,
	_Float32Array,
	_Float64Array,
	_Int16Array,
	_Int32Array,
	_Int8Array,
	_JSON,
	_Map,
	_Math,
	_ModuleSource,
	_Number,
	_Object,
	_Promise,
	_Proxy,
	_RangeError,
	_ReferenceError,
	_Reflect,
	_RegExp,
	_Set,
	_SharedArrayBuffer,
	_String,
	_Symbol,
	_SyntaxError,
	_TypeError,
	_TypedArray,
	_URIError,
	_Uint16Array,
	_Uint32Array,
	_Uint8Array,
	_Uint8ClampedArray,
	_WeakMap,
	_WeakRef,
	_WeakSet,
	_decodeURI,
	_decodeURIComponent,
	_encodeURI,
	_encodeURIComponent,
	_escape,
	_isFinite,
	_isNaN,
	_parseFloat,
	_parseInt,
	_trace,
	_unescape,
	_Infinity,
	_NaN,
	_undefined,
	_Compartment,
	_Function,
	_eval,
	_AsyncFunction,
	_AsyncGeneratorFunction,
	_BYTES_PER_ELEMENT,
	_E,
	_EPSILON,
	_Generator,
	_GeneratorFunction,
	_LN10,
	_LN2,
	_LOG10E,
	_LOG2E,
	_MAX_SAFE_INTEGER,
	_MAX_VALUE,
	_MIN_SAFE_INTEGER,
	_MIN_VALUE,
	_NEGATIVE_INFINITY,
	_PI,
	_POSITIVE_INFINITY,
	_SQRT1_2,
	_SQRT2,
	_UTC,
	___defineGetter__,
	___defineSetter__,
	___lookupGetter__,
	___lookupSetter__,
	___proto__,
	_abs,
	_acos,
	_acosh,
	_add,
	_aliases,
	_all,
	_allSettled,
	_and,
	_any,
	_append,
	_apply,
	_arguments,
	_as,
	_asIntN,
	_asUintN,
	_asin,
	_asinh,
	_assign,
	_asyncIterator,
	_at,
	_atan,
	_atanh,
	_atan2,
	_bind,
	_bindings,
	_bitLength,
	_boundArguments,
	_boundFunction,
	_boundThis,
	_buffer,
	_busy,
	_byteLength,
	_byteOffset,
	_cache,
	_call,
	_callee,
	_caller,
	_catch,
	_cause,
	_cbrt,
	_ceil,
	_center,
	_charAt,
	_charCodeAt,
	_chunk,
	_chunkify,
	_cleanupSome,
	_clear,
	_closure,
	_clz32,
	_codePointAt,
	_compare,
	_compareExchange,
	_compile,
	_concat,
	_configurable,
	_console,
	_construct,
	_constructor,
	_copyWithin,
	_cos,
	_cosh,
	_count,
	_create,
	_default,
	_defineProperties,
	_defineProperty,
	_delete,
	_deleteProperty,
	_deref,
	_description,
	_done,
	_dotAll,
	_eachDown,
	_eachUp,
	_endsWith,
	_entries,
	_enumerable,
	_enumerate,
	_errors,
	_evaluate,
	_every,
	_exchange,
	_exec,
	_exp,
	_expm1,
	_export,
	_exports,
	_fill,
	_filter,
	_finally_,
	_find,
	_findIndex,
	_findLast,
	_findLastIndex,
	_flags,
	_flat,
	_flatMap,
	_floor,
	_for,
	_forEach,
	_free,
	_freeze,
	_from,
	_fromArrayBuffer,
	_fromBigInt,
	_fromCharCode,
	_fromCodePoint,
	_fromEntries,
	_fromString,
	_fround,
	_function,
	_get,
	_getBigInt64,
	_getBigUint64,
	_getDate,
	_getDay,
	_getFloat32,
	_getFloat64,
	_getFullYear,
	_getHours,
	_getInt16,
	_getInt32,
	_getInt8,
	_getMilliseconds,
	_getMinutes,
	_getMonth,
	_getOwnPropertyDescriptor,
	_getOwnPropertyDescriptors,
	_getOwnPropertyNames,
	_getOwnPropertySymbols,
	_getPrototypeOf,
	_getSeconds,
	_getTime,
	_getTimezoneOffset,
	_getUTCDate,
	_getUTCDay,
	_getUTCFullYear,
	_getUTCHours,
	_getUTCMilliseconds,
	_getUTCMinutes,
	_getUTCMonth,
	_getUTCSeconds,
	_getUint16,
	_getUint32,
	_getUint8,
	_getYear,
	_global,
	_globalThis,
	_groups,
	_grow,
	_growable,
	_has,
	_hasIndices,
	_hasInstance,
	_hasOwn,
	_hasOwnProperty,
	_hypot_,
	_id,
	_idiv,
	_idivmod,
	_ignoreCase,
	_imod,
	_import,
	_importNow,
	_imports,
	_imul,
	_imuldiv,
	_includes,
	_index,
	_indexOf,
	_indices,
	_input,
	_irem,
	_is,
	_isArray,
	_isConcatSpreadable,
	_isExtensible,
	_isFrozen,
	_isInteger,
	_isLockFree,
	_isPrototypeOf,
	_isSafeInteger,
	_isSealed,
	_isView,
	_iterable,
	_iterator,
	_join,
	_keyFor,
	_keys,
	_lastIndex,
	_lastIndexOf,
	_left,
	_length,
	_line,
	_load,
	_local,
	_localeCompare,
	_log,
	_log1p,
	_log10,
	_log2,
	_map,
	_match,
	_matchAll,
	_max,
	_maxByteLength,
	_message,
	_min,
	_mod,
	_module,
	_multiline,
	_name,
	_needsImport,
	_needsImportMeta,
	_new_target,
	_next,
	_normalize,
	_notify,
	_now,
	_of,
	_or,
	_ownKeys,
	_padEnd,
	_padStart,
	_parse,
	_path,
	_peek,
	_poke,
	_pop,
	_pow,
	_preventExtensions,
	_propertyIsEnumerable,
	_prototype,
	_proxy,
	_push,
	_race,
	_random,
	_raw,
	_reason,
	_reduce,
	_reduceRight,
	_reexports,
	_register,
	_reject,
	_repeat,
	_replace,
	_replaceAll,
	_resizable,
	_resize,
	_resolve,
	_result,
	_return,
	_reverse,
	_revocable,
	_revoke,
	_right,
	_round,
	_seal,
	_search,
	_set,
	_setBigInt64,
	_setBigUint64,
	_setDate,
	_setFloat32,
	_setFloat64,
	_setFullYear,
	_setHours,
	_setInt16,
	_setInt32,
	_setInt8,
	_setMilliseconds,
	_setMinutes,
	_setMonth,
	_setPrototypeOf,
	_setSeconds,
	_setTime,
	_setUTCDate,
	_setUTCFullYear,
	_setUTCHours,
	_setUTCMilliseconds,
	_setUTCMinutes,
	_setUTCMonth,
	_setUTCSeconds,
	_setUint16,
	_setUint32,
	_setUint8,
	_setYear,
	_shift,
	_sign,
	_sin,
	_sinh,
	_size,
	_slice,
	_some,
	_sort,
	_source,
	_species,
	_splice,
	_split,
	_sqrt,
	_stack,
	_startsWith,
	_status,
	_sticky,
	_store,
	_stringify,
	_sub,
	_subarray,
	_substr,
	_substring,
	_tan,
	_tanh,
	_test,
	_then,
	_this,
	_throw,
	_toDateString,
	_toExponential,
	_toFixed,
	_toGMTString,
	_toISOString,
	_toJSON,
	_toLocaleDateString,
	_toLocaleLowerCase,
	_toLocaleString,
	_toLocaleTimeString,
	_toLocaleUpperCase,
	_toLowerCase,
	_toPrecision,
	_toPrimitive,
	_toString,
	_toStringTag,
	_toTimeString,
	_toUTCString,
	_toUpperCase,
	_transfer,
	_transfers,
	_trim,
	_trimEnd,
	_trimLeft,
	_trimRight,
	_trimStart,
	_trunc,
	_unicode,
	_unregister,
	_unscopables,
	_unshift,
	_uri,
	_value,
	_valueOf,
	_values,
	_wait,
	_wake,
	_weak,
	_writable,
	_xor,
	__empty_string_,
	__xsbug_script_,
	XS_ID_COUNT
};
#define XS_SYMBOL_ID_COUNT _AggregateError
#define XS_INTRINSICS_COUNT _AsyncFunction

extern const txString gxIDStrings[XS_ID_COUNT];

#ifndef c_malloc_uint32
	#define c_malloc_uint32(byteCount) c_malloc(byteCount)
	#define c_free_uint32(ptr) c_free(ptr)
#endif

#ifndef c_isEmpty
	#define c_isEmpty(s) (!c_read8(s))
#endif

#define mxStringLength(_STRING) ((txSize)c_strlen(_STRING))

#define mxPtrDiff(_DIFF) ((txSize)(_DIFF))

#ifndef mxIntegerDivideOverflowException
	#define mxIntegerDivideOverflowException 1
#endif

#ifdef __cplusplus
}
#endif

#endif /* __XS6COMMON__ */

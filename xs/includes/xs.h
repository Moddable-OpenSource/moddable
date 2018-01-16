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

#ifndef __XS__
#define __XS__

#ifdef INCLUDE_XSPLATFORM
	#include "xsPlatform.h"
	#define xsMachinePlatform mxMachinePlatform
#else
	#define xsMachinePlatform
	#ifndef __XSPLATFORM__

	#include <setjmp.h>

	#define mxBigEndian 0
	#define mxLittleEndian 0

	#define mxiOS 0
	#define mxLinux 0
	#define mxMacOSX 0
	#define mxWindows 0

	#if defined(_MSC_VER)
		#if defined(_M_IX86) || defined(_M_X64)
			#undef mxLittleEndian
			#define mxLittleEndian 1
			#undef mxWindows
			#define mxWindows 1
			#ifdef mxDynamicLink
				#define mxExport extern __declspec( dllexport )
				#define mxImport extern __declspec( dllimport )
			#else
				#define mxExport extern
				#define mxImport extern
			#endif
		#else 
			#error unknown Microsoft compiler
		#endif
	#elif defined(__GNUC__) 

		#define _setjmp(buffer) setjmp(buffer)

		#if defined(__i386__) || defined(i386) || defined(intel) || defined(arm) || defined(__arm__) || defined(__k8__) || defined(__x86_64__)
			#undef mxLittleEndian
			#define mxLittleEndian 1
			#if defined(__linux__)
				#undef mxLinux
				#define mxLinux 1
				#define mxExport extern    
				#define mxImport extern
			#else
				#if defined(__APPLE__)
					#if defined(iphone)
						#undef mxiOS
						#define mxiOS 1
					#else
						#undef mxMacOSX
						#define mxMacOSX 1
					#endif
				#endif
				#ifdef mxDynamicLink
					#define mxExport extern __attribute__ ((visibility("default")))
					#define mxImport extern __attribute__ ((visibility("default")))
				#else
					#define mxExport extern
					#define mxImport extern
				#endif
			#endif
		#else
			#define mxExport extern
			#define mxImport extern
		#endif

	#else 
		#error unknown compiler
	#endif

	typedef signed char txS1;
	typedef unsigned char txU1;
	typedef short txS2;
	typedef unsigned short txU2;
	#if __LP64__
	typedef int txS4;
	typedef unsigned int txU4;
	#else
	typedef long txS4;
	typedef unsigned long txU4;
	#endif

	#if mxWindows
		#undef _setjmp
		#define _setjmp setjmp
	#else
		#include <errno.h>
	#endif

	#ifdef __GNUC__
		#define XS_FUNCTION_NORETURN __attribute__((noreturn))
		#define XS_FUNCTION_ANALYZER_NORETURN
		#if defined(__clang__)
			#if __has_feature(attribute_analyzer_noreturn)
				#undef XS_FUNCTION_ANALYZER_NORETURN
				#define XS_FUNCTION_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
			#endif
		#endif
	#endif
	#ifndef XS_FUNCTION_NORETURN
		#define XS_FUNCTION_NORETURN
	#endif
	#ifndef XS_FUNCTION_ANALYZER_NORETURN
		#define XS_FUNCTION_ANALYZER_NORETURN
	#endif
	
	#endif /* !__XSPLATFORM__ */
#endif /* !INCLUDE_XSPLATFORM */

#ifndef NULL
	#define NULL 0
#endif

#define fxPop() (*(the->stack++))
#define fxPush(_SLOT) (*(--the->stack) = (_SLOT))

#ifdef mxDebug
#define xsOverflow(_COUNT) \
	(fxOverflow(the,_COUNT,(char *)__FILE__,__LINE__))
#else
#define xsOverflow(_COUNT) \
	(fxOverflow(the,_COUNT,NULL,0))
#endif

#ifndef __XSALL__
typedef struct xsCreationRecord xsCreation;
typedef struct xsJumpRecord xsJump;
typedef struct xsMachineRecord xsMachine;
typedef struct xsSlotRecord xsSlot;
typedef struct xsHostBuilderRecord xsHostBuilder;
typedef struct xsHostHooksStruct xsHostHooks;
#else
typedef struct sxCreation xsCreation;
typedef struct sxJump xsJump;
typedef struct sxMachine xsMachine;
typedef struct sxSlot xsSlot;
typedef struct sxHostFunctionBuilder xsHostBuilder;
typedef struct sxHostHooks xsHostHooks;
#endif

/* Slot */

struct xsSlotRecord {
	void* data[4];
};

enum {
	xsUndefinedType,
	xsNullType,
	xsBooleanType,
	xsIntegerType,
	xsNumberType,
	xsStringType,
	xsStringXType,
	xsSymbolType,
	xsReferenceType
};
typedef char xsType;

#define xsTypeOf(_SLOT) \
	(the->scratch = (_SLOT), \
	fxTypeOf(the, &(the->scratch)))

/* Primitives */

typedef txS4 xsBooleanValue;
typedef txS4 xsIntegerValue;
typedef double xsNumberValue;
typedef char* xsStringValue;
typedef txU4 xsUnsignedValue;

#define xsUndefined \
	(fxUndefined(the, &the->scratch), \
	the->scratch)
#define xsNull \
	(fxNull(the, &the->scratch), \
	the->scratch)
#define xsFalse \
	(fxBoolean(the, &the->scratch, 0), \
	the->scratch)
#define xsTrue \
	(fxBoolean(the, &the->scratch, 1), \
	the->scratch)
	
#define xsBoolean(_VALUE) \
	(fxBoolean(the, &the->scratch, _VALUE), \
	the->scratch)
#define xsToBoolean(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToBoolean(the, &(the->scratch)))

#define xsInteger(_VALUE) \
	(fxInteger(the, &the->scratch, _VALUE), \
	the->scratch)
#define xsToInteger(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToInteger(the, &(the->scratch)))

#define xsNumber(_VALUE) \
	(fxNumber(the, &the->scratch, _VALUE), \
	the->scratch)
#define xsToNumber(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToNumber(the, &(the->scratch)))

#define xsString(_VALUE) \
	(fxString(the, &the->scratch, (xsStringValue)(_VALUE)), \
	the->scratch)
#define xsStringBuffer(_BUFFER,_SIZE) \
	(fxStringBuffer(the, &the->scratch, _BUFFER ,_SIZE), \
	the->scratch)
#define xsToString(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToString(the, &(the->scratch)))
#define xsToStringBuffer(_SLOT,_BUFFER,_SIZE) \
	(the->scratch = (_SLOT), \
	fxToStringBuffer(the, &(the->scratch), _BUFFER ,_SIZE))

#define xsArrayBuffer(_BUFFER,_SIZE) \
	(fxArrayBuffer(the, &the->scratch, _BUFFER, _SIZE), \
	the->scratch)
#define xsGetArrayBufferData(_SLOT,_OFFSET,_BUFFER,_SIZE) \
	(the->scratch = (_SLOT), \
	fxGetArrayBufferData(the, &(the->scratch), _OFFSET, _BUFFER, _SIZE))
#define xsGetArrayBufferLength(_SLOT) \
	(the->scratch = (_SLOT), \
	fxGetArrayBufferLength(the, &(the->scratch)))
#define xsSetArrayBufferData(_SLOT,_OFFSET,_BUFFER,_SIZE) \
	(the->scratch = (_SLOT), \
	fxSetArrayBufferData(the, &(the->scratch), _OFFSET, _BUFFER, _SIZE))
#define xsSetArrayBufferLength(_SLOT,_LENGTH) \
	(the->scratch = (_SLOT), \
	fxSetArrayBufferLength(the, &(the->scratch), _LENGTH))
#define xsToArrayBuffer(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToArrayBuffer(the, &(the->scratch)))

/* Closures and References */

#define xsClosure(_VALUE) \
	(fxClosure(the, &the->scratch, _VALUE), \
	the->scratch)
#define xsToClosure(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToClosure(the, &(the->scratch)))

#define xsReference(_VALUE) \
	(fxReference(the, &the->scratch, _VALUE), \
	the->scratch)
#define xsToReference(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToReference(the, &(the->scratch)))

/* Instances and Prototypes */

#define prototypesStackIndex -17
#define xsObjectPrototype (the->stackPrototypes[prototypesStackIndex - 1])
#define xsFunctionPrototype (the->stackPrototypes[prototypesStackIndex - 2])
#define xsArrayPrototype (the->stackPrototypes[prototypesStackIndex - 3])
#define xsStringPrototype (the->stackPrototypes[prototypesStackIndex - 4])
#define xsBooleanPrototype (the->stackPrototypes[prototypesStackIndex - 5])
#define xsNumberPrototype (the->stackPrototypes[prototypesStackIndex - 6])
#define xsDatePrototype (the->stackPrototypes[prototypesStackIndex - 7])
#define xsRegExpPrototype (the->stackPrototypes[prototypesStackIndex - 8])
#define xsHostPrototype (the->stackPrototypes[prototypesStackIndex - 9])
#define xsErrorPrototype (the->stackPrototypes[prototypesStackIndex - 10])
#define xsEvalErrorPrototype (the->stackPrototypes[prototypesStackIndex - 11])
#define xsRangeErrorPrototype (the->stackPrototypes[prototypesStackIndex - 12])
#define xsReferenceErrorPrototype (the->stackPrototypes[prototypesStackIndex - 13])
#define xsSyntaxErrorPrototype (the->stackPrototypes[prototypesStackIndex - 14])
#define xsTypeErrorPrototype (the->stackPrototypes[prototypesStackIndex - 15])
#define xsURIErrorPrototype (the->stackPrototypes[prototypesStackIndex - 16])
#define xsSymbolPrototype (the->stackPrototypes[prototypesStackIndex - 17])
#define xsArrayBufferPrototype (the->stackPrototypes[prototypesStackIndex - 18])
#define xsDataViewPrototype (the->stackPrototypes[prototypesStackIndex - 19])
#define xsTypedArrayPrototype (the->stackPrototypes[prototypesStackIndex - 20])
#define xsMapPrototype (the->stackPrototypes[prototypesStackIndex - 21])
#define xsSetPrototype (the->stackPrototypes[prototypesStackIndex - 22])
#define xsWeakMapPrototype (the->stackPrototypes[prototypesStackIndex - 23])
#define xsWeakSetPrototype (the->stackPrototypes[prototypesStackIndex - 24])
#define xsPromisePrototype (the->stackPrototypes[prototypesStackIndex - 25])
#define xsProxyPrototype (the->stackPrototypes[prototypesStackIndex - 26])

#define xsNewArray(_LENGTH) \
	(fxNewArray(the,_LENGTH), \
	fxPop())

#define xsNewObject() \
	(fxNewObject(the), \
	fxPop())

#define xsIsInstanceOf(_SLOT,_PROTOTYPE) \
	(xsOverflow(-2), \
	fxPush(_PROTOTYPE), \
	fxPush(_SLOT), \
	fxIsInstanceOf(the))

/* Identifiers */

typedef unsigned char xsFlag;
typedef short xsIndex;
#define XS_NO_ID -1

#define xsID(_NAME) \
	fxID(the, _NAME)
#define xsFindID(_NAME) \
	fxFindID(the, _NAME)
#define xsIsID(_NAME) \
	fxIsID(the, _NAME)
#define xsToID(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToID(the, &(the->scratch)))
#define xsName(_ID) \
	fxName(the, _ID)

/* Properties */

#define xsEnumerate(_THIS) \
	(xsOverflow(-1), \
	fxPush(_THIS), \
	fxEnumerate(the), \
	fxPop())
	
#define xsHas(_THIS,_ID) \
	(xsOverflow(-1), \
	fxPush(_THIS), \
	fxHasID(the, _ID))

#define xsHasAt(_THIS,_AT) \
	(xsOverflow(-2), \
	fxPush(_THIS), \
	fxPush(_AT), \
	fxHasAt(the))

#define xsGet(_THIS,_ID) \
	(xsOverflow(-1), \
	fxPush(_THIS), \
	fxGetID(the, _ID), \
	fxPop())

#define xsGetAt(_THIS,_AT) \
	(xsOverflow(-2), \
	fxPush(_THIS), \
	fxPush(_AT), \
	fxGetAt(the), \
	fxPop())

#define xsSet(_THIS,_ID,_SLOT) \
	(xsOverflow(-2), \
	fxPush(_SLOT), \
	fxPush(_THIS), \
	fxSetID(the, _ID), \
	the->stack++)

#define xsSetAt(_THIS,_AT,_SLOT) \
	(xsOverflow(-3), \
	fxPush(_SLOT), \
	fxPush(_THIS), \
	fxPush(_AT), \
	fxSetAt(the), \
	the->stack++)

#define xsDefine(_THIS,_ID,_SLOT,_ATTRIBUTES) \
	(xsOverflow(-2), \
	fxPush(_SLOT), \
	fxPush(_THIS), \
	fxDefineID(the, _ID, _ATTRIBUTES, _ATTRIBUTES | xsDontDelete | xsDontEnum | xsDontSet), \
	the->stack++)

#define xsDefineAt(_THIS,_AT,_SLOT,_ATTRIBUTES) \
	(xsOverflow(-3), \
	fxPush(_SLOT), \
	fxPush(_THIS), \
	fxPush(_AT), \
	fxDefineAt(the, _ATTRIBUTES, _ATTRIBUTES | xsDontDelete | xsDontEnum | xsDontSet), \
	the->stack++)

#define xsDelete(_THIS,_ID) \
	(xsOverflow(-1), \
	fxPush(_THIS), \
	fxDeleteID(the, _ID), \
	the->stack++)

#define xsDeleteAt(_THIS,_AT) \
	(xsOverflow(-2), \
	fxPush(_THIS), \
	fxPush(_AT), \
	fxDeleteAt(the), \
	the->stack++)

#define xsCall0(_THIS,_ID) \
	(xsOverflow(-2), \
	fxPushCount(the, 0), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall0_noResult(_THIS,_ID) \
	(xsOverflow(-2), \
	fxPushCount(the, 0), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall1(_THIS,_ID,_SLOT0) \
	(xsOverflow(-3), \
	fxPush(_SLOT0), \
	fxPushCount(the, 1), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall1_noResult(_THIS,_ID,_SLOT0) \
	(xsOverflow(-3), \
	fxPush(_SLOT0), \
	fxPushCount(the, 1), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall2(_THIS,_ID,_SLOT0,_SLOT1) \
	(xsOverflow(-4), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPushCount(the, 2), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall2_noResult(_THIS,_ID,_SLOT0,_SLOT1) \
	(xsOverflow(-4), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPushCount(the, 2), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall3(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-5), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPushCount(the, 3), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall3_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-5), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPushCount(the, 3), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall4(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-6), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPushCount(the, 4), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall4_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-6), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPushCount(the, 4), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall5(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4) \
	(xsOverflow(-7), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPushCount(the, 5), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall5_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4) \
	(xsOverflow(-7), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPushCount(the, 5), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall6(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5) \
	(xsOverflow(-8), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPushCount(the, 6), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall6_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5) \
	(xsOverflow(-8), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPushCount(the, 6), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall7(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6) \
	(xsOverflow(-9), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPushCount(the, 7), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall7_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6) \
	(xsOverflow(-9), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPushCount(the, 7), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCall8(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6,_SLOT7) \
	(xsOverflow(-10), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPush(_SLOT7), \
	fxPushCount(the, 8), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	fxPop())

#define xsCall8_noResult(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6,_SLOT7) \
	(xsOverflow(-10), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPush(_SLOT7), \
	fxPushCount(the, 8), \
	fxPush(_THIS), \
	fxCallID(the, _ID), \
	the->stack++)

#define xsCallFunction0(_FUNCTION,_THIS) \
	(xsOverflow(-3), \
	fxPushCount(the, 0), \
	fxPush(_THIS), \
	fxPush(_FUNCTION), \
	fxCall(the), \
	fxPop())

#define xsCallFunction1(_FUNCTION,_THIS,_SLOT0) \
	(xsOverflow(-4), \
	fxPush(_SLOT0), \
	fxPushCount(the, 1), \
	fxPush(_THIS), \
	fxPush(_FUNCTION), \
	fxCall(the), \
	fxPop())

#define xsCallFunction2(_FUNCTION,_THIS,_SLOT0,_SLOT1) \
	(xsOverflow(-5), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPushCount(the, 2), \
	fxPush(_THIS), \
	fxPush(_FUNCTION), \
	fxCall(the), \
	fxPop())

#define xsCallFunction3(_FUNCTION,_THIS,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-6), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPushCount(the, 3), \
	fxPush(_THIS), \
	fxPush(_FUNCTION), \
	fxCall(the), \
	fxPop())

#define xsCallFunction4(_FUNCTION,_THIS,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-7), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPushCount(the, 4), \
	fxPush(_THIS), \
	fxPush(_FUNCTION), \
	fxCall(the), \
	fxPop())

#define xsCallFunction5(_FUNCTION,_THIS,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4) \
	(xsOverflow(-8), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPushCount(the, 5), \
	fxPush(_THIS), \
	fxPush(_FUNCTION), \
	fxCall(the), \
	fxPop())

#define xsCallFunction6(_FUNCTION,_THIS,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5) \
	(xsOverflow(-9), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPushCount(the, 6), \
	fxPush(_THIS), \
	fxPush(_FUNCTION), \
	fxCall(the), \
	fxPop())

#define xsCallFunction7(_FUNCTION,_THIS,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6) \
	(xsOverflow(-10), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPushCount(the, 7), \
	fxPush(_THIS), \
	fxPush(_FUNCTION), \
	fxCall(the), \
	fxPop())

#define xsCallFunction8(_FUNCTION,_THIS,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6,_SLOT7) \
	(xsOverflow(-11), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPush(_SLOT7), \
	fxPushCount(the, 8), \
	fxPush(_THIS), \
	fxPush(_FUNCTION), \
	fxCall(the), \
	fxPop())

#define xsNew0(_THIS,_ID) \
	(xsOverflow(-2), \
	fxPushCount(the, 0), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew1(_THIS,_ID,_SLOT0) \
	(xsOverflow(-3), \
	fxPush(_SLOT0), \
	fxPushCount(the, 1), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew2(_THIS,_ID,_SLOT0,_SLOT1) \
	(xsOverflow(-4), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPushCount(the, 2), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew3(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-5), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPushCount(the, 3), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew4(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-6), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPushCount(the, 4), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew5(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4) \
	(xsOverflow(-7), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPushCount(the, 5), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew6(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5) \
	(xsOverflow(-8), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPushCount(the, 6), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew7(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6) \
	(xsOverflow(-9), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPushCount(the, 7), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())

#define xsNew8(_THIS,_ID,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6,_SLOT7) \
	(xsOverflow(-10), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxPush(_SLOT2), \
	fxPush(_SLOT3), \
	fxPush(_SLOT4), \
	fxPush(_SLOT5), \
	fxPush(_SLOT6), \
	fxPush(_SLOT7), \
	fxPushCount(the, 8), \
	fxPush(_THIS), \
	fxNewID(the, _ID), \
	fxPop())
	
#define xsTest(_SLOT) \
	(xsOverflow(-1), \
	fxPush(_SLOT), \
	fxRunTest(the))
	
/* Globals */

#define xsGlobal (the->stackTop[-1])

/* Host Constructors, Functions and Objects */

typedef void (*xsCallback)(xsMachine*);

struct xsHostBuilderRecord {
	xsCallback callback;
	xsIndex length;
	xsIndex id;
};
	
#define xsNewHostConstructor(_CALLBACK,_LENGTH,_PROTOTYPE) \
	(xsOverflow(-1), \
	fxPush(_PROTOTYPE), \
	fxNewHostConstructor(the, _CALLBACK, _LENGTH, xsNoID), \
	fxPop())
	
#define xsNewHostConstructorObject(_CALLBACK,_LENGTH,_PROTOTYPE, _NAME) \
	(xsOverflow(-1), \
	fxPush(_PROTOTYPE), \
	fxNewHostConstructor(the, _CALLBACK, _LENGTH, _NAME), \
	fxPop())
	
#define xsNewHostFunction(_CALLBACK,_LENGTH) \
	(fxNewHostFunction(the, _CALLBACK, _LENGTH, xsNoID), \
	fxPop())
	
#define xsNewHostFunctionObject(_CALLBACK,_LENGTH, _NAME) \
	(fxNewHostFunction(the, _CALLBACK, _LENGTH, _NAME), \
	fxPop())

#define xsNewHostInstance(_PROTOTYPE) \
	(xsOverflow(-1), \
	fxPush(_PROTOTYPE), \
	fxNewHostInstance(the), \
	fxPop())

typedef void (*xsDestructor)(void*);

#define xsNewHostObject(_DESTRUCTOR) \
	(fxNewHostObject(the, _DESTRUCTOR), \
	fxPop())

#define xsGetHostChunk(_SLOT) \
	(the->scratch = (_SLOT), \
	fxGetHostChunk(the, &(the->scratch)))
#define xsSetHostChunk(_SLOT,_DATA,_SIZE) \
	(the->scratch = (_SLOT), \
	fxSetHostChunk(the, &(the->scratch), _DATA, _SIZE))

#define xsGetHostData(_SLOT) \
	(the->scratch = (_SLOT), \
	fxGetHostData(the, &(the->scratch)))
#define xsSetHostData(_SLOT,_DATA) \
	(the->scratch = (_SLOT), \
	fxSetHostData(the, &(the->scratch), _DATA))
	
#define xsGetHostDestructor(_SLOT) \
	(the->scratch = (_SLOT), \
	fxGetHostDestructor(the, &(the->scratch)))
#define xsSetHostDestructor(_SLOT,_DESTRUCTOR) \
	(the->scratch = (_SLOT), \
	fxSetHostDestructor(the, &(the->scratch), _DESTRUCTOR))

#define xsGetHostHandle(_SLOT) \
	(the->scratch = (_SLOT), \
	fxGetHostHandle(the, &(the->scratch)))
	
typedef void (*xsMarkRoot)(xsMachine*, xsSlot*);
typedef void (*xsMarker)(xsMachine*, void*, xsMarkRoot);
typedef void (*xsSweepRoot)(xsMachine*, xsSlot*);
typedef void (*xsSweeper)(xsMachine*, void*, xsMarkRoot);
struct xsHostHooksStruct {
	xsDestructor destructor;
	xsMarker marker;
	xsSweeper sweeper;
};
	
#define xsGetHostHooks(_SLOT) \
	(the->scratch = (_SLOT), \
	fxGetHostHooks(the, &(the->scratch)))
#define xsSetHostHooks(_SLOT,_HOOKS) \
	(the->scratch = (_SLOT), \
	fxSetHostHooks(the, &(the->scratch), _HOOKS))

#define xsBuildHosts(_COUNT, _BUILDERS) \
	(fxBuildHosts(the, _COUNT, _BUILDERS), \
	fxPop())

/* Arguments and Variables */

#define xsVars(_COUNT) fxVars(the, _COUNT)

#define xsArg(_INDEX) (the->frame[6 + fxCheckArg(the, _INDEX)])
#define xsArgc (the->frame[5])
#define xsThis (the->frame[4])
#define xsFunction (the->frame[3])
#define xsTarget (the->frame[2])
#define xsResult (the->frame[1])
#define xsVarc (the->frame[-1])
#define xsVar(_INDEX) (the->frame[-2 - fxCheckVar(the, _INDEX)])
	
/* Garbage Collector */

#define xsCollectGarbage() \
	fxCollectGarbage(the)
#define xsEnableGarbageCollection(_ENABLE) \
	fxEnableGarbageCollection(the, _ENABLE)
#define xsRemember(_SLOT) \
	fxRemember(the, &(_SLOT))
#define xsForget(_SLOT) \
	fxForget(the, &(_SLOT))
#define xsAccess(_SLOT) \
	(fxAccess(the, &(_SLOT)), the->scratch)

/* Exceptions */

struct xsJumpRecord {
	jmp_buf buffer;
	xsJump* nextJump;
	xsSlot* stack;
	xsSlot* scope;
	xsSlot* frame;
	xsSlot* environment;
	xsIndex* code;
	xsBooleanValue flag;
};

#define xsException (the->stackTop[-2])

#ifdef mxDebug
#define xsThrow(_SLOT) \
	(the->stackTop[-2] = (_SLOT), \
	fxThrow(the,(char *)__FILE__,__LINE__))
#else
#define xsThrow(_SLOT) \
	(the->stackTop[-2] = (_SLOT), \
	fxThrow(the,NULL,0))
#endif

#define xsTry \
	xsJump __JUMP__; \
	__JUMP__.nextJump = the->firstJump; \
	__JUMP__.stack = the->stack; \
	__JUMP__.scope = the->scope; \
	__JUMP__.frame = the->frame; \
	__JUMP__.environment = NULL; \
	__JUMP__.code = the->code; \
	__JUMP__.flag = 0; \
	the->firstJump = &__JUMP__; \
	if (setjmp(__JUMP__.buffer) == 0) {

#define xsCatch \
		the->firstJump = __JUMP__.nextJump; \
	} \
	else for ( \
		the->stack = __JUMP__.stack, \
		the->scope = __JUMP__.scope, \
		the->frame = __JUMP__.frame, \
		the->code = __JUMP__.code, \
		the->firstJump = __JUMP__.nextJump; \
		(__JUMP__.stack); \
		__JUMP__.stack = NULL)

/* Errors */

#ifndef __XSALL__
	enum {
		XS_NO_ERROR = 0,
		XS_UNKNOWN_ERROR,
		XS_EVAL_ERROR,
		XS_RANGE_ERROR,
		XS_REFERENCE_ERROR,
		XS_SYNTAX_ERROR,
		XS_TYPE_ERROR,
		XS_URI_ERROR,
		XS_ERROR_COUNT
	};
#endif

#ifdef mxDebug
	#define xsUnknownError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_UNKNOWN_ERROR, __VA_ARGS__)
	#define xsEvalError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_EVAL_ERROR, __VA_ARGS__)
	#define xsRangeError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_RANGE_ERROR, __VA_ARGS__)
	#define xsReferenceError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_REFERENCE_ERROR, __VA_ARGS__)
	#define xsSyntaxError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_SYNTAX_ERROR, __VA_ARGS__)
	#define xsTypeError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_TYPE_ERROR, __VA_ARGS__)
	#define xsURIError(...) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_URI_ERROR, __VA_ARGS__)
#else
	#define xsUnknownError(...) fxThrowMessage(the, NULL, 0, XS_UNKNOWN_ERROR, __VA_ARGS__)
	#define xsEvalError(...) fxThrowMessage(the, NULL, 0, XS_EVAL_ERROR, __VA_ARGS__)
	#define xsRangeError(...) fxThrowMessage(the, NULL, 0, XS_RANGE_ERROR, __VA_ARGS__)
	#define xsReferenceError(...) fxThrowMessage(the, NULL, 0, XS_REFERENCE_ERROR, __VA_ARGS__)
	#define xsSyntaxError(...) fxThrowMessage(the, NULL, 0, XS_SYNTAX_ERROR, __VA_ARGS__)
	#define xsTypeError(...) fxThrowMessage(the, NULL, 0, XS_TYPE_ERROR, __VA_ARGS__)
	#define xsURIError(...) fxThrowMessage(the, NULL, 0, XS_URI_ERROR, __VA_ARGS__)
#endif

/* Platform */

#ifdef mxDebug
	#define xsAssert(it)\
		if (!(it)) fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_UNKNOWN_ERROR, "%s", #it)
	#define xsErrorPrintf(_MESSAGE) \
		fxThrowMessage(the, (char *)__FILE__, __LINE__, XS_UNKNOWN_ERROR, "%s", _MESSAGE)
#else
	#define xsAssert(it)\
		if (!(it)) fxThrowMessage(the, NULL, 0, XS_UNKNOWN_ERROR, "%s", #it)
	#define xsErrorPrintf(_MESSAGE) \
		fxThrowMessage(the, NULL, 0, XS_UNKNOWN_ERROR, "%s", _MESSAGE)
#endif

/* Debugger */
	
#ifdef mxDebug
	#define xsDebugger() \
		fxDebugger(the,(char *)__FILE__,__LINE__)
#else
	#define xsDebugger() \
		fxDebugger(the,NULL,0)
#endif

#define xsTrace(_STRING) \
	fxReport(the, "%s", _STRING)
#define xsLog(...) \
	fxReport(the, __VA_ARGS__)

#if defined(mxDebug) || 1
	#define xsLogDebug(...) \
		fxReport(__VA_ARGS__)
#else
	#define xsLogDebug(...)
#endif

typedef xsCallback (*xsCallbackAt)(xsIndex);

/* Machine */

struct xsMachineRecord {
	xsSlot* stack;
	xsSlot* scope;
	xsSlot* frame;
	xsIndex* code;
	xsSlot* stackBottom;
	xsSlot* stackTop;
	xsSlot* stackPrototypes;
	xsJump* firstJump;
	void* context;
	void* archive;
	xsSlot scratch;
#ifndef __XSALL__
	xsMachinePlatform
#endif
};

struct xsCreationRecord {
	xsIntegerValue initialChunkSize;
	xsIntegerValue incrementalChunkSize;
	xsIntegerValue initialHeapCount;
	xsIntegerValue incrementalHeapCount;
	xsIntegerValue stackCount;
	xsIntegerValue keyCount;
	xsIntegerValue nameModulo;
	xsIntegerValue symbolModulo;
	xsIntegerValue staticSize;
};

#define xsCreateMachine(_CREATION,_NAME,_CONTEXT) \
	fxCreateMachine(_CREATION, _NAME, _CONTEXT)
	
#define xsDeleteMachine(_THE) \
	fxDeleteMachine(_THE)
	
#define xsCloneMachine(_ALLOCATION,_MACHINE,_NAME,_CONTEXT) \
	fxCloneMachine(_ALLOCATION, _MACHINE, _NAME, _CONTEXT)
	
#define xsShareMachine(_THE) \
	fxShareMachine(_THE)

/* Context */	
	
#define xsGetContext(_THE) \
	((_THE)->context)

#define xsSetContext(_THE,_CONTEXT) \
	((_THE)->context = (_CONTEXT))

/* Host */

#define xsBeginHost(_THE) \
	do { \
		xsMachine* __HOST_THE__ = _THE; \
		xsJump __HOST_JUMP__; \
		__HOST_JUMP__.nextJump = (__HOST_THE__)->firstJump; \
		__HOST_JUMP__.stack = (__HOST_THE__)->stack; \
		__HOST_JUMP__.scope = (__HOST_THE__)->scope; \
		__HOST_JUMP__.frame = (__HOST_THE__)->frame; \
		__HOST_JUMP__.environment = NULL; \
		__HOST_JUMP__.code = (__HOST_THE__)->code; \
		__HOST_JUMP__.flag = 0; \
		(__HOST_THE__)->firstJump = &__HOST_JUMP__; \
		if (setjmp(__HOST_JUMP__.buffer) == 0) { \
			xsMachine* the = fxBeginHost(__HOST_THE__)

#ifndef XSLOGEXCEPTION
	#ifndef __ets__
		#define XSLOGEXCEPTION xsLogDebug(__HOST_THE__, (xsStringValue)"unhandled exception arrived in function %s at line %d", __FUNCTION__, __LINE__)
	#else
		// cannoot have __FUNCTION__ as the string is stored in RAM
		#define XSLOGEXCEPTION xsLogDebug(__HOST_THE__, (xsStringValue)"unhandled exception")
	#endif
#endif

#define xsEndHost(_THE) \
			fxEndHost(the); \
			the = NULL; \
		} \
		else \
			XSLOGEXCEPTION; \
		(__HOST_THE__)->stack = __HOST_JUMP__.stack, \
		(__HOST_THE__)->scope = __HOST_JUMP__.scope, \
		(__HOST_THE__)->frame = __HOST_JUMP__.frame, \
		(__HOST_THE__)->code = __HOST_JUMP__.code, \
		(__HOST_THE__)->firstJump = __HOST_JUMP__.nextJump; \
		break; \
	} while(1)

enum {	
	xsNoID = -1,
	xsDefault = 0,
	xsDontDelete = 2,
	xsDontEnum = 4,
	xsDontSet = 8,
	xsStatic = 16,
	xsIsGetter = 32,
	xsIsSetter = 64,
	xsChangeAll = 30
};
typedef unsigned char xsAttribute;
	
#define xsArrayCacheBegin(_ARRAY) \
	(fxPush(_ARRAY), \
	fxArrayCacheBegin(the, the->stack), \
	the->stack++)
#define xsArrayCacheEnd(_ARRAY) \
	(fxPush(_ARRAY), \
	fxArrayCacheEnd(the, the->stack), \
	the->stack++)
#define xsArrayCacheItem(_ARRAY,_ITEM) \
	(fxPush(_ARRAY), \
	fxPush(_ITEM), \
	fxArrayCacheItem(the, the->stack + 1, the->stack), \
	the->stack += 2)

#define xsModulePaths() \
	(fxModulePaths(the), \
	fxPop())

#define xsDemarshall(_DATA) \
	(fxDemarshall(the, (_DATA), 0), \
	fxPop())
#define xsDemarshallAlien(_DATA) \
	(fxDemarshall(the, (_DATA), 1), \
	fxPop())
#define xsMarshall(_SLOT) \
	(xsOverflow(-1), \
	fxPush(_SLOT), \
	fxMarshall(the, 0))
#define xsMarshallAlien(_SLOT) \
	(xsOverflow(-1), \
	fxPush(_SLOT), \
	fxMarshall(the, 1))

#define xsIsProfiling() \
	fxIsProfiling(the)
#define xsStartProfiling() \
	fxStartProfiling(the)
#define xsStopProfiling() \
	fxStopProfiling(the)

#define xsInitializeSharedCluster fxInitializeSharedCluster
#define xsTerminateSharedCluster fxTerminateSharedCluster

#ifndef __XSALL__

#ifdef __cplusplus
extern "C" {
#endif

mxImport xsType fxTypeOf(xsMachine*, xsSlot*);

mxImport void fxPushCount(xsMachine*, xsIntegerValue);
mxImport void fxUndefined(xsMachine*, xsSlot*);
mxImport void fxNull(xsMachine*, xsSlot*);
mxImport void fxBoolean(xsMachine*, xsSlot*, xsBooleanValue);
mxImport xsBooleanValue fxToBoolean(xsMachine*, xsSlot*);
mxImport void fxInteger(xsMachine*, xsSlot*, xsIntegerValue);
mxImport xsIntegerValue fxToInteger(xsMachine*, xsSlot*);
mxImport void fxNumber(xsMachine*, xsSlot*, xsNumberValue);
mxImport xsNumberValue fxToNumber(xsMachine*, xsSlot*);
mxImport void fxString(xsMachine*, xsSlot*, xsStringValue);
mxImport void fxStringBuffer(xsMachine*, xsSlot*, xsStringValue, xsIntegerValue);
mxImport xsStringValue fxToString(xsMachine*, xsSlot*);
mxImport xsStringValue fxToStringBuffer(xsMachine*, xsSlot*, xsStringValue, xsIntegerValue);
mxImport void fxUnsigned(xsMachine*, xsSlot*, xsUnsignedValue);
mxImport xsUnsignedValue fxToUnsigned(xsMachine*, xsSlot*);

mxImport void fxArrayBuffer(xsMachine*, xsSlot*, void*, xsIntegerValue);
mxImport void fxGetArrayBufferData(xsMachine*, xsSlot*, xsIntegerValue, void*, xsIntegerValue);
mxImport xsIntegerValue fxGetArrayBufferLength(xsMachine*, xsSlot*);
mxImport void fxSetArrayBufferData(xsMachine*, xsSlot*, xsIntegerValue, void*, xsIntegerValue);
mxImport void fxSetArrayBufferLength(xsMachine*, xsSlot*, xsIntegerValue);
mxImport void* fxToArrayBuffer(xsMachine*, xsSlot*);

mxImport void fxClosure(xsMachine*, xsSlot*, xsSlot*);
mxImport xsSlot* fxToClosure(xsMachine*, xsSlot*);
mxImport void fxReference(xsMachine*, xsSlot*, xsSlot*);
mxImport xsSlot* fxToReference(xsMachine*, xsSlot*);

mxImport void fxNewArray(xsMachine*, xsIntegerValue);
mxImport void fxNewObject(xsMachine*);
mxImport xsBooleanValue fxIsInstanceOf(xsMachine*);
mxImport void fxArrayCacheBegin(xsMachine*, xsSlot*);
mxImport void fxArrayCacheEnd(xsMachine*, xsSlot*);
mxImport void fxArrayCacheItem(xsMachine*, xsSlot*, xsSlot*);

mxImport void fxBuildHosts(xsMachine*, xsIntegerValue, xsHostBuilder*);
mxImport void fxNewHostConstructor(xsMachine*, xsCallback, xsIntegerValue, xsIntegerValue);
mxImport void fxNewHostFunction(xsMachine*, xsCallback, xsIntegerValue, xsIntegerValue);
mxImport void fxNewHostInstance(xsMachine*);
mxImport void fxNewHostObject(xsMachine*, xsDestructor);
mxImport void* fxGetHostChunk(xsMachine*, xsSlot*);
mxImport void *fxSetHostChunk(xsMachine*, xsSlot*, void*, xsIntegerValue);
mxImport void* fxGetHostData(xsMachine*, xsSlot*);
mxImport void fxSetHostData(xsMachine*, xsSlot*, void*);
mxImport xsDestructor fxGetHostDestructor(xsMachine*, xsSlot*);
mxImport void fxSetHostDestructor(xsMachine*, xsSlot*, xsDestructor);
mxImport void* fxGetHostHandle(xsMachine*, xsSlot*);
mxImport xsHostHooks* fxGetHostHooks(xsMachine*, xsSlot*);
mxImport void fxSetHostHooks(xsMachine*, xsSlot*, xsHostHooks*);

mxImport xsIndex fxID(xsMachine*, const char*);
mxImport xsIndex fxFindID(xsMachine*, char*);
mxImport xsBooleanValue fxIsID(xsMachine*, char*);
mxImport xsIndex fxToID(xsMachine*, xsSlot*);
mxImport char* fxName(xsMachine*, xsIndex);

mxImport void fxEnumerate(xsMachine* the);
mxImport xsBooleanValue fxHasAt(xsMachine*);
mxImport xsBooleanValue fxHasID(xsMachine*, xsIntegerValue);
mxImport void fxGet(xsMachine*, xsSlot*, xsIntegerValue);
mxImport void fxGetAt(xsMachine*);
mxImport void fxGetID(xsMachine*, xsIntegerValue);
mxImport void fxSet(xsMachine*, xsSlot*, xsIntegerValue);
mxImport void fxSetAt(xsMachine*);
mxImport void fxSetID(xsMachine*, xsIntegerValue);
mxImport void fxDefineAt(xsMachine*, xsAttribute, xsAttribute);
mxImport void fxDefineID(xsMachine*, xsIntegerValue, xsAttribute, xsAttribute);
mxImport void fxDeleteAt(xsMachine*);
mxImport void fxDeleteID(xsMachine*, xsIntegerValue);
mxImport void fxCall(xsMachine*);
mxImport void fxCallID(xsMachine*, xsIntegerValue);
mxImport void fxNew(xsMachine*);
mxImport void fxNewID(xsMachine*, xsIntegerValue);
mxImport xsBooleanValue fxRunTest(xsMachine* the);

mxImport void fxVars(xsMachine*, xsIntegerValue);
mxImport xsIntegerValue fxCheckArg(xsMachine*, xsIntegerValue);
mxImport xsIntegerValue fxCheckVar(xsMachine*, xsIntegerValue);
mxImport void fxOverflow(xsMachine*, xsIntegerValue, xsStringValue, xsIntegerValue);

mxImport void fxThrow(xsMachine*, xsStringValue, xsIntegerValue) XS_FUNCTION_NORETURN;
mxImport void fxThrowMessage(xsMachine* the, xsStringValue thePath, xsIntegerValue theLine, xsIntegerValue theError, xsStringValue theMessage, ...) XS_FUNCTION_NORETURN;

mxImport void fxDebugger(xsMachine*, xsStringValue, xsIntegerValue);
mxImport void fxReport(xsMachine*, xsStringValue, ...);

mxImport xsMachine* fxCreateMachine(xsCreation*, xsStringValue, void*);
mxImport void fxDeleteMachine(xsMachine*);
mxImport xsMachine* fxCloneMachine(xsCreation*, xsMachine*, xsStringValue, void*);
mxImport void fxShareMachine(xsMachine*);

mxImport xsMachine* fxBeginHost(xsMachine*);
mxImport void fxEndHost(xsMachine*);

mxImport void fxCollectGarbage(xsMachine*);
mxImport void fxEnableGarbageCollection(xsMachine* the, xsBooleanValue enableIt);

mxImport xsSlot* fxDuplicateSlot(xsMachine*, xsSlot*);
mxImport void* fxNewChunk(xsMachine*, xsIntegerValue);
mxImport void* fxRenewChunk(xsMachine*, void*, xsIntegerValue);

mxImport void fxRemember(xsMachine*, xsSlot*);
mxImport void fxForget(xsMachine*, xsSlot*);
mxImport void fxAccess(xsMachine*, xsSlot*);

mxImport void fxDecodeURI(xsMachine*, xsStringValue);
mxImport void fxEncodeURI(xsMachine*, xsStringValue);

mxImport xsStringValue fxUTF8Decode(xsStringValue string, xsIntegerValue* character);
mxImport xsStringValue fxUTF8Encode(xsStringValue string, xsIntegerValue character);
mxImport xsIntegerValue fxUTF8Length(xsIntegerValue character);
mxImport xsIntegerValue fxUTF8ToUnicodeOffset(xsStringValue theString, xsIntegerValue theOffset);
mxImport xsIntegerValue fxUnicodeLength(xsStringValue theString);
mxImport xsIntegerValue fxUnicodeToUTF8Offset(xsStringValue theString, xsIntegerValue theOffset);

mxImport xsStringValue fxIntegerToString(xsMachine*, xsIntegerValue, xsStringValue, xsIntegerValue);
mxImport xsStringValue fxNumberToString(xsMachine*, xsNumberValue, xsStringValue, xsIntegerValue, char theMode, xsIntegerValue thePrecision);
mxImport xsNumberValue fxStringToNumber(xsMachine*, xsStringValue theString, unsigned char whole);

mxImport void fxDemarshall(xsMachine*, void*, xsBooleanValue);
mxImport void* fxMarshall(xsMachine*, xsBooleanValue);
mxImport void fxModulePaths(xsMachine*);

mxImport xsBooleanValue fxIsProfiling(xsMachine*);
mxImport void fxStartProfiling(xsMachine*);
mxImport void fxStopProfiling(xsMachine*);
	
mxImport void* fxMapArchive(const unsigned char *, unsigned long, xsStringValue, xsCallbackAt);
mxImport void fxUnmapArchive(void*);
mxImport void fxRunLoop(xsMachine*);
mxImport void fxRunProgram(xsMachine*, xsStringValue);
mxImport void fxRunModule(xsMachine*, xsStringValue);

mxImport void fxInitializeSharedCluster();
mxImport void fxTerminateSharedCluster();

mxImport xsBooleanValue fxCompileRegExp(xsMachine* the, xsStringValue pattern, xsStringValue modifier, xsIntegerValue** code, xsIntegerValue** data, xsStringValue errorBuffer, xsIntegerValue errorSize);
mxImport void fxDeleteRegExp(xsMachine* the, xsIntegerValue* code, xsIntegerValue* data);
mxImport xsBooleanValue fxMatchRegExp(xsMachine* the, xsIntegerValue* code, xsIntegerValue* data, xsStringValue subject, xsIntegerValue offset);

#ifdef __cplusplus
}
#endif

#endif /* !__XSALL__ */

#endif /* __XS__ */


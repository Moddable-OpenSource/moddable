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

#ifndef mxBoundsCheck
	#define mxBoundsCheck 1
#endif

#define	XS_PROFILE_COUNT (256 * 1024)

static txSlot* fxCheckHostObject(txMachine* the, txSlot* it);

#ifdef mxFrequency
static void fxReportFrequency(txMachine* the);
#endif

/* Slot */

txKind fxTypeOf(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_STRING_X_KIND)
		return XS_STRING_KIND;
	return theSlot->kind;
}

/* Primitives */

void fxPushCount(txMachine* the, txInteger count)
{
	mxPushInteger(count);
}

void fxUndefined(txMachine* the, txSlot* theSlot)
{
	theSlot->kind = XS_UNDEFINED_KIND;
}

void fxNull(txMachine* the, txSlot* theSlot)
{
	theSlot->kind = XS_NULL_KIND;
}

void fxBoolean(txMachine* the, txSlot* theSlot, txS1 theValue)
{
	theSlot->value.boolean = theValue;
	theSlot->kind = XS_BOOLEAN_KIND;
}

txBoolean fxToBoolean(txMachine* the, txSlot* theSlot)
{
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
		theSlot->kind = XS_BOOLEAN_KIND;
		theSlot->value.boolean = 0;
		break;
	case XS_BOOLEAN_KIND:
		break;
	case XS_INTEGER_KIND:
		theSlot->kind = XS_BOOLEAN_KIND;
		theSlot->value.boolean = (theSlot->value.integer == 0) ? 0 : 1;
		break;
	case XS_NUMBER_KIND:
		theSlot->kind = XS_BOOLEAN_KIND;
		switch (c_fpclassify(theSlot->value.number)) {
		case FP_NAN:
		case FP_ZERO:
			theSlot->value.boolean = 0;
			break;
		default:
			theSlot->value.boolean = 1;
			break;
		}
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		theSlot->kind = XS_BOOLEAN_KIND;
		if (c_isEmpty(theSlot->value.string))
			theSlot->value.boolean = 0;
		else
			theSlot->value.boolean = 1;
		break;
#ifdef mxHostFunctionPrimitive
	case XS_HOST_FUNCTION_KIND:
#endif
	case XS_SYMBOL_KIND:
	case XS_REFERENCE_KIND:
		theSlot->kind = XS_BOOLEAN_KIND;
		theSlot->value.boolean = 1;
		break;
	default:
		mxTypeError("Cannot coerce to boolean");
		break;
	}
	return theSlot->value.boolean;
}

void fxInteger(txMachine* the, txSlot* theSlot, txInteger theValue)
{
	theSlot->value.integer = theValue;
	theSlot->kind = XS_INTEGER_KIND;
}

txInteger fxToInteger(txMachine* the, txSlot* theSlot)
{
#if mxOptimize
	if (XS_INTEGER_KIND == theSlot->kind)
		return theSlot->value.integer;				// this is the case over 90% of the time, so avoid the switch
#endif

again:
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
		theSlot->kind = XS_INTEGER_KIND;
		theSlot->value.integer = 0;
		break;
	case XS_BOOLEAN_KIND:
		theSlot->kind = XS_INTEGER_KIND;
		if (theSlot->value.boolean == 0)
			theSlot->value.integer = 0;
		else
			theSlot->value.integer = 1;
		break;
	case XS_INTEGER_KIND:
		break;
	case XS_NUMBER_KIND:
		theSlot->kind = XS_INTEGER_KIND;
		switch (c_fpclassify(theSlot->value.number)) {
		case C_FP_INFINITE:
		case C_FP_NAN:
		case C_FP_ZERO:
			theSlot->value.integer = 0;
			break;
		default: {
#ifdef __ets__
			theSlot->value.integer = (txInteger)theSlot->value.number;
#else
			#define MODULO 4294967296.0
			txNumber aNumber = c_fmod(c_trunc(theSlot->value.number), MODULO);
			if (aNumber >= MODULO / 2)
				aNumber -= MODULO;
			else if (aNumber < -MODULO / 2)
				aNumber += MODULO;
			theSlot->value.integer = (txInteger)aNumber;
#endif
			} break;
		}
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = fxStringToNumber(the->dtoa, theSlot->value.string, 1);
		goto again;
	case XS_SYMBOL_KIND:
		mxTypeError("Cannot coerce symbol to integer");
		break;
	case XS_REFERENCE_KIND:
		fxToPrimitive(the, theSlot, XS_NUMBER_HINT);
		goto again;
	default:
		mxTypeError("Cannot coerce to integer");
		break;
	}
	return theSlot->value.integer;
}

void fxNumber(txMachine* the, txSlot* theSlot, txNumber theValue)
{
	theSlot->value.number = theValue;
	theSlot->kind = XS_NUMBER_KIND;
}

txNumber fxToNumber(txMachine* the, txSlot* theSlot)
{
again:
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = C_NAN;
		break;
	case XS_NULL_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = 0;
		break;
	case XS_BOOLEAN_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		if (theSlot->value.boolean == 0)
			theSlot->value.number = 0;
		else
			theSlot->value.number = 1;
		break;
	case XS_INTEGER_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = theSlot->value.integer;
		break;
	case XS_NUMBER_KIND:
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = fxStringToNumber(the->dtoa, theSlot->value.string, 1);
		break;
	case XS_SYMBOL_KIND:
		mxTypeError("Cannot coerce symbol to number");
		break;
	case XS_REFERENCE_KIND:
		fxToPrimitive(the, theSlot, XS_NUMBER_HINT);
		goto again;
	default:
		mxTypeError("Cannot coerce to number");
		break;
	}
	return theSlot->value.number;
}

void fxString(txMachine* the, txSlot* theSlot, txString theValue)
{
	fxCopyStringC(the, theSlot, theValue);
}

void fxStringX(txMachine* the, txSlot* theSlot, txString theValue)
{
	theSlot->value.string = theValue;
	theSlot->kind = XS_STRING_X_KIND;
}

void fxStringBuffer(txMachine* the, txSlot* theSlot, txString theValue, txSize theSize)
{
	theSlot->value.string = (txString)fxNewChunk(the, theSize + 1);
	if (theValue)
		c_memcpy(theSlot->value.string, theValue, theSize);
	else
		theSlot->value.string[0] = 0;
	theSlot->value.string[theSize] = 0;
	theSlot->kind = XS_STRING_KIND;
}

txString fxToString(txMachine* the, txSlot* theSlot)
{
	char aBuffer[256];
again:
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
		fxCopyStringC(the, theSlot, "undefined");
		break;
	case XS_NULL_KIND:
		fxCopyStringC(the, theSlot, "null");
		break;
	case XS_BOOLEAN_KIND:
		if (theSlot->value.boolean == 0)
			fxCopyStringC(the, theSlot, "false");
		else
			fxCopyStringC(the, theSlot, "true");
		break;
	case XS_INTEGER_KIND:
		fxCopyStringC(the, theSlot, fxIntegerToString(the->dtoa, theSlot->value.integer, aBuffer, sizeof(aBuffer)));
		break;
	case XS_NUMBER_KIND:
		fxCopyStringC(the, theSlot, fxNumberToString(the->dtoa, theSlot->value.number, aBuffer, sizeof(aBuffer), 0, 0));
		break;
	case XS_SYMBOL_KIND:
		mxTypeError("Cannot coerce symbol to string");
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		break;
	case XS_REFERENCE_KIND:
		fxToPrimitive(the, theSlot, XS_STRING_HINT);
		goto again;
	default:
		mxTypeError("Cannot coerce to string");
		break;
	}
	return theSlot->value.string;
}

txString fxToStringBuffer(txMachine* the, txSlot* theSlot, txString theBuffer, txSize theSize)
{
	char* aString;
	txSize aSize;

	aString = fxToString(the, theSlot);
	aSize = c_strlen(aString) + 1;
	if (aSize > theSize)
		mxRangeError("Cannot buffer string");
	c_memcpy(theBuffer, aString, aSize);
	return theBuffer;
}

void fxUnsigned(txMachine* the, txSlot* theSlot, txUnsigned theValue)
{
	if (((txInteger)theValue) >= 0) {
		theSlot->value.integer = theValue;
		theSlot->kind = XS_INTEGER_KIND;
	}
	else {
		theSlot->value.number = theValue;
		theSlot->kind = XS_NUMBER_KIND;
	}
}

txUnsigned fxToUnsigned(txMachine* the, txSlot* theSlot)
{
	txUnsigned result;
again:
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
		theSlot->kind = XS_INTEGER_KIND;
		result = theSlot->value.integer = 0;
		break;
	case XS_BOOLEAN_KIND:
		theSlot->kind = XS_INTEGER_KIND;
		if (theSlot->value.boolean == 0)
			result = theSlot->value.integer = 0;
		else
			result = theSlot->value.integer = 1;
		break;
	case XS_INTEGER_KIND:
		if (theSlot->value.integer >= 0)
			return (txUnsigned)theSlot->value.integer;
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = theSlot->value.integer;
		// continue
	case XS_NUMBER_KIND:
		theSlot->kind = XS_INTEGER_KIND;
		switch (c_fpclassify(theSlot->value.number)) {
		case C_FP_INFINITE:
		case C_FP_NAN:
		case C_FP_ZERO:
			result = theSlot->value.integer = 0;
			break;
		default: {
			#define MODULO 4294967296.0
			txNumber aNumber = c_fmod(c_trunc(theSlot->value.number), MODULO);
			if (aNumber < 0)
				aNumber += MODULO;
			result = (txUnsigned)aNumber;
			if (((txInteger)result) >= 0) {
				theSlot->kind = XS_INTEGER_KIND;
				theSlot->value.integer = (txInteger)result;
			}
			else {
				theSlot->kind = XS_NUMBER_KIND;
				theSlot->value.number = aNumber;
			}
			} break;
		}
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = fxStringToNumber(the->dtoa, theSlot->value.string, 1);
		goto again;
	case XS_SYMBOL_KIND:
		result = 0;
		mxTypeError("Cannot coerce symbol to unsigned");
		break;
	case XS_REFERENCE_KIND:
		fxToPrimitive(the, theSlot, XS_NUMBER_HINT);
		goto again;
	default:
		result = 0;
		mxTypeError("Cannot coerce to unsigned");
		break;
	}
	return result;
}

/* Closures and References */

void fxClosure(txMachine* the, txSlot* theSlot, txSlot* theClosure)
{
	theSlot->value.closure = theClosure;
	theSlot->kind = XS_CLOSURE_KIND;
}

txSlot* fxToClosure(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_CLOSURE_KIND)
		return theSlot->value.closure;
	return NULL;
}

void fxReference(txMachine* the, txSlot* theSlot, txSlot* theReference)
{
	theSlot->value.reference = theReference;
	theSlot->kind = XS_REFERENCE_KIND;
}

txSlot* fxToReference(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_REFERENCE_KIND)
		return theSlot->value.reference;
	return NULL;
}

/* Instances and Prototypes */

txSlot* fxNewArray(txMachine* the, txInteger size)
{
	txSlot* instance;
	mxPush(mxArrayPrototype);
	instance = fxNewArrayInstance(the);
	fxSetIndexSize(the, instance->next, size);
	return instance;
}

txSlot* fxNewObject(txMachine* the)
{
	mxPush(mxObjectPrototype);
	return fxNewObjectInstance(the);
}

txBoolean fxIsInstanceOf(txMachine* the)
{
	txBoolean result = 0;
	txSlot* theInstance = the->stack++;
	txSlot* thePrototype = the->stack++;

	if (mxIsReference(theInstance) && mxIsReference(thePrototype)) {
		theInstance	= fxGetInstance(the, theInstance);
		thePrototype = fxGetInstance(the, thePrototype);
		while (theInstance) {
			if (theInstance == thePrototype) {
				result = 1;
				break;
			}
			theInstance = theInstance->value.instance.prototype;
		}
	}
	return result;
}

void fxArrayCacheBegin(txMachine* the, txSlot* reference)
{
	txSlot* array = reference->value.reference->next;
	array->value.array.address = C_NULL;
	array->value.array.length = 0;
}

void fxArrayCacheEnd(txMachine* the, txSlot* reference)
{
	txSlot* array = reference->value.reference->next;
	txIndex length = array->value.array.length;
	if (length) {
		txSlot *srcSlot, *dstSlot;
		array->value.array.address = (txSlot*)fxNewChunk(the, length * sizeof(txSlot));
		srcSlot = array->next;
		dstSlot = array->value.array.address + length;
		while (srcSlot) {
			dstSlot--;
			length--;
			dstSlot->next = C_NULL;
			dstSlot->ID = XS_NO_ID;
			dstSlot->flag = XS_NO_FLAG;
			dstSlot->kind = srcSlot->kind;
			dstSlot->value = srcSlot->value;
			*((txIndex*)dstSlot) = length;
			srcSlot = srcSlot->next;
		}
		array->next = C_NULL;
	}
}

void fxArrayCacheItem(txMachine* the, txSlot* reference, txSlot* item)
{
	txSlot* array = reference->value.reference->next;
	txSlot* slot = fxNewSlot(the);
	slot->next = array->next;
	slot->kind = item->kind;
	slot->value = item->value;
	array->next = slot;
	array->value.array.length++;
}

/* Host Constructors, Functions and Objects */

void fxBuildHosts(txMachine* the, txInteger c, const txHostFunctionBuilder* builder)
{
	mxPushInteger(c);
	mxPushInteger(1);
	mxPush(mxArrayPrototype);
	fxNewArrayInstance(the);
	fxArrayCacheBegin(the, the->stack);
	while (c) {
		if (builder->length >= 0) {
		#ifdef mxHostFunctionPrimitive
			mxPushUndefined();
			the->stack->kind = XS_HOST_FUNCTION_KIND;
			the->stack->value.hostFunction.builder = builder;
			the->stack->value.hostFunction.IDs = (txID*)(the->code);
		#else
			fxNewHostFunction(the, builder->callback, builder->length, (the->code && (builder->id >= 0)) ? ((txID*)(the->code))[builder->id] : builder->id);
		#endif
		}
		else
			fxNewHostObject(the, (txDestructor)builder->callback);
		fxArrayCacheItem(the, the->stack + 1, the->stack);
		the->stack++;
		c--;
		builder++;
	}
	fxArrayCacheEnd(the, the->stack);
}

txSlot* fxNewHostConstructor(txMachine* the, txCallback theCallback, txInteger theLength, txInteger name)
{
	txSlot* aStack;
	txSlot* instance;
	txSlot* property;

	fxToInstance(the, the->stack);
	aStack = the->stack;
	instance = fxNewHostFunction(the, theCallback, theLength, name);
	instance->flag |= XS_CAN_CONSTRUCT_FLAG;
	property = fxLastProperty(the, instance);
	fxNextSlotProperty(the, property, aStack, mxID(_prototype), XS_GET_ONLY);
	property = mxBehaviorSetProperty(the, fxGetInstance(the, aStack), mxID(_constructor), XS_NO_ID, XS_ANY);
	property->flag = XS_DONT_ENUM_FLAG;
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	*aStack = *the->stack;
	the->stack++;
	return instance;
}

txSlot* fxNewHostFunction(txMachine* the, txCallback theCallback, txInteger theLength, txInteger name)
{
	txSlot* instance;
	txSlot* property;

	mxPushUndefined();
	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = mxFunctionPrototype.value.reference;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;

	/* CALLBACK */
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_CALLBACK_KIND;
	property->value.callback.address = theCallback;
	property->value.callback.IDs = C_NULL;

	/* HOME */
	property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_HOME_KIND;
	property->value.home.object = C_NULL;
	if (the->frame && (mxFunction->kind == XS_REFERENCE_KIND) && (mxIsFunction(mxFunction->value.reference))) {
		txSlot* slot = mxFunctionInstanceHome(mxFunction->value.reference);
		property->value.home.module = slot->value.home.module;
	}
	else
		property->value.home.module = C_NULL;

#ifdef mxProfile
	/* PROFILE */
	property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_INTEGER_KIND;
	property->value.integer = the->profileID;
	the->profileID++;
#endif

#ifndef mxNoFunctionLength
	/* LENGTH */
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->ID = mxID(_length);
	property->kind = XS_INTEGER_KIND;
	property->value.integer = theLength;
#endif

	/* NAME */
	if (name != XS_NO_ID)
		fxRenameFunction(the, instance, name, XS_NO_ID, C_NULL);

	return instance;
}

txSlot* fxNewHostInstance(txMachine* the)
{
	txSlot* prototype = fxGetInstance(the, the->stack);
	txSlot* instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = prototype;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;
	if (prototype) {
		txSlot* prototypeHost = prototype->next;
		if (prototypeHost && (prototypeHost->kind == XS_HOST_KIND)) {
			txSlot* instanceHost = instance->next = fxNewSlot(the);
			instanceHost->ID = XS_NO_ID;
			instanceHost->flag = XS_INTERNAL_FLAG | (prototypeHost->flag & ~XS_MARK_FLAG);
			instanceHost->kind = XS_HOST_KIND;
			instanceHost->value.host.data = C_NULL;
			instanceHost->value.host.variant.destructor = prototypeHost->value.host.variant.destructor;
		}
	}
	return instance;
}


txSlot* fxCheckHostObject(txMachine* the, txSlot* it)
{
	txSlot* result = C_NULL;
	if (it->kind == XS_REFERENCE_KIND) {
		it = it->value.reference;
		if (it->next) {
			it = it->next;
			if ((it->flag & XS_INTERNAL_FLAG) && (it->kind == XS_HOST_KIND))
				result = it;
		}
	}
	return result;
}

txSlot* fxNewHostObject(txMachine* the, txDestructor theDestructor)
{
	txSlot* anInstance;
	txSlot* aProperty;

	mxPushUndefined();

	anInstance = fxNewSlot(the);
	anInstance->kind = XS_INSTANCE_KIND;
	anInstance->value.instance.garbage = C_NULL;
	anInstance->value.instance.prototype = mxObjectPrototype.value.reference;
	the->stack->value.reference = anInstance;
	the->stack->kind = XS_REFERENCE_KIND;

	aProperty = anInstance->next = fxNewSlot(the);
	aProperty->ID = XS_NO_ID;
	aProperty->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_HOST_KIND;
	aProperty->value.host.data = C_NULL;
	aProperty->value.host.variant.destructor = theDestructor;
	
	if (the->frame && (mxFunction->kind == XS_REFERENCE_KIND) && (mxIsFunction(mxFunction->value.reference))) {
		txSlot* slot = mxFunctionInstanceHome(mxFunction->value.reference);
		if (slot->value.home.module) {
			aProperty = aProperty->next = fxNewSlot(the);
			aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
			aProperty->kind = XS_REFERENCE_KIND;
			aProperty->value.reference = slot->value.home.module;
		}
	}
	return anInstance;
}

void* fxGetHostChunk(txMachine* the, txSlot* slot)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		if (host->flag & XS_HOST_CHUNK_FLAG)
			return host->value.host.data;
		mxSyntaxError("C: xsGetHostChunk: no host data");
	}
	mxSyntaxError("C: xsGetHostChunk: no host object");
	return NULL;
}

void* fxGetHostChunkIf(txMachine* the, txSlot* slot)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		if (host->flag & XS_HOST_CHUNK_FLAG)
			return host->value.host.data;
	}
	return NULL;
}

void* fxGetHostData(txMachine* the, txSlot* slot)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		if (!(host->flag & XS_HOST_CHUNK_FLAG))
			return host->value.host.data;
		mxSyntaxError("C: xsGetHostData: no host data");
	}
	mxSyntaxError("C: xsGetHostData: no host object");
	return NULL;
}

void* fxGetHostDataIf(txMachine* the, txSlot* slot)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		if (!(host->flag & XS_HOST_CHUNK_FLAG))
			return host->value.host.data;
	}
	return NULL;
}

txDestructor fxGetHostDestructor(txMachine* the, txSlot* slot)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		if (!(host->flag & XS_HOST_HOOKS_FLAG))
			return host->value.host.variant.destructor;
		mxSyntaxError("C: xsGetHostDestructor: no host destructor");
	}
	mxSyntaxError("C: xsGetHostDestructor: no host object");
	return NULL;
}

void* fxGetHostHandle(txMachine* the, txSlot* slot)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		return &host->value.host.data;
	}
	mxSyntaxError("C: xsGetHostData: no host object");
	return NULL;
}

txHostHooks* fxGetHostHooks(txMachine* the, txSlot* slot)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		if (host->flag & XS_HOST_HOOKS_FLAG)
			return host->value.host.variant.hooks;
		mxSyntaxError("C: xsGetHostHooks: no host hooks");
	}
	mxSyntaxError("C: xsGetHostHooks: no host object");
	return NULL;
}

void *fxSetHostChunk(txMachine* the, txSlot* slot, void* theValue, txSize theSize)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		host->flag |= XS_HOST_CHUNK_FLAG;
		host->value.host.data = fxNewChunk(the, theSize);
		if (theValue)
			c_memcpy(host->value.host.data, theValue, theSize);
		else
			c_memset(host->value.host.data, 0, theSize);
		return host->value.host.data;
	}
	else
		mxSyntaxError("C: xsSetHostData: no host object");

	return NULL;
}

void fxSetHostData(txMachine* the, txSlot* slot, void* theData)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		host->flag &= ~XS_HOST_CHUNK_FLAG;
		host->value.host.data = theData;
	}
	else
		mxSyntaxError("C: xsSetHostData: no host object");
}

void fxSetHostDestructor(txMachine* the, txSlot* slot, txDestructor theDestructor)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		host->flag &= ~XS_HOST_HOOKS_FLAG;
		host->value.host.variant.destructor = theDestructor;
	}
	else
		mxSyntaxError("C: xsSetHostDestructor: no host object");
}

void fxSetHostHooks(txMachine* the, txSlot* slot, txHostHooks* theHooks)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		host->flag |= XS_HOST_HOOKS_FLAG;
		host->value.host.variant.hooks = theHooks;
	}
	else
		mxSyntaxError("C: xsSetHostHooks: no host object");
}

/* Identifiers */

txID fxID(txMachine* the, txString theName)
{
	return fxNewNameC(the, theName);
}

txID fxFindID(txMachine* the, txString theName)
{
	return fxFindName(the, theName);
}

txS1 fxIsID(txMachine* the, txString theName)
{
	return fxFindName(the, theName) ? 1 : 0;
}

txID fxToID(txMachine* the, txSlot* theSlot)
{
	txString string = fxToString(the, theSlot);
	if (theSlot->kind == XS_STRING_KIND)
		return fxNewName(the, theSlot);
	return fxNewNameX(the, string);
}

txString fxName(txMachine* the, txID theID)
{
	return fxGetKeyName(the, theID);
}

/* Properties */

void fxEnumerate(txMachine* the) 
{
	mxPushInteger(0);
	/* SWAP THIS */
	the->scratch = *(the->stack);
	*(the->stack) = *(the->stack + 1);
	*(the->stack + 1) = the->scratch;
	/* FUNCTION */
	mxPush(mxEnumeratorFunction);
	fxCall(the);
}

void fxGetAll(txMachine* the, txInteger id, txIndex index)
{
	txBoolean flag = mxIsReference(the->stack) ? 1 : 0;
	txSlot* instance = (flag) ? the->stack->value.reference : fxToInstance(the, the->stack);
	txSlot* property = mxBehaviorGetProperty(the, instance, (txID)id, index, XS_ANY);
	if (!property) {
		the->stack->kind = XS_UNDEFINED_KIND;
	}
	else if (property->kind == XS_ACCESSOR_KIND) {
		txSlot* function = property->value.accessor.getter;
		if (mxIsFunction(function)) {
			the->stack->value.integer = 0;
			the->stack->kind = XS_INTEGER_KIND;
			/* THIS */
			if (flag)
				mxPushReference(instance);
			else
				mxPushSlot(instance->next);
			/* FUNCTION */
			mxPushReference(function);
			fxCall(the);
		}
		else
			the->stack->kind = XS_UNDEFINED_KIND;
	}
	else {
		the->stack->kind = property->kind;
		the->stack->value = property->value;
	}
}

void fxGetAt(txMachine* the)
{
	txSlot* at = fxAt(the, the->stack);
	the->stack++;
	fxGetAll(the, at->value.at.id, at->value.at.index);
}

void fxGetID(txMachine* the, txInteger id)
{
	if (id < 0)
		fxGetAll(the, id, XS_NO_ID);
	else
		fxGetAll(the, 0, (txIndex)id);
}

void fxGetIndex(txMachine* the, txIndex index)
{
	fxGetAll(the, 0, index);
}

txBoolean fxHasAt(txMachine* the)
{
	txSlot* at = fxAt(the, the->stack);
	txSlot* instance = fxToInstance(the, the->stack + 1);
	txBoolean result = mxBehaviorHasProperty(the, instance, at->value.at.id, at->value.at.index);
	the->stack += 2;
	return result;
}

txBoolean fxHasID(txMachine* the, txInteger id)
{
	txSlot* instance = fxToInstance(the, the->stack);
	txBoolean result = (id < 0) ? mxBehaviorHasProperty(the, instance, (txID)id, XS_NO_ID) : mxBehaviorHasProperty(the, instance, 0, (txIndex)id);
	mxPop();
	return result;
}

txBoolean fxHasIndex(txMachine* the, txIndex index)
{
	txSlot* instance = fxToInstance(the, the->stack);
	txBoolean result = mxBehaviorHasProperty(the, instance, 0, index);
	mxPop();
	return result;
}

void fxSetAll(txMachine* the, txInteger id, txIndex index)
{
	txSlot* instance = fxToInstance(the, the->stack);
	txSlot* property;
	the->scratch = *(the->stack);
	*(the->stack) = *(the->stack + 1);
	*(the->stack + 1) = the->scratch;
	property = mxBehaviorSetProperty(the, instance, (txID)id, index, XS_ANY);
	if (!property)
		mxDebugID(XS_TYPE_ERROR, "C: xsSet %s: not extensible", id);
	if (property->kind == XS_ACCESSOR_KIND) {
		txSlot* function = property->value.accessor.setter;
		if (!mxIsFunction(function))
			mxDebugID(XS_TYPE_ERROR, "C: xsSet %s: no setter", id);
		*(the->stack + 1) = *(the->stack);
		the->stack->kind = XS_INTEGER_KIND;
		the->stack->value.integer = 1;
		/* THIS */
		mxPushReference(instance);
		/* FUNCTION */
		mxPushReference(function);
		fxCall(the);
	}
	else {
		if (property->flag & XS_DONT_SET_FLAG)
			mxDebugID(XS_TYPE_ERROR, "C: xsSet %s: not writable", id);
		property->kind = the->stack->kind;
		property->value = the->stack->value;
		*(the->stack + 1) = *(the->stack);
		the->stack++;
	}
}

void fxSetAt(txMachine* the)
{
	txSlot* at = fxAt(the, the->stack);
	the->stack++;
	fxSetAll(the, at->value.at.id, at->value.at.index);
}

void fxSetID(txMachine* the, txInteger id)
{
	if (id < 0)
		fxSetAll(the, id, XS_NO_ID);
	else
		fxSetAll(the, 0, (txIndex)id);
}

void fxSetIndex(txMachine* the, txIndex index)
{
	fxSetAll(the, 0, index);
}

void fxDeleteAll(txMachine* the, txInteger id, txIndex index)
{
	txSlot* instance = fxToInstance(the, the->stack);
	if (!mxBehaviorDeleteProperty(the, instance, (txID)id, index))
		mxDebugID(XS_TYPE_ERROR, "delete %s: not configurable", id);
}

void fxDeleteAt(txMachine* the)
{
	txSlot* at = fxAt(the, the->stack);
	txSlot* instance = fxToInstance(the, the->stack + 1);
	the->stack++;
	if (!mxBehaviorDeleteProperty(the, instance, at->value.at.id, at->value.at.index))
		mxDebugID(XS_TYPE_ERROR, "delete %s: not configurable", at->value.at.id);
}

void fxDeleteID(txMachine* the, txInteger id)
{
	txSlot* instance = fxToInstance(the, the->stack);
	if (!mxBehaviorDeleteProperty(the, instance, (txID)id, XS_NO_ID))
		mxDebugID(XS_TYPE_ERROR, "delete %s: not configurable", id);
}

void fxDeleteIndex(txMachine* the, txIndex index)
{
	txSlot* instance = fxToInstance(the, the->stack);
	if (!mxBehaviorDeleteProperty(the, instance, 0, index))
		mxTypeError("delete %ld: not configurable", index);
}

void fxDefineAll(txMachine* the, txID id, txIndex index, txFlag flag, txFlag mask)
{
	txSlot* instance = fxToInstance(the, the->stack);
	txSlot* slot = the->stack + 1;
	if (mask & XS_GETTER_FLAG) {
		slot->value.accessor.getter = slot->value.reference;
		slot->value.accessor.setter = C_NULL;
		slot->kind = XS_ACCESSOR_KIND;
	}
	else if (mask & XS_SETTER_FLAG) {
		slot->value.accessor.setter = slot->value.reference;
		slot->value.accessor.getter = C_NULL;
		slot->kind = XS_ACCESSOR_KIND;
	}
	slot->flag = flag & XS_GET_ONLY;
	if (!mxBehaviorDefineOwnProperty(the, instance, id, index, slot, mask))
		mxTypeError("define %ld: not configurable", id);
	the->stack++;
}

void fxDefineAt(txMachine* the, txFlag flag, txFlag mask)
{
	txSlot* at = fxAt(the, the->stack++);
	fxDefineAll(the, at->value.at.id, at->value.at.index, flag, mask);
}

void fxDefineID(txMachine* the, txID id, txFlag flag, txFlag mask)
{
	fxDefineAll(the, id, XS_NO_ID, flag, mask);
}

void fxDefineIndex(txMachine* the, txIndex index, txFlag flag, txFlag mask)
{
	fxDefineAll(the, 0, index, flag, mask);
}

void fxCall(txMachine* the)
{
#ifdef mxHostFunctionPrimitive
	txSlot* function = the->stack;
	if (function->kind != XS_HOST_FUNCTION_KIND) {
		function = fxGetInstance(the, the->stack);
		if (!mxIsCallable(function)) {
			mxTypeError("C: xsCall: no function");
		}
	}
#else
	txSlot* function = fxGetInstance(the, the->stack);
	if (!mxIsCallable(function))
		mxTypeError("C: xsCall: no function");
#endif
	/* TARGET */
	mxPushUndefined();
	/* RESULT */
	mxPushUndefined();
	fxRunID(the, C_NULL, XS_NO_ID);
}

void fxCallID(txMachine* the, txInteger theID)
{
	mxPushUndefined();
	*the->stack = *(the->stack + 1);
	fxGetID(the, theID);
	fxCall(the);
}

void fxNew(txMachine* the)
{
	txSlot* function;
	function = fxGetInstance(the, the->stack);
	if (!mxIsCallable(function))
		mxTypeError("C: xsNew: no function");
	/* THIS */
	mxPushUninitialized();
	/* FUNCTION */
	the->scratch = *(the->stack);
	*(the->stack) = *(the->stack + 1);
	*(the->stack + 1) = the->scratch;
	/* TARGET */
	--(the->stack);
	*(the->stack) = *(the->stack + 1);
	/* RESULT */
	mxPushUndefined();
	fxRunID(the, C_NULL, XS_NO_ID);
}

void fxNewID(txMachine* the, txInteger theID)
{
	fxGetID(the, theID);
	fxNew(the);
}

txBoolean fxRunTest(txMachine* the)
{
	txBoolean result;
	
	switch (the->stack->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
		result = 0;
		break;
	case XS_BOOLEAN_KIND:
		result = the->stack->value.boolean;
		break;
	case XS_INTEGER_KIND:
		result = (the->stack->value.integer == 0) ? 0 : 1;
		break;
	case XS_NUMBER_KIND:
		switch (c_fpclassify(the->stack->value.number)) {
		case FP_NAN:
		case FP_ZERO:
			result = 0;
			break;
		default:
			result = 1;
			break;
		}
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		if (c_isEmpty(the->stack->value.string))
			result = 0;
		else
			result = 1;
		break;
	default:
		result = 1;
		break;
	}
	the->stack++;
	return result;
}

/* Arguments and Variables */

void fxVars(txMachine* the, txInteger theCount)
{
	txSlot* environment = the->frame - 1;
	if (environment != the->stack)
		mxSyntaxError("C: xsVars: too late");
	fxOverflow(the, theCount, C_NULL, 0);
	environment->value.environment.variable.count = theCount;
	while (theCount) {
		mxPushUndefined();
		theCount--;
	}
}

txInteger fxCheckArg(txMachine* the, txInteger theIndex)
{
	txInteger aCount = mxArgc;
#if mxBoundsCheck
#if !mxOptimize
	if ((theIndex < 0) || (aCount <= theIndex))
		mxSyntaxError("C: xsArg(%ld): invalid index", theIndex);
	return aCount - 1 - theIndex;
#else
	aCount -= theIndex;
	if (aCount >= 0)
		return aCount - 1;

	mxSyntaxError("C: xsArg(%ld): invalid index", theIndex);
	return 0;			// never happens
#endif
#else
	return aCount - theIndex - 1;
#endif
}

txInteger fxCheckVar(txMachine* the, txInteger theIndex)
{
#if mxBoundsCheck
	if ((theIndex < 0) || (mxVarc <= theIndex))
		mxSyntaxError("C: xsVar(%ld): invalid index", theIndex);
#endif
	return theIndex;
}

void fxOverflow(txMachine* the, txInteger theCount, txString thePath, txInteger theLine)
{
#if mxBoundsCheck
	txSlot* aStack = the->stack + theCount;
	if (theCount < 0) {
		if (aStack < the->stackBottom) {
			fxReport(the, "stack overflow (%ld)!\n", (the->stack - the->stackBottom) + theCount);
			fxJump(the);
		}
	}
	else if (theCount > 0) {
		if (aStack > the->stackTop) {
			fxReport(the, "stack overflow (%ld)!\n", theCount - (the->stackTop - the->stack));
			fxJump(the);
		}
	}
#endif
}

/* Exceptions */

void fxThrow(txMachine* the, txString path, txInteger line)
{
#ifdef mxDebug
	fxDebugThrow(the, path, line, "C: xsThrow");
#endif
	fxJump(the);
}

void fxThrowMessage(txMachine* the, txString path, txInteger line, txError error, txString format, ...)
{
	char message[128] = "";
	txInteger length = 0;
    va_list arguments;
    txSlot* slot;
	fxBufferFrameName(the, message, sizeof(message), the->frame, ": ");
	length = c_strlen(message);
    va_start(arguments, format);
    c_vsnprintf(message + length, sizeof(message) - length, format, arguments);
    va_end(arguments);
	if ((error <= XS_NO_ERROR) || (XS_ERROR_COUNT <= error))
		error = XS_UNKNOWN_ERROR;

    slot = fxNewSlot(the);
    slot->kind = XS_INSTANCE_KIND;
    slot->value.instance.garbage = C_NULL;
    slot->value.instance.prototype = mxErrorPrototypes(error).value.reference;
	mxException.kind = XS_REFERENCE_KIND;
	mxException.value.reference = slot;
	slot = fxNextStringProperty(the, slot, message, mxID(_message), XS_DONT_ENUM_FLAG);
#ifdef mxDebug
	fxDebugThrow(the, path, line, message);
#endif
	fxJump(the);
}

/* Debugger */

void fxDebugger(txMachine* the, txString thePath, txInteger theLine)
{
#ifdef mxDebug
	fxDebugLoop(the, thePath, theLine, "C: xsDebugger");
#endif
}

/* Machine */

const txByte gxNoCode[3] ICACHE_FLASH_ATTR = { XS_CODE_BEGIN_STRICT, 0, XS_CODE_END };

txMachine* fxCreateMachine(txCreation* theCreation, txString theName, void* theContext)
{
	txMachine* the = (txMachine* )c_calloc(sizeof(txMachine), 1);
	if (the) {
		txJump aJump;

		aJump.nextJump = C_NULL;
		aJump.stack = C_NULL;
		aJump.scope = C_NULL;
		aJump.frame = C_NULL;
		aJump.code = C_NULL;
		aJump.flag = 0;
		the->firstJump = &aJump;
		if (c_setjmp(aJump.buffer) == 0) {
			txInteger anIndex;

			the->dtoa = fxNew_dtoa(the);
			the->context = theContext;
			fxCreateMachinePlatform(the);

		#ifdef mxDebug
			the->name = theName;
		#endif
		#ifdef mxProfile
			the->profileID = 1;
			the->profileBottom = c_malloc(XS_PROFILE_COUNT * sizeof(txProfileRecord));
			if (!the->profileBottom)
				fxJump(the);
			the->profileCurrent = the->profileBottom;
			the->profileTop = the->profileBottom + XS_PROFILE_COUNT;
		#endif

			fxAllocate(the, theCreation);

            c_memset(the->nameTable, 0, the->nameModulo * sizeof(txSlot *));
			c_memset(the->symbolTable, 0, the->symbolModulo * sizeof(txSlot *));
			c_memset(the->keyArray, 0, theCreation->keyCount * sizeof(txSlot*));

			/* mxGlobal */
			mxPushUndefined();
			/* mxException */
			mxPushUndefined();
			/* mxHosts */
			mxPushUndefined();
			/* mxClosures */
			mxPushUndefined();
			/* mxModulePaths */
			mxPushUndefined();
			/* mxImportingModules */
			fxNewInstance(the);
			/* mxLoadingModules */
			fxNewInstance(the);
			/* mxLoadedModules */
			fxNewInstance(the);
			/* mxResolvingModules */
			fxNewInstance(the);
			/* mxRunningModules */
			fxNewInstance(the);
			/* mxRequiredModules */
			fxNewInstance(the);
			/* mxModules */
			mxPushUndefined();
			/* mxPendingJobs */
			fxNewInstance(the);
			/* mxRunningJobs */
			fxNewInstance(the);
			/* mxBreakpoints */
			mxPushList();
			/* mxHostInspectors */
			mxPushList();
			/* mxInstanceInspectors */
			mxPushList();

			for (anIndex = mxObjectPrototypeStackIndex; anIndex < mxEmptyCodeStackIndex; anIndex++)
				mxPushUndefined();

			/* mxEmptyCode */
			mxPushUndefined();
			the->stack->value.code.address = (txByte*)gxNoCode;
			the->stack->value.code.closures = C_NULL;
			the->stack->kind = XS_CODE_X_KIND;	
			/* mxEmptyString */
			mxPushStringC("");
			/* mxEmptyRegExp */
			mxPushStringC("(?:)");
			/* mxBooleanString */
			mxPushStringX("boolean");
			/* mxDefaultString */
			mxPushStringX("default");
			/* mxFunctionString */
			mxPushStringX("function");
			/* mxNumberString */
			mxPushStringX("number");
			/* mxObjectString */
			mxPushStringX("object");
			/* mxStringString */
			mxPushStringX("string");
			/* mxSymbolString */
			mxPushStringX("symbol");
			/* mxUndefinedString */
			mxPushStringX("undefined");

			fxBuildKeys(the);
			fxBuildGlobal(the);
			fxBuildObject(the);
			fxBuildFunction(the);
			fxBuildGenerator(the);
			fxBuildArray(the);
			fxBuildString(the);
			fxBuildBoolean(the);
			fxBuildNumber(the);
			fxBuildDate(the);
			fxBuildMath(the);
			fxBuildRegExp(the);
			fxBuildError(the);
			fxBuildJSON(the);
			fxBuildDataView(the);
			fxBuildAtomics(the);
			fxBuildPromise(the);
			fxBuildSymbol(the);
			fxBuildProxy(the);
			fxBuildMapSet(the);
			fxBuildModule(the);

			mxPush(mxGlobal);
			fxNewEnvironmentInstance(the, &mxClosures);
			mxPull(mxClosures);

			mxPush(mxSetPrototype);
			fxNewSetInstance(the);
			mxPull(mxModulePaths);
			
			mxPush(mxObjectPrototype);
			fxNewWeakSetInstance(the);
			mxPull(mxModules);

            the->collectFlag = XS_COLLECTING_FLAG;
			
            /*{
				int c = 32;
				while (--c)
					fxCollectGarbage(the);
			}*/

		#ifdef mxDebug
			fxLogin(the);
		#endif

			the->firstJump = C_NULL;
		}
		else {
			fxFree(the);
			c_free(the);
			the = NULL;
		}
	}
	return the;
}

void fxDeleteMachine(txMachine* the)
{
	txSlot* aSlot;
	txSlot* bSlot;
	txSlot* cSlot;

	if (!(the->shared)) {
	#ifdef mxProfile
		fxStopProfiling(the);
	#endif
	#ifdef mxFrequency
		fxReportFrequency(the);
	#endif
	#ifdef mxDebug
		fxLogout(the);
	#endif
	}
	the->context = C_NULL;
	aSlot = the->cRoot;
	while (aSlot) {
		aSlot->flag |= XS_MARK_FLAG;
		aSlot = aSlot->next;
	}
	aSlot = the->firstHeap;
	while (aSlot) {
		bSlot = aSlot + 1;
		cSlot = aSlot->value.reference;
		while (bSlot < cSlot) {
			if ((bSlot->kind == XS_HOST_KIND) && (bSlot->value.host.variant.destructor)) {
				if (bSlot->flag & XS_HOST_HOOKS_FLAG) {
					if (bSlot->value.host.variant.hooks->destructor)
						(*(bSlot->value.host.variant.hooks->destructor))(bSlot->value.host.data);
				}
				else
					(*(bSlot->value.host.variant.destructor))(bSlot->value.host.data);
			}
			bSlot++;
		}
		aSlot = aSlot->next;
	}
	fxDelete_dtoa(the->dtoa);
	if (!(the->shared)) {
	#ifdef mxProfile
		if (the->profileBottom) {
			c_free(the->profileBottom);
			the->profileBottom = C_NULL;
			the->profileCurrent = C_NULL;
			the->profileTop = C_NULL;
		}
		if (the->profileDirectory) {
			c_free(the->profileDirectory);
		}
	#endif
	}
	fxDeleteMachinePlatform(the);
	fxFree(the);
	c_free(the);
}

txMachine* fxCloneMachine(txCreation* theCreation, txMachine* theMachine, txString theName, void* theContext)
{
	txMachine* the = (txMachine *)c_calloc(sizeof(txMachine), 1);
	if (the) {
		txJump aJump;

		aJump.nextJump = C_NULL;
		aJump.stack = C_NULL;
		aJump.scope = C_NULL;
		aJump.frame = C_NULL;
		aJump.code = C_NULL;
		aJump.flag = 0;
		the->firstJump = &aJump;
		if (c_setjmp(aJump.buffer) == 0) {
			txInteger anIndex;
			txSlot* aSlot;
			txSlot** aSlotAddress;
			txSlot* aSharedSlot;
			txSlot* aTemporarySlot;
			txID anID;

			the->dtoa = fxNew_dtoa(the);
			the->preparation = theMachine->preparation;
			the->context = theContext;
			the->archive = theMachine->archive;
			the->sharedMachine = theMachine;
			fxCreateMachinePlatform(the);

		#ifdef mxDebug
			the->name = theName;
		#endif
		#ifdef mxProfile
			the->profileID = theMachine->profileID;
			the->profileBottom = c_malloc(XS_PROFILE_COUNT * sizeof(txProfileRecord));
			if (!the->profileBottom)
				fxJump(the);
			the->profileCurrent = the->profileBottom;
			the->profileTop = the->profileBottom + XS_PROFILE_COUNT;
		#endif

			fxAllocate(the, theCreation);

			the->stackPrototypes = theMachine->stackTop;

            c_memcpy(the->nameTable, theMachine->nameTable, the->nameModulo * sizeof(txSlot *));
			c_memcpy(the->symbolTable, theMachine->symbolTable, the->symbolModulo * sizeof(txSlot *));
			c_memset(the->keyArray, 0, theCreation->keyCount * sizeof(txSlot*));
			the->keyCount = theMachine->keyIndex + (txID)theCreation->keyCount;
			the->keyIndex = theMachine->keyIndex;
			the->keyOffset = the->keyIndex;
			the->keyArrayHost = theMachine->keyArray;
			
			the->aliasCount = theMachine->aliasCount;
			the->aliasArray = (txSlot **)c_malloc_uint32(the->aliasCount * sizeof(txSlot*));
			if (!the->aliasArray)
				fxJump(the);
			c_memset(the->aliasArray, 0, the->aliasCount * sizeof(txSlot*));

			/* mxGlobal */
			aSharedSlot = theMachine->stackTop[-1].value.reference->next;
			anIndex = aSharedSlot->value.table.length;
			fxNewInstance(the);
			aSlot = the->stack->value.reference;
			aSlot->next = fxNewSlot(the);
			aSlot = aSlot->next;
			aSlot->value.table.address = (txSlot**)fxNewChunk(the, anIndex * sizeof(txSlot*));
			aSlot->value.table.length = anIndex;
			aSlot->kind = XS_GLOBAL_KIND;
			aSlot->flag = XS_INTERNAL_FLAG;
			c_memset(aSlot->value.table.address, 0, anIndex * sizeof(txSlot*));
			aSlotAddress = aSlot->value.table.address;
			aSharedSlot = aSharedSlot->next;
			while (aSharedSlot) {
				aSlot->next = aTemporarySlot = fxDuplicateSlot(the, aSharedSlot);
				anID = aTemporarySlot->ID;
				if (anID == mxID(_global))
					aTemporarySlot->value.reference = the->stack->value.reference;
				anID &= 0x7FFF;
				if (anID < anIndex)
					aSlotAddress[anID] = aTemporarySlot;
				aSharedSlot = aSharedSlot->next;
				aSlot = aSlot->next;
			}
			/* mxException */
			mxPushUndefined();
			/* mxHosts */
			mxPushUndefined();
			/* mxClosures */
			mxPushUndefined();
			/* mxModulePaths */
			mxPushUndefined();
			/* mxImportingModules */
			fxNewInstance(the);
			/* mxLoadingModules */
			fxNewInstance(the);
			/* mxLoadedModules */
			fxNewInstance(the);
			/* mxResolvingModules */
			fxNewInstance(the);
			/* mxRunningModules */
			fxNewInstance(the);
			/* mxRequiredModules */
			fxNewInstance(the);
			/* mxModules */
			mxPushUndefined();
			/* mxPendingJobs */
			fxNewInstance(the);
			/* mxRunningJobs */
			fxNewInstance(the);
			/* mxBreakpoints */
			mxPushList();
			/* mxHostInspectors */
			mxPushList();
			/* mxInstanceInspectors */
			mxPushList();
			
			mxPush(mxGlobal);
			fxNewEnvironmentInstance(the, &mxClosures);
			mxPull(mxClosures);

			mxPush(mxSetPrototype);
			fxNewSetInstance(the);
			mxPull(mxModulePaths);
			
			mxPush(mxObjectPrototype);
			fxNewWeakSetInstance(the);
			mxPull(mxModules);
			
			the->sharedModules = theMachine->stackTop[-1 - mxRequiredModulesStackIndex].value.reference->next;
			
			the->collectFlag = XS_COLLECTING_FLAG;

		#ifdef mxDebug
			fxLogin(the);
		#endif

			the->firstJump = C_NULL;

		}
		else {
			fxFree(the);
			c_free(the);
			the = NULL;
		}
	}
	return the;
}

void fxShareMachine(txMachine* the)
{
	if (!(the->shared)) {
	#ifdef mxProfile
		fxStopProfiling(the);
	#endif
	#ifdef mxDebug
		fxLogout(the);
	#endif
		fxShare(the);
		the->shared = 1;
	#ifdef mxProfile
		if (the->profileBottom) {
			c_free(the->profileBottom);
			the->profileBottom = C_NULL;
			the->profileCurrent = C_NULL;
			the->profileTop = C_NULL;
		}
	#endif
	}
}

/* Garbage Collector */

void fxCollectGarbage(txMachine* the)
{
	fxCollect(the, 1);
}

void fxEnableGarbageCollection(txMachine* the, txBoolean enableIt)
{
	if (enableIt)
		the->collectFlag |= XS_COLLECTING_FLAG;
	else
		the->collectFlag &= ~XS_COLLECTING_FLAG;
}

void fxRemember(txMachine* the, txSlot* theSlot)
{
	txSlot* aHeap;
	txSlot* aLimit;
	if ((the->stack <= theSlot) && (theSlot < the->stackTop)) {
		return;
	}
	aHeap = the->firstHeap;
	while (aHeap) {
		aLimit = aHeap->value.reference;
		if ((aHeap < theSlot) && (theSlot < aLimit)) {
			return;
		}
		aHeap = aHeap->next;
	}
	fxForget(the, theSlot);
	theSlot->next = the->cRoot;
	the->cRoot = theSlot;
}

void fxForget(txMachine* the, txSlot* theSlot)
{
	if (!(theSlot->flag & XS_MARK_FLAG)) {
		txSlot* aSlot = the->cRoot;
		txSlot** aSlotAddr = &(the->cRoot);
		while ((aSlot = *aSlotAddr)) {
			if (aSlot == theSlot) {
				*aSlotAddr = aSlot->next;
				return;
			}
			aSlotAddr = &(aSlot->next);
		}
	}
}

void fxAccess(txMachine* the, txSlot* theSlot)
{
	if (theSlot)
		the->scratch = *theSlot;
	else
		the->scratch.kind = XS_UNDEFINED_KIND;
	the->scratch.next = NULL;
	the->scratch.flag = XS_NO_FLAG;
	the->scratch.ID = XS_NO_ID;
}

/* Host */

txMachine* fxBeginHost(txMachine* the)
{
	/* ARGC */
	mxPushInteger(0);
	/* THIS */
	mxPush(mxGlobal);
	/* FUNCTION */
	mxPushUndefined();
	/* TARGET */
	mxPushUndefined();
	/* RESULT */
	mxPushUndefined();
	/* FRAME */
	--the->stack;
	the->stack->next = the->frame;
	the->stack->ID = XS_NO_ID;
	the->stack->flag = XS_C_FLAG;
#ifdef mxDebug
	if (the->breakOnStartFlag) {
		the->breakOnStartFlag = 0;
		the->stack->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
	}
#endif
	the->stack->kind = XS_FRAME_KIND;
	the->stack->value.frame.code = the->code;
	the->stack->value.frame.scope = the->scope;
	the->frame = the->stack;
	/* VARC */
	--the->stack;
	the->stack->next = C_NULL;
	the->stack->ID = XS_NO_ID;
	the->stack->flag = XS_NO_FLAG;
	the->stack->kind = XS_VAR_KIND;
	the->stack->value.environment.variable.count = 0;
	the->stack->value.environment.line = 0;
	the->scope = the->stack;
	the->code = C_NULL;
	return the;
}

void fxEndHost(txMachine* the)
{
	the->stack = the->frame + 6;
	the->scope = the->frame->value.frame.scope;
	the->code = the->frame->value.frame.code;
	the->frame = the->frame->next;
}

void fxCopyObject(txMachine* the)
{
	txSlot* toInstance;
	txSlot* fromInstance;
	txSlot* toProperty;
	txSlot* fromProperty;
	txSlot** firstAddress;
	txSlot** lastAddress;

	fxToInstance(the, the->stack + 1);
	toInstance = fxGetInstance(the, the->stack + 1);
	fxToInstance(the, the->stack);
	fromInstance = fxGetInstance(the, the->stack);
	firstAddress = &toInstance->next;
	while ((toProperty = *firstAddress))
		firstAddress = &toProperty->next;
	lastAddress = firstAddress;
	fromProperty = fromInstance->next;
	while (fromProperty) {
		if (fromProperty->ID != XS_NO_ID) {
			toProperty = toInstance->next;
			while (toProperty != *firstAddress) {
				if (toProperty->ID == fromProperty->ID)
					break;
				toProperty = toProperty->next;
			}
			if (toProperty != *firstAddress) {
				toProperty->kind = fromProperty->kind;
				toProperty->value = fromProperty->value;
			}
			else {
				*lastAddress = toProperty = fxNewSlot(the);
				toProperty->ID = fromProperty->ID;
				toProperty->flag = fromProperty->flag;
				toProperty->kind = fromProperty->kind;
				toProperty->value = fromProperty->value;
				lastAddress = &toProperty->next;
			}
		}
		fromProperty = fromProperty->next;
	}	
	the->stack++;
}

void fxModulePaths(txMachine* the)
{
	mxPush(mxModulePaths);
}

void fxBuildArchiveKeys(txMachine* the)
{
	txPreparation* preparation = the->preparation;
	if (preparation) {
		txU1* p = the->archive;
		if (p) {
			txID c, i;
			p += sizeof(Atom) + sizeof(Atom) + XS_VERSION_SIZE + sizeof(Atom) + XS_DIGEST_SIZE + sizeof(Atom) + XS_DIGEST_SIZE  + sizeof(Atom);
			c = (txID)c_read16be(p);
			p += 2;
			for (i = 0; i < c; i++) {
				fxNewNameX(the, (txString)p);
				p += c_strlen((txString)p) + 1;
			}
		}
	}
}

void* fxGetArchiveCode(txMachine* the, txString path, txSize* size)
{
	txPreparation* preparation = the->preparation;
	if (preparation) {
		txU1* p = the->archive;
		if (p) {
			txU4 atomSize;
			txU1* q;
			p += sizeof(Atom) + sizeof(Atom) + XS_VERSION_SIZE + sizeof(Atom) + XS_DIGEST_SIZE + sizeof(Atom) + XS_DIGEST_SIZE;
			// SYMB
			atomSize = c_read32be(p);
			p += atomSize;
			// MODS
			atomSize = c_read32be(p);
			q = p + atomSize;
			p += sizeof(Atom);
			while (p < q) {
				// PATH
				atomSize = c_read32be(p);
				if (!c_strcmp(path + preparation->baseLength, (txString)(p + sizeof(Atom)))) {
					p += atomSize;
					atomSize = c_read32be(p);
					*size = atomSize - sizeof(Atom);
					return p + sizeof(Atom);
				}
				p += atomSize;
				// CODE
				atomSize = c_read32be(p);
				p += atomSize;
			}
		}
	}
	return C_NULL;
}

void* fxGetArchiveData(txMachine* the, txString path, txSize* size)
{
	txPreparation* preparation = the->preparation;
	if (preparation) {
		txU1* p = the->archive;
		if (p) {
			txU4 atomSize;
			txU1* q;
			p += sizeof(Atom) + sizeof(Atom) + XS_VERSION_SIZE + sizeof(Atom) + XS_DIGEST_SIZE + sizeof(Atom) + XS_DIGEST_SIZE;
			// SYMB
			atomSize = c_read32be(p);
			p += atomSize;
			// MODS
			atomSize = c_read32be(p);
			p += atomSize;
			// RSRC
			atomSize = c_read32be(p);
			q = p + atomSize;
			p += sizeof(Atom);
			while (p < q) {
				// PATH
				atomSize = c_read32be(p);
				if (!c_strcmp(path, (txString)(p + sizeof(Atom)))) {
					p += atomSize;
					atomSize = c_read32be(p);
					*size = atomSize - sizeof(Atom);
					return p + sizeof(Atom);
				}
				p += atomSize;
				// DATA
				atomSize = c_read32be(p);
				p += atomSize;
			}
		}
	}
	return C_NULL;
}

#ifdef mxFrequency

typedef struct {
	txNumber exit;
	txNumber frequency;
	txU1 code;
} txFrequency;

static int fxCompareExit(const void* a, const void* b)
{
	return ((txFrequency*)b)->exit - ((txFrequency*)a)->exit;
}

static int fxCompareFrequency(const void* a, const void* b)
{
	return ((txFrequency*)b)->frequency - ((txFrequency*)a)->frequency;
}

void fxReportFrequency(txMachine* the)
{
	txFrequency frequencies[XS_CODE_COUNT];
	txU1 code;
	txNumber exitSum = 0;
	txNumber frequencySum = 0;
	for (code = 0; code < XS_CODE_COUNT; code++) {
		frequencies[code].exit = the->exits[code];
		frequencies[code].frequency = the->frequencies[code];
		frequencies[code].code = code;
		exitSum += the->exits[code];
		frequencySum += the->frequencies[code];
	}
	c_qsort(frequencies, XS_CODE_COUNT, sizeof(txFrequency), fxCompareFrequency);
	fprintf(stderr, "Frequencies %10.0lf\n", frequencySum);
	for (code = 0; code < XS_CODE_COUNT; code++) {
		if (!frequencies[code].frequency)
			break;
		fprintf(stderr, "%24s %10.3lf%%\n", 
			gxCodeNames[frequencies[code].code], 
			((double)(frequencies[code].frequency) * 100.0) / frequencySum);
	}
	c_qsort(frequencies, XS_CODE_COUNT, sizeof(txFrequency), fxCompareExit);
	fprintf(stderr, "Exits %10.0lf\n", exitSum);
	for (code = 0; code < XS_CODE_COUNT; code++) {
		if (!frequencies[code].exit)
			break;
		fprintf(stderr, "%24s%10.3lf%%\n", 
			gxCodeNames[frequencies[code].code], 
			((double)(frequencies[code].exit) * 100.0) / exitSum);
	}
}
#endif


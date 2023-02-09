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
	if (theSlot->kind == XS_BIGINT_X_KIND)
		return XS_BIGINT_KIND;
#ifdef mxHostFunctionPrimitive
	if (theSlot->kind == XS_HOST_FUNCTION_KIND)
		return XS_REFERENCE_KIND;
#endif
	return theSlot->kind;
}

/* Primitives */

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
	case XS_BIGINT_KIND:
	case XS_BIGINT_X_KIND:
		if ((theSlot->value.bigint.size == 1) && (theSlot->value.bigint.data[0] == 0))
			theSlot->value.boolean = 0;
		else
			theSlot->value.boolean = 1;
		theSlot->kind = XS_BOOLEAN_KIND;
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
			#define MODULO 4294967296.0
			txNumber aNumber = c_fmod(c_trunc(theSlot->value.number), MODULO);
			if (aNumber >= MODULO / 2)
				aNumber -= MODULO;
			else if (aNumber < -MODULO / 2)
				aNumber += MODULO;
			theSlot->value.integer = (txInteger)aNumber;
			} break;
		}
		mxFloatingPointOp("number to integer");
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = fxStringToNumber(the->dtoa, theSlot->value.string, 1);
		mxMeterOne();
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
		mxFloatingPointOp("integer to number");
		break;
	case XS_NUMBER_KIND:
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = fxStringToNumber(the->dtoa, theSlot->value.string, 1);
		mxMeterOne();
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
#ifdef mxSnapshot
	fxCopyStringC(the, theSlot, theValue);
#else
	theSlot->value.string = theValue;
	theSlot->kind = XS_STRING_X_KIND;
#endif
}

void fxStringBuffer(txMachine* the, txSlot* theSlot, txString theValue, txSize theSize)
{
	theSlot->value.string = (txString)fxNewChunk(the, fxAddChunkSizes(the, theSize, 1));
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
		*theSlot = mxUndefinedString;
		break;
	case XS_NULL_KIND:
		fxStringX(the, theSlot, "null");
		break;
	case XS_BOOLEAN_KIND:
		if (theSlot->value.boolean == 0)
			fxStringX(the, theSlot, "false");
		else
			fxStringX(the, theSlot, "true");
		break;
	case XS_INTEGER_KIND:
		fxCopyStringC(the, theSlot, fxIntegerToString(the->dtoa, theSlot->value.integer, aBuffer, sizeof(aBuffer)));
		mxMeterOne();
		break;
	case XS_NUMBER_KIND:
		fxCopyStringC(the, theSlot, fxNumberToString(the->dtoa, theSlot->value.number, aBuffer, sizeof(aBuffer), 0, 0));
		mxMeterOne();
		break;
	case XS_SYMBOL_KIND:
		mxTypeError("Cannot coerce symbol to string");
		break;
	case XS_BIGINT_KIND:
	case XS_BIGINT_X_KIND:
		gxTypeBigInt.toString(the, theSlot, 0);
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
	aSize = mxStringLength(aString) + 1;
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
		mxFloatingPointOp("number to unsigned");
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = fxStringToNumber(the->dtoa, theSlot->value.string, 1);
		mxMeterOne();
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
	if (theReference) {
		theSlot->value.reference = theReference;
		theSlot->kind = XS_REFERENCE_KIND;
	}
	else
		theSlot->kind = XS_NULL_KIND;
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
	fxSetIndexSize(the, instance->next, size, XS_CHUNK);
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
			theInstance = fxGetPrototype(the, theInstance);
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
		array->value.array.address = (txSlot*)fxNewChunk(the, fxMultiplyChunkSizes(the, length, sizeof(txSlot)));
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
			the->stack->value.hostFunction.profileID = the->profileID;
			the->profileID++;
		#else
			fxNewHostFunction(the, builder->callback, builder->length, builder->id, XS_NO_ID);
		#endif
		}
		else
			fxNewHostObject(the, (txDestructor)builder->callback);
		fxArrayCacheItem(the, the->stack + 1, the->stack);
		mxPop();
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
	instance = fxNewHostFunction(the, theCallback, theLength, name, XS_NO_ID);
	instance->flag |= XS_CAN_CONSTRUCT_FLAG;
	property = fxLastProperty(the, instance);
	fxNextSlotProperty(the, property, aStack, mxID(_prototype), XS_GET_ONLY);
	property = mxBehaviorSetProperty(the, fxGetInstance(the, aStack), mxID(_constructor), 0, XS_OWN);
	property->flag = XS_DONT_ENUM_FLAG;
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	*aStack = *the->stack;
	mxPop();
	return instance;
}

txSlot* fxNewHostFunction(txMachine* the, txCallback theCallback, txInteger theLength, txInteger name, txInteger profileID)
{
	txSlot* instance;
	txSlot* property;

	mxPushUndefined();
	instance = fxNewSlot(the);
	instance->flag |= XS_CAN_CALL_FLAG;
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = mxFunctionPrototype.value.reference;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;

	/* CALLBACK */
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_CALLBACK_KIND;
	property->value.callback.address = theCallback;
	property->value.callback.closures = C_NULL;

	/* HOME */
	property = property->next = fxNewSlot(the);
	if (profileID != XS_NO_ID)
		property->ID = profileID;
	else
		property->ID = fxGenerateProfileID(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_HOME_KIND;
	property->value.home.object = C_NULL;
	if (the->frame && (mxFunction->kind == XS_REFERENCE_KIND) && (mxIsFunction(mxFunction->value.reference))) {
		txSlot* slot = mxFunctionInstanceHome(mxFunction->value.reference);
		property->value.home.module = slot->value.home.module;
	}
	else
		property->value.home.module = C_NULL;

	/* LENGTH */
	if (gxDefaults.newFunctionLength)
		gxDefaults.newFunctionLength(the, instance, theLength);

	/* NAME */
	fxRenameFunction(the, instance, name, 0, XS_NO_ID, C_NULL);

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
		if (prototypeHost && (prototypeHost->kind == XS_HOST_KIND) && (prototypeHost->value.host.variant.destructor != fxReleaseSharedChunk)) {
			txSlot* instanceHost = instance->next = fxNewSlot(the);
			instanceHost->flag = XS_INTERNAL_FLAG;
			instanceHost->kind = XS_HOST_KIND;
			instanceHost->value.host.data = C_NULL;
			if (prototypeHost->flag & XS_HOST_HOOKS_FLAG) {
				instanceHost->flag |= XS_HOST_HOOKS_FLAG;
				instanceHost->value.host.variant.hooks = prototypeHost->value.host.variant.hooks;
			}
			else {
				instanceHost->value.host.variant.destructor = prototypeHost->value.host.variant.destructor;
			}
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
	aProperty->flag = XS_INTERNAL_FLAG;
	aProperty->kind = XS_HOST_KIND;
	aProperty->value.host.data = C_NULL;
	aProperty->value.host.variant.destructor = theDestructor;
	
	return anInstance;
}

txInteger fxGetHostBufferLength(txMachine* the, txSlot* slot)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		txSlot* bufferInfo = host->next;
		if (host->flag & XS_HOST_CHUNK_FLAG)
			mxSyntaxError("C: xsGetHostBufferLength: no host data");
		if (!bufferInfo || (bufferInfo->kind != XS_BUFFER_INFO_KIND))
			mxSyntaxError("C: xsGetHostBufferLength: no host buffer");
		return bufferInfo->value.bufferInfo.length;
	}
	mxSyntaxError("C: xsGetHostData: no host object");
	return 0;
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

void* fxGetHostChunkValidate(txMachine* the, txSlot* slot, void* validator)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		if (host->flag & XS_HOST_CHUNK_FLAG) {
			if (validator == host->value.host.variant.destructor)
				return host->value.host.data;
			mxSyntaxError("C: xsGetHostChunk: invalid");
		}
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

void* fxGetHostDataValidate(txMachine* the, txSlot* slot, void* validator)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		if (!(host->flag & XS_HOST_CHUNK_FLAG)) {
			if (validator == host->value.host.variant.destructor)
				return host->value.host.data;
			mxSyntaxError("C: xsGetHostData: invalid");
		}
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

void fxPetrifyHostBuffer(txMachine* the, txSlot* slot)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (!host)
		mxSyntaxError("C: xsPetrifyHostBuffer: no host object");
	if (host->flag & XS_HOST_CHUNK_FLAG)
		mxSyntaxError("C: xsPetrifyHostBuffer: no host data");
	host->flag |= XS_DONT_SET_FLAG;
}

void fxSetHostBuffer(txMachine* the, txSlot* slot, void* theData, txSize theSize)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		txSlot* bufferInfo = host->next;
		if (!bufferInfo || (bufferInfo->kind != XS_BUFFER_INFO_KIND)) {
			bufferInfo = fxNewSlot(the);
			bufferInfo->next = host->next;
			bufferInfo->flag = XS_INTERNAL_FLAG;
			bufferInfo->kind = XS_BUFFER_INFO_KIND;
			bufferInfo->value.bufferInfo.length = 0;
			bufferInfo->value.bufferInfo.maxLength = -1;
			host->next = bufferInfo;
		}
		host->flag &= ~XS_HOST_CHUNK_FLAG;
		host->value.host.data = theData;
		bufferInfo->value.bufferInfo.length = theSize;
	}
	else
		mxSyntaxError("C: xsSetHostData: no host object");
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

void fxSetHostHooks(txMachine* the, txSlot* slot, const txHostHooks* theHooks)
{
	txSlot* host = fxCheckHostObject(the, slot);
	if (host) {
		host->flag |= XS_HOST_HOOKS_FLAG;
		host->value.host.variant.hooks = (txHostHooks *) theHooks;
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
	mxPush(mxEnumeratorFunction);
	mxCall();
	mxRunCount(0);
}

void fxGetAll(txMachine* the, txID id, txIndex index)
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
			txSlot* slot;
			mxOverflow(-5);
            the->stack -= 5;
			slot = the->stack;
			mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
			mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
			mxInitSlotKind(slot++, XS_UNDEFINED_KIND);
			mxInitSlotKind(slot++, XS_UNDEFINED_KIND);
			slot->value.reference = function;
			mxInitSlotKind(slot++, XS_REFERENCE_KIND);
			if (!flag) {
				txSlot* primitive = instance->next;
				slot->value = primitive->value;
				mxInitSlotKind(slot, primitive->kind);
			}
			mxRunCount(0);
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
	mxPop();
	mxGetAll(at->value.at.id, at->value.at.index);
}

void fxGetID(txMachine* the, txID id)
{
	mxGetAll(id, 0);
}

void fxGetIndex(txMachine* the, txIndex index)
{
	mxGetAll(XS_NO_ID, index);
}

txBoolean fxHasAll(txMachine* the, txID id, txIndex index)
{
	txSlot* instance = fxToInstance(the, the->stack);
	txBoolean result = mxBehaviorHasProperty(the, instance, id, index);
	mxPop();
	return result;
}

txBoolean fxHasAt(txMachine* the)
{
	txSlot* at = fxAt(the, the->stack);
	mxPop();
	return mxHasAll(at->value.at.id, at->value.at.index);
}

txBoolean fxHasID(txMachine* the, txID id)
{
	return mxHasAll(id, 0);
}

txBoolean fxHasIndex(txMachine* the, txIndex index)
{
	return mxHasAll(XS_NO_ID, index);
}

void fxSetAll(txMachine* the, txID id, txIndex index)
{
	txSlot* value = the->stack + 1;
	txSlot* instance = fxToInstance(the, the->stack);
	txSlot* property = mxBehaviorSetProperty(the, instance, (txID)id, index, XS_ANY);
	if (!property)
		mxDebugID(XS_TYPE_ERROR, "C: xsSet %s: not extensible", id);
	if (property->kind == XS_ACCESSOR_KIND) {
		txSlot* slot;
		txSlot* function = property->value.accessor.setter;
		if (!mxIsFunction(function))
			mxDebugID(XS_TYPE_ERROR, "C: xsSet %s: no setter", id);
		mxOverflow(-5);
		the->stack -= 5;
		slot = the->stack;
		slot->value = value->value;
		mxInitSlotKind(slot++, value->kind);
		mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
		mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
		slot->value = value->value;
		mxInitSlotKind(slot++, value->kind);
		mxInitSlotKind(slot++, XS_UNDEFINED_KIND);
		slot->value.reference = function;
		mxInitSlotKind(slot++, XS_REFERENCE_KIND);
		slot->value.reference = instance;
		mxInitSlotKind(slot, XS_REFERENCE_KIND);
		mxRunCount(1);
	}
	else {
		if (property->flag & XS_DONT_SET_FLAG)
			mxDebugID(XS_TYPE_ERROR, "C: xsSet %s: not writable", id);
		property->kind = value->kind;
		property->value = value->value;
		mxPop();
	}
}

void fxSetAt(txMachine* the)
{
	txSlot* at = fxAt(the, the->stack);
	mxPop();
	mxSetAll(at->value.at.id, at->value.at.index);
}

void fxSetID(txMachine* the, txID id)
{
	mxSetAll(id, 0);
}

void fxSetIndex(txMachine* the, txIndex index)
{
	mxSetAll(XS_NO_ID, index);
}

void fxDeleteAll(txMachine* the, txID id, txIndex index)
{
	txSlot* instance = fxToInstance(the, the->stack);
	if (!mxBehaviorDeleteProperty(the, instance, (txID)id, index))
		mxDebugID(XS_TYPE_ERROR, "delete %s: not configurable", id);
}

void fxDeleteAt(txMachine* the)
{
	txSlot* at = fxAt(the, the->stack);
	txSlot* instance = fxToInstance(the, the->stack + 1);
	mxPop();
	if (!mxBehaviorDeleteProperty(the, instance, at->value.at.id, at->value.at.index))
		mxDebugID(XS_TYPE_ERROR, "delete %s: not configurable", at->value.at.id);
}

void fxDeleteID(txMachine* the, txID id)
{
	txSlot* instance = fxToInstance(the, the->stack);
	if (!mxBehaviorDeleteProperty(the, instance, (txID)id, 0))
		mxDebugID(XS_TYPE_ERROR, "delete %s: not configurable", id);
}

void fxDeleteIndex(txMachine* the, txIndex index)
{
	txSlot* instance = fxToInstance(the, the->stack);
	if (!mxBehaviorDeleteProperty(the, instance, XS_NO_ID, index))
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
	mxPop();
}

void fxDefineAt(txMachine* the, txFlag flag, txFlag mask)
{
	txSlot* at = fxAt(the, the->stack++);
	mxDefineAll(at->value.at.id, at->value.at.index, flag, mask);
}

void fxDefineID(txMachine* the, txID id, txFlag flag, txFlag mask)
{
	mxDefineAll(id, 0, flag, mask);
}

void fxDefineIndex(txMachine* the, txIndex index, txFlag flag, txFlag mask)
{
	mxDefineAll(XS_NO_ID, index, flag, mask);
}

void fxCall(txMachine* the)
{
	// TARGET
	(--the->stack)->next = C_NULL;
	mxInitSlotKind(the->stack, XS_UNDEFINED_KIND);
	// RESULT
	(--the->stack)->next = C_NULL;
	mxInitSlotKind(the->stack, XS_UNDEFINED_KIND);
	// FRAME
	(--the->stack)->next = C_NULL;
	mxInitSlotKind(the->stack, XS_UNINITIALIZED_KIND);
	// COUNT
	(--the->stack)->next = C_NULL;
	mxInitSlotKind(the->stack, XS_UNINITIALIZED_KIND);
}

void fxCallID(txMachine* the, txID theID)
{
	mxDub();
	mxGetID(theID);
	fxCall(the);
}

void fxCallFrame(txMachine* the)
{
	mxOverflow(-4);
	fxCall(the);
}

void fxNew(txMachine* the)
{
	txSlot* constructor = the->stack;
	txSlot* slot = constructor - 5;
	the->stack = slot;
	mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
	mxInitSlotKind(slot++, XS_UNINITIALIZED_KIND);
	mxInitSlotKind(slot++, XS_UNDEFINED_KIND);
	slot->value = constructor->value;
	mxInitSlotKind(slot++, constructor->kind);
	slot->value = constructor->value;
	mxInitSlotKind(slot++, constructor->kind);
	mxInitSlotKind(slot, XS_UNINITIALIZED_KIND);
}

void fxNewID(txMachine* the, txID theID)
{
	mxGetID(theID);
	fxNew(the);
}

void fxNewFrame(txMachine* the)
{
	mxOverflow(-5);
	fxNew(the);
}

void fxRunCount(txMachine* the, txInteger count)
{
	fxRunID(the, C_NULL, count);
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
	mxPop();
	return result;
}

/* Arguments and Variables */

void fxVars(txMachine* the, txInteger theCount)
{
	if (the->scope != the->stack)
		mxSyntaxError("C: xsVars: too late");
	mxOverflow(theCount);
	the->scope->value.environment.variable.count = theCount;
	while (theCount) {
		mxPushUndefined();
		theCount--;
	}
}

txInteger fxCheckArg(txMachine* the, txInteger theIndex)
{
	if ((theIndex < 0) || (mxArgc <= theIndex))
		mxSyntaxError("C: xsArg(%ld): invalid index", theIndex);
	return theIndex;
}

txInteger fxCheckVar(txMachine* the, txInteger theIndex)
{
	if ((theIndex < 0) || (mxVarc <= theIndex))
		mxSyntaxError("C: xsVar(%ld): invalid index", theIndex);
	return theIndex;
}

void fxOverflow(txMachine* the, txInteger theCount, txString thePath, txInteger theLine)
{
	txSlot* aStack = the->stack + theCount;
	if (theCount < 0) {
		if (aStack < the->stackBottom) {
			fxReport(the, "stack overflow (%ld)!\n", (the->stack - the->stackBottom) + theCount);
			fxAbort(the, XS_STACK_OVERFLOW_EXIT);
		}
	}
	else if (theCount > 0) {
		if (aStack > the->stackTop) {
			fxReport(the, "stack overflow (%ld)!\n", theCount - (the->stackTop - the->stack));
			fxAbort(the, XS_STACK_OVERFLOW_EXIT);
		}
	}
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
	txSize length = 0;
    va_list arguments;
    txSlot* slot;
#ifdef mxDebug
	fxBufferFrameName(the, message, sizeof(message), the->frame, ": ");
	length = mxStringLength(message);
#endif
    va_start(arguments, format);
    c_vsnprintf(message + length, sizeof(message) - length, format, arguments);
    va_end(arguments);

	length = c_strlen(message) - 1;
	while (length && (0x80 & message[length]))
		message[length--] = 0;

	if ((error <= XS_NO_ERROR) || (XS_ERROR_COUNT <= error))
		error = XS_UNKNOWN_ERROR;

    slot = fxNewSlot(the);
    slot->kind = XS_INSTANCE_KIND;
    slot->value.instance.garbage = C_NULL;
    slot->value.instance.prototype = mxErrorPrototypes(error).value.reference;
	mxException.kind = XS_REFERENCE_KIND;
	mxException.value.reference = slot;
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG;
	slot->kind = XS_ERROR_KIND;
	slot->value.error.info = C_NULL;
	slot->value.error.which = error;
	if (gxDefaults.captureErrorStack)
		gxDefaults.captureErrorStack(the, slot, the->frame);
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

txMachine* fxCreateMachine(txCreation* theCreation, txString theName, void* theContext, txID profileID)
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
			txInteger id;
			txSlot* slot;

			if (gxDefaults.initializeSharedCluster)
				gxDefaults.initializeSharedCluster();
				
			the->dtoa = fxNew_dtoa(the);
			the->context = theContext;
			fxCreateMachinePlatform(the);

		#ifdef mxDebug
			the->name = theName;
		#endif
			the->profileID = (profileID != XS_NO_ID) ? profileID : mxBaseProfileID;
			fxAllocate(the, theCreation);

            c_memset(the->nameTable, 0, the->nameModulo * sizeof(txSlot *));
			c_memset(the->symbolTable, 0, the->symbolModulo * sizeof(txSlot *));

			/* mxGlobal */
			mxPushUndefined();
			/* mxException */
			mxPushUndefined();
			/* mxProgram */
			mxPushNull();
			fxNewProgramInstance(the);
			/* mxHosts */
			mxPushUndefined();
			/* mxModuleQueue */
			fxNewInstance(the);
			/* mxUnhandledPromises */
			fxNewInstance(the);
			/* mxDuringJobs */
			fxNewInstance(the);
			/* mxFinalizationRegistries */
			fxNewInstance(the);
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

			for (id = mxInstanceInspectorsStackIndex + 1; id < mxEmptyCodeStackIndex; id++)
				mxPushUndefined();

			/* mxEmptyCode */
			mxPushUndefined();
			the->stack->value.code.address = (txByte*)gxNoCode;
			the->stack->value.code.closures = C_NULL;
			the->stack->kind = XS_CODE_X_KIND;	
			/* mxEmptyString */
			mxPushStringX("");
			/* mxEmptyRegExp */
			mxPushStringX("(?:)");
			/* mxBigIntString */
			mxPushStringX("bigint");
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
			fxBuildArguments(the);
			fxBuildArray(the);
			fxBuildString(the);
			fxBuildBoolean(the);
			fxBuildNumber(the);
			fxBuildBigInt(the);
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
			
			mxPushUndefined();
			mxPush(mxObjectPrototype);
	#ifdef mxLink
			slot = fxLastProperty(the, fxNewObjectInstance(the));
	#else
			slot = fxLastProperty(the, fxNewGlobalInstance(the));
	#endif
			for (id = XS_SYMBOL_ID_COUNT; id < _Infinity; id++)
				slot = fxNextSlotProperty(the, slot, &the->stackPrototypes[-1 - id], mxID(id), XS_DONT_ENUM_FLAG);
			for (; id < _Compartment; id++)
				slot = fxNextSlotProperty(the, slot, &the->stackPrototypes[-1 - id], mxID(id), XS_GET_ONLY);
			for (; id < XS_INTRINSICS_COUNT; id++)
				slot = fxNextSlotProperty(the, slot, &the->stackPrototypes[-1 - id], mxID(id), XS_DONT_ENUM_FLAG);
			slot = fxNextSlotProperty(the, slot, the->stack, mxID(_global), XS_DONT_ENUM_FLAG);
			slot = fxNextSlotProperty(the, slot, the->stack, mxID(_globalThis), XS_DONT_ENUM_FLAG);
			mxGlobal.value = the->stack->value;
			mxGlobal.kind = the->stack->kind;
			fxNewInstance(the);
			fxNewInstance(the);
			mxPushUndefined();
			fxNewEnvironmentInstance(the, C_NULL);
			mxPushUndefined();
			mxPushUndefined();
			mxPushUndefined();
			mxPushUndefined();
			mxPushUndefined();
			mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm = fxNewRealmInstance(the);
			mxPop();

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
			
			if (gxDefaults.terminateSharedCluster)
				gxDefaults.terminateSharedCluster();
		}
	}
	return the;
}

void fxDeleteMachine(txMachine* the)
{
	txSlot* aSlot;
#ifndef mxLink
	txSlot* bSlot;
	txSlot* cSlot;
#endif

	if (!(the->shared)) {
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
#ifndef mxLink
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
#endif
	fxDelete_dtoa(the->dtoa);
	fxDeleteMachinePlatform(the);
	fxFree(the);
	c_free(the);

	if (gxDefaults.terminateSharedCluster)
		gxDefaults.terminateSharedCluster();
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
			txSlot* sharedSlot;
			txSlot* slot;

			if (gxDefaults.initializeSharedCluster)
				gxDefaults.initializeSharedCluster();

			the->dtoa = fxNew_dtoa(the);
			the->preparation = theMachine->preparation;
			the->context = theContext;
			the->archive = theMachine->archive;
			the->sharedMachine = theMachine;
			fxCreateMachinePlatform(the);

		#ifdef mxDebug
			the->name = theName;
		#endif
			the->profileID = theMachine->profileID;
			fxAllocate(the, theCreation);

            c_memcpy(the->nameTable, theMachine->nameTable, the->nameModulo * sizeof(txSlot *));
			c_memcpy(the->symbolTable, theMachine->symbolTable, the->symbolModulo * sizeof(txSlot *));
			the->colors = theMachine->colors;
			the->keyCount = theMachine->keyIndex + (txID)theCreation->initialKeyCount;
			the->keyIndex = theMachine->keyIndex;
			the->keyOffset = the->keyIndex;
			the->keyArrayHost = theMachine->keyArray;
			fxBuildArchiveKeys(the);
			
			the->aliasCount = theMachine->aliasCount;
			if (the->aliasCount) {
				the->aliasArray = (txSlot **)c_malloc_uint32(the->aliasCount * sizeof(txSlot*));
				if (!the->aliasArray)
					fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
				c_memset(the->aliasArray, 0, the->aliasCount * sizeof(txSlot*));
			}

			/* mxGlobal */
			mxPushUndefined();
			/* mxException */
			mxPushUndefined();
			/* mxProgram */
			mxPushNull();
			fxNewProgramInstance(the);
			/* mxHosts */
			mxPushUndefined();
			/* mxModuleQueue */
			fxNewInstance(the);
			/* mxUnhandledPromises */
			fxNewInstance(the);
			/* mxDuringJobs */
			fxNewInstance(the);
			/* mxFinalizationRegistries */
			fxNewInstance(the);
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

			the->stackPrototypes = theMachine->stackTop;

			mxPushUndefined();
			mxPush(theMachine->stackTop[-1 - mxGlobalStackIndex]);
			slot = fxLastProperty(the, fxNewObjectInstance(the));
			slot = fxNextSlotProperty(the, slot, the->stack, mxID(_global), XS_DONT_ENUM_FLAG);
			slot = fxNextSlotProperty(the, slot, the->stack, mxID(_globalThis), XS_DONT_ENUM_FLAG);
			mxGlobal.value = the->stack->value;
			mxGlobal.kind = the->stack->kind;
			if (the->archive) {
				fxNewHostObject(the, C_NULL);
				the->stack->value.reference->next->value.host.data = the->archive;
				slot = fxNextSlotProperty(the, slot, the->stack, fxID(the, "archive"), XS_DONT_ENUM_FLAG);
				mxPop();
			}
			
			fxNewInstance(the);
			mxPush(theMachine->stackTop[-1 - mxProgramStackIndex]); //@@
			fxNewHostInstance(the);
			
			mxPushUndefined();
			slot = fxLastProperty(the, fxNewEnvironmentInstance(the, C_NULL));
			sharedSlot = theMachine->stackTop[-1 - mxExceptionStackIndex].value.reference->next->next; //@@
			while (sharedSlot) {
				slot = slot->next = fxDuplicateSlot(the, sharedSlot);
				sharedSlot = sharedSlot->next;
			}
			
			
			mxPushUndefined();
			mxPushUndefined();
			mxPushUndefined();
			mxPushUndefined();
			mxPushUndefined();
			mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm = fxNewRealmInstance(the);
			mxPop();
			
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

txMachine* fxPrepareMachine(txCreation* creation, txPreparation* preparation, txString name, void* context, void* archive)
{
	txMachine _root;
	txMachine* root = &_root;
	if ((preparation->version[0] != XS_MAJOR_VERSION) || (preparation->version[1] != XS_MINOR_VERSION))
		return C_NULL;
	c_memset(root, 0, sizeof(txMachine));
	root->preparation = preparation;
	root->archive = archive;
	root->keyArray = preparation->keys;
	root->colors = preparation->colors;
	root->keyCount = (txID)preparation->keyCount + (txID)preparation->creation.initialKeyCount;
	root->keyIndex = (txID)preparation->keyCount;
	root->nameModulo = preparation->nameModulo;
	root->nameTable = preparation->names;
	root->symbolModulo = preparation->symbolModulo;
	root->symbolTable = preparation->symbols;

	root->stack = &preparation->stack[0];
	root->stackBottom = &preparation->stack[0];
	root->stackTop = &preparation->stack[preparation->stackCount];

	root->firstHeap = &preparation->heap[0];
	root->freeHeap = &preparation->heap[preparation->heapCount - 1];
	root->aliasCount = (txID)preparation->aliasCount;
	
	root->profileID = (txID)preparation->profileID;
	
	if (!creation)
		creation = &preparation->creation;
	return fxCloneMachine(creation, root, name, context);
}

void fxShareMachine(txMachine* the)
{
	if (!(the->shared)) {
	#ifdef mxDebug
		fxLogout(the);
	#endif
		{
			txSlot* realm = mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm;
			txSlot* modules = mxOwnModules(realm)->value.reference;
			txSlot* module = modules->next;
			while (module) {
				mxModuleInstanceInternal(module->value.reference)->value.module.realm = NULL;
				module = module->next;
			}
			mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm = NULL;
			mxException.kind = XS_REFERENCE_KIND;
			mxException.value.reference = mxRealmClosures(realm)->value.reference;
			mxProgram.value.reference = modules; //@@
			
			{
				txSlot* target = fxNewInstance(the);
				txSlot* modules = mxOwnModules(realm)->value.reference;
				txSlot* module = modules->next;
				while (module) {
					target = target->next = fxNewSlot(the);
					target->value.symbol = mxModuleInstanceInternal(module->value.reference)->value.module.id;
					target->kind = XS_SYMBOL_KIND;
					target->ID = mxModuleInstanceInternal(module->value.reference)->value.module.id;
					module = module->next;
				}
				mxPull(mxHosts); //@@
			}
			mxModuleQueue = mxUndefined;
			mxDuringJobs = mxUndefined;
			mxFinalizationRegistries = mxUndefined;
			mxPendingJobs = mxUndefined;
			mxRunningJobs = mxUndefined;
			mxBreakpoints = mxUndefined;
			mxHostInspectors = mxUndefined;
			mxInstanceInspectors = mxUndefined;
		}
		fxCollectGarbage(the);
		fxShare(the);
		the->shared = 1;
	}
}

/* Garbage Collector */

void fxCollectGarbage(txMachine* the)
{
	fxCollect(the, XS_COMPACT_FLAG);
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
#if defined(mxInstrument) || defined(mxProfile)
	if (the->frame == C_NULL)
		fxCheckProfiler(the, C_NULL);
#endif
	mxOverflow(-7);
	/* THIS */
	(--the->stack)->next = C_NULL;
	mxInitSlotKind(the->stack, XS_UNDEFINED_KIND);
	/* FUNCTION */
	(--the->stack)->next = C_NULL;
	mxInitSlotKind(the->stack, XS_UNDEFINED_KIND);
	/* TARGET */
	(--the->stack)->next = C_NULL;
	mxInitSlotKind(the->stack, XS_UNDEFINED_KIND);
	/* RESULT */
	(--the->stack)->next = C_NULL;
	mxInitSlotKind(the->stack, XS_UNDEFINED_KIND);
	/* FRAME */
	(--the->stack)->next = the->frame;
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
	/* COUNT */
	(--the->stack)->next = C_NULL;
	mxInitSlotKind(the->stack, XS_INTEGER_KIND);
	the->stack->value.integer = 0;
	/* VARC */
	(--the->stack)->next = C_NULL;
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
    if (the->frame->next == C_NULL) {
        fxEndJob(the);
    }
	the->stack = the->frame + 5;
	the->scope = the->frame->value.frame.scope;
	the->code = the->frame->value.frame.code;
	the->frame = the->frame->next;
}

void fxEndJob(txMachine* the)
{
	if (gxDefaults.cleanupFinalizationRegistries)
		gxDefaults.cleanupFinalizationRegistries(the);
	if (mxDuringJobs.kind == XS_REFERENCE_KIND)
		mxDuringJobs.value.reference->next = C_NULL;
	fxCheckUnhandledRejections(the, 0);
}

void fxExitToHost(txMachine* the)
{
	txJump* jump = the->firstJump;
	while (jump->nextJump) {
		txJump* nextJump = jump->nextJump;
		if (jump->flag)
			c_free(jump);
		jump = nextJump;
	}
	c_longjmp(jump->buffer, 1);
}

typedef struct {
	txMachine* machine;
	txU1* archive;
	txArchiveRead read;
	txArchiveWrite write;
	txU1* buffer;
	txU1* scratch;
	size_t offset;
	size_t size;
	size_t bufferCode;
	size_t bufferLoop;
	size_t bufferOffset;
	size_t bufferSize;
	size_t scratchSize;
	txID* ids;
	txID* map;
	txID* maps;
	c_jmp_buf jmp_buf;
	txBoolean dirty;
} txMapper;

static void fxMapperMapID(txMapper* self);
static void fxMapperMapIDs(txMapper* self);
static txU1 fxMapperRead1(txMapper* self);
static txU2 fxMapperRead2(txMapper* self);
static txU4 fxMapperRead4(txMapper* self);
static void fxMapperReadAtom(txMapper* self, Atom* atom);
static void fxMapperSkip(txMapper* self, size_t size);
static void fxMapperStep(txMapper* self);

#define mxMapAtom(POINTER) \
	atom.atomSize = c_read32be(POINTER); \
	POINTER += 4; \
	atom.atomType = c_read32be(POINTER); \
	POINTER += 4
	

#define mxElseStatus(_ASSERTION,_STATUS) \
	((void)((_ASSERTION) || ((self->buffer[8] = (_STATUS)), c_longjmp(self->jmp_buf, 1), 0)))
#define mxElseFatalCheck(_ASSERTION) mxElseStatus(_ASSERTION, XS_FATAL_CHECK_EXIT)
#define mxElseNoMoreKeys(_ASSERTION) mxElseStatus(_ASSERTION, XS_NO_MORE_KEYS_EXIT)
#define mxElseNotEnoughMemory(_ASSERTION) mxElseStatus(_ASSERTION, XS_NOT_ENOUGH_MEMORY_EXIT)
#define mxElseInstall(_ASSERTION) if (!(_ASSERTION)) goto install

#define mxArchiveHeaderSize (sizeof(Atom) + sizeof(Atom) + XS_VERSION_SIZE + sizeof(Atom) + XS_DIGEST_SIZE)

void fxBuildArchiveKeys(txMachine* the)
{
	txPreparation* preparation = the->preparation;
	if (preparation) {
		txU1* p = the->archive;
		if (p) {
			txU4 atomSize;
			txID c, i;
			p += mxArchiveHeaderSize;
			// NAME
			atomSize = c_read32be(p);
			p += atomSize;
			// SYMB
			p += sizeof(Atom);
			c = (txID)c_read16(p);
			p += 2;
			p += mxStringLength((txString)p) + 1;
			for (i = 1; i < c; i++) {
				fxNewNameX(the, (txString)p);
				p += mxStringLength((txString)p) + 1;
			}
		}
	}
}

static txU1 *fxGetArchiveModules(txMachine *the, void* archive, txU4 *size)
{
	txPreparation* preparation = the->preparation;
	txU1* p = archive;
	if (!preparation || !p) {
		*size = 0;
		return NULL;
	}
	p += mxArchiveHeaderSize;
	// NAME
	p += c_read32be(p);
	// SYMB
	p += c_read32be(p);
	// IDEN
	p += c_read32be(p);
	// MAPS
	p += c_read32be(p);
	// MODS
	*size = c_read32be(p) - sizeof(Atom);
	return p + sizeof(Atom);
}

void* fxGetArchiveCode(txMachine* the, void* archive, txString path, size_t* size)
{
	txU4 atomSize;
	txU1 *p = fxGetArchiveModules(the, archive, &atomSize), *q;
	if (!p)
		return NULL; 
	q = p + atomSize;
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
		// CODE
		atomSize = c_read32be(p);
		p += atomSize;
	}
	return C_NULL;
}

txInteger fxGetArchiveCodeCount(txMachine* the, void* archive)
{
	txInteger count = 0;
	txU4 size;
	txU1 *p = fxGetArchiveModules(the, archive, &size);
	if (p) {
		txU1 *q = p + size;
		while (p < q) {
			// PATH
			p += c_read32be(p);
			// CODE
			p += c_read32be(p);
			count += 1;
		}
	}
	return count;
}

void* fxGetArchiveCodeName(txMachine* the, void* archive, txInteger index)
{
	txU4 atomSize;
	txU1 *p = fxGetArchiveModules(the, archive, &atomSize), *q;
	if (!p)
		return NULL;
	q = p + atomSize;
	while (p < q) {
		// PATH
		if (!index--)
			return (txString)(p + sizeof(Atom));
		p += c_read32be(p);
		// CODE
		p += c_read32be(p);
	}
	return C_NULL;
}

static txU1 *fxGetArchiveResources(txMachine *the, void* archive, txU4 *size)
{
	txPreparation* preparation = the->preparation;
	txU1* p = archive;
	if (!preparation || !p) {
		*size = 0;
		return NULL;
	}
	p += mxArchiveHeaderSize;
	// NAME
	p += c_read32be(p);
	// SYMB
	p += c_read32be(p);
	// IDEN
	p += c_read32be(p);
	// MAPS
	p += c_read32be(p);
	// MODS
	p += c_read32be(p);
	// RSRC
	*size = c_read32be(p) - sizeof(Atom);
	return p + sizeof(Atom);
}

void* fxGetArchiveData(txMachine* the, void* archive, txString path, size_t* size)
{
	txU4 atomSize;
	txU1 *p = fxGetArchiveResources(the, archive, &atomSize), *q;
	if (!p)
		return NULL; 
	q = p + atomSize;
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
	return C_NULL;
}

txInteger fxGetArchiveDataCount(txMachine* the, void* archive)
{
	txInteger count = 0;
	txU4 size;
	txU1 *p = fxGetArchiveResources(the, archive, &size);
	if (p) {
		txU1 *q = p + size;
		while (p < q) {
			// PATH
			p += c_read32be(p);
			// DATA
			p += c_read32be(p);
			count += 1;
		}
	}
	return count;
}

void* fxGetArchiveDataName(txMachine* the, void* archive, txInteger index)
{
	txU4 atomSize;
	txU1 *p = fxGetArchiveResources(the, archive, &atomSize), *q;
	if (!p)
		return NULL;
	q = p + atomSize;
	while (p < q) {
		// PATH
		if (!index--)
			return (txString)(p + sizeof(Atom));
		p += c_read32be(p);
		// DATA
		p += c_read32be(p);
	}
	return C_NULL;
}

void* fxGetArchiveName(txMachine* the, void* archive)
{
	txU1* p = archive;
	if (!p)
		return NULL;
	p += mxArchiveHeaderSize;
	// NAME
	return p + sizeof(Atom);
}

void* fxMapArchive(txMachine* the, txPreparation* preparation, void* archive, size_t bufferSize, txArchiveRead read, txArchiveWrite write)
{
	txMapper mapper;
	txMapper* self = &mapper;
	Atom atom;
	txU1* p;
	txU1* q;
	txID id;
	txID c, i;
	txFlag clean;

	c_memset(self, 0, sizeof(txMapper));
	if (c_setjmp(self->jmp_buf) == 0) {
		self->machine = the;
		self->archive = archive;
		self->read = read;
		self->write = write;
		
		self->scratchSize = 1024;
		self->scratch = c_malloc(self->scratchSize + bufferSize);
		mxElseNotEnoughMemory(self->scratch != C_NULL);
		self->bufferSize = bufferSize;
		self->buffer = self->scratch + self->scratchSize;
		
		mxElseFatalCheck(self->read(self->archive, 0, self->buffer, mxArchiveHeaderSize));
	
		p = self->buffer;
		mxMapAtom(p);
		if (atom.atomType != XS_ATOM_ARCHIVE) {
			self->archive = NULL;
			goto bail;
		}
		self->size = atom.atomSize;
		mxMapAtom(p);
		mxElseFatalCheck(atom.atomType == XS_ATOM_VERSION);
		mxElseFatalCheck(atom.atomSize == sizeof(Atom) + 4);
		mxElseFatalCheck(*p++ == XS_MAJOR_VERSION);
		mxElseFatalCheck(*p++ == XS_MINOR_VERSION);
		p++;
		p++;
		mxMapAtom(p);
		mxElseFatalCheck(atom.atomType == XS_ATOM_SIGNATURE);
		mxElseFatalCheck(atom.atomSize == sizeof(Atom) + XS_DIGEST_SIZE);
		p += XS_DIGEST_SIZE;
		
		self->bufferOffset = mxArchiveHeaderSize;
		if (self->bufferSize > self->size)
			self->bufferSize = self->size;
		mxElseFatalCheck(self->read(self->archive, mxArchiveHeaderSize, p, self->bufferSize - mxArchiveHeaderSize));
	
		fxMapperReadAtom(self, &atom);
		mxElseFatalCheck(atom.atomType == XS_ATOM_NAME);
		fxMapperSkip(self, atom.atomSize - sizeof(Atom));
	
		fxMapperReadAtom(self, &atom);
		mxElseFatalCheck(atom.atomType == XS_ATOM_SYMBOLS);
		c = fxMapperRead2(self);
		self->ids = c_malloc(c * sizeof(txID));
		mxElseFatalCheck(self->ids != C_NULL);
		id = (txID)preparation->keyCount;
		for (i = 0; i < c; i++) {
			txU1 byte;
			txU4 sum = 0;
			txU4 modulo = 0;
			txSlot* result;
			p = self->scratch;
			q = p + self->scratchSize;
			while ((byte = fxMapperRead1(self))) {
				mxElseFatalCheck(p < q);
				*p++ = byte;
				sum = (sum << 1) + byte;
			}
			mxElseFatalCheck(p < q);
			*p = 0;
			if (i == 0)
				self->ids[i] = XS_NO_ID;
			else if (the)
				self->ids[i] = fxID(the, (txString)self->scratch);
			else {
				sum &= 0x7FFFFFFF;
				modulo = sum % preparation->nameModulo;
				result = preparation->names[modulo];
				while (result != C_NULL) {
					if (result->value.key.sum == sum)
						if (c_strcmp(result->value.key.string, (txString)self->scratch) == 0)
							break;
					result = result->next;
				}
				if (result)
					self->ids[i] = result->ID;
				else {
					self->ids[i] = id;
					id++;
				}
			}
		}
		
		fxMapperReadAtom(self, &atom);
		mxElseFatalCheck(atom.atomType == XS_ATOM_IDENTIFIERS);
		self->bufferLoop = self->bufferOffset - sizeof(Atom) + atom.atomSize;
		i = 0;
		clean = 1;
		while (self->bufferOffset < self->bufferLoop) {
			txID id = self->ids[i];
			txU1 low = (txU1)(id & 0x00FF);
			txU1 high =  (txU1)(id >> 8);
			if (self->bufferOffset == self->bufferSize)
				fxMapperStep(self);
			if (*(self->buffer + self->bufferOffset) != low) {
				*(self->buffer + self->bufferOffset) = low;
				self->dirty = 1;
				clean = 0;
			}
			self->bufferOffset++;
			if (self->bufferOffset == self->bufferSize)
				fxMapperStep(self);
			if (*(self->buffer + self->bufferOffset) != high) {
				*(self->buffer + self->bufferOffset) = high;
				self->dirty = 1;
				clean = 0;
			}
			self->bufferOffset++;
			i++;
		}
		if (clean)
			goto bail;
		
		fxMapperReadAtom(self, &atom);
		mxElseFatalCheck(atom.atomType == XS_ATOM_MAPS);
		self->bufferLoop = self->bufferOffset - sizeof(Atom) + atom.atomSize;
		self->maps = self->map = c_malloc((self->bufferLoop - self->bufferOffset));
		mxElseFatalCheck(self->maps != C_NULL);
		while (self->bufferOffset < self->bufferLoop)
			*self->map++ = fxMapperRead2(self);
		self->map = self->maps;
		
		fxMapperReadAtom(self, &atom);
		mxElseFatalCheck(atom.atomType == XS_ATOM_MODULES);
		self->bufferLoop = self->bufferOffset - sizeof(Atom) + atom.atomSize;
		while (self->bufferOffset < self->bufferLoop) {
			id += 2;
			fxMapperReadAtom(self, &atom);
			mxElseFatalCheck(atom.atomType == XS_ATOM_PATH);
			fxMapperSkip(self, atom.atomSize - sizeof(Atom));
			fxMapperReadAtom(self, &atom);
			mxElseFatalCheck(atom.atomType == XS_ATOM_CODE);
			self->bufferCode = self->bufferOffset - sizeof(Atom) + atom.atomSize;
			fxMapperMapIDs(self);
		}
		
		if (preparation->creation.incrementalKeyCount == 0)
 			mxElseNoMoreKeys((id - (txID)preparation->keyCount) < (txID)preparation->creation.initialKeyCount);
	
		fxMapperReadAtom(self, &atom);
		mxElseFatalCheck(atom.atomType == XS_ATOM_RESOURCES);
		self->bufferLoop = self->bufferOffset - sizeof(Atom) + atom.atomSize;
		while (self->bufferOffset < self->bufferLoop) {
			fxMapperReadAtom(self, &atom);
			mxElseFatalCheck(atom.atomType == XS_ATOM_PATH);
			fxMapperSkip(self, atom.atomSize - sizeof(Atom));
			fxMapperReadAtom(self, &atom);
			mxElseFatalCheck(atom.atomType == XS_ATOM_DATA);
			fxMapperSkip(self, atom.atomSize - sizeof(Atom));
		}
	
		if (self->bufferOffset) {
			if (self->dirty) {
				mxElseFatalCheck(self->write(self->archive, self->offset, self->buffer, self->bufferOffset));
				self->dirty = 0;
			}
		}
	}
	else {
		self->buffer[0] = 0;
		self->buffer[1] = 0;
		self->buffer[2] = 0;
		self->buffer[3] = 9;
		self->buffer[4] = 'X';
		self->buffer[5] = 'S';
		self->buffer[6] = '_';
		self->buffer[7] = 'E';
		self->write(self->archive, 0, self->buffer, 9);
		self->archive = C_NULL;
	}
bail:
	if (self->ids)
		c_free(self->ids);
	if (self->maps)
		c_free(self->maps);
	if (self->scratch)
		c_free(self->scratch);
	return self->archive;
}

void fxMapperMapID(txMapper* self)
{
	txID id = self->ids[*(self->map++)];
	if (self->bufferOffset == self->bufferSize)
		fxMapperStep(self);
	*(self->buffer + self->bufferOffset) = (txU1)(id & 0x00FF);
	self->bufferOffset++;
	self->dirty = 1;
	if (self->bufferOffset == self->bufferSize)
		fxMapperStep(self);
	*(self->buffer + self->bufferOffset) = (txU1)(id >> 8);
	self->bufferOffset++;
	self->dirty = 1;
}

void fxMapperMapIDs(txMapper* self)
{
	register const txS1* bytes = gxCodeSizes;
	txS1 offset;
	txU4 index;
	while (self->bufferOffset < self->bufferCode) {
		//fprintf(stderr, "%s", gxCodeNames[*((txU1*)p)]);
		offset = (txS1)c_read8(bytes + fxMapperRead1(self));
		if (0 < offset)
			fxMapperSkip(self, offset - 1);
		else if (0 == offset)
			fxMapperMapID(self);
		else if (-1 == offset) {
			index = fxMapperRead1(self);
			fxMapperSkip(self, index);
		}
		else if (-2 == offset) {
			index = fxMapperRead2(self); 
			fxMapperSkip(self, index);
		}
		//fprintf(stderr, "\n");
	}
}

txU1 fxMapperRead1(txMapper* self)
{
	txU1 result;
	if (self->bufferOffset == self->bufferSize)
		fxMapperStep(self);
	result = *(self->buffer + self->bufferOffset);
	self->bufferOffset++;
	return result;
}

txU2 fxMapperRead2(txMapper* self)
{
	txU2 result;
	result = fxMapperRead1(self);
	result |= fxMapperRead1(self) << 8;
	return result;
}

txU4 fxMapperRead4(txMapper* self)
{
	txU4 result;
	result = fxMapperRead1(self) << 24;
	result |= fxMapperRead1(self) << 16;
	result |= fxMapperRead1(self) << 8;
	result |= fxMapperRead1(self);
	return result;
}

void fxMapperReadAtom(txMapper* self, Atom* atom)
{
	atom->atomSize = fxMapperRead4(self);
	atom->atomType = fxMapperRead4(self);
}

void fxMapperSkip(txMapper* self, size_t size)
{
	size_t offset = self->bufferOffset + size;
	while ((offset >= self->bufferSize) && (self->bufferSize > 0)) {
		offset -= self->bufferSize;
		fxMapperStep(self);
	}
	self->bufferOffset = offset;
}

void fxMapperStep(txMapper* self)
{
	if (self->dirty) {
		mxElseFatalCheck(self->write(self->archive, self->offset, self->buffer, self->bufferSize));
		self->dirty = 0;
	}
	self->offset += self->bufferSize;
	self->size -= self->bufferSize;
	self->bufferCode -= self->bufferSize;
	self->bufferLoop -= self->bufferSize;
	if (self->bufferSize > self->size)
		self->bufferSize = self->size;
	if (self->bufferSize > 0)
		mxElseFatalCheck(self->read(self->archive, self->offset, self->buffer, self->bufferSize));
	self->bufferOffset = 0;
}

txBoolean fxIsProfiling(txMachine* the)
{
#if defined(mxInstrument) || defined(mxProfile)
	return (the->profiler) ? 1 : 0;
#else
	return 0;
#endif
}

void fxStartProfiling(txMachine* the)
{
#if defined(mxInstrument) || defined(mxProfile)
	if (the->profiler)
		return;	
// 	if (the->frame)
// 		fxAbort(the, XS_FATAL_CHECK_EXIT);
	fxCreateProfiler(the);
#endif
}

void fxStopProfiling(txMachine* the, void* stream)
{
#if defined(mxInstrument) || defined(mxProfile)
	if (!the->profiler)
		return;	
// 	if (the->frame)
// 		fxAbort(the, XS_FATAL_CHECK_EXIT);
	fxDeleteProfiler(the, stream);
#endif
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


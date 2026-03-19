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

//#define mxPromisePrint 1


static txSlot* fxCheckIteratorStep(txMachine* the, txSlot* slot);
static txBoolean fxGetIteratorFlattenable(txMachine* the, txSlot* iterable, txSlot* iterator, txSlot* next, txBoolean optional);
static txSlot* fxNewIteratorHelperInstance(txMachine* the, txSlot* iterator, txInteger step);

static void fxNewGeneratorResult(txMachine* the, txBoolean done);

static txSlot* fxCheckGeneratorInstance(txMachine* the, txSlot* slot);
static void fx_Generator_prototype_aux(txMachine* the, txFlag status);

static void fxAsyncGeneratorStep(txMachine* the, txSlot* generator, txFlag status);
static txSlot* fxCheckAsyncGeneratorInstance(txMachine* the, txSlot* slot);
static void fx_AsyncGenerator_prototype_aux(txMachine* the, txFlag status);

static txSlot* fxCheckAsyncFromSyncIteratorInstance(txMachine* the, txSlot* slot);
static void fx_AsyncFromSyncIterator_prototype_aux(txMachine* the, txFlag status, txSlot* iterator, txBoolean* flag);

void fxBuildGenerator(txMachine* the)
{
	txSlot* slot;
	txSlot* property;

	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, the->stack->value.reference);
#if mxECMAScript2025
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_prototype_drop), 1, mxID(_drop), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_prototype_every), 1, mxID(_every), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_prototype_filter), 1, mxID(_filter), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_prototype_find), 1, mxID(_find), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_prototype_flatMap), 1, mxID(_flatMap), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_prototype_forEach), 1, mxID(_forEach), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_prototype_map), 1, mxID(_map), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_prototype_reduce), 1, mxID(_reduce), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_prototype_some), 1, mxID(_some), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_prototype_take), 1, mxID(_take), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_prototype_toArray), 0, mxID(_toArray), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_Iterator_prototype_toStringTag_get), mxCallback(fx_Iterator_prototype_toStringTag_set), mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG);
#endif
#if mxExplicitResourceManagement
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_prototype_dispose), 0, mxID(_Symbol_dispose), XS_DONT_ENUM_FLAG);
#endif
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_prototype_iterator), 0, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
#if mxECMAScript2025
	property = slot;
	slot = fxBuildHostConstructor(the, mxCallback(fx_Iterator), 0, mxID(_Iterator));
	mxIteratorConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
#if mxECMAScript2026
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_concat), 0, mxID(_concat), XS_DONT_ENUM_FLAG);
#endif
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_from), 1, mxID(_from), XS_DONT_ENUM_FLAG);
	
	mxPush(mxIteratorPrototype);
	property = fxNextHostAccessorProperty(the, property, mxCallback(fx_Iterator_prototype_constructor_get), mxCallback(fx_Iterator_prototype_constructor_set), mxID(_constructor), XS_DONT_ENUM_FLAG);
	mxPop();
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_IteratorHelper_prototype_next), 1, mxID(_next), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_IteratorHelper_prototype_return), 1, mxID(_return), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Iterator Helper", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxIteratorHelperPrototype);
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_IteratorWrapper_prototype_next), 1, mxID(_next), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_IteratorWrapper_prototype_return), 1, mxID(_return), XS_DONT_ENUM_FLAG);
	mxPull(mxIteratorWrapperPrototype);

#endif

	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Generator_prototype_next), 1, mxID(_next), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Generator_prototype_return), 1, mxID(_return), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Generator_prototype_throw), 1, mxID(_throw), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Generator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxGeneratorPrototype = *the->stack;
	mxPop();
	
	mxPush(mxFunctionPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextSlotProperty(the, slot, &mxGeneratorPrototype, mxID(_prototype), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	property = mxBehaviorSetProperty(the, mxGeneratorPrototype.value.reference, mxID(_constructor), 0, XS_OWN);
	property->flag = XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	slot = fxNextStringXProperty(the, slot, "GeneratorFunction", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxGeneratorFunctionPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_GeneratorFunction), 1, mxID(_GeneratorFunction));
	slot = mxBehaviorGetProperty(the, mxGeneratorFunctionPrototype.value.reference, mxID(_constructor), 0, XS_OWN);
	slot->flag |= XS_DONT_SET_FLAG;
	mxPop();
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Enumerator_prototype_next), 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	fxNewHostConstructor(the, mxCallback(fx_Enumerator), 0, XS_NO_ID);
	mxPull(mxEnumeratorFunction);

	mxPush(mxObjectPrototype);
	slot = fxNewObjectInstance(the);
#if mxExplicitResourceManagement
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncIterator_prototype_asyncDispose), 0, mxID(_Symbol_asyncDispose), XS_DONT_ENUM_FLAG);
#endif
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncIterator_prototype_asyncIterator), 0, mxID(_Symbol_asyncIterator), XS_DONT_ENUM_FLAG);
	mxPull(mxAsyncIteratorPrototype);
	
	mxPush(mxAsyncIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncGenerator_prototype_next), 1, mxID(_next), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncGenerator_prototype_return), 1, mxID(_return), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncGenerator_prototype_throw), 1, mxID(_throw), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "AsyncGenerator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxAsyncGeneratorPrototype = *the->stack;
	mxPop();

	mxPush(mxFunctionPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextSlotProperty(the, slot, &mxAsyncGeneratorPrototype, mxID(_prototype), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	property = mxBehaviorSetProperty(the, mxAsyncGeneratorPrototype.value.reference, mxID(_constructor), 0, XS_OWN);
	property->flag = XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	slot = fxNextStringXProperty(the, slot, "AsyncGeneratorFunction", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxAsyncGeneratorFunctionPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_AsyncGeneratorFunction), 1, mxID(_AsyncGeneratorFunction));
	slot = mxBehaviorGetProperty(the, mxAsyncGeneratorFunctionPrototype.value.reference, mxID(_constructor), 0, XS_OWN);
	slot->flag |= XS_DONT_SET_FLAG;
	mxPop();
	
	mxPush(mxAsyncIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncFromSyncIterator_prototype_next), 1, mxID(_next), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncFromSyncIterator_prototype_return), 1, mxID(_return), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncFromSyncIterator_prototype_throw), 1, mxID(_throw), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Async-from-Sync Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxAsyncFromSyncIteratorPrototype = *the->stack;
	mxPop();
}

typedef txBoolean (*txIteratorStep)(txMachine* the, txSlot* iterator, txSlot* next, txSlot* extra, txSlot* value, txFlag status);

enum {
	mx_Iterator_prototype_drop = 0,
	mx_Iterator_prototype_filter,
	mx_Iterator_prototype_flatMap,
	mx_Iterator_prototype_map,
	mx_Iterator_prototype_take,
	mx_Iterator_concat,
	mxIteratorStepCount
};

static txBoolean fx_Iterator_prototype_drop_step(txMachine* the, txSlot* iterator, txSlot* next, txSlot* extra, txSlot* value, txFlag status);
static txBoolean fx_Iterator_prototype_filter_step(txMachine* the, txSlot* iterator, txSlot* next, txSlot* extra, txSlot* value, txFlag status);
static txBoolean fx_Iterator_prototype_flatMap_step(txMachine* the, txSlot* iterator, txSlot* next, txSlot* extra, txSlot* value, txFlag status);
static txBoolean fx_Iterator_prototype_map_step(txMachine* the, txSlot* iterator, txSlot* next, txSlot* extra, txSlot* value, txFlag status);
static txBoolean fx_Iterator_prototype_take_step(txMachine* the, txSlot* iterator, txSlot* next, txSlot* extra, txSlot* value, txFlag status);
static txBoolean fx_Iterator_concat_step(txMachine* the, txSlot* iterator, txSlot* next, txSlot* extra, txSlot* value, txFlag status);

const txIteratorStep ICACHE_FLASH_ATTR gxIteratorSteps[mxIteratorStepCount]  = {
	fx_Iterator_prototype_drop_step,
	fx_Iterator_prototype_filter_step,
	fx_Iterator_prototype_flatMap_step,
	fx_Iterator_prototype_map_step,
	fx_Iterator_prototype_take_step,
	fx_Iterator_concat_step,
};

const txID ICACHE_FLASH_ATTR gxIteratorStepIDs[mxIteratorStepCount]  = {
	mxID(_drop),
	mxID(_filter),
	mxID(_flatMap),
	mxID(_map),
	mxID(_take),
};

txSlot* fxCheckIteratorInstance(txMachine* the, txSlot* slot, txID id)
{
	txSlot* instance;
	if (slot->kind == XS_REFERENCE_KIND) {
		instance = slot->value.reference;
		slot = instance->next;
		if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->ID == id) && (slot->kind == XS_REFERENCE_KIND)) {
			return instance;
		}
	}
	mxTypeError("this: not an iterator");
	return C_NULL;
}

txSlot* fxCheckIteratorResult(txMachine* the, txSlot* result) 
{
	txSlot* value = result->value.reference->next;
	while (value && (value->flag & XS_INTERNAL_FLAG))
		value = value->next;
	mxCheck(the, (value != C_NULL) && (value->ID == mxID(_value)));
	mxCheck(the, (value->next != C_NULL) && (value->next->ID == mxID(_done)));
	return value;
}

txSlot* fxCheckIteratorStep(txMachine* the, txSlot* slot) 
{
	if (slot && (slot->kind == XS_INTEGER_KIND)) {
		txInteger step = slot->value.integer;
		if ((0 <= step) && (step < mxIteratorStepCount)) {
			if (slot->ID == gxIteratorStepIDs[step]) {
				return slot;
			}
		}
	}
	mxTypeError("this: not an iterator helper");
	return C_NULL;
}

txBoolean fxIteratorNext(txMachine* the, txSlot* iterator, txSlot* next, txSlot* value)
{
	mxPushSlot(iterator);
	mxPushSlot(next);
	mxCall();
	mxRunCount(0);
	if (!mxIsReference(the->stack))
		mxTypeError("iterator result: not an object");
	mxDub();
	mxGetID(mxID(_done));
	if (fxToBoolean(the, the->stack)) {
		mxPop();
		mxPop();
		return 0;
	}
	mxPop();
	mxGetID(mxID(_value));
	mxPullSlot(value);
	return 1;
}

void fxIteratorReturn(txMachine* the, txSlot* iterator, txBoolean abrupt)
{
	if (abrupt)
		mxPush(mxException);
	mxTry(the) {
		mxPushSlot(iterator);
		mxDub();
		mxGetID(mxID(_return));
		if (mxIsUndefined(the->stack) || mxIsNull(the->stack)) 
			mxPop();
		else {
			mxCall();
			mxRunCount(0);
			if (!mxIsReference(the->stack))
				mxTypeError("iterator result: not an object");
		}
		mxPop();
	}
	mxCatch(the) {
		if (!abrupt)
			fxJump(the);
	}
	if (abrupt)
		mxPull(mxException);
}

txBoolean fxGetIterator(txMachine* the, txSlot* iterable, txSlot* iterator, txSlot* next, txBoolean optional)
{
	mxPushSlot(iterable);
	mxDub();
	mxGetID(mxID(_Symbol_iterator));
	if (optional && (mxIsUndefined(the->stack) || mxIsNull(the->stack))) {
		mxPop();
		mxPop();
		return 0;
	}
	mxCall();
	mxRunCount(0);
	if (!mxIsReference(the->stack))
		mxTypeError("iterator: not an object");
	if (next) {
		mxDub();
		mxGetID(mxID(_next));
		mxPullSlot(next);
	}
	mxPullSlot(iterator);
	return 1;
}

txBoolean fxGetIteratorFlattenable(txMachine* the, txSlot* iterable, txSlot* iterator, txSlot* next, txBoolean optional)
{
	if (!mxIsReference(iterable)) {
		if (optional) {
			if ((iterable->kind != XS_STRING_KIND) && (iterable->kind != XS_STRING_X_KIND))
				mxTypeError("iterator: not a string");
		}
		else
			mxTypeError("iterator: not an object");
	}
	mxPushSlot(iterable);
	mxDub();
	mxGetID(mxID(_Symbol_iterator));
	if ((mxIsUndefined(the->stack) || mxIsNull(the->stack)))
		mxPop();
	else {
		mxCall();
		mxRunCount(0);
	}
	if (!mxIsReference(the->stack))
		mxTypeError("iterator: not an object");
	mxDub();
	mxGetID(mxID(_next));
	mxPullSlot(next);
	mxPullSlot(iterator);
	return 1;
}

txSlot* fxNewIteratorInstance(txMachine* the, txSlot* iterable, txID id) 
{
	txSlot* instance;
	txSlot* result;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	mxPush(mxObjectPrototype);
	result = fxNewObjectInstance(the);
	property = fxNextUndefinedProperty(the, result, mxID(_value), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	property = fxNextBooleanProperty(the, property, 0, mxID(_done), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	property = fxNextSlotProperty(the, instance, the->stack, id, XS_INTERNAL_FLAG);
	property = fxNextSlotProperty(the, property, iterable, XS_NO_ID, XS_INTERNAL_FLAG);
	property = fxNextIntegerProperty(the, property, 0, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
	return instance;
}

void fxSetterThatIgnoresPrototypeProperties(txMachine* the, txSlot* reference, txID id, txString name, txSlot* value)
{
	txSlot* instance = fxGetInstance(the, reference);
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* property;
	if (!instance)
		mxTypeError("set %s: not an object", name);
	if (home->value.home.object == instance)
		mxTypeError("set %s: not writable", name);
	mxTemporary(property);
	if (!mxBehaviorGetOwnProperty(the, instance, id, 0, property)) {
		if (!mxBehaviorDefineOwnProperty(the, instance, id, 0, value, XS_NO_FLAG))
			mxTypeError("set %s: not extensible", name);
	}
	else /*if (!mxIsProxy(instance))*/ {
		property = mxBehaviorSetProperty(the, instance, id, 0, XS_OWN);
		if (!property)
			mxTypeError("set %s: not writable", name);
        if (property->kind == XS_ACCESSOR_KIND) {
            txSlot* function = property->value.accessor.setter;
            if (!mxIsFunction(function))
                mxTypeError("set %s: not writable", name);
            mxPushSlot(reference);
            mxPushReference(function);
            mxCall();
            mxPushSlot(value);
            mxRunCount(1);
            mxPop();
       }
        else {
            property->kind = value->kind;
            property->value = value->value;
        }
	}
	mxPop();
}

void fx_Iterator(txMachine* the)
{
	if (!mxHasTarget)
		mxTypeError("call: Iterator");
	if (fxIsSameSlot(the, mxTarget, mxFunction))
		mxTypeError("new: Iterator");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxIteratorPrototype);
	fxNewObjectInstance(the);
	mxPullSlot(mxResult);
}

void fx_Iterator_concat(txMachine* the)
{
	txSlot* list = fxNewInstance(the);
	txSlot* slot = list;
	txSlot* property;
	txInteger c = mxArgc, i;
	for (i = 0; i < c; i++) {
		txSlot* item = mxArgv(i);
		if (!mxIsReference(item))
			mxTypeError("items[%d]: not an object", i);
		mxPushSlot(item);
		slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
		mxGetID(mxID(_Symbol_iterator));
		if (!fxIsCallable(the, the->stack))
			mxTypeError("items[%d]: not iterable", i);
		slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
		mxPop();
	}
	mxPushNull();
	property = fxLastProperty(the, fxNewIteratorHelperInstance(the, the->stack, mx_Iterator_concat));
	property = fxNextReferenceProperty(the, property, list, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPullSlot(mxResult);
	mxPop();
	mxPop();
}

txBoolean fx_Iterator_concat_step(txMachine* the, txSlot* iterator, txSlot* next, txSlot* extra, txSlot* value, txFlag status)
{
	if (status == XS_NO_STATUS) {
		for (;;) {
			if (mxIsNull(iterator)) {
				txSlot* iterable = extra->value.reference->next;
				if (!iterable)
					break;
				mxPushSlot(iterable);
				mxPushSlot(iterable->next);
				extra->value.reference->next = iterable->next->next;
				mxCall();
				mxRunCount(0);
				if (!mxIsReference(the->stack))
					mxTypeError("iterator: not an object");
				mxDub();
				mxGetID(mxID(_next));
				mxPullSlot(next);
				mxPullSlot(iterator);
			}
			if (fxIteratorNext(the, iterator, next, value))
				return 1;
			next->kind = XS_NULL_KIND;
			iterator->kind = XS_NULL_KIND;
		}
	}
	else if (status == XS_RETURN_STATUS) {
		if (!mxIsNull(iterator))
			fxIteratorReturn(the, iterator, 0);
	}
	else if (status == XS_THROW_STATUS) {
		if (!mxIsNull(iterator))
			fxIteratorReturn(the, iterator, 1);
	}
	return 0;
}

void fx_Iterator_from(txMachine* the)
{
	txSlot *iterator, *next, *property;
	mxTemporary(iterator);
	mxTemporary(next);
	fxGetIteratorFlattenable(the, mxArgv(0), iterator, next, 1);
	mxPush(mxIteratorConstructor);
	mxDub();
	mxGetID(mxID(_Symbol_hasInstance));
	mxCall();
	mxPushSlot(iterator);
	mxRunCount(1);
	if (fxToBoolean(the, the->stack)) {
		mxResult->kind = iterator->kind;
		mxResult->value = iterator->value;
	}
	else {
		mxPush(mxIteratorWrapperPrototype);
		property = fxLastProperty(the, fxNewIteratorInstance(the, iterator, mxID(_Iterator)));
		property = fxNextSlotProperty(the, property, next, XS_NO_ID, XS_INTERNAL_FLAG);
		mxPullSlot(mxResult);
	}
	mxPop();
	mxPop();
	mxPop();
}

void fx_Iterator_prototype_constructor_get(txMachine* the)
{
	mxPush(mxIteratorConstructor);
	mxPullSlot(mxResult);
}

void fx_Iterator_prototype_constructor_set(txMachine* the)
{
	fxSetterThatIgnoresPrototypeProperties(the, mxThis, mxID(_constructor), "constructor", mxArgv(0));
}

void fx_Iterator_prototype_dispose(txMachine* the)
{	
	fxIteratorReturn(the, mxThis, 1);
}

void fx_Iterator_prototype_drop(txMachine* the)
{
	txSlot *iterator, *property;
	txNumber limit;
	if (!fxGetInstance(the, mxThis))
		mxTypeError("this: not an object");
	iterator = mxThis;
	{
		mxTry(the) {
			if (mxArgc > 0)
				mxPushSlot(mxArgv(0));
			else
				mxPushUndefined();
			limit = fxToNumber(the, the->stack);
			if (c_isnan(limit))
				mxRangeError("limit: NaN");
			if (c_isfinite(limit))
				limit = c_trunc(limit);
			if (limit < 0)
				mxRangeError("limit: < 0");
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	property = fxLastProperty(the, fxNewIteratorHelperInstance(the, iterator, mx_Iterator_prototype_drop));
	property = fxNextNumberProperty(the, property, limit, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPullSlot(mxResult);
	mxPop();
}

txBoolean fx_Iterator_prototype_drop_step(txMachine* the, txSlot* iterator, txSlot* next, txSlot* extra, txSlot* value, txFlag status)
{
	txNumber remaining = extra->value.number;
	if (status == XS_NO_STATUS) {
		while (remaining > 0) {
			if (fxIteratorNext(the, iterator, next, value)) {
				if (c_isfinite(remaining)) {
					remaining--;
					extra->value.number = remaining;
				}
			}
			else
				return 0;
		}
		if (fxIteratorNext(the, iterator, next, value))
			return 1;
	}
	else if (status == XS_RETURN_STATUS)
		fxIteratorReturn(the, iterator, 0);
	else if (status == XS_THROW_STATUS)
		fxIteratorReturn(the, iterator, 1);
	return 0;
}

void fx_Iterator_prototype_every(txMachine* the)
{	
	txSlot *iterator, *predicate, *next, *value;
	txInteger counter;
	if (!fxGetInstance(the, mxThis))
		mxTypeError("this: not an object");
	iterator = mxThis;
	{
		mxTry(the) {
			if (mxArgc > 0)
				mxPushSlot(mxArgv(0));
			else
				mxPushUndefined();
			predicate = the->stack;
			if (!fxIsCallable(the, predicate))
				mxTypeError("predicate: not a function");
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	mxPushSlot(iterator);
	mxGetID(mxID(_next));
	next = the->stack;
	mxTemporary(value);
	counter = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
	{
		mxTry(the) {
			while (fxIteratorNext(the, iterator, next, value)) {
				mxPushUndefined();
				mxPushSlot(predicate);
				mxCall();
				mxPushSlot(value);
				mxPushInteger(counter);
				mxRunCount(2);
				mxResult->value.boolean = fxToBoolean(the, the->stack);
				mxPop();
				if (!mxResult->value.boolean) {
					fxIteratorReturn(the, iterator, 0);
					break;
				}
				counter++;
			}
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	mxPop();
	mxPop();
	mxPop();
}

void fx_Iterator_prototype_filter(txMachine* the)
{	
	txSlot *iterator, *predicate, *property;
	if (!fxGetInstance(the, mxThis))
		mxTypeError("this: not an object");
	iterator = mxThis;
	{
		mxTry(the) {
			if (mxArgc > 0)
				mxPushSlot(mxArgv(0));
			else
				mxPushUndefined();
			predicate = the->stack;
			if (!fxIsCallable(the, predicate))
				mxTypeError("predicate: not a function");
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	property = fxLastProperty(the, fxNewIteratorHelperInstance(the, iterator, mx_Iterator_prototype_filter));
	property = fxNextSlotProperty(the, property, predicate, XS_NO_ID, XS_INTERNAL_FLAG);
	property = fxNextIntegerProperty(the, property, 0, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPullSlot(mxResult);
	mxPop();
}

txBoolean fx_Iterator_prototype_filter_step(txMachine* the, txSlot* iterator, txSlot* next, txSlot* extra, txSlot* value, txFlag status)
{
	txSlot* counter = extra->next;
	txBoolean selected = 0;
	if (status == XS_NO_STATUS) {
		while (fxIteratorNext(the, iterator, next, value)) {
			mxPushUndefined();
			mxPushSlot(extra);
			mxCall();
			mxPushSlot(value);
			mxPushSlot(counter);
			mxRunCount(2);
			selected = fxToBoolean(the, the->stack);
			mxPop();
			counter->value.integer++;
			if (selected)
				return 1;
		}
	}
	else if (status == XS_RETURN_STATUS)
		fxIteratorReturn(the, iterator, 0);
	else if (status == XS_THROW_STATUS)
		fxIteratorReturn(the, iterator, 1);
	return 0;
}

void fx_Iterator_prototype_find(txMachine* the)
{	
	txSlot *iterator, *predicate, *next, *value;
	txInteger counter;
	txBoolean result;
	if (!fxGetInstance(the, mxThis))
		mxTypeError("this: not an object");
	iterator = mxThis;
	{
		mxTry(the) {
			if (mxArgc > 0)
				mxPushSlot(mxArgv(0));
			else
				mxPushUndefined();
			predicate = the->stack;
			if (!fxIsCallable(the, predicate))
				mxTypeError("predicate: not a function");
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	mxPushSlot(iterator);
	mxGetID(mxID(_next));
	next = the->stack;
	mxTemporary(value);
	counter = 0;
	{
		mxTry(the) {
			while (fxIteratorNext(the, iterator, next, value)) {
				mxPushUndefined();
				mxPushSlot(predicate);
				mxCall();
				mxPushSlot(value);
				mxPushInteger(counter);
				mxRunCount(2);
				result = fxToBoolean(the, the->stack);
				mxPop();
				if (result) {
					mxResult->kind = value->kind;
					mxResult->value = value->value;
					fxIteratorReturn(the, iterator, 0);
					break;
				}
				counter++;
			}
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	mxPop();
	mxPop();
	mxPop();
}

void fx_Iterator_prototype_flatMap(txMachine* the)
{	
	txSlot *iterator, *mapper, *property;
	if (!fxGetInstance(the, mxThis))
		mxTypeError("this: not an object");
	iterator = mxThis;
	{
		mxTry(the) {
			if (mxArgc > 0)
				mxPushSlot(mxArgv(0));
			else
				mxPushUndefined();
			mapper = the->stack;
			if (!fxIsCallable(the, mapper))
				mxTypeError("mapper: not a function");
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	property = fxLastProperty(the, fxNewIteratorHelperInstance(the, iterator, mx_Iterator_prototype_flatMap));
	property = fxNextSlotProperty(the, property, mapper, XS_NO_ID, XS_INTERNAL_FLAG);
	property = fxNextIntegerProperty(the, property, 0, XS_NO_ID, XS_INTERNAL_FLAG);
	property = fxNextNullProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG);
	property = fxNextNullProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPullSlot(mxResult);
	mxPop();
}

txBoolean fx_Iterator_prototype_flatMap_step(txMachine* the, txSlot* iterator, txSlot* next, txSlot* extra, txSlot* value, txFlag status)
{
	txSlot* counter = extra->next;
	txSlot* innerIterator = counter->next;
	txSlot* innerNext = innerIterator->next;
	if (status == XS_NO_STATUS) {
	again:
		if (mxIsNull(innerIterator)) {
			if (fxIteratorNext(the, iterator, next, value)) {
				mxPushUndefined();
				mxPushSlot(extra);
				mxCall();
				mxPushSlot(value);
				mxPushSlot(counter);
				mxRunCount(2);
				fxGetIteratorFlattenable(the, the->stack, innerIterator, innerNext, 0);
				mxPop();
				counter->value.integer++;
			}
			else
				return 0;
		}
		if (fxIteratorNext(the, innerIterator, innerNext, value))
			return 1;
		innerIterator->kind = XS_NULL_KIND;
		innerNext->kind = XS_NULL_KIND;
		goto again;
	}
	else if (status == XS_RETURN_STATUS) {
		if (!mxIsNull(innerIterator))
			fxIteratorReturn(the, innerIterator, 0);
		fxIteratorReturn(the, iterator, 0);
	}
	else if (status == XS_THROW_STATUS) {
		if (!mxIsNull(innerIterator))
			fxIteratorReturn(the, innerIterator, 1);
		fxIteratorReturn(the, iterator, 1);
	}
	return 0;
}

void fx_Iterator_prototype_forEach(txMachine* the)
{	
	txSlot *iterator, *procedure, *next, *value;
	txInteger counter;
	if (!fxGetInstance(the, mxThis))
		mxTypeError("this: not an object");
	iterator = mxThis;
	{
		mxTry(the) {
			if (mxArgc > 0)
				mxPushSlot(mxArgv(0));
			else
				mxPushUndefined();
			procedure = the->stack;
			if (!fxIsCallable(the, procedure))
				mxTypeError("procedure: not a function");
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	mxPushSlot(iterator);
	mxGetID(mxID(_next));
	next = the->stack;
	mxTemporary(value);
	counter = 0;
	{
		mxTry(the) {
			while (fxIteratorNext(the, iterator, next, value)) {
				mxPushUndefined();
				mxPushSlot(procedure);
				mxCall();
				mxPushSlot(value);
				mxPushInteger(counter);
				mxRunCount(2);
				mxPop();
				counter++;
			}
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	mxPop();
	mxPop();
	mxPop();
}

void fx_Iterator_prototype_iterator(txMachine* the)
{
	*mxResult = *mxThis;
}

void fx_Iterator_prototype_map(txMachine* the)
{	
	txSlot *iterator, *mapper, *property;
	if (!fxGetInstance(the, mxThis))
		mxTypeError("this: not an object");
	iterator = mxThis;
	{
		mxTry(the) {
			if (mxArgc > 0)
				mxPushSlot(mxArgv(0));
			else
				mxPushUndefined();
			mapper = the->stack;
			if (!fxIsCallable(the, mapper))
				mxTypeError("mapper: not a function");
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	property = fxLastProperty(the, fxNewIteratorHelperInstance(the, iterator, mx_Iterator_prototype_map));
	property = fxNextSlotProperty(the, property, mapper, XS_NO_ID, XS_INTERNAL_FLAG);
	property = fxNextIntegerProperty(the, property, 0, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPullSlot(mxResult);
	mxPop();
}

txBoolean fx_Iterator_prototype_map_step(txMachine* the, txSlot* iterator, txSlot* next, txSlot* extra, txSlot* value, txFlag status)
{
	txSlot* counter = extra->next;
	if (status == XS_NO_STATUS) {
		if (fxIteratorNext(the, iterator, next, value)) {
			mxPushUndefined();
			mxPushSlot(extra);
			mxCall();
			mxPushSlot(value);
			mxPushSlot(counter);
			mxRunCount(2);
			mxPullSlot(value);
			counter->value.integer++;
			return 1;
		}
	}
	else if (status == XS_RETURN_STATUS)
		fxIteratorReturn(the, iterator, 0);
	else if (status == XS_THROW_STATUS)
		fxIteratorReturn(the, iterator, 1);
	return 0;
}

void fx_Iterator_prototype_reduce(txMachine* the)
{	
	txSlot *iterator, *reducer, *next, *value;
	txInteger counter;
	if (!fxGetInstance(the, mxThis))
		mxTypeError("this: not an object");
	iterator = mxThis;
	{
		mxTry(the) {
			if (mxArgc > 0)
				mxPushSlot(mxArgv(0));
			else
				mxPushUndefined();
			reducer = the->stack;
			if (!fxIsCallable(the, reducer))
				mxTypeError("reducer: not a function");
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	mxPushSlot(iterator);
	mxGetID(mxID(_next));
	next = the->stack;
	mxTemporary(value);
	if (mxArgc > 1) {
		mxPushSlot(mxArgv(1));
		counter = 0;
	}
	else {
		mxPushUndefined();
		if (!fxIteratorNext(the, iterator, next, the->stack))
			mxTypeError("no initial value");
		counter = 1;
	}
	mxPullSlot(mxResult);
	{
		mxTry(the) {
			while (fxIteratorNext(the, iterator, next, value)) {
				mxPushUndefined();
				mxPushSlot(reducer);
				mxCall();
				mxPushSlot(mxResult);
				mxPushSlot(value);
				mxPushInteger(counter);
				mxRunCount(3);
				mxPullSlot(mxResult);
				counter++;
			}
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	mxPop();
	mxPop();
	mxPop();
}

void fx_Iterator_prototype_some(txMachine* the)
{	
	txSlot *iterator, *predicate, *next, *value;
	txInteger counter;
	if (!fxGetInstance(the, mxThis))
		mxTypeError("this: not an object");
	iterator = mxThis;
	{
		mxTry(the) {
			if (mxArgc > 0)
				mxPushSlot(mxArgv(0));
			else
				mxPushUndefined();
			predicate = the->stack;
			if (!fxIsCallable(the, predicate))
				mxTypeError("predicate: not a function");
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	mxPushSlot(iterator);
	mxGetID(mxID(_next));
	next = the->stack;
	mxTemporary(value);
	counter = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	{
		mxTry(the) {
			while (fxIteratorNext(the, iterator, next, value)) {
				mxPushUndefined();
				mxPushSlot(predicate);
				mxCall();
				mxPushSlot(value);
				mxPushInteger(counter);
				mxRunCount(2);
				mxResult->value.boolean = fxToBoolean(the, the->stack);
				mxPop();
				if (mxResult->value.boolean) {
					fxIteratorReturn(the, iterator, 0);
					break;
				}
				counter++;
			}
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	mxPop();
	mxPop();
	mxPop();
}

void fx_Iterator_prototype_take(txMachine* the)
{	
	txSlot *iterator, *property;
	txNumber limit;
	if (!fxGetInstance(the, mxThis))
		mxTypeError("this: not an object");
	iterator = mxThis;
	{
		mxTry(the) {
			if (mxArgc > 0)
				mxPushSlot(mxArgv(0));
			else
				mxPushUndefined();
			limit = fxToNumber(the, the->stack);
			if (c_isnan(limit))
				mxRangeError("limit: NaN");
			if (c_isfinite(limit))
				limit = c_trunc(limit);
			if (limit < 0)
				mxRangeError("limit: < 0");
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator, 1);
			fxJump(the);
		}
	}
	property = fxLastProperty(the, fxNewIteratorHelperInstance(the, iterator, mx_Iterator_prototype_take));
	property = fxNextNumberProperty(the, property, limit, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPullSlot(mxResult);
	mxPop();
}

txBoolean fx_Iterator_prototype_take_step(txMachine* the, txSlot* iterator, txSlot* next, txSlot* extra, txSlot* value, txFlag status)
{
	txNumber remaining = extra->value.number;
	if (status == XS_NO_STATUS) {
		if (remaining == 0) {
			fxIteratorReturn(the, iterator, 0);
			return 0;
		}
		if (c_isfinite(remaining)) {
			remaining--;
			extra->value.number = remaining;
		}
		if (fxIteratorNext(the, iterator, next, value))
			return 1;
	}
	else if (status == XS_RETURN_STATUS)
		fxIteratorReturn(the, iterator, 0);
	else if (status == XS_THROW_STATUS)
		fxIteratorReturn(the, iterator, 1);
	return 0;
}

void fx_Iterator_prototype_toArray(txMachine* the)
{	
	txSlot *iterator, *next, *value, *instance, *internal, *item;
	if (!fxGetInstance(the, mxThis))
		mxTypeError("this: not an object");
	iterator = mxThis;
	mxPushSlot(iterator);
	mxGetID(mxID(_next));
	next = the->stack;
	mxTemporary(value);
	mxPush(mxArrayPrototype);
	instance = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	internal = instance->next;
	item = internal;
	while (fxIteratorNext(the, iterator, next, value)) {
		internal->value.array.length++;
		item->next = fxNewSlot(the);
		item = item->next;
		item->kind = value->kind;
		item->value = value->value;
	}
	fxCacheArray(the, instance);
	mxPop();
	mxPop();
}

void fx_Iterator_prototype_toStringTag_get(txMachine* the)
{	
	mxPushStringC("Iterator");
	mxPullSlot(mxResult);
}

void fx_Iterator_prototype_toStringTag_set(txMachine* the)
{	
	fxSetterThatIgnoresPrototypeProperties(the, mxThis, mxID(_Symbol_toStringTag), "Symbol(toStringTag)", mxArgv(0));
}

txSlot* fxNewIteratorHelperInstance(txMachine* the, txSlot* iterator, txInteger step) 
{
	txSlot* instance;
	txSlot* property;
	mxPush(mxIteratorHelperPrototype);
	instance = fxNewIteratorInstance(the, iterator, mxID(_Iterator));
	property = fxLastProperty(the, instance);
	property->ID = gxIteratorStepIDs[step];
	property->value.integer = step;
	if (mxIsNull(iterator))
		property = fxNextNullProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG);
	else {
		mxPushSlot(iterator);
		mxGetID(mxID(_next));
		property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
		mxPop();
	}
	return instance;
}

void fx_IteratorHelper_prototype_next(txMachine* the)
{
	txSlot* instance = fxCheckIteratorInstance(the, mxThis, mxID(_Iterator));
	txSlot* result = instance->next;
	txSlot* iterator = result->next;
	txSlot* step = fxCheckIteratorStep(the, iterator->next);
	txSlot* next = step->next;
	txSlot* extra = next->next;
	txSlot* value = fxCheckIteratorResult(the, result);
	txSlot* done = value->next;
	if (step->flag & XS_DONT_SET_FLAG)
		mxTypeError("recursive iterator");
	{
		mxTry(the) {
			step->flag |= XS_DONT_SET_FLAG;
			if (!done->value.boolean) {
				if (!(*gxIteratorSteps[step->value.integer])(the, iterator, next, extra, value, XS_NO_STATUS)) {
					value->kind = XS_UNDEFINED_KIND;
					done->value.boolean = 1;
				}
			}
			mxResult->kind = result->kind;
			mxResult->value = result->value;
			step->flag &= ~XS_DONT_SET_FLAG;
		}
		mxCatch(the) {
			step->flag &= ~XS_DONT_SET_FLAG;
			value->kind = XS_UNDEFINED_KIND;
			done->value.boolean = 1;
			(*gxIteratorSteps[step->value.integer])(the, iterator, next, extra, value, XS_THROW_STATUS);
			fxJump(the);
		}
	}
}

void fx_IteratorHelper_prototype_return(txMachine* the)
{
	txSlot* instance = fxCheckIteratorInstance(the, mxThis, mxID(_Iterator));
	txSlot* result = instance->next;
	txSlot* iterator = result->next;
	txSlot* step = fxCheckIteratorStep(the, iterator->next);
	txSlot* next = step->next;
	txSlot* extra = next->next;
	txSlot* value = fxCheckIteratorResult(the, result);
	txSlot* done = value->next;
	if (step->flag & XS_DONT_SET_FLAG)
		mxTypeError("recursive iterator");
	{
		mxTry(the) {
			step->flag |= XS_DONT_SET_FLAG;
			if (!done->value.boolean) {
				value->kind = XS_UNDEFINED_KIND;
				done->value.boolean = 1;
				(*gxIteratorSteps[step->value.integer])(the, iterator, next, extra, value, XS_RETURN_STATUS);
			}
			mxResult->kind = result->kind;
			mxResult->value = result->value;
			step->flag &= ~XS_DONT_SET_FLAG;
		}
		mxCatch(the) {
			step->flag &= ~XS_DONT_SET_FLAG;
			fxJump(the);
		}
	}
}

void fx_IteratorWrapper_prototype_next(txMachine* the)
{
	txSlot* instance = fxCheckIteratorInstance(the, mxThis, mxID(_Iterator));
	txSlot* result = instance->next;
	txSlot* iterator = result->next;
	txSlot* step = iterator->next;
	txSlot* next = step->next;
	mxPushSlot(iterator);
	mxPushSlot(next);
	mxCall();
	mxRunCount(0);
	mxPullSlot(mxResult);
}

void fx_IteratorWrapper_prototype_return(txMachine* the)
{
	txSlot* instance = fxCheckIteratorInstance(the, mxThis, mxID(_Iterator));
	txSlot* result = instance->next;
	txSlot* iterator = result->next;
	txSlot* value = fxCheckIteratorResult(the, result);
	txSlot* done = value->next;
	mxPushSlot(iterator);
	mxDub();
	mxGetID(mxID(_return));
	if (mxIsUndefined(the->stack) || mxIsNull(the->stack)) {
		mxPop();
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
		mxResult->kind = result->kind;
		mxResult->value = result->value;
	}
	else {
		mxCall();
		mxRunCount(0);
		mxPullSlot(mxResult);
	}
}

txSlot* fxCheckGeneratorInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		slot = instance->next;
		if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_STACK_KIND))
			return instance;
	}
	mxTypeError("this: not a Generator instance");
	return C_NULL;
}

txSlot* fxNewGeneratorInstance(txMachine* the)
{
	txSlot* prototype;
	txSlot* instance;
	txSlot* property;

	prototype = (the->stack->kind == XS_REFERENCE_KIND) ? the->stack->value.reference : mxGeneratorPrototype.value.reference;

	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = prototype;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;

	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_STACK_KIND;
	property->ID = XS_NO_ID;
    property->value.stack.length = 0;
    property->value.stack.address = C_NULL;
	
    property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_INTEGER_KIND;
	property->value.integer = XS_CODE_START_GENERATOR;
	
	return instance;
}

void fxNewGeneratorResult(txMachine* the, txBoolean done)
{
	txSlot* value = the->stack;
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextSlotProperty(the, slot, value, mxID(_value), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextBooleanProperty(the, slot, done, mxID(_done), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	mxPullSlot(value);
}

void fx_Generator(txMachine* the)
{
	if (mxHasTarget)
		mxTypeError("new: Generator");
}

void fx_Generator_prototype_aux(txMachine* the, txFlag status)
{
	txSlot* generator = fxCheckGeneratorInstance(the, mxThis);
	txSlot* stack = generator->next;
	txSlot* state = stack->next;

	if (state->value.integer == XS_NO_CODE)
		mxTypeError("generator is running");
	if ((state->value.integer == XS_CODE_START_GENERATOR) && (status != XS_NO_STATUS))
		state->value.integer = XS_CODE_END;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	if (state->value.integer == XS_CODE_END) {
		if (status == XS_THROW_STATUS) {
			mxException.kind = the->stack->kind;
			mxException.value = the->stack->value;
			fxJump(the);
		}
		if (status == XS_NO_STATUS)
			the->stack->kind = XS_UNDEFINED_KIND;
		fxNewGeneratorResult(the, 1);
		mxPullSlot(mxResult);
	}
	else {
		mxTry(the) {
			the->scratch.kind = the->stack->kind;
			the->scratch.value = the->stack->value;
			the->status = status;
			state->value.integer = XS_NO_CODE;
			fxRunID(the, generator, XS_NO_ID);
			if (state->value.integer == XS_NO_CODE) {
				state->value.integer = XS_CODE_END;
				fxNewGeneratorResult(the, 1);
			}
			mxPullSlot(mxResult);
		}
		mxCatch(the) {
			state->value.integer = XS_CODE_END;
			fxJump(the);
		}
	}
}

void fx_Generator_prototype_next(txMachine* the)
{
	fx_Generator_prototype_aux(the, XS_NO_STATUS);
}

void fx_Generator_prototype_return(txMachine* the)
{
	fx_Generator_prototype_aux(the, XS_RETURN_STATUS);
}

void fx_Generator_prototype_throw(txMachine* the)
{
	fx_Generator_prototype_aux(the, XS_THROW_STATUS);
}

txSlot* fxNewGeneratorFunctionInstance(txMachine* the, txID name)
{
	txSlot* instance;
	txSlot* property;

	instance = fxNewFunctionInstance(the, name);
	property = fxLastProperty(the, instance);
	mxPush(mxGeneratorPrototype);
	fxNewObjectInstance(the);
	fxNextSlotProperty(the, property, the->stack, mxID(_prototype), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	mxPop();
	
	return instance;
}

void fx_GeneratorFunction(txMachine* the)
{	
	txInteger c, i;
	txStringStream stream;
	txSlot* module = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.module;
	if (!module) module = mxProgram.value.reference;
	
	c = mxArgc;
	i = 0;
	mxPushStringX("(function* anonymous(");
	while (c > 1) {
		fxToString(the, mxArgv(i));
		fxConcatString(the, the->stack, mxArgv(i));
		if (c > 2)
			fxConcatStringC(the, the->stack, ", ");
		c--;
		i++;
	}
	fxConcatStringC(the, the->stack, "\n){");
	if (c > 0) {
		fxToString(the, mxArgv(i));
		fxConcatString(the, the->stack, mxArgv(i));
	}
	fxConcatStringC(the, the->stack, "\n})");
	stream.slot = the->stack;
	stream.offset = 0;
	stream.size = mxStringLength(the->stack->value.string);
	fxRunScript(the, fxParseScript(the, &stream, fxStringGetter, mxProgramFlag | mxGeneratorFlag), C_NULL, C_NULL, C_NULL, C_NULL, module);
	mxPullSlot(mxResult);
	if (mxHasTarget && !fxIsSameSlot(the, mxTarget, mxFunction)) {
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxGeneratorFunctionPrototype);
		mxResult->value.reference->value.instance.prototype = the->stack->value.reference;
		mxPop();
	}
}

void fx_Enumerator(txMachine* the)
{
	txSlot* iterator;
	txSlot* result;
	txSlot* slot;
	txSlot* keys;
	txSlot* visited;
	
	mxPush(mxEnumeratorFunction);
	mxGetID(mxID(_prototype));
	iterator = fxNewObjectInstance(the);
	mxPullSlot(mxResult);
	mxPush(mxObjectPrototype);
	result = fxNewObjectInstance(the);
	slot = fxNextUndefinedProperty(the, result, mxID(_value), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextBooleanProperty(the, slot, 0, mxID(_done), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextSlotProperty(the, iterator, the->stack, mxID(_result), XS_GET_ONLY);
	mxPop();
	
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_GET_ONLY;
	slot->ID = mxID(_iterable);
	slot->kind = mxThis->kind;
	slot->value = mxThis->value;
	if (mxIsUndefined(slot) || mxIsNull(slot))
		return;
	fxToInstance(the, slot);
		
	keys = fxNewInstance(the);
	mxBehaviorOwnKeys(the, slot->value.reference, XS_EACH_NAME_FLAG, keys);
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_GET_ONLY;
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = keys;
	mxPop();
	
	visited = fxNewInstance(the);
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_GET_ONLY;
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = visited;
	mxPop();
}

void fx_Enumerator_prototype_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* at = C_NULL;
	if (mxIsReference(iterable)) {
		txSlot* instance = iterable->value.reference;
		txSlot* keys = iterable->next;
		txSlot* visited = keys->next;
		txSlot* slot;
		txSlot* former;
	
		keys = keys->value.reference;
		visited = visited->value.reference;
	
		mxPushUndefined();
		slot = the->stack;
	again:	
		while ((at = keys->next)) {
			if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, slot)) {
				txSlot** address = &(visited->next);
				while ((former = *address)) {
					if ((at->value.at.id == former->value.at.id) && (at->value.at.index == former->value.at.index))
						break;
					address = &(former->next);
				}
				if (!former) {
					*address = at;
					keys->next = at->next;
					at->next = NULL;
					if (!(slot->flag & XS_DONT_ENUM_FLAG)) {
						fxKeyAt(the, at->value.at.id, at->value.at.index, result->value.reference->next);
						break;
					}
				}
				else
					keys->next = at->next;
			}
			else
				keys->next = at->next;
		}
		if (!at) {
			if (mxBehaviorGetPrototype(the, instance, slot)) {
				iterable->value.reference = instance = slot->value.reference;
				mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG, keys);
				goto again;
			}
		}
		mxPop();
	}
	if (!at) {
		result->value.reference->next->kind = XS_UNDEFINED_KIND;
		result->value.reference->next->next->value.boolean = 1;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}

void fx_AsyncIterator_prototype_asyncDispose(txMachine* the)
{	
	mxTry(the) {
		mxPushUndefined();
		mxPush(mxPromiseConstructor);
		mxPushSlot(mxThis);
		mxDub();
		mxGetID(mxID(_return));
		if (mxIsUndefined(the->stack)) {
			mxPop();
			the->stack->kind = XS_UNDEFINED_KIND;
		}
		else {
			mxCall();
			mxRunCount(0);		
		}
		fx_Promise_resolveAux(the);
		mxPop();
		mxPop();
		mxPullSlot(mxResult);
	}
	mxCatch(the) {
		txSlot* resolveFunction;
		txSlot* rejectFunction;
		mxTemporary(resolveFunction);
		mxTemporary(rejectFunction);
		mxPush(mxPromiseConstructor);
		fxNewPromiseCapability(the, resolveFunction, rejectFunction);
		mxPullSlot(mxResult);
		fxRejectException(the, rejectFunction);
	}
}

void fx_AsyncIterator_prototype_asyncIterator(txMachine* the)
{
	*mxResult = *mxThis;
}

void fxAsyncGeneratorRejectAwait(txMachine* the)
{
	txSlot* slot = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* generator = slot->value.home.object;
	the->scratch.kind = mxArgv(0)->kind;
	the->scratch.value = mxArgv(0)->value;
	fxAsyncGeneratorStep(the, generator, XS_THROW_STATUS);
}

void fxAsyncGeneratorRejectYield(txMachine* the)
{
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* generator = home->value.home.object;
	txSlot* stack = generator->next;
	txSlot* state = stack->next;
	if (state->value.integer == XS_CODE_END) {
		txSlot* queue = state->next;
		txSlot* current = queue->value.list.first;
		txSlot* resolveFunction = current->value.reference->next;
		txSlot* rejectFunction = resolveFunction->next;

		mxPushUndefined();
		mxPushSlot(rejectFunction);
		mxCall();
		mxPushSlot(mxArgv(0));
	
		queue->value.list.first = current = current->next;
		if (current == C_NULL)
			queue->value.list.last = C_NULL;
	
		mxRunCount(1);
		mxPop();
	
		if (current) {
			txSlot* resolveFunction = current->value.reference->next;
			txSlot* rejectFunction = resolveFunction->next;
			txSlot* status = rejectFunction->next;
			txSlot* value = status->next;
			the->scratch.kind = value->kind;
			the->scratch.value = value->value;
			fxAsyncGeneratorStep(the, generator, status->value.integer);
		}
	}
	else {
		mxPushSlot(mxArgv(0));
		mxPull(the->scratch);
		fxAsyncGeneratorStep(the, generator, XS_THROW_STATUS);
	}
}

void fxAsyncGeneratorResolveAwait(txMachine* the)
{
	txSlot* slot = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* generator = slot->value.home.object;
	the->scratch.kind = mxArgv(0)->kind;
	the->scratch.value = mxArgv(0)->value;
	fxAsyncGeneratorStep(the, generator, XS_NO_STATUS);
}

void fxAsyncGeneratorResolveYield(txMachine* the)
{
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* generator = home->value.home.object;
	txSlot* stack = generator->next;
	txSlot* state = stack->next;
	txSlot* queue = state->next;
	txSlot* current = queue->value.list.first;
	if (current) {
		txSlot* resolveFunction = current->value.reference->next;
	
		mxPushUndefined();
		mxPushSlot(resolveFunction);
		mxCall();
		mxPushSlot(mxArgv(0));
		fxNewGeneratorResult(the, (state->value.integer == XS_CODE_END) ? 1 : 0);
	
		queue->value.list.first = current = current->next;
		if (current == C_NULL)
			queue->value.list.last = C_NULL;
	
		mxRunCount(1);
		mxPop();
	}
	
	if (current) {
		txSlot* resolveFunction = current->value.reference->next;
		txSlot* rejectFunction = resolveFunction->next;
		txSlot* status = rejectFunction->next;
		txSlot* value = status->next;
		the->scratch.kind = value->kind;
		the->scratch.value = value->value;
		fxAsyncGeneratorStep(the, generator, status->value.integer);
	}
}

void fxAsyncGeneratorStep(txMachine* the, txSlot* generator, txFlag status)
{
	txSlot* stack = generator->next;
	txSlot* state = stack->next;
	txSlot* queue = state->next;
	txSlot* resolveAwaitFunction = queue->next;
	txSlot* rejectAwaitFunction = resolveAwaitFunction->next;
	txSlot* resolveYieldFunction = rejectAwaitFunction->next;
	txSlot* rejectYieldFunction = resolveYieldFunction->next;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
	txSlot* value;
	
	mxTry(the) {
		if (state->value.integer == XS_CODE_END) {
			mxPush(the->scratch);
			value = the->stack;
			mxTemporary(resolveFunction);
			mxTemporary(rejectFunction);
			mxPush(mxPromiseConstructor);
			fxNewPromiseCapability(the, resolveFunction, rejectFunction);
            fxPromiseThen(the, the->stack->value.reference, resolveYieldFunction, rejectYieldFunction, C_NULL, C_NULL);
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
			if (status == XS_THROW_STATUS)
				mxPushSlot(rejectFunction);
			else
				mxPushSlot(resolveFunction);
			mxCall();
			/* ARGUMENTS */
			mxPushSlot(value);
			mxRunCount(1);
			mxPop();
		}
		else {
			the->status = status;
			status = XS_NO_STATUS;
			state->value.integer = XS_NO_CODE;
			fxRunID(the, generator, XS_NO_ID);
			if (state->value.integer == XS_NO_CODE)
				state->value.integer = XS_CODE_END;
			value = the->stack;
			if (state->value.integer == XS_CODE_END) {
				txSlot* current = queue->value.list.first;
				if (current) {
					if (value->kind == XS_UNINITIALIZED_KIND)
						value->kind = XS_UNDEFINED_KIND;
					mxPushUndefined();
					if (status == XS_THROW_STATUS)
						mxPushSlot(rejectYieldFunction);
					else
						mxPushSlot(resolveYieldFunction);
					mxCall();
					mxPushSlot(value);
					mxRunCount(1);
					mxPop();
				}
			}
			else if (state->value.integer == XS_CODE_AWAIT) {
				mxPushUndefined();
				mxPush(mxPromiseConstructor);
				mxPushSlot(value);
				fx_Promise_resolveAux(the);
				mxPop();
				mxPop();
				fxPromiseThen(the, the->stack->value.reference, resolveAwaitFunction, rejectAwaitFunction, C_NULL, C_NULL);
			}
			else if (state->value.integer == XS_CODE_YIELD) {
				mxPushUndefined();
				mxPush(mxPromiseConstructor);
				mxPushSlot(value);
				fx_Promise_resolveAux(the);
				mxPop();
				mxPop();
				fxPromiseThen(the, the->stack->value.reference, resolveYieldFunction, rejectYieldFunction, C_NULL, C_NULL);
			}
			else if (state->value.integer == XS_CODE_YIELD_STAR) {
				txSlot* current = queue->value.list.first;
				if (current) {
					mxPushUndefined();
					mxPushSlot(resolveYieldFunction);
					mxCall();
					mxPushSlot(value);
					mxRunCount(1);
					mxPop();
				}
			}
		}
		mxPop();
	}
	mxCatch(the) {
		mxTemporary(resolveFunction);
		mxTemporary(rejectFunction);
		mxPush(mxPromiseConstructor);
		fxNewPromiseCapability(the, resolveFunction, rejectFunction);
        if (state->value.integer == XS_CODE_AWAIT) {
            fxPromiseThen(the, the->stack->value.reference, resolveAwaitFunction, rejectAwaitFunction, C_NULL, C_NULL);
        }
        else {
			state->value.integer = XS_CODE_END;
            fxPromiseThen(the, the->stack->value.reference, resolveYieldFunction, rejectYieldFunction, C_NULL, C_NULL);
        }
        fxRejectException(the, rejectFunction);
	}
}

txSlot* fxCheckAsyncGeneratorInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		slot = instance->next;
		if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_STACK_KIND)) {
			slot = slot->next;
			if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_INTEGER_KIND)) {
				slot = slot->next;
				if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_LIST_KIND)) {
					return instance;
				}
			}
		}
	}
	mxTypeError("this: not an AsyncGenerator instance");
	return C_NULL;
}

txSlot* fxNewAsyncGeneratorInstance(txMachine* the)
{
	txSlot* prototype;
	txSlot* instance;
	txSlot* property;
	txSlot* function;
	txSlot* home;

	prototype = (the->stack->kind == XS_REFERENCE_KIND) ? the->stack->value.reference : mxAsyncGeneratorPrototype.value.reference;

	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = prototype;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;

	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_STACK_KIND;
	property->ID = XS_NO_ID;
    property->value.stack.length = 0;
    property->value.stack.address = C_NULL;
	
    property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_INTEGER_KIND;
	property->value.integer = XS_CODE_START_ASYNC_GENERATOR;
	
    property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_LIST_KIND;
	property->value.list.first = C_NULL;
	property->value.list.last = C_NULL;
	
	function = fxNewHostFunction(the, fxAsyncGeneratorResolveAwait, 1, XS_NO_ID, mxAsyncGeneratorResolveAwaitProfileID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
	
	function = fxNewHostFunction(the, fxAsyncGeneratorRejectAwait, 1, XS_NO_ID, mxAsyncGeneratorRejectAwaitProfileID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
	
	function = fxNewHostFunction(the, fxAsyncGeneratorResolveYield, 1, XS_NO_ID, mxAsyncGeneratorResolveYieldProfileID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
	
	function = fxNewHostFunction(the, fxAsyncGeneratorRejectYield, 1, XS_NO_ID, mxAsyncGeneratorRejectYieldProfileID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
	
	return instance;
}


void fx_AsyncGenerator(txMachine* the)
{
	if (mxHasTarget)
		mxTypeError("new: AsyncGenerator");
}

void fx_AsyncGenerator_prototype_aux(txMachine* the, txFlag status)
{
	txSlot* stack = the->stack;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
	txSlot* slot;
	txSlot* generator;
	txSlot* state;
	txSlot* queue;
	txSlot* instance;
	txSlot* property;
	
	mxTemporary(resolveFunction);
	mxTemporary(rejectFunction);
	mxPush(mxPromiseConstructor);
	fxNewPromiseCapability(the, resolveFunction, rejectFunction);
	mxPullSlot(mxResult);
#ifdef mxPromisePrint
	fprintf(stderr, "fx_AsyncGenerator_prototype_aux %d\n", mxResult->value.reference->next->ID);
#endif
	{
		mxTry(the) {
			generator = fxCheckAsyncGeneratorInstance(the, mxThis);
			state = generator->next->next;
			if (((status == XS_RETURN_STATUS) || (status == XS_THROW_STATUS)) && (state->value.integer == XS_CODE_START_ASYNC_GENERATOR))
				state->value.integer = XS_CODE_END;
			instance = property = fxNewInstance(the);
			property = fxNextSlotProperty(the, property, resolveFunction, XS_NO_ID, XS_INTERNAL_FLAG);
			property = fxNextSlotProperty(the, property, rejectFunction, XS_NO_ID, XS_INTERNAL_FLAG);
			property = fxNextIntegerProperty(the, property, status, XS_NO_ID, XS_INTERNAL_FLAG);
			if (mxArgc > 0)
				property = fxNextSlotProperty(the, property, mxArgv(0), XS_NO_ID, XS_INTERNAL_FLAG);
			else
				property = fxNextUndefinedProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG);
			slot = fxNewSlot(the);
			slot->kind = XS_REFERENCE_KIND;
			slot->value.reference = instance;
			queue = state->next;
			if (queue->value.list.first) {
				queue->value.list.last->next = slot;
				queue->value.list.last = slot;
			}
			else {
				queue->value.list.first = slot;
				queue->value.list.last = slot;
				if (state->value.integer != XS_CODE_AWAIT) {
					the->scratch.kind = property->kind;
					the->scratch.value = property->value;
					fxAsyncGeneratorStep(the, generator, status);
				}
			}
		}
		mxCatch(the) {
			fxRejectException(the, rejectFunction);
		}
	}
	the->stack = stack;
}

void fx_AsyncGenerator_prototype_next(txMachine* the)
{
	fx_AsyncGenerator_prototype_aux(the, XS_NO_STATUS);
}

void fx_AsyncGenerator_prototype_return(txMachine* the)
{
	fx_AsyncGenerator_prototype_aux(the, XS_RETURN_STATUS);
}

void fx_AsyncGenerator_prototype_throw(txMachine* the)
{
	fx_AsyncGenerator_prototype_aux(the, XS_THROW_STATUS);
}

txSlot* fxNewAsyncGeneratorFunctionInstance(txMachine* the, txID name)
{
	txSlot* instance;
	txSlot* property;

	instance = fxNewFunctionInstance(the, name);
	property = fxLastProperty(the, instance);
	mxPush(mxAsyncGeneratorPrototype);
	fxNewObjectInstance(the);
	fxNextSlotProperty(the, property, the->stack, mxID(_prototype), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	mxPop();
	
	return instance;
}

void fx_AsyncGeneratorFunction(txMachine* the)
{	
	txInteger c, i;
	txStringStream stream;
	txSlot* module = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.module;
	if (!module) module = mxProgram.value.reference;
	
	c = mxArgc;
	i = 0;
	mxPushStringX("(async function* anonymous(");
	while (c > 1) {
		fxToString(the, mxArgv(i));
		fxConcatString(the, the->stack, mxArgv(i));
		if (c > 2)
			fxConcatStringC(the, the->stack, ", ");
		c--;
		i++;
	}
	fxConcatStringC(the, the->stack, "\n){");
	if (c > 0) {
		fxToString(the, mxArgv(i));
		fxConcatString(the, the->stack, mxArgv(i));
	}
	fxConcatStringC(the, the->stack, "\n})");
	stream.slot = the->stack;
	stream.offset = 0;
	stream.size = mxStringLength(the->stack->value.string);
	fxRunScript(the, fxParseScript(the, &stream, fxStringGetter, mxProgramFlag | mxGeneratorFlag), C_NULL, C_NULL, C_NULL, C_NULL, module);
	mxPullSlot(mxResult);
	if (mxHasTarget && !fxIsSameSlot(the, mxTarget, mxFunction)) {
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxAsyncGeneratorFunctionPrototype);
		mxResult->value.reference->value.instance.prototype = the->stack->value.reference;
		mxPop();
	}
}

void fxAsyncFromSyncIteratorDone(txMachine* the)
{
	txSlot* slot = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* instance = slot->value.home.object;
	txSlot* iterator = instance->next;
	txSlot* nextFunction = iterator->next;
	txSlot* doneFunction = nextFunction->next;
	txSlot* doneFlag = doneFunction->next;
	mxPushSlot(mxArgv(0));
	fxNewGeneratorResult(the, doneFlag->value.boolean);
	mxPullSlot(mxResult);
}

void fxAsyncFromSyncIteratorFailed(txMachine* the)
{
	txSlot* slot = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* instance = slot->value.home.object;
	txSlot* iterator = instance->next;
	mxTry(the) {
		mxPushSlot(iterator);
		mxPushSlot(iterator);
		mxGetID(mxID(_return));
		if (!mxIsUndefined(the->stack) && !mxIsNull(the->stack)) {
			mxCall();
			mxRunCount(0);
		}
		mxPop();
	}
	mxCatch(the) {
	}
	mxException.kind = mxArgv(0)->kind;
	mxException.value = mxArgv(0)->value;
	fxThrow(the, NULL, 0);
}

txSlot* fxCheckAsyncFromSyncIteratorInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		slot = instance->next;
		if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->ID == mxID(_iterator)))
			return instance;
	}
	mxTypeError("this: not an AsyncFromSyncIterator instance");
	return C_NULL;
}

txSlot* fxNewAsyncFromSyncIteratorInstance(txMachine* the)
{
	txSlot* iterator = the->stack;
	txSlot* instance;
	txSlot* slot;
	txSlot* function;
	txSlot* home;
	mxPush(mxAsyncFromSyncIteratorPrototype);
	instance = fxNewObjectInstance(the);
	slot = fxLastProperty(the, instance);
	
	slot = fxNextSlotProperty(the, slot, iterator, mxID(_iterator), XS_INTERNAL_FLAG);

	mxPushSlot(iterator);
	mxGetID(mxID(_next));
	slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
	
	function = fxNewHostFunction(the, fxAsyncFromSyncIteratorDone, 1, XS_NO_ID, mxAsyncFromSyncIteratorDoneProfileID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
	
    slot = fxNextBooleanProperty(the, slot, 0, XS_NO_ID, XS_INTERNAL_FLAG);
    
	function = fxNewHostFunction(the, fxAsyncFromSyncIteratorFailed, 1, XS_NO_ID, mxAsyncFromSyncIteratorDoneProfileID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();

	mxPullSlot(iterator);
	return instance;
}

void fx_AsyncFromSyncIterator_prototype_aux(txMachine* the, txFlag status, txSlot* iterator, txBoolean* flag)
{
	txSlot* stack = the->stack;
	txSlot* resolveFunction;
    txSlot* rejectFunction;
	txSlot* slot;
	txSlot* stepFunction = C_NULL;
	
	mxTemporary(resolveFunction);
	mxTemporary(rejectFunction);
	mxPush(mxPromiseConstructor);
	fxNewPromiseCapability(the, resolveFunction, rejectFunction);
	mxPullSlot(mxResult);
#ifdef mxPromisePrint
	fprintf(stderr, "fx_AsyncFromSyncIterator_prototype_aux %d %d\n", mxResult->value.reference->next->ID, status);
#endif
    {
		mxTry(the) {
			if (status == XS_NO_STATUS) {
				stepFunction = iterator->next;
			}
			else if (status == XS_RETURN_STATUS) {
				mxPushSlot(iterator);
				mxGetID(mxID(_return));
				if (mxIsUndefined(the->stack) || mxIsNull(the->stack)) {
					mxPushUndefined();
					mxPushSlot(resolveFunction);
					mxCall();
					mxPushUndefined();
					fxNewGeneratorResult(the, 1);
					mxRunCount(1);
				}
				else
					stepFunction = the->stack;
			}
			else {
				mxPushSlot(iterator);
				mxGetID(mxID(_throw));
				if (mxIsUndefined(the->stack) || mxIsNull(the->stack)) {
					*flag = 0;
					fxIteratorReturn(the, iterator, 0);
					mxTypeError("no throw");
				}
				else
					stepFunction = the->stack;
			}
			if (stepFunction) {
				txSlot* doneFunction = iterator->next->next;
				txSlot* doneFlag = doneFunction->next;
				txSlot* failedFunction = doneFlag->next;
				mxPushSlot(iterator);
				mxPushSlot(stepFunction);
				mxCall();
				if (mxArgc == 0)
					mxRunCount(0);
				else {
					mxPushSlot(mxArgv(0));
					mxRunCount(1);
				}
				slot = the->stack;
                if (!mxIsReference(slot)) {
                    mxTypeError("iterator result: not an object");
                }

				mxPushSlot(slot);
				mxGetID(mxID(_done));
				doneFlag->value.boolean = fxToBoolean(the, the->stack);

				mxPushUndefined();
				mxPush(mxPromiseConstructor);
				mxPushSlot(slot);
				mxGetID(mxID(_value));
				fx_Promise_resolveAux(the);
				mxPop();
				mxPop();
				fxPromiseThen(the, the->stack->value.reference, doneFunction, failedFunction, resolveFunction, rejectFunction);
			}
		}
		mxCatch(the) {
			if (*flag)
				fxIteratorReturn(the, iterator, 1);
			fxRejectException(the, rejectFunction);
		}
    }
	the->stack = stack;
}

void fx_AsyncFromSyncIterator_prototype_next(txMachine* the)
{
	txBoolean flag = 1;
	txSlot* instance = fxCheckAsyncFromSyncIteratorInstance(the, mxThis);
	fx_AsyncFromSyncIterator_prototype_aux(the, XS_NO_STATUS, instance->next, &flag);
}

void fx_AsyncFromSyncIterator_prototype_return(txMachine* the)
{
	txBoolean flag = 1;
	txSlot* instance = fxCheckAsyncFromSyncIteratorInstance(the, mxThis);
	fx_AsyncFromSyncIterator_prototype_aux(the, XS_RETURN_STATUS, instance->next, &flag);
}

void fx_AsyncFromSyncIterator_prototype_throw(txMachine* the)
{
	txBoolean flag = 1;
	txSlot* instance = fxCheckAsyncFromSyncIteratorInstance(the, mxThis);
	fx_AsyncFromSyncIterator_prototype_aux(the, XS_THROW_STATUS, instance->next, &flag);
}



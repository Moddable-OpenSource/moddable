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

static void fxAddUnhandledRejection(txMachine* the, txSlot* promise);
static void fxCombinePromises(txMachine* the, txInteger which);
static txSlot* fxNewCombinePromisesFunction(txMachine* the, txInteger which, txSlot* already, txSlot* object);

enum {
	XS_PROMISE_COMBINE_NONE = 0,
	XS_PROMISE_COMBINE_FULFILLED = 1,
	XS_PROMISE_COMBINE_REJECTED = 2,
	XS_PROMISE_COMBINE_SETTLED = 4,
};

void fxBuildPromise(txMachine* the)
{
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Promise_prototype_catch), 1, mxID(_catch), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Promise_prototype_finally), 1, mxID(_finally_), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Promise_prototype_then), 2, mxID(_then), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Promise", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPromisePrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_Promise), 1, mxID(_Promise));
	mxPromiseConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Promise_all), 1, mxID(_all), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Promise_allSettled), 1, mxID(_allSettled), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Promise_any), 1, mxID(_any), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Promise_race), 1, mxID(_race), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Promise_reject), 1, mxID(_reject), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Promise_resolve), 1, mxID(_resolve), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_species_get), C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	mxPop();
	fxNewHostFunction(the, mxCallback(fxOnRejectedPromise), 1, XS_NO_ID, XS_NO_ID);
	mxOnRejectedPromiseFunction = *the->stack;
	mxPop();
	fxNewHostFunction(the, mxCallback(fxOnResolvedPromise), 1, XS_NO_ID, XS_NO_ID);
	mxOnResolvedPromiseFunction = *the->stack;
	mxPop();
	fxNewHostFunction(the, mxCallback(fxOnThenable), 1, XS_NO_ID, XS_NO_ID);
	mxOnThenableFunction = *the->stack;
	mxPop();
}

txSlot* fxNewPromiseInstance(txMachine* the)
{
#ifdef mxPromisePrint
	static txID gID = 0;
#endif
	txSlot* promise;
	txSlot* slot;
	txSlot* instance;
	promise = fxNewSlot(the);
	promise->kind = XS_INSTANCE_KIND;
	promise->value.instance.garbage = C_NULL;
	promise->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = promise;
	/* STATUS */
	slot = promise->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG;
#ifdef mxPromisePrint
	slot->ID = gID++;
#endif
	slot->kind = XS_PROMISE_KIND;
	slot->value.integer = mxUndefinedStatus;
	/* THENS */
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG;
	slot->value.reference = instance = fxNewSlot(the);
    slot->kind = XS_REFERENCE_KIND;
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = C_NULL;
	/* RESULT */
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG;
#ifdef mxDebug
	/* ENVIRONMENT */
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG;
	instance = the->frame;
	while (instance) {
		txSlot* environment = mxFrameToEnvironment(instance);
		if (environment->ID != XS_NO_ID) {
			slot->ID = environment->ID;
			slot->value.environment.line = environment->value.environment.line;
			break;
		}
		instance = instance->next;
	}
#endif
	return promise;
}

txSlot* fxNewPromiseCapability(txMachine* the, txSlot* resolveFunction, txSlot* rejectFunction)
{
	txSlot* capability;
	txSlot* slot;
	txSlot* function;
	mxNew();
	resolveFunction->value.reference = fxNewHostFunction(the, fxNewPromiseCapabilityCallback, 2, XS_NO_ID, mxNewPromiseCapabilityCallbackProfileID);
	resolveFunction->kind = XS_REFERENCE_KIND;
    mxRunCount(1);
    capability = resolveFunction->value.reference;
	resolveFunction->kind = XS_UNDEFINED_KIND;
	slot = mxFunctionInstanceHome(capability)->value.home.object;
	if (!slot)
		mxTypeError("executor not called");
	slot = slot->next;
	if (!mxIsReference(slot))
		mxTypeError("resolve is no object");
	function = slot->value.reference;	
	if (!mxIsFunction(function))
		mxTypeError("resolve is no function");
	resolveFunction->kind = XS_REFERENCE_KIND;
	resolveFunction->value.reference = function;
	slot = slot->next;
	if (!mxIsReference(slot))
		mxTypeError("reject is no object");
	function = slot->value.reference;	
	if (!mxIsFunction(function))
		mxTypeError("reject is no function");
	rejectFunction->kind = XS_REFERENCE_KIND;
	rejectFunction->value.reference = function;
	return the->stack->value.reference;
}

void fxNewPromiseCapabilityCallback(txMachine* the)
{
	txSlot* slot = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* object = slot->value.home.object;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
	if (object) {
		resolveFunction = object->next;
		rejectFunction = resolveFunction->next;
		if (!mxIsUndefined(resolveFunction) || !mxIsUndefined(rejectFunction))
			mxTypeError("executor already called");
	}
	else {
		object = fxNewInstance(the);
		resolveFunction = object->next = fxNewSlot(the);
		rejectFunction = resolveFunction->next = fxNewSlot(the);
		slot->value.home.object = object;
        mxPop();
	}
	if (mxArgc > 0) {
		resolveFunction->kind = mxArgv(0)->kind;
		resolveFunction->value = mxArgv(0)->value;
	}
	if (mxArgc > 1) {
		rejectFunction->kind = mxArgv(1)->kind;
		rejectFunction->value = mxArgv(1)->value;
	}
}

void fxAddUnhandledRejection(txMachine* the, txSlot* promise)
{
	txSlot* reason = mxPromiseResult(promise);
	txSlot* list = &mxUnhandledPromises;
	txSlot** address = &list->value.reference->next;
	txSlot* slot;
	while ((slot = *address)) {
		if (slot->value.weakRef.target == promise)
			break;
		slot = slot->next;
		address = &slot->next;
	}
	if (!slot) {
#ifdef mxPromisePrint
		fprintf(stderr, "fxAddUnhandledRejection %d\n", promise->next->ID);
#endif
 		slot = *address = fxNewSlot(the);
		slot->kind = XS_WEAK_REF_KIND;
		slot->value.weakRef.target = promise;
 		slot = slot->next = fxNewSlot(the);
		slot->kind = reason->kind;
		slot->value = reason->value;
	}
}

void fxCheckUnhandledRejections(txMachine* the, txBoolean atExit)
{
	txSlot* list = &mxUnhandledPromises;
	txSlot** address = &list->value.reference->next;
	txSlot* slot;
	if (atExit) {
		while ((slot = *address)) {
			slot = slot->next;
			*address = slot->next;
			mxException.value = slot->value;
			mxException.kind = slot->kind;
			fxAbort(the, XS_UNHANDLED_REJECTION_EXIT);
		}
	}
	else {
		while ((slot = *address)) {
			if (slot->value.weakRef.target == C_NULL) {
				slot = slot->next;
				*address = slot->next;
				mxException.value = slot->value;
				mxException.kind = slot->kind;
				fxAbort(the, XS_UNHANDLED_REJECTION_EXIT);
			}
			else {
				slot = slot->next;
				address = &slot->next;
			}
		}
	}
}

void fxCombinePromises(txMachine* the, txInteger which)
{
	txSlot* stack = the->stack;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
	txSlot* promise;
	txSlot* object;
	txSlot* property;
	txSlot* array;
	txSlot* already;
	txSlot* iterator;
	txSlot* next;
	txSlot* value;
	txInteger index;
	
	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	mxTemporary(resolveFunction);
	mxTemporary(rejectFunction);
	mxPushSlot(mxThis);
	promise = fxNewPromiseCapability(the, resolveFunction, rejectFunction);
	mxPullSlot(mxResult);
	{
		mxTry(the) {
			txSlot* resolve = C_NULL;
			if (which) {
				object = fxNewInstance(the);
				property = fxNextIntegerProperty(the, object, 0, XS_NO_ID, XS_NO_FLAG);
				property = fxNextReferenceProperty(the, property, promise, XS_NO_ID, XS_NO_FLAG);
				if (which == XS_PROMISE_COMBINE_REJECTED)
					property = fxNextSlotProperty(the, property, rejectFunction, XS_NO_ID, XS_NO_FLAG);
				else
					property = fxNextSlotProperty(the, property, resolveFunction, XS_NO_ID, XS_NO_FLAG);
				mxPush(mxArrayPrototype);
				array = fxNewArrayInstance(the);
				already = array->next;
				property = fxNextReferenceProperty(the, property, array, XS_NO_ID, XS_NO_FLAG);
				mxPop();
			}
			mxPushSlot(mxThis);
			mxGetID(mxID(_resolve));	
			resolve = the->stack;
			if (!fxIsCallable(the, resolve))
				mxTypeError("resolve is no function");
			mxTemporary(iterator);
			mxTemporary(next);
			fxGetIterator(the, mxArgv(0), iterator, next, 0);
			index = 0;
			mxTemporary(value);
			while (fxIteratorNext(the, iterator, next, value)) {
				mxTry(the) {
					mxPushSlot(mxThis);
					mxPushSlot(resolve);
					mxCall();
					mxPushSlot(value);
					mxRunCount(1);
					mxDub();
					mxGetID(mxID(_then));
					mxCall();
					if (which) {
						already = already->next = fxNewSlot(the);
						already->kind = XS_UNINITIALIZED_KIND;
						array->next->value.array.length++;
					}
					if (which & XS_PROMISE_COMBINE_SETTLED) {
						fxNewCombinePromisesFunction(the, which | XS_PROMISE_COMBINE_FULFILLED, already, object);
						fxNewCombinePromisesFunction(the, which | XS_PROMISE_COMBINE_REJECTED, already, object);
					}
					else if (which & XS_PROMISE_COMBINE_FULFILLED) {
						fxNewCombinePromisesFunction(the, which, already, object);
						mxPushSlot(rejectFunction);
					}
					else if (which & XS_PROMISE_COMBINE_REJECTED) {
						mxPushSlot(resolveFunction);
						fxNewCombinePromisesFunction(the, which, already, object);
					}
					else {
						mxPushSlot(resolveFunction);
						mxPushSlot(rejectFunction);
					}
					mxRunCount(2);
					mxPop();
					index++;
				}
				mxCatch(the) {
					fxIteratorReturn(the, iterator);
					fxJump(the);
				}
			}
			if (which) {
				property = object->next;
				property->value.integer += index;
				index = property->value.integer;
			}
			if ((index == 0) && (which != XS_PROMISE_COMBINE_NONE)) {
				mxPushUndefined();
				if (which == XS_PROMISE_COMBINE_REJECTED)
					mxPushSlot(rejectFunction);
				else
					mxPushSlot(resolveFunction);
				mxCall();
				if ((which == XS_PROMISE_COMBINE_SETTLED) || (which == XS_PROMISE_COMBINE_FULFILLED)) {
					fxCacheArray(the, array);
					mxPushReference(array);
				}
				else if (which == XS_PROMISE_COMBINE_REJECTED) {
					mxPush(mxAggregateErrorConstructor);
					mxNew();
					fxCacheArray(the, array);
					mxPushReference(array);
					mxRunCount(1);
				}
				else {
					mxPushUndefined();
				}
				mxRunCount(1);
			}
		}
		mxCatch(the) {
			fxRejectException(the, rejectFunction);
		}
	}
	the->stack = stack;
}

void fxCombinePromisesCallback(txMachine* the)
{
	txSlot* slot = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.object->next;
	txInteger which = slot->value.integer;
	txSlot* instance;
	txSlot* property;
	slot = slot->next;
	if (slot->value.closure->kind != XS_UNINITIALIZED_KIND)
		return;
	if (which & XS_PROMISE_COMBINE_SETTLED) {
		mxPush(mxObjectPrototype);
		instance = fxNewObjectInstance(the);
	}
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	if (which & XS_PROMISE_COMBINE_SETTLED) {
		property = fxLastProperty(the, instance);
		if (which & XS_PROMISE_COMBINE_FULFILLED) {
			property = fxNextStringXProperty(the, property, "fulfilled", mxID(_status), XS_NO_FLAG);
			property = fxNextSlotProperty(the, property, the->stack, mxID(_value), XS_NO_FLAG);
		}
		else {
			property = fxNextStringXProperty(the, property, "rejected", mxID(_status), XS_NO_FLAG);
			property = fxNextSlotProperty(the, property, the->stack, mxID(_reason), XS_NO_FLAG);
		}
		mxPop();
	}
	mxPullSlot(slot->value.closure);
	slot = slot->next->value.reference->next;
	slot->value.integer--;
	if (slot->value.integer == 0) {
		/* THIS */
		slot = slot->next;
		mxPushSlot(slot);
		/* FUNCTION */
		slot = slot->next;
		mxPushSlot(slot);
		mxCall();
		/* ARGUMENTS */
		slot = slot->next;
		if (which == XS_PROMISE_COMBINE_REJECTED) {
			mxPush(mxAggregateErrorConstructor);
			mxNew();
		}
		fxCacheArray(the, slot->value.reference);
		mxPushSlot(slot);
		if (which == XS_PROMISE_COMBINE_REJECTED) {
			mxRunCount(1);
		}
		/* COUNT */
		mxRunCount(1);
		mxPullSlot(mxResult);
	}
}

txSlot* fxNewCombinePromisesFunction(txMachine* the, txInteger which, txSlot* already, txSlot* object)
{
	txSlot* result;
	txSlot* instance;
	txSlot* property;
	result = fxNewHostFunction(the, fxCombinePromisesCallback, 1, XS_NO_ID, mxCombinePromisesCallbackProfileID);
	instance = fxNewInstance(the);
	property = fxNextIntegerProperty(the, instance, which, XS_NO_ID, XS_NO_FLAG);
	property = property->next = fxNewSlot(the);
	property->kind = XS_CLOSURE_KIND;
	property->value.closure = already;
	property = fxNextReferenceProperty(the, property, object, XS_NO_ID, XS_NO_FLAG);
	property = mxFunctionInstanceHome(result);
	property->value.home.object = instance;
	mxPop();
	return result;
}

void fxOnRejectedPromise(txMachine* the)
{
	txSlot* reaction = mxThis->value.reference;
	txSlot* resolveFunction = reaction->next;
	txSlot* rejectFunction = resolveFunction->next;
	txSlot* resolveHandler = rejectFunction->next;
	txSlot* rejectHandler = resolveHandler->next;
	txSlot* argument = mxArgv(0);
	txSlot* function = rejectFunction;
	if (rejectHandler->kind == XS_REFERENCE_KIND) {
		mxTry(the) {
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
			mxPushSlot(rejectHandler);
			mxCall();
			/* ARGUMENTS */
			mxPushSlot(argument);
			mxRunCount(1);
			mxPullSlot(argument);
			function = resolveFunction;
		}
		mxCatch(the) {
			*argument = mxException;
			mxException = mxUndefined;
			function = rejectFunction;
		}
	}
    if (function->kind == XS_REFERENCE_KIND) {
		/* THIS */
		mxPushUndefined();
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(argument);
		mxRunCount(1);
		mxPop();
	}
}

void fxOnResolvedPromise(txMachine* the)
{
	txSlot* reaction = mxThis->value.reference;
	txSlot* resolveFunction = reaction->next;
	txSlot* rejectFunction = resolveFunction->next;
	txSlot* resolveHandler = rejectFunction->next;
	txSlot* argument = mxArgv(0);
	txSlot* function = resolveFunction;
	if (resolveHandler->kind == XS_REFERENCE_KIND) {
		mxTry(the) {
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
			mxPushSlot(resolveHandler);
			mxCall();
			/* ARGUMENTS */
			mxPushSlot(argument);
			mxRunCount(1);
			mxPullSlot(argument);
		}
		mxCatch(the) {
			*argument = mxException;
			mxException = mxUndefined;
			function = rejectFunction;
		}
	}
    if (function->kind == XS_REFERENCE_KIND) {
		/* THIS */
		mxPushUndefined();
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(argument);
		mxRunCount(1);
		mxPop();
    }
}

void fxOnThenable(txMachine* the)
{
	txSlot* resolveFunction = mxArgv(0);
	txSlot* rejectFunction = mxArgv(1);
	txSlot* thenFunction = mxArgv(2);
	mxTry(the) {
		/* THIS */
		mxPushSlot(mxThis);
		/* FUNCTION */
		mxPushSlot(thenFunction);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(resolveFunction);
		mxPushSlot(rejectFunction);
		mxRunCount(2);
		mxPop();
	}
	mxCatch(the) {
		fxRejectException(the, rejectFunction);
	}
}

void fxPromiseThen(txMachine* the, txSlot* promise, txSlot* onFullfilled, txSlot* onRejected, txSlot* resolveFunction, txSlot* rejectFunction)
{
	txSlot* reaction;
	txSlot* slot;
	txSlot* status;
	
	reaction = fxNewInstance(the);
	slot = reaction->next = fxNewSlot(the);
	if (resolveFunction) {
		slot->kind = resolveFunction->kind;
		slot->value = resolveFunction->value;
	}
	slot = slot->next = fxNewSlot(the);
	if (rejectFunction) {
		slot->kind = rejectFunction->kind;
		slot->value = rejectFunction->value;
	}
	slot = slot->next = fxNewSlot(the);
	if (onFullfilled) {
		slot->kind = onFullfilled->kind;
		slot->value = onFullfilled->value;
	}
	slot = slot->next = fxNewSlot(the);
	if (onRejected) {
		slot->kind = onRejected->kind;
		slot->value = onRejected->value;
	}
		
	status = mxPromiseStatus(promise);
	if (status->value.integer == mxPendingStatus) {
		txSlot** address = &(mxPromiseThens(promise)->value.reference->next);
		while ((slot = *address)) 
			address = &(slot->next);
		slot = *address = fxNewSlot(the);
		slot->kind = XS_REFERENCE_KIND;
		slot->value.reference = reaction;
	}
	else {
		mxPushReference(reaction);
		if (status->value.integer == mxFulfilledStatus)
			mxPush(mxOnResolvedPromiseFunction);
		else
			mxPush(mxOnRejectedPromiseFunction);
		mxCall();
		slot = mxPromiseResult(promise);
		mxPushSlot(slot);
		fxQueueJob(the, 1, promise);
	}
	mxPop(); // reaction
}

void fxPushPromiseFunctions(txMachine* the, txSlot* promise)
{
	txSlot* resolve;
	txSlot* reject;
	txSlot* object;
	txSlot* slot;
	resolve = fxNewHostFunction(the, fxResolvePromise, 1, XS_NO_ID, mxResolvePromiseProfileID);
	reject = fxNewHostFunction(the, fxRejectPromise, 1, XS_NO_ID, mxRejectPromiseProfileID);
	slot = object = fxNewInstance(the);
	slot = object->next = fxNewSlot(the);
	slot->kind = XS_BOOLEAN_KIND;
	slot->value.boolean = 0;
	slot = slot->next = fxNewSlot(the);
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = promise;
	slot = mxFunctionInstanceHome(resolve);
	slot->value.home.object = object;
	slot = mxFunctionInstanceHome(reject);
	slot->value.home.object = object;
	mxPop();
}

void fxRejectException(txMachine* the, txSlot* rejectFunction)
{
	/* THIS */
	mxPushUndefined();
	/* FUNCTION */
	mxPushSlot(rejectFunction);
	mxCall();
	/* ARGUMENTS */
	mxPush(mxException);
	mxException = mxUndefined;
	mxRunCount(1);
	mxPop();
}

void fxRejectPromise(txMachine* the)
{
	txSlot* slot;
	txSlot* promise;
	txSlot* argument;
	txSlot* result;
	slot = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.object->next;
	if (slot->value.boolean)
		return;
	slot->value.boolean = 1;
	mxPushSlot(slot->next);
	promise = the->stack->value.reference;
	slot->next = C_NULL;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	argument = the->stack;
#ifdef mxPromisePrint
	fprintf(stderr, "fxRejectPromise %d\n", promise->next->ID);
#endif
	result = mxPromiseResult(promise);
	result->kind = argument->kind;
	result->value = argument->value;
	slot = mxPromiseThens(promise)->value.reference->next;
	if (slot) {
		while (slot) {
			mxPushReference(slot->value.reference);
			mxPush(mxOnRejectedPromiseFunction);
			mxCall();
			mxPushSlot(argument);
			fxQueueJob(the, 1, promise);
			slot = slot->next;
		}
		mxPromiseThens(promise)->value.reference->next = C_NULL;
	}
	else {
		fxAddUnhandledRejection(the, promise);
	}
	slot = mxPromiseStatus(promise);
	slot->value.integer = mxRejectedStatus;
}

void fxResolvePromise(txMachine* the)
{
	txSlot* slot;
	txSlot* promise;
	txSlot* argument;
	txSlot* result;
	slot = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.object->next;
	if (slot->value.boolean)
		return;
	slot->value.boolean = 1;
	mxPushSlot(slot->next);
	promise = the->stack->value.reference;
	slot->next = C_NULL;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	argument = the->stack;	
#ifdef mxPromisePrint
	fprintf(stderr, "fxResolvePromise %d\n", promise->next->ID);
#endif
	mxTry(the) {
		if (mxIsReference(argument)) {
			if (argument->value.reference == promise)
				mxTypeError("promise resolves itself");
			mxPushSlot(argument);
			mxGetID(mxID(_then));
			slot = the->stack;
			if (fxIsCallable(the, slot)) {
#ifdef mxPromisePrint
	fprintf(stderr, "fxResolvePromise then %d\n", promise->next->ID);
#endif
				mxPushSlot(argument);
				mxPush(mxOnThenableFunction);
				mxCall();
				fxPushPromiseFunctions(the, promise);
				mxPushSlot(slot);
				fxQueueJob(the, 3, promise);
				goto bail;
			}
			mxPop();
		}
		result = mxPromiseResult(promise);
		result->kind = argument->kind;
		result->value = argument->value;
		slot = mxPromiseThens(promise)->value.reference->next;
		while (slot) {
			mxPushReference(slot->value.reference);
			mxPush(mxOnResolvedPromiseFunction);
			mxCall();
			mxPushSlot(result);
			fxQueueJob(the, 1, promise);
			slot = slot->next;
		}
		mxPromiseThens(promise)->value.reference->next = C_NULL;
		slot = mxPromiseStatus(promise);
		slot->value.integer = mxFulfilledStatus;
	}
bail:
	mxCatch(the) {
		result = mxPromiseResult(promise);
		result->kind = mxException.kind;
		result->value = mxException.value;
		mxException = mxUndefined;
		slot = mxPromiseThens(promise)->value.reference->next;
		if (slot) {
			while (slot) {
				mxPushReference(slot->value.reference);
				mxPush(mxOnRejectedPromiseFunction);
				mxCall();
				mxPushSlot(result);
				fxQueueJob(the, 1, promise);
				slot = slot->next;
			}
			mxPromiseThens(promise)->value.reference->next = C_NULL;
		}
		else {
			fxAddUnhandledRejection(the, promise);
		}
		slot = mxPromiseStatus(promise);
		slot->value.integer = mxRejectedStatus;
	}
}

void fx_Promise(txMachine* the)
{
	txSlot* stack = the->stack;
	txSlot* promise;
	txSlot* argument;
	txSlot* status;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: Promise");
	if (mxArgc < 1)
		mxTypeError("no executor parameter");
	argument = mxArgv(0);
	if (!fxIsCallable(the, argument))
		mxTypeError("executor is no function");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxPromisePrototype);
	promise = fxNewPromiseInstance(the);
#ifdef mxPromisePrint
	fprintf(stderr, "fx_Promise %d\n", promise->next->ID);
#endif
	mxPullSlot(mxResult);
	status = mxPromiseStatus(promise);
	status->value.integer = mxPendingStatus;
	fxPushPromiseFunctions(the, promise);
	resolveFunction = the->stack + 1;
	rejectFunction = the->stack;
	{
		mxTry(the) {
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
			mxPushSlot(argument);
			mxCall();
			/* ARGUMENTS */
			mxPushSlot(resolveFunction);
			mxPushSlot(rejectFunction);
			mxRunCount(2);
		}
		mxCatch(the) {
			fxRejectException(the, rejectFunction);
		}
	}
	the->stack = stack;
}

void fx_Promise_all(txMachine* the)
{
	fxCombinePromises(the, XS_PROMISE_COMBINE_FULFILLED);
}

void fx_Promise_allSettled(txMachine* the)
{
	fxCombinePromises(the, XS_PROMISE_COMBINE_SETTLED);
}

void fx_Promise_any(txMachine* the)
{
	fxCombinePromises(the, XS_PROMISE_COMBINE_REJECTED);
}

void fx_Promise_race(txMachine* the)
{
	fxCombinePromises(the, XS_PROMISE_COMBINE_NONE);
}

void fx_Promise_reject(txMachine* the)
{
	txSlot* resolveFunction;
	txSlot* rejectFunction;

	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	mxTemporary(resolveFunction);
	mxTemporary(rejectFunction);
	mxPushSlot(mxThis);
	fxNewPromiseCapability(the, resolveFunction, rejectFunction);
	mxPullSlot(mxResult);
	/* THIS */
	mxPushUndefined();
	/* FUNCTION */
	mxPushSlot(rejectFunction);
	mxCall();
	/* ARGUMENTS */
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	mxRunCount(1);
	mxPop();
}

void fx_Promise_resolve(txMachine* the)
{
	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	mxPushUndefined();
	mxPushSlot(mxThis);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	fx_Promise_resolveAux(the);		
	mxPop();
	mxPop();
	mxPullSlot(mxResult);
}

void fx_Promise_resolveAux(txMachine* the)
{
	txSlot* argument = the->stack;
	txSlot* constructor = the->stack + 1;
	txSlot* result = the->stack + 2;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
// 	if (!mxIsReference(mxThis))
// 		mxTypeError("this is no object");
	if (mxIsReference(argument)) {
		txSlot* promise = argument->value.reference;
		if (mxIsPromise(promise)) {
			mxPushReference(promise);
			mxGetID(mxID(_constructor));
			if (fxIsSameValue(the, constructor, the->stack, 0)) {
				*result = *argument;
    			mxPop();
				return;
			}
			mxPop();
		}
	}
	mxTemporary(resolveFunction);
	mxTemporary(rejectFunction);
	mxPushSlot(constructor);
	fxNewPromiseCapability(the, resolveFunction, rejectFunction);
	*result = *the->stack;
    mxPop();
	/* THIS */
	mxPushUndefined();
	/* FUNCTION */
	mxPushSlot(resolveFunction);
	mxCall();
	/* ARGUMENTS */
	mxPushSlot(argument);
	/* COUNT */
	mxRunCount(1);
	mxPop();
    mxPop(); // rejectFunction
    mxPop(); // resolveFunction
}

void fx_Promise_prototype_catch(txMachine* the)
{
	mxPushSlot(mxThis);
	mxDub();
	mxGetID(mxID(_then));
	mxCall();
	mxPushUndefined();
	if (mxArgc > 0) 
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	mxRunCount(2);
	mxPullSlot(mxResult);
}

#if 0
void fx_Promise_prototype_dumpAux(txMachine* the, txSlot* promise, txInteger c)
{
	txInteger i;
	txSlot* reference;
	for (i = 0; i < c; i++)
		fprintf(stderr, "\t");
	fprintf(stderr, "promise %d\n", promise->next->ID);
	reference = mxPromiseThens(promise)->value.reference->next;
    c++;
	while (reference) {
		fx_Promise_prototype_dumpAux(the, reference->value.reference, c);
		reference = reference->next;
	}
}
#endif

void fx_Promise_prototype_finally(txMachine* the)
{
	txSlot* constructor;
	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	mxPushSlot(mxThis);
	mxGetID(mxID(_constructor));
	fxToSpeciesConstructor(the, &mxPromiseConstructor);
	constructor = the->stack;
	
	mxPushSlot(mxThis);
	mxDub();
	mxGetID(mxID(_then));
	mxCall();
	if (mxArgc > 0) {
		if (mxIsReference(mxArgv(0)) && mxIsCallable(mxArgv(0)->value.reference)) {
			txSlot* function = fxNewHostFunction(the, fx_Promise_prototype_finallyAux, 1, XS_NO_ID, mx_Promise_prototype_finallyAuxProfileID);
			txSlot* object = fxNewInstance(the);
			txSlot* slot = object->next = fxNewSlot(the);
			slot->kind = XS_REFERENCE_KIND;
			slot->value.reference = constructor->value.reference;
			slot = slot->next = fxNewSlot(the);
			slot->kind = XS_REFERENCE_KIND;
			slot->value.reference = mxArgv(0)->value.reference;
			slot = slot->next = fxNewSlot(the);
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = 1;
			slot = mxFunctionInstanceHome(function);
			slot->value.home.object = object;
			mxPop();
			
			function = fxNewHostFunction(the, fx_Promise_prototype_finallyAux, 1, XS_NO_ID, mx_Promise_prototype_finallyAuxProfileID);
			object = fxNewInstance(the);
			slot = object->next = fxNewSlot(the);
			slot->kind = XS_REFERENCE_KIND;
			slot->value.reference = constructor->value.reference;
			slot = slot->next = fxNewSlot(the);
			slot->kind = XS_REFERENCE_KIND;
			slot->value.reference = mxArgv(0)->value.reference;
			slot = slot->next = fxNewSlot(the);
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = 0;
			slot = mxFunctionInstanceHome(function);
			slot->value.home.object = object;
			mxPop();
		}
		else {
			mxPushSlot(mxArgv(0));
			mxPushSlot(mxArgv(0));
		}
	}
	else {
		mxPushUndefined();
		mxPushUndefined();
	}
	mxRunCount(2);
	mxPullSlot(mxResult);
}

void fx_Promise_prototype_finallyAux(txMachine* the)
{
	txSlot* object = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.object;
	txSlot* constructor = object->next;
	txSlot* onFinally = constructor->next;
	txSlot* success = onFinally->next;
	txSlot* argument;
	txSlot* function;
	txSlot* slot;
	txSlot* home;
	
	{
		mxTry(the) {
			mxPushUndefined();
			mxPushSlot(onFinally);
			mxCall();
			mxRunCount(0);
		}
		mxCatch(the) {
			mxArgv(0)->kind = mxException.kind;
			mxArgv(0)->value = mxException.value;
			success->value.boolean = 0;
			mxPush(mxException);
			mxException = mxUndefined;
		}
	}
	argument = the->stack;
	
	mxPushUndefined();
	mxPushSlot(constructor);
	mxPushSlot(argument);
	fx_Promise_resolveAux(the);
	mxPop();
	mxPop();
	mxDub();
	mxGetID(mxID(_then));
	mxCall();
	
	if (success->value.boolean)
		function = fxNewHostFunction(the, fx_Promise_prototype_finallyReturn, 0, XS_NO_ID, mx_Promise_prototype_finallyReturnProfileID);
	else
		function = fxNewHostFunction(the, fx_Promise_prototype_finallyThrow, 0, XS_NO_ID, mx_Promise_prototype_finallyThrowProfileID);
	object = fxNewInstance(the);
	slot = object->next = fxNewSlot(the);
	slot->kind = mxArgv(0)->kind;
	slot->value = mxArgv(0)->value;
	home = mxFunctionInstanceHome(function);
	home->value.home.object = object;
	mxPop();
	mxRunCount(1);
	
	mxPullSlot(mxResult);
}

void fx_Promise_prototype_finallyReturn(txMachine* the)
{
	txSlot* object = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.object;
	txSlot* slot = object->next;
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

void fx_Promise_prototype_finallyThrow(txMachine* the)
{
	txSlot* object = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.object;
	txSlot* slot = object->next;
	mxException.kind = slot->kind;
	mxException.value = slot->value;
	fxThrow(the, NULL, 0);
}

void fx_Promise_prototype_then(txMachine* the)
{
	txSlot* promise;
	txSlot* onFullfilled = C_NULL;
	txSlot* onRejected = C_NULL;
	txSlot* resolveFunction;
	txSlot* rejectFunction;

	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	promise = mxThis->value.reference;
	if (!mxIsPromise(promise))
		mxTypeError("this is no promise");
#ifdef mxPromisePrint
	fprintf(stderr, "fx_Promise_prototype_then %d\n", promise->next->ID);
#endif

	if ((mxArgc > 0) && mxIsReference(mxArgv(0))) {
		onFullfilled = mxArgv(0);
	}
	if ((mxArgc > 1) && mxIsReference(mxArgv(1))) {
		onRejected = mxArgv(1);
	}
		
	mxTemporary(resolveFunction);
	mxTemporary(rejectFunction);
	mxPushSlot(mxThis);
	mxGetID(mxID(_constructor));
	fxToSpeciesConstructor(the, &mxPromiseConstructor);
	fxNewPromiseCapability(the, resolveFunction, rejectFunction);
	mxPullSlot(mxResult);
		
	fxPromiseThen(the, promise, onFullfilled, onRejected, resolveFunction, rejectFunction);
}

void fxQueueJob(txMachine* the, txInteger count, txSlot* promise)
{
	txSlot* slot;
	txSlot* job;
	txSlot* item;
	txSlot* stack;
	txSlot** address;
	
	if (promise) {
		txSlot* list = &mxUnhandledPromises;
		txSlot** address = &list->value.reference->next;
		while ((slot = *address)) {
			if (slot->value.weakRef.target == promise) {
				slot = slot->next;
				*address = slot->next;
				break;
			}
			slot = slot->next;
			address = &slot->next;
		}
#ifdef mxPromisePrint
		fprintf(stderr, "fxQueueJob %d\n", promise->next->ID);
#endif
	}
	if (mxPendingJobs.value.reference->next == NULL) {
		fxQueuePromiseJobs(the);
	}
	count += 6;
	item = stack = the->stack + count;
	slot = job = fxNewInstance(the);
	while (count > 0) {
		item--;
		slot = slot->next = fxNewSlot(the);
		slot->kind = item->kind;
		slot->value = item->value;
		count--;
	}
	address = &(mxPendingJobs.value.reference->next);
	while ((slot = *address)) 
		address = &(slot->next);
	slot = *address = fxNewSlot(the);	
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = job;
	the->stack = stack;
}

void fxRunPromiseJobs(txMachine* the)
{
	txSlot* job;
	txSlot* slot;
	txInteger count;
	
#ifdef mxPromisePrint
	fprintf(stderr, "\n# fxRunPromiseJobs\n");
#endif
	job = mxRunningJobs.value.reference->next = mxPendingJobs.value.reference->next;
	mxPendingJobs.value.reference->next = C_NULL;
	while (job) {
		mxTry(the) {
			count = 0;
			slot = job->value.reference->next;
			while (slot) {
				mxPushSlot(slot);
				count++;
				slot = slot->next;
			}
			mxRunCount(count - 6);
			mxPop();
			if (mxDuringJobs.kind == XS_REFERENCE_KIND)
				mxDuringJobs.value.reference->next = C_NULL;
		}
		mxCatch(the) {
			fxAbort(the, XS_UNHANDLED_EXCEPTION_EXIT);
		}
		job = job->next;
	}
	mxRunningJobs.value.reference->next = C_NULL;
}






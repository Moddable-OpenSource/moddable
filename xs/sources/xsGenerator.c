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

static void fxNewGeneratorResult(txMachine* the, txBoolean done);
static txSlot* fxCheckGeneratorInstance(txMachine* the, txSlot* slot);
static void fxAsyncGeneratorRejectAwait(txMachine* the);
static void fxAsyncGeneratorRejectYield(txMachine* the);
static void fxAsyncGeneratorResolveAwait(txMachine* the);
static void fxAsyncGeneratorResolveYield(txMachine* the);
static void fxAsyncGeneratorStep(txMachine* the, txSlot* generator, txFlag status);
static txSlot* fxCheckAsyncGeneratorInstance(txMachine* the, txSlot* slot);

void fxBuildGenerator(txMachine* the)
{
	txSlot* slot;
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Generator_prototype_next), 1, mxID(_next), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Generator_prototype_return), 1, mxID(_return), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Generator_prototype_throw), 1, mxID(_throw), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Generator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxGeneratorPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructor(the, mxCallback(fx_Generator), 1, XS_NO_ID));
	slot = fxNextStringXProperty(the, slot, "GeneratorFunction", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxGeneratorFunctionPrototype = *the->stack;
	slot = fxNewHostConstructor(the, mxCallback(fx_GeneratorFunction), 1, mxID(_GeneratorFunction));
	the->stack++;
	
	slot = mxBehaviorGetProperty(the, mxGeneratorPrototype.value.reference, mxID(_constructor), XS_NO_ID, XS_OWN);
	slot->flag |= XS_DONT_SET_FLAG;
	slot = mxBehaviorGetProperty(the, mxGeneratorFunctionPrototype.value.reference, mxID(_constructor), XS_NO_ID, XS_OWN);
	slot->flag |= XS_DONT_SET_FLAG;
	slot = mxBehaviorGetProperty(the, mxGeneratorFunctionPrototype.value.reference, mxID(_prototype), XS_NO_ID, XS_OWN);
	slot->flag &= ~XS_DONT_DELETE_FLAG;

	mxPush(mxObjectPrototype);
	slot = fxNewObjectInstance(the);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncIterator_asyncIterator), 0, mxID(_Symbol_asyncIterator), XS_DONT_ENUM_FLAG);
	mxPull(mxAsyncIteratorPrototype);
	
	mxPush(mxAsyncIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncGenerator_prototype_next), 1, mxID(_next), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncGenerator_prototype_return), 1, mxID(_return), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncGenerator_prototype_throw), 1, mxID(_throw), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "AsyncGenerator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxAsyncGeneratorPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructor(the, mxCallback(fx_AsyncGenerator), 1, XS_NO_ID));
	slot = fxNextStringXProperty(the, slot, "AsyncGeneratorFunction", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxAsyncGeneratorFunctionPrototype = *the->stack;
	slot = fxNewHostConstructor(the, mxCallback(fx_AsyncGeneratorFunction), 1, mxID(_AsyncGeneratorFunction));
	the->stack++;

	slot = mxBehaviorGetProperty(the, mxAsyncGeneratorPrototype.value.reference, mxID(_constructor), XS_NO_ID, XS_OWN);
	slot->flag |= XS_DONT_SET_FLAG;
	slot = mxBehaviorGetProperty(the, mxAsyncGeneratorFunctionPrototype.value.reference, mxID(_constructor), XS_NO_ID, XS_OWN);
	slot->flag |= XS_DONT_SET_FLAG;
	slot = mxBehaviorGetProperty(the, mxAsyncGeneratorFunctionPrototype.value.reference, mxID(_prototype), XS_NO_ID, XS_OWN);
	slot->flag &= ~XS_DONT_DELETE_FLAG;
	
	mxPush(mxAsyncIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncFromSyncIterator_prototype_next), 1, mxID(_next), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncFromSyncIterator_prototype_return), 1, mxID(_return), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncFromSyncIterator_prototype_throw), 1, mxID(_throw), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Async-from-Sync Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxAsyncFromSyncIteratorPrototype = *the->stack;
	the->stack++;
}

txSlot* fxCheckGeneratorInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		slot = instance->next;
		if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_STACK_KIND))
			return instance;
	}
	mxTypeError("this is no Generator instance");
	return C_NULL;
}

txSlot* fxNewGeneratorInstance(txMachine* the)
{
	txSlot* prototype;
	txSlot* instance;
	txSlot* property;
	txSlot* slot;

	prototype = (the->stack->kind == XS_REFERENCE_KIND) ? the->stack->value.reference : mxGeneratorPrototype.value.reference;

	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = prototype;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;

	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_STACK_KIND;
	property->ID = XS_NO_ID;
    property->value.stack.length = 0;
    property->value.stack.address = C_NULL;
	if (prototype) {
		slot = prototype->next;
		if (slot && (slot->kind == XS_STACK_KIND)) {
			property->value.stack.length = slot->value.stack.length;
			property->value.stack.address = slot->value.stack.address;
		}
	}
	
    property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
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
	if (mxTarget->kind != XS_UNDEFINED_KIND)
		mxTypeError("new Generator");
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
	the->stack++;
	
	return instance;
}

void fx_GeneratorFunction(txMachine* the)
{	
#ifdef mxParse
	txInteger c, i;
	txStringStream stream;
	
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
	fxConcatStringC(the, the->stack, "){");
	if (c > 0) {
		fxToString(the, mxArgv(i));
		fxConcatString(the, the->stack, mxArgv(i));
	}
	fxConcatStringC(the, the->stack, "})");
	stream.slot = the->stack;
	stream.offset = 0;
	stream.size = c_strlen(the->stack->value.string);
	fxRunScript(the, fxParseScript(the, &stream, fxStringGetter, mxProgramFlag), C_NULL, C_NULL, C_NULL, C_NULL, C_NULL);
	mxPullSlot(mxResult);
	if (!mxIsUndefined(mxTarget) && !fxIsSameSlot(the, mxTarget, mxFunction)) {
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxGeneratorFunctionPrototype);
		mxResult->value.reference->value.instance.prototype = the->stack->value.reference;
		mxPop();
	}
#else
	mxUnknownError("not built-in");
#endif
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
	txSlot* queue = state->next;
	txSlot* current = queue->value.list.first;
	txSlot* resolveFunction = current->value.reference->next;
	txSlot* rejectFunction = resolveFunction->next;

	mxPushSlot(mxArgv(0));
	mxPushInteger(1);
	mxPushUndefined();
	mxPushSlot(rejectFunction);
	
	queue->value.list.first = current = current->next;
	if (current == C_NULL)
		queue->value.list.last = C_NULL;
	
	fxCall(the);
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
	txSlot* resolveFunction = current->value.reference->next;
	
	mxPushSlot(mxArgv(0));
	fxNewGeneratorResult(the, (state->value.integer == XS_CODE_END) ? 1 : 0);
	mxPushInteger(1);
	mxPushUndefined();
	mxPushSlot(resolveFunction);
	
	queue->value.list.first = current = current->next;
	if (current == C_NULL)
		queue->value.list.last = C_NULL;
	
	fxCall(the);
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

void fxAsyncGeneratorStep(txMachine* the, txSlot* generator, txFlag status)
{
	txSlot* stack = generator->next;
	txSlot* state = stack->next;
	txSlot* queue = state->next;
	txSlot* resolveAwaitFunction = queue->next;
	txSlot* rejectAwaitFunction = resolveAwaitFunction->next;
	txSlot* resolveYieldFunction = rejectAwaitFunction->next;
	txSlot* rejectYieldFunction = resolveYieldFunction->next;
	txSlot* value;

	if ((state->value.integer == XS_CODE_START_ASYNC_GENERATOR) && (status != XS_NO_STATUS))
		state->value.integer = XS_CODE_END;
		
	if (state->value.integer == XS_CODE_END) {
		mxPushSlot(resolveYieldFunction);
		mxPushSlot(rejectYieldFunction);
		mxPushInteger(2);
		if ((status == XS_NO_STATUS) || (mxArgc == 0))
			mxPushUndefined();
		else 
			mxPushSlot(mxArgv(0));
		mxPushInteger(1);
		mxPush(mxPromiseConstructor);
		if (status == XS_THROW_STATUS)
			fxCallID(the, mxID(_reject));
		else
			fxCallID(the, mxID(_resolve));
		fxCallID(the, mxID(_then));
		mxPop();
	}
	else {
		mxTry(the) {
			the->status = status;
			state->value.integer = XS_NO_CODE;
			fxRunID(the, generator, XS_NO_ID);
			if (state->value.integer == XS_NO_CODE)
				state->value.integer = XS_CODE_END;
			value = the->stack;
			mxPushSlot(value);
			mxPushInteger(1);
			mxPush(mxPromiseConstructor);
			fxCallID(the, mxID(_resolve));
			mxPullSlot(value);
			if (state->value.integer == XS_CODE_AWAIT) {
				mxPushSlot(resolveAwaitFunction);
				mxPushSlot(rejectAwaitFunction);
			}
			else {
				mxPushSlot(resolveYieldFunction);
				mxPushSlot(rejectYieldFunction);
			}
			mxPushInteger(2);
			mxPushSlot(value);
			fxCallID(the, mxID(_then));
			mxPop();
		}
		mxCatch(the) {
			state->value.integer = XS_CODE_END;
			mxPushSlot(resolveYieldFunction);
			mxPushSlot(rejectYieldFunction);
			mxPushInteger(2);
			mxPush(mxException);
			mxPushInteger(1);
			mxPush(mxPromiseConstructor);
			fxCallID(the, mxID(_reject));
			fxCallID(the, mxID(_then));
			mxPop();
		}
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
	mxTypeError("this is no AsyncGenerator instance");
	return C_NULL;
}

txSlot* fxNewAsyncGeneratorInstance(txMachine* the)
{
	txSlot* prototype;
	txSlot* instance;
	txSlot* property;
	txSlot* slot;
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
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_STACK_KIND;
	property->ID = XS_NO_ID;
    property->value.stack.length = 0;
    property->value.stack.address = C_NULL;
	if (prototype) {
		slot = prototype->next;
		if (slot && (slot->kind == XS_STACK_KIND)) {
			property->value.stack.length = slot->value.stack.length;
			property->value.stack.address = slot->value.stack.address;
		}
	}
	
    property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_INTEGER_KIND;
	property->value.integer = XS_CODE_START_ASYNC_GENERATOR;
	
    property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_LIST_KIND;
	property->value.list.first = C_NULL;
	property->value.list.last = C_NULL;
	
	function = fxNewHostFunction(the, fxAsyncGeneratorResolveAwait, 1, XS_NO_ID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPop();
	
	function = fxNewHostFunction(the, fxAsyncGeneratorRejectAwait, 1, XS_NO_ID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPop();
	
	function = fxNewHostFunction(the, fxAsyncGeneratorResolveYield, 1, XS_NO_ID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPop();
	
	function = fxNewHostFunction(the, fxAsyncGeneratorRejectYield, 1, XS_NO_ID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPop();
	
	return instance;
}


void fx_AsyncGenerator(txMachine* the)
{
	if (mxTarget->kind != XS_UNDEFINED_KIND)
		mxTypeError("new AsyncGenerator");
}

void fx_AsyncGenerator_prototype_aux(txMachine* the, txFlag status)
{
	txSlot* stack = the->stack;
	txSlot* promise;
	txSlot* slot;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
	txSlot* generator;
	txSlot* queue;
	txSlot* instance;
	txSlot* property;

	mxTry(the) {
		mxPush(mxPromisePrototype);
		promise = fxNewPromiseInstance(the);
		slot = mxPromiseStatus(promise);
		slot->value.integer = mxPendingStatus;
		mxPullSlot(mxResult);
		slot = fxNewPromiseAlready(the);
		resolveFunction = fxNewPromiseFunction(the, slot, promise, mxResolvePromiseFunction.value.reference);
		rejectFunction =  fxNewPromiseFunction(the, slot, promise, mxRejectPromiseFunction.value.reference);
		generator = fxCheckAsyncGeneratorInstance(the, mxThis);
		queue = generator->next->next->next;
		instance = property = fxNewInstance(the);
		property = fxNextReferenceProperty(the, property, resolveFunction, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
 		property = fxNextReferenceProperty(the, property, rejectFunction, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
		property = fxNextIntegerProperty(the, property, status, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
		if (mxArgc > 0)
			property = fxNextSlotProperty(the, property, mxArgv(0), XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
		else
			property = fxNextUndefinedProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
		slot = fxNewSlot(the);
		slot->kind = XS_REFERENCE_KIND;
		slot->value.reference = instance;
		if (queue->value.list.first) {
			queue->value.list.last->next = slot;
			queue->value.list.last = slot;
		}
		else {
			queue->value.list.first = slot;
			queue->value.list.last = slot;
			the->scratch.kind = property->kind;
			the->scratch.value = property->value;
			fxAsyncGeneratorStep(the, generator, status);
		}
	}
	mxCatch(the) {
		mxPush(mxException);
		mxPushInteger(1);
		mxPushUndefined();
		mxPushReference(rejectFunction);
		fxCall(the);
		mxPop();
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
	the->stack++;
	
	return instance;
}

void fx_AsyncGeneratorFunction(txMachine* the)
{	
#ifdef mxParse
	txInteger c, i;
	txStringStream stream;
	
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
	fxConcatStringC(the, the->stack, "){");
	if (c > 0) {
		fxToString(the, mxArgv(i));
		fxConcatString(the, the->stack, mxArgv(i));
	}
	fxConcatStringC(the, the->stack, "})");
	stream.slot = the->stack;
	stream.offset = 0;
	stream.size = c_strlen(the->stack->value.string);
	fxRunScript(the, fxParseScript(the, &stream, fxStringGetter, mxProgramFlag), C_NULL, C_NULL, C_NULL, C_NULL, C_NULL);
	mxPullSlot(mxResult);
	if (!mxIsUndefined(mxTarget) && !fxIsSameSlot(the, mxTarget, mxFunction)) {
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxAsyncGeneratorFunctionPrototype);
		mxResult->value.reference->value.instance.prototype = the->stack->value.reference;
		mxPop();
	}
#else
	mxUnknownError("not built-in");
#endif
}

void fx_AsyncIterator_asyncIterator(txMachine* the)
{
	*mxResult = *mxThis;
}


txSlot* fxCheckAsyncFromSyncIteratorInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		slot = instance->next;
		if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->ID == mxID(_iterator)))
			return instance;
	}
	mxTypeError("this is no AsyncFromSyncIterator instance");
	return C_NULL;
}

txSlot* fxNewAsyncFromSyncIteratorInstance(txMachine* the)
{
	txSlot* iterator = the->stack;
	txSlot* instance;
	txSlot* slot;
	mxPush(mxAsyncFromSyncIteratorPrototype);
	instance = fxNewObjectInstance(the);
	slot = fxLastProperty(the, instance);
	slot = fxNextSlotProperty(the, slot, iterator, mxID(_iterator), XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushSlot(iterator);
	fxGetID(the, mxID(_next));
	slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPop();
	mxPullSlot(iterator);
	return instance;
}

void fx_AsyncFromSyncIterator_prototype_next(txMachine* the)
{
	txSlot* stack = the->stack;
	txSlot* promise;
	txSlot* slot;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
	txSlot* instance;
	txSlot* iterator;
	txSlot* nextFunction;
	txBoolean done;

	mxTry(the) {
		mxPush(mxPromisePrototype);
		promise = fxNewPromiseInstance(the);
		slot = mxPromiseStatus(promise);
		slot->value.integer = mxPendingStatus;
		mxPullSlot(mxResult);
		slot = fxNewPromiseAlready(the);
		resolveFunction = fxNewPromiseFunction(the, slot, promise, mxResolvePromiseFunction.value.reference);
		rejectFunction = fxNewPromiseFunction(the, slot, promise, mxRejectPromiseFunction.value.reference);
		instance = fxCheckAsyncFromSyncIteratorInstance(the, mxThis);
		iterator = instance->next;
		nextFunction = iterator->next;
		if (mxArgc == 0)
			mxPushUndefined();
		else
			mxPushSlot(mxArgv(0));
		mxPushInteger(1);
		mxPushSlot(iterator);
		mxPushSlot(nextFunction);
		fxCall(the);
		slot = the->stack;
		mxPushSlot(slot);
		fxGetID(the, mxID(_done));
		done = fxToBoolean(the, the->stack);
		mxPop();
		fxGetID(the, mxID(_value));
		fxNewGeneratorResult(the, done);
		mxPushInteger(1);
		mxPushUndefined();
		mxPushReference(resolveFunction);
		fxCall(the);
	}
	mxCatch(the) {
		mxPush(mxException);
		mxPushInteger(1);
		mxPushUndefined();
		mxPushReference(rejectFunction);
		fxCall(the);
		mxPop();
	}
	the->stack = stack;
}

void fx_AsyncFromSyncIterator_prototype_return(txMachine* the)
{
	txSlot* stack = the->stack;
	txSlot* promise;
	txSlot* slot;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
	txSlot* instance;
	txSlot* iterator;
	txSlot* returnFunction;
	txBoolean done;

	mxTry(the) {
		mxPush(mxPromisePrototype);
		promise = fxNewPromiseInstance(the);
		slot = mxPromiseStatus(promise);
		slot->value.integer = mxPendingStatus;
		mxPullSlot(mxResult);
		slot = fxNewPromiseAlready(the);
		resolveFunction = fxNewPromiseFunction(the, slot, promise, mxResolvePromiseFunction.value.reference);
		rejectFunction = fxNewPromiseFunction(the, slot, promise, mxRejectPromiseFunction.value.reference);
		instance = fxCheckAsyncFromSyncIteratorInstance(the, mxThis);
		iterator = instance->next;
		mxPushSlot(iterator);
		fxGetID(the, mxID(_return));
		returnFunction = the->stack;
		if (mxIsUndefined(returnFunction)) {
			done = 1;
			mxPushUndefined();
		}
		else {
			if (mxArgc == 0)
				mxPushUndefined();
			else
				mxPushSlot(mxArgv(0));
			mxPushInteger(1);
			mxPushSlot(iterator);
			mxPushSlot(returnFunction);
			fxCall(the);
			slot = the->stack;
			mxPushSlot(slot);
			fxGetID(the, mxID(_done));
			done = fxToBoolean(the, the->stack);
			mxPop();
			fxGetID(the, mxID(_value));
		}
		fxNewGeneratorResult(the, done);
		mxPushInteger(1);
		mxPushUndefined();
		mxPushReference(resolveFunction);
		fxCall(the);
	}
	mxCatch(the) {
		mxPush(mxException);
		mxPushInteger(1);
		mxPushUndefined();
		mxPushReference(rejectFunction);
		fxCall(the);
	}
	the->stack = stack;
}

void fx_AsyncFromSyncIterator_prototype_throw(txMachine* the)
{
	txSlot* stack = the->stack;
	txSlot* promise;
	txSlot* slot;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
	txSlot* instance;
	txSlot* iterator;
	txSlot* throwFunction;
	txBoolean done;

	mxTry(the) {
		mxPush(mxPromisePrototype);
		promise = fxNewPromiseInstance(the);
		slot = mxPromiseStatus(promise);
		slot->value.integer = mxPendingStatus;
		mxPullSlot(mxResult);
		slot = fxNewPromiseAlready(the);
		resolveFunction = fxNewPromiseFunction(the, slot, promise, mxResolvePromiseFunction.value.reference);
		rejectFunction = fxNewPromiseFunction(the, slot, promise, mxRejectPromiseFunction.value.reference);
		instance = fxCheckAsyncFromSyncIteratorInstance(the, mxThis);
		iterator = instance->next;
		mxPushSlot(iterator);
		fxGetID(the, mxID(_throw));
		throwFunction = the->stack;
		if (mxIsUndefined(throwFunction)) {
			if (mxArgc == 0)
				mxPushUndefined();
			else
				mxPushSlot(mxArgv(0));
			mxPushInteger(1);
			mxPushUndefined();
			mxPushReference(rejectFunction);
			fxCall(the);
		}
		else {
			if (mxArgc == 0)
				mxPushUndefined();
			else
				mxPushSlot(mxArgv(0));
			mxPushInteger(1);
			mxPushSlot(iterator);
			mxPushSlot(throwFunction);
			fxCall(the);
			slot = the->stack;
			mxPushSlot(slot);
			fxGetID(the, mxID(_done));
			done = fxToBoolean(the, the->stack);
			mxPop();
			fxGetID(the, mxID(_value));
			fxNewGeneratorResult(the, done);
			mxPushInteger(1);
			mxPushUndefined();
			mxPushReference(resolveFunction);
			fxCall(the);
		}
	}
	mxCatch(the) {
		mxPush(mxException);
		mxPushInteger(1);
		mxPushUndefined();
		mxPushReference(rejectFunction);
		fxCall(the);
	}
	the->stack = stack;
}



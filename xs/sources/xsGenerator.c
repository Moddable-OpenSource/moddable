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

static void fxNewGeneratorResult(txMachine* the, txBoolean done);

static txSlot* fxCheckGeneratorInstance(txMachine* the, txSlot* slot);
static void fx_Generator_prototype_aux(txMachine* the, txFlag status);

static void fxAsyncGeneratorStep(txMachine* the, txSlot* generator, txFlag status);
static txSlot* fxCheckAsyncGeneratorInstance(txMachine* the, txSlot* slot);
static void fx_AsyncGenerator_prototype_aux(txMachine* the, txFlag status);

static txSlot* fxCheckAsyncFromSyncIteratorInstance(txMachine* the, txSlot* slot);
static void fx_AsyncFromSyncIterator_prototype_aux(txMachine* the, txFlag status);

void fxBuildGenerator(txMachine* the)
{
	txSlot* slot;
	txSlot* property;

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
	if (!mxIsUndefined(mxTarget) && !fxIsSameSlot(the, mxTarget, mxFunction)) {
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxGeneratorFunctionPrototype);
		mxResult->value.reference->value.instance.prototype = the->stack->value.reference;
		mxPop();
	}
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
		}
		else {
			the->status = status;
			status = XS_NO_STATUS;
			state->value.integer = XS_NO_CODE;
			fxRunID(the, generator, XS_NO_ID);
		}
		value = the->stack;
		if (state->value.integer == XS_NO_CODE) {
			state->value.integer = XS_CODE_END;
			if (value->kind == XS_UNINITIALIZED_KIND)
				value->kind = XS_UNDEFINED_KIND;
			mxPushUndefined();
			mxPushSlot(resolveYieldFunction);
			mxCall();
			mxPushSlot(value);
			mxRunCount(1);
			mxPop();
		}
		else {
			if (status != XS_THROW_STATUS) {
				if (mxIsReference(value) && mxIsPromise(value->value.reference)) {
					mxDub();
					mxGetID(mxID(_constructor));
					if (fxIsSameValue(the, &mxPromiseConstructor, the->stack, 0)) {
						mxPop();
						if (state->value.integer == XS_CODE_AWAIT) {
							fxPromiseThen(the, value->value.reference, resolveAwaitFunction, rejectAwaitFunction, C_NULL, C_NULL);
						}
						else {
							fxPromiseThen(the, value->value.reference, resolveYieldFunction, rejectYieldFunction, C_NULL, C_NULL);
						}
						goto exit;
					}
					mxPop();
				}
			}
			mxTemporary(resolveFunction);
			mxTemporary(rejectFunction);
			mxPush(mxPromiseConstructor);
			fxNewPromiseCapability(the, resolveFunction, rejectFunction);
#ifdef mxPromisePrint
			fprintf(stderr, "fxAsyncGeneratorStep %d\n", the->stack->value.reference->next->ID);
#endif
			if (state->value.integer == XS_CODE_AWAIT) {
				fxPromiseThen(the, the->stack->value.reference, resolveAwaitFunction, rejectAwaitFunction, C_NULL, C_NULL);
			}
			else {
				fxPromiseThen(the, the->stack->value.reference, resolveYieldFunction, rejectYieldFunction, C_NULL, C_NULL);
			}
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
			/* COUNT */
			mxRunCount(1);
			mxPop();
		}
exit:			
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
	mxTypeError("this is no AsyncGenerator instance");
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
	if (mxTarget->kind != XS_UNDEFINED_KIND)
		mxTypeError("new AsyncGenerator");
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
			queue = state->next;
			if (((status == XS_RETURN_STATUS) || (status == XS_THROW_STATUS))
				&& (
				/*(state->value.integer == XS_CODE_AWAIT) ||*/ (state->value.integer == XS_CODE_START_ASYNC_GENERATOR)))
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
	if (!mxIsUndefined(mxTarget) && !fxIsSameSlot(the, mxTarget, mxFunction)) {
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxAsyncGeneratorFunctionPrototype);
		mxResult->value.reference->value.instance.prototype = the->stack->value.reference;
		mxPop();
	}
}

void fx_AsyncIterator_asyncIterator(txMachine* the)
{
	*mxResult = *mxThis;
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

	mxPullSlot(iterator);
	return instance;
}

void fx_AsyncFromSyncIterator_prototype_aux(txMachine* the, txFlag status)
{
	txSlot* stack = the->stack;
	txSlot* resolveFunction;
    txSlot* rejectFunction;
	txSlot* slot;
	txSlot* instance;
	txSlot* iterator;
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
			instance = fxCheckAsyncFromSyncIteratorInstance(the, mxThis);
			iterator = instance->next;
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
					mxPushUndefined();
					mxPushSlot(rejectFunction);
					mxCall();
					if (mxArgc == 0)
						mxPushUndefined();
					else
						mxPushSlot(mxArgv(0));
					mxRunCount(1);
				}
				else
					stepFunction = the->stack;
			}
			if (stepFunction) {
				txSlot* doneFunction = iterator->next->next;
				txSlot* doneFlag = doneFunction->next;
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
                    mxTypeError("no object");
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
				fxPromiseThen(the, the->stack->value.reference, doneFunction, C_NULL, resolveFunction, rejectFunction);
			}
		}
		mxCatch(the) {
			fxRejectException(the, rejectFunction);
		}
    }
	the->stack = stack;
}

void fx_AsyncFromSyncIterator_prototype_next(txMachine* the)
{
	fx_AsyncFromSyncIterator_prototype_aux(the, XS_NO_STATUS);
}

void fx_AsyncFromSyncIterator_prototype_return(txMachine* the)
{
	fx_AsyncFromSyncIterator_prototype_aux(the, XS_RETURN_STATUS);
}

void fx_AsyncFromSyncIterator_prototype_throw(txMachine* the)
{
	fx_AsyncFromSyncIterator_prototype_aux(the, XS_THROW_STATUS);
}



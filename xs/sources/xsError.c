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

static txSlot* fx_Error_aux(txMachine* the, txError error, txInteger i);

void fxBuildError(txMachine* the)
{
	txSlot* slot;
	txSlot* prototype;
	txSlot* instance;
#if mxExplicitResourceManagement
	txSlot* property;
#endif
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Error_toString), 0, mxID(_toString), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Error", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_Error_prototype_get_stack), C_NULL, mxID(_stack), XS_DONT_ENUM_FLAG);
	mxErrorPrototype = *the->stack;
	prototype = fxBuildHostConstructor(the, mxCallback(fx_Error), 1, mxID(_Error));
	mxErrorConstructor = *the->stack;
	mxPop();
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "AggregateError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxAggregateErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_AggregateError), 2, mxID(_AggregateError));
	instance->value.instance.prototype = prototype;
	mxAggregateErrorConstructor = *the->stack;
	mxPop();
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "EvalError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxEvalErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_EvalError), 1, mxID(_EvalError));
	instance->value.instance.prototype = prototype;
	mxEvalErrorConstructor = *the->stack;
	mxPop();
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "RangeError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxRangeErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_RangeError), 1, mxID(_RangeError));
	instance->value.instance.prototype = prototype;
	mxRangeErrorConstructor = *the->stack;
	mxPop();
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "ReferenceError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxReferenceErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_ReferenceError), 1, mxID(_ReferenceError));
	instance->value.instance.prototype = prototype;
	mxReferenceErrorConstructor = *the->stack;
	mxPop();
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "SuppressedError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxSuppressedErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_SuppressedError), 3, mxID(_SuppressedError));
	instance->value.instance.prototype = prototype;
	mxSuppressedErrorConstructor = *the->stack;
	mxPop();
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "SyntaxError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxSyntaxErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_SyntaxError), 1, mxID(_SyntaxError));
	instance->value.instance.prototype = prototype;
	mxSyntaxErrorConstructor = *the->stack;
	mxPop();
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "TypeError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxTypeErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_TypeError), 1, mxID(_TypeError));
	instance->value.instance.prototype = prototype;
	mxTypeErrorConstructor = *the->stack;
	mxPop();
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "URIError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxURIErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_URIError), 1, mxID(_URIError));
	instance->value.instance.prototype = prototype;
	mxURIErrorConstructor = *the->stack;
	mxPop();

#if mxExplicitResourceManagement
	mxPush(mxObjectPrototype);
    slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Error_toString), 0, mxID(_toString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_DisposableStack_prototype_get_disposed), C_NULL, mxID(_disposed), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DisposableStack_prototype_adopt), 2, mxID(_adopt), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DisposableStack_prototype_defer), 1, mxID(_defer), XS_DONT_ENUM_FLAG);
	property = slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DisposableStack_prototype_dispose), 0, mxID(_dispose), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DisposableStack_prototype_move), 0, mxID(_move), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DisposableStack_prototype_use), 1, mxID(_use), XS_DONT_ENUM_FLAG);
	slot = fxNextSlotProperty(the, slot, property, mxID(_Symbol_dispose), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "DisposableStack", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxDisposableStackPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_DisposableStack), 0, mxID(_DisposableStack));
	mxDisposableStackConstructor = *the->stack;
	mxPop();
	
	mxPush(mxObjectPrototype);
    slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Error_toString), 0, mxID(_toString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_AsyncDisposableStack_prototype_get_disposed), C_NULL, mxID(_disposed), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncDisposableStack_prototype_adopt), 2, mxID(_adopt), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncDisposableStack_prototype_defer), 1, mxID(_defer), XS_DONT_ENUM_FLAG);
	property = slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncDisposableStack_prototype_disposeAsync), 0, mxID(_disposeAsync), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncDisposableStack_prototype_move), 0, mxID(_move), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_AsyncDisposableStack_prototype_use), 1, mxID(_use), XS_DONT_ENUM_FLAG);
	slot = fxNextSlotProperty(the, slot, property, mxID(_Symbol_asyncDispose), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "AsyncDisposableStack", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxAsyncDisposableStackPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_AsyncDisposableStack), 0, mxID(_AsyncDisposableStack));
	mxAsyncDisposableStackConstructor = *the->stack;
	mxPop();
#endif
}

void fxCaptureErrorStack(txMachine* the, txSlot* internal, txSlot* frame)
{
	txSlot* slot = internal->value.reference = fxNewSlot(the);
	slot->kind = XS_INSTANCE_KIND;
	slot->value.instance.garbage = C_NULL;
	slot->value.instance.prototype = C_NULL;
	while (frame->next) {
		txSlot* environment = mxFrameToEnvironment(frame);
		txSlot* function = frame + 3; 
		if (function->kind == XS_REFERENCE_KIND) {
			function = function->value.reference;
			if (mxIsFunction(function)) {
				txSlot* name = mxBehaviorGetProperty(the, function, mxID(_name), 0, XS_OWN);
				slot = slot->next = fxNewSlot(the);
				slot->flag = XS_GET_ONLY;
				slot->kind = XS_KEY_KIND;
				slot->ID = environment->ID;
				slot->value.key.string = C_NULL;
				slot->value.key.sum = environment->value.environment.line;
				if (name && ((name->kind == XS_STRING_KIND) || (name->kind == XS_STRING_X_KIND))) {
					if (name->kind == XS_STRING_X_KIND)
						slot->kind = XS_KEY_X_KIND;
					slot->value.key.string = name->value.string;
				}
				else {
					txSlot* code = mxFunctionInstanceCode(function);
					if (code->ID != XS_NO_ID) {
						txSlot* key = fxGetKey(the, code->ID);
						if (key->kind == XS_KEY_X_KIND)
							slot->kind = XS_KEY_X_KIND;
						slot->value.key.string = key->value.key.string;
					}
				}
			}
		}
		frame = frame->next;
	}
}

void fx_Error(txMachine* the)
{
	fx_Error_aux(the, XS_UNKNOWN_ERROR, 0);
}

txSlot* fx_Error_aux(txMachine* the, txError error, txInteger i)
{
	txSlot* instance;
	txSlot* slot;
	if (mxIsUndefined(mxTarget))
		mxPushSlot(mxFunction);
	else
		mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxErrorPrototypes(error));
	instance = fxNewObjectInstance(the);
	mxPullSlot(mxResult);
	slot = instance->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG;
	slot->kind = XS_ERROR_KIND;
	slot->value.error.info = C_NULL;
	slot->value.error.which = error;
	if (gxDefaults.captureErrorStack)
		gxDefaults.captureErrorStack(the, slot, the->frame->next);
	if ((mxArgc > i) && (mxArgv(i)->kind != XS_UNDEFINED_KIND)) {
		fxToString(the, mxArgv(i));
		slot = fxNextSlotProperty(the, slot, mxArgv(i), mxID(_message), XS_DONT_ENUM_FLAG);
	}
	i++;
	if ((mxArgc > i) && (mxArgv(i)->kind == XS_REFERENCE_KIND)) {
		if (mxBehaviorHasProperty(the, mxArgv(i)->value.reference, mxID(_cause), 0)) {
			mxPushSlot(mxArgv(i));
			mxGetID(mxID(_cause));
			slot = fxNextSlotProperty(the, slot, the->stack, mxID(_cause), XS_DONT_ENUM_FLAG);
			mxPop();
		}
	}
	return slot;
}

void fx_Error_toString(txMachine* the)
{
	txSlot* name;
	txSlot* message;
	if (mxThis->kind != XS_REFERENCE_KIND)
		mxTypeError("this is no Error instance");
	mxPushSlot(mxThis);
	mxGetID(mxID(_name));
	if (the->stack->kind == XS_UNDEFINED_KIND) 
		fxStringX(the, the->stack, "Error");
	else	
		fxToString(the, the->stack);
	name = the->stack;
	mxPushSlot(mxThis);
	mxGetID(mxID(_message));
	if (the->stack->kind == XS_UNDEFINED_KIND) 
		*the->stack = mxEmptyString;
	else	
		fxToString(the, the->stack);
	message = the->stack;
	if (c_isEmpty(name->value.string))
		*mxResult = *message;
	else if (c_isEmpty(message->value.string))
		*mxResult = *name;
	else {
		fxStringX(the, mxResult, "");
		fxConcatString(the, mxResult, name);
		fxConcatStringC(the, mxResult, ": ");
		fxConcatString(the, mxResult, message);
	}
	the->stack += 2;
}

void fx_AggregateError(txMachine* the)
{
	txSlot* stack = the->stack;
	txSlot* property = fx_Error_aux(the, XS_AGGREGATE_ERROR, 1);
	txSlot* array;
	txSlot** address;
	txIndex length = 0;
	txSlot* iterator;
	txSlot* next;
	txSlot* value;
	txSlot* slot;
	
	mxPush(mxArrayPrototype);
	array = fxNewArrayInstance(the);
	fxNextSlotProperty(the, property, the->stack, mxID(_errors), XS_DONT_ENUM_FLAG);
	address = &array->next->next;
	mxTemporary(iterator);
	mxTemporary(next);
	fxGetIterator(the, mxArgv(0), iterator, next, 0);
	mxTemporary(value);
	while (fxIteratorNext(the, iterator, next, value)) {
		mxTry(the) {
			*address = slot = fxNewSlot(the);
			slot->kind = value->kind;
			slot->value = value->value;
			address = &slot->next;
			length++;
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator);
			fxJump(the);
		}
	}
	array->next->value.array.length = length;
	fxCacheArray(the, array);
	
	the->stack = stack;
}

void fx_EvalError(txMachine* the)
{
	fx_Error_aux(the, XS_EVAL_ERROR, 0);
}

void fx_RangeError(txMachine* the)
{
	fx_Error_aux(the, XS_RANGE_ERROR, 0);
}

void fx_ReferenceError(txMachine* the)
{
	fx_Error_aux(the, XS_REFERENCE_ERROR, 0);
}

void fx_SuppressedError(txMachine* the)
{
	txSlot* property = fx_Error_aux(the, XS_SUPPRESSED_ERROR, 2);
	property = fxNextSlotProperty(the, property, mxArgv(0), mxID(_error), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, mxArgv(1), mxID(_suppressed), XS_DONT_ENUM_FLAG);
}

void fx_SyntaxError(txMachine* the)
{
	fx_Error_aux(the, XS_SYNTAX_ERROR, 0);
}

void fx_TypeError(txMachine* the)
{
	fx_Error_aux(the, XS_TYPE_ERROR, 0);
}

void fx_URIError(txMachine* the)
{
	fx_Error_aux(the, XS_URI_ERROR, 0);
}

void fx_Error_prototype_get_stack(txMachine* the)
{
	txSlot* slot;
	if (mxThis->kind != XS_REFERENCE_KIND)
		mxTypeError("this is no Error instance");
	slot = mxThis->value.reference->next;
	if (slot && (slot->kind == XS_ERROR_KIND)) {
		fxStringX(the, mxResult, "");
		mxPushSlot(mxThis);
		mxGetID(mxID(_name));
		if (the->stack->kind != XS_UNDEFINED_KIND)  {
			fxToString(the, the->stack);
			fxConcatString(the, mxResult, the->stack);
		}
		else
			fxConcatStringC(the, mxResult, "Error");
		mxPop();
		mxPushSlot(mxThis);
		mxGetID(mxID(_message));
		if (the->stack->kind != XS_UNDEFINED_KIND) {
			fxToString(the, the->stack);
			if (!c_isEmpty(the->stack->value.string)) {
				fxConcatStringC(the, mxResult, ": ");
				fxConcatString(the, mxResult, the->stack);
			}
		}
		mxPop();
		slot = slot->value.reference;
		if (slot) {
			slot = slot->next;
			while (slot) {
				fxConcatStringC(the, mxResult, "\n at");
				if ((slot->value.key.string != C_NULL) && (mxStringLength(slot->value.key.string))) {
					fxConcatStringC(the, mxResult, " ");
					fxConcatString(the, mxResult, slot);
				}
				fxConcatStringC(the, mxResult, " (");
				if (slot->ID != XS_NO_ID) {
					fxIDToString(the, slot->ID, the->nameBuffer, sizeof(the->nameBuffer));
					fxConcatStringC(the, mxResult, the->nameBuffer);
					fxConcatStringC(the, mxResult, ":");
					fxIntegerToString(the, (txInteger)slot->value.key.sum, the->nameBuffer, sizeof(the->nameBuffer));
					fxConcatStringC(the, mxResult, the->nameBuffer);
				}
				fxConcatStringC(the, mxResult, ")");
				slot = slot->next;
			}
		}
	}
}

#if mxExplicitResourceManagement

static txSlot* fxCheckDisposableStackInstance(txMachine* the, txSlot* slot, txBoolean mutable, txBoolean disposable);
static void fxDisposableStackPush(txMachine* the, txSlot* property);

txSlot* fxCheckDisposableStackInstance(txMachine* the, txSlot* slot, txBoolean mutable, txBoolean disposable)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_DISPOSABLE_STACK_KIND)) {
			if (mutable && (slot->flag & XS_DONT_SET_FLAG))
				mxTypeError("DisposableStack instance is read-only");
			if (disposable && slot->value.disposableStack.disposed)
				mxReferenceError("DisposableStack instance is disposed");
			return instance;
		}
	}
	mxTypeError("this is no DisposableStack instance");
	return C_NULL;
}

void fx_DisposableStack(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: DisposableStack");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxDisposableStackPrototype);
	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = instance;
	mxPullSlot(mxResult);
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_DISPOSABLE_STACK_KIND;
	property->value.disposableStack.stack = C_NULL;
	property->value.disposableStack.disposed = 0;
}

void fx_DisposableStack_prototype_get_disposed(txMachine* the)
{
	txSlot* instance = fxCheckDisposableStackInstance(the, mxThis, 0, 0);
	txSlot* property = instance->next;
	mxResult->value.boolean = property->value.disposableStack.disposed;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_DisposableStack_prototype_adopt(txMachine* the)
{
	txSlot* instance = fxCheckDisposableStackInstance(the, mxThis, 1, 1);
	txSlot* property = instance->next;
	if (mxArgc > 0) 
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	if (mxArgc > 1) 
		mxPushSlot(mxArgv(1));
	else
		mxPushUndefined();
	fxDisposableStackPush(the, property);
	property->value.disposableStack.stack->flag |= XS_BASE_FLAG;
	mxPop();
	mxPullSlot(mxResult);
}

void fx_DisposableStack_prototype_defer(txMachine* the)
{
	txSlot* instance = fxCheckDisposableStackInstance(the, mxThis, 1, 1);
	txSlot* property = instance->next;
	mxPushUndefined();
	if (mxArgc > 0) 
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	fxDisposableStackPush(the, property);
	mxPop();
	mxPop();
}	

void fx_DisposableStack_prototype_dispose(txMachine* the)
{
	txSlot* instance = fxCheckDisposableStackInstance(the, mxThis, 1, 0);
	txSlot* property = instance->next;
	txSlot* exception;
	txBoolean selector = 1;
	txSlot* slot;
	if (property->value.disposableStack.disposed)
		return;
	property->value.disposableStack.disposed = 1;
	mxTemporary(exception);
	slot = property->value.disposableStack.stack;
	while (slot) {
		txSlot* dispose = slot;
		txSlot* resource = slot->next;
		mxTry(the) {
			if (dispose->flag & XS_BASE_FLAG) {
				mxPushUndefined();
				mxPushSlot(dispose);
				mxCall();
				mxPushSlot(resource);
				mxRunCount(1);
				mxPop();
			}
			else {
				mxPushSlot(resource);
				mxPushSlot(dispose);
				mxCall();
				mxRunCount(0);
				mxPop();
			}
		}
		mxCatch(the) {
			if (selector == 0) {
				mxPush(mxSuppressedErrorConstructor);
				mxNew();
				mxPush(mxException);
				mxPushSlot(exception);
				mxRunCount(2);
				mxPullSlot(exception);
			}
			else {
				*exception = mxException;
				selector = 0;
			}
			mxException = mxUndefined;
		}
		slot = resource->next;
	}
	if (selector == 0) {
		mxException.kind = exception->kind;
		mxException.value = exception->value;
		fxJump(the);
	}
}

void fx_DisposableStack_prototype_move(txMachine* the)
{
	txSlot* instance = fxCheckDisposableStackInstance(the, mxThis, 1, 1);
	txSlot* property = instance->next;
	txSlot* resultInstance;
	txSlot* resultProperty;
	mxPush(mxDisposableStackConstructor);
	fxGetPrototypeFromConstructor(the, &mxDisposableStackPrototype);
	resultInstance = fxNewSlot(the);
	resultInstance->kind = XS_INSTANCE_KIND;
	resultInstance->value.instance.garbage = C_NULL;
	resultInstance->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = resultInstance;
	mxPullSlot(mxResult);
	resultProperty = resultInstance->next = fxNewSlot(the);
	resultProperty->flag = XS_INTERNAL_FLAG;
	resultProperty->kind = XS_DISPOSABLE_STACK_KIND;
	resultProperty->value.disposableStack.stack = property->value.disposableStack.stack;
	resultProperty->value.disposableStack.disposed = 0;
	property->value.disposableStack.stack = C_NULL;
	property->value.disposableStack.disposed = 1;
}

void fx_DisposableStack_prototype_use(txMachine* the)
{
	txSlot* instance = fxCheckDisposableStackInstance(the, mxThis, 1, 1);
	txSlot* property = instance->next;
	txSlot* resource;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	resource = the->stack;
	if (!mxIsNull(resource) && !mxIsUndefined(resource)) {
		mxPushSlot(resource);
		mxGetID(mxID(_Symbol_dispose));
		fxDisposableStackPush(the, property);
		mxPop();
	}
	mxPullSlot(mxResult);
}

void fxDisposableStackPush(txMachine* the, txSlot* property)
{
	txSlot* dispose = the->stack;
	txSlot* resource = dispose + 1;
	txSlot** address = &property->value.disposableStack.stack;
	txSlot* slot;
	if (!fxIsCallable(the, dispose))
		mxTypeError("dispose is no function");
		
	slot = fxNewSlot(the);
	slot->next = *address;
	slot->kind = resource->kind;
	slot->value = resource->value;
	*address = slot;
	
	slot = fxNewSlot(the);
	slot->next = *address;
	slot->kind = dispose->kind;
	slot->value = dispose->value;
	*address = slot;
}

static txSlot* fxCheckAsyncDisposableStackInstance(txMachine* the, txSlot* slot, txBoolean mutable, txBoolean disposable);
static void fxAsyncDisposableStackPush(txMachine* the, txSlot* property);
static void fxAsyncDisposableStackReject(txMachine* the);
static void fxAsyncDisposableStackResolve(txMachine* the);

txSlot* fxCheckAsyncDisposableStackInstance(txMachine* the, txSlot* slot, txBoolean mutable, txBoolean disposable)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_ASYNC_DISPOSABLE_STACK_KIND)) {
			if (mutable && (slot->flag & XS_DONT_SET_FLAG))
				mxTypeError("AsyncDisposableStack instance is read-only");
			if (disposable && slot->value.disposableStack.disposed)
				mxReferenceError("AsyncDisposableStack instance is disposed");
			return instance;
		}
	}
	mxTypeError("this is no AsyncDisposableStack instance");
	return C_NULL;
}
void fx_AsyncDisposableStack(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	txSlot* function;
	txSlot* home;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: AsyncDisposableStack");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxAsyncDisposableStackPrototype);
	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = instance;
	mxPullSlot(mxResult);
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_ASYNC_DISPOSABLE_STACK_KIND;
	property->value.disposableStack.stack = C_NULL;
	property->value.disposableStack.disposed = 0;
	
    property = fxNextUndefinedProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG);
	function = fxNewHostFunction(the, fxAsyncDisposableStackResolve, 1, XS_NO_ID, mxAsyncGeneratorResolveYieldProfileID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
	function = fxNewHostFunction(the, fxAsyncDisposableStackReject, 1, XS_NO_ID, mxAsyncGeneratorRejectYieldProfileID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
    property = fxNextUndefinedProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG);
    property = fxNextUndefinedProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG);
}

void fx_AsyncDisposableStack_prototype_get_disposed(txMachine* the)
{
	txSlot* instance = fxCheckAsyncDisposableStackInstance(the, mxThis, 0, 0);
	txSlot* property = instance->next;
	mxResult->value.boolean = property->value.disposableStack.disposed;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_AsyncDisposableStack_prototype_adopt(txMachine* the)
{
	txSlot* instance = fxCheckAsyncDisposableStackInstance(the, mxThis, 1, 1);
	txSlot* property = instance->next;
	if (mxArgc > 0) 
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	if (mxArgc > 1) 
		mxPushSlot(mxArgv(1));
	else
		mxPushUndefined();
	fxAsyncDisposableStackPush(the, property);
	property->value.disposableStack.stack->flag |= XS_BASE_FLAG;
	mxPop();
	mxPullSlot(mxResult);
}

void fx_AsyncDisposableStack_prototype_defer(txMachine* the)
{
	txSlot* instance = fxCheckAsyncDisposableStackInstance(the, mxThis, 1, 1);
	txSlot* property = instance->next;
	mxPushUndefined();
	if (mxArgc > 0) 
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	fxAsyncDisposableStackPush(the, property);
	mxPop();
	mxPop();
}	

void fx_AsyncDisposableStack_prototype_disposeAsync(txMachine* the)
{
	txSlot* promise;
	txSlot* resolveFunction;
	txSlot* rejectFunction;

	mxPush(mxPromisePrototype);
	promise = fxNewPromiseInstance(the);
	mxPullSlot(mxResult);
	mxPromiseStatus(promise)->value.integer = mxPendingStatus;
	fxPushPromiseFunctions(the, promise);
	resolveFunction = the->stack + 1;
	rejectFunction = the->stack;
	mxTry(the) {
		txSlot* instance = fxCheckAsyncDisposableStackInstance(the, mxThis, 1, 0);
		txSlot* property = instance->next;
		if (property->value.disposableStack.disposed) {
			mxPushUndefined();
			mxPushSlot(resolveFunction);
			mxCall();
			mxPushUndefined();
			mxRunCount(1);
			mxPop();
		}
		else {
			txSlot* exception = property->next;
			txSlot* resolveStepFunction = exception->next;
			txSlot* rejectStepFunction = resolveStepFunction->next;
			property->value.disposableStack.disposed = 1;
			property = rejectStepFunction->next;
			property->kind = resolveFunction->kind;
			property->value = resolveFunction->value;
			property = property->next;
			property->kind = rejectFunction->kind;
			property->value = rejectFunction->value;
			mxPushUndefined();
			mxPushSlot(resolveStepFunction);
			mxCall();
			mxPushUndefined();
			mxRunCount(1);
			mxPop();
		}
	}
	mxCatch(the) {
		fxRejectException(the, rejectFunction);
	}
}

void fx_AsyncDisposableStack_prototype_move(txMachine* the)
{
	txSlot* instance = fxCheckAsyncDisposableStackInstance(the, mxThis, 1, 1);
	txSlot* property = instance->next;
	txSlot* resultInstance;
	txSlot* resultProperty;
	txSlot* function;
	txSlot* home;
	mxPush(mxAsyncDisposableStackConstructor);
	fxGetPrototypeFromConstructor(the, &mxAsyncDisposableStackPrototype);
	resultInstance = fxNewSlot(the);
	resultInstance->kind = XS_INSTANCE_KIND;
	resultInstance->value.instance.garbage = C_NULL;
	resultInstance->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = resultInstance;
	mxPullSlot(mxResult);
	resultProperty = resultInstance->next = fxNewSlot(the);
	resultProperty->flag = XS_INTERNAL_FLAG;
	resultProperty->kind = XS_ASYNC_DISPOSABLE_STACK_KIND;
	resultProperty->value.disposableStack.stack = property->value.disposableStack.stack;
	resultProperty->value.disposableStack.disposed = 0;
	
    resultProperty = fxNextUndefinedProperty(the, resultProperty, XS_NO_ID, XS_INTERNAL_FLAG);
	function = fxNewHostFunction(the, fxAsyncDisposableStackResolve, 1, XS_NO_ID, mxAsyncGeneratorResolveYieldProfileID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = resultInstance;
    resultProperty = fxNextSlotProperty(the, resultProperty, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
	function = fxNewHostFunction(the, fxAsyncDisposableStackReject, 1, XS_NO_ID, mxAsyncGeneratorRejectYieldProfileID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = resultInstance;
    resultProperty = fxNextSlotProperty(the, resultProperty, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
    resultProperty = fxNextUndefinedProperty(the, resultProperty, XS_NO_ID, XS_INTERNAL_FLAG);
    resultProperty = fxNextUndefinedProperty(the, resultProperty, XS_NO_ID, XS_INTERNAL_FLAG);
	
	property->value.disposableStack.stack = C_NULL;
	property->value.disposableStack.disposed = 1;
}

void fx_AsyncDisposableStack_prototype_use(txMachine* the)
{
	txSlot* instance = fxCheckAsyncDisposableStackInstance(the, mxThis, 1, 1);
	txSlot* property = instance->next;
	txSlot* resource;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	resource = the->stack;
	if (!mxIsNull(resource) && !mxIsUndefined(resource)) {
		mxPushSlot(resource);
		mxGetID(mxID(_Symbol_asyncDispose));
		if (!fxIsCallable(the, the->stack)) {
			mxPop();
			mxPushSlot(resource);
			mxGetID(mxID(_Symbol_dispose));
		}
		fxAsyncDisposableStackPush(the, property);
		mxPop();
	}
	mxPullSlot(mxResult);
}

void fxAsyncDisposableStackPush(txMachine* the, txSlot* property)
{
	txSlot* dispose = the->stack;
	txSlot* resource = dispose + 1;
	txSlot** address = &property->value.disposableStack.stack;
	txSlot* slot;
	if (!fxIsCallable(the, dispose))
		mxTypeError("dispose is no function");
		
	slot = fxNewSlot(the);
	slot->next = *address;
	slot->kind = resource->kind;
	slot->value = resource->value;
	*address = slot;
	
	slot = fxNewSlot(the);
	slot->next = *address;
	slot->kind = dispose->kind;
	slot->value = dispose->value;
	*address = slot;
}

void fxAsyncDisposableStackReject(txMachine* the)
{
	txSlot* slot = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* instance = slot->value.home.object;
	txSlot* property = instance->next;
	txSlot* exception = property->next;
	
	if (mxIsUndefined(exception)) {
		mxPushSlot(mxArgv(0));
		mxPullSlot(exception);
	}
	else {
		mxPush(mxSuppressedErrorConstructor);
		mxNew();
		mxPushSlot(mxArgv(0));
		mxPushSlot(exception);
		mxRunCount(2);
		mxPullSlot(exception);
	}
	fxAsyncDisposableStackResolve(the);
}

void fxAsyncDisposableStackResolve(txMachine* the)
{
	txSlot* slot = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* instance = slot->value.home.object;
	txSlot* property = instance->next;
	txSlot* exception = property->next;
	txSlot* resolveStepFunction = exception->next;
	txSlot* rejectStepFunction = resolveStepFunction->next;
	txSlot* resolveFunction = rejectStepFunction->next;
	txSlot* rejectFunction = resolveFunction->next;

	slot = property->value.disposableStack.stack;
	if (slot) {
		txSlot* dispose = slot;
		txSlot* resource = slot->next;
		property->value.disposableStack.stack = resource->next;
		mxTry(the) {
			mxPushUndefined();
			mxPush(mxPromiseConstructor);
			if (dispose->flag & XS_BASE_FLAG) {
				mxPushUndefined();
				mxPushSlot(dispose);
				mxCall();
				mxPushSlot(resource);
				mxRunCount(1);
			}
			else {
				mxPushSlot(resource);
				mxPushSlot(dispose);
				mxCall();
				mxRunCount(0);
			}
			fx_Promise_resolveAux(the);
			mxPop();
			mxPop();
			fxPromiseThen(the, the->stack->value.reference, resolveStepFunction, rejectStepFunction, C_NULL, C_NULL);
			mxPop();
		}
		mxCatch(the) {
			mxPushUndefined();
			mxPushSlot(rejectStepFunction);
			mxCall();
			mxPush(mxException);
			mxException = mxUndefined;
			mxRunCount(1);
			mxPop();
		}
	}
	else {
		if (mxIsUndefined(exception)) {
			mxPushUndefined();
			mxPushSlot(resolveFunction);
			mxCall();
			mxPushUndefined();
			mxRunCount(1);
			mxPop();
		}
		else {
			mxPushUndefined();
			mxPushSlot(rejectFunction);
			mxCall();
			mxPushSlot(exception);
			mxRunCount(1);
			mxPop();
		}
	}
}

#endif





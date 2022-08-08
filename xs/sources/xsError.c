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
						if (key) {
							if (key->kind == XS_KEY_X_KIND)
								slot->kind = XS_KEY_X_KIND;
							slot->value.key.string = key->value.key.string;
						}
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

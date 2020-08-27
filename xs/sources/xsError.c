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

static txSlot* fx_Error_aux(txMachine* the, txSlot* prototype, txInteger i);

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
	mxErrorPrototype = *the->stack;
	prototype = fxBuildHostConstructor(the, mxCallback(fx_Error), 1, mxID(_Error));
	mxErrorConstructor = *the->stack;
	the->stack++;
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "AggregateError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxAggregateErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_AggregateError), 2, mxID(_AggregateError));
	instance->value.instance.prototype = prototype;
	mxAggregateErrorConstructor = *the->stack;
	the->stack++;
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "EvalError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxEvalErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_EvalError), 1, mxID(_EvalError));
	instance->value.instance.prototype = prototype;
	mxEvalErrorConstructor = *the->stack;
	the->stack++;
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "RangeError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxRangeErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_RangeError), 1, mxID(_RangeError));
	instance->value.instance.prototype = prototype;
	mxRangeErrorConstructor = *the->stack;
	the->stack++;
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "ReferenceError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxReferenceErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_ReferenceError), 1, mxID(_ReferenceError));
	instance->value.instance.prototype = prototype;
	mxReferenceErrorConstructor = *the->stack;
	the->stack++;
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "SyntaxError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxSyntaxErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_SyntaxError), 1, mxID(_SyntaxError));
	instance->value.instance.prototype = prototype;
	mxSyntaxErrorConstructor = *the->stack;
	the->stack++;
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "TypeError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxTypeErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_TypeError), 1, mxID(_TypeError));
	instance->value.instance.prototype = prototype;
	mxTypeErrorConstructor = *the->stack;
	the->stack++;
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "URIError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxURIErrorPrototype = *the->stack;
	instance = fxBuildHostConstructor(the, mxCallback(fx_URIError), 1, mxID(_URIError));
	instance->value.instance.prototype = prototype;
	mxURIErrorConstructor = *the->stack;
	the->stack++;
}

void fx_Error(txMachine* the)
{
	fx_Error_aux(the, &mxErrorPrototype, 0);
}

txSlot* fx_Error_aux(txMachine* the, txSlot* prototype, txInteger i)
{
	txSlot* instance;
	txSlot* slot;
	if (mxIsUndefined(mxTarget))
		mxPushSlot(mxFunction);
	else
		mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, prototype);
	instance = fxNewObjectInstance(the);
	mxPullSlot(mxResult);
	slot = instance->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG | XS_GET_ONLY;
	slot->kind = XS_ERROR_KIND;
	if ((mxArgc > i) && (mxArgv(i)->kind != XS_UNDEFINED_KIND)) {
		fxToString(the, mxArgv(i));
		slot = fxNextSlotProperty(the, slot, mxArgv(i), mxID(_message), XS_DONT_ENUM_FLAG);
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
	fxGetID(the, mxID(_name));
	if (the->stack->kind == XS_UNDEFINED_KIND) 
		fxStringX(the, the->stack, "Error");
	else	
		fxToString(the, the->stack);
	name = the->stack;
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_message));
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
	txSlot* property = fx_Error_aux(the, &mxAggregateErrorPrototype, 1);
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
	fx_Error_aux(the, &mxEvalErrorPrototype, 0);
}

void fx_RangeError(txMachine* the)
{
	fx_Error_aux(the, &mxRangeErrorPrototype, 0);
}

void fx_ReferenceError(txMachine* the)
{
	fx_Error_aux(the, &mxReferenceErrorPrototype, 0);
}

void fx_SyntaxError(txMachine* the)
{
	fx_Error_aux(the, &mxSyntaxErrorPrototype, 0);
}

void fx_TypeError(txMachine* the)
{
	fx_Error_aux(the, &mxTypeErrorPrototype, 0);
}

void fx_URIError(txMachine* the)
{
	fx_Error_aux(the, &mxURIErrorPrototype, 0);
}


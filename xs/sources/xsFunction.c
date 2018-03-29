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

static txSlot* fxCheckFunctionInstance(txMachine* the, txSlot* slot);
static void fxResolveAwait(txMachine* the);
static void fxRejectAwait(txMachine* the);
static void fxStepAsync(txMachine* the, txSlot* instance, txFlag status);

void fxBuildFunction(txMachine* the)
{
	txSlot* slot;
	txSlot* function;
	txSlot* constructor;
	mxPush(mxFunctionPrototype);
	slot = fxLastProperty(the, the->stack->value.reference);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Function_prototype_apply), 2, mxID(_apply), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Function_prototype_bind), 1, mxID(_bind), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Function_prototype_call), 1, mxID(_call), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Function_prototype_toString), 0, mxID(_toString), XS_DONT_ENUM_FLAG);
    slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Function_prototype_hasInstance), 1, mxID(_Symbol_hasInstance), XS_GET_ONLY);
	function = mxThrowTypeErrorFunction.value.reference;
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_DONT_ENUM_FLAG;
	slot->ID = mxID(_arguments);
	slot->kind = XS_ACCESSOR_KIND;
	slot->value.accessor.getter = function;
	slot->value.accessor.setter = function;
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_DONT_ENUM_FLAG;
	slot->ID = mxID(_caller);
	slot->kind = XS_ACCESSOR_KIND;
	slot->value.accessor.getter = function;
	slot->value.accessor.setter = function;
	slot = fxNextSlotProperty(the, slot, &mxEmptyString, mxID(_name), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	constructor = fxNewHostConstructorGlobal(the, mxCallback(fx_Function), 1, mxID(_Function), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	mxPush(mxFunctionPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "AsyncFunction", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxAsyncFunctionPrototype = *the->stack;
	slot = fxNewHostConstructor(the, mxCallback(fx_AsyncFunction), 1, mxID(_AsyncFunction));
	slot->value.instance.prototype = constructor;
	the->stack++;
	slot = mxBehaviorGetProperty(the, mxAsyncFunctionPrototype.value.reference, mxID(_constructor), XS_NO_ID, XS_OWN);
	slot->flag |= XS_DONT_SET_FLAG;
}

void fxCheckCallable(txMachine* the, txSlot* slot)
{
	if (fxIsCallable(the, slot))
		return;
	mxTypeError("this is no Function instance");
}

txSlot* fxCheckFunctionInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if (fxIsFunction(the, slot))
			return slot;
	}
	mxTypeError("this is no Function instance");
	return C_NULL;
}

txBoolean fxIsCallable(txMachine* the, txSlot* slot) 
{
	if (slot->kind == XS_REFERENCE_KIND)
		return fxIsFunction(the, slot->value.reference);
#ifdef mxHostFunctionPrimitive
	if (slot->kind == XS_HOST_FUNCTION_KIND)
		return 1;
#endif
	return 0;
}

txBoolean fxIsFunction(txMachine* the, txSlot* instance)
{
again:
	if (instance) {
		txSlot* exotic = instance->next;
		if (exotic && (exotic->flag & XS_INTERNAL_FLAG)) {
			if (((exotic->kind == XS_CALLBACK_KIND) || (exotic->kind == XS_CALLBACK_X_KIND) || (exotic->kind == XS_CODE_KIND) || (exotic->kind == XS_CODE_X_KIND)))
				return 1;
			if (exotic->kind == XS_PROXY_KIND) {
				instance = exotic->value.proxy.target;
				goto again;
			}
		}
	}
	return 0;
}

txSlot* fxNewFunctionInstance(txMachine* the, txID name)
{
	txSlot* instance;
	txSlot* property;

	instance = fxNewObjectInstance(the);

	/* CODE */
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = mxEmptyCode.kind;
	property->value.code.address = mxEmptyCode.value.code.address;
	property->value.code.closures = C_NULL;

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
	property->value.integer = 0;
#endif
		
	/* NAME */
	if (name != XS_NO_ID)
		fxRenameFunction(the, instance, name, XS_NO_ID, C_NULL);

	return instance;
}

void fxDefaultFunctionPrototype(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = the->stack->value.reference;
	instance->flag |= XS_CAN_CONSTRUCT_FLAG;
	property = fxLastProperty(the, instance);
	mxPush(mxObjectPrototype);
	instance = fxNewObjectInstance(the);
	fxNextSlotProperty(the, property, the->stack, mxID(_prototype), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	the->stack++;
	fxNextSlotProperty(the, instance, the->stack, mxID(_constructor), XS_DONT_ENUM_FLAG);
}

txSlot* fxGetPrototypeFromConstructor(txMachine* the, txSlot* defaultPrototype)
{
	fxCheckCallable(the, the->stack);
	fxGetID(the, mxID(_prototype));
	if (!mxIsReference(the->stack)) {
		the->stack->kind = defaultPrototype->kind;
		the->stack->value = defaultPrototype->value;
	}
	return the->stack->value.reference;
}

void fxRenameFunction(txMachine* the, txSlot* instance, txInteger id, txInteger former, txString prefix)
{
	txSlot* property;
#ifndef mxNoFunctionName
	txSlot* key;
#endif
	if (instance->flag & XS_MARK_FLAG)
		return;
	property = mxFunctionInstanceCode(instance);
	if ((property->ID == XS_NO_ID) || (property->ID == former))
		property->ID = (txID)id;
	else
		return;
#ifndef mxNoFunctionName
	property = mxBehaviorGetProperty(the, instance, mxID(_name), XS_NO_ID, XS_OWN);
	if (property)
		return;
	property = fxNextSlotProperty(the, fxLastProperty(the, instance), &mxEmptyString, mxID(_name), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	key = fxGetKey(the, (txID)id);
	if (key) {
		txKind kind = mxGetKeySlotKind(key);
		if (kind == XS_KEY_KIND) {
			property->kind = XS_STRING_KIND;
			property->value.string = key->value.key.string;
		}
		else if (kind == XS_KEY_X_KIND) {
			property->kind = XS_STRING_X_KIND;
			property->value.string = key->value.key.string;
		}
		else if ((kind == XS_STRING_KIND) || (kind == XS_STRING_X_KIND)) {
			property->kind = kind;
			property->value.string = key->value.string;
			fxAdornStringC(the, "[", property, "]");
		}
		else {
			property->kind = mxEmptyString.kind;
			property->value = mxEmptyString.value;
		}
	}
	else {
		property->kind = mxEmptyString.kind;
		property->value = mxEmptyString.value;
	}
	if (prefix) 
		fxAdornStringC(the, prefix, property, C_NULL);
#endif
}

void fx_Function(txMachine* the)
{	
#ifdef mxParse
	txInteger c, i;
	txStringStream stream;
	
	c = mxArgc;
	i = 0;
	mxPushStringX("(function anonymous(");
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
		fxGetPrototypeFromConstructor(the, &mxFunctionPrototype);
		mxResult->value.reference->value.instance.prototype = the->stack->value.reference;
		mxPop();
	}
#else
	mxUnknownError("not built-in.");
#endif
}

void fx_Function_prototype_apply(txMachine* the)
{
	txInteger c, i;
	fxCheckCallable(the, mxThis);
	if ((mxArgc < 2) || (mxArgv(1)->kind == XS_UNDEFINED_KIND) || (mxArgv(1)->kind == XS_NULL_KIND))
		c = 0;
	else {
		if (mxArgv(1)->kind != XS_REFERENCE_KIND)
			mxTypeError("argArray is no object");
		fxToInstance(the, mxArgv(1));
		mxPushSlot(mxArgv(1));
		fxGetID(the, mxID(_length));
		c = fxToInteger(the, the->stack);
		the->stack++;
		for (i = 0; i < c; i++) {
			mxPushSlot(mxArgv(1));
			fxGetID(the, (txID)i);
		}
	}
	/* ARGC */
	mxPushInteger(c);
	/* THIS */
	if (mxArgc < 1)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	/* FUNCTION */
	mxPushSlot(mxThis);
	fxCall(the);
	mxPullSlot(mxResult);
}

void fx_Function_prototype_bind(txMachine* the)
{
	txSlot* function = fxToInstance(the, mxThis);
	txSlot* instance;
#ifndef mxNoFunctionLength
	txSize length;
#endif
	txSlot* slot;
	txSlot* arguments;
	txSlot* argument;
	txSize c = mxArgc, i;

	fxCheckCallable(the, mxThis);
    mxPushReference(function->value.instance.prototype);
    instance = fxNewFunctionInstance(the, XS_NO_ID);
    instance->flag |= function->flag & XS_CAN_CONSTRUCT_FLAG;
    mxPullSlot(mxResult);
    
	slot = mxFunctionInstanceCode(instance);
	slot->kind = XS_CALLBACK_KIND;
	slot->value.callback.address = fx_Function_prototype_bound;
	slot->value.callback.IDs = C_NULL;
	
#ifndef mxNoFunctionLength
	mxPushSlot(mxThis);
    mxPushUndefined();
	if (mxBehaviorGetOwnProperty(the, mxThis->value.reference, mxID(_length), XS_NO_ID, the->stack)) {
		mxPushSlot(mxThis);
		fxGetID(the, mxID(_length));
		length = fxToInteger(the, the->stack++);
		if (c > 1)
			length -= c - 1;
		if (length < 0)
			length = 0;
		mxPop();
	}
	else
		length = 0;
	mxPop();
	slot = mxFunctionInstanceLength(instance);
	slot->value.integer = length;
#endif

	slot = fxLastProperty(the, instance);
	
#ifndef mxNoFunctionName
	mxPushStringX("bound ");
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_name));
	if ((the->stack->kind == XS_STRING_KIND) || (the->stack->kind == XS_STRING_X_KIND))
		fxConcatString(the, the->stack + 1, the->stack);
	mxPop();
	slot = fxNextSlotProperty(the, slot, the->stack, mxID(_name), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
#endif

	slot = fxNextSlotProperty(the, slot, mxThis, mxID(_boundFunction), XS_GET_ONLY);
	if (c > 0)
		slot = fxNextSlotProperty(the, slot, mxArgv(0), mxID(_boundThis), XS_GET_ONLY);
	else
		slot = fxNextUndefinedProperty(the, slot, mxID(_boundThis), XS_GET_ONLY);
	
	if (c > 1) {
		mxPush(mxArrayPrototype);
		arguments = fxNewArrayInstance(the);
		argument = fxLastProperty(the, arguments);
		for (i = 1; i < c; i++) {
			argument->next = fxNewSlot(the);
			argument = argument->next;
			argument->kind = mxArgv(i)->kind;
			argument->value = mxArgv(i)->value;
		}
		arguments->next->value.array.length = c - 1;
		fxCacheArray(the, arguments);
		slot = fxNextSlotProperty(the, slot, the->stack, mxID(_boundArguments), XS_GET_ONLY);
		mxPop();
	}
}

void fx_Function_prototype_bound(txMachine* the)
{
	txSlot* boundArguments;
	txInteger c, i;
	txSlot* argument;
	mxPush(*mxFunction);
	fxGetID(the, mxID(_boundArguments));
	if (the->stack->kind == XS_REFERENCE_KIND) {
		boundArguments = fxGetInstance(the, the->stack);
		mxPop();
		c = boundArguments->next->value.array.length;
		argument = boundArguments->next->value.array.address;
		for (i = 0; i < c; i++) {
			mxPushSlot(argument);
			argument++;
		}
	}
	else {
		mxPop();
		c = 0;
	}
	for (i = 0; i < mxArgc; i++)
		mxPushSlot(mxArgv(i));
	/* ARGC */
	mxPushInteger(c + i);
	/* THIS */
	if (mxTarget->kind == XS_UNDEFINED_KIND) {
		mxPushSlot(mxFunction);
		fxGetID(the, mxID(_boundThis));
	}
	else
		mxPushUninitialized();
	/* FUNCTION */
	mxPushSlot(mxFunction);
	fxGetID(the, mxID(_boundFunction));
	/* TARGET */
	if (fxIsSameSlot(the, mxFunction, mxTarget)) {
		txSlot* slot = the->stack;
		mxPushSlot(slot);
	}
	else
		mxPushSlot(mxTarget);
	/* RESULT */
	mxPushUndefined();
	fxRunID(the, C_NULL, XS_NO_ID);
	mxPullSlot(mxResult);
}

void fx_Function_prototype_call(txMachine* the)
{	
	txInteger c, i;
	fxCheckCallable(the, mxThis);
	c = mxArgc;
	i = 1;
	while (i < c) {
		mxPushSlot(mxArgv(i));
		i++;
	}
	/* ARGC */
	mxPushInteger(i - 1);
	/* THIS */
	if (mxArgc < 1)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	/* FUNCTION */
	mxPushSlot(mxThis);
	fxCall(the);
	mxPullSlot(mxResult);
}

void fx_Function_prototype_hasInstance(txMachine* the)
{	
	txSlot* instance;
	txSlot* prototype;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc == 0)
		return;
	instance = fxGetInstance(the, mxArgv(0));
	if (!instance)
		return;
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_prototype));
	prototype = fxGetInstance(the, the->stack);
	mxPop();
	if (!prototype) {
		mxPushSlot(mxThis);
		fxGetID(the, mxID(_boundFunction));
		if (mxIsReference(the->stack)) {
			fxGetID(the, mxID(_prototype));
			prototype = fxGetInstance(the, the->stack);
		}
		mxPop();
	}
	if (!prototype)
		mxTypeError("prototype is no object");
	mxPushNull();
	while (mxBehaviorGetPrototype(the, instance, the->stack)) {
		instance = the->stack->value.reference;
		if (instance == prototype) {
			mxResult->value.boolean = 1;
			break;
		}
	}
	mxPop();
}

void fx_Function_prototype_toString(txMachine* the)
{	
	fxCheckFunctionInstance(the, mxThis);
	mxPushStringX("@ \"");
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_name));
	if ((the->stack->kind == XS_STRING_KIND) || (the->stack->kind == XS_STRING_X_KIND))
		fxConcatString(the, the->stack + 1, the->stack);
	mxPop();
	mxPushStringX("\"");
	fxConcatString(the, the->stack + 1, the->stack);
	mxPop();
	mxPullSlot(mxResult);
}

txSlot* fxNewAsyncInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	txSlot* promise;
	txSlot* status;
	txSlot* already;
	txSlot* function;
	txSlot* home;
	
	mxPushUndefined();

	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = C_NULL;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;

	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_STACK_KIND;
	property->ID = XS_NO_ID;
	property->value.stack.length = 0;
	property->value.stack.address = C_NULL;

	mxPush(mxPromisePrototype);
	promise = fxNewPromiseInstance(the);
	status = mxPromiseStatus(promise);
	status->value.integer = mxPendingStatus;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPop();
	
    property = fxNextIntegerProperty(the, property, -1, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	already = fxNewPromiseAlready(the);
	fxNewPromiseFunction(the, already, promise, mxResolvePromiseFunction.value.reference);
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPop();
	fxNewPromiseFunction(the, already, promise, mxRejectPromiseFunction.value.reference);
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPop();
	mxPop(); // already
	
	function = fxNewHostFunction(the, fxResolveAwait, 1, XS_NO_ID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPop();
	
	function = fxNewHostFunction(the, fxRejectAwait, 1, XS_NO_ID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPop();
	
	return instance;
}

void fxResolveAwait(txMachine* the)
{
	txSlot* slot = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* instance = slot->value.home.object;
	the->scratch.kind = mxArgv(0)->kind;
	the->scratch.value = mxArgv(0)->value;
	fxStepAsync(the, instance, XS_NO_STATUS);
}

void fxRejectAwait(txMachine* the)
{
	txSlot* slot = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* instance = slot->value.home.object;
	the->scratch.kind = mxArgv(0)->kind;
	the->scratch.value = mxArgv(0)->value;
	fxStepAsync(the, instance, XS_THROW_STATUS);
}

void fxRunAsync(txMachine* the, txSlot* instance)
{
	txSlot* promise = instance->next->next;
	fxBeginHost(the);
	the->scratch.kind = XS_UNDEFINED_KIND;
	fxStepAsync(the, instance, XS_NO_STATUS);
	fxEndHost(the);
	mxResult->kind = promise->kind;
	mxResult->value = promise->value;
}

void fxRunAwait(txMachine* the, txSlot* instance)
{
	txSlot* promise = instance->next->next;
	txSlot* state = promise->next;
	txSlot* resolveFunction = state->next;
	txSlot* rejectFunction = resolveFunction->next;
	txSlot* resolveAwaitFunction = rejectFunction->next;
	txSlot* rejectAwaitFunction = resolveAwaitFunction->next;
	txSlot* value = the->stack;
	
	fxBeginHost(the);
	mxPushSlot(value);
	mxPushInteger(1);
	mxPush(mxPromiseConstructor);
	fxCallID(the, mxID(_resolve));
	mxPullSlot(value);
	mxPushSlot(resolveAwaitFunction);
	mxPushSlot(rejectAwaitFunction);
	mxPushInteger(2);
	mxPushSlot(value);
	fxCallID(the, mxID(_then));
	mxPop();
	fxEndHost(the);
}

void fxStepAsync(txMachine* the, txSlot* instance, txFlag status)
{
	txSlot* promise = instance->next->next;
	txSlot* state = promise->next;
	txSlot* resolveFunction = state->next;
	txSlot* rejectFunction = resolveFunction->next;
	mxTry(the) {
		the->status = status;
		state->value.integer = 1;
		fxRunID(the, instance, XS_NO_ID);
		if (state->value.integer) {
			/* COUNT */
			mxPushInteger(1);
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
			mxPushSlot(resolveFunction);
			fxCall(the);
		}
		mxPop();
	}
	mxCatch(the) {
		mxPush(mxException);
		/* COUNT */
		mxPushInteger(1);
		/* THIS */
		mxPushUndefined();
		/* FUNCTION */
		mxPushSlot(rejectFunction);
		fxCall(the);
		mxPop();
	}
}

void fx_AsyncFunction(txMachine* the)
{	
#ifdef mxParse
	txInteger c, i;
	txStringStream stream;
	
	c = mxArgc;
	i = 0;
	mxPushStringX("(async function anonymous(");
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
		fxGetPrototypeFromConstructor(the, &mxAsyncFunctionPrototype);
		mxResult->value.reference->value.instance.prototype = the->stack->value.reference;
		mxPop();
	}
#else
	mxUnknownError("not built-in.");
#endif
}

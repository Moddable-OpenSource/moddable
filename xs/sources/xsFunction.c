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
	constructor = fxBuildHostConstructor(the, mxCallback(fx_Function), 1, mxID(_Function));
	mxFunctionConstructor = *the->stack;
	mxPop();
	
	mxPush(mxFunctionPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "AsyncFunction", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxAsyncFunctionPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_AsyncFunction), 1, mxID(_AsyncFunction));
	slot->value.instance.prototype = constructor;
	mxPop();
	slot = mxBehaviorGetProperty(the, mxAsyncFunctionPrototype.value.reference, mxID(_constructor), 0, XS_OWN);
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
	instance->flag |= XS_CAN_CALL_FLAG;

	/* CODE */
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = mxEmptyCode.kind;
	property->value.code.address = mxEmptyCode.value.code.address;
	property->value.code.closures = C_NULL;

	/* HOME */
	property = property->next = fxNewSlot(the);
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
		gxDefaults.newFunctionLength(the, instance, 0);
		
	/* NAME */
	fxRenameFunction(the, instance, name, 0, XS_NO_ID, C_NULL);

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
	mxPop();
	fxNextSlotProperty(the, instance, the->stack, mxID(_constructor), XS_DONT_ENUM_FLAG);
}

txSlot* fxGetPrototypeFromConstructor(txMachine* the, txSlot* defaultPrototype)
{
	txSlot* result = the->stack;
	fxCheckCallable(the, result);
	mxDub();
	mxGetID(mxID(_prototype));
	if (!mxIsReference(the->stack)) {
		txSlot* instance = result->value.reference;
		txSlot* proxy = instance->next;
		if (proxy->kind == XS_PROXY_KIND) {
			if (!proxy->value.proxy.handler)
				mxTypeError("(proxy).%s: handler is no object", fxName(the, mxID(_prototype)));
			if (!proxy->value.proxy.target)
				mxTypeError("(proxy).%s: target is no object", fxName(the, mxID(_prototype)));
		}
		the->stack->kind = defaultPrototype->kind;
		the->stack->value = defaultPrototype->value;
	}
	mxPullSlot(result);
	return result->value.reference;
}

#ifndef mxLink
txSlot* fxNewFunctionLength(txMachine* the, txSlot* instance, txNumber length)
{
	txSlot* property = mxBehaviorGetProperty(the, instance, mxID(_length), 0, XS_OWN);
	if (!property)
		property = fxNextIntegerProperty(the, fxLastProperty(the, instance), 0, mxID(_length), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	if (length <= 0x7FFFFFFF) {
		property->kind = XS_INTEGER_KIND;
		property->value.integer = (txInteger)length;
	}
	else {
		property->kind = XS_NUMBER_KIND;
		property->value.number = length;
	}
	return property;
}

txSlot* fxNewFunctionName(txMachine* the, txSlot* instance, txID id, txIndex index, txID former, txString prefix)
{
	txSlot* property;
	txSlot* key;
	property = mxBehaviorGetProperty(the, instance, mxID(_name), 0, XS_OWN);
	if (property) {
		if ((property->kind != mxEmptyString.kind) || (property->value.string != mxEmptyString.value.string))
			return property;
	}
	else
		property = fxNextSlotProperty(the, fxLastProperty(the, instance), &mxEmptyString, mxID(_name), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	if (id != XS_NO_ID) {
		key = fxGetKey(the, (txID)id);
		if (key) {
			txKind kind = mxGetKeySlotKind(key);
			if (kind == XS_KEY_KIND) {
				property->kind = XS_STRING_KIND;
				property->value.string = key->value.key.string;
				if (!(key->flag & XS_DONT_ENUM_FLAG))
					fxAdornStringC(the, "[", property, "]");
			}
			else if (kind == XS_KEY_X_KIND) {
				property->kind = XS_STRING_X_KIND;
				property->value.string = key->value.key.string;
				if (!(key->flag & XS_DONT_ENUM_FLAG))
					fxAdornStringC(the, "[", property, "]");
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
	}
	else if (former) {
		char buffer[16];
		fxCopyStringC(the, property, fxNumberToString(the->dtoa, index, buffer, sizeof(buffer), 0, 0));	
	}
	if (prefix) 
		fxAdornStringC(the, prefix, property, C_NULL);
	return property;
}
#endif

void fxRenameFunction(txMachine* the, txSlot* instance, txID id, txIndex index, txID former, txString prefix)
{
	txSlot* property;
	if (instance->flag & XS_MARK_FLAG)
		return;
	property = mxFunctionInstanceCode(instance);
	if ((property->ID == XS_NO_ID) || (property->ID == former)) {
		if (id != XS_NO_ID)
			property->ID = (txID)id;
	}
	if (gxDefaults.newFunctionName)
		property = gxDefaults.newFunctionName(the, instance, id, index, former, prefix);
}

void fx_Function(txMachine* the)
{	
	txInteger c, i;
	txStringStream stream;
	txSlot* module = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.module;
	if (!module) module = mxProgram.value.reference;
	
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
	fxConcatStringC(the, the->stack, "\n){");
	if (c > 0) {
		fxToString(the, mxArgv(i));
		fxConcatString(the, the->stack, mxArgv(i));
	}
	fxConcatStringC(the, the->stack, "\n})");
	stream.slot = the->stack;
	stream.offset = 0;
	stream.size = mxStringLength(the->stack->value.string);
	fxRunScript(the, fxParseScript(the, &stream, fxStringGetter, mxProgramFlag | mxFunctionFlag), C_NULL, C_NULL, C_NULL, C_NULL, module);
	mxPullSlot(mxResult);
	if (!mxIsUndefined(mxTarget) && !fxIsSameSlot(the, mxTarget, mxFunction)) {
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxFunctionPrototype);
		mxResult->value.reference->value.instance.prototype = the->stack->value.reference;
		mxPop();
	}
}

void fx_Function_prototype_apply(txMachine* the)
{
	txIndex c, i;
	fxCheckCallable(the, mxThis);
	/* THIS */
	if (mxArgc < 1)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	/* FUNCTION */
	mxPushSlot(mxThis);
	mxCall();
	/* ARGUMENTS */
	if ((mxArgc < 2) || (mxArgv(1)->kind == XS_UNDEFINED_KIND) || (mxArgv(1)->kind == XS_NULL_KIND))
		c = 0;
	else {
		if (mxArgv(1)->kind != XS_REFERENCE_KIND)
			mxTypeError("argArray is no object");
		fxToInstance(the, mxArgv(1));
		mxPushSlot(mxArgv(1));
		mxGetID(mxID(_length));
		c = (txIndex)fxToLength(the, the->stack);
		mxPop();
		for (i = 0; i < c; i++) {
			mxPushSlot(mxArgv(1));
			mxGetIndex(i);
		}
	}
	mxRunCount(c);
	mxPullSlot(mxResult);
}

void fx_Function_prototype_bind(txMachine* the)
{
	txSlot* function = fxToInstance(the, mxThis);
	txSlot* instance;
	txSlot* property;
	txSlot* arguments;
	txSlot* argument;
	txSize c = mxArgc, i;

	fxCheckCallable(the, mxThis);
	mxPushNull();
	if (mxBehaviorGetPrototype(the, function, the->stack))
		instance = fxNewObjectInstance(the);
	else {
		mxPop();
		instance = fxNewInstance(the);
	}
	instance->flag |= function->flag & (XS_CAN_CALL_FLAG | XS_CAN_CONSTRUCT_FLAG);
    mxPullSlot(mxResult);
    	
	/* CODE */
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_CALLBACK_KIND;
	property->value.callback.address = fx_Function_prototype_bound;
	property->value.callback.closures = C_NULL;

	/* HOME */
	property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_HOME_KIND;
	property->value.home.object = C_NULL;
	property->value.home.module = C_NULL;

	property = fxNextSlotProperty(the, property, mxThis, mxID(_boundFunction), XS_INTERNAL_FLAG);
	if (c > 0)
		property = fxNextSlotProperty(the, property, mxArgv(0), mxID(_boundThis), XS_INTERNAL_FLAG);
	else
		property = fxNextUndefinedProperty(the, property, mxID(_boundThis), XS_INTERNAL_FLAG);
	
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
		property = fxNextSlotProperty(the, property, the->stack, mxID(_boundArguments), XS_INTERNAL_FLAG);
		mxPop();
	}
	else {
		property = fxNextNullProperty(the, property, mxID(_boundArguments), XS_INTERNAL_FLAG);
	}
	
	if (gxDefaults.newFunctionLength) {
		txNumber length = 0;
		mxPushUndefined();
		if (mxBehaviorGetOwnProperty(the, mxThis->value.reference, mxID(_length), 0, the->stack)) {
			mxPushSlot(mxThis);
			mxGetID(mxID(_length));
			property = the->stack;
			if (property->kind == XS_INTEGER_KIND) {
				length = property->value.integer;
			}
			else if (property->kind == XS_NUMBER_KIND) {
				length = property->value.number;
				if (c_isnan(length))
					length = 0;
				else
					length = c_trunc(length);
			}
			if (c > 1)
				length -= c - 1;
			if (length < 0)
				length = 0;
			mxPop();
		}
		mxPop();
		gxDefaults.newFunctionLength(the, instance, length);
	}
	
	if (gxDefaults.newFunctionName) {
		txSize length = 0;
		txString name;
		mxPushSlot(mxThis);
		mxGetID(mxID(_name));
		if ((the->stack->kind == XS_STRING_KIND) || (the->stack->kind == XS_STRING_X_KIND))
			length = mxStringLength(the->stack->value.string);
		property = fxNextSlotProperty(the, fxLastProperty(the, instance), &mxEmptyString, mxID(_name), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
		name = (txString)fxNewChunk(the, fxAddChunkSizes(the, length, 6 + 1));
		c_memcpy(name, "bound ", 6);
		if (length)
			c_memcpy(name + 6, the->stack->value.string, length);
		name[6 + length] = 0;
		property->value.string = name;
		property->kind = XS_STRING_KIND;
		mxPop();
	}
}

void fx_Function_prototype_bound(txMachine* the)
{
	txSlot* function = fxToInstance(the, mxFunction);
	txSlot* boundArguments;
	txInteger c, i;
	txSlot* argument;
	/* THIS */
	if (mxTarget->kind == XS_UNDEFINED_KIND) {
		mxPushSlot(mxFunctionInstanceHome(function)->next->next);
	}
	else
		mxPushUninitialized();
	/* FUNCTION */
	mxPushSlot(mxFunctionInstanceHome(function)->next);
	/* TARGET */
	if (fxIsSameSlot(the, mxFunction, mxTarget)) {
		txSlot* slot = the->stack;
		mxPushSlot(slot);
	}
	else
		mxPushSlot(mxTarget);
	/* RESULT */
	mxPushUndefined();
	mxPushUninitialized();
	mxPushUninitialized();
	/* ARGUMENTS */
	mxPushSlot(mxFunctionInstanceHome(function)->next->next->next);
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
	mxRunCount(c + i);
	mxPullSlot(mxResult);
}

void fx_Function_prototype_call(txMachine* the)
{	
	txInteger c, i;
	fxCheckCallable(the, mxThis);
	/* THIS */
	if (mxArgc < 1)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	/* FUNCTION */
	mxPushSlot(mxThis);
	mxCall();
	/* ARGUMENTS */
	c = mxArgc;
	i = 1;
	while (i < c) {
		mxPushSlot(mxArgv(i));
		i++;
	}
	mxRunCount(i - 1);
	mxPullSlot(mxResult);
}

void fx_Function_prototype_hasInstance(txMachine* the)
{	
	txSlot* function;
	txSlot* slot;
	txSlot* instance;
	txSlot* prototype;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (!fxIsCallable(the, mxThis))
		return;
	function = fxToInstance(the, mxThis);
	if (!function)
		return;
	if (mxIsFunction(function)) {
		slot = mxFunctionInstanceHome(function)->next;
		if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->ID == mxID(_boundFunction))) {
			if (!fxIsCallable(the, slot))
				return;
			function = fxToInstance(the, slot);
			if (!function)
				return;
		}
	}
	if (mxArgc == 0)
		return;
	instance = fxGetInstance(the, mxArgv(0));
	if (!instance)
		return;
	mxPushReference(function);
	mxGetID(mxID(_prototype));
	prototype = fxGetInstance(the, the->stack);
	mxPop();
	if (!prototype)
		mxTypeError("prototype is no object");
	if (prototype->ID) {
		txSlot* alias = the->aliasArray[prototype->ID];
		if (alias)
			prototype = alias;
	}
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
	mxPushStringX("function [\"");
	mxPushSlot(mxThis);
	mxGetID(mxID(_name));
	if ((the->stack->kind == XS_STRING_KIND) || (the->stack->kind == XS_STRING_X_KIND))
		fxConcatString(the, the->stack + 1, the->stack);
	mxPop();
	mxPushStringX("\"] (){[native code]}");
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
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_STACK_KIND;
	property->ID = XS_NO_ID;
	property->value.stack.length = 0;
	property->value.stack.address = C_NULL;
	
    property = fxNextIntegerProperty(the, property, XS_CODE_START_ASYNC, XS_NO_ID, XS_INTERNAL_FLAG);

	mxPush(mxPromisePrototype);
	promise = fxNewPromiseInstance(the);
	status = mxPromiseStatus(promise);
	status->value.integer = mxPendingStatus;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
	
	fxPushPromiseFunctions(the, promise);
    property = fxNextSlotProperty(the, property, the->stack + 1, XS_NO_ID, XS_INTERNAL_FLAG);
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
	mxPop();
	
	function = fxNewHostFunction(the, fxResolveAwait, 1, XS_NO_ID, mxResolveAwaitProfileID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
	
	function = fxNewHostFunction(the, fxRejectAwait, 1, XS_NO_ID, mxRejectAwaitProfileID);
	home = mxFunctionInstanceHome(function);
	home->value.home.object = instance;
    property = fxNextSlotProperty(the, property, the->stack, XS_NO_ID, XS_INTERNAL_FLAG);
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
	txSlot* promise = instance->next->next->next;
	fxBeginHost(the);
	the->scratch.kind = XS_UNDEFINED_KIND;
	fxStepAsync(the, instance, XS_NO_STATUS);
	fxEndHost(the);
	mxResult->kind = promise->kind;
	mxResult->value = promise->value;
}

void fxStepAsync(txMachine* the, txSlot* instance, txFlag status)
{
	txSlot* state = instance->next->next;
	txSlot* promise = state->next;
	txSlot* resolveFunction = promise->next;
	txSlot* rejectFunction = resolveFunction->next;
	txSlot* resolveAwaitFunction = rejectFunction->next;
	txSlot* rejectAwaitFunction = resolveAwaitFunction->next;
	txSlot* value;
	mxTry(the) {
		the->status = status;
		state->value.integer = XS_NO_CODE;
		fxRunID(the, instance, XS_NO_ID);
		value = the->stack;
		if (state->value.integer == XS_NO_CODE) {
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
			mxPushSlot(resolveFunction);
			mxCall();
			/* ARGUMENTS */
			mxPushSlot(value);
			mxRunCount(1);
			mxPop();
		}
		else {
			if (mxIsReference(value) && mxIsPromise(value->value.reference)) {
				mxDub();
				mxGetID(mxID(_constructor));
				if (fxIsSameValue(the, &mxPromiseConstructor, the->stack, 0)) {
					mxPop();
					fxPromiseThen(the, value->value.reference, resolveAwaitFunction, rejectAwaitFunction, C_NULL, C_NULL);
					goto exit;
				}
				mxPop();
			}
			mxTemporary(resolveFunction);
			mxTemporary(rejectFunction);
			mxPush(mxPromiseConstructor);
			fxNewPromiseCapability(the, resolveFunction, rejectFunction);
#ifdef mxPromisePrint
			fprintf(stderr, "fxStepAsync %d\n", the->stack->value.reference->next->ID);
#endif
			fxPromiseThen(the, the->stack->value.reference, resolveAwaitFunction, rejectAwaitFunction, C_NULL, C_NULL);
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
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
		fxRejectException(the, rejectFunction);
	}
}

void fx_AsyncFunction(txMachine* the)
{	
	txInteger c, i;
	txStringStream stream;
	txSlot* module = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.module;
	if (!module) module = mxProgram.value.reference;
	
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
	fxConcatStringC(the, the->stack, "\n){");
	if (c > 0) {
		fxToString(the, mxArgv(i));
		fxConcatString(the, the->stack, mxArgv(i));
	}
	fxConcatStringC(the, the->stack, "\n})");
	stream.slot = the->stack;
	stream.offset = 0;
	stream.size = mxStringLength(the->stack->value.string);
	fxRunScript(the, fxParseScript(the, &stream, fxStringGetter, mxProgramFlag | mxFunctionFlag), C_NULL, C_NULL, C_NULL, C_NULL, module);
	mxPullSlot(mxResult);
	if (!mxIsUndefined(mxTarget) && !fxIsSameSlot(the, mxTarget, mxFunction)) {
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxAsyncFunctionPrototype);
		mxResult->value.reference->value.instance.prototype = the->stack->value.reference;
		mxPop();
	}
}

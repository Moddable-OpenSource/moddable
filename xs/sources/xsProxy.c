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

static txSlot* fxNewProxyInstance(txMachine* the);
static txSlot* fxCheckProxyFunction(txMachine* the, txSlot* proxy, txID index);

static void fxProxyCall(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments);
static void fxProxyConstruct(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target);
static txBoolean fxProxyDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask);
static txBoolean fxProxyDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static txBoolean fxProxyGetOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot);
static txSlot* fxProxyGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
static txBoolean fxProxyGetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* receiver, txSlot* value);
static txBoolean fxProxyGetPrototype(txMachine* the, txSlot* instance, txSlot* result);
static txBoolean fxProxyHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static txBoolean fxProxyIsExtensible(txMachine* the, txSlot* instance);
static void fxProxyOwnKeys(txMachine* the, txSlot* instance, txFlag flag, txSlot* list);
static txBoolean fxProxyPreventExtensions(txMachine* the, txSlot* instance);
static txSlot* fxProxySetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
static txBoolean fxProxySetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver);
static txBoolean fxProxySetPrototype(txMachine* the, txSlot* instance, txSlot* prototype);

const txBehavior ICACHE_FLASH_ATTR gxProxyBehavior = {
	fxProxyGetProperty,
	fxProxySetProperty,
	fxProxyCall,
	fxProxyConstruct,
	fxProxyDefineOwnProperty,
	fxProxyDeleteProperty,
	fxProxyGetOwnProperty,
	fxProxyGetPropertyValue,
	fxProxyGetPrototype,
	fxProxyHasProperty,
	fxProxyIsExtensible,
	fxProxyOwnKeys,
	fxProxyPreventExtensions,
	fxProxySetPropertyValue,
	fxProxySetPrototype,
};

void fxBuildProxy(txMachine* the)
{
	txSlot* slot;

	fxNewHostFunction(the, mxCallback(fxProxyGetter), 0, XS_NO_ID, XS_NO_ID);
	fxNewHostFunction(the, mxCallback(fxProxySetter), 1, XS_NO_ID, XS_NO_ID);
	mxPushUndefined();
	the->stack->flag = XS_DONT_DELETE_FLAG;
	the->stack->kind = XS_ACCESSOR_KIND;
	the->stack->value.accessor.getter = (the->stack + 2)->value.reference;
	the->stack->value.accessor.setter = (the->stack + 1)->value.reference;
	mxPull(mxProxyAccessor);
	the->stack += 2;
	
	slot = fxBuildHostFunction(the, mxCallback(fx_Proxy), 2, mxID(_Proxy));
	slot->flag |= XS_CAN_CONSTRUCT_FLAG;
	mxProxyConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Proxy_revocable), 2, mxID(_revocable), XS_DONT_ENUM_FLAG);
	mxPop();

	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Reflect_apply), 3, mxID(_apply), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Reflect_construct), 2, mxID(_construct), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Reflect_defineProperty), 3, mxID(_defineProperty), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Reflect_deleteProperty), 2, mxID(_deleteProperty), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Reflect_get), 2, mxID(_get), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Reflect_getOwnPropertyDescriptor), 2, mxID(_getOwnPropertyDescriptor), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Reflect_getPrototypeOf), 1, mxID(_getPrototypeOf), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Reflect_has), 2, mxID(_has), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Reflect_isExtensible), 1, mxID(_isExtensible), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Reflect_ownKeys), 1, mxID(_ownKeys), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Reflect_preventExtensions), 1, mxID(_preventExtensions), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Reflect_set), 3, mxID(_set), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Reflect_setPrototypeOf), 2, mxID(_setPrototypeOf), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Reflect", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxReflectObject);
}

txSlot* fxNewProxyInstance(txMachine* the)
{
	txSlot* prototype;
	txSlot* instance;
	txSlot* property;
	txSlot* slot;
	
	prototype = mxIsReference(the->stack) ? the->stack->value.reference : C_NULL;
	
	instance = fxNewSlot(the);
	instance->flag = XS_EXOTIC_FLAG;
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = C_NULL;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = instance;

	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_PROXY_KIND;
	property->ID = XS_PROXY_BEHAVIOR;
	if (prototype && ((slot = prototype->next)) && (slot->kind = XS_PROXY_KIND)) {
		property->value.proxy.handler = slot->value.proxy.handler;
		property->value.proxy.target = slot->value.proxy.target;
	}
	else {
		property->value.proxy.handler = C_NULL;
		property->value.proxy.target = C_NULL;
    }
	
	return instance;
}

#define mxProxyDeclarations(ID) \
	txSlot* proxy = instance->next; \
	txSlot* function = fxCheckProxyFunction(the, proxy, ID); \
	txSlot* handler = the->stack + 1; \
	txSlot* target = the->stack + 2
	
#define mxProxyPop() \
	mxPop(); \
	mxPop(); \
	mxPop()

txSlot* fxCheckProxyFunction(txMachine* the, txSlot* proxy, txID index)
{
	txSlot* function;
	mxCheckCStack();
	if (!proxy->value.proxy.handler)
		mxTypeError("(proxy).%s: handler is no object", fxName(the, mxID(index)));
	if (!proxy->value.proxy.target)
		mxTypeError("(proxy).%s: target is no object", fxName(the, mxID(index)));
	mxPushReference(proxy->value.proxy.target);
	mxPushReference(proxy->value.proxy.handler);
	mxDub();
	mxGetID(mxID(index));
	function = the->stack;
	if (mxIsUndefined(function) || (mxIsNull(function)))
		function = C_NULL;
	else if (!fxIsCallable(the, function))
		mxTypeError("(proxy).%s: no function", fxName(the, mxID(index)));
	return function;
}

void fxProxyGetter(txMachine* the)
{
	txSlot* instance = fxToInstance(the, mxThis);
	while (instance) {
		if (mxIsProxy(instance))
			break;
		instance = fxGetPrototype(the, instance);
	}
	if (instance) {
		txID id = the->scratch.value.at.id;
		txIndex index = the->scratch.value.at.index;
		fxProxyGetPropertyValue(the, instance, id, index, mxThis, mxResult);
	}
}

void fxProxySetter(txMachine* the)
{
	txSlot* instance = fxToInstance(the, mxThis);
	while (instance) {
		if (mxIsProxy(instance))
			break;
		instance = fxGetPrototype(the, instance);
	}
	if (instance) {
		txID id = the->scratch.value.at.id;
		txIndex index = the->scratch.value.at.index;
		txBoolean result = fxProxySetPropertyValue(the, instance, id, index, mxArgv(0), mxThis);
        if (!result) {
            if (the->frame->next->flag & XS_STRICT_FLAG)
				mxTypeError("(proxy).set: not extensible or not writable");
        }
	}
}

void fxProxyCall(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments)
{
	mxProxyDeclarations(_apply);
	if (function) {
		/* THIS */
		mxPushSlot(handler);
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(target);
		mxPushSlot(_this);
		mxPushSlot(arguments);
		mxRunCount(3);
		mxPullSlot(mxResult);
	}
	else 
		mxBehaviorCall(the, target->value.reference, _this, arguments);
	mxProxyPop();
}

void fxProxyConstruct(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* newTarget)
{
	mxProxyDeclarations(_construct);
	if (function) {
		/* THIS */
		mxPushSlot(handler);
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(target);
		mxPushSlot(arguments);
		mxPushSlot(newTarget);
		mxRunCount(3);
		mxPullSlot(mxResult);
		if (!mxIsReference(mxResult))
			mxTypeError("(proxy).construct: no object");
	}
	else 
		mxBehaviorConstruct(the, target->value.reference, arguments, newTarget);
	mxProxyPop();
}

txBoolean fxProxyDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask)
{
	txBoolean result;
	mxProxyDeclarations(_defineProperty);
	if (function) {
		/* THIS */
		mxPushSlot(handler);
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(target);
		mxPushUndefined();
		fxKeyAt(the, id, index, the->stack);
		fxDescribeProperty(the, slot, mask);
		mxRunCount(3);
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (result) {
			mxPushUndefined();
			if (mxBehaviorGetOwnProperty(the, target->value.reference, id, index, the->stack)) {
				if (fxIsPropertyCompatible(the, the->stack, slot, mask)) {
					if ((mask & XS_DONT_DELETE_FLAG) && (slot->flag & XS_DONT_DELETE_FLAG)) {
						if (!(the->stack->flag & XS_DONT_DELETE_FLAG))
							mxTypeError("(proxy).defineProperty: true with non-configurable descriptor for configurable property");
					}
					if (the->stack->flag & XS_DONT_DELETE_FLAG) {
						if ((mask & XS_DONT_SET_FLAG) && (slot->flag & XS_DONT_SET_FLAG) && !(the->stack->flag & XS_DONT_SET_FLAG))
							mxTypeError("(proxy).defineProperty: true with non-writable descriptor for non-configurable writable property");
					}
				}
				else
					mxTypeError("(proxy).defineProperty: true with incompatible descriptor for existent property");
			}
			else if (mxBehaviorIsExtensible(the, target->value.reference)) {
				if ((mask & XS_DONT_DELETE_FLAG) && (slot->flag & XS_DONT_DELETE_FLAG))
					mxTypeError("(proxy).defineProperty: true with non-configurable descriptor for non-existent property");
			}
			else
				mxTypeError("(proxy).defineProperty: true with descriptor for non-existent property of non-extensible object");
			mxPop();
		}
	}
	else
		result = mxBehaviorDefineOwnProperty(the, target->value.reference, id, index, slot, mask);
	mxProxyPop();
	return result;
}

txBoolean fxProxyDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	txBoolean result;
	mxProxyDeclarations(_deleteProperty);
	if (function) {
		/* THIS */
		mxPushSlot(handler);
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(target);
		mxPushUndefined();
		fxKeyAt(the, id, index, the->stack);
		mxRunCount(2);
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (result) {
			mxPushUndefined();
			if (mxBehaviorGetOwnProperty(the, target->value.reference, id, index, the->stack)) {
				if (the->stack->flag & XS_DONT_DELETE_FLAG)
					mxTypeError("(proxy).deleteProperty: true for non-configurable property");
				if (!mxBehaviorIsExtensible(the, target->value.reference))
					mxTypeError("(proxy).deleteProperty: true for non-extensible object");
			}
			mxPop();
		}
	}
	else
		result = mxBehaviorDeleteProperty(the, target->value.reference, id, index);
	mxProxyPop();
	return result;
}

txBoolean fxProxyGetOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot)
{
	txBoolean result;
	mxProxyDeclarations(_getOwnPropertyDescriptor);
	if (function) {
		txFlag mask;
		/* THIS */
		mxPushSlot(handler);
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(target);
		mxPushUndefined();
		fxKeyAt(the, id, index, the->stack);
		mxRunCount(2);
		mxPullSlot(slot);
		mxPushUndefined();
		if (slot->kind == XS_UNDEFINED_KIND) {
			if (mxBehaviorGetOwnProperty(the, target->value.reference, id, index, the->stack)) {
				if (the->stack->flag & XS_DONT_DELETE_FLAG)
					mxTypeError("(proxy).getOwnPropertyDescriptor: no descriptor for non-configurable property");
				if (!mxBehaviorIsExtensible(the, target->value.reference)) 
					mxTypeError("(proxy).getOwnPropertyDescriptor: no descriptor for existent property of non-extensible object");
			}
			result = 0;
		}
		else {
			mask = fxDescriptorToSlot(the, slot);
			if (!(mask & XS_DONT_DELETE_FLAG)) {
				mask |= XS_DONT_DELETE_FLAG;
				slot->flag |= XS_DONT_DELETE_FLAG;
			}
			if (!(mask & XS_DONT_ENUM_FLAG)) {
				mask |= XS_DONT_ENUM_FLAG;
				slot->flag |= XS_DONT_ENUM_FLAG;
			}
			if (!(mask & (XS_GETTER_FLAG | XS_SETTER_FLAG))) {
				if (!(mask & XS_DONT_SET_FLAG)) {
					mask |= XS_DONT_SET_FLAG;
					slot->flag |= XS_DONT_SET_FLAG;
				}
				if (slot->kind == XS_UNINITIALIZED_KIND)
					slot->kind = XS_UNDEFINED_KIND;
			}
			if (mxBehaviorGetOwnProperty(the, target->value.reference, id, index, the->stack)) {
				if (fxIsPropertyCompatible(the, the->stack, slot, mask)) {
					if ((mask & XS_DONT_DELETE_FLAG) && (slot->flag & XS_DONT_DELETE_FLAG)) {
						if (!(the->stack->flag & XS_DONT_DELETE_FLAG))
							mxTypeError("(proxy).getOwnPropertyDescriptor: non-configurable descriptor for configurable property");
					}
					if (the->stack->flag & XS_DONT_DELETE_FLAG) {
						if ((mask & XS_DONT_SET_FLAG) && (slot->flag & XS_DONT_SET_FLAG) && !(the->stack->flag & XS_DONT_SET_FLAG))
							mxTypeError("(proxy).getOwnPropertyDescriptor: true with non-writable descriptor for non-configurable writable property");
					}
				}
				else
					mxTypeError("(proxy).getOwnPropertyDescriptor: incompatible descriptor for existent property");
			}
			else if (mxBehaviorIsExtensible(the, target->value.reference)) {
				if ((mask & XS_DONT_DELETE_FLAG) && (slot->flag & XS_DONT_DELETE_FLAG))
					mxTypeError("(proxy).getOwnPropertyDescriptor: non-configurable descriptor for non-existent property");
			}
			else
				mxTypeError("(proxy).getOwnPropertyDescriptor: descriptor for non-existent property of non-extensible object");
			result = 1;
		}
		mxPop();
	}
	else
		result = mxBehaviorGetOwnProperty(the, target->value.reference, id, index, slot);
	mxProxyPop();
	return result;
}

txSlot* fxProxyGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	the->scratch.value.at.id = id;
	the->scratch.value.at.index = index;
	return &mxProxyAccessor;
}

txBoolean fxProxyGetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* receiver, txSlot* slot)
{
	txBoolean result;
	mxProxyDeclarations(_get);
	if (function) {
		/* THIS */
		mxPushSlot(handler);
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(target);
		mxPushUndefined();
		fxKeyAt(the, id, index, the->stack);
		mxPushSlot(receiver);
		mxRunCount(3);
		mxPullSlot(slot);
		mxPushUndefined();
		if (mxBehaviorGetOwnProperty(the, target->value.reference, id, index, the->stack)) {
			txSlot* property = the->stack;
			if (property->flag & XS_DONT_DELETE_FLAG) {
				if (property->kind == XS_ACCESSOR_KIND) {
					if ((property->value.accessor.getter == C_NULL) && (slot->kind != XS_UNDEFINED_KIND))
						mxTypeError("(proxy).get: different getter for non-configurable property");
				}
				else {
					if ((property->flag & XS_DONT_SET_FLAG) && (!fxIsSameValue(the, property, slot, 0)))
						mxTypeError("(proxy).get: different value for non-configurable, non-writable property");
				}
			}
		}
		result = 1;
		mxPop();
	}
	else
		result = mxBehaviorGetPropertyValue(the, target->value.reference, id, index, receiver, slot);
	mxProxyPop();
	return result;
}

txBoolean fxProxyGetPrototype(txMachine* the, txSlot* instance, txSlot* slot)
{
	txBoolean result;
	mxProxyDeclarations(_getPrototypeOf);
	if (function) {
		/* THIS */
		mxPushSlot(handler);
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(target);
		mxRunCount(1);
		mxPullSlot(slot);
		if ((slot->kind == XS_NULL_KIND) ||  (slot->kind == XS_REFERENCE_KIND)) {
			if (!mxBehaviorIsExtensible(the, target->value.reference)) {
				mxPushUndefined();
				mxBehaviorGetPrototype(the, target->value.reference, the->stack);
				if (!fxIsSameValue(the, slot, the->stack, 0))
					mxTypeError("(proxy).getPrototypeOf: different prototype for non-extensible object");
				mxPop();
			}
		}
		else
			mxTypeError("(proxy).getPrototypeOf: neither object nor null");
		result = (slot->kind == XS_NULL_KIND) ? 0 : 1;
	}
	else
		result = mxBehaviorGetPrototype(the, target->value.reference, slot);
	mxProxyPop();
	return result;
}

txBoolean fxProxyHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	txBoolean result;
	mxProxyDeclarations(_has);
	if (function) {
		/* THIS */
		mxPushSlot(handler);
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(target);
		mxPushUndefined();
		fxKeyAt(the, id, index, the->stack);
		mxRunCount(2);
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (!result) {
			mxPushUndefined();
			if (mxBehaviorGetOwnProperty(the, target->value.reference, id, index, the->stack)) {
				if (the->stack->flag & XS_DONT_DELETE_FLAG)
					mxTypeError("(proxy).has: false for non-configurable property");
				 if (!mxBehaviorIsExtensible(the, target->value.reference)) 
					mxTypeError("(proxy).has: false for property of not extensible object");
			}
			mxPop();
		}
	}
	else
		result = mxBehaviorHasProperty(the, target->value.reference, id, index);
	mxProxyPop();
	return result;
}

txBoolean fxProxyIsExtensible(txMachine* the, txSlot* instance)
{
	txBoolean result;
	mxProxyDeclarations(_isExtensible);
	if (function) {
		/* THIS */
		mxPushSlot(handler);
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(target);
		mxRunCount(1);
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (mxBehaviorIsExtensible(the, target->value.reference)) {
			if (!result)
				mxTypeError("(proxy).isExtensible: false for extensible object");
		}
		else {
			if (result)
				mxTypeError("(proxy).isExtensible: true for non-extensible object");
		}
	}
	else
		result = mxBehaviorIsExtensible(the, target->value.reference);
	mxProxyPop();
	return result;
}

void fxProxyOwnKeys(txMachine* the, txSlot* instance, txFlag flag, txSlot* list) 
{
	mxProxyDeclarations(_ownKeys);
	if (function) {
		txIndex length;
		txSlot* reference;
		txSlot* item;
		txIndex index;
		txSlot* at;
		txBoolean test;
		txSlot* property;
		/* THIS */
		mxPushSlot(handler);
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(target);
		mxRunCount(1);
		reference = the->stack;
		mxPushSlot(reference);
		mxGetID(mxID(_length));
		length = fxToInteger(the, the->stack++);
		item = list;
		index = 0;
		while (index < length) {
			mxPushSlot(reference);
			mxGetIndex(index);
			at = the->stack;
			test = (at->kind == XS_SYMBOL_KIND) ? 1 : 0;
			if (test || (at->kind == XS_STRING_KIND) || (at->kind == XS_STRING_X_KIND)) {
				fxAt(the, at);
				property = list;
				while ((property = property->next)) {
					if ((at->value.at.id == property->value.at.id) && (at->value.at.index == property->value.at.index))
						break;
				}
				if (property)
					mxTypeError("(proxy).ownKeys: duplicate key");
				item = item->next = fxNewSlot(the);
				mxPullSlot(item);
				if (test)
					item->flag |= XS_INTERNAL_FLAG;
			}
			else
				mxTypeError("(proxy).ownKeys: key is neither string nor symbol");
			index++;
		}
		mxPop();
		
		test = mxBehaviorIsExtensible(the, target->value.reference) ? 1 : 0;
		at = fxNewInstance(the);
		mxBehaviorOwnKeys(the, target->value.reference, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, at);
		mxPushUndefined();
		property = the->stack;
		while ((at = at->next)) {
			mxBehaviorGetOwnProperty(the, target->value.reference, at->value.at.id, at->value.at.index, property);
			item = list;
			while ((item = item->next)) {
				if ((at->value.at.id == item->value.at.id) && (at->value.at.index == item->value.at.index)) {
					length--;
					break;
				}
			}
			if (!item) {
				if (property->flag & XS_DONT_DELETE_FLAG)
					mxTypeError("(proxy).ownKeys: no key for non-configurable property");
				if (!test)
					mxTypeError("(proxy).ownKeys: no key for property of non-extensible object");
			}
		}
		if (!test && length)
			mxTypeError("(proxy).ownKeys: key for non-existent property of non-extensible object");
		mxPop();
		mxPop();
		
		item = list;
		while ((property = item->next)) {
			if (property->flag & XS_INTERNAL_FLAG) {
				property->flag &= ~XS_INTERNAL_FLAG;
				if (flag & XS_EACH_SYMBOL_FLAG)
					item = property;
				else
					item->next = property->next;
			}
			else {
				if (flag & XS_EACH_NAME_FLAG)
					item = property;
				else
					item->next = property->next;
			}
		}
	}
	else
		mxBehaviorOwnKeys(the, target->value.reference, flag, list);
	mxProxyPop();
}

txBoolean fxProxyPreventExtensions(txMachine* the, txSlot* instance)
{
	txBoolean result;
	mxProxyDeclarations(_preventExtensions);
	if (function) {
		/* THIS */
		mxPushSlot(handler);
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(target);
		mxRunCount(1);
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (result) {
			if (mxBehaviorIsExtensible(the, target->value.reference))
				mxTypeError("(proxy).preventExtensions: true for extensible object");
		}
	}
	else
		result = mxBehaviorPreventExtensions(the, target->value.reference);
	mxProxyPop();
	return result;
}

txSlot* fxProxySetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	the->scratch.value.at.id = id;
	the->scratch.value.at.index = index;
	return &mxProxyAccessor;
}

txBoolean fxProxySetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txSlot* receiver)
{
	txBoolean result;
	mxProxyDeclarations(_set);
	if (function) {
		/* THIS */
		mxPushSlot(handler);
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(target);
		mxPushUndefined();
		fxKeyAt(the, id, index, the->stack);
		mxPushSlot(slot);
		mxPushSlot(receiver);
		mxRunCount(4);
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (result) {
			mxPushUndefined();
			if (mxBehaviorGetOwnProperty(the, target->value.reference, id, index, the->stack)) {
				txSlot* property = the->stack;
				if (property->flag & XS_DONT_DELETE_FLAG) {
					if (property->kind == XS_ACCESSOR_KIND) {
						if (property->value.accessor.setter == C_NULL)
							mxTypeError("(proxy).set: true for non-configurable property with different setter");
					}
					else {
						if ((property->flag & XS_DONT_SET_FLAG) && (!fxIsSameValue(the, property, slot, 0)))
							mxTypeError("(proxy).set: true for non-configurable, non-writable property with different value");
					}
				}
			}
			mxPop();
		}
	}
	else
		result = mxBehaviorSetPropertyValue(the, target->value.reference, id, index, slot, receiver);
	mxProxyPop();
	return result;
}

txBoolean fxProxySetPrototype(txMachine* the, txSlot* instance, txSlot* prototype)
{
	txBoolean result;
	mxProxyDeclarations(_setPrototypeOf);
	if (function) {
		/* THIS */
		mxPushSlot(handler);
		/* FUNCTION */
		mxPushSlot(function);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(target);
		mxPushSlot(prototype);
		mxRunCount(2);
		result = fxToBoolean(the, the->stack);
		mxPop();
		if (result) {
			if (!mxBehaviorIsExtensible(the, target->value.reference)) {
				mxPushUndefined();
				mxBehaviorGetPrototype(the, target->value.reference, the->stack);
				if (!fxIsSameValue(the, prototype, the->stack, 0))
					mxTypeError("(proxy).setPrototypeOf: true for non-extensible object with different prototype");
				mxPop();
			}
		}
	}
	else
		result = mxBehaviorSetPrototype(the, target->value.reference, prototype);
	mxProxyPop();
	return result;
}

void fx_Proxy(txMachine* the)
{
	txSlot* instance;
	txSlot* proxy;
	txSlot* target;
	txSlot* handler;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: Proxy");
	mxPushUndefined();
	instance = fxNewProxyInstance(the);
	mxPullSlot(mxResult);
	proxy = instance->next;
	if (!proxy || (proxy->kind != XS_PROXY_KIND))
		mxTypeError("this is no proxy");
#ifdef mxHostFunctionPrimitive
	if ((mxArgc > 0) && (mxArgv(0)->kind == XS_HOST_FUNCTION_KIND))
		fxToInstance(the, mxArgv(0));
	if ((mxArgc > 1) && (mxArgv(1)->kind == XS_HOST_FUNCTION_KIND))
		fxToInstance(the, mxArgv(1));
#endif
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	target = mxArgv(0)->value.reference;
	if ((mxArgc < 2) || (mxArgv(1)->kind != XS_REFERENCE_KIND))
		mxTypeError("handler is no object");
	handler = mxArgv(1)->value.reference;
	instance->flag |= target->flag & (XS_CAN_CALL_FLAG | XS_CAN_CONSTRUCT_FLAG);
	proxy->value.proxy.target = target;
	proxy->value.proxy.handler = handler;
}

void fx_Proxy_revocable(txMachine* the)
{
	txSlot* target;
	txSlot* handler;
	txSlot* property;
	txSlot* instance;
	txSlot* slot;
	
#ifdef mxHostFunctionPrimitive
	if ((mxArgc > 0) && (mxArgv(0)->kind == XS_HOST_FUNCTION_KIND))
		fxToInstance(the, mxArgv(0));
	if ((mxArgc > 1) && (mxArgv(1)->kind == XS_HOST_FUNCTION_KIND))
		fxToInstance(the, mxArgv(1));
#endif
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	target = mxArgv(0)->value.reference;
	if ((mxArgc < 2) || (mxArgv(1)->kind != XS_REFERENCE_KIND))
		mxTypeError("handler is no object");
	handler = mxArgv(1)->value.reference;
		
	mxPush(mxObjectPrototype);
	property = fxLastProperty(the, fxNewObjectInstance(the));
	mxPullSlot(mxResult);
	
	mxPushUndefined();
	instance = fxNewProxyInstance(the);
	instance->flag |= target->flag & (XS_CAN_CALL_FLAG | XS_CAN_CONSTRUCT_FLAG);
	slot = instance->next;
	slot->value.proxy.target = target;
	slot->value.proxy.handler = handler;
	property = fxNextSlotProperty(the, property, the->stack, mxID(_proxy), XS_GET_ONLY);
	
	slot = fxLastProperty(the, fxNewHostFunction(the, mxCallback(fx_Proxy_revoke), 0, XS_NO_ID, XS_NO_ID));
	slot = fxNextSlotProperty(the, slot, the->stack + 1, mxID(_proxy), XS_GET_ONLY);
	property = fxNextSlotProperty(the, property, the->stack, mxID(_revoke), XS_GET_ONLY);
	
	the->stack += 2;
}

void fx_Proxy_revoke(txMachine* the)
{
	txSlot* property = mxBehaviorGetProperty(the, mxFunction->value.reference, mxID(_proxy), 0, XS_ANY);
	if (property && (property->kind == XS_REFERENCE_KIND)) {
		txSlot* instance = property->value.reference;
		txSlot* proxy = instance->next;
		if (!proxy || (proxy->kind != XS_PROXY_KIND))
			mxTypeError("no proxy");
		if (proxy->flag & XS_MARK_FLAG)
			mxTypeError("Proxy instance is read-only");
		proxy->value.proxy.target = C_NULL;
		proxy->value.proxy.handler = C_NULL;
		property->kind = XS_NULL_KIND;
	}
}

void fx_Reflect_apply(txMachine* the)
{
	if ((mxArgc < 1) || !(fxIsCallable(the, mxArgv(0))))
		mxTypeError("target is no function");
	if ((mxArgc < 3) || (mxArgv(2)->kind != XS_REFERENCE_KIND))
		mxTypeError("argumentsList is no object");
	mxBehaviorCall(the, fxToInstance(the, mxArgv(0)), mxArgv(1), mxArgv(2));
}

void fx_Reflect_construct(txMachine* the)
{
    txSlot* target;
	if ((mxArgc < 1) || !mxIsReference(mxArgv(0)) || !mxIsConstructor(mxArgv(0)->value.reference))
		mxTypeError("target is no constructor");
	if ((mxArgc < 2) || (mxArgv(1)->kind != XS_REFERENCE_KIND))
		mxTypeError("argumentsList is no object");
	if (mxArgc < 3)
		target = mxArgv(0);
	else if (!mxIsReference(mxArgv(2)) || !mxIsConstructor(mxArgv(2)->value.reference))
		mxTypeError("newTarget is no constructor");
	else
		target = mxArgv(2);
	mxBehaviorConstruct(the, fxToInstance(the, mxArgv(0)), mxArgv(1), target);
}

void fx_Reflect_defineProperty(txMachine* the)
{
	txSlot* at;
	txFlag mask;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	at = fxAt(the, mxArgv(1));
	if ((mxArgc < 3) || (mxArgv(2)->kind != XS_REFERENCE_KIND))
		mxTypeError("invalid descriptor");
	mask = fxDescriptorToSlot(the, mxArgv(2));
	mxResult->value.boolean = mxBehaviorDefineOwnProperty(the, mxArgv(0)->value.reference, at->value.at.id, at->value.at.index, mxArgv(2), mask);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Reflect_deleteProperty(txMachine* the)
{
	txSlot* at;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	at = fxAt(the, mxArgv(1));
	mxResult->value.boolean = mxBehaviorDeleteProperty(the, mxArgv(0)->value.reference, at->value.at.id, at->value.at.index);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Reflect_get(txMachine* the)
{
	txSlot* at;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	at = fxAt(the, mxArgv(1));
	mxBehaviorGetPropertyValue(the, 	mxArgv(0)->value.reference, at->value.at.id, at->value.at.index, mxArgc < 3 ? mxArgv(0) : mxArgv(2), mxResult);
}

void fx_Reflect_getOwnPropertyDescriptor(txMachine* the)
{
	txSlot* at;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	at = fxAt(the, mxArgv(1));
	mxPushUndefined();
	if (mxBehaviorGetOwnProperty(the, mxArgv(0)->value.reference, at->value.at.id, at->value.at.index, the->stack)) {
		fxDescribeProperty(the, the->stack, XS_GET_ONLY);
		mxPullSlot(mxResult);
	}
	mxPop();
}

void fx_Reflect_getPrototypeOf(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	mxBehaviorGetPrototype(the, mxArgv(0)->value.reference, mxResult);
}

void fx_Reflect_has(txMachine* the)
{
	txSlot* at;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	at = fxAt(the, mxArgv(1));
	mxResult->value.boolean = mxBehaviorHasProperty(the, mxArgv(0)->value.reference, at->value.at.id, at->value.at.index);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Reflect_isExtensible(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	mxResult->value.boolean = mxBehaviorIsExtensible(the, mxArgv(0)->value.reference);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Reflect_ownKeys(txMachine* the)
{
	txSlot* result;
	txSlot* array;
	txSlot* item;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	mxPush(mxArrayPrototype);
	result = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	array = result->next;
	mxBehaviorOwnKeys(the, mxArgv(0)->value.reference, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, array);
	item = array;
	while ((item = item->next)) {
		array->value.array.length++;
		fxKeyAt(the, item->value.at.id, item->value.at.index, item);
	}
	fxCacheArray(the, result);
}

void fx_Reflect_preventExtensions(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	mxResult->value.boolean = mxBehaviorPreventExtensions(the, mxArgv(0)->value.reference);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Reflect_set(txMachine* the)
{
	txSlot* at;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if (mxArgc < 2)
		mxTypeError("no key");
	at = fxAt(the, mxArgv(1));
	if (mxArgc < 3)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(2));
	mxResult->value.boolean = mxBehaviorSetPropertyValue(the, mxArgv(0)->value.reference, at->value.at.id, at->value.at.index, the->stack, mxArgc < 4 ? mxArgv(0) : mxArgv(3));
	mxResult->kind = XS_BOOLEAN_KIND;
	mxPop();
}

void fx_Reflect_setPrototypeOf(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("target is no object");
	if ((mxArgc < 2) || ((mxArgv(1)->kind != XS_NULL_KIND) && (mxArgv(1)->kind != XS_REFERENCE_KIND)))
		mxTypeError("invalid prototype");
	mxResult->value.boolean = mxBehaviorSetPrototype(the, mxArgv(0)->value.reference, mxArgv(1));
	mxResult->kind = XS_BOOLEAN_KIND;
}

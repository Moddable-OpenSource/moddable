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

void fxBuildObject(txMachine* the)
{
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, the->stack->value.reference);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_Object_prototype___proto__get), mxCallback(fx_Object_prototype___proto__set), mxID(___proto__), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_prototype_hasOwnProperty), 1, mxID(_hasOwnProperty), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_prototype_isPrototypeOf), 1, mxID(_isPrototypeOf), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_prototype_propertyIsEnumerable), 1, mxID(_propertyIsEnumerable), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_prototype_propertyIsScriptable), 1, mxID(_propertyIsScriptable), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_prototype_toLocaleString), 0, mxID(_toLocaleString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_prototype_toPrimitive), 1, mxID(_Symbol_toPrimitive), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_prototype_toString), 0, mxID(_toString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_prototype_valueOf), 0, mxID(_valueOf), XS_DONT_ENUM_FLAG);
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, mxCallback(fx_Object), 1, mxID(_Object), XS_DONT_ENUM_FLAG));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_assign), 2, mxID(_assign), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_create), 2, mxID(_create), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_defineProperties), 2, mxID(_defineProperties), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_defineProperty), 3, mxID(_defineProperty), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_entries), 1, mxID(_entries), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_freeze), 1, mxID(_freeze), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_getOwnPropertyDescriptor), 2, mxID(_getOwnPropertyDescriptor), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_getOwnPropertyDescriptors), 1, mxID(_getOwnPropertyDescriptors), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_getOwnPropertyNames), 1, mxID(_getOwnPropertyNames), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_getOwnPropertySymbols), 1, mxID(_getOwnPropertySymbols), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_getPrototypeOf), 1, mxID(_getPrototypeOf), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_is), 2, mxID(_is), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_isExtensible), 1, mxID(_isExtensible), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_isFrozen), 1, mxID(_isFrozen), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_isSealed), 1, mxID(_isSealed), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_keys), 1, mxID(_keys), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_preventExtensions), 1, mxID(_preventExtensions), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_seal), 1, mxID(_seal), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_setPrototypeOf), 2, mxID(_setPrototypeOf), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Object_values), 1, mxID(_values), XS_DONT_ENUM_FLAG);
	the->stack++;
}

void fxCopyObject(txMachine* the)
{
	txInteger c = mxArgc, i;
	txSlot* target;
	txSlot* source;
	txSlot* at;
	txSlot* property;
	if ((c < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	target = fxToInstance(the, mxArgv(0));
	*mxResult = *mxArgv(0);
	if ((c < 2) || (mxArgv(1)->kind == XS_UNDEFINED_KIND) || (mxArgv(1)->kind == XS_NULL_KIND))
		return;
	source = fxToInstance(the, mxArgv(1));
	at = fxNewInstance(the);
	mxBehaviorOwnKeys(the, source, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, at);
	mxPushUndefined();
	property = the->stack;
	while ((at = at->next)) {
		for (i = 2; i < c; i++) {
			txSlot* exclude = mxArgv(i);
			if ((exclude->value.at.id == at->value.at.id) && (exclude->value.at.index == at->value.at.index))
				break;
		}
		if (i == c) {
			if (mxBehaviorGetOwnProperty(the, source, at->value.at.id, at->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
				mxPushReference(source);
				fxGetAll(the, at->value.at.id, at->value.at.index);
				the->stack->flag = 0;
				mxBehaviorDefineOwnProperty(the, target, at->value.at.id, at->value.at.index, the->stack, XS_GET_ONLY);
				mxPop();
			}
		}
	}
	mxPop(); // property
	mxPop(); // at
}

txSlot* fxNewObjectInstance(txMachine* the)
{
	txSlot* instance;
	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = the->stack->value.reference;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;
	return instance;
}

void fx_Object(txMachine* the)
{
	if (!mxIsUndefined(mxTarget) && !fxIsSameSlot(the, mxTarget, mxFunction)) {
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxObjectPrototype);
		fxNewObjectInstance(the);
		mxPullSlot(mxResult);
		return;
	}
	if ((mxArgc == 0) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND)) {
		mxPush(mxObjectPrototype);
		fxNewObjectInstance(the);
		mxPullSlot(mxResult);
		return;
	}
	*mxResult = *mxArgv(0);
	fxToInstance(the, mxResult);
}

void fx_Object_prototype___proto__get(txMachine* the)
{
	if ((mxThis->kind == XS_UNDEFINED_KIND) || (mxThis->kind == XS_NULL_KIND))
		mxTypeError("invalid this");
	mxBehaviorGetPrototype(the, fxToInstance(the, mxThis), mxResult);
}

void fx_Object_prototype___proto__set(txMachine* the)
{
	txSlot* instance;
	if ((mxThis->kind == XS_UNDEFINED_KIND) || (mxThis->kind == XS_NULL_KIND))
		mxTypeError("invalid this");
	if ((mxArgc < 1) || ((mxArgv(0)->kind != XS_NULL_KIND) && (mxArgv(0)->kind != XS_REFERENCE_KIND)))
		return;
	if (mxThis->kind == XS_REFERENCE_KIND) {
		instance = mxThis->value.reference;
		if (!mxBehaviorSetPrototype(the, instance, mxArgv(0)))
			mxTypeError("invalid prototype");
	}
}

void fx_Object_prototype_hasOwnProperty(txMachine* the)
{
	txSlot* instance;
	txSlot* at;
	if ((mxThis->kind == XS_UNDEFINED_KIND) || (mxThis->kind == XS_NULL_KIND))
		mxTypeError("invalid this");
	if (mxArgc < 1)
		mxTypeError("invalid key");
	instance = fxToInstance(the, mxThis);
	at = fxAt(the, mxArgv(0));
	mxPushUndefined();
	mxResult->value.boolean = mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, the->stack);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxPop();
}

void fx_Object_prototype_isPrototypeOf(txMachine* the)
{
	if ((mxThis->kind == XS_UNDEFINED_KIND) || (mxThis->kind == XS_NULL_KIND))
		mxTypeError("invalid this");
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			txSlot* prototype = fxToInstance(the, mxThis);
			while (mxBehaviorGetPrototype(the, slot->value.reference, slot)) {
				if (prototype == slot->value.reference) {
					mxResult->value.boolean = 1;
					break;
				}
			}
		}
	}
}

void fx_Object_prototype_propertyIsEnumerable(txMachine* the)
{
	txSlot* instance;
	txSlot* at;
	if ((mxThis->kind == XS_UNDEFINED_KIND) || (mxThis->kind == XS_NULL_KIND))
		mxTypeError("invalid this");
	if (mxArgc < 1)
		mxTypeError("invalid key");
	mxResult->value.boolean = 0;
	instance = fxToInstance(the, mxThis);
	at = fxAt(the, mxArgv(0));
	mxPushUndefined();
	mxResult->value.boolean = mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, the->stack) && ((the->stack->flag & XS_DONT_ENUM_FLAG) == 0);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxPop();
}

void fx_Object_prototype_propertyIsScriptable(txMachine* the)
{
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
}

void fx_Object_prototype_toLocaleString(txMachine* the)
{
	mxPushInteger(0);
	mxPushSlot(mxThis);
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_toString));
	fxCall(the);
	mxPullSlot(mxResult);
}

void fx_Object_prototype_toPrimitive(txMachine* the)
{
	if (mxThis->kind == XS_REFERENCE_KIND) {
		txInteger hint = XS_NO_HINT;
		txInteger ids[2], i;
		if (mxArgc > 0) {
			txSlot* slot = mxArgv(0);
			if ((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) {
				if (!c_strcmp(slot->value.string, "default"))
					hint = XS_NUMBER_HINT;
				else if (!c_strcmp(slot->value.string, "number"))
					hint = XS_NUMBER_HINT;
				else if (!c_strcmp(slot->value.string, "string"))
					hint = XS_STRING_HINT;
			}
		}
		if (hint == XS_STRING_HINT) {
		 	ids[0] = mxID(_toString);
		 	ids[1] = mxID(_valueOf);
		}
		else if (hint == XS_NUMBER_HINT) {
		 	ids[0] = mxID(_valueOf);
		 	ids[1] = mxID(_toString);
		}
 		else
     		mxTypeError("invalid hint");
		for (i = 0; i < 2; i++) {
			mxPushInteger(0);
			mxPushSlot(mxThis);
			mxPushSlot(mxThis);
			fxGetID(the, ids[i]);
			if (
#ifdef mxHostFunctionPrimitive
				(XS_HOST_FUNCTION_KIND == the->stack->kind) ||
#endif
				(mxIsReference(the->stack) && mxIsFunction(the->stack->value.reference))) {
				fxCall(the);
				if (mxIsReference(the->stack))
					the->stack++;
				else {
					mxResult->kind = the->stack->kind;
					mxResult->value = the->stack->value;
					the->stack++;
					return;
      			}
			}
			else
				the->stack++;
		}
		if (hint == XS_STRING_HINT)
            mxTypeError("cannot coerce object to string");
        else
            mxTypeError("cannot coerce object to number");
	}
	else {
		mxResult->kind = mxThis->kind;
		mxResult->value = mxThis->value;
	}
}

void fx_Object_prototype_toString(txMachine* the)
{
	txString tag = C_NULL;
	txSlot* instance = C_NULL;
	txSlot* slot;
	txSlot* target;
	switch (mxThis->kind) {
	case XS_UNDEFINED_KIND:
		tag = "Undefined";
		break;
	case XS_NULL_KIND:
		tag = "Null";
		break;
	case XS_BOOLEAN_KIND:
		instance = mxBooleanPrototype.value.reference;
		tag = "Boolean";
		break;
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
		instance = mxNumberPrototype.value.reference;
		tag = "Number";
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		instance = mxStringPrototype.value.reference;
		tag = "String";
		break;
	case XS_SYMBOL_KIND:
		instance = mxSymbolPrototype.value.reference;
		tag = "Symbol";
		break;
	case XS_REFERENCE_KIND:
		instance = mxThis->value.reference;
		if (fxIsArray(the, instance))
			tag = "Array";
		else {
			slot = instance->next;
			if (slot) {
				if ((slot->ID == XS_ARGUMENTS_SLOPPY_BEHAVIOR) || (slot->ID == XS_ARGUMENTS_STRICT_BEHAVIOR))
					tag = "Arguments";
				else if (slot->flag & XS_INTERNAL_FLAG) {
					switch (slot->kind) {
					case XS_BOOLEAN_KIND:
						tag = "Boolean";
						break;
					case XS_CALLBACK_KIND:
					case XS_CALLBACK_X_KIND:
					case XS_CODE_KIND:
					case XS_CODE_X_KIND:
						tag = "Function";
						break;
					case XS_DATE_KIND:
						tag = "Date";
						break;
					case XS_ERROR_KIND:
						tag = "Error";
						break;
					case XS_GLOBAL_KIND:
						tag = "global";
						break;
					case XS_NUMBER_KIND:
						tag = "Number";
						break;
					case XS_REGEXP_KIND:
						tag = "RegExp";
						break;
					case XS_STRING_KIND:
					case XS_STRING_X_KIND:
						tag = "String";
						break;
					case XS_PROXY_KIND:
						while ((target = slot->value.proxy.target)) {
							slot = target->next;
							if (!slot || (slot->kind != XS_PROXY_KIND))
								break;
						}
						if (mxIsFunction(target)) {
							instance = target;
							tag = "Function";
						}
						else
							tag = "Object";
						break;
					default:
						tag = "Object";
						break;
					}
				}
				else
            		tag = "Object";
			}
			else
            	tag = "Object";
		}
		break;
#ifdef mxHostFunctionPrimitive
	case XS_HOST_FUNCTION_KIND:
		instance = mxFunctionPrototype.value.reference;
		tag = "Function";
		break;
#endif
	default:
		tag = "Object";
		break;
	}
	fxCopyStringC(the, mxResult, "[object ");
	if (instance) {
		mxPushReference(instance);
		fxGetID(the, mxID(_Symbol_toStringTag));
		if ((the->stack->kind == XS_STRING_KIND) || (the->stack->kind == XS_STRING_X_KIND))
			fxConcatString(the, mxResult, the->stack);
		else
			fxConcatStringC(the, mxResult, tag);
	}
	else
		fxConcatStringC(the, mxResult, tag);
	fxConcatStringC(the, mxResult, "]");
}

void fx_Object_prototype_valueOf(txMachine* the)
{
	fxToInstance(the, mxThis);
	*mxResult = *mxThis;
}

void fx_Object_assign(txMachine* the)
{
	txSlot* target;
	txInteger c, i;
	txSlot* instance;
	txSlot* at;
	txSlot* property;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid target");
	fxToInstance(the, mxArgv(0));
	target = mxArgv(0);
	c = mxArgc;
	for (i = 1; i < c; i++) {
		if ((mxArgv(i)->kind == XS_UNDEFINED_KIND) || (mxArgv(i)->kind == XS_NULL_KIND))
			continue;
		instance = fxToInstance(the, mxArgv(i));
		at = fxNewInstance(the);
		mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, at);
		mxPushUndefined();
		property = the->stack;
		while ((at = at->next)) {
			if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
				mxPushReference(instance);
				fxGetAll(the, at->value.at.id, at->value.at.index);
				mxPushSlot(target);
				fxSetAll(the, at->value.at.id, at->value.at.index);
				mxPop();
			}
		}
		mxPop();
		mxPop();
	}
	*mxResult = *target;
}

void fx_Object_create(txMachine* the)
{
	txSlot* instance;
	txSlot* properties;
	txSlot* at;
	txSlot* property;
	txFlag mask;
	if ((mxArgc < 1) || ((mxArgv(0)->kind != XS_NULL_KIND) && (mxArgv(0)->kind != XS_REFERENCE_KIND)))
		mxTypeError("invalid prototype");
	if (mxArgv(0)->kind == XS_NULL_KIND)
		fxNewInstance(the);
	else {
		mxPushSlot(mxArgv(0));
		fxNewHostInstance(the);
	}
	mxPullSlot(mxResult);
	instance = fxGetInstance(the, mxResult);
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		properties = fxToInstance(the, mxArgv(1));
		at = fxNewInstance(the);
		mxBehaviorOwnKeys(the, properties, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, at);
		mxPushUndefined();
		property = the->stack;
		while ((at = at->next)) {
			if (mxBehaviorGetOwnProperty(the, properties, at->value.at.id, at->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
				mxPushReference(properties);
				fxGetAll(the, at->value.at.id, at->value.at.index);
				mask = fxDescriptorToSlot(the, the->stack);
				if (!mxBehaviorDefineOwnProperty(the, instance, at->value.at.id, at->value.at.index, the->stack, mask))
					mxTypeError("invalid descriptor");
				mxPop();
			}
		}
		mxPop();
		mxPop();
	}
}

void fx_Object_defineProperties(txMachine* the)
{
	txSlot* instance;
	txSlot* properties;
	txSlot* at;
	txSlot* property;
	txFlag mask;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("invalid object");
	if ((mxArgc < 2) || (mxArgv(1)->kind == XS_UNDEFINED_KIND))
		mxTypeError("invalid properties");
	instance = fxGetInstance(the, mxArgv(0));
	properties = fxToInstance(the, mxArgv(1));
	at = fxNewInstance(the);
	mxBehaviorOwnKeys(the, properties, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, at);
	mxPushUndefined();
	property = the->stack;
	while ((at = at->next)) {
		if (mxBehaviorGetOwnProperty(the, properties, at->value.at.id, at->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
			mxPushReference(properties);
			fxGetAll(the, at->value.at.id, at->value.at.index);
			mask = fxDescriptorToSlot(the, the->stack);
			if (!mxBehaviorDefineOwnProperty(the, instance, at->value.at.id, at->value.at.index, the->stack, mask))
				mxTypeError("invalid descriptor");
			mxPop();
		}
	}
	mxPop();
	mxPop();
	*mxResult = *mxArgv(0);
}

void fx_Object_defineProperty(txMachine* the)
{
	txSlot* at;
	txFlag mask;
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("invalid object");
	if (mxArgc < 2)
		mxTypeError("invalid key");
	at = fxAt(the, mxArgv(1));
	if ((mxArgc < 3) || (mxArgv(2)->kind != XS_REFERENCE_KIND))
		mxTypeError("invalid descriptor");
	mask = fxDescriptorToSlot(the, mxArgv(2));
	if(!mxBehaviorDefineOwnProperty(the, mxArgv(0)->value.reference, at->value.at.id, at->value.at.index, mxArgv(2), mask))
		mxTypeError("invalid descriptor");
	*mxResult = *mxArgv(0);
}

void fx_Object_entries(txMachine* the)
{
	txSlot* instance;
	txSlot* result;
	txSlot* array;
	txSlot* property;
	txSlot** address;
	txSlot* item;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	instance = fxToInstance(the, mxArgv(0));
	mxPush(mxArrayPrototype);
	result = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	array = result->next;
	mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG, array);
	mxPushUndefined();
	property = the->stack;
	address = &array->next;
	while ((item = *address)) {
		if (mxBehaviorGetOwnProperty(the, instance, item->value.at.id, item->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
			array->value.array.length++;
			mxPushUndefined();
			fxKeyAt(the, item->value.at.id, item->value.at.index, the->stack);
			mxPushReference(instance);
			fxGetAll(the, item->value.at.id, item->value.at.index);
			fxConstructArrayEntry(the, item);
			address = &(item->next);
		}
		else
			*address = item->next;
	}
	mxPop();
	fxCacheArray(the, result);
}

void fx_Object_freeze(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			txSlot* instance = slot->value.reference;
			txSlot* at;
			txSlot* property;
			mxBehaviorPreventExtensions(the, instance);
			at = fxNewInstance(the);
			mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, at);
			mxPushUndefined();
			property = the->stack;
			while ((at = at->next)) {
				if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, property)) {
					if (property->kind != XS_ACCESSOR_KIND) 
						property->flag |= XS_DONT_SET_FLAG;
					property->flag |= XS_DONT_DELETE_FLAG;
					mxBehaviorDefineOwnProperty(the, instance, at->value.at.id, at->value.at.index, property, XS_GET_ONLY);
				}
			}
			mxPop();
			if ((mxArgc > 1) && fxToBoolean(the, mxArgv(1))) {
				at = the->stack->value.reference;
				mxPushUndefined();
				property = the->stack;
				while ((at = at->next)) {
					if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, property)) {
						if (property->kind == XS_REFERENCE_KIND) {
							mxPushSlot(property);
							mxPushInteger(1);
							mxPushSlot(mxThis);
							fxCallID(the, mxID(_isFrozen));
							if (!fxRunTest(the)) {
								mxPushSlot(property);
								mxPushBoolean(1);
								mxPushInteger(2);
								mxPushSlot(mxThis);
								mxPushSlot(mxFunction);
								fxCall(the);
								mxPop();
							}
						}
					}
				}
				mxPop();
			}
			mxPop();
		}
		*mxResult = *mxArgv(0);
	}
}

void fx_Object_getOwnPropertyDescriptor(txMachine* the)
{
	txSlot* instance;
	txSlot* at;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	instance = fxToInstance(the, mxArgv(0));
	if (mxArgc < 2)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(1));
	at = fxAt(the, the->stack);
	mxPushUndefined();
	if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, the->stack)) {
		fxDescribeProperty(the, the->stack, XS_GET_ONLY);
		mxPullSlot(mxResult);
	}
	mxPop();
	mxPop();
}

void fx_Object_getOwnPropertyDescriptors(txMachine* the)
{
	txSlot* instance;
	txSlot* result;
	txSlot* at;
	txSlot* property;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	instance = fxToInstance(the, mxArgv(0));
	mxPush(mxObjectPrototype);
	result = fxNewObjectInstance(the);
	mxPullSlot(mxResult);
	at = fxNewInstance(the);
	mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, at);
	mxPushUndefined();
	property = the->stack;
	while ((at = at->next)) {
		if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, property)) {
			fxDescribeProperty(the, property, XS_GET_ONLY);
			mxBehaviorDefineOwnProperty(the, result, at->value.at.id, at->value.at.index, the->stack, XS_GET_ONLY);
			mxPop();
		}
	}
	mxPop();
}

void fx_Object_getOwnPropertyNames(txMachine* the)
{
	txSlot* instance;
	txSlot* result;
	txSlot* array;
	txSlot* item;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	instance = fxToInstance(the, mxArgv(0));
	mxPush(mxArrayPrototype);
	result = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	array = result->next;
	mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG, array);
	item = array;
	while ((item = item->next)) {
		array->value.array.length++;
		fxKeyAt(the, item->value.at.id, item->value.at.index, item);
	}
	fxCacheArray(the, result);
}

void fx_Object_getOwnPropertySymbols(txMachine* the)
{
	txSlot* instance;
	txSlot* result;
	txSlot* array;
	txSlot* item;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	instance = fxToInstance(the, mxArgv(0));
	mxPush(mxArrayPrototype);
	result = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	array = result->next;
	mxBehaviorOwnKeys(the, instance, XS_EACH_SYMBOL_FLAG, array);
	item = array;
	while ((item = item->next)) {
		array->value.array.length++;
		fxKeyAt(the, item->value.at.id, item->value.at.index, item);
	}
	fxCacheArray(the, result);
}

void fx_Object_getPrototypeOf(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	mxBehaviorGetPrototype(the, fxToInstance(the, mxArgv(0)), mxResult);
}

void fx_Object_is(txMachine* the)
{
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	if (mxArgc > 1)
		mxPushSlot(mxArgv(1));
	else
		mxPushUndefined();
	mxResult->value.boolean = fxIsSameValue(the, the->stack + 1, the->stack, 0);
	mxResult->kind = XS_BOOLEAN_KIND;
	the->stack += 2;
}

void fx_Object_isExtensible(txMachine* the)
{
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			mxResult->value.boolean = mxBehaviorIsExtensible(the, slot);
		}
	}
}

void fx_Object_isFrozen(txMachine* the)
{
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			txSlot* instance = slot->value.reference;
            mxResult->value.boolean = mxBehaviorIsExtensible(the, instance) ? 0 : 1;
			if (mxResult->value.boolean) {
				txSlot* at;
				txSlot* property;
				at = fxNewInstance(the);
				mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, at);
				mxPushUndefined();
				property = the->stack;
				while ((at = at->next)) {
					if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, property)) {
						if (property->kind != XS_ACCESSOR_KIND) 
							if (!(property->flag & XS_DONT_SET_FLAG))
								mxResult->value.boolean = 0;
						if (!(property->flag & XS_DONT_DELETE_FLAG))
							mxResult->value.boolean = 0;
					}
				}
				mxPop();
				mxPop();
			}
		}
	}
}

void fx_Object_isSealed(txMachine* the)
{
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			txSlot* instance = slot->value.reference;
			mxResult->value.boolean = mxBehaviorIsExtensible(the, instance) ? 0 : 1;
			if (mxResult->value.boolean) {
				txSlot* at;
				txSlot* property;
				at = fxNewInstance(the);
				mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, at);
				mxPushUndefined();
				property = the->stack;
				while ((at = at->next)) {
					if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, property)) {
						if (!(property->flag & XS_DONT_DELETE_FLAG))
							mxResult->value.boolean = 0;
					}
				}
				mxPop();
				mxPop();
			}		
		}
	}
}

void fx_Object_keys(txMachine* the)
{
	txSlot* instance;
	txSlot* result;
	txSlot* array;
	txSlot* property;
	txSlot** address;
	txSlot* item;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	instance = fxToInstance(the, mxArgv(0));
	mxPush(mxArrayPrototype);
	result = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	array = result->next;
	mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG, array);
	mxPushUndefined();
	property = the->stack;
	address = &array->next;
	while ((item = *address)) {
		if (mxBehaviorGetOwnProperty(the, instance, item->value.at.id, item->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
			array->value.array.length++;
			fxKeyAt(the, item->value.at.id, item->value.at.index, item);
			address = &(item->next);
		}
		else
			*address = item->next;
	}
	mxPop();
	fxCacheArray(the, result);
}

void fx_Object_preventExtensions(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			mxBehaviorPreventExtensions(the, slot);
		}
		*mxResult = *mxArgv(0);
	}
}

void fx_Object_seal(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			txSlot* instance = slot->value.reference;
			txSlot* at;
			txSlot* property;
			mxBehaviorPreventExtensions(the, instance);
			at = fxNewInstance(the);
			mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, at);
			mxPushUndefined();
			property = the->stack;
			while ((at = at->next)) {
				if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, property)) {
					property->flag |= XS_DONT_DELETE_FLAG;
					mxBehaviorDefineOwnProperty(the, instance, at->value.at.id, at->value.at.index, property, XS_GET_ONLY);
				}
			}
			mxPop();
			mxPop();
		}
		*mxResult = *mxArgv(0);
	}
}

void fx_Object_setPrototypeOf(txMachine* the)
{
	txSlot* instance;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	if ((mxArgc < 2) || ((mxArgv(1)->kind != XS_NULL_KIND) && (mxArgv(1)->kind != XS_REFERENCE_KIND)))
		mxTypeError("invalid prototype");
	if (mxArgv(0)->kind == XS_REFERENCE_KIND) {
		instance = mxArgv(0)->value.reference;
		if (!mxBehaviorSetPrototype(the, instance, mxArgv(1)))
			mxTypeError("invalid prototype");
	}
	*mxResult = *mxArgv(0);
}

void fx_Object_values(txMachine* the)
{
	txSlot* instance;
	txSlot* result;
	txSlot* array;
	txSlot* property;
	txSlot** address;
	txSlot* item;
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND))
		mxTypeError("invalid object");
	instance = fxToInstance(the, mxArgv(0));
	mxPush(mxArrayPrototype);
	result = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	array = result->next;
	mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG, array);
	mxPushUndefined();
	property = the->stack;
	address = &array->next;
	while ((item = *address)) {
		if (mxBehaviorGetOwnProperty(the, instance, item->value.at.id, item->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
			array->value.array.length++;
			mxPushReference(instance);
			fxGetAll(the, item->value.at.id, item->value.at.index);
			mxPullSlot(item);
			address = &(item->next);
		}
		else
			*address = item->next;
	}
	mxPop();
	fxCacheArray(the, result);
}

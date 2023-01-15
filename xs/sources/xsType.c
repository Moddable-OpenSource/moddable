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

const txBehavior ICACHE_FLASH_ATTR gxOrdinaryBehavior = {
	fxOrdinaryGetProperty,
	fxOrdinarySetProperty,
	fxOrdinaryCall,
	fxOrdinaryConstruct,
	fxOrdinaryDefineOwnProperty,
	fxOrdinaryDeleteProperty,
	fxOrdinaryGetOwnProperty,
	fxOrdinaryGetPropertyValue,
	fxOrdinaryGetPrototype,
	fxOrdinaryHasProperty,
	fxOrdinaryIsExtensible,
	fxOrdinaryOwnKeys,
	fxOrdinaryPreventExtensions,
	fxOrdinarySetPropertyValue,
	fxOrdinarySetPrototype,
};

txSlot* fxAliasInstance(txMachine* the, txSlot* instance)
{
	txSlot* alias;
	txSlot* from;
	txSlot* to;
	the->aliasArray[instance->ID] = alias = fxNewSlot(the);
	alias->flag = instance->flag & ~(XS_MARK_FLAG | XS_DONT_MARSHALL_FLAG);
	alias->kind = XS_INSTANCE_KIND;
	alias->value.instance.garbage =  instance->value.instance.garbage;
	alias->value.instance.prototype = instance->value.instance.prototype;
	from = instance->next;
	to = alias;
	while (from) {
		to = to->next = fxDuplicateSlot(the, from);
		if (to->kind == XS_ARRAY_KIND) {
			txSlot* address = to->value.array.address;
			if (address) {
				txSize size = (((txChunk*)(((txByte*)address) - sizeof(txChunk)))->size) / sizeof(txSlot);
				txSlot* chunk = (txSlot*)fxNewChunk(the, size * sizeof(txSlot));
				c_memcpy(chunk, address, size * sizeof(txSlot));
				to->value.array.address = chunk;
				while (size) {
					chunk->flag &= ~XS_MARK_FLAG;
					chunk++;
					size--;
				}
			}
		}
		else if (to->kind == XS_DATE_KIND) {
			to->flag &= ~XS_DONT_SET_FLAG;
		}
		else if (to->kind == XS_PRIVATE_KIND) {
			txSlot** address = &to->value.private.first;
			txSlot* slot;
			while ((slot = *address)) {
				*address = slot = fxDuplicateSlot(the, slot);
				address = &slot->next;
			}
		}
		from = from->next;
	}
	return alias;
}

txSlot* fxDuplicateInstance(txMachine* the, txSlot* instance)
{
	txSlot* result;
	txSlot* from;
	txSlot* to;
	result = fxNewInstance(the);
	result->flag = instance->flag & ~XS_MARK_FLAG;
	result->value.instance.garbage = C_NULL;
	result->value.instance.prototype = instance->value.instance.prototype;
	from = instance->next;
	to = result;
	while (from) {
		if (from->kind != XS_WEAK_ENTRY_KIND) {
			to = to->next = fxDuplicateSlot(the, from);
			if (from->kind == XS_ARRAY_KIND) {
				txSlot* address = from->value.array.address;
				to->value.array.address = C_NULL;
				if (address) {
					txSize size = (((txChunk*)(((txByte*)address) - sizeof(txChunk)))->size) / sizeof(txSlot);
					txSlot* chunk = (txSlot*)fxNewChunk(the, size * sizeof(txSlot));
					address = from->value.array.address;
					c_memcpy(chunk, address, size * sizeof(txSlot));
					to->value.array.address = chunk;
					while (size) {
						chunk->flag &= ~XS_MARK_FLAG;
						chunk++;
						size--;
					}
				}
			}
		}
		from = from->next;
	}
	return result;
}

txSlot* fxGetInstance(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_REFERENCE_KIND)
		return theSlot->value.reference;
	return C_NULL;
}

txSlot* fxGetPrototype(txMachine* the, txSlot* instance)
{
	txSlot* prototype = instance->value.instance.prototype;
	if (prototype) {
		if (prototype->ID) {
			txSlot* alias = the->aliasArray[prototype->ID];
			if (alias)
				prototype = alias;
		}
	}
	return prototype;
}

txSlot* fxNewInstance(txMachine* the)
{
	txSlot* instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = C_NULL;
	mxPushReference(instance);
	return instance;
}

txBoolean fxIsSameInstance(txMachine* the, txSlot* a, txSlot* b)
{	
	if (a == b)
		return 1;
	if (a->ID) {
		txSlot* alias = the->aliasArray[a->ID];
		if (alias) {
			a = alias;
			if (a == b)
				return 1;
		}
	}
	if (b->ID) {
		txSlot* alias = the->aliasArray[b->ID];
		if (alias) {
			b = alias;
			if (a == b)
				return 1;
		}
	}
	return 0;
}

txSlot* fxToInstance(txMachine* the, txSlot* theSlot)
{
	txSlot* anInstance = C_NULL;
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
		mxTypeError("cannot coerce undefined to object");
		break;
	case XS_NULL_KIND:
		mxTypeError("cannot coerce null to object");
		break;
	case XS_BOOLEAN_KIND:
		mxPush(mxBooleanPrototype);
		anInstance = fxNewBooleanInstance(the);
		anInstance->next->value.boolean = theSlot->value.boolean;
		if (the->frame->flag & XS_STRICT_FLAG)
			anInstance->flag |= XS_DONT_PATCH_FLAG;
		mxPullSlot(theSlot);
		break;
	case XS_INTEGER_KIND:
		mxPush(mxNumberPrototype);
		anInstance = fxNewNumberInstance(the);
		anInstance->next->value.number = theSlot->value.integer;
		if (the->frame->flag & XS_STRICT_FLAG)
			anInstance->flag |= XS_DONT_PATCH_FLAG;
		mxPullSlot(theSlot);
		break;
	case XS_NUMBER_KIND:
		mxPush(mxNumberPrototype);
		anInstance = fxNewNumberInstance(the);
		anInstance->next->value.number = theSlot->value.number;
		if (the->frame->flag & XS_STRICT_FLAG)
			anInstance->flag |= XS_DONT_PATCH_FLAG;
		mxPullSlot(theSlot);
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		mxPush(mxStringPrototype);
		anInstance = fxNewStringInstance(the);
		anInstance->next->kind = theSlot->kind;
		anInstance->next->value.string = theSlot->value.string;
		if (the->frame->flag & XS_STRICT_FLAG)
			anInstance->flag |= XS_DONT_PATCH_FLAG;
		mxPullSlot(theSlot);
		break;
	case XS_SYMBOL_KIND:
		mxPush(mxSymbolPrototype);
		anInstance = fxNewSymbolInstance(the);
		anInstance->next->value.symbol = theSlot->value.symbol;
		if (the->frame->flag & XS_STRICT_FLAG)
			anInstance->flag |= XS_DONT_PATCH_FLAG;
		mxPullSlot(theSlot);
		break;
	case XS_BIGINT_KIND:
	case XS_BIGINT_X_KIND:
		anInstance = gxTypeBigInt.toInstance(the, theSlot);
		break;
	case XS_REFERENCE_KIND:
		anInstance = theSlot->value.reference;
		break;
#ifdef mxHostFunctionPrimitive
	case XS_HOST_FUNCTION_KIND: {
		const txHostFunctionBuilder* builder = theSlot->value.hostFunction.builder;
		anInstance = fxNewHostFunction(the, builder->callback, builder->length, builder->id, theSlot->value.hostFunction.profileID);
		mxPullSlot(theSlot);
		} break;
#endif
	default:
		mxTypeError("cannot coerce to instance");
		break;
	}
	return anInstance;
}

void fxToPrimitive(txMachine* the, txSlot* theSlot, txInteger theHint)
{
	if (mxIsReference(theSlot)) {
		fxBeginHost(the);
		mxPushSlot(theSlot);
		mxPushSlot(theSlot);
		mxGetID(mxID(_Symbol_toPrimitive));
		if (mxIsUndefined(the->stack) || mxIsNull(the->stack)) {
			mxPop();
			mxPush(mxOrdinaryToPrimitiveFunction);
		}
		mxCall();
		if (theHint == XS_NO_HINT)
			mxPushSlot(&mxDefaultString);
		else if (theHint == XS_NUMBER_HINT)
			mxPushSlot(&mxNumberString);
		else
			mxPushSlot(&mxStringString);
		mxRunCount(1);
		theSlot->kind = the->stack->kind;
		theSlot->value = the->stack->value;
		if (mxIsReference(the->stack)) {
			mxTypeError("cannot coerce to primitive");
		}
		mxPullSlot(theSlot);
		fxEndHost(the);
	}
}

void fxToSpeciesConstructor(txMachine* the, txSlot* constructor)
{
	if (mxIsUndefined(the->stack)) {
		mxPop();
		mxPushSlot(constructor);
		return;
	}
	if (!mxIsReference(the->stack)) {
		mxTypeError("no constructor");
	}
	mxGetID(mxID(_Symbol_species));
	if (mxIsUndefined(the->stack) || mxIsNull(the->stack)) {
		mxPop();
		mxPushSlot(constructor);
		return;
	}
	if (!mxIsReference(the->stack) || !mxIsConstructor(the->stack->value.reference))
		mxTypeError("no constructor");
}

void fxOrdinaryCall(txMachine* the, txSlot* instance, txSlot* _this, txSlot* arguments)
{
    txIndex c, i;
	/* THIS */
	mxPushSlot(_this);
	/* FUNCTION */
	mxPushReference(instance);
	/* TARGET */
	mxPushUndefined();
	/* RESULT */
	mxPushUndefined();
	/* FRAME */
	mxPushUninitialized();
	/* COUNT */
	mxPushUninitialized();
	/* ARGUMENTS */
	mxPushSlot(arguments);
	mxGetID(mxID(_length));
    c = (txIndex)fxToLength(the, the->stack);
	mxPop();
	for (i = 0; i < c; i++) {
		mxPushSlot(arguments);
		mxGetIndex(i);
	}
	mxRunCount(c);
	mxPullSlot(mxResult);
}

void fxOrdinaryConstruct(txMachine* the, txSlot* instance, txSlot* arguments, txSlot* target)
{
    txIndex c, i;
	/* THIS */
	mxPushUninitialized();
	/* FUNCTION */
	mxPushReference(instance);
	/* TARGET */
	mxPushSlot(target);
	/* RESULT */
	mxPushUndefined();
	/* FRAME */
	mxPushUninitialized();
	/* COUNT */
	mxPushUninitialized();
	/* ARGUMENTS */
	mxPushSlot(arguments);
	mxGetID(mxID(_length));
    c = (txIndex)fxToLength(the, the->stack);
	mxPop();
	for (i = 0; i < c; i++) {
		mxPushSlot(arguments);
		mxGetIndex(i);
	}
	mxRunCount(c);
	mxPullSlot(mxResult);
}

txBoolean fxOrdinaryDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask) 
{
	txSlot* property = mxBehaviorGetProperty(the, instance, id, index, XS_OWN);
	if (property) {
		if (property->flag & XS_DONT_DELETE_FLAG) {
			if ((mask & XS_DONT_DELETE_FLAG) && !(slot->flag & XS_DONT_DELETE_FLAG))
				return 0;
			if ((mask & XS_DONT_ENUM_FLAG) && ((property->flag & XS_DONT_ENUM_FLAG) != (slot->flag & XS_DONT_ENUM_FLAG)))
				return 0;
			if (mask & XS_ACCESSOR_FLAG) {
				if (property->kind != XS_ACCESSOR_KIND)
					return 0;
				if (mask & XS_GETTER_FLAG) {
					if (property->value.accessor.getter != slot->value.accessor.getter)
						return 0;
				}
				if (mask & XS_SETTER_FLAG) {
					if (property->value.accessor.setter != slot->value.accessor.setter)
						return 0;
				}
				return 1;
			}
			else if ((mask & XS_DONT_SET_FLAG) || (slot->kind != XS_UNINITIALIZED_KIND)) {
				if (property->kind == XS_ACCESSOR_KIND)
					return 0;
				if (property->flag & XS_DONT_SET_FLAG) {
					if ((mask & XS_DONT_SET_FLAG) && !(slot->flag & XS_DONT_SET_FLAG))
						return 0;
					if ((slot->kind != XS_UNINITIALIZED_KIND) && !fxIsSameValue(the, property, slot, 0))
						return 0;
					return 1;
				}
			}
		}
		if (instance->ID) {
			txSlot* alias = the->aliasArray[instance->ID];
			if (!alias) {
				alias = fxAliasInstance(the, instance);
				property = mxBehaviorGetProperty(the, instance, id, index, XS_OWN);
			}
		}
	}
	else {
		property = mxBehaviorSetProperty(the, instance, id, index, XS_OWN);
		if (!property)
			return 0;
		property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	}
	if (mask & XS_DONT_DELETE_FLAG) {
		if (slot->flag & XS_DONT_DELETE_FLAG)
			property->flag |= XS_DONT_DELETE_FLAG;
		else
			property->flag &= ~XS_DONT_DELETE_FLAG;
	}
	if (mask & XS_DONT_ENUM_FLAG) {
		if (slot->flag & XS_DONT_ENUM_FLAG)
			property->flag |= XS_DONT_ENUM_FLAG;
		else
			property->flag &= ~XS_DONT_ENUM_FLAG;
	}
	if (mask & XS_ACCESSOR_FLAG) {
		txSlot* getter = C_NULL;
		txSlot* setter = C_NULL;
		if (property->kind != XS_ACCESSOR_KIND) {
			property->kind = XS_ACCESSOR_KIND;
			property->value.accessor.getter = C_NULL;
			property->value.accessor.setter = C_NULL;
		}
		if (mask & XS_GETTER_FLAG)
			getter = property->value.accessor.getter = slot->value.accessor.getter;
		if (mask & XS_SETTER_FLAG)
			setter = property->value.accessor.setter = slot->value.accessor.setter;
		if (getter) {
			if (mxIsFunction(getter)) {
				if ((mask & XS_METHOD_FLAG) && ((getter->flag & XS_MARK_FLAG) == 0)) {
					txSlot* home = mxFunctionInstanceHome(getter);
					home->value.home.object = instance;
				}
				if ((mask & XS_NAME_FLAG) && ((getter->flag & XS_MARK_FLAG) == 0))
					fxRenameFunction(the, getter, id, index, mxID(_get), "get ");
			}
		}
		if (setter) {
			if (mxIsFunction(setter)) {
				if ((mask & XS_METHOD_FLAG) && ((setter->flag & XS_MARK_FLAG) == 0)) {
					txSlot* home = mxFunctionInstanceHome(setter);
					home->value.home.object = instance;
				}
				if ((mask & XS_NAME_FLAG) && ((setter->flag & XS_MARK_FLAG) == 0))
					fxRenameFunction(the, setter, id, index, mxID(_set), "set ");
			}
		}
	}
	else if ((mask & XS_DONT_SET_FLAG) || (slot->kind != XS_UNINITIALIZED_KIND)) {
        if (mask & XS_DONT_SET_FLAG) {
            if (slot->flag & XS_DONT_SET_FLAG) {
                property->flag |= XS_DONT_SET_FLAG;
            }
            else
                property->flag &= ~XS_DONT_SET_FLAG;
        }
		if (slot->kind != XS_UNINITIALIZED_KIND) {
			property->kind = slot->kind;
			property->value = slot->value;
			if (slot->kind == XS_REFERENCE_KIND) {
				txSlot* function = slot->value.reference;
				if (mxIsFunction(function)) {
					if ((mask & XS_METHOD_FLAG) && ((function->flag & XS_MARK_FLAG) == 0)) {
						txSlot* home = mxFunctionInstanceHome(function);
						home->value.home.object = instance;
					}
					if ((mask & XS_NAME_FLAG) && ((function->flag & XS_MARK_FLAG) == 0))
						fxRenameFunction(the, function, id, index, mxID(_value), C_NULL);
				}
			}
		}
	}
	return 1;
}


txBoolean fxOrdinaryDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	txSlot** address = &(instance->next);
	txSlot* property;
	if (instance->ID) {
		txSlot* alias = the->aliasArray[instance->ID];
		if (alias)
			instance = alias;
	}
	address = &(instance->next);
	if (id) {
		while ((property = *address) && (property->flag & XS_INTERNAL_FLAG))
			address = &(property->next);
		while ((property = *address)) {
			if (property->ID == id) {
				if (property->flag & XS_DONT_DELETE_FLAG)
					return 0;
				if (instance->ID)
					return fxOrdinaryDeleteProperty(the, fxAliasInstance(the, instance), id, index);
				*address = property->next;
				property->next = C_NULL;
				return 1;
			}
			address = &(property->next);
		}
		return 1;
	}
	else {
		while ((property = *address) && (property->flag & XS_INTERNAL_FLAG)) {
			if (property->kind == XS_ARRAY_KIND)
				return fxDeleteIndexProperty(the, property, index);
			address = &(property->next);
		}
		return 1;
	}
}

txBoolean fxOrdinaryGetOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot)
{
	txSlot* property = mxBehaviorGetProperty(the, instance, id, index, XS_OWN);
	if (property) {
		slot->flag = property->flag;
		slot->kind = property->kind;
		slot->value = property->value;
		return 1;
	}
	slot->flag = XS_NO_FLAG;
	slot->kind = XS_UNDEFINED_KIND;
	return 0;
}

txSlot* fxOrdinaryGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
    txSlot* result;
	mxCheck(the, instance->kind == XS_INSTANCE_KIND);
again:
	if (instance->ID) {
		txSlot* alias = the->aliasArray[instance->ID];
		if (alias)
			instance = alias;
	}
	if (id) {
		if (the->colors && (instance->flag & XS_DONT_MARSHALL_FLAG)) {
			if (id < the->keyOffset) {
				txID color = the->colors[id];
				if (color) {
					result = instance + color;
					if ((result->ID == id) && (result->kind != XS_INSTANCE_KIND))
						return result;
				}
			}
		}
		result = instance->next;
		while (result && (result->flag & XS_INTERNAL_FLAG))
			result = result->next;
		while (result) {
			if (result->ID == id)
				return result;
			result = result->next;
		}
	}
	else {
		result = instance->next;
		while (result && (result->flag & XS_INTERNAL_FLAG)) {
			if (result->kind == XS_ARRAY_KIND) {
				result = fxGetIndexProperty(the, result, index);
				if (result)
					return result;
				break;
			}		
			result = result->next;
		}		
	}
	if (flag) {
		txSlot* prototype = fxGetPrototype(the, instance);
		if (prototype) {
			if (prototype->flag & XS_EXOTIC_FLAG)
				return mxBehaviorGetProperty(the, prototype, id, index, flag);
			instance = prototype;
			goto again;
		}
	}
	return C_NULL;
}

txBoolean fxOrdinaryGetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* receiver, txSlot* value)
{
	txBoolean result = 0;
	txSlot* property;
	txSlot* prototype;
	mxPushUndefined();
	property = the->stack;
	mxPushNull();
	prototype = the->stack;
	if (!mxBehaviorGetOwnProperty(the, instance, id, index, property)) {
		if (mxBehaviorGetPrototype(the, instance, the->stack))
			result = mxBehaviorGetPropertyValue(the, prototype->value.reference, id, index, receiver, value);
		goto bail;
	}
	if (property->kind == XS_ACCESSOR_KIND) {
		txSlot* function = property->value.accessor.getter;
		if (!mxIsFunction(function))
			goto bail;
		/* THIS */
		mxPushSlot(receiver);
		/* FUNCTION */
		mxPushReference(function);
		mxCall();
		mxRunCount(0);
		mxPullSlot(value);
	}
	else {
		value->kind = property->kind;
		value->value = property->value;
	}
	result = 1;
bail:
	mxPop();
	mxPop();
	return result;
}

txBoolean fxOrdinaryGetPrototype(txMachine* the, txSlot* instance, txSlot* result)
{
	txSlot* prototype;
	if (instance->ID) {
		txSlot* alias = the->aliasArray[instance->ID];
		if (alias)
			instance = alias;
	}
	prototype = fxGetPrototype(the, instance);
	if (prototype) {
		result->kind = XS_REFERENCE_KIND;
		result->value.reference = prototype;
		return 1;
	}
	result->kind = XS_NULL_KIND;
	return 0;
}

txBoolean fxOrdinaryHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index) 
{
	txBoolean result;
	txSlot* property = mxBehaviorGetProperty(the, instance, id, index, XS_OWN);
	if (property)
		return 1;
	mxPushUndefined();
	if (mxBehaviorGetPrototype(the, instance, the->stack))
		result = mxBehaviorHasProperty(the, the->stack->value.reference, id, index);
	else
		result = 0;
	mxPop();
	return result;
}

txBoolean fxOrdinaryIsExtensible(txMachine* the, txSlot* instance)
{
	if (instance->ID) {
		txSlot* alias = the->aliasArray[instance->ID];
		if (alias)
			instance = alias;
	}
	return (instance->flag & XS_DONT_PATCH_FLAG) ? 0 : 1;
}

void fxOrdinaryOwnKeys(txMachine* the, txSlot* instance, txFlag flag, txSlot* keys)
{
	txSlot* property;
	if (instance->ID) {
		txSlot* alias = the->aliasArray[instance->ID];
		if (alias)
			instance = alias;
	}
	property = instance->next;
	while (property && (property->flag & XS_INTERNAL_FLAG)) {
		if (property->kind == XS_ARRAY_KIND)
			keys = fxQueueIndexKeys(the, property, flag, keys);
		property = property->next;
	}
	fxQueueIDKeys(the, property, flag, keys);
}

txBoolean fxOrdinaryPreventExtensions(txMachine* the, txSlot* instance)
{
	if (instance->ID) {
		txSlot* alias = the->aliasArray[instance->ID];
		if (alias)
			instance = alias;
	}
	if (instance->flag & XS_DONT_PATCH_FLAG)
		return 1;
	if (instance->ID)
		instance = fxAliasInstance(the, instance);
	instance->flag |= XS_DONT_PATCH_FLAG;
	return 1;
}

txSlot* fxOrdinarySetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	txSlot** address;
	txSlot* property;
	txSlot* result;
	if (instance->ID) {
		txSlot* alias = the->aliasArray[instance->ID];
		if (alias)
			instance = alias;
		else {
			property = mxBehaviorGetProperty(the, instance, id, index, flag);
			if (property) {
				if (property->kind == XS_ACCESSOR_KIND)
					return property;
				if (property->flag & XS_DONT_SET_FLAG)
					return property;
			}
			if (instance->flag & XS_DONT_PATCH_FLAG)
				return C_NULL;
			instance = fxAliasInstance(the, instance);
		}
	}
	if (id) {
		if (the->colors && (instance->flag & XS_DONT_MARSHALL_FLAG)) {
			if (id < the->keyOffset) {
				txID color = the->colors[id];
				if (color) {
					property = instance + color;
					if (property->ID == id)
						return property;
				}
			}
		}
		address = &(instance->next);
		while ((property = *address) && (property->flag & XS_INTERNAL_FLAG))
			address = &(property->next);
		while ((property = *address)) {
			if (property->ID == id)
				return property;
			address = &(property->next);
		}
	}
	else {
		address = &(instance->next);
		while ((property = *address) && (property->flag & XS_INTERNAL_FLAG)) {
			if (property->kind == XS_ARRAY_KIND) {
				result = fxGetIndexProperty(the, property, index);
				if (result)
					return result;
				break;
			}		
			address = &(property->next);
		}		
	}
	if (flag) {
		txSlot* prototype = fxGetPrototype(the, instance);
		if (prototype) {
			result = mxBehaviorGetProperty(the, prototype, id, index, flag);
			if (result) {
				if (result->kind == XS_ACCESSOR_KIND)
					return result;
				if (result->flag & XS_DONT_SET_FLAG)
					return result;
			}
		}
	}
	if (instance->flag & XS_DONT_PATCH_FLAG)
		return C_NULL;
	if (id) {
		*address = result = fxNewSlot(the);
		result->ID = id;
	}
	else {
		if (property && (property->kind == XS_ARRAY_KIND)) {
			result = fxSetIndexProperty(the, instance, property, index);
		}
		else {
			property = fxNewSlot(the);
			property->next = *address;
			property->flag = XS_INTERNAL_FLAG;
			property->ID = 0;
			property->kind = XS_ARRAY_KIND;
			property->value.array.address = C_NULL;
			property->value.array.length = 0;
			*address = property;
			result = fxSetIndexProperty(the, instance, property, index);
		}
	}
	return result;
}

txBoolean fxOrdinarySetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver)
{
	txBoolean result = 0;
	txSlot* property;
	txSlot* prototype;
	mxPushUndefined();
	property = the->stack;
	mxPushNull();
	prototype = the->stack;
	if (!mxBehaviorGetOwnProperty(the, instance, id, index, property)) {
		if (mxBehaviorGetPrototype(the, instance, prototype)) {
			result = mxBehaviorSetPropertyValue(the, prototype->value.reference, id, index, value, receiver);
			goto bail;
		}
	}
	if (property->kind == XS_ACCESSOR_KIND) {
		txSlot* function = property->value.accessor.setter;
		if (!mxIsFunction(function))
			goto bail;
		/* THIS */
		mxPushSlot(receiver);
		/* FUNCTION */
		mxPushReference(function);
		mxCall();
		mxPushSlot(value);
		mxRunCount(1);
		mxPop();
		result = 1;
		goto bail;
	}
	if (property->flag & XS_DONT_SET_FLAG)
		goto bail;
	value->flag = property->flag;
	if (receiver->kind != XS_REFERENCE_KIND)
		goto bail;
	if (mxBehaviorGetOwnProperty(the, receiver->value.reference, id, index, property)) {
		if (property->kind == XS_ACCESSOR_KIND)
			goto bail;
		if (property->flag & XS_DONT_SET_FLAG)
			goto bail;
	}
	result = mxBehaviorDefineOwnProperty(the, receiver->value.reference, id, index, value, XS_GET_ONLY);
bail:
	mxPop();
	mxPop();
	return result;
}

txBoolean fxOrdinarySetPrototype(txMachine* the, txSlot* instance, txSlot* slot)
{
	txSlot* prototype = (slot->kind == XS_NULL_KIND) ? C_NULL : slot->value.reference;
	if (instance->ID) {
		txSlot* alias = the->aliasArray[instance->ID];
		if (alias)
			instance = alias;
	}
	if (instance->value.instance.prototype != prototype) {
		if (instance->flag & XS_DONT_PATCH_FLAG)
			return 0;
		slot = prototype;
		while (slot) {
			if (instance == slot) 
				return 0;
			slot = fxGetPrototype(the, slot);
		}
		if (instance->ID)
			instance = fxAliasInstance(the, instance);
		instance->value.instance.prototype = prototype;
	}
	return 1;
}

void fxOrdinaryToPrimitive(txMachine* the)
{
	if (mxIsReference(mxThis)) {
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
			mxPushSlot(mxThis);
			mxPushSlot(mxThis);
			mxGetID(ids[i]);
			if (fxIsCallable(the, the->stack)) {
				mxCall();
				mxRunCount(0);
				if (mxIsReference(the->stack))
					mxPop();
				else {
					mxPullSlot(mxResult);
					return;
      			}
			}
			else {
				mxPop();
				mxPop();
			}
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

void fx_species_get(txMachine* the)
{
	*mxResult = *mxThis;
}

txFlag fxDescriptorToSlot(txMachine* the, txSlot* descriptor)
{
	txSlot* stack = the->stack;
	txSlot* configurable = C_NULL;
	txSlot* enumerable = C_NULL;
	txSlot* get = C_NULL;
	txSlot* set = C_NULL;
	txSlot* value = C_NULL;
	txSlot* writable = C_NULL;
	txSlot* getFunction = C_NULL;
	txSlot* setFunction = C_NULL;
	txFlag mask = 0;
	if (!mxIsReference(descriptor))
		mxTypeError("descriptor is no object");
	mxPushSlot(descriptor);
	if (mxHasID(mxID(_enumerable))) {
		mxPushSlot(descriptor);
		mxGetID(mxID(_enumerable));
		enumerable = the->stack;
	}
	mxPushSlot(descriptor);
	if (mxHasID(mxID(_configurable))) {
		mxPushSlot(descriptor);
		mxGetID(mxID(_configurable));
		configurable = the->stack;
	}
	mxPushSlot(descriptor);
	if (mxHasID(mxID(_value))) {
		mxPushSlot(descriptor);
		mxGetID(mxID(_value));
		value = the->stack;
	}
	mxPushSlot(descriptor);
	if (mxHasID(mxID(_writable))) {
		mxPushSlot(descriptor);
		mxGetID(mxID(_writable));
		writable = the->stack;
	}
	mxPushSlot(descriptor);
	if (mxHasID(mxID(_get))) {
		mxPushSlot(descriptor);
		mxGetID(mxID(_get));
		get = the->stack;
	}
	mxPushSlot(descriptor);
	if (mxHasID(mxID(_set))) {
		mxPushSlot(descriptor);
		mxGetID(mxID(_set));
		set = the->stack;
	}
	if (get) {
		if (value)
			mxTypeError("get and value");
		if (writable)
			mxTypeError("get and writable");
		if (get->kind != XS_UNDEFINED_KIND) {
			getFunction = fxToInstance(the, get);
			if (!getFunction || !mxIsFunction(getFunction))
				mxTypeError("get is no function");
		}
		mask |= XS_GETTER_FLAG;
	}
	if (set) {
		if (value)
			mxTypeError("set and value");
		if (writable)
			mxTypeError("set and writable");
		if (set->kind != XS_UNDEFINED_KIND) {
			setFunction = fxToInstance(the, set);
			if (!setFunction || !mxIsFunction(setFunction))
				mxTypeError("set is no function");
		}
		mask |= XS_SETTER_FLAG;
	}
	descriptor->flag = 0;
	if (configurable) {
		mask |= XS_DONT_DELETE_FLAG;
		if (!fxToBoolean(the, configurable))
			descriptor->flag |= XS_DONT_DELETE_FLAG;
	}
	if (enumerable) {
		mask |= XS_DONT_ENUM_FLAG;
		if (!fxToBoolean(the, enumerable))
			descriptor->flag |= XS_DONT_ENUM_FLAG;
	}
	if (get || set) {
		descriptor->kind = XS_ACCESSOR_KIND;
		descriptor->value.accessor.getter = getFunction;
		descriptor->value.accessor.setter = setFunction;
	}
	else {
		if (writable) {
			mask |= XS_DONT_SET_FLAG;
			if (!fxToBoolean(the, writable))
				descriptor->flag |= XS_DONT_SET_FLAG;
		}
		if (value) {
			descriptor->kind = value->kind;
			descriptor->value = value->value;
		}
		else {
			descriptor->kind = XS_UNINITIALIZED_KIND;
		}
	}
	the->stack = stack;
	return mask;
}

void fxDescribeProperty(txMachine* the, txSlot* property, txFlag mask)
{
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	if (property->kind == XS_ACCESSOR_KIND) {
		slot = fxNextUndefinedProperty(the, slot, mxID(_get), XS_NO_FLAG);
		if (property->value.accessor.getter) {
			slot->kind = XS_REFERENCE_KIND;
			slot->value.reference = property->value.accessor.getter;
		}
		slot = fxNextUndefinedProperty(the, slot, mxID(_set), XS_NO_FLAG);
		if (property->value.accessor.setter) {
			slot->kind = XS_REFERENCE_KIND;
			slot->value.reference = property->value.accessor.setter;
		}
	}
	else {
		if (property->kind != XS_UNINITIALIZED_KIND)
			slot = fxNextSlotProperty(the, slot, property, mxID(_value), XS_NO_FLAG);
		if (mask & XS_DONT_SET_FLAG)
			slot = fxNextBooleanProperty(the, slot, (property->flag & XS_DONT_SET_FLAG) ? 0 : 1, mxID(_writable), XS_NO_FLAG);
	}
	if (mask & XS_DONT_ENUM_FLAG)
		slot= fxNextBooleanProperty(the, slot, (property->flag & XS_DONT_ENUM_FLAG) ? 0 : 1, mxID(_enumerable), XS_NO_FLAG);
	if (mask & XS_DONT_DELETE_FLAG)
		slot= fxNextBooleanProperty(the, slot, (property->flag & XS_DONT_DELETE_FLAG) ? 0 : 1, mxID(_configurable), XS_NO_FLAG);
}

txBoolean fxIsPropertyCompatible(txMachine* the, txSlot* property, txSlot* slot, txFlag mask)
{
	if (property->flag & XS_DONT_DELETE_FLAG) {
		if ((mask & XS_DONT_DELETE_FLAG) && !(slot->flag & XS_DONT_DELETE_FLAG))
			return 0;
		if ((mask & XS_DONT_ENUM_FLAG) && ((property->flag & XS_DONT_ENUM_FLAG) != (slot->flag & XS_DONT_ENUM_FLAG)))
			return 0;
		if (mask & XS_ACCESSOR_FLAG) {
			if (property->kind != XS_ACCESSOR_KIND)
				return 0;
			if (mask & XS_GETTER_FLAG) {
				if (property->value.accessor.getter != slot->value.accessor.getter)
					return 0;
			}
			if (mask & XS_SETTER_FLAG) {
				if (property->value.accessor.setter != slot->value.accessor.setter)
					return 0;
			}
		}
		else if ((mask & XS_DONT_SET_FLAG) || (slot->kind != XS_UNINITIALIZED_KIND)) {
			if (property->kind == XS_ACCESSOR_KIND)
				return 0;
			if (property->flag & XS_DONT_SET_FLAG) {
				if ((mask & XS_DONT_SET_FLAG) && !(slot->flag & XS_DONT_SET_FLAG))
					return 0;
				if ((slot->kind != XS_UNINITIALIZED_KIND) && !fxIsSameValue(the, property, slot, 0))
					return 0;
			}
		}
	}
	return 1;
}

static txBoolean fxEnvironmentDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask);
static txBoolean fxEnvironmentDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static txSlot* fxEnvironmentGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
static txBoolean fxEnvironmentHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static txSlot* fxEnvironmentSetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);

const txBehavior ICACHE_FLASH_ATTR gxEnvironmentBehavior = {
	fxEnvironmentGetProperty,
	fxEnvironmentSetProperty,
	fxOrdinaryCall,
	fxOrdinaryConstruct,
	fxEnvironmentDefineOwnProperty,
	fxEnvironmentDeleteProperty,
	fxOrdinaryGetOwnProperty,
	fxOrdinaryGetPropertyValue,
	fxOrdinaryGetPrototype,
	fxEnvironmentHasProperty,
	fxOrdinaryIsExtensible,
	fxOrdinaryOwnKeys,
	fxOrdinaryPreventExtensions,
	fxOrdinarySetPropertyValue,
	fxOrdinarySetPrototype,
};

txSlot* fxNewEnvironmentInstance(txMachine* the, txSlot* environment)
{
	txSlot* with = the->stack;
	txSlot* instance = fxNewSlot(the);
	txSlot* slot;
	instance->flag = XS_EXOTIC_FLAG;
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = (environment && (environment->kind == XS_REFERENCE_KIND)) ? environment->value.reference : C_NULL;
	mxPushReference(instance);
	slot = instance->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG;
	slot->ID = XS_ENVIRONMENT_BEHAVIOR;
	slot->kind = with->kind;
	slot->value = with->value;
    mxPop();
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;
	return instance;
}

txBoolean fxEnvironmentDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask) 
{
	txSlot* property = fxOrdinarySetProperty(the, instance, id, index, XS_OWN);
	if (!property)
		return 0;
	property->flag = slot->flag & mask;
	property->kind = slot->kind;
	property->value = slot->value;
	return 1;
}

txBoolean fxEnvironmentDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	if (id) {
		txSlot** address = &(instance->next->next);
		txSlot* property;
		while ((property = *address)) {
			if (property->ID == id) {
				if (property->flag & XS_DONT_DELETE_FLAG)
					return 0;
				*address = property->next;
				property->next = C_NULL;
				return 1;
			}
			address = &(property->next);
		}
		return 1;
	}
	return 0;
}

txSlot* fxEnvironmentGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	txID alias;
	if (id) {
		txSlot* result = instance->next->next;
		while (result) {
			if (result->ID == id) {
				if (result->kind == XS_CLOSURE_KIND) {
					result = result->value.closure;
					if (result) {
						alias = result->ID;
						if (alias) {
							txSlot* slot = the->aliasArray[alias];
							if (slot)
								result = slot;
						}	
					}	
				}	
				return result;
			}
			result = result->next;
		}
	}
	return C_NULL;
}

txBoolean fxEnvironmentHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index) 
{
	if (id) {
		txSlot* result = instance->next->next;
		while (result) {
			if (result->ID == id) {
				return 1;
			}
			result = result->next;
		}
	}
	return 0;
}

txSlot* fxEnvironmentSetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	txID alias;
	if (id) {
		txSlot* result = instance->next->next;
		while (result) {
			if (result->ID == id) {
				if (result->kind == XS_CLOSURE_KIND) {
					result = result->value.closure;
					if (result->flag & XS_DONT_SET_FLAG)
						mxDebugID(XS_TYPE_ERROR, "set %s: const", id);
					alias = result->ID;
					if (alias) {
						result = the->aliasArray[alias];
						if (!result) {
							result = fxNewSlot(the);
							the->aliasArray[alias] = result;
						}
					}	
				}	
				return result;
			}
			result = result->next;
		}
	}
	return C_NULL;
}

void fxRunEvalEnvironment(txMachine* the)
{
	txSlot* function = mxFunction;
	txSlot* module = mxFunctionInstanceHome(function->value.reference)->value.home.module;
	txSlot* realm = mxModuleInstanceInternal(module)->value.module.realm;
	txSlot* global = mxRealmGlobal(realm)->value.reference;
	txSlot* top = mxFrameToEnvironment(the->frame);
	txSlot* bottom = the->scope;
	txSlot* slot;
	txSlot* property;
	txSlot* currentEnvironment = top->value.reference;
	txSlot* varEnvironment = currentEnvironment;
	top--;
	while (varEnvironment) {
		property = varEnvironment->next;
		if (property->kind == XS_NULL_KIND)
			break;
		varEnvironment = varEnvironment->value.instance.prototype;
	}
	slot = top;
	while (slot >= bottom) {
		txSlot* environment = currentEnvironment;
		while (environment != varEnvironment) {
			if (mxBehaviorHasProperty(the, environment, slot->ID, 0))
				mxDebugID(XS_SYNTAX_ERROR, "%s: duplicate variable", slot->ID);
			environment = environment->value.instance.prototype;
		}
		slot--;
	}
	if (varEnvironment) {
		slot = top;
		while (slot >= bottom) {
			property = mxBehaviorGetProperty(the, varEnvironment, slot->ID, 0, XS_OWN);
			if (!property) {
				slot->value.closure = fxNewSlot(the);
				slot->kind = XS_CLOSURE_KIND;
				mxBehaviorDefineOwnProperty(the, varEnvironment, slot->ID, 0, slot, XS_NO_FLAG); // configurable variable!
			}
			slot--;
		}
	}
	else {
		mxPushUndefined();
		slot = bottom;
		while (slot <= top) {
			if (slot->kind == XS_NULL_KIND) {
				property = the->stack;
				if (!mxBehaviorGetOwnProperty(the, global, slot->ID, 0, property)) {
					if (!mxBehaviorIsExtensible(the, global))
						mxDebugID(XS_TYPE_ERROR, "%s: global object not extensible", slot->ID);
				}
				else if (property->flag & XS_DONT_DELETE_FLAG) {
					if (property->flag & (XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG))
						mxDebugID(XS_TYPE_ERROR, "%s: global property not configurable and not enumerable or writable", slot->ID);
				}
			}
			slot++;
		}
		slot = top;
		while (slot >= bottom) {
			if (slot->kind == XS_UNDEFINED_KIND) {
				property = the->stack;
				if (!mxBehaviorGetOwnProperty(the, global, slot->ID, 0, property)) {
					if (!mxBehaviorIsExtensible(the, global))
						mxDebugID(XS_TYPE_ERROR, "%s: global object not extensible", slot->ID);
				}
			}
			slot--;
		}
		mxPop();
		slot = bottom;
		while (slot <= top) {
			if (slot->kind == XS_NULL_KIND) {
				property = mxBehaviorSetProperty(the, global, slot->ID, 0, XS_OWN);
				if (!(property->flag & XS_DONT_DELETE_FLAG))
					property->flag = XS_NO_FLAG;
			}
			slot++;
		}
		slot = top;
		while (slot >= bottom) {
			if (slot->kind == XS_UNDEFINED_KIND) {
				property = mxBehaviorSetProperty(the, global, slot->ID, 0, XS_OWN);
			}
			slot--;
		}
	}
	the->stack = the->scope = top + 1;
}

void fxRunProgramEnvironment(txMachine* the)
{
	txSlot* function = mxFunction;
	txSlot* module = mxFunctionInstanceHome(function->value.reference)->value.home.module;
	txSlot* realm = mxModuleInstanceInternal(module)->value.module.realm;
	txSlot* environment = mxRealmClosures(realm)->value.reference;
	txSlot* global = mxRealmGlobal(realm)->value.reference;
	txSlot* top = mxFrameToEnvironment(the->frame);
	txSlot* middle = C_NULL;
	txSlot* bottom = the->scope;
	txSlot* slot;
	txSlot* property;
	top--;
	slot = top;
	while (slot >= bottom) {
		if (slot->kind == XS_CLOSURE_KIND) {
			property = mxBehaviorGetProperty(the, environment, slot->ID, 0, XS_OWN);
			if (property)
				mxDebugID(XS_SYNTAX_ERROR, "%s: duplicate variable", slot->ID);
			property = mxBehaviorGetProperty(the, global, slot->ID, 0, XS_OWN);
			if (property && (property->flag & XS_DONT_DELETE_FLAG))
				mxDebugID(XS_SYNTAX_ERROR, "%s: restricted variable", slot->ID);
		}
		else
			break;
		
		slot--;
	}
	middle = slot;
	while (slot >= bottom) {
		property = mxBehaviorGetProperty(the, environment, slot->ID, 0, XS_OWN);
		if (property)
			mxDebugID(XS_SYNTAX_ERROR, "%s: duplicate variable", slot->ID);
		slot--;
	}
	mxPushUndefined();
	slot = bottom;
	while (slot <= middle) {
		if (slot->kind == XS_NULL_KIND) {
			property = the->stack;
			if (!mxBehaviorGetOwnProperty(the, global, slot->ID, 0, property)) {
				if (!mxBehaviorIsExtensible(the, global))
					mxDebugID(XS_TYPE_ERROR, "%s: global object not extensible", slot->ID);
			}
			else if (property->flag & XS_DONT_DELETE_FLAG) {
				if (property->flag & (XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG))
					mxDebugID(XS_TYPE_ERROR, "%s: global property not configurable and not enumerable or writable", slot->ID);
			}
		}
		slot++;
	}
	while (slot >= bottom) {
		if (slot->kind == XS_UNDEFINED_KIND) {
			property = the->stack;
			if (!mxBehaviorGetOwnProperty(the, global, slot->ID, 0, property)) {
				if (!mxBehaviorIsExtensible(the, global))
					mxDebugID(XS_TYPE_ERROR, "%s: global object not extensible", slot->ID);
			}
		}
        slot--;
	}
	
	slot = top;
	while (slot > middle) {
		mxBehaviorDefineOwnProperty(the, environment, slot->ID, 0, slot, XS_GET_ONLY);
		slot--;
	}
	slot = bottom;
	while (slot <= middle) {
		if (slot->kind == XS_NULL_KIND) {
			property = mxBehaviorSetProperty(the, global, slot->ID, 0, XS_OWN);
			if (!(property->flag & XS_DONT_DELETE_FLAG))
				property->flag = XS_DONT_DELETE_FLAG;
			slot->value.closure = property;
			slot->kind = XS_CLOSURE_KIND;
		}
		slot++;
	}
	while (slot >= bottom) {
		if (slot->kind == XS_UNDEFINED_KIND) {
			property = mxBehaviorGetProperty(the, global, slot->ID, 0, XS_OWN);
			if (!property) {
				property = mxBehaviorSetProperty(the, global, slot->ID, 0, XS_OWN);
				property->flag = XS_DONT_DELETE_FLAG;
			}
			slot->value.closure = property;
			slot->kind = XS_CLOSURE_KIND;
		}
		slot--;
	}
	mxPop();
	the->stack = the->scope = middle + 1;
}


txSlot* fxNewRealmInstance(txMachine* the)
{
	txSlot* parent = the->stack + 9;
	txSlot* global = the->stack + 8;
	txSlot* moduleMap = the->stack + 7;
	txSlot* own = the->stack + 6;
	txSlot* closures = the->stack + 5;
	txSlot* resolveHook = the->stack + 4;
	txSlot* moduleMapHook = the->stack + 3;
	txSlot* loadHook = the->stack + 2;
	txSlot* loadNowHook = the->stack + 1;
	txSlot* importMetaHook = the->stack;
	txSlot* realm = fxNewInstance(the);
	txSlot* slot;
	/* mxRealmGlobal */
	slot = fxNextSlotProperty(the, realm, global, XS_NO_ID, XS_GET_ONLY);
	/* mxRealmClosures */
	slot = fxNextSlotProperty(the, slot, closures, XS_NO_ID, XS_GET_ONLY);
	/* mxRealmTemplateCache */
	slot = fxNextReferenceProperty(the, slot, fxNewInstance(the), XS_NO_ID, XS_GET_ONLY);
	mxPop();
	/* mxOwnModules */
	slot = fxNextSlotProperty(the, slot, own, XS_NO_ID, XS_GET_ONLY);
	/* mxResolveHook */
	slot = fxNextSlotProperty(the, slot, resolveHook, XS_NO_ID, XS_GET_ONLY);
	/* mxModuleMap */
	slot = fxNextSlotProperty(the, slot, moduleMap, XS_NO_ID, XS_GET_ONLY);
	/* mxModuleMapHook */
	slot = fxNextSlotProperty(the, slot, moduleMapHook, XS_NO_ID, XS_GET_ONLY);
	/* mxLoadHook */
	slot = fxNextSlotProperty(the, slot, loadHook, XS_NO_ID, XS_GET_ONLY);
	/* mxLoadNowHook */
	slot = fxNextSlotProperty(the, slot, loadNowHook, XS_NO_ID, XS_GET_ONLY);
	/* mxImportMetaHook */
	slot = fxNextSlotProperty(the, slot, importMetaHook, XS_NO_ID, XS_GET_ONLY);
	/* mxRealmParent */
	slot = fxNextSlotProperty(the, slot, parent, XS_NO_ID, XS_GET_ONLY);
    parent->value.reference = realm;
    the->stack = parent;
	return realm;
}

txSlot* fxNewProgramInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* slot;
	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = mxIsReference(the->stack) ? the->stack->value.reference : C_NULL;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;
	slot = instance->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG;
	slot->kind = XS_PROGRAM_KIND;
	slot->value.module.realm = C_NULL;
	slot->value.module.id = XS_NO_ID;
	return instance;
}

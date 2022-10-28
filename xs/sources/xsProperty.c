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

txSlot* fxLastProperty(txMachine* the, txSlot* slot)
{
	txSlot* property;
	while ((property = slot->next))
		slot = property;
	return slot;
}

#ifndef mxLink
txSlot* fxNextHostAccessorProperty(txMachine* the, txSlot* property, txCallback get, txCallback set, txID id, txFlag flag)
{
	txSlot *getter = NULL, *setter = NULL, *home = the->stack, *slot;
	if (get) {
		getter = fxBuildHostFunction(the, get, 0, XS_NO_ID);
		slot = mxFunctionInstanceHome(getter);
		slot->value.home.object = home->value.reference;
		fxRenameFunction(the, getter, id, 0, XS_NO_ID, "get ");
	}
	if (set) {
		setter = fxBuildHostFunction(the, set, 1, XS_NO_ID);
		slot = mxFunctionInstanceHome(setter);
		slot->value.home.object = home->value.reference;
		fxRenameFunction(the, setter, id, 0, XS_NO_ID, "set ");
	}
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_ACCESSOR_KIND;
	property->value.accessor.getter = getter;
	property->value.accessor.setter = setter;
	if (set)
		mxPop();
	if (get)
		mxPop();
	return property;
}

txSlot* fxNextHostFunctionProperty(txMachine* the, txSlot* property, txCallback call, txInteger length, txID id, txFlag flag)
{
	txSlot *function, *home = the->stack, *slot;
	function = fxNewHostFunction(the, call, length, id, XS_NO_ID);
	slot = mxFunctionInstanceHome(function);
	slot->value.home.object = home->value.reference;
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	mxPop();
	return property;
}
#endif

txSlot* fxNextUndefinedProperty(txMachine* the, txSlot* property, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_UNDEFINED_KIND;
	return property;
}

txSlot* fxNextNullProperty(txMachine* the, txSlot* property, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_NULL_KIND;
	return property;
}

txSlot* fxNextBooleanProperty(txMachine* the, txSlot* property, txBoolean boolean, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_BOOLEAN_KIND;
	property->value.boolean = boolean;
	return property;
}

txSlot* fxNextIntegerProperty(txMachine* the, txSlot* property, txInteger integer, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_INTEGER_KIND;
	property->value.integer = integer;
	return property;
}

txSlot* fxNextNumberProperty(txMachine* the, txSlot* property, txNumber number, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_NUMBER_KIND;
	property->value.number = number;
	return property;
}

txSlot* fxNextReferenceProperty(txMachine* the, txSlot* property, txSlot* slot, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_REFERENCE_KIND;
	property->value.reference = slot;
	return property;
}

txSlot* fxNextSlotProperty(txMachine* the, txSlot* property, txSlot* slot, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = slot->kind;
	property->value = slot->value;
	return property;
}

txSlot* fxNextStringProperty(txMachine* the, txSlot* property, txString string, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	fxCopyStringC(the, property, string);
	return property;
}

txSlot* fxNextStringXProperty(txMachine* the, txSlot* property, txString string, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
#ifdef mxSnapshot
	fxCopyStringC(the, property, string);
#else
	property->kind = XS_STRING_X_KIND;
	property->value.string = string;
#endif
	return property;
}


txSlot* fxNextSymbolProperty(txMachine* the, txSlot* property, txID symbol, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_SYMBOL_KIND;
	property->value.symbol = symbol;
	return property;
}

txSlot* fxNextTypeDispatchProperty(txMachine* the, txSlot* property, txTypeDispatch* dispatch, txTypeAtomics* atomics, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_TYPED_ARRAY_KIND;
	property->value.typedArray.dispatch = dispatch;
	property->value.typedArray.atomics = atomics;
	return property;
}

txSlot* fxQueueKey(txMachine* the, txID id, txIndex index, txSlot* keys)
{
	keys = keys->next = fxNewSlot(the);
	keys->kind = XS_AT_KIND;
	keys->value.at.id = id;
	keys->value.at.index = index;
	return keys;
}

txSlot* fxQueueIDKeys(txMachine* the, txSlot* first, txFlag flag, txSlot* keys)
{
	if (flag & XS_EACH_NAME_FLAG) {
		txSlot* property = first;
		while (property) {
			if (!(property->flag & XS_INTERNAL_FLAG) && fxIsKeyName(the, property->ID))
				keys = fxQueueKey(the, property->ID, 0, keys);
			property = property->next;
		}
	}
	if (flag & XS_EACH_SYMBOL_FLAG) {
		txSlot* property = first;
		while (property) {
			if (!(property->flag & XS_INTERNAL_FLAG) && fxIsKeySymbol(the, property->ID))
				keys = fxQueueKey(the, property->ID, 0, keys);
			property = property->next;
		}
	}
	return keys;
}

// INDEX

static txSize fxSizeToCapacity(txMachine* the, txSize size)
{
	return fxAddChunkSizes(the, size, size / 3);
}

txBoolean fxDeleteIndexProperty(txMachine* the, txSlot* array, txIndex index) 
{
	txSlot* address = array->value.array.address;
	if (address) {
		txIndex length = array->value.array.length;
		txIndex size = (((txChunk*)(((txByte*)address) - sizeof(txChunk)))->size) / sizeof(txSlot);
		txSlot* result = address;
		txSlot* limit = result + size;
		if (length == size)
			result = address + index;
		else {
			txIndex at;
			while (result < limit) {
				at = *((txIndex*)result);
				if (at == index)
					break;
				if (at > index)
					return 1;
				result++;
			}
		}
		if (result < limit) {
			if (result->flag & XS_DONT_DELETE_FLAG)
				return 0;
			index = (txIndex)(result - address);
			size--;
			if (size > 0) {
				txSlot* chunk = (txSlot*)fxNewChunk(the, size * sizeof(txSlot));
				address = array->value.array.address;
				if (index > 0)
					c_memcpy(chunk, address, index * sizeof(txSlot));
				if (index < size)
					c_memcpy(chunk + index, address + index + 1, (size - index) * sizeof(txSlot));
				array->value.array.address = chunk;
			}
			else
				array->value.array.address = C_NULL;
		}
	}
	return 1;
}

txSlot* fxGetIndexProperty(txMachine* the, txSlot* array, txIndex index) 
{
	txSlot* address = array->value.array.address;
	if (address) {
		txIndex length = array->value.array.length;
		txIndex size = (((txChunk*)(((txByte*)address) - sizeof(txChunk)))->size) / sizeof(txSlot);
		if (length == size) {
			if (index < length)
				return address + index;
		}
		else {
			txSlot* result = address;
			txSlot* limit = result + size;
			txIndex at;
			while (result < limit) {
				at = *((txIndex*)result);
				if (at == index)
					return result;
				result++;
			}
		}
	}
	return C_NULL;
}

txIndex fxGetIndexSize(txMachine* the, txSlot* array)
{
	txSlot* address = array->value.array.address;
	if (address)
		return (((txChunk*)(((txByte*)address) - sizeof(txChunk)))->size) / sizeof(txSlot);
	return 0;
}

txSlot* fxQueueIndexKeys(txMachine* the, txSlot* array, txFlag flag, txSlot* keys)
{
	if (flag & XS_EACH_NAME_FLAG) {
		if (array->value.array.address) {
			txSize offset = 0;
			txSize size = (((txChunk*)(((txByte*)array->value.array.address) - sizeof(txChunk)))->size) / sizeof(txSlot);
			while (offset < size) {
				txSlot* slot = array->value.array.address + offset;
				txIndex index = *((txIndex*)slot);
				keys = fxQueueKey(the, 0, index, keys);
				offset++;
			}
		}
	}
	return keys;
}

txSlot* fxSetIndexProperty(txMachine* the, txSlot* instance, txSlot* array, txIndex index) 
{
	txSlot* address = array->value.array.address;
	txSlot* chunk = address;
	txIndex length = array->value.array.length;
	txIndex current;
	txSize size;
	txSlot* result;
	txSlot* limit;
	txIndex at;
	if (address) {
		current = (((txChunk*)(((txByte*)chunk) - sizeof(txChunk)))->size) / sizeof(txSlot);
		if (length == current) {
			if (index < length) 
				return address + index;
			if (instance->flag & XS_DONT_PATCH_FLAG)
				return C_NULL;
			if (array->flag & XS_DONT_SET_FLAG)
				return C_NULL;
			current++;
			size = fxMultiplyChunkSizes(the, current, sizeof(txSlot));
			chunk = (txSlot*)fxRenewChunk(the, address, size);
			if (!chunk) {
			#ifndef mxNoArrayOverallocation
				if (array->ID == XS_ARRAY_BEHAVIOR) {
					txSize capacity = fxSizeToCapacity(the, size);
					chunk = (txSlot*)fxNewGrowableChunk(the, size, capacity);
				}
				else
			#endif
					chunk = (txSlot*)fxNewChunk(the, size);
				address = array->value.array.address;
				c_memcpy(chunk, address, length * sizeof(txSlot));
			}
			result = chunk + length;
		}
		else {
			result = address;
			limit = result + current;
			while (result < limit) {
				at = *((txIndex*)result);
				if (at == index)
					return result;
				if (at > index)
					break;
				result++;
			}
			if (instance->flag & XS_DONT_PATCH_FLAG)
				return C_NULL;
			if ((array->flag & XS_DONT_SET_FLAG) && (index >= length))
				return C_NULL;
			at = mxPtrDiff(result - address);
			current++;
			size = fxMultiplyChunkSizes(the, current, sizeof(txSlot));
			chunk = (txSlot*)fxNewChunk(the, size);
			address = array->value.array.address;
			result = address + at;
			limit = address + current - 1;
			if (result > address)
				c_memcpy(chunk, address, (result - address) * sizeof(txSlot));
			if (result < limit)
				c_memcpy(chunk + (result - address) + 1, result, (limit - result) * sizeof(txSlot));
			result = chunk + (result - address);
		}
	}
	else {
        if (instance->flag & XS_DONT_PATCH_FLAG)
            return C_NULL;
		if ((array->flag & XS_DONT_SET_FLAG) && (index >= length))
			return C_NULL;
		current = 1;
		chunk = (txSlot*)fxNewChunk(the, sizeof(txSlot));
		result = chunk;
	}
	result->next = C_NULL;
	result->ID = XS_NO_ID;
	result->flag = XS_NO_FLAG;
	result->kind = XS_UNDEFINED_KIND;
	*((txIndex*)result) = index;	
	array->value.array.address = chunk;
	if (index >= length) {
		array->value.array.length = index + 1;
	}
	return result;
}

void fxSetIndexSize(txMachine* the, txSlot* array, txIndex target, txBoolean growable)
{
	txSlot* address = array->value.array.address;
	txSlot* chunk = C_NULL;
	txIndex current = (address) ? (((txChunk*)(((txByte*)address) - sizeof(txChunk)))->size) / sizeof(txSlot) : 0;
	txSize size;
	if (current != target) {
		if (array->flag & XS_DONT_SET_FLAG)
			mxTypeError("set length: not writable");
		size = fxMultiplyChunkSizes(the, target, sizeof(txSlot));
		if (address) {
			if (target) {
				chunk = (txSlot*)fxRenewChunk(the, address, size);
				if (!chunk) {
				#ifndef mxNoArrayOverallocation
					if (growable) {
						txSize capacity = fxSizeToCapacity(the, size);
						chunk = (txSlot*)fxNewGrowableChunk(the, size, capacity);
					}
					else
				#endif
						chunk = (txSlot*)fxNewChunk(the, size);
					address = array->value.array.address;
					if (current < target)
						c_memcpy(chunk, address, current * sizeof(txSlot));
					else
						c_memcpy(chunk, address, size);
				}
			}
		}
		else {
		#ifndef mxNoArrayOverallocation
			if (growable) {
				txSize capacity = fxSizeToCapacity(the, size);
				chunk = (txSlot*)fxNewGrowableChunk(the, size, capacity);
			}
			else
		#endif
				chunk = (txSlot*)fxNewChunk(the, size);
		}
		if (current < target)
			c_memset(chunk + current, 0, (target - current) * sizeof(txSlot));
		array->value.array.length = target;
		array->value.array.address = chunk;
	}
}

txBoolean fxDefinePrivateProperty(txMachine* the, txSlot* instance, txSlot* check, txID id, txSlot* slot, txFlag mask) 
{
	txSlot** address;
	txSlot* property;
	mxCheck(the, instance->kind == XS_INSTANCE_KIND);
	address = &(instance->next);
	while ((property = *address)) {
		if (!(property->flag & XS_INTERNAL_FLAG)) {
			property = C_NULL;
			break;
		}
		if (property->kind == XS_PRIVATE_KIND) {
			if (property->value.private.check == check) {
				break;
			}
		}
		address = &(property->next);
	}
	if (!property) {
		property = fxNewSlot(the);
		property->next = *address;
		property->flag = XS_INTERNAL_FLAG;
		property->kind = XS_PRIVATE_KIND;
		property->value.private.check = check;
		property->value.private.first = C_NULL;
		*address = property;
	}
	address = &(property->value.private.first);
	while ((property = *address)) {
		if (property->ID == id)
			break;
		address = &(property->next);
	}
	if (mask & XS_ACCESSOR_FLAG) {
		if (property) {
			if (property->kind != XS_ACCESSOR_KIND)
				return 0;
		}
		else {
			*address = property = fxNewSlot(the);
			property->ID = id;
			property->kind = XS_ACCESSOR_KIND;
			property->value.accessor.getter = C_NULL;
			property->value.accessor.setter = C_NULL;
		}
		if (mask & XS_GETTER_FLAG) {
			txSlot* function = slot->value.accessor.getter;
			if (property->value.accessor.getter)
				return 0;
			property->value.accessor.getter = function;
			if (mxIsFunction(function)) {
				if (((mask & XS_METHOD_FLAG) && (function->flag & XS_MARK_FLAG) == 0)) {
					txSlot* home = mxFunctionInstanceHome(function);
					home->value.home.object = instance;
				}
				if ((mask & XS_NAME_FLAG) && ((function->flag & XS_MARK_FLAG) == 0))
					fxRenameFunction(the, function, id, 0, mxID(_get), "get ");
			}
		}
		else {
			txSlot* function = slot->value.accessor.setter;
			if (property->value.accessor.setter)
				return 0;
			property->value.accessor.setter = function;
			if (mxIsFunction(function)) {
				if (((mask & XS_METHOD_FLAG) && (function->flag & XS_MARK_FLAG) == 0)) {
					txSlot* home = mxFunctionInstanceHome(function);
					home->value.home.object = instance;
				}
				if ((mask & XS_NAME_FLAG) && ((function->flag & XS_MARK_FLAG) == 0))
					fxRenameFunction(the, function, id, 0, mxID(_set), "set ");
			}
		}
	}
	else {
		if (property)
			return 0;
		*address = property = fxNewSlot(the);
		property->flag = (mask & XS_METHOD_FLAG) ? XS_DONT_SET_FLAG : XS_NO_FLAG;
		property->ID = id;
		property->kind = slot->kind;
		property->value = slot->value;
		if (property->kind == XS_REFERENCE_KIND) {
			txSlot* function = slot->value.reference;
			if (mxIsFunction(function)) {
				if ((mask & XS_METHOD_FLAG) && ((function->flag & XS_MARK_FLAG) == 0)) {
					txSlot* home = mxFunctionInstanceHome(function);
					home->value.home.object = instance;
				}
				if ((mask & XS_NAME_FLAG) && ((function->flag & XS_MARK_FLAG) == 0))
					fxRenameFunction(the, function, id, 0, mxID(_value), C_NULL);
			}
		}
	}
	return 1;
}

txSlot* fxGetPrivateProperty(txMachine* the, txSlot* instance, txSlot* check, txID id) 
{
    txSlot* result;
	mxCheck(the, instance->kind == XS_INSTANCE_KIND);
	if (instance->ID) {
		txSlot* alias = the->aliasArray[instance->ID];
		if (alias)
			instance = alias;
	}
	result = instance->next;
	while (result) {
		if (result->kind == XS_PRIVATE_KIND) {
			if (result->value.private.check == check)
				break;
		}
		result = result->next;
	}
	if (result) {
		result = result->value.private.first;
		while (result) {
			if (result->ID == id)
				break;
			result = result->next;
		}
	}
	if (result) {
		if ((result->kind == XS_ACCESSOR_KIND) && (result->value.accessor.getter == C_NULL))
			result = C_NULL;
	}
	return result;
}

txSlot* fxSetPrivateProperty(txMachine* the, txSlot* instance, txSlot* check, txID id) 
{
    txSlot* result;
	mxCheck(the, instance->kind == XS_INSTANCE_KIND);
	if (instance->ID) {
		txSlot* alias = the->aliasArray[instance->ID];
		if (alias)
			instance = alias;
		else
			instance = fxAliasInstance(the, instance);
	}
	result = instance->next;
	while (result) {
		if (result->kind == XS_PRIVATE_KIND) {
			if (result->value.private.check == check)
				break;
		}
		result = result->next;
	}
	if (result) {
		result = result->value.private.first;
		while (result) {
			if (result->ID == id)
				break;
			result = result->next;
		}
	}
	if (result) {
		if ((result->kind == XS_ACCESSOR_KIND) && (result->value.accessor.setter == C_NULL))
			result = C_NULL;
	}
	return result;
}








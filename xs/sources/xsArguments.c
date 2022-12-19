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

static txBoolean fxArgumentsSloppyDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* descriptor, txFlag mask);
static txBoolean fxArgumentsSloppyDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static txSlot* fxArgumentsSloppyGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
static txSlot* fxArgumentsSloppySetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);

const txBehavior ICACHE_FLASH_ATTR gxArgumentsSloppyBehavior = {
	fxArgumentsSloppyGetProperty,
	fxArgumentsSloppySetProperty,
	fxOrdinaryCall,
	fxOrdinaryConstruct,
	fxArgumentsSloppyDefineOwnProperty,
	fxArgumentsSloppyDeleteProperty,
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

void fxBuildArguments(txMachine* the)
{
	mxPush(mxObjectPrototype);
	mxArgumentsSloppyPrototype = *the->stack;
	mxPop();
	
	mxPush(mxObjectPrototype);
	mxArgumentsStrictPrototype = *the->stack;
	mxPop();
}

txSlot* fxNewArgumentsSloppyInstance(txMachine* the, txIndex count)
{
	txSlot* instance;
	txSlot* array;
	txSlot* property;
	txIndex index;
	txSlot* address;
	txIndex length = (txIndex)mxArgc;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_EXOTIC_FLAG;
	array = instance->next = fxNewSlot(the);
	array->flag = XS_INTERNAL_FLAG;
	array->ID = XS_ARGUMENTS_SLOPPY_BEHAVIOR;
	array->kind = XS_ARRAY_KIND;
	array->value.array.length = 0;
	array->value.array.address = C_NULL;
	property = fxNextNumberProperty(the, array, length, mxID(_length), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, mxFunction, mxID(_callee), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, &mxArrayIteratorFunction, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	fxSetIndexSize(the, array, length, XS_CHUNK);
	index = 0;
	address = array->value.array.address;
	property = the->scope + count;
	while ((index < length) && (index < count)) {
		*((txIndex*)address) = index;
		address->ID = XS_NO_ID;
		if (property->kind == XS_CLOSURE_KIND) {
			address->kind = property->kind;
			address->value = property->value;
		}
		else {
			txSlot* argument = mxArgv(index);
			address->kind = argument->kind;
			address->value = argument->value;
		}
		index++;
		address++;
		property--;
	}
	while (index < length) {
		property = mxArgv(index);
		*((txIndex*)address) = index;
		address->ID = XS_NO_ID;
		address->kind = property->kind;
		address->value = property->value;
		index++;
		address++;
	}
	return instance;
}

txBoolean fxArgumentsSloppyDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* descriptor, txFlag mask) 
{
	if (!id) {
		txSlot* property = fxGetIndexProperty(the, instance->next, index);
		if (property && (property->kind == XS_CLOSURE_KIND)) {
			txSlot* closure = property->value.closure;
			if (mask & XS_ACCESSOR_FLAG) {
				property->flag = closure->flag;
				property->kind = closure->kind;
				property->value = closure->value;
			}
			else if ((descriptor->flag & XS_DONT_SET_FLAG) && (mask & XS_DONT_SET_FLAG)) {
				property->flag = closure->flag;
				property->kind = closure->kind;
				property->value = closure->value;
				if (descriptor->kind != XS_UNINITIALIZED_KIND) {
					closure->kind = descriptor->kind;
					closure->value = descriptor->value;
				}
			}
		}
	}
	return fxOrdinaryDefineOwnProperty(the, instance, id, index, descriptor, mask);
}

txBoolean fxArgumentsSloppyDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	if (!id) {
		txSlot* property = fxGetIndexProperty(the, instance->next, index);
		if (property && (property->kind == XS_CLOSURE_KIND)) {
			if (property->value.closure->flag & XS_DONT_DELETE_FLAG)
				return 0;
		}
	}
	return fxOrdinaryDeleteProperty(the, instance, id, index);
}

txSlot* fxArgumentsSloppyGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	txSlot* result = fxOrdinaryGetProperty(the, instance, id, index, flag);
	if (!id && result && result->kind == XS_CLOSURE_KIND)
		result = result->value.closure;
	return result;
}

txSlot* fxArgumentsSloppySetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	txSlot* result = fxOrdinarySetProperty(the, instance, id, index, flag);
	if (!id && result && result->kind == XS_CLOSURE_KIND)
		result = result->value.closure;
	return result;
}

txSlot* fxNewArgumentsStrictInstance(txMachine* the, txIndex count)
{
	txSlot* instance;
	txSlot* array;
	txSlot* property;
	txSlot* function;
	txIndex index;
	txSlot* address;
	txIndex length = (txIndex)mxArgc;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_EXOTIC_FLAG;
	array = instance->next = fxNewSlot(the);
	array->flag = XS_INTERNAL_FLAG;
	array->ID = XS_ARGUMENTS_STRICT_BEHAVIOR;
	array->kind = XS_ARRAY_KIND;
	array->value.array.length = 0;
	array->value.array.address = C_NULL;
	property = fxNextNumberProperty(the, array, length, mxID(_length), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, &mxArrayIteratorFunction, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	function = mxThrowTypeErrorFunction.value.reference;
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	property->ID = mxID(_callee);
	property->kind = XS_ACCESSOR_KIND;
	property->value.accessor.getter = function;
	property->value.accessor.setter = function;
	fxSetIndexSize(the, array, length, XS_CHUNK);
	index = 0;
	address = array->value.array.address;
	while (index < length) {
		property = mxArgv(index);
		*((txIndex*)address) = index;
		address->ID = XS_NO_ID;
		address->kind = property->kind;
		address->value = property->value;
		index++;
		address++;
	}
	return instance;
}

void fxThrowTypeError(txMachine* the)
{
	mxTypeError("strict mode");
}

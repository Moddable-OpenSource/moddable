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
#if mxWindows
	#include <Winnls.h>
#elif mxMacOSX
	#include <CoreServices/CoreServices.h>
#elif mxiOS
	#include <CoreFoundation/CoreFoundation.h>
#endif

#define mxStringInstanceLength(INSTANCE) ((txIndex)((INSTANCE)->next->value.key.sum))

static txSlot* fx_String_prototype_split_aux(txMachine* the, txSlot* theString, txSlot* theArray, txSlot* theItem, txInteger theStart, txInteger theStop);

static txSlot* fxCheckString(txMachine* the, txSlot* it);
static txString fxCoerceToString(txMachine* the, txSlot* theSlot);
static txInteger fxArgToPosition(txMachine* the, txInteger i, txInteger index, txInteger length);
static void fx_String_prototype_pad(txMachine* the, txBoolean flag);

static txBoolean fxStringDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static txBoolean fxStringDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask);
static txBoolean fxStringGetOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot);
static txSlot* fxStringGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
static txBoolean fxStringHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static void fxStringOwnKeys(txMachine* the, txSlot* instance, txFlag flag, txSlot* keys);
static txSlot* fxStringSetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);

const txBehavior ICACHE_FLASH_ATTR gxStringBehavior = {
	fxStringGetProperty,
	fxStringSetProperty,
	fxOrdinaryCall,
	fxOrdinaryConstruct,
	fxStringDefineOwnProperty,
	fxStringDeleteProperty,
	fxStringGetOwnProperty,
	fxOrdinaryGetPropertyValue,
	fxOrdinaryGetPrototype,
	fxStringHasProperty,
	fxOrdinaryIsExtensible,
	fxStringOwnKeys,
	fxOrdinaryPreventExtensions,
	fxOrdinarySetPropertyValue,
	fxOrdinarySetPrototype,
};

void fxBuildString(txMachine* the)
{
	txSlot* slot;
	
	fxNewHostFunction(the, mxCallback(fxStringAccessorGetter), 0, XS_NO_ID);
	fxNewHostFunction(the, mxCallback(fxStringAccessorSetter), 1, XS_NO_ID);
	mxPushUndefined();
	the->stack->flag = XS_DONT_DELETE_FLAG;
	the->stack->kind = XS_ACCESSOR_KIND;
	the->stack->value.accessor.getter = (the->stack + 2)->value.reference;
	the->stack->value.accessor.setter = (the->stack + 1)->value.reference;
	mxPull(mxStringAccessor);
	the->stack += 2;
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewStringInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_charAt), 1, mxID(_charAt), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_charCodeAt), 1, mxID(_charCodeAt), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_codePointAt), 1, mxID(_codePointAt), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_compare), 1, mxID(_compare), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_concat), 1, mxID(_concat), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_endsWith), 1, mxID(_endsWith), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_includes), 1, mxID(_includes), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_indexOf), 1, mxID(_indexOf), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_lastIndexOf), 1, mxID(_lastIndexOf), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_compare), 1, mxID(_localeCompare), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_normalize), 0, mxID(_normalize), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_padEnd), 1, mxID(_padEnd), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_padStart), 1, mxID(_padStart), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_repeat), 1, mxID(_repeat), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_replace), 2, mxID(_replace), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_slice), 2, mxID(_slice), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_split), 2, mxID(_split), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_startsWith), 1, mxID(_startsWith), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_substr), 1, mxID(_substr), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_substring), 2, mxID(_substring), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_toLowerCase), 0, mxID(_toLocaleLowerCase), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_valueOf), 0, mxID(_toLocaleString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_toUpperCase), 0, mxID(_toLocaleUpperCase), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_toLowerCase), 0, mxID(_toLowerCase), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_valueOf), 0, mxID(_toString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_toUpperCase), 0, mxID(_toUpperCase), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_trim), 0, mxID(_trim), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_valueOf), 0, mxID(_valueOf), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_iterator), 0, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_match), 1, mxID(_match), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_search), 1, mxID(_search), XS_DONT_ENUM_FLAG);
	mxStringPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, mxCallback(fx_String), 1, mxID(_String), XS_DONT_ENUM_FLAG));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_fromArrayBuffer), 1, mxID(_fromArrayBuffer), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_fromCharCode), 1, mxID(_fromCharCode), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_fromCodePoint), 1, mxID(_fromCodePoint), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_raw), 1, mxID(_raw), XS_DONT_ENUM_FLAG);

	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_String_prototype_iterator_next), 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "String Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxStringIteratorPrototype);
}

txSlot* fxNewStringInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_EXOTIC_FLAG;
	property = fxNextSlotProperty(the, instance, &mxEmptyString, XS_STRING_BEHAVIOR, XS_INTERNAL_FLAG | XS_GET_ONLY);
	property->value.key.sum = 0;	
	return instance;
}

void fxStringAccessorGetter(txMachine* the)
{
	txSlot* instance = fxToInstance(the, mxThis);
	txSlot* string;
	txID id = the->scratch.value.at.id;
	txIndex index = the->scratch.value.at.index;
	while (instance) {
		if (instance->flag & XS_EXOTIC_FLAG) {
			string = instance->next;
			if (string->ID == XS_STRING_BEHAVIOR)
				break;
		}
		instance = instance->value.instance.prototype;
	}
	if (id == mxID(_length)) {
		mxResult->value.integer = string->value.key.sum;
		mxResult->kind = XS_INTEGER_KIND;
	}
	else {
		txInteger from = fxUnicodeToUTF8Offset(string->value.key.string, index);
		if (from >= 0) {
			txInteger to = fxUnicodeToUTF8Offset(string->value.key.string, index + 1);
			if (to >= 0) {
				mxResult->value.string = fxNewChunk(the, to - from + 1);
				c_memcpy(mxResult->value.string, string->value.key.string + from, to - from);
				mxResult->value.string[to - from] = 0;
				mxResult->kind = XS_STRING_KIND;
			}
		}
	}
}

void fxStringAccessorSetter(txMachine* the)
{
}

txBoolean fxStringDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask)
{
	if ((id == mxID(_length)) || (!id && (mxStringInstanceLength(instance) > index)))
		return 0;
	return fxOrdinaryDefineOwnProperty(the, instance, id, index, slot, mask);
}

txBoolean fxStringDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	if ((id == mxID(_length)) || (!id && (mxStringInstanceLength(instance) > index)))
		return 0;
	return fxOrdinaryDeleteProperty(the, instance, id, index);
}

txBoolean fxStringGetOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* descriptor)
{
	if (id == mxID(_length)) {
		txSlot* string = instance->next;
		descriptor->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
		descriptor->ID = id;
		descriptor->kind = XS_INTEGER_KIND;
		descriptor->value.integer = string->value.key.sum;
		return 1;
	}
	if (!id && (mxStringInstanceLength(instance) > index)) {
		txSlot* string = instance->next;
		txInteger from = fxUnicodeToUTF8Offset(string->value.key.string, index);
		txInteger to = fxUnicodeToUTF8Offset(string->value.key.string, index + 1);
		descriptor->value.string = fxNewChunk(the, to - from + 1);
		c_memcpy(descriptor->value.string, string->value.key.string + from, to - from);
		descriptor->value.string[to - from] = 0;
		descriptor->kind = XS_STRING_KIND;
		descriptor->flag = XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
		return 1;
	}
	return fxOrdinaryGetOwnProperty(the, instance, id, index, descriptor);
}

txSlot* fxStringGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	if ((id == mxID(_length)) || (!id && (mxStringInstanceLength(instance) > index))) {
		the->scratch.value.at.id = id;
		the->scratch.value.at.index = index;
		return &mxStringAccessor;
	}
	return fxOrdinaryGetProperty(the, instance, id, index, flag);
}

txBoolean fxStringHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	if ((id == mxID(_length)) || (!id && (mxStringInstanceLength(instance) > index)))
		return 1;
	return fxOrdinaryHasProperty(the, instance, id, index);
}

void fxStringOwnKeys(txMachine* the, txSlot* instance, txFlag flag, txSlot* keys)
{
	txSlot* property = instance->next;
	if (flag & XS_EACH_NAME_FLAG) {
		txIndex length = mxStringInstanceLength(instance), index;
		for (index = 0; index < length; index++)
			keys = fxQueueKey(the, 0, index, keys);
	}
	property = property->next;
	if (property && (property->kind == XS_ARRAY_KIND)) {
		keys = fxQueueIndexKeys(the, property, flag, keys);
		property = property->next;
	}
	if (flag & XS_EACH_NAME_FLAG)
		keys = fxQueueKey(the, mxID(_length), XS_NO_ID, keys);
	fxQueueIDKeys(the, property, flag, keys);
}

txSlot* fxStringSetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	if ((id == mxID(_length)) || (!id && (mxStringInstanceLength(instance) > index))) {
		the->scratch.value.at.id = id;
		the->scratch.value.at.index = index;
		return &mxStringAccessor;
	}
	return fxOrdinarySetProperty(the, instance, id, index, flag);
}

void fx_String(txMachine* the)
{
	txSlot* slot;
	txSlot* instance;
	if (mxArgc > 0) {
		slot = mxArgv(0);
		if ((mxTarget->kind == XS_UNDEFINED_KIND) && (slot->kind == XS_SYMBOL_KIND)) {
			fxSymbolToString(the, slot);
			*mxResult = *slot;
			return;
		}
		fxToString(the, slot);
	}
	else {
		slot = &mxEmptyString;
	}
	if (mxTarget->kind == XS_UNDEFINED_KIND) {
		*mxResult = *slot;
		return;
	}
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxStringPrototype);
	instance = fxNewStringInstance(the);
	instance->next->kind = slot->kind; // @@
	instance->next->value.key.string = slot->value.string;
	instance->next->value.key.sum = fxUnicodeLength(slot->value.string);	
	mxPullSlot(mxResult);
}

void fx_String_fromArrayBuffer(txMachine* the)
{
	txSlot* slot;
	txSlot* arrayBuffer = C_NULL;
	txInteger length;
	txString string;
	if (mxArgc < 1)
		mxTypeError("no argument");
	slot = mxArgv(0);
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference->next;
		if (slot && (slot->kind == XS_ARRAY_BUFFER_KIND))
			arrayBuffer = slot;
	}
	if (!arrayBuffer)
		mxTypeError("argument is no ArrayBuffer instance");
	length = arrayBuffer->value.arrayBuffer.length;
	string = fxNewChunk(the, length + 1);
	c_memcpy(string, arrayBuffer->value.arrayBuffer.address, length);
	string[length] = 0;
	mxResult->value.string = string;
	mxResult->kind = XS_STRING_KIND;
}

void fx_String_fromCharCode(txMachine* the)
{
	txInteger aLength;
	txInteger aCount;
	txInteger anIndex;
	txU4 c; 
	txU1* p;
	
	aLength = 0;
	aCount = mxArgc;
	for (anIndex = 0; anIndex < aCount; anIndex++) {
		c = fxToUnsigned(the, mxArgv(anIndex));
		if ((0x0000D800 <= c) && (c <= 0x0000DBFF) && ((anIndex + 1) < aCount)) {
			txU4 d = fxToUnsigned(the, mxArgv(anIndex + 1));
			if ((0x0000DC00 <= d) && (d <= 0x0000DFFF)) {
				c = 0x00010000 + ((c & 0x000003FF) << 10) + (d & 0x000003FF);
				anIndex++;
			}
		}
		if (c < 0x80)
			aLength++;
		else if (c < 0x800)
			aLength += 2;
		else if (c < 0x10000)
			aLength += 3;
		else
			aLength += 4;
	}
	mxResult->value.string = (txString)fxNewChunk(the, aLength + 1);
	mxResult->kind = XS_STRING_KIND;
	p = (txU1*)mxResult->value.string;
	for (anIndex = 0; anIndex < aCount; anIndex++) {
		c = fxToUnsigned(the, mxArgv(anIndex));
		if ((0x0000D800 <= c) && (c <= 0x0000DBFF) && ((anIndex + 1) < aCount)) {
			txU4 d = fxToUnsigned(the, mxArgv(anIndex + 1));
			if ((0x0000DC00 <= d) && (d <= 0x0000DFFF)) {
				c = 0x00010000 + ((c & 0x000003FF) << 10) + (d & 0x000003FF);
				anIndex++;
			}
		}
		if (c < 0x80) {
			*p++ = (txU1)c;
		}
		else if (c < 0x800) {
			*p++ = (txU1)(0xC0 | (c >> 6));
			*p++ = (txU1)(0x80 | (c & 0x3F));
		}
		else if (c < 0x10000) {
			*p++ = (txU1)(0xE0 | (c >> 12));
			*p++ = (txU1)(0x80 | ((c >> 6) & 0x3F));
			*p++ = (txU1)(0x80 | (c & 0x3F));
		}
		else if (c < 0x200000) {
			*p++ = (txU1)(0xF0 | (c >> 18));
			*p++ = (txU1)(0x80 | ((c >> 12) & 0x3F));
			*p++ = (txU1)(0x80 | ((c >> 6) & 0x3F));
			*p++ = (txU1)(0x80 | (c  & 0x3F));
		}
	}	
	*p = 0;
}

void fx_String_fromCodePoint(txMachine* the)
{
	txInteger length = 0;
	txInteger count = mxArgc;
	txInteger index = 0;
	txU4 c; 
	txU1* p;
	while (index < count) {
		txNumber number = fxToNumber(the, mxArgv(index));
		txNumber check = c_trunc(number);
		if (number != check)
			mxRangeError("invalid code point %lf", number);
		if ((number < 0) || (0x10FFFF < number))
			mxRangeError("invalid code point %lf", number);
		c = (txU4)number;
		if (c < 0x80)
			length++;
		else if (c < 0x800)
			length += 2;
		else if (c < 0x10000)
			length += 3;
		else
			length += 4;
		index++;
	}
	mxResult->value.string = (txString)fxNewChunk(the, length + 1);
	mxResult->kind = XS_STRING_KIND;
	p = (txU1*)mxResult->value.string;
	index = 0;
	while (index < count) {
		c = fxToUnsigned(the, mxArgv(index));
		if (c < 0x80) {
			*p++ = (txU1)c;
		}
		else if (c < 0x800) {
			*p++ = (txU1)(0xC0 | (c >> 6));
			*p++ = (txU1)(0x80 | (c & 0x3F));
		}
		else if (c < 0x10000) {
			*p++ = (txU1)(0xE0 | (c >> 12));
			*p++ = (txU1)(0x80 | ((c >> 6) & 0x3F));
			*p++ = (txU1)(0x80 | (c & 0x3F));
		}
		else if (c < 0x200000) {
			*p++ = (txU1)(0xF0 | (c >> 18));
			*p++ = (txU1)(0x80 | ((c >> 12) & 0x3F));
			*p++ = (txU1)(0x80 | ((c >> 6) & 0x3F));
			*p++ = (txU1)(0x80 | (c  & 0x3F));
		}
		index++;
	}	
	*p = 0;
}

void fx_String_raw(txMachine* the)
{
	txInteger argCount = mxArgc;
	txSlot* raw;
	txInteger rawCount;
	if (argCount > 0)
		fxToInstance(the, mxArgv(0));
	else
		mxTypeError("cannot coerce undefined to object");
	mxPushSlot(mxArgv(0));
	fxGetID(the, mxID(_raw));
	raw = the->stack;
	mxPushSlot(raw);
	fxGetID(the, mxID(_length));
	rawCount = fxToInteger(the, the->stack);
	mxPop();
	if (rawCount <= 0) {
		mxResult->value = mxEmptyString.value;
	}
	else {
		txSlot* list;
		txInteger index = 0;
		txSlot* item;
		txInteger size;
		list = item = fxNewInstance(the);
		mxPushSlot(list);
		for (;;) {
			mxPushSlot(raw);
			fxGetID(the, index);
			fxToString(the, the->stack);
			item = item->next = fxNewSlot(the);
			mxPullSlot(item);
			index++;
			if (index == rawCount)
				break;
			if (index < argCount) {
				mxPushSlot(mxArgv(index));
				fxToString(the, the->stack);
			}
			else
				mxPush(mxEmptyString);
			item = item->next = fxNewSlot(the);
			mxPullSlot(item);
		}
		size = 0;
		item = list->next;
		while (item) {
			item->value.key.sum = c_strlen(item->value.string);
			size += item->value.key.sum;
			item = item->next;
		}
		size++;
		mxResult->value.string = (txString)fxNewChunk(the, size);
		size = 0;
		item = list->next;
		while (item) {
			c_memcpy(mxResult->value.string + size, item->value.string, item->value.key.sum);
			size += item->value.key.sum;
			item = item->next;
		}
		mxResult->value.string[size] = 0;
		mxPop();
	}
	mxResult->kind = XS_STRING_KIND;
	mxPop();
}

void fx_String_prototype_charAt(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txInteger anOffset;

	aString = fxCoerceToString(the, mxThis);
	aLength = fxUnicodeLength(aString);
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND))
		anOffset = fxToInteger(the, mxArgv(0));
	else
		anOffset = 0;
	if ((0 <= anOffset) && (anOffset < aLength)) {
		anOffset = fxUnicodeToUTF8Offset(aString, anOffset);
		aLength = fxUnicodeToUTF8Offset(aString + anOffset, 1);
		if ((anOffset >= 0) && (aLength > 0)) {
			mxResult->value.string = (txString)fxNewChunk(the, aLength + 1);
			c_memcpy(mxResult->value.string, mxThis->value.string + anOffset, aLength);
			mxResult->value.string[aLength] = 0;
			mxResult->kind = XS_STRING_KIND;
		}
		else {
			mxResult->value.string = mxEmptyString.value.string;
			mxResult->kind = mxEmptyString.kind;
		}
	}
	else {
		mxResult->value.string = mxEmptyString.value.string;
		mxResult->kind = mxEmptyString.kind;
	}
}

void fx_String_prototype_charCodeAt(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txInteger anOffset;

	aString = fxCoerceToString(the, mxThis);
	aLength = fxUnicodeLength(aString);
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND))
		anOffset = fxToInteger(the, mxArgv(0));
	else
		anOffset = 0;
	if ((0 <= anOffset) && (anOffset < aLength)) {
		anOffset = fxUnicodeToUTF8Offset(aString, anOffset);
		aLength = fxUnicodeToUTF8Offset(aString + anOffset, 1);
		if ((anOffset >= 0) && (aLength > 0)) {
			mxResult->value.integer = fxUnicodeCharacter(aString + anOffset);
			mxResult->kind = XS_INTEGER_KIND;
		}
		else {
			mxResult->value.number = C_NAN;
			mxResult->kind = XS_NUMBER_KIND;
		}
	}
	else {
		mxResult->value.number = C_NAN;
		mxResult->kind = XS_NUMBER_KIND;
	}
}

void fx_String_prototype_codePointAt(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis);
	txInteger length = fxUnicodeLength(string);
	txNumber at = (mxArgc > 0) ? fxToNumber(the, mxArgv(0)) : 0;
	if (c_isnan(at))
		at = 0;
	if ((0 <= at) && (at < (txNumber)length)) {
		txInteger offset = fxUnicodeToUTF8Offset(string, (txInteger)at);
		length = fxUnicodeToUTF8Offset(string + offset, 1);
		if ((offset >= 0) && (length > 0)) {
			mxResult->value.integer = fxUnicodeCharacter(string + offset);
			mxResult->kind = XS_INTEGER_KIND;
		}
	}
}


void fx_String_prototype_compare(txMachine* the)
{
	txString aString;

	aString = fxCoerceToString(the, mxThis);
	if (mxArgc < 1)
		mxResult->value.integer = c_strcmp(aString, "undefined");
	else {
		fxToString(the, mxArgv(0));
		mxResult->value.integer = c_strcmp(aString, mxArgv(0)->value.string);
	}
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_String_prototype_concat(txMachine* the)
{
	txInteger aCount;
	txInteger aLength;
	txInteger anIndex;
	
	fxCoerceToString(the, mxThis);
	aCount = mxArgc;
	aLength = c_strlen(mxThis->value.string);
	for (anIndex = 0; anIndex < aCount; anIndex++)
		aLength += c_strlen(fxToString(the, mxArgv(anIndex)));
	mxResult->value.string = (txString)fxNewChunk(the, aLength + 1);
	mxResult->kind = XS_STRING_KIND;
	c_strcpy(mxResult->value.string, mxThis->value.string);
	for (anIndex = 0; anIndex < aCount; anIndex++)
		c_strcat(mxResult->value.string, mxArgv(anIndex)->value.string);
}

void fx_String_prototype_endsWith(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis);
	txInteger length = fxUnicodeLength(string);
	txString searchString;
	txInteger searchLength;
	txInteger offset;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc < 1)
		return;
	if (fxIsRegExp(the, mxArgv(0)))
		mxTypeError("future editions");
	searchString = fxToString(the, mxArgv(0));
	searchLength = c_strlen(searchString);
	offset = fxUnicodeToUTF8Offset(string, fxArgToPosition(the, 1, length, length));
	if (offset < searchLength)
		return;
	if (!c_strncmp(string + offset - searchLength, searchString, searchLength))
		mxResult->value.boolean = 1;
}

void fx_String_prototype_includes(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis);
	txInteger length = c_strlen(string);
	txString searchString;
	txInteger searchLength;
	txInteger offset;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc < 1)
		return;
	if (fxIsRegExp(the, mxArgv(0)))
		mxTypeError("future editions");
	searchString = fxToString(the, mxArgv(0));
	searchLength = c_strlen(searchString);
	offset = fxUnicodeToUTF8Offset(string, fxArgToPosition(the, 1, 0, fxUnicodeLength(string)));
	if ((length - offset) < searchLength)
		return;
	if (c_strstr(string + offset, searchString))
		mxResult->value.boolean = 1;
}

void fx_String_prototype_indexOf(txMachine* the)
{
	txString aString;
	txString aSubString;
	txInteger aLength;
	txInteger aSubLength;
	txInteger anOffset;
	txNumber aNumber;
	txInteger aLimit;
	txString p;
	txString q;
	
	aString = fxCoerceToString(the, mxThis);
	if (mxArgc < 1) {
		mxResult->value.integer = -1;
		mxResult->kind = XS_INTEGER_KIND;
		return;
	}
	aSubString = fxToString(the, mxArgv(0));
	aLength = fxUnicodeLength(aString);
	aSubLength = fxUnicodeLength(aSubString);
	anOffset = 0;
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		aNumber = fxToNumber(the, mxArgv(1));
		anOffset = (c_isnan(aNumber)) ? 0 : (aNumber < 0) ? 0 : (aNumber > aLength) ? aLength : (txInteger)c_floor(aNumber);
	}
	if (anOffset + aSubLength <= aLength) {
		anOffset = fxUnicodeToUTF8Offset(aString, anOffset);
		aLimit = c_strlen(aString) - c_strlen(aSubString);
		while (anOffset <= aLimit) {
			p = aString + anOffset;
			q = aSubString;
			while (c_read8(q) && (c_read8(p) == c_read8(q))) {
				p++;
				q++;
			}
			if (c_read8(q))
				anOffset++;
			else
				break;
		}
		if (anOffset <= aLimit)
			anOffset = fxUTF8ToUnicodeOffset(aString, anOffset);
		else
			anOffset = -1;
	}
	else
		anOffset = -1;
	mxResult->value.integer = anOffset;
	mxResult->kind = XS_INTEGER_KIND;
}

static txInteger fx_String_prototype_indexOf_aux(txMachine* the, txString theString, 
		txInteger theLength, txInteger theOffset,
		txString theSubString, txInteger theSubLength, txInteger* theOffsets)
{
	txString p;
	txString q;
	
	theOffsets[0] = theOffset;
	theOffsets[1] = theOffset + theSubLength;
	while (theOffsets[1] <= theLength) {
		p = theString + theOffsets[0];
		q = theSubString;
		while (c_read8(q) && (c_read8(p) == c_read8(q))) {
			p++;
			q++;
		}
		if (c_read8(q)) {
			theOffsets[0]++;
			theOffsets[1]++;
		}
		else
			return 1;
	}
	return 0;
}

void fx_String_prototype_lastIndexOf(txMachine* the)
{
	txString aString;
	txString aSubString;
	txInteger aLength;
	txInteger aSubLength;
	txInteger anOffset;
	txNumber aNumber;
	txString p;
	txString q;

	aString = fxCoerceToString(the, mxThis);
	if (mxArgc < 1) {
		mxResult->value.integer = -1;
		mxResult->kind = XS_INTEGER_KIND;
		return;
	}
	aSubString = fxToString(the, mxArgv(0));
	aLength = fxUnicodeLength(aString);
	aSubLength = fxUnicodeLength(aSubString);
	anOffset = aLength;
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		aNumber = fxToNumber(the, mxArgv(1));
		anOffset = (c_isnan(aNumber)) ? aLength : (aNumber < 0) ? 0 : (aNumber > aLength) ? aLength : (txInteger)c_floor(aNumber);
		anOffset += aSubLength;
		if (anOffset > aLength)
			anOffset = aLength;
	}
	if (anOffset - aSubLength >= 0) {
		anOffset = fxUnicodeToUTF8Offset(aString, anOffset - aSubLength);
		while (anOffset >= 0) {
			p = aString + anOffset;
			q = aSubString;
			while (c_read8(q) && (c_read8(p) == c_read8(q))) {
				p++;
				q++;
			}
			if (c_read8(q))
				anOffset--;
			else
				break;
		}		
		anOffset = fxUTF8ToUnicodeOffset(aString, anOffset);
	}
	else
		anOffset = -1;
	mxResult->value.integer = anOffset;
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_String_prototype_match(txMachine* the)
{	
	txSlot* regexp = C_NULL;

	fxCoerceToString(the, mxThis);
	mxPushSlot(mxThis);
	mxPushInteger(1);
	if (mxArgc > 0) {
		regexp = mxArgv(0);
		if (mxIsReference(regexp)) {
			mxPushSlot(regexp);
			mxPushSlot(regexp);
			fxGetID(the, mxID(_Symbol_match));
			if (the->stack->kind != XS_UNDEFINED_KIND) {
				fxCall(the);
				mxPullSlot(mxResult);
				return;
			}
			mxPop();
			mxPop();
		}
	}	
	if (regexp)
		mxPushSlot(regexp);
	else
		mxPushUndefined();
	mxPushUndefined();
	mxPushInteger(2);
	mxPush(mxRegExpPrototype);
	fxNewRegExpInstance(the);
	mxPush(mxInitializeRegExpFunction);
	fxCall(the);
	fxCallID(the, mxID(_Symbol_match));
	mxPullSlot(mxResult);
}

void fx_String_prototype_normalize(txMachine* the)
{
	txString result = mxEmptyString.value.string;
	#if (mxWindows && (WINVER >= 0x0600))
	txString string = fxCoerceToString(the, mxThis);
	txInteger stringLength = c_strlen(string);
	{
		NORM_FORM form;
		txInteger unicodeLength;
		txU2* unicodeBuffer = NULL;
		txInteger normLength;
		txU2* normBuffer = NULL;
		txInteger resultLength;
		mxTry(the) {
			if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND))
				form = NormalizationC;
			else {
				result = fxToString(the, mxArgv(0));
				if (!c_strcmp(result, "NFC"))
					form = NormalizationC;
				else if (!c_strcmp(result, "NFD"))
					form = NormalizationD;
				else if (!c_strcmp(result, "NFKC"))
					form = NormalizationKC;
				else if (!c_strcmp(result, "NFKD"))
					form = NormalizationKD;
				else
					mxRangeError("invalid form");
			}
			unicodeLength = MultiByteToWideChar(CP_UTF8, 0, string, stringLength, NULL, 0);
			unicodeBuffer = c_malloc(unicodeLength * 2);
			if (!unicodeBuffer) fxJump(the);
			MultiByteToWideChar(CP_UTF8, 0, string, stringLength, unicodeBuffer, unicodeLength);
			normLength = NormalizeString(form, unicodeBuffer, unicodeLength, NULL, 0);
			normBuffer = c_malloc(normLength * 2);
			if (!normBuffer) fxJump(the);
			NormalizeString(form, unicodeBuffer, unicodeLength, normBuffer, normLength);
			resultLength = WideCharToMultiByte(CP_UTF8, 0, normBuffer, normLength, NULL, 0, NULL, NULL);
			result = fxNewChunk(the, resultLength + 1);
			WideCharToMultiByte(CP_UTF8, 0, normBuffer, normLength, result, resultLength, NULL, NULL);
			result[resultLength] = 0;
			c_free(unicodeBuffer);
		}
		mxCatch(the) {
			if (unicodeBuffer)
				c_free(unicodeBuffer);
			fxJump(the);
		}
	}
	#elif (mxMacOSX || mxiOS)
	txString string = fxCoerceToString(the, mxThis);
	txInteger stringLength = c_strlen(string);
	{
		CFStringNormalizationForm form;
		CFStringRef cfString = NULL;
		CFMutableStringRef mutableCFString = NULL;
		CFIndex resultLength;
		CFRange range;
		mxTry(the) {
			if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND))
				form = kCFStringNormalizationFormC;
			else {
				result = fxToString(the, mxArgv(0));
				if (!c_strcmp(result, "NFC"))
					form = kCFStringNormalizationFormC;
				else if (!c_strcmp(result, "NFD"))
					form = kCFStringNormalizationFormD;
				else if (!c_strcmp(result, "NFKC"))
					form = kCFStringNormalizationFormKC;
				else if (!c_strcmp(result, "NFKD"))
					form = kCFStringNormalizationFormKD;
				else
					mxRangeError("invalid form");
			}
			cfString = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)string, stringLength, kCFStringEncodingUTF8, false);
			if (cfString == NULL) fxJump(the);
			mutableCFString = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, cfString);
			if (mutableCFString == NULL) fxJump(the);
			CFStringNormalize(mutableCFString, form);
			range = CFRangeMake(0, CFStringGetLength(mutableCFString));
			CFStringGetBytes(mutableCFString, range, kCFStringEncodingUTF8, 0, false, NULL, 0, &resultLength);
			result = fxNewChunk(the, resultLength + 1);
			CFStringGetBytes(mutableCFString, range, kCFStringEncodingUTF8, 0, false, (UInt8 *)result, resultLength, NULL);
			result[resultLength] = 0;
			CFRelease(mutableCFString);
			CFRelease(cfString);
		}
		mxCatch(the) {
			if (mutableCFString)
				CFRelease(mutableCFString);
			if (cfString)
				CFRelease(cfString);
			fxJump(the);
		}
	}
	#else
	{
		mxRangeError("invalid form");
	}
	#endif
	mxResult->value.string = result;
	mxResult->kind = XS_STRING_KIND;
}

void fx_String_prototype_pad(txMachine* the, txBoolean flag)
{
	txString string = fxCoerceToString(the, mxThis), filler;
	txInteger stringLength = c_strlen(string), fillerLength;
	txInteger stringSize = fxUnicodeLength(string), fillerSize;
	txInteger resultSize = (txInteger)fxArgToRange(the, 0, 0, 0, 0x7FFFFFFF);
	*mxResult = *mxThis;
	if (resultSize > stringSize) {
		if ((mxArgc <= 1) || (mxIsUndefined(mxArgv(1))))
			mxPushStringX(" ");
		else
			mxPushSlot(mxArgv(1));
		filler = fxToString(the, the->stack);
		fillerLength = c_strlen(filler);
		fillerSize = fxUnicodeLength(filler);
		if (fillerSize > 0) {
			txInteger delta = resultSize - stringSize;
			txInteger count = delta / fillerSize;
			txInteger rest = fxUnicodeToUTF8Offset(filler, delta % fillerSize);
			txString result = mxResult->value.string = (txString)fxNewChunk(the, stringLength + (fillerLength * count) + rest + 1);
			mxResult->kind = XS_STRING_KIND;
			string = fxToString(the, mxThis);
			filler = fxToString(the, the->stack);
			if (flag) {
				c_memcpy(result, string, stringLength);
				result += stringLength;
			}
			while (count) {
				c_memcpy(result, filler, fillerLength);
				count--;
				result += fillerLength;
			}
			if (rest) {
				c_memcpy(result, filler, rest);
				result += rest;
			}
			if (!flag) {
				c_memcpy(result, string, stringLength);
				result += stringLength;
			}
			*result = 0;
		}
		mxPop();
	}
}

void fx_String_prototype_padEnd(txMachine* the)
{
	fx_String_prototype_pad(the, 1);
}

void fx_String_prototype_padStart(txMachine* the)
{
	fx_String_prototype_pad(the, 0);
}

void fx_String_prototype_repeat(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis), result;
	txInteger length = c_strlen(string), count;
	txNumber COUNT = (mxArgc > 0) ? c_trunc(fxToNumber(the, mxArgv(0))) : 0;
    if (COUNT < 0)
		mxRangeError("count < 0");
	else if (COUNT == C_INFINITY)
		mxRangeError("count == Infinity");
    else if (c_isnan(COUNT))
        count = 0;
    else
        count = (txInteger)COUNT;
	result = mxResult->value.string = (txString)fxNewChunk(the, (length * count) + 1);
	mxResult->kind = XS_STRING_KIND;
	string = fxToString(the, mxThis);
	if (length) {
		while (count) {
			c_memcpy(result, string, length);
			count--;
			result += length;
		}
	}
	*result = 0;
	string = mxThis->value.string;
}

void fx_String_prototype_replace(txMachine* the)
{
	txString string;
	txSlot* match;
	txSlot* function = C_NULL;
	txSlot* replace;

	if (mxArgc > 0) {
		txSlot* regexp = mxArgv(0);
		if (mxIsReference(regexp)) {
			mxPushSlot(regexp);
			fxGetID(the, mxID(_Symbol_replace));
			if (the->stack->kind != XS_UNDEFINED_KIND) {
				txSlot* function = the->stack;
				mxPushSlot(mxThis);
				if (mxArgc > 1)
					mxPushSlot(mxArgv(1));
				else
					mxPushUndefined();
				mxPushInteger(2);
				mxPushSlot(regexp);
				mxPushSlot(function);
				fxCall(the);
				mxPullSlot(mxResult);
				mxPop();
				return;
			}
			mxPop();
		}
	}
	string = fxCoerceToString(the, mxThis);
	if (mxArgc <= 0)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	match = the->stack;
	fxToString(the, match);
	if (mxArgc <= 1)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(1));
	if (mxIsReference(the->stack) && mxIsFunction(the->stack->value.reference))
		function = the->stack;
	else {		
		replace = the->stack;
		fxToString(the, replace);
	}
	string = c_strstr(mxThis->value.string, match->value.string);
	if (string) {
		txInteger offset = string - mxThis->value.string;
		txInteger size = c_strlen(mxThis->value.string);
		txInteger matchLength = c_strlen(match->value.string);
		txInteger replaceLength;
		if (function) {
			mxPushSlot(match);
			mxPushInteger(fxUTF8ToUnicodeOffset(mxThis->value.string, offset));
			mxPushSlot(mxThis);
			mxPushInteger(3);
			mxPushUndefined();
			mxPushSlot(function);
			fxCall(the);
			fxToString(the, the->stack);
		}
		else
			fxPushSubstitutionString(the, mxThis, size, offset, match, matchLength, 0, C_NULL, replace);
		replaceLength = c_strlen(the->stack->value.string);
		mxResult->value.string = (txString)fxNewChunk(the, size - matchLength + replaceLength + 1);
		c_memcpy(mxResult->value.string, mxThis->value.string, offset);
		c_memcpy(mxResult->value.string + offset, the->stack->value.string, replaceLength);
		c_memcpy(mxResult->value.string + offset + replaceLength, mxThis->value.string + offset + matchLength, size - (offset + matchLength));
		mxResult->value.string[size - matchLength + replaceLength] = 0;
		mxResult->kind = XS_STRING_KIND;
		mxPop();
	}
	else
		*mxResult = *mxThis;
	mxPop();
	mxPop();
}

void fx_String_prototype_search(txMachine* the)
{
	txString string;
	txSlot* regexp = C_NULL;

	string = fxCoerceToString(the, mxThis);
	mxPushString(string);
	mxPushInteger(1);
	if (mxArgc > 0) {
		regexp = mxArgv(0);
		if (mxIsReference(regexp)) {
			mxPushSlot(regexp);
			mxPushSlot(regexp);
			fxGetID(the, mxID(_Symbol_search));
			if (the->stack->kind != XS_UNDEFINED_KIND) {
				fxCall(the);
				mxPullSlot(mxResult);
				return;
			}
			mxPop();
			mxPop();
		}
	}	
	if (regexp)
		mxPushSlot(regexp);
	else
		mxPushUndefined();
    mxPushUndefined();
	mxPushInteger(2);
	mxPush(mxRegExpPrototype);
	fxNewRegExpInstance(the);
	mxPush(mxInitializeRegExpFunction);
	fxCall(the);
	fxCallID(the, mxID(_Symbol_search));
	mxPullSlot(mxResult);
}

void fx_String_prototype_slice(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis);
	txInteger length = fxUnicodeLength(string);
	txNumber start = fxArgToIndex(the, 0, 0, length);
	txNumber end = fxArgToIndex(the, 1, length, length);
	if (start < end) {
		txInteger offset = fxUnicodeToUTF8Offset(string, (txInteger)start);
		length = fxUnicodeToUTF8Offset(string + offset, (txInteger)(end - start));
		if ((offset >= 0) && (length > 0)) {
			mxResult->value.string = (txString)fxNewChunk(the, length + 1);
			c_memcpy(mxResult->value.string, mxThis->value.string + offset, length);
			mxResult->value.string[length] = 0;
			mxResult->kind = XS_STRING_KIND;
			return;
		}
	}
	mxResult->value.string = mxEmptyString.value.string;
	mxResult->kind = mxEmptyString.kind;
}

void fx_String_prototype_split(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txIndex aLimit;
	txSlot* anArray;
	txSlot* anItem;
	txInteger anOffset;
	txInteger aCount;
	txString aSubString;
	txInteger aSubLength;
	txInteger aSubOffset;
	txInteger subOffsets[2];

	aString = fxCoerceToString(the, mxThis);
	aLength = c_strlen(aString);

	if (mxArgc > 0) {
		txSlot* regexp = mxArgv(0);
		if (mxIsReference(regexp)) {
			mxPushSlot(regexp);
			fxGetID(the, mxID(_Symbol_split));
			if (the->stack->kind != XS_UNDEFINED_KIND) {
				txSlot* function = the->stack;
				mxPushSlot(mxThis);
				if (mxArgc > 1) {
					mxPushSlot(mxArgv(1));
					mxPushInteger(2);
				}
				else
					mxPushInteger(1);
				mxPushSlot(regexp);
				mxPushSlot(function);
				fxCall(the);
				mxPullSlot(mxResult);
				mxPop();
				return;
			}
			the->stack++;
		}
	}

	aLimit = ((mxArgc > 1) && (!mxIsUndefined(mxArgv(1)))) ? (txIndex)fxToUnsigned(the, mxArgv(1)) : 0xFFFFFFFF;
	mxPush(mxArrayPrototype);
	anArray = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	fxGetInstance(the, mxResult);
	if (!aLimit)
		goto bail;
	anItem = fxLastProperty(the, anArray);
	if ((mxArgc < 1) || (mxArgv(0)->kind == XS_UNDEFINED_KIND)) {
		fx_String_prototype_split_aux(the, mxThis, anArray, anItem, 0, aLength);
		goto bail;
	}
	aSubString = fxToString(the, mxArgv(0));
	aSubLength = c_strlen(aSubString);
	if (aSubLength == 0) {
		anOffset = 0;
		while (anOffset < aLength) {
			aSubOffset = anOffset + fxUnicodeToUTF8Offset(mxThis->value.string + anOffset, 1);
			anItem = fx_String_prototype_split_aux(the, mxThis, anArray, anItem, anOffset, aSubOffset);
			if (anArray->next->value.array.length >= aLimit)
				goto bail;
			anOffset = aSubOffset;
		}
	}
	else if (aLength == 0) {
		fx_String_prototype_split_aux(the, mxThis, anArray, anItem, 0, 0);
	}
	else {
		anOffset = 0;
		for (;;) {
			aCount = fx_String_prototype_indexOf_aux(the, mxThis->value.string, aLength, anOffset, mxArgv(0)->value.string, aSubLength, subOffsets);
			if (aCount <= 0)
				break;
			if (anOffset <= subOffsets[0]) {
				anItem = fx_String_prototype_split_aux(the, mxThis, anArray, anItem, anOffset, subOffsets[0]);
				if (anArray->next->value.array.length >= aLimit)
					goto bail;
			}
			anOffset = subOffsets[1];
		}
		if (anOffset <= aLength)
			fx_String_prototype_split_aux(the, mxThis, anArray, anItem, anOffset, aLength);
	}
bail:
	fxCacheArray(the, anArray);
}

txSlot* fx_String_prototype_split_aux(txMachine* the, txSlot* theString, txSlot* theArray, txSlot* theItem, txInteger theStart, txInteger theStop)
{
	theStop -= theStart;
	theItem->next = fxNewSlot(the);
	theItem = theItem->next;
	theItem->next = C_NULL;
	theItem->ID = XS_NO_ID;
	theItem->flag = XS_NO_FLAG;
	if (theStart >= 0) {
		theItem->value.string = (txString)fxNewChunk(the, theStop + 1);
		c_memcpy(theItem->value.string, theString->value.string + theStart, theStop);
		theItem->value.string[theStop] = 0;
		theItem->kind = XS_STRING_KIND;
	}
	theArray->next->value.array.length++;
	return theItem;
}

void fx_String_prototype_startsWith(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis);
	txInteger length = c_strlen(string);
	txString searchString;
	txInteger searchLength;
	txInteger offset;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc < 1)
		return;
	if (fxIsRegExp(the, mxArgv(0)))
		mxTypeError("future editions");
	searchString = fxToString(the, mxArgv(0));
	searchLength = c_strlen(searchString);
	offset = fxUnicodeToUTF8Offset(string, fxArgToPosition(the, 1, 0, fxUnicodeLength(string)));
	if (length - offset < searchLength)
		return;
	if (!c_strncmp(string + offset, searchString, searchLength))
		mxResult->value.boolean = 1;
}

void fx_String_prototype_substr(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis);
	txInteger length = c_strlen(string);
	txInteger size = fxUnicodeLength(string);
	txInteger start = (txInteger)fxArgToIndex(the, 0, 0, size);
	txInteger stop = (mxArgc > 1) ? start + fxToInteger(the, mxArgv(1)) : size;
	if (start < stop) {
		start = fxUnicodeToUTF8Offset(string, start);
		stop = fxUnicodeToUTF8Offset(string, stop);
		length = stop - start;
		mxResult->value.string = (txString)fxNewChunk(the, length + 1);
		c_memcpy(mxResult->value.string, string + start, length);
		mxResult->value.string[length] = 0;
		mxResult->kind = XS_STRING_KIND;
	}
	else {
		mxResult->value.string = mxEmptyString.value.string;
		mxResult->kind = mxEmptyString.kind;
	}
}

void fx_String_prototype_substring(txMachine* the)
{
	txString aString;
	txInteger aLength;
	txNumber aNumber;
	txInteger aStart;
	txInteger aStop;
	txInteger anOffset;

	aString = fxCoerceToString(the, mxThis);
	aLength = fxUnicodeLength(aString);
	aStart = 0;
	aStop = aLength;
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
		aNumber = fxToNumber(the, mxArgv(0));
		aStart = (c_isnan(aNumber)) ? 0 : (aNumber < 0) ? 0 : (aNumber > aLength) ? aLength : (txInteger)c_floor(aNumber);
	}
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		aNumber = fxToNumber(the, mxArgv(1));
		aStop = (c_isnan(aNumber)) ? 0 : (aNumber < 0) ? 0 : (aNumber > aLength) ? aLength : (txInteger)c_floor(aNumber);
	}
	if (aStart > aStop) {
		aLength = aStart;
		aStart = aStop;
		aStop = aLength;
	}
	if (aStart < aStop) {
		anOffset = fxUnicodeToUTF8Offset(aString, aStart);
		aLength = fxUnicodeToUTF8Offset(aString + anOffset, aStop - aStart);
		if ((anOffset >= 0) && (aLength > 0)) {
			mxResult->value.string = (txString)fxNewChunk(the, aLength + 1);
			c_memcpy(mxResult->value.string, mxThis->value.string + anOffset, aLength);
			mxResult->value.string[aLength] = 0;
			mxResult->kind = XS_STRING_KIND;
		}
		else {
			mxResult->value.string = mxEmptyString.value.string;
			mxResult->kind = mxEmptyString.kind;
		}
	}
	else {
		mxResult->value.string = mxEmptyString.value.string;
		mxResult->kind = mxEmptyString.kind;
	}
}

void fx_String_prototype_toCase(txMachine* the, txBoolean flag)
{
	txString string;
	txInteger stringLength;
	txString result;
	string = fxCoerceToString(the, mxThis);
	stringLength = c_strlen(string);
	if (stringLength) {
	#if mxWindows
		txInteger unicodeLength;
		txU2* unicodeBuffer = NULL;
		txInteger resultLength;
		mxTry(the) {
			unicodeLength = MultiByteToWideChar(CP_UTF8, 0, string, stringLength, NULL, 0);
			if (unicodeLength == 0) fxJump(the);
			unicodeBuffer = c_malloc(unicodeLength * 2);
			if (unicodeBuffer == NULL) fxJump(the);
			MultiByteToWideChar(CP_UTF8, 0, string, stringLength, unicodeBuffer, unicodeLength);
			if (flag)
				CharUpperBuffW(unicodeBuffer, unicodeLength);
			else
				CharLowerBuffW(unicodeBuffer, unicodeLength);
			resultLength = WideCharToMultiByte(CP_UTF8, 0, unicodeBuffer, unicodeLength, NULL, 0, NULL, NULL);
			result = fxNewChunk(the, resultLength + 1);
			WideCharToMultiByte(CP_UTF8, 0, unicodeBuffer, unicodeLength, result, resultLength, NULL, NULL);
			result[resultLength] = 0;
			c_free(unicodeBuffer);
		}
		mxCatch(the) {
			if (unicodeBuffer)
				c_free(unicodeBuffer);
			fxJump(the);
		}
	#elif (mxMacOSX || mxiOS)
		CFStringRef cfString = NULL;
		CFMutableStringRef mutableCFString = NULL;
		CFIndex resultLength;
		CFRange range;
		mxTry(the) {
			cfString = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)string, stringLength, kCFStringEncodingUTF8, false);
			if (cfString == NULL) fxJump(the);
			mutableCFString = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, cfString);
			if (mutableCFString == NULL) fxJump(the);
			if (flag)
				CFStringUppercase(mutableCFString, 0);
			else
				CFStringLowercase(mutableCFString, 0);
			range = CFRangeMake(0, CFStringGetLength(mutableCFString));
			CFStringGetBytes(mutableCFString, range, kCFStringEncodingUTF8, 0, false, NULL, 0, &resultLength);
			result = fxNewChunk(the, resultLength + 1);
			CFStringGetBytes(mutableCFString, range, kCFStringEncodingUTF8, 0, false, (UInt8 *)result, resultLength, NULL);
			result[resultLength] = 0;
			CFRelease(mutableCFString);
			CFRelease(cfString);
		}
		mxCatch(the) {
			if (mutableCFString)
				CFRelease(mutableCFString);
			if (cfString)
				CFRelease(cfString);
			fxJump(the);
		}
	#else
		txU1 *s, *r;
		txInteger c;
		const txUTF8Sequence *aSequence;
		txInteger aSize;
		const txCharCase* current;
		const txCharCase* limit = flag ? &gxCharCaseToUpper[mxCharCaseToUpperCount] : &gxCharCaseToLower[mxCharCaseToLowerCount];
		result = fxNewChunk(the, stringLength + 1);
		s = (txU1*)string;
		r = (txU1*)result;
		while ((c = c_read8(s++))) {
			for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
				if ((c & aSequence->cmask) == aSequence->cval)
					break;
			}
			aSize = aSequence->size - 1;
			while (aSize > 0) {
				aSize--;
				c = (c << 6) | (c_read8(s++) & 0x3F);
			}
			c &= aSequence->lmask;
			current = flag ? gxCharCaseToUpper : gxCharCaseToLower;
			while (current < limit) {
				if (c < current->from)
					break;
				if (c <= current->to) {
					if (current->delta)
						c += current->delta;
					else if (flag)
						c &= ~1;
					else
						c |= 1;
					break;
				}
				current++;
			}
			if (c < 0x80) {
				*r++ = (txU1)c;
			}
			else if (c < 0x800) {
				*r++ = (txU1)(0xC0 | (c >> 6));
				*r++ = (txU1)(0x80 | (c & 0x3F));
			}
			else if (c < 0x10000) {
				*r++ = (txU1)(0xE0 | (c >> 12));
				*r++ = (txU1)(0x80 | ((c >> 6) & 0x3F));
				*r++ = (txU1)(0x80 | (c & 0x3F));
			}
			else if (c < 0x200000) {
				*r++ = (txU1)(0xF0 | (c >> 18));
				*r++ = (txU1)(0x80 | ((c >> 12) & 0x3F));
				*r++ = (txU1)(0x80 | ((c >> 6) & 0x3F));
				*r++ = (txU1)(0x80 | (c  & 0x3F));
			}
		}
		*r = 0;
	#endif
	}
	else
		result = mxEmptyString.value.string;
	mxResult->value.string = result;
	mxResult->kind = XS_STRING_KIND;
}

void fx_String_prototype_toLowerCase(txMachine* the)
{
	fx_String_prototype_toCase(the, 0);
}

void fx_String_prototype_toUpperCase(txMachine* the)
{
	fx_String_prototype_toCase(the, 1);
}

void fx_String_prototype_trim(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis);
	txString start = fxSkipSpaces(string);
	txString current = start;
	txString stop = current;
	txInteger offset, length;
	while (c_read8(current)) {
		stop = current + 1;
		current = fxSkipSpaces(stop);
	}
	offset = start - string;
	length = stop - start;
	mxResult->value.string = (txString)fxNewChunk(the, length + 1);
	c_memcpy(mxResult->value.string, mxThis->value.string + offset, length);
	mxResult->value.string[length] = 0;
	mxResult->kind = XS_STRING_KIND;
}

void fx_String_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxCheckString(the, mxThis);
	if (!slot) mxTypeError("this is no string");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

txSlot* fxCheckString(txMachine* the, txSlot* it)
{
	txSlot* result = C_NULL;
	if ((it->kind == XS_STRING_KIND) || (it->kind == XS_STRING_X_KIND))
		result = it;
	else if (it->kind == XS_REFERENCE_KIND) {
		it = it->value.reference->next;
		if (it && (it->flag & XS_INTERNAL_FLAG) && ((it->kind == XS_STRING_KIND) || (it->kind == XS_STRING_X_KIND)))
			result = it;
	}
	return result;
}

txString fxCoerceToString(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_UNDEFINED_KIND)
		mxTypeError("this is undefined");
	if (theSlot->kind == XS_NULL_KIND)
		mxTypeError("this is null");
	return fxToString(the, theSlot);
}

void fx_String_prototype_iterator(txMachine* the)
{
	txString string = fxCoerceToString(the, mxThis);
	txSlot* property;
	mxPush(mxStringIteratorPrototype);
	property = fxLastProperty(the, fxNewIteratorInstance(the, mxThis));
	property = fxNextIntegerProperty(the, property, fxUnicodeLength(string), mxID(_length), XS_GET_ONLY);
	mxPullSlot(mxResult);
}

void fx_String_prototype_iterator_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* length = index->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	if (index->value.integer < length->value.integer) {
		txInteger offset = fxUnicodeToUTF8Offset(iterable->value.string, index->value.integer);
		txInteger length = fxUnicodeToUTF8Offset(iterable->value.string + offset, 1);
		value->value.string = (txString)fxNewChunk(the, length + 1);
		c_memcpy(value->value.string, iterable->value.string + offset, length);
		value->value.string[length] = 0;
		value->kind = XS_STRING_KIND;
		index->value.integer++;
	}
	else {
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}

txInteger fxArgToPosition(txMachine* the, txInteger argi, txInteger index, txInteger length)
{
	if ((mxArgc > argi) && (mxArgv(argi)->kind != XS_UNDEFINED_KIND)) {
		txNumber i = c_trunc(fxToNumber(the, mxArgv(argi)));
		if (c_isnan(i))
			i = 0;
		if (i < 0)
			index = 0;
		else if (i > (txNumber)length)
			index = length;
		else
			index = (txInteger)i;
	}
	return index;
}

void fxPushSubstitutionString(txMachine* the, txSlot* string, txInteger size, txInteger offset, txSlot* match, txInteger length, txInteger count, txSlot* captures, txSlot* replace)
{
	txString r;
	txInteger l;
	txBoolean flag;
	txByte c, d;
	txInteger i, j;
	txSlot* capture;
	txString s;
	r = replace->value.string;
	l = 0;
	flag = 0;
	while ((c = c_read8(r++))) {
		if (c == '$') {
			c = c_read8(r++);
			switch (c) {
			case '$':
				l++;
				flag = 1;
				break;
			case '&':
				l += length;
				flag = 1;
				break;
			case '`':
				l += offset;
				flag = 1;
				break;
			case '\'':
				l += size - (offset + length);
				flag = 1;
				break;
			default:
				if (('0' <= c) && (c <= '9')) {
					i = c - '0';
					d = c_read8(r);
					if (('0' <= d) && (d <= '9')) {
						j = (i * 10) + d - '0';
						if ((0 < j) && (j <= count)) {
							i = j;
							r++;
						}
						else
							d = 0;
					}
					else
						d = 0;
					if ((0 < i) && (i <= count)) {
						capture = (captures + count - i);
						if (capture->kind != XS_UNDEFINED_KIND)
							l += c_strlen(capture->value.string);
						flag = 1;
					}
					else {
						l++;
						l++;
						if (d)
							l++;
					}
				}
				else {
					l++;
					if (c)
						l++;
				}
				break;
			}
			if (!c)
				break;
		}
		else
			l++;
	}
	if (flag) {
		mxPushUndefined();
		the->stack->value.string = (txString)fxNewChunk(the, l + 1);
		the->stack->kind = XS_STRING_KIND;
		r = replace->value.string;
		s = the->stack->value.string;
		while ((c = c_read8(r++))) {
			if (c == '$') {
				c = c_read8(r++);
				switch (c) {
				case '$':
					*s++ = c;
					break;
				case '&':
					l = length;
					c_memcpy(s, match->value.string, l);
					s += l;
					break;
				case '`':
					l = offset;
					c_memcpy(s, string->value.string, l);
					s += l;
					break;
				case '\'':
					l = size - (offset + length);
					c_memcpy(s, string->value.string + offset + length, l);
					s += l;
					break;
				default:
					if (('0' <= c) && (c <= '9')) {
						i = c - '0';
						d = c_read8(r);
						if (('0' <= d) && (d <= '9')) {
							j = (i * 10) + d - '0';
							if ((0 < j) && (j <= count)) {
								i = j;
								r++;
							}
							else
								d = 0;
						}
						else
							d = 0;
						if ((0 < i) && (i <= count)) {
							capture = (captures + count - i);
							if (capture->kind != XS_UNDEFINED_KIND) {
								l = c_strlen(capture->value.string);
								c_memcpy(s, capture->value.string, l);
								s += l;
							}
						}
						else {
							*s++ = '$';
							*s++ = c;
							if (d)
								*s++ = d;
						}
					}
					else {
						*s++ = '$';
						if (c)
							*s++ = c;
					}
					break;
				}
				if (!c)
					break;
			}
			else
				*s++ = c;
		}
		*s = 0;
	}
	else
		mxPushSlot(replace);
}


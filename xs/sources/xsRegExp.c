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
#if mxRegExp
static txNumber fxAdvanceStringIndex(txString string, txNumber index, txBoolean unicodeFlag);
static txSlot* fxCheckRegExpInstance(txMachine* the, txSlot* slot);
static void fxExecuteRegExp(txMachine* the, txSlot* regexp, txSlot* argument);
#endif

static void fx_RegExp_prototype_get_flag(txMachine* the, txU4 flag);
static void fx_RegExp_prototype_split_aux(txMachine* the, txSlot* string, txIndex start, txIndex stop, txSlot* item);

void fxBuildRegExp(txMachine* the)
{
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_RegExp_prototype_compile), 0, mxID(_compile), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_RegExp_prototype_exec), 1, mxID(_exec), XS_DONT_ENUM_FLAG);
	mxExecuteRegExpFunction.kind = slot->kind;
	mxExecuteRegExpFunction.value = slot->value;
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_RegExp_prototype_match), 1, mxID(_Symbol_match), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_RegExp_prototype_matchAll), 1, mxID(_Symbol_matchAll), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_RegExp_prototype_replace), 2, mxID(_Symbol_replace), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_RegExp_prototype_search), 1, mxID(_Symbol_search), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_RegExp_prototype_split), 2, mxID(_Symbol_split), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_RegExp_prototype_test), 1, mxID(_test), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_RegExp_prototype_toString), 0, mxID(_toString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_RegExp_prototype_get_dotAll), C_NULL, mxID(_dotAll), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_RegExp_prototype_get_flags), C_NULL, mxID(_flags), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_RegExp_prototype_get_global), C_NULL, mxID(_global), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_RegExp_prototype_get_hasIndices), C_NULL, mxID(_hasIndices), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_RegExp_prototype_get_ignoreCase), C_NULL, mxID(_ignoreCase), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_RegExp_prototype_get_multiline), C_NULL, mxID(_multiline), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_RegExp_prototype_get_source), C_NULL, mxID(_source), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_RegExp_prototype_get_sticky), C_NULL, mxID(_sticky), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_RegExp_prototype_get_unicode), C_NULL, mxID(_unicode), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxRegExpPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_RegExp), 2, mxID(_RegExp));
	mxRegExpConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_species_get), C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	mxPop();
	fxNewHostFunction(the, mxCallback(fxInitializeRegExp), 2, XS_NO_ID, XS_NO_ID);
	mxInitializeRegExpFunction = *the->stack;
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_RegExp_prototype_matchAll_next), 0, mxID(_next), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "RegExp String Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxRegExpStringIteratorPrototype);

	mxPop();
}

#if mxRegExp
txNumber fxAdvanceStringIndex(txString string, txNumber index, txBoolean unicodeFlag)
{
#if mxCESU8
	if (unicodeFlag) {
		txInteger character;
		txSize offset = fxUnicodeLength(string);
		if (index >= offset)
			return index + 1;
		string += fxUnicodeToUTF8Offset(string, (txInteger)index);
		offset = mxPtrDiff(mxStringByteDecode(string, &character) - string);
		if (character == C_EOF)
			return index + 1;
		return index + fxUTF8ToUnicodeOffset(string, offset);
	}
#endif
	return index + 1;
}

txSlot* fxCheckRegExpInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if ((slot->next) && (slot->next->flag & XS_INTERNAL_FLAG) && (slot->next->kind == XS_REGEXP_KIND))
			return slot;
	}
	mxTypeError("this is no RegExp instance");
	return C_NULL;
}

void fxExecuteRegExp(txMachine* the, txSlot* regexp, txSlot* argument)
{
	mxPushSlot(regexp);
	mxDub();
	mxGetID(mxID(_exec));
	if ((the->stack->kind != XS_REFERENCE_KIND) || (!mxIsFunction(the->stack->value.reference))) {
		the->stack->kind = mxExecuteRegExpFunction.kind;
		the->stack->value = mxExecuteRegExpFunction.value;
	}
	mxCall();
	mxPushSlot(argument);
	mxRunCount(1);
	if ((the->stack->kind != XS_NULL_KIND) && (the->stack->kind != XS_REFERENCE_KIND))
		mxTypeError("invalid exec result");
}
#endif

void fxInitializeRegExp(txMachine* the) 
{
#if mxRegExp
	txSlot* instance = fxToInstance(the, mxThis);
	txSlot* regexp = instance->next;
	txSlot* key = regexp->next;
	txString pattern;
	txString modifier;

	if (mxArgv(0)->kind == XS_UNDEFINED_KIND)
		*mxArgv(0) = mxEmptyString;
	else
		fxToString(the, mxArgv(0));
	if (mxArgv(1)->kind == XS_UNDEFINED_KIND)
		*mxArgv(1) = mxEmptyString;
	else
		fxToString(the, mxArgv(1));
    key->kind = (mxArgv(0)->kind == XS_STRING_X_KIND) ? XS_KEY_X_KIND : XS_KEY_KIND;
    pattern = key->value.key.string = mxArgv(0)->value.string;
	modifier = mxArgv(1)->value.string;
	if (!fxCompileRegExp(the, pattern, modifier, &regexp->value.regexp.code, &regexp->value.regexp.data, the->nameBuffer, sizeof(the->nameBuffer)))
		mxSyntaxError("invalid regular expression: %s", the->nameBuffer);
	*mxResult = *mxThis;
#endif
}

txBoolean fxIsRegExp(txMachine* the, txSlot* slot)
{
#if mxRegExp
    if (mxIsReference(slot)) {
        txSlot* instance = slot->value.reference;
        mxPushSlot(slot);
        mxGetID(mxID(_Symbol_match));
        if (the->stack->kind != XS_UNDEFINED_KIND)
            return fxToBoolean(the, the->stack++);
        mxPop();
        if ((instance->next) && (instance->next->flag & XS_INTERNAL_FLAG) && (instance->next->kind == XS_REGEXP_KIND))
            return 1;
    }
#endif
	return 0;
}

txSlot* fxNewRegExpInstance(txMachine* the)
{
#if mxRegExp
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);

	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_REGEXP_KIND;
	property->value.regexp.code = C_NULL;
	property->value.regexp.data = C_NULL;

	property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = (mxEmptyString.kind == XS_STRING_X_KIND) ? XS_KEY_X_KIND : XS_KEY_KIND;
	property->value.key.string = mxEmptyString.value.string;
	property->value.key.sum = 0;

	property = fxNextIntegerProperty(the, property, 0, mxID(_lastIndex), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	
	return instance;
#else
	return NULL;
#endif
}

void fx_RegExp(txMachine* the)
{
#if mxRegExp
	txSlot* pattern = ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) ? mxArgv(0) : C_NULL;
	txSlot* flags = ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) ? mxArgv(1) : C_NULL;
	txBoolean patternIsRegExp = (pattern && fxIsRegExp(the, pattern)) ? 1 : 0;
	
	if (mxTarget->kind == XS_UNDEFINED_KIND) {
		if (patternIsRegExp && !flags) {
			mxPushSlot(pattern);
			mxGetID(mxID(_constructor));
			if ((the->stack->kind == mxFunction->kind) && (the->stack->value.reference == mxFunction->value.reference)) {
				mxPop();
				*mxResult = *pattern;
				return;
			}
			mxPop();
		}
		mxPushSlot(mxFunction);
	}
	else {
		mxPushSlot(mxTarget);
	}
	fxGetPrototypeFromConstructor(the, &mxRegExpPrototype);
	fxNewRegExpInstance(the);
	mxPush(mxInitializeRegExpFunction);
	mxCall();
	if (patternIsRegExp) {
		mxPushSlot(pattern);
		mxGetID(mxID(_source));
		if (flags)
			mxPushSlot(flags);
		else {
			mxPushSlot(pattern);
			mxGetID(mxID(_flags));
		}
	}
	else {
		if (pattern)
			mxPushSlot(pattern);
		else
			mxPushUndefined();
		if (flags)
			mxPushSlot(flags);
		else
			mxPushUndefined();
	}
	mxRunCount(2);
	mxPullSlot(mxResult);
#else
	mxUnknownError("not built-in");
#endif
}

void fx_RegExp_prototype_get_flag(txMachine* the, txU4 flag)
{
#if mxRegExp
	txSlot* slot = mxThis;
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if (slot == mxRegExpPrototype.value.reference)
			return;
		slot = slot->next;
		if ((slot) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_REGEXP_KIND)) {
			txInteger flags = slot->value.regexp.code[0];
			mxResult->value.boolean = (flags & flag) ? 1 : 0;
			mxResult->kind = XS_BOOLEAN_KIND;
			return;
		}
	}
	mxTypeError("this is no object");
#endif
}

void fx_RegExp_prototype_get_flags(txMachine* the)
{
#if mxRegExp
	char flags[8];
	txIndex index = 0;
	if (mxThis->kind != XS_REFERENCE_KIND)
		mxTypeError("this is no object");
	mxPushSlot(mxThis);
	mxGetID(mxID(_hasIndices));
	if (fxToBoolean(the, the->stack++))
		flags[index++] = 'd';
	mxPushSlot(mxThis);
	mxGetID(mxID(_global));
	if (fxToBoolean(the, the->stack++))
		flags[index++] = 'g';
	mxPushSlot(mxThis);
	mxGetID(mxID(_ignoreCase));
	if (fxToBoolean(the, the->stack++))
		flags[index++] = 'i';
	mxPushSlot(mxThis);
	mxGetID(mxID(_multiline));
	if (fxToBoolean(the, the->stack++))
		flags[index++] = 'm';
	mxPushSlot(mxThis);
	mxGetID(mxID(_dotAll));
	if (fxToBoolean(the, the->stack++))
		flags[index++] = 's';
	mxPushSlot(mxThis);
	mxGetID(mxID(_unicode));
	if (fxToBoolean(the, the->stack++))
		flags[index++] = 'u';
	mxPushSlot(mxThis);
	mxGetID(mxID(_sticky));
	if (fxToBoolean(the, the->stack++))
		flags[index++] = 'y';
	flags[index] = 0;
	fxCopyStringC(the, mxResult, flags);
#endif
}

void fx_RegExp_prototype_get_dotAll(txMachine* the)
{
#if mxRegExp
	fx_RegExp_prototype_get_flag(the, XS_REGEXP_S);
#endif
}

void fx_RegExp_prototype_get_global(txMachine* the)
{
#if mxRegExp
	fx_RegExp_prototype_get_flag(the, XS_REGEXP_G);
#endif
}

void fx_RegExp_prototype_get_hasIndices(txMachine* the)
{
#if mxRegExp
	fx_RegExp_prototype_get_flag(the, XS_REGEXP_D);
#endif
}

void fx_RegExp_prototype_get_ignoreCase(txMachine* the)
{
#if mxRegExp
	fx_RegExp_prototype_get_flag(the, XS_REGEXP_I);
#endif
}

void fx_RegExp_prototype_get_multiline(txMachine* the)
{
#if mxRegExp
	fx_RegExp_prototype_get_flag(the, XS_REGEXP_M);
#endif
}

void fx_RegExp_prototype_get_source(txMachine* the)
{
#if mxRegExp
	txSlot* slot = mxThis;
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if (slot == mxRegExpPrototype.value.reference) {
			*mxResult = mxEmptyRegExp;
			return;
		}
		slot = slot->next;
		if ((slot) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_REGEXP_KIND)) {
			txString pattern;
			txInteger escape = 0;
			txInteger count = 0;
			txU1 c, *s, *r;
            slot = slot->next;
			pattern = slot->value.key.string;
			if (*pattern == 0) {
				*mxResult = mxEmptyRegExp;
				return;
			}
			s = (txU1*)pattern;
			while ((c = *s++)) {
				if ((c == 10) || (c == 13) || (c == '/'))
					escape++;
				else if ((c == 0xE2) && (s[0] == 0x80) && ((s[1] == 0xA8) || (s[1] == 0xA9))) /* LS || PS */
					escape += 3;
				count++;
			}
			if (escape) {
				mxResult->value.string = fxNewChunk(the, count + escape + 1);
				mxResult->kind = XS_STRING_KIND;
				s = (txU1*)slot->value.key.string;
				r = (txU1*)mxResult->value.string;
				while ((c = *s++)) {
					if (c == 10) {
						*r++ = '\\'; *r++ = 'n';
					}
					else if (c == 13) {
						*r++ = '\\'; *r++ = 'r';
					}
					else if (c == '/') {
						*r++ = '\\'; *r++ = '/';
					}
					else if ((c == 0xE2) && (s[0] == 0x80) && (s[1] == 0xA8)) {
						*r++ = '\\'; *r++ = 'u'; *r++ = '2'; *r++ = '0'; *r++ = '2'; *r++ = '8';
						s += 2;
					}
					else if ((c == 0xE2) && (s[0] == 0x80) && (s[1] == 0xA9)) {
						*r++ = '\\'; *r++ = 'u'; *r++ = '2'; *r++ = '0'; *r++ = '2'; *r++ = '9';
						s += 2;
					}
					else {
						*r++ = c;	
					}
				}
				*r = 0;
			}
			else {
				mxResult->value.string = slot->value.string;
				mxResult->kind = (slot->kind == XS_KEY_X_KIND) ? XS_STRING_X_KIND : XS_STRING_KIND;
			}
			return;
		}
	}
	mxTypeError("this is no RegExp instance");
#endif
}

void fx_RegExp_prototype_get_sticky(txMachine* the)
{
#if mxRegExp
	fx_RegExp_prototype_get_flag(the, XS_REGEXP_Y);
#endif
}

void fx_RegExp_prototype_get_unicode(txMachine* the)
{
#if mxRegExp
	fx_RegExp_prototype_get_flag(the, XS_REGEXP_U);
#endif
}

void fx_RegExp_prototype_compile(txMachine* the)
{
#if mxRegExp
	*mxResult = *mxThis;
#endif
}

void fx_RegExp_prototype_exec(txMachine* the)
{
#if mxRegExp
	txSlot* instance = fxCheckRegExpInstance(the, mxThis);
	txSlot* regexp = instance->next;
	txSlot* argument;
	txSlot* temporary;
	txNumber lastIndex;
	txInteger flags;
	txBoolean globalFlag;
	txBoolean namedFlag;
	txBoolean stickyFlag;
	txBoolean hasIndicesFlag;
	txInteger offset;

	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	fxToString(the, the->stack);
	argument = the->stack;
	
	if (regexp->value.regexp.data == C_NULL) {
		mxTemporary(temporary);
		temporary->value.regexp.code = C_NULL;
		temporary->value.regexp.data = fxAllocateRegExpData(the, regexp->value.regexp.code);
		temporary->kind = XS_REGEXP_KIND;
	}
	else
		temporary = regexp;

	mxPushSlot(mxThis);
	mxGetID(mxID(_lastIndex));
	lastIndex = fxToLength(the, the->stack);
	mxPop();
	if (lastIndex > 0x7FFFFFFF)
		lastIndex = 0x7FFFFFFF;

	flags = regexp->value.regexp.code[0];
	globalFlag = (flags & XS_REGEXP_G) ? 1 : 0;
	namedFlag = (flags & XS_REGEXP_N) ? 1 : 0;
	stickyFlag = (flags & XS_REGEXP_Y) ? 1 : 0;
	hasIndicesFlag = (flags & XS_REGEXP_D) ? 1 : 0;
	offset = (globalFlag || stickyFlag) ? fxUnicodeToUTF8Offset(argument->value.string, (txInteger)lastIndex) : 0;
	if ((offset >= 0) && fxMatchRegExp(the, regexp->value.regexp.code, temporary->value.regexp.data, argument->value.string, offset)) {
		txSlot* resultArray;
		txSlot* resultItem;
		txSlot* indicesArray;
		txSlot* indicesItem;
		txSlot* resultObject;
		txSlot* resultProperty;
		txSlot* indicesObject;
		txSlot* indicesProperty;
		txInteger count;
		txInteger index;
		txInteger length;
		if (globalFlag || stickyFlag) {
			index = fxUTF8ToUnicodeOffset(argument->value.string, temporary->value.regexp.data[1]);
			mxPushInteger(index);
			mxPushSlot(mxThis);
			mxSetID(mxID(_lastIndex));
			mxPop();
		}
		mxPush(mxArrayPrototype);
		resultArray = fxNewArrayInstance(the);
		resultItem = fxLastProperty(the, resultArray);
		if (hasIndicesFlag) {
			mxPush(mxArrayPrototype);
			indicesArray = fxNewArrayInstance(the);
			indicesItem = fxLastProperty(the, indicesArray);
		}
		if (namedFlag) {
			resultObject = fxNewInstance(the);
			resultProperty = fxLastProperty(the, resultObject);
			if (hasIndicesFlag) {
				indicesObject = fxNewInstance(the);
				indicesProperty = fxLastProperty(the, indicesObject);
			}
		}
		count = regexp->value.regexp.code[1];
		for (index = 0; index < count; index++) {
			txInteger start = temporary->value.regexp.data[2 * index];
			resultItem = resultItem->next = fxNewSlot(the);
			if (hasIndicesFlag)
				indicesItem = indicesItem->next = fxNewSlot(the);
			if (start >= 0) {
				txInteger end = temporary->value.regexp.data[(2 * index) + 1];
				length = end - start;
				resultItem->value.string = (txString)fxNewChunk(the, length + 1);
				c_memcpy(resultItem->value.string, argument->value.string + start, length);
				resultItem->value.string[length] = 0;
				resultItem->kind = XS_STRING_KIND;
				if (hasIndicesFlag) {
					start = fxUTF8ToUnicodeOffset(argument->value.string, start);
					end = start + fxUTF8ToUnicodeOffset(argument->value.string + start, length);
					mxPushInteger(start);
					mxPushInteger(end);
					fxConstructArrayEntry(the, indicesItem);
				}
			}
			if (namedFlag) {
				txInteger tmp = regexp->value.regexp.code[2 + index];
				txID name = (txID)tmp;
				if (name != XS_NO_ID) {
					resultProperty = resultProperty->next = fxNewSlot(the);
					resultProperty->value = resultItem->value;
					resultProperty->kind = resultItem->kind;
					resultProperty->ID = name;
					if (hasIndicesFlag) {
						indicesProperty = indicesProperty->next = fxNewSlot(the);
						indicesProperty->value = indicesItem->value;
						indicesProperty->kind = indicesItem->kind;
						indicesProperty->ID = name;
					}
				}
			}
			resultArray->next->value.array.length++;
			if (hasIndicesFlag)
				indicesArray->next->value.array.length++;
		}
		if (hasIndicesFlag) {
			fxCacheArray(the, indicesArray);
			indicesItem = fxLastProperty(the, indicesArray);
			indicesItem = indicesItem->next = fxNewSlot(the);
			indicesItem->ID = mxID(_groups);
			if (namedFlag) {
				indicesItem->value.reference = indicesObject;
				indicesItem->kind = XS_REFERENCE_KIND;
				mxPop();
			}
		}
		fxCacheArray(the, resultArray);
		resultItem = fxLastProperty(the, resultArray);
		resultItem = resultItem->next = fxNewSlot(the);
		resultItem->ID = mxID(_index);
		resultItem->kind = XS_INTEGER_KIND;
		resultItem->value.integer = fxUTF8ToUnicodeOffset(argument->value.string, temporary->value.regexp.data[0]);
		resultItem = resultItem->next = fxNewSlot(the);
		resultItem->ID = mxID(_input);
		resultItem->value.string = argument->value.string;
		resultItem->kind = argument->kind;
		resultItem = resultItem->next = fxNewSlot(the);
		resultItem->ID = mxID(_groups);
		if (namedFlag) {
			resultItem->value.reference = resultObject;
			resultItem->kind = XS_REFERENCE_KIND;
			mxPop();
		}
		if (hasIndicesFlag) {
			resultItem = resultItem->next = fxNewSlot(the);
			resultItem->ID = mxID(_indices);
			resultItem->value.reference = indicesArray;
			resultItem->kind = XS_REFERENCE_KIND;
			mxPop();
		}
	}
	else {
		if (globalFlag || stickyFlag) {
			mxPushInteger(0);
			mxPushSlot(mxThis);
			mxSetID(mxID(_lastIndex));
			mxPop();
		}
		mxPushNull();
	}
	mxPullSlot(mxResult);
#endif
}

void fx_RegExp_prototype_match(txMachine* the)
{
#if mxRegExp
	txSlot* argument;
	fxToInstance(the, mxThis);
	if (mxArgc == 0)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	argument = the->stack;
	fxToString(the, argument);
	mxPushSlot(mxThis);
	mxGetID(mxID(_global));
	if (fxToBoolean(the, the->stack++)) {
		txIndex count = 0;
		txBoolean unicodeFlag;
		mxPushSlot(mxThis);
		mxGetID(mxID(_unicode));
		unicodeFlag = fxToBoolean(the, the->stack);
		mxPop();
		mxPushInteger(0);
		mxPushSlot(mxThis);
		mxSetID(mxID(_lastIndex));
		mxPop();
		mxPush(mxArrayPrototype);
		fxNewArrayInstance(the);
		mxPullSlot(mxResult);
		for (;;) {
			fxExecuteRegExp(the, mxThis, argument);
			if (the->stack->kind == XS_NULL_KIND) {
				if (count == 0)
					mxPullSlot(mxResult);
				else
					mxPop();
				break;
			}
			mxGetIndex(0);
            fxToString(the, the->stack);
			mxPushSlot(mxResult);
			mxDefineIndex(count, 0, XS_GET_ONLY);
			if (c_isEmpty(the->stack->value.string)) {
				mxPushSlot(mxThis);
				mxGetID(mxID(_lastIndex));
				fxToLength(the, the->stack);
				the->stack->value.number = fxAdvanceStringIndex(argument->value.string, the->stack->value.number, unicodeFlag);
				mxPushSlot(mxThis);
				mxSetID(mxID(_lastIndex));
				mxPop();
			}
			mxPop();
			count++;
		}
	}
	else {
		fxExecuteRegExp(the, mxThis, argument);
		mxPullSlot(mxResult);
	}
	mxPop();
#endif
}

void fx_RegExp_prototype_matchAll(txMachine* the)
{
#if mxRegExp
	txSlot* argument;
	txBoolean globalFlag = 0;
	txBoolean unicodeFlag = 0;
	txSlot* matcher;
	txSlot* iterator;
	txSlot* property;
	
	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	if (mxArgc == 0)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	argument = the->stack;
	fxToString(the, argument);
	
	mxPushSlot(mxThis);
	mxGetID(mxID(_constructor));
	fxToSpeciesConstructor(the, &mxRegExpConstructor);
	mxNew();	
	mxPushSlot(mxThis);
	mxPushSlot(mxThis);
	mxGetID(mxID(_flags));
	if (c_strchr(fxToString(the, the->stack), 'g'))
		globalFlag = 1;
	if (c_strchr(fxToString(the, the->stack), 'u'))
		unicodeFlag = 1;
	mxRunCount(2);
	matcher = the->stack;
	
	mxPushSlot(mxThis);
	mxGetID(mxID(_lastIndex));
	fxToInteger(the, the->stack);
	mxPushSlot(matcher);
	mxSetID(mxID(_lastIndex));
	
	mxPush(mxRegExpStringIteratorPrototype);
	iterator = fxNewIteratorInstance(the, matcher, mxID(_RegExp));
	property = fxLastProperty(the, iterator);
	property->kind = argument->kind;
	property->value = argument->value;
	property = fxNextBooleanProperty(the, property, globalFlag, XS_NO_ID, XS_INTERNAL_FLAG);
	property = fxNextBooleanProperty(the, property, unicodeFlag, XS_NO_ID, XS_INTERNAL_FLAG);
	property = fxNextBooleanProperty(the, property, 0, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPullSlot(mxResult);
#endif
}

void fx_RegExp_prototype_matchAll_next(txMachine* the)
{
#if mxRegExp
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis, mxID(_RegExp));
	txSlot* result = iterator->next;
	txSlot* value = fxCheckIteratorResult(the, result);
	txSlot* done = value->next;
	txSlot* matcher = result->next;
	txSlot* argument = matcher->next;
	txSlot* global = argument->next;
	txSlot* unicode = global->next;
	txSlot* complete = unicode->next;
	txSlot* match;
	if (complete->value.boolean) {
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
	}
	else {
		fxExecuteRegExp(the, matcher, argument);
		match = the->stack;
		if (match->kind == XS_NULL_KIND) {
			value->kind = XS_UNDEFINED_KIND;
			done->value.boolean = 1;
            complete->value.boolean = 1;
		}
		else if (global->value.boolean) {
			mxPushSlot(match);
			mxGetIndex(0);
			fxToString(the, the->stack);
			if (c_isEmpty(the->stack->value.string)) {
				mxPushSlot(matcher);
				mxGetID(mxID(_lastIndex));
				fxToLength(the, the->stack);
				the->stack->value.number = fxAdvanceStringIndex(argument->value.string, the->stack->value.number, unicode->value.boolean);
				mxPushSlot(matcher);
				mxSetID(mxID(_lastIndex));
				mxPop();
			}
			mxPop();
			value->kind = match->kind;
			value->value = match->value;
			done->value.boolean = 0;
		}
		else {
			value->kind = match->kind;
			value->value = match->value;
			done->value.boolean = 0;
			complete->value.boolean = 1;
		}
		mxPop();
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
#endif
}

void fx_RegExp_prototype_replace(txMachine* the)
{
#if mxRegExp
	txSlot* argument;
	txSlot* function = C_NULL;;
	txSlot* replacement = C_NULL;;
	txBoolean globalFlag;
	txBoolean unicodeFlag = 0;
	txSlot* list;
	txSlot* item;
	txSize size; 
	txSize utf8Size; 
	txInteger former; 
	txSlot* result;
	txSlot* matched;
	txSize matchLength; 
	txInteger c, i;
	txInteger position; 
	fxToInstance(the, mxThis);
	if (mxArgc <= 0)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	argument = the->stack;
	fxToString(the, argument);
	if (mxArgc <= 1)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(1));
	if (mxIsReference(the->stack) && mxIsFunction(the->stack->value.reference))
		function = the->stack;
	else {		
		replacement = the->stack;
		fxToString(the, replacement);
	}
	mxPushSlot(mxThis);
	mxGetID(mxID(_global));
	globalFlag = fxToBoolean(the, the->stack++);
	if (globalFlag) {
		mxPushSlot(mxThis);
		mxGetID(mxID(_unicode));
		unicodeFlag = fxToBoolean(the, the->stack++);
		
		mxPushInteger(0);
		mxPushSlot(mxThis);
		mxSetID(mxID(_lastIndex));
		mxPop();
	}
	list = item = fxNewInstance(the);
	mxPushSlot(list);
	size = fxUnicodeLength(argument->value.string);
	utf8Size = mxStringLength(argument->value.string);
	former = 0;
	for (;;) {
		fxExecuteRegExp(the, mxThis, argument);
		if (the->stack->kind == XS_NULL_KIND) {
			mxPop();
			break;
		}
		result = the->stack;
		
		mxPushSlot(result);
		mxGetID(mxID(_index));
		position = fxToInteger(the, the->stack);
		mxPop();
		if (position < 0) position = 0;
		else if (position > size) position = size;
		
		if (former < position) {
			item = item->next = fxNewSlot(the);
			fx_RegExp_prototype_split_aux(the, argument, former, position, item);
		}

        if (former <= position) {
            mxPushSlot(result);
            mxGetIndex(0);
            fxToString(the, the->stack);
            matched = the->stack;
            matchLength = fxUnicodeLength(matched->value.string);

            mxPushSlot(result);
            mxGetID(mxID(_length));
            c = fxToInteger(the, the->stack);
			mxPop();

           	if (function) {
                mxPushUndefined();
                mxPushSlot(function);
            	mxCall();
            }
           	mxPushSlot(matched);
            for (i = 1; i < c; i++) {
                mxPushSlot(result);
                mxGetIndex(i);
                if (the->stack->kind != XS_UNDEFINED_KIND)
               		fxToString(the, the->stack);
            }
            if (function) {
               	mxPushInteger(position);
				mxPushSlot(argument);
				mxPushSlot(result);
				mxGetID(mxID(_groups));
				if (mxIsUndefined(the->stack)) {
					mxPop();
					mxRunCount(3 + i - 1);
				}
				else {
					mxRunCount(4 + i - 1);
				}
                fxToString(the, the->stack);
                item = item->next = fxNewSlot(the);
                mxPullSlot(item);
            }
            else {
 				mxPushSlot(result);
				mxGetID(mxID(_groups));
				if (!mxIsUndefined(the->stack))
					fxToInstance(the, the->stack);
				fxPushSubstitutionString(the, argument, utf8Size, fxUnicodeToUTF8Offset(argument->value.string, position), matched, mxStringLength(matched->value.string), i - 1, the->stack + 1, the->stack, replacement);
                item = item->next = fxNewSlot(the);
                mxPullSlot(item);
                the->stack += 1 + i;			
            }
            former = position + matchLength;
            
            mxPop(); // matched
        }
        else
            matchLength = 0;
            
		if (!globalFlag)
			break;
			
		if (0 == matchLength) {
			mxPushSlot(mxThis);
			mxGetID(mxID(_lastIndex));
			fxToLength(the, the->stack);
			the->stack->value.number = fxAdvanceStringIndex(argument->value.string, the->stack->value.number, unicodeFlag);
			mxPushSlot(mxThis);
			mxSetID(mxID(_lastIndex));
			mxPop();
		}
		mxPop();
	}
	if (former < size) {
		item = item->next = fxNewSlot(the);
		fx_RegExp_prototype_split_aux(the, argument, former, size, item);
	}
	size = 0;
	item = list->next;
	while (item) {
		item->value.key.sum = mxStringLength(item->value.string);
        size = fxAddChunkSizes(the, size, item->value.key.sum);
		item = item->next;
	}
    size = fxAddChunkSizes(the, size, 1);
	mxResult->value.string = (txString)fxNewChunk(the, size);
	size = 0;
	item = list->next;
	while (item) {
		c_memcpy(mxResult->value.string + size, item->value.string, item->value.key.sum);
		size += item->value.key.sum;
		item = item->next;
	}
	mxResult->value.string[size] = 0;
	mxResult->kind = XS_STRING_KIND;
	mxPop();
	mxPop();
#endif
}

void fx_RegExp_prototype_search(txMachine* the)
{
#if mxRegExp
	txSlot* argument;
	txSlot* lastIndex;
	fxToInstance(the, mxThis);
	if (mxArgc == 0)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	argument = the->stack;
	fxToString(the, argument);
	mxPushSlot(mxThis);
	mxGetID(mxID(_lastIndex));
	lastIndex = the->stack;
	
	mxPushInteger(0);
	if (!fxIsSameValue(the, the->stack, lastIndex, 0)) {
		mxPushInteger(0);
		mxPushSlot(mxThis);
		mxSetID(mxID(_lastIndex));
		mxPop();
	}
	mxPop();
	
	fxExecuteRegExp(the, mxThis, argument);
	if (the->stack->kind == XS_NULL_KIND) {
		the->stack->kind = XS_INTEGER_KIND;
		the->stack->value.integer = -1;
	}
	else
		mxGetID(mxID(_index));
	mxPullSlot(mxResult);
	
	mxPushSlot(mxThis);
	mxGetID(mxID(_lastIndex));
	if (!fxIsSameValue(the, the->stack, lastIndex, 0)) {
		mxPushSlot(lastIndex);
		mxPushSlot(mxThis);
		mxSetID(mxID(_lastIndex));
		mxPop();
	}
	mxPop();
	
	mxPop();
#endif
}

void fx_RegExp_prototype_split(txMachine* the)
{
#if mxRegExp
	txSlot* argument;
	txBoolean unicodeFlag = 0;
	txIndex limit;
	txSlot* splitter;
	txSlot* array;
	txSlot* item;
	txInteger size, p, q, e, c, i;
	
	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	if (mxArgc == 0)
		mxPushUndefined();
	else
		mxPushSlot(mxArgv(0));
	argument = the->stack;
	fxToString(the, argument);
	limit = ((mxArgc > 1) && (!mxIsUndefined(mxArgv(1)))) ? fxToUnsigned(the, mxArgv(1)) : 0xFFFFFFFF;
	
	mxPushSlot(mxThis);
	mxGetID(mxID(_constructor));
	fxToSpeciesConstructor(the, &mxRegExpConstructor);
	mxNew();	
	mxPushSlot(mxThis);
	mxPushSlot(mxThis);
	mxGetID(mxID(_flags));
	if (!c_strchr(fxToString(the, the->stack), 'y'))
		fxConcatStringC(the, the->stack, "y");
	if (c_strchr(fxToString(the, the->stack), 'u'))
		unicodeFlag = 1;
	mxRunCount(2);
	splitter = the->stack;
	
	mxPush(mxArrayPrototype);
	array = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	item = fxLastProperty(the, array);
	if (!limit)
		goto bail;
	size = fxUnicodeLength(argument->value.string);
	if (size == 0) {
		fxExecuteRegExp(the, splitter, argument);
		if (the->stack->kind == XS_NULL_KIND) {
			item = item->next = fxNewSlot(the);
			item->value.string = mxEmptyString.value.string;
			item->kind = mxEmptyString.kind;
			array->next->value.array.length++;
		}
		mxPop();
		goto bail;
	}
	p = q = 0;
	while (q < size) {
		mxPushInteger(q);
		mxPushSlot(splitter);
		mxSetID(mxID(_lastIndex));
		mxPop();
		fxExecuteRegExp(the, splitter, argument);
		if (the->stack->kind == XS_NULL_KIND) {
			q = (txInteger)fxAdvanceStringIndex(argument->value.string, q, unicodeFlag);
		}
		else {
			mxPushSlot(splitter);
			mxGetID(mxID(_lastIndex));
			e = fxToInteger(the, the->stack);
			mxPop();
			if (e == p) {
				q = (txInteger)fxAdvanceStringIndex(argument->value.string, q, unicodeFlag);
			}
			else {
				txSlot* result = the->stack;
				item = item->next = fxNewSlot(the);
				fx_RegExp_prototype_split_aux(the, argument, p, q, item);
				array->next->value.array.length++;
				if (array->next->value.array.length == limit)
					goto bail;
				p = e;
				mxPushSlot(result);
				mxGetID(mxID(_length));
				c = fxToInteger(the, the->stack);
				mxPop();
				c--; if (c < 0) c = 0;
				i = 1;
				while (i <= c) {
					mxPushSlot(result);
					mxGetIndex(i);
					item = item->next = fxNewSlot(the);
					mxPullSlot(item);
					array->next->value.array.length++;
					if (array->next->value.array.length == limit)
						goto bail;
					i++;
				}
				q = p;
			}
		}
		mxPop();
	}
	//if (p < 0)
	//	p = 0;
	item = item->next = fxNewSlot(the);
	fx_RegExp_prototype_split_aux(the, argument, p, size, item);
	array->next->value.array.length++;
bail:
	fxCacheArray(the, array);
	mxPop();
	mxPop();
	mxPop();
#endif
}

void fx_RegExp_prototype_split_aux(txMachine* the, txSlot* string, txIndex start, txIndex stop, txSlot* item)
{
#if mxRegExp
	txInteger offset = fxUnicodeToUTF8Offset(string->value.string, start);
	txInteger length = fxUnicodeToUTF8Offset(string->value.string + offset, stop - start);
	if ((offset >= 0) && (length > 0)) {
		item->value.string = (txString)fxNewChunk(the, length + 1);
		c_memcpy(item->value.string, string->value.string + offset, length);
		item->value.string[length] = 0;
		item->kind = XS_STRING_KIND;
	}
	else {
		item->value.string = mxEmptyString.value.string;
		item->kind = mxEmptyString.kind;
	}
#endif
}

void fx_RegExp_prototype_test(txMachine* the)
{
#if mxRegExp
	fxToInstance(the, mxThis);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	fxToString(the, the->stack);
	fxExecuteRegExp(the, mxThis, the->stack);
	mxResult->value.boolean = (the->stack->kind != XS_NULL_KIND) ? 1 : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxPop();
#endif
}

void fx_RegExp_prototype_toString(txMachine* the)
{
#if mxRegExp
	fxToInstance(the, mxThis);
	fxStringX(the, mxResult, "/");
	mxPushSlot(mxThis);
	mxGetID(mxID(_source));
	fxToString(the, the->stack);
	fxConcatString(the, mxResult, the->stack);
	mxPop();
	fxConcatStringC(the, mxResult, "/");
	mxPushSlot(mxThis);
	mxGetID(mxID(_flags));
	fxToString(the, the->stack);
	fxConcatString(the, mxResult, the->stack);
	mxPop();
#endif
}

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

static txSlot* fxCheckSymbol(txMachine* the, txSlot* it);

void fxBuildSymbol(txMachine* the)
{
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_Symbol_prototype_get_description), C_NULL, mxID(_description), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Symbol_prototype_toString), 0, mxID(_toString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Symbol_prototype_valueOf), 0, mxID(_valueOf), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Symbol_prototype_toPrimitive), 1, mxID(_Symbol_toPrimitive), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextStringXProperty(the, slot, "Symbol", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxSymbolPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_Symbol), 0, mxID(_Symbol));
	mxSymbolConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Symbol_for), 1, mxID(_for), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Symbol_keyFor), 1, mxID(_keyFor), XS_DONT_ENUM_FLAG);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_asyncIterator), mxID(_asyncIterator), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_hasInstance), mxID(_hasInstance), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_isConcatSpreadable), mxID(_isConcatSpreadable), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_iterator), mxID(_iterator), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_match), mxID(_match), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_matchAll), mxID(_matchAll), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_replace), mxID(_replace), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_search), mxID(_search), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_species), mxID(_species), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_split), mxID(_split), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_toPrimitive), mxID(_toPrimitive), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_toStringTag), mxID(_toStringTag), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_unscopables), mxID(_unscopables), XS_GET_ONLY);
#if mxExplicitResourceManagement	
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_asyncDispose), mxID(_asyncDispose), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_dispose), mxID(_dispose), XS_GET_ONLY);
#endif
	mxPop();
}

txSlot* fxNewSymbolInstance(txMachine* the)
{
	txSlot* instance;
	instance = fxNewObjectInstance(the);
	fxNextSymbolProperty(the, instance, XS_NO_ID, XS_NO_ID, XS_INTERNAL_FLAG);
	return instance;
}

void fx_Symbol(txMachine* the)
{
	txSlot* instance;
	txSlot* slot;
	txSlot* property;
	if (mxTarget->kind != XS_UNDEFINED_KIND)
		mxTypeError("new Symbol");
	instance = fxNewInstance(the);
	property = fxNextUndefinedProperty(the, instance, XS_NO_ID, XS_INTERNAL_FLAG);
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
		fxToString(the, mxArgv(0));
		fxNextSlotProperty(the, property, mxArgv(0), XS_NO_ID, XS_INTERNAL_FLAG);
	}	
	else
		fxNextUndefinedProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG);
	slot = fxFindKey(the);
	slot->flag = XS_INTERNAL_FLAG;
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = instance;
	property->kind = XS_SYMBOL_KIND;
	property->value.symbol = slot->ID;
	mxPop();
	mxResult->kind = XS_SYMBOL_KIND;
	mxResult->value.symbol = slot->ID;
}

void fx_Symbol_for(txMachine* the)
{
	txString string;
	txU1* p;
	txU4 sum;
	txU4 modulo;
	txSlot* result;
	if (mxArgc < 1)
		mxSyntaxError("no key parameter");
	string = fxToString(the, mxArgv(0));
	p = (txU1*)string;
	sum = 0;
	while(*p != 0) {
		sum = (sum << 1) + *p++;
	}
	sum &= 0x7FFFFFFF;
	modulo = sum % the->symbolModulo;
	result = the->symbolTable[modulo];
	while (result != C_NULL) {
		if (result->value.key.sum == sum)
			if (c_strcmp(result->value.key.string, string) == 0)
				break;
		result = result->next;
	}
	if (result == C_NULL) {
		result = fxFindKey(the);
		result->next = the->symbolTable[modulo];
		result->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG;
		result->kind = (mxArgv(0)->kind == XS_STRING_X_KIND) ? XS_KEY_X_KIND : XS_KEY_KIND;
		result->value.key.string = mxArgv(0)->value.string;
		result->value.key.sum = sum;
		the->symbolTable[modulo] = result;
	}
	mxResult->kind = XS_SYMBOL_KIND;
	mxResult->value.symbol = result->ID;
}

void fx_Symbol_keyFor(txMachine* the)
{
	txSlot* key;
	if ((mxArgc == 0) || (mxArgv(0)->kind != XS_SYMBOL_KIND))
		mxTypeError("sym is no symbol");
	key = fxGetKey(the, mxArgv(0)->value.symbol);
	if (((key->kind == XS_KEY_KIND) || (key->kind == XS_KEY_X_KIND)) && ((key->flag & XS_DONT_ENUM_FLAG) == 0)) {
		mxResult->kind = (key->kind == XS_KEY_KIND) ? XS_STRING_KIND : XS_STRING_X_KIND;
		mxResult->value.string = key->value.key.string;
	}
}

void fx_Symbol_prototype_get_description(txMachine* the)
{
	txSlot* slot = fxCheckSymbol(the, mxThis);
	if (!slot) mxTypeError("this is no symbol");
	slot = fxGetKey(the, slot->value.symbol);
	if ((slot->kind == XS_KEY_KIND) || (slot->kind == XS_KEY_X_KIND)) {
		mxResult->kind = (slot->kind == XS_KEY_KIND) ? XS_STRING_KIND : XS_STRING_X_KIND;
		mxResult->value.string = slot->value.key.string;
	}
	else if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference->next->next;
		mxResult->kind = slot->kind;
		mxResult->value = slot->value;
	}
}

void fx_Symbol_prototype_toPrimitive(txMachine* the)
{
	txSlot* slot = fxCheckSymbol(the, mxThis);
	if (!slot) mxTypeError("this is no symbol");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

void fx_Symbol_prototype_toString(txMachine* the)
{
	txSlot* slot = fxCheckSymbol(the, mxThis);
	if (!slot) mxTypeError("this is no symbol");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
	fxSymbolToString(the, mxResult);
}

void fx_Symbol_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxCheckSymbol(the, mxThis);
	if (!slot) mxTypeError("this is no symbol");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

txSlot* fxCheckSymbol(txMachine* the, txSlot* it)
{
	txSlot* result = C_NULL;
	if (it->kind == XS_SYMBOL_KIND)
		result = it;
	else if (it->kind == XS_REFERENCE_KIND) {
		txSlot* instance = it->value.reference;
		it = instance->next;
		if ((it) && (it->flag & XS_INTERNAL_FLAG) && (it->kind == XS_SYMBOL_KIND))
			result = it;
	}
	return result;
}

void fxSymbolToString(txMachine* the, txSlot* slot)
{
	txSlot* key = fxGetKey(the, slot->value.symbol);
	fxStringX(the, slot, "Symbol(");
	if ((key->kind == XS_KEY_KIND) || (key->kind == XS_KEY_X_KIND))
		fxConcatString(the, slot, key);
	else if (key->kind == XS_REFERENCE_KIND) {
		key = key->value.reference->next->next;
		if (key->kind != XS_UNDEFINED_KIND)
			fxConcatString(the, slot, key);
	}
	fxConcatStringC(the, slot, ")");
}

txSlot* fxGetKey(txMachine* the, txID theID)
{
	txSlot* key;
	mxCheck(the, 0 < theID);
	mxCheck(the, theID < the->keyIndex);
	if (theID < the->keyOffset)
		key = the->keyArrayHost[theID];
	else
		key = the->keyArray[theID - the->keyOffset];
	mxCheck(the, key->kind != XS_UNDEFINED_KIND);
	return key;
}

char* fxGetKeyName(txMachine* the, txID theID)
{
	txSlot* key = fxGetKey(the, theID);
	if (key->flag & XS_DONT_ENUM_FLAG)
		return key->value.string;
	return C_NULL;
}

char* fxGetKeyString(txMachine* the, txID theID, txBoolean* adorn)
{
	txString result = mxEmptyString.value.string;
	txSlot* key = fxGetKey(the, theID);
	txKind kind = key->kind;
	if ((kind == XS_KEY_KIND) || (kind == XS_KEY_X_KIND))
		result = key->value.key.string;
	else if (key->kind == XS_REFERENCE_KIND) {
		key = key->value.reference->next->next;
		if (key->kind != XS_UNDEFINED_KIND)
			result = key->value.string;
	}
	if (adorn)
		*adorn = (key->flag & XS_DONT_ENUM_FLAG) ? 0 : 1;
	return result;
}

void fxPushKeyString(txMachine* the, txID theID, txBoolean* adorn)
{
	txSlot* key = fxGetKey(the, theID);
	txKind kind = key->kind;
	if (adorn)
		*adorn = (key->flag & XS_DONT_ENUM_FLAG) ? 0 : 1;
	if (kind == XS_KEY_KIND)
		mxPushString(key->value.key.string);
	else if (kind == XS_KEY_X_KIND)
		mxPushStringX(key->value.key.string);
	else {
		mxCheck(the, key->kind == XS_REFERENCE_KIND);
		key = key->value.reference->next->next;
		if (key->kind == XS_UNDEFINED_KIND) {
			*adorn = 0;
			mxPush(mxEmptyString);
		}
		else
			mxPushSlot(key);
	}
}

txID fxFindName(txMachine* the, txString theString)
{
	txU1* aString;
	txU4 aSum;
	txU4 aModulo;
	txSlot* result;
	
	aString = (txU1*)theString;
	aSum = 0;
	while (*aString != 0) {
		aSum = (aSum << 1) + *aString++;
	}
	aSum &= 0x7FFFFFFF;
	aModulo = aSum % the->nameModulo;
	result = the->nameTable[aModulo];
	while (result != C_NULL) {
		if (result->value.key.sum == aSum)
			if (c_strcmp(result->value.key.string, theString) == 0)
				return mxGetKeySlotID(result);
		result = result->next;
	}
	return 0;
}

txBoolean fxIsKeyName(txMachine* the, txID theID)
{
	txSlot* key = fxGetKey(the, theID);
	return (key->flag & XS_DONT_ENUM_FLAG) ? 1 : 0;
}

txBoolean fxIsKeySymbol(txMachine* the, txID theID)
{
	txSlot* key = fxGetKey(the, theID);
	return (key->flag & XS_DONT_ENUM_FLAG) ? 0 : 1;
}

txID fxNewName(txMachine* the, txSlot* theSlot)
{
	txU1* string;
	txU4 sum;
	txU4 modulo;
	txSlot* result;

	string = (txU1*)theSlot->value.string;
	sum = 0;
	while(*string != 0) {
		sum = (sum << 1) + *string++;
	}
	sum &= 0x7FFFFFFF;
	modulo = sum % the->nameModulo;
	result = the->nameTable[modulo];
	while (result != C_NULL) {
		if (result->value.key.sum == sum) {
			if (c_strcmp(result->value.key.string, theSlot->value.string) == 0)
				return mxGetKeySlotID(result);
		}
		result = result->next;
	}
	result = fxFindKey(the);
	result->next = the->nameTable[modulo];
	result->flag = XS_INTERNAL_FLAG | XS_DONT_ENUM_FLAG;
	result->kind = XS_KEY_KIND;
	result->value.key.string = theSlot->value.string;
	result->value.key.sum = sum;
	the->nameTable[modulo] = result;
	return result->ID;
}

txID fxNewNameC(txMachine* the, txString theString)
{
	txU1* string;
	txU4 sum;
	txU4 modulo;
	txSlot* result;

	string = (txU1*)theString;
	sum = 0;
	while(c_read8(string) != 0) {
		sum = (sum << 1) + c_read8(string++);
	}
	sum &= 0x7FFFFFFF;
	modulo = sum % the->nameModulo;
	result = the->nameTable[modulo];
	while (result != C_NULL) {
		if (result->value.key.sum == sum) {
			if (c_strcmp(result->value.key.string, theString) == 0) {
				txID id = mxGetKeySlotID(result);
				if (id >= the->keyOffset)
					result->flag |= XS_DONT_DELETE_FLAG;
				return id;
			}
		}
		result = result->next;
	}
	result = fxFindKey(the);
	result->next = the->nameTable[modulo];
	result->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	result->kind = XS_KEY_KIND;
	result->value.key.string = C_NULL;
	result->value.key.sum = sum;
	the->nameTable[modulo] = result;
	result->value.key.string = (txString)fxNewChunk(the, mxStringLength(theString) + 1);
	c_strcpy(result->value.key.string, theString);
	return result->ID;
}

txID fxNewNameX(txMachine* the, txString theString)
{
	txU1* string;
	txU4 sum;
	txU4 modulo;
	txSlot* result;
	
	string = (txU1*)theString;
	sum = 0;
	while (c_read8(string) != 0) {
		sum = (sum << 1) + c_read8(string++);
	}
	sum &= 0x7FFFFFFF;
	modulo = sum % the->nameModulo;
	result = the->nameTable[modulo];
	while (result != C_NULL) {
		if (result->value.key.sum == sum) {
			if (c_strcmp(result->value.key.string, theString) == 0) {
				txID id = mxGetKeySlotID(result);
				if (id >= the->keyOffset)
					result->flag |= XS_DONT_DELETE_FLAG;
				return id;
			}
		}
		result = result->next;
	}
	result = fxFindKey(the);
	result->next = the->nameTable[modulo];
	result->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	result->kind = XS_KEY_X_KIND;
	result->value.key.string = theString;
	result->value.key.sum = sum;
	the->nameTable[modulo] = result;
	return result->ID;
}

txSlot* fxAt(txMachine* the, txSlot* slot)
{
	txIndex index;
	txString string;
again:
	if ((slot->kind == XS_INTEGER_KIND) && fxIntegerToIndex(the, slot->value.integer, &index)) {
		slot->value.at.id = XS_NO_ID;
		slot->value.at.index = index;
	}
	else if ((slot->kind == XS_NUMBER_KIND) && fxNumberToIndex(the, slot->value.number, &index)) {
		slot->value.at.id = XS_NO_ID;
		slot->value.at.index = index;
	}
	else if (slot->kind == XS_SYMBOL_KIND) {
		slot->value.at.id = slot->value.symbol;
		slot->value.at.index = 0;
	}
    else {
        if (slot->kind == XS_REFERENCE_KIND) {
            fxToPrimitive(the, slot, XS_STRING_HINT);
            goto again;
        }
        string = fxToString(the, slot);
        if (fxStringToIndex(the, string, &index)) {
            slot->value.at.id = XS_NO_ID;
            slot->value.at.index = index;
        }
        else {
			txID id;
            if (slot->kind == XS_STRING_X_KIND)
                id = fxNewNameX(the, string);
            else
                id = fxNewName(the, slot);
            slot->value.at.id = id;
            slot->value.at.index = 0;
        }
    }
	slot->kind = XS_AT_KIND;
	return slot;
}

void fxKeyAt(txMachine* the, txID id, txIndex index, txSlot* slot)
{
	if (id) {
		txSlot* key = fxGetKey(the, id);
		if (key->flag & XS_DONT_ENUM_FLAG) {
			if (key->kind == XS_KEY_KIND) {
				slot->kind = XS_STRING_KIND;
				slot->value.string = key->value.key.string;
			}
			else{
				slot->kind = XS_STRING_X_KIND;
				slot->value.string = key->value.key.string;
			}
		}
		else {
			slot->kind = XS_SYMBOL_KIND;
			slot->value.symbol = id;
		}
	}
	else {
		char buffer[16];
		fxCopyStringC(the, slot, fxNumberToString(the, index, buffer, sizeof(buffer), 0, 0));
	}
}

void fxIDToString(txMachine* the, txID id, txString theBuffer, txSize theSize)
{
	if (id != XS_NO_ID) {
		txBoolean adorn;
		txString string = fxGetKeyString(the, id, &adorn);
		if (adorn)
			c_snprintf(theBuffer, theSize, "[%s]", string);
		else
			c_snprintf(theBuffer, theSize, "%s", string);
	}
	else {
		theBuffer[0] = '?';
		theBuffer[1] = 0;
	}
}

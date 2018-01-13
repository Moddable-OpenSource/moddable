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

static txSlot* fxNewGlobalInstance(txMachine* the);

static const char ICACHE_RODATA_ATTR gxURIEmptySet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char ICACHE_RODATA_ATTR gxURIReservedSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,0,0,1,1,0,1,0,0,0,0,1,1,0,0,1,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,1,1,0,1,0,1,	/* 3x  0123456789:;<=>?  */
	 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char ICACHE_RODATA_ATTR gxURIUnescapedSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,1,0,0,0,0,0,1,1,1,1,0,0,1,1,0,	/* 2x   !"#$%&'()*+,-./  */
	 1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char ICACHE_RODATA_ATTR gxURIReservedAndUnescapedSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,	/* 2x   !"#$%&'()*+,-./  */
	 1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,	/* 3x  0123456789:;<=>?  */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static txBoolean fxEnvironmentDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static txSlot* fxEnvironmentGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
static txSlot* fxEnvironmentSetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);

const txBehavior ICACHE_FLASH_ATTR gxEnvironmentBehavior = {
	fxEnvironmentGetProperty,
	fxEnvironmentSetProperty,
	fxOrdinaryCall,
	fxOrdinaryConstruct,
	fxOrdinaryDefineOwnProperty,
	fxEnvironmentDeleteProperty,
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

const txBehavior ICACHE_FLASH_ATTR gxGlobalBehavior = {
	fxGlobalGetProperty,
	fxGlobalSetProperty,
	fxOrdinaryCall,
	fxOrdinaryConstruct,
	fxOrdinaryDefineOwnProperty,
	fxGlobalDeleteProperty,
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
			
void fxBuildGlobal(txMachine* the)
{
	txSlot* slot;

	fxNewInstance(the);
	mxPull(mxObjectPrototype);
	
	mxPush(mxObjectPrototype);
	fxNewFunctionInstance(the, XS_NO_ID);
	mxPull(mxFunctionPrototype);
	
	fxNewGlobalInstance(the);
	mxPull(mxGlobal);
	fxNewHostFunctionGlobal(the, mxCallback(fx_isFinite), 1, mxID(_isFinite), XS_DONT_ENUM_FLAG);
	fxNewHostFunctionGlobal(the, mxCallback(fx_isNaN), 1, mxID(_isNaN), XS_DONT_ENUM_FLAG);
	fxNewHostFunctionGlobal(the, mxCallback(fx_parseFloat), 1, mxID(_parseFloat), XS_DONT_ENUM_FLAG);
	fxNewHostFunctionGlobal(the, mxCallback(fx_parseInt), 2, mxID(_parseInt), XS_DONT_ENUM_FLAG);
	fxNewHostFunctionGlobal(the, mxCallback(fx_trace), 1, mxID(_trace), XS_DONT_ENUM_FLAG);
	fxNewHostFunctionGlobal(the, mxCallback(fx_decodeURI), 1, mxID(_decodeURI), XS_DONT_ENUM_FLAG);
	fxNewHostFunctionGlobal(the, mxCallback(fx_decodeURIComponent), 1, mxID(_decodeURIComponent), XS_DONT_ENUM_FLAG);
	fxNewHostFunctionGlobal(the, mxCallback(fx_encodeURI), 1, mxID(_encodeURI), XS_DONT_ENUM_FLAG);
	fxNewHostFunctionGlobal(the, mxCallback(fx_encodeURIComponent), 1, mxID(_encodeURIComponent), XS_DONT_ENUM_FLAG);
	fxNewHostFunctionGlobal(the, mxCallback(fx_escape), 1, mxID(_escape), XS_DONT_ENUM_FLAG);
	fxNewHostFunctionGlobal(the, mxCallback(fx_unescape), 1, mxID(_unescape), XS_DONT_ENUM_FLAG);
	
	slot = fxNewHostFunctionGlobal(the, mxCallback(fx_eval), 1, mxID(_eval), XS_DONT_ENUM_FLAG);
	mxEvalFunction.value.reference = slot;
	mxEvalFunction.kind = XS_REFERENCE_KIND;

	slot = fxGlobalSetProperty(the, mxGlobal.value.reference, mxID(_global), XS_NO_ID, XS_OWN);
	slot->flag = XS_DONT_ENUM_FLAG;
	slot->kind = mxGlobal.kind;
	slot->value = mxGlobal.value;
	slot = fxGlobalSetProperty(the, mxGlobal.value.reference, mxID(_undefined), XS_NO_ID, XS_OWN);
	slot->flag = XS_GET_ONLY;
	
	mxPush(mxObjectPrototype);
	slot = fxNewObjectInstance(the);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_iterator), 0, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	mxPull(mxIteratorPrototype);
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Enumerator_next), 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	fxNewHostConstructor(the, mxCallback(fx_Enumerator), 0, XS_NO_ID);
	mxPull(mxEnumeratorFunction);
	
	fxNewHostFunction(the, mxCallback(fxThrowTypeError), 0, XS_NO_ID);
	mxThrowTypeErrorFunction = *the->stack;
	slot = the->stack->value.reference;
	slot->flag |= XS_DONT_PATCH_FLAG;
	slot = slot->next;
	while (slot) {
		slot->flag |= XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
		slot = slot->next;
	}
	the->stack++;
}

txSlot* fxNewEnvironmentInstance(txMachine* the, txSlot* environment)
{
	txSlot* with = the->stack;
	txSlot* instance = fxNewSlot(the);
	txSlot* slot;
	instance->flag = XS_EXOTIC_FLAG;
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = (environment->kind == XS_REFERENCE_KIND) ? environment->value.reference : C_NULL;
	mxPushReference(instance);
	slot = instance->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG;
	slot->ID = XS_ENVIRONMENT_BEHAVIOR;
	slot->kind = XS_WITH_KIND;
	slot->value.reference = C_NULL;
	if (with->kind == XS_REFERENCE_KIND) {
		if (with->value.reference == mxGlobal.value.reference) {
			if (environment->kind == XS_REFERENCE_KIND)
				mxClosures.value.reference = instance;
			else {
				slot->flag |= XS_DONT_DELETE_FLAG;
				slot->value.reference = with->value.reference;
			}
		}
		else {
			slot->flag |= XS_DONT_SET_FLAG;
			slot->value.reference = with->value.reference;
		}
	}
	else if (with->kind == XS_NULL_KIND) {
		slot->flag |= XS_DONT_DELETE_FLAG;
		slot->value.reference = fxNewInstance(the);
		mxPop();
	}
    mxPop();
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;
	return instance;
}

txSlot* fxNewGlobalInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
#ifdef mxFewGlobalsTable
	txSize length = XS_FEW_GLOBALS_COUNT;
#else
	txSize length = the->keyCount;
#endif
	instance = fxNewSlot(the);
	instance->flag = XS_EXOTIC_FLAG;
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = mxObjectPrototype.value.reference;
	mxPushReference(instance);
	property = instance->next = fxNewSlot(the);
	property->value.table.address = (txSlot**)fxNewChunk(the, length * sizeof(txSlot*));
	c_memset(property->value.table.address, 0, length * sizeof(txSlot*));
	property->value.table.length = length;
	property->flag = XS_INTERNAL_FLAG;
	property->ID = XS_GLOBAL_BEHAVIOR;
	property->kind = XS_GLOBAL_KIND;
	return instance;
}

#ifndef mxLink
txSlot* fxNewHostConstructorGlobal(txMachine* the, txCallback call, txInteger length, txID id, txFlag flag)
{
	txSlot *function, *property;
	function = fxNewHostConstructor(the, call, length, id);
	property = fxGlobalSetProperty(the, mxGlobal.value.reference, id, XS_NO_ID, XS_OWN);
	property->flag = flag;
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	return function;
}

txSlot* fxNewHostFunctionGlobal(txMachine* the, txCallback call, txInteger length, txID id, txFlag flag)
{
	txSlot *function, *property;
	function = fxNewHostFunction(the, call, length, id);
	property = fxGlobalSetProperty(the, mxGlobal.value.reference, id, XS_NO_ID, XS_OWN);
	property->flag = flag;
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	return function;
}
#endif

txSlot* fxCheckIteratorInstance(txMachine* the, txSlot* slot)
{
	txSlot* instance;
	if (slot->kind == XS_REFERENCE_KIND) {
		instance = slot->value.reference;
		slot = instance->next;
		if (slot && (slot->ID == mxID(_result))) {
			slot = slot->next;
			if (slot && (slot->ID == mxID(_iterable))) {
				slot = slot->next;
				if (slot && (slot->ID == mxID(_index))) {
					return instance;
				}
			}
		}
	}
	mxTypeError("this is no iterator");
	return C_NULL;
}

void fxCloseIterator(txMachine* the, txSlot* iterator)
{
 	mxPushInteger(0);
   	mxPushSlot(iterator);
	mxPushSlot(iterator);
	fxGetID(the, mxID(_return));
	if (the->stack->kind != XS_UNDEFINED_KIND)
		fxCall(the);
	else
		the->stack += 3;
}

txSlot* fxNewIteratorInstance(txMachine* the, txSlot* iterable) 
{
	txSlot* instance;
	txSlot* result;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	mxPush(mxObjectPrototype);
	result = fxNewObjectInstance(the);
	property = fxNextUndefinedProperty(the, result, mxID(_value), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	property = fxNextBooleanProperty(the, property, 0, mxID(_done), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	property = fxNextSlotProperty(the, instance, the->stack, mxID(_result), XS_INTERNAL_FLAG | XS_GET_ONLY);
	property = fxNextSlotProperty(the, property, iterable, mxID(_iterable), XS_INTERNAL_FLAG | XS_GET_ONLY);
	property = fxNextIntegerProperty(the, property, 0, mxID(_index), XS_INTERNAL_FLAG | XS_GET_ONLY);
    the->stack++;
	return instance;
}

void fx_Iterator_iterator(txMachine* the)
{
	*mxResult = *mxThis;
}

void fx_Enumerator(txMachine* the)
{
	txSlot* iterator;
	txSlot* result;
	txSlot* slot;
	txSlot* list;
	txSlot* item;
	txSlot* instance;
	txSlot** address;
	
	mxPush(mxEnumeratorFunction);
	fxGetID(the, mxID(_prototype));
	iterator = fxNewObjectInstance(the);
	mxPullSlot(mxResult);
	mxPush(mxObjectPrototype);
	result = fxNewObjectInstance(the);
	slot = fxNextUndefinedProperty(the, result, mxID(_value), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextBooleanProperty(the, slot, 0, mxID(_done), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextSlotProperty(the, iterator, the->stack, mxID(_result), XS_GET_ONLY);
	mxPop();
	list = slot->next = fxNewSlot(the);
	list->flag = XS_GET_ONLY;
	list->ID = mxID(_iterable);
	list->kind = mxThis->kind;
	list->value = mxThis->value;
	if (!mxIsReference(list))
		return;	
	instance = list->value.reference;
	item = list;
	mxPushUndefined();
	result = the->stack;
	for (;;) {
		txSlot* at = fxNewInstance(the);
		mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG, at);
		address = &at->next;
		while ((at = *address)) {
			if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, result)) {
				slot = list->next;
				while ((slot)) {
					if ((at->value.at.id == slot->value.at.id) && (at->value.at.index == slot->value.at.index))
						break;
					slot = slot->next;
				}
				if (slot)
					address = &(at->next);
				else {
					*address = at->next;
					item = item->next = at;
					item->next = C_NULL;
					item->flag = result->flag;
				}
			}
			else
				address = &(at->next);
		}
		mxPop();
		if (mxBehaviorGetPrototype(the, instance, result))
			instance = result->value.reference;
		else
			break;
	}
	mxPop();
	address = &list->next;
	while ((slot = *address)) {
		if (slot->flag & XS_DONT_ENUM_FLAG) {
			*address = slot->next;
		}
		else {
			txID id = slot->value.at.id;
			txIndex index = slot->value.at.index;
			fxKeyAt(the, id, index, slot);
			address = &(slot->next);
		}
	}
}

void fx_Enumerator_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	if (index) {
		value->kind = index->kind;
		value->value = index->value;
		iterable->next = index->next;
	}
	else {
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}

txBoolean fxEnvironmentDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	return 0;
}

txSlot* fxEnvironmentGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	if (id) {
		txSlot* result = instance->next->next;
		while (result) {
			if (result->ID == id)
				return result->value.closure;
			result = result->next;
		}
	}
	return C_NULL;
}

txSlot* fxEnvironmentSetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	if (id) {
		txSlot* result = instance->next->next;
		while (result) {
			if (result->ID == id)
				return result->value.closure;
			result = result->next;
		}
	}
	return C_NULL;
}

txBoolean fxGlobalDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index) 
{
	txBoolean result = fxOrdinaryDeleteProperty(the, instance, id, index);
	if (id && result) {
		txInteger i = id & 0x00007FFF;
		txSlot* globals = instance->next;
		if (i < globals->value.table.length)
			globals->value.table.address[i] = C_NULL;
	}
	return result;
}

txSlot* fxGlobalGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag) 
{
	if (id) {
		txInteger i = id & 0x00007FFF;
		txSlot* globals = instance->next;
		if (i < globals->value.table.length) {
			txSlot* result = globals->value.table.address[i];
			if (!result) {
				result = fxOrdinaryGetProperty(the, instance, id, index, flag);
				globals->value.table.address[i] = result;
			}
			return result;
		}
	}
	return fxOrdinaryGetProperty(the, instance, id, index, flag);
}

txSlot* fxGlobalSetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag) 
{
	if (id) {
		txInteger i = id & 0x00007FFF;
		txSlot* globals = instance->next;
		if (i < globals->value.table.length) {
			txSlot* result = globals->value.table.address[i];
			if (!result) {
				result = fxOrdinarySetProperty(the, instance, id, index, flag);
				globals->value.table.address[i] = result;
			}
			return result;
		}
	}
	return fxOrdinarySetProperty(the, instance, id, index, flag);
}

void fx_decodeURI(txMachine* the)
{
	if (mxArgc < 1)
		mxSyntaxError("no URI parameter");
	fxDecodeURI(the, (txString)gxURIReservedSet);
}

void fx_decodeURIComponent(txMachine* the)
{
	if (mxArgc < 1)
		mxSyntaxError("no URI Component parameter");
	fxDecodeURI(the, (txString)gxURIEmptySet);
}

void fx_encodeURI(txMachine* the)
{
	if (mxArgc < 1)
		mxSyntaxError("no URI parameter");
	fxEncodeURI(the, (txString)gxURIReservedAndUnescapedSet);
}

void fx_encodeURIComponent(txMachine* the)
{
	if (mxArgc < 1)
		mxSyntaxError("no URI Component parameter");
	fxEncodeURI(the, (txString)gxURIUnescapedSet);
}

void fx_escape(txMachine* the)
{
	static const char ICACHE_RODATA_ATTR gxSet[128] = {
	  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
		 0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,1,	/* 2x   !"#$%&'()*+,-./  */
		 1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
		 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
		 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
		 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0 	/* 7X  pqrstuvwxyz{|}~   */
	};
	txString src;
	txInteger length;
	txInteger c;
	txString dst;
	
	if (mxArgc < 1) {
		mxResult->value.string = mxUndefinedString.value.string;
		mxResult->kind = mxUndefinedString.kind;
		return;
	}
	src = fxToString(the, mxArgv(0));
	length = 0;
	while (((src = fxUTF8Decode(src, &c))) && (c != C_EOF)) {
		if ((c < 128) && c_read8(gxSet + (int)c))
			length += 1;
		else if (c < 256)
			length += 3;
		else if (c < 0x10000)
			length += 6;
		else
			length += 12;
	}
	if (length == (src - mxArgv(0)->value.string)) {
		mxResult->value.string = mxArgv(0)->value.string;
		mxResult->kind = mxArgv(0)->kind;
		return;
	}
	mxResult->value.string = fxNewChunk(the, length + 1);
	mxResult->kind = XS_STRING_KIND;
	src = mxArgv(0)->value.string;
	dst = mxResult->value.string;
	while (((src = fxUTF8Decode(src, &c))) && (c != C_EOF)) {
		if ((c < 128) && c_read8(gxSet + (int)c))
			*dst++ = (char)c;
		else if (c < 256) {
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, c);
		}
		else {
			*dst++ = '%'; 
			*dst++ = 'u'; 
			dst = fxStringifyUnicodeEscape(dst, c, '%');
		}
	}
	*dst = 0;
}

void fx_eval(txMachine* the)
{
#ifdef mxParse
	txStringStream aStream;

	if (mxArgc < 1)
		return;
	if (!mxIsStringPrimitive(mxArgv(0))) {
		*mxResult = *mxArgv(0);
		return;
	}
	aStream.slot = mxArgv(0);
	aStream.offset = 0;
	aStream.size = c_strlen(fxToString(the, mxArgv(0)));
	fxRunScript(the, fxParseScript(the, &aStream, fxStringGetter, mxProgramFlag | mxEvalFlag), &mxGlobal, C_NULL, mxClosures.value.reference, C_NULL, C_NULL);
	mxPullSlot(mxResult);
#else
	mxUnknownError("not built-in");
#endif
}

void fx_trace(txMachine* the)
{
//@@#ifdef mxDebug
	int i;
	for (i = 0; i < mxArgc; i++) {
		fxToString(the, mxArgv(i));
		fxReport(the, "%s", mxArgv(i)->value.string);
	}
//#endif
}

void fx_unescape(txMachine* the)
{
	txString src;
	txInteger length;
	char c;
	txInteger d;
	txString dst;

	if (mxArgc < 1) {
		mxResult->value.string = mxUndefinedString.value.string;
		mxResult->kind = mxUndefinedString.kind;
		return;
	}
	src = fxToString(the, mxArgv(0));
	length = 0;
	while ((c = c_read8(src++))) {
		if (c == '%') {
			c = c_read8(src++);
			if (c == 'u') {
				if (fxParseUnicodeEscape(&src, &d, 0, '%'))
					length += fxUTF8Length(d);
				else
					length += 2;
			}
			else {
				src--;
				if (fxParseHexEscape(&src, &d))
					length += fxUTF8Length(d);
				else
					length += 1;
			}
		}
		else
			length += 1;
	}		
	length += 1;
	if (length == (src - mxArgv(0)->value.string)) {
		mxResult->value.string = mxArgv(0)->value.string;
		mxResult->kind = mxArgv(0)->kind;
		return;
	}
	mxResult->value.string = fxNewChunk(the, length);
	mxResult->kind = XS_STRING_KIND;
	src = mxArgv(0)->value.string;
	dst = mxResult->value.string;
	while ((c = c_read8(src++))) {
		if (c == '%') {
			c = c_read8(src++);
			d = 0;
			if (c == 'u') {
				if (fxParseUnicodeEscape(&src, &d, 0, '%'))
					dst = fxUTF8Encode(dst, d);
				else {
					*dst++ = '%';
					*dst++ = 'u';
				}
			}
			else {
				src--;
				if (fxParseHexEscape(&src, &d))
					dst = fxUTF8Encode(dst, d);
				else
					*dst++ = '%';
			}
		}
		else
			*dst++ = (txU1)c;
	}
	*dst = 0;
}

void fxDecodeURI(txMachine* the, txString theSet)
{
	txString src;
	txInteger length;
	txInteger c, d;
	const txUTF8Sequence *sequence;
	txInteger size;
	txString dst;
	
	src = fxToString(the, mxArgv(0));
	length = 0;
	while ((c = c_read8(src++))) {
		if (c == '%') {
			if (!fxParseHexEscape(&src, &d))
				mxURIError("invalid URI");
			if ((d < 128) && c_read8(theSet + d))
				length += 3;
			else {
				for (sequence = gxUTF8Sequences; sequence->size; sequence++) {
					if ((d & sequence->cmask) == sequence->cval)
						break;
				}
				if (!sequence->size)
					mxURIError("invalid URI");
				size = sequence->size - 1;
				while (size > 0) {
					c = c_read8(src++);
					if (c != '%')
						mxURIError("invalid URI");
					if (!fxParseHexEscape(&src, &c))
						mxURIError("invalid URI");
					if ((c & 0xC0) != 0x80)
						mxURIError("invalid URI");
					d = (d << 6) | (c & 0x3F);
					size--;
				}
				d &= sequence->lmask;
				length += fxUTF8Length(d);
			}
		}
		else
			length += 1;
	}		
	length += 1;
	if (length == (src - mxArgv(0)->value.string)) {
		mxResult->value.string = mxArgv(0)->value.string;
		mxResult->kind = mxArgv(0)->kind;
		return;
	}
	mxResult->value.string = fxNewChunk(the, length);
	mxResult->kind = XS_STRING_KIND;
	src = mxArgv(0)->value.string;
	dst = mxResult->value.string;
	while ((c = c_read8(src++))) {
		if (c == '%') {
			fxParseHexEscape(&src, &d);
			if ((d < 128) && c_read8(theSet + d)) {
				*dst++ = c_read8(src - 3);
				*dst++ = c_read8(src - 2);
				*dst++ = c_read8(src - 1);
			}
			else {
				for (sequence = gxUTF8Sequences; sequence->size; sequence++) {
					if ((d & sequence->cmask) == sequence->cval)
						break;
				}
				size = sequence->size - 1;
				while (size > 0) {
					src++;
					fxParseHexEscape(&src, &c);
					d = (d << 6) | (c & 0x3F);
					size--;
				}
				d &= sequence->lmask;
				dst = fxUTF8Encode(dst, d);
			}
		}
		else
			*dst++ = c;
	}
	*dst = 0;
}

void fxEncodeURI(txMachine* the, txString theSet)
{
	txString src;
	txInteger length;
	txInteger c;
	txString dst;

	src = fxToString(the, mxArgv(0));
	length = 0;
	while (((src = fxUTF8Decode(src, &c))) && (c != C_EOF)) {
		if (c < 0x80) {
			if (c_read8(theSet + c))
				length += 1;
			else
				length += 3;
		}
		else if (c < 0x800)
			length += 6;
		else if ((0xD800 <= c) && (c <= 0xDFFF))
			mxURIError("invalid string");
		else if (c < 0x10000)
			length += 9;
		else
			length += 12;
	}
	length += 1;
	if (length == (src - mxArgv(0)->value.string)) {
		mxResult->value.string = mxArgv(0)->value.string;
		mxResult->kind = mxArgv(0)->kind;
		return;
	}
	mxResult->value.string = fxNewChunk(the, length);
	mxResult->kind = XS_STRING_KIND;
	src = mxArgv(0)->value.string;
	dst = mxResult->value.string;
	while (((src = fxUTF8Decode(src, &c))) && (c != C_EOF)) {
		if (c < 0x80) {
			if (c_read8(theSet + c))
				*dst++ = (char)c;
			else {
				*dst++ = '%';
				dst = fxStringifyHexEscape(dst, c);
			}
		}
		else if (c < 0x800) {
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0xC0 | (c >> 6));
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0x80 | (c & 0x3F));
		}
		else if (c < 0x10000) {
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0xE0 | (c >> 12));
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0x80 | ((c >> 6) & 0x3F));
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0x80 | (c & 0x3F));
		}
		else {
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0xF0 | (c >> 18));
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0x80 | ((c >> 12) & 0x3F));
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0x80 | ((c >> 6) & 0x3F));
			*dst++ = '%';
			dst = fxStringifyHexEscape(dst, 0x80 | (c & 0x3F));
		}
	}
	*dst = 0;
}




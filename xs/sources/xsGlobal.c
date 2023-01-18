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

static void fx_trace_aux(txMachine* the, txInteger flags);

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
	
	fxBuildHostFunction(the, mxCallback(fx_isFinite), 1, mxID(_isFinite));
	mxPull(mxIsFiniteFunction);
	fxBuildHostFunction(the, mxCallback(fx_isNaN), 1, mxID(_isNaN));
	mxPull(mxIsNaNFunction);
	fxBuildHostFunction(the, mxCallback(fx_parseFloat), 1, mxID(_parseFloat));
	mxPull(mxParseFloatFunction);
	fxBuildHostFunction(the, mxCallback(fx_parseInt), 2, mxID(_parseInt));
	mxPull(mxParseIntFunction);
	fxBuildHostFunction(the, mxCallback(fx_decodeURI), 1, mxID(_decodeURI));
	mxPull(mxDecodeURIFunction);
	fxBuildHostFunction(the, mxCallback(fx_decodeURIComponent), 1, mxID(_decodeURIComponent));
	mxPull(mxDecodeURIComponentFunction);
	fxBuildHostFunction(the, mxCallback(fx_encodeURI), 1, mxID(_encodeURI));
	mxPull(mxEncodeURIFunction);
	fxBuildHostFunction(the, mxCallback(fx_encodeURIComponent), 1, mxID(_encodeURIComponent));
	mxPull(mxEncodeURIComponentFunction);
	fxBuildHostFunction(the, mxCallback(fx_escape), 1, mxID(_escape));
	mxPull(mxEscapeFunction);
	fxBuildHostFunction(the, mxCallback(fx_eval), 1, mxID(_eval));
	mxPull(mxEvalFunction);
	fxBuildHostFunction(the, mxCallback(fx_unescape), 1, mxID(_unescape));
	mxPull(mxUnescapeFunction);
	
	slot = fxBuildHostFunction(the, mxCallback(fx_trace), 1, mxID(_trace));
	slot = fxLastProperty(the, slot);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_trace_center), 1, mxID(_center), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_trace_left), 1, mxID(_left), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_trace_right), 1, mxID(_right), XS_DONT_ENUM_FLAG);
    mxPull(mxTraceFunction);

	mxPush(mxObjectPrototype);
	slot = fxNewObjectInstance(the);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Iterator_iterator), 0, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	mxPull(mxIteratorPrototype);
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Enumerator_next), 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	fxNewHostConstructor(the, mxCallback(fx_Enumerator), 0, XS_NO_ID);
	mxPull(mxEnumeratorFunction);
	
	fxNewHostFunction(the, mxCallback(fxThrowTypeError), 0, XS_NO_ID, XS_NO_ID);
	mxThrowTypeErrorFunction = *the->stack;
	slot = the->stack->value.reference;
	slot->flag |= XS_DONT_PATCH_FLAG;
	slot = slot->next;
	while (slot) {
		slot->flag |= XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
		slot = slot->next;
	}
	mxPop();
}

txSlot* fxNewGlobalInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	txSize size = fxMultiplyChunkSizes(the, XS_INTRINSICS_COUNT, sizeof(txSlot*));
	instance = fxNewSlot(the);
	instance->flag = XS_EXOTIC_FLAG;
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = the->stack->value.reference;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;
	property = instance->next = fxNewSlot(the);
	property->value.table.address = (txSlot**)fxNewChunk(the, size);
	c_memset(property->value.table.address, 0, size);
	property->value.table.length = XS_INTRINSICS_COUNT;
	property->flag = XS_INTERNAL_FLAG;
	property->ID = XS_GLOBAL_BEHAVIOR;
	property->kind = XS_GLOBAL_KIND;
	return instance;
}

txSlot* fxCheckIteratorInstance(txMachine* the, txSlot* slot, txID id)
{
	txSlot* instance;
	if (slot->kind == XS_REFERENCE_KIND) {
		instance = slot->value.reference;
		slot = instance->next;
		if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->ID == id) && (slot->kind == XS_REFERENCE_KIND)) {
			return instance;
		}
	}
	mxTypeError("this is no iterator");
	return C_NULL;
}

txSlot* fxCheckIteratorResult(txMachine* the, txSlot* result) 
{
	txSlot* value = result->value.reference->next;
	while (value && (value->flag & XS_INTERNAL_FLAG))
		value = value->next;
	mxCheck(the, (value != C_NULL) && (value->ID == mxID(_value)));
	mxCheck(the, (value->next != C_NULL) && (value->next->ID == mxID(_done)));
	return value;
}

txBoolean fxIteratorNext(txMachine* the, txSlot* iterator, txSlot* next, txSlot* value)
{
	mxPushSlot(iterator);
	mxPushSlot(next);
	mxCall();
	mxRunCount(0);
	if (!mxIsReference(the->stack))
		mxTypeError("iterator result is no object");
	mxDub();
	mxGetID(mxID(_done));
	if (fxToBoolean(the, the->stack)) {
		mxPop();
		mxPop();
		return 0;
	}
	mxPop();
	mxGetID(mxID(_value));
	mxPullSlot(value);
	return 1;
}

void fxIteratorReturn(txMachine* the, txSlot* iterator)
{
	mxPush(mxException);
	mxTry(the) {
		mxPushSlot(iterator);
		mxDub();
		mxGetID(mxID(_return));
		if (mxIsUndefined(the->stack)) 
			mxPop();
		else {
			mxCall();
			mxRunCount(0);
		}
		mxPop();
	}
	mxCatch(the) {
	}
	mxPull(mxException);
}

txBoolean fxGetIterator(txMachine* the, txSlot* iterable, txSlot* iterator, txSlot* next, txBoolean optional)
{
	mxPushSlot(iterable);
	mxDub();
	mxGetID(mxID(_Symbol_iterator));
	if (optional && (mxIsUndefined(the->stack) || mxIsNull(the->stack))) {
		mxPop();
		mxPop();
		return 0;
	}
	mxCall();
	mxRunCount(0);
	if (!mxIsReference(the->stack))
		mxTypeError("iterator is no object");
	if (next) {
		mxDub();
		mxGetID(mxID(_next));
		mxPullSlot(next);
	}
	mxPullSlot(iterator);
	return 1;
}

txSlot* fxNewIteratorInstance(txMachine* the, txSlot* iterable, txID id) 
{
	txSlot* instance;
	txSlot* result;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	mxPush(mxObjectPrototype);
	result = fxNewObjectInstance(the);
	property = fxNextUndefinedProperty(the, result, mxID(_value), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	property = fxNextBooleanProperty(the, property, 0, mxID(_done), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	property = fxNextSlotProperty(the, instance, the->stack, id, XS_INTERNAL_FLAG);
	property = fxNextSlotProperty(the, property, iterable, XS_NO_ID, XS_INTERNAL_FLAG);
	property = fxNextIntegerProperty(the, property, 0, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
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
	txSlot* keys;
	txSlot* visited;
	
	mxPush(mxEnumeratorFunction);
	mxGetID(mxID(_prototype));
	iterator = fxNewObjectInstance(the);
	mxPullSlot(mxResult);
	mxPush(mxObjectPrototype);
	result = fxNewObjectInstance(the);
	slot = fxNextUndefinedProperty(the, result, mxID(_value), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextBooleanProperty(the, slot, 0, mxID(_done), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextSlotProperty(the, iterator, the->stack, mxID(_result), XS_GET_ONLY);
	mxPop();
	
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_GET_ONLY;
	slot->ID = mxID(_iterable);
	slot->kind = mxThis->kind;
	slot->value = mxThis->value;
	if (mxIsUndefined(slot) || mxIsNull(slot))
		return;
	fxToInstance(the, slot);
		
	keys = fxNewInstance(the);
	mxBehaviorOwnKeys(the, slot->value.reference, XS_EACH_NAME_FLAG, keys);
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_GET_ONLY;
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = keys;
	mxPop();
	
	visited = fxNewInstance(the);
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_GET_ONLY;
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = visited;
	mxPop();
}

void fx_Enumerator_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* at = C_NULL;
	if (mxIsReference(iterable)) {
		txSlot* instance = iterable->value.reference;
		txSlot* keys = iterable->next;
		txSlot* visited = keys->next;
		txSlot* slot;
		txSlot* former;
	
		keys = keys->value.reference;
		visited = visited->value.reference;
	
		mxPushUndefined();
		slot = the->stack;
	again:	
		while ((at = keys->next)) {
			if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, slot)) {
				txSlot** address = &(visited->next);
				while ((former = *address)) {
					if ((at->value.at.id == former->value.at.id) && (at->value.at.index == former->value.at.index))
						break;
					address = &(former->next);
				}
				if (!former) {
					*address = at;
					keys->next = at->next;
					at->next = NULL;
					if (!(slot->flag & XS_DONT_ENUM_FLAG)) {
						fxKeyAt(the, at->value.at.id, at->value.at.index, result->value.reference->next);
						break;
					}
				}
				else
					keys->next = at->next;
			}
			else
				keys->next = at->next;
		}
		if (!at) {
			if (mxBehaviorGetPrototype(the, instance, slot)) {
				iterable->value.reference = instance = slot->value.reference;
				mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG, keys);
				goto again;
			}
		}
		mxPop();
	}
	if (!at) {
		result->value.reference->next->kind = XS_UNDEFINED_KIND;
		result->value.reference->next->next->value.boolean = 1;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}

txBoolean fxGlobalDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index) 
{
	txBoolean result = fxOrdinaryDeleteProperty(the, instance, id, index);
	if ((XS_SYMBOL_ID_COUNT <= id) && (id < XS_INTRINSICS_COUNT) && result) {
		txSlot* globals = instance->next;
		globals->value.table.address[id] = C_NULL;
	}
	return result;
}

txSlot* fxGlobalGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag) 
{
	txSlot* result = C_NULL;
	if ((XS_SYMBOL_ID_COUNT <= id) && (id < XS_INTRINSICS_COUNT)) {
		txSlot* globals = instance->next;
		result = globals->value.table.address[id];
		if (!result) {
			result = fxOrdinaryGetProperty(the, instance, id, index, flag);
			globals->value.table.address[id] = result;
		}
	}
	if (!result)
		result = fxOrdinaryGetProperty(the, instance, id, index, flag);
	return result;
}

txSlot* fxGlobalSetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag) 
{
	txSlot* result = C_NULL;
	if ((XS_SYMBOL_ID_COUNT <= id) && (id < XS_INTRINSICS_COUNT)) {
		txSlot* globals = instance->next;
		result = globals->value.table.address[id];
		if (!result) {
			result = fxOrdinarySetProperty(the, instance, id, index, flag);
			globals->value.table.address[id] = result;
		}
	}
	if (!result)
		result = fxOrdinarySetProperty(the, instance, id, index, flag);
	return result;
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
	while (((src = mxStringByteDecode(src, &c))) && (c != C_EOF)) {
		if ((c < 128) && c_read8(gxSet + (int)c))
			length = fxAddChunkSizes(the, length, 1);
		else if (c < 256)
			length = fxAddChunkSizes(the, length, 3);
		else if (c < 0x10000)
			length = fxAddChunkSizes(the, length, 6);
		else
			length = fxAddChunkSizes(the, length, 12);
	}
	if (length == (src - mxArgv(0)->value.string)) {
		mxResult->value.string = mxArgv(0)->value.string;
		mxResult->kind = mxArgv(0)->kind;
		return;
	}
	mxResult->value.string = fxNewChunk(the, fxAddChunkSizes(the, length, 1));
	mxResult->kind = XS_STRING_KIND;
	src = mxArgv(0)->value.string;
	dst = mxResult->value.string;
	while (((src = mxStringByteDecode(src, &c))) && (c != C_EOF)) {
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
	txStringStream aStream;
	txSlot* realm;
	txSlot* module = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.module;
	if (!module) module = mxProgram.value.reference;
	
	realm = mxModuleInstanceInternal(module)->value.module.realm;
	if (mxArgc < 1)
		return;
	if (!mxIsStringPrimitive(mxArgv(0))) {
		*mxResult = *mxArgv(0);
		return;
	}
	aStream.slot = mxArgv(0);
	aStream.offset = 0;
	aStream.size = mxStringLength(fxToString(the, mxArgv(0)));
	fxRunScript(the, fxParseScript(the, &aStream, fxStringGetter, mxProgramFlag | mxEvalFlag), mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, module);
	mxPullSlot(mxResult);
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

void fx_trace_aux(txMachine* the, txInteger flags)
{
	txSlot* conversation = C_NULL;
	void* message = C_NULL;
	txInteger length = 0;
	if (mxArgc > 1) {
		conversation = mxArgv(1);
		fxToString(the, conversation);
	}
	if ((mxArgc > 0) && (mxArgv(0)->kind == XS_REFERENCE_KIND)) {
		txSlot* slot = mxArgv(0)->value.reference->next;
		if (slot && (slot->kind == XS_ARRAY_BUFFER_KIND)) {
			txSlot* bufferInfo = slot->next;
			length = bufferInfo->value.bufferInfo.length;
			message = slot->value.arrayBuffer.address;
			flags |= XS_BUBBLE_BINARY;
		}
		else if (slot && (slot->kind == XS_HOST_KIND)) {
			txSlot* bufferInfo = slot->next;
			if (bufferInfo && (bufferInfo->kind == XS_BUFFER_INFO_KIND)) {
				length = bufferInfo->value.bufferInfo.length;
				message = slot->value.host.data;
				flags |= XS_BUBBLE_BINARY;
			}
		}
	}
	if (!message)
		message = fxToString(the, mxArgv(0));
	fxBubble(the, flags, message, length, (conversation) ? conversation->value.string : C_NULL);
}

void fx_trace_center(txMachine* the)
{
	fx_trace_aux(the, XS_NO_FLAG);
}

void fx_trace_left(txMachine* the)
{
	fx_trace_aux(the, XS_BUBBLE_LEFT);
}

void fx_trace_right(txMachine* the)
{
	fx_trace_aux(the, XS_BUBBLE_RIGHT);
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
					length += mxStringByteLength(d);
				else
					length += 2;
			}
			else {
				src--;
				if (fxParseHexEscape(&src, &d))
					length += mxStringByteLength(d);
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
					dst = mxStringByteEncode(dst, d);
				else {
					*dst++ = '%';
					*dst++ = 'u';
				}
			}
			else {
				src--;
				if (fxParseHexEscape(&src, &d))
					dst = mxStringByteEncode(dst, d);
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
				length += mxStringByteLength(d);
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
				dst = mxStringByteEncode(dst, d);
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
	while (((src = mxStringByteDecode(src, &c))) && (c != C_EOF)) {
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
	while (((src = mxStringByteDecode(src, &c))) && (c != C_EOF)) {
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

/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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

static txSlot* fxArgToInstance(txMachine* the, txInteger i);
static txBoolean fxCheckLength(txMachine* the, txSlot* slot, txInteger* index);

static txSlot* fxCheckArrayBufferDetached(txMachine* the, txSlot* slot);
static txSlot* fxCheckArrayBufferInstance(txMachine* the, txSlot* slot);
static txSlot* fxCheckArrayBufferMutable(txMachine* the, txSlot* slot);
static txSlot* fxNewArrayBufferInstance(txMachine* the);

static txSlot* fxCheckDataViewInstance(txMachine* the, txSlot* slot, txBoolean mutable);
static txInteger fxCheckDataViewSize(txMachine* the, txSlot* view, txSlot* buffer, txBoolean mutable);
static txSlot* fxNewDataViewInstance(txMachine* the);

static void fxCallTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index, txSlot* item);
static txSlot* fxCheckTypedArrayInstance(txMachine* the, txSlot* slot);
static txSlot* fxConstructTypedArray(txMachine* the);
static txSlot* fxNewTypedArrayInstance(txMachine* the, txTypeDispatch* dispatch, txTypeAtomics* atomics);
static void fxReduceTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index);

static txBoolean fxTypedArrayDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask);
static txBoolean fxTypedArrayDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static txBoolean fxTypedArrayGetOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot);
static txSlot* fxTypedArrayGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
static txBoolean fxTypedArrayGetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver);
static txBoolean fxTypedArrayHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static void fxTypedArrayOwnKeys(txMachine* the, txSlot* instance, txFlag flag, txSlot* keys);
static txBoolean fxTypedArrayPreventExtensions(txMachine* the, txSlot* instance);
static txSlot* fxTypedArraySetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
static txBoolean fxTypedArraySetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver);

static void fx_TypedArray_from_object(txMachine* the, txSlot* instance, txSlot* function, txSlot* _this);

const txBehavior ICACHE_FLASH_ATTR gxTypedArrayBehavior = {
	fxTypedArrayGetProperty,
	fxTypedArraySetProperty,
	fxOrdinaryCall,
	fxOrdinaryConstruct,
	fxTypedArrayDefineOwnProperty,
	fxTypedArrayDeleteProperty,
	fxTypedArrayGetOwnProperty,
	fxTypedArrayGetPropertyValue,
	fxOrdinaryGetPrototype,
	fxTypedArrayHasProperty,
	fxOrdinaryIsExtensible,
	fxTypedArrayOwnKeys,
	fxTypedArrayPreventExtensions,
	fxTypedArraySetPropertyValue,
	fxOrdinarySetPrototype,
};

void *fxArrayBuffer(txMachine* the, txSlot* slot, void* data, txInteger byteLength, txInteger maxByteLength)
{
	txSlot* instance;
	txSlot* arrayBuffer;
	txSlot* bufferInfo;
	if (byteLength < 0)
		mxRangeError("invalid byteLength %ld", byteLength);
	mxPush(mxArrayBufferPrototype);
	instance = fxNewArrayBufferInstance(the);
	arrayBuffer = instance->next;
	arrayBuffer->value.arrayBuffer.address = fxNewChunk(the, byteLength);
	bufferInfo = arrayBuffer->next;
	bufferInfo->value.bufferInfo.length = byteLength;
	bufferInfo->value.bufferInfo.maxLength = maxByteLength;
	if (data != NULL)
		c_memcpy(arrayBuffer->value.arrayBuffer.address, data, byteLength);
	else
		c_memset(arrayBuffer->value.arrayBuffer.address, 0, byteLength);
	mxPullSlot(slot);
	return arrayBuffer->value.arrayBuffer.address;
}

void fxGetArrayBufferData(txMachine* the, txSlot* slot, txInteger byteOffset, void* data, txInteger byteLength)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	txSlot* bufferInfo = arrayBuffer->next;
	txInteger length = bufferInfo->value.bufferInfo.length;
	if ((byteOffset < 0) || (length < byteOffset))
		mxRangeError("invalid byteOffset %ld", byteOffset);
	if ((byteLength < 0) || (length < (byteOffset + byteLength)))
		mxRangeError("invalid byteLength %ld", byteLength);
	c_memcpy(data, arrayBuffer->value.arrayBuffer.address + byteOffset, byteLength);
}

txInteger fxGetArrayBufferLength(txMachine* the, txSlot* slot)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	txSlot* bufferInfo = arrayBuffer->next;
	return bufferInfo->value.bufferInfo.length;
}

txInteger fxGetArrayBufferMaxLength(txMachine* the, txSlot* slot)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	txSlot* bufferInfo = arrayBuffer->next;
	return bufferInfo->value.bufferInfo.maxLength;
}

void fxSetArrayBufferData(txMachine* the, txSlot* slot, txInteger byteOffset, void* data, txInteger byteLength)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	txSlot* bufferInfo = arrayBuffer->next;
	txInteger length = bufferInfo->value.bufferInfo.length;
	if ((byteOffset < 0) || (length < byteOffset))
		mxRangeError("invalid byteOffset %ld", byteOffset);
	if ((byteLength < 0) || (length < (byteOffset + byteLength)))
		mxRangeError("invalid byteLength %ld", byteLength);
	c_memcpy(arrayBuffer->value.arrayBuffer.address + byteOffset, data, byteLength);
}

void fxSetArrayBufferLength(txMachine* the, txSlot* slot, txInteger target)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	txSlot* bufferInfo = arrayBuffer->next;
	txInteger length = bufferInfo->value.bufferInfo.length;
	txByte* address = arrayBuffer->value.arrayBuffer.address;
	if (bufferInfo->value.bufferInfo.maxLength < 0)
		fxReport(the, "# Use xsArrayBufferResizable instead of xsArrayBuffer\n");
	if (length != target) {
		if (address)
			address = (txByte*)fxRenewChunk(the, address, target);
		if (address) {
			if (length < target)
				c_memset(address + length, 0, target - length);
		}
		else {
			address = (txByte*)fxNewChunk(the, target);
			if (length < target) {
				c_memcpy(address, arrayBuffer->value.arrayBuffer.address, length);
				c_memset(address + length, 0, target - length);
			}
			else
				c_memcpy(address, arrayBuffer->value.arrayBuffer.address, target);
		}
		arrayBuffer->value.arrayBuffer.address = address;
		bufferInfo->value.bufferInfo.length = target;
	}
}

void* fxToArrayBuffer(txMachine* the, txSlot* slot)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	return arrayBuffer->value.arrayBuffer.address;
}

void fxBuildDataView(txMachine* the)
{
	txSlot* instance;
    txSlot* slot;
	txInteger index;
	const txTypeDispatch *dispatch;
	const txTypeAtomics *atomics;
	txSlot* property;
    txSlot* constructor;
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_get_byteLength), C_NULL, mxID(_byteLength), XS_DONT_ENUM_FLAG);
#if mxECMAScript2023
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_get_detached), C_NULL, mxID(_detached), XS_DONT_ENUM_FLAG);
#endif
#if mxImmutableArrayBuffers
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_get_immutable), C_NULL, mxID(_immutable), XS_DONT_ENUM_FLAG);
#endif
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_get_maxByteLength), C_NULL, mxID(_maxByteLength), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_get_resizable), C_NULL, mxID(_resizable), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_concat), 1, mxID(_concat), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_resize), 1, mxID(_resize), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_slice), 2, mxID(_slice), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_transfer), 0, mxID(_transfer), XS_DONT_ENUM_FLAG);
#if mxECMAScript2024
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_transferToFixedLength), 0, mxID(_transferToFixedLength), XS_DONT_ENUM_FLAG);
#endif
#if mxImmutableArrayBuffers
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_prototype_transferToImmutable), 0, mxID(_transferToImmutable), XS_DONT_ENUM_FLAG);
#endif
	slot = fxNextStringXProperty(the, slot, "ArrayBuffer", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxArrayBufferPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_ArrayBuffer), 1, mxID(_ArrayBuffer));
	mxArrayBufferConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_fromBigInt), 1, mxID(_fromBigInt), XS_DONT_ENUM_FLAG);
#ifndef mxCESU8
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_fromString), 1, mxID(_fromString), XS_DONT_ENUM_FLAG);
#endif
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_ArrayBuffer_isView), 1, mxID(_isView), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_species_get), C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	mxPop();
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getBigInt64), 1, mxID(_getBigInt64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setBigInt64), 2, mxID(_setBigInt64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getBigUint64), 1, mxID(_getBigUint64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setBigUint64), 2, mxID(_setBigUint64), XS_DONT_ENUM_FLAG);
#if mxFloat16
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getFloat16), 1, mxID(_getFloat16), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setFloat16), 2, mxID(_setFloat16), XS_DONT_ENUM_FLAG);
#endif
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getFloat32), 1, mxID(_getFloat32), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setFloat32), 2, mxID(_setFloat32), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getFloat64), 1, mxID(_getFloat64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setFloat64), 2, mxID(_setFloat64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getInt8), 1, mxID(_getInt8), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setInt8), 2, mxID(_setInt8), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getInt16), 1, mxID(_getInt16), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setInt16), 2, mxID(_setInt16), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getInt32), 1, mxID(_getInt32), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setInt32), 2, mxID(_setInt32), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getUint8), 1, mxID(_getUint8), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setUint8), 2, mxID(_setUint8), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getUint16), 1, mxID(_getUint16), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setUint16), 2, mxID(_setUint16), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_getUint32), 1, mxID(_getUint32), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_DataView_prototype_setUint32), 2, mxID(_setUint32), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_DataView_prototype_buffer_get), C_NULL, mxID(_buffer), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_DataView_prototype_byteLength_get), C_NULL, mxID(_byteLength), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_DataView_prototype_byteOffset_get), C_NULL, mxID(_byteOffset), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "DataView", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxDataViewPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_DataView), 1, mxID(_DataView));
	mxDataViewConstructor = *the->stack;
	mxPop();
	
	mxPush(mxObjectPrototype);
	instance = fxNewObjectInstance(the);
	
	fxNewHostFunction(the, mxCallback(fxTypedArrayGetter), 0, XS_NO_ID, XS_NO_ID);
	property = mxFunctionInstanceHome(the->stack->value.reference);
	property->value.home.object = instance;
	fxNewHostFunction(the, mxCallback(fxTypedArraySetter), 1, XS_NO_ID, XS_NO_ID);
	property = mxFunctionInstanceHome(the->stack->value.reference);
	property->value.home.object = instance;
	mxPushUndefined();
	the->stack->flag = XS_DONT_DELETE_FLAG;
	the->stack->kind = XS_ACCESSOR_KIND;
	the->stack->value.accessor.getter = (the->stack + 2)->value.reference;
	the->stack->value.accessor.setter = (the->stack + 1)->value.reference;
	mxPull(mxTypedArrayAccessor);
	mxPop();
	mxPop();
	
	slot = fxLastProperty(the, instance);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_at), 1, mxID(_at), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_TypedArray_prototype_buffer_get), C_NULL, mxID(_buffer), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_TypedArray_prototype_byteLength_get), C_NULL, mxID(_byteLength), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_TypedArray_prototype_byteOffset_get), C_NULL, mxID(_byteOffset), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_TypedArray_prototype_length_get), C_NULL, mxID(_length), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_TypedArray_prototype_toStringTag_get), C_NULL, mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_copyWithin), 2, mxID(_copyWithin), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_entries), 0, mxID(_entries), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_every), 1, mxID(_every), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_fill), 1, mxID(_fill), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_filter), 1, mxID(_filter), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_find), 1, mxID(_find), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_findIndex), 1, mxID(_findIndex), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_findLast), 1, mxID(_findLast), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_findLastIndex), 1, mxID(_findLastIndex), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_forEach), 1, mxID(_forEach), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_includes), 1, mxID(_includes), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_indexOf), 1, mxID(_indexOf), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_join), 1, mxID(_join), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_keys), 0, mxID(_keys), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_lastIndexOf), 1, mxID(_lastIndexOf), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_map), 1, mxID(_map), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_reduce), 1, mxID(_reduce), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_reduceRight), 1, mxID(_reduceRight), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_reverse), 0, mxID(_reverse), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_set), 1, mxID(_set), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_slice), 2, mxID(_slice), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_some), 1, mxID(_some), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_sort), 1, mxID(_sort), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_subarray), 2, mxID(_subarray), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_toLocaleString), 0, mxID(_toLocaleString), XS_DONT_ENUM_FLAG);
	property = mxBehaviorGetProperty(the, mxArrayPrototype.value.reference, mxID(_toString), 0, XS_OWN);
	slot = fxNextSlotProperty(the, slot, property, mxID(_toString), XS_DONT_ENUM_FLAG);
	property = slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_values), 0, mxID(_values), XS_DONT_ENUM_FLAG);
	slot = fxNextSlotProperty(the, slot, property, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
#if mxECMAScript2023
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_toReversed), 0, mxID(_toReversed), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_toSorted), 1, mxID(_toSorted), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_prototype_with), 2, mxID(_with), XS_DONT_ENUM_FLAG);
#endif
	mxTypedArrayPrototype = *the->stack;	
	constructor = fxBuildHostConstructor(the, mxCallback(fx_TypedArray), 0, mxID(_TypedArray));
	mxTypedArrayConstructor = *the->stack;
	slot = fxLastProperty(the, constructor);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_from), 1, mxID(_from), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_TypedArray_of), 0, mxID(_of), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_species_get), C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	for (index = 0, dispatch = &gxTypeDispatches[0], atomics = &gxTypeAtomics[0]; index < mxTypeArrayCount; index++, dispatch++, atomics++) {
		mxPush(mxTypedArrayPrototype);
		slot = fxLastProperty(the, fxNewObjectInstance(the));
		slot = fxNextIntegerProperty(the, slot, dispatch->size, mxID(_BYTES_PER_ELEMENT), XS_GET_ONLY);
		slot = fxBuildHostConstructor(the, mxCallback(fx_TypedArray), 3, mxID(dispatch->constructorID));
		the->stackIntrinsics[-1 - (txInteger)dispatch->constructorID] = *the->stack; //@@
		slot->value.instance.prototype = constructor;
		property = mxFunctionInstanceHome(slot);
		slot = property->next;
		property = fxNextTypeDispatchProperty(the, property, (txTypeDispatch*)dispatch, (txTypeAtomics*)atomics, XS_NO_ID, XS_INTERNAL_FLAG);
		property->next = slot;
		slot = fxLastProperty(the, slot);
		slot = fxNextIntegerProperty(the, slot, dispatch->size, mxID(_BYTES_PER_ELEMENT), XS_GET_ONLY);
		mxPop();
	}
	mxPop();
	
#if mxUint8ArrayBase64
	mxPush(mxUint8ArrayConstructor);
    instance = fxToInstance(the, the->stack);
 	slot = fxLastProperty(the, instance);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Uint8Array_fromBase64), 1, mxID(_fromBase64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Uint8Array_fromHex), 1, mxID(_fromHex), XS_DONT_ENUM_FLAG);
    mxGetID(mxID(_prototype));
    instance = fxToInstance(the, the->stack);
 	slot = fxLastProperty(the, instance);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Uint8Array_prototype_setFromBase64), 1, mxID(_setFromBase64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Uint8Array_prototype_setFromHex), 1, mxID(_setFromHex), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Uint8Array_prototype_toBase64), 0, mxID(_toBase64), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Uint8Array_prototype_toHex), 0, mxID(_toHex), XS_DONT_ENUM_FLAG);
	mxPop();
#endif
}

txInteger fxArgToByteLength(txMachine* the, txInteger argi, txInteger length)
{
	txSlot *arg = mxArgv(argi);
	if ((mxArgc > argi) && (arg->kind != XS_UNDEFINED_KIND)) {
		txNumber value;
		if (XS_INTEGER_KIND == arg->kind) {
			txInteger value = arg->value.integer;
			if (value < 0)
				mxRangeError("byteLength < 0");
			return value;
		}
		value = c_trunc(fxToNumber(the, arg));
		if (c_isnan(value))
			return 0;
		if (value < 0) 
			mxRangeError("byteLength < 0");
		if (0x7FFFFFFF < value)
			mxRangeError("byteLength too big");
		return (txInteger)value;
	}
	return length;
}

txS8 fxArgToSafeByteLength(txMachine* the, txInteger argi, txInteger length)
{
	txSlot *arg = mxArgv(argi);
	if ((mxArgc > argi) && (arg->kind != XS_UNDEFINED_KIND)) {
		txNumber value;
		if (XS_INTEGER_KIND == arg->kind) {
			txS8 value = arg->value.integer;
			if (value < 0)
				mxRangeError("byteLength < 0");
			return value;
		}
		value = c_trunc(fxToNumber(the, arg));
		if (c_isnan(value))
			return 0;
		if (value < 0) 
			mxRangeError("byteLength < 0");
		if (C_MAX_SAFE_INTEGER < value)
			mxRangeError("byteLength too big");
		return (txS8)value;
	}
	return length;
}

txSlot* fxArgToInstance(txMachine* the, txInteger i)
{
	if (mxArgc > i)
		return fxToInstance(the, mxArgv(i));
	mxTypeError("cannot coerce undefined to object");
	return C_NULL;
}

txBoolean fxCheckLength(txMachine* the, txSlot* slot, txInteger* index)
{
	txNumber number = fxToNumber(the, slot);
	txNumber check = c_trunc(number);
	if ((number == check) && (0 <= number) && (number <= 0x7FFFFFFF)) {
		*index = (txInteger)number;
		return 1 ;
	}
	return 0;
}

txSlot* fxCheckArrayBufferDetached(txMachine* the, txSlot* slot)
{
	slot = slot->value.reference->next;
	if (slot->value.arrayBuffer.address == C_NULL)
		mxTypeError("detached buffer");
	return slot;
}

txSlot* fxCheckArrayBufferInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		txSlot* arrayBuffer = instance->next;
		if (arrayBuffer && (arrayBuffer->flag & XS_INTERNAL_FLAG) && (arrayBuffer->kind == XS_ARRAY_BUFFER_KIND))
			return instance;
	}
	if (slot == mxThis)
		mxTypeError("this: not an ArrayBuffer instance");
	mxTypeError("not an ArrayBuffer instance");
	return C_NULL;
}

txSlot* fxCheckArrayBufferMutable(txMachine* the, txSlot* slot)
{
	slot = slot->value.reference->next;
	if (slot->flag & XS_DONT_SET_FLAG)
		mxTypeError("ArrayBuffer instance is read-only");
	return slot;
}

void fxConstructArrayBufferResult(txMachine* the, txSlot* constructor, txInteger length)
{
	txSlot* instance;
	if (constructor)
		mxPushSlot(constructor);
	else {
		mxPushSlot(mxThis);
		mxGetID(mxID(_constructor));
		fxToSpeciesConstructor(the, &mxArrayBufferConstructor);
	}
	mxNew();
	mxPushInteger(length);
	mxRunCount(1);
	if (the->stack->kind != XS_REFERENCE_KIND)
		mxTypeError("not an object");
	instance = the->stack->value.reference;
	if (!(instance->next) || (instance->next->kind != XS_ARRAY_BUFFER_KIND))
		mxTypeError("not an ArrayBuffer instance");
	if (!constructor && (mxThis->value.reference == instance))
		mxTypeError("same ArrayBuffer instance");
	if (instance->next->next->value.bufferInfo.length < length)
		mxTypeError("smaller ArrayBuffer instance");
	mxPullSlot(mxResult);
}

txSlot* fxNewArrayBufferInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_ARRAY_BUFFER_KIND;
	property->value.arrayBuffer.address = C_NULL;
	property->value.arrayBuffer.detachKey = C_NULL;
	property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_BUFFER_INFO_KIND;
	property->value.bufferInfo.length = 0;
	property->value.bufferInfo.maxLength = -1;
	return instance;
}

void fx_ArrayBuffer(txMachine* the)
{
	txSlot* instance;
	txS8 byteLength;
	txS8 maxByteLength = -1;
	txSlot* property;
	if (!mxHasTarget)
		mxTypeError("call: ArrayBuffer");
	byteLength = fxArgToSafeByteLength(the, 0, 0);
	if ((mxArgc > 1) && mxIsReference(mxArgv(1))) {
		mxPushSlot(mxArgv(1));
		mxGetID(mxID(_maxByteLength));
		mxPullSlot(mxArgv(1));
		maxByteLength = fxArgToSafeByteLength(the, 1, -1);
	}
	if (maxByteLength >= 0) {
		if (byteLength > maxByteLength)
			mxRangeError("byteLength > maxByteLength");
	}
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxArrayBufferPrototype);
	instance = fxNewArrayBufferInstance(the);
	mxPullSlot(mxResult);
	if (byteLength > 0x7FFFFFFF)
		mxRangeError("byteLength too big");
	if (maxByteLength > 0x7FFFFFFF)
		mxRangeError("maxByteLength too big");
	property = instance->next;
	property->value.arrayBuffer.address = fxNewChunk(the, (txSize)byteLength);
	c_memset(property->value.arrayBuffer.address, 0, (txSize)byteLength);
	property = property->next;
	property->value.bufferInfo.length = (txSize)byteLength;
	property->value.bufferInfo.maxLength = (txSize)maxByteLength;
}

void fx_ArrayBuffer_fromBigInt(txMachine* the)
{
	txU4 minBytes = 0;
	txBoolean sign = 0;
	int endian = EndianBig;
	if (mxArgc < 1)
		mxTypeError("no argument");
	if (mxArgc > 1)
		minBytes = (txU4)fxArgToByteLength(the, 1, 0);
	if ((mxArgc > 2) && fxToBoolean(the, mxArgv(2)))
		sign = 1;
	if ((mxArgc > 3) && fxToBoolean(the, mxArgv(3)))
		endian = EndianLittle;
	if (gxTypeBigInt.toArrayBuffer) {
		gxTypeBigInt.toArrayBuffer(the, mxArgv(0), minBytes, sign, endian);
	}
	else {
		mxUnknownError("not built-in");
	}
}

#ifndef mxCESU8
void fx_ArrayBuffer_fromString(txMachine* the)
{
	txSize length = 0;
	if (mxArgc < 1)
		mxTypeError("no argument");
	
	txString c = fxToString(the, mxArgv(0));
	txInteger nulls = 0;
	while (1) {
		uint8_t b = (uint8_t)c_read8(c++);
		if (!b) break;

		length += 1;
		if ((0xc0 == b) && (0x80 == (uint8_t)c_read8(c)))
			nulls += 1;
	} 
	
	fxConstructArrayBufferResult(the, mxThis, length - nulls);
	if (!nulls)
		c_memcpy(mxResult->value.reference->next->value.arrayBuffer.address, mxArgv(0)->value.string, length);
	else {
		txString c = mxArgv(0)->value.string, end = c + length;
		txByte *out = mxResult->value.reference->next->value.arrayBuffer.address;
		while (c < end) {
			uint8_t b = (uint8_t)c_read8(c++);
			if ((0xc0 == (uint8_t)b) && (0x80 == (uint8_t)c_read8(c))) {
				b = 0;
				c += 1;
			}
			*out++ = b;
		}
	}
}
#endif

void fx_ArrayBuffer_isView(txMachine* the)
{
	txSlot* slot;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc > 0) {
		slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			if (slot->next) {
				slot = slot->next;
				if ((slot->kind == XS_DATA_VIEW_KIND) || (slot->kind == XS_TYPED_ARRAY_KIND)) {
					mxResult->value.boolean = 1;
				}
			}
		}
	}
}

void fx_ArrayBuffer_prototype_get_byteLength(txMachine* the)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = instance->next;
	txSlot* bufferInfo = arrayBuffer->next;
	mxResult->kind = XS_INTEGER_KIND;
	if (arrayBuffer->value.arrayBuffer.address == C_NULL)
		mxResult->value.integer = 0;
	else
		mxResult->value.integer = bufferInfo->value.bufferInfo.length;
}

void fx_ArrayBuffer_prototype_get_detached(txMachine* the)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = instance->next;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (arrayBuffer->value.arrayBuffer.address == C_NULL) ? 1 : 0;
}

void fx_ArrayBuffer_prototype_get_immutable(txMachine* the)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = instance->next;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (arrayBuffer->flag & XS_DONT_SET_FLAG) ? 1 : 0;
}

void fx_ArrayBuffer_prototype_get_maxByteLength(txMachine* the)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = instance->next;
	txSlot* bufferInfo = arrayBuffer->next;
	mxResult->kind = XS_INTEGER_KIND;
	if (arrayBuffer->value.arrayBuffer.address == C_NULL)
		mxResult->value.integer = 0;
	else if (bufferInfo->value.bufferInfo.maxLength >= 0)
		mxResult->value.integer = bufferInfo->value.bufferInfo.maxLength;
	else
		mxResult->value.integer = bufferInfo->value.bufferInfo.length;
}

void fx_ArrayBuffer_prototype_get_resizable(txMachine* the)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = instance->next;
	txSlot* bufferInfo = arrayBuffer->next;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (bufferInfo->value.bufferInfo.maxLength >= 0) ? 1 : 0;
}

void fx_ArrayBuffer_prototype_concat(txMachine* the)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = instance->next;
	txSlot* bufferInfo = arrayBuffer->next;
	txInteger length = bufferInfo->value.bufferInfo.length;
	txInteger c = mxArgc, i = 0;
	txByte* address;
	txSlot* slot;
	while (i < c) {
		arrayBuffer = C_NULL;
		bufferInfo = C_NULL;
		slot = mxArgv(i);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference->next;
			if (slot) {
				if (slot->kind == XS_ARRAY_BUFFER_KIND) {
					bufferInfo = slot->next;
				}
				else if ((slot->kind == XS_HOST_KIND) && !(slot->flag & XS_HOST_CHUNK_FLAG)) {
					slot = slot->next;
					if (slot && (slot->kind == XS_BUFFER_INFO_KIND)) {
						bufferInfo = slot;
					}
				}
			}
		}
		if (bufferInfo) 
			length = fxAddChunkSizes(the, length, bufferInfo->value.bufferInfo.length);
		else
			mxTypeError("arguments[%ld]: not an ArrayBuffer instance", i);
		i++;
	}
	fxConstructArrayBufferResult(the, C_NULL, length);
	arrayBuffer = instance->next;
	bufferInfo = arrayBuffer->next;
	address = mxResult->value.reference->next->value.arrayBuffer.address;
	length = bufferInfo->value.bufferInfo.length;
	c_memcpy(address, arrayBuffer->value.arrayBuffer.address, length);
	address += length;
	i = 0;
	while (i < c) {
		slot = mxArgv(i)->value.reference->next;
		bufferInfo = slot->next;
		length = bufferInfo->value.bufferInfo.length;
		if (slot->kind == XS_ARRAY_BUFFER_KIND)
			c_memcpy(address, slot->value.arrayBuffer.address, length);
		else
			c_memcpy(address, slot->value.host.data, length);
		address += length;
		i++;
	}
}

void fx_ArrayBuffer_prototype_resize(txMachine* the)
{
	/* txSlot* instance = */ fxCheckArrayBufferInstance(the, mxThis);
	fxCheckArrayBufferMutable(the, mxThis);
	txInteger newByteLength = fxArgToByteLength(the, 0, 0);
	txSlot* arrayBuffer = fxCheckArrayBufferDetached(the, mxThis);
	txSlot* bufferInfo = arrayBuffer->next;
	txInteger maxByteLength, oldByteLength;
	txByte* chunk;
	maxByteLength = bufferInfo->value.bufferInfo.maxLength;
	if (maxByteLength < 0)
		mxTypeError("not resizable");
	oldByteLength = bufferInfo->value.bufferInfo.length;
	if (newByteLength > maxByteLength)
		mxRangeError("newLength > maxByteLength");
	arrayBuffer = fxCheckArrayBufferDetached(the, mxThis);
	chunk = (txByte*)fxRenewChunk(the, arrayBuffer->value.arrayBuffer.address, newByteLength);
	if (!chunk) {
		chunk = (txByte*)fxNewChunk(the, newByteLength);
		c_memcpy(chunk, arrayBuffer->value.arrayBuffer.address, (newByteLength < oldByteLength) ? newByteLength : oldByteLength);
	}
	if (newByteLength > oldByteLength)
		c_memset(chunk + oldByteLength, 0, newByteLength - oldByteLength);
	arrayBuffer->value.arrayBuffer.address = chunk;
	bufferInfo->value.bufferInfo.length = newByteLength;
}

void fx_ArrayBuffer_prototype_slice(txMachine* the)
{
	/* txSlot* instance = */ fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = fxCheckArrayBufferDetached(the, mxThis);
	txSlot* bufferInfo = arrayBuffer->next;
	txInteger length = bufferInfo->value.bufferInfo.length;
	txInteger start = fxArgToIndexInteger(the, 0, 0, length);
	txInteger stop = fxArgToIndexInteger(the, 1, length, length);
	txSlot* resultBuffer;
	if (stop < start) 
		stop = start;
	fxConstructArrayBufferResult(the, C_NULL, stop - start);
	resultBuffer = fxCheckArrayBufferDetached(the, mxResult);
	fxCheckArrayBufferMutable(the, mxResult);
	arrayBuffer = fxCheckArrayBufferDetached(the, mxThis);
	bufferInfo = arrayBuffer->next;
	if (bufferInfo->value.bufferInfo.length < stop)
		mxTypeError("resized this");
	c_memcpy(resultBuffer->value.arrayBuffer.address, arrayBuffer->value.arrayBuffer.address + start, stop - start);
}

static void fx_ArrayBuffer_prototype_transferAux(txMachine* the, txFlag flag)
{
	/* txSlot* instance = */ fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = fxCheckArrayBufferDetached(the, mxThis);
	txSlot* bufferInfo = arrayBuffer->next;
	txInteger oldByteLength = bufferInfo->value.bufferInfo.length;
	txInteger maxByteLength = bufferInfo->value.bufferInfo.maxLength;
	txInteger newByteLength = fxArgToByteLength(the, 0, oldByteLength);
	fxCheckArrayBufferMutable(the, mxThis);
	txSlot* resultBuffer;
	if ((maxByteLength >= 0) && (newByteLength > maxByteLength))
		mxRangeError("newLength > maxByteLength");
	fxConstructArrayBufferResult(the, &mxArrayBufferConstructor, newByteLength);
	resultBuffer = fxCheckArrayBufferDetached(the, mxResult);
	arrayBuffer = fxCheckArrayBufferDetached(the, mxThis);
	c_memcpy(resultBuffer->value.arrayBuffer.address, arrayBuffer->value.arrayBuffer.address, (newByteLength < oldByteLength) ? newByteLength : oldByteLength);
	if (newByteLength > oldByteLength)
		c_memset(resultBuffer->value.arrayBuffer.address + oldByteLength, 0, newByteLength - oldByteLength);
	arrayBuffer->value.arrayBuffer.address = C_NULL;
	bufferInfo->value.bufferInfo.length = 0;
	if (flag & 2)
		resultBuffer->flag |= XS_DONT_SET_FLAG;
	resultBuffer->next->value.bufferInfo.maxLength = (flag & 1) ? -1 : maxByteLength;
}

void fx_ArrayBuffer_prototype_transfer(txMachine* the)
{
	fx_ArrayBuffer_prototype_transferAux(the, 0);
}

void fx_ArrayBuffer_prototype_transferToFixedLength(txMachine* the)
{
	fx_ArrayBuffer_prototype_transferAux(the, 1);
}

void fx_ArrayBuffer_prototype_transferToImmutable(txMachine* the)
{
	fx_ArrayBuffer_prototype_transferAux(the, 3);
}

txSlot* fxCheckDataViewInstance(txMachine* the, txSlot* slot, txBoolean mutable)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_DATA_VIEW_KIND)) {
			if (mutable) {
				txSlot* arrayBuffer = slot->next->value.reference->next;
				if (arrayBuffer->flag & XS_DONT_SET_FLAG)
					mxTypeError("read-only buffer");
			}
			return instance;
		}
	}
	mxTypeError("this: not a DataView instance");
	return C_NULL;
}

txInteger fxCheckDataViewSize(txMachine* the, txSlot* view, txSlot* buffer, txBoolean mutable)
{
	txInteger size = view->value.dataView.size;
	txSlot* arrayBuffer = buffer->value.reference->next;
	txSlot* bufferInfo = arrayBuffer->next;
	if (arrayBuffer->value.arrayBuffer.address == C_NULL)
		mxTypeError("detached buffer");
	if (mutable && (arrayBuffer->flag & XS_DONT_SET_FLAG))
		mxTypeError("read-only buffer");
	if (bufferInfo->value.bufferInfo.maxLength >= 0) {
		txInteger offset = view->value.dataView.offset;
		txInteger byteLength = bufferInfo->value.bufferInfo.length;
		if (offset > byteLength)
			mxTypeError("out of bounds view");
		else if (size < 0)
			size = byteLength - offset;
		else if (offset + size > byteLength)
			mxTypeError("out of bounds view");
	}
	return size;
}

txSlot* fxGetBufferInfo(txMachine* the, txSlot* buffer)
{
	txSlot* arrayBuffer = buffer->value.reference->next;
	txSlot* bufferInfo = arrayBuffer->next;
	if (arrayBuffer->kind == XS_ARRAY_BUFFER_KIND) {
		if (arrayBuffer->value.arrayBuffer.address == C_NULL)
			mxTypeError("detached buffer");
		return bufferInfo;
	}
	if (arrayBuffer->kind == XS_HOST_KIND) {
		txInteger byteLength;
		if (bufferInfo && (bufferInfo->kind == XS_BUFFER_INFO_KIND))
			return bufferInfo;
		mxPushSlot(buffer);
		mxGetID(mxID(_byteLength));
		if (!fxCheckLength(the, the->stack, &byteLength))
			mxTypeError("invalid byteLength");
		fxReport(the, "# Use xsSetHostBuffer instead of xsSetHostData\n");
		mxPop();
		bufferInfo = fxNewSlot(the);
		bufferInfo->next = arrayBuffer->next;
		bufferInfo->flag = XS_INTERNAL_FLAG;
		bufferInfo->kind = XS_BUFFER_INFO_KIND;
		bufferInfo->value.bufferInfo.length = byteLength;
		bufferInfo->value.bufferInfo.maxLength = -1;
		arrayBuffer->next = bufferInfo;
		return bufferInfo;
	}
	mxTypeError("invalid buffer");
	return C_NULL;
}

txInteger fxGetDataViewSize(txMachine* the, txSlot* view, txSlot* buffer)
{
	txInteger size = view->value.dataView.size;
	txSlot* arrayBuffer = buffer->value.reference->next;
	txSlot* bufferInfo = arrayBuffer->next;
	if (arrayBuffer->value.arrayBuffer.address == C_NULL)
		return 0;
	if (bufferInfo->value.bufferInfo.maxLength >= 0) {
		txInteger offset = view->value.dataView.offset;
		txInteger byteLength = bufferInfo->value.bufferInfo.length;
		if (offset > byteLength)
			size = 0;
		else if (size < 0)
			size = byteLength - offset;
		else if (offset + size > byteLength)
			size = 0;
	}
	return size;
}

txBoolean fxIsDataViewOutOfBound(txMachine* the, txSlot* view, txSlot* buffer)
{
	txInteger size = view->value.dataView.size;
	txSlot* arrayBuffer = buffer->value.reference->next;
	txSlot* bufferInfo = arrayBuffer->next;
	if (arrayBuffer->value.arrayBuffer.address == C_NULL)
		return 1;
	if (bufferInfo->value.bufferInfo.maxLength >= 0) {
		txInteger offset = view->value.dataView.offset;
		txInteger byteLength = bufferInfo->value.bufferInfo.length;
		if (offset > byteLength)
			return 1;
		if ((size > 0) && (offset + size > byteLength))
			return 1;
	}
	return 0;
}

txSlot* fxNewDataViewInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_DATA_VIEW_KIND;
	property->value.dataView.offset = 0;
	property->value.dataView.size = 0;
	property = fxNextNullProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG);
	return instance;
}

void fx_DataView(txMachine* the)
{
	txSlot* slot;
	txBoolean flag = 0;
	txInteger offset, size;
	txSlot* info;
	txSlot* instance;
	txSlot* view;
	txSlot* buffer;
	if (!mxHasTarget)
		mxTypeError("call: DataView");
	if ((mxArgc > 0) && (mxArgv(0)->kind == XS_REFERENCE_KIND)) {
		slot = mxArgv(0)->value.reference->next;
		if (slot && ((slot->kind == XS_ARRAY_BUFFER_KIND) || (slot->kind == XS_HOST_KIND))) {
			flag = 1;
		}
	}
	if (!flag)
		mxTypeError("buffer: not an ArrayBuffer instance");
		
	offset = fxArgToByteLength(the, 1, 0);
	info = fxGetBufferInfo(the, mxArgv(0));
	if (info->value.bufferInfo.length < offset)
		mxRangeError("invalid byteOffset %ld", offset);
	size = fxArgToByteLength(the, 2, -1);
	if (size >= 0) {
		txInteger end = offset + size;
		if ((info->value.bufferInfo.length < end) || (end < offset))
			mxRangeError("invalid byteLength %ld", size);
	}
	else {
		if (info->value.bufferInfo.maxLength < 0)
			size = info->value.bufferInfo.length - offset;
	}
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxDataViewPrototype);
	instance = fxNewDataViewInstance(the);
	mxPullSlot(mxResult);
	view = instance->next;
	buffer = view->next;
	buffer->kind = XS_REFERENCE_KIND;
	buffer->value.reference = mxArgv(0)->value.reference;
	info = fxGetBufferInfo(the, buffer);
	if (info->value.bufferInfo.maxLength >= 0) {
		if (info->value.bufferInfo.length < offset)
			mxRangeError("invalid byteOffset %ld", offset);
		else if (size >= 0) {
			txInteger end = offset + size;
			if ((info->value.bufferInfo.length < end) || (end < offset))
				mxRangeError("invalid byteLength %ld", size);
		}
	}
	view->value.dataView.offset = offset;
	view->value.dataView.size = size;
}

void fx_DataView_prototype_buffer_get(txMachine* the)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	mxResult->kind = buffer->kind;
	mxResult->value = buffer->value;
}

void fx_DataView_prototype_byteLength_get(txMachine* the)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	txInteger size = fxCheckDataViewSize(the, view, buffer, XS_IMMUTABLE);
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = size;
}

void fx_DataView_prototype_byteOffset_get(txMachine* the)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	fxCheckDataViewSize(the, view, buffer, XS_IMMUTABLE);
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = view->value.dataView.offset;
}

void fx_DataView_prototype_get(txMachine* the, txNumber delta, txTypeCallback getter)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	txInteger offset = fxArgToByteLength(the, 0, 0);
	txInteger size;
	int endian = EndianBig;
	if ((mxArgc > 1) && fxToBoolean(the, mxArgv(1)))
		endian = EndianLittle;
	size = fxCheckDataViewSize(the, view, buffer, XS_IMMUTABLE);
	if ((size < delta) || ((size - delta) < offset))
		mxRangeError("invalid byteOffset");
	offset += view->value.dataView.offset;
	(*getter)(the, buffer->value.reference->next, offset, mxResult, endian);
}

void fx_DataView_prototype_getBigInt64(txMachine* the)
{
	fx_DataView_prototype_get(the, 8, fxBigInt64Getter);
}

void fx_DataView_prototype_getBigUint64(txMachine* the)
{
	fx_DataView_prototype_get(the, 8, fxBigUint64Getter);
}

#if mxFloat16
void fx_DataView_prototype_getFloat16(txMachine* the)
{
	fx_DataView_prototype_get(the, 2, fxFloat16Getter);
}
#endif

void fx_DataView_prototype_getFloat32(txMachine* the)
{
	fx_DataView_prototype_get(the, 4, fxFloat32Getter);
}

void fx_DataView_prototype_getFloat64(txMachine* the)
{
	fx_DataView_prototype_get(the, 8, fxFloat64Getter);
}

void fx_DataView_prototype_getInt8(txMachine* the)
{
	fx_DataView_prototype_get(the, 1, fxInt8Getter);
}

void fx_DataView_prototype_getInt16(txMachine* the)
{
	fx_DataView_prototype_get(the, 2, fxInt16Getter);
}

void fx_DataView_prototype_getInt32(txMachine* the)
{
	fx_DataView_prototype_get(the, 4, fxInt32Getter);
}

void fx_DataView_prototype_getUint8(txMachine* the)
{
	fx_DataView_prototype_get(the, 1, fxUint8Getter);
}

void fx_DataView_prototype_getUint16(txMachine* the)
{
	fx_DataView_prototype_get(the, 2, fxUint16Getter);
}

void fx_DataView_prototype_getUint32(txMachine* the)
{
	fx_DataView_prototype_get(the, 4, fxUint32Getter);
}

void fx_DataView_prototype_set(txMachine* the, txNumber delta, txTypeCoerce coercer, txTypeCallback setter)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis, XS_MUTABLE);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	txInteger offset = fxArgToByteLength(the, 0, 0);
	txInteger size;
	int endian = EndianBig;
	txSlot* value;
	if (mxArgc > 1)
		mxPushSlot(mxArgv(1));
	else
		mxPushUndefined();
	value = the->stack;	
	(*coercer)(the, value);
	if ((mxArgc > 2) && fxToBoolean(the, mxArgv(2)))
		endian = EndianLittle;
	size = fxCheckDataViewSize(the, view, buffer, XS_MUTABLE);
	if ((size < delta) || ((size - delta) < offset))
		mxRangeError("invalid byteOffset");
	offset += view->value.dataView.offset;
	(*setter)(the, buffer->value.reference->next, offset, value, endian);
	mxPop();
}

void fx_DataView_prototype_setBigInt64(txMachine* the)
{
	fx_DataView_prototype_set(the, 8, fxBigIntCoerce, fxBigInt64Setter);
}

void fx_DataView_prototype_setBigUint64(txMachine* the)
{
	fx_DataView_prototype_set(the, 8, fxBigIntCoerce, fxBigUint64Setter);
}

#if mxFloat16
void fx_DataView_prototype_setFloat16(txMachine* the)
{
	fx_DataView_prototype_set(the, 2, fxNumberCoerce, fxFloat16Setter);
}
#endif

void fx_DataView_prototype_setFloat32(txMachine* the)
{
	fx_DataView_prototype_set(the, 4, fxNumberCoerce, fxFloat32Setter);
}

void fx_DataView_prototype_setFloat64(txMachine* the)
{
	fx_DataView_prototype_set(the, 8, fxNumberCoerce, fxFloat64Setter);
}

void fx_DataView_prototype_setInt8(txMachine* the)
{
	fx_DataView_prototype_set(the, 1, fxIntCoerce, fxInt8Setter);
}

void fx_DataView_prototype_setInt16(txMachine* the)
{
	fx_DataView_prototype_set(the, 2, fxIntCoerce, fxInt16Setter);
}

void fx_DataView_prototype_setInt32(txMachine* the)
{
	fx_DataView_prototype_set(the, 4, fxIntCoerce, fxInt32Setter);
}

void fx_DataView_prototype_setUint8(txMachine* the)
{
	fx_DataView_prototype_set(the, 1, fxUintCoerce, fxUint8Setter);
}

void fx_DataView_prototype_setUint16(txMachine* the)
{
	fx_DataView_prototype_set(the, 2, fxUintCoerce, fxUint16Setter);
}

void fx_DataView_prototype_setUint32(txMachine* the)
{
	fx_DataView_prototype_set(the, 4, fxUintCoerce, fxUint32Setter);
}


#define mxTypedArrayDeclarations \
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis); \
	txSlot* dispatch = instance->next; \
	txSlot* view = dispatch->next; \
	txSlot* buffer = view->next; \
	txInteger length = fxCheckDataViewSize(the, view, buffer, XS_IMMUTABLE) >> dispatch->value.typedArray.dispatch->shift

#define mxMutableTypedArrayDeclarations \
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis); \
	txSlot* dispatch = instance->next; \
	txSlot* view = dispatch->next; \
	txSlot* buffer = view->next; \
	txInteger length = fxCheckDataViewSize(the, view, buffer, XS_MUTABLE) >> dispatch->value.typedArray.dispatch->shift

#define mxResultTypedArrayDeclarations \
	txSlot* resultInstance = fxCheckTypedArrayInstance(the, mxResult); \
	txSlot* resultDispatch = resultInstance->next; \
	txSlot* resultView = resultDispatch->next; \
	txSlot* resultBuffer = resultView->next; \
	txInteger resultLength = fxCheckDataViewSize(the, resultView, resultBuffer, XS_MUTABLE) >> resultDispatch->value.typedArray.dispatch->shift

void fxTypedArrayGetter(txMachine* the)
{
	txSlot* instance = fxToInstance(the, mxThis);
	txSlot* dispatch;
	while (instance) {
		if (instance->flag & XS_EXOTIC_FLAG) {
			dispatch = instance->next;
			if (dispatch->ID == XS_TYPED_ARRAY_BEHAVIOR)
				break;
		}
		instance = fxGetPrototype(the, instance);
	}
	if (instance) {
		txID id = the->scratch.value.at.id;
		txIndex index = the->scratch.value.at.index;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = fxGetDataViewSize(the, view, buffer) >> shift;
		if ((!id) && (index < length)) {
			(*dispatch->value.typedArray.dispatch->getter)(the, buffer->value.reference->next, view->value.dataView.offset + (index << shift), mxResult, EndianNative);
		}
	}
}

void fxTypedArraySetter(txMachine* the)
{
	txSlot* instance = fxToInstance(the, mxThis);
	while (instance) {
		if (instance->flag & XS_EXOTIC_FLAG) {
			if (instance->next->ID == XS_TYPED_ARRAY_BEHAVIOR)
				break;
		}
		instance = fxGetPrototype(the, instance);
	}
	if (instance) {
		txSlot* value = mxArgv(0);
		txID id = the->scratch.value.at.id;
		txIndex index = the->scratch.value.at.index;
        if (!fxTypedArraySetPropertyValue(the, instance, id, index, value, mxThis)) {
            if (the->frame->next->flag & XS_STRICT_FLAG)
				mxTypeError("not extensible or not writable");
        }
	}
}

txBoolean fxTypedArrayDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask) 
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txSlot* arrayBuffer = buffer->value.reference->next;
		txIndex length = fxGetDataViewSize(the, view, buffer) >> shift;
		if (id || (index >= length))
			return 0;
		if ((mask & XS_DONT_DELETE_FLAG) && (slot->flag & XS_DONT_DELETE_FLAG))
			return 0;
		if ((mask & XS_DONT_ENUM_FLAG) && (slot->flag & XS_DONT_ENUM_FLAG))
			return 0;
		if (mask & XS_ACCESSOR_FLAG)
			return 0;
		if ((mask & XS_DONT_SET_FLAG) && (slot->flag & XS_DONT_SET_FLAG))
			return 0;
		if (slot->kind != XS_UNINITIALIZED_KIND) {
			dispatch->value.typedArray.dispatch->coerce(the, slot);
			if (arrayBuffer->flag & XS_DONT_SET_FLAG)
                return 0;
			length = fxGetDataViewSize(the, view, buffer) >> shift;
			if (index < length)
				(*dispatch->value.typedArray.dispatch->setter)(the, arrayBuffer, view->value.dataView.offset + (index << shift), slot, EndianNative);
		}
		return 1;
	}
	return fxOrdinaryDefineOwnProperty(the, instance, id, index, slot, mask);
}

txBoolean fxTypedArrayDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = fxGetDataViewSize(the, view, buffer) >> shift;
		return ((!id) && (index < length)) ? 0 : 1;
	}
	return fxOrdinaryDeleteProperty(the, instance, id, index);
}

txBoolean fxTypedArrayGetOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = fxGetDataViewSize(the, view, buffer) >> shift;
		if ((!id) && (index < length)) {
			(*dispatch->value.typedArray.dispatch->getter)(the, buffer->value.reference->next, view->value.dataView.offset + (index << shift), slot, EndianNative);
			return 1;
		}
		slot->kind = XS_UNDEFINED_KIND;
		slot->flag = XS_NO_FLAG;
		return 0;
	}
	return fxOrdinaryGetOwnProperty(the, instance, id, index, slot);
}

txSlot* fxTypedArrayGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		the->scratch.value.at.id = id;
		the->scratch.value.at.index = index;
		return &mxTypedArrayAccessor;
	}
	return fxOrdinaryGetProperty(the, instance, id, index, flag);
}

txBoolean fxTypedArrayGetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* receiver, txSlot* value)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = fxGetDataViewSize(the, view, buffer) >> shift;
		if ((!id) && (index < length)) {
			(*dispatch->value.typedArray.dispatch->getter)(the, buffer->value.reference->next, view->value.dataView.offset + (index << shift), value, EndianNative);
			return 1;
		}
		value->kind = XS_UNDEFINED_KIND;
		return 0;
	}
	return fxOrdinaryGetPropertyValue(the, instance, id, index, receiver, value);
}

txBoolean fxTypedArrayHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = fxGetDataViewSize(the, view, buffer) >> shift;
		return ((!id) && (index < length)) ? 1 : 0;
	}
	return fxOrdinaryHasProperty(the, instance, id, index);
}

void fxTypedArrayOwnKeys(txMachine* the, txSlot* instance, txFlag flag, txSlot* keys)
{
	if (flag & XS_EACH_NAME_FLAG) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txIndex length = fxGetDataViewSize(the, view, buffer) >> shift;
		if (length) {
			txIndex index;
			for (index = 0; index < length; index++)
				keys = fxQueueKey(the, 0, index, keys);
		}
	}
	fxOrdinaryOwnKeys(the, instance, flag, keys);
}

txBoolean fxTypedArrayPreventExtensions(txMachine* the, txSlot* instance)
{
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txSlot* arrayBuffer = buffer->value.reference->next;
	txSlot* bufferInfo = arrayBuffer->next;
	if ((arrayBuffer->kind == XS_ARRAY_BUFFER_KIND) && (bufferInfo->value.bufferInfo.maxLength >= 0))
		return 0;
	return fxOrdinaryPreventExtensions(the, instance);
}

txSlot* fxTypedArraySetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		the->scratch.value.at.id = id;
		the->scratch.value.at.index = index;
		return &mxTypedArrayAccessor;
	}
	return fxOrdinarySetProperty(the, instance, id, index, flag);
}

txBoolean fxTypedArraySetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver)
{
	if ((!id) || fxIsCanonicalIndex(the, id)) {
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
		txSlot* arrayBuffer = buffer->value.reference->next;
		txIndex length;
		if ((receiver->kind == XS_REFERENCE_KIND) && (receiver->value.reference == instance)) {
			dispatch->value.typedArray.dispatch->coerce(the, value);
			if (arrayBuffer->flag & XS_DONT_SET_FLAG) {
				if (the->frame->next->flag & XS_STRICT_FLAG)
					mxTypeError("read-only buffer");
				else
					return 0;
			}
			length = fxGetDataViewSize(the, view, buffer) >> shift;
			if ((!id) && (index < length)) {
				(*dispatch->value.typedArray.dispatch->setter)(the, buffer->value.reference->next, view->value.dataView.offset + (index << shift), value, EndianNative);
			}
			return 1;
		}
		length = fxGetDataViewSize(the, view, buffer) >> shift;
		if ((id) || (index >= length)) {
			return 1;
		}
	}
	return fxOrdinarySetPropertyValue(the, instance, id, index, value, receiver);
}

void fxCallTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index, txSlot* item)
{
	/* THIS */
	if (mxArgc > 1)
		mxPushSlot(mxArgv(1));
	else
		mxPushUndefined();
	/* FUNCTION */
	mxPushSlot(function);
	mxCall();
	/* ARGUMENTS */
	mxPushSlot(mxThis);
	mxGetIndex(index);
	if (item) {
		item->kind = the->stack->kind;
		item->value = the->stack->value;
	}
	mxPushInteger(index);
	mxPushSlot(mxThis);
	mxRunCount(3);
}

txSlot* fxCheckTypedArrayInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
        txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_TYPED_ARRAY_KIND))
			return instance;
	}
	mxTypeError("this: not a TypedArray instance");
	return C_NULL;
}

txSlot* fxConstructTypedArray(txMachine* the)
{
	txSlot* prototype;
	txSlot* dispatch;
	txSlot* instance;
	if (!mxHasTarget)
		mxTypeError("call: TypedArray");
	dispatch = mxFunctionInstanceHome(mxFunction->value.reference);
	dispatch = dispatch->next;
	prototype = mxBehaviorGetProperty(the, mxFunction->value.reference, mxID(_prototype), 0, XS_ANY);
	if (!dispatch || (dispatch->kind != XS_TYPED_ARRAY_KIND))
		mxTypeError("new: TypedArray");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, prototype);
	instance = fxNewTypedArrayInstance(the, dispatch->value.typedArray.dispatch, dispatch->value.typedArray.atomics);
	mxPullSlot(mxResult);
	return instance;
}

void fxCreateTypedArraySpecies(txMachine* the)
{
	txSlot* instance = fxToInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* constructor = &the->stackIntrinsics[-1 - (txInteger)dispatch->value.typedArray.dispatch->constructorID];
	mxPushSlot(mxThis);
	mxGetID(mxID(_constructor));
	fxToSpeciesConstructor(the, constructor);
	mxNew();
}

void fxReduceTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index)
{
	/* THIS */
	mxPushUndefined();
	/* FUNCTION */
	mxPushSlot(function);
	mxCall();
	/* ARGUMENTS */
	mxPushSlot(mxResult);
	mxPushSlot(mxThis);
	mxGetIndex(index);
	mxPushInteger(index);
	mxPushSlot(mxThis);
	mxRunCount(4);
	mxPullSlot(mxResult);
}

txSlot* fxNewTypedArrayInstance(txMachine* the, txTypeDispatch* dispatch, txTypeAtomics* atomics)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_EXOTIC_FLAG;
	property = fxNextTypeDispatchProperty(the, instance, dispatch, atomics, XS_TYPED_ARRAY_BEHAVIOR, XS_INTERNAL_FLAG);
	property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_DATA_VIEW_KIND;
	property->value.dataView.offset = 0;
	property->value.dataView.size = 0;
	property = fxNextNullProperty(the, property, XS_NO_ID, XS_INTERNAL_FLAG);
	return instance;
}

void fx_TypedArray(txMachine* the)
{
	if (mxArgc > 0) {
		if (mxArgv(0)->kind == XS_REFERENCE_KIND) {
			txSlot* instance = fxConstructTypedArray(the);
			txSlot* dispatch = instance->next;
			txSlot* view = dispatch->next;
			txSlot* buffer = view->next;
			txSlot* data = C_NULL;
			txU2 shift = dispatch->value.typedArray.dispatch->shift;
			txSlot* slot = mxArgv(0)->value.reference->next;
			if (slot && ((slot->kind == XS_ARRAY_BUFFER_KIND) || (slot->kind == XS_HOST_KIND))) {
				txInteger offset = fxArgToByteLength(the, 1, 0);
				txInteger size;
				txSlot* info;
				if (offset & ((1 << shift) - 1))
					mxRangeError("invalid byteOffset %ld", offset);
				size = fxArgToByteLength(the, 2, -1);
				info = fxGetBufferInfo(the, mxArgv(0));
				if (size >= 0) {
					txInteger delta = size << shift;		//@@ overflow
					txInteger end = fxAddChunkSizes(the, offset, delta);
					if ((info->value.bufferInfo.length < end) || (end < offset))
						mxRangeError("invalid length %ld", size);
					size = delta;
				}
				else if (info->value.bufferInfo.maxLength >= 0) {
					if (info->value.bufferInfo.length < offset)
						mxRangeError("invalid offset %ld", offset);
					size = -1;
				}
				else {
					if (info->value.bufferInfo.length & ((1 << shift) - 1))
						mxRangeError("invalid byteLength %ld", info->value.bufferInfo.length);
					size = info->value.bufferInfo.length - offset;
					if (size < 0)
						mxRangeError("invalid byteLength %ld", size);
				}
				view->value.dataView.offset = offset;
				view->value.dataView.size = size;
				buffer->kind = XS_REFERENCE_KIND;
				buffer->value.reference = mxArgv(0)->value.reference;
			}
			else if (slot && (slot->kind == XS_TYPED_ARRAY_KIND)) {
				txSlot* sourceDispatch = slot;
				txSlot* sourceView = sourceDispatch->next;
				txSlot* sourceBuffer = sourceView->next;
				txU2 sourceShift = sourceDispatch->value.typedArray.dispatch->shift;
				txInteger sourceLength = fxCheckDataViewSize(the, sourceView, sourceBuffer, XS_IMMUTABLE) >> sourceShift;
				txSlot* sourceData = sourceBuffer->value.reference->next;
				txInteger sourceDelta = sourceDispatch->value.typedArray.dispatch->size;
				txInteger sourceOffset = sourceView->value.dataView.offset;
				txInteger offset = 0;
				txInteger size = sourceLength << shift;
				/* TARGET */
				mxPush(mxArrayBufferConstructor);
				/* THIS */
				mxPushUninitialized();
				/* FUNCTION */
				mxPush(mxArrayBufferConstructor);
				/* RESULT */
				mxPushUndefined();
				/* FRAME */
				mxPushUninitialized();
				the->stack->flag |= XS_TARGET_FLAG;	
				/* ARGUMENTS */
				sourceLength = fxGetDataViewSize(the, sourceView, sourceBuffer) >> sourceShift;
				size = sourceLength << shift;
				mxPushInteger(size);
				mxRunCount(1);
				mxPullSlot(buffer);
				sourceLength = fxCheckDataViewSize(the, sourceView, sourceBuffer, XS_IMMUTABLE) >> sourceShift;
				size = sourceLength << shift;
				
				data = fxCheckArrayBufferDetached(the, buffer);
				fxCheckArrayBufferMutable(the, buffer);
				view->value.dataView.offset = offset;
				view->value.dataView.size = size;
				if (dispatch == sourceDispatch)
					c_memcpy(data->value.arrayBuffer.address + offset, sourceData->value.arrayBuffer.address + sourceOffset, size);
				else {
					txBoolean contentType = (dispatch->value.typedArray.dispatch->constructorID == _BigInt64Array)
							|| (dispatch->value.typedArray.dispatch->constructorID == _BigUint64Array);
					txBoolean sourceContentType = (sourceDispatch->value.typedArray.dispatch->constructorID == _BigInt64Array)
							|| (sourceDispatch->value.typedArray.dispatch->constructorID == _BigUint64Array);
					if (contentType != sourceContentType)
						mxTypeError("incompatible content type");
					mxPushUndefined();
					while (offset < size) {
						(*sourceDispatch->value.typedArray.dispatch->getter)(the, sourceData, sourceOffset, the->stack, EndianNative);
						(*dispatch->value.typedArray.dispatch->coerce)(the, the->stack);
						(*dispatch->value.typedArray.dispatch->setter)(the, data, offset, the->stack, EndianNative);
						sourceOffset += sourceDelta;
						offset += 1 << shift;
					}
					mxPop();
				}
			}
			else {
				fx_TypedArray_from_object(the, instance, C_NULL, C_NULL);
			}
			return;
		}
		fxToNumber(the, mxArgv(0));
	}
	{
		txSlot* instance = fxConstructTypedArray(the);
		txSlot* dispatch = instance->next;
		txSlot* view = dispatch->next;
		txSlot* buffer = view->next;
		txU2 shift = dispatch->value.typedArray.dispatch->shift;
        txInteger length = fxArgToByteLength(the, 0, 0);
       if (length > (0x7FFFFFFF >> shift))
			mxRangeError("byteLength too big");
        length <<= shift;
		mxPush(mxArrayBufferConstructor);
		mxNew();
		mxPushInteger(length);
		mxRunCount(1);
		mxPullSlot(buffer);
        view->value.dataView.offset = 0;
        view->value.dataView.size = length;
	}
}

void fx_TypedArray_from(txMachine* the)
{
	txSlot* function = C_NULL;
	txSlot* _this = C_NULL;
	if (!mxIsReference(mxThis) || !(mxIsConstructor(mxThis->value.reference)))
		mxTypeError("this: not a constructor");
	if (mxArgc > 1) {
		txSlot* slot = mxArgv(1);
		if (!mxIsUndefined(slot)) {
			function = slot;
			if (!fxIsCallable(the, function))
				mxTypeError("map: not a function");
			if (mxArgc > 2)
				_this = mxArgv(2);
		}
	}
	fx_TypedArray_from_object(the, C_NULL, function, _this);
}

void fx_TypedArray_from_object(txMachine* the, txSlot* instance, txSlot* function, txSlot* _this)
{
	txSlot* stack = the->stack;
	txSlot* iterator;
	txSlot* next;
	txSlot* value;
	txSlot* list = C_NULL;
	txSlot* slot;
	txSlot* dispatch;
	txSlot* view;
	txSlot* buffer;
	txSlot* data;
	txNumber length;
	mxTemporary(iterator);
	mxTemporary(next);
	if (fxGetIterator(the, mxArgv(0), iterator, next, 1)) {
		list = fxNewInstance(the);
		slot = list;
		length = 0;
		mxTemporary(value);
		while (fxIteratorNext(the, iterator, next, value)) {
			slot = fxNextSlotProperty(the, slot, value, XS_NO_ID, XS_NO_FLAG);
			length++;
		}
	}
	else {
		mxPushSlot(mxArgv(0));
		mxGetID(mxID(_length));
		length = fxToLength(the, the->stack);
		mxPop();
	}
	if (instance) {
		dispatch = instance->next;
		view = dispatch->next;
		buffer = view->next;
		mxPush(mxArrayBufferConstructor);
		mxNew();
		mxPushNumber(length * dispatch->value.typedArray.dispatch->size);
		mxRunCount(1);
		mxPullSlot(buffer);
		data = fxCheckArrayBufferDetached(the, buffer);
		fxCheckArrayBufferMutable(the, buffer);
		view->value.dataView.offset = 0;
		view->value.dataView.size = data->next->value.bufferInfo.length;
	}
	else {
		mxPushSlot(mxThis);
		mxNew();
		mxPushNumber(length);
		mxRunCount(1);
		mxPullSlot(mxResult);
		instance = fxToInstance(the, mxResult);
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_TYPED_ARRAY_KIND)) {
			dispatch = instance->next;
			view = dispatch->next;
			buffer = view->next;
			data = fxCheckArrayBufferDetached(the, buffer);
			fxCheckArrayBufferMutable(the, buffer);
			if (length > (fxGetDataViewSize(the, view, buffer) >> dispatch->value.typedArray.dispatch->shift))
				mxTypeError("result: too small TypedArray instance");
		}
		else
			mxTypeError("result: not a TypedArray instance");
	}
	if (list) {
		txInteger index = 0;
		slot = list->next;
		while (slot) {
			/* ARG0 */
			if (function) {
				/* THIS */
				if (_this)
					mxPushSlot(_this);
				else
					mxPushUndefined();
				/* FUNCTION */
				mxPushSlot(function);
				mxCall();
				/* ARGUMENTS */
				mxPushSlot(slot);
				mxPushInteger(index);
				mxRunCount(2);
			}
			else
				mxPushSlot(slot);
			mxPushSlot(mxResult);
			mxSetIndex(index);
			mxPop();
			index++;
			slot = slot->next;
		}
	}
	else {
		txInteger index = 0;
		txInteger count = (txInteger)length;
		while (index < count) {
			if (function) {
				/* THIS */
				if (_this)
					mxPushSlot(_this);
				else
					mxPushUndefined();
				/* FUNCTION */
				mxPushSlot(function);
				mxCall();
				/* ARGUMENTS */
				mxPushSlot(mxArgv(0));
				mxGetIndex(index);
				mxPushInteger(index);
				mxRunCount(2);
			}
			else {
				mxPushSlot(mxArgv(0));
				mxGetIndex(index);
			}
			mxPushSlot(mxResult);
			mxSetIndex(index);
			mxPop();
			index++;
		}	
	}
	the->stack = stack;
}

void fx_TypedArray_of(txMachine* the)
{
	txInteger count = mxArgc;
	txInteger index = 0;
	mxPushSlot(mxThis);
	mxNew();
	mxPushInteger(count);
	mxRunCount(1);
	mxPullSlot(mxResult);
	{
		mxResultTypedArrayDeclarations;
		txU2 shift = resultDispatch->value.typedArray.dispatch->shift;
		if (resultLength < count)
			mxTypeError("result: too small TypedArray instance");
		while (index < count) {
			(*resultDispatch->value.typedArray.dispatch->coerce)(the, mxArgv(index));
			if (resultBuffer->value.arrayBuffer.address == C_NULL)
				mxTypeError("detached buffer");
			(*resultDispatch->value.typedArray.dispatch->setter)(the, resultBuffer->value.reference->next, resultView->value.dataView.offset + (index << shift), mxArgv(index), EndianNative);
			index++;
		}
	}
}

void fx_TypedArray_prototype_at(txMachine* the)
{
	mxTypedArrayDeclarations;
	txInteger index = (mxArgc > 0) ? fxToInteger(the, mxArgv(0)) : 0;
	if (index < 0)
		index = length + index;
	if ((0 <= index) && (index < length)) {
		mxPushSlot(mxThis);
		mxGetIndex(index);
		mxPullSlot(mxResult);
	}
}

void fx_TypedArray_prototype_buffer_get(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	mxResult->kind = buffer->kind;
	mxResult->value = buffer->value;
}

void fx_TypedArray_prototype_byteLength_get(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txU2 shift = dispatch->value.typedArray.dispatch->shift;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = fxGetDataViewSize(the, view, buffer) & ~((1 << shift) - 1);
}

void fx_TypedArray_prototype_byteOffset_get(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txInteger offset = view->value.dataView.offset;
	txInteger size = view->value.dataView.size;
	txSlot* arrayBuffer = buffer->value.reference->next;
	txSlot* bufferInfo = arrayBuffer->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = 0;
	if (arrayBuffer->value.arrayBuffer.address == C_NULL)
		return;
	if (bufferInfo->value.bufferInfo.maxLength >= 0) {
		txInteger byteLength = bufferInfo->value.bufferInfo.length;
		if (offset > byteLength)
			return;
		size = (size < 0) ? byteLength : offset + size;
		if (size > byteLength)
			return;
		size -= offset;
	}
	mxResult->value.integer = offset;
}

void fx_TypedArray_prototype_copyWithin(txMachine* the)
{
	mxMutableTypedArrayDeclarations;
	txInteger delta = dispatch->value.typedArray.dispatch->size;
	txInteger target = fxArgToIndexInteger(the, 0, 0, length);
	txInteger start = fxArgToIndexInteger(the, 1, 0, length);
	txInteger end = fxArgToIndexInteger(the, 2, length, length);
	txInteger count = end - start;
	if (count > length - target)
		count = length - target;
	if (count > 0) {
		txByte* address = buffer->value.reference->next->value.arrayBuffer.address;
		txInteger offset = view->value.dataView.offset;
		if (fxIsDataViewOutOfBound(the, view, buffer))
			mxTypeError("out of bound buffer");
		target = offset + (target * delta);
		start = offset + (start * delta);
		end = offset + fxCheckDataViewSize(the, view, buffer, XS_MUTABLE);
		count = count * delta;
		if (count > end - target)
			count = end - target;
		if (count > end - start)
			count = end - start;
		if (count > 0) {
			c_memmove(address + target, address + start, count);
			mxMeterSome((txU4)count * 2);
		}
	}
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_TypedArray_prototype_entries(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txSlot* property;
	fxCheckDataViewSize(the, view, buffer, XS_IMMUTABLE);
	mxPush(mxArrayIteratorPrototype);
	property = fxLastProperty(the, fxNewIteratorInstance(the, mxThis, mxID(_Array)));
	property = fxNextIntegerProperty(the, property, 2, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPullSlot(mxResult);
}

void fx_TypedArray_prototype_every(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, buffer->value.reference->next, index, C_NULL);
		mxResult->value.boolean = fxToBoolean(the, the->stack++);
		if (!mxResult->value.boolean)
			break;
		index++;
	}
}

void fx_TypedArray_prototype_fill(txMachine* the)
{
	mxMutableTypedArrayDeclarations;
	txInteger delta = dispatch->value.typedArray.dispatch->size;
	txInteger start = fxArgToIndexInteger(the, 1, 0, length);
	txInteger end = fxArgToIndexInteger(the, 2, length, length);
	start *= delta;
	end *= delta;
	start += view->value.dataView.offset;
	end += view->value.dataView.offset;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	(*dispatch->value.typedArray.dispatch->coerce)(the, the->stack);
	fxCheckDataViewSize(the, view, buffer, XS_MUTABLE);
	while (start < end) {
		(*dispatch->value.typedArray.dispatch->setter)(the, buffer->value.reference->next, start, the->stack, EndianNative);
		start += delta;
	}
	mxPop();
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_TypedArray_prototype_filter(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txSlot* list = fxNewInstance(the);
	txSlot* slot = list;
	txInteger count = 0;
	txInteger index = 0;
	mxPushUndefined();
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, buffer->value.reference->next, index, the->stack);
		if (fxToBoolean(the, the->stack++)) {
			count++;
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
		}
		index++;
	}
	mxPop();
	fxCreateTypedArraySpecies(the);
	mxPushNumber(count);
	mxRunCount(1);
	mxPullSlot(mxResult);
	{
		mxResultTypedArrayDeclarations;
		txInteger resultOffset = 0;
		txInteger resultSize = resultDispatch->value.typedArray.dispatch->size;
		if (resultLength < count)
			mxTypeError("result: too small TypedArray instance");
		slot = list->next;
		while (slot) {
			(*resultDispatch->value.typedArray.dispatch->coerce)(the, slot);
			(*resultDispatch->value.typedArray.dispatch->setter)(the, resultBuffer->value.reference->next, resultOffset, slot, EndianNative);
			resultOffset += resultSize;
			slot = slot->next;
		}
	}
	mxPop();
}

void fx_TypedArray_prototype_find(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	mxPushUndefined();
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, buffer->value.reference->next, index, the->stack);
		if (fxToBoolean(the, the->stack++)) {
			mxResult->kind = the->stack->kind;
			mxResult->value = the->stack->value;
			break;
		}
		index++;
	}
	mxPop();
}

void fx_TypedArray_prototype_findIndex(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = -1;
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, buffer->value.reference->next, index, C_NULL);
		if (fxToBoolean(the, the->stack++)) {
			mxResult->value.integer = index;
			break;
		}
		index++;
	}
}

void fx_TypedArray_prototype_findLast(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = length;
	mxPushUndefined();
	while (index > 0) {
		index--;
		fxCallTypedArrayItem(the, function, dispatch, view, buffer->value.reference->next, index, the->stack);
		if (fxToBoolean(the, the->stack++)) {
			mxResult->kind = the->stack->kind;
			mxResult->value = the->stack->value;
			break;
		}
	}
	mxPop();
}

void fx_TypedArray_prototype_findLastIndex(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = length;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = -1;
	while (index > 0) {
		index--;
		fxCallTypedArrayItem(the, function, dispatch, view, buffer->value.reference->next, index, C_NULL);
		if (fxToBoolean(the, the->stack++)) {
			mxResult->value.integer = index;
			break;
		}
	}
}

void fx_TypedArray_prototype_forEach(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, buffer->value.reference->next, index, C_NULL);
		mxPop();
		index++;
	}
}

void fx_TypedArray_prototype_includes(txMachine* the)
{
	mxTypedArrayDeclarations;
	fxBoolean(the, mxResult, 0);
	if (length) {
		txInteger index = fxArgToIndexInteger(the, 1, 0, length);
		txSlot* argument;
		if (mxArgc > 0)
			mxPushSlot(mxArgv(0));
		else
			mxPushUndefined();
		argument = the->stack;
		while (index < length) {
			mxPushSlot(mxThis);
			mxGetIndex(index);
			if (fxIsSameValue(the, the->stack++, argument, 1)) {
				mxResult->value.boolean = 1;
				break;
			}
			index++;
			mxCheckMetering();
		}
		mxPop();
	}
}

void fx_TypedArray_prototype_indexOf(txMachine* the)
{
	mxTypedArrayDeclarations;
	fxInteger(the, mxResult, -1);
	if (length) {
		txInteger index = fxArgToIndexInteger(the, 1, 0, length);
		txSlot* argument;
		if (mxArgc > 0)
			mxPushSlot(mxArgv(0));
		else
			mxPushUndefined();
		argument = the->stack;
		while (index < length) {
			mxPushSlot(mxThis);
			if (fxHasIndex(the, index)) {
				mxPushSlot(mxThis);
				mxGetIndex(index);
				if (fxIsSameSlot(the, the->stack++, argument)) {
					mxResult->value.integer = index;
					break;
				}
			}
			index++;
			mxCheckMetering();
		}
		mxPop();
	}
}

void fx_TypedArray_prototype_join(txMachine* the)
{
	mxTypedArrayDeclarations;
	txInteger delta = dispatch->value.typedArray.dispatch->size;
	txInteger offset = view->value.dataView.offset;
	txInteger limit = offset + (length << dispatch->value.typedArray.dispatch->shift);
	txString string;
	txSlot* list = fxNewInstance(the);
	txSlot* slot = list;
	txBoolean comma = 0;
	txInteger size = 0;
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
		mxPushSlot(mxArgv(0));
		string = fxToString(the, the->stack);
		the->stack->kind += XS_KEY_KIND - XS_STRING_KIND;
		the->stack->value.key.sum = mxStringLength(the->stack->value.string);
	}
	else {
		mxPushStringX(",");
		the->stack->kind += XS_KEY_KIND - XS_STRING_KIND;
		the->stack->value.key.sum = 1;
	}
	length = offset + fxGetDataViewSize(the, view, buffer);
	while (offset < limit) {
		if (comma) {
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
            size = fxAddChunkSizes(the, size, slot->value.key.sum);
		}
		else
			comma = 1;
		if (offset < length) {
			mxPushUndefined();
			(*dispatch->value.typedArray.dispatch->getter)(the, buffer->value.reference->next, offset, the->stack, EndianNative);
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
			string = fxToString(the, slot);
			slot->kind += XS_KEY_KIND - XS_STRING_KIND;
			slot->value.key.sum = mxStringLength(string);
			size = fxAddChunkSizes(the, size, slot->value.key.sum);
			mxPop();
		}
		offset += delta;
	}
	mxPop();
	string = mxResult->value.string = fxNewChunk(the, fxAddChunkSizes(the, size, 1));
	slot = list->next;
	while (slot) {
		c_memcpy(string, slot->value.key.string, slot->value.key.sum);
		string += slot->value.key.sum;
		slot = slot->next;
	}
	*string = 0;
	mxResult->kind = XS_STRING_KIND;
	mxPop();
}

void fx_TypedArray_prototype_keys(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txSlot* property;
	fxCheckDataViewSize(the, view, buffer, XS_IMMUTABLE);
	mxPush(mxArrayIteratorPrototype);
	property = fxLastProperty(the, fxNewIteratorInstance(the, mxThis, mxID(_Array)));
	property = fxNextIntegerProperty(the, property, 1, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPullSlot(mxResult);
}

void fx_TypedArray_prototype_lastIndexOf(txMachine* the)
{
	mxTypedArrayDeclarations;
	fxInteger(the, mxResult, -1);
	if (length) {
		txIndex index = (txIndex)fxArgToLastIndex(the, 1, length, length);
		txSlot* argument;
		if (mxArgc > 0)
			mxPushSlot(mxArgv(0));
		else
			mxPushUndefined();
		argument = the->stack;
		while (index > 0) {
			index--;
			mxPushSlot(mxThis);
			if (fxHasIndex(the, index)) {
				mxPushSlot(mxThis);
				mxGetIndex(index);
				if (fxIsSameSlot(the, the->stack++, argument)) {
					mxResult->value.integer = index;
					break;
				}
			}
			mxCheckMetering();
		}
		mxPop();
	}
}

void fx_TypedArray_prototype_length_get(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txU2 shift = dispatch->value.typedArray.dispatch->shift;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = fxGetDataViewSize(the, view, buffer) >> shift;
}

void fx_TypedArray_prototype_map(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	fxCreateTypedArraySpecies(the);
	mxPushNumber(length);
	mxRunCount(1);
	mxPullSlot(mxResult);
	{
		mxResultTypedArrayDeclarations;
		txU2 shift = resultDispatch->value.typedArray.dispatch->shift;
		txInteger index = 0;
		if (resultLength < length)
			mxTypeError("result: too small TypedArray instance");
		while (index < length) {
			fxCallTypedArrayItem(the, function, dispatch, view, buffer->value.reference->next, index, C_NULL);
			if (resultBuffer->value.arrayBuffer.address == C_NULL)
				mxTypeError("detached buffer");
			(*resultDispatch->value.typedArray.dispatch->coerce)(the, the->stack);
			(*resultDispatch->value.typedArray.dispatch->setter)(the, resultBuffer->value.reference->next, resultView->value.dataView.offset + (index << shift), the->stack, EndianNative);
			mxPop();
			index++;
		}
	}
}

void fx_TypedArray_prototype_reduce(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	if (mxArgc > 1)
		*mxResult = *mxArgv(1);
	else if (index < length) {
		(*dispatch->value.typedArray.dispatch->getter)(the, buffer->value.reference->next, view->value.dataView.offset, mxResult, EndianNative);
		index++;
	}
	else
		mxTypeError("no initial value");
	while (index < length) {
		fxReduceTypedArrayItem(the, function, dispatch, view, buffer->value.reference->next, index);
		index++;
	}
}

void fx_TypedArray_prototype_reduceRight(txMachine* the)
{
	mxTypedArrayDeclarations;
	txInteger delta = dispatch->value.typedArray.dispatch->size;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = length - 1;
	if (mxArgc > 1)
		*mxResult = *mxArgv(1);
	else if (index >= 0) {
		(*dispatch->value.typedArray.dispatch->getter)(the, buffer->value.reference->next, view->value.dataView.offset + (index * delta), mxResult, EndianNative);
		index--;
	}
	else
		mxTypeError("no initial value");
	while (index >= 0) {
		fxReduceTypedArrayItem(the, function, dispatch, view, buffer->value.reference->next, index);
		index--;
	}
}

void fx_TypedArray_prototype_reverse(txMachine* the)
{
	mxMutableTypedArrayDeclarations;
	txInteger delta = dispatch->value.typedArray.dispatch->size;
	if (length) {
		txByte tmp;
		txByte* first = buffer->value.reference->next->value.arrayBuffer.address + view->value.dataView.offset;
		txByte* last = first + (length << dispatch->value.typedArray.dispatch->shift) - delta;
		txInteger offset;
		while (first < last) {
			for (offset = 0; offset < delta; offset++) {
				tmp = last[offset];
				last[offset] = first[offset];
				first[offset] = tmp;
			}
			first += delta;
			last -= delta;
		}
		mxMeterSome(length * 4);
	}
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_TypedArray_prototype_set(txMachine* the)
{
	mxMutableTypedArrayDeclarations;
	txSlot* data = buffer->value.reference->next;
	txInteger delta = dispatch->value.typedArray.dispatch->size;
	txSlot* source = fxArgToInstance(the, 0);
	txInteger target = fxArgToByteLength(the, 1, 0);
	txInteger offset = view->value.dataView.offset + (target * delta);	
	if (source->next && (source->next->kind == XS_TYPED_ARRAY_KIND)) {
		txSlot* sourceDispatch = source->next;
		txSlot* sourceView = sourceDispatch->next;
		txSlot* sourceBuffer = sourceView->next;
		txU2 shift = sourceDispatch->value.typedArray.dispatch->shift;
		txInteger sourceLength = fxCheckDataViewSize(the, sourceView, sourceBuffer, XS_IMMUTABLE) >> shift;
		txInteger sourceOffset = sourceView->value.dataView.offset;	
		txSlot* sourceData = sourceBuffer->value.reference->next;
		txInteger limit = offset + (sourceLength * delta);
		if ((length - sourceLength < target))		//@@ target can never be negative?
			mxRangeError("invalid offset");
		if (data == sourceData) {
			txSlot* resultBuffer;
			mxPush(mxArrayBufferConstructor);
			mxNew();
			mxPushInteger(sourceLength << shift);
			mxRunCount(1);
			resultBuffer = the->stack->value.reference->next;
			c_memcpy(resultBuffer->value.arrayBuffer.address, sourceData->value.arrayBuffer.address + sourceOffset, sourceLength << shift);
			sourceData = resultBuffer;
			sourceOffset = 0;
		}
		else 
			mxPushUndefined();
		if (dispatch->value.typedArray.dispatch == sourceDispatch->value.typedArray.dispatch) {
            if (data->value.arrayBuffer.address == C_NULL)
                mxTypeError("detached buffer");
			c_memcpy(data->value.arrayBuffer.address + offset, sourceData->value.arrayBuffer.address + sourceOffset, limit - offset);
			mxMeterSome(((txU4)(limit - offset)) * 2);
		}
		else {
			txInteger sourceDelta = 1 << shift;
			mxPushUndefined();
			while (offset < limit) {
				(*sourceDispatch->value.typedArray.dispatch->getter)(the, sourceData, sourceOffset, the->stack, EndianNative);
				(*dispatch->value.typedArray.dispatch->coerce)(the, the->stack);
                if (data->value.arrayBuffer.address == C_NULL)
                    mxTypeError("detached buffer");
				(*dispatch->value.typedArray.dispatch->setter)(the, data, offset, the->stack, EndianNative);
				sourceOffset += sourceDelta;
				offset += delta;
			}
			mxPop();
		}
		mxPop();
	}
	else {
		txInteger count, index;
		if (fxIsDataViewOutOfBound(the, view, buffer))
			mxTypeError("out of bound buffer");
		mxPushSlot(mxArgv(0));
		mxGetID(mxID(_length));
		count = fxToInteger(the, the->stack);
		mxPop();
		if (length - count < target)
			mxRangeError("invalid offset");
		index = 0;
		while (index < count) {
			mxPushSlot(mxArgv(0));
			mxGetIndex(index);
			mxPushSlot(mxThis);
			mxPushInteger(target + index);
			mxSetAt();
			mxPop();
			index++;
			mxCheckMetering();
		}	
	}
}

void fx_TypedArray_prototype_slice(txMachine* the)
{
	mxTypedArrayDeclarations;
	txInteger delta = dispatch->value.typedArray.dispatch->size;
	txInteger start = fxArgToIndexInteger(the, 0, 0, length);
	txInteger end = fxArgToIndexInteger(the, 1, length, length);
	txInteger count = (end > start) ? end - start : 0;
	txInteger index = 0;
	fxCreateTypedArraySpecies(the);
	mxPushNumber(count);
	mxRunCount(1);
	mxPullSlot(mxResult);
	{
		mxResultTypedArrayDeclarations;
		if (resultLength < count)
			mxTypeError("result: too small TypedArray instance");
		if (count) {
			length = fxCheckDataViewSize(the, view, buffer, XS_IMMUTABLE) >> dispatch->value.typedArray.dispatch->shift;
			if ((end <= length) && (resultDispatch->value.typedArray.dispatch->constructorID == dispatch->value.typedArray.dispatch->constructorID) && (resultBuffer->value.reference != buffer->value.reference)) {
				txInteger shift = dispatch->value.typedArray.dispatch->shift;
				txSlot* data = buffer->value.reference->next;
				txSlot* resultData = resultBuffer->value.reference->next;
				txByte* address = data->value.arrayBuffer.address;
				txByte* resultAddress = resultData->value.arrayBuffer.address;
				address += view->value.dataView.offset;
				resultAddress += resultView->value.dataView.offset;
				c_memmove(resultAddress, address + (start << shift), count << shift);
				mxMeterSome(((txU4)(count)) * 2);
			}
			else {
				mxPushUndefined();
				while ((start < length) && (start < end)) {
					(*dispatch->value.typedArray.dispatch->getter)(the, buffer->value.reference->next, view->value.dataView.offset + (start * delta), the->stack, EndianNative);
					(*resultDispatch->value.typedArray.dispatch->coerce)(the, the->stack);
					(*resultDispatch->value.typedArray.dispatch->setter)(the, resultBuffer->value.reference->next, resultView->value.dataView.offset + (index << resultDispatch->value.typedArray.dispatch->shift), the->stack, EndianNative);
					start++;
					index++;
				}
				mxPop();
			}
		}
	}
}

void fx_TypedArray_prototype_some(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, buffer->value.reference->next, index, C_NULL);
		mxResult->value.boolean = fxToBoolean(the, the->stack++);
		if (mxResult->value.boolean)
			break;
		index++;
	}
}

void fx_TypedArray_prototype_sort(txMachine* the)
{
	mxMutableTypedArrayDeclarations;
	txSlot* data = buffer->value.reference->next;
	txInteger delta = dispatch->value.typedArray.dispatch->size;
	txSlot* function = C_NULL;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind != XS_UNDEFINED_KIND) {
			if (fxIsCallable(the, slot))
				function = slot;
			else
				mxTypeError("compare: not a function");
		}
	}
	if (function)
		fxSortArrayItems(the, function, C_NULL, length, mxThis);
	else
		c_qsort(data->value.arrayBuffer.address + view->value.dataView.offset, length, delta, dispatch->value.typedArray.dispatch->compare);
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_TypedArray_prototype_subarray(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
    txU2 shift = dispatch->value.typedArray.dispatch->shift;
    txInteger length = fxGetDataViewSize(the, view, buffer) >> shift;
	txInteger start = fxArgToIndexInteger(the, 0, 0, length);
	if ((view->value.dataView.size < 0) && ((mxArgc < 2) || mxIsUndefined(mxArgv(1)))) {
		fxCreateTypedArraySpecies(the);
		mxPushSlot(buffer);
		mxPushInteger(view->value.dataView.offset + (start << shift));
		mxRunCount(2);
	}
	else {
		txInteger stop = fxArgToIndexInteger(the, 1, length, length);
		if (stop < start) 
			stop = start;
		fxCreateTypedArraySpecies(the);
		mxPushSlot(buffer);
		mxPushInteger(view->value.dataView.offset + (start << shift));
		mxPushInteger(stop - start);
		mxRunCount(3);
	}
	mxPullSlot(mxResult);
	fxCheckTypedArrayInstance(the, mxResult);
}

void fx_TypedArray_prototype_toLocaleString(txMachine* the)
{
	mxTypedArrayDeclarations;
	txInteger index = 0;
	txString string;
	txSlot* list = fxNewInstance(the);
	txSlot* slot = list;
	txBoolean comma = 0;
	txInteger size = 0;
	mxPushStringX(",");
	the->stack->kind += XS_KEY_KIND - XS_STRING_KIND;
	the->stack->value.key.sum = 1;
	while (index < length) {
		if (comma) {
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
			size += slot->value.key.sum;
		}
		else
			comma = 1;
		mxPushSlot(mxThis);
		mxGetIndex(index);
		if ((the->stack->kind != XS_UNDEFINED_KIND) && (the->stack->kind != XS_NULL_KIND)) {
			mxDub();
			mxGetID(mxID(_toLocaleString));
			mxCall();
			mxRunCount(0);
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
			string = fxToString(the, slot);
			slot->kind += XS_KEY_KIND - XS_STRING_KIND;
			slot->value.key.sum = mxStringLength(string);
			size = fxAddChunkSizes(the, size, slot->value.key.sum);
		}
		mxPop();
		index++;
	}
	string = mxResult->value.string = fxNewChunk(the, fxAddChunkSizes(the, size, 1));
	slot = list->next;
	while (slot) {
		c_memcpy(string, slot->value.key.string, slot->value.key.sum);
		string += slot->value.key.sum;
		slot = slot->next;
	}
	*string = 0;
	mxResult->kind = XS_STRING_KIND;
	mxPop();
}

void fx_TypedArray_prototype_toReversed(txMachine* the)
{
	mxTypedArrayDeclarations;
	txInteger delta = dispatch->value.typedArray.dispatch->size;
	txSlot* constructor = &the->stackIntrinsics[-1 - (txInteger)dispatch->value.typedArray.dispatch->constructorID];
	mxPushSlot(constructor);
	mxNew();
	mxPushInteger(length);
	mxRunCount(1);
	mxPullSlot(mxResult);
	if (length) {
		mxResultTypedArrayDeclarations;
		txByte* base = buffer->value.reference->next->value.arrayBuffer.address + view->value.dataView.offset;
		txByte* from = base + (resultLength << dispatch->value.typedArray.dispatch->shift) - delta;
		txByte* to = resultBuffer->value.reference->next->value.arrayBuffer.address;
		while (from >= base) {
			txInteger offset;
			for (offset = 0; offset < delta; offset++)
				*to++ = *from++;
			from -= delta << 1;
		}
		mxMeterSome((txU4)length * 4);
	}
}

void fx_TypedArray_prototype_toSorted(txMachine* the)
{
	mxMutableTypedArrayDeclarations;
	txInteger delta = dispatch->value.typedArray.dispatch->size;
	txSlot* constructor = &the->stackIntrinsics[-1 - (txInteger)dispatch->value.typedArray.dispatch->constructorID];
	txSlot* function = C_NULL;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind != XS_UNDEFINED_KIND) {
			if (fxIsCallable(the, slot))
				function = slot;
			else
				mxTypeError("compare: not a function");
		}
	}
	mxPushSlot(constructor);
	mxNew();
	mxPushInteger(length);
	mxRunCount(1);
	mxPullSlot(mxResult);
	if (function)
		fxSortArrayItems(the, function, C_NULL, length, mxResult);
	else {
		mxResultTypedArrayDeclarations;
		txByte* from = buffer->value.reference->next->value.arrayBuffer.address + view->value.dataView.offset;
		txByte* to = resultBuffer->value.reference->next->value.arrayBuffer.address;
		c_memcpy(to, from, 	resultLength << dispatch->value.typedArray.dispatch->shift);		
		c_qsort(to, length, delta, dispatch->value.typedArray.dispatch->compare);
	}
}

void fx_TypedArray_prototype_toStringTag_get(txMachine* the)
{
	if (mxThis->kind == XS_REFERENCE_KIND) {
        txSlot* instance = mxThis->value.reference;
        txSlot* slot = instance->next;
		if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_TYPED_ARRAY_KIND)) {
			txTypeDispatch *dispatch = instance->next->value.typedArray.dispatch;
			txSlot* key = fxGetKey(the, mxID(dispatch->constructorID));
			if (key->kind == XS_KEY_X_KIND)
				mxResult->kind = XS_STRING_X_KIND;
			else
				mxResult->kind = XS_STRING_KIND;
			mxResult->value.string = key->value.key.string;
		}
	}
}

void fx_TypedArray_prototype_values(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txSlot* property;
	fxCheckDataViewSize(the, view, buffer, XS_IMMUTABLE);
	mxPush(mxArrayIteratorPrototype);
	property = fxLastProperty(the, fxNewIteratorInstance(the, mxThis, mxID(_Array)));
	property = fxNextIntegerProperty(the, property, 0, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPullSlot(mxResult);
}

void fx_TypedArray_prototype_with(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* constructor = &the->stackIntrinsics[-1 - (txInteger)dispatch->value.typedArray.dispatch->constructorID];
	txInteger index = (txInteger)fxArgToRelativeIndex(the, 0, 0, length), count, i;
	txSlot* value;
	if (mxArgc > 1)
		mxPushSlot(mxArgv(1));
	else
		mxPushUndefined();
	value = the->stack;	
	(*dispatch->value.typedArray.dispatch->coerce)(the, value);
	count = fxGetDataViewSize(the, view, buffer) >> dispatch->value.typedArray.dispatch->shift;
	if ((index < 0) || (count <= index))
		mxRangeError("invalid index");
	mxPushSlot(constructor);
	mxNew();
	mxPushInteger(length);
	mxRunCount(1);
	mxPullSlot(mxResult);
	i = 0;
	while (i < index) {
		mxPushSlot(mxThis);
		mxPushInteger(i);
		mxGetAt();
		mxPushSlot(mxResult);
		mxPushInteger(i);
		mxSetAt();
		mxPop();
		i++;
		mxCheckMetering();
	}
	mxPushSlot(value);
	mxPushSlot(mxResult);
	mxPushInteger(i);
	mxSetAt();
	mxPop();
	i++;
	while (i < length) {
		mxPushSlot(mxThis);
		mxPushInteger(i);
		mxGetAt();
		mxPushSlot(mxResult);
		mxPushInteger(i);
		mxSetAt();
		mxPop();
		i++;
		mxCheckMetering();
	}
	mxPop();
}

#if mxBigEndian
	#define mxEndianFloat16_BtoN(a) (a)
	#define mxEndianFloat32_BtoN(a) (a)
	#define mxEndianFloat64_BtoN(a) (a)
	#define mxEndianS64_BtoN(a) (a)
	#define mxEndianU64_BtoN(a) (a)
	#define mxEndianS32_BtoN(a) (a)
	#define mxEndianU32_BtoN(a) (a)
	#define mxEndianS16_BtoN(a) (a)
	#define mxEndianU16_BtoN(a) (a)

	#define mxEndianFloat16_NtoB(a) (a)
	#define mxEndianFloat32_NtoB(a) (a)
	#define mxEndianFloat64_NtoB(a) (a)
	#define mxEndianS64_NtoB(a) (a)
	#define mxEndianU64_NtoB(a) (a)
	#define mxEndianS32_NtoB(a) (a)
	#define mxEndianU32_NtoB(a) (a)
	#define mxEndianS16_NtoB(a) (a)
	#define mxEndianU16_NtoB(a) (a)
#else
	#define mxEndianFloat16_LtoN(a) (a)
	#define mxEndianFloat32_LtoN(a) (a)
	#define mxEndianFloat64_LtoN(a) (a)
	#define mxEndianS64_LtoN(a) (a)
	#define mxEndianU64_LtoN(a) (a)
	#define mxEndianS32_LtoN(a) (a)
	#define mxEndianU32_LtoN(a) (a)
	#define mxEndianS16_LtoN(a) (a)
	#define mxEndianU16_LtoN(a) (a)

	#define mxEndianFloat16_NtoL(a) (a)
	#define mxEndianFloat32_NtoL(a) (a)
	#define mxEndianFloat64_NtoL(a) (a)
	#define mxEndianS64_NtoL(a) (a)
	#define mxEndianU64_NtoL(a) (a)
	#define mxEndianS32_NtoL(a) (a)
	#define mxEndianU32_NtoL(a) (a)
	#define mxEndianS16_NtoL(a) (a)
	#define mxEndianU16_NtoL(a) (a)
#endif

#if mxLittleEndian
	#define mxEndianFloat16_BtoN(a) (mxEndianFloat16_Swap(a))
	#define mxEndianFloat32_BtoN(a) (mxEndianFloat32_Swap(a))
	#define mxEndianFloat64_BtoN(a) (mxEndianFloat64_Swap(a))
	#define mxEndianS64_BtoN(a) ((txS8) mxEndian64_Swap(a))
	#define mxEndianU64_BtoN(a) ((txU8) mxEndian64_Swap(a))
	#define mxEndianS32_BtoN(a) ((txS4) mxEndian32_Swap(a))
	#define mxEndianU32_BtoN(a) ((txU4) mxEndian32_Swap(a))
	#define mxEndianS16_BtoN(a) ((txS2) mxEndian16_Swap(a))
	#define mxEndianU16_BtoN(a) ((txU2) mxEndian16_Swap(a))

	#define mxEndianFloat16_NtoB(a) (mxEndianFloat16_Swap(a))
	#define mxEndianFloat32_NtoB(a) (mxEndianFloat32_Swap(a))
	#define mxEndianFloat64_NtoB(a) (mxEndianFloat64_Swap(a))
	#define mxEndianS64_NtoB(a) ((txS8) mxEndian64_Swap(a))
	#define mxEndianU64_NtoB(a) ((txU8) mxEndian64_Swap(a))
	#define mxEndianS32_NtoB(a) ((txS4) mxEndian32_Swap(a))
	#define mxEndianU32_NtoB(a) ((txU4) mxEndian32_Swap(a))
	#define mxEndianS16_NtoB(a) ((txS2) mxEndian16_Swap(a))
	#define mxEndianU16_NtoB(a) ((txU2) mxEndian16_Swap(a))
#else
	#define mxEndianFloat16_LtoN(a) (mxEndianFloat16_Swap(a))
	#define mxEndianFloat32_LtoN(a) (mxEndianFloat32_Swap(a))
	#define mxEndianFloat64_LtoN(a) (mxEndianFloat64_Swap(a))
	#define mxEndianS64_LtoN(a) ((txS8) mxEndian64_Swap(a))
	#define mxEndianU64_LtoN(a) ((txU8) mxEndian64_Swap(a))
	#define mxEndianS32_LtoN(a) ((txS4) mxEndian32_Swap(a))
	#define mxEndianU32_LtoN(a) ((txU4) mxEndian32_Swap(a))
	#define mxEndianS16_LtoN(a) ((txS2) mxEndian16_Swap(a))
	#define mxEndianU16_LtoN(a) ((txU2) mxEndian16_Swap(a))

	#define mxEndianFloat16_NtoL(a) (mxEndianFloat16_Swap(a))
	#define mxEndianFloat32_NtoL(a) (mxEndianFloat32_Swap(a))
	#define mxEndianFloat64_NtoL(a) (mxEndianFloat64_Swap(a))
	#define mxEndianS64_NtoL(a) ((txS8) mxEndian64_Swap(a))
	#define mxEndianU64_NtoL(a) ((txU8) mxEndian64_Swap(a))
	#define mxEndianS32_NtoL(a) ((txS4) mxEndian32_Swap(a))
	#define mxEndianU32_NtoL(a) ((txU4) mxEndian32_Swap(a))
	#define mxEndianS16_NtoL(a) ((txS2) mxEndian16_Swap(a))
	#define mxEndianU16_NtoL(a) ((txU2) mxEndian16_Swap(a))
#endif

#if defined(__GNUC__) || defined(__llvm__)
	#define mxEndian16_Swap(a) __builtin_bswap16(a)
#else
	static txU2 mxEndian16_Swap(txU2 a)
	{
		txU2 b;
		txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
		int i;
		for (i = 0; i < 2; i++)
			p2[i] = p1[1 - i];
		return b;
	}
#endif

#if defined(__GNUC__) || defined(__llvm__)
	#define mxEndian32_Swap(a) __builtin_bswap32(a)
#else
	static txU4 mxEndian32_Swap(txU4 a)
	{
		txU4 b;
		txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
		int i;
		for (i = 0; i < 4; i++)
			p2[i] = p1[3 - i];
		return b;
	}
#endif

#if defined(__GNUC__) || defined(__llvm__)
	#define mxEndian64_Swap(a) __builtin_bswap64(a)
#else
	static txU8 mxEndian64_Swap(txU8 a)
	{
		txU8 b;
		txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
		int i;
		for (i = 0; i < 8; i++)
			p2[i] = p1[7 - i];
		return b;
	}
#endif

#define toNative(size, endian) mxEndian##size##_##endian##toN
#define fromNative(size, endian) mxEndian##size##_Nto##endian
#define IMPORT(size) (endian == EndianBig ? toNative(size, B)(value) : endian == EndianLittle ? toNative(size, L)(value) : (value))
#define EXPORT(size) (endian == EndianBig ? fromNative(size, B)(value) : endian == EndianLittle ? toNative(size, L)(value) : (value))

int fxBigInt64Compare(const void* p, const void* q)
{
	txS8 a = *((txS8*)p);
	txS8 b = *((txS8*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxBigInt64Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS8 value;
#ifdef mxMisalignedSettersCrash
	c_memcpy(&value, data->value.arrayBuffer.address + offset, sizeof(value));
#else
	value = *((txS8*)(data->value.arrayBuffer.address + offset));
#endif
	value = IMPORT(S64);
	fxFromBigInt64(the, slot, value);
	mxMeterOne();
}

void fxBigInt64Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS8 value = (txS8)fxToBigInt64(the, slot);
#ifdef mxMisalignedSettersCrash
	value = EXPORT(S64);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(txS8));
#else
	*((txS8*)(data->value.arrayBuffer.address + offset)) = EXPORT(S64);
#endif
	mxMeterOne();
}

int fxBigUint64Compare(const void* p, const void* q)
{
	txU8 a = *((txU8*)p);
	txU8 b = *((txU8*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxBigUint64Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txU8 value;
#ifdef mxMisalignedSettersCrash
	c_memcpy(&value, data->value.arrayBuffer.address + offset, sizeof(value));
#else
	value = *((txU8*)(data->value.arrayBuffer.address + offset));
#endif
	value = IMPORT(U64);
	fxFromBigUint64(the, slot, value);
	mxMeterOne();
}

void fxBigUint64Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txU8 value = (txU8)fxToBigUint64(the, slot);
#ifdef mxMisalignedSettersCrash
	value = EXPORT(U64);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(txU8));
#else
	*((txU8*)(data->value.arrayBuffer.address + offset)) = EXPORT(U64);
#endif
	mxMeterOne();
}

#if mxFloat16

#ifdef mxUseFloat16
typedef _Float16 txFloat16;
#define mxFloat16to64(X) ((double)X) 
#define mxFloat64to16(X) ((_Float16)X) 
#else
typedef uint16_t txFloat16;
#define mxFloat16to64(X) fxFloat16to64(X) 
#define mxFloat64to16(X) fxFloat64to16(X) 

// adapted from https://half.sourceforge.net

static double fxFloat16to64(uint16_t value)
{
	uint32_t hi, abs;
	uint64_t dbits;
	double out;
	
	hi = (uint32_t)(value & 0x8000) << 16;
	abs = value & 0x7FFF;
	if (abs) {
		hi |= 0x3F000000 << (abs >= 0x7C00);
		for (; abs < 0x400; abs <<= 1, hi -= 0x100000);
		hi += abs << 10;
	}
	dbits = (uint64_t)(hi) << 32;
	c_memcpy(&out, &dbits, sizeof(double));
	return out;
}

static uint32_t fxRoundFloat16(uint32_t value, uint32_t g, uint32_t s) {
  return value + (g & (s | value));
}

static uint16_t fxFloat64to16(double value)
{
	uint64_t dbits;
	uint32_t hi, lo, sign;
	
	c_memcpy(&dbits, &value, sizeof(double));
	hi = dbits >> 32;
	lo = dbits & 0xFFFFFFFF;
	sign = (hi >> 16) & 0x8000;
	hi &= 0x7FFFFFFF;
	if (hi >= 0x7FF00000)
		return sign | 0x7C00 | ((dbits & 0xFFFFFFFFFFFFF) ? (0x200 | ((hi >> 10) & 0x3FF)) : 0);
	if (hi >= 0x40F00000)
		return sign | 0x7C00;
	if (hi >= 0x3F100000)
		return fxRoundFloat16(sign | (((hi >> 20) - 1008) << 10) | ((hi >> 10) & 0x3FF), (hi >> 9) & 1, ((hi & 0x1FF) | lo) != 0);
	if (hi >= 0x3E600000) {
		int i = 1018 - (hi >> 20);
		hi = (hi & 0xFFFFF) | 0x100000;
		return fxRoundFloat16(sign | (hi >> (i + 1)), (hi >> i) & 1, ((hi & (((uint32_t)(1) << i) - 1)) | lo) != 0);
	}
	return sign;
}

#endif

#if mxCanonicalNaN
	#if mxBigEndian
		static uint8_t gxCanonicalNaN16Bytes[2] = { 0x7E, 0 };
	#else
		static uint8_t gxCanonicalNaN16Bytes[2] = { 0, 0x7E };
	#endif
	static txFloat16* gxCanonicalNaN16 = (txFloat16*)gxCanonicalNaN16Bytes;
#endif

static txFloat16 mxEndianFloat16_Swap(txFloat16 a)
{
#if defined(__GNUC__) || defined(__llvm__)
	uint32_t result = __builtin_bswap16(*(uint16_t *)&a);
	return *(txFloat16 *)&result;
#else
	txFloat16 b;
	txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
	int i;
	for (i = 0; i < 2; i++)
		p2[i] = p1[1 - i];
	return b;
#endif
}

int fxFloat16Compare(const void* p, const void* q)
{
	double a = mxFloat16to64(*((txFloat16*)p));
	double b = mxFloat16to64(*((txFloat16*)q));
	if (c_isnan(a)) {
		if (c_isnan(b)) 
			return 0;
		return 1;
	}
	if (c_isnan(b))
		return -1;
	if (a < b)
		return -1;
	if (a > b)
		return 1;
	if (a == 0) {
		if (c_signbit(a)) {
			if (c_signbit(b)) 
				return 0;
			return -1;
		}
		if (c_signbit(b))
			return 1;
	}
	return 0;
}

void fxFloat16Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txFloat16 value;
	slot->kind = XS_NUMBER_KIND;
#ifdef mxMisalignedSettersCrash
	c_memcpy(&value, data->value.arrayBuffer.address + offset, sizeof(value));
#else
	value = *((txFloat16*)(data->value.arrayBuffer.address + offset));
#endif
	slot->value.number = mxFloat16to64(IMPORT(Float16));
	mxMeterOne();
}

void fxFloat16Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
#if mxCanonicalNaN
	txFloat16 value = (c_isnan(slot->value.number)) ? *gxCanonicalNaN16 : mxFloat64to16(slot->value.number);
#else
	txFloat16 value = mxFloat64to16(slot->value.number);
#endif
#ifdef mxMisalignedSettersCrash
	value = EXPORT(Float16);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(value));
#else
	*((txFloat16*)(data->value.arrayBuffer.address + offset)) = EXPORT(Float16);
#endif
	mxMeterOne();
}

void fx_Math_f16round(txMachine* the)
{
	txFloat16 value = mxFloat64to16((mxArgc < 1) ? C_NAN : fxToNumber(the, mxArgv(0)));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = mxFloat16to64(value);
}

#endif

#if mxCanonicalNaN
	#if mxBigEndian
		static uint8_t gxCanonicalNaN32Bytes[4] = { 0x7F, 0xC0, 0, 0 };
	#else
		static uint8_t gxCanonicalNaN32Bytes[4] = { 0, 0, 0xC0, 0x7F };
	#endif
	static float* gxCanonicalNaN32 = (float*)gxCanonicalNaN32Bytes;
#endif

static float mxEndianFloat32_Swap(float a)
{
#if defined(__GNUC__) || defined(__llvm__)
	uint32_t result = __builtin_bswap32(*(uint32_t *)&a);
	return *(float *)&result;
#else
	float b;
	txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
	int i;
	for (i = 0; i < 4; i++)
		p2[i] = p1[3 - i];
	return b;
#endif
}

int fxFloat32Compare(const void* p, const void* q)
{
	float a = *((float*)p);
	float b = *((float*)q);
	if (c_isnan(a)) {
		if (c_isnan(b)) 
			return 0;
		return 1;
	}
	if (c_isnan(b))
		return -1;
	if (a < b)
		return -1;
	if (a > b)
		return 1;
	if (a == 0) {
		if (c_signbit(a)) {
			if (c_signbit(b)) 
				return 0;
			return -1;
		}
		if (c_signbit(b))
			return 1;
	}
	return 0;
}

void fxFloat32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	float value;
	slot->kind = XS_NUMBER_KIND;
#ifdef mxMisalignedSettersCrash
	c_memcpy(&value, data->value.arrayBuffer.address + offset, sizeof(value));
#else
	value = *((float*)(data->value.arrayBuffer.address + offset));
#endif
	slot->value.number = IMPORT(Float32);
	mxMeterOne();
}

void fxFloat32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
#if mxCanonicalNaN
	float value = (c_isnan(slot->value.number)) ? *gxCanonicalNaN32 : (float)slot->value.number;
#else
	float value = (float)slot->value.number;
#endif
#ifdef mxMisalignedSettersCrash
	value = EXPORT(Float32);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(value));
#else
	*((float*)(data->value.arrayBuffer.address + offset)) = EXPORT(Float32);
#endif
	mxMeterOne();
}

#if mxCanonicalNaN
	#if mxBigEndian
		static uint8_t gxCanonicalNaN64Bytes[8] = { 0x7F, 0xF8, 0, 0, 0, 0, 0, 0 };
	#else
		static uint8_t gxCanonicalNaN64Bytes[8] = { 0, 0, 0, 0, 0, 0, 0xF8, 0x7F };
	#endif
	double* gxCanonicalNaN64 = (double*)gxCanonicalNaN64Bytes;
#endif

static double mxEndianFloat64_Swap(double a)
{
#if defined(__GNUC__) || defined(__llvm__)
	uint64_t result = __builtin_bswap64(*(uint64_t *)&a);
	return *(double *)&result;
#else
	double b;
	txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
	int i;
	for (i = 0; i < 8; i++)
		p2[i] = p1[7 - i];
	return b;
#endif
}

int fxFloat64Compare(const void* p, const void* q)
{
	double a = *((double*)p);
	double b = *((double*)q);
	if (c_isnan(a)) {
		if (c_isnan(b)) 
			return 0;
		return 1;
	}
	if (c_isnan(b))
		return -1;
	if (a < b)
		return -1;
	if (a > b)
		return 1;
	if (a == 0) {
		if (c_signbit(a)) {
			if (c_signbit(b)) 
				return 0;
			return -1;
		}
		if (c_signbit(b))
			return 1;
	}
	return 0;
}

void fxFloat64Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	double value;
	slot->kind = XS_NUMBER_KIND;
#ifdef mxMisalignedSettersCrash
	c_memcpy(&value, data->value.arrayBuffer.address + offset, sizeof(value));
#else
	value = *((double*)(data->value.arrayBuffer.address + offset));
#endif
	slot->value.number = IMPORT(Float64);
	mxMeterOne();
}

void fxFloat64Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
#if mxCanonicalNaN
	double value = (c_isnan(slot->value.number)) ? *gxCanonicalNaN32 : slot->value.number;
#else
	double value = slot->value.number;
#endif
#ifdef mxMisalignedSettersCrash
	value = EXPORT(Float64);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(value));
#else
	*((double*)(data->value.arrayBuffer.address + offset)) = EXPORT(Float64);
#endif
	mxMeterOne();
}

void fxIntCoerce(txMachine* the, txSlot* slot)
{
	fxToInteger(the, slot);
}

void fxUintCoerce(txMachine* the, txSlot* slot)
{
	fxToUnsigned(the, slot);
}

int fxInt8Compare(const void* p, const void* q)
{
	txS1 a = *((txS1*)p);
	txS1 b = *((txS1*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxInt8Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	slot->kind = XS_INTEGER_KIND;
	slot->value.integer = *((txS1*)(data->value.arrayBuffer.address + offset));
	mxMeterOne();
}

void fxInt8Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	*((txS1*)(data->value.arrayBuffer.address + offset)) = (txS1)slot->value.integer;
	mxMeterOne();
}

int fxInt16Compare(const void* p, const void* q)
{
	txS2 a = *((txS2*)p);
	txS2 b = *((txS2*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxInt16Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS2 value;
	slot->kind = XS_INTEGER_KIND;
#ifdef mxMisalignedSettersCrash
	c_memcpy(&value, data->value.arrayBuffer.address + offset, sizeof(value));
#else
	value = *((txS2*)(data->value.arrayBuffer.address + offset));
#endif
	slot->value.integer = IMPORT(S16);
	mxMeterOne();
}

void fxInt16Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS2 value = (txS2)slot->value.integer;
#ifdef mxMisalignedSettersCrash
	value = EXPORT(S16);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(txS2));
#else
	*((txS2*)(data->value.arrayBuffer.address + offset)) = EXPORT(S16);
#endif
	mxMeterOne();
}

int fxInt32Compare(const void* p, const void* q)
{
	txS4 a = *((txS4*)p);
	txS4 b = *((txS4*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxInt32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS4 value;
	slot->kind = XS_INTEGER_KIND;
#ifdef mxMisalignedSettersCrash
	value = c_read32(data->value.arrayBuffer.address + offset);
#else
	value = *((txS4*)(data->value.arrayBuffer.address + offset));
#endif
	slot->value.integer = IMPORT(S32);
	mxMeterOne();
}

void fxInt32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS4 value = (txS4)slot->value.integer;
#ifdef mxMisalignedSettersCrash
	value = EXPORT(S32);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(txS4));
#else
	*((txS4*)(data->value.arrayBuffer.address + offset)) = EXPORT(S32);
#endif
	mxMeterOne();
}

int fxUint8Compare(const void* p, const void* q)
{
	txU1 a = c_read8((txU1*)p);
	txU1 b = c_read8((txU1*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxUint8Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	slot->kind = XS_INTEGER_KIND;
	slot->value.integer = c_read8((txU1*)(data->value.arrayBuffer.address + offset));
	mxMeterOne();
}

void fxUint8Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txUnsigned tmp = (slot->kind == XS_INTEGER_KIND) ? (txUnsigned)slot->value.integer : (txUnsigned)slot->value.number;
	*((txU1*)(data->value.arrayBuffer.address + offset)) = (txU1)tmp;
	mxMeterOne();
}

int fxUint16Compare(const void* p, const void* q)
{
	txU2 a = *((txU2*)p);
	txU2 b = *((txU2*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxUint16Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txU2 value;
	slot->kind = XS_INTEGER_KIND;
#ifdef mxMisalignedSettersCrash
	c_memcpy(&value, data->value.arrayBuffer.address + offset, sizeof(value));
#else
	value = *((txU2*)(data->value.arrayBuffer.address + offset));
#endif
	slot->value.integer = IMPORT(U16);
	mxMeterOne();
}

void fxUint16Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txUnsigned tmp = (slot->kind == XS_INTEGER_KIND) ? (txUnsigned)slot->value.integer : (txUnsigned)slot->value.number;
	txU2 value = (txU2)tmp;
#ifdef mxMisalignedSettersCrash
	value = EXPORT(U16);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(txU2));
#else
	*((txU2*)(data->value.arrayBuffer.address + offset)) = EXPORT(U16);
#endif
	mxMeterOne();
}

int fxUint32Compare(const void* p, const void* q)
{
	txU4 a = *((txU4*)p);
	txU4 b = *((txU4*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxUint32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
#ifdef mxMisalignedSettersCrash
	txUnsigned value = c_read32(data->value.arrayBuffer.address + offset);
#else
	txUnsigned value = *((txU4*)(data->value.arrayBuffer.address + offset));
#endif
	value = IMPORT(U32);
	if (((txInteger)value) >= 0) {
		slot->kind = XS_INTEGER_KIND;
		slot->value.integer = value;
	}
	else {
		slot->kind = XS_NUMBER_KIND;
		slot->value.number = value;
	}
	mxMeterOne();
}

void fxUint32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txU4 value = (slot->kind == XS_INTEGER_KIND) ? (txU4)slot->value.integer : (txU4)slot->value.number;
#ifdef mxMisalignedSettersCrash
	value = EXPORT(U32);
	c_memcpy(data->value.arrayBuffer.address + offset, &value, sizeof(txU4));
#else
	*((txU4*)(data->value.arrayBuffer.address + offset)) = EXPORT(U32);
#endif
	mxMeterOne();
}

void fxUint8ClampedSetter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txNumber value = fxToNumber(the, slot);
	if (value <= 0)
		value = 0;
	else if (value >= 255)
		value = 255;
	else if (c_isnan(value))
		value = 0;
	else
		value = c_nearbyint(value);
	*((txU1*)(data->value.arrayBuffer.address + offset)) = (txU1)value;
	mxMeterOne();
}

#if mxUint8ArrayBase64

static const char gxBase64Alphabet[] ICACHE_FLASH_ATTR = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char gxBase64URLAlphabet[] ICACHE_FLASH_ATTR = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static const char gxHexAlphabet[] ICACHE_FLASH_ATTR = "0123456789abcdef";

enum {
	mxBase64Loose,
	mxBase64StopBeforePartial,
	mxBase64Strict,
};

static txU1 fxSkipAsciiWhitespace(txU1** src)
{
	txU1* p = *src;
	txU1 c;
	while ((c = c_read8(p))) {
		if ((c != 0x09) && (c != 0x0A) && (c != 0x0C) && (c != 0x0D) && (c != 0x20))
			 break;
		p++;
	}
	*src = p;
	return c;
}

static void fxUint8ArrayFromBase64(txMachine* the, txU1* srcStart, txU1* dstStart, txSize* read, txSize* written, txU1* alphabet, txInteger lastChunkHandling)
{
	txU1* src = srcStart;
	txU1* dst = dstStart;
	txSize remaining = *written;
	txBoolean url = (alphabet == (txU1*)gxBase64URLAlphabet) ? 1 : 0, done = 0;
	txU1 byte, buffer[4];
	txSize bufferIndex, bufferLength = 0;
	
	if (remaining == 0) {
		*read = 0;
		return;
	}
	for (;;) {
		byte = fxSkipAsciiWhitespace(&src);
		if (byte == 0) {
			if (bufferLength == 0)
				break;
			if (lastChunkHandling == mxBase64StopBeforePartial)
				break;
			if ((lastChunkHandling == mxBase64Strict) || (bufferLength == 1))
				mxSyntaxError("invalid string");
			if (bufferLength == 2)
				buffer[2] = 0;
			buffer[3] = 0;
			done = 1;
		}
		else if (byte == '=') {
			if (bufferLength < 2)	
				mxSyntaxError("invalid string");
			src++;
			byte = fxSkipAsciiWhitespace(&src);
			if (bufferLength == 2)	{
				if (byte == 0) {
					if (lastChunkHandling == mxBase64StopBeforePartial)
						break;
					mxSyntaxError("invalid string");
				}
				if (byte == '=') {
					src++;
					byte = fxSkipAsciiWhitespace(&src);
				}
				buffer[2] = 0;
			}
			if (byte != 0)
				mxSyntaxError("invalid string");
			buffer[3] = 0;
			done = 1;
		} 
		else {
			if (('A' <= byte) && (byte <= 'Z'))
				byte = byte - 'A';
			else if (('a' <= byte) && (byte <= 'z'))
				byte = byte - 'a' + 26;
			else if (('0' <= byte) && (byte <= '9'))
				byte = byte - '0' + 52;
			else if ((byte == '+') && !url)
				byte = 62;
			else if ((byte == '-') && url)
				byte = 62;
			else if ((byte == '/') && !url)
				byte = 63;
			else if ((byte == '_') && url)
				byte = 63;
			else
				mxSyntaxError("invalid string");
			if (((remaining == 1) && (bufferLength == 2)) || ((remaining == 2) && (bufferLength == 3)))
				break;
			buffer[bufferLength] = byte;
			bufferLength++;
			src++;
			if (bufferLength < 4)
				continue;
		}
		*read = (txSize)(src - srcStart);
		buffer[0] = (buffer[0] << 2) | ((buffer[1] & 0x30) >> 4);
		buffer[1] = ((buffer[1] & 0x0F) << 4) | ((buffer[2] & 0x3C) >> 2);
		buffer[2] = ((buffer[2] & 0x03) << 6) | (buffer[3] & 0x3F);
		bufferLength--;
		if ((lastChunkHandling == mxBase64Strict) && (bufferLength < 3) && (buffer[bufferLength] != 0))
			mxSyntaxError("invalid string");
		bufferIndex = 0;
		while ((bufferIndex < bufferLength) && (remaining > 0)) {
			*dst++ = buffer[bufferIndex];
			bufferIndex++;
			remaining--;
		}
		bufferLength = 0;
		if ((done) || (remaining == 0))
			break;
	}
	*written = (txSize)(dst - dstStart);
}

static void fxUint8ArrayFromHex(txMachine* the, txU1* srcStart, txU1* dstStart, txSize* read, txSize* written)
{
#define mxFromHex(X) \
	if (('0' <= X) && (X <= '9')) X = X - '0'; \
	else if (('A' <= X) && (X <= 'F')) X = X - 'A' + 10; \
	else if (('a' <= X) && (X <= 'f')) X =  X - 'a' + 10; \
	else mxSyntaxError("invalid string")

	txU1* src = srcStart;
	txU1* dst = dstStart;
	txSize srcSize = *read;
	txSize dstSize = *written;
	while ((srcSize > 0) && (dstSize > 0)) {
		txU1 high = c_read8(src++); 
		txU1 low = c_read8(src++); 
		mxFromHex(high);
		mxFromHex(low);
		*dst++ = (high << 4) + low;
		srcSize -= 2;
		dstSize--;
	}
	*read = (txSize)(src - srcStart);
	*written = (txSize)(dst - dstStart);
}

static void fxUint8ArrayGetBase64Options(txMachine* the, txInteger argi, txU1** alphabet, txInteger* lastChunkHandling, txBoolean* omitPadding)
{
	if ((mxArgc > argi) && !mxIsUndefined(mxArgv(argi))) {
		if (!mxIsReference(mxArgv(argi)))
			mxTypeError("options: not an object");
		mxPushSlot(mxArgv(argi));
		mxGetID(mxID(_alphabet));
		if (!mxIsUndefined(the->stack)) {
			if (!mxIsStringPrimitive(the->stack))
				mxTypeError("options.alphabet: not a string");
			if (!c_strcmp(the->stack->value.string, "base64url"))
				*alphabet = (txU1*)gxBase64URLAlphabet;
			else if (c_strcmp(the->stack->value.string, "base64"))
				mxTypeError("options.alphabet: neither 'base64' nor 'base64url'");
		}
		mxPop();
		if (lastChunkHandling) {
			mxPushSlot(mxArgv(argi));
			mxGetID(mxID(_lastChunkHandling));
			if (!mxIsUndefined(the->stack)) {
				if (!mxIsStringPrimitive(the->stack))
					mxTypeError("options.lastChunkHandling: not a string");
				if (!c_strcmp(the->stack->value.string, "stop-before-partial"))
					*lastChunkHandling = mxBase64StopBeforePartial;
				else if (!c_strcmp(the->stack->value.string, "strict"))
					*lastChunkHandling = mxBase64Strict;
				else if (c_strcmp(the->stack->value.string, "loose"))
					mxTypeError("options.lastChunkHandling: neither 'loose' nor 'strict' nor 'stop-before-partial'");
			}
			mxPop();
		}
		if (omitPadding) {
			mxPushSlot(mxArgv(argi));
			mxGetID(mxID(_omitPadding));
			fxToBoolean(the, the->stack);
			*omitPadding = the->stack->value.boolean;
			mxPop();
		}
	}
}

void fx_Uint8Array_fromBase64(txMachine* the)
{
	txSize srcSize;
	txSize dstSize;
	txU1* alphabet = (txU1*)gxBase64Alphabet;
	txInteger lastChunkHandling = mxBase64Loose;
	txU1* src;
	txU1* dst;
	if ((mxArgc < 1) || !mxIsStringPrimitive(mxArgv(0)))
		mxTypeError("string: not a string");
	fxUint8ArrayGetBase64Options(the, 1, &alphabet, &lastChunkHandling, NULL);
	srcSize = (txSize)c_strlen(mxArgv(0)->value.string);
	dstSize = (((srcSize + 3) / 4) * 3);
	mxPush(mxUint8ArrayConstructor);
	mxNew();
	mxPushInteger(dstSize);
	mxRunCount(1);
	mxPullSlot(mxResult);
	{
		txSlot* resultInstance = fxCheckTypedArrayInstance(the, mxResult); \
		txSlot* resultDispatch = resultInstance->next; \
		txSlot* resultView = resultDispatch->next; \
		txSlot* resultBuffer = resultView->next; \
		src = (txU1*)(mxArgv(0)->value.string);
		dst = (txU1*)(resultBuffer->value.reference->next->value.arrayBuffer.address + resultView->value.dataView.offset);
		resultBuffer->value.reference->next->next->value.bufferInfo.maxLength = dstSize;
		fxUint8ArrayFromBase64(the, src, dst, &srcSize, &dstSize, alphabet, lastChunkHandling);
		fxSetArrayBufferLength(the, resultBuffer, dstSize);
		resultBuffer->value.reference->next->next->value.bufferInfo.maxLength = -1;
		resultView->value.dataView.size = dstSize;
	}
}

void fx_Uint8Array_fromHex(txMachine* the)
{
	txSize srcSize;
	txSize dstSize;
	txU1* src;
	txU1* dst;
	if ((mxArgc < 1) || !mxIsStringPrimitive(mxArgv(0)))
		mxTypeError("string: not a string");
	srcSize = (txSize)c_strlen(mxArgv(0)->value.string);
	if (srcSize & 1)
		mxSyntaxError("string: odd length");
	dstSize = srcSize >> 1;
	mxPush(mxUint8ArrayConstructor);
	mxNew();
	mxPushInteger(dstSize);
	mxRunCount(1);
	mxPullSlot(mxResult);
	{
		txSlot* resultInstance = fxCheckTypedArrayInstance(the, mxResult); \
		txSlot* resultDispatch = resultInstance->next; \
		txSlot* resultView = resultDispatch->next; \
		txSlot* resultBuffer = resultView->next; \
		src = (txU1*)(mxArgv(0)->value.string);
		dst = (txU1*)(resultBuffer->value.reference->next->value.arrayBuffer.address + resultView->value.dataView.offset);
		fxUint8ArrayFromHex(the, src, dst, &srcSize, &dstSize);
	}
}

void fx_Uint8Array_prototype_setFromBase64(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txU1* alphabet = (txU1*)gxBase64Alphabet;
	txInteger lastChunkHandling = mxBase64Loose;
	txSize srcSize;
	txSize dstSize;
	txU1* src;
	txU1* dst;
	txSlot* property;
	if (dispatch->value.typedArray.dispatch->constructorID != mxID(_Uint8Array))
		mxTypeError("this: not a Uint8Array instance");
	if ((mxArgc < 1) || !mxIsStringPrimitive(mxArgv(0)))
		mxTypeError("string: not a string");
	fxUint8ArrayGetBase64Options(the, 1, &alphabet, &lastChunkHandling, NULL);
	srcSize = (txSize)c_strlen(mxArgv(0)->value.string);
	dstSize = fxCheckDataViewSize(the, view, buffer, XS_MUTABLE);
	src = (txU1*)(mxArgv(0)->value.string);
	dst = (txU1*)(buffer->value.reference->next->value.arrayBuffer.address + view->value.dataView.offset);
	fxUint8ArrayFromBase64(the, src, dst, &srcSize, &dstSize, alphabet, lastChunkHandling);
	mxPush(mxObjectPrototype);
	property = fxLastProperty(the, fxNewObjectInstance(the));
	property = fxNextIntegerProperty(the, property, srcSize, mxID(_read_), XS_NO_FLAG);
	property = fxNextIntegerProperty(the, property, dstSize, mxID(_written), XS_NO_FLAG);
	mxPullSlot(mxResult);
}

void fx_Uint8Array_prototype_setFromHex(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txSize srcSize;
	txSize dstSize;
	txU1* src;
	txU1* dst;
	txSlot* property;
	if (dispatch->value.typedArray.dispatch->constructorID != mxID(_Uint8Array))
		mxTypeError("this: not a Uint8Array instance");
	if ((mxArgc < 1) || !mxIsStringPrimitive(mxArgv(0)))
		mxTypeError("string: not a string");
	srcSize = (txSize)c_strlen(mxArgv(0)->value.string);
	if (srcSize & 1)
		mxSyntaxError("string: odd length");
	dstSize = fxCheckDataViewSize(the, view, buffer, XS_MUTABLE);
	src = (txU1*)(mxArgv(0)->value.string);
	dst = (txU1*)(buffer->value.reference->next->value.arrayBuffer.address + view->value.dataView.offset);
	fxUint8ArrayFromHex(the, src, dst, &srcSize, &dstSize);
	mxPush(mxObjectPrototype);
	property = fxLastProperty(the, fxNewObjectInstance(the));
	property = fxNextIntegerProperty(the, property, srcSize, mxID(_read_), XS_NO_FLAG);
	property = fxNextIntegerProperty(the, property, dstSize, mxID(_written), XS_NO_FLAG);
	mxPullSlot(mxResult);
}

void fx_Uint8Array_prototype_toBase64(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txU1* alphabet = (txU1*)gxBase64Alphabet;
	txBoolean omitPadding = 0;
	txU1* src;
	txU1* dst;
	txSize srcSize;
	txSize dstSize;
	txU1 a, b, c;
	if (dispatch->value.typedArray.dispatch->constructorID != mxID(_Uint8Array))
		mxTypeError("this: not a Uint8Array instance");
	fxUint8ArrayGetBase64Options(the, 0, &alphabet, C_NULL, &omitPadding);
	srcSize = fxCheckDataViewSize(the, view, buffer, XS_IMMUTABLE);
	if (srcSize > (((0x7FFFFFFF >> 2) * 3) - 2))
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	dstSize = (((srcSize + 2) / 3) << 2);
	fxStringBuffer(the, mxResult, C_NULL, dstSize);
	src = (txU1*)buffer->value.reference->next->value.arrayBuffer.address + view->value.dataView.offset;
	dst = (txU1*)mxResult->value.string;
	while (srcSize > 2) {
		a = c_read8(src++);
		b = c_read8(src++);
		c = c_read8(src++);
		*dst++ = c_read8(alphabet + ((a & 0xfc) >> 2));
		*dst++ = c_read8(alphabet + (((a & 0x3) << 4) | ((b & 0xf0) >> 4)));
		*dst++ = c_read8(alphabet + (((b & 0xf) << 2) | ((c & 0xc0) >> 6)));
		*dst++ = c_read8(alphabet + (c & 0x3f));
		srcSize -= 3;
	}
	if (srcSize == 2) {
		a = c_read8(src++);
		b = c_read8(src++);
		*dst++ = c_read8(alphabet + ((a & 0xfc) >> 2));
		*dst++ = c_read8(alphabet + (((a & 0x3) << 4) | ((b & 0xf0) >> 4)));
		*dst++ = c_read8(alphabet + ((b & 0xf) << 2));
		if (!omitPadding)
			*dst++ = '=';
	}
	else if (srcSize == 1) {
		a = c_read8(src++);
		*dst++ = c_read8(alphabet + ((a & 0xfc) >> 2));
		*dst++ = c_read8(alphabet + ((a & 0x3) << 4));
		if (!omitPadding) {
			*dst++ = '=';
			*dst++ = '=';
		}
	}
	*dst++ = 0;
}

void fx_Uint8Array_prototype_toHex(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txU1* alphabet = (txU1*)gxHexAlphabet;
	txU1* src;
	txU1* dst;
	txSize srcSize;
	txSize dstSize;
	txU1 a;
	if (dispatch->value.typedArray.dispatch->constructorID != mxID(_Uint8Array))
		mxTypeError("this: not a Uint8Array instance");
	srcSize = fxCheckDataViewSize(the, view, buffer, XS_IMMUTABLE);
	if (srcSize > (0x7FFFFFFF >> 1))
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	dstSize = (srcSize << 1);
	fxStringBuffer(the, mxResult, C_NULL, dstSize);
	src = (txU1*)buffer->value.reference->next->value.arrayBuffer.address + view->value.dataView.offset;
	dst = (txU1*)mxResult->value.string;
	while (srcSize > 0) {
		a = c_read8(src++);
		*dst++ = c_read8(alphabet + ((a & 0xf0) >> 4));
		*dst++ = c_read8(alphabet + (a & 0x0f));
		srcSize--;
	}
	*dst++ = 0;
}

#endif

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
 */

#include "xsAll.h"

static txInteger fxCheckAtomicsIndex(txMachine* the, txInteger index, txInteger length);
static txSlot* fxCheckAtomicsTypedArray(txMachine* the, txBoolean onlyInt32);
static txSlot* fxCheckSharedArrayBuffer(txMachine* the, txSlot* slot, txString which);
static void fxPushAtomicsValue(txMachine* the, int i);

#define mxAtomicsHead0(TYPE,TO) \
	TYPE result = 0; \
	void* data = host->value.host.data; \
	TYPE* address = (TYPE*)(((txByte*)data) + offset)

#define mxAtomicsHead1(TYPE,TO) \
	TYPE result = 0; \
	TYPE value = (TYPE)TO(the, slot); \
	void* data = host->value.host.data; \
	TYPE* address = (TYPE*)(((txByte*)data) + offset)

#define mxAtomicsHead2(TYPE,TO) \
	TYPE result = (TYPE)TO(the, slot + 1); \
	TYPE value = (TYPE)TO(the, slot); \
	void* data = host->value.host.data; \
	TYPE* address = (TYPE*)(((txByte*)data) + offset)

#ifdef mxUseGCCAtomics
	#define mxAtomicsCompareExchange() __atomic_compare_exchange(address, &result, &value, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
	#define mxAtomicsLoad() __atomic_load(address, &result, __ATOMIC_SEQ_CST)
	#define mxAtomicsAdd() result = __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST)
	#define mxAtomicsAnd() result = __atomic_fetch_and(address, value, __ATOMIC_SEQ_CST)
	#define mxAtomicsExchange() __atomic_exchange(address, &value, &result, __ATOMIC_SEQ_CST)
	#define mxAtomicsOr() result = __atomic_fetch_or(address, value, __ATOMIC_SEQ_CST)
	#define mxAtomicsStore() __atomic_store(address, &value, __ATOMIC_SEQ_CST)
	#define mxAtomicsSub() result = __atomic_fetch_sub(address, value, __ATOMIC_SEQ_CST)
	#define mxAtomicsXor() result = __atomic_fetch_xor(address, value, __ATOMIC_SEQ_CST)
#else
	#define mxAtomicsCompareExchange() fxLockSharedChunk(data); if (*address == result) *address = value; else result = *address; fxUnlockSharedChunk(data)
	#define mxAtomicsLoad() fxLockSharedChunk(data); result = *address;  fxUnlockSharedChunk(data)
	#define mxAtomicsAdd() fxLockSharedChunk(data); result = *address; *address = result + value; fxUnlockSharedChunk(data)
	#define mxAtomicsAnd() fxLockSharedChunk(data); result = *address; *address = result & value; fxUnlockSharedChunk(data)
	#define mxAtomicsExchange() fxLockSharedChunk(data); result = *address; *address = value; fxUnlockSharedChunk(data)
	#define mxAtomicsOr() fxLockSharedChunk(data); result = *address; *address = result | value; fxUnlockSharedChunk(data)
	#define mxAtomicsStore() fxLockSharedChunk(data); *address = value; fxUnlockSharedChunk(data)
	#define mxAtomicsSub() fxLockSharedChunk(data); result = *address; *address = result - value; fxUnlockSharedChunk(data)
	#define mxAtomicsXor() fxLockSharedChunk(data); result = *address; *address = result ^ value; fxUnlockSharedChunk(data)
#endif	

#define mxAtomicsTail() \
	slot->kind = XS_INTEGER_KIND; \
	slot->value.integer = result
	
#define mxAtomicsTailOverflow() \
	if (result <= 0x7FFFFFFF) { \
		slot->kind = XS_INTEGER_KIND; \
		slot->value.integer = result; \
	} \
	else { \
		slot->kind = XS_NUMBER_KIND; \
		slot->value.number = result; \
	}

#define mxAtomicsDeclarations(onlyInt32) \
	txSlot* dispatch = fxCheckAtomicsTypedArray(the, onlyInt32); \
	txSlot* view = dispatch->next; \
	txSlot* buffer = view->next; \
	txSlot* host = fxCheckSharedArrayBuffer(the, buffer, "typedArray.buffer"); \
	txInteger delta = dispatch->value.typedArray.dispatch->size; \
	txInteger index = fxCheckAtomicsIndex(the, 1, view->value.dataView.size / delta); \
	txInteger offset = view->value.dataView.offset + (index * delta)

void fxInt8Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsAdd(); mxAtomicsTail(); }
void fxInt16Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsAdd(); mxAtomicsTail(); }
void fxInt32Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsAdd(); mxAtomicsTail(); }
void fxUint8Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsAdd(); mxAtomicsTail(); }
void fxUint16Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsAdd(); mxAtomicsTail(); }
void fxUint32Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsAdd(); mxAtomicsTailOverflow(); }

void fxInt8And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsAnd(); mxAtomicsTail(); }
void fxInt16And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsAnd(); mxAtomicsTail(); }
void fxInt32And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsAnd(); mxAtomicsTail(); }
void fxUint8And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsAnd(); mxAtomicsTail(); }
void fxUint16And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsAnd(); mxAtomicsTail(); }
void fxUint32And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsAnd(); mxAtomicsTailOverflow(); }

void fxInt8CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txS1, fxToInteger); mxAtomicsCompareExchange(); mxAtomicsTail(); }
void fxInt16CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txS2, fxToInteger); mxAtomicsCompareExchange(); mxAtomicsTail(); }
void fxInt32CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txS4, fxToInteger); mxAtomicsCompareExchange(); mxAtomicsTail(); }
void fxUint8CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txU1, fxToUnsigned); mxAtomicsCompareExchange(); mxAtomicsTail(); }
void fxUint16CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txU2, fxToUnsigned); mxAtomicsCompareExchange(); mxAtomicsTail(); }
void fxUint32CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txU4, fxToUnsigned); mxAtomicsCompareExchange(); mxAtomicsTailOverflow(); }

void fxInt8Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsExchange(); mxAtomicsTail(); }
void fxInt16Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsExchange(); mxAtomicsTail(); }
void fxInt32Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsExchange(); mxAtomicsTail(); }
void fxUint8Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsExchange(); mxAtomicsTail(); }
void fxUint16Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsExchange(); mxAtomicsTail(); }
void fxUint32Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsExchange(); mxAtomicsTailOverflow(); }

void fxInt8Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txS1, fxToInteger); mxAtomicsLoad(); mxAtomicsTail(); }
void fxInt16Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txS2, fxToInteger); mxAtomicsLoad(); mxAtomicsTail(); }
void fxInt32Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txS4, fxToInteger); mxAtomicsLoad(); mxAtomicsTail(); }
void fxUint8Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txU1, fxToUnsigned); mxAtomicsLoad(); mxAtomicsTail(); }
void fxUint16Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txU2, fxToUnsigned); mxAtomicsLoad(); mxAtomicsTail(); }
void fxUint32Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txU4, fxToUnsigned); mxAtomicsLoad(); mxAtomicsTailOverflow(); }

void fxInt8Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsOr(); mxAtomicsTail(); }
void fxInt16Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsOr(); mxAtomicsTail(); }
void fxInt32Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsOr(); mxAtomicsTail(); }
void fxUint8Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsOr(); mxAtomicsTail(); }
void fxUint16Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsOr(); mxAtomicsTail(); }
void fxUint32Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsOr(); mxAtomicsTailOverflow(); }

void fxInt8Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsStore(); mxAtomicsTail(); }
void fxInt16Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsStore(); mxAtomicsTail(); }
void fxInt32Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsStore(); mxAtomicsTail(); }
void fxUint8Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsStore(); mxAtomicsTail(); }
void fxUint16Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsStore(); mxAtomicsTail(); }
void fxUint32Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsStore(); mxAtomicsTailOverflow(); }

void fxInt8Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsSub(); mxAtomicsTail(); }
void fxInt16Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsSub(); mxAtomicsTail(); }
void fxInt32Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsSub(); mxAtomicsTail(); }
void fxUint8Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsSub(); mxAtomicsTail(); }
void fxUint16Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsSub(); mxAtomicsTail(); }
void fxUint32Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsSub(); mxAtomicsTailOverflow(); }

void fxInt8Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsXor(); mxAtomicsTail(); }
void fxInt16Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsXor(); mxAtomicsTail(); }
void fxInt32Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsXor(); mxAtomicsTail(); }
void fxUint8Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsXor(); mxAtomicsTail(); }
void fxUint16Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsXor(); mxAtomicsTail(); }
void fxUint32Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsXor(); mxAtomicsTailOverflow(); }

void fxBuildAtomics(txMachine* the)
{
	txSlot* slot;
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_SharedArrayBuffer_prototype_get_byteLength), C_NULL, mxID(_byteLength), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_SharedArrayBuffer_prototype_slice), 2, mxID(_slice), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "SharedArrayBuffer", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxSharedArrayBufferPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, mxCallback(fx_SharedArrayBuffer), 1, mxID(_SharedArrayBuffer), XS_DONT_ENUM_FLAG));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_species_get), C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Atomics_add), 3, mxID(_add), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Atomics_and), 3, mxID(_and), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Atomics_compareExchange), 4, mxID(_compareExchange), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Atomics_exchange), 3, mxID(_exchange), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Atomics_isLockFree), 1, mxID(_isLockFree), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Atomics_load), 2, mxID(_load), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Atomics_or), 3, mxID(_or), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Atomics_notify), 3, mxID(_notify), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Atomics_store), 3, mxID(_store), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Atomics_sub), 3, mxID(_sub), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Atomics_wait), 4, mxID(_wait), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Atomics_notify), 3, mxID(_wake), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Atomics_xor), 3, mxID(_xor), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Atomics", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxGlobalSetProperty(the, mxGlobal.value.reference, mxID(_Atomics), XS_NO_ID, XS_OWN);
	slot->flag = XS_DONT_ENUM_FLAG;
	slot->kind = the->stack->kind;
	slot->value = the->stack->value;
	the->stack++;
}

txInteger fxCheckAtomicsIndex(txMachine* the, txInteger i, txInteger length)
{
	txNumber index = (mxArgc > i) ? c_trunc(fxToNumber(the, mxArgv(i))) : C_NAN; 
	if (c_isnan(index))
		index = 0;
	else if (index < 0)
		mxRangeError("invalid index");
	else if (index >= length)
		mxRangeError("invalid index");
	return (txInteger)index;
}

txSlot* fxCheckAtomicsTypedArray(txMachine* the, txBoolean onlyInt32)
{
	txSlot* slot = (mxArgc > 0) ? mxArgv(0) : C_NULL;
	if ((!slot) || (!mxIsReference(slot)))
		mxTypeError("typedArray is no object");
	slot = slot->value.reference->next;
	if ((!slot) || ((slot->kind != XS_TYPED_ARRAY_KIND)))
		mxTypeError("typedArray is no TypedArray");
	if (onlyInt32) {
		if (slot->value.typedArray.dispatch->constructorID != _Int32Array)
			mxTypeError("typedArray is no Int32Array");
	}
	else {
		if (slot->value.typedArray.dispatch->constructorID == _Float32Array)
			mxTypeError("typedArray is Float32Array");
		else if (slot->value.typedArray.dispatch->constructorID == _Float64Array)
			mxTypeError("typedArray is Float64Array");
		else if (slot->value.typedArray.dispatch->constructorID == _Uint8ClampedArray)
			mxTypeError("typedArray is Uint8ClampedArray");
	}
	return slot;
}

txSlot* fxCheckSharedArrayBuffer(txMachine* the, txSlot* slot, txString which)
{
	if ((!slot) || (!mxIsReference(slot)))
		mxTypeError("%s is no object", which);
	slot = slot->value.reference->next;
	if ((!slot) || (slot->kind != XS_HOST_KIND) || (slot->value.host.variant.destructor != fxReleaseSharedChunk))
		mxTypeError("%s is no SharedArrayBuffer", which);
	return slot;
}

void fxPushAtomicsValue(txMachine* the, int i)
{
	if (mxArgc > i) {
		txSlot* slot = mxArgv(i);
		if (slot->kind == XS_INTEGER_KIND)
			mxPushSlot(slot);
		else {
			txNumber value = c_trunc(fxToNumber(the, slot)); 
			if (c_isnan(value))
				value = 0;
			mxPushNumber(value);
		}
	}
	else
		mxPushInteger(0);
}


void fx_SharedArrayBuffer(txMachine* the)
{
	txSlot* instance;
	txInteger byteLength;
	txSlot* host;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: SharedArrayBuffer");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxSharedArrayBufferPrototype);
	byteLength = fxCheckAtomicsIndex(the, 0, 0x7FFFFFFF);
	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = the->stack->value.reference;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;
	host = instance->next = fxNewSlot(the);
	host->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	host->kind = XS_HOST_KIND;
	host->value.host.data = fxCreateSharedChunk(byteLength);
	host->value.host.variant.destructor = fxReleaseSharedChunk;
	mxPullSlot(mxResult);
}

void fx_SharedArrayBuffer_prototype_get_byteLength(txMachine* the)
{
	txSlot* host = fxCheckSharedArrayBuffer(the, mxThis, "this");
	mxResult->value.integer = fxMeasureSharedChunk(host->value.host.data);
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_SharedArrayBuffer_prototype_slice(txMachine* the)
{
	txSlot* host = fxCheckSharedArrayBuffer(the, mxThis, "this");
	txInteger length = fxMeasureSharedChunk(host->value.host.data);
	txInteger start = (txInteger)fxArgToIndex(the, 0, 0, length);
	txInteger stop = (txInteger)fxArgToIndex(the, 1, length, length);
	txSlot* result;
	if (stop < start) 
		stop = start;
	length = stop - start;
	mxPushInteger(length);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_constructor));
	if (mxIsUndefined(the->stack)) {
		*the->stack = mxGlobal;
		fxGetID(the, mxID(_SharedArrayBuffer));
	}
	if (!mxIsReference(the->stack)) {
		mxTypeError("no constructor");
	}
	fxGetID(the, mxID(_Symbol_species));
	if (mxIsUndefined(the->stack) || mxIsNull(the->stack)) {
		*the->stack = mxGlobal;
		fxGetID(the, mxID(_SharedArrayBuffer));
	}
	if (!mxIsReference(the->stack)) {
		mxTypeError("no constructor");
	}
	fxNew(the);
	mxPullSlot(mxResult);
	result = fxCheckSharedArrayBuffer(the, mxResult, "result");
	if (result == host)
		mxTypeError("same SharedArrayBuffer instance");
	if (fxMeasureSharedChunk(result->value.host.data) < length)
		mxTypeError("smaller SharedArrayBuffer instance");
	c_memcpy(result->value.host.data, ((txByte*)host->value.host.data + start), stop - start);
}

void fx_Atomics_add(txMachine* the)
{
	mxAtomicsDeclarations(0);
	fxPushAtomicsValue(the, 2);
	(*dispatch->value.typedArray.atomics->add)(the, host, offset, the->stack, 0);
	mxPullSlot(mxResult);
}

void fx_Atomics_and(txMachine* the)
{
	mxAtomicsDeclarations(0);
	fxPushAtomicsValue(the, 2);
	(*dispatch->value.typedArray.atomics->and)(the, host, offset, the->stack, 0);
	mxPullSlot(mxResult);
}

void fx_Atomics_compareExchange(txMachine* the)
{
	mxAtomicsDeclarations(0);
	fxPushAtomicsValue(the, 2);
	fxPushAtomicsValue(the, 3);
	(*dispatch->value.typedArray.atomics->compareExchange)(the, host, offset, the->stack, 0);
	mxPullSlot(mxResult);
	mxPop();
}

void fx_Atomics_exchange(txMachine* the)
{
	mxAtomicsDeclarations(0);
	fxPushAtomicsValue(the, 2);
	(*dispatch->value.typedArray.atomics->exchange)(the, host, offset, the->stack, 0);
	mxPullSlot(mxResult);
}

void fx_Atomics_isLockFree(txMachine* the)
{
	txInteger size = (mxArgc > 0) ? fxToInteger(the, mxArgv(0)) : 0;
	mxResult->value.boolean = (size == 4) ? 1 : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Atomics_load(txMachine* the)
{
	mxAtomicsDeclarations(0);
	(*dispatch->value.typedArray.atomics->load)(the, host, offset, mxResult, 0);
}

void fx_Atomics_or(txMachine* the)
{
	mxAtomicsDeclarations(0);
	fxPushAtomicsValue(the, 2);
	(*dispatch->value.typedArray.atomics->or)(the, host, offset, the->stack, 0);
	mxPullSlot(mxResult);
}

void fx_Atomics_notify(txMachine* the)
{
	mxAtomicsDeclarations(1);
	txInteger count = ((mxArgc > 2) && !mxIsUndefined(mxArgv(2))) ? fxToInteger(the, mxArgv(2)) : 20;
	if (count < 0)
		count = 0;
	mxResult->value.integer = fxWakeSharedChunk(the, host->value.host.data, offset, count);
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_Atomics_store(txMachine* the)
{
	mxAtomicsDeclarations(0);
	fxPushAtomicsValue(the, 2);
	*mxResult = *the->stack;
	(*dispatch->value.typedArray.atomics->store)(the, host, offset, the->stack, 0);
	mxPop();
}

void fx_Atomics_sub(txMachine* the)
{
	mxAtomicsDeclarations(0);
	fxPushAtomicsValue(the, 2);
	(*dispatch->value.typedArray.atomics->sub)(the, host, offset, the->stack, 0);
	mxPullSlot(mxResult);
}

void fx_Atomics_wait(txMachine* the)
{
	mxAtomicsDeclarations(1);
	txInteger value = (mxArgc > 2) ? fxToInteger(the, mxArgv(2)) : 0;
	txNumber timeout = (mxArgc > 3) ? fxToNumber(the, mxArgv(3)) : C_NAN;
	txInteger result;
	if (c_isnan(timeout))
		timeout = C_INFINITY;
	else {
		txNumber now = fxDateNow();
		timeout += now;
		if (timeout <= now)
			timeout = now;
		else if (timeout > 8.64e15)
			timeout = C_INFINITY;
		else
			timeout = c_trunc(timeout);
	}
	result = fxWaitSharedChunk(the, host->value.host.data, offset, value, timeout);
	if (result < 0)
		mxResult->value.string = "not-equal";
	else if (result > 0)
		mxResult->value.string = "ok";
	else
		mxResult->value.string = "timed-out";
	mxResult->kind = XS_STRING_X_KIND;
}

void fx_Atomics_xor(txMachine* the)
{
	mxAtomicsDeclarations(0);
	fxPushAtomicsValue(the, 2);
	(*dispatch->value.typedArray.atomics->xor)(the, host, offset, the->stack, 0);
	mxPullSlot(mxResult);
}

#ifdef mxUseDefaultSharedChunks

#if defined(mxUseLinuxFutex) && defined(mxUseGCCAtomics)
	#define mxThreads 1
	static long futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2, int val3)
	{
		return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
	}
	static pid_t gettid()
	{
		return syscall(SYS_gettid);
	}
	typedef pid_t txThread;
	#define mxCurrentThread() gettid()
#elif defined(mxUsePOSIXThreads)
	#define mxThreads 1
	typedef pthread_cond_t txCondition;
	typedef pthread_mutex_t txMutex;
	typedef pthread_t txThread;
	#define mxCreateCondition(CONDITION) pthread_cond_init(CONDITION,NULL)
	#define mxCreateMutex(MUTEX) pthread_mutex_init(MUTEX,NULL)
	#define mxCurrentThread() pthread_self()
	#define mxDeleteCondition(CONDITION) pthread_cond_destroy(CONDITION)
	#define mxDeleteMutex(MUTEX) pthread_mutex_destroy(MUTEX)
	#define mxLockMutex(MUTEX) pthread_mutex_lock(MUTEX)
	#define mxUnlockMutex(MUTEX) pthread_mutex_unlock(MUTEX)
	#define mxWakeCondition(CONDITION) pthread_cond_signal(CONDITION)
#elif defined(mxUseFreeRTOSTasks)
	#define mxThreads 1

	#include "FreeRTOS.h"
	#include "freertos/queue.h"
	#include "freertos/semphr.h"

	typedef TaskHandle_t txCondition;
	typedef struct {
		QueueHandle_t handle;
		StaticSemaphore_t buffer;
	} txMutex;
	typedef TaskHandle_t txThread;
	#define mxCreateCondition(CONDITION) *(CONDITION) = xTaskGetCurrentTaskHandle()
	#define mxCreateMutex(MUTEX) (MUTEX)->handle = xSemaphoreCreateMutexStatic(&((MUTEX)->buffer))
	#define mxCurrentThread() xTaskGetCurrentTaskHandle()
	#define mxDeleteCondition(CONDITION) *(CONDITION) = NULL
	#define mxDeleteMutex(MUTEX) vSemaphoreDelete((MUTEX)->handle)
	#define mxLockMutex(MUTEX) xSemaphoreTake((MUTEX)->handle, portMAX_DELAY)
	#define mxUnlockMutex(MUTEX) xSemaphoreGive((MUTEX)->handle)
	#define mxWakeCondition(CONDITION) xTaskNotifyGive(*(CONDITION));
#elif mxWindows
	#define mxThreads 1
	typedef CONDITION_VARIABLE txCondition;
	typedef CRITICAL_SECTION txMutex;
	typedef DWORD txThread;
	#define mxCreateCondition(CONDITION) InitializeConditionVariable(CONDITION)
	#define mxCreateMutex(MUTEX) InitializeCriticalSection(MUTEX)
	#define mxCurrentThread() GetCurrentThreadId()
	#define mxDeleteCondition(CONDITION) (void)(CONDITION)
	#define mxDeleteMutex(MUTEX) DeleteCriticalSection(MUTEX)
	#define mxLockMutex(MUTEX) EnterCriticalSection(MUTEX)
	#define mxUnlockMutex(MUTEX) LeaveCriticalSection(MUTEX)
	#define mxWakeCondition(CONDITION) WakeConditionVariable(CONDITION)
#else
	#define mxThreads 0
	typedef void* txThread;
	#define mxCurrentThread() C_NULL
#endif

typedef struct sxSharedCluster txSharedCluster;
typedef struct sxSharedChunk txSharedChunk;

struct sxSharedCluster {
	txThread mainThread;
	txSize usage;
#if mxThreads && !defined(mxUseLinuxFutex)
	txMachine* waiterLink; 
	txMutex waiterMutex; 
#endif
};

struct sxSharedChunk {
#if mxThreads && !defined(mxUseGCCAtomics)
	txMutex mutex;
#endif
	txSize size;
	txSize usage;
};

txSharedCluster* gxSharedCluster = C_NULL;

void fxInitializeSharedCluster()
{
	if (gxSharedCluster) {
		gxSharedCluster->usage++;
	}
	else {
		gxSharedCluster = c_calloc(sizeof(txSharedCluster), 1);
		if (gxSharedCluster) {
			gxSharedCluster->mainThread = mxCurrentThread();
			gxSharedCluster->usage++;
		#if mxThreads && !defined(mxUseLinuxFutex)
			mxCreateMutex(&gxSharedCluster->waiterMutex);
		#endif
		}
	}
}

void fxTerminateSharedCluster()
{
	if (gxSharedCluster) {
		gxSharedCluster->usage--;
		if (gxSharedCluster->usage == 0) {
		#if mxThreads && !defined(mxUseLinuxFutex)
			mxDeleteMutex(&gxSharedCluster->waiterMutex);
		#endif
			c_free(gxSharedCluster);
			gxSharedCluster = C_NULL;
		}
	}
}

void* fxCreateSharedChunk(txInteger size)
{
	txSharedChunk* chunk = c_malloc(sizeof(txSharedChunk) + size);
	if (chunk) {
		void* data = (((txByte*)chunk) + sizeof(txSharedChunk));
	#if mxThreads && !defined(mxUseGCCAtomics)
		mxCreateMutex(&(chunk->mutex));
	#endif
		chunk->size = size;
		chunk->usage = 1;
		c_memset(data, 0, size);
		return data;
	}
	return C_NULL;
}

void fxLockSharedChunk(void* data)
{
#if mxThreads && !defined(mxUseGCCAtomics)
	txSharedChunk* chunk = (txSharedChunk*)(((txByte*)data) - sizeof(txSharedChunk));
    mxLockMutex(&(chunk->mutex));
#endif
}

txInteger fxMeasureSharedChunk(void* data)
{
	txSharedChunk* chunk = (txSharedChunk*)(((txByte*)data) - sizeof(txSharedChunk));
	return chunk->size;
}

void* fxRetainSharedChunk(void* data)
{
	txSharedChunk* chunk = (txSharedChunk*)(((txByte*)data) - sizeof(txSharedChunk));
	txS4 result = 0;
	txS4 value = 1;
	txS4* address = &(chunk->usage);
	mxAtomicsAdd();
	if (result == 0)
		return C_NULL;
	return data;
}

void fxReleaseSharedChunk(void* data)
{
	txSharedChunk* chunk = (txSharedChunk*)(((txByte*)data) - sizeof(txSharedChunk));
	txS4 result = 0;
	txS4 value = 1;
	txS4* address = &(chunk->usage);
	mxAtomicsSub();
	if (result == 1) {
		c_free(chunk);
	}
}

void fxUnlockSharedChunk(void* data)
{
#if mxThreads && !defined(mxUseGCCAtomics)
	txSharedChunk* chunk = (txSharedChunk*)(((txByte*)data) - sizeof(txSharedChunk));
    mxUnlockMutex(&(chunk->mutex));
#endif
}

txInteger fxWaitSharedChunk(txMachine* the, void* data, txInteger offset, txInteger value, txNumber timeout)
{
	txInteger* address = (txInteger*)((txByte*)data + offset);
	txInteger result = 0;
	if (gxSharedCluster && (gxSharedCluster->mainThread != mxCurrentThread())) {
	#if defined(mxUseLinuxFutex)
		if (timeout == C_INFINITY)
			futex(address, FUTEX_WAIT, value, C_NULL, C_NULL, 0);
		else {
			struct timespec ts;
			timeout -= fxDateNow();
			ts.tv_sec = c_floor(timeout / 1000);
			ts.tv_nsec = c_fmod(timeout, 1000) * 1000000;
			futex(address, FUTEX_WAIT, value, &ts, C_NULL, 0);
		}
		result = errno;
		if (result == EAGAIN)
			result = -1;
		else if (result == ETIMEDOUT)
			result = 0;
		else
			result = 1;
	#elif mxThreads
		txMachine** machineAddress;
		txMachine* machine;
		mxLockMutex(&gxSharedCluster->waiterMutex);
		machineAddress = &gxSharedCluster->waiterLink;
		while ((machine = *machineAddress))
			machineAddress = (txMachine**)&machine->waiterLink;
		*machineAddress = the;
		mxAtomicsLoad();
		if (result != value) {
			result = -1;
		}
		else {
			txCondition condition;
			mxCreateCondition(&condition);
			the->waiterCondition = &condition;
			the->waiterData = address;
			if (timeout == C_INFINITY) {
			#if defined(mxUsePOSIXThreads)
				while (the->waiterData == address)
					pthread_cond_wait(&condition, &gxSharedCluster->waiterMutex);
				result = 1;
			#elif defined(mxUseFreeRTOSTasks)
				mxUnlockMutex(&gxSharedCluster->waiterMutex);
				ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
				mxLockMutex(&gxSharedCluster->waiterMutex);
			#else
				while (the->waiterData == address)
					SleepConditionVariableCS(&condition, &gxSharedCluster->waiterMutex, INFINITE);
			#endif
				result = 1;
			}
			else {
			#if defined(mxUsePOSIXThreads)
				struct timespec ts;
				ts.tv_sec = c_floor(timeout / 1000);
				ts.tv_nsec = c_fmod(timeout, 1000) * 1000000;
				while (the->waiterData == address) {
					result = (pthread_cond_timedwait(&condition, &gxSharedCluster->waiterMutex, &ts) == ETIMEDOUT) ? 0 : 1;
					if (!result)
						break;
				}
			#elif defined(mxUseFreeRTOSTasks)
				mxUnlockMutex(&gxSharedCluster->waiterMutex);
				ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS((timeout - fxDateNow())));
				mxLockMutex(&gxSharedCluster->waiterMutex);
				result = (the->waiterData == address) ? 0 : 1;
			#else
				while (the->waiterData == address) {
					result = (SleepConditionVariableCS(&condition, &gxSharedCluster->waiterMutex, (DWORD)(timeout - fxDateNow()))) ? 1 : 0;
					if (!result)
						break;
				}
			#endif
			}
			the->waiterCondition = C_NULL;
			mxDeleteCondition(&condition);
		}
		machineAddress = &gxSharedCluster->waiterLink;
		while ((machine = *machineAddress)) {
			if (machine == the) {
				*machineAddress = the->waiterLink;
				the->waiterLink = C_NULL;
				break;
			}
			machineAddress = (txMachine**)&machine->waiterLink;
		}
		mxUnlockMutex(&gxSharedCluster->waiterMutex);
	#endif
	}
	else {
		mxTypeError("main thread cannot wait");
	}
	return result;
}

txInteger fxWakeSharedChunk(txMachine* the, void* data, txInteger offset, txInteger count)
{
	txInteger* address = (txInteger*)((txByte*)data + offset);
	txInteger result = 0;
	if (gxSharedCluster) {
	#if defined(mxUseLinuxFutex)
		if (count > 0) {
			result = futex(address, FUTEX_WAKE, count, C_NULL, C_NULL, 0);
		}
	#elif mxThreads
		txMachine* machine;
		mxLockMutex(&gxSharedCluster->waiterMutex);
		machine = gxSharedCluster->waiterLink;
		while (machine) {
			if (machine->waiterData == address) {
				if (count == 0)
					break;
				count--;
				machine->waiterData = C_NULL;
				mxWakeCondition((txCondition*)machine->waiterCondition);
				result++;
			}
			machine = machine->waiterLink;
		}
		mxUnlockMutex(&gxSharedCluster->waiterMutex);
	#endif	
	}
	return result;
}

#endif /* mxUseDefaultSharedChunks */



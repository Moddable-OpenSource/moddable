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
static txSlot* fxCheckAtomicsArrayBuffer(txMachine* the, txSlot* slot, txBoolean onlyShared);
static void* fxCheckAtomicsArrayBufferDetached(txMachine* the, txSlot* slot, txBoolean mutable);
static txSlot* fxCheckSharedArrayBuffer(txMachine* the, txSlot* slot, txString which);
static void fxPushAtomicsValue(txMachine* the, int i, txID id);

#define mxAtomicsHead0(TYPE,TO) \
	TYPE result = 0; \
	txBoolean lock = host->kind == XS_HOST_KIND; \
	void* data = (lock) ? host->value.host.data : fxCheckAtomicsArrayBufferDetached(the, host, XS_IMMUTABLE); \
	TYPE* address = (TYPE*)(((txByte*)data) + offset)

#define mxAtomicsHead1(TYPE,TO) \
	TYPE result = 0; \
	TYPE value = (TYPE)TO(the, slot); \
	txBoolean lock = host->kind == XS_HOST_KIND; \
	void* data = (lock) ? host->value.host.data : fxCheckAtomicsArrayBufferDetached(the, host, XS_MUTABLE); \
	TYPE* address = (TYPE*)(((txByte*)data) + offset)

#define mxAtomicsHead2(TYPE,TO) \
	TYPE result = (TYPE)TO(the, slot + 1); \
	TYPE value = (TYPE)TO(the, slot); \
	txBoolean lock = host->kind == XS_HOST_KIND; \
	void* data = (lock) ? host->value.host.data : fxCheckAtomicsArrayBufferDetached(the, host, XS_MUTABLE); \
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
	#define mxAtomicsCompareExchange() if (lock) fxLockSharedChunk(data); if (*address == result) *address = value; else result = *address; if (lock) fxUnlockSharedChunk(data)
	#define mxAtomicsLoad() if (lock) fxLockSharedChunk(data); result = *address;  if (lock) fxUnlockSharedChunk(data)
	#define mxAtomicsAdd() if (lock) fxLockSharedChunk(data); result = *address; *address = result + value; if (lock) fxUnlockSharedChunk(data)
	#define mxAtomicsAnd() if (lock) fxLockSharedChunk(data); result = *address; *address = result & value; if (lock) fxUnlockSharedChunk(data)
	#define mxAtomicsExchange() if (lock) fxLockSharedChunk(data); result = *address; *address = value; if (lock) fxUnlockSharedChunk(data)
	#define mxAtomicsOr() if (lock) fxLockSharedChunk(data); result = *address; *address = result | value; if (lock) fxUnlockSharedChunk(data)
	#define mxAtomicsStore() if (lock) fxLockSharedChunk(data); *address = value; if (lock) fxUnlockSharedChunk(data)
	#define mxAtomicsSub() if (lock) fxLockSharedChunk(data); result = *address; *address = result - value; if (lock) fxUnlockSharedChunk(data)
	#define mxAtomicsXor() if (lock) fxLockSharedChunk(data); result = *address; *address = result ^ value; if (lock) fxUnlockSharedChunk(data)
#endif	

#define mxAtomicsTail() \
	slot->kind = XS_INTEGER_KIND; \
	slot->value.integer = result

#define mxAtomicsTailBigInt64() \
	fxFromBigInt64(the, slot, result)

#define mxAtomicsTailBigUint64() \
	fxFromBigUint64(the, slot, result)
	
#define mxAtomicsTailOverflow() \
	if (result <= 0x7FFFFFFF) { \
		slot->kind = XS_INTEGER_KIND; \
		slot->value.integer = result; \
	} \
	else { \
		slot->kind = XS_NUMBER_KIND; \
		slot->value.number = result; \
	}
	
#define mxAtomicsTailWait() \
	return (result != value) ? -1 : fxWaitSharedChunk(the, address, timeout)

#define mxAtomicsDeclarations(onlyInt32, onlyShared) \
	txSlot* dispatch = fxCheckAtomicsTypedArray(the, onlyInt32); \
	txSlot* view = dispatch->next; \
	txSlot* buffer = view->next; \
	txSlot* host = fxCheckAtomicsArrayBuffer(the, buffer, onlyShared); \
	txU2 shift = dispatch->value.typedArray.dispatch->shift; \
	txInteger index = fxCheckAtomicsIndex(the, 1, fxGetDataViewSize(the, view, buffer) >> shift); \
	txInteger offset = view->value.dataView.offset + (index << shift)

void fxInt8Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsAdd(); mxAtomicsTail(); }
void fxInt16Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsAdd(); mxAtomicsTail(); }
void fxInt32Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsAdd(); mxAtomicsTail(); }
void fxInt64Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS8, fxToBigInt64); mxAtomicsAdd(); mxAtomicsTailBigInt64(); }
void fxUint8Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsAdd(); mxAtomicsTail(); }
void fxUint16Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsAdd(); mxAtomicsTail(); }
void fxUint32Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsAdd(); mxAtomicsTailOverflow(); }
void fxUint64Add(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU8, fxToBigUint64); mxAtomicsAdd(); mxAtomicsTailBigUint64(); }

void fxInt8And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsAnd(); mxAtomicsTail(); }
void fxInt16And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsAnd(); mxAtomicsTail(); }
void fxInt32And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsAnd(); mxAtomicsTail(); }
void fxInt64And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS8, fxToBigInt64); mxAtomicsAnd(); mxAtomicsTailBigInt64(); }
void fxUint8And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsAnd(); mxAtomicsTail(); }
void fxUint16And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsAnd(); mxAtomicsTail(); }
void fxUint32And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsAnd(); mxAtomicsTailOverflow(); }
void fxUint64And(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU8, fxToBigUint64); mxAtomicsAnd(); mxAtomicsTailBigUint64(); }

void fxInt8CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txS1, fxToInteger); mxAtomicsCompareExchange(); mxAtomicsTail(); }
void fxInt16CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txS2, fxToInteger); mxAtomicsCompareExchange(); mxAtomicsTail(); }
void fxInt32CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txS4, fxToInteger); mxAtomicsCompareExchange(); mxAtomicsTail(); }
void fxInt64CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txS8, fxToBigInt64); mxAtomicsCompareExchange(); mxAtomicsTailBigInt64(); }
void fxUint8CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txU1, fxToUnsigned); mxAtomicsCompareExchange(); mxAtomicsTail(); }
void fxUint16CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txU2, fxToUnsigned); mxAtomicsCompareExchange(); mxAtomicsTail(); }
void fxUint32CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txU4, fxToUnsigned); mxAtomicsCompareExchange(); mxAtomicsTailOverflow(); }
void fxUint64CompareExchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead2(txU8, fxToBigUint64); mxAtomicsCompareExchange(); mxAtomicsTailBigUint64(); }

void fxInt8Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsExchange(); mxAtomicsTail(); }
void fxInt16Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsExchange(); mxAtomicsTail(); }
void fxInt32Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsExchange(); mxAtomicsTail(); }
void fxInt64Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS8, fxToBigInt64); mxAtomicsExchange(); mxAtomicsTailBigInt64(); }
void fxUint8Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsExchange(); mxAtomicsTail(); }
void fxUint16Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsExchange(); mxAtomicsTail(); }
void fxUint32Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsExchange(); mxAtomicsTailOverflow(); }
void fxUint64Exchange(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU8, fxToBigUint64); mxAtomicsExchange(); mxAtomicsTailBigUint64(); }

void fxInt8Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txS1, fxToInteger); mxAtomicsLoad(); mxAtomicsTail(); }
void fxInt16Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txS2, fxToInteger); mxAtomicsLoad(); mxAtomicsTail(); }
void fxInt32Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txS4, fxToInteger); mxAtomicsLoad(); mxAtomicsTail(); }
void fxInt64Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txS8, fxToBigInt64); mxAtomicsLoad(); mxAtomicsTailBigInt64(); }
void fxUint8Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txU1, fxToUnsigned); mxAtomicsLoad(); mxAtomicsTail(); }
void fxUint16Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txU2, fxToUnsigned); mxAtomicsLoad(); mxAtomicsTail(); }
void fxUint32Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txU4, fxToUnsigned); mxAtomicsLoad(); mxAtomicsTailOverflow(); }
void fxUint64Load(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead0(txU8, fxToBigUint64); mxAtomicsLoad(); mxAtomicsTailBigUint64(); }

void fxInt8Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsOr(); mxAtomicsTail(); }
void fxInt16Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsOr(); mxAtomicsTail(); }
void fxInt32Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsOr(); mxAtomicsTail(); }
void fxInt64Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS8, fxToBigInt64); mxAtomicsOr(); mxAtomicsTailBigInt64(); }
void fxUint8Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsOr(); mxAtomicsTail(); }
void fxUint16Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsOr(); mxAtomicsTail(); }
void fxUint32Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsOr(); mxAtomicsTailOverflow(); }
void fxUint64Or(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU8, fxToBigUint64); mxAtomicsOr(); mxAtomicsTailBigUint64(); }

void fxInt8Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsStore(); mxAtomicsTail(); }
void fxInt16Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsStore(); mxAtomicsTail(); }
void fxInt32Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsStore(); mxAtomicsTail(); }
void fxInt64Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS8, fxToBigInt64); mxAtomicsStore(); mxAtomicsTailBigInt64(); }
void fxUint8Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsStore(); mxAtomicsTail(); }
void fxUint16Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsStore(); mxAtomicsTail(); }
void fxUint32Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsStore(); mxAtomicsTailOverflow(); }
void fxUint64Store(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU8, fxToBigUint64); mxAtomicsStore(); mxAtomicsTailBigUint64(); }

void fxInt8Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsSub(); mxAtomicsTail(); }
void fxInt16Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsSub(); mxAtomicsTail(); }
void fxInt32Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsSub(); mxAtomicsTail(); }
void fxInt64Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS8, fxToBigInt64); mxAtomicsSub(); mxAtomicsTailBigInt64(); }
void fxUint8Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsSub(); mxAtomicsTail(); }
void fxUint16Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsSub(); mxAtomicsTail(); }
void fxUint32Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsSub(); mxAtomicsTailOverflow(); }
void fxUint64Sub(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU8, fxToBigUint64); mxAtomicsSub(); mxAtomicsTailBigUint64(); }

void fxInt8Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS1, fxToInteger); mxAtomicsXor(); mxAtomicsTail(); }
void fxInt16Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS2, fxToInteger); mxAtomicsXor(); mxAtomicsTail(); }
void fxInt32Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsXor(); mxAtomicsTail(); }
void fxInt64Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txS8, fxToBigInt64); mxAtomicsXor(); mxAtomicsTailBigInt64(); }
void fxUint8Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU1, fxToUnsigned); mxAtomicsXor(); mxAtomicsTail(); }
void fxUint16Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU2, fxToUnsigned); mxAtomicsXor(); mxAtomicsTail(); }
void fxUint32Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU4, fxToUnsigned); mxAtomicsXor(); mxAtomicsTailOverflow(); }
void fxUint64Xor(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, int endian) { mxAtomicsHead1(txU8, fxToBigUint64); mxAtomicsXor(); mxAtomicsTailBigUint64(); }

txInteger fxInt32Wait(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, txNumber timeout) { mxAtomicsHead1(txS4, fxToInteger); mxAtomicsLoad(); mxAtomicsTailWait(); }
txInteger fxInt64Wait(txMachine* the, txSlot* host, txInteger offset, txSlot* slot, txNumber timeout) { mxAtomicsHead1(txS8, fxToBigInt64); mxAtomicsLoad(); mxAtomicsTailWait(); }

void fxBuildAtomics(txMachine* the)
{
	txSlot* slot;
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_SharedArrayBuffer_prototype_get_byteLength), C_NULL, mxID(_byteLength), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_SharedArrayBuffer_prototype_get_growable), C_NULL, mxID(_growable), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_SharedArrayBuffer_prototype_get_maxByteLength), C_NULL, mxID(_maxByteLength), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_SharedArrayBuffer_prototype_grow), 1, mxID(_grow), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_SharedArrayBuffer_prototype_slice), 2, mxID(_slice), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "SharedArrayBuffer", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxSharedArrayBufferPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_SharedArrayBuffer), 1, mxID(_SharedArrayBuffer));
	mxSharedArrayBufferConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_species_get), C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	mxPop();
	
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
	mxPull(mxAtomicsObject);
}

txSlot* fxCheckAtomicsArrayBuffer(txMachine* the, txSlot* slot, txBoolean onlyShared)
{
	if ((!slot) || (!mxIsReference(slot)))
		mxTypeError("typedArray.buffer is no object");
	slot = slot->value.reference->next;
	if (slot && (slot->kind == XS_HOST_KIND) && (slot->value.host.variant.destructor == fxReleaseSharedChunk))
		return slot;
	if (onlyShared)
		mxTypeError("typedArray.buffer is no SharedArrayBuffer");
	if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_ARRAY_BUFFER_KIND)) {
		if (slot->value.arrayBuffer.address == C_NULL)
			mxTypeError("typedArray.buffer is detached");
		return slot;
	}
	mxTypeError("typedArray.buffer is no SharedArrayBuffer, no ArrayBuffer");
	return C_NULL;
}

void* fxCheckAtomicsArrayBufferDetached(txMachine* the, txSlot* slot, txBoolean mutable)
{
	if (slot->value.arrayBuffer.address == C_NULL)
		mxTypeError("typedArray.buffer is detached");
	if (mutable && (slot->flag & XS_DONT_SET_FLAG))
		mxTypeError("typedArray.buffer is read-only");
	return slot->value.arrayBuffer.address;
}

txInteger fxCheckAtomicsIndex(txMachine* the, txInteger i, txInteger length)
{
	txNumber index = (mxArgc > i) ? c_trunc(fxToNumber(the, mxArgv(i))) : C_NAN; 
	if (c_isnan(index))
		index = 0;
	if (index < 0)
		mxRangeError("invalid index");
	else if (index >= length)
		mxRangeError("invalid index");
	return (txInteger)index;
}

txSlot* fxCheckAtomicsTypedArray(txMachine* the, txBoolean onlyInt32)
{
	txSlot* slot = (mxArgc > 0) ? mxArgv(0) : C_NULL;
	txID id;
	if ((!slot) || (!mxIsReference(slot)))
		mxTypeError("typedArray is no object");
	slot = slot->value.reference->next;
	if ((!slot) || ((slot->kind != XS_TYPED_ARRAY_KIND)))
		mxTypeError("typedArray is no TypedArray");
	id = slot->value.typedArray.dispatch->constructorID;
	if (onlyInt32) {
		if ((id != _Int32Array) && (id != _BigInt64Array))
			mxTypeError("typedArray is no Int32Array");
	}
	else {
		if (id == _Float32Array)
			mxTypeError("typedArray is Float32Array");
		else if (id == _Float64Array)
			mxTypeError("typedArray is Float64Array");
		else if (id == _Uint8ClampedArray)
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

void fxPushAtomicsValue(txMachine* the, int i, txID id)
{
	txSlot* slot;
	if (mxArgc > i)
		mxPushSlot(mxArgv(i));
	else
		mxPushUndefined();
	slot = the->stack;
	if ((id == _BigInt64Array) || (id == _BigUint64Array))
		fxBigIntCoerce(the, slot);
	else {
		txNumber value;
		fxNumberCoerce(the, slot);
		value = c_trunc(slot->value.number); 
		if (c_isnan(value) || (value == -0))
			value = 0;
		slot->value.number = value;
	}
}


void fx_SharedArrayBuffer(txMachine* the)
{
	txSlot* instance;
	txInteger byteLength;
	txInteger maxByteLength = -1;
	txSlot* property;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: SharedArrayBuffer");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxSharedArrayBufferPrototype);
	byteLength = fxCheckAtomicsIndex(the, 0, 0x7FFFFFFF);
	if ((mxArgc > 1) && mxIsReference(mxArgv(1))) {
		mxPushSlot(mxArgv(1));
		mxGetID(mxID(_maxByteLength));
		mxPullSlot(mxArgv(1));
		maxByteLength = fxArgToByteLength(the, 1, -1);
	}
	if (maxByteLength >= 0) {
		if (byteLength > maxByteLength)
			mxRangeError("byteLength > maxByteLength");
	}
	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = the->stack->value.reference;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_HOST_KIND;
	property->value.host.data = fxCreateSharedChunk(byteLength);
	if (!property->value.host.data)
		mxRangeError("cannot allocate SharedArrayBuffer");
	property->value.host.variant.destructor = fxReleaseSharedChunk;
	property = property->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG;
	property->kind = XS_BUFFER_INFO_KIND;
	property->value.bufferInfo.length = byteLength;
	property->value.bufferInfo.maxLength = maxByteLength;
	mxPullSlot(mxResult);
}

void fx_SharedArrayBuffer_prototype_get_byteLength(txMachine* the)
{
	txSlot* host = fxCheckSharedArrayBuffer(the, mxThis, "this");
	txSlot* bufferInfo = host->next; 
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = bufferInfo->value.bufferInfo.length;
}

void fx_SharedArrayBuffer_prototype_get_growable(txMachine* the)
{
	txSlot* host = fxCheckSharedArrayBuffer(the, mxThis, "this");
	txSlot* bufferInfo = host->next;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (bufferInfo->value.bufferInfo.maxLength >= 0) ? 1 : 0;
}

void fx_SharedArrayBuffer_prototype_get_maxByteLength(txMachine* the)
{
	txSlot* host = fxCheckSharedArrayBuffer(the, mxThis, "this");
	txSlot* bufferInfo = host->next; 
	mxResult->kind = XS_INTEGER_KIND;
	if (bufferInfo->value.bufferInfo.maxLength >= 0)
		mxResult->value.integer = bufferInfo->value.bufferInfo.maxLength;
	else
		mxResult->value.integer = bufferInfo->value.bufferInfo.length;
}

void fx_SharedArrayBuffer_prototype_grow(txMachine* the)
{
	txSlot* host = fxCheckSharedArrayBuffer(the, mxThis, "this");
	txSlot* bufferInfo = host->next; 
	txInteger maxByteLength, oldByteLength, newByteLength;
	maxByteLength = bufferInfo->value.bufferInfo.maxLength;
	if (maxByteLength < 0)
		mxTypeError("not resizable");
	oldByteLength = bufferInfo->value.bufferInfo.length;
	newByteLength = fxArgToByteLength(the, 0, 0);
	if (newByteLength < oldByteLength)
		mxRangeError("newLength < byteLength");
	if (newByteLength > maxByteLength)
		mxRangeError("newLength > maxByteLength");
	mxRangeError("cannot grow SharedArrayBuffer");
}

void fx_SharedArrayBuffer_prototype_slice(txMachine* the)
{
	txSlot* host = fxCheckSharedArrayBuffer(the, mxThis, "this");
	txSlot* bufferInfo = host->next; 
	txInteger length = bufferInfo->value.bufferInfo.length;
	txInteger start = (txInteger)fxArgToIndex(the, 0, 0, length);
	txInteger stop = (txInteger)fxArgToIndex(the, 1, length, length);
	txSlot* result;
	if (stop < start) 
		stop = start;
	length = stop - start;
	mxPushSlot(mxThis);
	mxGetID(mxID(_constructor));
	fxToSpeciesConstructor(the, &mxSharedArrayBufferConstructor);
	mxNew();
	mxPushInteger(length);
	mxRunCount(1);
	mxPullSlot(mxResult);
	result = fxCheckSharedArrayBuffer(the, mxResult, "result");
	if (result == host)
		mxTypeError("same SharedArrayBuffer instance");
	bufferInfo = result->next; 
	if (bufferInfo->value.bufferInfo.length < length)
		mxTypeError("smaller SharedArrayBuffer instance");
	c_memcpy(result->value.host.data, ((txByte*)host->value.host.data + start), stop - start);
}

void fx_Atomics_add(txMachine* the)
{
	mxAtomicsDeclarations(0, 0);
	fxPushAtomicsValue(the, 2, dispatch->value.typedArray.dispatch->constructorID);
	(*dispatch->value.typedArray.atomics->add)(the, host, offset, the->stack, 0);
	mxPullSlot(mxResult);
}

void fx_Atomics_and(txMachine* the)
{
	mxAtomicsDeclarations(0, 0);
	fxPushAtomicsValue(the, 2, dispatch->value.typedArray.dispatch->constructorID);
	(*dispatch->value.typedArray.atomics->and)(the, host, offset, the->stack, 0);
	mxPullSlot(mxResult);
}

void fx_Atomics_compareExchange(txMachine* the)
{
	mxAtomicsDeclarations(0, 0);
	fxPushAtomicsValue(the, 2, dispatch->value.typedArray.dispatch->constructorID);
	fxPushAtomicsValue(the, 3, dispatch->value.typedArray.dispatch->constructorID);
	(*dispatch->value.typedArray.atomics->compareExchange)(the, host, offset, the->stack, 0);
	mxPullSlot(mxResult);
	mxPop();
}

void fx_Atomics_exchange(txMachine* the)
{
	mxAtomicsDeclarations(0, 0);
	fxPushAtomicsValue(the, 2, dispatch->value.typedArray.dispatch->constructorID);
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
	mxAtomicsDeclarations(0, 0);
	(*dispatch->value.typedArray.atomics->load)(the, host, offset, mxResult, 0);
}

void fx_Atomics_or(txMachine* the)
{
	mxAtomicsDeclarations(0, 0);
	fxPushAtomicsValue(the, 2, dispatch->value.typedArray.dispatch->constructorID);
	(*dispatch->value.typedArray.atomics->or)(the, host, offset, the->stack, 0);
	mxPullSlot(mxResult);
}

void fx_Atomics_notify(txMachine* the)
{
	mxAtomicsDeclarations(1, 0);
	txInteger count = ((mxArgc > 2) && !mxIsUndefined(mxArgv(2))) ? fxToInteger(the, mxArgv(2)) : 20;
	if (count < 0)
		count = 0;
	if (host->kind == XS_ARRAY_BUFFER_KIND) {
		mxResult->value.integer = 0;
	}
	else {
		mxResult->value.integer = fxNotifySharedChunk(the, host->value.host.data, offset, count);
	}
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_Atomics_store(txMachine* the)
{
	mxAtomicsDeclarations(0, 0);
	fxPushAtomicsValue(the, 2, dispatch->value.typedArray.dispatch->constructorID);
	*mxResult = *the->stack;
	(*dispatch->value.typedArray.atomics->store)(the, host, offset, the->stack, 0);
	mxPop();
}

void fx_Atomics_sub(txMachine* the)
{
	mxAtomicsDeclarations(0, 0);
	fxPushAtomicsValue(the, 2, dispatch->value.typedArray.dispatch->constructorID);
	(*dispatch->value.typedArray.atomics->sub)(the, host, offset, the->stack, 0);
	mxPullSlot(mxResult);
}

void fx_Atomics_wait(txMachine* the)
{
	mxAtomicsDeclarations(1, 1);
	txNumber timeout;
	txInteger result;
	fxPushAtomicsValue(the, 2, dispatch->value.typedArray.dispatch->constructorID);
	timeout = (mxArgc > 3) ? fxToNumber(the, mxArgv(3)) : C_NAN;
	if (c_isnan(timeout))
		timeout = C_INFINITY;
	else if (timeout < 0)
		timeout = 0;
	fxLinkSharedChunk(the);
	result = (*dispatch->value.typedArray.atomics->wait)(the, host, offset, the->stack, timeout);
	fxUnlinkSharedChunk(the);
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
	mxAtomicsDeclarations(0, 0);
	fxPushAtomicsValue(the, 2, dispatch->value.typedArray.dispatch->constructorID);
	(*dispatch->value.typedArray.atomics->xor)(the, host, offset, the->stack, 0);
	mxPullSlot(mxResult);
}

#ifdef mxUseDefaultSharedChunks

#if defined(mxUsePOSIXThreads)
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
#if mxThreads
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
		#if mxThreads
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
		#if mxThreads
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

void fxLinkSharedChunk(txMachine* the)
{
	if (gxSharedCluster && (gxSharedCluster->mainThread != mxCurrentThread())) {
#if mxThreads
		txMachine** machineAddress;
		txMachine* machine;
		mxLockMutex(&gxSharedCluster->waiterMutex);
		machineAddress = &gxSharedCluster->waiterLink;
		while ((machine = *machineAddress))
			machineAddress = (txMachine**)&machine->waiterLink;
		*machineAddress = the;
#endif
	}
	else {
		mxTypeError("main thread cannot wait");
	}
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

txInteger fxNotifySharedChunk(txMachine* the, void* data, txInteger offset, txInteger count)
{
	txInteger* address = (txInteger*)((txByte*)data + offset);
	txInteger result = 0;
	if (gxSharedCluster) {
	#if mxThreads
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

void* fxRetainSharedChunk(void* data)
{
	txSharedChunk* chunk = (txSharedChunk*)(((txByte*)data) - sizeof(txSharedChunk));
	txS4 result = 0;
	txS4 value = 1;
	txS4* address = &(chunk->usage);
#ifndef mxUseGCCAtomics
	txBoolean lock = 1;
#endif
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
#ifndef mxUseGCCAtomics
	txBoolean lock = 1;
#endif
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

void fxUnlinkSharedChunk(txMachine* the)
{
#if mxThreads
	txMachine** machineAddress;
	txMachine* machine;
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

txInteger fxWaitSharedChunk(txMachine* the, void* address, txNumber timeout)
{
	txInteger result = 1;
#if mxThreads
	txCondition condition;
	mxCreateCondition(&condition);
	the->waiterCondition = &condition;
	the->waiterData = address;
	if (timeout == C_INFINITY) {
	#if defined(mxUsePOSIXThreads)
		while (the->waiterData == address)
			pthread_cond_wait(&condition, &gxSharedCluster->waiterMutex);
	#elif defined(mxUseFreeRTOSTasks)
		mxUnlockMutex(&gxSharedCluster->waiterMutex);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		mxLockMutex(&gxSharedCluster->waiterMutex);
	#else
		while (the->waiterData == address)
			SleepConditionVariableCS(&condition, &gxSharedCluster->waiterMutex, INFINITE);
	#endif
	}
	else {
	#if defined(mxUsePOSIXThreads)
		struct timespec ts;
		timeout += fxDateNow();
		ts.tv_sec = c_floor(timeout / 1000);
		ts.tv_nsec = c_fmod(timeout, 1000) * 1000000;
		while (the->waiterData == address) {
			result = (pthread_cond_timedwait(&condition, &gxSharedCluster->waiterMutex, &ts) == ETIMEDOUT) ? 0 : 1;
			if (!result)
				break;
		}
	#elif defined(mxUseFreeRTOSTasks)
		mxUnlockMutex(&gxSharedCluster->waiterMutex);
		ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout));
		mxLockMutex(&gxSharedCluster->waiterMutex);
		result = (the->waiterData == address) ? 0 : 1;
	#else
		timeout += fxDateNow();
		while (the->waiterData == address) {
			result = (SleepConditionVariableCS(&condition, &gxSharedCluster->waiterMutex, (DWORD)(timeout - fxDateNow()))) ? 1 : 0;
			if (!result)
				break;
		}
	#endif
	}
	the->waiterCondition = C_NULL;
	mxDeleteCondition(&condition);
#endif
	return result;
}

#endif /* mxUseDefaultSharedChunks */



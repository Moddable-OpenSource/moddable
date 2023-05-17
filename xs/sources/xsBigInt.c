/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

#ifdef mxRun
static txSlot* fxBigIntCheck(txMachine* the, txSlot* it);
static txBigInt* fxIntegerToBigInt(txMachine* the, txSlot* slot);
static txBigInt* fxNumberToBigInt(txMachine* the, txSlot* slot);
static txBigInt* fxStringToBigInt(txMachine* the, txSlot* slot, txFlag whole);
#endif

#ifndef howmany
#define howmany(x, y)	(((x) + (y) - 1) / (y))
#endif
#ifndef MAX
#define MAX(a, b)	((a) >= (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)	((a) <= (b) ? (a) : (b))
#endif

const txU4 gxDataOne[1] = { 1 };
const txU4 gxDataZero[1] = { 0 };
const txBigInt gxBigIntNaN = { .sign=0, .size=0, .data=(txU4*)gxDataZero };
const txBigInt gxBigIntOne = { .sign=0, .size=1, .data=(txU4*)gxDataOne };
const txBigInt gxBigIntZero = { .sign=0, .size=1, .data=(txU4*)gxDataZero };

static txBigInt *fxBigInt_fit(txMachine* the, txBigInt *r);

#ifdef mxMetering
static void fxBigInt_meter(txMachine* the, int n);
#define mxBigInt_meter(N) if (the) fxBigInt_meter(the, N)
#else
#define mxBigInt_meter(N)
#endif

// BYTE CODE

#ifdef mxRun

void fxBuildBigInt(txMachine* the)
{
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_prototype_toString), 0, mxID(_toLocaleString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_prototype_toString), 0, mxID(_toString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_prototype_valueOf), 0, mxID(_valueOf), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "BigInt", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxBigIntPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_BigInt), 1, mxID(_BigInt));
	mxBigIntConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_asIntN), 2, mxID(_asIntN), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_asUintN), 2, mxID(_asUintN), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_bitLength), 1, mxID(_bitLength), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_fromArrayBuffer), 1, mxID(_fromArrayBuffer), XS_DONT_ENUM_FLAG);
	mxPop();
}

void fx_BigInt(txMachine* the)
{
	if (mxTarget->kind != XS_UNDEFINED_KIND)
		mxTypeError("new BigInt");
	if (mxArgc > 0)
		*mxResult = *mxArgv(0);
	fxToPrimitive(the, mxResult, XS_NUMBER_HINT);
	if (mxResult->kind == XS_NUMBER_KIND) {
		int fpclass = c_fpclassify(mxResult->value.number);
		txNumber check = c_trunc(mxResult->value.number);
		if ((fpclass != FP_NAN) && (fpclass != FP_INFINITE) && (mxResult->value.number == check))
			fxNumberToBigInt(the, mxResult);
		else
			mxRangeError("Cannot coerce number to bigint");
	}
	else if (mxResult->kind == XS_INTEGER_KIND) {
		fxIntegerToBigInt(the, mxResult);
	}
	else
		fxToBigInt(the, mxResult, 1);
}

txNumber fx_BigInt_asAux(txMachine* the)
{
	txNumber value, index;
	if (mxArgc < 1)
		index = 0;
	else {
		if (mxArgv(0)->kind == XS_UNDEFINED_KIND)
			index = 0;
		else {
			value = fxToNumber(the, mxArgv(0));
			if (c_isnan(value))
				index = 0;
			else {
				value = c_trunc(value);
				if (value < 0)
					mxRangeError("out of range index");
				index = value;
				if (index <= 0)
					index = 0;
				else if (index > C_MAX_SAFE_INTEGER)
					index = C_MAX_SAFE_INTEGER;
				if (value != index)
					mxRangeError("out of range index");
			}
		}
	}
	if (mxArgc < 2)
		mxTypeError("no bigint");
	return index;
}

void fx_BigInt_asIntN(txMachine* the)
{
	txInteger index = (txInteger)fx_BigInt_asAux(the);
	txBigInt* arg = fxToBigInt(the, mxArgv(1), 1);
	txBigInt* result;
	if (fxBigInt_iszero(arg)) {
		result = fxBigInt_dup(the, arg);
	}
	else {
		txBigInt* bits = fxBigInt_ulsl1(the, C_NULL, (txBigInt *)&gxBigIntOne, index);
		txBigInt* mask = fxBigInt_usub(the, C_NULL, bits, (txBigInt *)&gxBigIntOne);
		result = fxBigInt_uand(the, C_NULL, arg, mask);
		if ((arg->sign) && !fxBigInt_iszero(result))
			result = fxBigInt_usub(the, C_NULL, bits, result);
		if (index && fxBigInt_comp(result, fxBigInt_ulsl1(the, C_NULL, (txBigInt *)&gxBigIntOne, index - 1)) >= 0)
			result = fxBigInt_sub(the, C_NULL, result, bits);
		result = fxBigInt_fit(the, result);
	}
	mxResult->value.bigint = *result;
	mxResult->kind = XS_BIGINT_KIND;
// 	txBigInt* bits = fxBigInt_ulsl1(the, C_NULL, (txBigInt *)&gxBigIntOne, index);
// 	txBigInt* result = fxBigInt_mod(the, C_NULL, fxToBigInt(the, mxArgv(1), 1), bits);
// 	if (index && fxBigInt_comp(result, fxBigInt_ulsl1(the, C_NULL, (txBigInt *)&gxBigIntOne, index - 1)) >= 0)
// 		result = fxBigInt_sub(the, C_NULL, result, bits);
// 	mxResult->value.bigint = *result;
// 	mxResult->kind = XS_BIGINT_KIND;
}

void fx_BigInt_asUintN(txMachine* the)
{
	txInteger index = (txInteger)fx_BigInt_asAux(the);
	txBigInt* arg = fxToBigInt(the, mxArgv(1), 1);
	txBigInt* result;
	if (fxBigInt_iszero(arg)) {
		result = fxBigInt_dup(the, arg);
	}
	else {
		txBigInt* bits = fxBigInt_ulsl1(the, C_NULL, (txBigInt *)&gxBigIntOne, index);
		txBigInt* mask = fxBigInt_sub(the, C_NULL, bits, (txBigInt *)&gxBigIntOne);
		result = fxBigInt_uand(the, C_NULL, arg, mask);
		if ((arg->sign) && !fxBigInt_iszero(result))
			result = fxBigInt_usub(the, C_NULL, bits, result);
		result = fxBigInt_fit(the, result);
	}
	mxResult->value.bigint = *result;
	mxResult->kind = XS_BIGINT_KIND;
// 	fxToBigInt(the, mxArgv(1), 1);
// 	mxResult->value.bigint = *fxBigInt_mod(the, C_NULL, &mxArgv(1)->value.bigint, fxBigInt_ulsl1(the, C_NULL, (txBigInt *)&gxBigIntOne, index));
// 	mxResult->kind = XS_BIGINT_KIND;
}

void fx_BigInt_bitLength(txMachine *the)
{
	txBigInt* arg = fxToBigInt(the, mxArgv(0), 1);
	mxResult->value.integer = fxBigInt_bitsize(arg);;
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_BigInt_fromArrayBuffer(txMachine* the)
{
	txSlot* slot;
	txSlot* arrayBuffer = C_NULL;
	txSlot* bufferInfo;
	txBoolean sign = 0;
	int endian = EndianBig;
	txInteger length;
	txBigInt* bigint;
	txU1 *src, *dst;
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
	bufferInfo = arrayBuffer->next;
	length = bufferInfo->value.bufferInfo.length;
	if ((mxArgc > 1) && fxToBoolean(the, mxArgv(1)))
		sign = 1;
	if ((mxArgc > 2) && fxToBoolean(the, mxArgv(2)))
		endian = EndianLittle;
    if (sign)
        length--;
	if (length <= 0) {
		mxSyntaxError("invalid ArrayBuffer instance");
// 		mxResult->value.bigint = gxBigIntNaN;
// 		mxResult->kind = XS_BIGINT_X_KIND;
		return;
	}
	bigint = fxBigInt_alloc(the, howmany(length, sizeof(txU4)));
	bigint->data[bigint->size - 1] = 0;
	src = (txU1*)(arrayBuffer->value.arrayBuffer.address);
	dst = (txU1*)(bigint->data);
	if (sign)
		bigint->sign = *src++;
#if mxBigEndian
	if (endian != EndianLittle) {
#else
	if (endian != EndianBig) {
#endif	
		c_memcpy(dst, src, length);
	}
	else {
		dst += length;
		while (length > 0) {
			dst--;
			*dst = *src;
			src++;
			length--;
		}
	}
	length = bigint->size - 1;
	while (length && (bigint->data[length] == 0))
		length--;
	bigint->size = length + 1;
	mxPullSlot(mxResult);
}

void fx_BigInt_prototype_toString(txMachine* the)
{
	txSlot* slot;
	txU4 radix;
	slot = fxBigIntCheck(the, mxThis);
	if (!slot) mxTypeError("this is no bigint");
	if (mxArgc == 0)
		radix = 10;
	else if (mxIsUndefined(mxArgv(0)))
		radix = 10;
	else {
		radix = fxToInteger(the, mxArgv(0));
		if ((radix < 2) || (36 < radix))
			mxRangeError("invalid radix");
	}
	mxPushSlot(slot);
	fxBigintToString(the, the->stack, radix);
	mxPullSlot(mxResult);
}

void fx_BigInt_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxBigIntCheck(the, mxThis);
	if (!slot) mxTypeError("this is no bigint");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

void fxBigInt(txMachine* the, txSlot* slot, txU1 sign, txU2 size, txU4* data)
{
	txBigInt* bigint = fxBigInt_alloc(the, size);
	c_memcpy(bigint->data, data, size * sizeof(txU4));
	bigint->sign = sign;
	mxPullSlot(slot);
}

void fxBigIntX(txMachine* the, txSlot* slot, txU1 sign, txU2 size, txU4* data)
{
	slot->value.bigint.data = data;
	slot->value.bigint.size = size;
	slot->value.bigint.sign = sign;
	slot->kind = XS_BIGINT_X_KIND;
}

txSlot* fxBigIntCheck(txMachine* the, txSlot* it)
{
	txSlot* result = C_NULL;
	if ((it->kind == XS_BIGINT_KIND) || (it->kind == XS_BIGINT_X_KIND))
		result = it;
	else if (it->kind == XS_REFERENCE_KIND) {
		txSlot* instance = it->value.reference;
		it = instance->next;
		if ((it) && (it->flag & XS_INTERNAL_FLAG) && ((it->kind == XS_BIGINT_KIND) || (it->kind == XS_BIGINT_X_KIND)))
			result = it;
	}
	return result;
}

void fxBigIntCoerce(txMachine* the, txSlot* slot)
{
	fxToBigInt(the, slot, 1);
}

txBoolean fxBigIntCompare(txMachine* the, txBoolean less, txBoolean equal, txBoolean more, txSlot* left, txSlot* right)
{
	int result;
	txNumber delta = 0;
	if (mxBigIntIsNaN(&left->value.bigint))
		return less & more & !equal;
	if (right->kind == XS_STRING_KIND)
		fxStringToBigInt(the, right, 1);
	if ((right->kind != XS_BIGINT_KIND) && (right->kind != XS_BIGINT_X_KIND)) {
		fxToNumber(the, right);
		result = c_fpclassify(right->value.number);
		if (result == FP_NAN)
			return less & more & !equal;
		if (result == C_FP_INFINITE)
			return (right->value.number > 0) ? less : more;
		delta = right->value.number - c_trunc(right->value.number);
		fxNumberToBigInt(the, right);
	}
	if (mxBigIntIsNaN(&right->value.bigint))
		return less & more & !equal;
	result = fxBigInt_comp(&left->value.bigint, &right->value.bigint);
	if (result < 0)
		return less;
    if (result > 0)
        return more;
	if (delta > 0)
		return less;
	if (delta < 0)
		return more;
	return equal;
}

void fxBigIntDecode(txMachine* the, txSize size)
{
	txBigInt* bigint;
	mxPushUndefined();
	bigint = &the->stack->value.bigint;
	bigint->data = fxNewChunk(the, size);
	bigint->size = size >> 2;
	bigint->sign = 0;
#if mxBigEndian
	{
		txS1* src = (txS1*)the->code;
		txS1* dst = (txS1*)bigint->data;
		while (size) {
			dst[3] = *src++;
			dst[2] = *src++;
			dst[1] = *src++;
			dst[0] = *src++;
			dst += 4;
			size--;
		}
	}
#else
	c_memcpy(bigint->data, the->code, size);
#endif	
	the->stack->kind = XS_BIGINT_KIND;
}

#endif	

void fxBigIntEncode(txByte* code, txBigInt* bigint, txSize size)
{
#if mxBigEndian
	{
		txS1* src = (txS1*)bigint->data;
		txS1* dst = (txS1*)code;
		while (size) {
			dst[3] = *src++;
			dst[2] = *src++;
			dst[1] = *src++;
			dst[0] = *src++;
			dst += 4;
			size--;
		}
	}
#else
	c_memcpy(code, bigint->data, size);
#endif
}

#ifdef mxRun
txSlot* fxBigIntToInstance(txMachine* the, txSlot* slot)
{
	txSlot* instance;
	txSlot* internal;
	mxPush(mxBigIntPrototype);
	instance = fxNewObjectInstance(the);
	internal = instance->next = fxNewSlot(the);
	internal->flag = XS_INTERNAL_FLAG;
	internal->kind = slot->kind;
	internal->value = slot->value;
	if (the->frame->flag & XS_STRICT_FLAG)
		instance->flag |= XS_DONT_PATCH_FLAG;
	mxPullSlot(slot);
	return instance;
}
#endif

txSize fxBigIntMeasure(txBigInt* bigint)
{
	return bigint->size * sizeof(txU4);
}

txSize fxBigIntMaximum(txSize length)
{
	return sizeof(txU4) * (1 + (((txSize)c_ceil((txNumber)length * c_log(10) / c_log(2))) / 32));
}

txSize fxBigIntMaximumB(txSize length)
{
	return sizeof(txU4) * (1 + howmany(1, mxBigIntWordSize) + (length / 32));
}

txSize fxBigIntMaximumO(txSize length)
{
	return sizeof(txU4) * (1 + howmany(3, mxBigIntWordSize) + ((length * 3) / 32));
}

txSize fxBigIntMaximumX(txSize length)
{
	return sizeof(txU4) * (1 + howmany(4, mxBigIntWordSize) + ((length * 4) / 32));
}

void fxBigIntParse(txBigInt* bigint, txString p, txSize length, txInteger sign)
{
	txU4 data[1] = { 0 };
	txBigInt digit = { .sign=0, .size=1, .data=data };
	bigint->sign = 0;
	bigint->size = 1;
	bigint->data[0] = 0;
	while (length) {
		char c = *p++;
		data[0] = c - '0';
		fxBigInt_uadd(NULL, bigint, fxBigInt_umul1(NULL, bigint, bigint, 10), &digit);
		length--;
	}
	if ((bigint->size > 1) || (bigint->data[0] != 0))
		bigint->sign = sign;
}

void fxBigIntParseB(txBigInt* bigint, txString p, txSize length)
{
	txU4 data[1] = { 0 };
	txBigInt digit = { .sign=0, .size=1, .data=data };
	bigint->sign = 0;
	bigint->size = 1;
	bigint->data[0] = 0;
	while (length) {
		char c = *p++;
		data[0] = c - '0';
		fxBigInt_uadd(NULL, bigint, fxBigInt_ulsl1(NULL, bigint, bigint, 1), &digit);
		length--;
	}
}

void fxBigIntParseO(txBigInt* bigint, txString p, txSize length)
{
	txU4 data[1] = { 0 };
	txBigInt digit = { .sign=0, .size=1, .data=data };
	bigint->sign = 0;
	bigint->size = 1;
	bigint->data[0] = 0;
	while (length) {
		char c = *p++;
		data[0] = c - '0';
		fxBigInt_uadd(NULL, bigint, fxBigInt_ulsl1(NULL, bigint, bigint, 3), &digit);
		length--;
	}
}

void fxBigIntParseX(txBigInt* bigint, txString p, txSize length)
{
	txU4 data[1] = { 0 };
	txBigInt digit = { .sign=0, .size=1, .data=data };
	bigint->sign = 0;
	bigint->size = 1;
	bigint->data[0] = 0;
	while (length) {
		char c = *p++;
		if (('0' <= c) && (c <= '9'))
			data[0] = c - '0';
		else if (('a' <= c) && (c <= 'f'))
			data[0] = 10 + c - 'a';
		else
			data[0] = 10 + c - 'A';
		fxBigInt_uadd(NULL, bigint, fxBigInt_ulsl1(NULL, bigint, bigint, 4), &digit);
		length--;
	}
}

#ifdef mxRun

void fxBigintToArrayBuffer(txMachine* the, txSlot* slot, txU4 total, txBoolean sign, int endian)
{
	txBigInt* bigint = fxToBigInt(the, slot, 1);
	txU4 length = howmany(fxBigInt_bitsize(bigint), 8);
	txU4 offset = 0;
	txU1 *src, *dst;
	if (length == 0)
		length = 1;
	if (length < total) {
		if (endian == EndianBig)
			offset = total - length;
	}
	else {
		total = length;
	}
	if (sign) {
		offset++;
		total++;
	}
	fxConstructArrayBufferResult(the, mxThis, total);
	src = (txU1*)(bigint->data);
	dst = (txU1*)(mxResult->value.reference->next->value.arrayBuffer.address);
	if (sign)
		*dst = bigint->sign;
	dst += offset;
#if mxBigEndian
	if (endian != EndianLittle) {
#else
	if (endian != EndianBig) {
#endif	
		c_memcpy(dst, src, length);
	}
	else {
		src += length;
		while (length > 0) {
			src--;
			*dst = *src;
			dst++;
			length--;
		}
	}
}

txNumber fxBigIntToNumber(txMachine* the, txSlot* slot)
{
	txBigInt* bigint = &slot->value.bigint;
	txNumber base = c_pow(2, 32);
	txNumber number = 0;
	int i;
	for (i = bigint->size; --i >= 0;)
		number = (number * base) + bigint->data[i];
	if (bigint->sign)
		number = -number;
	slot->value.number = number;
	slot->kind = XS_NUMBER_KIND;
	return number;
}

void fxBigintToString(txMachine* the, txSlot* slot, txU4 radix)
{
	static const char gxDigits[] ICACHE_FLASH_ATTR = "0123456789abcdefghijklmnopqrstuvwxyz";
	txU4 data[1] = { 10 };
	txBigInt divider = { .sign=0, .size=1, .data=data };
	txSize length, offset;
	txBoolean minus = 0;
	txSlot* result;
	txSlot* stack;
	
	if (mxBigIntIsNaN(&slot->value.bigint)) {
		fxStringX(the, slot, "NaN");
		return;
	}
	
	mxMeterSome(slot->value.bigint.size);
	
	mxPushUndefined();
	result = the->stack;
	
	mxPushSlot(slot);
	stack = the->stack;
	
	if (radix)
		divider.data[0] = radix;
	
	length = 1 + (txSize)c_ceil((txNumber)stack->value.bigint.size * 32 * c_log(2) / c_log(data[0]));
	if (stack->value.bigint.sign) {
		stack->value.bigint.sign = 0;
		length++;
		minus = 1;
	}
	offset = length;
	result->value.string = fxNewChunk(the, length);
	result->kind = XS_STRING_KIND;

	result->value.string[--offset] = 0;
	do {
		txBigInt* remainder = NULL;
		txBigInt* quotient = fxBigInt_udiv(the, C_NULL, &stack->value.bigint, &divider, &remainder);
		result->value.string[--offset] = c_read8(gxDigits + remainder->data[0]);
        stack->value.bigint = *quotient;
        stack->kind = XS_BIGINT_KIND;
		the->stack = stack;
	}
	while (!fxBigInt_iszero(&stack->value.bigint));
	if (minus)
		result->value.string[--offset] = '-';
	c_memmove(result->value.string, result->value.string + offset, length - offset);
	
	mxPop();
	mxPullSlot(slot);
}

txS8 fxToBigInt64(txMachine* the, txSlot* slot)
{
	return (txS8)fxToBigUint64(the, slot);
}

txU8 fxToBigUint64(txMachine* the, txSlot* slot)
{
	txBigInt* bigint = fxToBigInt(the, slot, 1);
	txU8 result = bigint->data[0];
	if (bigint->size > 1)
		result |= (txU8)(bigint->data[1]) << 32;
	if (bigint->sign && result) {
		result--;
		result = 0xFFFFFFFFFFFFFFFFll - result;
	}
	return result;
}

txBigInt* fxIntegerToBigInt(txMachine* the, txSlot* slot)
{
	txInteger integer = slot->value.integer;
	txBigInt* bigint = &slot->value.bigint;
	txU1 sign = 0;
	if (integer < 0) {
		integer = -integer;
		sign = 1;
	}
	bigint->data = fxNewChunk(the, sizeof(txU4));
	bigint->data[0] = (txU4)integer;
	bigint->sign = sign;
	bigint->size = 1;
	slot->kind = XS_BIGINT_KIND;
	return bigint;
}

txBigInt* fxNumberToBigInt(txMachine* the, txSlot* slot)
{
	txBigInt* bigint = &slot->value.bigint;
	txNumber number = slot->value.number;
	txNumber limit = c_pow(2, 32);
	txU1 sign = 0;
	txU2 size = 1;
	if (number < 0) {
		number = -number;
		sign = 1;
	}
	while (number >= limit) {
		size++;
		number /= limit;
	}
	bigint->data = fxNewChunk(the, fxMultiplyChunkSizes(the, size, sizeof(txU4)));
	bigint->size = size;
	while (size > 0) {
		txU4 part = (txU4)number;
		number -= part;
		size--;
		bigint->data[size] = part;
		number *= limit;
	}
	bigint->sign = sign;
	slot->kind = XS_BIGINT_KIND;
	return bigint;
}

txBigInt* fxStringToBigInt(txMachine* the, txSlot* slot, txFlag whole)
{
	txString s = slot->value.string;
	txString p = fxSkipSpaces(s);
	txSize offset, length;
	txInteger sign = 0;
	txBigInt bigint = {.sign=0, .size=0, .data=C_NULL };
	char c = *p;
	if (c == '0') {
		char d = *(p + 1);
		if (whole && ((d == 'B') || (d == 'b') || (d == 'O') || (d == 'o') || (d == 'X') || (d == 'x'))) {
			p += 2;
			offset = mxPtrDiff(p - s);
			if ((d == 'B') || (d == 'b')) {
				while (((c = *p)) && ('0' <= c) && (c <= '1'))
					p++;
				length = mxPtrDiff(p - s - offset);
				p = fxSkipSpaces(p);
				if ((length > 0) && (*p == 0)) {
					bigint.data = fxNewChunk(the, fxBigIntMaximumB(length));
					fxBigIntParseB(&bigint, slot->value.string + offset, length);
				}
			}
			else if ((d == 'O') || (d == 'o')) {
				while (((c = *p)) && ('0' <= c) && (c <= '7'))
					p++;
				length = mxPtrDiff(p - s - offset);
				p = fxSkipSpaces(p);
				if ((length > 0) && (*p == 0)) {
					bigint.data = fxNewChunk(the, fxBigIntMaximumO(length));
					fxBigIntParseO(&bigint, slot->value.string + offset, length);
				}
			}
			else if ((d == 'X') || (d == 'x')) {
				while (((c = *p)) && ((('0' <= c) && (c <= '9')) || (('a' <= c) && (c <= 'f')) || (('A' <= c) && (c <= 'F'))))
					p++;
				length = mxPtrDiff(p - s - offset);
				p = fxSkipSpaces(p);
				if ((length > 0) && (*p == 0)) {
					bigint.data = fxNewChunk(the, fxBigIntMaximumX(length));
					fxBigIntParseX(&bigint, slot->value.string + offset, length);
				}
			}
			goto bail;
		}
	}
	if (c == '-') {
		sign = 1;
		p++;
	}
	else if (c == '+') {
		p++;
	}
	offset = mxPtrDiff(p - s);
	while (((c = *p)) && ('0' <= c) && (c <= '9'))
		p++;
	length = mxPtrDiff(p - s - offset);
	p = fxSkipSpaces(p);
	if (*p == 0) {
		bigint.data = fxNewChunk(the, fxBigIntMaximum(length));
		fxBigIntParse(&bigint, slot->value.string + offset, length, sign);
	}
bail:
	if (bigint.data) {
		slot->value.bigint = bigint;
		slot->kind = XS_BIGINT_KIND;
	}
	else {
		slot->value.bigint = gxBigIntNaN;
		slot->kind = XS_BIGINT_X_KIND;
	}
	return &slot->value.bigint;
}

txBigInt* fxToBigInt(txMachine* the, txSlot* slot, txFlag strict)
{
again:
	switch (slot->kind) {
	case XS_BOOLEAN_KIND:
		slot->value.bigint = *fxBigInt_dup(the, (txBigInt *)((slot->value.boolean) ? &gxBigIntOne : &gxBigIntZero));
		slot->kind = XS_BIGINT_KIND;
		break;
	case XS_INTEGER_KIND:
		if (strict)
			mxTypeError("Cannot coerce number to bigint");
		fxIntegerToBigInt(the, slot);	
		break;
	case XS_NUMBER_KIND:
		if (strict)
			mxTypeError("Cannot coerce number to bigint");
		fxNumberToBigInt(the, slot);	
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		fxStringToBigInt(the, slot, 1);
		if (mxBigIntIsNaN(&slot->value.bigint))
			mxSyntaxError("Cannot coerce string to bigint");
		break;
	case XS_BIGINT_KIND:
	case XS_BIGINT_X_KIND:
		break;
	case XS_SYMBOL_KIND:
		mxTypeError("Cannot coerce symbol to bigint");
		break;
	case XS_REFERENCE_KIND:
		fxToPrimitive(the, slot, XS_NUMBER_HINT);
		goto again;
	default:
		mxTypeError("Cannot coerce to bigint");
		break;
	}
	return &slot->value.bigint;
}

void fxFromBigInt64(txMachine* the, txSlot* slot, txS8 value)
{
	txU1 sign = 0;
	if (value < 0) {
		value = -value;
		sign = 1;
	}
	if (value > 0x00000000FFFFFFFFll) {
		slot->value.bigint.data = fxNewChunk(the, 2 * sizeof(txU4));
		slot->value.bigint.data[0] = (txU4)(value);
		slot->value.bigint.data[1] = (txU4)(value >> 32);
		slot->value.bigint.size = 2;
	}
	else {
		slot->value.bigint.data = fxNewChunk(the, sizeof(txU4));
		slot->value.bigint.data[0] = (txU4)value;
		slot->value.bigint.size = 1;
	}
    slot->value.bigint.sign = sign;
	slot->kind = XS_BIGINT_KIND;
}

void fxFromBigUint64(txMachine* the, txSlot* slot, txU8 value)
{
	if (value > 0x00000000FFFFFFFFll) {
		slot->value.bigint.data = fxNewChunk(the, 2 * sizeof(txU4));
		slot->value.bigint.data[0] = (txU4)(value);
		slot->value.bigint.data[1] = (txU4)(value >> 32);
		slot->value.bigint.size = 2;
	}
	else {
		slot->value.bigint.data = fxNewChunk(the, sizeof(txU4));
		slot->value.bigint.data[0] = (txU4)value;
		slot->value.bigint.size = 1;
	}
    slot->value.bigint.sign = 0;
	slot->kind = XS_BIGINT_KIND;
}

#endif

txBigInt *fxBigInt_alloc(txMachine* the, txU4 size)
{
#ifdef mxRun
	txBigInt* bigint;
	if (size > 0xFFFF) {
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	}
	mxPushUndefined();
	bigint = &the->stack->value.bigint;
	bigint->data = fxNewChunk(the, fxMultiplyChunkSizes(the, size, sizeof(txU4)));
	bigint->size = size;
	bigint->sign = 0;
	the->stack->kind = XS_BIGINT_KIND;
	return bigint;
#else
	return NULL;
#endif
}

void fxBigInt_free(txMachine* the, txBigInt *bigint)
{
#ifdef mxRun
	if (bigint == &the->stack->value.bigint)
		the->stack++;
// 	else
// 		fprintf(stderr, "oops\n");
#endif
}

int fxBigInt_comp(txBigInt *a, txBigInt *b)
{
	if (a->sign != b->sign)
		return(a->sign ? -1: 1);
	else if (a->sign)
		return(fxBigInt_ucomp(b, a));
	else
		return(fxBigInt_ucomp(a, b));
}

int fxBigInt_ucomp(txBigInt *a, txBigInt *b)
{
	int i;

	if (a->size != b->size)
		return(a->size > b->size ? 1: -1);
	for (i = a->size; --i >= 0;) {
		if (a->data[i] != b->data[i])
			return(a->data[i] > b->data[i] ? 1: -1);
	}
	return(0);
}

int fxBigInt_iszero(txBigInt *a)
{
	return(a->size == 1 && a->data[0] == 0);
}

void
fxBigInt_fill0(txBigInt *r)
{
	c_memset(r->data, 0, r->size * sizeof(txU4));
}

void fxBigInt_copy(txBigInt *a, txBigInt *b)
{
	c_memmove(a->data, b->data, b->size * sizeof(txU4));
	a->size = b->size;
	a->sign = b->sign;
}

txBigInt *fxBigInt_dup(txMachine* the, txBigInt *a)
{
	txBigInt *r = fxBigInt_alloc(the, a->size);
	fxBigInt_copy(r, a);
	return(r);
}

int
fxBigInt_ffs(txBigInt *a)
{
	int i;
	txU4 w = a->data[a->size - 1];

	for (i = 0; i < (int)mxBigIntWordSize && !(w & ((txU4)1 << (mxBigIntWordSize - 1 - i))); i++)
		;
	return(i);
}

txBigInt *fxBigInt_fit(txMachine* the, txBigInt *r)
{
	int i = r->size;
	while (i > 0) {
		i--;
		if (r->data[i])
			break;
	}
	r->size = (txU2)(i + 1);
	mxBigInt_meter(r->size);
	return r;
}


#if mxWindows
int
fxBigInt_bitsize(txBigInt *e)
{
	return(e->size * mxBigIntWordSize - fxBigInt_ffs(e));
}
#else
int
fxBigInt_bitsize(txBigInt *a)
{
	int i = a->size;
	txU4 w;
	while (i > 0) {
		i--;
		w = a->data[i];
		if (w) {
			int c = __builtin_clz(w);
			return (i * mxBigIntWordSize) + mxBigIntWordSize - c;
		}
	}
	return 0;
}
#endif

int fxBigInt_isset(txBigInt *e, txU4 i)
{
	txU4 w = e->data[i / mxBigIntWordSize];
	return((w & ((txU4)1 << (i % mxBigIntWordSize))) != 0);
}

/*
 * bitwise operations
 */

txBigInt *fxBigInt_not(txMachine* the, txBigInt *r, txBigInt *a)
{
	if (a->sign)
		r = fxBigInt_usub(the, r, a, (txBigInt *)&gxBigIntOne);
	else {
		r = fxBigInt_uadd(the, r, a, (txBigInt *)&gxBigIntOne);
		r->sign = 1;
	}
	return(r);
}

txBigInt *fxBigInt_and(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b)
{
	txBigInt *aa, *bb;
	int i;
	if (a->sign) {
		if (b->sign)
			goto AND_MINUS_MINUS;
		aa = a;
		a = b;
		b = aa;	
		goto AND_PLUS_MINUS;
	}
	if (b->sign)
		goto AND_PLUS_MINUS;
	return fxBigInt_fit(the, fxBigInt_uand(the, r, a, b));
AND_PLUS_MINUS:
// GMP: OP1 & -OP2 == OP1 & ~(OP2 - 1)
	if (r == NULL)
		r = fxBigInt_alloc(the, a->size);
	bb = fxBigInt_usub(the, NULL, b, (txBigInt *)&gxBigIntOne);
    if (a->size > bb->size) {
		for (i = 0; i < bb->size; i++)
			r->data[i] = a->data[i] & ~bb->data[i];
		for (; i < a->size; i++)
			r->data[i] = a->data[i];
    }
    else {
		for (i = 0; i < a->size; i++)
			r->data[i] = a->data[i] & ~bb->data[i];
    }
    fxBigInt_free(the, bb);
	return fxBigInt_fit(the, r);
AND_MINUS_MINUS:
// GMP: -((-OP1) & (-OP2)) = -(~(OP1 - 1) & ~(OP2 - 1)) == ~(~(OP1 - 1) & ~(OP2 - 1)) + 1 == ((OP1 - 1) | (OP2 - 1)) + 1
	if (r == NULL)
		r = fxBigInt_alloc(the, MAX(a->size, b->size) + 1);
	aa = fxBigInt_usub(the, NULL, a, (txBigInt *)&gxBigIntOne);
	bb = fxBigInt_usub(the, NULL, b, (txBigInt *)&gxBigIntOne);
	r = fxBigInt_uor(the, r, aa, bb);
	r = fxBigInt_uadd(the, r, r, (txBigInt *)&gxBigIntOne);
	r->sign = 1;
	fxBigInt_free(the, bb);
	fxBigInt_free(the, aa);
	return fxBigInt_fit(the, r);
}

txBigInt *fxBigInt_uand(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b)
{
	int i;
	if (a->size > b->size) {
		txBigInt *t = b;
		b = a;
		a = t;
	}
	if (r == NULL)
		r = fxBigInt_alloc(the, a->size);
	else
		r->size = a->size;
	for (i = 0; i < a->size; i++)
		r->data[i] = a->data[i] & b->data[i];
	return(r);
}

txBigInt *fxBigInt_or(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b)
{
	txBigInt *aa, *bb;
	int i;
	mxBigInt_meter(MAX(a->size, b->size));
	if (a->sign) {
		if (b->sign)
			goto OR_MINUS_MINUS;
		aa = a;
		a = b;
		b = aa;	
		goto OR_PLUS_MINUS;
	}
	if (b->sign)
		goto OR_PLUS_MINUS;
	return fxBigInt_fit(the, fxBigInt_uor(the, r, a, b));
OR_PLUS_MINUS:
// GMP: -(OP1 | (-OP2)) = -(OP1 | ~(OP2 - 1)) == ~(OP1 | ~(OP2 - 1)) + 1 == (~OP1 & (OP2 - 1)) + 1
	if (r == NULL)
		r = fxBigInt_alloc(the, b->size + 1);
	bb = fxBigInt_usub(the, NULL, b, (txBigInt *)&gxBigIntOne);
    if (a->size < bb->size) {
		for (i = 0; i < a->size; i++)
			r->data[i] = ~a->data[i] & bb->data[i];
		for (; i < bb->size; i++)
			r->data[i] = bb->data[i];
	}
	else {
		for (i = 0; i < bb->size; i++)
			r->data[i] = ~a->data[i] & bb->data[i];
	}
	r->size = bb->size;
	r = fxBigInt_uadd(the, r, r, (txBigInt *)&gxBigIntOne);
	r->sign = 1;
	fxBigInt_free(the, bb);
	return fxBigInt_fit(the, r);
OR_MINUS_MINUS:
// GMP: -((-OP1) | (-OP2)) = -(~(OP1 - 1) | ~(OP2 - 1)) == ~(~(OP1 - 1) | ~(OP2 - 1)) + 1 = = ((OP1 - 1) & (OP2 - 1)) + 1
	if (r == NULL)
		r = fxBigInt_alloc(the, MIN(a->size, b->size) + 1);
	aa = fxBigInt_usub(the, NULL, a, (txBigInt *)&gxBigIntOne);
	bb = fxBigInt_usub(the, NULL, b, (txBigInt *)&gxBigIntOne);
	r = fxBigInt_uand(the, r, aa, bb);
	r = fxBigInt_uadd(the, r, r, (txBigInt *)&gxBigIntOne);
	r->sign = 1;
	fxBigInt_free(the, bb);
	fxBigInt_free(the, aa);
	return fxBigInt_fit(the, r);
}

txBigInt *fxBigInt_uor(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b)
{
	int i;
	if (a->size < b->size) {
		txBigInt *t = b;
		b = a;
		a = t;
	}
	if (r == NULL)
		r = fxBigInt_alloc(the, a->size);
	else
		r->size = a->size;
	for (i = 0; i < b->size; i++)
		r->data[i] = a->data[i] | b->data[i];
	for (; i < a->size; i++)
		r->data[i] = a->data[i];
	return(r);
}

txBigInt *fxBigInt_xor(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b)
{
	txBigInt *aa, *bb;
	if (a->sign) {
		if (b->sign)
			goto XOR_MINUS_MINUS;
		aa = a;
		a = b;
		b = aa;	
		goto XOR_PLUS_MINUS;
	}
	if (b->sign)
		goto XOR_PLUS_MINUS;
	return fxBigInt_fit(the, fxBigInt_uxor(the, r, a, b));
XOR_PLUS_MINUS:
// GMP: -(OP1 ^ (-OP2)) == -(OP1 ^ ~(OP2 - 1)) == ~(OP1 ^ ~(OP2 - 1)) + 1 == (OP1 ^ (OP2 - 1)) + 1
	if (r == NULL)
		r = fxBigInt_alloc(the, MAX(a->size, b->size) + 1);
	bb = fxBigInt_usub(the, NULL, b, (txBigInt *)&gxBigIntOne);
	r = fxBigInt_uxor(the, r, a, bb);
	r = fxBigInt_uadd(the, r, r, (txBigInt *)&gxBigIntOne);
	r->sign = 1;
	fxBigInt_free(the, bb);
	return fxBigInt_fit(the, r);
XOR_MINUS_MINUS:
// GMP: (-OP1) ^ (-OP2) == ~(OP1 - 1) ^ ~(OP2 - 1) == (OP1 - 1) ^ (OP2 - 1)
	if (r == NULL)
		r = fxBigInt_alloc(the, MAX(a->size, b->size));
	aa = fxBigInt_usub(the, NULL, a, (txBigInt *)&gxBigIntOne);
	bb = fxBigInt_usub(the, NULL, b, (txBigInt *)&gxBigIntOne);
	r = fxBigInt_uxor(the, r, aa, bb);
	fxBigInt_free(the, bb);
	fxBigInt_free(the, aa);
	return fxBigInt_fit(the, r);
}

txBigInt *fxBigInt_uxor(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b)
{
	int i;
	if (a->size < b->size) {
		txBigInt *t = b;
		b = a;
		a = t;
	}
	if (r == NULL)
		r = fxBigInt_alloc(the, a->size);
	else
		r->size = a->size;
	for (i = 0; i < b->size; i++)
		r->data[i] = a->data[i] ^ b->data[i];
	for (; i < a->size; i++)
		r->data[i] = a->data[i];
	return(r);
}

txBigInt *fxBigInt_lsl(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b)
{
	if (b->sign) {
		if (a->sign) {
			r = fxBigInt_ulsr1(the, r, a, b->data[0]);
            if (b->data[0])
                r = fxBigInt_uadd(the, C_NULL, r, (txBigInt *)&gxBigIntOne);
			r->sign = 1;
		}
		else
			r = fxBigInt_ulsr1(the, r, a, b->data[0]);
	}
	else {
		r = fxBigInt_ulsl1(the, r, a, b->data[0]);
		r->sign = a->sign;
	}
	return fxBigInt_fit(the, r);
}

txBigInt *fxBigInt_ulsl1(txMachine* the, txBigInt *r, txBigInt *a, txU4 sw)
{
	txU4 wsz, bsz;
	int n;

	wsz = sw / mxBigIntWordSize;
	bsz = sw % mxBigIntWordSize;
	n = a->size + wsz + ((bsz == 0) ? 0 : 1);
	if (r == NULL) 
		r = fxBigInt_alloc(the, n);
	if (bsz == 0) {
		c_memmove(&r->data[wsz], a->data, a->size * sizeof(txU4));
		c_memset(r->data, 0, wsz * sizeof(txU4));
		r->size = a->size + wsz;
	}
	else {
		r->data[a->size + wsz] = a->data[a->size - 1] >> (mxBigIntWordSize - bsz);
		for (n = a->size; --n >= 1;)
			r->data[n + wsz] = (a->data[n] << bsz) | (a->data[n - 1] >> (mxBigIntWordSize - bsz));
		r->data[wsz] = a->data[0] << bsz;
		/* clear the remaining part */
		for (n = wsz; --n >= 0;)
			r->data[n] = 0;
		/* adjust r */
		r->size = a->size + wsz + (r->data[a->size + wsz] == 0 ? 0: 1);
	}
	return(r);
}

txBigInt *fxBigInt_lsr(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b)
{
	if (b->sign) {
		r = fxBigInt_ulsl1(the, r, a, b->data[0]);
		r->sign = a->sign;
	}
	else {
		if (a->sign) {
			r = fxBigInt_ulsr1(the, r, a, b->data[0]);
            if (b->data[0])
                r = fxBigInt_uadd(the, C_NULL, r, (txBigInt *)&gxBigIntOne);
			r->sign = 1;
		}
		else
			r = fxBigInt_ulsr1(the, r, a, b->data[0]);
	}
	return fxBigInt_fit(the, r);
}

txBigInt *fxBigInt_ulsr1(txMachine* the, txBigInt *r, txBigInt *a, txU4 sw)
{
	int wsz, bsz;
	int i, n;

	wsz = sw / mxBigIntWordSize;
	bsz = sw % mxBigIntWordSize;
	n = a->size - wsz;
	if (n <= 0) {
		if (r == NULL) r = fxBigInt_alloc(the, 1);
		r->size = 1;
		r->data[0] = 0;
		return(r);
	}
	/* 'r' can be the same as 'a' */
	if (r == NULL)
		r = fxBigInt_alloc(the, n);
	if (bsz == 0) {
		c_memmove(r->data, &a->data[wsz], n * sizeof(txU4));
		r->size = n;
	}
	else {
		for (i = 0; i < n - 1; i++)
			r->data[i] = (a->data[i + wsz] >> bsz) | (a->data[i + wsz + 1] << (mxBigIntWordSize - bsz));
		r->data[i] = a->data[i + wsz] >> bsz;
		r->size = (n > 1 && r->data[n - 1] == 0) ? n - 1: n;
	}
	return(r);
}

txBigInt *fxBigInt_nop(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b)
{
#ifdef mxRun
	mxTypeError("no such operation");
#endif
	return C_NULL;
}

/*
 * arithmetic
 */

txBigInt *fxBigInt_add(txMachine* the, txBigInt *rr, txBigInt *aa, txBigInt *bb)
{
	if ((aa->sign ^ bb->sign) == 0) {
		rr = fxBigInt_uadd(the, rr, aa, bb);
		if (aa->sign)
			rr->sign = 1;
	}
	else {
		if (!aa->sign)
			rr = fxBigInt_usub(the, rr, aa, bb);
		else
			rr = fxBigInt_usub(the, rr, bb, aa);
	}
	mxBigInt_meter(rr->size);
	return(rr);
}

txBigInt *fxBigInt_dec(txMachine* the, txBigInt *r, txBigInt *a)
{
	return fxBigInt_sub(the, r, a, (txBigInt *)&gxBigIntOne);
}

txBigInt *fxBigInt_inc(txMachine* the, txBigInt *r, txBigInt *a)
{
	return fxBigInt_add(the, r, a, (txBigInt *)&gxBigIntOne);
}

txBigInt *fxBigInt_neg(txMachine* the, txBigInt *r, txBigInt *a)
{
	if (r == NULL)
		r = fxBigInt_alloc(the, a->size);
	fxBigInt_copy(r, a);
	if (!fxBigInt_iszero(a))
		r->sign = !a->sign;
	return(r);
}

txBigInt *fxBigInt_sub(txMachine* the, txBigInt *rr, txBigInt *aa, txBigInt *bb)
{
	if ((aa->sign ^ bb->sign) == 0) {
		if (!aa->sign)
			rr = fxBigInt_usub(the, rr, aa, bb);
		else
			rr = fxBigInt_usub(the, rr, bb, aa);
	}
	else {
		txU1 sign = aa->sign;	/* could be replaced if rr=aa */
		rr = fxBigInt_uadd(the, rr, aa, bb);
		rr->sign = sign;
	}
	mxBigInt_meter(rr->size);
	return(rr);
}

#if __has_builtin(__builtin_add_overflow)
static int fxBigInt_uadd_prim(txU4 *rp, txU4 *ap, txU4 *bp, int an, int bn)
{
	txU4 c = 0;
	int i;

	for (i = 0; i < an; i++) {
#ifdef __ets__
	txU4 r;
	if (__builtin_uadd_overflow(ap[i], bp[i], &r)) {
		rp[i] = r + c;
		c = 1;
	}
	else
		c = __builtin_uadd_overflow(r, c, &rp[i]);
#else
		c = __builtin_uadd_overflow(ap[i], bp[i], &rp[i]) | __builtin_uadd_overflow(rp[i], c, &rp[i]);
#endif
	}
	for (; c && (i < bn); i++) {
		c = __builtin_uadd_overflow(1, bp[i], &rp[i]);
	}
	for (; i < bn; i++) {
		rp[i] = bp[i];
	}
	return(c);
}
#else
static int fxBigInt_uadd_prim(txU4 *rp, txU4 *ap, txU4 *bp, int an, int bn)
{
	txU4 a, b, t, r, c = 0;
	int i;

	for (i = 0; i < an; i++) {
		a = ap[i];
		b = bp[i];
		t = a + b;
		r = t + c;
		rp[i] = r;
		c = t < a || r < t;
	}
	for (; i < bn; i++) {
		r = bp[i] + c;
		rp[i] = r;
		c = r < c;
	}
	return(c);
}
#endif

txBigInt *fxBigInt_uadd(txMachine* the, txBigInt *rr, txBigInt *aa, txBigInt *bb)
{
	txBigInt *x, *y;
	int c;

	if (aa->size < bb->size) {
		x = aa;
		y = bb;
	}
	else {
		x = bb;
		y = aa;
	}
	if (rr == NULL)
		rr = fxBigInt_alloc(the, y->size + 1);
	c = fxBigInt_uadd_prim(rr->data, x->data, y->data, x->size, y->size);
	/* CAUTION: rr may equals aa or bb. do not touch until here */
	rr->size = y->size;
	rr->sign = 0;
	if (c != 0)
		rr->data[rr->size++] = 1;
	return(rr);
}

txBigInt *fxBigInt_usub(txMachine* the, txBigInt *rr, txBigInt *aa, txBigInt *bb)
{
	int i, n;
	txU4 a, b, r, t;
	txU4 *ap, *bp, *rp;
	txU4 c = 0;

	if (rr == NULL)
		rr = fxBigInt_alloc(the, MAX(aa->size, bb->size));
	rr->sign = (aa->size < bb->size ||
		    (aa->size == bb->size && fxBigInt_ucomp(aa, bb) < 0));
	if (rr->sign) {
		txBigInt *tt = aa;
		aa = bb;
		bb = tt;
	}
	ap = aa->data;
	bp = bb->data;
	rp = rr->data;
	n = MIN(aa->size, bb->size);
	for (i = 0; i < n; i++) {
		a = ap[i];
		b = bp[i];
		t = a - b;
		r = t - c;
		rp[i] = r;
		c = a < b || r > t;
	}
	if (aa->size >= bb->size) {
		n = aa->size;
		for (; i < n; i++) {
			t = ap[i];
			r = t - c;
			rp[i] = r;
			c = r > t;
		}
	}
	else {
		n = bb->size;
		for (; i < n; i++) {
			t = 0 - bp[i];
			r = t - c;
			rp[i] = r;
			c = r > t;
		}
	}
	/* remove leading 0s */
	while (--i > 0 && rp[i] == 0)
		;
	rr->size = i + 1;
	return(rr);
}

txBigInt *fxBigInt_mul(txMachine* the, txBigInt *rr, txBigInt *aa, txBigInt *bb)
{
	rr = fxBigInt_umul(the, rr, aa, bb);
	if ((aa->sign != bb->sign) && !fxBigInt_iszero(rr))
		rr->sign = 1;
	mxBigInt_meter(rr->size);
	return(rr);
}

txBigInt *fxBigInt_umul(txMachine* the, txBigInt *rr, txBigInt *aa, txBigInt *bb)
{
	txU8 a, b, r;
	txU4 *ap, *bp, *rp;
	txU4 c = 0;
	int i, j, n, m;

	if (rr == NULL)
		rr = fxBigInt_alloc(the, aa->size + bb->size);
	fxBigInt_fill0(rr);
	ap = aa->data;
	bp = bb->data;
	rp = rr->data;
	n = bb->size;
	for (i = 0, j = 0; i < n; i++) {
		b = (txU8)bp[i];
		c = 0;
		m = aa->size;
		for (j = 0; j < m; j++) {
			a = (txU8)ap[j];
			r = a * b + c;
			r += (txU8)rp[i + j];
			rp[i + j] = mxBigIntLowWord(r);
			c = mxBigIntHighWord(r);
		}
		rp[i + j] = c;
	}
	/* remove leading 0s */
	for (n = i + j; --n > 0 && rp[n] == 0;)
		;
	rr->size = n + 1;
	rr->sign = 0;
	return(rr);
}

txBigInt *fxBigInt_umul1(txMachine* the, txBigInt *r, txBigInt *a, txU4 b)
{
	int i, n;
	txU4 c = 0;
	txU4 *ap, *rp;
	txU8 wa, wr;

	if (r == NULL)
		r = fxBigInt_alloc(the, a->size + 1);
	ap = a->data;
	rp = r->data;
	n = a->size;
	for (i = 0; i < n; i++) {
		wa = (txU8)ap[i];
		wr = wa * b + c;
		c = mxBigIntHighWord(wr);
		rp[i] = mxBigIntLowWord(wr);
	}
	if (c != 0)
		rp[i] = c;
	else {
		/* remove leading 0s */
		while (--i > 0 && rp[i] == 0)
			;
	}
	r->size = i + 1;
	return(r);
}

txBigInt *fxBigInt_exp(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b)
{
#ifdef mxRun
	if (b->sign)
		mxRangeError("negative exponent");
#endif
	if (fxBigInt_iszero(b)) {
		if (r == NULL)
			r = fxBigInt_alloc(the, 1);
		else {
			r->size = 1;
			r->sign = 0;
		}
		r->data[0] = 1;
	}
	else {
		int i = fxBigInt_bitsize(b) - 1;
		int odd = fxBigInt_isset(b, i);
		txU4 c = fxBigInt_bitsize(a);
		txBigInt *t = fxBigInt_umul1(the, NULL, b, c);
		t = fxBigInt_ulsr1(the, t, t, 5);
#ifdef mxRun
		if ((t->size > 1) || (t->data[0] > 0xFFFF))
			mxRangeError("too big exponent");
#endif
		c = 2 + t->data[0];
		fxBigInt_free(the, t);
        if (r == NULL)
			r = fxBigInt_alloc(the, c);
   		t = fxBigInt_alloc(the, c);
      	fxBigInt_copy(r, a);
		while (i > 0) {
            i--;
			t = fxBigInt_sqr(the, t, r);
			if ((odd = fxBigInt_isset(b, i))) {
				r->size = c;
				r = fxBigInt_umul(the, r, t, a);
			}
			else {
				txBigInt u = *r;
				*r = *t;
				*t = u;
			}
			t->size = c;
		}
		r->sign = a->sign & odd;
		fxBigInt_free(the, t);
	}
	mxBigInt_meter(r->size);
	return(r);
}

txBigInt *fxBigInt_sqr(txMachine* the, txBigInt *r, txBigInt *a)
{
	int i, j, t;
	txU4 *ap, *rp;
	txU8 uv, t1, t2, t3, ai;
	txU4 c, cc;
	txU4 overflow = 0;	/* overflow flag of 'u' */

	if (r == NULL)
		r = fxBigInt_alloc(the, a->size * 2);
	fxBigInt_fill0(r);
	t = a->size;
	ap = a->data;
	rp = r->data;

	for (i = 0; i < t - 1; i++) {
		uv = (txU8)ap[i] * ap[i] + rp[i * 2];
		rp[i * 2] = mxBigIntLowWord(uv);
		c = mxBigIntHighWord(uv);
		cc = 0;
		ai = ap[i];
		for (j = i + 1; j < t; j++) {
			int k = i + j;
			t1 = ai * ap[j];
			t2 = t1 + c + ((txU8)cc << mxBigIntWordSize);	/* 'cc:c' must be <= 2(b-1) so no overflow here */
			t3 = t1 + t2;
			uv = t3 + rp[k];
			cc = t3 < t1 || uv < t3;
			c = (txU4)mxBigIntHighWord(uv);
			rp[k] = mxBigIntLowWord(uv);
		}
		c += overflow;
		rp[i + t] = c;		/* c = u */
		overflow = cc || c < overflow;
	}
	/* the last loop */
	uv = (txU8)ap[i] * ap[i] + rp[i * 2];
	rp[i * 2] = mxBigIntLowWord(uv);
	rp[i + t] = mxBigIntHighWord(uv) + overflow;

	/* remove leading 0s */
	for (i = 2*t; --i > 0 && rp[i] == 0;)
		;
	r->size = i + 1;
	return(r);
}

txBigInt *fxBigInt_div(txMachine* the, txBigInt *q, txBigInt *a, txBigInt *b)
{
#ifdef mxRun
	if (fxBigInt_iszero(b))
		mxRangeError("zero divider");
#endif
	q = fxBigInt_udiv(the, q, a, b, C_NULL);
	if (!fxBigInt_iszero(q)) {
		if (a->sign)
			q->sign = !q->sign;
		if (b->sign)
			q->sign = !q->sign;
	}
	mxBigInt_meter(q->size);
	return(q);
}

txBigInt *fxBigInt_mod(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b)
{
	txBigInt *q;
	mxBigInt_meter(((a->size - b->size) * (a->size + b->size)));
	if (r == NULL)
		r = fxBigInt_alloc(the, a->sign ? b->size : MIN(a->size, b->size));
	q = fxBigInt_udiv(the, NULL, a, b, &r);
	if (a->sign) {
		if (!fxBigInt_iszero(r))
			r = fxBigInt_sub(the, r, b, r);
	}
	fxBigInt_free(the, q);
	mxBigInt_meter(r->size);
	return(r);
}

txBigInt *fxBigInt_rem(txMachine* the, txBigInt *r, txBigInt *a, txBigInt *b)
{
	txBigInt *q;
#ifdef mxRun
	if (fxBigInt_iszero(b))
		mxRangeError("zero divider");
#endif
	mxBigInt_meter(((a->size - b->size) * (a->size + b->size)));
	if (r == NULL)
		r = fxBigInt_alloc(the, MIN(a->size, b->size));
	q = fxBigInt_udiv(the, NULL, a, b, &r);
	if (a->sign) {
		if (!fxBigInt_iszero(r))
            r->sign = !r->sign;
	}
	fxBigInt_free(the, q);
	mxBigInt_meter(r->size);
	return(r);
}

static void fxBigInt_makepoly(txBigInt *r, txBigInt *a, int t)
{
	int n;
	txU4 *rp, *ap;

	/* make up a polynomial a_t*b^n + a_{t-1}*b^{n-1} + ... */
	n = r->size;
	rp = &r->data[n];
	ap = a->data;
	while (--n >= 0) {
		*--rp = t < 0 ? 0: ap[t];
		--t;
	}
}

#if BN_NO_ULDIVMOD
static txU8
div64_32(txU8 a, txU4 b)
{
	txU4 high = (txU4)(a >> 32);
	txU8 r = 0, bb = b, d = 1;

	if (high >= b) {
		high /= b;
		r = (txU8)high << 32;
		a -= (txU8)(high * b) << 32;
	}
	while ((long long)bb > 0 && bb < a) {
		bb += bb;
		d += d;
	}
	do {
		if (a >= bb) {
			a -= bb;
			r += d;
		}
		bb >>= 1;
		d >>= 1;
	} while (d != 0);
	return r;
}
#endif

txBigInt *fxBigInt_udiv(txMachine* the, txBigInt *q, txBigInt *a, txBigInt *b, txBigInt **r)
{
	int sw;
	txBigInt *nb, *na, *tb, *a2, *b2, *tb2, *tb3;
	int i, n, t;
	txU4 *qp, *ap, *bp;
#define mk_dword(p, i)	(((txU8)(p)[i] << mxBigIntWordSize) | (p)[i - 1])

	if (fxBigInt_ucomp(a, b) < 0) {
		if (q == NULL) {
			q = fxBigInt_alloc(the, 1);
		}
		else {
			q->sign = 0;
			q->size = 1;
		}
		q->data[0] = 0;
		if (r != NULL) {
			if (*r == NULL)
				*r = fxBigInt_dup(the, a);
			else
				fxBigInt_copy(*r, a);
			(*r)->sign = 0;
		}
		return(q);
	}

	/* CAUTION: if q is present, it must take account of normalization */
	if (q == NULL)
		q = fxBigInt_alloc(the, a->size - b->size + 2);
	if (r != NULL && *r == NULL)
		*r = fxBigInt_alloc(the, b->size);

	/* normalize */
	sw = fxBigInt_ffs(b);
	nb = fxBigInt_ulsl1(the, NULL, b, sw);
	na = fxBigInt_ulsl1(the, NULL, a, sw);
	t = nb->size - 1;	/* the size must not change from 'b' */
	n = na->size - 1;

	/* adjust size of q */
	q->size = na->size - nb->size + 1;
	fxBigInt_fill0(q);	/* set 0 to quotient */

	/* process the most significant word */
	tb = fxBigInt_ulsl1(the, NULL, nb, (n - t) * mxBigIntWordSize);	/* y*b^n */
	if (fxBigInt_ucomp(na, tb) >= 0) {
		q->data[q->size - 1]++;
		fxBigInt_sub(C_NULL, na, na, tb);
		/* since nomalization done, must be na < tb here */
	}

	/* prepare the constant for the adjustment: y_t*b + y_{t-1} */
	b2 = fxBigInt_alloc(the, 2);
	fxBigInt_makepoly(b2, nb, t);
	/* and allocate for temporary buffer */
	a2 = fxBigInt_alloc(the, 3);
	tb2 = fxBigInt_alloc(the, 3);
	tb3 = fxBigInt_alloc(the, tb->size + 1);

	qp = &q->data[q->size - 1];
	ap = na->data;
	bp = nb->data;
	for (i = n; i >= t + 1; --i) {
		txU4 tq;

		/* the first estimate */
#if BN_NO_ULDIVMOD
		tq = (ap[i] == bp[t]) ? ~(txU4)0: (txU4)div64_32(mk_dword(ap, i), bp[t]);
#else
		tq = (ap[i] == bp[t]) ? ~(txU4)0: (txU4)(mk_dword(ap, i) / bp[t]);
#endif

		/* adjust */
		fxBigInt_makepoly(a2, na, i);
		while (fxBigInt_ucomp(fxBigInt_umul1(the, tb2, b2, tq), a2) > 0)
			--tq;

		/* next x */
		fxBigInt_ulsr1(the, tb, tb, mxBigIntWordSize);
		fxBigInt_usub(the, na, na, fxBigInt_umul1(the, tb3, tb, tq));
		if (na->sign) {
			fxBigInt_add(the, na, na, tb);
			--tq;
		}
		*--qp = tq;
	}
	if (r != NULL)
		*r = fxBigInt_ulsr1(the, *r, na, sw);
	/* remove leading 0s from q */
	for (i = q->size; --i > 0 && q->data[i] == 0;)
		;
	q->size = i + 1;
	
	fxBigInt_free(the, tb3);
	fxBigInt_free(the, tb2);
	fxBigInt_free(the, a2);
	fxBigInt_free(the, b2);
	fxBigInt_free(the, tb);
	fxBigInt_free(the, na);
	fxBigInt_free(the, nb);
	return(q);
}

#ifdef mxMetering
void fxBigInt_meter(txMachine* the, int n)
{
	the->meterIndex += n - 1;
}
#endif


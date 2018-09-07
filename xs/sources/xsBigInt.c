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


#ifdef mxRun
	#include "xsAll.h"
	#define bn_context sxMachine
#else
	#include "xsCommon.h"
#endif

typedef unsigned long long bn_dword;
typedef unsigned int bn_word;
typedef unsigned char bn_bool;
typedef unsigned short bn_size;
typedef unsigned char bn_byte;

typedef txBigInt bn_t;

typedef enum {
	BN_ERR_NONE,
	BN_ERR_NOMEM,
	BN_ERR_DIVIDE_BY_ZERO,
	BN_ERR_MISCALCULATION,
} bn_err_t;

typedef struct bn_context bn_context_t;

static int bn_iszero(bn_t *a);
static int bn_isNaN(bn_t *a);
static bn_bool bn_sign(bn_t *a);
static bn_size bn_wsizeof(bn_t *a);
static void bn_negate(bn_t *a);
static void bn_copy(bn_t *a, bn_t *b);
static bn_t *bn_dup(bn_context_t *ctx, bn_t *a);
static int bn_bitsize(bn_t *e);
static int bn_comp(bn_t *a, bn_t *b);

static bn_t *bn_lsl(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw);
static bn_t *bn_ulsl(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw);
static bn_t *bn_lsr(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw);
static bn_t *bn_ulsr(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw);
static bn_t *bn_xor(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b);
static bn_t *bn_or(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b);
static bn_t *bn_and(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b);

static bn_t *bn_add(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb);
static bn_t *bn_sub(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb);
static bn_t *bn_mul(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb);
static bn_t *bn_umul1(bn_context_t *ctx, bn_t *r, bn_t *a, bn_word b);
static bn_t *bn_div(bn_context_t *ctx, bn_t *q, bn_t *a, bn_t *b, bn_t **r);
static bn_t *bn_mod(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b);
static bn_t *bn_rem(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b);

void fxBigIntEncode(txByte* code, txBigInt* bigint, txSize size)
{
#if mxBigEndian
	#error
#else
	c_memcpy(code, bigint->data, size);
#endif
}

txSize fxBigIntMaximum(txInteger length)
{
	return sizeof(txU4) * (1 + (((txSize)c_ceil((txNumber)length * c_log(10) / c_log(2))) / 32));
}

txSize fxBigIntMaximumB(txInteger length)
{
	return sizeof(txU4) * (1 + (length / 32));
}

txSize fxBigIntMaximumO(txInteger length)
{
	return sizeof(txU4) * (1 + ((length * 3) / 32));
}

txSize fxBigIntMaximumX(txInteger length)
{
	return sizeof(txU4) * (1 + ((length * 4) / 32));
}

txSize fxBigIntMeasure(txBigInt* bigint)
{
	return bigint->size * sizeof(txU4);
}


void printbn(bn_t* result)
{
	int i = result->size - 1;	
	if (result->sign)
		fprintf(stderr, "-");
	fprintf(stderr, "0x");
	while (i >= 0) {
		fprintf(stderr, "%8.8X", result->data[i]);
		i--;
	}
	fprintf(stderr, "n\n");
}

void fxBigIntParse(txBigInt* bigint, txString p, txInteger length, txInteger sign)
{
	txU4 data[1] = { 0 };
	bn_t digit = { .sign=0, .size=1, .data=data };
	bigint->sign = 0;
	bigint->size = 1;
	bigint->data[0] = 0;
	while (length) {
		char c = *p++;
		digit.data[0] = c - '0';
		bn_add(NULL, bigint, bn_umul1(NULL, bigint, bigint, 10), &digit);
		length--;
	}
	bigint->sign = sign;
}

void fxBigIntParseB(txBigInt* bigint, txString p, txInteger length)
{
	txU4 data[1] = { 0 };
	bn_t digit = { .sign=0, .size=1, .data=data };
	bigint->sign = 0;
	bigint->size = 1;
	bigint->data[0] = 0;
	while (length) {
		char c = *p++;
		digit.data[0] = c - '0';
		bn_add(NULL, bigint, bn_ulsl(NULL, bigint, bigint, 1), &digit);
		length--;
	}
}

void fxBigIntParseO(txBigInt* bigint, txString p, txInteger length)
{
	txU4 data[1] = { 0 };
	bn_t digit = { .sign=0, .size=1, .data=data };
	bigint->sign = 0;
	bigint->size = 1;
	bigint->data[0] = 0;
	while (length) {
		char c = *p++;
		digit.data[0] = c - '0';
		bn_add(NULL, bigint, bn_ulsl(NULL, bigint, bigint, 3), &digit);
		length--;
	}
}

void fxBigIntParseX(txBigInt* bigint, txString p, txInteger length)
{
	txU4 data[1] = { 0 };
	bn_t digit = { .sign=0, .size=1, .data=data };
	bigint->sign = 0;
	bigint->size = 1;
	bigint->data[0] = 0;
	while (length) {
		char c = *p++;
		if (('0' <= c) && (c <= '9'))
			digit.data[0] = c - '0';
		else if (('a' <= c) && (c <= 'f'))
			digit.data[0] = 10 + c - 'a';
		else
			digit.data[0] = 10 + c - 'A';
		bn_add(NULL, bigint, bn_ulsl(NULL, bigint, bigint, 4), &digit);
		length--;
	}
}

#ifdef mxRun

void bn_freebuf(txMachine *the, void *bufptr)
{
}

void bn_throw(txMachine *the, bn_err_t code)
{
	switch(code) {
	case BN_ERR_NOMEM:
		break;
	case BN_ERR_DIVIDE_BY_ZERO:
		mxRangeError("bigint divide by zero");
		break;
	case BN_ERR_MISCALCULATION:
		mxUnknownError("bigint miscalculation");
		break;
	default:
		mxUnknownError("bigint unknwon");
		break;
	}
}

static txSlot* fxCheckBigInt(txMachine* the, txSlot* it);
static txBigInt* fxIntegerToBigInt(txMachine* the, txSlot* slot);
static txBigInt* fxNumberToBigInt(txMachine* the, txSlot* slot);
static txBigInt* fxStringToBigInt(txMachine* the, txSlot* slot, txFlag whole);
static txBigInt* fxToBigInt(txMachine* the, txSlot* theSlot);

const txU4 gxDataOne[1] = { 1 };
const txU4 gxDataZero[1] = { 0 };
const bn_t gxBigIntNaN = { .sign=0, .size=0, .data=(txU4*)gxDataZero };
const bn_t gxBigIntOne = { .sign=0, .size=1, .data=(txU4*)gxDataOne };
const bn_t gxBigIntZero = { .sign=0, .size=1, .data=(txU4*)gxDataZero };

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
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, mxCallback(fx_BigInt), 1, mxID(_BigInt), XS_DONT_ENUM_FLAG));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_asIntN), 2, mxID(_asIntN), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_asUintN), 2, mxID(_asUintN), XS_DONT_ENUM_FLAG);
	the->stack++;
}

txSlot* fxNewBigIntInstance(txMachine* the, txSlot* slot)
{
	txSlot* instance;
	txSlot* internal;
	instance = fxNewObjectInstance(the);
	internal = instance->next = fxNewSlot(the);
	internal->flag = XS_INTERNAL_FLAG | XS_GET_ONLY;
	internal->kind = slot->kind;
	internal->value = slot->value;
	return instance;
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
		fxToBigInt(the, mxResult);
}

txNumber fxToIndex(txMachine* the, txSlot* slot)
{
	txNumber value, index;
	if (slot->kind == XS_UNDEFINED_KIND)
		return 0;
	value = fxToNumber(the, slot);
	if (c_isnan(value))
		return 0;
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
	return index;
}

void fx_BigInt_asIntN(txMachine* the)
{
	txInteger index = (mxArgc < 1) ? 0 : (txInteger)fxToIndex(the, mxArgv(0));
	txBigInt* bits;
	txBigInt* result;
	if (mxArgc < 2)
		mxTypeError("no bigint");
	bits = bn_ulsl(the, C_NULL, (bn_t *)&gxBigIntOne, index);
	result = bn_mod(the, C_NULL, fxToBigInt(the, mxArgv(1)), bits);
	if (index && bn_comp(result, bn_ulsl(the, C_NULL, (bn_t *)&gxBigIntOne, index - 1)) >= 0)
		result = bn_sub(the, C_NULL, result, bits);
	mxResult->value.bigint = *result;
	mxResult->kind = XS_BIGINT_KIND;
}

void fx_BigInt_asUintN(txMachine* the)
{
	txInteger index = (mxArgc < 1) ? 0 : (txInteger)fxToIndex(the, mxArgv(0));
	if (mxArgc < 2)
		mxTypeError("no bigint");
	fxToBigInt(the, mxArgv(1));
	mxResult->value.bigint = *bn_mod(the, C_NULL, &mxArgv(1)->value.bigint, bn_ulsl(the, C_NULL, (bn_t *)&gxBigIntOne, index));
	mxResult->kind = XS_BIGINT_KIND;
}

void fx_BigInt_prototype_toString(txMachine* the)
{
	static const char gxDigits[] ICACHE_FLASH_ATTR = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	txSlot* slot;
	txU4 data[1] = { 0 };
	txBigInt radix = { .sign=0, .size=1, .data=data };
	txSize length, offset;
	txBoolean minus = 0;
	txSlot* stack;
	
	slot = fxCheckBigInt(the, mxThis);
	if (!slot) mxTypeError("this is no bigint");
	if (mxArgc == 0)
		data[0] = 10;
	else if (mxIsUndefined(mxArgv(0)))
		data[0] = 10;
	else {
		data[0] = fxToInteger(the, mxArgv(0));
		if ((data[0] < 2) || (36 < data[0]))
			mxRangeError("invalid radix");
	}
	
	length = 1 + (txSize)c_ceil((txNumber)slot->value.bigint.size * 32 * c_log(2) / c_log(data[0]));
	if (slot->value.bigint.sign) {
		slot->value.bigint.sign = 0;
		length++;
		minus = 1;
	}
	offset = length;
	mxResult->value.string = fxNewChunk(the, length);
	mxResult->kind = XS_STRING_KIND;

	stack = the->stack;
	mxResult->value.string[--offset] = 0;
	do {
		txBigInt* remainder = NULL;
		slot->value.bigint = *bn_div(the, C_NULL, &slot->value.bigint, &radix, &remainder);
		mxResult->value.string[--offset] = c_read8(gxDigits + remainder->data[0]);
		the->stack = stack;
	}
	while (!bn_iszero(&slot->value.bigint));
	if (minus)
		mxResult->value.string[--offset] = '-';
		
	c_memmove(mxResult->value.string, mxResult->value.string + offset, length - offset);
}

void fx_BigInt_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxCheckBigInt(the, mxThis);
	if (!slot) mxTypeError("this is no bigint");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

void fxBigIntBinary(txMachine* the, txU1 code, txSlot* left, txSlot* right)
{
	if ((left->kind != XS_BIGINT_KIND) && (left->kind != XS_BIGINT_X_KIND))
		mxTypeError("Cannot coerce left operand to bigint");
	if ((right->kind != XS_BIGINT_KIND) && (right->kind != XS_BIGINT_X_KIND))
		mxTypeError("Cannot coerce right operand to bigint");
	switch (code) {
	case XS_CODE_BIT_AND:
		left->value.bigint = *bn_and(the, C_NULL, &left->value.bigint, &right->value.bigint);
		break;
	case XS_CODE_BIT_OR:
		left->value.bigint = *bn_or(the, C_NULL, &left->value.bigint, &right->value.bigint);
		break;
	case XS_CODE_BIT_XOR:
		left->value.bigint = *bn_xor(the, C_NULL, &left->value.bigint, &right->value.bigint);
		break;
	case XS_CODE_LEFT_SHIFT:
		left->value.bigint = *bn_lsl(the, C_NULL, &left->value.bigint, right->value.bigint.data[0]); //@@
		break;
	case XS_CODE_SIGNED_RIGHT_SHIFT:
		left->value.bigint = *bn_lsr(the, C_NULL, &left->value.bigint, right->value.bigint.data[0]); //@@
		break;
	case XS_CODE_UNSIGNED_RIGHT_SHIFT:
		mxTypeError("bigint >>> bigint");
		break;
	case XS_CODE_ADD:
		left->value.bigint = *bn_add(the, C_NULL, &left->value.bigint, &right->value.bigint);
		break;
	case XS_CODE_SUBTRACT:
		left->value.bigint = *bn_sub(the, C_NULL, &left->value.bigint, &right->value.bigint);
		break;
	case XS_CODE_EXPONENTIATION:
		mxTypeError("bigint ** bigint");
		break;
	case XS_CODE_MULTIPLY:
		left->value.bigint = *bn_mul(the, C_NULL, &left->value.bigint, &right->value.bigint);
		break;
	case XS_CODE_DIVIDE:
		left->value.bigint = *bn_div(the, C_NULL, &left->value.bigint, &right->value.bigint, C_NULL); //@@
		break;
	case XS_CODE_MODULO:
		left->value.bigint = *bn_rem(the, C_NULL, &left->value.bigint, &right->value.bigint);//@@
		break;
	}
	left->kind = XS_BIGINT_KIND;
	the->stack = right;
}

txBoolean fxBigIntCompare(txMachine* the, txU1 code, txSlot* left, txSlot* right)
{
	int result;
	if (bn_isNaN(&left->value.bigint))
		return 0;
	if (right->kind == XS_STRING_KIND)
		fxStringToBigInt(the, right, 0);
	if ((right->kind != XS_BIGINT_KIND) && (right->kind != XS_BIGINT_X_KIND)) {
		fxToNumber(the, right);
		result = c_fpclassify(right->value.number);
		if (result == FP_NAN)
			return 0;
		if (result == C_FP_INFINITE) {
			if ((code == XS_CODE_LESS) || (code == XS_CODE_LESS_EQUAL))
				return right->value.number > 0;
			if ((code == XS_CODE_MORE) || (code == XS_CODE_MORE_EQUAL))
				return right->value.number < 0;
			return 0;
		}
		fxNumberToBigInt(the, right);
	}
	if (bn_isNaN(&right->value.bigint))
		return 0;
	result = bn_comp(&left->value.bigint, &right->value.bigint);
	if (result < 0)
		return ((code == XS_CODE_LESS) || (code == XS_CODE_LESS_EQUAL) || (code == XS_CODE_NOT_EQUAL));
	if (result > 0)
		return ((code == XS_CODE_MORE) || (code == XS_CODE_MORE_EQUAL) || (code == XS_CODE_NOT_EQUAL));
	return ((code == XS_CODE_EQUAL) || (code == XS_CODE_LESS_EQUAL) || (code == XS_CODE_MORE_EQUAL));
}

void fxBigIntDecode(txMachine* the, txSize size)
{
	txBigInt* bigint;
	mxPushUndefined();
	bigint = &the->stack->value.bigint;
	bigint->data = fxNewChunk(the, size);
#if mxBigEndian
	#error
#else
	c_memcpy(bigint->data, the->code, size);
#endif	
	bigint->size = size >> 2;
	bigint->sign = 0;
	the->stack->kind = XS_BIGINT_KIND;
}

void fxBigIntUnary(txMachine* the, txU1 code, txSlot* left)
{
	switch (code) {
	case XS_CODE_BIT_NOT:
		//@@
		break;
	case XS_CODE_MINUS:
		left->value.bigint = *bn_dup(the, &left->value.bigint);
		bn_negate(&left->value.bigint);
		break;
	case XS_CODE_DECREMENT:
		left->value.bigint = *bn_sub(the, C_NULL, &left->value.bigint, (bn_t *)&gxBigIntOne);
		break;
	case XS_CODE_INCREMENT:
		left->value.bigint = *bn_add(the, C_NULL, &left->value.bigint, (bn_t *)&gxBigIntOne);
		break;
	}
	left->kind = XS_BIGINT_KIND;
	the->stack = left;
}

txBoolean fxToNumericInteger(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_REFERENCE_KIND)
		fxToPrimitive(the, theSlot, XS_NUMBER_HINT);
	if ((theSlot->kind == XS_BIGINT_KIND) || (theSlot->kind == XS_BIGINT_X_KIND))
		return 0;
	fxToInteger(the, theSlot);
	return 1;
}

txBoolean fxToNumericNumber(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_REFERENCE_KIND)
		fxToPrimitive(the, theSlot, XS_NUMBER_HINT);
	if ((theSlot->kind == XS_BIGINT_KIND) || (theSlot->kind == XS_BIGINT_X_KIND))
		return 0;
	fxToNumber(the, theSlot);
	return 1;
}

txSlot* fxCheckBigInt(txMachine* the, txSlot* it)
{
	txSlot* result = C_NULL;
	if (it->kind == XS_BIGINT_KIND)
		result = it;
	else if (it->kind == XS_REFERENCE_KIND) {
		txSlot* instance = it->value.reference;
		it = instance->next;
		if ((it) && (it->flag & XS_INTERNAL_FLAG) && (it->kind == XS_BIGINT_KIND))
			result = it;
	}
	return result;
}

txBigInt* fxIntegerToBigInt(txMachine* the, txSlot* slot)
{
	txInteger integer = slot->value.integer;
	txBigInt* bigint = &slot->value.bigint;
	bn_bool sign = 0;
	if (integer < 0) {
		integer = -integer;
		sign = 1;
	}
	bigint->data = fxNewChunk(the, sizeof(txU4));
	bigint->data[0] = (bn_word)integer;
	bigint->sign = sign;
	bigint->size = 1;
	slot->kind = XS_BIGINT_KIND;
	return bigint;
}

txBigInt* fxNumberToBigInt(txMachine* the, txSlot* slot)
{
	int64_t integer = (int64_t)slot->value.number;
	txBigInt* bigint = &slot->value.bigint;
	bn_bool sign = 0;
	if (integer < 0) {
		integer = -integer;
		sign = 1;
	}
	if (integer > 0x00000000FFFFFFFFll) {
		bigint->data = fxNewChunk(the, 2 * sizeof(txU4));
		bigint->data[0] = (bn_word)(integer);
		bigint->data[1] = (bn_word)(integer >> 32);
		bigint->size = 2;
		bigint->sign = sign;
	}
	else {
		bigint->data = fxNewChunk(the, sizeof(txU4));
		bigint->data[0] = (uint32_t)integer;
		bigint->size = 1;
		bigint->sign = sign;
	}
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
			offset = p - s;
			if ((d == 'B') || (d == 'b')) {
				while (((c = *p)) && ('0' <= c) && (c <= '1'))
					p++;
				length = p - s - offset;
				p = fxSkipSpaces(p);
				if ((length > 0) && (*p == 0)) {
					bigint.data = fxNewChunk(the, fxBigIntMaximumB(length));
					fxBigIntParseB(&bigint, slot->value.string + offset, length);
				}
			}
			else if ((d == 'O') || (d == 'o')) {
				while (((c = *p)) && ('0' <= c) && (c <= '7'))
					p++;
				length = p - s - offset;
				p = fxSkipSpaces(p);
				if ((length > 0) && (*p == 0)) {
					bigint.data = fxNewChunk(the, fxBigIntMaximumO(length));
					fxBigIntParseO(&bigint, slot->value.string + offset, length);
				}
			}
			else if ((d == 'X') || (d == 'x')) {
				while (((c = *p)) && ((('0' <= c) && (c <= '9')) || (('a' <= c) && (c <= 'f')) || (('A' <= c) && (c <= 'F'))))
					p++;
				length = p - s - offset;
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
	offset = p - s;
	while (((c = *p)) && ('0' <= c) && (c <= '9'))
		p++;
	length = p - s - offset;
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

txBigInt* fxToBigInt(txMachine* the, txSlot* theSlot)
{
again:
	switch (theSlot->kind) {
	case XS_BOOLEAN_KIND:
		theSlot->value.bigint = *bn_dup(the, (bn_t *)((theSlot->value.boolean) ? &gxBigIntOne : &gxBigIntZero));
		theSlot->kind = XS_BIGINT_KIND;
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		fxStringToBigInt(the, theSlot, 1);
		if (bn_isNaN(&theSlot->value.bigint))
			mxSyntaxError("Cannot coerce string to bigint");
		break;
	case XS_BIGINT_KIND:
	case XS_BIGINT_X_KIND:
		break;
	case XS_REFERENCE_KIND:
		fxToPrimitive(the, theSlot, XS_NUMBER_HINT);
		goto again;
	default:
		mxTypeError("Cannot coerce to bigint");
		break;
	}
	return &theSlot->value.bigint;
}

#endif

#define bn_high_word(x)		((bn_word)((x) >> 32))
#define bn_low_word(x)		((bn_word)(x))
#define bn_wordsize		(sizeof(bn_word) * 8)

#ifndef howmany
#define howmany(x, y)	(((x) + (y) - 1) / (y))
#endif
#ifndef MAX
#define MAX(a, b)	((a) >= (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)	((a) <= (b) ? (a) : (b))
#endif

static void
bn_memset(void *data, int c, unsigned int n)
{
	unsigned char *p = data;

	while (n-- != 0)
		*p++ = c;
}

static void
bn_memcpy(void *dst, const void *src, unsigned int n)
{
	unsigned char *p1 = dst;
	const unsigned char *p2 = src;

	while (n-- != 0)
		*p1++ = *p2++;
}

static void
bn_memmove(void *dst, const void *src, unsigned int n)
{
	unsigned char *p1;
	const unsigned char *p2;

	if (src >= dst) {
		p1 = dst;
		p2 = src;
		while (n-- != 0)
			*p1++ = *p2++;
	}
	else {
		p1 = (unsigned char *)dst + n;
		p2 = (unsigned char *)src + n;
		while (n-- != 0)
			*--p1 = *--p2;
	}
}

static bn_t *
bn_alloct(bn_context_t *ctx, unsigned int n)
{
#ifdef mxRun
	txMachine* the = ctx;
	txBigInt* bigint;
	mxPushUndefined();
	bigint = &the->stack->value.bigint;
	bigint->data = fxNewChunk(the, n * sizeof(bn_word));
	bigint->size = n;
	bigint->sign = 0;
	the->stack->kind = XS_BIGINT_KIND;
	return bigint;
#else
	return NULL;
#endif
}

static void bn_freet(bn_context_t *ctx, bn_t *bigint)
{
#ifdef mxRun
	txMachine* the = ctx;
	if (bigint == &the->stack->value.bigint)
		mxPop();
#endif
}

int
bn_iszero(bn_t *a)
{
	return(a->size == 1 && a->data[0] == 0);
}

int
bn_isNaN(bn_t *a)
{
	return(a->size == 0);
}

bn_bool
bn_sign(bn_t *a)
{
	return(a->sign);
}

bn_size
bn_wsizeof(bn_t *a)
{
	return(a->size);
}

void
bn_negate(bn_t *a)
{
	a->sign = !a->sign;
}

static void
bn_fill0(bn_t *r)
{
	bn_memset(r->data, 0, r->size * sizeof(bn_word));
}

void
bn_copy(bn_t *a, bn_t *b)
{
	bn_memcpy(a->data, b->data, b->size * sizeof(bn_word));
	a->size = b->size;
	a->sign = b->sign;
}

bn_t *
bn_dup(bn_context_t *ctx, bn_t *a)
{
	bn_t *r = bn_alloct(ctx, a->size);

	bn_copy(r, a);
	return(r);
}

static int
bn_ffs(bn_t *a)
{
	int i;
	bn_word w = a->data[a->size - 1];

	for (i = 0; i < (int)bn_wordsize && !(w & ((bn_word)1 << (bn_wordsize - 1 - i))); i++)
		;
	return(i);
}

int
bn_bitsize(bn_t *e)
{
	return(e->size * bn_wordsize - bn_ffs(e));
}

static int
bn_isset(bn_t *e, unsigned int i)
{
	bn_word w = e->data[i / bn_wordsize];
	return((w & ((bn_word)1 << (i % bn_wordsize))) != 0);
}

static int
bn_ucomp(bn_t *a, bn_t *b)
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

int
bn_comp(bn_t *a, bn_t *b)
{
	if (a->sign != b->sign)
		return(a->sign ? -1: 1);
	else if (a->sign)
		return(bn_ucomp(b, a));
	else
		return(bn_ucomp(a, b));
}

/*
 * bitwise operations
 */

bn_t *
bn_ulsl(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw)
{
	unsigned int wsz, bsz;
	int n;

	/* 'r' can be the same as 'a' */
	/* assume 'r' is large enough if 'r' is present */
	if (r == NULL)
		r = bn_alloct(ctx, a->size + howmany(sw, bn_wordsize));
	wsz = sw / bn_wordsize;
	bsz = sw % bn_wordsize;
	if (bsz == 0) {
		bn_memmove(&r->data[wsz], a->data, a->size * sizeof(bn_word));
		bn_memset(r->data, 0, wsz * sizeof(bn_word));
		r->size = a->size + wsz;
	}
	else {
		r->data[a->size + wsz] = a->data[a->size - 1] >> (bn_wordsize - bsz);
		for (n = a->size; --n >= 1;)
			r->data[n + wsz] = (a->data[n] << bsz) | (a->data[n - 1] >> (bn_wordsize - bsz));
		r->data[wsz] = a->data[0] << bsz;
		/* clear the remaining part */
		for (n = wsz; --n >= 0;)
			r->data[n] = 0;
		/* adjust r */
		r->size = a->size + wsz + (r->data[a->size + wsz] == 0 ? 0: 1);
	}
	return(r);
}

bn_t *
bn_lsl(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw)
{
	r = bn_ulsl(ctx, r, a, sw);
	r->sign = a->sign;
	return(r);
}

bn_t *
bn_ulsr(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw)
{
	int wsz, bsz;
	int i, n;

	wsz = sw / bn_wordsize;
	bsz = sw % bn_wordsize;
	n = a->size - wsz;
	if (n <= 0) {
		if (r == NULL) r = bn_alloct(ctx, 1);
		r->size = 1;
		r->data[0] = 0;
		return(r);
	}
	/* 'r' can be the same as 'a' */
	if (r == NULL)
		r = bn_alloct(ctx, n);
	if (bsz == 0) {
		bn_memmove(r->data, &a->data[wsz], n * sizeof(bn_word));
		r->size = n;
	}
	else {
		for (i = 0; i < n - 1; i++)
			r->data[i] = (a->data[i + wsz] >> bsz) | (a->data[i + wsz + 1] << (bn_wordsize - bsz));
		r->data[i] = a->data[i + wsz] >> bsz;
		r->size = (n > 1 && r->data[n - 1] == 0) ? n - 1: n;
	}
	return(r);
}

bn_t *
bn_lsr(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw)
{
	if (a->sign) {
		a->sign = 0;
		r = bn_add(ctx, r, bn_ulsr(ctx, NULL, a, sw), (bn_t *)&gxBigIntOne);
		r->sign = 1;
		return(r);
	}
	return bn_ulsr(ctx, r, a, sw);
}

// binary bitwise operators
// negative operands algorithms inspired by GMP

bn_t *
bn_uxor(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b)
{
	int i;
	if (a->size < b->size) {
		bn_t *t = b;
		b = a;
		a = t;
	}
	if (r == NULL)
		r = bn_alloct(ctx, a->size);
	for (i = 0; i < b->size; i++)
		r->data[i] = a->data[i] ^ b->data[i];
	for (; i < a->size; i++)
		r->data[i] = a->data[i];
	return(r);
}

bn_t *
bn_xor(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b)
{
	bn_t *t;
	if (a->sign) {
		if (b->sign)
			goto XOR_MINUS_MINUS;
		t = a;
		a = b;
		b = t;	
		goto XOR_PLUS_MINUS;
	}
	if (b->sign)
		goto XOR_PLUS_MINUS;
	return bn_uxor(ctx, r, a, b);
XOR_PLUS_MINUS:
    /* Operand 2 negative, so will be the result.
       -(OP1 ^ (-OP2)) = -(OP1 ^ ~(OP2 - 1)) == ~(OP1 ^ ~(OP2 - 1)) + 1 == (OP1 ^ (OP2 - 1)) + 1      */
	b->sign = 0;
	b = bn_sub(ctx, NULL, b, (bn_t *)&gxBigIntOne);
	r = bn_add(ctx, r, bn_uxor(ctx, NULL, a, b), (bn_t *)&gxBigIntOne);
	r->sign = 1;
	return(r);
XOR_MINUS_MINUS:
// (-OP1) ^ (-OP2) == ~(OP1 - 1) ^ ~(OP2 - 1) == (OP1 - 1) ^ (OP2 - 1)
	
	a->sign = 0;
	b->sign = 0;
	a = bn_sub(ctx, NULL, a, (bn_t *)&gxBigIntOne);
	b = bn_sub(ctx, NULL, b, (bn_t *)&gxBigIntOne);
	return bn_uxor(ctx, r, a, b);
}

bn_t *
bn_or(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b)
{
	int i;
	bn_t *t;
	if (a->sign) {
		if (b->sign)
			goto OR_MINUS_MINUS;
		t = a;
		a = b;
		b = t;	
		goto OR_PLUS_MINUS;
	}
	if (b->sign)
		goto OR_PLUS_MINUS;
// OR_PLUS_PLUS		
	if (a->size < b->size) {
		t = b;
		b = a;
		a = t;
	}
	if (r == NULL)
		r = bn_alloct(ctx, a->size);
	for (i = 0; i < b->size; i++)
		r->data[i] = a->data[i] | b->data[i];
	for (; i < a->size; i++)
		r->data[i] = a->data[i];
	return(r);
OR_PLUS_MINUS:
    /* Operand 2 negative, so will be the result.
       -(OP1 | (-OP2)) = -(OP1 | ~(OP2 - 1)) =
       = ~(OP1 | ~(OP2 - 1)) + 1 =
       = (~OP1 & (OP2 - 1)) + 1      */
	b->sign = 0;
	b = bn_sub(ctx, NULL, b, (bn_t *)&gxBigIntOne);
	t = bn_alloct(ctx, b->size);
    if (a->size < b->size) {
		for (i = 0; i < a->size; i++)
			t->data[i] = ~a->data[i] & b->data[i];
		for (; i < b->size; i++)
			t->data[i] = b->data[i];
	}
	else {
		for (i = 0; i < b->size; i++)
			t->data[i] = ~a->data[i] & b->data[i];
	}
	r = bn_add(ctx, r, t, (bn_t *)&gxBigIntOne);
	r->sign = 1;
	return(r);
OR_MINUS_MINUS:
	  /* Both operands are negative, so will be the result.
	     -((-OP1) | (-OP2)) = -(~(OP1 - 1) | ~(OP2 - 1)) =
	     = ~(~(OP1 - 1) | ~(OP2 - 1)) + 1 =
	     = ((OP1 - 1) & (OP2 - 1)) + 1      */
	a->sign = 0;
	b->sign = 0;
	a = bn_sub(ctx, NULL, a, (bn_t *)&gxBigIntOne);
	b = bn_sub(ctx, NULL, b, (bn_t *)&gxBigIntOne);
	r = bn_add(ctx, r, bn_and(ctx, NULL, a, b), (bn_t *)&gxBigIntOne);
	r->sign = 1;
	return(r);
}

static bn_t *
bn_and(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b)
{
	int i;
	bn_t *t;
	if (a->sign) {
		if (b->sign)
			goto AND_MINUS_MINUS;
		t = a;
		a = b;
		b = t;	
		goto AND_PLUS_MINUS;
	}
	if (b->sign)
		goto AND_PLUS_MINUS;
// AND_PLUS_PLUS		
	if (a->size > b->size) {
		bn_t *t = b;
		b = a;
		a = t;
	}
	if (r == NULL)
		r = bn_alloct(ctx, a->size);
	for (i = 0; i < a->size; i++)
		r->data[i] = a->data[i] & b->data[i];
	return(r);
AND_PLUS_MINUS:
    /* OP1 is positive and zero-extended,
       OP2 is negative and ones-extended.
       The result will be positive.
       OP1 & -OP2 = OP1 & ~(OP2 - 1).  */
	b->sign = 0;
	b = bn_sub(ctx, NULL, b, (bn_t *)&gxBigIntOne);
	if (r == NULL)
		r = bn_alloct(ctx, a->size);
    if (a->size > b->size) {
		for (i = 0; i < b->size; i++)
			r->data[i] = a->data[i] & ~b->data[i];
		for (; i < a->size; i++)
			r->data[i] = a->data[i];
    }
    else {
		for (i = 0; i < a->size; i++)
			r->data[i] = a->data[i] & ~b->data[i];
    }
	return(r);
AND_MINUS_MINUS:
  /* Both operands are negative, so will be the result.
	 -((-OP1) & (-OP2)) = -(~(OP1 - 1) & ~(OP2 - 1)) =
	 = ~(~(OP1 - 1) & ~(OP2 - 1)) + 1 =
	 = ((OP1 - 1) | (OP2 - 1)) + 1      */
	a->sign = 0;
	b->sign = 0;
	a = bn_sub(ctx, NULL, a, (bn_t *)&gxBigIntOne);
	b = bn_sub(ctx, NULL, b, (bn_t *)&gxBigIntOne);
	r = bn_add(ctx, r, bn_or(ctx, NULL, a, b), (bn_t *)&gxBigIntOne);
	r->sign = 1;
	return(r);
}

/*
 * arith on Z
 */

static int
bn_uadd_prim(bn_word *rp, bn_word *ap, bn_word *bp, int an, int bn)
{
	bn_word a, b, t, r, c = 0;
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

static bn_t *
bn_uadd(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb)
{
	bn_t *x, *y;
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
		rr = bn_alloct(ctx, y->size + 1);
	c = bn_uadd_prim(rr->data, x->data, y->data, x->size, y->size);
	/* CAUTION: rr may equals aa or bb. do not touch until here */
	rr->size = y->size;
	rr->sign = 0;
	if (c != 0)
		rr->data[rr->size++] = 1;
	return(rr);
}

static bn_t *
bn_usub(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb)
{
	int i, n;
	bn_word a, b, r, t;
	bn_word *ap, *bp, *rp;
	unsigned int c = 0;

	if (rr == NULL)
		rr = bn_alloct(ctx, MAX(aa->size, bb->size));
	rr->sign = (aa->size < bb->size ||
		    (aa->size == bb->size && bn_ucomp(aa, bb) < 0));
	if (rr->sign) {
		bn_t *tt = aa;
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
			t = -bp[i];
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

bn_t *
bn_add(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb)
{
	if ((aa->sign ^ bb->sign) == 0) {
		rr = bn_uadd(ctx, rr, aa, bb);
		if (aa->sign)
			rr->sign = 1;
	}
	else {
		if (!aa->sign)
			rr = bn_usub(ctx, rr, aa, bb);
		else
			rr = bn_usub(ctx, rr, bb, aa);
	}
	return(rr);
}

bn_t *
bn_sub(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb)
{
	if ((aa->sign ^ bb->sign) == 0) {
		if (!aa->sign)
			rr = bn_usub(ctx, rr, aa, bb);
		else
			rr = bn_usub(ctx, rr, bb, aa);
	}
	else {
		bn_bool sign = aa->sign;	/* could be replaced if rr=aa */
		rr = bn_uadd(ctx, rr, aa, bb);
		rr->sign = sign;
	}
	return(rr);
}

static bn_t *
bn_umul(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb)
{
	bn_dword a, b, r;
	bn_word *ap, *bp, *rp;
	bn_word c = 0;
	int i, j, n, m;

	if (rr == NULL)
		rr = bn_alloct(ctx, aa->size + bb->size);
	bn_fill0(rr);
	ap = aa->data;
	bp = bb->data;
	rp = rr->data;
	n = bb->size;
	for (i = 0, j = 0; i < n; i++) {
		b = (bn_dword)bp[i];
		c = 0;
		m = aa->size;
		for (j = 0; j < m; j++) {
			a = (bn_dword)ap[j];
			r = a * b + c;
			r += (bn_dword)rp[i + j];
			rp[i + j] = bn_low_word(r);
			c = bn_high_word(r);
		}
		rp[i + j] = c;
	}
	/* remove leading 0s */
	for (n = i + j; --n > 0 && rp[n] == 0;)
		;
	rr->size = n + 1;
	return(rr);
}

bn_t *
bn_mul(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb)
{
	rr = bn_umul(ctx, rr, aa, bb);
	if (aa->sign != bb->sign)
		rr->sign = 1;
	return(rr);
}

bn_t *
bn_umul1(bn_context_t *ctx, bn_t *r, bn_t *a, bn_word b)
{
	int i, n;
	bn_word c = 0;
	bn_word *ap, *rp;
	bn_dword wa, wr;

	if (r == NULL)
		r = bn_alloct(ctx, a->size + 1);
	ap = a->data;
	rp = r->data;
	n = a->size;
	for (i = 0; i < n; i++) {
		wa = (bn_dword)ap[i];
		wr = wa * b + c;
		c = bn_high_word(wr);
		rp[i] = bn_low_word(wr);
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

static void
bn_makepoly(bn_t *r, bn_t *a, int t)
{
	int n;
	bn_word *rp, *ap;

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
static bn_dword
div64_32(bn_dword a, bn_word b)
{
	bn_word high = (bn_word)(a >> 32);
	bn_dword r = 0, bb = b, d = 1;

	if (high >= b) {
		high /= b;
		r = (bn_dword)high << 32;
		a -= (bn_dword)(high * b) << 32;
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

static bn_t *
bn_udiv(bn_context_t *ctx, bn_t *q, bn_t *a, bn_t *b, bn_t **r)
{
	int sw;
	bn_t *nb, *na, *tb, *a2, *b2, *tb2, *tb3;
	int i, n, t;
	bn_word *qp, *ap, *bp;
#define mk_dword(p, i)	(((bn_dword)(p)[i] << bn_wordsize) | (p)[i - 1])

	if (bn_ucomp(a, b) < 0) {
		if (q == NULL) {
			q = bn_alloct(ctx, 1);
		}
		else {
			q->sign = 0;
			q->size = 1;
		}
		q->data[0] = 0;
		if (r != NULL) {
			if (*r == NULL)
				*r = bn_dup(ctx, a);
			else
				bn_copy(*r, a);
			(*r)->sign = 0;
		}
		return(q);
	}

	/* CAUTION: if q is present, it must take account of normalization */
	if (q == NULL)
		q = bn_alloct(ctx, a->size - b->size + 2);
	if (r != NULL && *r == NULL)
		*r = bn_alloct(ctx, b->size);

	/* normalize */
	sw = bn_ffs(b);
	nb = bn_ulsl(ctx, NULL, b, sw);
	na = bn_ulsl(ctx, NULL, a, sw);
	t = nb->size - 1;	/* the size must not change from 'b' */
	n = na->size - 1;

	/* adjust size of q */
	q->size = na->size - nb->size + 1;
	bn_fill0(q);	/* set 0 to quotient */

	/* process the most significant word */
	qp = &q->data[q->size - 1];
	tb = bn_ulsl(ctx, NULL, nb, (n - t) * bn_wordsize);	/* y*b^n */
	if (bn_ucomp(na, tb) >= 0) {
		(*qp)++;
		bn_sub(ctx, na, na, tb);
		/* since nomalization done, must be na < tb here */
	}

	/* prepare the constant for the adjustment: y_t*b + y_{t-1} */
	b2 = bn_alloct(ctx, 2);
	bn_makepoly(b2, nb, t);
	/* and allocate for temporary buffer */
	a2 = bn_alloct(ctx, 3);
	tb2 = bn_alloct(ctx, 3);
	tb3 = bn_alloct(ctx, tb->size + 1);

	ap = na->data;
	bp = nb->data;
	for (i = n; i >= t + 1; --i) {
		bn_word tq;

		/* the first estimate */
#if BN_NO_ULDIVMOD
		tq = (ap[i] == bp[t]) ? ~(bn_word)0: (bn_word)div64_32(mk_dword(ap, i), bp[t]);
#else
		tq = (ap[i] == bp[t]) ? ~(bn_word)0: (bn_word)(mk_dword(ap, i) / bp[t]);
#endif

		/* adjust */
		bn_makepoly(a2, na, i);
		while (bn_ucomp(bn_umul1(ctx, tb2, b2, tq), a2) > 0)
			--tq;

		/* next x */
		bn_ulsr(ctx, tb, tb, bn_wordsize);
		bn_usub(ctx, na, na, bn_umul1(ctx, tb3, tb, tq));
		if (na->sign) {
			bn_add(ctx, na, na, tb);
			--tq;
		}
		*--qp = tq;
	}
	if (r != NULL)
		*r = bn_ulsr(ctx, *r, na, sw);
	/* remove leading 0s from q */
	for (i = q->size; --i > 0 && q->data[i] == 0;)
		;
	q->size = i + 1;
	bn_freebuf(ctx, nb);
	return(q);
}

bn_t *
bn_div(bn_context_t *ctx, bn_t *q, bn_t *a, bn_t *b, bn_t **r)
{
	q = bn_udiv(ctx, q, a, b, r);
	if (a->sign) {
		q->sign = !q->sign;
		if (r && !bn_iszero(*r)) {
			bn_sub(ctx, *r, b, *r);
//             (*r)->sign = !(*r)->sign;
		}
	}
	if (b->sign)
		q->sign = !q->sign;
	return(q);
}

bn_t *
bn_mod(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b)
{
	(void)bn_div(ctx, NULL, a, b, &r);
	/* cannot deallocate 'q' here */
	return(r);
}

bn_t *
bn_rem(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b)
{
	bn_t *q = bn_udiv(ctx, NULL, a, b, &r);
	if (a->sign) {
		if (!bn_iszero(r))
            r->sign = !r->sign;
	}
	bn_freet(ctx, q);
	return(r);
}


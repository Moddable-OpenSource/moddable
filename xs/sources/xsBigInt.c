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
	#include "bn.h"

	void *bn_allocbuf(txMachine *the, unsigned int n)
	{
		return fxNewChunk(the, n);
	}

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
#else
	#include "xsCommon.h"
	#include "bn.h"
#endif

void fxBigIntEncode(txByte* code, void *it, txSize size)
{
#if mxBigEndian
	#error
#else
	c_memcpy(code, it, size);
#endif
}

txSize fxBigIntMaximum(txInteger length)
{
	return sizeof(bn_t) + ((txSize)c_ceil(((double)length * c_log(10) / c_log(2))) >> 3);
}

txSize fxBigIntMaximumB(txInteger length)
{
	return sizeof(bn_t) + (length >> 3);
}

txSize fxBigIntMaximumO(txInteger length)
{
	return sizeof(bn_t) + ((length * 3) >> 3);
}

txSize fxBigIntMaximumX(txInteger length)
{
	return sizeof(bn_t) + ((length * 4) >> 3);
}

txSize fxBigIntMeasure(void *it)
{
	bn_t* result = (bn_t*)it;
	return sizeof(bn_t) + ((result->size - 1) * sizeof(bn_word));
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

void fxBigIntParse(void *it, txString p, txInteger length, txInteger sign)
{
	bn_t digit = { .sign=0, .size=1, .data={ 0 } };
	bn_t* result = (bn_t*)it;
	result->sign = 0;
	result->size = 1;
	result->data[0] = 0;
	while (length) {
		char c = *p++;
		digit.data[0] = c - '0';
		result = bn_add(NULL, result, bn_umul1(NULL, result, result, 10), &digit);
		length--;
	}
	result->sign = sign;
}

void fxBigIntParseB(void *it, txString p, txInteger length)
{
	bn_t digit = { .sign=0, .size=1, .data={ 0 } };
	bn_t* result = (bn_t*)it;
	result->sign = 0;
	result->size = 1;
	result->data[0] = 0;
	while (length) {
		char c = *p++;
		digit.data[0] = c - '0';
		result = bn_add(NULL, result, bn_lsl(NULL, result, result, 1), &digit);
		length--;
	}
}

void fxBigIntParseO(void *it, txString p, txInteger length)
{
	bn_t digit = { .sign=0, .size=1, .data={ 0 } };
	bn_t* result = (bn_t*)it;
	result->sign = 0;
	result->size = 1;
	result->data[0] = 0;
	while (length) {
		char c = *p++;
		digit.data[0] = c - '0';
		result = bn_add(NULL, result, bn_lsl(NULL, result, result, 3), &digit);
		length--;
	}
}

void fxBigIntParseX(void *it, txString p, txInteger length)
{
	bn_t digit = { .sign=0, .size=1, .data={ 0 } };
	bn_t* result = (bn_t*)it;
	result->sign = 0;
	result->size = 1;
	result->data[0] = 0;
	while (length) {
		char c = *p++;
		if (('0' <= c) && (c <= '9'))
			digit.data[0] = c - '0';
		else if (('a' <= c) && (c <= 'f'))
			digit.data[0] = 10 + c - 'a';
		else
			digit.data[0] = 10 + c - 'A';
		result = bn_add(NULL, result, bn_lsl(NULL, result, result, 4), &digit);
		length--;
	}
}

#ifdef mxRun

static txSlot* fxCheckBigInt(txMachine* the, txSlot* it);
static void* fxIntegerToBigInt(txMachine* the, txSlot* slot);
static void* fxNumberToBigInt(txMachine* the, txSlot* slot);
static void* fxToBigInt(txMachine* the, txSlot* theSlot);

const bn_t gxBigIntNaN = { .sign=0, .size=0, .data={0} };
const bn_t gxBigIntOne = { .sign=0, .size=1, .data={1} };
const bn_t gxBigIntZero = { .sign=0, .size=1, .data={0} };

void* fxNewBigInt(txMachine* the, bn_size size)
{
	bn_t* bigint = fxNewChunk(the, sizeof(bn_t) + ((size - 1) * sizeof(bn_word)));
	bigint->sign = 0;
	bigint->size = 1;
	bigint->data[0] = 0;
	return bigint;
}

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
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, mxCallback(fx_BigInt), 0, mxID(_BigInt), XS_DONT_ENUM_FLAG));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_asIntN), 2, mxID(_asIntN), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_asUintN), 2, mxID(_asUintN), XS_DONT_ENUM_FLAG);
	the->stack++;
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
	if (slot->kind != XS_UNDEFINED_KIND)
		return 0;
	value = fxToNumber(the, slot);
	if (c_isnan(value))
		return 0;
	index = c_trunc(value);
	if ((index < 0) || (C_MAX_SAFE_INTEGER < index) || (value != index))
		mxRangeError("out of range index");
	return index;
}

void fx_BigInt_asIntN(txMachine* the)
{
	txInteger index = (mxArgc < 1) ? 0 : (txInteger)fxToIndex(the, mxArgv(0));
	if (mxArgc < 2)
		mxTypeError("no bigint");
	fxToBigInt(the, mxArgv(1));
	mxResult->value.bigint = fxNewBigInt(the, 1 + (index >> 5));
	mxResult->kind = XS_BIGINT_KIND;
	mxResult->value.bigint = bn_lsl(the, mxResult->value.bigint, (bn_t *)&gxBigIntOne, index);
	mxResult->value.bigint = bn_sub(the, mxResult->value.bigint, mxResult->value.bigint, (bn_t *)&gxBigIntOne);
	mxResult->value.bigint = bn_and(the, mxResult->value.bigint, mxArgv(1)->value.bigint, mxResult->value.bigint);
}

void fx_BigInt_asUintN(txMachine* the)
{
	txInteger index = (mxArgc < 1) ? 0 : (txInteger)fxToIndex(the, mxArgv(0));
	if (mxArgc < 2)
		mxTypeError("no bigint");
	fxToBigInt(the, mxArgv(1));
	mxResult->value.bigint = fxNewBigInt(the, 1 + (index >> 5));
	mxResult->kind = XS_BIGINT_KIND;
	mxResult->value.bigint = bn_lsl(the, mxResult->value.bigint, (bn_t *)&gxBigIntOne, index);
	mxResult->value.bigint = bn_sub(the, mxResult->value.bigint, mxResult->value.bigint, (bn_t *)&gxBigIntOne);
	mxResult->value.bigint = bn_and(the, mxResult->value.bigint, mxArgv(1)->value.bigint, mxResult->value.bigint);
	printbn(mxResult->value.bigint);
}

void fx_BigInt_prototype_toString(txMachine* the)
{
	txSlot* slot = fxCheckBigInt(the, mxThis);
	txInteger radix;
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
	// @@	
}

void fx_BigInt_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxCheckBigInt(the, mxThis);
	if (!slot) mxTypeError("this is no bigint");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
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

void* fxToBigInt(txMachine* the, txSlot* theSlot)
{
again:
	switch (theSlot->kind) {
	case XS_BOOLEAN_KIND:
		theSlot->value.bigint = bn_dup(the, (bn_t *)((theSlot->value.boolean) ? &gxBigIntOne : &gxBigIntZero));
		theSlot->kind = XS_BIGINT_KIND;
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		theSlot->value.bigint = fxStringToBigInt(the, theSlot, 1);
		if (bn_isNaN(theSlot->value.bigint))
			mxSyntaxError("Cannot coerce string to bigint");
		theSlot->kind = XS_BIGINT_KIND;
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
	return theSlot->value.bigint;
}

void fxBigIntBinary(txMachine* the, txU1 code, txSlot* left, txSlot* right)
{
	bn_size leftSize, rightSize;
	void* result;
	if ((left->kind != XS_BIGINT_KIND) && (left->kind != XS_BIGINT_X_KIND))
		mxTypeError("Cannot coerce left operand to bigint");
	if ((right->kind != XS_BIGINT_KIND) && (right->kind != XS_BIGINT_X_KIND))
		mxTypeError("Cannot coerce right operand to bigint");
	leftSize = ((bn_t*)left->value.bigint)->size;
	rightSize = ((bn_t*)left->value.bigint)->size;
	switch (code) {
	case XS_CODE_BIT_AND:
		result = fxNewBigInt(the, leftSize < rightSize ? leftSize : rightSize);
		left->value.bigint = bn_and(the, result, left->value.bigint, right->value.bigint);
		break;
	case XS_CODE_BIT_OR:
		result = fxNewBigInt(the, leftSize > rightSize ? leftSize : rightSize);
		left->value.bigint = bn_or(the, result, left->value.bigint, right->value.bigint);
		break;
	case XS_CODE_BIT_XOR:
		result = fxNewBigInt(the, leftSize > rightSize ? leftSize : rightSize);
		left->value.bigint = bn_xor(the, result, left->value.bigint, right->value.bigint);
		break;
	case XS_CODE_LEFT_SHIFT:
		result = fxNewBigInt(the, leftSize + (((bn_t*)(right->value.bigint))->data[0] >> 5));
		left->value.bigint = bn_lsl(the, C_NULL, left->value.bigint, ((bn_t*)(right->value.bigint))->data[0]); //@@
		break;
	case XS_CODE_SIGNED_RIGHT_SHIFT:
		result = fxNewBigInt(the, leftSize);
		left->value.bigint = bn_lsr(the, C_NULL, left->value.bigint, ((bn_t*)(right->value.bigint))->data[0]); //@@
		break;
	case XS_CODE_UNSIGNED_RIGHT_SHIFT:
		result = fxNewBigInt(the, leftSize);
		left->value.bigint = bn_lsr(the, C_NULL, left->value.bigint, ((bn_t*)(right->value.bigint))->data[0]); //@@
		break;
	case XS_CODE_ADD:
		result = fxNewBigInt(the, leftSize > rightSize ? leftSize + 1 : rightSize + 1);
		left->value.bigint = bn_add(the, result, left->value.bigint, right->value.bigint);
		break;
	case XS_CODE_SUBTRACT:
		result = fxNewBigInt(the, leftSize > rightSize ? leftSize : rightSize);
		left->value.bigint = bn_sub(the, result, left->value.bigint, right->value.bigint);
		break;
	case XS_CODE_EXPONENTIATION:
		//@@
		break;
	case XS_CODE_MULTIPLY:
		result = fxNewBigInt(the, leftSize + rightSize);
		left->value.bigint = bn_mul(the, C_NULL, left->value.bigint, right->value.bigint);
		break;
	case XS_CODE_DIVIDE:
		//@@
		left->value.bigint = bn_div(the, C_NULL, left->value.bigint, right->value.bigint, C_NULL);
		break;
	case XS_CODE_MODULO:
		//@@
		left->value.bigint = bn_mod(the, C_NULL, left->value.bigint, right->value.bigint);
		break;
	}
	left->kind = XS_BIGINT_KIND;
}

txBoolean fxBigIntCompare(txMachine* the, txU1 code, txSlot* left, txSlot* right)
{
	int result;
	if (bn_isNaN(left->value.bigint))
		return 0;
	if (right->kind == XS_STRING_KIND)
		fxStringToBigInt(the, right, 0);
	if ((right->kind == XS_BIGINT_KIND) || (right->kind == XS_BIGINT_X_KIND)) {
		if (bn_isNaN(right->value.bigint))
			return 0;
		result = bn_comp(left->value.bigint, right->value.bigint);
		if (result < 0)
			return ((code == XS_CODE_LESS) || (code == XS_CODE_LESS_EQUAL) || (code == XS_CODE_NOT_EQUAL));
		if (result > 0)
			return ((code == XS_CODE_MORE) || (code == XS_CODE_MORE_EQUAL) || (code == XS_CODE_NOT_EQUAL));
		return ((code == XS_CODE_EQUAL) || (code == XS_CODE_LESS_EQUAL) || (code == XS_CODE_MORE_EQUAL));
	}
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
	return 0; //@@ compare bigint to number
}

void* fxBigIntDecode(txMachine* the, txSize size)
{
	void* bigint = fxNewChunk(the, size);
#if mxBigEndian
	#error
#else
	c_memcpy(bigint, the->code, size);
#endif	
	return bigint;
}

void fxBigIntUnary(txMachine* the, txU1 code, txSlot* left)
{
	bn_size leftSize;
	void* result;
	leftSize = ((bn_t*)left->value.bigint)->size;
	switch (code) {
	case XS_CODE_BIT_NOT:
		//@@
		break;
	case XS_CODE_MINUS:
		left->value.bigint = bn_dup(the, left->value.bigint);
		bn_negate(left->value.bigint);
		break;
	case XS_CODE_DECREMENT:
		result = fxNewBigInt(the, leftSize);
		left->value.bigint = bn_sub(the, result, left->value.bigint, (bn_t *)&gxBigIntOne);
		break;
	case XS_CODE_INCREMENT:
		result = fxNewBigInt(the, leftSize + 1);
		left->value.bigint = bn_add(the, result, left->value.bigint, (bn_t *)&gxBigIntOne);
		break;
	}
	left->kind = XS_BIGINT_KIND;
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

void* fxIntegerToBigInt(txMachine* the, txSlot* slot)
{
	bn_t* bigint;
	txInteger integer = slot->value.integer;
	bn_bool sign = 0;
	if (integer < 0) {
		integer = -integer;
		sign = 1;
	}
	bigint = fxNewChunk(the, sizeof(bn_t));
	bigint->sign = sign;
	bigint->size = 1;
	bigint->data[0] = (bn_word)integer;
	slot->value.bigint = bigint;
	slot->kind = XS_BIGINT_KIND;
	return bigint;
}

void* fxNumberToBigInt(txMachine* the, txSlot* slot)
{
	bn_t* bigint;
	int64_t integer = (int64_t)slot->value.number;
	bn_bool sign = 0;
	if (integer < 0) {
		integer = -integer;
		sign = 1;
	}
	if (integer > 0x00000000FFFFFFFFll) {
		bigint = fxNewChunk(the, sizeof(bn_t) + sizeof(bn_word));
		bigint->sign = sign;
		bigint->size = 2;
		bigint->data[0] = (bn_word)(integer & 0x00000000FFFFFFFFll);
		bigint->data[1] = (bn_word)(integer >> 32);
	}
	else {
		bigint = fxNewChunk(the, sizeof(bn_t));
		bigint->sign = sign;
		bigint->size = 1;
		bigint->data[0] = (uint32_t)integer;
	}
	slot->value.bigint = bigint;
	slot->kind = XS_BIGINT_KIND;
	return bigint;
}

void* fxStringToBigInt(txMachine* the, txSlot* slot, txFlag whole)
{
	txString s = slot->value.string;
	txString p = fxSkipSpaces(s);
	txSize offset, length;
	txInteger sign = 0;
	void* chunk = C_NULL;
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
					chunk = fxNewChunk(the, fxBigIntMaximumB(length));
					fxBigIntParseB(chunk, slot->value.string + offset, length);
				}
			}
			else if ((d == 'O') || (d == 'o')) {
				while (((c = *p)) && ('0' <= c) && (c <= '7'))
					p++;
				length = p - s - offset;
				p = fxSkipSpaces(p);
				if ((length > 0) && (*p == 0)) {
					chunk = fxNewChunk(the, fxBigIntMaximumO(length));
					fxBigIntParseO(chunk, slot->value.string + offset, length);
				}
			}
			else if ((d == 'X') || (d == 'x')) {
				while (((c = *p)) && ((('0' <= c) && (c <= '9')) || (('a' <= c) && (c <= 'f')) || (('A' <= c) && (c <= 'F'))))
					p++;
				length = p - s - offset;
				p = fxSkipSpaces(p);
				if ((length > 0) && (*p == 0)) {
					chunk = fxNewChunk(the, fxBigIntMaximumX(length));
					fxBigIntParseX(chunk, slot->value.string + offset, length);
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
		chunk = fxNewChunk(the, fxBigIntMaximum(length));
		fxBigIntParse(chunk, slot->value.string + offset, length, sign);
	}
bail:
	if (chunk) {
		slot->value.bigint = chunk;
		slot->kind = XS_BIGINT_KIND;
	}
	else {
		slot->value.bigint = (void*)&gxBigIntNaN;
		slot->kind = XS_BIGINT_X_KIND;
	}
	return slot->value.bigint;
}

#endif



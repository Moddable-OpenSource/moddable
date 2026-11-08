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

static txSlot* fxCheckNumber(txMachine* the, txSlot* it);

void fxBuildNumber(txMachine* the)
{
	txSlot* slot;
	
	mxPushNumber((txNumber)C_INFINITY);
	mxPull(mxInfinity);
	mxPushNumber((txNumber)C_NAN);
	mxPull(mxNaN);

	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewNumberInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Number_prototype_toExponential), 1, mxID(_toExponential), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Number_prototype_toFixed), 1, mxID(_toFixed), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Number_prototype_toLocaleString), 0, mxID(_toLocaleString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Number_prototype_toPrecision), 1, mxID(_toPrecision), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Number_prototype_toString), 1, mxID(_toString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Number_prototype_valueOf), 0, mxID(_valueOf), XS_DONT_ENUM_FLAG);
	mxNumberPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_Number), 1, mxID(_Number));
	mxNumberConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Number_isFinite), 1, mxID(_isFinite), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Number_isInteger), 1, mxID(_isInteger), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Number_isNaN), 1, mxID(_isNaN), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Number_isSafeInteger), 1, mxID(_isSafeInteger), XS_DONT_ENUM_FLAG);
	slot = fxNextSlotProperty(the, slot, &mxParseFloatFunction, mxID(_parseFloat), XS_DONT_ENUM_FLAG);
	slot = fxNextSlotProperty(the, slot, &mxParseIntFunction, mxID(_parseInt), XS_DONT_ENUM_FLAG);
	slot = fxNextNumberProperty(the, slot, C_EPSILON, mxID(_EPSILON), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, C_MAX_SAFE_INTEGER, mxID(_MAX_SAFE_INTEGER), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, C_DBL_MAX, mxID(_MAX_VALUE), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, C_MIN_SAFE_INTEGER, mxID(_MIN_SAFE_INTEGER), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, C_DBL_MIN, mxID(_MIN_VALUE), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, C_NAN, mxID(_NaN), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, -((txNumber)C_INFINITY), mxID(_NEGATIVE_INFINITY), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextNumberProperty(the, slot, (txNumber)C_INFINITY, mxID(_POSITIVE_INFINITY), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPop();
}

txSlot* fxNewNumberInstance(txMachine* the)
{
	txSlot* instance;
	instance = fxNewObjectInstance(the);
	fxNextNumberProperty(the, instance, 0, XS_NO_ID, XS_INTERNAL_FLAG);
	return instance;
}

void fxNumberCoerce(txMachine* the, txSlot* slot)
{
	fxToNumber(the, slot);
}

void fx_isFinite(txMachine* the)
{
	int fpclass;
	txNumber number = (mxArgc < 1) ?  C_NAN : fxToNumber(the, mxArgv(0)); 
	mxResult->kind = XS_BOOLEAN_KIND;
    mxResult->value.boolean = 0;
	fpclass = c_fpclassify(number);
	if ((fpclass != C_FP_NAN) && (fpclass != C_FP_INFINITE))
		mxResult->value.boolean = 1;
}

void fx_isNaN(txMachine* the)
{
	int fpclass;
	txNumber number = (mxArgc < 1) ?  C_NAN : fxToNumber(the, mxArgv(0)); 
	mxResult->kind = XS_BOOLEAN_KIND;
    mxResult->value.boolean = 0;
	fpclass = c_fpclassify(number);
	if (fpclass == C_FP_NAN)
		mxResult->value.boolean = 1;
}

void fx_parseFloat(txMachine* the)
{
	if (mxArgc < 1) {
		mxResult->value.number = C_NAN;
		mxResult->kind = XS_NUMBER_KIND;
        return;
	}
	fxToString(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = fxStringToNumber(the, mxArgv(0)->value.string, 0);
}

void fx_parseInt(txMachine* the)
{
	txInteger aRadix, aDigit;
	txNumber aSign, aResult;
	txString s, r;
	char c;
	
	if (mxArgc < 1) {
		mxResult->value.number = C_NAN;
		mxResult->kind = XS_NUMBER_KIND;
        return;
	}
	fxToString(the, mxArgv(0));
	if (mxArgc > 1) {
		aRadix = fxToInteger(the, mxArgv(1));
		if (aRadix) {
			if ((aRadix < 2) || (36 < aRadix)) {
				mxResult->kind = XS_NUMBER_KIND;
				mxResult->value.number = C_NAN;
				return;
			}
		}
	}
	else
		aRadix = 0;
	s = fxSkipSpaces(mxArgv(0)->value.string);
	c = *s;
	aSign = 1;
	if (c == '+')
		s++;
	else if (c == '-') {
		s++;
		aSign = -1;
	}
	if ((*s == '0') && ((*(s + 1) == 'x') || (*(s + 1) == 'X'))) {
		if ((aRadix == 0) || (aRadix == 16)) {
			aRadix = 16;
			s += 2;
		}
	}
	/*if (*s == '0') {
		if ((aRadix == 0) || (aRadix == 8)) {
			aRadix = 8;
		}
	}*/
	if (aRadix == 0)
		aRadix = 10;
	aResult = 0;
	r = s;
	while ((c = *s)) {
		if (('0' <= c) && (c <= '9'))
			aDigit = c - '0';
		else if (('a' <= c) && (c <= 'z'))
			aDigit = 10 + c - 'a';
		else if (('A' <= c) && (c <= 'Z'))
			aDigit = 10 + c - 'A';
		else
			break;
		if (aDigit >= aRadix)
			break;
		aResult = (aResult * aRadix) + aDigit;
		s++;
	}
	if (r == s) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
	}
	else {
		aResult *= aSign;
		aRadix = (txInteger)aResult;
		aSign = aRadix;
		if (aSign == aResult) {
			mxResult->value.integer = aRadix;
			mxResult->kind = XS_INTEGER_KIND;
		}
		else {
			mxResult->value.number = aResult;
			mxResult->kind = XS_NUMBER_KIND;
		}
	}
}

void fx_Number(txMachine* the)
{
	txNumber value = 0;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND)
			fxToPrimitive(the, slot, XS_NUMBER_HINT);
		if ((slot->kind == XS_BIGINT_KIND) || (slot->kind == XS_BIGINT_X_KIND))
			value = gxTypeBigInt.toNumber(the, slot);
		else
			value = fxToNumber(the, slot);
	}
	if (!mxHasTarget) {
        mxResult->kind = XS_NUMBER_KIND;
        mxResult->value.number = value;
        fx_Math_toInteger(the);
	}
	else {
		txSlot* instance;
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxNumberPrototype);
		instance = fxNewNumberInstance(the);
		instance->next->value.number = value;
		mxPullSlot(mxResult);
	}
}

void fx_Number_isFinite(txMachine* the)
{
	int fpclass;
	txSlot* slot = (mxArgc < 1) ?  C_NULL : mxArgv(0);
	mxResult->kind = XS_BOOLEAN_KIND;
    mxResult->value.boolean = 0;
    if (slot) {
    	if (slot->kind == XS_INTEGER_KIND)
			mxResult->value.boolean = 1;
    	else if (slot->kind == XS_NUMBER_KIND) {
			fpclass = c_fpclassify(slot->value.number);
			if ((fpclass != C_FP_NAN) && (fpclass != C_FP_INFINITE))
				mxResult->value.boolean = 1;
		}
	}
}

void fx_Number_isInteger(txMachine* the)
{
	int fpclass;
	txSlot* slot = (mxArgc < 1) ?  C_NULL : mxArgv(0);
	mxResult->kind = XS_BOOLEAN_KIND;
    mxResult->value.boolean = 0;
    if (slot) {
    	if (slot->kind == XS_INTEGER_KIND)
			mxResult->value.boolean = 1;
    	else if (slot->kind == XS_NUMBER_KIND) {
			fpclass = c_fpclassify(slot->value.number);
			if ((fpclass != C_FP_NAN) && (fpclass != C_FP_INFINITE)) {
				txNumber check = c_trunc(slot->value.number);
				if (slot->value.number == check)
					mxResult->value.boolean = 1;
			}
		}
	}
}

void fx_Number_isNaN(txMachine* the)
{
	int fpclass;
	txSlot* slot = (mxArgc < 1) ?  C_NULL : mxArgv(0);
	mxResult->kind = XS_BOOLEAN_KIND;
    mxResult->value.boolean = 0;
    if (slot) {
    	if (slot->kind == XS_NUMBER_KIND) {
			fpclass = c_fpclassify(slot->value.number);
			if (fpclass == C_FP_NAN)
				mxResult->value.boolean = 1;
		}
	}
}

void fx_Number_isSafeInteger(txMachine* the)
{
	int fpclass;
	txSlot* slot = (mxArgc < 1) ?  C_NULL : mxArgv(0);
	mxResult->kind = XS_BOOLEAN_KIND;
    mxResult->value.boolean = 0;
    if (slot) {
    	if (slot->kind == XS_INTEGER_KIND)
			mxResult->value.boolean = 1;
    	else if (slot->kind == XS_NUMBER_KIND) {
			fpclass = c_fpclassify(slot->value.number);
			if ((fpclass != C_FP_NAN) && (fpclass != C_FP_INFINITE)) {
				txNumber check = c_trunc(slot->value.number);
				if (slot->value.number == check) {
					if ((C_MIN_SAFE_INTEGER <= check) && (check <= C_MAX_SAFE_INTEGER))
						mxResult->value.boolean = 1;
				}
			}
		}
	}
}

void fx_Number_prototype_toExponential(txMachine* the)
{
	char buffer[256];
	txInteger mode = 0;
	txNumber precision = 0;
	txNumber value;
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this: not a number");
	value = slot->value.number;
	if ((mxArgc > 0) && !mxIsUndefined(mxArgv(0))) {
		precision = fxToNumber(the, mxArgv(0));
		if (c_isnan(precision))
			precision = 0;
		else if (c_isfinite(precision))
			precision = c_trunc(precision);
	}
	else
		precision = 0;
	if (c_isnan(value))
		precision = 0;
	else if (c_isfinite(value)) {
		if ((precision < 0) || (100 < precision))
			mxRangeError("invalid fractionDigits");
		if ((mxArgc > 0) && !mxIsUndefined(mxArgv(0)))
			precision++;
		mode = 'e';
	}
	else
		precision = 0;
	fxNumberToString(the, slot->value.number, buffer, sizeof(buffer), mode, (int)precision);
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Number_prototype_toFixed(txMachine* the)
{
	char buffer[256];
	txInteger mode = 0;
	txNumber precision = 0;
	txNumber value;
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this: not a number");
	value = slot->value.number;
	if ((mxArgc > 0) && !mxIsUndefined(mxArgv(0))) {
		precision = fxToNumber(the, mxArgv(0));
		if (c_isnan(precision))
			precision = 0;
		else if (c_isfinite(precision))
			precision = c_trunc(precision);
	}
	else
		precision = 0;
	if ((precision < 0) || (100 < precision))
		mxRangeError("invalid fractionDigits");
	if (c_fabs(value) >= 1e21)
		precision = 0;
	else
		mode = 'f';
	fxNumberToString(the, value, buffer, sizeof(buffer), mode, (int)precision);
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Number_prototype_toLocaleString(txMachine* the)
{
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this: not a number");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
	fxToString(the, mxResult);
}

void fx_Number_prototype_toPrecision(txMachine* the)
{
	char buffer[256];
	txInteger mode = 0;
	txNumber precision = 0;
	txNumber value;
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this: not a number");
	value = slot->value.number;
	if ((mxArgc > 0) && !mxIsUndefined(mxArgv(0))) {
		precision = fxToNumber(the, mxArgv(0));
		if (c_isnan(precision))
			precision = 0;
		else if (c_isfinite(precision))
			precision = c_trunc(precision);
		if (c_isnan(value))
			precision = 0;
		else if (c_isfinite(value)) {
			if ((precision < 1) || (100 < precision))
				mxRangeError("invalid precision");
			mode = 'g';
		}
		else
			precision = 0;
	}
	fxNumberToString(the, value, buffer, sizeof(buffer), mode, (int)precision);
	fxCopyStringC(the, mxResult, buffer);
}

#define mxRadixBigLimbs 72
#define mxRadixDigits 1100

// Multi-precision helpers for Number.prototype.toString(radix): little-endian arrays of 16-bit limbs.
static void fxRadixBigFromInteger(txU2* big, txNumber integer)
{
	txInteger i;
	for (i = 0; i < mxRadixBigLimbs; i++) {
		txNumber limb = c_fmod(integer, 65536.0);
		big[i] = (txU2)limb;
		integer = (integer - limb) / 65536.0;
	}
}

static void fxRadixBigMultiply(txU2* big, txInteger multiplier)
{
	txInteger i;
	txU4 carry = 0;
	for (i = 0; i < mxRadixBigLimbs; i++) {
		txU4 product = ((txU4)big[i] * (txU4)multiplier) + carry;
		big[i] = (txU2)product;
		carry = product >> 16;
	}
}

static txInteger fxRadixBigDivide(txU2* big, txInteger divisor)
{
	txInteger i;
	txU4 remainder = 0;
	for (i = mxRadixBigLimbs - 1; i >= 0; i--) {
		txU4 dividend = (remainder << 16) | big[i];
		big[i] = (txU2)(dividend / divisor);
		remainder = dividend % divisor;
	}
	return (txInteger)remainder;
}

static txInteger fxRadixBigIsZero(txU2* big)
{
	txInteger i;
	for (i = 0; i < mxRadixBigLimbs; i++) {
		if (big[i])
			return 0;
	}
	return 1;
}

static txInteger fxRadixBigCompare(txU2* a, txU2* b)
{
	txInteger i;
	for (i = mxRadixBigLimbs - 1; i >= 0; i--) {
		if (a[i] != b[i])
			return (a[i] < b[i]) ? -1 : 1;
	}
	return 0;
}

static void fxRadixBigSubtract(txU2* a, txU2* b)
{
	txInteger i;
	txInteger borrow = 0;
	for (i = 0; i < mxRadixBigLimbs; i++) {
		txInteger difference = (txInteger)a[i] - (txInteger)b[i] - borrow;
		borrow = (difference < 0) ? 1 : 0;
		a[i] = (txU2)(difference + (borrow << 16));
	}
}

static void fxRadixBigIncrement(txU2* big)
{
	txInteger i;
	for (i = 0; i < mxRadixBigLimbs; i++) {
		if (++big[i] != 0)
			break;
	}
}

// Exact conversion of a finite, nonzero number to a string in the given radix.
// The digits are chosen as in Number::toString (ECMA-262 6.1.6.1.20) step 5: they
// are the shortest sequence whose value rounds back to the number. The fraction
// digits are produced with exact integer arithmetic (see issue #1810).
static void fxNumberToRadixString(txMachine* the, txSlot* result, txNumber value, txInteger radix)
{
	static const char gxDigits[] ICACHE_FLASH_ATTR = "0123456789abcdefghijklmnopqrstuvwxyz";
	txU2 integer[mxRadixBigLimbs];
	txU2 fraction[mxRadixBigLimbs];
	txU2 scale[mxRadixBigLimbs];
	txU2 product[mxRadixBigLimbs];
	txU2 bound[mxRadixBigLimbs];
	txNumber mantissa, whole, fractionPart;
	txInteger exponent = 0;
	txInteger minus = 0;
	txInteger boundary, limbs, shift, count = 0, base, i, j, digit, length, start, end;
	txString buffer;
	int roundUp = 0;
	if (value < 0) {
		minus = 1;
		value = -value;
	}
	// value = mantissa * 2^exponent, with mantissa an integer
	mantissa = value;
	while (mantissa >= 9007199254740992.0) {
		mantissa /= 2;
		exponent++;
	}
	while ((mantissa < 4503599627370496.0) && (exponent > -1074)) {
		mantissa *= 2;
		exponent--;
	}
	// a power of two has a smaller gap below it than above it
	boundary = (mantissa == 4503599627370496.0) && (exponent > -1074);
	buffer = fxNewChunk(the, (2 * mxRadixDigits) + 2);
	base = mxRadixDigits;
	whole = c_floor(value);
	fractionPart = value - whole;
	fxRadixBigFromInteger(integer, whole);
	if (fractionPart != 0) {
		// fraction = F / 2^(16 * limbs), with 2^(16 * limbs) >= 2^-exponent
		txInteger k = -exponent;
		limbs = (k + 15) / 16;
		shift = (16 * limbs) - k;
		for (i = 0; i < mxRadixBigLimbs; i++) {
			fraction[i] = 0;
			scale[i] = 0;
		}
		for (i = 0; i < limbs; i++) {
			txNumber scaled = fractionPart * 65536.0;
			txNumber limb = c_floor(scaled);
			fraction[limbs - 1 - i] = (txU2)limb;
			fractionPart = scaled - limb;
		}
		// scale = 2^shift * radix^count, the weight of one unit in the last digit
		scale[0] = (txU2)(1 << shift);
		for (;;) {
			// generate the next digit: F = F * radix, the digit is the part above 2^(16 * limbs)
			fxRadixBigMultiply(fraction, radix);
			digit = fraction[limbs];
			fraction[limbs] = 0;
			fxRadixBigMultiply(scale, radix);
			buffer[base + 1 + count] = (char)digit;
			count++;
			// truncating here is within the lower half gap: 4F < 2 * scale (1 * scale at a boundary)
			for (i = 0; i < mxRadixBigLimbs; i++) {
				product[i] = fraction[i];
				bound[i] = scale[i];
			}
			fxRadixBigMultiply(product, 4);
			fxRadixBigMultiply(bound, boundary ? 1 : 2);
			if (fxRadixBigCompare(product, bound) < 0)
				break;
			// rounding up here is within the upper half gap: 4 * (2^(16 * limbs) - F) < 2 * scale
			for (i = 0; i < mxRadixBigLimbs; i++) {
				product[i] = 0;
				bound[i] = scale[i];
			}
			product[limbs] = 1;
			fxRadixBigSubtract(product, fraction);
			fxRadixBigMultiply(product, 4);
			fxRadixBigMultiply(bound, 2);
			if (fxRadixBigCompare(product, bound) < 0) {
				roundUp = 1;
				break;
			}
			if (count >= mxRadixDigits - 1)
				break;
		}
		if (roundUp) {
			j = count;
			for (;;) {
				if (j == 0) {
					fxRadixBigIncrement(integer);
					break;
				}
				digit = buffer[base + j] + 1;
				if (digit < radix) {
					buffer[base + j] = (char)digit;
					break;
				}
				buffer[base + j] = 0;
				j--;
			}
		}
		while ((count > 0) && (buffer[base + count] == 0))
			count--;
	}
	// integer digits, least significant first, ending just before the dot
	i = 0;
	do {
		buffer[base - 1 - i] = (char)fxRadixBigDivide(integer, radix);
		i++;
	} while (!fxRadixBigIsZero(integer));
	start = base - i;
	for (j = start; j < base; j++)
		buffer[j] = gxDigits[(txInteger)buffer[j]];
	if (count > 0) {
		buffer[base] = '.';
		for (j = base + 1; j <= base + count; j++)
			buffer[j] = gxDigits[(txInteger)buffer[j]];
		end = base + count + 1;
	}
	else
		end = base;
	if (minus)
		buffer[--start] = '-';
	buffer[end] = 0;
	length = end - start + 1;
	c_memmove(buffer, buffer + start, length);
	result->value.string = fxRenewChunk(the, buffer, length);
	result->kind = XS_STRING_KIND;
}

void fx_Number_prototype_toString(txMachine* the)
{
	txInteger radix;
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this: not a number");
	if ((mxArgc > 0) && (!mxIsUndefined(mxArgv(0)))) {
		radix = fxToInteger(the, mxArgv(0));
		if ((radix < 2) || (36 < radix))
			mxRangeError("invalid radix");
	}
	else	
		radix = 10;
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
	if (radix == 10)
		fxToString(the, mxResult);
	else {
		txNumber value = mxResult->value.number;
		switch (c_fpclassify(value)) {
		case C_FP_INFINITE:
			if (value < 0)
				fxStringX(the, mxResult, "-Infinity");
			else
				fxStringX(the, mxResult, "Infinity");
			break;
		case C_FP_NAN:
			fxStringX(the, mxResult, "NaN");
			break;
		case C_FP_ZERO:
			fxStringX(the, mxResult, "0");
			break;
		default:
			fxNumberToRadixString(the, mxResult, value, radix);
			break;
		}
	}
}

void fx_Number_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this: not a number");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

txSlot* fxCheckNumber(txMachine* the, txSlot* it)
{
	txSlot* result = C_NULL;
	if (it->kind == XS_INTEGER_KIND) {
		fxToNumber(the, it);
		result = it;
	}
	else if (it->kind == XS_NUMBER_KIND)
		result = it;
	else if (it->kind == XS_REFERENCE_KIND) {
		txSlot* instance = it->value.reference;
		it = instance->next;
		if ((it) && (it->flag & XS_INTERNAL_FLAG) && (it->kind == XS_NUMBER_KIND))
			result = it;
	}
	return result;
}

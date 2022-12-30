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
	if ((fpclass != FP_NAN) && (fpclass != FP_INFINITE))
		mxResult->value.boolean = 1;
}

void fx_isNaN(txMachine* the)
{
	int fpclass;
	txNumber number = (mxArgc < 1) ?  C_NAN : fxToNumber(the, mxArgv(0)); 
	mxResult->kind = XS_BOOLEAN_KIND;
    mxResult->value.boolean = 0;
	fpclass = c_fpclassify(number);
	if (fpclass == FP_NAN)
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
	mxResult->value.number = fxStringToNumber(the->dtoa, mxArgv(0)->value.string, 0);
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
			value = fxBigIntToNumber(the, slot);
		else
			value = fxToNumber(the, slot);
	}
	if (mxIsUndefined(mxTarget)) {
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
			if ((fpclass != FP_NAN) && (fpclass != FP_INFINITE))
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
			if ((fpclass != FP_NAN) && (fpclass != FP_INFINITE)) {
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
			if (fpclass == FP_NAN)
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
			if ((fpclass != FP_NAN) && (fpclass != FP_INFINITE)) {
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
	if (!slot) mxTypeError("this is no number");
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
	fxNumberToString(the->dtoa, slot->value.number, buffer, sizeof(buffer), mode, (int)precision);
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Number_prototype_toFixed(txMachine* the)
{
	char buffer[256];
	txInteger mode = 0;
	txNumber precision = 0;
	txNumber value;
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this is no number");
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
	fxNumberToString(the->dtoa, value, buffer, sizeof(buffer), mode, (int)precision);
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Number_prototype_toLocaleString(txMachine* the)
{
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this is no number");
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
	if (!slot) mxTypeError("this is no number");
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
	fxNumberToString(the->dtoa, value, buffer, sizeof(buffer), mode, (int)precision);
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Number_prototype_toString(txMachine* the)
{
	txInteger radix;
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this is no number");
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
		default: {
			// Thanks Google V8 for the fraction part
			static const char gxDigits[] ICACHE_FLASH_ATTR = "0123456789abcdefghijklmnopqrstuvwxyz";
			txInteger minus;
			txNumber integer;
			txNumber fraction;
			txNumber next;
			txU8* nextCast = (txU8*)&next;
			txNumber delta;
			txSize length;
			txString string;
			txNumber modulo;
			if (value < 0) {
				minus = 1;
				value = -value;
			} 
			else
				minus = 0;
			integer = c_floor(value);
			fraction = value - integer;
			next = value;
			*nextCast = *nextCast + 1;
			delta = 0.5 * (next - value);
			next = 0;
			*nextCast = *nextCast + 1;
			if (delta < next)
				delta = next;
			length = minus + ((integer)? (txSize)c_round(c_log1p(integer) / c_log(radix)) : 0) + 1;
			if (fraction >= delta) {
				#define mxFractionPartLength 2048
				txString dot;
				txInteger i = 0;
				txInteger digit;
				string = mxResult->value.string = fxNewChunk(the, length + 1 + mxFractionPartLength + 1);
				dot = string + length;
				dot[i++] = '.';
				do {
					fraction *= radix;
					delta *= radix;
					digit = (txInteger)fraction;
					dot[i++] = c_read8(gxDigits + digit);
					fraction -= digit;
					if (fraction > 0.5 || (fraction == 0.5 && (digit & 1))) {
						if (fraction + delta > 1) {
							for (;;) {
								char c;
								i--;
								if (i == 0) {
									integer += 1;
									break;
								}
								c = dot[i];
								digit = c > '9' ? (c - 'a' + 10) : (c - '0');
								if (digit + 1 < radix) {
									dot[i++] = c_read8(gxDigits + digit + 1);
									break;
								}
							}
							break;
						}
					}
				} while ((fraction >= delta) && (i < mxFractionPartLength));
				dot[i++] = 0;
				length += i;
				string = dot;
			}
			else {
				length += 1;
				string = mxResult->value.string = fxNewChunk(the, length);
				string += length;
				*(--string) = 0;
			}
			modulo = C_MAX_SAFE_INTEGER * radix;
			while (integer > modulo) {
				*(--string) = '0';
				integer = integer / radix;
			}
			do {
				modulo = c_fmod(integer, radix);
				*(--string) = c_read8(gxDigits + (txInteger)modulo);
				integer = (integer - modulo) / radix;
			} while (integer >= 1);
			if (minus) {
				*(--string) = '-';
			}
			minus = (txInteger)(string - mxResult->value.string);
			if (minus > 0) {
				length -= minus;
				c_memmove(mxResult->value.string, string, length);
			}
			mxResult->value.string = fxRenewChunk(the, mxResult->value.string, length);
			mxResult->kind = XS_STRING_KIND;
		}}
	}
}

void fx_Number_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxCheckNumber(the, mxThis);
	if (!slot) mxTypeError("this is no number");
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

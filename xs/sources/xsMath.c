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

void fxBuildMath(txMachine* the)
{
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_abs), 1, mxID(_abs), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_acos), 1, mxID(_acos), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_acosh), 1, mxID(_acosh), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_asin), 1, mxID(_asin), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_asinh), 1, mxID(_asinh), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_atan), 1, mxID(_atan), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_atanh), 1, mxID(_atanh), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_atan2), 2, mxID(_atan2), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_cbrt), 1, mxID(_cbrt), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_ceil), 1, mxID(_ceil), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_clz32), 1, mxID(_clz32), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_cos), 1, mxID(_cos), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_cosh), 1, mxID(_cosh), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_exp), 1, mxID(_exp), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_expm1), 1, mxID(_expm1), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_floor), 1, mxID(_floor), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_fround), 1, mxID(_fround), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_hypot), 2, mxID(_hypot_), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_idiv), 2, mxID(_idiv), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_idivmod), 2, mxID(_idivmod), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_imod), 2, mxID(_imod), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_imul), 2, mxID(_imul), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_imuldiv), 2, mxID(_imuldiv), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_irem), 2, mxID(_irem), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_log), 1, mxID(_log), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_log1p), 1, mxID(_log1p), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_log10), 1, mxID(_log10), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_log2), 1, mxID(_log2), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_max), 2, mxID(_max), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_min), 2, mxID(_min), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_mod), 2, mxID(_mod), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_pow), 2, mxID(_pow), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_random), 0, mxID(_random), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_round), 1, mxID(_round), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_sign), 1, mxID(_sign), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_sin), 1, mxID(_sin), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_sinh), 1, mxID(_sinh), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_sqrt), 1, mxID(_sqrt), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_tan), 1, mxID(_tan), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_tanh), 1, mxID(_tanh), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Math_trunc), 1, mxID(_trunc), XS_DONT_ENUM_FLAG);
	slot = fxNextNumberProperty(the, slot, C_M_E, mxID(_E), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_LN10, mxID(_LN10), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_LN2, mxID(_LN2), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_LOG10E, mxID(_LOG10E), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_LOG2E, mxID(_LOG2E), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_PI, mxID(_PI), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_SQRT1_2, mxID(_SQRT1_2), XS_GET_ONLY);
	slot = fxNextNumberProperty(the, slot, C_M_SQRT2, mxID(_SQRT2), XS_GET_ONLY);
	slot = fxNextStringXProperty(the, slot, "Math", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxMathObject);
//@@	c_srand((unsigned)c_time(0));
}

#define mxNanResultIfNoArg \
	if (mxArgc < 1) {  \
		mxResult->kind = XS_NUMBER_KIND; \
		mxResult->value.number = C_NAN; \
		return; \
	}

#define mxNanResultIfNoArg2 \
	if (mxArgc < 2) {  \
		mxResult->kind = XS_NUMBER_KIND; \
		mxResult->value.number = C_NAN; \
		return; \
	}

void fx_Math_abs(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_fabs(mxArgv(0)->value.number);
}

void fx_Math_acos(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_acos(mxArgv(0)->value.number);
}

void fx_Math_acosh(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_acosh(mxArgv(0)->value.number);
}

void fx_Math_asin(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_asin(mxArgv(0)->value.number);
}

void fx_Math_asinh(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_asinh(mxArgv(0)->value.number);
}

void fx_Math_atan(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_atan(mxArgv(0)->value.number);
}

void fx_Math_atanh(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_atanh(mxArgv(0)->value.number);
}

void fx_Math_atan2(txMachine* the)
{
	mxNanResultIfNoArg2;
	fxToNumber(the, mxArgv(0));
	fxToNumber(the, mxArgv(1));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_atan2(mxArgv(0)->value.number, mxArgv(1)->value.number);
}

void fx_Math_cbrt(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_cbrt(mxArgv(0)->value.number);
}

void fx_Math_ceil(txMachine* the)
{
	mxNanResultIfNoArg;
	if (XS_INTEGER_KIND == mxArgv(0)->kind) {
		mxResult->kind = XS_INTEGER_KIND;
		mxResult->value.integer = mxArgv(0)->value.integer;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_ceil(mxArgv(0)->value.number);
	fx_Math_toInteger(the);
}

void fx_Math_clz32(txMachine* the)
{
	txUnsigned x = (mxArgc > 0) ? fxToUnsigned(the, mxArgv(0)) : 0;
	txInteger r;
	if (x)
#if mxWindows
		_BitScanForward(&r, x);
#else
		r = __builtin_clz(x);
#endif	
	else
		r = 32;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = r;
}

void fx_Math_cos(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_cos(mxArgv(0)->value.number);
}

void fx_Math_cosh(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_cosh(mxArgv(0)->value.number);
}

void fx_Math_exp(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_exp(mxArgv(0)->value.number);
}

void fx_Math_expm1(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_expm1(mxArgv(0)->value.number);
}

void fx_Math_floor(txMachine* the)
{
	mxNanResultIfNoArg;
	if (XS_INTEGER_KIND == mxArgv(0)->kind) {
		mxResult->kind = XS_INTEGER_KIND;
		mxResult->value.integer = mxArgv(0)->value.integer;
		return;
	}
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_floor(mxArgv(0)->value.number);
	fx_Math_toInteger(the);
}

void fx_Math_fround(txMachine* the)
{
	float arg;
	mxNanResultIfNoArg;
	if (XS_INTEGER_KIND == mxArgv(0)->kind) {
		mxResult->kind = XS_INTEGER_KIND;
		mxResult->value.integer = mxArgv(0)->value.integer;
		return;
	}
	arg = (float)fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = arg;
}

void fx_Math_hypot(txMachine* the)
{
	if (mxArgc == 2) {
		fxToNumber(the, mxArgv(0));
		fxToNumber(the, mxArgv(1));
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = c_hypot(mxArgv(0)->value.number, mxArgv(1)->value.number);
	}
	else {
		txInteger c = mxArgc, i;
		txNumber result = 0;
		for (i = 0; i < c; i++) {
			txNumber argument = fxToNumber(the, mxArgv(i));
			result += argument * argument;
		}
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = c_sqrt(result);
	}
}

void fx_Math_idiv(txMachine* the)
{
	txInteger x = (mxArgc > 0) ? fxToInteger(the, mxArgv(0)) : 0;
	txInteger y = (mxArgc > 1) ? fxToInteger(the, mxArgv(1)) : 0;
	if (y == 0) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
	}
	else {
		mxResult->kind = XS_INTEGER_KIND;
#if mxIntegerDivideOverflowException
		if ((x == (txInteger)0x80000000) && (y == -1))
			mxResult->value.integer = x;
		else
#endif
			mxResult->value.integer = x / y;
	}
}

void fx_Math_idivmod(txMachine* the)
{
	txInteger x = (mxArgc > 0) ? fxToInteger(the, mxArgv(0)) : 0;
	txInteger y = (mxArgc > 1) ? fxToInteger(the, mxArgv(1)) : 0;
	if (y == 0) {
		mxPushNumber(C_NAN);
		mxPushNumber(C_NAN);
	}
	else {
#if mxIntegerDivideOverflowException
		if ((x == (txInteger)0x80000000) && (y == -1)) {
			mxPushInteger(x);
			mxPushInteger(0);
		}
		else
#endif
		{
			mxPushInteger(x / y);
			mxPushInteger((x % y + y) % y);
		}
	}
	fxConstructArrayEntry(the, mxResult);
}

void fx_Math_imod(txMachine* the)
{
	txInteger x = (mxArgc > 0) ? fxToInteger(the, mxArgv(0)) : 0;
	txInteger y = (mxArgc > 1) ? fxToInteger(the, mxArgv(1)) : 0;
	if (y == 0) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
	}
	else {
		mxResult->kind = XS_INTEGER_KIND;
#if mxIntegerDivideOverflowException
		if ((x == (txInteger)0x80000000) && (y == -1))
			mxResult->value.integer = 0;
		else
#endif
			mxResult->value.integer = (x % y + y) % y;
	}
}

void fx_Math_imul(txMachine* the)
{
	txInteger x = (mxArgc > 0) ? fxToInteger(the, mxArgv(0)) : 0;
	txInteger y = (mxArgc > 1) ? fxToInteger(the, mxArgv(1)) : 0;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = x * y;
}

void fx_Math_imuldiv(txMachine* the)
{
	txS8 x = (mxArgc > 0) ? fxToInteger(the, mxArgv(0)) : 0;
	txS8 y = (mxArgc > 1) ? fxToInteger(the, mxArgv(1)) : 0;
	txS8 z = (mxArgc > 2) ? fxToInteger(the, mxArgv(2)) : 0;
	if (z == 0) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
	}
	else {
		txS8 r = (x * y) / z;
		if ((-2147483648LL <= r) && (r <= 2147483647LL)) {
			mxResult->kind = XS_INTEGER_KIND;
			mxResult->value.integer = (txInteger)r;
		}
		else {
			mxResult->kind = XS_NUMBER_KIND;
			mxResult->value.number = (txNumber)r;
		}
	}
}

void fx_Math_irem(txMachine* the)
{
	txInteger x = (mxArgc > 0) ? fxToInteger(the, mxArgv(0)) : 0;
	txInteger y = (mxArgc > 1) ? fxToInteger(the, mxArgv(1)) : 0;
	if (y == 0) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
	}
	else {
		mxResult->kind = XS_INTEGER_KIND;
#if mxIntegerDivideOverflowException
		if ((x == (txInteger)0x80000000) && (y == -1))
			mxResult->value.integer = 0;
		else
#endif
			mxResult->value.integer = x % y;
	}
}

void fx_Math_log(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_log(mxArgv(0)->value.number);
}

void fx_Math_log1p(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_log1p(mxArgv(0)->value.number);
}

void fx_Math_log10(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_log10(mxArgv(0)->value.number);
}

void fx_Math_log2(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
#if mxAndroid
	mxResult->value.number = c_log(mxArgv(0)->value.number) / c_log(2);
#else
	mxResult->value.number = c_log2(mxArgv(0)->value.number);
#endif
}

void fx_Math_max(txMachine* the)
{
	txInteger c = mxArgc, i;
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = -((txNumber)C_INFINITY);
	for (i = 0; i < c; i++) {
		txNumber n = fxToNumber(the, mxArgv(i));
		if (c_isnan(n)) {
			for (; i < c; i++)
				fxToNumber(the, mxArgv(i));
			mxResult->value.number = C_NAN;
			return;
		}
		if (mxResult->value.number < n)
			mxResult->value.number = n;
		else if ((mxResult->value.number == 0) && (n == 0)) {
			if (c_signbit(mxResult->value.number) != c_signbit(n))
				mxResult->value.number = 0;
		}
	}
}

void fx_Math_min(txMachine* the)
{
	txInteger c = mxArgc, i;
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = (txNumber)C_INFINITY;
	for (i = 0; i < c; i++) {
		txNumber n = fxToNumber(the, mxArgv(i));
		if (c_isnan(n)) {
			for (; i < c; i++)
				fxToNumber(the, mxArgv(i));
			mxResult->value.number = C_NAN;
			return;
		}
		if (mxResult->value.number > n)
			mxResult->value.number = n;
		else if ((mxResult->value.number == 0) && (n == 0)) {
			if (c_signbit(mxResult->value.number) != c_signbit(n))
				mxResult->value.number = -0.0;
		}
	}
}

void fx_Math_mod(txMachine* the)
{
	txNumber x = (mxArgc > 0) ? fxToNumber(the, mxArgv(0)) : 0;
	txNumber y = (mxArgc > 1) ? fxToNumber(the, mxArgv(1)) : 0;
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_fmod((c_fmod(x, y) + y), y);
}

txNumber fx_pow(txNumber x, txNumber y)
{
	if (!c_isfinite(y) && (c_fabs(x) == 1.0))
		return C_NAN;
	return c_pow(x, y);
}

void fx_Math_pow(txMachine* the)
{
	txNumber x, y;
	mxNanResultIfNoArg2;
	x = fxToNumber(the, mxArgv(0));
	y = fxToNumber(the, mxArgv(1));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = fx_pow(x, y);
}

void fx_Math_random(txMachine* the)
{
	uint32_t result = c_rand();
	while (result == C_RAND_MAX)
		result = c_rand();
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = (double)result / (double)C_RAND_MAX;
}

void fx_Math_random_secure(txMachine* the)
{
	mxTypeError("secure mode");
}

void fx_Math_round(txMachine* the)
{
    txNumber arg;
	mxNanResultIfNoArg;
	if (XS_INTEGER_KIND == mxArgv(0)->kind) {
		mxResult->kind = XS_INTEGER_KIND;
		mxResult->value.integer = mxArgv(0)->value.integer;
		return;
	}
	arg = fxToNumber(the, mxArgv(0));
	if (c_isnormal(arg) && (-4503599627370495 < arg) && (arg < 4503599627370495)) { // 2 ** 52 - 1
		if ((arg < -0.5) || (0.5 <= arg))
			arg = c_floor(arg + 0.5);
		else if (arg < 0)
			arg = -0.0;
		else if (arg > 0)
			arg = 0.0;
    }
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = arg;
	fx_Math_toInteger(the);
}

void fx_Math_sqrt(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_sqrt(mxArgv(0)->value.number);
}

void fx_Math_sign(txMachine* the)
{
	txNumber arg;
	mxNanResultIfNoArg;
	arg = fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	if (c_isnan(arg))
		mxResult->value.number = C_NAN;
	else if (arg < 0)
		mxResult->value.number = -1;
	else if (arg > 0)
		mxResult->value.number = 1;
	else
		mxResult->value.number = arg;
	fx_Math_toInteger(the);
}

void fx_Math_sin(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_sin(mxArgv(0)->value.number);
}

void fx_Math_sinh(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_sinh(mxArgv(0)->value.number);
}

void fx_Math_tan(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_tan(mxArgv(0)->value.number);
}

void fx_Math_tanh(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_tanh(mxArgv(0)->value.number);
}

void fx_Math_trunc(txMachine* the)
{
	mxNanResultIfNoArg;
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = c_trunc(mxArgv(0)->value.number);
	fx_Math_toInteger(the);
}

void fx_Math_toInteger(txMachine* the)
{
	txNumber number = mxResult->value.number;
	txInteger integer = (txInteger)number;
	txNumber check = integer;
	if ((number == check) && (number || !c_signbit(number))) {
		mxResult->value.integer = integer;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

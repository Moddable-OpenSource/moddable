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

static txSlot* fxCheckBigInt(txMachine* the, txSlot* it);

const bn_t gxBigIntOne = { .sign=0, .size=1, .data={1} };
const bn_t gxBigIntZero = { .sign=0, .size=1, .data={0} };

void fxBuildBigInt(txMachine* the)
{
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_prototype_toString), 0, mxID(_toLocaleString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_prototype_toString), 0, mxID(_toString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_BigInt_prototype_valueOf), 0, mxID(_valueOf), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "BigInt", mxID(_BigInt_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
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
		
	mxResult->kind = XS_BIGINT_KIND;
	mxResult->value.bigint = bigint;
}

void fx_BigInt_asIntN(txMachine* the)
{
	if ((mxArgc < 2)
		mxTypeError("no bigint");
	fxToBigInt(mxArgv(1));
}

void fx_BigInt_asUintN(txMachine* the)
{
	if ((mxArgc < 2)
		mxTypeError("no bigint");
	fxToBigInt(mxArgv(1));
}

void fx_BigInt_prototype_toString(txMachine* the)
{
	txSlot* slot = fxCheckSymbol(the, mxThis);
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

txBigInt fxToBigInt(txMachine* the, txSlot* theSlot)
{
again:
	switch (theSlot->kind) {
	case XS_BOOLEAN_KIND: {
		theSlot->value.bigint = bn_dup(the, (theSlot->value.boolean) ? &gxBigIntOne : &gxBigIntZero);
		theSlot->kind = XS_BIGINT_KIND;
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		theSlot->value.bigint = fxStringToBigInt(the, theSlot->value.string);
		if (bn_isNaN(theSlot->value.bigint))
			mxTypeError("Cannot coerce string to bigint");
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

txBigInt64 fxToBigInt64(txMachine* the, txSlot* theSlot)
{
	fxToBigInt(the, theSlot);
}

void fxBigIntBinary(txMachine* the, txByte code, txSlot* left, txSlot* right)
{
	if ((left->kind != XS_BIGINT_KIND) && (left->kind != XS_BIGINT_X_KIND))
		mxTypeError("Cannot coerce left operand to bigint");
	if ((right->kind != XS_BIGINT_KIND) && (right->kind != XS_BIGINT_X_KIND))
		mxTypeError("Cannot coerce right operand to bigint");
	switch (code) {
	case XS_CODE_BIT_AND:
		left->value.bigint = bn_and(the, C_NULL, left->value.bigint, right->value.bigint);
		break;
	case XS_CODE_BIT_OR:
		left->value.bigint = bn_or(the, C_NULL, left->value.bigint, right->value.bigint);
		break;
	case XS_CODE_BIT_XOR:
		left->value.bigint = bn_xor(the, C_NULL, left->value.bigint, right->value.bigint);
		break;
	case XS_CODE_LEFT_SHIFT:
		left->value.bigint = bn_lsl(the, C_NULL, left->value.bigint, ((bn_t*)(right->value.bigint))->data[0]); //@@
		break;
	case XS_CODE_SIGNED_RIGHT_SHIFT:
		left->value.bigint = bn_lsr(the, C_NULL, left->value.bigint, ((bn_t*)(right->value.bigint))->data[0]); //@@
		break;
	case XS_CODE_UNSIGNED_RIGHT_SHIFT:
		left->value.bigint = bn_lsr(the, C_NULL, left->value.bigint, ((bn_t*)(right->value.bigint))->data[0]); //@@
		break;
	case XS_CODE_ADD:
		left->value.bigint = bn_add(the, C_NULL, left->value.bigint, right->value.bigint);
		break;
	case XS_CODE_SUBTRACT:
		left->value.bigint = bn_sub(the, C_NULL, left->value.bigint, right->value.bigint);
		break;
	case XS_CODE_EXPONENTIATION:
		//@@
		break;
	case XS_CODE_MULTIPLY:
		left->value.bigint = bn_mul(the, C_NULL, left->value.bigint, right->value.bigint);
		break;
	case XS_CODE_DIVIDE:
		left->value.bigint = bn_div(the, C_NULL, left->value.bigint, right->value.bigint, C_NULL);
		break;
	case XS_CODE_MODULO:
		left->value.bigint = bn_mod(the, C_NULL, left->value.bigint, right->value.bigint);
		break;
	}
	left->kind = XS_BIGINT_KIND;
}

txInteger fxBigIntCompare(txMachine* the, txByte code, txSlot* left, txSlot* right)
{
	int fpclass;
	if (bn_isNaN(left->value.bigint))
		return 0;
	if (right->kind == XS_STRING_KIND)
		fxStringToBigInt(the, right);
	if ((right->kind == XS_BIGINT_KIND) || (right->kind == XS_BIGINT_X_KIND)) {
		if (bn_isNaN(right->value.bigint))
			return 0;
		return bn_comp(left->value.bigint, right->value.bigint);
	}
	fxToNumber(the, right);
	fpclass = c_fpclassify(right->value.number);
	if (fpclass == FP_NAN)
		return 0;
	if (fpclass == C_FP_INFINITE) {
		if ((code == XS_CODE_LESS) || (code == XS_CODE_LESS_EQUAL))
			return right->value.number > 0;
		if ((code == XS_CODE_MORE) || (code == XS_CODE_MORE_EQUAL))
			return right->value.number < 0;
		return 0;
	}
	return 0; //@@ compare bigint to number
}

void fxBigIntUnary(txMachine* the, txByte code, txSlot* left)
{
	switch (code) {
	case XS_CODE_BIT_NOT:
		//@@
		break;
	case XS_CODE_MINUS:
		left->value.bigint = bn_dup(the, left->value.bigint);
		bn_negate(left->value.bigint);
		break;
	case XS_CODE_DECREMENT:
		left->value.bigint = bn_sub(the, C_NULL, left->value.bigint, &gxBigIntOne);
		break;
	case XS_CODE_INCREMENT:
		left->value.bigint = bn_add(the, C_NULL, left->value.bigint, &gxBigIntOne);
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




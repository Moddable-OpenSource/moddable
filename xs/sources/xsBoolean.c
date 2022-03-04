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

static txSlot* fxCheckBoolean(txMachine* the, txSlot* it);

void fxBuildBoolean(txMachine* the)
{
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewBooleanInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Boolean_prototype_toString), 0, mxID(_toString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Boolean_prototype_valueOf), 0, mxID(_valueOf), XS_DONT_ENUM_FLAG);
	mxBooleanPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_Boolean), 1, mxID(_Boolean));
	mxBooleanConstructor = *the->stack;
	mxPop();
}

txSlot* fxNewBooleanInstance(txMachine* the)
{
	txSlot* instance;
	instance = fxNewObjectInstance(the);
	fxNextBooleanProperty(the, instance, 0, XS_NO_ID, XS_INTERNAL_FLAG);
	return instance;
}

void fx_Boolean(txMachine* the)
{
	txBoolean value = (mxArgc > 0) ? fxToBoolean(the, mxArgv(0)) : 0;
	if (mxIsUndefined(mxTarget)) {
		mxResult->kind = XS_BOOLEAN_KIND;
		mxResult->value.boolean = value;
	}
	else {
		txSlot* instance;
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxBooleanPrototype);
		instance = fxNewBooleanInstance(the);
		instance->next->value.boolean = value;
		mxPullSlot(mxResult);
	}
}

void fx_Boolean_prototype_toString(txMachine* the)
{
	txSlot* slot = fxCheckBoolean(the, mxThis);
	if (!slot) mxTypeError("this is no boolean");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
	fxToString(the, mxResult);
}

void fx_Boolean_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxCheckBoolean(the, mxThis);
	if (!slot) mxTypeError("this is no boolean");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

txSlot* fxCheckBoolean(txMachine* the, txSlot* it)
{
	txSlot* result = C_NULL;
	if (it->kind == XS_BOOLEAN_KIND)
		result = it;
	else if (it->kind == XS_REFERENCE_KIND) {
		txSlot* instance = it->value.reference;
		it = instance->next;
		if ((it) && (it->flag & XS_INTERNAL_FLAG) && (it->kind == XS_BOOLEAN_KIND))
			result = it;
	}
	return result;
}

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

static void fx_Error_aux(txMachine* the, txSlot* prototype);

void fxBuildError(txMachine* the)
{
	txSlot* slot;
	txSlot* prototype;
	txSlot* instance;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Error_toString), 0, mxID(_toString), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Error", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxErrorPrototype = *the->stack;
	prototype = fxNewHostConstructorGlobal(the, mxCallback(fx_Error), 1, mxID(_Error), XS_DONT_ENUM_FLAG);
	the->stack++;
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "EvalError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxEvalErrorPrototype = *the->stack;
	instance = fxNewHostConstructorGlobal(the, mxCallback(fx_EvalError), 1, mxID(_EvalError), XS_DONT_ENUM_FLAG);
	instance->value.instance.prototype = prototype;
	the->stack++;
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "RangeError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxRangeErrorPrototype = *the->stack;
	instance = fxNewHostConstructorGlobal(the, mxCallback(fx_RangeError), 1, mxID(_RangeError), XS_DONT_ENUM_FLAG);
	instance->value.instance.prototype = prototype;
	the->stack++;
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "ReferenceError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxReferenceErrorPrototype = *the->stack;
	instance = fxNewHostConstructorGlobal(the, mxCallback(fx_ReferenceError), 1, mxID(_ReferenceError), XS_DONT_ENUM_FLAG);
	instance->value.instance.prototype = prototype;
	the->stack++;
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "SyntaxError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxSyntaxErrorPrototype = *the->stack;
	instance = fxNewHostConstructorGlobal(the, mxCallback(fx_SyntaxError), 1, mxID(_SyntaxError), XS_DONT_ENUM_FLAG);
	instance->value.instance.prototype = prototype;
	the->stack++;
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "TypeError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxTypeErrorPrototype = *the->stack;
	instance = fxNewHostConstructorGlobal(the, mxCallback(fx_TypeError), 1, mxID(_TypeError), XS_DONT_ENUM_FLAG);
	instance->value.instance.prototype = prototype;
	the->stack++;
	mxPush(mxErrorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "URIError", mxID(_name), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "", mxID(_message), XS_DONT_ENUM_FLAG);
	mxURIErrorPrototype = *the->stack;
	instance = fxNewHostConstructorGlobal(the, mxCallback(fx_URIError), 1, mxID(_URIError), XS_DONT_ENUM_FLAG);
	instance->value.instance.prototype = prototype;
	the->stack++;
}

void fx_Error(txMachine* the)
{
	fx_Error_aux(the, &mxErrorPrototype);
}

void fx_Error_aux(txMachine* the, txSlot* prototype)
{
	txSlot* slot;
	if (mxIsUndefined(mxTarget))
		mxPushSlot(mxFunction);
	else
		mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, prototype);
	slot = fxNewObjectInstance(the);
	mxPullSlot(mxResult);
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG | XS_GET_ONLY;
	slot->kind = XS_ERROR_KIND;
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND))
		slot = fxNextSlotProperty(the, slot, mxArgv(0), mxID(_message), XS_DONT_ENUM_FLAG);
}

void fx_Error_toString(txMachine* the)
{
	txSlot* name;
	txSlot* message;
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_name));
	if (the->stack->kind == XS_UNDEFINED_KIND) 
		fxStringX(the, the->stack, "Error");
	else	
		fxToString(the, the->stack);
	name = the->stack;
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_message));
	if (the->stack->kind == XS_UNDEFINED_KIND) 
		*the->stack = mxEmptyString;
	else	
		fxToString(the, the->stack);
	message = the->stack;
	if (c_isEmpty(name->value.string))
		*mxResult = *message;
	else if (c_isEmpty(message->value.string))
		*mxResult = *name;
	else {
		fxStringX(the, mxResult, "");
		fxConcatString(the, mxResult, name);
		fxConcatStringC(the, mxResult, ": ");
		fxConcatString(the, mxResult, message);
	}
	the->stack += 2;
}

void fx_EvalError(txMachine* the)
{
	fx_Error_aux(the, &mxEvalErrorPrototype);
}

void fx_RangeError(txMachine* the)
{
	fx_Error_aux(the, &mxRangeErrorPrototype);
}

void fx_ReferenceError(txMachine* the)
{
	fx_Error_aux(the, &mxReferenceErrorPrototype);
}

void fx_SyntaxError(txMachine* the)
{
	fx_Error_aux(the, &mxSyntaxErrorPrototype);
}

void fx_TypeError(txMachine* the)
{
	fx_Error_aux(the, &mxTypeErrorPrototype);
}

void fx_URIError(txMachine* the)
{
	fx_Error_aux(the, &mxURIErrorPrototype);
}


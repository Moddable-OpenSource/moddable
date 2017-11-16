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

static txSlot* fxCheckGeneratorInstance(txMachine* the, txSlot* slot);

void fxBuildGenerator(txMachine* the)
{
	txSlot* slot;
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Generator_prototype_next), 1, mxID(_next), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Generator_prototype_return), 1, mxID(_return), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Generator_prototype_throw), 1, mxID(_throw), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Generator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxGeneratorPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructor(the, mxCallback(fx_Generator), 1, XS_NO_ID));
	slot = fxNextStringXProperty(the, slot, "GeneratorFunction", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxGeneratorFunctionPrototype = *the->stack;
	slot = fxNewHostConstructor(the, mxCallback(fx_GeneratorFunction), 1, mxID(_GeneratorFunction));
	the->stack++;
	
	slot = mxBehaviorGetProperty(the, mxGeneratorPrototype.value.reference, mxID(_constructor), XS_NO_ID, XS_OWN);
	slot->flag |= XS_DONT_SET_FLAG;
	slot = mxBehaviorGetProperty(the, mxGeneratorFunctionPrototype.value.reference, mxID(_constructor), XS_NO_ID, XS_OWN);
	slot->flag |= XS_DONT_SET_FLAG;
	slot = mxBehaviorGetProperty(the, mxGeneratorFunctionPrototype.value.reference, mxID(_prototype), XS_NO_ID, XS_OWN);
	slot->flag &= ~XS_DONT_DELETE_FLAG;
}

txSlot* fxCheckGeneratorInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		slot = instance->next;
		if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_STACK_KIND))
			return instance;
	}
	mxTypeError("this is no Generator instance");
	return C_NULL;
}

txSlot* fxNewGeneratorInstance(txMachine* the)
{
	txSlot* prototype;
	txSlot* instance;
	txSlot* property;
	txSlot* result;
	txSlot* slot;

	prototype = (the->stack->kind == XS_REFERENCE_KIND) ? the->stack->value.reference : mxGeneratorPrototype.value.reference;

	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = prototype;
	the->stack->value.reference = instance;
	the->stack->kind = XS_REFERENCE_KIND;

	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_STACK_KIND;
	property->ID = XS_NO_ID;
    property->value.stack.length = 0;
    property->value.stack.address = C_NULL;
	if (prototype) {
		slot = prototype->next;
		if (slot && (slot->kind == XS_STACK_KIND)) {
			property->value.stack.length = slot->value.stack.length;
			property->value.stack.address = slot->value.stack.address;
		}
	}
	result = fxNewInstance(the);
	result->value.instance.prototype = mxObjectPrototype.value.reference;
	slot = result->next = fxNewSlot(the);
	slot->flag = XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
	slot->kind = XS_UNDEFINED_KIND;
	slot->ID = mxID(_value);
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
	slot->kind = XS_BOOLEAN_KIND;
	slot->ID = mxID(_done);
	slot->value.boolean = 0;
    
    property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_REFERENCE_KIND;
	property->value.reference = the->stack->value.reference;
	
    property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_INTEGER_KIND;
	property->value.integer = -1;
	the->stack++;
	
	return instance;
}

void fx_Generator(txMachine* the)
{
	if (mxTarget->kind != XS_UNDEFINED_KIND)
		mxTypeError("new Generator");
}

void fx_Generator_prototype_aux(txMachine* the, txFlag status)
{
	txSlot* generator = fxCheckGeneratorInstance(the, mxThis);
	txSlot* slot = generator->next->next;
	txSlot* result = slot->value.reference;
	txSlot* value = result->next;
	txSlot* done = value->next;
	txSlot* state = slot->next;
	
	if (state->value.integer > 0)
		mxTypeError("generator is running");
	
	if (mxArgc > 0) {
		the->scratch.kind = mxArgv(0)->kind;
		the->scratch.value = mxArgv(0)->value;
	}
	else
		the->scratch.kind = XS_UNDEFINED_KIND;
	mxResult->kind = XS_REFERENCE_KIND;
	mxResult->value.reference = result;
	
	if ((status != XS_NO_STATUS) && (state->value.integer < 0)) {
		done->value.boolean = 1;
		state->value.integer = 0;
	}
	if (done->value.boolean) {
		if (status == XS_NO_STATUS)
			value->kind = XS_UNDEFINED_KIND;
		else if (status == XS_RETURN_STATUS) {
			value->kind = the->scratch.kind;
			value->value = the->scratch.value;
		}
		else {
			mxException.kind = the->scratch.kind;
			mxException.value = the->scratch.value;
			fxJump(the);
		}
	}
	else {
		mxTry(the) {
			the->status = status;
			state->value.integer = 1;
			fxRunID(the, generator, XS_NO_ID);
			if (state->value.integer) {
				state->value.integer = 0;
				done->value.boolean = 1;
				mxPullSlot(value);
			}
			else
				mxPullSlot(mxResult);
		}
		mxCatch(the) {
			state->value.integer = 0;
			done->value.boolean = 1;
			fxJump(the);
		}
	}
}

void fx_Generator_prototype_next(txMachine* the)
{
	fx_Generator_prototype_aux(the, XS_NO_STATUS);
}

void fx_Generator_prototype_return(txMachine* the)
{
	fx_Generator_prototype_aux(the, XS_RETURN_STATUS);
}

void fx_Generator_prototype_throw(txMachine* the)
{
	fx_Generator_prototype_aux(the, XS_THROW_STATUS);
}

txSlot* fxNewGeneratorFunctionInstance(txMachine* the, txID name)
{
	txSlot* instance;
	txSlot* property;

	instance = fxNewFunctionInstance(the, name);
	property = fxLastProperty(the, instance);
	mxPush(mxGeneratorPrototype);
	fxNewObjectInstance(the);
	fxNextSlotProperty(the, property, the->stack, mxID(_prototype), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	the->stack++;
	
	return instance;
}

void fx_GeneratorFunction(txMachine* the)
{	
#ifdef mxParse
	txInteger c, i;
	txStringStream stream;
	
	c = mxArgc;
	i = 0;
	mxPushStringX("(function* anonymous(");
	while (c > 1) {
		fxToString(the, mxArgv(i));
		fxConcatString(the, the->stack, mxArgv(i));
		if (c > 2)
			fxConcatStringC(the, the->stack, ", ");
		c--;
		i++;
	}
	fxConcatStringC(the, the->stack, "){");
	if (c > 0) {
		fxToString(the, mxArgv(i));
		fxConcatString(the, the->stack, mxArgv(i));
	}
	fxConcatStringC(the, the->stack, "})");
	stream.slot = the->stack;
	stream.offset = 0;
	stream.size = c_strlen(the->stack->value.string);
	fxRunScript(the, fxParseScript(the, &stream, fxStringGetter, mxProgramFlag), C_NULL, C_NULL, C_NULL, C_NULL, C_NULL);
	mxPullSlot(mxResult);
	if (!mxIsUndefined(mxTarget) && !fxIsSameSlot(the, mxTarget, mxFunction)) {
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxGeneratorFunctionPrototype);
		mxResult->value.reference->value.instance.prototype = the->stack->value.reference;
		mxPop();
	}
#else
	mxUnknownError("not built-in");
#endif
}







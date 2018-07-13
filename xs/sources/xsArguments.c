#include "xsAll.h"

static txBoolean fxArgumentsSloppyDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* descriptor, txFlag mask);
static txBoolean fxArgumentsSloppyDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static txSlot* fxArgumentsSloppyGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
static txSlot* fxArgumentsSloppySetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);

const txBehavior ICACHE_FLASH_ATTR gxArgumentsSloppyBehavior = {
	fxArgumentsSloppyGetProperty,
	fxArgumentsSloppySetProperty,
	fxOrdinaryCall,
	fxOrdinaryConstruct,
	fxArgumentsSloppyDefineOwnProperty,
	fxArgumentsSloppyDeleteProperty,
	fxOrdinaryGetOwnProperty,
	fxOrdinaryGetPropertyValue,
	fxOrdinaryGetPrototype,
	fxOrdinaryHasProperty,
	fxOrdinaryIsExtensible,
	fxOrdinaryOwnKeys,
	fxOrdinaryPreventExtensions,
	fxOrdinarySetPropertyValue,
	fxOrdinarySetPrototype,
};

void fxBuildArguments(txMachine* the)
{
	mxPush(mxObjectPrototype);
	mxArgumentsSloppyPrototype = *the->stack;
	the->stack++;
	
	mxPush(mxObjectPrototype);
	mxArgumentsStrictPrototype = *the->stack;
	the->stack++;
}

txSlot* fxNewArgumentsSloppyInstance(txMachine* the, txIndex count)
{
	txSlot* instance;
	txSlot* array;
	txSlot* property;
	txIndex index;
	txSlot* address;
	txIndex length = (txIndex)mxArgc;
	txByte* code = the->code;
	the->code = C_NULL;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_EXOTIC_FLAG;
	array = instance->next = fxNewSlot(the);
	array->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	array->ID = XS_ARGUMENTS_SLOPPY_BEHAVIOR;
	array->kind = XS_ARRAY_KIND;
	array->value.array.length = 0;
	array->value.array.address = C_NULL;
	property = fxNextNumberProperty(the, array, length, mxID(_length), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, mxFunction, mxID(_callee), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, &mxArrayIteratorFunction, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	the->code = code;
	fxSetIndexSize(the, array, length);
	index = 0;
	address = array->value.array.address;
	property = the->scope + count;
	while ((index < length) && (index < count)) {
		*((txIndex*)address) = index;
		address->ID = XS_NO_ID;
		if (property->kind == XS_CLOSURE_KIND) {
			address->kind = property->kind;
			address->value = property->value;
		}
		else {
			txSlot* argument = mxArgv(index);
			address->kind = argument->kind;
			address->value = argument->value;
		}
		index++;
		address++;
		property--;
	}
	while (index < length) {
		property = mxArgv(index);
		*((txIndex*)address) = index;
		address->ID = XS_NO_ID;
		address->kind = property->kind;
		address->value = property->value;
		index++;
		address++;
	}
	return instance;
}

txBoolean fxArgumentsSloppyDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* descriptor, txFlag mask) 
{
	if (!id) {
		txSlot* property = fxGetIndexProperty(the, instance->next, index);
		if (property && (property->kind == XS_CLOSURE_KIND)) {
			txSlot* closure = property->value.closure;
			if (mask & XS_ACCESSOR_FLAG) {
				property->flag = closure->flag;
				property->kind = closure->kind;
				property->value = closure->value;
			}
			else if ((descriptor->flag & XS_DONT_SET_FLAG) && (mask & XS_DONT_SET_FLAG)) {
				property->flag = closure->flag;
				property->kind = closure->kind;
				property->value = closure->value;
				if (descriptor->kind != XS_UNINITIALIZED_KIND)
					closure->value = descriptor->value;
			}
		}
	}
	return fxOrdinaryDefineOwnProperty(the, instance, id, index, descriptor, mask);
}

txBoolean fxArgumentsSloppyDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	if (!id) {
		txSlot* property = fxGetIndexProperty(the, instance->next, index);
		if (property && (property->kind == XS_CLOSURE_KIND)) {
			if (property->value.closure->flag & XS_DONT_DELETE_FLAG)
				return 0;
		}
	}
	return fxOrdinaryDeleteProperty(the, instance, id, index);
}

txSlot* fxArgumentsSloppyGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	txSlot* result = fxOrdinaryGetProperty(the, instance, id, index, flag);
	if (!id && result && result->kind == XS_CLOSURE_KIND)
		result = result->value.closure;
	return result;
}

txSlot* fxArgumentsSloppySetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	txSlot* result = fxOrdinarySetProperty(the, instance, id, index, flag);
	if (!id && result && result->kind == XS_CLOSURE_KIND)
		result = result->value.closure;
	return result;
}

txSlot* fxNewArgumentsStrictInstance(txMachine* the, txIndex count)
{
	txSlot* instance;
	txSlot* array;
	txSlot* property;
	txSlot* function;
	txIndex index;
	txSlot* address;
	txIndex length = (txIndex)mxArgc;
	txByte* code = the->code;
	the->code = C_NULL;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_EXOTIC_FLAG;
	array = instance->next = fxNewSlot(the);
	array->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	array->ID = XS_ARGUMENTS_STRICT_BEHAVIOR;
	array->kind = XS_ARRAY_KIND;
	array->value.array.length = 0;
	array->value.array.address = C_NULL;
	property = fxNextNumberProperty(the, array, length, mxID(_length), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, &mxArrayIteratorFunction, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	function = mxThrowTypeErrorFunction.value.reference;
	property = property->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	property->ID = mxID(_callee);
	property->kind = XS_ACCESSOR_KIND;
	property->value.accessor.getter = function;
	property->value.accessor.setter = function;
	the->code = code;
	fxSetIndexSize(the, array, length);
	index = 0;
	address = array->value.array.address;
	while (index < length) {
		property = mxArgv(index);
		*((txIndex*)address) = index;
		address->ID = XS_NO_ID;
		address->kind = property->kind;
		address->value = property->value;
		index++;
		address++;
	}
	return instance;
}

void fxThrowTypeError(txMachine* the)
{
	mxTypeError("strict mode");
}
#include "xsAll.h"		//@@ add copyright notice!
#include "xsScript.h"

static txBoolean fx_deepEqualArrays(txMachine* the, txBoolean strict);
static txBoolean fx_deepEqualInstances(txMachine* the, txBoolean strict);
static txBoolean fx_deepEqualSlots(txMachine* the, txBoolean strict);

void fx_deepEqual(txMachine* the)
{
	txBoolean strict = 0;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	if (mxArgc > 1)
		mxPushSlot(mxArgv(1));
	else
		mxPushUndefined();
	if (mxArgc > 2)
		strict = fxToBoolean(the, mxArgv(2));
	mxResult->value.boolean = fx_deepEqualSlots(the, strict);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxPop();
	mxPop();
}

txBoolean fx_deepEqualArrays(txMachine* the, txBoolean strict)
{
	txSlot* leftArray = the->stack + 1;
	txSlot* rightArray = the->stack;
	if (leftArray->value.array.length != rightArray->value.array.length)
		return 0;
	txIndex leftSize = (((txChunk*)(((txByte*)(leftArray->value.array.address)) - sizeof(txChunk)))->size) / sizeof(txSlot);
	txIndex rightSize = (((txChunk*)(((txByte*)(rightArray->value.array.address)) - sizeof(txChunk)))->size) / sizeof(txSlot);
	if (leftSize != rightSize)
		return 0;
	txIndex i = 0;
	while (i < leftSize) {
		txSlot* leftSlot = leftArray->value.array.address + i;
		txSlot* rightSlot = rightArray->value.array.address + i;
		txIndex leftIndex = *((txIndex*)leftSlot);
		txIndex rightIndex = *((txIndex*)rightSlot);
		if (leftIndex != rightIndex)
			return 0;
		mxPushSlot(leftSlot);
		mxPushSlot(rightSlot);
		if (!fx_deepEqualSlots(the, strict))
			return 0;
		mxPop();
		mxPop();
		i++;
	}
	return 1;
}

txBoolean fx_deepEqualInstances(txMachine* the, txBoolean strict)
{
	txSlot* leftInstance = (the->stack + 1)->value.reference;
	txSlot* leftProperty;
	txSlot* leftBase;
	txSlot* leftArray;
	txIndex leftCount;
	txSlot* rightInstance = the->stack->value.reference;
	txSlot* rightProperty;
	txSlot* rightBase;
	txSlot* rightArray;
	txIndex rightCount;
	
	if (strict && (leftInstance->value.instance.prototype != rightInstance->value.instance.prototype))
		return 0;
	// wrappers
	leftProperty = leftInstance->next;
	leftBase = C_NULL;
	if (leftProperty) {
		if (leftProperty->flag & XS_INTERNAL_FLAG) {
			if ((leftProperty->kind == XS_BOOLEAN_KIND)
			 || (leftProperty->kind == XS_NUMBER_KIND) 
			 || (leftProperty->kind == XS_STRING_KIND) 
			 || (leftProperty->kind == XS_STRING_X_KIND) 
			 || (leftProperty->kind == XS_SYMBOL_KIND)) {
				leftBase = leftProperty;
			}
		}
	}
	rightProperty = rightInstance->next;
	rightBase = C_NULL;
	if (rightProperty) {
		if (rightProperty->flag & XS_INTERNAL_FLAG) {
			if ((rightProperty->kind == XS_BOOLEAN_KIND)
			 || (rightProperty->kind == XS_NUMBER_KIND) 
			 || (rightProperty->kind == XS_STRING_KIND) 
			 || (rightProperty->kind == XS_STRING_X_KIND) 
			 || (rightProperty->kind == XS_SYMBOL_KIND)) {
				rightBase = rightProperty;
			}
		}
	}
	if (leftBase) {
		if (rightBase) {
			if (leftBase->kind != rightBase->kind)
				return 0;
			mxPushSlot(leftBase);
			mxPushSlot(rightBase);
			if (!fx_deepEqualSlots(the, strict))
				return 0;
			mxPop();
			mxPop();
		}
		else
			return 0;
	}
	else if (rightBase)
		return 0;
	
	// indexed properties
	leftProperty = leftInstance->next;
	leftArray = C_NULL;
	while (leftProperty && (leftProperty->flag & XS_INTERNAL_FLAG)) {
		if (leftProperty->kind == XS_ARRAY_KIND)
			leftArray = leftProperty;
		leftProperty = leftProperty->next;
	}
	leftBase = leftProperty;
	rightProperty = rightInstance->next;
	rightArray = C_NULL;
	while (rightProperty && (rightProperty->flag & XS_INTERNAL_FLAG)) {
		if (rightProperty->kind == XS_ARRAY_KIND)
			rightArray = rightProperty;
		rightProperty = rightProperty->next;
	}
	rightBase = rightProperty;
	if (leftArray) {
		if (rightArray) {
			mxPushSlot(leftArray);
			mxPushSlot(rightArray);
			if (!fx_deepEqualArrays(the, strict))
				return 0;
			mxPop();
			mxPop();
		}
		else
			return 0;
	}
	else if (rightArray)
		return 0;
	// named and symbol properties
	leftCount = 0;
	leftProperty = leftBase;
	while (leftProperty) {
		if (!(leftProperty->flag & XS_DONT_ENUM_FLAG))
			leftCount++;
		leftProperty = leftProperty->next;
	}
	rightCount = 0;
	rightProperty = rightBase;
	while (rightProperty) {
		if (!(rightProperty->flag & XS_DONT_ENUM_FLAG))
			rightCount++;
		rightProperty = rightProperty->next;
	}
	if (leftCount != rightCount)
		return 0;
	leftProperty = leftBase;
	while (leftProperty) {
		if (!(leftProperty->flag & XS_DONT_ENUM_FLAG)) {
			rightProperty = rightBase;
			while (rightProperty) {
				if (!(rightProperty->flag & XS_DONT_ENUM_FLAG)) {
					if (leftProperty->ID == rightProperty->ID) {
						mxPushSlot(leftProperty);
						mxPushSlot(rightProperty);
						if (!fx_deepEqualSlots(the, strict))
							return 0;
						mxPop();
						mxPop();
						break;
					}
				}
				rightProperty = rightProperty->next;
			}
			if (!rightProperty)
				return 0;
		}
		leftProperty = leftProperty->next;
	}
	return 1;
}

txBoolean fx_deepEqualSlots(txMachine* the, txBoolean strict)
{
	txSlot* left = the->stack + 1;
	txSlot* right = the->stack;
	txBoolean result = 0;
again:
	if (left->kind == right->kind) {
		if ((XS_UNDEFINED_KIND == left->kind) || (XS_NULL_KIND == left->kind))
			result = 1;
		else if (XS_BOOLEAN_KIND == left->kind)
			result = left->value.boolean == right->value.boolean;
		else if (XS_INTEGER_KIND == left->kind)
			result = left->value.integer == right->value.integer;
		else if (XS_NUMBER_KIND == left->kind) {
			result = ((c_isnan(left->value.number) && c_isnan(right->value.number)) || ((left->value.number == right->value.number) && (c_signbit(left->value.number) == c_signbit(right->value.number))));
			mxFloatingPointOp("fx_deepEqual");
		}
		else if ((XS_STRING_KIND == left->kind) || (XS_STRING_X_KIND == left->kind))
			result = c_strcmp(left->value.string, right->value.string) == 0;
		else if (XS_SYMBOL_KIND == left->kind)
			result = left->value.symbol == right->value.symbol;
		else if (XS_REFERENCE_KIND == left->kind) {
			if (fxIsSameReference(the, left, right))
				result = 1;
			else
				result = fx_deepEqualInstances(the, strict);
		}
	#ifdef mxHostFunctionPrimitive
		else if (XS_HOST_FUNCTION_KIND == left->kind)
			result = left->value.hostFunction.builder == right->value.hostFunction.builder;
	#endif
		else if ((XS_BIGINT_KIND == left->kind) || (XS_BIGINT_X_KIND == left->kind))
			result = gxTypeBigInt.compare(the, 0, 1, 0, left, right);
		else
			result = 0;
	}
	else if ((XS_INTEGER_KIND == left->kind) && (XS_NUMBER_KIND == right->kind)) {
		if (strict) {
			txNumber aNumber = left->value.integer;
			result = (aNumber == right->value.number) && (signbit(aNumber) == signbit(right->value.number));
		}
		else
			result = (!c_isnan(right->value.number)) && ((txNumber)(left->value.integer) == right->value.number);
		mxFloatingPointOp("fx_deepEqual");
	}
	else if ((XS_NUMBER_KIND == left->kind) && (XS_INTEGER_KIND == right->kind)) {
		if (strict) {
			txNumber bNumber = right->value.integer;
			result = (left->value.number == bNumber) && (signbit(left->value.number) == signbit(bNumber));
		}
		else
			result = (!c_isnan(left->value.number)) && (left->value.number == (txNumber)(right->value.integer));
		mxFloatingPointOp("fx_deepEqual");
	}
	else if ((XS_STRING_KIND == left->kind) && (XS_STRING_X_KIND == right->kind))
		result = c_strcmp(left->value.string, right->value.string) == 0;
	else if ((XS_STRING_X_KIND == left->kind) && (XS_STRING_KIND == right->kind))
		result = c_strcmp(left->value.string, right->value.string) == 0;
	else if (!strict) {
		if ((left->kind == XS_UNDEFINED_KIND) && (right->kind == XS_NULL_KIND))
			result = 1;
		else if ((left->kind == XS_NULL_KIND) && (right->kind == XS_UNDEFINED_KIND))
			result = 1;
		else if (((XS_INTEGER_KIND == left->kind) || (XS_NUMBER_KIND == left->kind)) && ((right->kind == XS_STRING_KIND) || (right->kind == XS_STRING_X_KIND))) {
			fxToNumber(the, right); 
			goto again;
		}
		else if (((left->kind == XS_STRING_KIND) || (left->kind == XS_STRING_X_KIND)) && ((XS_INTEGER_KIND == right->kind) || (XS_NUMBER_KIND == right->kind))) {
			fxToNumber(the, left);
			goto again;
		}
		else if (XS_BOOLEAN_KIND == left->kind) {
			fxToNumber(the, left);
			goto again;
		}
		else if (XS_BOOLEAN_KIND == right->kind) {
			fxToNumber(the, right);
			goto again;
		}
		else if (((XS_BIGINT_KIND == left->kind) || (XS_BIGINT_X_KIND == left->kind)) && ((right->kind == XS_INTEGER_KIND) || (right->kind == XS_NUMBER_KIND) || (right->kind == XS_STRING_KIND) || (right->kind == XS_STRING_X_KIND))) {
			result = gxTypeBigInt.compare(the, 0, 1, 0, left, right);
		}
		else if (((left->kind == XS_INTEGER_KIND) || (left->kind == XS_NUMBER_KIND) || (left->kind == XS_STRING_KIND) || (left->kind == XS_STRING_X_KIND)) && ((XS_BIGINT_KIND == right->kind) || (XS_BIGINT_X_KIND == right->kind))) {
			result = gxTypeBigInt.compare(the, 0, 1, 0, right, left);
		}
		else
			result = 0;
	}
	else 
		result = 0;
	return result;
}


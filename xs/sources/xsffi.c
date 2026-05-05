#include "xsAll.h"
#include "xsffi.h"

extern txInteger fxArgc(txMachine*);

txSlot* fxArgv(txMachine* the, txInteger index) {
	return the->frame - 1 - fxCheckArg(the, index);
}

void fxPop(txMachine* the)
{
	mxPop();
}

void fxPush(txMachine* the, txSlot* slot)
{
	mxPushSlot(slot);
}

txSlot* fxResult(txMachine* the)
{
	return mxResult;
}

txSlot* fxThis(txMachine* the)
{
	return mxThis;
}

void** fxToArrayBufferHandle(txMachine* the, txSlot* slot, size_t size)
{
	fxToArrayBuffer(the, slot);
	txSlot* arrayBuffer = slot->value.reference->next;
	if (size) {
		txSlot* bufferInfo = arrayBuffer->next;
		if (arrayBuffer->value.arrayBuffer.address == C_NULL)
			mxTypeError("detached buffer");
		if (bufferInfo->value.bufferInfo.length < (txSize)size)
			mxRangeError("invalid buffer size %ld", bufferInfo->value.bufferInfo.length);
	}
	return (void**)&(arrayBuffer->value.arrayBuffer.address);
}

char** fxToStringHandle(txMachine* the, txSlot* slot)
{
	fxToString(the, slot);
	return &(slot->value.string);
}

extern txAPI gxAPI;
txAPI gxAPI = {
	fxThis,
	fxArgc,
	fxArgv,
	fxPop,
	fxPush,
	fxResult,

	fxAbort,
	fxDefineID,
	fxID,
	fxNewHostFunction,
	
	fxFromBigInt64,
	fxFromBigUint64,
	fxInteger,
	fxNumber,
	fxUnsigned,
	
	fxToBigInt64,
	fxToBigUint64,
	fxToInteger,
	fxToNumber,
	fxToUnsigned,
	
	fxString,
	fxStringX,
	fxToStringHandle,
	
	fxArrayBuffer,
	fxToArrayBufferHandle,
};
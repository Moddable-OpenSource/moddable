#include "xsAll.h"
#include "xsffi.h"

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

void** fxToArrayBufferHandle(txMachine* the, txSlot* slot)
{
	fxToArrayBuffer(the, slot);
	return (void**)&(slot->value.reference->next->value.arrayBuffer.address);
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
	fxDefineID,
	fxID,
	fxInteger,
	fxNewHostFunction,
	fxNumber,
	fxPop,
	fxPush,
	fxResult,
	fxString,
	fxStringX,
	fxToArrayBufferHandle,
	fxToInteger,
	fxToNumber,
	fxToStringHandle,
	fxToUnsigned,
	fxUnsigned,
};
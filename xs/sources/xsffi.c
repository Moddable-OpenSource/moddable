#include "xsAll.h"
#include "xsffi.h"

extern txInteger fxArgc(txMachine*);

#if pebble
	#include "syscall/syscall_internal.h"
#else
	#define DEFINE_SYSCALL(RETURN, NAME, ...) RETURN NAME(__VA_ARGS__)
#endif

DEFINE_SYSCALL(txSlot*, fxArgv, txMachine* the, txInteger index) {
	return the->frame - 1 - fxCheckArg(the, index);
}

DEFINE_SYSCALL(void, fxPop, txMachine* the) {
	mxPop();
}

DEFINE_SYSCALL(void, fxPush, txMachine* the, txSlot* slot) {
	mxPushSlot(slot);
}

DEFINE_SYSCALL(txSlot*, fxResult, txMachine* the) {
	return mxResult;
}

DEFINE_SYSCALL(txSlot*, fxThis, txMachine* the) {
	return mxThis;
}

DEFINE_SYSCALL(void**, fxToArrayBufferHandle, txMachine* the, txSlot* slot, size_t size) {
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

DEFINE_SYSCALL(char**, fxToStringHandle, txMachine* the, txSlot* slot) {
	fxToString(the, slot);
	return &(slot->value.string);
}

extern txAPI gxAPI;
#if !pebble
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
#else

DEFINE_SYSCALL(txInteger, fxArgc_, txMachine* the) {
	return fxArgc(the);
}

DEFINE_SYSCALL(void, fxAbort_, txMachine* the, int status) {
	fxAbort(the, status);
}

DEFINE_SYSCALL(void, fxDefineID_, txMachine* the, txID id, txFlag flag, txFlag mask) {
	fxDefineID(the, id, flag, mask);
}

DEFINE_SYSCALL(txID, fxID_, txMachine* the, txString theString) {
	return fxID(the, theString);
}

DEFINE_SYSCALL(txSlot*, fxNewHostFunction_, txMachine* the, txCallback callback, txInteger length, txInteger id, txInteger profileID) {
	return fxNewHostFunction(the, callback, length, id, profileID);
}

DEFINE_SYSCALL(void, fxFromBigInt64_, txMachine* the, txSlot* slot, txS8 value) {
	fxFromBigInt64(the, slot, value);
}

DEFINE_SYSCALL(void, fxFromBigUint64_, txMachine* the, txSlot* slot, txU8 value) {
	fxFromBigUint64(the, slot, value);
}

DEFINE_SYSCALL(void, fxInteger_, txMachine* the, txSlot* slot, txInteger value) {
	fxInteger(the, slot, value);
}

DEFINE_SYSCALL(void, fxNumber_, txMachine* the, txSlot* slot, txNumber value) {
	fxNumber(the, slot, value);
}

DEFINE_SYSCALL(void, fxUnsigned_, txMachine* the, txSlot* slot, txUnsigned value) {
	fxUnsigned(the, slot, value);
}

DEFINE_SYSCALL(txS8, fxToBigInt64_, txMachine* the, txSlot* slot) {
	return fxToBigInt64(the, slot);
}

DEFINE_SYSCALL(txU8, fxToBigUint64_, txMachine* the, txSlot* slot) {
	return fxToBigUint64(the, slot);
}

DEFINE_SYSCALL(txInteger, fxToInteger_, txMachine* the, txSlot* slot) {
	return fxToInteger(the, slot);
}

DEFINE_SYSCALL(txNumber, fxToNumber_, txMachine* the, txSlot* slot) {
	return fxToNumber(the, slot);
}

DEFINE_SYSCALL(txUnsigned, fxToUnsigned_, txMachine* the, txSlot* slot) {
	return fxToUnsigned(the, slot);
}

DEFINE_SYSCALL(void, fxString_, txMachine* the, txSlot* slot, txString value) {
	fxString(the, slot, value);
}

DEFINE_SYSCALL(void, fxStringX_, txMachine* the, txSlot* slot, txString value) {
	fxStringX(the, slot, value);
}

DEFINE_SYSCALL(void*, fxArrayBuffer_, txMachine* the, txSlot* slot, void* data, txInteger byteLength, txInteger maxByteLength) {
	return fxArrayBuffer(the, slot, data, byteLength, maxByteLength);
}

txAPI gxAPI = {
	fxThis,
	fxArgc_,
	fxArgv,
	fxPop,
	fxPush,
	fxResult,

	fxAbort_,
	fxDefineID_,
	fxID_,
	fxNewHostFunction_,
	
	fxFromBigInt64_,
	fxFromBigUint64_,
	fxInteger_,
	fxNumber_,
	fxUnsigned_,
	
	fxToBigInt64_,
	fxToBigUint64_,
	fxToInteger_,
	fxToNumber_,
	fxToUnsigned_,
	
	fxString_,
	fxStringX_,
	fxToStringHandle,
	
	fxArrayBuffer_,
	fxToArrayBufferHandle,
};
#endif

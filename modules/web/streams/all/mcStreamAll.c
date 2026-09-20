/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
 */

#include "mcStreamAll.h"

void fxNewError(xsMachine* the, xsStringValue format, ...)
{
	char message[128] = "";
    mxNew();
    va_list arguments;
    va_start(arguments, format);
    c_vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    mxPushStringC(message);
    mxRunCount(1);
}

void fxChainPromise(xsMachine* the, xsSlot* promise, xsSlot* resolved, xsSlot* rejected, xsSlot* result)
{
	if (resolved) {
		mxPushReference(resolved);
		resolved = the->stack;
	}
	if (rejected) {
		mxPushReference(rejected);
		rejected = the->stack;
	}
	if (result) {
		txSlot* resolveFunction;
		txSlot* rejectFunction;
		mxTemporary(resolveFunction);
		mxTemporary(rejectFunction);
		mxPushSlot(promise);
		promise = the->stack;
		if (result != mxResult)
			mxPushSlot(mxResult);
		mxPush(mxPromiseConstructor);
		fxNewPromiseCapability(the, resolveFunction, rejectFunction);
		//@@ fxPromiseThen expects the new promise in mxResult
		mxPullSlot(mxResult);
		fxPromiseThen(the, promise->value.reference, resolved, rejected, resolveFunction, rejectFunction);
		if (result != mxResult) {
			*result = *mxResult;
			mxPullSlot(mxResult);
		}
		mxPop(); // promise
		mxPop(); // rejectFunction
		mxPop(); // resolveFunction
	}
	else {
		fxPromiseThen(the, promise->value.reference, resolved, rejected, NULL, NULL);
	}
	if (rejected)
		mxPop();
	if (resolved)
		mxPop();
}

xsSlot* fxCreatePromiseRecord(xsMachine* the, xsSlot* record)
{
	mxPush(mxPromiseConstructor);
	mxDub();
	mxGetID(xsID_withResolvers);
	mxCall();
	mxRunCount(0);
	xsSlot* instance = the->stack->value.reference;
	if (record)
		mxPullSlot(record);
	else
		mxPop();
	return instance;
}

void fxCreateRejectedPromise(xsMachine* the, xsSlot* reason, xsSlot* promise)
{
	mxPush(mxPromiseConstructor);
	mxDub();
	mxGetID(xsID_reject);
	mxCall();
	if (reason) {
		if (reason->kind == XS_INSTANCE_KIND)
			mxPushReference(reason);
		else
			mxPushSlot(reason);
	}
	else
		mxPushNull();
	mxRunCount(1);
	mxPullSlot(promise);
}

xsSlot* fxCreateRejectedPromiseRecord(xsMachine* the, xsSlot* reason, xsSlot* record)
{
	mxPush(mxObjectPrototype);
	xsSlot* instance = fxNewObjectInstance(the);
	xsSlot* slot = fxLastProperty(the, instance);
	xsSlot* promise;
	mxTemporary(promise);
	fxCreateRejectedPromise(the, reason, promise);
	slot = fxNextSlotProperty(the, slot, promise, xsID_promise, XS_NO_FLAG);
	mxPop(); // promise
	if (record)
		mxPullSlot(record);
	else
		mxPop();
	return instance;
}

void fxCreateResolvedPromise(xsMachine* the, xsSlot* value, xsSlot* promise)
{
	mxPush(mxPromiseConstructor);
	mxDub();
	mxGetID(xsID_resolve);
	mxCall();
	if (value) {
		if (value->kind == XS_INSTANCE_KIND)
			mxPushReference(value);
		else
			mxPushSlot(value);
	}
	else
		mxPushNull();
	mxRunCount(1);
	mxPullSlot(promise);
}

xsSlot* fxCreateResolvedPromiseRecord(xsMachine* the, xsSlot* value, xsSlot* record)
{
	mxPush(mxObjectPrototype);
	xsSlot* instance = fxNewObjectInstance(the);
	xsSlot* slot = fxLastProperty(the, instance);
	xsSlot* promise;
	mxTemporary(promise);
	fxCreateResolvedPromise(the, value, promise);
	slot = fxNextSlotProperty(the, slot, promise, xsID_promise, XS_NO_FLAG);
	mxPop(); // promise
	if (record)
		mxPullSlot(record);
	else
		mxPop();
	return instance;
}

void fxGetPromiseRecordPromise(xsMachine* the, xsSlot* record, xsSlot* promise)
{
	mxPushReference(record);
	mxGetID(xsID_promise);
	mxPullSlot(promise);
}

void fxHandlePromiseRecord(xsMachine* the, xsSlot* record, xsSlot* closures)
{
	mxPushReference(record);
	mxGetID(xsID_promise);
	xsSlot* promise = the->stack;
	mxPushReference(closures);
	mxGetID(xsID_ignore);
	xsSlot* rejected = the->stack;
	fxChainPromise(the, promise, NULL, rejected->value.reference, NULL);
	mxPop();
	mxPop();
}

txBoolean fxIsThenableObject(xsMachine* the, xsSlot* slot)
{
	txBoolean result = 0;
	if (mxIsReference(slot)) {
		mxPushSlot(slot);
		mxGetID(mxID(_then));
		if (fxIsCallable(the, the->stack))
			result = 1;
		mxPop();
	}
	return result;
}

txBoolean fxIsPromiseRecordPending(xsMachine* the, xsSlot* record)
{
	xsSlot* promise = record->next;
	xsSlot* status = promise->value.reference->next;
	return (status->value.integer == mxPendingStatus) ? 1 : 0;
}

void fxRejectPromiseRecord(xsMachine* the, xsSlot* record, xsSlot* reason)
{
	if (fxIsPromiseRecordPending(the, record)) {
		mxPushReference(record);
		mxDub();
		mxGetID(xsID_reject);
		mxCall();
		if (reason) {
			if (reason->kind == XS_INSTANCE_KIND)
				mxPushReference(reason);
			else
				mxPushSlot(reason);
		}
		else
			mxPushNull();
		mxRunCount(1);
		mxPop();
	}
}
void fxResolvePromiseRecord(xsMachine* the, xsSlot* record, xsSlot* value)
{
	if (fxIsPromiseRecordPending(the, record)) {
		mxPushReference(record);
		mxDub();
		mxGetID(xsID_resolve);
		mxCall();
		if (value) {
			if (value->kind == XS_INSTANCE_KIND)
				mxPushReference(value);
			else
				mxPushSlot(value);
		}
		else
			mxPushNull();
		mxRunCount(1);
		mxPop();
	}
}

void fxStreamAssert(xsMachine* the, char* file, int line)
{
	fxDebugger(the, file, line);
	mxTypeError("assertion");
}

void** fxStreamHandle(xsMachine* the, xsSlot* it, StreamDispatch dispatch)
{
	txSlot* host = C_NULL;
	if (it->kind == XS_REFERENCE_KIND)
		it = it->value.reference;
	if (it->kind == XS_INSTANCE_KIND) {
		if (it->next) {
			it = it->next;
			if ((it->flag & XS_INTERNAL_FLAG) && (it->kind == XS_HOST_KIND))
				host = it;
		}
	}
	if (host) {
		StreamStuff stuff = host->value.host.data;
		if (stuff) {
			if ((dispatch == NULL) || (stuff->dispatch == dispatch))
				return &host->value.host.data;
		}
	}
	mxTypeError("not a %s", dispatch->type);
}
		
xsSlot* fxNewHostFunctionWithHandle(txMachine* the, void* handle, txCallback callback, txInteger length)
{
	txSlot* function = fxNewHostFunction(the, callback, length, XS_NO_ID, 0);
	txSlot* home = mxFunctionInstanceHome(function);
	home->value.home.object = (*((StreamHandle*)(handle)))->reference;
	return function;
}

void* fxGetHostFunctionHandle(txMachine* the)
{
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	mxPushReference(home->value.home.object);
	void* handle = fxGetHostHandle(the, the->stack);
	mxPop();
	return handle;
}

// 7.4 Abstract operations
xsSlot* ExtractAlgorithmReference(xsMachine* the, xsSlot* target, txID id)
{
	xsSlot* reference = NULL;
	mxPushSlot(target);
	if (fxRunTest(the)) {
		mxPushSlot(target);
		mxGetID(id);
		if (!mxIsUndefined(the->stack)) {
			if (!fxIsCallable(the, the->stack))
				mxTypeError("callback is no function");
			fxToInstance(the, the->stack);
			reference = fxToReference(the, the->stack);
			mxPop();
		}
	}
	return reference;
}
xsNumberValue ExtractHighWaterMark(xsMachine* the, xsSlot* strategy, xsNumberValue defaultValue)
{
	xsNumberValue value = defaultValue;
	mxPushSlot(strategy);
	if (fxRunTest(the)) {
		mxPushSlot(strategy);
		if (mxHasID(xsID_highWaterMark)) {
			mxPushSlot(strategy);
			mxGetID(xsID_highWaterMark);
			value = fxToNumber(the, the->stack);
			if (c_isnan(value) || (value < 0))
				mxRangeError("invalid highWaterMark");
		}
	}
	return value;
}
xsSlot* ExtractSizeAlgorithm(xsMachine* the, xsSlot* strategy)
{
	xsSlot* reference = NULL;;
	mxPushSlot(strategy);
	if (fxRunTest(the)) {
		mxPushSlot(strategy);
		if (mxHasID(xsID_size)) {
			mxPushSlot(strategy);
			mxGetID(xsID_size);
			mxDub();
			if (fxRunTest(the)) {
				if (!fxIsCallable(the, the->stack))
					mxTypeError("size is no function");
				fxToInstance(the, the->stack);
				reference = fxToReference(the, the->stack);
			}
			mxPop();
		}
	}
	return reference;
}

void fxChainAlgorithm(xsMachine* the, xsSlot* result, txBoolean success, xsSlot* resolved, xsSlot* rejected)
{
	if (fxIsThenableObject(the, result)) {
		txSlot* promise;
		mxTemporary(promise);
		fxCreateResolvedPromise(the, result, promise);
		fxChainPromise(the, promise, resolved, rejected, NULL);
		mxPop();
	}
	else {
		mxPushUndefined();
		if (success)
			mxPushReference(resolved);
		else
			mxPushReference(rejected);
		mxCall();	
		mxPushSlot(result);
		fxQueueJob(the, 1, C_NULL);
	}
}

void StreamMarkAlgorithm(xsMachine* the, StreamAlgorithm algorithm, xsMarkRoot markRoot)
{
	StreamMarkReference(the, algorithm->callback);
	StreamMarkReference(the, algorithm->resolved);
	StreamMarkReference(the, algorithm->rejected);
}

void fxNewStreamResult(txMachine* the, txBoolean done)
{
	txSlot* value = the->stack;
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextSlotProperty(the, slot, value, mxID(_value), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextBooleanProperty(the, slot, done, mxID(_done), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	mxPullSlot(value);
}

xsSlot* CreateSlotQueue(xsMachine* the)
{
	xsSlot* instance = fxNewInstance(the);
	xsSlot* length = fxNextIntegerProperty(the, instance, 0, XS_NO_ID, XS_INTERNAL_FLAG);
	xsSlot* list = fxNextUndefinedProperty(the, length, XS_NO_ID, XS_INTERNAL_FLAG);
	list->value.list.first = C_NULL;	
	list->value.list.last = C_NULL;	
	list->kind = XS_LIST_KIND;
	return instance;
}
void DequeueSlot(xsMachine* the, xsSlot* queue, xsSlot* it)
{
	xsSlot* length = queue->next;
	xsSlot* list = length->next;
	xsSlot* slot = list->value.list.first;
	it->kind = slot->kind;
	it->value = slot->value;
	list->value.list.first = slot->next;
	if (list->value.list.first == NULL)
		list->value.list.last = NULL;
	length->value.integer--;
}
void EnqueueSlot(xsMachine* the, xsSlot* queue, xsSlot* it)
{
	xsSlot* length = queue->next;
	xsSlot* list = length->next;
	xsSlot* slot = fxNewSlot(the);
	slot->kind = it->kind;
	slot->value = it->value;
	if (list->value.list.last == NULL)
		list->value.list.first = slot;
	else
		list->value.list.last->next = slot;
	list->value.list.last = slot;
	length->value.integer++;
}
txInteger GetSlotQueueLength(xsMachine* the, xsSlot* queue)
{
	xsSlot* length = queue->next;
	return length->value.integer;
}
void PeekSlotQueue(xsMachine* the, xsSlot* queue, xsSlot* it)
{
	xsSlot* length = queue->next;
	xsSlot* list = length->next;
	xsSlot* slot = list->value.list.first;
	it->kind = slot->kind;
	it->value = slot->value;
}
void ResetSlotQueue(xsMachine* the, xsSlot* queue)
{
	xsSlot* length = queue->next;
	xsSlot* list = length->next;
	list->value.list.first = C_NULL;	
	list->value.list.last = C_NULL;	
	length->value.integer = 0;
}
void ShiftSlotQueue(xsMachine* the, xsSlot* queue)
{
	xsSlot* length = queue->next;
	xsSlot* list = length->next;
	xsSlot* slot = list->value.list.first;
	list->value.list.first = slot->next;
	if (list->value.list.first == NULL)
		list->value.list.last = NULL;
	length->value.integer--;
}

// 8.1 Queue-with-sizes

xsSlot* CreateValueSizeQueue(xsMachine* the)
{
	xsSlot* instance = fxNewInstance(the);
	xsSlot* length = fxNextIntegerProperty(the, instance, 0, XS_NO_ID, XS_INTERNAL_FLAG);
	fxNextNumberProperty(the, length, 0, XS_NO_ID, XS_INTERNAL_FLAG);
	return instance;
}
void DequeueValueSize(xsMachine* the, xsSlot* queue, xsSlot* result)
{
	xsSlot* length = queue->next;
	xsSlot* totalSize = length->next;
	xsSlot* slot = totalSize->next;
	result->kind = slot->kind;
	result->value = slot->value;
	slot = slot->next;
	length->value.integer--;
	totalSize->value.number -= slot->value.number;
	if (totalSize->value.number < 0)
		totalSize->value.number = 0;
	totalSize->next = slot->next;
}
void EnqueueValueSize(xsMachine* the, xsSlot* queue, xsSlot* value, txNumber size)
{
	if (c_isnan(size) || (size < 0) || (size == C_INFINITY))
		xsRangeError("invalid size");
	xsSlot* length = queue->next;
	xsSlot* totalSize = length->next;
	xsSlot* slot = fxLastProperty(the, totalSize);
	slot = fxNextSlotProperty(the, slot, value, XS_NO_ID, XS_INTERNAL_FLAG);
	slot = fxNextNumberProperty(the, slot, size, XS_NO_ID, XS_INTERNAL_FLAG);
	length->value.integer++;
	totalSize->value.number += size;
}
txInteger GetQueueLength(xsMachine* the, xsSlot* queue)
{
	xsSlot* length = queue->next;
	return length->value.integer;
}
txNumber GetQueueTotalSize(xsMachine* the, xsSlot* queue)
{
	xsSlot* length = queue->next;
	xsSlot* totalSize = length->next;
	return totalSize->value.number;
}
void PeekQueueValue(xsMachine* the, xsSlot* queue, xsSlot* result, txNumber* size)
{
	xsSlot* length = queue->next;
	xsSlot* totalSize = length->next;
	xsSlot* slot = totalSize->next;
	result->kind = slot->kind;
	result->value = slot->value;
	slot = slot->next;
	*size = slot->value.number;
}
void ResetQueue(xsMachine* the, xsSlot* queue)
{
	xsSlot* length = queue->next;
	xsSlot* totalSize = length->next;
	length->value.integer = 0;
	totalSize->value.number = 0;
	totalSize->next = NULL;
}

/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
#include <stdio.h>

#ifndef ICACHE_XS6STRING_ATTR
	#define ICACHE_XS6STRING_ATTR
#endif

#define fxPop() (*(the->stack++))
#define fxPush(_SLOT) (*(--the->stack) = (_SLOT))

void _xsNewArray(txMachine *the, txSlot *res, txInteger length)
{
	fxNewArray(the, length);
	*res = fxPop();
}

void _xsNewObject(txMachine *the, txSlot *res)
{
	fxNewObject(the);
	*res = fxPop();
}

void _xsNewHostInstance(txMachine *the, txSlot *res, txSlot *proto)
{
	mxOverflow(-1);
	fxPush(*proto);
	fxNewHostInstance(the);
	*res = fxPop();
}

txBoolean _xsIsInstanceOf(txMachine *the, txSlot *v, txSlot *proto)
{
	mxOverflow(-2);
	fxPush(*proto);
	fxPush(*v);
	return fxIsInstanceOf(the);
}

txBoolean _xsHas(txMachine *the, txSlot *self, txID id)
{
	mxOverflow(-1);
	fxPush(*self);
	return mxHasID(id);
}

txBoolean _xsHasIndex(txMachine *the, txSlot *self, txIndex index)
{
	mxOverflow(-1);
	fxPush(*self);
	return fxHasIndex(the, index);
}

void _xsGet(txMachine *the, txSlot *res, txSlot *self, txID id)
{
	mxOverflow(-1);
	fxPush(*self);
	mxGetID(id);
	*res = fxPop();
}

void _xsGetAt(txMachine *the, txSlot *res, txSlot *self, txSlot *at)
{
	mxOverflow(-2);
	fxPush(*self);
	fxPush(*at);
	fxGetAt(the);
	*res = fxPop();
}

void _xsGetIndex(txMachine *the, txSlot *res, txSlot *self, txIndex index)
{
	mxOverflow(-1);
	fxPush(*self);
	mxGetIndex(index);
	*res = fxPop();
}

void _xsSet(txMachine *the, txSlot *self, txID id, txSlot *v)
{
	mxOverflow(-2);
	fxPush(*v);
	fxPush(*self);
	mxSetID(id);
	mxPop();
}

void _xsSetAt(txMachine *the, txSlot *self, txSlot *at , txSlot *v)
{
	mxOverflow(-3);
	fxPush(*v);
	fxPush(*self);
	fxPush(*at);
	fxSetAt(the);
	mxPop();
}

void _xsSetIndex(txMachine *the, txSlot *self, txIndex index, txSlot *v)
{
	mxOverflow(-2);
	fxPush(*v);
	fxPush(*self);
	mxSetIndex(index);
	mxPop();
}

void _xsDefine(txMachine *the, txSlot *self, txID id, txSlot *v, txFlag attributes)
{
	mxOverflow(-2);
	fxPush(*v);
	fxPush(*self);
	fxDefineID(the, id, attributes, attributes | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPop();
}

void _xsDelete(txMachine *the, txSlot *self, txID id)
{
	mxOverflow(-1);
	fxPush(*self);
	mxDeleteID(id);
}

void _xsDeleteAt(txMachine *the, txSlot *self, txSlot *at)
{
	mxOverflow(-2);
	fxPush(*self);
	fxPush(*at);
	fxDeleteAt(the);
}

void _xsCall(txMachine *the, txSlot *res, txSlot *self, txUnsigned id, ...)
{
	va_list ap;
	int n;
	txSlot *v;

	va_start(ap, id);
	for (n = 0; va_arg(ap, txSlot *) != NULL; n++)
		;
	va_end(ap);
	mxOverflow(-(n+6));
	fxPush(*self);
	fxCallID(the, (txID)id);
	va_start(ap, id);
	while ((v = va_arg(ap, txSlot *)) != NULL)
		fxPush(*v);
	va_end(ap);
	fxRunCount(the, n);
	if (res != NULL)
		*res = fxPop();
	else
		mxPop();
}

void _xsNew(txMachine *the, txSlot *res, txSlot *self, txUnsigned id, ...)
{
	va_list ap;
	int n;
	txSlot *v;

	va_start(ap, id);
	for (n = 0; va_arg(ap, txSlot *) != NULL; n++)
		;
	va_end(ap);
	mxOverflow(-(n+6));
	fxPush(*self);
	fxNewID(the, (txID)id);
	va_start(ap, id);
	while ((v = va_arg(ap, txSlot *)) != NULL)
		fxPush(*v);
	va_end(ap);
	fxRunCount(the, n);
	*res = fxPop();
}

txBoolean _xsTest(txMachine *the, txSlot *v)
{
	mxOverflow(-1);
	fxPush(*v);
	return fxRunTest(the);
}

txInteger fxIncrementalVars(txMachine* the, txInteger theCount)
{
	txSlot* aStack = the->scope;
	txInteger aVar;

	if (aStack - aStack->value.integer != the->stack) {
		static const char msg[] ICACHE_XS6STRING_ATTR = "C: xsVars: too late";
		mxSyntaxError((char *)msg);
	}
	mxOverflow(theCount);
	aVar = aStack->value.integer;
	aStack->value.integer += theCount;
	if (theCount > 0) {
		while (theCount) {
			mxPushUndefined();
			theCount--;
		}
	}
	else
		the->stack -= theCount;
	return aVar;
}

txInteger _xsArgc(txMachine *the)
{
	return mxArgc;
}

#ifndef __XSMC_H__
enum {
	xsBufferNonrelocatable,
	xsBufferRelocatable
};
#endif

static txInteger _xsmcGetViewBuffer(txMachine *the, txSlot* view, txSlot* buffer, void **data, txUnsigned *count, txBoolean writable)
{
	txInteger offset = view->value.dataView.offset;
	txInteger size = view->value.dataView.size;
	txSlot* arrayBuffer = buffer->value.reference->next;
	txSlot* bufferInfo = arrayBuffer->next;
	if (arrayBuffer->value.arrayBuffer.address == C_NULL)
		mxTypeError("detached buffer");
	if (writable && (arrayBuffer->flag & XS_DONT_SET_FLAG))
		mxTypeError("read-only buffer");
	if (bufferInfo->value.bufferInfo.maxLength >= 0) {
		txInteger byteLength = bufferInfo->value.bufferInfo.length;
		if (offset > byteLength)
			mxTypeError("out of bounds view");
		if (size < 0)
			size = byteLength - offset;
		else if (offset + size > byteLength)
			mxTypeError("out of bounds view");
	}
	*data = arrayBuffer->value.arrayBuffer.address + offset;
	*count = size;

	return (XS_ARRAY_BUFFER_KIND == arrayBuffer->kind) ? xsBufferRelocatable : xsBufferNonrelocatable;
}

txInteger _xsmcGetBuffer(txMachine *the, txSlot *slot, void **data, txUnsigned *count, txBoolean writable)
{
	txSlot* instance;

	if (slot->kind != XS_REFERENCE_KIND)
		goto bail;

	instance = slot->value.reference;
	if (!(((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG)))
		goto bail;

	if (slot->kind == XS_ARRAY_BUFFER_KIND) {
		txSlot* bufferInfo = slot->next;
		if (slot->value.arrayBuffer.address == C_NULL)
			mxTypeError("detached buffer");
		if (writable && (slot->flag & XS_DONT_SET_FLAG))
			mxTypeError("read-only buffer");
		*data = slot->value.arrayBuffer.address;
		*count = bufferInfo->value.bufferInfo.length;
		return xsBufferRelocatable;
	}

	if (slot->kind == XS_TYPED_ARRAY_KIND) {
		txSlot* view = slot->next;
		txSlot* buffer = view->next;
		if (1 != slot->value.typedArray.dispatch->size)
			goto bail;
		return _xsmcGetViewBuffer(the, view, buffer, data, count, writable);
	}
	
	if (slot->kind == XS_HOST_KIND) {
		txSlot* bufferInfo = slot->next;
		if (slot->flag & XS_HOST_CHUNK_FLAG)
			goto bail;
		if (writable && (slot->flag & XS_DONT_SET_FLAG))
			mxTypeError("read-only buffer");
		if (bufferInfo && (bufferInfo->kind == XS_BUFFER_INFO_KIND)) {
			*data = (uint8_t *)slot->value.host.data;
			*count = bufferInfo->value.bufferInfo.length;
		}
		else {
			// for compatibility. should throw eventually.
			txSlot tmp, ref;
			fxReport(the, "# Use xsmcSetHostBuffer instead of xsmcSetHostData\n");
			fxReference(the, &ref, instance);
			_xsGet(the, &tmp, &ref, _byteLength);
			*data = (uint8_t *)slot->value.host.data;
			*count = (txUnsigned)fxToInteger(the, &tmp);
		}
		return xsBufferNonrelocatable;
	}

	if (slot->kind == XS_DATA_VIEW_KIND) {
		txSlot* view = slot;
		txSlot* buffer = view->next;
		return _xsmcGetViewBuffer(the, view, buffer, data, count, writable);
	}

bail:
	mxTypeError("this is no buffer");
	return xsBufferNonrelocatable;
}

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
	fxOverflow(the, -1, C_NULL, 0);
	fxPush(*proto);
	fxNewHostInstance(the);
	*res = fxPop();
}

txBoolean _xsIsInstanceOf(txMachine *the, txSlot *v, txSlot *proto)
{
	fxOverflow(the, -2, C_NULL, 0);
	fxPush(*proto);
	fxPush(*v);
	return fxIsInstanceOf(the);
}

txBoolean _xsHas(txMachine *the, txSlot *self, txInteger id)
{
	fxOverflow(the, -1, C_NULL, 0);
	fxPush(*self);
	return fxHasID(the, id);
}

void _xsGet(txMachine *the, txSlot *res, txSlot *self, txInteger id)
{
	fxOverflow(the, -1, C_NULL, 0);
	fxPush(*self);
	fxGetID(the, id);
	*res = fxPop();
}

void _xsGetAt(txMachine *the, txSlot *res, txSlot *self, txSlot *at)
{
	fxOverflow(the, -2, C_NULL, 0);
	fxPush(*self);
	fxPush(*at);
	fxGetAt(the);
	*res = fxPop();
}

void _xsSet(txMachine *the, txSlot *self, txInteger id, txSlot *v)
{
	fxOverflow(the, -2, C_NULL, 0);
	fxPush(*v);
	fxPush(*self);
	fxSetID(the, id);
	the->stack++;
}

void _xsSetAt(txMachine *the, txSlot *self, txSlot *at , txSlot *v)
{
	fxOverflow(the, -3, C_NULL, 0);
	fxPush(*v);
	fxPush(*self);
	fxPush(*at);
	fxSetAt(the);
	the->stack++;
}

void _xsDelete(txMachine *the, txSlot *self, txInteger id)
{
	fxOverflow(the, -1, C_NULL, 0);
	fxPush(*self);
	fxDeleteID(the, id);
}

void _xsDeleteAt(txMachine *the, txSlot *self, txSlot *at)
{
	fxOverflow(the, -2, C_NULL, 0);
	fxPush(*self);
	fxPush(*at);
	fxDeleteAt(the);
}

void _xsCall(txMachine *the, txSlot *res, txSlot *self, txInteger id, ...)
{
	va_list ap;
	int n;
	txSlot *v;

	va_start(ap, id);
	for (n = 0; va_arg(ap, txSlot *) != NULL; n++)
		;
	va_end(ap);
	fxOverflow(the, -(n+2), C_NULL, 0);
	va_start(ap, id);
	while ((v = va_arg(ap, txSlot *)) != NULL)
		fxPush(*v);
	va_end(ap);
	fxInteger(the, --the->stack, n);
	fxPush(*self);
	fxCallID(the, id);
	if (res != NULL)
		*res = fxPop();
	else
		the->stack++;
}

void _xsNew(txMachine *the, txSlot *res, txSlot *self, txInteger id, ...)
{
	va_list ap;
	int n;
	txSlot *v;

	va_start(ap, id);
	for (n = 0; va_arg(ap, txSlot *) != NULL; n++)
		;
	va_end(ap);
	fxOverflow(the, -(n+2), C_NULL, 0);
	va_start(ap, id);
	while ((v = va_arg(ap, txSlot *)) != NULL)
		fxPush(*v);
	va_end(ap);
	fxInteger(the, --the->stack, n);
	fxPush(*self);
	fxNewID(the, id);
	*res = fxPop();
}

txBoolean _xsTest(txMachine *the, txSlot *v)
{
	fxOverflow(the, -1, C_NULL, 0);
	fxPush(*v);
	return fxRunTest(the);
}

txInteger fxIncrementalVars(txMachine* the, txInteger theCount)
{
	txSlot* aStack = the->frame - 1;
	txInteger aVar;

	if (aStack - aStack->value.integer != the->stack) {
		static const char msg[] ICACHE_XS6STRING_ATTR = "C: xsVars: too late";
		mxSyntaxError((char *)msg);
	}
	fxOverflow(the, theCount, C_NULL, 0);
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

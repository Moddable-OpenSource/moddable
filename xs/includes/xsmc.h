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

#ifndef __XSMC_H__
#define __XSMC_H__

#include "xs.h"

#if 0
#undef fxPop
#undef fxPush

static inline xsSlot _fxPop(xsMachine *the)
{
	return *the->stack++;
}

static inline void _fxPush(xsMachine *the, xsSlot *v)
{
	*--the->stack = *v;
}

#define fxPop()		_fxPop(the)
#define fxPush(v)	_fxPush(the, &v)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#undef xsTypeOf
#define xsmcTypeOf(_SLOT)	fxTypeOf(the, &_SLOT)

#undef xsToBoolean
#undef xsToInteger
#undef xsToNumber
#undef xsToString
#undef xsToStringBuffer
#define xsmcToBoolean(_SLOT)	fxToBoolean(the, &_SLOT)
#define xsmcToInteger(_SLOT)	fxToInteger(the, &_SLOT)
#define xsmcToNumber(_SLOT)	fxToNumber(the, &_SLOT)
#define xsmcToString(_SLOT)	fxToString(the, &_SLOT)
#define xsmcToStringBuffer(_SLOT,_BUFFER,_SIZE)	fxToStringBuffer(the, &_SLOT, _BUFFER ,_SIZE)

#define xsmcSetUndefined(_SLOT)	fxUndefined(the, &_SLOT)
#define xsmcSetNull(_SLOT)	fxNull(the, &_SLOT)
#define xsmcSetFalse(_SLOT)	fxBoolean(the, &_SLOT, 0)
#define xsmcSetTrue(_SLOT)	fxBoolean(the, &_SLOT, 1)
#define xsmcSetBoolean(_SLOT, _VALUE)	 fxBoolean(the, &_SLOT, _VALUE)
#define xsmcSetInteger(_SLOT, _VALUE)	fxInteger(the, &_SLOT, _VALUE)
#define xsmcSetNumber(_SLOT, _VALUE)	fxNumber(the, &_SLOT, _VALUE)
#define xsmcSetString(_SLOT, _VALUE)	fxString(the, &_SLOT, _VALUE)
#define xsmcSetStringBuffer(_SLOT, _BUFFER,_SIZE)	fxStringBuffer(the, &_SLOT, _BUFFER ,_SIZE)

#define xsmcSetArrayBuffer(_SLOT, _BUFFER, _SIZE)	fxArrayBuffer(the, &_SLOT, _BUFFER, _SIZE)
#undef xsGetArrayBufferData
#define xsmcGetArrayBufferData(_SLOT,_OFFSET,_BUFFER,_SIZE)	fxGetArrayBufferData(the, &_SLOT, _OFFSET, _BUFFER, _SIZE)
#undef xsSetArrayBufferData
#define xsmcSetArrayBufferData(_SLOT,_OFFSET,_BUFFER,_SIZE)	fxSetArrayBufferData(the, &_SLOT, _OFFSET, _BUFFER, _SIZE)
#undef xsToArrayBuffer
#define xsmcToArrayBuffer(_SLOT)	fxToArrayBuffer(the, &_SLOT)

mxImport void _xsNewArray(xsMachine*, xsSlot*, xsIntegerValue);
#define xsmcNewArray(_LENGTH)	(_xsNewArray(the, &the->scratch, _LENGTH), the->scratch)
#define xsmcSetNewArray(_SLOT, _LENGTH)	_xsNewArray(the, &_SLOT, _LENGTH)

mxImport void _xsNewObject(xsMachine*, xsSlot*);
#define xsmcNewObject()	(_xsNewObject(the, &the->scratch), the->scratch)
#define xsmcSetNewObject(_SLOT)	_xsNewObject(the, &_SLOT)

mxImport void _xsNewHostInstance(xsMachine *, xsSlot *, xsSlot *);
#define xsmcNewHostInstance(_PROTOTYPE)	(_xsNewHostInstance(the, &the->scratch, &_PROTOTYPE), the->scratch)
#define xsmcSetNewHostInstance(_SLOT, _PROTOTYPE)	_xsNewHostInstance(the, &_SLOT, &_PROTOTYPE)

#undef xsIsInstanceOf
mxImport xsBooleanValue _xsIsInstanceOf(xsMachine *, xsSlot *, xsSlot *);
#define xsmcIsInstanceOf(_SLOT,_PROTOTYPE)	_xsIsInstanceOf(the, &_SLOT, &_PROTOTYPE)

#undef xsHas
#undef xsGet
#undef xsGetAt
#undef xsSet
#undef xsSetAt
#undef xsDelete
#undef xsDeleteAt
mxImport xsBooleanValue _xsHas(xsMachine *, xsSlot *, xsIntegerValue);
#define xsmcHas(_THIS, _ID)	_xsHas(the, &_THIS, _ID)
mxImport void _xsGet(xsMachine *, xsSlot *, xsSlot *, xsIntegerValue);
#define xsmcGet(_SLOT, _THIS, _ID)	_xsGet(the, &_SLOT, &_THIS, _ID)
mxImport void _xsGetAt(xsMachine *, xsSlot *, xsSlot *, xsSlot *);
#define xsmcGetAt(_SLOT, _THIS, _AT)	_xsGetAt(the, &_SLOT, &_THIS, &_AT)
mxImport void _xsSet(xsMachine *, xsSlot *, xsIntegerValue, xsSlot *);
#define xsmcSet(_THIS, _ID, _SLOT)	_xsSet(the, &_THIS, _ID, &_SLOT)
mxImport void _xsSetAt(xsMachine *, xsSlot *, xsSlot *, xsSlot *);
#define xsmcSetAt(_THIS, _AT, _SLOT)	_xsSetAt(the, &_THIS, &_AT, &_SLOT)
mxImport void _xsDelete(xsMachine *, xsSlot *, xsIntegerValue);
#define xsmcDelete(_THIS, _ID)	_xsDelete(the, &_THIS, _ID)
mxImport void _xsDeleteAt(xsMachine *, xsSlot *, xsSlot *);
#define xsmcDeleteAt(_THIS, _AT)	_xsDeleteAt(the, &_THIS, &_AT)
mxImport void _xsCall(xsMachine *, xsSlot *, xsSlot *, xsIntegerValue, ...);
#define xsmcCall(_RES, _THIS, _ID, ...)		_xsCall(the, &_RES, &_THIS, _ID, __VA_ARGS__)
#define xsmcCall_noResult(_THIS, _ID, ...)	_xsCall(the, NULL, &_THIS, _ID, __VA_ARGS__)
mxImport void _xsNew(xsMachine *, xsSlot *, xsSlot *, xsIntegerValue, ...);
#define xsmcNew(_RES, _THIS, _ID, ...)	_xsNew(the, &_RES, &_THIS, _ID, __VA_ARGS__)
mxImport xsBooleanValue _xsTest(xsMachine *, xsSlot *);
#undef xsTest
#define xsmcTest(_SLOT)	_xsTest(the, &_SLOT)

#undef xsGetHostData
#undef xsSetHostData
#define xsmcGetHostData(_SLOT)	fxGetHostData(the, &_SLOT)
#define xsmcSetHostData(_SLOT, _DATA)	fxSetHostData(the, &_SLOT, _DATA)

#undef xsGetHostChunk
#undef xsSetHostChunk
#define xsmcGetHostChunk(_SLOT)	fxGetHostChunk(the, &_SLOT)
#define xsmcSetHostChunk(_SLOT, _DATA, _SIZE)	fxSetHostChunk(the, &_SLOT, _DATA, _SIZE)

#undef xsVars
mxImport xsIntegerValue fxIncrementalVars(xsMachine*, xsIntegerValue);
#define xsmcVars(_COUNT) fxIncrementalVars(the, _COUNT)

mxImport xsIntegerValue _xsArgc(xsMachine*);
#define xsmcArgc _xsArgc(the)

#ifndef true
	#define true (1)
#endif
#ifndef false
	#define false (0)
#endif

#ifdef __cplusplus
}
#endif

#endif	/* __XSMC_H__ */


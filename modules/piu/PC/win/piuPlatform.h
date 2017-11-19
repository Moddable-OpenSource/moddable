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
 */

#define piuPC 1
typedef int32_t PiuCoordinate;
typedef int32_t PiuDimension;
#define xsPiuCoordinate(VALUE) xsInteger((xsIntegerValue)(VALUE))
#define xsPiuDimension(VALUE) xsInteger((xsIntegerValue)(VALUE))
#define xsToPiuCoordinate(SLOT) (PiuCoordinate)xsToInteger(SLOT)
#define xsToPiuDimension(SLOT) (PiuDimension)xsToInteger(SLOT)

#define xsStringW(_VALUE) \
	(fxStringW(the, &the->scratch, _VALUE), \
	the->scratch)

#define xsToStringBufferW(_SLOT,_BUFFER,_SIZE) \
	(the->scratch = (_SLOT), \
	fxToStringBufferW(the, &(the->scratch), _BUFFER ,_SIZE))

#define xsToStringCopy(_SLOT) _strdup(xsToString(_SLOT))
#define xsToStringCopyW(_SLOT) \
	(the->scratch = (_SLOT), \
	fxToStringCopyW(the, &(the->scratch)))

#ifdef mxDebug
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowLastError(the,(char *)__FILE__,__LINE__), 1)))
#else
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowLastError(the,NULL,0), 1)))
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern void fxStringW(xsMachine* the, xsSlot* slot, wchar_t* wideString);
extern wchar_t* fxToStringCopyW(xsMachine* the, xsSlot* slot);
extern wchar_t* fxToStringBufferW(xsMachine* the, xsSlot* slot, wchar_t* wideString, xsIntegerValue length);
extern void fxThrowLastError(xsMachine* the, xsStringValue path, xsIntegerValue line);
#ifdef __cplusplus
}
#endif

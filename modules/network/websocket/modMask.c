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

#include "xsPlatform.h"
#include "xs.h"
#include "mc.xs.h"			// for xsID_ values

void xs_websocket_mask(xsMachine *the)
{
	const unsigned char *src;
	int srcSize;
	const unsigned char *mask = xsToArrayBuffer(xsArg(1));
	unsigned char *dst;
	int i;
	xsType srcType, dstType;

	srcType = xsTypeOf(xsArg(0));
	if (xsStringType == srcType) {
		src = (unsigned char *)xsToString(xsArg(0));
		srcSize = c_strlen((char *)src);
	}
	else {
		src = xsToArrayBuffer(xsArg(0));
		srcSize = xsGetArrayBufferLength(xsArg(0));
	}

	if (xsToInteger(xsArgc) <= 2)
		dstType = srcType;
	else {
		xsSlot *s1, *s2;

		xsResult = xsGet(xsGlobal, xsID_String);
		s1 = &xsArg(2);
		s2 = &xsResult;
		if (s1->data[2] == s2->data[2])		//@@
			dstType = xsStringType;
		else
			dstType = xsReferenceType;
	}

	if (xsStringType == dstType) {
		xsResult = xsStringBuffer(NULL, srcSize);
		dst = (unsigned char *)xsToString(xsResult);
	}
	else {
		xsResult = xsArrayBuffer(NULL, srcSize);
		dst = xsToArrayBuffer(xsResult);
	}

	for (i = 0; srcSize; srcSize--, i = (i + 1) & 3)
		*dst++ = c_read8(src++) ^ mask[i];
}

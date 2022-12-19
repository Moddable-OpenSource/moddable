/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
#include "xsmc.h"

enum {
	kOpXor,
	kOpAnd,
	kOpOr
};

static void xs_logical_op(xsMachine *the, int operation)
{
	int srcType = xsmcTypeOf(xsArg(1));
	uint8_t scratch;
	uint8_t *src, *dst;
	xsUnsignedValue srcLen, dstLen, i = 0;

	xsmcGetBufferWritable(xsArg(0), (void **)&dst, &dstLen);

	switch (srcType) {
		case xsIntegerType:
		case xsNumberType:
			scratch = (uint8_t)xsmcToInteger(xsArg(1));
			src = &scratch;
			srcLen = 1;
			break;

		case xsStringType:
			src = (uint8_t *)xsmcToString(xsArg(1));
			srcLen = c_strlen((char *)src);
			break;

		case xsReferenceType:
			xsmcGetBufferReadable(xsArg(1), (void **)&src, &srcLen);
			break;

		default:
			xsUnknownError("unsupported source");
			break;
	}

	if (kOpXor == operation) {
		while (dstLen--) {
			*dst++ ^= src[i++];
			if (i == srcLen)
				i = 0;
		}
	}
	else
	if (kOpOr == operation) {
		while (dstLen--) {
			*dst++ |= src[i++];
			if (i == srcLen)
				i = 0;
		}
	}
	else {
		while (dstLen--) {
			*dst++ &= src[i++];
			if (i == srcLen)
				i = 0;
		}
	}
}

void xs_logical_xor(xsMachine *the)
{
	xs_logical_op(the, kOpXor);
}

void xs_logical_and(xsMachine *the)
{
	xs_logical_op(the, kOpAnd);
}

void xs_logical_or(xsMachine *the)
{
	xs_logical_op(the, kOpOr);
}

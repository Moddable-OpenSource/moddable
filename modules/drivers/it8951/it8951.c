/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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
 
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#include "xsHost.h"

void xs_applyFilter(xsMachine *the)
{
	uint8_t *filter, *src, *dst;
	xsUnsignedValue filterLen, srcLen, dstLen;
	int srcOffset = xsmcToInteger(xsArg(2)), dstOffset = xsmcToInteger(xsArg(5)), byteLength = xsmcToInteger(xsArg(3));

	xsmcGetBufferReadable(xsArg(1), (void **)&src, &srcLen);
	xsmcGetBufferWritable(xsArg(4), (void **)&dst, &dstLen);

	if ((srcOffset < 0) || (dstOffset < 0) ||
		((srcOffset + byteLength) > srcLen) || ((dstOffset + byteLength) > dstLen))
		xsUnknownError("invalid");
	
	src += srcOffset;
	dst += dstOffset;
	if (xsUndefinedType != xsmcTypeOf(xsArg(0))) {
		xsmcGetBufferReadable(xsArg(0), (void **)&filter, &filterLen);
		while (byteLength--) {
			uint8_t pixels = *src++;
			*dst++ = (filter[pixels >> 4] << 4) | filter[pixels & 0x0F];
		}
	}
	else
		c_memmove(dst, src, byteLength);
}

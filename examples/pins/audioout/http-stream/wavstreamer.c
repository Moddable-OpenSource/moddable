/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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

#include "xsHost.h"

void xs_wavestream_swap(xsMachine *the)
{
	uint8_t *buffer;
	xsUnsignedValue bufferLength;

	xsmcGetBufferWritable(xsArg(0), (void **)&buffer, &bufferLength);
	bufferLength >>= 1;
	while (bufferLength--) {
		uint8_t t = buffer[0];
		buffer[0] = buffer[1];
		buffer[1] = t;
		buffer += 2;
	}
}

void xs_wavestream_mix(xsMachine *the)
{
	uint8_t *bufferIn, *bufferOut;
	xsUnsignedValue bufferInLength, bufferOutLength;

	xsmcGetBufferReadable(xsArg(0), (void **)&bufferIn, &bufferInLength);
	xsmcGetBufferWritable(xsArg(1), (void **)&bufferOut, &bufferOutLength);
	bufferInLength >>= 2;
	bufferOutLength >>= 1;
	if (bufferOutLength < bufferInLength)
		xsUnknownError("invalid");
	int16_t *in = (int16_t *)bufferIn;
	int16_t *out = (int16_t *)bufferOut;
	while (bufferInLength--) {
		*out++ = (in[0] + in[1]) >> 1;
		in += 2;
	}
}

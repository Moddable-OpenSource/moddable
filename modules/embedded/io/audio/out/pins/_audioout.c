/*
 * Copyright (c) 2024 Moddable Tech, Inc.
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
#include "mc.xs.h"
#include "mc.defines.h"
#include "string.h"

void xs_audioout_zero(xsMachine *the)
{
	void* buffer;
	xsUnsignedValue length;
	xsmcGetBufferWritable(xsArg(0), (void **)&buffer, &length);
	memset(buffer, 0, length);
}

void xs_audioout_write(xsMachine *the)
{
	xsUnsignedValue available, offset, length;
	void* buffer;
	void* samples;
	xsmcVars(1);

	xsmcGet(xsVar(0), xsThis, xsID_buffer);
	xsmcGetBufferWritable(xsVar(0), (void **)&buffer, &available);
	xsmcGet(xsVar(0), xsThis, xsID_offset);
	offset = xsmcToInteger(xsVar(0));
	available -= offset;
	xsmcGetBufferReadable(xsArg(0), (void **)&samples, &length);
	xsmcSetInteger(xsResult, length);
	if ((length <= 0) || (length > available)) 
		xsUnknownError("invalid size");
	memcpy(((uint8_t*)buffer) + offset, samples, length);
	offset += length;
	xsVar(0) = xsInteger(offset);
	xsmcSet(xsThis, xsID_offset, xsVar(0));
}

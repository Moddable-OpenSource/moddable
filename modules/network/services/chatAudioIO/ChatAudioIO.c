/*
 * Copyright (c) 2024-2025 Moddable Tech, Inc.
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

 #include "mc.xs.h"
#include "math.h"

mxImport xsIntegerValue _xsmcGetBuffer(xsMachine *the, xsSlot *slot, void **data, xsUnsignedValue *count, xsBooleanValue writable);

void xs_computeLevel(xsMachine *the)
{
	uint8_t* buffer;
	xsUnsignedValue size, i;
	int16_t* samples;
	double average = 0.0;
	int32_t	result;

	_xsmcGetBuffer(the, &(xsArg(0)), (void**)&buffer, &size, 0);
	size >>= 1;
	samples = (int16_t*)buffer;
	for (i = 0; i < size; i++) {
		int16_t sample = samples[i];
		if (sample < 0) 
			average -= sample;
		else
			average += sample;
	}
	average /= size;
	result = round(average);
	xsResult = xsInteger(result);
}

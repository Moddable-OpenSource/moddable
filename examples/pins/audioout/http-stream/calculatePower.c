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
#include "mc.xs.h"
#include "xsHost.h"

void xs_calculatePower(xsMachine *the)
{
	int16_t *samples;
	xsUnsignedValue count, i;
	uint32_t power = 0;

	xsmcGetBufferReadable(xsArg(0), (void *)&samples, &count);
	count >>= 1;
	
	for (i = 0; i < count; i++) {
		int16_t sample = *samples++;
		power += sample * sample;
	}

	xsmcSetNumber(xsResult, c_sqrt((double)power / (double)count));
}

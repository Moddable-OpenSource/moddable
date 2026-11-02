/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

/*
	ECMA-419 Crypto Random — Raspberry Pi Pico: pico_rand (hardware entropy)
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "pico/rand.h"

void xs_random_fill(xsMachine *the)
{
	void *buffer;
	xsUnsignedValue length;
	xsmcGetBufferWritable(xsArg(0), &buffer, &length);

	uint8_t *p = buffer;
	xsUnsignedValue remaining = length;
	while (remaining) {
		uint64_t value = get_rand_64();
		xsUnsignedValue use = (remaining > sizeof(value)) ? sizeof(value) : remaining;
		c_memcpy(p, &value, use);
		p += use;
		remaining -= use;
	}

	xsmcSetInteger(xsResult, length);
}

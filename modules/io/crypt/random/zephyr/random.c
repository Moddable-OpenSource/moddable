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
	ECMA-419 Crypto Random — Zephyr: sys_csrand_get, the cryptographically secure generator over the hardware entropy driver
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include <zephyr/random/random.h>

#if !defined(CONFIG_CSPRNG_ENABLED) || defined(CONFIG_TEST_CSPRNG_GENERATOR)
	#error "embedded:crypt/random requires a hardware entropy driver (CONFIG_ENTROPY_GENERATOR with a CSPRNG source)"
#endif

void xs_random_fill(xsMachine *the)
{
	void *buffer;
	xsUnsignedValue length;
	xsmcGetBufferWritable(xsArg(0), &buffer, &length);

	if (0 != sys_csrand_get(buffer, length))
		xsUnknownError("sys_csrand_get failed");

	xsmcSetInteger(xsResult, length);
}

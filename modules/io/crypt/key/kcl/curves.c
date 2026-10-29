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
	Which curves this build supports: the manifest "defines" {"crypt": {"curve": {"p384": 0}}} omits one (MODDEF_CRYPT_CURVE_P384)
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.defines.h"

#ifndef MODDEF_CRYPT_CURVE_P256
	#define MODDEF_CRYPT_CURVE_P256 1
#endif
#ifndef MODDEF_CRYPT_CURVE_P384
	#define MODDEF_CRYPT_CURVE_P384 1
#endif
#ifndef MODDEF_CRYPT_CURVE_X25519
	#define MODDEF_CRYPT_CURVE_X25519 1
#endif

void xs_crypt_ec_supports(xsMachine *the)
{
	const char *name = xsmcToString(xsArg(0));
	int supported = 0;
	if (0 == c_strcmp(name, "P-256"))
		supported = MODDEF_CRYPT_CURVE_P256;
	else if (0 == c_strcmp(name, "P-384"))
		supported = MODDEF_CRYPT_CURVE_P384;
	else if (0 == c_strcmp(name, "X25519"))
		supported = MODDEF_CRYPT_CURVE_X25519;
	xsmcSetBoolean(xsResult, supported);
}

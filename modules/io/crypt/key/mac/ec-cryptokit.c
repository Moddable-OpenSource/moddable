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
	ECMA-419 Key — EC provider (macOS, CryptoKit): generate a key pair; derive the public point of a private scalar
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"
#include "mc.defines.h"

#include "xscryptokit.h"
#include "ec-cryptokit.h"

// a build may omit curves (manifest "defines": crypt.curve.<name> 0)
#ifndef MODDEF_CRYPT_CURVE_P256
	#define MODDEF_CRYPT_CURVE_P256 1
#endif
#ifndef MODDEF_CRYPT_CURVE_P384
	#define MODDEF_CRYPT_CURVE_P384 1
#endif
#ifndef MODDEF_CRYPT_CURVE_X25519
	#define MODDEF_CRYPT_CURVE_X25519 1
#endif

static const CurveEntry gCurves[] = {
#if MODDEF_CRYPT_CURVE_P256
	{ "P-256",  ckCurveP256,   32, 65 },
#endif
#if MODDEF_CRYPT_CURVE_P384
	{ "P-384",  ckCurveP384,   48, 97 },
#endif
#if MODDEF_CRYPT_CURVE_X25519
	{ "X25519", ckCurveX25519, 32, 32 },
#endif
	{ NULL, 0, 0, 0 }
};

const CurveEntry *xs_crypt_curve(const char *name)
{
	const CurveEntry *entry;
	for (entry = gCurves; entry->name; entry++) {
		if (0 == c_strcmp(name, entry->name))
			return entry;
	}
	return NULL;
}

void xs_ec_generate(xsMachine *the)
{
	const CurveEntry *entry = xs_crypt_curve(xsmcToString(xsArg(0)));
	if (!entry)
		xsRangeError("unsupported curve");

	uint8_t scalar[48], point[97];
	long scalarLength = sizeof(scalar), pointLength = sizeof(point);
	if ((0 != ckECGenerate(entry->curve, scalar, &scalarLength, point, &pointLength)) || (scalarLength != entry->scalarBytes) || (pointLength != entry->pointBytes))
		xsUnknownError("key generation failed");

	xsmcVars(1);
	xsmcSetNewObject(xsResult);
	xsmcSetArrayBuffer(xsVar(0), scalar, scalarLength);
	xsmcSet(xsResult, xsID_scalar, xsVar(0));
	xsmcSetArrayBuffer(xsVar(0), point, pointLength);
	xsmcSet(xsResult, xsID_point, xsVar(0));
	c_memset(scalar, 0, sizeof(scalar));
}

void xs_ec_public(xsMachine *the)
{
	const CurveEntry *entry = xs_crypt_curve(xsmcToString(xsArg(0)));
	if (!entry)
		xsRangeError("unsupported curve");

	void *scalar;
	xsUnsignedValue scalarLength;
	xsmcGetBufferReadable(xsArg(1), &scalar, &scalarLength);

	uint8_t point[97];
	long pointLength = sizeof(point);
	if ((0 != ckECPublic(entry->curve, scalar, scalarLength, point, &pointLength)) || (pointLength != entry->pointBytes))
		xsRangeError("invalid key");
	xsmcSetArrayBuffer(xsResult, point, pointLength);
}

void xs_crypt_ec_supports(xsMachine *the)
{
	xsmcSetBoolean(xsResult, NULL != xs_crypt_curve(xsmcToString(xsArg(0))));
}

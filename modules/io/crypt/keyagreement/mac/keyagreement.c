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
	ECMA-419 Crypto Key Agreement — macOS, CryptoKit: ECDH on P-256, P-384 and X25519
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"

#include "xscryptokit.h"
#include "ec-cryptokit.h"

typedef struct {
	const CurveEntry *curve;
	uint8_t scalar[48];
	uint8_t scalarLength;
} AgreementRecord, *Agreement;

void xs_keyagreement_destructor(void *data)
{
	if (data)
		c_memset(data, 0, sizeof(AgreementRecord));		// the private scalar
}

// (scalar, curve)
void xs_keyagreement_setup(xsMachine *the)
{
	void *scalar;
	xsUnsignedValue scalarLength;
	xsmcGetBufferReadable(xsArg(0), &scalar, &scalarLength);

	AgreementRecord init;
	c_memset(&init, 0, sizeof(init));
	init.curve = xs_crypt_curve(xsmcToString(xsArg(1)));
	if (!init.curve)
		xsRangeError("unsupported curve");
	if (scalarLength != init.curve->scalarBytes)
		xsRangeError("invalid key");
	xsmcGetBufferReadable(xsArg(0), &scalar, &scalarLength);
	c_memcpy(init.scalar, scalar, scalarLength);
	init.scalarLength = (uint8_t)scalarLength;

	xsmcSetHostChunk(xsThis, &init, sizeof(init));
	c_memset(&init, 0, sizeof(init));
}

void xs_keyagreement_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis))
		return;
	Agreement a = xsmcGetHostChunkValidate(xsThis, xs_keyagreement_destructor);
	xs_keyagreement_destructor(a);
	xsmcSetHostData(xsThis, NULL);	// downgrade to data slot so subsequent calls throw SyntaxError
}

// (point)
void xs_keyagreement_derive(xsMachine *the)
{
	Agreement a = xsmcGetHostChunkValidate(xsThis, xs_keyagreement_destructor);
	void *point;
	xsUnsignedValue pointLength;
	xsmcGetBufferReadable(xsArg(0), &point, &pointLength);

	uint8_t *out = xsmcSetArrayBuffer(xsResult, NULL, a->curve->scalarBytes);
	xsmcGetBufferReadable(xsArg(0), &point, &pointLength);		// allocation may have moved it
	a = xsmcGetHostChunk(xsThis);
	if (a->curve->scalarBytes != ckECDH(a->curve->curve, a->scalar, a->scalarLength, point, pointLength, out, a->curve->scalarBytes))
		xsRangeError("invalid public key");
}

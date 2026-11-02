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
	ECMA-419 Crypto Key Agreement — PSA Crypto
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"
#include "psa/crypto.h"

extern psa_key_type_t xs_crypt_ecKeyType(const char *curve, size_t *bits, uint8_t isPrivate);

typedef struct {
	psa_key_id_t key;
	uint16_t bytes;		// of the shared secret
} AgreementRecord, *Agreement;

static void psaCheck(xsMachine *the, psa_status_t status, const char *what)
{
	if (PSA_SUCCESS != status) {
		if (PSA_ERROR_NOT_SUPPORTED == status)
			xsUnknownError("algorithm not enabled in build");
		xsUnknownError("%s failed %d", what, (int)status);
	}
}

void xs_keyagreement_destructor(void *data)
{
	Agreement a = data;
	if (a) {
		if (a->key)
			psa_destroy_key(a->key);
		c_free(a);
	}
}

// (scalar, curve)
void xs_keyagreement_setup(xsMachine *the)
{
	void *scalar;
	xsUnsignedValue scalarLength;
	xsmcGetBufferReadable(xsArg(0), &scalar, &scalarLength);
	size_t bits;
	psa_key_type_t type = xs_crypt_ecKeyType(xsmcToString(xsArg(1)), &bits, 1);
	if (!type)
		xsRangeError("unsupported curve");

	Agreement a = c_calloc(1, sizeof(AgreementRecord));
	if (!a)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, a);
	a->bytes = (bits + 7) >> 3;

	psaCheck(the, psa_crypto_init(), "psa_crypto_init");
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attributes, type);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECDH);
	psa_status_t status = psa_import_key(&attributes, scalar, scalarLength, &a->key);
	psa_reset_key_attributes(&attributes);
	if (PSA_SUCCESS != status) {
		a->key = 0;
		if (PSA_ERROR_INVALID_ARGUMENT == status)
			xsRangeError("invalid key");
		psaCheck(the, status, "psa_import_key");
	}
}

void xs_keyagreement_close(xsMachine *the)
{
	if (!xsmcGetHostData(xsThis))
		return;
	Agreement a = xsmcGetHostDataValidate(xsThis, xs_keyagreement_destructor);
	xs_keyagreement_destructor(a);
	xsmcSetHostData(xsThis, NULL);
	xsmcSetHostDestructor(xsThis, NULL);
}

// (point)
void xs_keyagreement_derive(xsMachine *the)
{
	Agreement a = xsmcGetHostDataValidate(xsThis, xs_keyagreement_destructor);
	void *point;
	xsUnsignedValue pointLength;
	xsmcGetBufferReadable(xsArg(0), &point, &pointLength);
	uint8_t *out = xsmcSetArrayBuffer(xsResult, NULL, a->bytes);
	xsmcGetBufferReadable(xsArg(0), &point, &pointLength);		// allocation may have moved it
	a = xsmcGetHostData(xsThis);
	size_t length = 0;
	psa_status_t status = psa_raw_key_agreement(PSA_ALG_ECDH, a->key, point, pointLength, out, a->bytes, &length);
	if (PSA_ERROR_INVALID_ARGUMENT == status)
		xsRangeError("invalid public key");
	psaCheck(the, status, "psa_raw_key_agreement");
	if (length != a->bytes)
		xsUnknownError("internal: secret size");
}

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
	ECMA-419 Key — EC provider (PSA Crypto): generate a key pair; derive the public point of a private scalar
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"
#include "psa/crypto.h"

// curve name -> PSA key type for a key pair, and bits
psa_key_type_t xs_crypt_ecKeyType(const char *curve, size_t *bits, uint8_t isPrivate)
{
	psa_ecc_family_t family;
	if (0 == c_strcmp(curve, "P-256")) { family = PSA_ECC_FAMILY_SECP_R1; *bits = 256; }
	else if (0 == c_strcmp(curve, "P-384")) { family = PSA_ECC_FAMILY_SECP_R1; *bits = 384; }
	else if (0 == c_strcmp(curve, "X25519")) { family = PSA_ECC_FAMILY_MONTGOMERY; *bits = 255; }
	else return 0;
	return isPrivate ? PSA_KEY_TYPE_ECC_KEY_PAIR(family) : PSA_KEY_TYPE_ECC_PUBLIC_KEY(family);
}

// hash name -> PSA algorithm, and digest length. Here, as Signature includes Key, so both RSA cipher and Signature have it
psa_algorithm_t xs_crypt_hashAlgorithm(const char *name, int *length)
{
	static const struct { const char *name; psa_algorithm_t alg; int length; } hashes[] = {
		{ "MD5", PSA_ALG_MD5, 16 }, { "SHA-1", PSA_ALG_SHA_1, 20 }, { "SHA-224", PSA_ALG_SHA_224, 28 },
		{ "SHA-256", PSA_ALG_SHA_256, 32 }, { "SHA-384", PSA_ALG_SHA_384, 48 }, { "SHA-512", PSA_ALG_SHA_512, 64 }, { NULL, 0, 0 }
	};
	for (int i = 0; hashes[i].name; i++) {
		if (0 == c_strcmp(name, hashes[i].name)) {
			if (length) *length = hashes[i].length;
			return hashes[i].alg;
		}
	}
	return PSA_ALG_NONE;
}

static void psaCheck(xsMachine *the, psa_status_t status, const char *what)
{
	if (PSA_SUCCESS != status) {
		if (PSA_ERROR_NOT_SUPPORTED == status)
			xsUnknownError("algorithm not enabled in build");
		xsUnknownError("%s failed %d", what, (int)status);
	}
}

static void exportPair(xsMachine *the, psa_key_id_t key, size_t bits)
{
	uint8_t scalar[48], point[97];
	size_t scalarLength = 0, pointLength = 0;
	psa_status_t status = psa_export_key(key, scalar, sizeof(scalar), &scalarLength);
	if (PSA_SUCCESS == status)
		status = psa_export_public_key(key, point, sizeof(point), &pointLength);
	psa_destroy_key(key);
	psaCheck(the, status, "psa_export_key");

	xsmcVars(1);
	xsmcSetNewObject(xsResult);
	xsmcSetArrayBuffer(xsVar(0), scalar, scalarLength);
	xsmcSet(xsResult, xsID_scalar, xsVar(0));
	xsmcSetArrayBuffer(xsVar(0), point, pointLength);
	xsmcSet(xsResult, xsID_point, xsVar(0));
}

void xs_ec_generate(xsMachine *the)
{
	size_t bits;
	psa_key_type_t type = xs_crypt_ecKeyType(xsmcToString(xsArg(0)), &bits, 1);
	if (!type)
		xsRangeError("unsupported curve");
	psaCheck(the, psa_crypto_init(), "psa_crypto_init");
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attributes, type);
	psa_set_key_bits(&attributes, bits);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_EXPORT);
	psa_key_id_t key = 0;
	psa_status_t status = psa_generate_key(&attributes, &key);
	psa_reset_key_attributes(&attributes);
	psaCheck(the, status, "psa_generate_key");
	exportPair(the, key, bits);
}

void xs_ec_public(xsMachine *the)
{
	size_t bits;
	psa_key_type_t type = xs_crypt_ecKeyType(xsmcToString(xsArg(0)), &bits, 1);
	if (!type)
		xsRangeError("unsupported curve");
	void *scalar;
	xsUnsignedValue scalarLength;
	xsmcGetBufferReadable(xsArg(1), &scalar, &scalarLength);
	psaCheck(the, psa_crypto_init(), "psa_crypto_init");
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attributes, type);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_EXPORT);
	psa_key_id_t key = 0;
	psa_status_t status = psa_import_key(&attributes, scalar, scalarLength, &key);
	psa_reset_key_attributes(&attributes);
	if (PSA_ERROR_INVALID_ARGUMENT == status)
		xsRangeError("invalid key");
	psaCheck(the, status, "psa_import_key");
	uint8_t point[97];
	size_t pointLength = 0;
	status = psa_export_public_key(key, point, sizeof(point), &pointLength);
	psa_destroy_key(key);
	psaCheck(the, status, "psa_export_public_key");
	xsmcSetArrayBuffer(xsResult, point, pointLength);
}

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

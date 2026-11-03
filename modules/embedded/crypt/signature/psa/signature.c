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
	ECMA-419 Crypto Signature — PSA Crypto
	ECDSA with a hash is deterministic (RFC 6979, PSA_ALG_DETERMINISTIC_ECDSA); verification is ECDSA either way
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"
#include "psa/crypto.h"

typedef struct {
	psa_key_id_t key;
	psa_algorithm_t signAlg;
	psa_algorithm_t verifyAlg;
	uint16_t bytes;
	uint8_t hashLength;		// deterministic ECDSA: the digest length its hash produces (PSA does not check it)
	uint8_t isPrivate;
} SignatureRecord, *Signature;

extern psa_algorithm_t xs_crypt_hashAlgorithm(const char *name, int *length);		// in key/psa/ec-psa.c

static void psaCheck(xsMachine *the, psa_status_t status, const char *what)
{
	if (PSA_SUCCESS != status) {
		if (PSA_ERROR_NOT_SUPPORTED == status)
			xsUnknownError("algorithm not enabled in build");
		xsUnknownError("%s failed %d", what, (int)status);
	}
}

void xs_signature_destructor(void *data)
{
	Signature s = data;
	if (s) {
		if (s->key)
			psa_destroy_key(s->key);
		c_free(s);
	}
}

extern psa_key_type_t xs_crypt_ecKeyType(const char *curve, size_t *bits, uint8_t isPrivate);

// (material, type, algorithm, hash, saltLength, curve): material is PKCS#1 DER for RSA, the scalar or the point for ECDSA
void xs_signature_setup(xsMachine *the)
{
	void *der;
	xsUnsignedValue derLen;
	xsmcGetBufferReadable(xsArg(0), &der, &derLen);
	uint8_t isPrivate = (0 == c_strcmp(xsmcToString(xsArg(1)), "private"));
	char *algorithm = xsmcToString(xsArg(2));
	uint8_t isECDSA = (0 == c_strcmp(algorithm, "ECDSA"));
	int hashLength = 0;
	psa_algorithm_t hash = PSA_ALG_NONE;
	psa_key_type_t keyType;
	size_t bits = 0;
	if (isECDSA) {
		keyType = xs_crypt_ecKeyType(xsmcToString(xsArg(5)), &bits, isPrivate);
		if (!keyType)
			xsRangeError("unsupported curve");
		if (xsUndefinedType != xsmcTypeOf(xsArg(3))) {		// deterministic signing with this hash
			hash = xs_crypt_hashAlgorithm(xsmcToString(xsArg(3)), &hashLength);
			if (PSA_ALG_NONE == hash)
				xsRangeError("unsupported hash");
		}
	}
	else {
		hash = xs_crypt_hashAlgorithm(xsmcToString(xsArg(3)), &hashLength);
		if (PSA_ALG_NONE == hash)
			xsRangeError("unsupported hash");
		keyType = isPrivate ? PSA_KEY_TYPE_RSA_KEY_PAIR : PSA_KEY_TYPE_RSA_PUBLIC_KEY;
	}

	Signature s = c_calloc(1, sizeof(SignatureRecord));
	if (!s)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, s);
	s->isPrivate = isPrivate;

	if (isECDSA) {		// r || s over the digest as given (truncated to the order by PSA); deterministic signing needs the digest of its hash
		s->signAlg = (PSA_ALG_NONE == hash) ? PSA_ALG_ECDSA_ANY : PSA_ALG_DETERMINISTIC_ECDSA(hash);
		s->verifyAlg = PSA_ALG_ECDSA_ANY;
		s->hashLength = (uint8_t)hashLength;
	}
	else if (0 == c_strcmp(algorithm, "RSA-PSS")) {
		int saltLength = (xsUndefinedType == xsmcTypeOf(xsArg(4))) ? hashLength : xsmcToInteger(xsArg(4));
		if (saltLength < 0)
			xsRangeError("invalid saltLength");
		// PSA signs with a salt of the hash length; verifying any other salt length is only possible as "any"
		s->signAlg = (hashLength == saltLength) ? PSA_ALG_RSA_PSS(hash) : PSA_ALG_NONE;
		s->verifyAlg = (hashLength == saltLength) ? PSA_ALG_RSA_PSS(hash) : PSA_ALG_RSA_PSS_ANY_SALT(hash);
	}
	else {
		s->signAlg = PSA_ALG_RSA_PKCS1V15_SIGN(hash);
		s->verifyAlg = s->signAlg;
	}

	psaCheck(the, psa_crypto_init(), "psa_crypto_init");
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attributes, keyType);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_algorithm(&attributes, s->verifyAlg);
	if (s->signAlg && (s->signAlg != s->verifyAlg))
		psa_set_key_enrollment_algorithm(&attributes, s->signAlg);
	psa_status_t status = psa_import_key(&attributes, der, derLen, &s->key);
	psa_reset_key_attributes(&attributes);
	if (PSA_SUCCESS != status) {
		s->key = 0;
		if (PSA_ERROR_INVALID_ARGUMENT == status)
			xsRangeError("invalid key");
		psaCheck(the, status, "psa_import_key");
	}
	psa_get_key_attributes(s->key, &attributes);
	s->bytes = (psa_get_key_bits(&attributes) + 7) >> 3;
	if (isECDSA)
		s->bytes *= 2;		// r || s
	psa_reset_key_attributes(&attributes);
}

void xs_signature_close(xsMachine *the)
{
	if (!xsmcGetHostData(xsThis))
		return;
	Signature s = xsmcGetHostDataValidate(xsThis, xs_signature_destructor);
	xs_signature_destructor(s);
	xsmcSetHostData(xsThis, NULL);
	xsmcSetHostDestructor(xsThis, NULL);
}

void xs_signature_sign(xsMachine *the)
{
	Signature s = xsmcGetHostDataValidate(xsThis, xs_signature_destructor);
	if (!s->isPrivate)
		xsUnknownError("private key required");
	if (PSA_ALG_NONE == s->signAlg)
		xsRangeError("PSA signs RSA-PSS with a salt of the hash length only");

	void *digest;
	xsUnsignedValue digestLen;
	xsmcGetBufferReadable(xsArg(0), &digest, &digestLen);
	if (s->hashLength && (digestLen != s->hashLength))
		xsRangeError("digest length does not match hash");

	uint8_t *out = xsmcSetArrayBuffer(xsResult, NULL, s->bytes);
	xsmcGetBufferReadable(xsArg(0), &digest, &digestLen);		// allocation may have moved it
	s = xsmcGetHostData(xsThis);

	size_t length = 0;
	psa_status_t status = psa_sign_hash(s->key, s->signAlg, digest, digestLen, out, s->bytes, &length);
	if (PSA_ERROR_INVALID_ARGUMENT == status)
		xsRangeError("digest length does not match hash");
	psaCheck(the, status, "psa_sign_hash");
	if (length != s->bytes)
		xsUnknownError("internal: signature size");
}

void xs_signature_verify(xsMachine *the)
{
	Signature s = xsmcGetHostDataValidate(xsThis, xs_signature_destructor);
	void *digest, *signature;
	xsUnsignedValue digestLen, signatureLen;
	xsmcGetBufferReadable(xsArg(0), &digest, &digestLen);
	xsmcGetBufferReadable(xsArg(1), &signature, &signatureLen);
	psa_status_t status = psa_verify_hash(s->key, s->verifyAlg, digest, digestLen, signature, signatureLen);
	if ((PSA_ERROR_INVALID_SIGNATURE == status) || (PSA_ERROR_INVALID_ARGUMENT == status)) {
		xsmcSetFalse(xsResult);
		return;
	}
	psaCheck(the, status, "psa_verify_hash");
	xsmcSetTrue(xsResult);
}

void xs_signature_get_size(xsMachine *the)
{
	Signature s = xsmcGetHostDataValidate(xsThis, xs_signature_destructor);
	xsmcSetInteger(xsResult, s->bytes);
}

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
	ECMA-419 Crypto KeyDerivation — PSA Crypto: HKDF, HKDF-Extract, HKDF-Expand and PBKDF2 through the PSA key
	derivation operation, whose output is a stream as read() is
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_* values

#include "digest-psa.h"
#include "mc.defines.h"

enum {
	kHKDF,
	kHKDFExtract,
	kHKDFExpand,
	kPBKDF2,
};

// a build may omit algorithms (manifest "defines": crypt.keyderivation.<name> 0): HKDF covers its three forms
#ifndef MODDEF_CRYPT_KEYDERIVATION_HKDF
	#define MODDEF_CRYPT_KEYDERIVATION_HKDF 1
#endif
#ifndef MODDEF_CRYPT_KEYDERIVATION_PBKDF2
	#define MODDEF_CRYPT_KEYDERIVATION_PBKDF2 1
#endif
#if MODDEF_CRYPT_KEYDERIVATION_HKDF
	#define mxHKDFName(name) name
#else
	#define mxHKDFName(name) NULL
#endif
#if MODDEF_CRYPT_KEYDERIVATION_PBKDF2
	#define mxPBKDF2Name(name) name
#else
	#define mxPBKDF2Name(name) NULL
#endif

static const char *gAlgorithmNames[] = { mxHKDFName("HKDF"), mxHKDFName("HKDF-Extract"), mxHKDFName("HKDF-Expand"), mxPBKDF2Name("PBKDF2") };		// NULL: omitted

typedef struct {
	psa_key_derivation_operation_t op;
	uint8_t algorithm;
	uint8_t hashIndex;
} KDFRecord, *KDF;

static void psaCheck(xsMachine *the, psa_status_t status, const char *what)
{
	if (PSA_SUCCESS != status) {
		if (PSA_ERROR_NOT_SUPPORTED == status)
			xsUnknownError("algorithm not enabled in build");
		xsUnknownError("%s failed %d", what, (int)status);
	}
}

void xs_keyderivation_destructor(void *data)
{
	KDF kdf = data;
	if (kdf)
		psa_key_derivation_abort(&kdf->op);
}

void xs_keyderivation_constructor(xsMachine *the)
{
	xsmcVars(1);

	if (!xsmcGet(xsVar(0), xsArg(0), xsID_algorithm))
		xsUnknownError("algorithm required");
	char *name = xsmcToString(xsVar(0));
	uint8_t algorithm;
	for (algorithm = 0; algorithm < 4; algorithm++) {
		if (gAlgorithmNames[algorithm] && (0 == c_strcmp(name, gAlgorithmNames[algorithm])))
			break;
	}
	if (4 == algorithm)
		xsUnknownError("unsupported algorithm");

	if (!xsmcGet(xsVar(0), xsArg(0), xsID_hash))
		xsUnknownError("hash required");
	const DigestAlgEntry *entry = xs_crypt_digestAlgorithm(xsmcToString(xsVar(0)));
	if (!entry)
		xsRangeError("unsupported hash");

	if (!xsmcGet(xsVar(0), xsArg(0), xsID_key))
		xsUnknownError("key required");
	void *key;
	xsUnsignedValue keyLen;
	xsmcGetBufferReadable(xsVar(0), &key, &keyLen);

	void *salt = NULL, *info = NULL;
	xsUnsignedValue saltLen = 0, infoLen = 0;
	uint64_t iterations = 0;
	int hasSalt = xsmcGet(xsVar(0), xsArg(0), xsID_salt);
	if (hasSalt)
		xsmcGetBufferReadable(xsVar(0), &salt, &saltLen);
	int hasInfo = xsmcGet(xsVar(0), xsArg(0), xsID_info);
	if (hasInfo)
		xsmcGetBufferReadable(xsVar(0), &info, &infoLen);
	int hasIterations = xsmcGet(xsVar(0), xsArg(0), xsID_iterations);
	if (hasIterations) {
		double value = xsmcToNumber(xsVar(0));
		if (!(value >= 1) || (value > 4294967295.0))
			xsRangeError("invalid iterations");
		iterations = (uint64_t)value;
	}
	if (kPBKDF2 == algorithm) {
		if (!hasIterations)
			xsUnknownError("iterations required");
		if (hasInfo)
			xsRangeError("info not supported by algorithm");
	}
	else {
		if (hasIterations)
			xsRangeError("iterations not supported by algorithm");
		if ((kHKDFExtract == algorithm) && hasInfo)
			xsRangeError("info not supported by algorithm");
		if ((kHKDFExpand == algorithm) && hasSalt)
			xsRangeError("salt not supported by algorithm");
	}

	psa_algorithm_t alg = PSA_ALG_NONE;
	switch (algorithm) {
#if MODDEF_CRYPT_KEYDERIVATION_HKDF
		case kHKDF: alg = PSA_ALG_HKDF(entry->alg); break;
		case kHKDFExtract: alg = PSA_ALG_HKDF_EXTRACT(entry->alg); break;
		case kHKDFExpand: alg = PSA_ALG_HKDF_EXPAND(entry->alg); break;
#endif
#if MODDEF_CRYPT_KEYDERIVATION_PBKDF2
		case kPBKDF2: alg = PSA_ALG_PBKDF2_HMAC(entry->alg); break;
#endif
	}

	KDFRecord init;
	init.op = (psa_key_derivation_operation_t)PSA_KEY_DERIVATION_OPERATION_INIT;
	init.algorithm = algorithm;
	init.hashIndex = (uint8_t)(entry - xs_crypt_digestAlgorithms);

	psa_status_t status = psa_crypto_init();
	psaCheck(the, status, "psa_crypto_init");
	status = psa_key_derivation_setup(&init.op, alg);
	if (PSA_SUCCESS == status) {
		if (kPBKDF2 == algorithm) {		// cost, salt, password
			status = psa_key_derivation_input_integer(&init.op, PSA_KEY_DERIVATION_INPUT_COST, iterations);
			if (PSA_SUCCESS == status)
				status = psa_key_derivation_input_bytes(&init.op, PSA_KEY_DERIVATION_INPUT_SALT, salt, saltLen);
			if (PSA_SUCCESS == status)
				status = psa_key_derivation_input_bytes(&init.op, PSA_KEY_DERIVATION_INPUT_PASSWORD, key, keyLen);
		}
		else {		// salt (Extract steps; omitted or empty is hashLen zeros per RFC 5869, which PSA's Extract step needs stated), secret, info (Expand steps)
			uint8_t zeros[64];
			if (kHKDFExpand != algorithm) {
				if (!hasSalt || !saltLen) {
					c_memset(zeros, 0, entry->outputSize);
					salt = zeros;
					saltLen = entry->outputSize;
				}
				status = psa_key_derivation_input_bytes(&init.op, PSA_KEY_DERIVATION_INPUT_SALT, salt, saltLen);
			}
			if (PSA_SUCCESS == status)
				status = psa_key_derivation_input_bytes(&init.op, PSA_KEY_DERIVATION_INPUT_SECRET, key, keyLen);
			if ((PSA_SUCCESS == status) && (kHKDFExtract != algorithm))
				status = psa_key_derivation_input_bytes(&init.op, PSA_KEY_DERIVATION_INPUT_INFO, info, infoLen);
		}
	}
	if (PSA_SUCCESS != status) {
		psa_key_derivation_abort(&init.op);
		psaCheck(the, status, "psa_key_derivation_setup");
	}

	xsmcSetHostChunk(xsThis, &init, sizeof(init));
}

void xs_keyderivation_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis))
		return;
	KDF kdf = xsmcGetHostChunkValidate(xsThis, xs_keyderivation_destructor);
	psa_key_derivation_abort(&kdf->op);
	xsmcSetHostData(xsThis, NULL);	// downgrade to data slot so subsequent calls throw SyntaxError
}

void xs_keyderivation_read(xsMachine *the)
{
	KDF kdf = xsmcGetHostChunkValidate(xsThis, xs_keyderivation_destructor);
	size_t available;		// the operation's remaining capacity: the algorithm's output limit less what was read
	psa_status_t status = psa_key_derivation_get_capacity(&kdf->op, &available);
	psaCheck(the, status, "psa_key_derivation_get_capacity");
	uint8_t *out;
	xsUnsignedValue count;

	if (xsmcTypeOf(xsArg(0)) == xsReferenceType) {
		xsmcGetBufferWritable(xsArg(0), (void **)&out, &count);
		if (!available)
			return;		// output exhausted: undefined
		if (count > available)
			count = available;
		xsmcSetInteger(xsResult, count);
	}
	else {
		double value = xsmcToNumber(xsArg(0));
		if (!(value >= 0) || (value > 0x7FFFFFFF))
			xsRangeError("invalid count");
		if (!available)
			return;		// output exhausted: undefined
		count = (xsUnsignedValue)value;
		if (count > available)
			count = available;
		out = xsmcSetArrayBuffer(xsResult, NULL, count);
	}

	kdf = xsmcGetHostChunk(xsThis);
	status = psa_key_derivation_output_bytes(&kdf->op, out, count);
	psaCheck(the, status, "psa_key_derivation_output_bytes");
}

void xs_keyderivation_get_algorithm(xsMachine *the)
{
	KDF kdf = xsmcGetHostChunkValidate(xsThis, xs_keyderivation_destructor);
	xsmcSetString(xsResult, (char *)gAlgorithmNames[kdf->algorithm]);
}

void xs_keyderivation_get_hash(xsMachine *the)
{
	KDF kdf = xsmcGetHostChunkValidate(xsThis, xs_keyderivation_destructor);
	xsmcSetString(xsResult, (char *)xs_crypt_digestAlgorithms[kdf->hashIndex].name);
}

void xs_keyderivation_supports(xsMachine *the)
{
	xsmcVars(1);
	int supported = 0;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_algorithm) && (xsStringType == xsmcTypeOf(xsVar(0)))) {
		const char *name = xsmcToString(xsVar(0));
		for (int i = 0; i < 4; i++) {
			if (gAlgorithmNames[i] && (0 == c_strcmp(name, gAlgorithmNames[i]))) {
				supported = 1;
				break;
			}
		}
		if (supported && xsmcGet(xsVar(0), xsArg(0), xsID_hash)) {
			const DigestAlgEntry *entry = (xsStringType == xsmcTypeOf(xsVar(0))) ? xs_crypt_digestAlgorithm(xsmcToString(xsVar(0))) : NULL;
			supported = NULL != entry;
		}
	}
	xsmcSetBoolean(xsResult, supported);
}

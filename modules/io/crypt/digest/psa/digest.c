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
	ECMA-419 Crypto Digest — PSA Crypto
	SHA family + MD5 via PSA Crypto. Algorithm names follow Web Crypto.
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_* values

#include "psa/crypto.h"

typedef struct {
	psa_hash_operation_t op;
	psa_algorithm_t alg;
	uint16_t outputSize;
	uint16_t blockSize;
} DigestRecord, *Digest;

#include "digest-psa.h"

const DigestAlgEntry xs_crypt_digestAlgorithms[] = {
#if MODDEF_CRYPT_DIGEST_MD5
	{ "MD5",    PSA_ALG_MD5,    16, 64  },
#endif
#if MODDEF_CRYPT_DIGEST_SHA1
	{ "SHA-1",   PSA_ALG_SHA_1,  20, 64  },
#endif
#if MODDEF_CRYPT_DIGEST_SHA224
	{ "SHA-224", PSA_ALG_SHA_224, 28, 64  },
#endif
#if MODDEF_CRYPT_DIGEST_SHA256
	{ "SHA-256", PSA_ALG_SHA_256, 32, 64  },
#endif
#if MODDEF_CRYPT_DIGEST_SHA384
	{ "SHA-384", PSA_ALG_SHA_384, 48, 128 },
#endif
#if MODDEF_CRYPT_DIGEST_SHA512
	{ "SHA-512", PSA_ALG_SHA_512, 64, 128 },
#endif
	{ NULL, 0, 0, 0 }
};

const DigestAlgEntry *xs_crypt_digestAlgorithm(const char *name)
{
	const DigestAlgEntry *entry;
	for (entry = xs_crypt_digestAlgorithms; entry->name; entry++) {
		if (0 == c_strcmp(name, entry->name))
			return entry;
	}
	return NULL;
}

void xs_digest_destructor(void *data)
{
	Digest digest = data;
	if (digest)
		psa_hash_abort(&digest->op);
}

void xs_digest_constructor(xsMachine *the)
{
	xsmcVars(1);
	if (!xsmcGet(xsVar(0), xsArg(0), xsID_algorithm))
		xsUnknownError("algorithm required");
	char *algorithm = xsmcToString(xsVar(0));

	const DigestAlgEntry *entry = xs_crypt_digestAlgorithm(algorithm);
	if (!entry)
		xsUnknownError("unsupported algorithm");

	DigestRecord init = {
		.alg = entry->alg,
		.outputSize = entry->outputSize,
		.blockSize = entry->blockSize,
	};

	psa_status_t status = psa_crypto_init();
	if (PSA_SUCCESS != status)
		xsUnknownError("psa_crypto_init failed %d", (int)status);

	status = psa_hash_setup(&init.op, init.alg);
	if (PSA_SUCCESS != status) {
		if (PSA_ERROR_NOT_SUPPORTED == status)
			xsUnknownError("algorithm not enabled in build");
		xsUnknownError("psa_hash_setup failed %d", (int)status);
	}

	xsmcSetHostChunk(xsThis, &init, sizeof(init));
}

void xs_digest_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis))
		return;
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);
	psa_hash_abort(&digest->op);
	xsmcSetHostData(xsThis, NULL);	// downgrade to data slot so subsequent calls throw SyntaxError
}

void xs_digest_write(xsMachine *the)
{
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);
	void *data;
	xsUnsignedValue size;

	xsmcGetBufferReadable(xsArg(0), &data, &size);

	psa_status_t status = psa_hash_update(&digest->op, data, size);
	if (PSA_SUCCESS != status)
		xsUnknownError("psa_hash_update failed %d", (int)status);
}

void xs_digest_read(xsMachine *the)
{
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);
	uint8_t *out;

	if (xsmcArgc > 0) {
		xsUnsignedValue outLen;
		xsmcGetBufferWritable(xsArg(0), (void **)&out, &outLen);
		if (outLen < digest->outputSize)
			xsRangeError("buffer too small");
		xsmcSetInteger(xsResult, digest->outputSize);
	}
	else
		out = xsmcSetArrayBuffer(xsResult, NULL, digest->outputSize);

	digest = xsmcGetHostChunk(xsThis);

	psa_hash_operation_t clone = PSA_HASH_OPERATION_INIT;
	psa_status_t status = psa_hash_clone(&digest->op, &clone);
	if (PSA_SUCCESS != status)
		xsUnknownError("psa_hash_clone failed %d", (int)status);

	size_t actual = 0;
	status = psa_hash_finish(&clone, out, digest->outputSize, &actual);
	if (PSA_SUCCESS != status) {
		psa_hash_abort(&clone);
		xsUnknownError("psa_hash_finish failed %d", (int)status);
	}
}

void xs_digest_reset(xsMachine *the)
{
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);

	psa_hash_abort(&digest->op);

	psa_status_t status = psa_hash_setup(&digest->op, digest->alg);
	if (PSA_SUCCESS != status)
		xsUnknownError("psa_hash_setup failed %d", (int)status);
}

void xs_digest_get_blockSize(xsMachine *the)
{
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);
	xsmcSetInteger(xsResult, digest->blockSize);
}

void xs_digest_get_outputSize(xsMachine *the)
{
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);
	xsmcSetInteger(xsResult, digest->outputSize);
}

void xs_digest_supports(xsMachine *the)
{
	xsmcVars(1);
	int supported = 0;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_algorithm) && (xsStringType == xsmcTypeOf(xsVar(0))))
		supported = NULL != xs_crypt_digestAlgorithm(xsmcToString(xsVar(0)));
	xsmcSetBoolean(xsResult, supported);
}

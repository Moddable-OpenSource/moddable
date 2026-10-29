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
	ECMA-419 Crypto Digest — macOS
	MD5 and the SHA family via CryptoKit (SHA-224 via CommonCrypto, which CryptoKit lacks).
	Algorithm names follow Web Crypto.
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_* values

#include <CommonCrypto/CommonDigest.h>
#include "xscryptokit.h"
#include "digest-cryptokit.h"

typedef struct {
	union {
		void *ck;				// a CryptoKit hash
		CC_SHA256_CTX cc;		// SHA-224
	} op;
	uint8_t provider;
	uint16_t outputSize;
	uint16_t blockSize;
} DigestRecord, *Digest;

const DigestAlgEntry xs_crypt_digestAlgorithms[] = {
#if MODDEF_CRYPT_DIGEST_MD5
	{ "MD5",     kDigestCryptoKit,    ckHashMD5,    16, 64  },
#endif
#if MODDEF_CRYPT_DIGEST_SHA1
	{ "SHA-1",   kDigestCryptoKit,    ckHashSHA1,   20, 64  },
#endif
#if MODDEF_CRYPT_DIGEST_SHA224
	{ "SHA-224", kDigestCommonCrypto, 0,            28, 64  },
#endif
#if MODDEF_CRYPT_DIGEST_SHA256
	{ "SHA-256", kDigestCryptoKit,    ckHashSHA256, 32, 64  },
#endif
#if MODDEF_CRYPT_DIGEST_SHA384
	{ "SHA-384", kDigestCryptoKit,    ckHashSHA384, 48, 128 },
#endif
#if MODDEF_CRYPT_DIGEST_SHA512
	{ "SHA-512", kDigestCryptoKit,    ckHashSHA512, 64, 128 },
#endif
	{ NULL, 0, 0, 0, 0 }
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
	if (digest && (kDigestCryptoKit == digest->provider) && digest->op.ck)
		ckHashFree(digest->op.ck);
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
		.provider = entry->provider,
		.outputSize = entry->outputSize,
		.blockSize = entry->blockSize,
	};

	if (kDigestCommonCrypto == entry->provider)
		CC_SHA224_Init(&init.op.cc);
	else {
		init.op.ck = ckHashCreate(entry->alg);
		if (!init.op.ck)
			xsUnknownError("CryptoKit hash unavailable");
	}

	xsmcSetHostChunk(xsThis, &init, sizeof(init));
}

void xs_digest_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis))
		return;
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);
	xs_digest_destructor(digest);
	xsmcSetHostData(xsThis, NULL);	// downgrade to data slot so subsequent calls throw SyntaxError
}

void xs_digest_write(xsMachine *the)
{
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);
	void *data;
	xsUnsignedValue size;

	xsmcGetBufferReadable(xsArg(0), &data, &size);
	digest = xsmcGetHostChunk(xsThis);

	if (kDigestCommonCrypto == digest->provider)
		CC_SHA224_Update(&digest->op.cc, data, size);
	else
		ckHashUpdate(digest->op.ck, data, size);
}

// non-destructive: the running state continues after read
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

	if (kDigestCommonCrypto == digest->provider) {
		CC_SHA256_CTX clone = digest->op.cc;
		CC_SHA224_Final(out, &clone);
	}
	else
		ckHashFinalize(digest->op.ck, out);
}

void xs_digest_reset(xsMachine *the)
{
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);

	if (kDigestCommonCrypto == digest->provider)
		CC_SHA224_Init(&digest->op.cc);
	else
		ckHashReset(digest->op.ck);
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

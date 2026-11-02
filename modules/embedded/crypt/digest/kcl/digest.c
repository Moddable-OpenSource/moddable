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
	ECMA-419 Crypto Digest — KCL software backend
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include <stddef.h>
#include "mc.xs.h"			// for xsID_* values

#include "digest-kcl.h"

typedef struct {
	uint8_t alg;
	union DigestCtx ctx;		// last: allocated to the algorithm's size
} DigestRecord, *Digest;

static const DigestAlgEntry gDigestAlgs[] = {		// in enum order: indexed by alg
#if MODDEF_CRYPT_DIGEST_MD5
	{ "MD5",    kAlgMD5,    MD5_DGSTSIZE,    MD5_BLKSIZE,    sizeof(struct md5) },
#else
	{ NULL, kAlgMD5, 0, 0, 0 },		// omitted from this build
#endif
#if MODDEF_CRYPT_DIGEST_SHA1
	{ "SHA-1",   kAlgSHA1,   SHA1_DGSTSIZE,   SHA1_BLKSIZE,   sizeof(struct sha1) },
#else
	{ NULL, kAlgSHA1, 0, 0, 0 },		// omitted from this build
#endif
#if MODDEF_CRYPT_DIGEST_SHA224
	{ "SHA-224", kAlgSHA224, SHA224_DGSTSIZE, SHA224_BLKSIZE, sizeof(struct sha256) },
#else
	{ NULL, kAlgSHA224, 0, 0, 0 },		// omitted from this build
#endif
#if MODDEF_CRYPT_DIGEST_SHA256
	{ "SHA-256", kAlgSHA256, SHA256_DGSTSIZE, SHA256_BLKSIZE, sizeof(struct sha256) },
#else
	{ NULL, kAlgSHA256, 0, 0, 0 },		// omitted from this build
#endif
#if MODDEF_CRYPT_DIGEST_SHA384
	{ "SHA-384", kAlgSHA384, SHA384_DGSTSIZE, SHA384_BLKSIZE, sizeof(struct sha512) },
#else
	{ NULL, kAlgSHA384, 0, 0, 0 },		// omitted from this build
#endif
#if MODDEF_CRYPT_DIGEST_SHA512
	{ "SHA-512", kAlgSHA512, SHA512_DGSTSIZE, SHA512_BLKSIZE, sizeof(struct sha512) },
#else
	{ NULL, kAlgSHA512, 0, 0, 0 },		// omitted from this build
#endif
	{ NULL, 0, 0, 0 }
};

const DigestAlgEntry *xs_crypt_digestAlgorithm(const char *name)
{
	const DigestAlgEntry *entry;
	for (entry = gDigestAlgs; entry <= &gDigestAlgs[kAlgSHA512]; entry++) {
		if (entry->name && (0 == c_strcmp(name, entry->name)))
			return entry;
	}
	return NULL;
}

void xs_crypt_digestCreate(uint8_t alg, union DigestCtx *ctx)
{
	switch (alg) {
#if MODDEF_CRYPT_DIGEST_MD5
		case kAlgMD5:    md5_create(&ctx->md5); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA1
		case kAlgSHA1:   sha1_create(&ctx->sha1); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA224
		case kAlgSHA224: sha224_create(&ctx->sha256); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA256
		case kAlgSHA256: sha256_create(&ctx->sha256); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA384
		case kAlgSHA384: sha384_create(&ctx->sha512); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA512
		case kAlgSHA512: sha512_create(&ctx->sha512); break;
#endif
	}
}

void xs_crypt_digestUpdate(uint8_t alg, union DigestCtx *ctx, const void *data, uint32_t size)
{
	switch (alg) {
#if MODDEF_CRYPT_DIGEST_MD5
		case kAlgMD5:    md5_update(&ctx->md5, data, size); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA1
		case kAlgSHA1:   sha1_update(&ctx->sha1, data, size); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA224
		case kAlgSHA224: sha224_update(&ctx->sha256, data, size); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA256
		case kAlgSHA256: sha256_update(&ctx->sha256, data, size); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA384
		case kAlgSHA384: sha384_update(&ctx->sha512, data, size); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA512
		case kAlgSHA512: sha512_update(&ctx->sha512, data, size); break;
#endif
	}
}

void xs_crypt_digestFinish(uint8_t alg, union DigestCtx *ctx, uint8_t *out)
{
	switch (alg) {
#if MODDEF_CRYPT_DIGEST_MD5
		case kAlgMD5:    md5_fin(&ctx->md5, out); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA1
		case kAlgSHA1:   sha1_fin(&ctx->sha1, out); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA224
		case kAlgSHA224: sha224_fin(&ctx->sha256, out); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA256
		case kAlgSHA256: sha256_fin(&ctx->sha256, out); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA384
		case kAlgSHA384: sha384_fin(&ctx->sha512, out); break;
#endif
#if MODDEF_CRYPT_DIGEST_SHA512
		case kAlgSHA512: sha512_fin(&ctx->sha512, out); break;
#endif
	}
}

void xs_digest_destructor(void *data)
{
	// this space intentionally left black
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

	DigestRecord init = { .alg = entry->alg };
	xs_crypt_digestCreate(entry->alg, &init.ctx);

	xsmcSetHostChunk(xsThis, &init, offsetof(DigestRecord, ctx) + entry->ctxSize);
}

void xs_digest_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis))
		return;
	xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);
	xsmcSetHostData(xsThis, NULL);	// downgrade to data slot so subsequent calls throw SyntaxError
}

void xs_digest_write(xsMachine *the)
{
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);
	void *data;
	xsUnsignedValue size;

	xsmcGetBufferReadable(xsArg(0), &data, &size);

	xs_crypt_digestUpdate(digest->alg, &digest->ctx, data, (uint32_t)size);
}

void xs_digest_read(xsMachine *the)
{
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);
	const DigestAlgEntry *entry = &gDigestAlgs[digest->alg];
	uint8_t *out;

	if (xsmcArgc > 0) {
		xsUnsignedValue outLen;
		xsmcGetBufferWritable(xsArg(0), (void **)&out, &outLen);
		if (outLen < entry->outputSize)
			xsRangeError("buffer too small");
		xsmcSetInteger(xsResult, entry->outputSize);
	}
	else
		out = xsmcSetArrayBuffer(xsResult, NULL, entry->outputSize);

	digest = xsmcGetHostChunk(xsThis);
	union DigestCtx clone;
	c_memcpy(&clone, &digest->ctx, entry->ctxSize);		// non-destructive read: finish a copy of the context
	xs_crypt_digestFinish(digest->alg, &clone, out);
}

void xs_digest_reset(xsMachine *the)
{
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);
	xs_crypt_digestCreate(digest->alg, &digest->ctx);
}

void xs_digest_get_blockSize(xsMachine *the)
{
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);
	xsmcSetInteger(xsResult, gDigestAlgs[digest->alg].blockSize);
}

void xs_digest_get_outputSize(xsMachine *the)
{
	Digest digest = xsmcGetHostChunkValidate(xsThis, xs_digest_destructor);
	xsmcSetInteger(xsResult, gDigestAlgs[digest->alg].outputSize);
}

void xs_digest_supports(xsMachine *the)
{
	xsmcVars(1);
	int supported = 0;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_algorithm) && (xsStringType == xsmcTypeOf(xsVar(0))))
		supported = NULL != xs_crypt_digestAlgorithm(xsmcToString(xsVar(0)));
	xsmcSetBoolean(xsResult, supported);
}

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
	ECMA-419 Crypto MAC — KCL software backend: HMAC (RFC 2104)
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include <stddef.h>
#include "mc.xs.h"			// for xsID_* values

#include "mac-kcl.h"

/*
	HMAC core
*/

uint32_t xs_crypt_hmacSize(const DigestAlgEntry *entry)
{
	return offsetof(HMACRecord, ctx) + 3 * entry->ctxSize;
}

static void hmacPadState(HMAC h, union DigestCtx *ctx, const uint8_t *key, uint32_t keyLen, uint8_t pad)
{
	uint8_t block[128];		// the largest block size (SHA-384, SHA-512)
	uint32_t i;

	for (i = 0; i < h->blockSize; i++)
		block[i] = ((i < keyLen) ? key[i] : 0) ^ pad;
	xs_crypt_digestCreate(h->alg, ctx);
	xs_crypt_digestUpdate(h->alg, ctx, block, h->blockSize);
	c_memset(block, 0, sizeof(block));
}

void xs_crypt_hmacInit(HMAC h, const DigestAlgEntry *entry, const void *key, uint32_t keyLen)
{
	uint8_t hashed[64];		// a key longer than the block is replaced by its hash

	h->alg = entry->alg;
	h->outputSize = entry->outputSize;
	h->blockSize = entry->blockSize;
	h->ctxSize = entry->ctxSize;

	if (keyLen > entry->blockSize) {
		union DigestCtx *ctx = xs_crypt_hmacContext(h, 0);
		xs_crypt_digestCreate(entry->alg, ctx);
		xs_crypt_digestUpdate(entry->alg, ctx, key, keyLen);
		xs_crypt_digestFinish(entry->alg, ctx, hashed);
		key = hashed;
		keyLen = entry->outputSize;
	}
	hmacPadState(h, xs_crypt_hmacContext(h, 1), key, keyLen, 0x36);
	hmacPadState(h, xs_crypt_hmacContext(h, 2), key, keyLen, 0x5c);
	c_memset(hashed, 0, sizeof(hashed));
	xs_crypt_hmacReset(h);
}

void xs_crypt_hmacUpdate(HMAC h, const void *data, uint32_t size)
{
	xs_crypt_digestUpdate(h->alg, xs_crypt_hmacContext(h, 0), data, size);
}

void xs_crypt_hmacFinish(HMAC h, uint8_t *out)
{
	union DigestCtx ctx;
	uint8_t inner[64];

	c_memcpy(&ctx, xs_crypt_hmacContext(h, 0), h->ctxSize);
	xs_crypt_digestFinish(h->alg, &ctx, inner);
	c_memcpy(&ctx, xs_crypt_hmacContext(h, 2), h->ctxSize);
	xs_crypt_digestUpdate(h->alg, &ctx, inner, h->outputSize);
	xs_crypt_digestFinish(h->alg, &ctx, out);
}

void xs_crypt_hmacReset(HMAC h)
{
	c_memcpy(xs_crypt_hmacContext(h, 0), xs_crypt_hmacContext(h, 1), h->ctxSize);
}

/*
	MAC class
*/

void xs_mac_destructor(void *data)
{
	// this space intentionally left blank
}

void xs_mac_constructor(xsMachine *the)
{
	xsmcVars(1);
	if (!xsmcGet(xsVar(0), xsArg(0), xsID_algorithm))
		xsUnknownError("algorithm required");
	if (0 != c_strcmp(xsmcToString(xsVar(0)), "HMAC"))
		xsUnknownError("unsupported algorithm");

	if (!xsmcGet(xsVar(0), xsArg(0), xsID_hash))
		xsUnknownError("hash required");
	const DigestAlgEntry *entry = xs_crypt_digestAlgorithm(xsmcToString(xsVar(0)));
	if (!entry)
		xsRangeError("unsupported hash");

	if (!xsmcGet(xsVar(0), xsArg(0), xsID_key))
		xsUnknownError("key required");
	HMAC h = xsmcSetHostChunk(xsThis, NULL, xs_crypt_hmacSize(entry));

	void *key;
	xsUnsignedValue keyLen;
	xsmcGetBufferReadable(xsVar(0), &key, &keyLen);		// after the allocation, which can move the key (XS in C, Memory Safety)
	xs_crypt_hmacInit(h, entry, key, keyLen);
}

void xs_mac_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis))
		return;
	HMAC h = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	c_memset(h->ctx, 0, 3 * h->ctxSize);		// the key-derived states
	xsmcSetHostData(xsThis, NULL);	// downgrade to data slot so subsequent calls throw SyntaxError
}

void xs_mac_write(xsMachine *the)
{
	HMAC h = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	void *data;
	xsUnsignedValue size;

	xsmcGetBufferReadable(xsArg(0), &data, &size);
	xs_crypt_hmacUpdate(h, data, (uint32_t)size);
}

void xs_mac_read(xsMachine *the)
{
	HMAC h = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	uint8_t *out;

	if (xsmcArgc > 0) {
		xsUnsignedValue outLen;
		xsmcGetBufferWritable(xsArg(0), (void **)&out, &outLen);
		if (outLen < h->outputSize)
			xsRangeError("buffer too small");
		xsmcSetInteger(xsResult, h->outputSize);
	}
	else
		out = xsmcSetArrayBuffer(xsResult, NULL, h->outputSize);

	h = xsmcGetHostChunk(xsThis);
	xs_crypt_hmacFinish(h, out);
}

void xs_mac_reset(xsMachine *the)
{
	HMAC h = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	xs_crypt_hmacReset(h);
}

void xs_mac_verify(xsMachine *the)
{
	HMAC h = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	void *tag;
	xsUnsignedValue tagLen;
	uint8_t expected[64];
	uint8_t diff = 0;
	uint32_t i;

	xsmcGetBufferReadable(xsArg(0), &tag, &tagLen);
	h = xsmcGetHostChunk(xsThis);
	xs_crypt_hmacFinish(h, expected);
	if (tagLen == h->outputSize) {		// constant time over the tag
		for (i = 0; i < h->outputSize; i++)
			diff |= expected[i] ^ ((uint8_t *)tag)[i];
	}
	else
		diff = 1;
	c_memset(expected, 0, sizeof(expected));
	xsmcSetBoolean(xsResult, 0 == diff);
}

void xs_mac_get_hash(xsMachine *the)
{
	HMAC h = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	static const char *names[] = { "MD5", "SHA-1", "SHA-224", "SHA-256", "SHA-384", "SHA-512" };
	xsmcSetString(xsResult, (char *)names[h->alg]);
}

void xs_mac_get_blockSize(xsMachine *the)
{
	HMAC h = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	xsmcSetInteger(xsResult, h->blockSize);
}

void xs_mac_get_outputSize(xsMachine *the)
{
	HMAC h = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	xsmcSetInteger(xsResult, h->outputSize);
}

void xs_mac_supports(xsMachine *the)
{
	xsmcVars(1);
	int supported = 0;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_algorithm) && (xsStringType == xsmcTypeOf(xsVar(0))) && (0 == c_strcmp(xsmcToString(xsVar(0)), "HMAC"))) {
		supported = 1;
		if (xsmcGet(xsVar(0), xsArg(0), xsID_hash)) {
			const DigestAlgEntry *entry = (xsStringType == xsmcTypeOf(xsVar(0))) ? xs_crypt_digestAlgorithm(xsmcToString(xsVar(0))) : NULL;
			supported = NULL != entry;
		}
	}
	xsmcSetBoolean(xsResult, supported);
}

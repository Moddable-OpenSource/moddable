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
	ECMA-419 Crypto MAC — macOS: HMAC via CryptoKit (SHA-224 via CommonCrypto)
	read() yields the tag without disturbing the state, as on the other backends: CryptoKit's HMAC finalizes a copy,
	and CommonCrypto's context is plain memory to copy
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_* values

#include <CommonCrypto/CommonHMAC.h>
#include "xscryptokit.h"
#include "digest-cryptokit.h"

typedef struct {
	union {
		void *ck;
		struct {
			CCHmacContext running;
			CCHmacContext initial;		// after the key: the reset state
		} cc;
	} op;
	uint8_t provider;
	uint8_t hashIndex;
	uint16_t outputSize;
	uint16_t blockSize;
} MACRecord, *MAC;

static void macFinish(MAC mac, uint8_t *out)
{
	if (kDigestCommonCrypto == mac->provider) {
		CCHmacContext clone = mac->op.cc.running;
		CCHmacFinal(&clone, out);
		c_memset(&clone, 0, sizeof(clone));
	}
	else
		ckHashFinalize(mac->op.ck, out);
}

void xs_mac_destructor(void *data)
{
	MAC mac = data;
	if (mac) {
		if ((kDigestCryptoKit == mac->provider) && mac->op.ck)
			ckHashFree(mac->op.ck);
		c_memset(mac, 0, sizeof(MACRecord));		// key material in the CommonCrypto contexts
	}
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
	void *key;
	xsUnsignedValue keyLen;
	xsmcGetBufferReadable(xsVar(0), &key, &keyLen);

	MACRecord init;
	c_memset(&init, 0, sizeof(init));
	init.provider = entry->provider;
	init.outputSize = entry->outputSize;
	init.blockSize = entry->blockSize;
	init.hashIndex = (uint8_t)(entry - xs_crypt_digestAlgorithms);

	if (kDigestCommonCrypto == entry->provider) {
		CCHmacInit(&init.op.cc.initial, kCCHmacAlgSHA224, key, keyLen);
		init.op.cc.running = init.op.cc.initial;
	}
	else {
		init.op.ck = ckHMACCreate(entry->alg, key, keyLen);
		if (!init.op.ck)
			xsUnknownError("CryptoKit HMAC unavailable");
	}

	xsmcSetHostChunk(xsThis, &init, sizeof(init));
	c_memset(&init, 0, sizeof(init));
}

void xs_mac_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis))
		return;
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	xs_mac_destructor(mac);
	xsmcSetHostData(xsThis, NULL);	// downgrade to data slot so subsequent calls throw SyntaxError
}

void xs_mac_write(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	void *data;
	xsUnsignedValue size;

	xsmcGetBufferReadable(xsArg(0), &data, &size);
	mac = xsmcGetHostChunk(xsThis);

	if (kDigestCommonCrypto == mac->provider)
		CCHmacUpdate(&mac->op.cc.running, data, size);
	else
		ckHashUpdate(mac->op.ck, data, size);
}

void xs_mac_read(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	uint8_t *out;

	if (xsmcArgc > 0) {
		xsUnsignedValue outLen;
		xsmcGetBufferWritable(xsArg(0), (void **)&out, &outLen);
		if (outLen < mac->outputSize)
			xsRangeError("buffer too small");
		xsmcSetInteger(xsResult, mac->outputSize);
	}
	else
		out = xsmcSetArrayBuffer(xsResult, NULL, mac->outputSize);

	mac = xsmcGetHostChunk(xsThis);
	macFinish(mac, out);
}

void xs_mac_reset(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	if (kDigestCommonCrypto == mac->provider)
		mac->op.cc.running = mac->op.cc.initial;
	else
		ckHashReset(mac->op.ck);
}

void xs_mac_verify(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	void *tag;
	xsUnsignedValue tagLen;
	uint8_t expected[64];
	uint8_t diff = 0;

	xsmcGetBufferReadable(xsArg(0), &tag, &tagLen);
	mac = xsmcGetHostChunk(xsThis);
	macFinish(mac, expected);
	if (tagLen == mac->outputSize) {		// constant time over the tag
		for (uint32_t i = 0; i < mac->outputSize; i++)
			diff |= expected[i] ^ ((uint8_t *)tag)[i];
	}
	else
		diff = 1;
	c_memset(expected, 0, sizeof(expected));
	xsmcSetBoolean(xsResult, 0 == diff);
}

void xs_mac_get_hash(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	xsmcSetString(xsResult, (char *)xs_crypt_digestAlgorithms[mac->hashIndex].name);
}

void xs_mac_get_blockSize(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	xsmcSetInteger(xsResult, mac->blockSize);
}

void xs_mac_get_outputSize(xsMachine *the)
{
	MAC mac = xsmcGetHostChunkValidate(xsThis, xs_mac_destructor);
	xsmcSetInteger(xsResult, mac->outputSize);
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

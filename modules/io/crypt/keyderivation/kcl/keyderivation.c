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
	ECMA-419 Crypto KeyDerivation — KCL software backend: HKDF (RFC 5869), its Extract and Expand steps, PBKDF2 (RFC 8018)
	over the HMAC core of the MAC class. The output is produced one hash block at a time as read() asks for it.
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include <stddef.h>
#include "mc.xs.h"			// for xsID_* values

#include "mac-kcl.h"
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
	uint8_t algorithm;
	uint8_t started;		// a block has been produced
	uint16_t position;		// bytes of block already read
	uint32_t blockNumber;	// of the block in block[]: HKDF 1..255, PBKDF2 1..
	uint32_t iterations;	// PBKDF2
	uint16_t extraLen;		// HKDF info or PBKDF2 salt, stored after the HMAC record
	uint16_t hmacSize;
	uint8_t block[64];		// the current output block (HKDF-Extract: the PRK)
	uint8_t data[];			// HMACRecord, then extra
} KDFRecord, *KDF;

#define kdfHMAC(kdf) ((HMAC)(kdf)->data)
#define kdfExtra(kdf) ((kdf)->data + (kdf)->hmacSize)

static void kdfNextBlock(xsMachine *the, KDF kdf)
{
	HMAC h = kdfHMAC(kdf);
	uint32_t n = kdf->blockNumber + 1;
	uint8_t counter[4];

	switch (kdf->algorithm) {
#if MODDEF_CRYPT_KEYDERIVATION_HKDF
		case kHKDF:
		case kHKDFExpand:		// T(n) = HMAC(PRK, T(n-1) | info | n)
			if (n > 255)
				xsRangeError("HKDF output limit");
			xs_crypt_hmacReset(h);
			if (kdf->started)
				xs_crypt_hmacUpdate(h, kdf->block, h->outputSize);
			xs_crypt_hmacUpdate(h, kdfExtra(kdf), kdf->extraLen);
			counter[0] = (uint8_t)n;
			xs_crypt_hmacUpdate(h, counter, 1);
			xs_crypt_hmacFinish(h, kdf->block);
			break;

		case kHKDFExtract:		// the PRK was produced at construction
			xsRangeError("HKDF-Extract output limit");
			break;
#endif
#if MODDEF_CRYPT_KEYDERIVATION_PBKDF2
		case kPBKDF2: {		// T(n) = U1 ^ U2 ^ ... ^ Uc, U1 = HMAC(P, S | n), Uj = HMAC(P, Uj-1)
			uint8_t u[64];
			uint32_t j, k;
			counter[0] = (uint8_t)(n >> 24); counter[1] = (uint8_t)(n >> 16); counter[2] = (uint8_t)(n >> 8); counter[3] = (uint8_t)n;
			xs_crypt_hmacReset(h);
			xs_crypt_hmacUpdate(h, kdfExtra(kdf), kdf->extraLen);
			xs_crypt_hmacUpdate(h, counter, 4);
			xs_crypt_hmacFinish(h, u);
			c_memcpy(kdf->block, u, h->outputSize);
			for (j = 1; j < kdf->iterations; j++) {
				xs_crypt_hmacReset(h);
				xs_crypt_hmacUpdate(h, u, h->outputSize);
				xs_crypt_hmacFinish(h, u);
				for (k = 0; k < h->outputSize; k++)
					kdf->block[k] ^= u[k];
			}
			c_memset(u, 0, sizeof(u));
			} break;
#endif
	}
	kdf->blockNumber = n;
	kdf->position = 0;
	kdf->started = 1;
}

void xs_keyderivation_destructor(void *data)
{
	// this space intentionally left blank
}

void xs_keyderivation_constructor(xsMachine *the)
{
	xsmcVars(4);		// 0: scratch; 1: key, 2: salt, 3: info, held in slots until their pointers are taken (XS in C, Memory Safety)

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

	// the options into slots and their lengths: every get, coercion and allocation below can run the collector, which moves
	// buffers, so the pointers are taken only after the last of them, just before use
	if (!xsmcGet(xsVar(1), xsArg(0), xsID_key))
		xsUnknownError("key required");
	void *key, *salt = NULL, *info = NULL;
	xsUnsignedValue keyLen, saltLen = 0, infoLen = 0;
	xsmcGetBufferReadable(xsVar(1), &key, &keyLen);

	uint32_t iterations = 0;
	int hasSalt = xsmcGet(xsVar(2), xsArg(0), xsID_salt);
	if (hasSalt)
		xsmcGetBufferReadable(xsVar(2), &salt, &saltLen);
	int hasInfo = xsmcGet(xsVar(3), xsArg(0), xsID_info);
	if (hasInfo)
		xsmcGetBufferReadable(xsVar(3), &info, &infoLen);
	int hasIterations = xsmcGet(xsVar(0), xsArg(0), xsID_iterations);
	if (hasIterations) {
		double value = xsmcToNumber(xsVar(0));
		if (!(value >= 1) || (value > 4294967295.0))
			xsRangeError("invalid iterations");
		iterations = (uint32_t)value;
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

	// the HMAC record is followed by the bytes mixed into every block: HKDF info or PBKDF2 salt
	uint32_t extraLen = (kPBKDF2 == algorithm) ? saltLen : infoLen;
	if (extraLen > 0xFFFF)
		xsRangeError("salt or info too long");
	uint32_t hmacSize = xs_crypt_hmacSize(entry);
	KDF kdf = xsmcSetHostChunk(xsThis, NULL, offsetof(KDFRecord, data) + hmacSize + extraLen);

	xsmcGetBufferReadable(xsVar(1), &key, &keyLen);		// the pointers, after the allocation: nothing below can move them
	if (hasSalt)
		xsmcGetBufferReadable(xsVar(2), &salt, &saltLen);
	if (hasInfo)
		xsmcGetBufferReadable(xsVar(3), &info, &infoLen);
	const void *extra = (kPBKDF2 == algorithm) ? salt : info;
	kdf->algorithm = algorithm;
	kdf->iterations = iterations;
	kdf->hmacSize = hmacSize;
	kdf->extraLen = extraLen;
	if (extraLen)
		c_memcpy(kdfExtra(kdf), extra, extraLen);

	HMAC h = kdfHMAC(kdf);
	if ((kHKDF == algorithm) || (kHKDFExtract == algorithm)) {		// PRK = HMAC(salt, IKM); a missing salt is hashLen zeros
		uint8_t zeros[64];
		if (!hasSalt || (0 == saltLen)) {
			c_memset(zeros, 0, entry->outputSize);
			salt = zeros;
			saltLen = entry->outputSize;
		}
		xs_crypt_hmacInit(h, entry, salt, saltLen);
		xs_crypt_hmacUpdate(h, key, keyLen);
		xs_crypt_hmacFinish(h, kdf->block);
		if (kHKDFExtract == algorithm) {
			kdf->started = 1;		// the PRK is the whole output
			kdf->blockNumber = 1;
			return;
		}
		xs_crypt_hmacInit(h, entry, kdf->block, entry->outputSize);		// re-key with the PRK for Expand
		c_memset(kdf->block, 0, sizeof(kdf->block));
	}
	else
		xs_crypt_hmacInit(h, entry, key, keyLen);		// HKDF-Expand: the key is the PRK; PBKDF2: the password
}

void xs_keyderivation_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis))
		return;
	KDF kdf = xsmcGetHostChunkValidate(xsThis, xs_keyderivation_destructor);
	c_memset(kdf->data, 0, kdf->hmacSize + kdf->extraLen);
	c_memset(kdf->block, 0, sizeof(kdf->block));
	xsmcSetHostData(xsThis, NULL);	// downgrade to data slot so subsequent calls throw SyntaxError
}

// bytes the algorithm can still produce: HKDF and HKDF-Expand 255 blocks, HKDF-Extract the one PRK block, PBKDF2 unlimited
static uint32_t kdfAvailable(KDF kdf)
{
	uint32_t outputSize = kdfHMAC(kdf)->outputSize;
	uint32_t produced = kdf->started ? (((kdf->blockNumber - 1) * outputSize) + kdf->position) : 0;
	uint32_t limit;
	switch (kdf->algorithm) {
		case kHKDFExtract: limit = outputSize; break;
		case kPBKDF2: return 0x7FFFFFFF;
		default: limit = 255 * outputSize; break;
	}
	return limit - produced;
}

void xs_keyderivation_read(xsMachine *the)
{
	KDF kdf = xsmcGetHostChunkValidate(xsThis, xs_keyderivation_destructor);
	uint32_t available = kdfAvailable(kdf);
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
	uint16_t outputSize = kdfHMAC(kdf)->outputSize;
	while (count) {
		if (!kdf->started || (kdf->position >= outputSize)) {
			kdfNextBlock(the, kdf);
			kdf = xsmcGetHostChunk(xsThis);
		}
		uint32_t use = outputSize - kdf->position;
		if (use > count)
			use = count;
		c_memcpy(out, kdf->block + kdf->position, use);
		kdf->position += use;
		out += use;
		count -= use;
	}
}

void xs_keyderivation_get_algorithm(xsMachine *the)
{
	KDF kdf = xsmcGetHostChunkValidate(xsThis, xs_keyderivation_destructor);
	xsmcSetString(xsResult, (char *)gAlgorithmNames[kdf->algorithm]);
}

void xs_keyderivation_get_hash(xsMachine *the)
{
	KDF kdf = xsmcGetHostChunkValidate(xsThis, xs_keyderivation_destructor);
	static const char *names[] = { "MD5", "SHA-1", "SHA-224", "SHA-256", "SHA-384", "SHA-512" };
	xsmcSetString(xsResult, (char *)names[kdfHMAC(kdf)->alg]);
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

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
	ECMA-419 Crypto KeyDerivation — macOS: HKDF, HKDF-Extract and HKDF-Expand via CryptoKit; PBKDF2 via CommonCrypto

	Both APIs are one shot where read() is a stream, so each read derives the output up to the new position and
	returns the tail. The outputs are prefix-stable (HKDF's T(i) and PBKDF2's blocks depend only on their index), so
	the bytes a read returns are the bytes a single derivation of the same length would hold there. The inputs are kept
	for that; a read costs a derivation of everything read so far.
	CryptoKit has no SHA-224 and CommonCrypto's PBKDF2 has no MD5: those pairings are unsupported.
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_* values

#include <CommonCrypto/CommonKeyDerivation.h>
#include <CommonCrypto/CommonCryptoError.h>
#include "xscryptokit.h"
#include "digest-cryptokit.h"
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
	uint8_t hashIndex;
	uint32_t consumed;		// bytes returned so far
	uint32_t limit;			// the algorithm's output limit in bytes; 0 for none
	uint32_t iterations;
	uint8_t *key;			// c_malloc copies of the inputs
	uint8_t *salt;
	uint8_t *info;
	uint32_t keyLen;
	uint32_t saltLen;
	uint32_t infoLen;
} KDFRecord, *KDF;

// CommonCrypto's PBKDF2 pseudorandom function for a hash, or 0
static CCPseudoRandomAlgorithm pbkdf2PRF(const DigestAlgEntry *entry)
{
	if (kDigestCommonCrypto == entry->provider)
		return kCCPRFHmacAlgSHA224;
	if (kDigestCryptoKit != entry->provider)
		return 0;
	switch (entry->alg) {
		case ckHashSHA1: return kCCPRFHmacAlgSHA1;
		case ckHashSHA256: return kCCPRFHmacAlgSHA256;
		case ckHashSHA384: return kCCPRFHmacAlgSHA384;
		case ckHashSHA512: return kCCPRFHmacAlgSHA512;
	}
	return 0;
}

static int hashSupported(uint8_t algorithm, const DigestAlgEntry *entry)
{
	if (kPBKDF2 == algorithm)
		return 0 != pbkdf2PRF(entry);
	return kDigestCryptoKit == entry->provider;
}

static void freeInputs(KDF kdf)
{
	if (kdf->key) {
		c_memset(kdf->key, 0, kdf->keyLen);
		c_free(kdf->key);
	}
	if (kdf->salt)
		c_free(kdf->salt);
	if (kdf->info)
		c_free(kdf->info);
	kdf->key = kdf->salt = kdf->info = NULL;
}

static uint8_t *copyInput(xsMachine *the, const void *data, uint32_t length)
{
	uint8_t *copy = c_malloc(length ? length : 1);
	if (!copy)
		xsUnknownError("no memory");
	if (length)
		c_memcpy(copy, data, length);
	return copy;
}

void xs_keyderivation_destructor(void *data)
{
	if (data)
		freeInputs(data);
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
	if (!entry || !hashSupported(algorithm, entry))
		xsRangeError("unsupported hash");

	if (!xsmcGet(xsVar(0), xsArg(0), xsID_key))
		xsUnknownError("key required");
	void *key;
	xsUnsignedValue keyLen;
	xsmcGetBufferReadable(xsVar(0), &key, &keyLen);

	void *salt = NULL, *info = NULL;
	xsUnsignedValue saltLen = 0, infoLen = 0;
	uint32_t iterations = 0;
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

	KDFRecord init;
	c_memset(&init, 0, sizeof(init));
	init.algorithm = algorithm;
	init.hashIndex = (uint8_t)(entry - xs_crypt_digestAlgorithms);
	init.iterations = iterations;
	init.limit = (kHKDFExtract == algorithm) ? entry->outputSize : (kPBKDF2 == algorithm) ? 0 : 255 * entry->outputSize;

	// the inputs are copied before anything can throw, so a failure frees them all
	xsmcSetHostChunk(xsThis, &init, sizeof(init));
	KDF kdf = xsmcGetHostChunk(xsThis);
	kdf->key = copyInput(the, key, keyLen);
	kdf->keyLen = keyLen;
	kdf = xsmcGetHostChunk(xsThis);
	kdf->salt = copyInput(the, salt, saltLen);
	kdf->saltLen = saltLen;
	kdf = xsmcGetHostChunk(xsThis);
	kdf->info = copyInput(the, info, infoLen);
	kdf->infoLen = infoLen;
}

void xs_keyderivation_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis))
		return;
	KDF kdf = xsmcGetHostChunkValidate(xsThis, xs_keyderivation_destructor);
	freeInputs(kdf);
	xsmcSetHostData(xsThis, NULL);	// downgrade to data slot so subsequent calls throw SyntaxError
}

// the first count bytes of the output into out
static void derive(xsMachine *the, KDF kdf, uint8_t *out, uint32_t count)
{
	const DigestAlgEntry *entry = &xs_crypt_digestAlgorithms[kdf->hashIndex];
	if (kPBKDF2 == kdf->algorithm) {
		if (kCCSuccess != CCKeyDerivationPBKDF(kCCPBKDF2, (const char *)kdf->key, kdf->keyLen, kdf->salt, kdf->saltLen, pbkdf2PRF(entry), kdf->iterations, out, count))
			xsUnknownError("CCKeyDerivationPBKDF failed");
		return;
	}
	int32_t mode = (kHKDF == kdf->algorithm) ? ckHKDFFull : (kHKDFExtract == kdf->algorithm) ? ckHKDFExtract : ckHKDFExpand;
	if (count != (uint32_t)ckHKDF(entry->alg, mode, kdf->key, kdf->keyLen, kdf->salt, kdf->saltLen, kdf->info, kdf->infoLen, out, count))
		xsUnknownError("HKDF failed");
}

void xs_keyderivation_read(xsMachine *the)
{
	KDF kdf = xsmcGetHostChunkValidate(xsThis, xs_keyderivation_destructor);
	uint32_t available = kdf->limit ? (kdf->limit - kdf->consumed) : 0xFFFFFFFF;		// the remaining capacity

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

	// HKDF-Extract has one output: always derive all of it and return the slice
	uint32_t total = kdf->consumed + count;
	uint32_t deriveLen = (kHKDFExtract == kdf->algorithm) ? kdf->limit : total;
	uint8_t *whole = c_malloc(deriveLen ? deriveLen : 1);
	if (!whole)
		xsUnknownError("no memory");
	derive(the, kdf, whole, deriveLen);		//@@ throws leak whole
	c_memcpy(out, whole + kdf->consumed, count);
	c_memset(whole, 0, deriveLen);
	c_free(whole);
	kdf->consumed = total;
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
		uint8_t algorithm;
		for (algorithm = 0; algorithm < 4; algorithm++) {
			if (gAlgorithmNames[algorithm] && (0 == c_strcmp(name, gAlgorithmNames[algorithm]))) {
				supported = 1;
				break;
			}
		}
		if (supported && xsmcGet(xsVar(0), xsArg(0), xsID_hash)) {
			const DigestAlgEntry *entry = (xsStringType == xsmcTypeOf(xsVar(0))) ? xs_crypt_digestAlgorithm(xsmcToString(xsVar(0))) : NULL;
			supported = entry && hashSupported(algorithm, entry);
		}
	}
	xsmcSetBoolean(xsResult, supported);
}

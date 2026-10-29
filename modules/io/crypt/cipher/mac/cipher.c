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
	ECMA-419 Crypto Cipher — macOS: AES-ECB, AES-CBC and AES-CTR via CommonCrypto; AES-GCM and ChaCha20-Poly1305 via CryptoKit

	CryptoKit's authenticated modes are one shot with a 16-byte tag: an operation is a single encrypt or decrypt call
	(more is a RangeError), tagLength is 16, and a GCM nonce is at least 12 bytes. ChaCha20 without Poly1305 is in
	neither API. CommonCrypto does not process ECB and CBC in place, so overlapping input is staged through a buffer.
	CommonCrypto's CTR mode increments only the low 64 bits of the counter block; this Standard increments all 128,
	so CTR runs here over CommonCrypto's ECB block function.
	PKCS#7 is applied here, as on the other backends, so that the output size is known before allocation; decrypt
	defers a possible padding block across calls with more: true.
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_* values
#include "mc.defines.h"

#include <CommonCrypto/CommonCryptor.h>
#include "xscryptokit.h"

enum {
	kAlgAESECB,
	kAlgAESCBC,
	kAlgAESCTR,
	kAlgAESGCM,
	kAlgChaCha20Poly1305,
};

enum {
	kDirectionEncrypt,
	kDirectionDecrypt,
};

enum {
	kStateReady,
	kStateMore,
	kStateDone,
};

typedef struct {
	const char *name;
	uint8_t alg;
	uint8_t blockSize;
	uint8_t aead;
} CipherAlgEntry;

// a build may omit algorithms (manifest "defines": crypt.cipher.<name> 0), the same names as the other backends
#ifndef MODDEF_CRYPT_CIPHER_AES_ECB
	#define MODDEF_CRYPT_CIPHER_AES_ECB 1
#endif
#ifndef MODDEF_CRYPT_CIPHER_AES_CBC
	#define MODDEF_CRYPT_CIPHER_AES_CBC 1
#endif
#ifndef MODDEF_CRYPT_CIPHER_AES_CTR
	#define MODDEF_CRYPT_CIPHER_AES_CTR 1
#endif
#ifndef MODDEF_CRYPT_CIPHER_AES_GCM
	#define MODDEF_CRYPT_CIPHER_AES_GCM 1
#endif
#ifndef MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
	#define MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305 1
#endif

static const CipherAlgEntry gCipherAlgs[] = {
#if MODDEF_CRYPT_CIPHER_AES_ECB
	{ "AES-ECB",           kAlgAESECB,           16, 0 },
#endif
#if MODDEF_CRYPT_CIPHER_AES_CBC
	{ "AES-CBC",           kAlgAESCBC,           16, 0 },
#endif
#if MODDEF_CRYPT_CIPHER_AES_CTR
	{ "AES-CTR",           kAlgAESCTR,           16, 0 },
#endif
#if MODDEF_CRYPT_CIPHER_AES_GCM
	{ "AES-GCM",           kAlgAESGCM,           16, 1 },
#endif
#if MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
	{ "ChaCha20-Poly1305", kAlgChaCha20Poly1305, 64, 1 },
#endif
	{ NULL, 0, 0, 0 }
};

#define kMaxIV 32

typedef struct {
	uint8_t alg;
	uint8_t direction;
	uint8_t padding;		// 1 for pkcs7 (ECB, CBC)
	uint8_t aead;
	uint8_t blockSize;
	uint8_t keySize;
	uint8_t tagLength;		// 0 for non-AEAD
	uint8_t state;
	uint8_t heldLen;
	uint8_t ivLen;
	CCCryptorRef cryptor;	// the running operation of a block mode; the block function of CTR
	uint8_t ksOff;			// CTR: bytes of ks[] already consumed
	uint8_t key[32];
	uint8_t iv[kMaxIV];
	uint8_t chain[16];		// CTR: the counter block
	uint8_t ks[16];			// CTR: keystream block
	uint8_t held[16];
	uint8_t lastCt[16];		// CBC decrypt: last ciphertext block fed (the chaining value), for the padding peek
	uint8_t *aad;
	uint32_t aadLen;
} CipherRecord, *Cipher;

static void endOperation(Cipher cipher)
{
	if (cipher->cryptor) {
		CCCryptorRelease(cipher->cryptor);
		cipher->cryptor = NULL;
	}
}

// begin an operation of a block or counter mode; the authenticated modes run when the data is complete
static void startOperation(xsMachine *the, Cipher cipher)
{
	endOperation(cipher);
	cipher->state = kStateReady;
	cipher->heldLen = 0;
	if (cipher->aead)
		return;

	CCMode mode = (kAlgAESCBC == cipher->alg) ? kCCModeCBC : kCCModeECB;		// CTR: the forward block function
	CCOperation op = ((kDirectionEncrypt == cipher->direction) || (kAlgAESCTR == cipher->alg)) ? kCCEncrypt : kCCDecrypt;
	CCCryptorStatus status = CCCryptorCreateWithMode(op, mode, kCCAlgorithmAES, ccNoPadding, (kAlgAESCBC == cipher->alg) ? cipher->iv : NULL, cipher->key, cipher->keySize, NULL, 0, 0, 0, &cipher->cryptor);
	if (kCCSuccess != status) {
		cipher->cryptor = NULL;
		xsUnknownError("CCCryptorCreateWithMode failed %d", (int)status);
	}
	if (kAlgAESCBC == cipher->alg)
		c_memcpy(cipher->lastCt, cipher->iv, 16);
	if (kAlgAESCTR == cipher->alg) {
		c_memcpy(cipher->chain, cipher->iv, 16);
		cipher->ksOff = 16;
	}
}

static int overlaps(const uint8_t *a, uint32_t aLen, const uint8_t *b, uint32_t bLen)
{
	return aLen && bLen && (a < b + bLen) && (b < a + aLen);
}

static void cryptorUpdate(xsMachine *the, Cipher cipher, const uint8_t *in, uint8_t *out, uint32_t count)
{
	size_t produced = 0;
	CCCryptorStatus status = CCCryptorUpdate(cipher->cryptor, in, count, out, count, &produced);
	if ((kCCSuccess != status) || (produced != count)) {
		endOperation(cipher);
		cipher->state = kStateDone;
		if (kCCSuccess != status)
			xsUnknownError("CCCryptorUpdate failed %d", (int)status);
		xsUnknownError("internal: update produced %d of %d", (int)produced, (int)count);
	}
}

// CTR: any count; in and out may be the same bytes. The full 128-bit counter block increments.
static void counterFeed(xsMachine *the, Cipher cipher, const uint8_t *in, uint8_t *out, uint32_t count)
{
	while (count) {
		if (16 == cipher->ksOff) {
			cryptorUpdate(the, cipher, cipher->chain, cipher->ks, 16);
			for (int i = 15; i >= 0; i--) {
				if (++cipher->chain[i])
					break;
			}
			cipher->ksOff = 0;
		}
		uint32_t use = 16 - cipher->ksOff;
		if (use > count)
			use = count;
		for (uint32_t i = 0; i < use; i++)
			out[i] = in[i] ^ cipher->ks[cipher->ksOff + i];
		cipher->ksOff += use;
		in += use;
		out += use;
		count -= use;
	}
}

// one update. in and out may be the same bytes: the block modes are staged, since CommonCrypto does not run them in place
static void feed(xsMachine *the, Cipher cipher, const uint8_t *in, uint8_t *out, uint32_t count)
{
	if (!count)
		return;
	if (kAlgAESCTR == cipher->alg) {
		counterFeed(the, cipher, in, out, count);
		return;
	}
	if ((kAlgAESCBC == cipher->alg) && (kDirectionDecrypt == cipher->direction) && (count >= 16))
		c_memcpy(cipher->lastCt, in + count - 16, 16);		// before an in-place update overwrites it
	if (!overlaps(in, count, out, count)) {
		cryptorUpdate(the, cipher, in, out, count);
		return;
	}
	uint8_t staged[64];
	while (count) {
		uint32_t use = (count < sizeof(staged)) ? count : sizeof(staged);
		c_memcpy(staged, in, use);
		cryptorUpdate(the, cipher, staged, out, use);
		in += use;
		out += use;
		count -= use;
	}
}

// feed held || data[0..dataCount) keeping every update 16-byte aligned (the last update of an operation may be unaligned).
// Writes heldLen + dataCount bytes to out.
static void feedHeld(xsMachine *the, Cipher cipher, const uint8_t *data, uint32_t dataCount, uint8_t *out)
{
	uint8_t heldLen = cipher->heldLen;
	if (!heldLen) {
		feed(the, cipher, data, out, dataCount);
		return;
	}
	if (0 == (heldLen % 16)) {
		feed(the, cipher, cipher->held, out, heldLen);
		feed(the, cipher, data, out + heldLen, dataCount);
		return;
	}
	uint8_t block[16];
	uint32_t take = 16 - heldLen;
	if (take > dataCount)
		take = dataCount;
	c_memcpy(block, cipher->held, heldLen);
	c_memcpy(block + heldLen, data, take);
	feed(the, cipher, block, out, heldLen + take);
	feed(the, cipher, data + take, out + heldLen + take, dataCount - take);
}

static const uint8_t gEmpty[1] = {0};

static void copyRange(uint8_t *dst, const uint8_t *held, uint32_t heldLen, const uint8_t *data, uint32_t start, uint32_t count)
{
	while (count--) {
		*dst++ = (start < heldLen) ? held[start] : data[start - heldLen];
		start++;
	}
}

// PKCS#7 decrypt: learn the pad length from the last block without disturbing the running operation
static uint8_t peekPad(xsMachine *the, Cipher cipher, const uint8_t *prev, const uint8_t *last)
{
	uint8_t plain[16];
	size_t plainLen = 0;
	CCCryptorStatus status = CCCrypt(kCCDecrypt, kCCAlgorithmAES, kCCOptionECBMode, cipher->key, cipher->keySize, NULL, last, 16, plain, sizeof(plain), &plainLen);
	if ((kCCSuccess != status) || (16 != plainLen))
		xsUnknownError("CCCrypt failed %d", (int)status);
	if (kAlgAESCBC == cipher->alg) {
		for (int i = 0; i < 16; i++)
			plain[i] ^= prev[i];
	}
	uint8_t pad = plain[15];
	uint8_t bad = (0 == pad) | (pad > 16);
	for (int i = 0; i < 16; i++)
		bad |= (i >= 16 - pad) & (plain[i] != pad);
	if (bad)
		xsUnknownError("invalid padding");
	return pad;
}

static int32_t aeadAlg(Cipher cipher)
{
	return (kAlgAESGCM == cipher->alg) ? ckAEADGCM : ckAEADChaChaPoly;
}

static void setIV(xsMachine *the, Cipher cipher, const uint8_t *iv, uint32_t ivLen)
{
	switch (cipher->alg) {
		case kAlgAESECB:		// a property defined for the other modes, not accepted by ECB
			xsRangeError("iv not supported by algorithm");
		case kAlgAESCBC:
		case kAlgAESCTR:
			if (16 != ivLen)
				xsRangeError("iv must be 16 bytes");
			break;
		case kAlgAESGCM:
			if ((ivLen < 12) || (ivLen > kMaxIV))
				xsRangeError("iv must be 12 to 32 bytes on this platform");
			break;
		case kAlgChaCha20Poly1305:
			if (12 != ivLen)
				xsRangeError("iv must be 12 bytes");
			break;
	}
	c_memcpy(cipher->iv, iv, ivLen);
	cipher->ivLen = (uint8_t)ivLen;
}

static void setAdditionalData(xsMachine *the, Cipher cipher, const uint8_t *aad, uint32_t aadLen)
{
	uint8_t *copy = NULL;
	if (aadLen) {
		copy = c_malloc(aadLen);
		if (!copy)
			xsUnknownError("no memory");
		c_memcpy(copy, aad, aadLen);
	}
	if (cipher->aad)
		c_free(cipher->aad);
	cipher->aad = copy;
	cipher->aadLen = aadLen;
}

static void disposeCipher(Cipher cipher)
{
	endOperation(cipher);
	if (cipher->aad)
		c_free(cipher->aad);
	c_memset(cipher, 0, sizeof(CipherRecord));		// key material
	c_free(cipher);
}

/*
	class
*/

void xs_cipher_destructor(void *data)
{
	if (data)
		disposeCipher(data);
}

// the 16-byte tag only: CryptoKit's sealed box carries the full tag
static int validTagLength(uint8_t alg, int tagLength)
{
	return 16 == tagLength;
}

void xs_cipher_constructor(xsMachine *the)
{
	xsmcVars(1);
	if (!xsmcGet(xsVar(0), xsArg(0), xsID_algorithm))
		xsUnknownError("algorithm required");
	char *algorithm = xsmcToString(xsVar(0));
	const CipherAlgEntry *entry;
	for (entry = gCipherAlgs; entry->name; entry++) {
		if (0 == c_strcmp(algorithm, entry->name))
			break;
	}
	if (!entry->name)
		xsUnknownError("unsupported algorithm");

	Cipher cipher = c_calloc(1, sizeof(CipherRecord));
	if (!cipher)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, cipher);		// destructor owns it from here
	cipher->alg = entry->alg;
	cipher->blockSize = entry->blockSize;
	cipher->aead = entry->aead;

	if (!xsmcGet(xsVar(0), xsArg(0), xsID_direction))
		xsUnknownError("direction required");
	char *direction = xsmcToString(xsVar(0));
	if (0 == c_strcmp(direction, "encrypt"))
		cipher->direction = kDirectionEncrypt;
	else if (0 == c_strcmp(direction, "decrypt"))
		cipher->direction = kDirectionDecrypt;
	else
		xsRangeError("invalid direction");

	if (xsmcGet(xsVar(0), xsArg(0), xsID_padding)) {
		char *padding = xsmcToString(xsVar(0));
		if (0 == c_strcmp(padding, "pkcs7")) {
			if ((kAlgAESECB != cipher->alg) && (kAlgAESCBC != cipher->alg))
				xsRangeError("padding not supported by algorithm");
			cipher->padding = 1;
		}
		else if (0 != c_strcmp(padding, "none"))
			xsRangeError("invalid padding");
	}

	if (!cipher->aead && xsmcGet(xsVar(0), xsArg(0), xsID_tagLength))
		xsRangeError("tagLength not supported by algorithm");
	if (cipher->aead) {
		cipher->tagLength = 16;
		if (xsmcGet(xsVar(0), xsArg(0), xsID_tagLength)) {
			int tagLength = xsmcToInteger(xsVar(0));
			if (!validTagLength(cipher->alg, tagLength))
				xsRangeError("invalid tagLength");
			cipher->tagLength = (uint8_t)tagLength;
		}
	}

	if (xsmcGet(xsVar(0), xsArg(0), xsID_hash))		// RSA-OAEP's properties, not accepted by the symmetric algorithms
		xsRangeError("hash not supported by algorithm");
	if (xsmcGet(xsVar(0), xsArg(0), xsID_label))
		xsRangeError("label not supported by algorithm");

	if (!xsmcGet(xsVar(0), xsArg(0), xsID_key))
		xsUnknownError("key required");
	void *key;
	xsUnsignedValue keyLen;
	xsmcGetBufferReadable(xsVar(0), &key, &keyLen);
	if (kAlgChaCha20Poly1305 == cipher->alg) {
		if (32 != keyLen)
			xsRangeError("key must be 32 bytes");
	}
	else if ((16 != keyLen) && (24 != keyLen) && (32 != keyLen))
		xsRangeError("key must be 16, 24, or 32 bytes");
	c_memcpy(cipher->key, key, keyLen);
	cipher->keySize = (uint8_t)keyLen;

	if (xsmcGet(xsVar(0), xsArg(0), xsID_iv)) {
		void *iv;
		xsUnsignedValue ivLen;
		xsmcGetBufferReadable(xsVar(0), &iv, &ivLen);
		setIV(the, cipher, iv, ivLen);
	}
	else if (kAlgAESECB != cipher->alg)
		xsUnknownError("iv required");

	if (xsmcGet(xsVar(0), xsArg(0), xsID_additionalData)) {
		if (!cipher->aead)
			xsRangeError("additionalData not supported by algorithm");
		void *aad;
		xsUnsignedValue aadLen;
		xsmcGetBufferReadable(xsVar(0), &aad, &aadLen);
		setAdditionalData(the, cipher, aad, aadLen);
	}

	startOperation(the, cipher);
}

void xs_cipher_close(xsMachine *the)
{
	if (!xsmcGetHostData(xsThis))
		return;
	Cipher cipher = xsmcGetHostDataValidate(xsThis, xs_cipher_destructor);
	disposeCipher(cipher);
	xsmcSetHostData(xsThis, NULL);
	xsmcSetHostDestructor(xsThis, NULL);	// subsequent calls fail validation and throw SyntaxError
}

void xs_cipher_reset(xsMachine *the)
{
	Cipher cipher = xsmcGetHostDataValidate(xsThis, xs_cipher_destructor);
	xsmcVars(1);

	if (xsmcArgc > 0) {
		if (xsmcGet(xsVar(0), xsArg(0), xsID_tagLength)) {
			if (!cipher->aead)
				xsRangeError("tagLength not supported by algorithm");
			int tagLength = xsmcToInteger(xsVar(0));
			if (!validTagLength(cipher->alg, tagLength))
				xsRangeError("invalid tagLength");
			cipher->tagLength = (uint8_t)tagLength;
		}
		if (xsmcGet(xsVar(0), xsArg(0), xsID_iv)) {
			void *iv;
			xsUnsignedValue ivLen;
			xsmcGetBufferReadable(xsVar(0), &iv, &ivLen);
			setIV(the, cipher, iv, ivLen);
		}
		if (xsmcGet(xsVar(0), xsArg(0), xsID_additionalData)) {
			if (!cipher->aead)
				xsRangeError("additionalData not supported by algorithm");
			void *aad;
			xsUnsignedValue aadLen;
			xsmcGetBufferReadable(xsVar(0), &aad, &aadLen);
			setAdditionalData(the, cipher, aad, aadLen);
		}
	}

	startOperation(the, cipher);
}

// encrypt and decrypt: the direction is fixed at construction, and the other method throws, as read does on an IO instance that only writes
static void cipherProcess(xsMachine *the, uint8_t method)
{
	Cipher cipher = xsmcGetHostDataValidate(xsThis, xs_cipher_destructor);
	xsmcVars(1);

	if (method != cipher->direction)
		xsUnknownError((kDirectionEncrypt == method) ? "instance does not encrypt" : "instance does not decrypt");
	if (kStateDone == cipher->state)
		xsUnknownError("operation complete; reset required");

	uint8_t more = 0, hasOutput = 0;
	if (xsmcArgc > 1) {
		if (xsmcGet(xsVar(0), xsArg(1), xsID_more))
			more = xsmcTest(xsVar(0));
		if (xsmcGet(xsVar(0), xsArg(1), xsID_output))
			hasOutput = 1;
	}
	if (more && cipher->aead)
		xsRangeError("more not supported for authenticated algorithms on this platform");

	uint32_t dataLen = 0;
	const uint8_t *peekData = gEmpty;		// valid only until the output is allocated
	uint8_t hasData = (xsmcArgc > 0) && (xsUndefinedType != xsmcTypeOf(xsArg(0)));
	if (hasData) {
		void *p;
		xsUnsignedValue n;
		xsmcGetBufferReadable(xsArg(0), &p, &n);
		dataLen = n;
		peekData = p;
	}

	const uint8_t blockSize = cipher->blockSize;
	const uint8_t direction = cipher->direction;
	const uint8_t aead = cipher->aead;
	const uint8_t padding = cipher->padding;
	const uint8_t tagLength = cipher->tagLength;
	const uint8_t heldLen = cipher->heldLen;
	const uint8_t isBlockMode = (kAlgAESECB == cipher->alg) || (kAlgAESCBC == cipher->alg);
	const uint8_t trailerHold = ((kDirectionDecrypt == direction) && padding) ? 16 : 0;
	uint32_t total = heldLen + dataLen;
	uint32_t emitMax;
	uint8_t pad = 0;

	if (more) {
		if (dataLen % blockSize)
			xsRangeError("data length must be a multiple of blockSize");
		emitMax = (total > trailerHold) ? (total - trailerHold) : 0;
	}
	else if (kDirectionEncrypt == direction) {
		if (aead)
			emitMax = dataLen + tagLength;
		else if (padding)
			emitMax = dataLen + (16 - (dataLen % 16));
		else if (isBlockMode && (dataLen % 16))
			xsRangeError("data length must be a multiple of blockSize");
		else
			emitMax = dataLen;
	}
	else {
		if (aead) {
			if (total < tagLength)
				xsRangeError("data shorter than authentication tag");
			emitMax = total - tagLength;
		}
		else if (padding) {
			if ((total < 16) || (total % 16))
				xsRangeError("data length must be a multiple of blockSize");
			uint8_t last[16], prev[16];
			copyRange(last, cipher->held, heldLen, peekData, total - 16, 16);
			if (total >= 32)
				copyRange(prev, cipher->held, heldLen, peekData, total - 32, 16);
			else
				c_memcpy(prev, cipher->lastCt, 16);
			pad = peekPad(the, cipher, prev, last);
			emitMax = total - pad;
		}
		else if (isBlockMode && (total % 16))
			xsRangeError("data length must be a multiple of blockSize");
		else
			emitMax = total;
	}

	uint8_t *out = NULL;
	if (hasOutput) {
		xsmcGet(xsVar(0), xsArg(1), xsID_output);
		void *p;
		xsUnsignedValue n;
		xsmcGetBufferWritable(xsVar(0), &p, &n);
		out = p;
		if (n < emitMax)
			xsRangeError("output too small");
	}
	else
		out = xsmcSetArrayBuffer(xsResult, NULL, emitMax);

	const uint8_t *data = gEmpty;
	if (hasData) {
		void *p;
		xsUnsignedValue n;
		xsmcGetBufferReadable(xsArg(0), &p, &n);
		data = p;
	}
	if (heldLen && overlaps(out, emitMax, data, dataLen))
		xsRangeError("in-place not permitted with deferred output");

	uint32_t emitted = 0;
	if (more) {
		cipher->state = kStateMore;
		if (!trailerHold)
			feed(the, cipher, data, out, dataLen);
		else if (total <= trailerHold) {
			c_memcpy(cipher->held + heldLen, data, dataLen);
			cipher->heldLen = (uint8_t)total;
		}
		else {
			uint32_t fromHeld = (heldLen < emitMax) ? heldLen : emitMax;
			uint32_t fromData = emitMax - fromHeld;
			if (fromHeld < heldLen) {	// only part of held is emitted: shuffle so feedHeld sees exactly the emitted prefix
				uint8_t keep[16];
				c_memcpy(keep, cipher->held + fromHeld, heldLen - fromHeld);
				cipher->heldLen = (uint8_t)fromHeld;
				feedHeld(the, cipher, data, fromData, out);
				c_memcpy(cipher->held, keep, heldLen - fromHeld);
				c_memcpy(cipher->held + (heldLen - fromHeld), data + fromData, dataLen - fromData);
			}
			else {
				feedHeld(the, cipher, data, fromData, out);
				c_memcpy(cipher->held, data + fromData, dataLen - fromData);
			}
			cipher->heldLen = (uint8_t)trailerHold;
		}
		emitted = emitMax;
	}
	else if (aead) {		// one shot: CryptoKit takes the whole message and the tag together
		int32_t result;
		if (kDirectionEncrypt == direction) {
			uint8_t tag[16];
			result = ckAEADSeal(aeadAlg(cipher), cipher->key, cipher->keySize, cipher->iv, cipher->ivLen, cipher->aad, cipher->aadLen, data, dataLen, out, tag);
			if (0 == result)
				c_memcpy(out + dataLen, tag, 16);
		}
		else {
			uint8_t tag[16];
			c_memcpy(tag, data + emitMax, 16);
			result = ckAEADOpen(aeadAlg(cipher), cipher->key, cipher->keySize, cipher->iv, cipher->ivLen, cipher->aad, cipher->aadLen, data, emitMax, tag, out);
		}
		cipher->state = kStateDone;
		if (1 == result) {
			c_memset(out, 0, emitMax);
			xsUnknownError("authentication failed");
		}
		if (0 != result)
			xsUnknownError((kDirectionEncrypt == direction) ? "encryption failed" : "decryption failed");
		emitted = emitMax;
	}
	else if (kDirectionEncrypt == direction) {
		if (padding) {
			uint32_t full = dataLen - (dataLen % 16);
			uint8_t block[16];
			uint8_t padLen = (uint8_t)(16 - (dataLen % 16));
			c_memcpy(block, data + full, dataLen - full);
			c_memset(block + (dataLen - full), padLen, padLen);
			feed(the, cipher, data, out, full);
			feed(the, cipher, block, out + full, 16);
		}
		else
			feed(the, cipher, data, out, dataLen);
		emitted = emitMax;
	}
	else {
		uint32_t trailer = padding ? 16 : 0;
		uint32_t bodyLen = total - trailer;
		uint32_t fromHeld = (heldLen < bodyLen) ? heldLen : bodyLen;
		uint32_t fromData = bodyLen - fromHeld;
		uint8_t tail[16];
		c_memcpy(tail, cipher->held + fromHeld, heldLen - fromHeld);
		c_memcpy(tail + (heldLen - fromHeld), data + fromData, dataLen - fromData);
		cipher->heldLen = (uint8_t)fromHeld;
		feedHeld(the, cipher, data, fromData, out);
		emitted = bodyLen;
		if (padding) {
			uint8_t block[16];
			feed(the, cipher, tail, block, 16);
			c_memcpy(out + bodyLen, block, 16 - pad);
			emitted += 16 - pad;
		}
	}

	if (!more) {
		endOperation(cipher);
		cipher->state = kStateDone;
		cipher->heldLen = 0;
	}

	if (emitted != emitMax)
		xsUnknownError("internal: output size");
	if (hasOutput)
		xsmcSetInteger(xsResult, (int)emitted);
}

void xs_cipher_encrypt(xsMachine *the)
{
	cipherProcess(the, kDirectionEncrypt);
}

void xs_cipher_decrypt(xsMachine *the)
{
	cipherProcess(the, kDirectionDecrypt);
}

void xs_cipher_get_blockSize(xsMachine *the)
{
	Cipher cipher = xsmcGetHostDataValidate(xsThis, xs_cipher_destructor);
	xsmcSetInteger(xsResult, cipher->blockSize);
}

void xs_cipher_get_keySize(xsMachine *the)
{
	Cipher cipher = xsmcGetHostDataValidate(xsThis, xs_cipher_destructor);
	xsmcSetInteger(xsResult, cipher->keySize * 8);		// bits
}

void xs_cipher_get_tagLength(xsMachine *the)
{
	Cipher cipher = xsmcGetHostDataValidate(xsThis, xs_cipher_destructor);
	xsmcSetInteger(xsResult, cipher->tagLength);
}

// the constructor's tagLength and iv rules, and whether more is accepted, for supports
static int supportsOptions(xsMachine *the, const CipherAlgEntry *entry)
{
	if (xsmcGet(xsVar(0), xsArg(0), xsID_tagLength)) {
		if (!entry->aead || !validTagLength(entry->alg, xsmcToInteger(xsVar(0))))
			return 0;
	}
	if (xsmcGet(xsVar(0), xsArg(0), xsID_iv)) {
		void *iv;
		xsUnsignedValue ivLen;
		xsmcGetBufferReadable(xsVar(0), &iv, &ivLen);
		switch (entry->alg) {
			case kAlgAESECB: return 0;
			case kAlgAESCBC:
			case kAlgAESCTR: if (16 != ivLen) return 0; break;
			case kAlgAESGCM: if ((ivLen < 12) || (ivLen > kMaxIV)) return 0; break;
			default: if (12 != ivLen) return 0; break;
		}
	}
	if (xsmcGet(xsVar(0), xsArg(0), xsID_more) && xsmcTest(xsVar(0)) && entry->aead)
		return 0;		// the authenticated modes are one shot
	return 1;
}

// supports: the constructor's properties, plus more (whether the algorithm may be processed in more than one call)
void xs_cipher_supports(xsMachine *the)
{
	xsmcVars(1);
	int supported = 0;
	if (xsmcGet(xsVar(0), xsArg(0), xsID_algorithm) && (xsStringType == xsmcTypeOf(xsVar(0)))) {
		const char *name = xsmcToString(xsVar(0));
		for (int i = 0; gCipherAlgs[i].name; i++) {
			if (0 == c_strcmp(name, gCipherAlgs[i].name)) {
				supported = supportsOptions(the, &gCipherAlgs[i]);
				break;
			}
		}
	}
	xsmcSetBoolean(xsResult, supported);
}

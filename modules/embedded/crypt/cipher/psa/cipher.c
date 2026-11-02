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
	ECMA-419 Crypto Cipher — PSA Crypto (mbedtls)
	AES-ECB, AES-CBC, AES-CTR, AES-GCM, ChaCha20, ChaCha20-Poly1305

	PKCS#7 is applied here over the NO_PADDING algorithms so that the output size is known before allocation.
	Decrypt defers a possible trailer (one block for PKCS#7, tagLength bytes for AEAD) across calls with more: true.
	Every psa_*_update call except the last of an operation is kept 16-byte aligned: the ESP32 hardware GCM driver
	produces wrong output for a non-final unaligned chunk.
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_* values

#include "psa/crypto.h"
#include "mc.defines.h"

enum {
	kAlgAESECB,
	kAlgAESCBC,
	kAlgAESCTR,
	kAlgAESGCM,
	kAlgChaCha20,
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
	psa_key_type_t keyType;
	psa_algorithm_t psaAlg;
} CipherAlgEntry;

// a build may omit algorithms (manifest "defines": crypt.cipher.<name> 0), the same names as the KCL build
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
#ifndef MODDEF_CRYPT_CIPHER_CHACHA20
	#define MODDEF_CRYPT_CIPHER_CHACHA20 1
#endif
#ifndef MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
	#define MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305 1
#endif

static const CipherAlgEntry gCipherAlgs[] = {
#if MODDEF_CRYPT_CIPHER_AES_ECB
	{ "AES-ECB",           kAlgAESECB,           16, 0, PSA_KEY_TYPE_AES,      PSA_ALG_ECB_NO_PADDING },
#endif
#if MODDEF_CRYPT_CIPHER_AES_CBC
	{ "AES-CBC",           kAlgAESCBC,           16, 0, PSA_KEY_TYPE_AES,      PSA_ALG_CBC_NO_PADDING },
#endif
#if MODDEF_CRYPT_CIPHER_AES_CTR
	{ "AES-CTR",           kAlgAESCTR,           16, 0, PSA_KEY_TYPE_AES,      PSA_ALG_CTR },
#endif
#if MODDEF_CRYPT_CIPHER_AES_GCM
	{ "AES-GCM",           kAlgAESGCM,           16, 1, PSA_KEY_TYPE_AES,      PSA_ALG_GCM },
#endif
#if MODDEF_CRYPT_CIPHER_CHACHA20
	{ "ChaCha20",          kAlgChaCha20,         64, 0, PSA_KEY_TYPE_CHACHA20, PSA_ALG_STREAM_CIPHER },
#endif
#if MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
	{ "ChaCha20-Poly1305", kAlgChaCha20Poly1305, 64, 1, PSA_KEY_TYPE_CHACHA20, PSA_ALG_CHACHA20_POLY1305 },
#endif
	{ NULL, 0, 0, 0, 0, 0 }
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
	uint8_t active;			// an operation is set up in op

	psa_key_id_t key;
	psa_algorithm_t psaAlg;	// without tag length
	uint8_t iv[kMaxIV];
	uint8_t held[16];
	uint8_t lastCt[16];		// CBC decrypt: last ciphertext block fed (the chaining value), for the padding peek
	uint8_t *aad;
	uint32_t aadLen;

	union {
		psa_cipher_operation_t cipher;
		psa_aead_operation_t aead;
	} op;
} CipherRecord, *Cipher;

static void psaCheck(xsMachine *the, psa_status_t status, const char *what)
{
	if (PSA_SUCCESS != status) {
		if (PSA_ERROR_NOT_SUPPORTED == status)
			xsUnknownError("algorithm not enabled in build");
		xsUnknownError("%s failed %d", what, (int)status);
	}
}

static psa_algorithm_t effectiveAlg(Cipher cipher)
{
	if (cipher->aead)
		return PSA_ALG_AEAD_WITH_SHORTENED_TAG(cipher->psaAlg, cipher->tagLength);
	return cipher->psaAlg;
}

static void abortOperation(Cipher cipher)
{
	if (!cipher->active)
		return;
	if (cipher->aead)
		psa_aead_abort(&cipher->op.aead);
	else
		psa_cipher_abort(&cipher->op.cipher);
	cipher->active = 0;
}

// begin an operation: setup, iv/nonce, additional data
static void startOperation(xsMachine *the, Cipher cipher)
{
	abortOperation(cipher);
	cipher->state = kStateReady;
	cipher->heldLen = 0;

	psa_status_t status;
	if (cipher->aead) {
		cipher->op.aead = (psa_aead_operation_t)PSA_AEAD_OPERATION_INIT;
		if (kDirectionEncrypt == cipher->direction)
			status = psa_aead_encrypt_setup(&cipher->op.aead, cipher->key, effectiveAlg(cipher));
		else
			status = psa_aead_decrypt_setup(&cipher->op.aead, cipher->key, effectiveAlg(cipher));
		psaCheck(the, status, "psa_aead_setup");
		cipher->active = 1;
		status = psa_aead_set_nonce(&cipher->op.aead, cipher->iv, cipher->ivLen);
		if (PSA_SUCCESS != status) {
			abortOperation(cipher);
			xsRangeError("invalid iv");
		}
		if (cipher->aadLen) {
			status = psa_aead_update_ad(&cipher->op.aead, cipher->aad, cipher->aadLen);
			if (PSA_SUCCESS != status) {
				abortOperation(cipher);
				psaCheck(the, status, "psa_aead_update_ad");
			}
		}
	}
	else {
		cipher->op.cipher = (psa_cipher_operation_t)PSA_CIPHER_OPERATION_INIT;
		if (kDirectionEncrypt == cipher->direction)
			status = psa_cipher_encrypt_setup(&cipher->op.cipher, cipher->key, cipher->psaAlg);
		else
			status = psa_cipher_decrypt_setup(&cipher->op.cipher, cipher->key, cipher->psaAlg);
		psaCheck(the, status, "psa_cipher_setup");
		cipher->active = 1;
		if (kAlgAESECB != cipher->alg) {
			status = psa_cipher_set_iv(&cipher->op.cipher, cipher->iv, cipher->ivLen);
			if (PSA_SUCCESS != status) {
				abortOperation(cipher);
				xsRangeError("invalid iv");
			}
		}
		if (kAlgAESCBC == cipher->alg)
			c_memcpy(cipher->lastCt, cipher->iv, 16);
	}
}

// one update call. in and out may be the same bytes. Output must equal input for the algorithms and alignment used here.
static void feed(xsMachine *the, Cipher cipher, const uint8_t *in, uint8_t *out, uint32_t count)
{
	if (!count)
		return;

	size_t produced = 0;
	psa_status_t status;
	if (cipher->aead)
		status = psa_aead_update(&cipher->op.aead, in, count, out, count, &produced);
	else
		status = psa_cipher_update(&cipher->op.cipher, in, count, out, count, &produced);
	if (PSA_SUCCESS != status) {
		abortOperation(cipher);
		cipher->state = kStateDone;
		psaCheck(the, status, "psa update");
	}
	if (produced != count) {
		abortOperation(cipher);
		cipher->state = kStateDone;
		xsUnknownError("internal: update produced %d of %d", (int)produced, (int)count);
	}
	if ((kAlgAESCBC == cipher->alg) && (kDirectionDecrypt == cipher->direction) && (count >= 16))
		c_memcpy(cipher->lastCt, in + count - 16, 16);
}

// feed held || data[0..dataCount) keeping every update 16-byte aligned (the last update of the operation may be unaligned).
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

// out/outLen: plaintext written by this call, zeroed if verification fails
static void finishOperation(xsMachine *the, Cipher cipher, uint8_t *tag, const uint8_t *expectedTag, uint8_t *out, uint32_t outLen)
{
	uint8_t trailing[16];
	size_t trailingLen = 0, tagLen = 0;
	psa_status_t status;
	if (cipher->aead) {
		if (kDirectionEncrypt == cipher->direction)
			status = psa_aead_finish(&cipher->op.aead, trailing, sizeof(trailing), &trailingLen, tag, cipher->tagLength, &tagLen);
		else
			status = psa_aead_verify(&cipher->op.aead, trailing, sizeof(trailing), &trailingLen, expectedTag, cipher->tagLength);
	}
	else
		status = psa_cipher_finish(&cipher->op.cipher, trailing, sizeof(trailing), &trailingLen);
	cipher->active = 0;		// finish and verify end the operation, success or not
	cipher->state = kStateDone;
	if (PSA_ERROR_INVALID_SIGNATURE == status) {
		if (outLen)
			c_memset(out, 0, outLen);
		xsUnknownError("authentication failed");
	}
	psaCheck(the, status, "psa finish");
	if (trailingLen)
		xsUnknownError("internal: unexpected trailing output");
}

static int overlaps(const uint8_t *a, uint32_t aLen, const uint8_t *b, uint32_t bLen)
{
	return aLen && bLen && (a < b + bLen) && (b < a + aLen);
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
	uint8_t input[32], plain[16];
	size_t inputLen = 16, plainLen = 0;
	if (kAlgAESCBC == cipher->alg) {
		c_memcpy(input, prev, 16);
		c_memcpy(input + 16, last, 16);
		inputLen = 32;
	}
	else
		c_memcpy(input, last, 16);
	psa_status_t status = psa_cipher_decrypt(cipher->key, cipher->psaAlg, input, inputLen, plain, sizeof(plain), &plainLen);
	psaCheck(the, status, "psa_cipher_decrypt");
	uint8_t pad = plain[15];
	uint8_t bad = (0 == pad) | (pad > 16) | (16 != plainLen);
	for (int i = 0; i < 16; i++)
		bad |= (i >= 16 - pad) & (plain[i] != pad);
	if (bad)
		xsUnknownError("invalid padding");
	return pad;
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
			if ((ivLen < 1) || (ivLen > kMaxIV))
				xsRangeError("iv must be 1 to 32 bytes");
			break;
		case kAlgChaCha20:
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
	abortOperation(cipher);
	if (cipher->key)
		psa_destroy_key(cipher->key);
	if (cipher->aad)
		c_free(cipher->aad);
	c_memset(cipher, 0, sizeof(CipherRecord));
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



// AES-GCM accepts a shortened tag; RFC 8439 defines only the full tag of ChaCha20-Poly1305
static int validTagLength(uint8_t alg, int tagLength)
{
	return (tagLength >= ((kAlgChaCha20Poly1305 == alg) ? 16 : 4)) && (tagLength <= 16);
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
	cipher->psaAlg = entry->psaAlg;

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
	if (PSA_KEY_TYPE_CHACHA20 == entry->keyType) {
		if (32 != keyLen)
			xsRangeError("key must be 32 bytes");
	}
	else if ((16 != keyLen) && (24 != keyLen) && (32 != keyLen))
		xsRangeError("key must be 16, 24, or 32 bytes");
	cipher->keySize = (uint8_t)keyLen;

	psa_status_t status = psa_crypto_init();
	psaCheck(the, status, "psa_crypto_init");

	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attributes, entry->keyType);
	psa_set_key_bits(&attributes, keyLen * 8);
	psa_set_key_usage_flags(&attributes, (kDirectionEncrypt == cipher->direction) ? PSA_KEY_USAGE_ENCRYPT : PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attributes, cipher->aead ? PSA_ALG_AEAD_WITH_AT_LEAST_THIS_LENGTH_TAG(cipher->psaAlg, 4) : cipher->psaAlg);
	status = psa_import_key(&attributes, key, keyLen, &cipher->key);
	psa_reset_key_attributes(&attributes);
	if (PSA_SUCCESS != status) {
		cipher->key = 0;
		psaCheck(the, status, "psa_import_key");
	}

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

	startOperation(the, cipher);		// also reports an algorithm the build does not include
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
	const uint8_t trailerHold = (kDirectionDecrypt == direction) ? (aead ? tagLength : (padding ? 16 : 0)) : 0;
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
	else if (kDirectionEncrypt == direction) {
		if (padding) {
			uint32_t full = dataLen - (dataLen % 16);
			uint8_t block[16];
			uint8_t padLen = (uint8_t)(16 - (dataLen % 16));
			c_memcpy(block, data + full, dataLen - full);
			c_memset(block + (dataLen - full), padLen, padLen);
			feed(the, cipher, data, out, full);
			feed(the, cipher, block, out + full, 16);
			finishOperation(the, cipher, NULL, NULL, NULL, 0);
		}
		else {
			feed(the, cipher, data, out, dataLen);
			finishOperation(the, cipher, out + dataLen, NULL, NULL, 0);
		}
		emitted = emitMax;
	}
	else {
		uint32_t trailer = aead ? tagLength : (padding ? 16 : 0);
		uint32_t bodyLen = total - trailer;
		uint32_t fromHeld = (heldLen < bodyLen) ? heldLen : bodyLen;
		uint32_t fromData = bodyLen - fromHeld;
		uint8_t tail[16];
		c_memcpy(tail, cipher->held + fromHeld, heldLen - fromHeld);
		c_memcpy(tail + (heldLen - fromHeld), data + fromData, dataLen - fromData);

		cipher->heldLen = (uint8_t)fromHeld;
		feedHeld(the, cipher, data, fromData, out);
		emitted = bodyLen;

		if (aead)
			finishOperation(the, cipher, NULL, tail, out, bodyLen);		// throws on mismatch, zeroing out
		else if (padding) {
			uint8_t block[16];
			feed(the, cipher, tail, block, 16);
			finishOperation(the, cipher, NULL, NULL, NULL, 0);
			c_memcpy(out + bodyLen, block, 16 - pad);
			emitted += 16 - pad;
		}
		else
			finishOperation(the, cipher, NULL, NULL, NULL, 0);
	}

	if (!more) {
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
			case kAlgAESGCM: if ((ivLen < 1) || (ivLen > kMaxIV)) return 0; break;
			default: if (12 != ivLen) return 0; break;
		}
	}
	return 1;		// every algorithm streams
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

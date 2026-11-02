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
	ECMA-419 Crypto Cipher — KCL software backend
	AES-ECB, AES-CBC, AES-CTR, AES-GCM, ChaCha20, ChaCha20-Poly1305
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_* values

#include "fips197.h"
#include "ghash.h"
#include "mc.defines.h"

// a build may omit algorithms (manifest "defines": crypt.cipher.<name> 0); every one is present unless omitted.
// Without AES-ECB and AES-CBC the inverse cipher (its tables and key schedule) is not built: CTR and GCM only encrypt.
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
#define mxAESBlockModes (MODDEF_CRYPT_CIPHER_AES_ECB || MODDEF_CRYPT_CIPHER_AES_CBC)
#define mxChaCha (MODDEF_CRYPT_CIPHER_CHACHA20 || MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305)
#if mxChaCha
	#include "chacha.h"
#endif
#if MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
	#include "poly1305.h"
#endif

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
	kStateReady,		// no data processed in this operation
	kStateMore,			// at least one call with more: true
	kStateDone,			// final call made; reset required
};

typedef struct {
	const char *name;
	uint8_t alg;
	uint8_t blockSize;
	uint8_t aead;
} CipherAlgEntry;

static const CipherAlgEntry gCipherAlgs[] = {
#if MODDEF_CRYPT_CIPHER_AES_ECB
	{ "AES-ECB",  kAlgAESECB,  16, 0 },
#endif
#if MODDEF_CRYPT_CIPHER_AES_CBC
	{ "AES-CBC",  kAlgAESCBC,  16, 0 },
#endif
#if MODDEF_CRYPT_CIPHER_AES_CTR
	{ "AES-CTR",  kAlgAESCTR,  16, 0 },
#endif
#if MODDEF_CRYPT_CIPHER_AES_GCM
	{ "AES-GCM",  kAlgAESGCM,  16, 1 },
#endif
#if MODDEF_CRYPT_CIPHER_CHACHA20
	{ "ChaCha20", kAlgChaCha20, 64, 0 },
#endif
#if MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
	{ "ChaCha20-Poly1305", kAlgChaCha20Poly1305, 64, 1 },
#endif
	{ NULL, 0, 0, 0 }
};

typedef struct {
	uint8_t alg;
	uint8_t direction;
	uint8_t padding;		// 1 for pkcs7
	uint8_t aead;
	uint8_t blockSize;
	uint8_t keySize;
	uint8_t tagLength;		// 0 for non-AEAD
	uint8_t state;
	uint8_t heldLen;		// bytes deferred on decrypt (possible trailer)
	uint8_t ksOff;			// bytes of ks[] already consumed (CTR/GCM)
	uint8_t ivLen;

	uint8_t key[32];
	uint8_t iv0[16];		// operation start value: CBC IV, CTR counter block, GCM J0, ChaCha20 counter 0 || nonce
	uint8_t chain[16];		// CBC previous ciphertext block / CTR counter block
	uint8_t ks[16];			// CTR keystream block
	uint8_t held[16];

	union {
		struct {
			uint32_t subkey[(14 + 1) * 4];
			uint8_t Nr;
		} aes;
#if mxChaCha
		chacha_ctx chacha;
#endif
	} ctx;
	union {					// the authenticator of an AEAD
		ghash_t ghash;		// GCM
#if MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
		poly1305_t poly;	// ChaCha20-Poly1305
#endif
	} mac;
#if MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
	uint8_t *aad;			// the additional data, replayed by every operation (its key depends on the nonce); c_malloc
	uint32_t aadLen;
	uint32_t textLen;		// ciphertext bytes authenticated in this operation
#endif
} CipherRecord, *Cipher;

static void cipherStart(Cipher cipher);

/*
	primitives
*/

static void aesEncryptBlock(Cipher cipher, const uint8_t *in, uint8_t *out)
{
	uint8_t tmp[16];
	c_memcpy(tmp, in, 16);
	aes_encrypt4(tmp, out, cipher->ctx.aes.Nr, cipher->ctx.aes.subkey);
}

#if mxAESBlockModes
static void aesDecryptBlock(Cipher cipher, const uint8_t *in, uint8_t *out)
{
	uint8_t tmp[16];
	c_memcpy(tmp, in, 16);
	aes_decrypt4(tmp, out, cipher->ctx.aes.Nr, cipher->ctx.aes.subkey);
}
#endif

static void xor16(uint8_t *out, const uint8_t *a, const uint8_t *b)
{
	for (int i = 0; i < 16; i++)
		out[i] = a[i] ^ b[i];
}

// CTR: increment the full block; GCM: increment the low 32 bits only (inc32)
static void counterIncrement(Cipher cipher)
{
	int stop = (kAlgAESGCM == cipher->alg) ? 12 : 0;
	for (int i = 15; i >= stop; i--) {
		if (++cipher->chain[i])
			break;
	}
}

// block modes: count is a multiple of 16; in and out may be the same bytes
#if mxAESBlockModes
static void processBlocks(Cipher cipher, const uint8_t *in, uint8_t *out, uint32_t count)
{
	while (count) {
		if (kAlgAESECB == cipher->alg) {
			if (kDirectionEncrypt == cipher->direction)
				aesEncryptBlock(cipher, in, out);
			else
				aesDecryptBlock(cipher, in, out);
		}
		else if (kDirectionEncrypt == cipher->direction) {	// CBC
			xor16(cipher->chain, cipher->chain, in);
			aesEncryptBlock(cipher, cipher->chain, out);
			c_memcpy(cipher->chain, out, 16);
		}
		else {
			uint8_t ct[16];
			c_memcpy(ct, in, 16);
			aesDecryptBlock(cipher, ct, out);
			xor16(out, out, cipher->chain);
			c_memcpy(cipher->chain, ct, 16);
		}
		in += 16;
		out += 16;
		count -= 16;
	}
}
#endif

// stream modes: any count; in and out may be the same bytes
static void processStream(Cipher cipher, const uint8_t *in, uint8_t *out, uint32_t count)
{
#if MODDEF_CRYPT_CIPHER_CHACHA20
	if (kAlgChaCha20 == cipher->alg) {
		chacha_process(in, out, count, &cipher->ctx.chacha);
		return;
	}
#endif
#if MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
	if (kAlgChaCha20Poly1305 == cipher->alg) {		// the ciphertext is authenticated: before decrypting, after encrypting
		if (kDirectionDecrypt == cipher->direction)
			poly1305_update(&cipher->mac.poly, in, count);
		chacha_process(in, out, count, &cipher->ctx.chacha);
		if (kDirectionEncrypt == cipher->direction)
			poly1305_update(&cipher->mac.poly, out, count);
		cipher->textLen += count;
		return;
	}
#endif

	if ((kAlgAESGCM == cipher->alg) && (kDirectionDecrypt == cipher->direction))
		_ghash_update(&cipher->mac.ghash, in, count);

	uint8_t *start = out;
	uint32_t total = count;
	while (count) {
		if (16 == cipher->ksOff) {
			aesEncryptBlock(cipher, cipher->chain, cipher->ks);
			counterIncrement(cipher);
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

	if ((kAlgAESGCM == cipher->alg) && (kDirectionEncrypt == cipher->direction))
		_ghash_update(&cipher->mac.ghash, start, total);
}

static void processBytes(Cipher cipher, const uint8_t *in, uint8_t *out, uint32_t count)
{
#if mxAESBlockModes
	if ((kAlgAESECB == cipher->alg) || (kAlgAESCBC == cipher->alg))
		processBlocks(cipher, in, out, count);
	else
#endif
		processStream(cipher, in, out, count);
}

#if MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
static void polyPad16(poly1305_t *poly, uint32_t length)
{
	static const uint8_t zeros[16] = {0};
	if (length % 16)
		poly1305_update(poly, zeros, 16 - (length % 16));
}

static void putLE64(uint8_t *p, uint32_t value)
{
	p[0] = (uint8_t)value;
	p[1] = (uint8_t)(value >> 8);
	p[2] = (uint8_t)(value >> 16);
	p[3] = (uint8_t)(value >> 24);
	p[4] = p[5] = p[6] = p[7] = 0;
}
#endif

// the full 16-byte tag: GCM's E_K(J0) xor GHASH, or Poly1305 over AAD || pad || ciphertext || pad || le64 lengths (RFC 8439 section 2.8)
static void aeadTag(Cipher cipher, uint8_t *tag)
{
#if MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
	if (kAlgChaCha20Poly1305 == cipher->alg) {
		uint8_t lengths[16];
		polyPad16(&cipher->mac.poly, cipher->textLen);
		putLE64(lengths, cipher->aadLen);
		putLE64(lengths + 8, cipher->textLen);
		poly1305_update(&cipher->mac.poly, lengths, 16);
		poly1305_finish(&cipher->mac.poly, tag);
		return;
	}
#endif
	uint8_t y[16], ej0[16];
	_ghash_fin(&cipher->mac.ghash, y);
	aesEncryptBlock(cipher, cipher->iv0, ej0);
	xor16(tag, y, ej0);
}

/*
	setup
*/

static void setIV(xsMachine *the, Cipher cipher, const uint8_t *iv, uint32_t ivLen)
{
	switch (cipher->alg) {
		case kAlgAESECB:		// a property defined for the other modes, not accepted by ECB
			xsRangeError("iv not supported by algorithm");
		case kAlgAESCBC:
		case kAlgAESCTR:
			if (16 != ivLen)
				xsRangeError("iv must be 16 bytes");
			c_memcpy(cipher->iv0, iv, 16);
			break;
		case kAlgAESGCM:
			if (!ivLen)
				xsRangeError("iv required");
			if (12 == ivLen) {
				c_memcpy(cipher->iv0, iv, 12);
				cipher->iv0[12] = cipher->iv0[13] = cipher->iv0[14] = 0;
				cipher->iv0[15] = 1;
			}
			else {		// J0 = GHASH_H(IV || 0 || len64(IV))
				ghash_t g;
				c_memset(&g, 0, sizeof(g));
				g.h = cipher->mac.ghash.h;
				_ghash_create(&g);
				_ghash_update(&g, iv, ivLen);
				_ghash_fin(&g, cipher->iv0);
			}
			break;
		case kAlgChaCha20:
		case kAlgChaCha20Poly1305:
			if (12 != ivLen)
				xsRangeError("iv must be 12 bytes");
			c_memset(cipher->iv0, 0, 4);		// block counter 0
			c_memcpy(cipher->iv0 + 4, iv, 12);
			break;
	}
	cipher->ivLen = 16;
}

static void setAdditionalData(xsMachine *the, Cipher cipher, const uint8_t *aad, uint32_t aadLen)
{
#if MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
	if (kAlgChaCha20Poly1305 == cipher->alg) {		// kept: each operation authenticates it under that operation's key
		uint8_t *copy = NULL;
		if (aadLen) {
			copy = c_malloc(aadLen);
			if (!copy)
				xsUnknownError("not enough memory");
			c_memcpy(copy, aad, aadLen);
		}
		if (cipher->aad)
			c_free(cipher->aad);
		cipher->aad = copy;
		cipher->aadLen = aadLen;
		return;
	}
#endif
	ghash_t *g = &cipher->mac.ghash;
	c_memset(&g->y0, 0, sizeof(g->y0));
	_ghash_create(g);
	_ghash_update(g, aad, aadLen);
	_ghash_flush_partial(g);
	g->y0 = g->y;
	g->aad_len = aadLen;
}

// begin an operation from iv0 and the stored AAD state
static void cipherStart(Cipher cipher)
{
	cipher->state = kStateReady;
	cipher->heldLen = 0;
	cipher->ksOff = 16;

	switch (cipher->alg) {
		case kAlgAESCBC:
			c_memcpy(cipher->chain, cipher->iv0, 16);
			break;
		case kAlgAESCTR:
			c_memcpy(cipher->chain, cipher->iv0, 16);
			break;
		case kAlgAESGCM:
			c_memcpy(cipher->chain, cipher->iv0, 16);
			counterIncrement(cipher);		// data starts at inc32(J0)
			_ghash_create(&cipher->mac.ghash);
			break;
#if MODDEF_CRYPT_CIPHER_CHACHA20
		case kAlgChaCha20: {
			uint32_t counter = cipher->iv0[0] | (cipher->iv0[1] << 8) | (cipher->iv0[2] << 16) | ((uint32_t)cipher->iv0[3] << 24);
			chacha_ivsetup(&cipher->ctx.chacha, cipher->iv0 + 4, 12, counter);
			} break;
#endif
#if MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
		case kAlgChaCha20Poly1305: {		// RFC 8439 section 2.8: the Poly1305 key is keystream block 0; the data starts at block 1
			uint8_t block[64];
			c_memset(block, 0, sizeof(block));
			chacha_ivsetup(&cipher->ctx.chacha, cipher->iv0 + 4, 12, 0);
			chacha_process(block, block, sizeof(block), &cipher->ctx.chacha);
			poly1305_init(&cipher->mac.poly, block);
			c_memset(block, 0, sizeof(block));
			chacha_ivsetup(&cipher->ctx.chacha, cipher->iv0 + 4, 12, 1);
			if (cipher->aadLen)
				poly1305_update(&cipher->mac.poly, cipher->aad, cipher->aadLen);
			polyPad16(&cipher->mac.poly, cipher->aadLen);
			cipher->textLen = 0;
			} break;
#endif
	}
}

/*
	class
*/


void xs_cipher_destructor(void *data)
{
	if (data) {
#if MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
		if (((Cipher)data)->aad)
			c_free(((Cipher)data)->aad);
#endif
		c_memset(data, 0, sizeof(CipherRecord));		// key material
	}
}

// AES-GCM accepts a shortened tag; RFC 8439 defines only the full tag of ChaCha20-Poly1305
static int validTagLength(uint8_t alg, int tagLength)
{
	return (tagLength >= ((kAlgChaCha20Poly1305 == alg) ? 16 : 4)) && (tagLength <= 16);
}

void xs_cipher_constructor(xsMachine *the)
{
	CipherRecord init;
	xsmcVars(1);

	c_memset(&init, 0, sizeof(init));

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
	init.alg = entry->alg;
	init.blockSize = entry->blockSize;
	init.aead = entry->aead;

	if (!xsmcGet(xsVar(0), xsArg(0), xsID_direction))
		xsUnknownError("direction required");
	char *direction = xsmcToString(xsVar(0));
	if (0 == c_strcmp(direction, "encrypt"))
		init.direction = kDirectionEncrypt;
	else if (0 == c_strcmp(direction, "decrypt"))
		init.direction = kDirectionDecrypt;
	else
		xsRangeError("invalid direction");

	if (xsmcGet(xsVar(0), xsArg(0), xsID_padding)) {
		char *padding = xsmcToString(xsVar(0));
		if (0 == c_strcmp(padding, "pkcs7")) {
			if ((kAlgAESECB != init.alg) && (kAlgAESCBC != init.alg))
				xsRangeError("padding not supported by algorithm");
			init.padding = 1;
		}
		else if (0 != c_strcmp(padding, "none"))
			xsRangeError("invalid padding");
	}

	if (!init.aead && xsmcGet(xsVar(0), xsArg(0), xsID_tagLength))
		xsRangeError("tagLength not supported by algorithm");
	if (init.aead) {
		init.tagLength = 16;
		if (xsmcGet(xsVar(0), xsArg(0), xsID_tagLength)) {
			int tagLength = xsmcToInteger(xsVar(0));
			if (!validTagLength(init.alg, tagLength))
				xsRangeError("invalid tagLength");
			init.tagLength = (uint8_t)tagLength;
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
#if mxChaCha
	if ((kAlgChaCha20 == init.alg) || (kAlgChaCha20Poly1305 == init.alg)) {
		if (32 != keyLen)
			xsRangeError("key must be 32 bytes");
		chacha_keysetup(&init.ctx.chacha, key, 32);
	}
	else
#endif
	{
		if ((16 != keyLen) && (24 != keyLen) && (32 != keyLen))
			xsRangeError("key must be 16, 24, or 32 bytes");
		int Nk = keyLen / 4;
		init.ctx.aes.Nr = Nk + 6;
		// ECB and CBC decrypt use the inverse cipher; CTR and GCM always use the forward cipher
		enum aes_cipher_direction dir = ((kDirectionDecrypt == init.direction) && ((kAlgAESECB == init.alg) || (kAlgAESCBC == init.alg))) ? aes_cipher_decryption : aes_cipher_encryption;
		aes_keysched(key, Nk, 4, init.ctx.aes.Nr, dir, init.ctx.aes.subkey);
	}
	c_memcpy(init.key, key, keyLen);
	init.keySize = (uint8_t)keyLen;

	if (kAlgAESGCM == init.alg) {		// H = E_K(0)
		uint8_t zero[16] = {0};
		aes_encrypt4(zero, init.mac.ghash.h._8, init.ctx.aes.Nr, init.ctx.aes.subkey);
		_ghash_fix128(&init.mac.ghash.h);
	}

	if (xsmcGet(xsVar(0), xsArg(0), xsID_iv)) {
		void *iv;
		xsUnsignedValue ivLen;
		xsmcGetBufferReadable(xsVar(0), &iv, &ivLen);
		setIV(the, &init, iv, ivLen);
	}
	else if (kAlgAESECB != init.alg)
		xsUnknownError("iv required");

	if (xsmcGet(xsVar(0), xsArg(0), xsID_additionalData)) {
		if (!init.aead)
			xsRangeError("additionalData not supported by algorithm");
		{
			void *aad;
			xsUnsignedValue aadLen;
			xsmcGetBufferReadable(xsVar(0), &aad, &aadLen);
			setAdditionalData(the, &init, aad, aadLen);
		}
	}

	cipherStart(&init);

	xsmcSetHostChunk(xsThis, &init, sizeof(init));
	c_memset(&init, 0, sizeof(init));
}

void xs_cipher_close(xsMachine *the)
{
	if (!xsGetHostChunkIf(xsThis))
		return;
	Cipher cipher = xsmcGetHostChunkValidate(xsThis, xs_cipher_destructor);
#if MODDEF_CRYPT_CIPHER_CHACHA20_POLY1305
	if (cipher->aad)
		c_free(cipher->aad);
#endif
	c_memset(cipher, 0, sizeof(CipherRecord));
	xsmcSetHostData(xsThis, NULL);	// downgrade to data slot so subsequent calls throw SyntaxError
}

void xs_cipher_reset(xsMachine *the)
{
	Cipher cipher = xsmcGetHostChunkValidate(xsThis, xs_cipher_destructor);
	xsmcVars(1);

	if (xsmcArgc > 0) {
		if (xsmcGet(xsVar(0), xsArg(0), xsID_tagLength)) {
			if (!cipher->aead)
				xsRangeError("tagLength not supported by algorithm");
			int tagLength = xsmcToInteger(xsVar(0));
			if (!validTagLength(cipher->alg, tagLength))
				xsRangeError("invalid tagLength");
			cipher = xsmcGetHostChunk(xsThis);
			cipher->tagLength = (uint8_t)tagLength;
		}
		if (xsmcGet(xsVar(0), xsArg(0), xsID_iv)) {
			void *iv;
			xsUnsignedValue ivLen;
			xsmcGetBufferReadable(xsVar(0), &iv, &ivLen);
			cipher = xsmcGetHostChunk(xsThis);
			setIV(the, cipher, iv, ivLen);
		}
		if (xsmcGet(xsVar(0), xsArg(0), xsID_additionalData)) {
			if (!cipher->aead)
				xsRangeError("additionalData not supported by algorithm");
			void *aad;
			xsUnsignedValue aadLen;
			xsmcGetBufferReadable(xsVar(0), &aad, &aadLen);
			cipher = xsmcGetHostChunk(xsThis);
			setAdditionalData(the, cipher, aad, aadLen);
		}
	}

	cipher = xsmcGetHostChunk(xsThis);
	cipherStart(cipher);
}

static int overlaps(const uint8_t *a, uint32_t aLen, const uint8_t *b, uint32_t bLen)
{
	return aLen && bLen && (a < b + bLen) && (b < a + aLen);
}

static const uint8_t gEmpty[1] = {0};

// copy bytes [start, start + count) of the virtual concatenation held || data
static void copyRange(uint8_t *dst, const uint8_t *held, uint32_t heldLen, const uint8_t *data, uint32_t start, uint32_t count)
{
	while (count--) {
		*dst++ = (start < heldLen) ? held[start] : data[start - heldLen];
		start++;
	}
}

// encrypt and decrypt: the direction is fixed at construction, and the other method throws, as read does on an IO instance that only writes
static void cipherProcess(xsMachine *the, uint8_t method)
{
	Cipher cipher = xsmcGetHostChunkValidate(xsThis, xs_cipher_destructor);
	xsmcVars(1);

	if (method != cipher->direction)
		xsUnknownError((kDirectionEncrypt == method) ? "instance does not encrypt" : "instance does not decrypt");

	if (kStateDone == cipher->state)
		xsUnknownError("operation complete; reset required");

	// options
	uint8_t more = 0, hasOutput = 0;
	if (xsmcArgc > 1) {
		if (xsmcGet(xsVar(0), xsArg(1), xsID_more))
			more = xsmcTest(xsVar(0));
		if (xsmcGet(xsVar(0), xsArg(1), xsID_output))
			hasOutput = 1;
	}

	// sizes (no pointers held yet)
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
	uint32_t emitMax;		// bytes of output in this call (upper bound for pkcs7 decrypt)

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
#if mxAESBlockModes
		else if (padding) {		// only the block modes accept padding
			if ((total < 16) || (total % 16))
				xsRangeError("data length must be a multiple of blockSize");
			// peek at the padding to size the output exactly: decrypt the last block without touching the cipher state
			uint8_t last[16], prev[16], plain[16];
			copyRange(last, cipher->held, heldLen, peekData, total - 16, 16);
			if (total >= 32)
				copyRange(prev, cipher->held, heldLen, peekData, total - 32, 16);
			else
				c_memcpy(prev, cipher->chain, 16);
			aesDecryptBlock(cipher, last, plain);
			if (kAlgAESCBC == cipher->alg)
				xor16(plain, plain, prev);
			uint8_t pad = plain[15];
			uint8_t bad = (0 == pad) | (pad > 16);
			for (int i = 0; i < 16; i++)
				bad |= (i >= 16 - pad) & (plain[i] != pad);
			if (bad)
				xsUnknownError("invalid padding");
			emitMax = total - pad;
		}
#endif
		else if (isBlockMode && (total % 16))
			xsRangeError("data length must be a multiple of blockSize");
		else
			emitMax = total;
	}

	// output: allocate first, then fetch every pointer
	uint8_t *out = NULL;
	uint32_t outLen = 0;
	if (hasOutput) {
		xsmcGet(xsVar(0), xsArg(1), xsID_output);
		void *p;
		xsUnsignedValue n;
		xsmcGetBufferWritable(xsVar(0), &p, &n);
		out = p;
		outLen = n;
		if (outLen < emitMax)
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
	cipher = xsmcGetHostChunk(xsThis);

	uint32_t emitted = 0;

	if (more) {
		cipher->state = kStateMore;
		if (!trailerHold)
			processBytes(cipher, data, out, dataLen);
		else if (total <= trailerHold) {		// everything may still be the trailer
			c_memcpy(cipher->held + heldLen, data, dataLen);
			cipher->heldLen = (uint8_t)total;
		}
		else {
			if (heldLen && overlaps(out, emitMax, data, dataLen))
				xsRangeError("in-place not permitted with deferred output");
			uint32_t fromHeld = (heldLen < emitMax) ? heldLen : emitMax;
			uint32_t fromData = emitMax - fromHeld;
			processBytes(cipher, cipher->held, out, fromHeld);
			processBytes(cipher, data, out + fromHeld, fromData);
			c_memmove(cipher->held, cipher->held + fromHeld, heldLen - fromHeld);
			c_memcpy(cipher->held + (heldLen - fromHeld), data + fromData, dataLen - fromData);
			cipher->heldLen = (uint8_t)trailerHold;
		}
		emitted = emitMax;
	}
	else if (kDirectionEncrypt == direction) {
#if mxAESBlockModes
		if (padding) {
			uint32_t full = dataLen - (dataLen % 16);
			uint8_t block[16];
			uint8_t pad = (uint8_t)(16 - (dataLen % 16));
			c_memcpy(block, data + full, dataLen - full);
			c_memset(block + (dataLen - full), pad, pad);
			processBlocks(cipher, data, out, full);
			processBlocks(cipher, block, out + full, 16);
		}
		else
#endif
		{
			processBytes(cipher, data, out, dataLen);
			if (aead) {
				uint8_t tag[16];
				aeadTag(cipher, tag);
				c_memcpy(out + dataLen, tag, tagLength);
			}
		}
		emitted = emitMax;
	}
	else {
		if (heldLen && overlaps(out, emitMax, data, dataLen))
			xsRangeError("in-place not permitted with deferred output");

		uint32_t trailer = aead ? tagLength : (padding ? 16 : 0);
		uint32_t bodyLen = total - trailer;
		uint32_t fromHeld = (heldLen < bodyLen) ? heldLen : bodyLen;
		uint32_t fromData = bodyLen - fromHeld;
		uint8_t tail[16];		// the trailer bytes, from held then data
		c_memcpy(tail, cipher->held + fromHeld, heldLen - fromHeld);
		c_memcpy(tail + (heldLen - fromHeld), data + fromData, dataLen - fromData);

		processBytes(cipher, cipher->held, out, fromHeld);
		processBytes(cipher, data, out + fromHeld, fromData);
		emitted = bodyLen;

		if (aead) {
			uint8_t tag[16];
			aeadTag(cipher, tag);
			uint8_t diff = 0;
			for (int i = 0; i < tagLength; i++)
				diff |= tag[i] ^ tail[i];
			if (diff) {
				c_memset(out, 0, bodyLen);
				cipher->state = kStateDone;
				cipher->heldLen = 0;
				xsUnknownError("authentication failed");
			}
		}
#if mxAESBlockModes
		else if (padding) {
			uint8_t block[16];
			processBlocks(cipher, tail, block, 16);
			uint8_t pad = block[15];
			uint8_t bad = (0 == pad) | (pad > 16);
			for (int i = 0; i < 16; i++)
				bad |= (i >= 16 - pad) & (block[i] != pad);
			if (bad) {
				c_memset(out, 0, bodyLen);
				cipher->state = kStateDone;
				cipher->heldLen = 0;
				xsUnknownError("invalid padding");
			}
			c_memcpy(out + bodyLen, block, 16 - pad);
			emitted += 16 - pad;
		}
#endif
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
	Cipher cipher = xsmcGetHostChunkValidate(xsThis, xs_cipher_destructor);
	xsmcSetInteger(xsResult, cipher->blockSize);
}

void xs_cipher_get_keySize(xsMachine *the)
{
	Cipher cipher = xsmcGetHostChunkValidate(xsThis, xs_cipher_destructor);
	xsmcSetInteger(xsResult, cipher->keySize * 8);		// bits
}

void xs_cipher_get_tagLength(xsMachine *the)
{
	Cipher cipher = xsmcGetHostChunkValidate(xsThis, xs_cipher_destructor);
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
			case kAlgAESGCM: if (!ivLen) return 0; break;
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

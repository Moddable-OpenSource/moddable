/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

#include "xsPlatform.h"
#include "xsmc.h"

#include "fips180.h"
#include "rfc1321.h"
#include "ghash.h"

#include "kcl_symmetric.h"

#define CryptHandlePart \
	xsSlot* reference

typedef struct {
	CryptHandlePart;
} CryptHandleRecord, *CryptHandle;

#define CryptMarkHandle(THE, HANDLE) if (HANDLE) (*markRoot)(THE, (*((CryptHandle*)(HANDLE)))->reference)
#define CryptMarkReference(THE, REFERENCE) if (REFERENCE) (*markRoot)(THE, REFERENCE)

void resolveBuffer(xsMachine *the, xsSlot *slot, uint8_t **data, uint32_t *count);

/*
	Digest
*/

typedef struct {
	const char *name;
	uint32_t ctxSize;
	uint32_t digestSize;
	uint32_t blockSize;
	void (*doCreate)(void *ctx);
	void (*doUpdate)(void *ctx, const void *data, uint32_t size);
	void (*doFin)(void *ctx, uint8_t *dgst);
} digestRecord, *digest;

static const digestRecord gDigests[] ICACHE_XS6RO_ATTR = {
	{
		"MD5",
		sizeof(struct md5),
		MD5_DGSTSIZE,
		MD5_BLKSIZE,
		(void *)md5_create,
		(void *)md5_update,
		(void *)md5_fin
	},
	{
		"SHA1",
		sizeof(struct sha1),
		SHA1_DGSTSIZE,
		SHA1_BLKSIZE,
		(void *)sha1_create,
		(void *)sha1_update,
		(void *)sha1_fin
	},
	{
		"SHA256",
		sizeof(struct sha256),
		SHA256_DGSTSIZE,
		SHA256_BLKSIZE,
		(void *)sha256_create,
		(void *)sha256_update,
		(void *)sha256_fin
	},
	{
		"SHA224",
		sizeof(struct sha256),
		SHA224_DGSTSIZE,
		SHA224_BLKSIZE,
		(void *)sha224_create,
		(void *)sha224_update,
		(void *)sha224_fin
	},
	{
		"SHA512",
		sizeof(struct sha512),
		SHA512_DGSTSIZE,
		SHA512_BLKSIZE,
		(void *)sha512_create,
		(void *)sha512_update,
		(void *)sha512_fin
	},
	{
		"SHA384",
		sizeof(struct sha512),
		SHA384_DGSTSIZE,
		SHA384_BLKSIZE,
		(void *)sha384_create,
		(void *)sha384_update,
		(void *)sha384_fin
	},
	{
		"GHASH",
		sizeof(struct ghash),
		GHASH_DGSTSIZE,
		GHASH_BLKSIZE,
		(void *)_ghash_create,
		(void *)_ghash_update,
		(void *)_ghash_fin
	},
	{0}
};

typedef struct xsCryptDigestRecord xsCryptDigestRecord;
typedef xsCryptDigestRecord *xsCryptDigest;

struct xsCryptDigestRecord {
#if mxNoFunctionLength
	CryptHandlePart;
#endif
	uint32_t		digest;
	unsigned char	ctx[1];
};

void xs_crypt_Digest(xsMachine *the)
{
	char *typeName = xsmcToString(xsArg(0));
	xsCryptDigest cd;
	digest digest;

	for (digest = (void *)gDigests; digest->name; digest += 1) {
		if (0 == c_strcmp(typeName, digest->name))
			break;
	}

	if (NULL == digest->name)
		xsUnknownError("crypt: unsupported digest");

	cd = xsmcSetHostChunk(xsThis, NULL, digest->ctxSize + sizeof(xsCryptDigestRecord) - sizeof(unsigned char));
#if mxNoFunctionLength
	cd->reference = xsToReference(xsThis);
#endif
	cd->digest = digest - gDigests;

	(digest->doCreate)(cd->ctx);
}

void xs_crypt_Digest_destructor(void *data)
{
}

void xs_crypt_Digest_write(xsMachine *the)
{
	xsCryptDigest cd;
	unsigned char *data;
	uint32_t size;

	if (xsStringType == xsmcTypeOf(xsArg(0))) {
		data = (unsigned char *)xsmcToString(xsArg(0));
		size = c_strlen((char *)data);
	}
	else
		resolveBuffer(the, &xsArg(0), &data, &size);

	cd = xsmcGetHostChunk(xsThis);
	if (!cd)
		xsUnknownError("crypt: can't call digest write after close");

#ifdef __ets__
	while (size) {
		unsigned char buffer[32];
		int use = size;
		if (use > sizeof(buffer))
			use = sizeof(buffer);

		c_memcpy(buffer, data, use);		// spool through RAM as it may be in ROM
		(gDigests[cd->digest].doUpdate)(cd->ctx, buffer, use);

		data += use;
		size -= use;
	}
#else
	(gDigests[cd->digest].doUpdate)(cd->ctx, data, size);
#endif
}

void xs_crypt_Digest_close(xsMachine *the)
{
	xsCryptDigest cd = xsmcGetHostChunk(xsThis);
	int argc = xsmcArgc;

	if (argc && xsmcTest(xsArg(0))) {
		xsmcVars(1);
		xsmcSetArrayBuffer(xsVar(0), NULL, gDigests[cd->digest].ctxSize);
		cd = xsmcGetHostChunk(xsThis);
		c_memcpy(xsmcToArrayBuffer(xsVar(0)), cd->ctx, gDigests[cd->digest].ctxSize);
	}
	else
		argc = 0;

	xsmcSetArrayBuffer(xsResult, NULL, gDigests[cd->digest].digestSize);

	cd = xsmcGetHostChunk(xsThis);
	(gDigests[cd->digest].doFin)(cd->ctx, xsmcToArrayBuffer(xsResult));

	if (argc)
		c_memcpy(cd->ctx, xsmcToArrayBuffer(xsVar(0)), gDigests[cd->digest].ctxSize);
}

void xs_crypt_Digest_reset(xsMachine *the)
{
	xsCryptDigest cd = xsmcGetHostChunk(xsThis);
	(gDigests[cd->digest].doCreate)(cd->ctx);
}

void xs_crypt_Digest_get_blockSize(xsMachine *the)
{
	xsCryptDigest cd = xsmcGetHostChunk(xsThis);
	xsResult = xsInteger(gDigests[cd->digest].blockSize);
}

void xs_crypt_Digest_get_outputSize(xsMachine *the)
{
	xsCryptDigest cd = xsmcGetHostChunk(xsThis);
	xsResult = xsInteger(gDigests[cd->digest].digestSize);
}

#if !mxNoFunctionLength

void xs_cryptdigest(xsMachine *the)
{
	xsmcGet(xsResult, xsTarget, xsID("prototype"));
	xsResult = xsNewHostInstance(xsResult);
	xsThis = xsResult;
	xs_crypt_Digest(the);
}

void modInstallCryptDigest(xsMachine *the)
{
	#define kPrototype (0)
	#define kConstructor (1)
	#define kScratch (2)

	xsBeginHost(the);
	xsmcVars(3);

	xsVar(kPrototype) = xsNewHostObject(NULL);
	xsVar(kConstructor) = xsNewHostConstructor(xs_cryptdigest, 1, xsVar(kPrototype));
	xsmcSet(xsGlobal, xsID("Digest"), xsVar(kConstructor));
	xsVar(kScratch) = xsNewHostFunction(xs_crypt_Digest_write, 1);
	xsmcSet(xsVar(kPrototype), xsID("write"), xsVar(kScratch));
	xsVar(kScratch) = xsNewHostFunction(xs_crypt_Digest_close, 0);
	xsmcSet(xsVar(kPrototype), xsID("close"), xsVar(kScratch));
	xsVar(kScratch) = xsNewHostFunction(xs_crypt_Digest_reset, 0);
	xsmcSet(xsVar(kPrototype), xsID("reset"), xsVar(kScratch));
	xsmcDefine(xsVar(kPrototype), xsID("blockSize"), xsNewHostFunction(xs_crypt_Digest_get_blockSize, 0), xsIsGetter);
	xsmcDefine(xsVar(kPrototype), xsID("outputSize"), xsNewHostFunction(xs_crypt_Digest_get_outputSize, 0), xsIsGetter);

	xsEndHost(the);
}

#endif

//
// FIPS-198 HMAC implementation. See http://csrc.nist.gov/publications/fips/fips198-1/FIPS-198-1_final.pdf
// Tested by comparisons vs. the 'hmac' module in Python 2.7.
//

typedef struct {
	const char *hash;
} hmacRecord, *hmac;

struct xsCryptHMACRecord {
	digest digest;
	uint8_t *ictx;
	uint8_t *octx;
};

typedef struct xsCryptHMACRecord xsCryptHMACRecord;
typedef xsCryptHMACRecord *xsCryptHMAC;

void xs_crypt_HMAC(xsMachine *the)
{
	char *typeName = xsmcToString(xsArg(0));
	xsCryptHMAC ch;
	digest digest;
	unsigned char *data;
	unsigned char *key;
	uint32_t size;
	uint32_t left;

	// first identify the digest to use
	for (digest = (void *)gDigests; digest->name; digest += 1) {
		if (0 == c_strcmp(typeName, digest->name))
			break;
	}
	if (NULL == digest->name)
		xsUnknownError("crypt: unsupported digest for hmac");

	// allocate RAM for everything. note that ictx and octx point to offsets within the struct
	ch = c_calloc(1, (digest->ctxSize * 2) + sizeof(xsCryptHMACRecord));
	if (!ch) {
		xsUnknownError("crypt: out of memory (hmac context)");
	}
	xsmcSetHostData(xsThis, ch);

	ch->digest = digest;
	ch->ictx = ((uint8_t*)ch) + sizeof(xsCryptHMACRecord);
	ch->octx = ((uint8_t*)ch) + sizeof(xsCryptHMACRecord) + digest->ctxSize;

	// initialize the 2 hasher instances required by HMAC (i.e. "ipad" and "opad")
	(digest->doCreate)(ch->ictx);
	(digest->doCreate)(ch->octx);

	// fetch & preprocess the HMAC key; see URL above for spec
	if (xsStringType == xsmcTypeOf(xsArg(1))) {
		data = (unsigned char *)xsmcToString(xsArg(1));
		size = c_strlen((char *)data);
	}
	else {
		data = xsmcToArrayBuffer(xsArg(1));
		size = xsmcGetArrayBufferLength(xsArg(1));
	}
	key = c_calloc(1, ch->digest->blockSize);
	if (!key) {
		xsUnknownError("crypt: out of memory (hmac key)");
	}
	if (size > ch->digest->blockSize) {
		// per spec, we have to hash long keys down to single hash block size
		unsigned char* sha = c_calloc(1, ch->digest->ctxSize);
		if (!sha) {
			c_free(key);
			xsUnknownError("crypt: out of memory (hmac key hasher)");
		}
		(digest->doCreate)(sha);
		left = size;
		while (left) {
			unsigned int use = left;
			unsigned char buffer[32];
			if (use > sizeof(buffer))
				use = sizeof(buffer);

			c_memcpy(buffer, data + (size - left), use);
			(ch->digest->doUpdate)(sha, buffer, use);

			left -= use;
		}
		(ch->digest->doFin)(sha, key);
		c_free(sha);
	}
	else {
		// shorter keys pass unmolested
		c_memcpy(key, data, size);
	}

	// take prepared key and do required FIPS-specified XORs on ipad & opad with constants
        // this requires either 1 buffer of size blockSize and 2x to run, or 2 buffers; favor
        // runtime over RAM usage here
	uint8_t *ipad = c_calloc(1, ch->digest->blockSize);
	if (!ipad) {
		c_free(key);
		xsUnknownError("crypt: out of memory (ipad)");
	}
	uint8_t *opad = c_calloc(1, ch->digest->blockSize);
	if (!opad) {
		c_free(key);
		c_free(ipad);
		xsUnknownError("crypt: out of memory (opad)");
	}
	for (uint32_t i = 0; i < ch->digest->blockSize; ++i) {
		ipad[i] = 0x36 ^ key[i];
		opad[i] = 0x5c ^ key[i];
	}

	// run the now-prepared key through the ipad and opad hash instances
	(ch->digest->doUpdate)(ch->ictx, ipad, ch->digest->blockSize);
	(ch->digest->doUpdate)(ch->octx, opad, ch->digest->blockSize);
	c_free(ipad);
	c_free(opad);
	c_free(key);
}

void xs_crypt_HMAC_destructor(void *data)
{
	if (data)
		c_free(data);
}

void xs_crypt_HMAC_write(xsMachine *the)
{
	xsCryptHMAC ch = xsmcGetHostData(xsThis);
	unsigned char *data;
	uint32_t size;

	if (!ch)
		xsUnknownError("crypt: can't call hmac write after close");

	if (xsStringType == xsmcTypeOf(xsArg(0))) {
		data = (unsigned char *)xsmcToString(xsArg(0));
		size = c_strlen((char *)data);
	}
	else {
		data = xsmcToArrayBuffer(xsArg(0));
		size = xsmcGetArrayBufferLength(xsArg(0));
	}
	while (size) {
		unsigned char buffer[32];
		unsigned int use = size;
		if (use > sizeof(buffer))
			use = sizeof(buffer);

		c_memcpy(buffer, data, use);		// spool through RAM as it may be in ROM
		(ch->digest->doUpdate)(ch->ictx, buffer, use);

		data += use;
		size -= use;
	}
}

void xs_crypt_HMAC_close(xsMachine *the)
{
	xsCryptHMAC ch = xsmcGetHostData(xsThis);
	uint8_t* buffer;

	xsmcVars(1);
	xsmcSetArrayBuffer(xsVar(0), NULL, ch->digest->digestSize);
	buffer = xsmcToArrayBuffer(xsVar(0));

	// take the final hash from ipad and run it into opad (which should already be "primed" with the key
	(ch->digest->doFin)(ch->ictx, buffer);
	(ch->digest->doUpdate)(ch->octx, buffer, ch->digest->digestSize);

	// now fish out final hash from opad into return buffer
	(ch->digest->doFin)(ch->octx, buffer);

	xs_crypt_HMAC_destructor(ch);

	xsResult = xsVar(0);
	xsmcSetHostData(xsThis, NULL);
}

/*
	Block ciphers (DES, Triple DES, AES)
*/

#include "fips46.h"
#include "fips197.h"

struct des_context {
	des_subkey subkey;
	uint8_t key[8];
};

struct tdes_context {
	des_subkey subkey[3];
	uint8_t key[8*3];
	uint32_t keySize;
};

struct aes_context {
	uint32_t Nk, Nb, Nr;
	uint32_t subkey[(14+1)*4];	/* (Nr + 1) * Nb, Nr = MAX(Nk, Nb) + 6, Nb is fixed to 4 */
	uint8_t key[32];
	enum aes_cipher_direction direction;
};

enum {
	kBlockCipherDES = 1,
	kBlockCipherTDES = 2,
	kBlockCipherAES = 3
};
typedef struct {
	CryptHandlePart;
	kcl_symmetric_direction_t direction;
	uint8_t keySize;
	uint8_t blockSize;
	uint8_t kind;

	union {
		struct des_context des;
		struct tdes_context tdes;
		struct aes_context aes;
	} context;
} crypt_blockcipher_t;

void xs_crypt_cipher_destructor(void *data)
{
}

void xs_crypt_cipher_constructor(xsMachine *the)
{
	char *cipherName = xsmcToString(xsArg(0));
	crypt_blockcipher_t cipher = {0};
	size_t contextSize;
	void *key = xsmcToArrayBuffer(xsArg(1));
	xsIntegerValue keySize = xsmcGetArrayBufferLength(xsArg(1));

	if (0 == c_strcmp(cipherName, "AES")) {
		if ((16 != keySize) && (24 != keySize) && (32 != keySize))
			xsUnknownError("bad key size");
		cipher.context.aes.Nk = keySize / 4;
		cipher.context.aes.Nb = 16 / 4;
		cipher.context.aes.Nr = cipher.context.aes.Nk >= cipher.context.aes.Nb ? cipher.context.aes.Nk + 6 : cipher.context.aes.Nb + 6;
		c_memcpy(cipher.context.aes.key, key, keySize);
		cipher.blockSize = 16;
		cipher.keySize = cipher.context.aes.Nk * 4;;
		cipher.kind = kBlockCipherAES;
		contextSize = sizeof(struct aes_context);
	}
	else if (0 == c_strcmp(cipherName, "DES")) {
		if (keySize < 8)
			xsUnknownError("key too short");
		else if (keySize > 8)
			keySize = 8;
		c_memcpy(cipher.context.des.key, key, keySize);
		cipher.blockSize = 8;
		cipher.keySize = 8;
		cipher.kind = kBlockCipherDES;
		contextSize = sizeof(struct des_context);
	}
	else if (0 == c_strcmp(cipherName, "TDES")) {
		if (keySize > 8 * 3)
			keySize = 8 * 3;
		c_memcpy(cipher.context.tdes.key, key, keySize);
		if (keySize <= 8) {
			c_memcpy(&cipher.context.tdes.key[8], key, keySize);
			c_memcpy(&cipher.context.tdes.key[16], key, keySize);
		}
		else if (keySize <= 16)
			c_memcpy(&cipher.context.tdes.key[16], key, 8);
		cipher.context.tdes.keySize = keySize;
		cipher.blockSize = 8;
		cipher.keySize = cipher.context.tdes.keySize;
		cipher.kind = kBlockCipherTDES;
		contextSize = sizeof(struct tdes_context);
	}
	else
		xsUnknownError("unsupported cipher");

	cipher.direction = -1;
	cipher.reference = xsToReference(xsThis);
	xsmcSetHostChunk(xsThis, &cipher, offsetof(crypt_blockcipher_t, context) + contextSize);
}

static void xs_crypt_cipher_setDirection(crypt_blockcipher_t *cipher, kcl_symmetric_direction_t direction)
{
	if (cipher->direction != direction) {
		cipher->direction = direction;
		switch (cipher->kind) {
			case kBlockCipherDES:
				des_keysched(cipher->context.des.key, direction == KCL_DIRECTION_ENCRYPTION ? des_cipher_encryption : des_cipher_decryption, cipher->context.des.subkey);
				break;
			case kBlockCipherTDES:
				des_keysched(cipher->context.tdes.key, direction == KCL_DIRECTION_ENCRYPTION ? des_cipher_encryption : des_cipher_decryption, cipher->context.tdes.subkey[0]);
				des_keysched(&cipher->context.tdes.key[8], direction == KCL_DIRECTION_ENCRYPTION ? des_cipher_decryption : des_cipher_encryption, cipher->context.tdes.subkey[1]);
				des_keysched(&cipher->context.tdes.key[16], direction == KCL_DIRECTION_ENCRYPTION ? des_cipher_encryption : des_cipher_decryption, cipher->context.tdes.subkey[2]);
				break;
			case kBlockCipherAES:
				cipher->context.aes.direction = direction == KCL_DIRECTION_ENCRYPTION ? aes_cipher_encryption : aes_cipher_decryption;
				aes_keysched(cipher->context.aes.key, cipher->context.aes.Nk, cipher->context.aes.Nb, cipher->context.aes.Nr, cipher->context.aes.direction, cipher->context.aes.subkey);
				break;
		}
	}
}

/*
	implementation assumes key and data buffers can be read directly
	on __ets__ this will fail from ROM. data could be pulled into RAM.
*/
static void xs_crypt_cipher_process(crypt_blockcipher_t *cipher, uint8_t *data, uint8_t *result)
{
	switch (cipher->kind) {
		case kBlockCipherDES:
			des_process(data, result, cipher->context.des.subkey);
			break;
		case kBlockCipherTDES:
			des_process(data, result, cipher->context.tdes.subkey[0]);
			des_process(result, result, cipher->context.tdes.subkey[1]);
			des_process(result, result, cipher->context.tdes.subkey[2]);
			break;
		case kBlockCipherAES:
			if (cipher->context.aes.direction == aes_cipher_encryption)
				aes_encrypt4(data, result, cipher->context.aes.Nr, cipher->context.aes.subkey);
			else
				aes_decrypt4(data, result, cipher->context.aes.Nr, cipher->context.aes.subkey);
			break;
	}
}

static void xs_crypt_cipher_crypt(xsMachine *the, kcl_symmetric_direction_t direction)
{
	crypt_blockcipher_t **cipherH = xsGetHostHandle(xsThis);		//@@ xsmc version?
	crypt_blockcipher_t *cipher = *cipherH;
	int argc = xsmcArgc;
	unsigned char *data, *result;
	uint32_t size;

	xs_crypt_cipher_setDirection(cipher, direction);

	if (argc > 1 && xsmcTest(xsArg(1))) {
		if (xsmcGetArrayBufferLength(xsArg(1)) < (xsIntegerValue)cipher->blockSize)
			xsUnknownError("buffer too small");
		xsResult = xsArg(1);
	}
	else
		xsmcSetArrayBuffer(xsResult, NULL, cipher->blockSize);

	if (xsmcTypeOf(xsArg(0)) == xsStringType) {
		data = (unsigned char *)xsmcToString(xsArg(0));
		size = c_strlen((char *)data);
	}
	else {
		data = xsmcToArrayBuffer(xsArg(0));
		size = xsmcGetArrayBufferLength(xsArg(0));
	}

	cipher = *cipherH;
	if (size < cipher->blockSize)
		xsUnknownError("bad size");

	result = xsmcToArrayBuffer(xsResult);
	xs_crypt_cipher_process(cipher, data, result);
}

void xs_crypt_cipher_encrypt(xsMachine *the)
{
	xs_crypt_cipher_crypt(the, KCL_DIRECTION_ENCRYPTION);
}

void xs_crypt_cipher_decrypt(xsMachine *the)
{
	xs_crypt_cipher_crypt(the, KCL_DIRECTION_DECRYPTION);
}

void xs_crypt_cipher_get_keySize(xsMachine *the)
{
	crypt_blockcipher_t *cipher = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, cipher->keySize);
}

void xs_crypt_cipher_get_blockSize(xsMachine *the)
{
	crypt_blockcipher_t *cipher = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, cipher->blockSize);
}

/*
	Stream ciphers (RC4, ChaCha)
*/

#include "rc.h"
#include "chacha.h"

enum {
	kStreamCipherRC4 = 1,
	kStreamCipherChaCha = 2
};

typedef struct {
	CryptHandlePart;
	uint8_t kind;

	union {
		rc4_state_t rc4;
		chacha_ctx chacha;
	} context;
} crypt_streamcipher_t;

void xs_crypt_streamcipher_destructor(void *data)
{
}

void xs_crypt_streamcipher_constructor(xsMachine *the)
{
	char *cipherName = xsmcToString(xsArg(0));
	crypt_streamcipher_t stream = {0};
	size_t contextSize;
	void *key = xsmcToArrayBuffer(xsArg(1));
	xsIntegerValue keySize = xsmcGetArrayBufferLength(xsArg(1));

	if (0 == c_strcmp(cipherName, "RC4")) {
		rc4_init(&stream.context.rc4, key, keySize);

		stream.kind = kStreamCipherRC4;
		contextSize = sizeof(rc4_state_t);
	}
	else if (0 == c_strcmp(cipherName, "ChaCha")) {
		int argc = xsmcArgc;
		uint64_t counter = 0;
		void *iv = NULL;
		xsIntegerValue ivSize = 0;

		if (argc >= 3) {
			if (argc >= 4)
				counter = xsmcToInteger(xsArg(3));

			if (xsmcTest(xsArg(2))) {
				iv = xsmcToArrayBuffer(xsArg(2));
				ivSize = xsmcGetArrayBufferLength(xsArg(2));
			}
		}

		if (0 == chacha_keysetup(&stream.context.chacha, key, keySize))
			xsUnknownError("chacha_keysetup failed");
		chacha_ivsetup(&stream.context.chacha, iv, ivSize, counter);

		stream.kind = kStreamCipherChaCha;
		contextSize = sizeof(chacha_ctx);
	}
	else
		xsUnknownError("unsupported cipher");

	stream.reference = xsToReference(xsThis);
	xsmcSetHostChunk(xsThis, &stream, offsetof(crypt_streamcipher_t, context) + contextSize);
}

void xs_crypt_streamcipher_process(xsMachine *the)
{
	crypt_streamcipher_t *stream;
	int argc = xsmcArgc;
	uint32_t count;
	uint8_t *data, *result;

	if (xsStringType == xsmcTypeOf(xsArg(0)))
		count = c_strlen(xsmcToString(xsArg(0)));
	else
		resolveBuffer(the, &xsArg(0), NULL, &count);

	if ((argc >= 3) && (xsUndefinedType != xsmcTypeOf(xsArg(2)))) {
		uint32_t request = xsmcToInteger(xsArg(2));
		if (request < count)
			count = request;
	}

	if ((argc >= 2) && xsmcTest(xsArg(1))) {
		uint32_t dstCount;
		resolveBuffer(the, &xsArg(1), NULL, &dstCount);
		if (dstCount < count)
			xsUnknownError("output too small");
		xsResult = xsArg(1);
	}
	else
		xsmcSetArrayBuffer(xsResult, NULL, count);

	if (xsStringType == xsmcTypeOf(xsArg(0)))
		data = (uint8_t *)xsmcToString(xsArg(0));
	else
		resolveBuffer(the, &xsArg(0), &data, NULL);

	resolveBuffer(the, &xsResult, &result, NULL);

	stream = xsmcGetHostChunk(xsThis);
	if (kStreamCipherRC4 == stream->kind)
		rc4_process(data, result, count, &stream->context.rc4);
	else
	if (kStreamCipherChaCha == stream->kind)
		chacha_process(data, result, count, &stream->context.chacha);
}

void xs_crypt_streamcipher_setIV(xsMachine *the)
{
	crypt_streamcipher_t *stream;
	int argc = xsmcArgc;
	uint64_t counter = 0;
	void *iv = NULL;
	xsIntegerValue ivSize = 0;

	if (argc >= 1) {
		if (argc >= 2)
			counter = xsmcToInteger(xsArg(1));

		if (xsmcTest(xsArg(2))) {
			iv = xsmcToArrayBuffer(xsArg(0));
			ivSize = xsmcGetArrayBufferLength(xsArg(0));
		}
	}

	stream = xsmcGetHostChunk(xsThis);
	chacha_ivsetup(&stream->context.chacha, iv, ivSize, counter);
}

/*
	Modes (ECB, CTR, CBC)
*/

enum {
	kCryptModeECB = 1,
	kCryptModeCTR = 2,
	kCryptModeCBC = 3
};

#define CRYPT_MAX_BLOCKSIZE (16)

typedef struct crypt_mode {
	CryptHandlePart;
	crypt_blockcipher_t **cipherH;
	xsIntegerValue maxSlop;
	uint8_t kind;
	uint8_t padding;
	uint8_t eof;
	uint16_t offset;
	uint8_t em_buf[CRYPT_MAX_BLOCKSIZE];
} crypt_mode_t;

static void ctr_setIV(crypt_mode_t *mode, const uint8_t *iv, xsIntegerValue ivsize);
static void cbc_setIV(crypt_mode_t *mode, const uint8_t *iv, xsIntegerValue ivsize);
static void cbc_xor(uint8_t *t, const uint8_t *x, size_t count);

static void xs_crypt_mode_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
void xs_crypt_mode_delete(void* it);

const xsHostHooks ICACHE_FLASH_ATTR xs_crypt_mode_hooks = {
	xs_crypt_mode_delete,
	xs_crypt_mode_mark,
	NULL
};

void xs_crypt_mode_mark(xsMachine* the, void *it, xsMarkRoot markRoot)
{
	crypt_mode_t *mode = it;
	(*markRoot)(the, (*((CryptHandle *)(mode->cipherH)))->reference);
}

void xs_crypt_mode_delete(void* it)
{
}

void xs_crypt_mode_constructor(xsMachine *the)
{
	char *modeName = xsmcToString(xsArg(0));
	crypt_mode_t *mode;
	uint8_t kind;
	int argc = xsmcArgc;

	if (0 == c_strcmp(modeName, "ECB"))
		kind = kCryptModeECB;
	else if (0 == c_strcmp(modeName, "CTR"))
		kind = kCryptModeCTR;
	else if (0 == c_strcmp(modeName, "CBC"))
		kind = kCryptModeCBC;
	else
		xsUnknownError("unsupported mode");

	mode = xsmcSetHostChunk(xsThis, NULL, sizeof(crypt_mode_t));
	mode->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&xs_crypt_mode_hooks);
	mode->kind = kind;
	mode->cipherH = xsGetHostHandle(xsArg(1));

	if ((*mode->cipherH)->blockSize > CRYPT_MAX_BLOCKSIZE)
		xsUnknownError("bad blockSize");

	if (kCryptModeECB == kind) {
		mode->maxSlop = (*mode->cipherH)->blockSize;
		mode->padding = (argc >= 4) && xsmcToBoolean(xsArg(3));
	}
	else if (kCryptModeCTR == kind) {
		mode->maxSlop = 0;
		if ((argc >= 3) && xsmcTest(xsArg(2))) {
			uint8_t *iv = xsmcToArrayBuffer(xsArg(2));
			xsIntegerValue ivSize = xsmcGetArrayBufferLength(xsArg(2));
			ctr_setIV(mode, iv, ivSize);
		}
		xs_crypt_cipher_setDirection(*mode->cipherH, KCL_DIRECTION_ENCRYPTION);		// CTR uses encryption only
	}
	else if (kCryptModeCBC == kind) {
		mode->maxSlop = (*mode->cipherH)->blockSize;
		mode->padding = (argc >= 4) && xsmcToBoolean(xsArg(3));
		if ((argc >= 3) && xsmcTest(xsArg(2))) {
			uint8_t *iv = xsmcToArrayBuffer(xsArg(2));
			xsIntegerValue ivSize = xsmcGetArrayBufferLength(xsArg(2));
			cbc_setIV(mode, iv, ivSize);
		}
	}
}

void xs_crypt_mode_encrypt(xsMachine *the)
{
	crypt_mode_t *mode = xsmcGetHostChunk(xsThis);
	crypt_blockcipher_t *cipher = *mode->cipherH;
	xsIntegerValue blockSize = cipher->blockSize;
	int argc = xsmcArgc;
	uint32_t count_;
	xsIntegerValue count;
	uint8_t *data, *result, *resultStart;

	if (KCL_DIRECTION_ENCRYPTION != cipher->direction)
		xs_crypt_cipher_setDirection(cipher, KCL_DIRECTION_ENCRYPTION);

	resolveBuffer(the, &xsArg(0), NULL, &count_);
	count = count_;
	if ((argc > 1) && xsmcTest(xsArg(1))) {
		if (xsmcGetArrayBufferLength(xsArg(1)) < count)
			xsUnknownError("output too small");
		xsResult = xsArg(1);
	}
	else
		xsmcSetArrayBuffer(xsResult, NULL, count);

	resolveBuffer(the, &xsArg(0), &data, NULL);
	result = resultStart = xsmcToArrayBuffer(xsResult);

	mode = xsmcGetHostChunk(xsThis);
	cipher = *mode->cipherH;

	if (kCryptModeECB == mode->kind) {
		while (count >= blockSize) {
			xs_crypt_cipher_process(cipher, data, result);
			count -= blockSize;
			data += blockSize;
			result += blockSize;
		}
//@@ even when 0 == count?? - and if padded... shouldnt we check that result is big enough for the padded size?
		if (mode->eof && mode->padding) {
			/* process padding in the RFC2630 compatible way */
			uint8_t pad = blockSize - count;
			uint8_t tbuf[CRYPT_MAX_BLOCKSIZE];
			xsIntegerValue i;
			for (i = 0; i < count; i++)
				tbuf[i] = data[i];
			for (; i < blockSize; i++)
				tbuf[i] = pad;
			xs_crypt_cipher_process(cipher, tbuf, result);
			result += blockSize;
		}
	}
	else if (kCryptModeCTR == mode->kind) {		// duplicate of encrypt
		uint8_t *ctrbuf = mode->em_buf;
		uint8_t tbuf[CRYPT_MAX_BLOCKSIZE];

		while (count) {
			xsIntegerValue i;

			xs_crypt_cipher_process(cipher, ctrbuf, tbuf);
			for (i = mode->offset; (0 != count) && (i < blockSize); i++, --count)
				*result++ = *data++ ^ tbuf[i];

			if (count) {
				uint8_t c;

				mode->offset = 0;
				/* increment counter */
				for (i = blockSize, c = 1; --i >= 0 && (0 != c); ) {
					uint8_t t = ctrbuf[i];
					ctrbuf[i]++;
					c = ctrbuf[i] < t;
				}
			}
			else
				mode->offset = i;
		}
	}
	else if (kCryptModeCBC == mode->kind) {
		uint8_t *prev = mode->em_buf;

		while (count >= blockSize) {
			cbc_xor(prev, data, blockSize);
			xs_crypt_cipher_process(cipher, prev, result);
			c_memcpy(prev, result, blockSize);
			count -= blockSize;
			data += blockSize;
			result += blockSize;
		}
//@@ even when 0 == count?? - and if padded... shouldn't we check that result is big enough for the padded size?
		if (mode->eof && mode->padding) {
			/* process the padding method defined in RFC2630 */
			uint8_t pad[CRYPT_MAX_BLOCKSIZE];
			uint8_t padValue = blockSize - count;
			xsIntegerValue i;
			for (i = 0; i < count; i++)
				pad[i] = data[i];
			for (; i < blockSize; i++)
				pad[i] = padValue;
			cbc_xor(prev, pad, blockSize);
			xs_crypt_cipher_process(cipher, prev, result);
			result += blockSize;
		}
	}
}

void xs_crypt_mode_decrypt(xsMachine *the)
{
	crypt_mode_t *mode = xsmcGetHostChunk(xsThis);
	crypt_blockcipher_t *cipher = *mode->cipherH;
	xsIntegerValue blockSize = cipher->blockSize;
	int argc = xsmcArgc;
	uint32_t count, countStart;
	uint8_t *data, *result, *resultStart;

	if (kCryptModeCTR == mode->kind) {
		if (KCL_DIRECTION_ENCRYPTION != cipher->direction)
			xs_crypt_cipher_setDirection(cipher, KCL_DIRECTION_ENCRYPTION);
	}
	else if (KCL_DIRECTION_DECRYPTION != cipher->direction)
		xs_crypt_cipher_setDirection(cipher, KCL_DIRECTION_DECRYPTION);

	resolveBuffer(the, &xsArg(0), NULL, &count);
	if ((argc > 1) && xsmcTest(xsArg(1))) {
		uint32_t dstCount;
		resolveBuffer(the, &xsArg(1), &result, &dstCount);
		if (dstCount < count)
			xsUnknownError("output too small");
		xsResult = xsArg(1);
	}
	else {
		xsmcSetArrayBufferResizable(xsResult, NULL, count, count);
		result = xsmcToArrayBuffer(xsResult);
	}

	resultStart = result;
	countStart = count;
	resolveBuffer(the, &xsArg(0), &data, NULL);

	mode = xsmcGetHostChunk(xsThis);
	cipher = *mode->cipherH;

	if (kCryptModeECB == mode->kind) {
		while (count >= (uint32_t)blockSize) {
			xs_crypt_cipher_process(cipher, data, result);
			count -= blockSize;
			data += blockSize;
			result += blockSize;
		}
	}
	else if (kCryptModeCTR == mode->kind) {		// duplicate of decrypt
		uint8_t *ctrbuf = mode->em_buf;
		uint8_t tbuf[CRYPT_MAX_BLOCKSIZE];

		while (count) {
			xsIntegerValue i;

			xs_crypt_cipher_process(cipher, ctrbuf, tbuf);
			for (i = mode->offset; (0 != count) && (i < blockSize); i++, --count)
				*result++ = *data++ ^ tbuf[i];

			if (count) {
				uint8_t c;

				mode->offset = 0;
				/* increment counter */
				for (i = blockSize, c = 1; --i >= 0 && (0 != c); ) {
					uint8_t t = ctrbuf[i];
					ctrbuf[i]++;
					c = ctrbuf[i] < t;
				}
			}
			else
				mode->offset = i;
		}
	}
	else if (kCryptModeCBC == mode->kind) {
		while (count >= (uint32_t)blockSize) {
			uint8_t *prev = mode->em_buf;
			uint8_t tbuf[CRYPT_MAX_BLOCKSIZE], *t = tbuf;
			int i;

			xs_crypt_cipher_process(cipher, data, tbuf);
			for (i = blockSize; i-- != 0;) {
				uint8_t c = *prev;
				*prev++ = *data++;
				*result++ = c ^ *t++;
			}
			count -= blockSize;
		}
	}

	if (mode->eof && mode->padding) {
		/* process RFC2630 */
		xsIntegerValue padLen = *(result - 1);
		if (padLen > blockSize)
			padLen = 0;	/* what should we do?? */
		result -= padLen;
	}
//@@
	if ((result - resultStart) != (int)countStart)
		xsmcSetArrayBufferLength(xsResult, result - resultStart);
}

void xs_crypt_mode_setIV(xsMachine *the)
{
	crypt_mode_t *mode = xsmcGetHostChunk(xsThis);
	uint8_t *iv = xsmcToArrayBuffer(xsArg(0));
	xsIntegerValue ivSize = xsmcGetArrayBufferLength(xsArg(0));
	if (kCryptModeCTR == mode->kind)
		ctr_setIV(mode, iv, ivSize);
	else if (kCryptModeCBC == mode->kind)
		cbc_setIV(mode, iv, ivSize);
}

void xs_crypt_mode_get_eof(xsMachine *the)
{
	crypt_mode_t *mode = xsmcGetHostChunk(xsThis);
	xsmcSetBoolean(xsResult, mode->eof);
}

void xs_crypt_mode_set_eof(xsMachine *the)
{
	crypt_mode_t *mode = xsmcGetHostChunk(xsThis);
	mode->eof = xsmcToBoolean(xsArg(0));
}

void ctr_setIV(crypt_mode_t *mode, const uint8_t *iv, xsIntegerValue ivSize)
{
	crypt_blockcipher_t *cipher = *mode->cipherH;
	xsIntegerValue blockSize = cipher->blockSize;

	c_memset(mode->em_buf, 0, sizeof(mode->em_buf));
	if (ivSize > blockSize)
		ivSize = blockSize;
	c_memcpy(&mode->em_buf[blockSize - ivSize], iv, ivSize);
	mode->offset = 0;
}

void cbc_setIV(crypt_mode_t *mode, const uint8_t *iv, xsIntegerValue ivSize)
{
	crypt_blockcipher_t *cipher = *mode->cipherH;
	xsIntegerValue blockSize = cipher->blockSize;

	c_memset(mode->em_buf, 0, sizeof(mode->em_buf));
	if (ivSize > blockSize)
		ivSize = blockSize;
	c_memcpy(mode->em_buf, iv, ivSize);
}

void cbc_xor(uint8_t *t, const uint8_t *x, size_t count)
{
	while (count-- != 0)
		*t++ ^= *x++;
}

void resolveBuffer(xsMachine *the, xsSlot *slot, uint8_t **data, uint32_t *count)
{
	xsUnsignedValue aCount;
	void *aBuffer;

	xsmcGetBuffer(*slot, &aBuffer, &aCount);
	if (data) *data = aBuffer;
	if (count) *count = aCount;
}

/*
 * GHASH specific part
 */
void
xs_ghash_init(xsMachine *the)
{
	xsCryptDigest cd = xsmcGetHostChunk(xsThis);
	ghash_t *ghash = (ghash_t *)cd->ctx;
	int ac = xsmcArgc;
	size_t len;

	len = xsmcGetArrayBufferLength(xsArg(0));
	if (len > sizeof(ghash->h))
		len = sizeof(ghash->h);
	c_memcpy(&ghash->h, xsmcToArrayBuffer(xsArg(0)), len);
	_ghash_fix128(&ghash->h);
	if (ac > 1 && xsmcTest(xsArg(1))) {
		void *aad = xsmcToArrayBuffer(xsArg(1));
		len = xsmcGetArrayBufferLength(xsArg(1));
		c_memset(&ghash->y, 0, sizeof(ghash->y));
		_ghash_update(ghash, aad, len);
		c_memcpy(&ghash->y0, &ghash->y, sizeof(ghash->y));
		ghash->aad_len = len;
	}
	else {
		c_memset(&ghash->y0, 0, sizeof(ghash->y0));
		ghash->aad_len = 0;
	}
	_ghash_create(ghash);
}

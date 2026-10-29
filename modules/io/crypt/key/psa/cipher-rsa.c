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
	RSA for the Cipher class — PSA Crypto
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"
#include "psa/crypto.h"

extern psa_algorithm_t xs_crypt_hashAlgorithm(const char *name, int *length);

typedef struct {
	psa_key_id_t key;
	psa_algorithm_t alg;
	uint8_t *label;
	uint32_t labelLen;
	uint16_t bytes;
	uint8_t encrypt;
	uint8_t done;
} RSACipherRecord, *RSACipher;

static void psaCheck(xsMachine *the, psa_status_t status, const char *what)
{
	if (PSA_SUCCESS != status) {
		if (PSA_ERROR_NOT_SUPPORTED == status)
			xsUnknownError("algorithm not enabled in build");
		xsUnknownError("%s failed %d", what, (int)status);
	}
}

void xs_rsacipher_destructor(void *data)
{
	RSACipher c = data;
	if (c) {
		if (c->key)
			psa_destroy_key(c->key);
		if (c->label)
			c_free(c->label);
		c_free(c);
	}
}

// (der, type, algorithm, direction, hash, label)
void xs_rsacipher_setup(xsMachine *the)
{
	void *der;
	xsUnsignedValue derLen;
	xsmcGetBufferReadable(xsArg(0), &der, &derLen);
	uint8_t isPrivate = (0 == c_strcmp(xsmcToString(xsArg(1)), "private"));
	uint8_t oaep = (0 == c_strcmp(xsmcToString(xsArg(2)), "RSA-OAEP"));
	uint8_t encrypt = (0 == c_strcmp(xsmcToString(xsArg(3)), "encrypt"));

	RSACipher c = c_calloc(1, sizeof(RSACipherRecord));
	if (!c)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, c);
	c->encrypt = encrypt;

	if (oaep) {
		psa_algorithm_t hash = xs_crypt_hashAlgorithm(xsmcToString(xsArg(4)), NULL);
		if (PSA_ALG_NONE == hash)
			xsRangeError("unsupported hash");
		c->alg = PSA_ALG_RSA_OAEP(hash);
		if (xsUndefinedType != xsmcTypeOf(xsArg(5))) {
			void *label;
			xsUnsignedValue labelLen;
			xsmcGetBufferReadable(xsArg(5), &label, &labelLen);
			if (labelLen) {
				c->label = c_malloc(labelLen);
				if (!c->label)
					xsUnknownError("no memory");
				c_memcpy(c->label, label, labelLen);
				c->labelLen = labelLen;
			}
		}
	}
	else
		c->alg = PSA_ALG_RSA_PKCS1V15_CRYPT;

	psaCheck(the, psa_crypto_init(), "psa_crypto_init");
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attributes, isPrivate ? PSA_KEY_TYPE_RSA_KEY_PAIR : PSA_KEY_TYPE_RSA_PUBLIC_KEY);
	psa_set_key_usage_flags(&attributes, encrypt ? PSA_KEY_USAGE_ENCRYPT : PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attributes, c->alg);
	psa_status_t status = psa_import_key(&attributes, der, derLen, &c->key);
	psa_reset_key_attributes(&attributes);
	if (PSA_SUCCESS != status) {
		c->key = 0;
		if (PSA_ERROR_INVALID_ARGUMENT == status)
			xsRangeError("invalid key");
		psaCheck(the, status, "psa_import_key");
	}
	psa_get_key_attributes(c->key, &attributes);
	c->bytes = (psa_get_key_bits(&attributes) + 7) >> 3;
	psa_reset_key_attributes(&attributes);
}

void xs_rsacipher_close(xsMachine *the)
{
	if (!xsmcGetHostData(xsThis))
		return;
	RSACipher c = xsmcGetHostDataValidate(xsThis, xs_rsacipher_destructor);
	xs_rsacipher_destructor(c);
	xsmcSetHostData(xsThis, NULL);
	xsmcSetHostDestructor(xsThis, NULL);
}

void xs_rsacipher_reset(xsMachine *the)
{
	RSACipher c = xsmcGetHostDataValidate(xsThis, xs_rsacipher_destructor);
	c->done = 0;
}

// (data, output)
void xs_rsacipher_process(xsMachine *the)
{
	RSACipher c = xsmcGetHostDataValidate(xsThis, xs_rsacipher_destructor);
	if (c->done)
		xsUnknownError("operation complete; reset required");

	uint8_t hasData = (xsmcArgc > 0) && (xsUndefinedType != xsmcTypeOf(xsArg(0)));
	uint8_t hasOutput = (xsmcArgc > 1) && (xsUndefinedType != xsmcTypeOf(xsArg(1)));
	void *data = NULL;
	xsUnsignedValue dataLen = 0;
	if (hasData)
		xsmcGetBufferReadable(xsArg(0), &data, &dataLen);
	if (!c->encrypt && (dataLen != c->bytes))
		xsRangeError("ciphertext length must equal the key size");

	uint8_t *out;
	xsUnsignedValue outLen;
	if (hasOutput) {
		void *p;
		xsmcGetBufferWritable(xsArg(1), &p, &outLen);
		if (outLen < c->bytes)
			xsRangeError("output too small");
		out = p;
	}
	else {
		out = xsmcSetArrayBuffer(xsResult, NULL, c->bytes);
		outLen = c->bytes;
		if (hasData)
			xsmcGetBufferReadable(xsArg(0), &data, &dataLen);
	}
	c = xsmcGetHostData(xsThis);

	size_t length = 0;
	psa_status_t status;
	if (c->encrypt)
		status = psa_asymmetric_encrypt(c->key, c->alg, data, dataLen, c->label, c->labelLen, out, outLen, &length);
	else
		status = psa_asymmetric_decrypt(c->key, c->alg, data, dataLen, c->label, c->labelLen, out, outLen, &length);
	if (PSA_SUCCESS != status) {
		if (c->encrypt && (PSA_ERROR_INVALID_ARGUMENT == status))
			xsRangeError("message too long");
		if (!c->encrypt && ((PSA_ERROR_INVALID_PADDING == status) || (PSA_ERROR_INVALID_ARGUMENT == status)))
			xsUnknownError("decryption failed");
		psaCheck(the, status, "psa_asymmetric");
	}
	c->done = 1;
	if (hasOutput)
		xsmcSetInteger(xsResult, (int)length);
	else if (length != c->bytes)
		xsmcSetArrayBufferLength(xsResult, length);
}

void xs_rsacipher_get_blockSize(xsMachine *the)		// bytes
{
	RSACipher c = xsmcGetHostDataValidate(xsThis, xs_rsacipher_destructor);
	xsmcSetInteger(xsResult, c->bytes);
}

void xs_rsacipher_get_keySize(xsMachine *the)		// bits
{
	RSACipher c = xsmcGetHostDataValidate(xsThis, xs_rsacipher_destructor);
	xsmcSetInteger(xsResult, c->bytes * 8);
}

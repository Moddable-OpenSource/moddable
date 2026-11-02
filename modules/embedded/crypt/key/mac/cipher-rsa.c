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
	RSA for the Cipher class — macOS, SecKey: RSA-OAEP (without a label, which SecKey has no parameter for) and RSAES-PKCS1-v1_5
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"

#include "seckey-rsa.h"

typedef struct {
	SecKeyRef key;			// as given: decryption needs the private key
	SecKeyRef publicKey;	// encryption: a private SecKey does not encrypt
	SecKeyAlgorithm algorithm;
	uint16_t bytes;
	uint16_t limit;			// longest message
	uint8_t encrypt;
	uint8_t done;
} RSACipherRecord, *RSACipher;

static const SecKeyAlgorithm *gOAEPAlgorithms[] = { &kSecKeyAlgorithmRSAEncryptionOAEPSHA1, &kSecKeyAlgorithmRSAEncryptionOAEPSHA224, &kSecKeyAlgorithmRSAEncryptionOAEPSHA256, &kSecKeyAlgorithmRSAEncryptionOAEPSHA384, &kSecKeyAlgorithmRSAEncryptionOAEPSHA512 };

void xs_rsacipher_destructor(void *data)
{
	RSACipher c = data;
	if (c) {
		if (c->publicKey)
			CFRelease(c->publicKey);
		if (c->key)
			CFRelease(c->key);
		c_free(c);
	}
}

// (der, type, algorithm, direction, hash, label, size)
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

	int overhead = 11;		// PKCS#1 v1.5
	if (oaep) {
		int hash = xs_crypt_rsaHashIndex(xsmcToString(xsArg(4)));
		if (hash < 0)
			xsRangeError("unsupported hash");
		c->algorithm = *gOAEPAlgorithms[hash];
		overhead = 2 * xs_crypt_rsaHashSizes[hash] + 2;
		if (xsUndefinedType != xsmcTypeOf(xsArg(5))) {
			void *label;
			xsUnsignedValue labelLen;
			xsmcGetBufferReadable(xsArg(5), &label, &labelLen);
			if (labelLen)
				xsRangeError("label not supported by this implementation");
		}
	}
	else
		c->algorithm = kSecKeyAlgorithmRSAEncryptionPKCS1;

	xsmcGetBufferReadable(xsArg(0), &der, &derLen);
	c->key = xs_crypt_rsaKey(der, derLen, isPrivate, xsmcToInteger(xsArg(6)));
	if (!c->key)
		xsRangeError("invalid key");
	c->publicKey = isPrivate ? SecKeyCopyPublicKey(c->key) : (SecKeyRef)CFRetain(c->key);
	if (!c->publicKey)
		xsRangeError("invalid key");
	c->bytes = (uint16_t)SecKeyGetBlockSize(c->key);
	c->limit = (c->bytes > overhead) ? (c->bytes - overhead) : 0;
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
	if (c->encrypt && (dataLen > c->limit))
		xsRangeError("message too long");
	if (!c->encrypt && (dataLen != c->bytes))
		xsRangeError("ciphertext length must equal the key size");
	if (hasOutput) {		// sized before the operation, as the other backends do
		void *p;
		xsUnsignedValue outLen;
		xsmcGetBufferWritable(xsArg(1), &p, &outLen);
		if (outLen < c->bytes)
			xsRangeError("output too small");
	}

	// SecKey takes and returns its own data, so the result is known before anything is allocated on the XS heap
	CFDataRef input = CFDataCreate(NULL, data, dataLen);
	CFErrorRef error = NULL;
	CFDataRef result = NULL;
	if (input)
		result = c->encrypt ? SecKeyCreateEncryptedData(c->publicKey, c->algorithm, input, &error) : SecKeyCreateDecryptedData(c->key, c->algorithm, input, &error);
	if (input)
		CFRelease(input);
	if (error)
		CFRelease(error);
	c->done = 1;
	if (!result)
		xsUnknownError(c->encrypt ? "encryption failed" : "decryption failed");
	xsUnsignedValue length = (xsUnsignedValue)CFDataGetLength(result);

	if (hasOutput) {
		void *p;
		xsUnsignedValue outLen;
		xsmcGetBufferWritable(xsArg(1), &p, &outLen);
		c_memcpy(p, CFDataGetBytePtr(result), length);
		xsmcSetInteger(xsResult, (int)length);
	}
	else
		xsmcSetArrayBuffer(xsResult, (void *)CFDataGetBytePtr(result), length);
	CFRelease(result);
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

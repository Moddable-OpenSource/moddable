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
	ECMA-419 Crypto Signature — macOS: ECDSA via CryptoKit, RSASSA-PKCS1-v1_5 and RSA-PSS via SecKey
	SecKey's digest algorithms name the hash, check the digest's length against it, and use a PSS salt of the hash
	length; the script layer admits only that salt length.
*/

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"

#include "xscryptokit.h"
#include "ec-cryptokit.h"
#include "seckey-rsa.h"

typedef struct {
	SecKeyRef key;				// RSA, as given
	SecKeyRef publicKey;		// RSA: the key verification uses (a private SecKey does not verify)
	SecKeyAlgorithm algorithm;	// RSA: sign and verify
	const CurveEntry *curve;	// ECDSA
	uint8_t material[97];		// ECDSA: the private scalar or the public point
	uint8_t materialLength;
	uint16_t bytes;				// signature size
	uint8_t hashSize;			// RSA: the digest length the algorithm names (SecKey does not check it)
	uint8_t isPrivate;
} SignatureRecord, *Signature;

static const SecKeyAlgorithm *gPKCS1Algorithms[] = { &kSecKeyAlgorithmRSASignatureDigestPKCS1v15SHA1, &kSecKeyAlgorithmRSASignatureDigestPKCS1v15SHA224, &kSecKeyAlgorithmRSASignatureDigestPKCS1v15SHA256, &kSecKeyAlgorithmRSASignatureDigestPKCS1v15SHA384, &kSecKeyAlgorithmRSASignatureDigestPKCS1v15SHA512 };
static const SecKeyAlgorithm *gPSSAlgorithms[] = { &kSecKeyAlgorithmRSASignatureDigestPSSSHA1, &kSecKeyAlgorithmRSASignatureDigestPSSSHA224, &kSecKeyAlgorithmRSASignatureDigestPSSSHA256, &kSecKeyAlgorithmRSASignatureDigestPSSSHA384, &kSecKeyAlgorithmRSASignatureDigestPSSSHA512 };

void xs_signature_destructor(void *data)
{
	Signature s = data;
	if (s) {
		if (s->publicKey)
			CFRelease(s->publicKey);
		if (s->key)
			CFRelease(s->key);
		c_memset(s, 0, sizeof(SignatureRecord));		// a private scalar
		c_free(s);
	}
}

// (material, type, algorithm, hash, saltLength, curve, size): material is PKCS#1 DER for RSA, the scalar or the point for ECDSA
void xs_signature_setup(xsMachine *the)
{
	void *material;
	xsUnsignedValue materialLength;
	xsmcGetBufferReadable(xsArg(0), &material, &materialLength);
	uint8_t isPrivate = (0 == c_strcmp(xsmcToString(xsArg(1)), "private"));
	char *algorithm = xsmcToString(xsArg(2));

	Signature s = c_calloc(1, sizeof(SignatureRecord));
	if (!s)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, s);
	s->isPrivate = isPrivate;

	if (0 == c_strcmp(algorithm, "ECDSA")) {
		s->curve = xs_crypt_curve(xsmcToString(xsArg(5)));
		if (!s->curve)
			xsRangeError("unsupported curve");
		if (materialLength != (isPrivate ? s->curve->scalarBytes : s->curve->pointBytes))
			xsRangeError("invalid key");
		xsmcGetBufferReadable(xsArg(0), &material, &materialLength);
		c_memcpy(s->material, material, materialLength);
		s->materialLength = (uint8_t)materialLength;
		s->bytes = 2 * s->curve->scalarBytes;		// r || s
		return;
	}

	int hash = xs_crypt_rsaHashIndex(xsmcToString(xsArg(3)));
	if (hash < 0)
		xsRangeError("unsupported hash");
	s->algorithm = *((0 == c_strcmp(algorithm, "RSA-PSS")) ? gPSSAlgorithms : gPKCS1Algorithms)[hash];
	s->hashSize = (uint8_t)xs_crypt_rsaHashSizes[hash];
	xsmcGetBufferReadable(xsArg(0), &material, &materialLength);
	s->key = xs_crypt_rsaKey(material, materialLength, isPrivate, xsmcToInteger(xsArg(6)));
	if (!s->key)
		xsRangeError("invalid key");
	s->publicKey = isPrivate ? SecKeyCopyPublicKey(s->key) : (SecKeyRef)CFRetain(s->key);
	if (!s->publicKey)
		xsRangeError("invalid key");
	s->bytes = (uint16_t)SecKeyGetBlockSize(s->key);
}

void xs_signature_close(xsMachine *the)
{
	if (!xsmcGetHostData(xsThis))
		return;
	Signature s = xsmcGetHostDataValidate(xsThis, xs_signature_destructor);
	xs_signature_destructor(s);
	xsmcSetHostData(xsThis, NULL);
	xsmcSetHostDestructor(xsThis, NULL);
}

void xs_signature_sign(xsMachine *the)
{
	Signature s = xsmcGetHostDataValidate(xsThis, xs_signature_destructor);
	if (!s->isPrivate)
		xsUnknownError("private key required");

	void *digest;
	xsUnsignedValue digestLength;
	xsmcGetBufferReadable(xsArg(0), &digest, &digestLength);

	if (s->curve) {
		uint8_t *out = xsmcSetArrayBuffer(xsResult, NULL, s->bytes);
		xsmcGetBufferReadable(xsArg(0), &digest, &digestLength);		// allocation may have moved it
		s = xsmcGetHostData(xsThis);
		if (s->bytes != ckECDSASign(s->curve->curve, s->material, s->materialLength, digest, digestLength, out, s->bytes))
			xsRangeError("unsupported digest length");
		return;
	}

	if (digestLength != s->hashSize)
		xsRangeError("digest length does not match hash");
	CFDataRef data = CFDataCreate(NULL, digest, digestLength);
	CFErrorRef error = NULL;
	CFDataRef signature = data ? SecKeyCreateSignature(s->key, s->algorithm, data, &error) : NULL;
	if (data)
		CFRelease(data);
	if (error)
		CFRelease(error);
	if (!signature)
		xsUnknownError("signing failed");
	if (CFDataGetLength(signature) != s->bytes) {
		CFRelease(signature);
		xsUnknownError("internal: signature size");
	}
	xsmcSetArrayBuffer(xsResult, (void *)CFDataGetBytePtr(signature), s->bytes);
	CFRelease(signature);
}

void xs_signature_verify(xsMachine *the)
{
	Signature s = xsmcGetHostDataValidate(xsThis, xs_signature_destructor);
	void *digest, *signature;
	xsUnsignedValue digestLength, signatureLength;
	xsmcGetBufferReadable(xsArg(0), &digest, &digestLength);
	xsmcGetBufferReadable(xsArg(1), &signature, &signatureLength);

	if (s->curve) {
		xsmcSetBoolean(xsResult, 1 == ckECDSAVerify(s->curve->curve, s->material, s->materialLength, digest, digestLength, signature, signatureLength));
		return;
	}

	if (digestLength != s->hashSize) {
		xsmcSetFalse(xsResult);
		return;
	}
	CFDataRef data = CFDataCreate(NULL, digest, digestLength);
	CFDataRef sig = CFDataCreate(NULL, signature, signatureLength);
	CFErrorRef error = NULL;
	Boolean valid = (data && sig) ? SecKeyVerifySignature(s->publicKey, s->algorithm, data, sig, &error) : false;		// any failure is "not valid"
	if (error)
		CFRelease(error);
	if (sig)
		CFRelease(sig);
	if (data)
		CFRelease(data);
	xsmcSetBoolean(xsResult, valid);
}

void xs_signature_get_size(xsMachine *the)
{
	Signature s = xsmcGetHostDataValidate(xsThis, xs_signature_destructor);
	xsmcSetInteger(xsResult, s->bytes);
}

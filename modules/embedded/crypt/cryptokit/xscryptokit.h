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
	CryptoKit for the ECMA-419 crypto classes on macOS: the C side of xscryptokit.swift

	Sizes are Swift Int (long). Handles are Swift objects: free them with the matching *Free.
*/

#ifndef __XSCRYPTOKIT_H__
#define __XSCRYPTOKIT_H__

#include <stdint.h>

enum {		// hash algorithms
	ckHashMD5,
	ckHashSHA1,
	ckHashSHA256,
	ckHashSHA384,
	ckHashSHA512,
};

enum {		// HKDF modes
	ckHKDFFull,
	ckHKDFExtract,
	ckHKDFExpand,
};

enum {		// AEAD algorithms
	ckAEADGCM,
	ckAEADChaChaPoly,
};

enum {		// curves
	ckCurveP256,
	ckCurveP384,
	ckCurveX25519,
};

// running hash: the finalize of a copy, so the state continues
extern void *ckHashCreate(int32_t alg);		// NULL if unsupported
extern void ckHashUpdate(void *handle, const void *data, long count);
extern void ckHashFinalize(void *handle, void *out);
extern void ckHashReset(void *handle);
extern void ckHashFree(void *handle);

// running HMAC: the same operations as a hash (ckHashUpdate, ckHashFinalize, ckHashReset, ckHashFree)
extern void *ckHMACCreate(int32_t alg, const void *key, long keyCount);		// NULL if unsupported

// one shot; returns the bytes written or -1
extern int32_t ckHKDF(int32_t alg, int32_t mode, const void *ikm, long ikmCount, const void *salt, long saltCount, const void *info, long infoCount, void *out, long outCount);

// one shot, 16-byte tag; seal returns 0 or -1; open returns 0, 1 (authentication failed) or -1 (invalid key, nonce or tag)
extern int32_t ckAEADSeal(int32_t alg, const void *key, long keyCount, const void *nonce, long nonceCount, const void *aad, long aadCount, const void *in, long inCount, void *out, void *tag);
extern int32_t ckAEADOpen(int32_t alg, const void *key, long keyCount, const void *nonce, long nonceCount, const void *aad, long aadCount, const void *in, long inCount, const void *tag, void *out);

// scalars are raw private keys; points are X9.63 uncompressed on P-256 and P-384, u on X25519. Counts are in/out: capacity, then bytes written.
extern int32_t ckECGenerate(int32_t curve, void *scalar, long *scalarCount, void *point, long *pointCount);
extern int32_t ckECPublic(int32_t curve, const void *scalar, long scalarCount, void *point, long *pointCount);
extern int32_t ckECDSASign(int32_t curve, const void *scalar, long scalarCount, const void *digest, long digestCount, void *out, long outCount);		// r || s bytes written, or -1
extern int32_t ckECDSAVerify(int32_t curve, const void *point, long pointCount, const void *digest, long digestCount, const void *signature, long signatureCount);		// 1, 0, or -1 for an invalid point
extern int32_t ckECDH(int32_t curve, const void *scalar, long scalarCount, const void *point, long pointCount, void *out, long outCount);		// secret bytes written, or -1

#endif

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
	Web Crypto API over ECMA-419 Digest and Cipher

	Implements the subset of SubtleCrypto that symmetric primitives can support:
		digest: SHA-1, SHA-256, SHA-384, SHA-512
		encrypt/decrypt: AES-CBC, AES-CTR, AES-GCM
		sign/verify: HMAC
		generateKey/importKey/exportKey: AES-CBC, AES-CTR, AES-GCM, AES-KW, HMAC (raw, jwk); PBKDF2, HKDF (raw, import only)
		deriveBits/deriveKey: PBKDF2, HKDF
		wrapKey/unwrapKey: AES-KW (RFC 3394) and the AES encryption modes
		RSASSA-PKCS1-v1_5, RSA-PSS: sign/verify; RSA-OAEP: encrypt/decrypt; importKey/exportKey (spki, pkcs8, jwk)
		ECDSA (P-256, P-384): sign/verify; ECDH (P-256, P-384) and X25519: deriveBits/deriveKey; generateKey/importKey/exportKey (spki, pkcs8, jwk, raw)
	RSA key generation, P-521 and the EdDSA algorithms are not implemented.
	On PSA backends (ESP32) RSA-PSS signs only with saltLength equal to the hash length (OperationError otherwise) and
	verify accepts a valid signature of any salt length; see embedded:crypt/signature.

	Random values come from embedded:crypt/random, the platform's cryptographic random source.

	The module exports classes only; the application creates the instance at run time, as with WebStorage:
		globalThis.crypto = new Crypto;

	See:
		https://w3c.github.io/webcrypto/
*/

import Digest from "embedded:crypt/digest";
import Cipher from "embedded:crypt/cipher";
import Random from "embedded:crypt/random";
import Key from "embedded:crypt/key";
import Signature from "embedded:crypt/signature";
import KeyAgreement from "embedded:crypt/keyagreement";
import MAC from "embedded:crypt/MAC";
import KeyDerivation from "embedded:crypt/keyderivation";
import TextEncoder from "text/encoder";
import TextDecoder from "text/decoder";
import {DOMException} from "web/domexception";

const Hashes = {
	"SHA-1":   {length: 20, block: 64,  jwk: "1"},
	"SHA-256": {length: 32, block: 64,  jwk: "256"},
	"SHA-384": {length: 48, block: 128, jwk: "384"},
	"SHA-512": {length: 64, block: 128, jwk: "512"},
};
const AESNames = ["AES-CBC", "AES-CTR", "AES-GCM", "AES-KW"];
const AESJWK = {"AES-CBC": "CBC", "AES-CTR": "CTR", "AES-GCM": "GCM", "AES-KW": "KW"};
const GCMTagLengths = [32, 64, 96, 104, 112, 120, 128];
const AESUsages = ["encrypt", "decrypt", "wrapKey", "unwrapKey"];
const KWUsages = ["wrapKey", "unwrapKey"];
const HMACUsages = ["sign", "verify"];
const DeriveUsages = ["deriveKey", "deriveBits"];
const RSANames = ["RSASSA-PKCS1-v1_5", "RSA-PSS", "RSA-OAEP"];
const ECNames = ["ECDSA", "ECDH", "X25519"];		// X25519 is its own Web Crypto algorithm but an EC key here
const ECCurves = ["P-256", "P-384"];
const ECDSAPublicUsages = ["verify"], ECDSAPrivateUsages = ["sign"], ECDHPublicUsages = [];
const ECDSAJWKAlg = {"P-256": "ES256", "P-384": "ES384"};
// algorithm name lists per operation, built once here rather than per call
const CryptNames = [...AESNames, "RSA-OAEP"];
const DerivedKeyNames = [...AESNames, "HMAC", "PBKDF2", "HKDF"];
const GenerateNames = [...AESNames, "HMAC", ...RSANames, ...ECNames];
const ImportNames = [...AESNames, "HMAC", "PBKDF2", "HKDF", ...RSANames, ...ECNames];
const SignNames = ["HMAC", "RSASSA-PKCS1-v1_5", "RSA-PSS", "ECDSA"];
const DeriveNames = ["PBKDF2", "HKDF", "ECDH", "X25519"];
const RSAJWK = {"RSASSA-PKCS1-v1_5": "RS", "RSA-PSS": "PS", "RSA-OAEP": "RSA-OAEP"};
const RSAPublicUsages = {"RSASSA-PKCS1-v1_5": ["verify"], "RSA-PSS": ["verify"], "RSA-OAEP": ["encrypt", "wrapKey"]};
const RSAPrivateUsages = {"RSASSA-PKCS1-v1_5": ["sign"], "RSA-PSS": ["sign"], "RSA-OAEP": ["decrypt", "unwrapKey"]};
Object.freeze({Hashes, AESNames, AESJWK, GCMTagLengths, AESUsages, KWUsages, HMACUsages, DeriveUsages, RSANames, RSAJWK, RSAPublicUsages, RSAPrivateUsages, CryptNames, DerivedKeyNames, GenerateNames, ImportNames, ECNames, ECCurves, ECDSAPublicUsages, ECDSAPrivateUsages, ECDHPublicUsages, ECDSAJWKAlg, SignNames, DeriveNames}, true);

const IntegerArrays = Object.freeze([Int8Array, Uint8Array, Uint8ClampedArray, Int16Array, Uint16Array, Int32Array, Uint32Array, BigInt64Array, BigUint64Array]);

function exception(name, message) {
	return new DOMException(message, name);
}

// a detached buffer holds no bytes: Web Crypto reads it as empty (the algorithm is normalized before the data is read)
function bytesOf(source, what) {
	if (source instanceof ArrayBuffer)
		return source.detached ? new Uint8Array(0) : new Uint8Array(source);
	if (ArrayBuffer.isView(source))
		return source.buffer.detached ? new Uint8Array(0) : new Uint8Array(source.buffer, source.byteOffset, source.byteLength);
	throw new TypeError(`${what ?? "data"} must be a BufferSource`);
}

function copyOf(source, what) {
	return bytesOf(source, what).slice();
}

// algorithm argument: string or {name, ...}; names are case-insensitive
function nameOf(algorithm) {
	if ("string" === typeof algorithm)
		return algorithm;
	if (algorithm && ("object" === typeof algorithm) && ("string" === typeof algorithm.name))
		return algorithm.name;
	throw new TypeError("algorithm must be a string or an object with a name");
}

// a build may omit algorithms (see the ECMA-419 classes' supports): those are NotSupportedError here, like unknown names
function available(name) {
	if (name in Hashes)
		return Digest.supports({algorithm: name});
	if (name.startsWith("AES-"))
		return Cipher.supports({algorithm: ("AES-KW" === name) ? "AES-ECB" : name});
	if (("HKDF" === name) || ("PBKDF2" === name))
		return KeyDerivation.supports({algorithm: name});
	return true;
}

function normalizeName(algorithm, supported) {
	const name = nameOf(algorithm).toUpperCase();
	for (const candidate of supported) {
		if ((candidate.toUpperCase() === name) && available(candidate))
			return candidate;
	}
	throw exception("NotSupportedError", `unsupported algorithm ${nameOf(algorithm)}`);
}

function normalizeHash(hash) {
	return normalizeName(hash, Object.keys(Hashes));
}

function checkUsages(usages, allowed) {
	if (!Array.isArray(usages))
		throw new TypeError("usages must be an array");
	for (const usage of usages) {
		if (!allowed.includes(usage))
			throw exception("SyntaxError", `invalid key usage ${usage}`);
	}
	return Object.freeze([...new Set(usages)]);
}

function requireUsage(key, usage) {
	if (!key.usages.includes(usage))
		throw exception("InvalidAccessError", `key does not permit ${usage}`);
}

function requireKey(key, name) {
	if (!(key instanceof CryptoKey))
		throw new TypeError("key must be a CryptoKey");
	if (key.algorithm.name !== name)
		throw exception("InvalidAccessError", `key is not for ${name}`);
}

function cipherOperation(fn) {
	try {
		return fn();
	}
	catch (e) {
		if (e instanceof DOMException)
			throw e;
		throw exception("OperationError", e.message);
	}
}

// constant-time comparison for MACs and the AES-KW check: no early exit, so timing does not reveal where a mismatch is
// WebIDL [EnforceRange] unsigned long
function unsignedLong(value, what) {
	value = Number(value);
	if (!Number.isFinite(value) || (value < 0) || (value > 0xFFFFFFFF))
		throw new TypeError(`${what} out of range`);
	return Math.trunc(value);
}

function equalBytes(a, b) {
	if (a.byteLength !== b.byteLength)
		return false;
	let diff = 0;
	for (let i = 0; i < a.byteLength; i++)
		diff |= a[i] ^ b[i];
	return 0 === diff;
}

/*
	CryptoKey
*/

let keyData;		// (key) => Uint8Array; private field accessor shared within this module
let createKey;		// (data, type, extractable, algorithm, usages) => CryptoKey
const Internal = Symbol("internal");

class CryptoKey {
	#data;
	#type;
	#extractable;
	#algorithm;
	#usages;

	constructor(internal, data, type, extractable, algorithm, usages) {
		if (Internal !== internal)
			throw new TypeError("Illegal constructor");
		this.#data = data;
		this.#type = type;
		this.#extractable = extractable;
		this.#algorithm = Object.freeze(algorithm, true);
		this.#usages = usages;
	}
	get [Symbol.toStringTag]() { return "CryptoKey"; }
	get type() { return this.#type; }
	get extractable() { return this.#extractable; }
	get algorithm() { return this.#algorithm; }
	get usages() { return this.#usages; }

	static {
		keyData = key => key.#data;
		createKey = (...args) => new CryptoKey(Internal, ...args);
	}
}

/*
	primitives
*/

function digestBytes(hash, data) {
	const d = new Digest({algorithm: hash});
	d.write(data);
	const result = new Uint8Array(d.read());
	d.close();
	return result;
}


// RFC 3394 key wrap over AES-ECB
const KWIV = Uint8Array.of(0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6);		// read-only by convention: typed arrays cannot be frozen

function aesKeyWrap(kek, plain) {
	if ((plain.byteLength < 16) || (plain.byteLength % 8))
		throw exception("OperationError", "AES-KW data must be a multiple of 8 bytes, at least 16");
	const n = plain.byteLength / 8;
	const out = new Uint8Array(8 + plain.byteLength);
	out.set(KWIV);
	out.set(plain, 8);
	const cipher = new Cipher({algorithm: "AES-ECB", key: kek, direction: "encrypt"});
	const block = new Uint8Array(16);
	const A = out.subarray(0, 8);
	for (let j = 0; j <= 5; j++) {
		for (let i = 1; i <= n; i++) {
			block.set(A);
			block.set(out.subarray(i * 8, i * 8 + 8), 8);
			cipher.encrypt(block, {output: block});
			cipher.reset();
			const t = n * j + i;
			block[7] ^= t; block[6] ^= t >>> 8; block[5] ^= t >>> 16; block[4] ^= t >>> 24;
			A.set(block.subarray(0, 8));
			out.set(block.subarray(8, 16), i * 8);
		}
	}
	cipher.close();
	return out;
}

function aesKeyUnwrap(kek, wrapped) {
	if ((wrapped.byteLength < 24) || (wrapped.byteLength % 8))
		throw exception("OperationError", "AES-KW data must be a multiple of 8 bytes, at least 24");
	const n = wrapped.byteLength / 8 - 1;
	const R = wrapped.slice();
	const cipher = new Cipher({algorithm: "AES-ECB", key: kek, direction: "decrypt"});
	const block = new Uint8Array(16);
	const A = R.subarray(0, 8);
	for (let j = 5; j >= 0; j--) {
		for (let i = n; i >= 1; i--) {
			const t = n * j + i;
			block.set(A);
			block[7] ^= t; block[6] ^= t >>> 8; block[5] ^= t >>> 16; block[4] ^= t >>> 24;
			block.set(R.subarray(i * 8, i * 8 + 8), 8);
			cipher.decrypt(block, {output: block});
			cipher.reset();
			A.set(block.subarray(0, 8));
			R.set(block.subarray(8, 16), i * 8);
		}
	}
	cipher.close();
	if (!equalBytes(A, KWIV))
		throw exception("OperationError", "AES-KW integrity check failed");
	return R.slice(8);
}

/*
	SubtleCrypto
*/

function aesParams(algorithm, name) {
	const params = ("string" === typeof algorithm) ? {} : algorithm;
	switch (name) {
		case "AES-CBC": {
			const iv = bytesOf(params.iv, "iv");
			if (16 !== iv.byteLength)
				throw exception("OperationError", "AES-CBC iv must be 16 bytes");
			return {iv};
		}
		case "AES-CTR": {
			const counter = bytesOf(params.counter, "counter");
			if (16 !== counter.byteLength)
				throw exception("OperationError", "AES-CTR counter must be 16 bytes");
			const length = params.length;
			if (!Number.isInteger(length) || (length < 1) || (length > 128))
				throw exception("OperationError", "AES-CTR length must be 1 to 128");
			return {counter, length};
		}
		case "AES-GCM": {
			const iv = bytesOf(params.iv, "iv");
			if (0 === iv.byteLength)
				throw exception("OperationError", "AES-GCM iv is empty");
			const tagLength = params.tagLength ?? 128;
			if (!GCMTagLengths.includes(tagLength))
				throw exception("OperationError", "invalid AES-GCM tagLength");
			const additionalData = (undefined === params.additionalData) ? undefined : bytesOf(params.additionalData, "additionalData");
			return {iv, tagLength, additionalData};
		}
	}
}

// AES-CTR per Web Crypto: only the low `length` bits of the counter block increment, wrapping within that width
function aesCTR(key, direction, counter, length, data) {
	const blocks = Math.ceil(data.byteLength / 16);
	const options = {algorithm: "AES-CTR", key, direction, iv: counter};
	// blocks before the low bits wrap: 2^length minus their value, accumulated from the low byte up until it exceeds blocks
	let capacity = 1, scale = 1;
	for (let i = 15, bits = length; (bits > 0) && (capacity <= blocks); i--, bits -= 8) {
		capacity += (~counter[i] & ((bits >= 8) ? 0xFF : ((1 << bits) - 1))) * scale;
		scale *= 256;
	}
	if (blocks <= capacity) {
		const cipher = new Cipher(options);
		try {		// the key is held until close, also when the operation throws
			return cipher[direction](data);
		}
		finally {
			cipher.close();
		}
	}
	// wrapped counter: high bits unchanged, low `length` bits zero
	const wrapped = Uint8Array.from(counter);
	for (let i = 15, bits = length; bits > 0; i--, bits -= 8)
		wrapped[i] &= (bits >= 8) ? 0 : (0xFF << bits);
	const split = capacity * 16;
	const result = new Uint8Array(data.byteLength);
	const cipher = new Cipher(options);
	try {
		cipher[direction](data.subarray(0, split), {output: result});
		cipher.reset({iv: wrapped});
		cipher[direction](data.subarray(split), {output: result.subarray(split)});
	}
	finally {
		cipher.close();
	}
	return result.buffer;
}

function aesCrypt(direction, algorithm, key, data, usage = direction) {
	const name = normalizeName(algorithm, ["AES-CBC", "AES-CTR", "AES-GCM"]);
	requireKey(key, name);
	requireUsage(key, usage);
	const params = aesParams(algorithm, name);
	data = bytesOf(data);
	const raw = keyData(key);
	return cipherOperation(() => {
		switch (name) {
			case "AES-CBC": {
				if (("decrypt" === direction) && ((0 === data.byteLength) || (data.byteLength % 16)))
					throw exception("OperationError", "AES-CBC ciphertext length");
				const cipher = new Cipher({algorithm: name, key: raw, direction, iv: params.iv, padding: "pkcs7"});
				try {		// a decrypt with bad padding throws
					return cipher[direction](data);
				}
				finally {
					cipher.close();
				}
			}
			case "AES-CTR":
				return aesCTR(raw, direction, params.counter, params.length, data);
			case "AES-GCM": {
				const tagLength = params.tagLength >> 3;
				if (("decrypt" === direction) && (data.byteLength < tagLength))
					throw exception("OperationError", "AES-GCM ciphertext shorter than tag");
				const options = {algorithm: name, key: raw, direction, iv: params.iv, tagLength};
				if (params.additionalData)
					options.additionalData = params.additionalData;
				const cipher = new Cipher(options);
				try {		// a decrypt that fails authentication throws
					return cipher[direction](data);
				}
				finally {
					cipher.close();
				}
			}
		}
	});
}

function importAES(name, format, keyData, extractable, usages) {
	const allowed = ("AES-KW" === name) ? KWUsages : AESUsages;
	usages = checkUsages(usages, allowed);
	if ("raw" === format)
		keyData = copyOf(keyData, "keyData");
	else if ("jwk" === format) {
		if (!keyData || ("object" !== typeof keyData))
			throw new TypeError("keyData must be a JsonWebKey object");
		if ("oct" !== keyData.kty)
			throw exception("DataError", "JWK kty must be oct");
		if ("string" !== typeof keyData.k)
			throw exception("DataError", "JWK k missing");
		if ((undefined !== keyData.use) && ("enc" !== keyData.use))
			throw exception("DataError", "JWK use must be enc");
		checkJWKOps(keyData, usages);
		if ((false === keyData.ext) && extractable)
			throw exception("DataError", "JWK is not extractable");
		const alg = keyData.alg;		// checked against the key length, known once decoded
		keyData = fromBase64url(keyData.k);
		if ((undefined !== alg) && (alg !== `A${keyData.byteLength * 8}${AESJWK[name]}`))
			throw exception("DataError", "JWK alg does not match");
	}
	else
		throw exception("NotSupportedError", `unsupported key format ${format}`);
	if (![16, 24, 32].includes(keyData.byteLength))
		throw exception("DataError", "AES key must be 128, 192, or 256 bits");
	return createKey(keyData, "secret", extractable, {name, length: keyData.byteLength * 8}, usages);
}

function importHMAC(format, keyData, algorithm, extractable, usages) {
	const params = ("string" === typeof algorithm) ? {} : algorithm;
	if (undefined === params.hash)
		throw new TypeError("HMAC hash required");
	const hash = normalizeHash(params.hash);
	usages = checkUsages(usages, HMACUsages);
	if ("raw" === format)
		keyData = copyOf(keyData, "keyData");
	else if ("jwk" === format) {
		if (!keyData || ("object" !== typeof keyData))
			throw new TypeError("keyData must be a JsonWebKey object");
		if ("oct" !== keyData.kty)
			throw exception("DataError", "JWK kty must be oct");
		if ("string" !== typeof keyData.k)
			throw exception("DataError", "JWK k missing");
		if ((undefined !== keyData.alg) && (keyData.alg !== `HS${Hashes[hash].jwk}`))
			throw exception("DataError", "JWK alg does not match hash");
		if ((undefined !== keyData.use) && ("sig" !== keyData.use))
			throw exception("DataError", "JWK use must be sig");
		checkJWKOps(keyData, usages);
		if ((false === keyData.ext) && extractable)
			throw exception("DataError", "JWK is not extractable");
		keyData = fromBase64url(keyData.k);
	}
	else
		throw exception("NotSupportedError", `unsupported key format ${format}`);
	if (0 === keyData.byteLength)
		throw exception("DataError", "HMAC key is empty");
	let length = keyData.byteLength * 8;
	if (undefined !== params.length) {
		const requested = unsignedLong(params.length, "length");
		if ((requested > length) || (requested <= length - 8))
			throw exception("DataError", "HMAC length does not match key");
		length = requested;
	}
	return createKey(keyData, "secret", extractable, {name: "HMAC", hash: {name: hash}, length}, usages);
}

function importDerive(name, format, keyData, extractable, usages) {
	usages = checkUsages(usages, DeriveUsages);
	if ("raw" !== format)
		throw exception("NotSupportedError", `${name} keys are imported in raw format only`);
	if (extractable)
		throw exception("SyntaxError", `${name} keys are not extractable`);
	return createKey(copyOf(keyData, "keyData"), "secret", false, {name}, usages);
}

/*
	RSA keys: the CryptoKey's data is an embedded:crypt/key Key
*/

function rsaJWKAlg(name, hash) {
	const bits = Hashes[hash].jwk;
	if ("RSA-OAEP" === name)
		return ("SHA-1" === hash) ? "RSA-OAEP" : `RSA-OAEP-${bits}`;
	return `${RSAJWK[name]}${bits}`;
}

function rsaKeyAlgorithm(name, key, hash) {
	const jwk = key.extract("jwk");
	return {name, modulusLength: key.keySize, publicExponent: Uint8Array.fromBase64(jwk.e, {alphabet: "base64url"}), hash: {name: hash}};
}

function importRSA(name, format, keyData, algorithm, extractable, usages) {
	const params = ("string" === typeof algorithm) ? {} : algorithm;
	if (undefined === params.hash)
		throw new TypeError(`${name} hash required`);
	const hash = normalizeHash(params.hash);
	let type, keyOptions;
	if ("spki" === format) {
		type = "public";
		keyOptions = {algorithm: "RSA", format: "spki", data: bytesOf(keyData, "keyData"), extractable: true};
	}
	else if ("pkcs8" === format) {
		type = "private";
		keyOptions = {algorithm: "RSA", format: "pkcs8", data: bytesOf(keyData, "keyData"), extractable: true};
	}
	else if ("jwk" === format) {
		if (!keyData || ("object" !== typeof keyData))
			throw new TypeError("keyData must be a JsonWebKey object");
		if ("RSA" !== keyData.kty)
			throw exception("DataError", "JWK kty must be RSA");
		if (undefined !== keyData.oth)
			throw exception("DataError", "multi-prime RSA keys are not supported");
		type = (undefined === keyData.d) ? "public" : "private";
		if ((undefined !== keyData.alg) && (keyData.alg !== rsaJWKAlg(name, hash)))
			throw exception("DataError", "JWK alg does not match");
		if ((undefined !== keyData.use) && (keyData.use !== (("RSA-OAEP" === name) ? "enc" : "sig")))
			throw exception("DataError", "JWK use does not match");
		if (("private" === type) && ["p", "q", "dp", "dq", "qi"].some(member => undefined === keyData[member]))
			throw exception("DataError", "JWK private key is incomplete");
		keyOptions = {algorithm: "RSA", format: "jwk", data: keyData, extractable: true};
	}
	else
		throw exception("NotSupportedError", `unsupported key format ${format}`);

	usages = checkUsages(usages, ("public" === type) ? RSAPublicUsages[name] : RSAPrivateUsages[name]);
	if ("jwk" === format) {
		checkJWKOps(keyData, usages);
		if ((false === keyData.ext) && extractable)
			throw exception("DataError", "JWK is not extractable");
	}
	let key;
	try {
		key = new Key(keyOptions);
	}
	catch (e) {
		throw exception("DataError", e.message);
	}
	return createKey(key, type, extractable, rsaKeyAlgorithm(name, key, hash), usages);
}

function exportRSA(format, key) {
	const material = keyData(key);
	if ("jwk" === format) {
		const jwk = material.extract("jwk");
		jwk.alg = rsaJWKAlg(key.algorithm.name, key.algorithm.hash.name);
		jwk.key_ops = [...key.usages];
		jwk.ext = key.extractable;
		return jwk;
	}
	if ("spki" === format) {
		if ("public" !== key.type)
			throw exception("InvalidAccessError", "spki exports a public key");
		return material.extract("spki");
	}
	if ("pkcs8" === format) {
		if ("private" !== key.type)
			throw exception("InvalidAccessError", "pkcs8 exports a private key");
		return material.extract("pkcs8");
	}
	throw exception("NotSupportedError", `unsupported key format ${format}`);
}

// EC: ECDSA and ECDH on P-256/P-384, and X25519
function ecCurveOf(name, params) {		// the Web Crypto algorithm's curve, validated
	if ("X25519" === name)
		return "X25519";
	if (undefined === params.namedCurve)
		throw new TypeError(`${name} namedCurve required`);
	const curve = String(params.namedCurve);
	if (!ECCurves.includes(curve))
		throw exception("NotSupportedError", `unsupported curve ${curve}`);
	return curve;
}

function ecKeyAlgorithm(name, curve) {
	return ("X25519" === name) ? {name} : {name, namedCurve: curve};
}

function ecUsages(name, type) {
	if ("ECDSA" === name)
		return ("public" === type) ? ECDSAPublicUsages : ECDSAPrivateUsages;
	return ("public" === type) ? ECDHPublicUsages : DeriveUsages;
}

function importEC(name, format, keyData, algorithm, extractable, usages) {
	const params = ("string" === typeof algorithm) ? {} : algorithm;
	const curve = ecCurveOf(name, params);
	let type, keyOptions;
	if ("spki" === format) {
		type = "public";
		keyOptions = {algorithm: "EC", format: "spki", namedCurve: curve, data: bytesOf(keyData, "keyData"), extractable: true};
	}
	else if ("pkcs8" === format) {
		type = "private";
		keyOptions = {algorithm: "EC", format: "pkcs8", namedCurve: curve, data: bytesOf(keyData, "keyData"), extractable: true};
	}
	else if ("raw" === format) {
		type = "public";
		keyOptions = {algorithm: "EC", format: "raw", namedCurve: curve, data: bytesOf(keyData, "keyData"), extractable: true};
	}
	else if ("jwk" === format) {
		if (!keyData || ("object" !== typeof keyData))
			throw new TypeError("keyData must be a JsonWebKey object");
		if (keyData.kty !== (("X25519" === curve) ? "OKP" : "EC"))
			throw exception("DataError", "JWK kty does not match");
		if (keyData.crv !== curve)
			throw exception("DataError", "JWK crv does not match");
		type = (undefined === keyData.d) ? "public" : "private";
		if (("ECDSA" === name) && (undefined !== keyData.alg) && (keyData.alg !== ECDSAJWKAlg[curve]))
			throw exception("DataError", "JWK alg does not match");
		if ((undefined !== keyData.use) && (keyData.use !== (("ECDSA" === name) ? "sig" : "enc")))
			throw exception("DataError", "JWK use does not match");
		if (("string" !== typeof keyData.x) || (("X25519" !== curve) && ("string" !== typeof keyData.y)))
			throw exception("DataError", "JWK coordinates missing");
		keyOptions = {algorithm: "EC", format: "jwk", namedCurve: curve, data: keyData, extractable: true};
	}
	else
		throw exception("NotSupportedError", `unsupported key format ${format}`);

	usages = checkUsages(usages, ecUsages(name, type));
	if ("jwk" === format) {
		checkJWKOps(keyData, usages);
		if ((false === keyData.ext) && extractable)
			throw exception("DataError", "JWK is not extractable");
	}
	let key;
	try {
		key = new Key(keyOptions);
	}
	catch (e) {
		throw exception("DataError", e.message);
	}
	return createKey(key, type, extractable, ecKeyAlgorithm(name, curve), usages);
}

function exportEC(format, key) {
	const material = keyData(key);
	if ("jwk" === format) {
		const jwk = material.extract("jwk");
		if ("ECDSA" === key.algorithm.name)
			jwk.alg = ECDSAJWKAlg[key.algorithm.namedCurve];
		jwk.key_ops = [...key.usages];
		jwk.ext = key.extractable;
		return jwk;
	}
	if ("raw" === format) {
		if ("public" !== key.type)
			throw exception("InvalidAccessError", "raw exports a public key");
		return material.extract("raw");
	}
	if ("spki" === format) {
		if ("public" !== key.type)
			throw exception("InvalidAccessError", "spki exports a public key");
		return material.extract("spki");
	}
	if ("pkcs8" === format) {
		if ("private" !== key.type)
			throw exception("InvalidAccessError", "pkcs8 exports a private key");
		return material.extract("pkcs8");
	}
	throw exception("NotSupportedError", `unsupported key format ${format}`);
}

function generateEC(name, params, extractable, usages) {
	const curve = ecCurveOf(name, params);
	usages = checkUsages(usages, ("ECDSA" === name) ? [...ECDSAPublicUsages, ...ECDSAPrivateUsages] : DeriveUsages);
	const privateUsages = usages.filter(usage => ecUsages(name, "private").includes(usage));
	if (0 === privateUsages.length)
		throw exception("SyntaxError", "usages is empty");
	const publicUsages = usages.filter(usage => ecUsages(name, "public").includes(usage));
	const privateKey = new Key({algorithm: "EC", namedCurve: curve, extractable: true});
	const publicKey = new Key({algorithm: "EC", format: "raw", namedCurve: curve, data: privateKey.extract("raw"), extractable: true});
	const algorithm = ecKeyAlgorithm(name, curve);
	return {
		publicKey: createKey(publicKey, "public", true, algorithm, Object.freeze(publicUsages)),
		privateKey: createKey(privateKey, "private", extractable, algorithm, Object.freeze(privateUsages))
	};
}

function ecdsaSign(direction, algorithm, key, data, signature) {
	normalizeName(algorithm, ["ECDSA"]);
	requireKey(key, "ECDSA");
	requireUsage(key, direction);
	const params = ("string" === typeof algorithm) ? {} : algorithm;
	if (undefined === params.hash)
		throw new TypeError("ECDSA hash required");
	const hash = normalizeHash(params.hash);
	data = bytesOf(data);
	if ("verify" === direction)
		signature = bytesOf(signature, "signature");
	const digest = digestBytes(hash, data);
	const s = new Signature({algorithm: "ECDSA", key: keyData(key)});
	try {
		return cipherOperation(() => ("sign" === direction) ? s.sign(digest) : s.verify(digest, signature));
	}
	finally {
		s.close();
	}
}

function ecdhDerive(name, params, baseKey, length) {
	const publicKey = params.public;
	if (!(publicKey instanceof CryptoKey))
		throw new TypeError("public must be a CryptoKey");
	if ((publicKey.algorithm.name !== name) || ("public" !== publicKey.type))
		throw exception("InvalidAccessError", "public is not a public key of this algorithm");
	if (publicKey.algorithm.namedCurve !== baseKey.algorithm.namedCurve)
		throw exception("InvalidAccessError", "public key curve does not match");
	const agreement = new KeyAgreement({algorithm: "ECDH", key: keyData(baseKey)});
	let secret;
	try {
		secret = cipherOperation(() => new Uint8Array(agreement.derive(keyData(publicKey))));
	}
	finally {
		agreement.close();
	}
	if ((null === length) || (undefined === length))
		return secret;
	if (length > (secret.byteLength << 3))
		throw exception("OperationError", "length exceeds the shared secret");
	const result = secret.subarray(0, (length + 7) >> 3);
	if (length & 7)
		result[result.byteLength - 1] &= 0xff << (8 - (length & 7));
	return result;
}

function rsaSign(direction, algorithm, key, data, signature) {
	const name = normalizeName(algorithm, ["RSASSA-PKCS1-v1_5", "RSA-PSS"]);
	requireKey(key, name);
	requireUsage(key, direction);
	if (key.type !== (("sign" === direction) ? "private" : "public"))
		throw exception("InvalidAccessError", `${direction} needs a ${("sign" === direction) ? "private" : "public"} key`);
	const params = ("string" === typeof algorithm) ? {} : algorithm;
	const options = {algorithm: name, key: keyData(key), hash: key.algorithm.hash.name};
	if ("RSA-PSS" === name) {
		if (undefined === params.saltLength)
			throw new TypeError("RSA-PSS saltLength required");
		options.saltLength = unsignedLong(params.saltLength, "saltLength");
	}
	if (undefined !== signature)
		signature = bytesOf(signature, "signature");
	data = bytesOf(data);
	const digest = digestBytes(key.algorithm.hash.name, data);
	return cipherOperation(() => {		// a backend that cannot use these options, such as a salt length, fails here as an OperationError
		const s = new Signature(options);
		try {
			return ("sign" === direction) ? s.sign(digest) : s.verify(digest, signature);
		}
		finally {
			s.close();
		}
	});
}

function rsaCrypt(direction, algorithm, key, data, usage = direction) {
	normalizeName(algorithm, ["RSA-OAEP"]);
	requireKey(key, "RSA-OAEP");
	requireUsage(key, usage);
	if (key.type !== (("encrypt" === direction) ? "public" : "private"))
		throw exception("InvalidAccessError", `${direction} needs a ${("encrypt" === direction) ? "public" : "private"} key`);
	const params = ("string" === typeof algorithm) ? {} : algorithm;
	const options = {algorithm: "RSA-OAEP", key: keyData(key), direction, hash: key.algorithm.hash.name};
	if (undefined !== params.label)
		options.label = bytesOf(params.label, "label");
	data = bytesOf(data);
	return cipherOperation(() => {
		const cipher = new Cipher(options);
		try {		// a decrypt that fails throws
			return cipher[direction](data);
		}
		finally {
			cipher.close();
		}
	});
}

function checkJWKOps(jwk, usages) {
	if (undefined === jwk.key_ops)
		return;
	if (!Array.isArray(jwk.key_ops))
		throw exception("DataError", "JWK key_ops must be an array");
	for (const usage of usages) {
		if (!jwk.key_ops.includes(usage))
			throw exception("DataError", `JWK key_ops does not include ${usage}`);
	}
}

function fromBase64url(text) {
	try {
		return Uint8Array.fromBase64(text, {alphabet: "base64url"});
	}
	catch {
		throw exception("DataError", "JWK k is not base64url");
	}
}

function toBase64url(bytes) {
	return bytes.toBase64({alphabet: "base64url", omitPadding: true});
}

function exportRaw(key) {
	if (!key.extractable)
		throw exception("InvalidAccessError", "key is not extractable");
	return keyData(key).slice().buffer;
}

function exportJWK(key) {
	if (!key.extractable)
		throw exception("InvalidAccessError", "key is not extractable");
	const algorithm = key.algorithm;
	const data = keyData(key);
	const jwk = {kty: "oct", k: toBase64url(data)};
	if ("HMAC" === algorithm.name)
		jwk.alg = `HS${Hashes[algorithm.hash.name].jwk}`;
	else if (AESNames.includes(algorithm.name))
		jwk.alg = `A${data.byteLength * 8}${AESJWK[algorithm.name]}`;
	else
		throw exception("NotSupportedError", `${algorithm.name} keys cannot be exported as jwk`);
	jwk.key_ops = [...key.usages];
	jwk.ext = key.extractable;
	return jwk;
}

function derivedLength(derivedKeyType) {
	const name = normalizeName(derivedKeyType, DerivedKeyNames);
	const params = ("string" === typeof derivedKeyType) ? {} : derivedKeyType;
	if (AESNames.includes(name)) {
		if (![128, 192, 256].includes(params.length))
			throw exception("OperationError", "AES key length must be 128, 192, or 256");
		return params.length;
	}
	if ("HMAC" === name) {
		if (undefined === params.hash)
			throw new TypeError("HMAC hash required");
		const length = params.length ?? Hashes[normalizeHash(params.hash)].block * 8;
		if (!Number.isInteger(length) || (length <= 0))
			throw exception("TypeError", "invalid HMAC length");
		return length;
	}
	return null;		// PBKDF2 and HKDF derived keys: length is not defined
}

class SubtleCrypto {
	#random;

	constructor(internal, random) {
		if (Internal !== internal)
			throw new TypeError("Illegal constructor");
		this.#random = random;
	}
	#randomBytes(count) {
		return new Uint8Array(this.#random.read(count));
	}
	get [Symbol.toStringTag]() { return "SubtleCrypto"; }

	digest(algorithm, data) {
		return Promise.try(() => {
			const hash = normalizeHash(algorithm);
			return digestBytes(hash, bytesOf(data)).buffer;
		});
	}

	encrypt(algorithm, key, data) {
		return Promise.try(() => ("RSA-OAEP" === normalizeName(algorithm, CryptNames)) ? rsaCrypt("encrypt", algorithm, key, data) : aesCrypt("encrypt", algorithm, key, data));
	}

	decrypt(algorithm, key, data) {
		return Promise.try(() => ("RSA-OAEP" === normalizeName(algorithm, CryptNames)) ? rsaCrypt("decrypt", algorithm, key, data) : aesCrypt("decrypt", algorithm, key, data));
	}

	sign(algorithm, key, data) {
		return Promise.try(() => {
			const name = normalizeName(algorithm, SignNames);
			if ("ECDSA" === name)
				return ecdsaSign("sign", algorithm, key, data);
			if ("HMAC" !== name)
				return rsaSign("sign", algorithm, key, data);
			normalizeName(algorithm, ["HMAC"]);
			requireKey(key, "HMAC");
			requireUsage(key, "sign");
			const mac = new MAC({algorithm: "HMAC", hash: key.algorithm.hash.name, key: keyData(key)});
			mac.write(bytesOf(data));
			const result = mac.read();
			mac.close();
			return result;
		});
	}

	verify(algorithm, key, signature, data) {
		return Promise.try(() => {
			const name = normalizeName(algorithm, SignNames);
			if ("ECDSA" === name)
				return ecdsaSign("verify", algorithm, key, data, signature);
			if ("HMAC" !== name)
				return rsaSign("verify", algorithm, key, data, signature);
			normalizeName(algorithm, ["HMAC"]);
			requireKey(key, "HMAC");
			requireUsage(key, "verify");
			signature = bytesOf(signature, "signature");
			const mac = new MAC({algorithm: "HMAC", hash: key.algorithm.hash.name, key: keyData(key)});
			mac.write(bytesOf(data));
			const result = mac.verify(signature);
			mac.close();
			return result;
		});
	}

	generateKey(algorithm, extractable, usages) {
		return Promise.try(() => {
			const name = normalizeName(algorithm, GenerateNames);
			const params = ("string" === typeof algorithm) ? {} : algorithm;
			if (RSANames.includes(name))
				throw exception("NotSupportedError", "RSA key generation is not supported");
			if (ECNames.includes(name))
				return generateEC(name, params, extractable, usages);
			if (AESNames.includes(name)) {
				usages = checkUsages(usages, ("AES-KW" === name) ? KWUsages : AESUsages);
				const length = unsignedLong(params.length, "length");
				if (![128, 192, 256].includes(length))
					throw exception("OperationError", "AES key length must be 128, 192, or 256");
				if (0 === usages.length)
					throw exception("SyntaxError", "usages is empty");
				return createKey(this.#randomBytes(length >> 3), "secret", extractable, {name, length}, usages);
			}
			if (undefined === params.hash)
				throw new TypeError("HMAC hash required");
			const hash = normalizeHash(params.hash);
			usages = checkUsages(usages, HMACUsages);
			const length = (undefined === params.length) ? Hashes[hash].block * 8 : unsignedLong(params.length, "length");
			if ((0 === length) || (length % 8))
				throw exception("OperationError", "invalid HMAC length");
			if (0 === usages.length)
				throw exception("SyntaxError", "usages is empty");
			return createKey(this.#randomBytes(length >> 3), "secret", extractable, {name: "HMAC", hash: {name: hash}, length}, usages);
		});
	}

	importKey(format, keyData, algorithm, extractable, usages) {
		return Promise.try(() => {
			const name = normalizeName(algorithm, ImportNames);
			extractable = Boolean(extractable);
			let key;
			if (RSANames.includes(name))
				key = importRSA(name, format, keyData, algorithm, extractable, usages);
			else if (ECNames.includes(name))
				key = importEC(name, format, keyData, algorithm, extractable, usages);
			else if (AESNames.includes(name))
				key = importAES(name, format, keyData, extractable, usages);
			else if ("HMAC" === name)
				key = importHMAC(format, keyData, algorithm, extractable, usages);
			else
				key = importDerive(name, format, keyData, extractable, usages);
			if ((0 === key.usages.length) && ("public" !== key.type))		// secret and private keys need a usage; checked after the import, as generateKey does
				throw exception("SyntaxError", "usages is empty");
			return key;
		});
	}

	exportKey(format, key) {
		return Promise.try(() => {
			if (!(key instanceof CryptoKey))
				throw new TypeError("key must be a CryptoKey");
			if (!AESNames.includes(key.algorithm.name) && ("HMAC" !== key.algorithm.name) && !RSANames.includes(key.algorithm.name) && !ECNames.includes(key.algorithm.name))
				throw exception("NotSupportedError", `${key.algorithm.name} keys cannot be exported`);
			if (!key.extractable)
				throw exception("InvalidAccessError", "key is not extractable");
			if (RSANames.includes(key.algorithm.name))
				return exportRSA(format, key);
			if (ECNames.includes(key.algorithm.name))
				return exportEC(format, key);
			if ("raw" === format)
				return exportRaw(key);
			if ("jwk" === format)
				return exportJWK(key);
			throw exception("NotSupportedError", `unsupported key format ${format}`);
		});
	}

	deriveBits(algorithm, baseKey, length) {
		return Promise.try(() => this.#deriveBits(algorithm, baseKey, length, "deriveBits").slice().buffer);
	}

	deriveKey(algorithm, baseKey, derivedKeyType, extractable, usages) {
		return Promise.try(() => {
			const length = derivedLength(derivedKeyType);
			if (null === length)
				throw exception("OperationError", "derived key type has no length");
			const bits = this.#deriveBits(algorithm, baseKey, length, "deriveKey");
			return this.importKey("raw", bits, derivedKeyType, extractable, usages);
		});
	}

	#deriveBits(algorithm, baseKey, length, usage) {
		const name = normalizeName(algorithm, DeriveNames);
		requireKey(baseKey, name);
		requireUsage(baseKey, usage);
		const params = ("string" === typeof algorithm) ? {} : algorithm;
		if (("ECDH" === name) || ("X25519" === name)) {
			if ("private" !== baseKey.type)
				throw exception("InvalidAccessError", "baseKey must be a private key");
			return ecdhDerive(name, params, baseKey, (null === length || undefined === length) ? null : unsignedLong(length, "length"));
		}
		if (undefined === params.hash)
			throw new TypeError(`${name} hash required`);
		const hash = normalizeHash(params.hash);
		if ((null === length) || (undefined === length))
			throw exception("OperationError", "length is required");
		length = Number(length);
		if (!Number.isFinite(length) || (length < 0) || (length > 0xFFFFFFFF))
			throw new TypeError("length out of range");
		length = Math.trunc(length);
		if (length % 8)
			throw exception("OperationError", "length must be a multiple of 8");
		const bytes = length >> 3;
		if (0 === bytes) {
			bytesOf(params.salt, "salt");
			return new Uint8Array(0);
		}
		if ("PBKDF2" === name) {
			const salt = bytesOf(params.salt, "salt");
			if (!Number.isInteger(params.iterations) || (params.iterations <= 0))
				throw exception("OperationError", "iterations must be a positive integer");
			const kdf = new KeyDerivation({algorithm: "PBKDF2", hash, key: keyData(baseKey), salt, iterations: params.iterations});
			const result = new Uint8Array(kdf.read(bytes));
			kdf.close();
			return result;
		}
		const salt = bytesOf(params.salt, "salt");
		const info = bytesOf(params.info, "info");
		if (bytes > 255 * Hashes[hash].length)
			throw exception("OperationError", "HKDF length too large");
		const kdf = new KeyDerivation({algorithm: "HKDF", hash, key: keyData(baseKey), salt, info});
		const result = new Uint8Array(kdf.read(bytes));
		kdf.close();
		return result;
	}

	wrapKey(format, key, wrappingKey, wrapAlgorithm) {
		return Promise.try(async () => {
			const name = normalizeName(wrapAlgorithm, CryptNames);
			requireKey(wrappingKey, name);
			requireUsage(wrappingKey, "wrapKey");
			const exported = await this.exportKey(format, key);
			const bytes = ("jwk" === format) ? (new TextEncoder).encode(JSON.stringify(exported)) : new Uint8Array(exported);
			if ("AES-KW" === name)
				return cipherOperation(() => aesKeyWrap(keyData(wrappingKey), bytes)).buffer;
			if ("RSA-OAEP" === name)
				return rsaCrypt("encrypt", wrapAlgorithm, wrappingKey, bytes, "wrapKey");
			return aesCrypt("encrypt", wrapAlgorithm, wrappingKey, bytes, "wrapKey");
		});
	}

	unwrapKey(format, wrappedKey, unwrappingKey, unwrapAlgorithm, unwrappedKeyAlgorithm, extractable, usages) {
		return Promise.try(() => {
			const name = normalizeName(unwrapAlgorithm, CryptNames);
			requireKey(unwrappingKey, name);
			requireUsage(unwrappingKey, "unwrapKey");
			let data;		// the unwrapped key: bytes, or for "jwk" the parsed object
			if ("AES-KW" === name)
				data = cipherOperation(() => aesKeyUnwrap(keyData(unwrappingKey), bytesOf(wrappedKey, "wrappedKey")));
			else if ("RSA-OAEP" === name)
				data = new Uint8Array(rsaCrypt("decrypt", unwrapAlgorithm, unwrappingKey, wrappedKey, "unwrapKey"));
			else
				data = new Uint8Array(aesCrypt("decrypt", unwrapAlgorithm, unwrappingKey, wrappedKey, "unwrapKey"));
			if ("jwk" === format) {
				try {
					data = JSON.parse((new TextDecoder).decode(data));
				}
				catch {
					throw exception("DataError", "unwrapped key is not a JWK");
				}
			}
			return this.importKey(format, data, unwrappedKeyAlgorithm, extractable, usages);
		});
	}
}

/*
	Crypto
*/

class Crypto {
	#random = new Random({});
	#subtle = new SubtleCrypto(Internal, this.#random);

	get [Symbol.toStringTag]() { return "Crypto"; }
	get subtle() {
		return this.#subtle;
	}
	getRandomValues(array) {
		if (!IntegerArrays.some(constructor => array instanceof constructor))
			throw exception("TypeMismatchError", "getRandomValues requires an integer typed array");
		if (array.byteLength > 65536)
			throw exception("QuotaExceededError", "getRandomValues limit is 65536 bytes");
		this.#random.read((1 === array.BYTES_PER_ELEMENT) ? array : new Uint8Array(array.buffer, array.byteOffset, array.byteLength));		// in place; Random reads byte buffers
		return array;
	}
	randomUUID() {
		const b = new Uint8Array(this.#random.read(16));
		b[6] = (b[6] & 0x0f) | 0x40;
		b[8] = (b[8] & 0x3f) | 0x80;
		const h = b.toHex();
		return `${h.slice(0, 8)}-${h.slice(8, 12)}-${h.slice(12, 16)}-${h.slice(16, 20)}-${h.slice(20)}`;
	}
}

export {Crypto, SubtleCrypto, CryptoKey};
export default Crypto;

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
	RSA primitives for the KCL backends: modular exponentiation through the Montgomery natives,
	and the PKCS#1 encodings (EMSA-PKCS1-v1_5, EMSA-PSS, EME-PKCS1-v1_5, EME-OAEP).
	Used by the Signature and Cipher classes; not an application API.
*/

import {internal} from "embedded:crypt/key";
import Mont from "mont";
import Digest from "embedded:crypt/digest";
import Random from "embedded:crypt/random";

// DER DigestInfo prefixes (RFC 8017 §9.2 note 1)
const DigestInfo = {
	"MD5": "3020300c06082a864886f70d020505000410",
	"SHA-1": "3021300906052b0e03021a05000414",
	"SHA-224": "302d300d06096086480165030402040500041c",
	"SHA-256": "3031300d060960864801650304020105000420",
	"SHA-384": "3041300d060960864801650304020205000430",
	"SHA-512": "3051300d060960864801650304020305000440",
};
Object.freeze(DigestInfo);

function components(key) {
	return internal(key).components;
}

export function byteLength(key) {
	return (internal(key).size + 7) >> 3;
}

export function isPrivate(key) {
	return "private" === internal(key).type;
}

// RSAEP / RSAVP1
export function publicOp(key, x) {
	const i = internal(key);
	const c = components(key);
	if ((x < 0n) || (x >= c.n))
		throw new RangeError("value out of range");
	const mn = i.cache.mn ??= new Mont({m: c.n});		// Montgomery: n is odd; the plain Modular path divides after every multiply
	return mn.exp(x, c.e);
}

// RSADP / RSASP1 with CRT
export function privateOp(key, x) {
	const i = internal(key);
	const c = components(key);
	if ("private" !== c.type)
		throw new Error("private key required");
	if ((x < 0n) || (x >= c.n))
		throw new RangeError("value out of range");
	const cache = i.cache;
	cache.mp ??= new Mont({m: c.p});		// LR: the sliding-window path (Mont.prototype.SW) computes wrong results (Oct 2026)
	cache.mq ??= new Mont({m: c.q});
	const m1 = cache.mp.exp(x, c.dp);
	const m2 = cache.mq.exp(x, c.dq);
	let h = cache.mp.mul(c.qi, m1 - m2);
	if (h < 0n)
		h += c.p;
	return m2 + h * c.q;
}

export function I2OSP(x, length) {
	return new Uint8Array(ArrayBuffer.fromBigInt(x, length));
}

export function OS2IP(bytes) {
	const buffer = bytes.buffer;
	return BigInt.fromArrayBuffer(((0 === bytes.byteOffset) && (bytes.byteLength === buffer.byteLength)) ? buffer : bytes.slice().buffer);
}

function hashLength(hash) {
	const d = new Digest({algorithm: hash});
	const result = d.outputSize;
	d.close();
	return result;
}

function digestOf(hash, a, b, c) {		// digest of up to three parts
	const d = new Digest({algorithm: hash});
	d.write(a);
	if (b) {
		d.write(b);
		if (c)
			d.write(c);
	}
	const result = new Uint8Array(d.read());
	d.close();
	return result;
}

function randomBytes(count) {
	const r = new Random({});
	const result = new Uint8Array(r.read(count));
	r.close();
	return result;
}

function equal(a, b) {
	if (a.byteLength !== b.byteLength)
		return false;
	let diff = 0;
	for (let i = 0; i < a.byteLength; i++)
		diff |= a[i] ^ b[i];
	return 0 === diff;
}

// MGF1 (RFC 8017 B.2.1)
export function mgf1(hash, seed, length) {
	const result = new Uint8Array(length);
	const counter = new Uint8Array(4);
	const hLen = hashLength(hash);
	for (let i = 0, offset = 0; offset < length; i++, offset += hLen) {
		counter[0] = i >>> 24; counter[1] = i >>> 16; counter[2] = i >>> 8; counter[3] = i;
		const block = digestOf(hash, seed, counter);
		result.set(block.subarray(0, Math.min(hLen, length - offset)), offset);
	}
	return result;
}

// EMSA-PKCS1-v1_5 (RFC 8017 9.2): 00 01 FF.. 00 DigestInfo
export function emsaPkcs1v15(hash, digest, k) {
	const prefix = DigestInfo[hash];
	if (!prefix)
		throw new RangeError("unsupported hash");
	const t = Uint8Array.fromHex(prefix);
	const hLen = t[t.byteLength - 1];
	if (digest.byteLength !== hLen)
		throw new RangeError("digest length does not match hash");
	const tLen = t.byteLength + hLen;
	if (k < tLen + 11)
		throw new RangeError("key too small for hash");
	const em = new Uint8Array(k);
	em[1] = 0x01;
	em.fill(0xff, 2, k - tLen - 1);
	em.set(t, k - tLen);
	em.set(digest, k - hLen);
	return em;
}

// EMSA-PSS (RFC 8017 9.1); emBits = modBits - 1
// EMSA-PKCS1-v1_5 verification without building the expected encoding: structure, then DigestInfo prefix and digest
export function emsaPkcs1v15Verify(hash, digest, em) {
	const prefix = DigestInfo[hash];
	if (!prefix)
		throw new RangeError("unsupported hash");
	const t = Uint8Array.fromHex(prefix);
	const hLen = t[t.byteLength - 1], k = em.byteLength, tLen = t.byteLength + hLen;
	if ((digest.byteLength !== hLen) || (k < tLen + 11) || (0 !== em[0]) || (1 !== em[1]) || (0 !== em[k - tLen - 1]))
		return false;
	let diff = 0;
	for (let i = 2; i < k - tLen - 1; i++)
		diff |= em[i] ^ 0xff;
	for (let i = 0; i < t.byteLength; i++)
		diff |= em[k - tLen + i] ^ t[i];
	for (let i = 0; i < hLen; i++)
		diff |= em[k - hLen + i] ^ digest[i];
	return 0 === diff;
}

export function emsaPssEncode(hash, digest, saltLength, emBits) {
	const hLen = hashLength(hash);
	if (digest.byteLength !== hLen)
		throw new RangeError("digest length does not match hash");
	const emLen = (emBits + 7) >> 3;
	if (emLen < hLen + saltLength + 2)
		throw new RangeError("key too small for salt");
	const salt = saltLength ? randomBytes(saltLength) : new Uint8Array(0);
	const h = digestOf(hash, new Uint8Array(8), digest, salt);
	const psLength = emLen - saltLength - hLen - 2;
	const db = new Uint8Array(emLen - hLen - 1);
	db[psLength] = 0x01;
	db.set(salt, psLength + 1);
	const mask = mgf1(hash, h, db.byteLength);
	for (let i = 0; i < db.byteLength; i++)
		db[i] ^= mask[i];
	db[0] &= 0xff >> (8 * emLen - emBits);
	const em = new Uint8Array(emLen);
	em.set(db);
	em.set(h, db.byteLength);
	em[emLen - 1] = 0xbc;
	return em;
}

export function emsaPssVerify(hash, digest, em, saltLength, emBits) {
	const hLen = hashLength(hash);
	const emLen = (emBits + 7) >> 3;
	if ((digest.byteLength !== hLen) || (em.byteLength !== emLen) || (emLen < hLen + saltLength + 2) || (0xbc !== em[emLen - 1]))
		return false;
	const db = em.slice(0, emLen - hLen - 1);
	const h = em.subarray(emLen - hLen - 1, emLen - 1);
	if (db[0] & ~(0xff >> (8 * emLen - emBits)))
		return false;
	const mask = mgf1(hash, h, db.byteLength);
	for (let i = 0; i < db.byteLength; i++)
		db[i] ^= mask[i];
	db[0] &= 0xff >> (8 * emLen - emBits);
	const psLength = emLen - hLen - saltLength - 2;
	for (let i = 0; i < psLength; i++)
		if (0 !== db[i]) return false;
	if (0x01 !== db[psLength])
		return false;
	const salt = db.subarray(psLength + 1);
	return equal(digestOf(hash, new Uint8Array(8), digest, salt), h);
}

// EME-PKCS1-v1_5 (RFC 8017 7.2): 00 02 PS(nonzero, >= 8) 00 M
export function emePkcs1v15Encode(message, k) {
	if (message.byteLength > k - 11)
		throw new RangeError("message too long");
	const em = new Uint8Array(k);
	em[1] = 0x02;
	const psLength = k - message.byteLength - 3;
	const ps = randomBytes(psLength);
	for (let i = 0; i < psLength; i++) {
		while (0 === ps[i])
			ps[i] = randomBytes(1)[0];
	}
	em.set(ps, 2);
	em.set(message, k - message.byteLength);
	return em;
}

export function emePkcs1v15Decode(em) {
	if ((0 !== em[0]) || (0x02 !== em[1]))
		return undefined;
	let i = 2;
	while ((i < em.byteLength) && (0 !== em[i]))
		i++;
	if ((i >= em.byteLength) || (i < 10))
		return undefined;
	return em.slice(i + 1);
}

// EME-OAEP (RFC 8017 7.1)
export function emeOaepEncode(hash, label, message, k) {
	const hLen = hashLength(hash);
	if (message.byteLength > k - 2 * hLen - 2)
		throw new RangeError("message too long");
	const lHash = digestOf(hash, label);
	const db = new Uint8Array(k - hLen - 1);
	db.set(lHash);
	db[k - message.byteLength - hLen - 2] = 0x01;
	db.set(message, k - hLen - 1 - message.byteLength);
	const seed = randomBytes(hLen);
	const dbMask = mgf1(hash, seed, db.byteLength);
	for (let i = 0; i < db.byteLength; i++)
		db[i] ^= dbMask[i];
	const seedMask = mgf1(hash, db, hLen);
	for (let i = 0; i < hLen; i++)
		seed[i] ^= seedMask[i];
	const em = new Uint8Array(k);
	em.set(seed, 1);
	em.set(db, 1 + hLen);
	return em;
}

export function emeOaepDecode(hash, label, em) {
	const hLen = hashLength(hash);
	const k = em.byteLength;
	if ((k < 2 * hLen + 2) || (0 !== em[0]))
		return undefined;
	const seed = em.slice(1, 1 + hLen);
	const db = em.slice(1 + hLen);
	const seedMask = mgf1(hash, db, hLen);
	for (let i = 0; i < hLen; i++)
		seed[i] ^= seedMask[i];
	const dbMask = mgf1(hash, seed, db.byteLength);
	for (let i = 0; i < db.byteLength; i++)
		db[i] ^= dbMask[i];
	const lHash = digestOf(hash, label);
	let bad = equal(db.subarray(0, hLen), lHash) ? 0 : 1;
	let i = hLen;
	while ((i < db.byteLength) && (0 === db[i]))
		i++;
	if ((i >= db.byteLength) || (0x01 !== db[i]))
		bad = 1;
	return bad ? undefined : db.slice(i + 1);
}

export function hashSize(hash) {
	return hashLength(hash);
}

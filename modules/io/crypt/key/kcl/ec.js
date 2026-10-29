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
	ECMA-419 Key — EC provider (script): generation, ECDH, ECDSA for P-256 and P-384 over the EC point arithmetic in C,
	X25519 over the ladder in C. Scalars and points cross this module as bytes (big-endian scalars, uncompressed points),
	except X25519 which is little-endian throughout.
*/

import EC from "ec";
import Mont from "mont";
import x25519 from "x25519";
import Random from "embedded:crypt/random";

const curves = {
	"P-256": {
		bytes: 32,
		p: 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFn,
		a: 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFCn,
		b: 0x5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604bn,
		Gx: 0x6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296n,
		Gy: 0x4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5n,
		n: 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551n,
	},
	"P-384": {
		bytes: 48,
		p: 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFFn,
		a: 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFCn,
		b: 0xB3312FA7E23EE7E4988E056BE3F82D19181D9C6EFE8141120314088F5013875AC656398D8A2ED19D2A85C8EDD3EC2AEFn,
		Gx: 0xAA87CA22BE8B05378EB1C71EF320AD746E1D3B628BA79B9859F741E082542A385502F25DBF55296C3A545E3872760AB7n,
		Gy: 0x3617DE4A96262C6F5D9E98BF9292DC29F8F41DBD289A147CE9DA3113B5F0B8C00A60B1CE1D7E819D7A431D7C90EA0E5Fn,
		n: 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF581A0DB248B0A77AECEC196ACCC52973n,
	},
};
const basePoint = new Uint8Array(32);
basePoint[0] = 9;
Object.freeze({curves, basePoint}, true);

function bigEndian(bytes) {
	return BigInt.fromArrayBuffer(bytes.slice().buffer);
}

function point(curve, bytes) {		// uncompressed point bytes -> ECPoint
	const size = curve.bytes;
	return new ECPoint(bigEndian(bytes.subarray(1, 1 + size)), bigEndian(bytes.subarray(1 + size)));
}

function pointBytes(curve, P) {
	const size = curve.bytes;
	const result = new Uint8Array(1 + 2 * size);
	result[0] = 4;
	result.set(new Uint8Array(ArrayBuffer.fromBigInt(P.x, size)), 1);
	result.set(new Uint8Array(ArrayBuffer.fromBigInt(P.y, size)), 1 + size);
	return result;
}

// a curve's parameters with an EC instance and base point for one operation (the table stays frozen); the natives take
// any object with x, y, z for a point, so points are plain objects here
function arith(name) {
	const curve = curves[name];
	if (!curve)
		throw new RangeError("unsupported curve");
	return {bytes: curve.bytes, n: curve.n, ec: new EC(curve.a, curve.b, curve.p), G: {x: curve.Gx, y: curve.Gy, z: 1n}};
}

// uniform in [1, n - 1] by rejection; the Random instance is the caller's
function randomScalar(random, n, bytes) {
	while (true) {
		const k = BigInt.fromArrayBuffer(random.read(bytes));
		if ((k > 0n) && (k < n))
			return k;
	}
}

// components of a new private key
export function generate(name) {
	const random = new Random({});
	let result;
	if ("X25519" === name) {
		const d = new Uint8Array(random.read(32));
		result = {type: "private", curve: name, d, u: new Uint8Array(x25519(d, basePoint))};
	}
	else {
		const curve = arith(name);
		const d = randomScalar(random, curve.n, curve.bytes);
		const Q = curve.ec._mul(curve.G, d);
		result = {type: "private", curve: name, d, x: Q.x, y: Q.y};
	}
	random.close();
	return result;
}

// public point bytes for a private scalar (bytes): for private key encodings that omit the public key
export function publicFromPrivate(name, d) {
	if ("X25519" === name)
		return new Uint8Array(x25519(d, basePoint));
	const curve = arith(name);
	const Q = curve.ec._mul(curve.G, bigEndian(d));
	const result = new Uint8Array(1 + 2 * curve.bytes);
	result[0] = 4;
	result.set(new Uint8Array(ArrayBuffer.fromBigInt(Q.x, curve.bytes)), 1);
	result.set(new Uint8Array(ArrayBuffer.fromBigInt(Q.y, curve.bytes)), 1 + curve.bytes);
	return result;
}

// ECDH: shared secret bytes from our components and the peer's
export function agree(c, peer) {
	if ("X25519" === c.curve) {
		const result = new Uint8Array(x25519(c.d, peer.u));
		let any = 0;
		for (let i = 0; i < 32; i++)
			any |= result[i];
		if (!any)
			throw new Error("X25519: low-order point");		// RFC 7748 section 6.1
		return result;
	}
	const curve = arith(c.curve);
	const P = curve.ec._mul({x: peer.x, y: peer.y, z: 1n}, c.d);
	return new Uint8Array(ArrayBuffer.fromBigInt(P.x, curve.bytes));
}

// the leftmost bits of the digest, as many as the order has (FIPS 186-4 6.4)
function truncate(n, digest) {
	let e = bigEndian(digest);
	const excess = (digest.byteLength << 3) - BigInt.bitLength(n);
	if (excess > 0)
		e >>= BigInt(excess);
	return e;
}

// ECDSA sign: r || s (bytes), each the curve's byte length
export function sign(c, digest) {
	const curve = arith(c.curve);
	const n = new Mont({m: curve.n});
	const e = truncate(curve.n, digest);
	const random = new Random({});
	let r, s;
	do {
		const k = randomScalar(random, curve.n, curve.bytes);
		r = n.mod(curve.ec._mul(curve.G, k).x);
		if (0n === r)
			continue;
		s = n.mul(n.add(e, n.mul(c.d, r)), n.mulinv(k));
	} while (0n === s);
	random.close();
	const result = new Uint8Array(2 * curve.bytes);
	result.set(new Uint8Array(ArrayBuffer.fromBigInt(r, curve.bytes)));
	result.set(new Uint8Array(ArrayBuffer.fromBigInt(s, curve.bytes)), curve.bytes);
	return result;
}

// ECDSA verify: r || s (bytes) against the public components
export function verify(c, digest, signature) {
	const curve = arith(c.curve);
	if (signature.byteLength !== 2 * curve.bytes)
		return false;
	const r = bigEndian(signature.subarray(0, curve.bytes)), s = bigEndian(signature.subarray(curve.bytes));
	if ((r <= 0n) || (r >= curve.n) || (s <= 0n) || (s >= curve.n))
		return false;
	const n = new Mont({m: curve.n});
	const w = n.mulinv(s);
	const R = curve.ec._mul2(curve.G, n.mul(truncate(curve.n, digest), w), {x: c.x, y: c.y, z: 1n}, n.mul(r, w));
	return (0n !== R.z) && (n.mod(R.x) === r);
}

function supportsNative(name) { return native("xs_crypt_ec_supports").call(this, name); }
export function supports(name) {		// whether this build includes the curve (manifest defines crypt.curve.*)
	return supportsNative(name);
}

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
	ECMA-419 Cryptographic Key: RSA and EC (P-256, P-384, X25519) keys in their standard encodings.
	Components are never exposed; the operation classes reach the material through `internal`.
	`data` omitted with algorithm "EC" generates a private key (the platform provides embedded:crypt/key/ec).
*/

import BER from "ber";
import Modules from "modules";

const Formats = ["pkcs1", "spki", "pkcs8", "jwk", "sec1", "raw"];
const Curves = ["P-256", "P-384", "X25519"];
const rsaAlgorithmIdentifier = Uint8Array.of(0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00);
const ecPublicKeyOID = Uint8Array.of(0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01);		// 1.2.840.10045.2.1
const x25519OID = Uint8Array.of(0x06, 0x03, 0x2b, 0x65, 0x6e);		// 1.3.101.110
const curveOIDs = {"P-256": Uint8Array.of(0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07), "P-384": Uint8Array.of(0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x22)};
const curveByOID = {"1,2,840,10045,3,1,7": "P-256", "1,3,132,0,34": "P-384"};
const curveBytes = {"P-256": 32, "P-384": 48, "X25519": 32};
// field prime and curve equation constants, for decompressing points (y^2 = x^3 + ax + b; both primes are 3 mod 4)
const curveField = {
	"P-256": {p: 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFn, a: 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFCn, b: 0x5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604bn},
	"P-384": {p: 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFFn, a: 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFCn, b: 0xB3312FA7E23EE7E4988E056BE3F82D19181D9C6EFE8141120314088F5013875AC656398D8A2ED19D2A85C8EDD3EC2AEFn},
};
const ECProvider = "embedded:crypt/key/ec";
Object.freeze({Formats, Curves, rsaAlgorithmIdentifier, ecPublicKeyOID, x25519OID, curveOIDs, curveByOID, curveBytes, curveField}, true);

export let internal;		// (key) => Internal: for the operation classes

/*
	DER helpers (parsing through BER; building by hand, which is a few bytes of length encoding)
*/

function bytesOf(data, what) {
	if (data instanceof ArrayBuffer)
		return new Uint8Array(data);
	if (ArrayBuffer.isView(data))
		return new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
	if ((typeof data === "object") && (undefined !== data?.byteLength))		// a host buffer, such as a Resource
		return new Uint8Array(data);
	throw new TypeError(`${what ?? "data"} must be a byte buffer`);
}

// parts: Uint8Array, or a number for a single byte
function derNode(tag, parts) {
	let length = 0;
	for (let i = 0; i < parts.length; i++) {
		const part = parts[i];
		length += ("number" === typeof part) ? 1 : part.byteLength;
	}
	const headLength = (length < 0x80) ? 2 : (length < 0x100) ? 3 : (length < 0x10000) ? 4 : 0;
	if (!headLength)
		throw new RangeError("DER too long");
	const result = new Uint8Array(headLength + length);
	result[0] = tag;
	if (2 === headLength)
		result[1] = length;
	else if (3 === headLength) {
		result[1] = 0x81;
		result[2] = length;
	}
	else {
		result[1] = 0x82;
		result[2] = length >> 8;
		result[3] = length & 0xff;
	}
	let offset = headLength;
	for (let i = 0; i < parts.length; i++) {
		const part = parts[i];
		if ("number" === typeof part)
			result[offset++] = part;
		else {
			result.set(part, offset);
			offset += part.byteLength;
		}
	}
	return result;
}

function derInteger(value) {		// non-negative BigInt
	const bytes = new Uint8Array(ArrayBuffer.fromBigInt(value));
	return derNode(0x02, (bytes[0] & 0x80) ? [0, bytes] : [bytes]);
}

function expectSequence(ber) {
	if (0x30 !== ber.getTag())
		throw new RangeError("invalid key encoding");
	ber.getLength();
}

function expectOctetString(ber) {
	if (0x04 !== ber.getTag())
		throw new RangeError("invalid key encoding");
	return ber.getChunk(ber.getLength());
}

function bigEndian(bytes) {
	return BigInt.fromArrayBuffer(bytes.slice().buffer);
}

function ecProvider() {
	if (!Modules.has(ECProvider))
		throw new Error("EC keys unsupported");
	return Modules.importNow(ECProvider);
}

/*
	RSA: PKCS#1 RSAPublicKey is SEQ {n, e}; RSAPrivateKey is SEQ {0, n, e, d, p, q, dp, dq, qi}
*/

function parsePKCS1(der, wantType) {		// few locals: this runs at the TLS handshake's stack peak
	const ber = new BER(der);
	expectSequence(ber);
	const result = {algorithm: "RSA", type: "public", n: ber.getInteger(), e: undefined};
	if (0n === result.n) {		// RSAPrivateKey begins with version 0
		result.type = "private";
		result.n = ber.getInteger();
	}
	if (wantType && (wantType !== result.type))
		throw new RangeError(`expected a ${wantType} key`);
	result.e = ber.getInteger();
	if ("private" === result.type) {
		result.d = ber.getInteger();
		result.p = ber.getInteger();
		result.q = ber.getInteger();
		result.dp = ber.getInteger();
		result.dq = ber.getInteger();
		result.qi = ber.getInteger();
	}
	return result;
}

function toPKCS1(c) {		// components -> PKCS#1 RSAPublicKey / RSAPrivateKey DER
	if ("private" !== c.type)
		return derNode(0x30, [derInteger(c.n), derInteger(c.e)]).buffer;
	return derNode(0x30, [derInteger(0n), derInteger(c.n), derInteger(c.e), derInteger(c.d), derInteger(c.p), derInteger(c.q), derInteger(c.dp), derInteger(c.dq), derInteger(c.qi)]).buffer;
}

/*
	EC: points are {x, y} BigInt on P-256/P-384; X25519 keeps the 32-byte u (and d) as bytes
*/

function modPow(base, exponent, m) {		// square and multiply; used only to decompress points
	let result = 1n;
	base %= m;
	for (let i = BigInt.bitLength(exponent) - 1; i >= 0; i--) {
		result = result * result % m;
		if ((exponent >> BigInt(i)) & 1n)
			result = result * base % m;
	}
	return result;
}

function pointOf(curve, bytes) {		// uncompressed or compressed point, or X25519 u -> components
	const size = curveBytes[curve];
	if ("X25519" === curve) {
		if (32 !== bytes.byteLength)
			throw new RangeError("invalid X25519 key");
		return {algorithm: "EC", type: "public", curve, u: bytes.slice()};
	}
	const prefix = bytes[0];
	if ((4 === prefix) && ((2 * size + 1) === bytes.byteLength))
		return {algorithm: "EC", type: "public", curve, x: bigEndian(bytes.subarray(1, 1 + size)), y: bigEndian(bytes.subarray(1 + size))};
	if (((2 === prefix) || (3 === prefix)) && ((size + 1) === bytes.byteLength)) {
		const {p, a, b} = curveField[curve];
		const x = bigEndian(bytes.subarray(1));
		if (x >= p)
			throw new RangeError("invalid EC point");
		const rhs = (x * x * x + a * x + b) % p;
		let y = modPow(rhs, (p + 1n) >> 2n, p);
		if (y * y % p !== rhs)
			throw new RangeError("invalid EC point");
		if ((y & 1n) !== BigInt(prefix & 1))
			y = p - y;
		return {algorithm: "EC", type: "public", curve, x, y};
	}
	throw new RangeError("invalid EC point");
}

function rawPoint(c) {		// components -> uncompressed point or X25519 u
	if ("X25519" === c.curve)
		return c.u;
	const size = curveBytes[c.curve];
	const result = new Uint8Array(1 + 2 * size);
	result[0] = 4;
	result.set(new Uint8Array(ArrayBuffer.fromBigInt(c.x, size)), 1);
	result.set(new Uint8Array(ArrayBuffer.fromBigInt(c.y, size)), 1 + size);
	return result;
}

function scalarOf(c) {		// private scalar as bytes: big-endian for P-256/P-384, the raw 32 bytes for X25519
	return ("X25519" === c.curve) ? c.d : new Uint8Array(ArrayBuffer.fromBigInt(c.d, curveBytes[c.curve]));
}

function withPrivate(curve, d, point) {		// private components; the public point is computed when the encoding omits it
	if (!point)
		point = pointOf(curve, ecProvider().publicFromPrivate(curve, d));
	point.type = "private";
	point.d = ("X25519" === curve) ? d : bigEndian(d);
	return point;
}

// RFC 5915 ECPrivateKey: SEQ {1, OCTET d, [0] curve OID (optional), [1] BIT STRING point (optional)}
function parseECPrivateKey(der, curve) {
	const ber = new BER(der);
	expectSequence(ber);
	if (1n !== ber.getInteger())
		throw new RangeError("unsupported ECPrivateKey version");
	const d = expectOctetString(ber);
	let point;
	if (0xa0 === ber.peek()) {
		ber.getTag();
		ber.getLength();
		const named = curveByOID[ber.getObjectIdentifier().toString()];
		if (!named || (curve && (curve !== named)))
			throw new RangeError("unsupported or mismatched curve");
		curve = named;
	}
	if (!curve)
		throw new RangeError("ECPrivateKey without a curve");
	if (0xa1 === ber.peek()) {
		ber.getTag();
		ber.getLength();
		point = pointOf(curve, ber.getBitString());
	}
	return withPrivate(curve, d, point);
}

function toECPrivateKey(c, withCurve) {
	const parts = [derInteger(1n), derNode(0x04, [scalarOf(c)])];
	if (withCurve)
		parts.push(derNode(0xa0, [curveOIDs[c.curve]]));
	parts.push(derNode(0xa1, [derNode(0x03, [0, rawPoint(c)])]));
	return derNode(0x30, parts);
}

// AlgorithmIdentifier -> {algorithm, curve}
function parseAlgorithm(ber) {
	expectSequence(ber);
	const oid = ber.getObjectIdentifier().toString();
	if ("1,2,840,113549,1,1,1" === oid) {
		ber.next();		// parameters (NULL)
		return {algorithm: "RSA"};
	}
	if ("1,2,840,10045,2,1" === oid) {
		const curve = curveByOID[ber.getObjectIdentifier().toString()];
		if (!curve)
			throw new RangeError("unsupported curve");
		return {algorithm: "EC", curve};
	}
	if ("1,3,101,110" === oid)
		return {algorithm: "EC", curve: "X25519"};
	throw new RangeError("unsupported key algorithm");
}

function algorithmIdentifier(c) {
	if ("RSA" === c.algorithm)
		return rsaAlgorithmIdentifier;
	if ("X25519" === c.curve)
		return derNode(0x30, [x25519OID]);
	return derNode(0x30, [ecPublicKeyOID, curveOIDs[c.curve]]);
}

function parseSPKI(data, algorithm) {
	const ber = new BER(bytesOf(data));
	expectSequence(ber);
	const id = parseAlgorithm(ber);
	if (id.algorithm !== algorithm)
		throw new RangeError(`not an ${algorithm} key`);
	const bits = ber.getBitString();
	return ("RSA" === algorithm) ? parsePKCS1(bits, "public") : pointOf(id.curve, bits);
}

function parsePKCS8(data, algorithm) {
	const ber = new BER(bytesOf(data));
	expectSequence(ber);
	if (0n !== ber.getInteger())
		throw new RangeError("unsupported PKCS#8 version");
	const id = parseAlgorithm(ber);
	if (id.algorithm !== algorithm)
		throw new RangeError(`not an ${algorithm} key`);
	const inner = expectOctetString(ber);
	if ("RSA" === algorithm)
		return parsePKCS1(inner, "private");
	if ("X25519" === id.curve)
		return withPrivate("X25519", expectOctetString(new BER(inner)).slice());
	return parseECPrivateKey(inner, id.curve);
}

function toPKCS8(c) {
	const inner = ("RSA" === c.algorithm) ? new Uint8Array(toPKCS1(c)) : ("X25519" === c.curve) ? derNode(0x04, [c.d]) : toECPrivateKey(c, false);
	return derNode(0x30, [derInteger(0n), algorithmIdentifier(c), derNode(0x04, [inner])]).buffer;
}

function toSPKI(c) {
	const bits = ("RSA" === c.algorithm) ? new Uint8Array(toPKCS1({type: "public", n: c.n, e: c.e})) : rawPoint(c);
	return derNode(0x30, [algorithmIdentifier(c), derNode(0x03, [0, bits])]).buffer;
}

/*
	JWK
*/

function fromBase64url(text, what) {
	if ("string" !== typeof text)
		throw new RangeError(`JWK ${what} missing`);
	return Uint8Array.fromBase64(text, {alphabet: "base64url"});
}

function base64urlBigInt(text, what) {
	return BigInt.fromArrayBuffer(fromBase64url(text, what).buffer);
}

function toBase64url(bytes) {
	return bytes.toBase64({alphabet: "base64url", omitPadding: true});
}

function toBase64urlBigInt(value, byteLength) {
	return toBase64url(new Uint8Array(ArrayBuffer.fromBigInt(value, byteLength)));
}

function fromJWK(jwk, algorithm) {
	if (!jwk || ("object" !== typeof jwk))
		throw new TypeError("JWK must be an object");
	if ("RSA" === algorithm) {
		if ("RSA" !== jwk.kty)
			throw new RangeError("JWK kty must be RSA");
		const n = base64urlBigInt(jwk.n, "n"), e = base64urlBigInt(jwk.e, "e");
		if (undefined === jwk.d)
			return {algorithm, type: "public", n, e};
		return {algorithm, type: "private", n, e, d: base64urlBigInt(jwk.d, "d"), p: base64urlBigInt(jwk.p, "p"), q: base64urlBigInt(jwk.q, "q"),
			dp: base64urlBigInt(jwk.dp, "dp"), dq: base64urlBigInt(jwk.dq, "dq"), qi: base64urlBigInt(jwk.qi, "qi")};
	}
	const curve = jwk.crv;
	if (!Curves.includes(curve) || (jwk.kty !== (("X25519" === curve) ? "OKP" : "EC")))
		throw new RangeError("unsupported JWK curve");
	const size = curveBytes[curve];
	let point;
	if ("X25519" === curve)
		point = pointOf(curve, fromBase64url(jwk.x, "x"));
	else {
		const x = fromBase64url(jwk.x, "x"), y = fromBase64url(jwk.y, "y");
		if ((size !== x.byteLength) || (size !== y.byteLength))
			throw new RangeError("JWK coordinate length");
		point = {algorithm, type: "public", curve, x: bigEndian(x), y: bigEndian(y)};
	}
	if (undefined === jwk.d)
		return point;
	const d = fromBase64url(jwk.d, "d");
	if (size !== d.byteLength)
		throw new RangeError("JWK d length");
	// the public key must be the private scalar's
	const expected = ecProvider().publicFromPrivate(curve, d);
	const given = rawPoint(point);
	if (expected.byteLength !== given.byteLength)
		throw new RangeError("JWK key pair mismatch");
	for (let i = 0; i < given.byteLength; i++)
		if (expected[i] !== given[i])
			throw new RangeError("JWK key pair mismatch");
	return withPrivate(curve, d, point);
}

function toJWK(c, size) {
	if ("RSA" === c.algorithm) {
		const bytes = (size + 7) >> 3, half = (bytes + 1) >> 1;
		const jwk = {kty: "RSA", n: toBase64urlBigInt(c.n, bytes), e: toBase64urlBigInt(c.e)};
		if ("private" === c.type) {
			jwk.d = toBase64urlBigInt(c.d, bytes);
			jwk.p = toBase64urlBigInt(c.p, half);
			jwk.q = toBase64urlBigInt(c.q, half);
			jwk.dp = toBase64urlBigInt(c.dp, half);
			jwk.dq = toBase64urlBigInt(c.dq, half);
			jwk.qi = toBase64urlBigInt(c.qi, half);
		}
		return jwk;
	}
	const bytes = curveBytes[c.curve];
	const jwk = ("X25519" === c.curve) ? {kty: "OKP", crv: c.curve, x: toBase64url(c.u)} : {kty: "EC", crv: c.curve, x: toBase64urlBigInt(c.x, bytes), y: toBase64urlBigInt(c.y, bytes)};
	if ("private" === c.type)
		jwk.d = toBase64url(scalarOf(c));
	return jwk;
}

/*
	what the operation classes see: the parsed components and the encodings each backend imports from, built on demand
*/

class Internal {
	constructor(components) {
		this.components = components;
		this.algorithm = components.algorithm;
		this.type = components.type;
		this.size = ("RSA" === components.algorithm) ? BigInt.bitLength(components.n) : ("X25519" === components.curve) ? 255 : curveBytes[components.curve] << 3;
		this.cache = {};
	}
	get curve() {
		return this.components.curve;
	}
	get der() {		// RSA: PKCS#1
		return toPKCS1(this.components);
	}
	get point() {		// EC: the public point as bytes
		return rawPoint(this.components);
	}
	get scalar() {		// EC: the private scalar as bytes
		return scalarOf(this.components);
	}
}

/*
	Key
*/

class Key {
	#internal;
	#extractable;
	#algorithm;
	#curve;

	constructor(options) {
		const algorithm = options.algorithm;
		if (("RSA" !== algorithm) && ("EC" !== algorithm))
			throw new Error("unsupported algorithm");
		if (("RSA" === algorithm) && (undefined !== options.namedCurve))		// an EC property, not accepted by RSA
			throw new RangeError("namedCurve not supported by algorithm");
		let components;
		if (("EC" === algorithm) && (undefined === options.data)) {		// generate
			const curve = options.namedCurve;
			if (!Curves.includes(curve))
				throw new RangeError("unsupported curve");
			components = ecProvider().generate(curve);
			components.algorithm = "EC";
		}
		else {
			if (!Formats.includes(options.format))
				throw new RangeError("unsupported format");
			try {
				switch (options.format) {
					case "pkcs1":
						if ("RSA" !== algorithm) throw new RangeError("pkcs1 is an RSA format");
						components = parsePKCS1(bytesOf(options.data));		// parsed in place; nothing of the caller's buffer is retained
						break;
					case "spki": components = parseSPKI(options.data, algorithm); break;
					case "pkcs8": components = parsePKCS8(options.data, algorithm); break;
					case "jwk": components = fromJWK(options.data, algorithm); break;
					case "sec1":
						if ("EC" !== algorithm) throw new RangeError("sec1 is an EC format");
						components = parseECPrivateKey(bytesOf(options.data), options.namedCurve);
						break;
					case "raw":
						if ("EC" !== algorithm) throw new RangeError("raw is an EC format");
						if (!Curves.includes(options.namedCurve)) throw new RangeError("unsupported curve");
						components = pointOf(options.namedCurve, bytesOf(options.data));
						break;
				}
			}
			catch (e) {
				if ((e instanceof TypeError) || (e instanceof RangeError))
					throw e;
				throw new RangeError(`invalid key: ${e.message}`);		// BER reports structure errors as Error
			}
			if (undefined !== options.namedCurve && ("EC" === algorithm) && (options.namedCurve !== components.curve))
				throw new RangeError("namedCurve does not match the key");
		}
		if (("EC" === algorithm) && !ecProvider().supports(components.curve))
			throw new RangeError(`curve ${components.curve} not supported by this build`);
		this.#internal = new Internal(components);
		this.#extractable = Boolean(options.extractable);
		this.#algorithm = algorithm;
		this.#curve = components.curve;
	}
	static supports(options) {		// without constructing: the algorithm, and for EC the curve, this build includes
		if (("object" !== typeof options) || (null === options))
			throw new TypeError("options required");
		if ("RSA" === options.algorithm)
			return undefined === options.namedCurve;
		if ("EC" !== options.algorithm)
			return false;
		if (!Modules.has(ECProvider))
			return false;
		if (undefined === options.namedCurve)
			return true;
		return Curves.includes(options.namedCurve) && ecProvider().supports(options.namedCurve);
	}
	close() {
		const i = this.#internal;
		if (i) {
			i.components = undefined;
			i.cache = {};
			this.#internal = undefined;
		}
	}
	get algorithm() { return this.#algorithm; }
	get type() { return this.#state.type; }
	get keySize() { return this.#state.size; }
	get namedCurve() { return this.#curve; }
	get extractable() { return this.#extractable; }

	// Only an extractable key is extracted, in any format: a public key in its public formats, a private key in its
	// private formats and in the public formats of its public key.
	extract(format) {
		const {components, type, size, algorithm} = this.#state;
		if (!Formats.includes(format))
			throw new RangeError("unsupported format");
		if (("RSA" === algorithm) && (("sec1" === format) || ("raw" === format)))
			throw new RangeError("unsupported format for RSA");
		if (("EC" === algorithm) && ("pkcs1" === format))
			throw new RangeError("unsupported format for EC");
		if (!this.#extractable)
			throw new Error("key is not extractable");
		if ("private" === type) {
			switch (format) {
				case "pkcs1": return toPKCS1(components);
				case "pkcs8": return toPKCS8(components);
				case "sec1":
					if ("X25519" === components.curve) throw new RangeError("X25519 has no sec1 form");
					return toECPrivateKey(components, true).buffer;
				case "jwk": return toJWK(components, size);
			}
			// public forms of a private key
		}
		switch (format) {
			case "pkcs1": return toPKCS1({type: "public", n: components.n, e: components.e});
			case "spki": return toSPKI(components);
			case "jwk": return toJWK(("private" === type) ? Object.assign({}, components, {type: "public"}) : components, size);
			case "raw": return rawPoint(components).slice().buffer;
			default: throw new RangeError("a public key has no private encoding");
		}
	}

	get #state() {
		const i = this.#internal;
		if (!i)
			throw new Error("closed");
		return i;
	}

	static {
		internal = key => {
			const i = key.#internal;
			if (!i)
				throw new Error("closed");
			return i;
		};
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export {parsePKCS1, curveBytes};
export default Key;

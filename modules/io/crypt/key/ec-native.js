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
	ECMA-419 Key — EC provider over natives (PSA, CryptoKit): key generation and the public point of a private scalar
	Scalars and points are bytes: big-endian scalars and uncompressed points on P-256/P-384, little-endian on X25519
*/

const curveBytes = {"P-256": 32, "P-384": 48, "X25519": 32};
Object.freeze(curveBytes);

function bigEndian(bytes) {
	return BigInt.fromArrayBuffer(bytes.slice().buffer);
}

function components(curve, d, point) {
	if ("X25519" === curve)
		return {type: "private", curve, d, u: point};
	const size = curveBytes[curve];
	return {type: "private", curve, d: bigEndian(d), x: bigEndian(point.subarray(1, 1 + size)), y: bigEndian(point.subarray(1 + size))};
}

function generateNative(curve) { return native("xs_ec_generate").call(this, curve); }
function publicNative(curve, d) { return native("xs_ec_public").call(this, curve, d); }

export function generate(curve) {
	if (!(curve in curveBytes))
		throw new RangeError("unsupported curve");
	const pair = generateNative(curve);		// {scalar, point} as ArrayBuffers
	return components(curve, new Uint8Array(pair.scalar), new Uint8Array(pair.point));
}

export function publicFromPrivate(curve, d) {
	return new Uint8Array(publicNative(curve, d));
}

function supportsNative(name) { return native("xs_crypt_ec_supports").call(this, name); }
export function supports(name) {		// whether this build includes the curve (manifest defines crypt.curve.*)
	return supportsNative(name);
}

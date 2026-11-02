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
	RSA for the Cipher class — KCL backend: RSA-OAEP and RSAES-PKCS1-v1_5 over the key family's RSA primitives.
	Loaded by embedded:crypt/cipher on demand (Modules.importNow); not an application API.
*/

import Digest from "embedded:crypt/digest";
import Key, {internal} from "embedded:crypt/key";
import Cipher from "embedded:crypt/cipher";
import * as RSA from "embedded:crypt/key/rsa";

const Hashes = ["MD5", "SHA-1", "SHA-224", "SHA-256", "SHA-384", "SHA-512"];
Object.freeze(Hashes);

function bytes(data, what) {
	if (data instanceof ArrayBuffer)
		return new Uint8Array(data);
	if (ArrayBuffer.isView(data))
		return new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
	throw new TypeError(`${what} must be a byte buffer`);
}

function processRSA(data, options) {		// encrypt or decrypt, the direction checked by the caller
	const key = this.live;
	if (this.done)
		throw new Error("operation complete; reset required");
	if (options?.more)
		throw new RangeError("RSA does not stream");
	data = (undefined === data) ? new Uint8Array(0) : bytes(data, "data");
	const k = RSA.byteLength(key);
	let result;
	if ("encrypt" === this.direction) {
		const em = ("RSA-OAEP" === this.algorithm) ? RSA.emeOaepEncode(this.hash, this.label, data, k) : RSA.emePkcs1v15Encode(data, k);
		result = RSA.I2OSP(RSA.publicOp(key, RSA.OS2IP(em)), k);
	}
	else {
		if (data.byteLength !== k)
			throw new RangeError("ciphertext length must equal the key size");
		const em = RSA.I2OSP(RSA.privateOp(key, RSA.OS2IP(data)), k);
		result = ("RSA-OAEP" === this.algorithm) ? RSA.emeOaepDecode(this.hash, this.label, em) : RSA.emePkcs1v15Decode(em);
		if (!result)
			throw new Error("decryption failed");
	}
	this.done = true;
	const output = options?.output;
	if (undefined === output)
		return result.buffer;
	const out = bytes(output, "output");
	if (out.byteLength < result.byteLength)
		throw new RangeError("output too small");
	out.set(result);
	return result.byteLength;
}

class RSACipher {
	constructor(options) {
		const algorithm = options.algorithm;
		if (("RSA-OAEP" !== algorithm) && ("RSAES-PKCS1-v1_5" !== algorithm))
			throw new Error("unsupported algorithm");
		const key = options.key;
		if (!(key instanceof Key) || ("RSA" !== key.algorithm))
			throw new TypeError("key must be an RSA Key");
		internal(key);
		const direction = options.direction;
		if (("encrypt" !== direction) && ("decrypt" !== direction))
			throw new RangeError("invalid direction");
		if (("decrypt" === direction) && !RSA.isPrivate(key))
			throw new Error("private key required to decrypt");
		for (const name of ["iv", "additionalData", "tagLength", "padding"])
			if (undefined !== options[name]) throw new RangeError(`${name} not supported by algorithm`);
		this.key = key;
		this.algorithm = algorithm;
		this.direction = direction;
		if ("RSA-OAEP" === algorithm) {
			if (!Hashes.includes(options.hash) || !Digest.supports({algorithm: options.hash}))		// known here, and in this build
				throw new RangeError("unsupported hash");
			this.hash = options.hash;
			this.label = (undefined === options.label) ? new Uint8Array(0) : bytes(options.label, "label").slice();
		}
		else if ((undefined !== options.hash) || (undefined !== options.label))
			throw new RangeError("hash and label not supported by algorithm");
		this.done = false;
	}
	close() {
		this.key = undefined;
	}
	reset() {
		this.live;
		this.done = false;
	}
	get live() {
		if (!this.key)
			throw new Error("closed");
		return this.key;
	}
	get blockSize() { return RSA.byteLength(this.live); }
	get keySize() { return internal(this.live).size; }		// bits
	get tagLength() { return 0; }

	encrypt(data, options) {
		if ("encrypt" !== this.direction)
			throw new Error("instance does not encrypt");
		return processRSA.call(this, data, options);
	}
	decrypt(data, options) {
		if ("decrypt" !== this.direction)
			throw new Error("instance does not decrypt");
		return processRSA.call(this, data, options);
	}
}

Object.setPrototypeOf(RSACipher.prototype, Cipher.prototype);		// instanceof Cipher
RSACipher.prototype[Symbol.dispose] = RSACipher.prototype.close;

export function create(options) {
	return new RSACipher(options);
}

export function supports(options) {
	if ("RSA-OAEP" === options.algorithm)
		return (undefined === options.hash) || (Hashes.includes(options.hash) && Digest.supports({algorithm: options.hash}));
	return ("RSAES-PKCS1-v1_5" === options.algorithm) && (undefined === options.hash) && (undefined === options.label);
}

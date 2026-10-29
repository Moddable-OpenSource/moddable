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
	RSA for the Cipher class — PSA Crypto: RSA-OAEP and RSAES-PKCS1-v1_5.
	Loaded by embedded:crypt/cipher on demand (Modules.importNow); not an application API.
*/

import Digest from "embedded:crypt/digest";
import Key, {internal} from "embedded:crypt/key";
import Cipher from "embedded:crypt/cipher";

const Hashes = ["MD5", "SHA-1", "SHA-224", "SHA-256", "SHA-384", "SHA-512"];
Object.freeze(Hashes);

function setup(der, type, algorithm, direction, hash, label) { return native("xs_rsacipher_setup").call(this, der, type, algorithm, direction, hash, label); }
function processNative(data, output) { return native("xs_rsacipher_process").call(this, data, output); }
function closeNative() { return native("xs_rsacipher_close").call(this); }

function processRSA(data, options) {		// encrypt or decrypt, the direction checked by the caller
	if (options?.more)
		throw new RangeError("RSA does not stream");
	const key = this.key;		// PSA holds its own copy of the key; fail like the script implementation once the Key is closed
	if (key)
		internal(key);
	return processNative.call(this, data, options?.output);
}

class RSACipher extends Native("xs_rsacipher_destructor") {
	constructor(options) {
		const algorithm = options.algorithm;
		if (("RSA-OAEP" !== algorithm) && ("RSAES-PKCS1-v1_5" !== algorithm))
			throw new Error("unsupported algorithm");
		const key = options.key;
		if (!(key instanceof Key) || ("RSA" !== key.algorithm))
			throw new TypeError("key must be an RSA Key");
		const i = internal(key);
		const direction = options.direction;
		if (("encrypt" !== direction) && ("decrypt" !== direction))
			throw new RangeError("invalid direction");
		if (("decrypt" === direction) && ("private" !== i.type))
			throw new Error("private key required to decrypt");
		for (const name of ["iv", "additionalData", "tagLength", "padding"])
			if (undefined !== options[name]) throw new RangeError(`${name} not supported by algorithm`);
		let hash, label;
		if ("RSA-OAEP" === algorithm) {
			if (!Hashes.includes(options.hash) || !Digest.supports({algorithm: options.hash}))		// known here, and in this build
				throw new RangeError("unsupported hash");
			hash = options.hash;
			label = options.label;
		}
		else if ((undefined !== options.hash) || (undefined !== options.label))
			throw new RangeError("hash and label not supported by algorithm");
		super();
		setup.call(this, i.der, i.type, algorithm, direction, hash, label);
		this.key = key;
		this.direction = direction;
	}
	close() {
		this.key = undefined;
		return closeNative.call(this);
	}
	reset() { return native("xs_rsacipher_reset").call(this); }
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
	get blockSize() { return native("xs_rsacipher_get_blockSize").call(this); }
	get keySize() { return native("xs_rsacipher_get_keySize").call(this); }
	get tagLength() { return 0; }
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

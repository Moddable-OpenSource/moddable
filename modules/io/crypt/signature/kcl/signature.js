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
	ECMA-419 Crypto Signature — KCL backend
	RSASSA-PKCS1-v1_5, RSA-PSS and ECDSA over a digest
*/

import Digest from "embedded:crypt/digest";
import Key, {internal} from "embedded:crypt/key";
import * as RSA from "embedded:crypt/key/rsa";
import * as EC from "embedded:crypt/key/ec";

const Algorithms = ["RSASSA-PKCS1-v1_5", "RSA-PSS", "ECDSA"];
const Hashes = ["MD5", "SHA-1", "SHA-224", "SHA-256", "SHA-384", "SHA-512"];
Object.freeze({Algorithms, Hashes}, true);

class Signature {
	#key;
	#algorithm;
	#hash;
	#saltLength;

	constructor(options) {
		const algorithm = options.algorithm;
		if (!Algorithms.includes(algorithm))
			throw new Error("unsupported algorithm");
		const key = options.key;
		if (!(key instanceof Key))
			throw new TypeError("key must be a Key");
		const i = internal(key);		// throws if closed
		this.#key = key;
		this.#algorithm = algorithm;
		if ("ECDSA" === algorithm) {		// signs a digest as given, so no hash is needed
			if (("EC" !== key.algorithm) || ("X25519" === i.curve))
				throw new TypeError("key must be a P-256 or P-384 Key");
			if (undefined !== options.hash)		// hash selects deterministic ECDSA (RFC 6979), which this implementation does not provide
				throw new RangeError("deterministic ECDSA not supported");
			if (undefined !== options.saltLength)
				throw new RangeError("saltLength not supported by algorithm");
			return;
		}
		if ("RSA" !== key.algorithm)
			throw new TypeError("key must be an RSA Key");
		const hash = options.hash;
		if (!Hashes.includes(hash) || !Digest.supports({algorithm: hash}))		// known here, and in this build
			throw new RangeError("unsupported hash");
		if (("RSA-PSS" !== algorithm) && (undefined !== options.saltLength))
			throw new RangeError("saltLength not supported by algorithm");
		this.#hash = hash;
		if ("RSA-PSS" === algorithm) {
			const saltLength = Number(options.saltLength ?? RSA.hashSize(hash));		// coerced, as 419 APIs do
			if (saltLength < 0)
				throw new RangeError("invalid saltLength");
			this.#saltLength = saltLength;
		}
	}
	static supports(options) {
		if (("object" !== typeof options) || (null === options))
			throw new TypeError("options required");
		if (!Algorithms.includes(options.algorithm))
			return false;
		if ("ECDSA" === options.algorithm)
			return (undefined === options.hash) && Key.supports({algorithm: "EC"});
		if ((undefined !== options.hash) && !(Hashes.includes(options.hash) && Digest.supports({algorithm: options.hash})))
			return false;
		if (undefined === options.saltLength)
			return true;
		return ("RSA-PSS" === options.algorithm) && (Number(options.saltLength) >= 0);
	}
	close() {
		this.#key = undefined;
	}
	get algorithm() { return this.#algorithm; }
	get hash() { return this.#hash; }
	get outputSize() {
		const i = internal(this.#live);
		const bytes = (i.size + 7) >> 3;
		return ("ECDSA" === this.#algorithm) ? 2 * bytes : bytes;
	}

	sign(digest) {
		const key = this.#live;
		const i = internal(key);
		if ("private" !== i.type)
			throw new Error("private key required");
		digest = bytes(digest, "digest");
		if ("ECDSA" === this.#algorithm) {
			return EC.sign(i.components, digest).buffer;
		}
		const k = RSA.byteLength(key);
		let em;
		if ("RSA-PSS" === this.#algorithm)
			em = RSA.emsaPssEncode(this.#hash, digest, this.#saltLength, internal(key).size - 1);
		else
			em = RSA.emsaPkcs1v15(this.#hash, digest, k);
		return RSA.I2OSP(RSA.privateOp(key, RSA.OS2IP(em)), k).buffer;
	}

	verify(digest, signature) {
		const key = this.#live;
		digest = bytes(digest, "digest");
		signature = bytes(signature, "signature");
		if ("ECDSA" === this.#algorithm) {
			const i = internal(key);
			return EC.verify(i.components, digest, signature);
		}
		const k = RSA.byteLength(key);
		if (signature.byteLength !== k)
			return false;
		const s = RSA.OS2IP(signature);
		let em;
		try {
			em = RSA.I2OSP(RSA.publicOp(key, s), k);
		}
		catch {
			return false;		// signature not below the modulus
		}
		if ("RSA-PSS" === this.#algorithm) {
			const emBits = internal(key).size - 1;
			return RSA.emsaPssVerify(this.#hash, digest, em.subarray(k - ((emBits + 7) >> 3)), this.#saltLength, emBits);
		}
		return RSA.emsaPkcs1v15Verify(this.#hash, digest, em);
	}

	get #live() {
		const key = this.#key;
		if (!key)
			throw new Error("closed");
		return key;
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

function bytes(data, what) {
	if (data instanceof ArrayBuffer)
		return new Uint8Array(data);
	if (ArrayBuffer.isView(data))
		return new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
	throw new TypeError(`${what} must be a byte buffer`);
}

export default Signature;

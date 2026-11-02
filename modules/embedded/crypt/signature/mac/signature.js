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

	SecKey signs and verifies RSA-PSS with a salt of the hash length only, so any other saltLength is a RangeError at
	construction; MD5 is not one of its digest algorithms.
*/

import Digest from "embedded:crypt/digest";
import Key, {internal} from "embedded:crypt/key";

const Algorithms = ["RSASSA-PKCS1-v1_5", "RSA-PSS", "ECDSA"];
const Hashes = ["SHA-1", "SHA-224", "SHA-256", "SHA-384", "SHA-512"];
const HashSizes = [20, 28, 32, 48, 64];		// parallel to Hashes
Object.freeze({Algorithms, Hashes, HashSizes}, true);

function setup(material, type, algorithm, hash, curve, size) { return native("xs_signature_setup").call(this, material, type, algorithm, hash, undefined, curve, size); }
function closeNative() { return native("xs_signature_close").call(this); }
function signNative(digest) { return native("xs_signature_sign").call(this, digest); }
function verifyNative(digest, signature) { return native("xs_signature_verify").call(this, digest, signature); }

class Signature extends Native("xs_signature_destructor") {
	#key;
	#algorithm;
	#hash;

	constructor(options) {
		const algorithm = options.algorithm;
		if (!Algorithms.includes(algorithm))
			throw new Error("unsupported algorithm");
		const key = options.key;
		if (!(key instanceof Key))
			throw new TypeError("key must be a Key");
		const i = internal(key);
		let hash;
		if ("ECDSA" === algorithm) {		// signs a digest as given, so no hash is needed
			if (("EC" !== key.algorithm) || ("X25519" === i.curve))
				throw new TypeError("key must be a P-256 or P-384 Key");
			if (undefined !== options.hash)		// hash selects deterministic ECDSA (RFC 6979), which this implementation does not provide
				throw new RangeError("deterministic ECDSA not supported");
			if (undefined !== options.saltLength)
				throw new RangeError("saltLength not supported by algorithm");
		}
		else {
			if ("RSA" !== key.algorithm)
				throw new TypeError("key must be an RSA Key");
			hash = options.hash;
			if (!Hashes.includes(hash) || !Digest.supports({algorithm: hash}))		// known here, and in this build
				throw new RangeError("unsupported hash");
			if (("RSA-PSS" !== algorithm) && (undefined !== options.saltLength))
				throw new RangeError("saltLength not supported by algorithm");
			if (("RSA-PSS" === algorithm) && (undefined !== options.saltLength) && (Number(options.saltLength) !== HashSizes[Hashes.indexOf(hash)]))		// coerced, as 419 APIs do
				throw new RangeError("saltLength must be the hash's output size on this platform");
		}
		super();
		if ("ECDSA" === algorithm)
			setup.call(this, ("private" === i.type) ? i.scalar : i.point, i.type, algorithm, undefined, i.curve);
		else
			setup.call(this, i.der, i.type, algorithm, hash, undefined, i.size);
		this.#key = key;
		this.#algorithm = algorithm;
		this.#hash = hash;
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
		return ("RSA-PSS" === options.algorithm) && (Number(options.saltLength) === HashSizes[Hashes.indexOf(options.hash)]);		// a salt of the hash length only
	}
	close() {
		this.#key = undefined;
		return closeNative.call(this);
	}
	sign(digest) {
		this.#checkKey();
		return signNative.call(this, digest);
	}
	verify(digest, signature) {
		this.#checkKey();
		return verifyNative.call(this, digest, signature);
	}
	#checkKey() {		// the native holds its own copy of the key; fail like the script implementation once the Key is closed
		const key = this.#key;
		if (key)
			internal(key);
	}
	get algorithm() { return this.#algorithm; }
	get hash() { return this.#hash; }
	get outputSize() { return native("xs_signature_get_size").call(this); }

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default Signature;

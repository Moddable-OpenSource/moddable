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
	ECMA-419 Crypto Key Agreement — KCL backend
	ECDH on P-256, P-384 and X25519: the shared secret from our private key and a peer's public key
*/

import Key, {internal} from "embedded:crypt/key";
import * as EC from "embedded:crypt/key/ec";


class KeyAgreement {
	#key;

	constructor(options) {
		if ("ECDH" !== options.algorithm)
			throw new Error("unsupported algorithm");
		const key = options.key;
		if (!(key instanceof Key) || ("EC" !== key.algorithm))
			throw new TypeError("key must be an EC Key");
		if ("private" !== internal(key).type)
			throw new Error("private key required");
		this.#key = key;
	}
	close() {
		this.#key = undefined;
	}
	derive(publicKey) {
		const key = this.#key;
		if (!key)
			throw new Error("closed");
		const i = internal(key);
		if (!(publicKey instanceof Key) || ("EC" !== publicKey.algorithm))
			throw new TypeError("publicKey must be an EC Key");
		const peer = internal(publicKey);
		if (peer.curve !== i.curve)
			throw new RangeError("curve mismatch");
		return EC.agree(i.components, peer.components).buffer;
	}
	get algorithm() { return "ECDH"; }
	static supports(options) {
		if (("object" !== typeof options) || (null === options))
			throw new TypeError("options required");
		if ("ECDH" !== options.algorithm)
			return false;
		return Key.supports({algorithm: "EC", namedCurve: options.namedCurve});
	}
	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default KeyAgreement;

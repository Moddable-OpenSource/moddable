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

import Modules from "modules";

// RSA-OAEP and RSAES-PKCS1-v1_5 come from the key family's optional module, so builds without RSA carry none of it.
// The constructor returns that module's instance; it carries Cipher.prototype in its chain.
const RSAModule = "embedded:crypt/cipher/rsa";

function construct(options) { return native("xs_cipher_constructor").call(this, options); }

function supportsNative(options) { return native("xs_cipher_supports").call(this, options); }

class Cipher extends Native("xs_cipher_destructor") {
	constructor(options) {
		super();
		if (("string" === typeof options?.algorithm) && options.algorithm.startsWith("RSA")) {
			if (!Modules.has(RSAModule))
				throw new Error("unsupported algorithm");
			return Modules.importNow(RSAModule).create(options);
		}
		construct.call(this, options);
	}
	static supports(options) {		// whether this build includes the algorithm, without constructing
		if (("object" !== typeof options) || (null === options))
			throw new TypeError("options required");
		if (("string" === typeof options.algorithm) && options.algorithm.startsWith("RSA"))
			return Modules.has(RSAModule) && Modules.importNow(RSAModule).supports(options);
		return (undefined === options.hash) && supportsNative(options);		// hash is RSA-OAEP's
	}
	close() { return native("xs_cipher_close").call(this); }

	encrypt(data, options) { return native("xs_cipher_encrypt").call(this, data, options); }
	decrypt(data, options) { return native("xs_cipher_decrypt").call(this, data, options); }
	reset(options) { return native("xs_cipher_reset").call(this, options); }

	get blockSize() { return native("xs_cipher_get_blockSize").call(this); }
	get keySize() { return native("xs_cipher_get_keySize").call(this); }
	get tagLength() { return native("xs_cipher_get_tagLength").call(this); }

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default Cipher;

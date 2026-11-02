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
	ECMA-419 Crypto KeyDerivation — HKDF (and its Extract and Expand steps) and PBKDF2 over HMAC
	read(count) returns up to the next count bytes of the derived output; read(buffer) fills as much of the buffer
	as it can. Successive reads continue the output stream; at the algorithm's output limit, read returns undefined.
*/

function supportsNative(options) { return native("xs_keyderivation_supports").call(this, options); }

class KeyDerivation extends Native("xs_keyderivation_destructor") {
	constructor(options) { super(); native("xs_keyderivation_constructor").call(this, options); }
	static supports(options) {		// whether this build includes the algorithm and hash, without constructing
		if (("object" !== typeof options) || (null === options))
			throw new TypeError("options required");
		return supportsNative(options);
	}
	close() { return native("xs_keyderivation_close").call(this); }

	read(count) { return native("xs_keyderivation_read").call(this, count); }

	get algorithm() { return native("xs_keyderivation_get_algorithm").call(this); }
	get hash() { return native("xs_keyderivation_get_hash").call(this); }

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default KeyDerivation;

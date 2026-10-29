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
	ECMA-419 Crypto MAC — HMAC over the platform's hash primitives
	write() feeds the message; read() yields the tag without disturbing the state (as Digest.read()); reset() restarts the
	message with the same key; verify(tag) compares in constant time
*/

function supportsNative(options) { return native("xs_mac_supports").call(this, options); }

class MAC extends Native("xs_mac_destructor") {
	constructor(options) { super(); native("xs_mac_constructor").call(this, options); }
	static supports(options) {		// whether this build includes the algorithm and hash, without constructing
		if (("object" !== typeof options) || (null === options))
			throw new TypeError("options required");
		return supportsNative(options);
	}
	close() { return native("xs_mac_close").call(this); }

	write(data) { return native("xs_mac_write").call(this, data); }
	read(buffer) { return native("xs_mac_read").call(this, buffer); }
	reset() { return native("xs_mac_reset").call(this); }
	verify(tag) { return native("xs_mac_verify").call(this, tag); }

	get algorithm() { return "HMAC"; }
	get hash() { return native("xs_mac_get_hash").call(this); }
	get blockSize() { return native("xs_mac_get_blockSize").call(this); }
	get outputSize() { return native("xs_mac_get_outputSize").call(this); }

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default MAC;

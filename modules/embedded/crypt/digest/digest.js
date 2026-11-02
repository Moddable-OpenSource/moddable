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

function supportsNative(options) { return native("xs_digest_supports").call(this, options); }

class Digest extends Native("xs_digest_destructor") {
	constructor(options) { super(); native("xs_digest_constructor").call(this, options); }
	close() { return native("xs_digest_close").call(this); }

	write(data) { return native("xs_digest_write").call(this, data); }
	read(buffer) { return native("xs_digest_read").call(this, buffer); }
	reset() { return native("xs_digest_reset").call(this); }

	static supports(options) {		// whether this build includes the algorithm, without constructing
		if (("object" !== typeof options) || (null === options))
			throw new TypeError("options required");
		return supportsNative(options);
	}

	get blockSize() { return native("xs_digest_get_blockSize").call(this); }
	get outputSize() { return native("xs_digest_get_outputSize").call(this); }

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default Digest;

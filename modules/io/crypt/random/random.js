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
	ECMA-419 Crypto Random — a read-only IO class delivering bytes from the platform's cryptographic random source
*/

function fill(buffer) { return native("xs_random_fill").call(this, buffer); }

class Random {
	#closed = false;

	constructor(options) {
		if (!options || ("object" !== typeof options))
			throw new TypeError("options required");
	}
	close() {
		this.#closed = true;
	}
	read(count) {
		if (this.#closed)
			throw new Error("closed");
		if (count && ("object" === typeof count))
			return fill(count);		// byte buffer: filled in place, returns the byte count; another object throws
		const result = new ArrayBuffer(count);		// coerces, and rejects a negative or impossible count
		fill(result);
		return result;
	}
	get format() {
		return "buffer";
	}
	set format(value) {
		if ("buffer" !== value)
			throw new RangeError("unsupported format");
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default Random;

/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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

import REPLCore from "replcore";

const baud = 115200;

class REPL extends REPLCore {
	#read = new ArrayBuffer;
	#write = new ArrayBuffer;
	#writable = 0;
	#serial;
	#trace;

	constructor() {
		const trace = globalThis.trace;
		const isDebug = REPL.isDebug();

		super();

		if (isDebug) {
			trace("** Warning: debugger connection active. REPL unavailable over serial. **\n");
			trace(`** Connect with a serial terminal / monitor at ${baud} baud 8N1. **\n`);
		}

		this.#trace = trace;
		if (!isDebug) {
			this.#serial = new device.io.Serial({
				...device.Serial.default,
				baud,
				format: "buffer",
				target: this,
				onWritable: function(count) {
					const target = this.target;
					target.#writable = count;
					const write = target.#write;
					if (!write)
						return;

					this.write(write.slice(0, count));
					target.#writable -= Math.min(count, write.byteLength);

					if (count >= write.byteLength)
						target.#write = new ArrayBuffer;
					else
						target.#write = write.slice(count);
				},
				onReadable: function() {
					this.target.#read = this.target.#read.concat(this.read());
					this.target.poll();
				}
			});
		}

		this.ready();
	}
	receive() {
		if (!this.#read.byteLength)
			return;
		const result = (new Uint8Array(this.#read))[0];
		this.#read = this.#read.slice(1);
		return result;
	}
	write(...parts) {
		if (!this.#serial) {
			parts.forEach(part => this.#trace(part));
			return;
		}

		parts = parts.map(buffer => ArrayBuffer.fromString(buffer));

		let i = 0;
		if (!this.#write || !this.#write.byteLength) {
			for (; i < parts.length; i++) {
				const part = parts[i];
				if (part.byteLength > this.#writable)
					break;

				this.#serial.write(part);
				this.#writable -= part.byteLength;
			}
		}

		parts = parts.slice(i);
		if (!parts.length)
			return;
		this.#write = this.#write.concat.apply(this.#write, parts);
	}
	static isDebug() @ "xs_repl_isDebug"
}
Object.freeze(REPL.prototype);

export default REPL;

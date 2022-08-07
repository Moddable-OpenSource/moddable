/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

class SSLStream {
	#write = 0;
	#read = 0;
	#bytes;

	constructor(buffer) {
		if (buffer) {
			if (!(buffer instanceof Uint8Array))
				buffer = new Uint8Array(buffer);
			this.#bytes = buffer;
			this.#write = this.#bytes.length;
		}
		else {
			this.#bytes = new Uint8Array(new ArrayBuffer(32, {maxByteLength: 0x10000000}));
			this.#bytes.i = true;
		}
	}
	morebuf(n) {
		this.#bytes.buffer.resize((this.#write + n + 31) & ~31);
	}
	writeChar(c) {
		if (this.#write >= this.#bytes.length)
			this.morebuf(1);
		this.#bytes[this.#write++] = c;
	}
	writeChars(v, n) {
		if (this.#write + n > this.#bytes.length)
			this.morebuf(n);
		while (--n >= 0)
			this.#bytes[this.#write++] = (v >>> (n * 8)) & 0xff;
	}
	writeChunk(a) {
		const n = a.byteLength;
		if (n <= 0)
			return;
		if (this.#write + n > this.#bytes.length)
			this.morebuf(n);
		this.#bytes.set((a instanceof Uint8Array) ? a : (new Uint8Array(a)), this.#write);
		this.#write += n;
	}
	writeString(s) {
		s = ArrayBuffer.fromString(s);
		const n = s.byteLength;
		if (n <= 0)
			return;
		if (this.#write + n > this.#bytes.length)
			this.morebuf(n);
		this.#bytes.set(new Uint8Array(s), this.#write);
		this.#write += n;
	}
	readChar() {
		return this.#read < this.#write ? this.#bytes[this.#read++] : undefined;
	}
	readChars(n) {
		if (this.#read + n > this.#write)
			return;
		let v = 0;
		while (--n >= 0)
			v = (v << 8) | this.#bytes[this.#read++];
		return v;
	}
	readChunk(n, reference) {
		if (this.#read + n > this.#write)
			return undefined;

		let result;
		if (reference)
			result = new Uint8Array(this.#bytes.buffer, this.#bytes.byteOffset + this.#read, n);
		else
			result = this.#bytes.slice(this.#read, this.#read + n).buffer;

		this.#read += n;

		return result;
	}
	getChunk() {
		let bytes = this.#bytes;
		this.#bytes = undefined;
		if (bytes.i) {
			delete bytes.i;
			bytes.buffer.resize(this.#write); 
		}
		else
			bytes = new Uint8Array(bytes.buffer, bytes.byteOffset, this.#write);
		return bytes;
	}
	get bytesAvailable() {
		return this.#write - this.#read;
	}
	get bytesWritten() {
		return this.#write;
	}
	close() {
		this.#bytes = undefined;
	}
}

export default SSLStream;

/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
	constructor(buf) {
		if (buf) {
			if (buf instanceof Uint8Array)
				this.buf = buf;
			else
				this.buf = new Uint8Array(buf);
			this.widx = this.buf.length;
		}
		else {
			this.buf = new Uint8Array(128);
			this.widx = 0;
		}
		this.ridx = 0;
	};
	morebuf(n) {
		if (n < 128) n = 128;
		var nbuf = new Uint8Array(this.buf.length + n);
		nbuf.set(this.buf);
		this.buf = nbuf;
	}
	writeChar(c) {
		if (this.widx >= this.buf.length)
			this.morebuf(1);
		this.buf[this.widx++] = c;
	};
	writeChars(v, n) {
		if (this.widx + n > this.buf.length)
			this.morebuf(n);
		while (--n >= 0)
			this.buf[this.widx++] = (v >>> (n * 8)) & 0xff;
	};
	writeChunk(a) {
		var n = a.byteLength;
		if (n <= 0)
			return;
		if (this.widx + n > this.buf.length)
			this.morebuf(n);
		this.buf.set((a instanceof Uint8Array) ? a : (new Uint8Array(a)), this.widx);
		this.widx += n;
	};
	writeString(s) {
		var n = s.length;
		if (n <= 0)
			return;
		if (this.widx + n > this.buf.length)
			this.morebuf(n);
		this.buf.set(new Uint8Array(ArrayBuffer.fromString(s)), this.widx);
		this.widx += n;
	};
	readChar() {
		return this.ridx < this.widx ? this.buf[this.ridx++] : undefined;
	};
	readChars(n) {
		if (this.ridx + n > this.widx)
			return undefined;
		var v = 0;
		while (--n >= 0)
			v = (v << 8) | this.buf[this.ridx++];
		return v;
	};
	readChunk(n, reference) {
		if (this.ridx + n > this.widx)
			return undefined;
		if (reference) {
			let result = new Uint8Array(this.buf.buffer, this.buf.byteOffset + this.ridx, n)
			this.ridx += n;
			return result;
		}

		let tbuf = this.buf.slice(this.ridx, this.ridx + n);
		this.ridx += n;
		return tbuf.buffer;
	};
	getChunk() {
		let result = new Uint8Array(this.buf.buffer, this.buf.byteOffset, this.widx);
		delete this.buf;
		delete this.widx;
		delete this.ridx;
		return result;
	};
	get bytesAvailable() {
		return this.widx - this.ridx;
	};
	get bytesWritten() {
		return this.widx;
	};
	close() {
		delete this.buf;
	}
};

Object.freeze(SSLStream.prototype);

export default SSLStream;

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

import Arith from "arith";

export default class BER {
	constructor(buf) {
		this.i = 0;
		if (buf instanceof ArrayBuffer)
			this.a = new Uint8Array(buf)
		else if (buf instanceof Uint8Array)
			this.a = buf;
		else
			this.a = new Uint8Array(0);
	};
	getTag() {
		return this.a[this.i++];
	};
	getLength() {
		var len = this.a[this.i++];
		if (len & 0x80) {
			var lenlen = len & ~0x80;
			if (lenlen == 0)
				return -1;	// indefinite length
			len = 0;
			while (--lenlen >= 0)
				len = (len << 8) | this.a[this.i++];
		}
		return len;
	};
	peek() {
		return this.a[this.i];
	};
	skip(n) {
		this.i += n;
	};
	next() {
		let i = this.i;
		void this.getTag();
		let length = this.getLength();		//@@ merging this line with the next fails. should it??
		this.i += length;
		return new Uint8Array(this.a.buffer, this.a.byteOffset + i, this.i - i);
	};
	getInteger() {
		if (this.getTag() != 2)
			throw new Error("BER: not an integer");
		let length = this.getLength();
		let ai = new Arith.Integer(new Uint8Array(this.a.buffer, this.a.byteOffset + this.i, length));
		this.i += length;
		return ai;
	};
	getBitString() {
		let result;
		if (this.getTag() != 3)
			throw new Error("BER: not a bit string");
		let length = this.getLength();
		let pad = this.a[this.i++];
		if (pad) {
			result = new Uint8Array(length - 1);
			for (var i = 0; i < length - 1; i++)
				bs[i] = this.a[this.i++] >>> pad;
		}
		else {
			result = new Uint8Array(this.a.buffer, this.a.byteOffset + this.i, length - 1);
			this.i += length;
		}
		return result;
	};
	getOctetString() {
		if (this.getTag() != 0x04)
			throw new Error("BER: not a octet string");
		var len = this.getLength();
		return this.getChunk(len);
	};
	getObjectIdentifier() {
		if (this.getTag() != 0x06)
			throw new Error("BER: not an object identifier");
		var len = this.getLength();
		return this._getObjectIdentifier(len)
	}
	_getObjectIdentifier(len) {
		let oid = [];
		let i = this.a[this.i++];
		let rem = i % 40;
		oid.push((i - rem) / 40);
		oid.push(rem);
		--len;
		while (len > 0) {
			let v = 0;
			while (--len >= 0 && (i = this.a[this.i++]) >= 0x80)
				v = (v << 7) | (i & 0x7f);
			oid.push((v << 7) | i);
		}
		return Int32Array.from(oid);
	};
	getSequence() {
		if (this.getTag() != 0x30)
			throw new Error("BER: not a sequence");
		var len = this.getLength();
		var seq = new Uint8Array(this.a.buffer, this.a.byteOffset + this.i, len);
		this.i += len;
		return seq;
	};
	getChunk(n) {
		let res = new Uint8Array(this.a.buffer, this.a.byteOffset + this.i, n);
		this.i += n;
		return res;
	};
	getBuffer() {
		return this.a.slice(0, this.i).buffer;
	};

	morebuf(n) {
		if (n < 128) n = 128;
		var nbuf = new Uint8Array(this.a.length + n);
		nbuf.set(this.a);
		this.a = nbuf;
	};
	getc() {
		return this.a[this.i++];
	};
	putc(c) {
		if (this.i >= this.a.length)
			this.morebuf(1);
		this.a[this.i++] = c;
	};
	putTag(tag) {
		this.putc(tag);
	};
	putLength(len) {
		if (len < 128)
			this.putc(len);
		else {
			var lenlen = 1;
			var x = len;
			while (x >>>= 8)
				lenlen++;
			this.putc(lenlen | 0x80);
			while (--lenlen >= 0)
				this.putc(len >>> (lenlen * 8));
		}
	};
	putChunk(c) {
		if (this.i + c.byteLength > this.a.length)
			this.morebuf(c.byteLength);
		this.a.set(new Uint8Array(c), this.i);
		this.i += c.byteLength;
	};
	static itoa(n, col = 2) {
		var a = n.toString();
		var prepend = col - a.length;
		while (--prepend >= 0)
			a = '0' + a;
		return a;
	};
	static encode(arr) {
		var b = new BER();
		var tag = arr[0];
		var val = arr.length > 1 ? arr[1] : undefined;
		b.putTag(tag);
		switch (tag) {
		case 0x01:	// boolean
			b.putLength(1);
			b.putc(val ? 1 : 0);
			break;
		case 0x02:	// integer
			var c = val.toChunk(0, true);	// signess = true
			b.putLength(c.byteLength);
			b.putChunk(c);
			break;
		case 0x03:	// bit string
			b.putLength(val.byteLength + 1);
			var pad = val.byteLength * 8 - arr[2];
			b.putc(pad);
			if (pad != 0) {
				for (var i = 0; i < val.byteLength; i++)
					b.putc(val[i] << pad);
			}
			else
				b.putChunk(val);
			break;
		case 0x04:	// octet string
			b.putLength(val.byteLength);
			b.putChunk(val);
			break;
		case 0x05:	// null
			b.putLength(0);
			break;
		case 0x06:	// object identifier
			var t = new BER();
			t.putc(val[0] * 40 + (val.length < 2 ? 0 : val[1]));
			for (var i = 2; i < val.length; i++) {
				var x = val[i];
				var n = 1;
				while (x >>>= 7)
					n++;
				x = val[i];
				while (--n >= 1)
					t.putc((x >>> (n * 7)) | 0x80);
				t.putc(x & 0x7f);
			}
			b.putLength(t.i);
			b.putChunk(t.getBuffer());
			break;
		case 0x09:	// real -- not supported
			debugger;
			break;
		case 0x07:	// object descriptor
		case 0x0c:	// UTF8 string
		case 0x12:	// numeric string
		case 0x13:	// printable string
		case 0x14:	// telex string
		case 0x16:	// IA5 string
			var c = ArrayBuffer.fromString(val);
			b.putLength(c.byteLength);
			b.putChunk(c);
			break;
		case 0x17:	// UTC time
		case 0x18:	// generalized time
			var date = new Date(val);
			/*
			var s = (date.getUTCFullYear() - (tag == 0x17 ? 1900: 0)).toString() +
				(date.getUTCMonth()).toString() +
				(date.getUTCDate()).toString() +
				(date.getUTCHours()).toString() +
				(date.getUTCMinutes()).toString() +
				(date.getUTCSeconds()).toString() +
				(tag == 0x18 ? "." + (date.getUTCMilliSedonds()).toString(): "") +
				"Z";
			*/
			var s = "";
			var yy = date.getUTCFullYear();
			if (tag == 0x17)
				yy -= yy >= 2000 ? 2000 : 1900;
			s += this.itoa(yy);
			s += this.itoa(date.getUTCMonth() + 1);
			s += this.itoa(date.getUTCDate());
			s += this.itoa(date.getUTCHours());
			s += this.itoa(date.getUTCMinutes());
			s += this.itoa(date.getUTCSeconds());
			if (tag == 0x18)
				s += this.itoa(date.getUTCMilliSedonds());
			s += "Z";
			var c = ArrayBuffer.fromString(s);
			b.putLength(c.byteLength);
			b.putChunk(c);
			break;
		case 0x19:	// graphics string
		case 0x1a:	// ISO64 string
		case 0x1b:	// general string
		case 0x1c:	// universal string
		case 0x1e:	// BMP string
			b.putLength(val.byteLength);
			b.putChunk(val);
			break;
		case 0x30:
		case 0x31:
			var len = 0;
			var seq = [];
			for (var i = 1; i < arr.length; i++) {
				var e = this.encode(arr[i]);
				seq.push(e);
				len += e.byteLength;
			}
			b.putLength(len);
			for (var i = 0; i < seq.length; i++)
				b.putChunk(seq[i]);
			break;
		default:
			if ((tag >> 6) == 2) {
				b.putLength(val.byteLength);
				b.putChunk(val);
			}
			break;
		}
		return b.getBuffer();
	};
	static decode(a) {
		return this._decode(new BER(a));
	}
	static _decode(b) {
		let tag = b.getTag();
		if (tag == 0) {
			// must be 00 terminator
			if (b.getc() != 0)
				throw new Error();
			return null;
		}
		if ((tag & 0x1f) == 0x1f) {
			// extended tag -- just ignore
			while (b.getc() >= 0x80)
				;
		}
		let res = this.decodeTag(tag, b, b.getLength());
		if (!(res instanceof Array))
			res = [res];
		res.unshift(tag);
		return res;
	};
	static decodeTag(tag, b, len) {
		let res;
		if ((tag >> 6) == 2) {	// context specific class
			// just get a content
			return b.getChunk(len);
		}
		if (tag & 0x20) {	// construct type
			let seq = [], r;
			if (len < 0) {
				while (r = this._decode(b))
					seq.push(r);
			}
			else {
				let endOffset = b.i + len;
				while (b.i < endOffset && (r = this._decode(b)))
					seq.push(r);
			}
			return seq;
		}
		// universal class
		if (len < 0)
			throw new Error("BER: no unspecific length");
		switch (tag) {
		case 0x01:	// boolean
			if (len != 1)
				throw new Error();
			res = b.getc() != 0;
			break;
		case 0x02:	// integer
			res = new Arith.Integer(b.getChunk(len));
			break;
		case 0x03:	{// bit string
			let pad = b.getc();
			if (pad == 0) {
				res = [b.getChunk(len - 1), (len - 1) * 8];
			}
			else {
				var c = Uint8Array(len - 1);
				for (var i = 0; i < len - 1; i++)
					c[i] = b.getc() >>> pad;
				res = [c, (len - 1) * 8 - pad];
			}
			}
			break;
		case 0x04:	// octet string
			res = b.getChunk(len);
			break;
		case 0x05:	// null
			res = null;
			break;
		case 0x06:	// object identifier
			res = [b._getObjectIdentifier(len)];
			break;
		case 0x09:	// real -- not supported
			throw new Error("BER: unsupported");
			break;
		case 0x07:	// object descriptor
		case 0x0c:	// UTF8 string
		case 0x12:	// numeric string
		case 0x13:	// printable string
		case 0x14:	// telex string
		case 0x16:	// IA5 string
			res = String.fromArrayBuffer(b.getChunk(len));
			break;
		case 0x17:	// ITC time
		case 0x18: {// generalized time
			let s = String.fromArrayBuffer(b.getChunk(len));
			let prefix = ""
			if (tag == 0x18) {
				prefix = s.substring(0, 2);
				s = s.substring(2);
			}
			let ymd = /(\d\d)(\d\d)(\d\d)(\d\d)(\d\d)(\d*)[\.]*(\d*)([Z\+\-].*)/.exec(s);
			if (tag == 0x18)
				ymd[1] = prefix + ymd[1];
			else
				ymd[1] = (ymd[1] >= 90) ? "19": "20" + ymd[1];
			let date = Date.UTC(ymd[1], ymd[2] - 1, ymd[3], ymd[4], ymd[5], ymd[6], ymd[7]);
			if (ymd[8] && ymd[8] != "Z") {
				let ms = /(\d\d)(\d\d)/.exec(ymd[8]);
				let dif = Date.setUTCHours(ms[1], ms[2]);
				date -= dif;
			}
			res = date;
			}
			break;
		case 0x19:	// graphics string
		case 0x1a:	// ISO64 string
		case 0x1b:	// general string
		case 0x1c:	// universal string
		case 0x1e:	// BMP string
			res = b.getChunk(len);
			break;
		case 0x1f:	// extended tag
			// just return the content
			res = b.getChunk(len);
			break;
		}
		return res;
	};
};

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

import RNG from "rng";
import BER from "ber";

export default class PKCS1 {
	static I2OSP(I, l) {
		var c = ArrayBuffer.fromBigInt(I);
		if (l && l > c.byteLength) {
			// prepend 0
			var d = l - c.byteLength;
			var t = new Uint8Array(d);
			for (var i = 0; i < d; i++)	// just in case
				t[i] = 0x00;
			c = t.buffer.concat(c);
		}
		return c;
	};
	static sIS2SP(sI, l) {
		if (!l) {
			l = 4;
			var c = new Uint8Array(l);
			var skip = true;
			var i = 0;
			while (--l >= 0) {
				var x;
				if ((x = (sI >>> (l*8))) != 0 || !skip) {
					c[i++] = x & 0xff;
					skip = false;
				}
			}
			if (i == 0)
				c[i++] = 0;
			c = c.slice(0, i);
		}
		else {
			// l must be <= 4
			var c = new new Uint8Array(l);
			var i = 0;
			while (--l >= 0)
				c[i++] = (sI >>> (l*8)) & 0xff;
		}
		return c.buffer;
	};
	static OS2IP(OS) {
		if (OS instanceof Uint8Array) {
			let offset = OS.byteOffset;
			OS = OS.buffer.slice(offset, offset + OS.byteLength);
		}
		return BigInt.fromArrayBuffer(OS);
	};
	static randint(max, z) {
		var i = BigInt.fromArrayBuffer(RNG.get(BigInt.bitLength(max) >>> 3));
		while (i >= max)
			i >>>= 1;
		return i;
	};
	static parse(buf, privFlag) {
		// currently RSA only
		var key = {};
		var ber = new BER(buf);
		if (ber.getTag() != 0x30)	// SEQUENCE
			throw new Error("PKCS1: not a sequence");
		ber.getLength();	// skip the sequence length
		ber.getInteger();	// ignore the first INTEGER
		key.modulus = ber.getInteger();
		key.exponent = ber.getInteger();
		if (privFlag) {
			key.privExponent = ber.getInteger();
			key.prim1 = ber.getInteger();
			key.prim2 = ber.getInteger();
			key.exponent1 = ber.getInteger();
			key.exponent2 = ber.getInteger();
			key.coefficient = ber.getInteger();
		}
		return key;
	};
};

Object.freeze(PKCS1.prototype);

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

import Bin from "bin";
import RSA from "rsa";
import PKCS1 from "pkcs1";
import BER from "ber";
import RNG from "rng";

export default class PKCS1_5 {
	constructor(key, priv, oid) {
		this.rsa = new RSA(priv ? key.privExponent: key.exponent, key.modulus, key.prim1, key.prim2, key.exponent1, key.exponent2, key.coefficient);
		this.modulusSize = this.rsa.modulusSize;
		this.oid = oid;
	};
	emsaEncode(H, emLen) {
		if (this.oid)
			var bc = BER.encode([0x30, [0x30, [0x06, this.oid], [0x05]], [0x04, H]]);
		else
			var bc = H;
		// prepend the prefix part
		var ffsize = emLen - bc.byteLength - 2;
		var s = new Uint8Array(ffsize + 2);
		var i = 0;
		s[i++] = 0x01;
		for (; i <= ffsize; i++)
			s[i] = 0xff;
		s[i++] = 0x00;
		return BigInt.fromArrayBuffer(s.buffer.concat(bc));
	};
	emsaDecode(EM) {
		let s = new Uint8Array(ArrayBuffer.fromBigInt(EM));
		let i = 0;
		if (s[i++] != 0x01)
			return;		// not an encoded data??
		for (; i < s.length && s[i] == 0xff; i++)
			;
		if (i >= s.length || s[i] != 0x00)
			return;		// decode failed

		let bc = s.slice(i+1);
		if (!this.oid)
			return bc.buffer;

		// not using BER to decode this to reduce stack use
		if ((0x30 != bc[0]) || (0x30 != bc[2]))
			return;

		let position = 3;
		let length = bc[position++];

		if (6 != bc[position++])
			return;

		length = bc[position++];

		// decode oid
		i = bc[position++];
//		const oid = [Math.idiv(i, 40), Math.irem(i, 40)];
		--length;
		while (length > 0) {
			let v = 0;
			while (--length >= 0 && (i = bc[position++]) >= 0x80)
				v = (v << 7) | (i & 0x7f);
//			oid.push((v << 7) | i);
		}

		if ((5 != bc[position]) || (0 != bc[position + 1]) || (4 != bc[position + 2]))
			return;

		position += 3;
		length = bc[position++];
		return bc.slice(position, position + length);
	};
	emeEncode(M, emLen) {
		var pssize = emLen - M.byteLength - 2;
		if (pssize < 0)
			throw new Error("emeEncode malformed input");
		var s = new Uint8Array(emLen);
		var ps = new Uint8Array(RNG.get(pssize));
		var i = 0;
		s[i++] = 0x02;
		for (var j = 0; j < ps.length; j++) {
			// make sure of nonzero
			var c = ps[j];
			if (c == 0)
				c = 0xff;
			s[i++] = c;
		}
		s[i++] = 0x00;
		s.set(M, pssize + 2);
		return BigInt.fromArrayBuffer(s.buffer);
	};
	emeDecode(EM) {
		var s = new Uint8Array(ArrayBuffer.fromBigInt(EM));
		var i = 0;
		if (s[i++] != 0x02)
			return;		// not an encoded data??
		var c;
		for (; i < s.length; i++) {
			if (s[i] == 0x00)
				return s.slice(i + 1).buffer;
		}
		// decode failed
	};
	sign(H) {
		var f = this.emsaEncode(H, this.modulusSize-1);
		var v = this.rsa.process(f);
		return PKCS1.I2OSP(v, this.modulusSize);
	};
	verify(H, sig) {
		let R = this.emsaDecode(this.rsa.process(PKCS1.OS2IP(sig)));
		return !!R && Bin.comp(H, R) == 0;
	};
	encrypt(M) {
		var e = this.emeEncode(M, this.modulusSize-1);
		var v = this.rsa.process(e);
		return PKCS1.I2OSP(v, this.modulusSize);
	};
	decrypt(e) {
		var d = this.rsa.process(PKCS1.OS2IP(e));
		var M = this.emeDecode(d);
		if (!M)
			throw new Error("pkcs1_5: malformed input");
		return M;
	};
};

Object.freeze(PKCS1_5.prototype);

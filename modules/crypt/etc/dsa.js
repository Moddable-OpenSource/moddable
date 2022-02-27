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

import Crypt from "crypt";
import Mont from "mont";
import BER from "ber";

export default class DSA {
	constructor(key, priv) {
		this.x = priv ? key.x : key.y;
		this.g = key.g;
		this.p = new Mont({m: key.p, method: Mont.SW});
		this.q = new Mont({m: key.q, method: Mont.SW});
	};
	_sign(H) {
		// r = (g^k mod p) mod q
		// s = (SHA_1(M) + xr)/k mod q
		var p = this.p;
		var q = this.q;
		var g = this.g;
		var x = this.x;
		var k = this.randint(q.m, this.z);
		var r = q.mod(p.exp(g, k));
		var H = BigInt.fromArrayBuffer(H);
		var s = q.mul(q.mulinv(k), q.add(H, q.mul(x, r)));
		var sig = new Object;
		sig.r = r;
		sig.s = s;
		return sig;
	};
	sign(H, asn1) {
		var sig = this._sign(H);
		if (asn1) {
			return BER.encode([0x30, [0x02, sig.r], [0x02, sig.s]]);
		}
		else {
			var os = new ArrayBuffer();
			return os.concat(Crypt.PKCS1.I2OSP(sig.r, 20), Crypt.PKCS1.I2OSP(sig.s, 20));
		}
	};
	_verify(H, r, s) {
		// w = 1/s mod q
		// u1 = (SHA_1(M) * w) mod q
		// u2 = rw mod q
		// v = (g^u1 * y^u2 mod p) mod q
		var p = this.p;
		var q = this.q;
		var g = this.g;
		var y = this.x;		// as the public key
		var w = q.mulinv(s);
		var h = BigInt.fromArrayBuffer(H);
		var u1 = q.mul(h, w);
		var u2 = q.mul(r, w);
		var v = q.mod(p.exp2(g, u1, y, u2));
		return this.comp(v, r) == 0;
	};
	verify(H, sig, asn1) {
		var r, s;
		if (asn1) {
			let ber = new BER(sig);
			let seq = new BER(ber.getSequence());
			r = seq.getInteger();
			s = seq.getInteger();
		}
		else {
			// "20" is specified in the xmldsig-core spec.
			r = Crypt.PKCS1.OS2IP(sig.slice(0, 20));
			s = Crypt.PKCS1.OS2IP(sig.slice(20, 40));
		}
		return(this._verify(H, r, s));
	};
};

Object.freeze(DSA.prototype);

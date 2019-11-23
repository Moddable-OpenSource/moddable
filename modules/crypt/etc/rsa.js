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

import Modular from "modular";
import Mont from "mont";

export default class RSA {
	constructor(e, m, p, q, dp, dq, C2) {
		if (BigInt.bitLength(e) > 17 && p && q) {
			// use CRT
			this.dp = dp ? dp : e % (p - 1);
			this.dq = dq ? dq : e % (q - 1);
			this.mp = new Mont({m: p, method: Mont.SW});
			this.mq = new Mont({m: q, method: Mont.SW});
			this.C2 = C2 ? C2 : this.mp.mulinv(q);	// (C2 = q^-1 mod p) according to PKCS8
			this.p = p;
			this.q = q;
			if (!m)
				m = z.mul(p, q);
		}
		else {
			if (!m && p && q)
				m = z.mul(p, q);
			this.mn = new Modular(m);
			this.e = e;
		}
		let n = BigInt.bitLength(m);
		this.orderSize = (n + 7) >>> 3;
	};
	process(c) {
		if (this.mn)
			return this.mn.exp(c, this.e);

		let v1 = this.mq.exp(c, this.dq);
		let v2 = this.mp.exp(c, this.dp);
		let u = this.mp.mul(this.C2, this.mp.sub(v2, v1));
		return v1 + (u * this.q);
	};
	get modulusSize() {
		return this.orderSize;
	};
};

Object.freeze(RSA.prototype);

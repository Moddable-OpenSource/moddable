/*
 * Copyright (c) 2016-2024  Moddable Tech, Inc.
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

import EC from "ec";
import ECPoint from "ecp";

const curves = {
	secp256r1: {
		m: 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFn,
		a: 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFCn,
		b: 0x5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604bn,
		Gx: 0x6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296n,
		Gy: 0x4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5n,
		n: 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551n,
	},
	secp384r1: {
		m: 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFFn,
		a: 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFCn,
		b: 0xB3312FA7E23EE7E4988E056BE3F82D19181D9C6EFE8141120314088F5013875AC656398D8A2ED19D2A85C8EDD3EC2AEFn,
		Gx: 0xAA87CA22BE8B05378EB1C71EF320AD746E1D3B628BA79B9859F741E082542A385502F25DBF55296C3A545E3872760AB7n,
		Gy: 0x3617DE4A96262C6F5D9E98BF9292DC29F8F41DBD289A147CE9DA3113B5F0B8C00A60B1CE1D7E819D7A431D7C90EA0E5Fn,
		n: 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF581A0DB248B0A77AECEC196ACCC52973n,
	},
};
Object.freeze(curves, true);

export default class Curve {
	constructor(name) {
		if (!(name in curves))
			return;
		const curve = curves[name];
		this.G = new ECPoint(curve.Gx, curve.Gy);
		this.ec = new EC(curve.a, curve.b, curve.m);
		this.n = curve.n;
	};
	dh(x, P) {
		let G;
		if (P) {
			if (P.byteLength != this.orderSize * 2 + 1)
				return;
			let x = P.slice(1, this.orderSize + 1);
			let y = P.slice(this.orderSize + 1, this.orderSize * 2 + 1);
			G = new ECPoint(BigInt.fromArrayBuffer(x), BigInt.fromArrayBuffer(y));
		}
		else
			G = this.G;
		P = this.ec.mul(G, BigInt.fromArrayBuffer(x));
		let xs = ArrayBuffer.fromBigInt(P.X, this.orderSize);
		let ys = ArrayBuffer.fromBigInt(P.Y, this.orderSize);
		let a = new Uint8Array(this.orderSize * 2 + 1);
		a[0] = 0x04;	// uncompressed
		a.set((new Uint8Array(xs)), 1);
		a.set((new Uint8Array(ys)), this.orderSize + 1);
		return a.buffer;
	};
	get orderSize() {
		let n = BigInt.bitLength(this.n);
		return (n + 7) >>> 3;
	};
	Z(P) {
		return P.slice(1, this.orderSize + 1);
	};
};

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

/*
const curves = {
	secp256r1: {
		m: 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFn,
		a: 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFCn,
		b: 0x5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604bn,
		Gx: 0x6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296n,
		Gy: 0x4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5n,
		n: 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551n,
	},
};
*/
const curves = {
	secp256r1: {
		m: "0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF",
		a: "0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC",
		b: "0x5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b",
		Gx: "0x6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296",
		Gy: "0x4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5",
		n: "0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551",
	},
};
Object.freeze(curves);

export default class Curve {
	constructor(name) {
		if (!(name in curves))
			return;
		let curve = curves[name];
		this.G = new Arith.ECPoint(new Arith.Integer(curve.Gx), new Arith.Integer(curve.Gy));
		this.ec = new Arith.EC(new Arith.Integer(curve.a), new Arith.Integer(curve.b), new Arith.Module(new Arith.Z(), new Arith.Integer(curve.m)));
		this.n = new Arith.Integer(curve.n);
	};
	dh(x, P) {
		let G;
		if (P) {
			if (P.byteLength != this.orderSize * 2 + 1)
				return;
			
			let x = P.slice(1, this.orderSize + 1);
			let y = P.slice(this.orderSize + 1, this.orderSize * 2 + 1);
			G = new Arith.ECPoint(new Arith.Integer(x), new Arith.Integer(y));
		}
		else
			G = this.G;
		P = this.ec.mul(G, new Arith.Integer(x));
		let xs = P.X.toChunk(this.orderSize);
		let ys = P.Y.toChunk(this.orderSize);
		let a = new Uint8Array(this.orderSize * 2 + 1);
		a[0] = 0x04;	// uncompressed
		a.set((new Uint8Array(xs)), 1);
		a.set((new Uint8Array(ys)), this.orderSize + 1);
		return a.buffer;
	};
	get orderSize() {
		return this.n.sizeof();
	};
	Z(P) {
		return P.slice(1, this.orderSize + 1);
	};
};

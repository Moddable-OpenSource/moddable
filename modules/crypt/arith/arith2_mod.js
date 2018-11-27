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

import Integer from "arith2_int";

export default class Module {
	constructor(z, m) {
		this.m = m.value;
	};
	add(a, b) {
		return new Integer((a.value + b.value) % this.m);
	}
	inv(a) {
		return new Integer(this.m - a.value);
	}
	sub(a, b) {
		let c = (a.value - b.value) % this.m;
		return new Integer(c >= 0n ? c : c + this.m);
	}
	mul(a, b) {
		return new Integer((a.value * b.value) % this.m);
	}
	square(a) {
		return new Integer((a.value * a.value) % this.m);
	}
	mulinv(a) {
		return new Integer(this._mulinv_general(a.value, this.m));
	}
	_mulinv_general(a, m) @ "xs_mod2_mulinv_general";
	_mulinv_euclid(a, m) @ "xs_mod2_mulinv_euclid";
	_mulinv_euclid2(a, m) {
		let m0 = m; 
		let y = 0n, x = 1n;

		if (m == 1n)
			return 0n;
		
		while (a > 1n) {
			// q is quotient 
			let q = a / m; 
			let t = m; 
			
			// m is remainder now, process same as 
			// Euclid's algo 
			m = a % m, a = t; 
			t = y; 
			
			// Update y and x 
			y = x - q * y; 
			x = t; 
		} 
		
		// Make x positive 
		if (x < 0n)
			x += m0;
		return x;
  	}
	exp(a, e) {
		return new Integer(this._exp(a.value, e.value, this.m));
	}
	_exp(a, e, m) @ "xs_mod2_exp";
	mod(a) {
		return new Integer(a.value % this.m);
	}
};

Object.freeze(Module.prototype);

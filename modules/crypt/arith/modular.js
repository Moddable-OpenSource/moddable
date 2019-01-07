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

export default class Modular {
	constructor(m) {
		this.m = m;
	};
	add(a, b) {
		return a + b % this.m;
	}
	inv(a) {
		return this.m - a;
	}
	sub(a, b) {
		let c = (a - b) % this.m;
		return c >= 0n ? c : c + this.m;
	}
	mul(a, b) {
		return a * b % this.m;
	}
	square(a) {
		return a * a % this.m;	// can be optimized
	}
	mulinv(a) {
		return this._mulinv_general(a, this.m);
	}
	_mulinv_general(a, m) @ "xs_mod2_mulinv_general";
	_mulinv_euclid(a, m) @ "xs_mod2_mulinv_euclid";
	exp(a, e) {
		return this._exp(a, e, this.m);
	}
	_exp(a, e, m) @ "xs_mod2_exp";
	mod(a) {
		return a % this.m;
	}
};

Object.freeze(Modular.prototype);

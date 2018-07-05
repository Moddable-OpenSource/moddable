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

export default class Z @ "xs_z_destructor" {
	constructor() {
		this._proto_int = Arith.Integer.prototype;
		this._init();
	};
	add(a, b) @ "xs_z_add";
	sub(a, b) @ "xs_z_sub";
	mul(a, b) @ "xs_z_mul";
	div2(a, b) @ "xs_z_div2";
	div(a, b) @ "xs_z_div";
	mod(a, b) @ "xs_z_mod";
	square(a) @ "xs_z_square";
	xor(a, b) @ "xs_z_xor";
	or(a, b) @ "xs_z_or";
	and(a, b) @ "xs_z_and";
	lsl(a, b) @ "xs_z_lsl";
	lsr(a, b) @ "xs_z_lsr";
	_init() @ "xs_z_init";
	inc(a, d) {
		return this.add(a, new Arith.Integer(d));
	};
	toString(i, radix) @ "xs_z_toString";
	toInteger(digits, radix) @ "xs_z_toInteger";
};

Object.freeze(Z.prototype);

let
q,
r
;

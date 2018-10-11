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

export default class Integer @ "xs_integer_destructor" {
	constructor(a, opt1, opt2) {
		this._init();
		switch (typeof a) {
		case "undefined":
			this.setNaN();
			break;
		case "number":
			this.setNumber(a);
			break;
		case "string":
			this.setString(a);
			break;
		case "object":
			if ((a instanceof ArrayBuffer) || (a instanceof Uint8Array))
				this.setChunk(a, opt1, opt2);
			else if (a instanceof Integer)
				this.setInteger(a);
			else
				this.setNaN();
			break;
		default:
			this.setNaN();
			break;
		}
	};

	toChunk(minBytes, signess) @ "xs_integer_toChunk";
	toNumber() @ "xs_integer_toNumber";
	negate() @ "xs_integer_negate";
	isZero() @ "xs_integer_isZero";
	isNaN() @ "xs_integer_isNaN";
	comp(a) @ "xs_integer_comp";
	sign() @ "xs_integer_sign";
	sizeof() @ "xs_integer_sizeof";
	setNumber(n) @ "xs_integer_setNumber";
	setChunk(chunk, opt1, opt2) @ "xs_integer_setChunk";
	setInteger(i) @ "xs_integer_setInteger";
	setNaN() @ "xs_integer_setNaN";
	free() @ "xs_integer_free";
	_init() @ "xs_integer_init";
	inc(d) @ "xs_integer_inc";
	toHexString() @ "xs_integer_toHexString";
	setHexString(xstr) @ "xs_integer_setHexString";
	toString(radix, col) {
		var str;
		if (radix == 16)
			str = this.toHexString();
		else {
			var z = new Arith.Z();
			str = z.toString(this, radix || 10);
		}
		if (col && (col -= str.length) > 0) {
			var ns = "", i = 0;
			if (str[0] == '-' || str[0] == '+') {
				i = 1;
				ns = str[0];
				--col;
			}
			while (--col >= 0)
				ns += "0";
			ns += str.substr(i);
			str = ns;
		}
		return str;
	};
	setString(digits) {
		var i = 0;
		function nextc() {
			return i < digits.length ? digits[i++] : undefined;
		}
		var radix = 10;
		var neg = false;
		var c = nextc();
		if (c == '-') {
			neg = true;
			c = nextc();
		}
		else if (c == '+')
			c = nextc();
		if (c == '0') {
			c = nextc();
			if (c == 'x' || c == 'X') {
				radix = 16;
				c = nextc();
			}
			else
				radix = 8;
		}
		if (radix == 16) {
			// short cut -- can do without loading Z
			this.setHexString(digits);
			return;
		}
		var z = new Arith.Z();
		if (--i > 0)
			digits = digits(i);
		this.setInteger(z.toInteger(digits, radix));
		if (neg)
			this.negate();
	};
};

Object.freeze(Integer.prototype);

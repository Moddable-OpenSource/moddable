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

export default class Integer {
	constructor(a, opt1, opt2) {
		switch (typeof a) {
		case "number":
		case "string":
			this.value = BigInt(a);
			break;
		case "undefined":
			this.value = undefined;
			break;
		case "bigint":
			this.value = a;
			break;
		case "object":
			if ((a instanceof ArrayBuffer) || (a instanceof Uint8Array))
				this.value = this.fromChunk(a, opt1, opt2);
			else if (a instanceof Integer)
				this.value = a.value;
			else
				this.value = undefined;
			break;
		default:
			this.value = undefined;
			break;
		}
	};

	fromChunk(sign, lsb) @ "xs_integer2_fromChunk";
	toChunk(minBytes, signess) {
		return this._toChunk(this.value, minBytes, signess);
	}
	_toChunk(a, minBytes, signess) @ "xs_integer2_toChunk";
	toNumber() {
		return BigInt.asIntN(32, this.value);
	}
	negate() {
		this.value = -this.value;
	}
	isZero() {
		return this.value == 0n;
	}
	isNaN() {
		return this.value == undefined;
	}
	comp(a) {
		return this.value - a.value;
	}
	sign() {
		return this.value >= 0n;
	}
	sizeof() {
		return this._sizeof(this.value);
	}
	_sizeof() @ "xs_integer2_sizeof";
	inc(d) {
		if (d === undefined)
			d = 1;
		this.value += BigInt(d);
	}
	toHexString() {
		return this.value.toString(16);
	}
	setHexString(xstr) {
		this.value = BigInt("0x" + xstr);
	}
	toString(radix, col) {
		let str = this.value.toString(radix || 10);
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
		this.value = BigInt(digits);
	};
	valueOf() {
		return this.value;
	}
};

Object.freeze(Integer.prototype);

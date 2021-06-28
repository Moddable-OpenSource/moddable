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

import Modular from "modular";

export default class ECPoint {
	constructor(x, y, identity = false) {
		this.x = x;
		this.y = y;
		this.z = identity ? 0n : 1n;
	};
	get X() {
		return this.x;
	}
	get Y() {
		return this.y;
	}
	set X(x) {
		this.x = x;
	}
	set Y(y) {
		this.y = y;
	}
	isZero() {
		return this.z == 0n;
	};
	toString() {
//		this.norm();
		if (this.isZero())
			return "0"
		else
			return this.x + "," + this.y + "," + this.z;
	}
	static serialize(o) {
		o.toString();
	};
	static parse(txt) {
		let a = txt.split(",");
		if (3 === a.length) {
			return new ECPoint(a[0], a[1], a[2]);
		}
	};
	static fromOctetString(os) {
		if (os[0] != 0x04)
			throw new Error("unsupported format");
		let flen = (os.length - 1) / 2;
		let x = BigInt.fromArrayBuffer(os.slice(1, 1 + flen).buffer);
		let y = BigInt.fromArrayBuffer(os.slice(1 + flen, os.length).buffer);
		return new ECPoint(x, y);
	}
};

Object.freeze(ECPoint.prototype);

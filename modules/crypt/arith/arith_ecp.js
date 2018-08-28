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

export default class ECPoint @ "xs_ecpoint_destructor" {
	constructor(x, y) {
		this._proto_int = Arith.Integer.prototype;
		this._init(x, y);
	};
	_init(x, y) @ "xs_ecpoint_init";
	get identity() @ "xs_ecpoint_getIdentity";
	get x() @ "xs_ecpoint_getX";
	get y() @ "xs_ecpoint_getY";
	set x(x) @ "xs_ecpoint_setX";
	set y(y) @ "xs_ecpoint_setY";
	isZero() {
		return this.identify;
	};
	toString() {
		return this.x + "," + this.y;
	}
	static serialize(o) {
		o.toString();
	};
	static parse(txt) {
		var a = txt.split(",");
		if (a.length == 2) {
			return new ECPoint(new Arith.Integer(a[0]), new Arith.Integer(a[1]));
		}
	};
};

Object.freeze(ECPoint.prototype);

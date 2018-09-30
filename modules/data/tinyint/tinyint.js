/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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
 */

export class BitsView extends DataView {
//	getIntBits(bitsOffset, bitsSize, endian) @ "BitsView_prototype_getIntBits"
	getUintBits(bitsOffset, bitsSize, endian) @ "BitsView_prototype_getUintBits"
//	setIntBits(bitsOffset, bitsSize, value, endian) @ "BitsView_prototype_setIntBits"
	setUintBits(bitsOffset, bitsSize, value, endian) @ "BitsView_prototype_setUintBits"
}

export class UintBitsArray {
	constructor(buffer, bitsSize) {
		if ("number" === typeof buffer) {
			this.length = buffer;
			buffer = new ArrayBuffer(((bitsSize * buffer) + 7) >> 3);
		}
		else if (buffer instanceof ArrayBuffer)
			this.length = (buffer.byteLength / bitsSize) | 0;
		else
			throw new Error("invalid buffer");
		this.bitsSize = bitsSize;
		return new Proxy(new BitsView(buffer), this);
	}
	get(target, key) {
		if ("length" === key)
			return this.length;
		if ("buffer" === key)
			return target.buffer;
		const bitsSize = this.bitsSize;
		return target.getUintBits(key * bitsSize, bitsSize);
	}
	set(target, key, value) {
		const bitsSize = this.bitsSize;
		return target.setUintBits(key * bitsSize, bitsSize, value);
	}
}

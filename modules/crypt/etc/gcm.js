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

import {Mode, GHASH} from "crypt";
import Bin from "bin";

export default class GCM {
	constructor(cipher, tagLength = 16) {
		this.block = cipher;
		this.ctr = new Mode("CTR", this.block);
		this.tagLength = tagLength;
	};
	init(iv, aad) {
		let h = this.block.encrypt(new ArrayBuffer(this.block.blockSize));
		this.ghash = new GHASH(h, aad);
		if (iv.byteLength == 12) {
			let one = new DataView(new ArrayBuffer(4));
			one.setUint32(0, 1);	// big endian
			iv = iv.concat(one.buffer);
		}
		else {
			let ghash = new GHASH(h);
			iv = ghash.process(iv);
		}
		this.y0 = iv;
		// start with y1
		let y1 = BigInt.fromArrayBuffer(iv);
		y1++;
		this.ctr.setIV(ArrayBuffer.fromBigInt(y1));
	}
	encrypt(data, buf) {
		buf = this.ctr.encrypt(data, buf);
		this.ghash.update(buf);
		return buf;
	};
	decrypt(data, buf) {
		this.ghash.update(data);
		return this.ctr.decrypt(data, buf);
	};
	close() {
		let t = this.ghash.close();
		return Bin.xor(t, this.block.encrypt(this.y0));
	};
	process(data, buf, iv, aad, encFlag) {
		if (encFlag) {
			this.init(iv, aad);
			buf = this.encrypt(data, buf);
			let tag = this.close();
			if (tag.byteLength > this.tagLength)
				tag = tag.slice(0, this.tagLength);
			return buf.concat(tag);
		}
		else {
			this.init(iv, aad);

			const subarray = data.subarray(0, data.byteLength - this.tagLength);
			buf = this.decrypt(subarray, (buf === data) ? subarray : buf);
			const tag = this.close();
			if (Bin.comp(tag, data.subarray(data.byteLength - this.tagLength), this.tagLength) === 0)
				return buf;
		}
	};
};

Object.freeze(GCM.prototype);

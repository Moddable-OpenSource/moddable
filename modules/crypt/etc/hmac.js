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

import Bin from "bin";

export default class HMAC {
	constructor(h, key, bitlen = 0) {
		this.h = h;
		this.init(key);
		this.hashLen = bitlen >> 3;	// in byte
	};
	init(key) {
		var arr = new Uint8Array(key);
		var h = this.h;
		if (arr.length > h.blockSize) {
			// truncate the key
			h.reset();
			h.update(key);
			arr = new Uint8Array(h.close());
		}
		var n = h.blockSize;
		var l = arr.length;
		this.ipad = new Uint8Array(n);
		this.opad = new Uint8Array(n);
		if (l > n)
			l = n;
		var i = 0;
		for (; i < l; i++) {
			var c = arr[i];
			this.ipad[i] = c ^ 0x36;
			this.opad[i] = c ^ 0x5c;
		}
		for (; i < n; i++) {
			this.ipad[i] = 0x36;
			this.opad[i] = 0x5c;
		}
		h.reset();
		h.update(this.ipad.buffer);
	};
	update(...items) {
		items.forEach(item => this.h.update(item));
	};
	close() {
		let h = this.h;
		let digest = h.close();
		h.reset();
		h.update(this.opad.buffer);
		h.update(digest);
		return h.close();
	};
	reset() {
		this.h.reset();
		this.h.update(this.ipad.buffer);
	};
	process(...items) {
		this.reset();
		items.forEach(item => this.h.update(item));
		return this.close();
	};
	sign(H) {
		this.update(H);
		let sig = this.close();
		if (this.hashLen)
			sig = sig.slice(0, this.hashLen);
		return sig;
	};
	verify(H, sig) {
		this.update(H);
		let t = this.close();
		return Bin.comp(t, sig, this.hashLen) == 0;
	};
};

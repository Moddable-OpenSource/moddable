/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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

const Bin = {
	comp(a1, a2, n = -1) {
		var i1 = new Uint8Array(a1), i2 = new Uint8Array(a2);
		if (n >= 0) {
			if (i1.length < n && i2.length < n)
				;	// fall thru
			else if (i1.length < n)
				return -1;
			else if (i2.length < n)
				return 1;
		}
		else {
			if (i1.length > i2.length)
				return 1;
			else if (i1.length < i2.length)
				return -1;
			n = i1.length;
		}
		for (var i = 0; i < n; i++) {
			if (i1[i] != i2[i])
				return i1[i] - i2[i];
		}
		return 0;
	},
	xor(a1, a2) {
		var i1 = new Uint8Array(a1), i2 = new Uint8Array(a2);
		var len1 = i1.length, len2 = i2.length;
		var r = new Uint8Array(len1);
		for (var i = 0; i < len1; i++)
			r[i] = i1[i] ^ i2[i % len2];
		return r.buffer;
	},
};

Object.freeze(Bin);

export default Bin;

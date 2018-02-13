/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

/**
 * Kinoma LowPAN Framework: Common Utilities
 */
 
const INT_64_SIZE = 8;
const INT_32_SIZE = 4;
const INT_16_SIZE = 2;
const INT_8_SIZE = 1;
const BYTE_SIZE = 8;

function multiIntToByteArray(src, size, length, littleEndian) {
	if (littleEndian === undefined) {
		littleEndian = true;
	}
	let dest = new Uint8Array(size * length);
	for (let index = 0; index < length; index++) {
		for (let p = 0; p < size; p++) {
			if (littleEndian) {
				dest[p + size * index] = (src[index] >> (BYTE_SIZE * p)) & 0xff;
			} else {
				dest[p + size * index] = (src[index] >> (BYTE_SIZE * (size - 1 - p))) & 0xff;
			}
		}
	}
	return dest;
}

function toHexString0(b) {
    let hex = b.toString(16);
    let len = hex.length;
    if (len == 2) {
        return hex;
    } else if (len == 1) {
        return "0" + hex;
    } else {
        return hex.substring(len - 2);
    }
}

export default class Utils {
	static toInt(src, off, len, littleEndian) {
		if (littleEndian === undefined) {
			littleEndian = true;
		}
		let dest = 0;
		for (let p = 0; p < len; p++) {
			let d = src[off + p] & 0xff;
			if (littleEndian) {
				dest |= (d << (BYTE_SIZE * p));
			} else {
				dest |= (d << (BYTE_SIZE * (len - 1 - p)));
			}
		}
		return dest;
	}
	static toInt16(src, littleEndian) {
		return Utils.toInt(src, 0, INT_16_SIZE, littleEndian);
	}
	static toInt32(src, littleEndian) {
		return Utils.toInt(src, 0, INT_32_SIZE, littleEndian);
	}
	static toByteArray(src, size, littleEndian) {
		return multiIntToByteArray([src], size, 1, littleEndian);
	}
	static toCharArray(str) {
		let len = str.length;
		let ca = new Array(len);
		for (let i = 0; i < len; i++)
			ca[i] = str.charCodeAt(i);
		return ca;
	}
	static toHexString(src, size = 1, prefix) {
		if (prefix === undefined) {
			prefix = "0x";
		}
		let dst = prefix;
		for (let p = 0; p < size; p++) {
			dst = dst.concat(toHexString0((src >> (BYTE_SIZE * (size - 1 - p))) & 0xff));
		}
		return dst;
	}
}
Utils.INT_32_SIZE = INT_32_SIZE;
Utils.INT_16_SIZE = INT_16_SIZE;
Utils.INT_8_SIZE = INT_8_SIZE;

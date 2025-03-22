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
 */


/*
	uuid
*/

// Compliant with UUIDv1 and UUIDv4 for RFC 9562 (supercedes RFC 5122) https://datatracker.ietf.org/doc/html/rfc9562 

import Net from "net";

function UUID() {
	let time = new Date().getTime();
	if (time < 1542410693111) {
		// trace("Time not set. Generating random UUID.\n");
		return UUIDv4();
	} else {
		// trace("Generating time-based UUID.\n");
		return UUIDv1(time);
	}
}
// Version 1 (Time-Based) UUID
function UUIDv1(time) {
    return [
        hex((time & 0xf) << 24, 1).substring(0, 1) +
            hex(Math.random() * 0xfffffff, 7), // add random number since we don't have 100-nanosecond precision
        hex((time & 0xffff0) >> 4, 4),
        "1" + hex(time & (0xfff00000 >> 20), 3),
        hex(Math.random() * 0x3fff | 0x8000, 4),
        Net.get('MAC')?.split(':').join('') ??
            hex((Math.random() * 0xffffffff) | 0x02000000, 8) +
                hex(Math.random() * 0xffff, 4),
    ]
        .join('-')
        .toUpperCase();
}

// Version 4 (Random) UUID
function UUIDv4() {
    return [
        hex(Math.random() * 0xffffffff, 8),
        hex(Math.random() * 0xffff, 4),
        '4' + hex(Math.random() * 0xfff, 3),
        hex(Math.random() * 0x3fff | 0x8000, 4),
        hex(Math.random() * 0xffffffff, 8) + hex(Math.random() * 0xffff, 4),
    ]
        .join('-')
        .toUpperCase();
}

function hex(value, count) {
    if (value < 0) {
        value = -value;
    }
    value = (value | 0).toString(16);
    return value.padStart(count, '0').slice(-count);
}

export default UUID;
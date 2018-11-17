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

// https://tools.ietf.org/html/rfc4122

import Net from "net";

function UUID() {
	let time = new Date().getTime();
	if (time < 1542410693111) {
		trace("Time not set. Generating random UUID.\n");
		return UUIDv4();
	} else {
		trace("Generating time-based UUID.\n");
		return UUIDv1(time);
	}
}

// Version 1 (Time-Based) UUID
function UUIDv1(time) {
	return [
		hex((time & 0xF) << 24, 1).substring(0, 1) + hex(Math.random() * 268435455, 7),		// add random number since we don't have 100-nanosecond precision
		hex((time & 0xFFFF0) >> 4, 4),
		hex((time & 0xFFF00000 >> 20) | 0x1000, 4),
		hex((Math.random() * 32767), 4),
		Net.get("MAC").split(":").join("")
	].join("-").toUpperCase();
}

// Version 4 (Random) UUID
function UUIDv4() {
	return [
		hex(Math.random() * 4294967295, 8),
		hex((Math.random() * 32767), 4),
		"4" + hex((Math.random() * 4095), 3),
		hex(0b10000000 + Math.random() * 0b111111) + hex((Math.random() * 255), 2),
		hex(Math.random() * 4294967295, 8) + hex((Math.random() * 32767), 4),
	].join("-").toUpperCase();
}


function hex(value, count)
{
	if (value < 0) {
		value = -value;
	}
	value = value.toString(16);
	return value.padStart(count, "0");
}

export default UUID;

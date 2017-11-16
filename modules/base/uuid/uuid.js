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

import Net from "net";

function UUID() {
//@@ need a real time for this to be correct
	return [
				hex(Math.random() * 10000000, 8),
				hex(Math.random() * 32767, 4),
				hex(Math.random() * 32767, 4),
				hex((Math.random() * 32767) | 0x8000, 4),
				Net.get("MAC").split(":").join("")
			].join("-").toUpperCase();
}

function hex(value, count)
{
	if (value < 0) value = -value;
	value = value.toString(16);
	return value.padStart(count, "0");
}

export default UUID;

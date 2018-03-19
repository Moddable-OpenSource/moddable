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
	LPM013M126A SPI Display
*/

export default class LPM013M126A @ "xs_LPM013M126A_destructor" {
	constructor(dictionary) @ "xs_LPM013M126A";

	begin(x, y, width, height) @ "xs_lpm013m126a_begin";
	send(pixels, offset, count) @  "xs_lpm013m126a_send";
	end() @  "xs_lpm013m126a_end";

	adaptInvalid() @ "xs_lpm013m126a_adaptInvalid";

	continue() {
	}

	pixelsToBytes(count) {
		return count;
	}

	get c_dispatch() @ "xs_lpm013m126a_get_c_dispatch";
	get async() {return false;}

	get pixelFormat() @ "xs_lpm013m126a_get_pixelFormat";
	get width() @ "xs_lpm013m126a_get_width";
	get height() @ "xs_lpm013m126a_get_height";
}

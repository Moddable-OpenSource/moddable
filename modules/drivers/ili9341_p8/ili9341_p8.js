/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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
	MIPI - 8-bit parallel
*/

export default class ILI9341 @ "xs_ILI9341p8_destructor" {
	constructor(options) @ "xs_ILI9341p8";

	begin(x, y, width, height) @ "xs_ILI9341p8_begin";
	send(pixels, offset, count) @ "xs_ILI9341p8_send";
	end() @  "xs_ILI9341p8_end";

	adaptInvalid() {}
	continue() {}

	pixelsToBytes(count) @ "xs_ILI9341p8_pixelsToBytes";

	get pixelFormat() @ "xs_ILI9341p8_get_pixelFormat";
	get width() @ "xs_ILI9341p8_get_width";
	get height() @ "xs_ILI9341p8_get_height";
	get async() {return true;}

	get c_dispatch() @ "xs_ILI9341p8_get_c_dispatch";

	// driver specific
	command(id, data) @ "xs_ILI9341p8_command";

	close() @ "xs_ILI9341p8_close";
}

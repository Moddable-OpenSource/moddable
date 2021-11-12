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
	ili9341 and ili9163c SPI display controller
*/

export default class ILI9341 @ "xs_ILI9341_destructor" {
	constructor(dictionary) @ "xs_ILI9341";

	begin(x, y, width, height) @ "xs_ILI9341_begin";
	send(pixels, offset, count) @ "xs_ILI9341_send";
	end() @  "xs_ILI9341_end";

	adaptInvalid() @ "xs_ILI9341_adaptInvalid";
	continue() {}

	pixelsToBytes(count) @ "xs_ILI9341_pixelsToBytes";

	get pixelFormat() @ "xs_ILI9341_get_pixelFormat";
	get width() @ "xs_ILI9341_get_width";
	get height() @ "xs_ILI9341_get_height";
	get async() {return true;}

	get clut() @ "xs_ILI9341_get_clut";
	set clut() @ "xs_ILI9341_set_clut";

	get rotation() @ "xs_ILI9341_get_rotation";
	set rotation() @ "xs_ILI9341_set_rotation";

	get c_dispatch() @ "xs_ILI9341_get_c_dispatch";

	// driver specific
	command(id, data) @ "xs_ILI9341_command";

	close() @ "xs_ILI9341_close";
}

Object.freeze(ILI9341.prototype);

/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
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

export default class ILI9341 extends Native("xs_ILI9341_destructor") {
	constructor(dictionary) { super(); native("xs_ILI9341").call(this, dictionary); }

	begin(x, y, width, height) { return native("xs_ILI9341_begin").call(this, x, y, width, height); }
	send(pixels, offset, count) { return native("xs_ILI9341_send").call(this, pixels, offset, count); }
	end() { return native("xs_ILI9341_end").call(this); }

	adaptInvalid(r) { return native("xs_ILI9341_adaptInvalid").call(this, r); }
	continue() {}

	pixelsToBytes(count) { return native("xs_ILI9341_pixelsToBytes").call(this, count); }

	get pixelFormat() { return native("xs_ILI9341_get_pixelFormat").call(this); }
	get width() { return native("xs_ILI9341_get_width").call(this); }
	get height() { return native("xs_ILI9341_get_height").call(this); }
	get async() {return true;}

	get clut() { return native("xs_ILI9341_get_clut").call(this); }
	set clut(value) { native("xs_ILI9341_set_clut").call(this, value); }

	get rotation() { return native("xs_ILI9341_get_rotation").call(this); }
	set rotation(value) { native("xs_ILI9341_set_rotation").call(this, value); }

	get c_dispatch() { return native("xs_ILI9341_get_c_dispatch").call(this); }

	// driver specific
	command(id, data) { return native("xs_ILI9341_command").call(this, id, data); }

	close() { return native("xs_ILI9341_close").call(this); }
}


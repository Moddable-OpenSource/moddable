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
	ssd1306 SPI display controller
*/

import Bitmap from "commodetto/Bitmap";

export default class SSD1306 @ "xs_SSD1306_destructor" {
	constructor(dictionary) @ "xs_SSD1306";

	begin(x, y, width, height) @ "xs_SSD1306_begin";
	send(pixels, offset, count) @  "xs_SSD1306_send";
	end() @  "xs_SSD1306_end";

	continue() {
		// empty implementation overrides PixelOut.continue which throws		//@@ check this
	}
	pixelsToBytes(count) {
		return count;
	}

	get pixelFormat() {return Bitmap.Gray256;}
	get width() @ "xs_SSD1306_width";
	get height() @ "xs_SSD1306_height";

	get c_dispatch() @ "xs_ssd1306_get_c_dispatch";

	adaptInvalid(r) {
		r.x = 0;
		r.y = 0;
		r.w = this.width;
		r.h = this.height;
	}
}

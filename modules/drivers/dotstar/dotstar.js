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
	Adafruit DotStar SPI display controller
*/

import Bitmap from "commodetto/Bitmap";

export default class DotStar @ "xs_DotStar_destructor" {
	constructor(dictionary) @ "xs_DotStar";

	begin(x, y, width, height) @ "xs_DotStar_begin";
	send(pixels, offset, count) @ "xs_DotStar_send";
	end() @  "xs_DotStar_end";

	adaptInvalid() @ "xs_DotStar_adaptInvalid";
	continue() {
		// empty implementation overrides PixelOut.continue which throws
	}

	pixelsToBytes(count) @ "xs_DotStar_pixelsToBytes";

	get pixelFormat() @ "xs_DotStar_get_pixelFormat";
	get width() @ "xs_DotStar_get_width";
	get height() @ "xs_DotStar_get_height";
	get async() {return false;}

	get c_dispatch() @ "xs_DotStar_get_c_dispatch";
}

Object.freeze(DotStar.prototype);

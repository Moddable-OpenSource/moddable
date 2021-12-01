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


import Bitmap from "commodetto/Bitmap";

export default class ColorCellOut @ "xs_colorcell_destructor" /* extends PixelsOut */ {
	constructor(dictionary) @ "xs_colorcell";
	begin(x, y, width, height, dictionary) @ "xs_colorcell_begin";
	send(pixels, offsetIn, count) @ "xs_colorcell_send"
	end() @ "xs_colorcell_end"
	pixelsToBytes(count) {
		return count * 2;		// assumes 16-bit pixels
	}

	// private
	build(width, height, format, pixels) {
		this.bitmap = new Bitmap(width, height, format, pixels, 0);
	}
}

Object.freeze(ColorCellOut.prototype);

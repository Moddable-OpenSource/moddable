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
	Commodetto Buffer in Memory
		
		implements PixelOut to write pixels to an ArrayBuffer
*/

import Bitmap from "commodetto/Bitmap";

export default class BufferOut @ "xs_BufferOut_destructor"{
	constructor(dictionary) {
		this.init(dictionary.width, dictionary.height, dictionary.pixelFormat, dictionary.buffer);
		this.bitmap = new Bitmap(dictionary.width, dictionary.height, dictionary.pixelFormat, this.buffer, 0);
	}
	init(width, height, pixelFormat) @ "xs_BufferOut_init"
	begin(x, y, width, height)  @ "xs_BufferOut_begin"
	send(pixels, offsetIn, count) @ "xs_BufferOut_send"
	end() {
	}
	continue() {
		// empty implementation overrides PixelOut.continue which throws
	}
	pixelsToBytes(count) @ "xs_BufferOut_pixelsToBytes"
	get width() @ "xs_BufferOut_getWidth"
	get height() @ "xs_BufferOut_getHeight"
	get pixelFormat() @ "xs_BufferOut_getPixelFormat"
}

Object.freeze(BufferOut.prototype);

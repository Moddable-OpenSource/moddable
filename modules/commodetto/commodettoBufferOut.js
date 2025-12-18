/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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

export default class BufferOut extends Native("xs_BufferOut_destructor"){
	constructor(dictionary) {
		super();
		native("xs_BufferOut_init").call(this, dictionary.width, dictionary.height, dictionary.pixelFormat, dictionary.buffer);
		this.bitmap = new Bitmap(dictionary.width, dictionary.height, dictionary.pixelFormat, this.buffer, 0);
	}
	begin(x, y, width, height) { return native("xs_BufferOut_begin").call(this, x, y, width, height); }
	send(pixels, offsetIn, count) { return native("xs_BufferOut_send").call(this, pixels, offsetIn, count); }
	end() {
	}
	continue() {
		// empty implementation overrides PixelOut.continue which throws
	}
	pixelsToBytes(count) { return native("xs_BufferOut_pixelsToBytes").call(this, count); }
	get width() { return native("xs_BufferOut_getWidth").call(this); }
	get height() { return native("xs_BufferOut_getHeight").call(this); }
	get pixelFormat() { return native("xs_BufferOut_getPixelFormat").call(this); }
}

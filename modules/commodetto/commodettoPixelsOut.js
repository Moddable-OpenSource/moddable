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
	Commodetto pixel sink
	
		Receives a stream of pixels
*/

import Bitmap from "commodetto/Bitmap";

export default class PixelsOut {
	/*
		Dictonary contains:
			width and height are in pixels
			pixelFormat is a string
			orientation is 0, 90, 180, 270 (not suppported on all displays)
	*/
	constructor(dictionary) {
		this.w = dictionary.width;
		this.h = dictionary.height;
		this.format = dictionary.pixelFormat;
	}
	/*
		before any pixels are sent
		area of display to update.
	*/
	begin(x, y, width, height) {
	}

	/*
		after the last pixel has been sent
	*/
	end() {
	}
	/*
		in-between a sequence of begin/end within a single frame
		subclass must override to allow
	*/
	continue() {
		throw new Error("continue not supported for this PixelOut")
	}
	/*
		send block of pixels (contained in ArrayBuffer) to display
		optional offset and count indicate the part of the pixels ArrayBuffer to use
		multiple calls to send may be required to send a full frame of pixels
	*/
	send(pixels, offset = 0, count = pixels.byteLength - offset) {
	}

	/*
		returns width and height of display in pixels
	*/
	get width() {
		return this.w;
	}
	get height() {
		return this.h;
	}
	/*
		returns format of pixel in use
	*/
	get pixelFormat() {
		return this.format;
	}
	/*
		returns byteCount of count pixels
	*/
	pixelsToBytes(count) {
		return ((count * Bitmap.depth(this.format)) + 7) >> 3;
	}

	/*
		Optional
	*/
	adaptInvalid(rectangle) {}

	get async() {return false;}

	get frameBuffer() {return false;}

	get clut() {return null;}
	set clut(clut) {throw new Error("set clut unsupported");}

	get c_dispatch() {return null;}
}

Object.freeze(PixelsOut);

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
	Commodetto JPEG reader
		
		Pull model - returns block of pixels on each call to read
		Uses picojpeg decoder

	To do:


*/

import Bitmap from "Bitmap";

export default class JPEG @ "xs_JPEG_destructor" {
	constructor(buffer) @ "xs_JPEG_constructor"
	read() @ "xs_JPEG_read"

	initialize(pixelFormat) {
		this.bitmap = new Bitmap(0, 0, pixelFormat, this.pixels, 0);
		return this.bitmap;
	}

	static decompress(data, dictionary) {
		let BufferOut = require("BufferOut");
		let Poco = require("Poco");
		let jpeg = new JPEG(data, dictionary);

		let offscreen = new BufferOut({width: jpeg.width, height: jpeg.height, pixelFormat: jpeg.bitmap.format});
		let render = new Poco(offscreen);

		while (true) {
			let block = jpeg.read();
			if (!block)
				break;

			render.begin(block.x, block.y, block.width, block.height);
				render.drawBitmap(block, block.x, block.y);
			render.end();
		}

		return offscreen.bitmap;
	}
}

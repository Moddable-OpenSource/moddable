/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

import JPEG from "commodetto/readJPEG";
import Poco from "commodetto/Poco";
import BufferOut from "commodetto/BufferOut";

export default function decompress(data, dictionary) {
	let jpeg = new JPEG(data, dictionary);

	let offscreen = new BufferOut({width: jpeg.width, height: jpeg.height, pixelFormat: jpeg.bitmap.pixelFormat});
	let render = new Poco(offscreen);

	while (jpeg.ready) {
		let block = jpeg.read();
		render.begin(block.x, block.y, block.width, block.height);
		render.drawBitmap(block, block.x, block.y);
		render.end();
	}

	return offscreen.bitmap;
}

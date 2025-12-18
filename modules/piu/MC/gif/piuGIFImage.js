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

import Bitmap from "commodetto/Bitmap";
import ReadGIF from "commodetto/ReadGIF";
import Resource from "Resource";

const gifImage = {
	__proto__: Content.prototype,
	_create($, it) { return native("PiuGIFImage_create").call(this, $, it); },

	load(path, colorized) {
		const gif = new ReadGIF(("string" === typeof path) ? new Resource(path) : path, colorized ? {pixelFormat: Bitmap.Gray16} : {});
		this.duration = gif.duration;
		this.time = 0;
		return gif;
	},
	unload(gif) {
		gif.close();
	},
};
export const GIFImage = Template(gifImage);
Object.freeze(gifImage);
globalThis.GIFImage = GIFImage;
export default GIFImage;
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
	Commodetto Poco renderer
	
		Renders 565 pixels
		rectangle
		bitmap - unscaled
		text
		etc
*/

// import Render from "Render"
// class Poco extends Render

export default class Poco extends Native("xs_poco_destructor") {
	constructor(pixelsOut, options = {}) {
		super();
		this.pixelsOut = pixelsOut;

		let pixels = pixelsOut.pixels?.(options.pixels)
		if (!pixels) {
			pixels = pixelsOut.width << 1;									// default is enough for double buffering at full width to allow async pixel transmission
			if (options.pixels >= pixelsOut.width)							// caller can request larger or smaller (down to a single scan line)
				pixels = options.pixels;
		}

		build.call(this,
					pixelsOut.width,
					pixelsOut.height,
					pixelsOut.pixelsToBytes(pixels),
					pixelsOut.pixelFormat,
					options.displayListLength ?? 1024,
					options.rotation ?? 0,
					pixelsOut.async,
					pixelsOut.adaptInvalid,
					pixelsOut.c_dispatch,
					pixelsOut.frameBuffer,
					pixelsOut.clut);
	}
	close() { return native("xs_poco_close").call(this); }

	// for drawing calls, see commodettoPocoDraw.js

	// rectangle
	rectangle(x, y, width, height) {
		if (undefined !== height)
			return new Rectangle(x, y, width, height);
		return new Rectangle(0, 0, 0, 0);
	}
}

function build(width, height, byteLength, pixelFormat, displayListLength, rotation) { return native("xs_poco_build").call(this, width, height, byteLength, pixelFormat, displayListLength, rotation); }

class Rectangle extends Native("xs_rectangle_destructor") {
	constructor(x, y, w, h) { super(); native("xs_rectangle_constructor").call(this, x, y, w, h); }
	set(x, y, w, h) { return native("xs_rectangle_set").call(this, x, y, w, h); }
	get x() { return native("xs_rectangle_get_x").call(this); }
	get y() { return native("xs_rectangle_get_y").call(this); }
	get w() { return native("xs_rectangle_get_w").call(this); }
	get h() { return native("xs_rectangle_get_h").call(this); }
	set x(it) { native("xs_rectangle_set_x").call(this, it); }
	set y(it) { native("xs_rectangle_set_y").call(this, it); }
	set w(it) { native("xs_rectangle_set_w").call(this, it); }
	set h(it) { native("xs_rectangle_set_h").call(this, it); }

	// for Ecma-419 adaptInvalid
	get width() { return native("xs_rectangle_get_w").call(this); }
	get height() { return native("xs_rectangle_get_h").call(this); }
	set width(it) { native("xs_rectangle_set_w").call(this, it); }
	set height(it) { native("xs_rectangle_set_h").call(this, it); }
}

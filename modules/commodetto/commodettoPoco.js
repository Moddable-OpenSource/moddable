/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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

export default class Poco @ "xs_poco_destructor" {
	constructor(pixelsOut, options = {}) {
		this.pixelsOut = pixelsOut;

		let pixels = pixelsOut.width << 1;								// default is enough for double buffering at full width to allow async pixel transmission
		if (options.pixels >= pixelsOut.width)							// caller can request larger or smaller (down to a single scan line)
			pixels = options.pixels;

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
	close() @ "xs_poco_close"
	begin(x, y, width, height) @ "xs_poco_begin"
	end() @ "xs_poco_end"
	continue(x, y, w, h) {
		this.end(true);
		this.begin(x, y, w, h);
	}

	// clip and origin stacks
	clip(x, y, width, height) @ "xs_poco_clip"
	origin(x, y) @ "xs_poco_origin"

	// rendering calls
	makeColor(r, g, b) @ "xs_poco_makeColor"
	fillRectangle(color, x, y, width, height) @ "xs_poco_fillRectangle"
	blendRectangle(color, blend, x, y, width, height) @ "xs_poco_blendRectangle"
	drawPixel(color, x, y) @ "xs_poco_drawPixel"
	drawBitmap(bits, x, y, sx, sy, sw, sh) @ "xs_poco_drawBitmap"
	drawMonochrome(monochrome, fore, back, x, y, sx, sy, sw, sh) @ "xs_poco_drawMonochrome"
	drawGray(bits, color, x, y, sx, sy, sw, sh, blend) @ "xs_poco_drawGray"
	drawMasked(bits, x, y, sx, sy, sw, sh, mask, mask_sx, mask_sy, blend) @ "xs_poco_drawMasked"
	fillPattern(bits, x, y, w, h, sx, sy, sw, sh) @ "xs_poco_fillPattern"

	drawFrame(frame, stream, x, y) @ "xs_poco_drawFrame"

	drawText(text, font, color, x, y) @ "xs_poco_drawText"

	// metrics
	getTextWidth(text, font) @ "xs_poco_getTextWidth"

	// invalidate area
	adaptInvalid(rectangle) @ "xs_poco_adaptInvalid";

	get width() @ "xs_poco_get_width";
	get height() @ "xs_poco_get_height";

	// rectangle
	rectangle(x, y, width, height) {
		if (undefined !== height)
			return new Rectangle(x, y, width, height);
		return new Rectangle(0, 0, 0, 0);
	}

}

function build(width, height, byteLength, pixelFormat, displayListLength, rotation) @ "xs_poco_build";

class Rectangle @ "xs_rectangle_destructor" {
	constructor(x, y, w, h) @ "xs_rectangle_constructor"
	set(x, y, w, h) @ "xs_rectangle_set"
	get x() @ "xs_rectangle_get_x"
	get y() @ "xs_rectangle_get_y"
	get w() @ "xs_rectangle_get_w"
	get h() @ "xs_rectangle_get_h"
	set x() @ "xs_rectangle_set_x"
	set y() @ "xs_rectangle_set_y"
	set w() @ "xs_rectangle_set_w"
	set h() @ "xs_rectangle_set_h"

	// for Ecma-419 adaptInvalid
	get width() @ "xs_rectangle_get_w"
	get height() @ "xs_rectangle_get_h"
	set width() @ "xs_rectangle_set_w"
	set height() @ "xs_rectangle_set_h"
}


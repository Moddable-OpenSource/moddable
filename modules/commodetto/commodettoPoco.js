/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

import Poco from "commodetto/PocoCore";

Poco.prototype.begin = function (x, y, width, height) @ "xs_poco_begin";
Poco.prototype.end = function () @ "xs_poco_end";
Poco.prototype.continue = function (x, y, w, h) {
	this.end(true);
	this.begin(x, y, w, h);
}

// // clip and origin stacks
Poco.prototype.clip = function (x, y, width, height) @ "xs_poco_clip";
Poco.prototype.origin = function(x, y) @ "xs_poco_origin";

// // rendering calls
Poco.prototype.makeColor = function (r, g, b) @ "xs_poco_makeColor";
Poco.prototype.fillRectangle = function(color, x, y, width, height) @ "xs_poco_fillRectangle";
Poco.prototype.blendRectangle = function(color, blend, x, y, width, height) @ "xs_poco_blendRectangle";
Poco.prototype.drawPixel = function(color, x, y) @ "xs_poco_drawPixel";
Poco.prototype.drawBitmap = function(bits, x, y, sx, sy, sw, sh) @ "xs_poco_drawBitmap";
Poco.prototype.drawMonochrome = function(monochrome, fore, back, x, y, sx, sy, sw, sh) @ "xs_poco_drawMonochrome";
Poco.prototype.drawGray = function(bits, color, x, y, sx, sy, sw, sh, blend) @ "xs_poco_drawGray";
Poco.prototype.drawMasked = function(bits, x, y, sx, sy, sw, sh, mask, mask_sx, mask_sy, blend) @ "xs_poco_drawMasked";
Poco.prototype.fillPattern = function(bits, x, y, w, h, sx, sy, sw, sh) @ "xs_poco_fillPattern";

Poco.prototype.drawFrame = function(frame, stream, x, y) @ "xs_poco_drawFrame";

Poco.prototype.drawText = function(text, font, color, x, y) @ "xs_poco_drawText";

// // metrics
Poco.prototype.getTextWidth = function(text, font) @ "xs_poco_getTextWidth";

// // invalidate area
Poco.prototype.adaptInvalid = function(rectangle) @ "xs_poco_adaptInvalid";

Object.defineProperty(Poco.prototype, "width", {
	get: function() @ "xs_poco_get_width"
});
Object.defineProperty(Poco.prototype, "height", {
	get: function() @ "xs_poco_get_height",
});

export default Poco;

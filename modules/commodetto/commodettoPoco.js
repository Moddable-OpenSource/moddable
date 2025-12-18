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

import Poco from "commodetto/PocoCore";

Poco.prototype.begin = function (x, y, width, height) { return native("xs_poco_begin").call(this, x, y, width, height); };
Poco.prototype.end = function () { return native("xs_poco_end").call(this); };
Poco.prototype.continue = function (x, y, w, h) {
	this.end(true);
	this.begin(x, y, w, h);
}

// // clip and origin stacks
Poco.prototype.clip = function (x, y, width, height) { return native("xs_poco_clip").call(this, x, y, width, height); };
Poco.prototype.origin = function(x, y) { return native("xs_poco_origin").call(this, x, y); };

// // rendering calls
Poco.prototype.makeColor = function (r, g, b) { return native("xs_poco_makeColor").call(this, r, g, b); };
Poco.prototype.fillRectangle = function(color, x, y, width, height) { return native("xs_poco_fillRectangle").call(this, color, x, y, width, height); };
Poco.prototype.blendRectangle = function(color, blend, x, y, width, height) { return native("xs_poco_blendRectangle").call(this, color, blend, x, y, width, height); };
Poco.prototype.drawPixel = function(color, x, y) { return native("xs_poco_drawPixel").call(this, color, x, y); };
Poco.prototype.drawBitmap = function(bits, x, y, sx, sy, sw, sh) { return native("xs_poco_drawBitmap").call(this, bits, x, y, sx, sy, sw, sh); };
Poco.prototype.drawMonochrome = function(monochrome, fore, back, x, y, sx, sy, sw, sh) { return native("xs_poco_drawMonochrome").call(this, monochrome, fore, back, x, y, sx, sy, sw, sh); };
Poco.prototype.drawGray = function(bits, color, x, y, sx, sy, sw, sh, blend) { return native("xs_poco_drawGray").call(this, bits, color, x, y, sx, sy, sw, sh, blend); };
Poco.prototype.drawMasked = function(bits, x, y, sx, sy, sw, sh, mask, mask_sx, mask_sy, blend) { return native("xs_poco_drawMasked").call(this, bits, x, y, sx, sy, sw, sh, mask, mask_sx, mask_sy, blend); };
Poco.prototype.fillPattern = function(bits, x, y, w, h, sx, sy, sw, sh) { return native("xs_poco_fillPattern").call(this, bits, x, y, w, h, sx, sy, sw, sh); };

Poco.prototype.drawFrame = function(frame, stream, x, y) { return native("xs_poco_drawFrame").call(this, frame, stream, x, y); };

Poco.prototype.drawText = function(text, font, color, x, y) { return native("xs_poco_drawText").call(this, text, font, color, x, y); };

// // metrics
Poco.prototype.getTextWidth = function(text, font) { return native("xs_poco_getTextWidth").call(this, text, font); };

// // invalidate area
Poco.prototype.adaptInvalid = function(rectangle) { return native("xs_poco_adaptInvalid").call(this, rectangle); };

Object.defineProperty(Poco.prototype, "width", {
	get: function() { return native("xs_poco_get_width").call(this); }
});
Object.defineProperty(Poco.prototype, "height", {
	get: function() { return native("xs_poco_get_height").call(this); },
});

export default Poco;

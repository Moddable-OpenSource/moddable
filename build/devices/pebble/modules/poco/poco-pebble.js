/*
* Copyright (c) 2025  Moddable Tech, Inc.
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

import Poco from "commodetto/PocoCore";
import Bitmap from "commodetto/Bitmap"

Poco.prototype.Font = class @ "xs_pocopebble_Font_destructor" {
	constructor(name, size) @ "xs_pocopebble_Font"
}

Poco.prototype.getTextWidth = function(text, font) @ "xs_pocopebble_getTextWidth";
Poco.prototype.drawText = function(text, font, color, x, y) @ "xs_pocopebbble_drawText";
Poco.prototype.drawLine = function(x0, y0, x1, y1, color, width) @ "xs_pocopebbble_drawLine";
Poco.prototype.drawRoundRect = function(x0, y0, x1, y1, color, radius, corners) @ "xs_pocopebbble_drawRoundRect";
Poco.prototype.frameRoundRect = function(x0, y0, x1, y1, color, radius) @ "xs_pocopebbble_frameRoundRect";
Poco.prototype.drawCircle = function(color, x, y, r, from, to) @ "xs_pocopebbble_drawCircle";
Poco.prototype.drawDCI = function(dci, x, y) @ "xs_pocopebbble_drawDCI";

function build(id) @ "xs_pebblebitmap_build"

Poco.PebbleBitmap = class extends Bitmap {
	constructor(id) {
		const b = new Bitmap(0, 0, Bitmap.Pebble);
		return build(b, id);
	}
}

function clone() @ "xs_pebbledci_clone"
Poco.PebbleDrawCommandImage = class @ "xs_pebbledci_destructor" {
	constructor(id) @ "xs_pebbledci"
	get width() @ "xs_pebbledci_get_width"
	get height() @ "xs_pebbledci_get_height"
	clone() {
		return clone.call(this, Poco.PebbleDrawCommandList.prototype);
	}
}

Poco.PebbleDrawCommandSequence = class @ "xs_pebbledcs_destructor" {
	constructor(id) @ "xs_pebbledcs"
	get width() @ "xs_pebbledcs_get_width"
	get height() @ "xs_pebbledcs_get_height"
	get duration() @ "xs_pebbledcs_get_duration"
	get frameDuration() @ "xs_pebbledcs_get_frameDuration"
	clone() {
		return clone.call(this, Poco.PebbleDrawCommandList.prototype);
	}
}

Poco.PebbleDrawCommandList = class @ "xs_pebbledcl_destructor" {
	scale(x, y) @ "xs_pebbledcl_scale"
	rotate(angle, cx, cy) @ "xs_pebbledcl_rotate"
}

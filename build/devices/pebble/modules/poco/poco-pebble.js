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

function build(id) @ "xs_pebblebitmap_build"

Poco.PebbleBitmap = class extends Bitmap {
	constructor(id) {
		const b = new Bitmap(0, 0, Bitmap.Pebble);
		build(b, id);
		return b;
	}
}

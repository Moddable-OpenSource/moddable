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

let cache;

Poco.prototype.Font = class extends Native("xs_pocopebble_Font_destructor") {
	constructor(name, size) { super(); native("xs_pocopebble_Font").call(this, name, size); }
}

Poco.prototype.getTextWidth = function(text, font) { return native("xs_pocopebble_getTextWidth").call(this, text, font); };
Poco.prototype.drawText = function(text, font, color, x, y) { return native("xs_pocopebbble_drawText").call(this, text, font, color, x, y); };
Poco.prototype.drawLine = function(x0, y0, x1, y1, color, width) { return native("xs_pocopebbble_drawLine").call(this, x0, y0, x1, y1, color, width); };
Poco.prototype.drawRoundRect = function(x0, y0, x1, y1, color, radius, corners) { return native("xs_pocopebbble_drawRoundRect").call(this, x0, y0, x1, y1, color, radius, corners); };
Poco.prototype.frameRoundRect = function(x0, y0, x1, y1, color, radius) { return native("xs_pocopebbble_frameRoundRect").call(this, x0, y0, x1, y1, color, radius); };
Poco.prototype.drawCircle = function(color, x, y, r, from, to) { return native("xs_pocopebbble_drawCircle").call(this, color, x, y, r, from, to); };
Poco.prototype.drawDCI = function(dci, x, y) { return native("xs_pocopebbble_drawDCI").call(this, dci, x, y); };

function build(id) { return native("xs_pebblebitmap_build").call(this, id); }

Poco.PebbleBitmap = class extends Bitmap {
	constructor(id) {
		cache ??= [];
		id = Number(id);
		let b = cache[id];
		if (b) {
			b = b.deref();
			if (b) return b;
			delete cache[id];
		}
		b = build(new Bitmap(0, 0, Bitmap.Pebble), id);
		cache[id] = new WeakRef(b);
		return b;
	}
}

Poco.PebbleDrawCommandImage = class extends Native("xs_pebbledci_destructor") {
	constructor(id) {
		super();

		cache ??= [];
		id = Number(id);
		let b = cache[id];
		if (b) {
			b = b.deref();
			if (b) return b;
			delete cache[id];
		}
		native("xs_pebbledci").call(this, id);
		cache[id] = new WeakRef(this);
	}
	get width() { return native("xs_pebbledci_get_width").call(this); }
	get height() { return native("xs_pebbledci_get_height").call(this); }
	clone() {
		return native("xs_pebbledci_clone").call(this, Poco.PebbleDrawCommandList.prototype);
	}
}

Poco.PebbleDrawCommandSequence = class extends Native("xs_pebbledcs_destructor") {
	constructor(id) { super(); native("xs_pebbledcs").call(this, id); }
	get width() { return native("xs_pebbledcs_get_width").call(this); }
	get height() { return native("xs_pebbledcs_get_height").call(this); }
	get duration() { return native("xs_pebbledcs_get_duration").call(this); }
	get frameDuration() { return native("xs_pebbledcs_get_frameDuration").call(this); }
	clone() {
		return native("xs_pebbledci_clone").call(this, Poco.PebbleDrawCommandList.prototype);
	}
}

Poco.PebbleDrawCommandList = class extends Native("xs_pebbledcl_destructor") {
	scale(x, y) { return native("xs_pebbledcl_scale").call(this, x, y); }
	rotate(angle, cx, cy) { return native("xs_pebbledcl_rotate").call(this, angle, cx, cy); }
}

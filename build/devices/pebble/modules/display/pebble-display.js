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

class PebbleDisplay extends Native("xs_pebbledisplay_destructor") {
	constructor(options) { super(); native("xs_pebbledisplay").call(this, options); };

	begin(x, y, width, height) { return native("xs_pebbledisplay_begin").call(this, x, y, width, height); };
	send(pixels, offset, count) { return native("xs_pebbledisplay_send").call(this, pixels, offset, count); };
	end() { return native("xs_pebbledisplay_end").call(this); };

	adaptInvalid() { return native("xs_pebbledisplay_adaptInvalid").call(this); };
	continue() {}

	pixelsToBytes(count) { return native("xs_pebbledisplay_pixelsToBytes").call(this, count); };

	get pixelFormat() { return native("xs_pebbledisplay_get_pixelFormat").call(this); };
	get width() { return native("xs_pebbledisplay_get_width").call(this); };
	get height() { return native("xs_pebbledisplay_get_height").call(this); };
	get async() {return false;}

	get c_dispatch() { return native("xs_pebbledisplay_get_c_dispatch").call(this); };

	close() { return native("xs_pebbledisplay_close").call(this); };

	get color() {return native("xs_pebbledisplay_color_get").call(this);}
	get round() {return native("xs_pebbledisplay_round_get").call(this);}
}
PebbleDisplay.prototype.frameBuffer = true

export default PebbleDisplay;
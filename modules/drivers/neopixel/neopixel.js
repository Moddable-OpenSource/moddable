/*
 * Copyright (c) 2018-2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 *
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
class NeoPixel extends Native("xs_neopixel_destructor") {
	constructor(dictionary) { super(); native("xs_neopixel").call(this, dictionary); }

	close() { return native("xs_neopixel_close").call(this); }
	update() { return native("xs_neopixel_update").call(this); }

	makeRGB(r, g, b, w) { return native("xs_neopixel_makeRGB").call(this, r, g, b, w); }
	makeHSB(h, s, b, w) { return native("xs_neopixel_makeHSB").call(this, h, s, b, w); }

	setPixel(index, color) { return native("xs_neopixel_setPixel").call(this, index, color); }
	fill(color, index, count) { return native("xs_neopixel_fill").call(this, color, index, count); }

	getPixel(index) { return native("xs_neopixel_getPixel").call(this, index); }

	set brightness(value) { native("xs_neopixel_brightness_set").call(this, value); }
	get brightness() { return native("xs_neopixel_brightness_get").call(this); }

	get length() { return native("xs_neopixel_length_get").call(this); }
	get byteLength() { return native("xs_neopixel_byteLength_get").call(this); }
}

export default NeoPixel;

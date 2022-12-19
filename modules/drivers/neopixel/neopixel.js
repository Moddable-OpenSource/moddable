/*
 * Copyright (c) 2018-2022  Moddable Tech, Inc.
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
class NeoPixel @ "xs_neopixel_destructor" {
	constructor(dictionary) @ "xs_neopixel";

	close() @ "xs_neopixel_close";
	update() @ "xs_neopixel_update";

	makeRGB(r, g, b) @ "xs_neopixel_makeRGB";	// r, g, b are 0 to 255
	makeHSB(h, s, b) @ "xs_neopixel_makeHSB";	// h is 0 to 359, s and b are 0 to 1000

	setPixel(index, color) @ "xs_neopixel_setPixel";
	fill(color, index, count) @ "xs_neopixel_fill";

	getPixel(index) @ "xs_neopixel_getPixel";

	set brightness(value) @ "xs_neopixel_brightness_set";
	get brightness() @ "xs_neopixel_brightness_get";

	get length() @ "xs_neopixel_length_get";
	get byteLength() @ "xs_neopixel_byteLength_get";
}
Object.freeze(NeoPixel.prototype);

export default NeoPixel;

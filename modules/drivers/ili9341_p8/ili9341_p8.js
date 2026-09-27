/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
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
	MIPI - 8-bit parallel
*/

// from data sheet
//const frameRate = Uint8Array.of(119, 111, 105, 99, 94, 90, 86, 82, 78, 75, 72, 69, 67, 64, 62, 60, 58, 57, 55, 53, 52, 50, 49, 48, 46, 45, 44, 43, 42, 41, 40, 30);

// from device measurements
const frameRate = Uint8Array.of(114,108,102,96,91,87,83,79,76,73,70,67,65,63,61,59,57,55,53,52,50,49,48,46,45,44,43,42,41,40,39,38);

export default class ILI9341 extends Native("xs_ILI9341p8_destructor") {
	constructor(options) { super(); native("xs_ILI9341p8").call(this, options); }

	begin(x, y, width, height) { return native("xs_ILI9341p8_begin").call(this, x, y, width, height); }
	send(pixels, offset, count) { return native("xs_ILI9341p8_send").call(this, pixels, offset, count); }
	end() { return native("xs_ILI9341p8_end").call(this); }

	adaptInvalid() {}
	continue() { return native("xs_ILI9341p8_continue").call(this); }

	pixelsToBytes(count) { return native("xs_ILI9341p8_pixelsToBytes").call(this, count); }

	get pixelFormat() { return native("xs_ILI9341p8_get_pixelFormat").call(this); }
	get width() { return native("xs_ILI9341p8_get_width").call(this); }
	get height() { return native("xs_ILI9341p8_get_height").call(this); }
	get async() {return true;}

	get c_dispatch() { return native("xs_ILI9341p8_get_c_dispatch").call(this); }

	// driver specific
	command(id, data) { return native("xs_ILI9341p8_command").call(this, id, data); }
	set frameRate(value) {
		let i = 0, length = frameRate.length - 1;
		for (; i < length; i++) {
			if (frameRate[i] <= value)
				break;
		}
		this._frameRate = frameRate[i];
		this.command(0xc6, Uint8Array.of(i))
	}
	get frameRate() {
		return this._frameRate ?? 50;
	}
	set syncFrames(value) { native("xs_ili9341p8_set_syncFrames").call(this, value); }
	get syncFrames() { return native("xs_ili9341p8_get_syncFrames").call(this); }

	close() { return native("xs_ILI9341p8_close").call(this); }
	
	pixels(value = 0) {
		const pixels = this.width << 5;		// 32 scan lines
		return (value > pixels) ? value : pixels;
	}
}

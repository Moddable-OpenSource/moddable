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
	LS013B4DN04 SPI Display
*/

export default class LS013B4DN04 extends Native("xs_ls013b4dn04_destructor") {
	constructor(dictionary) { super(); native("xs_LS013B4DN04").call(this, dictionary); }
	close() { return native("xs_LS013B4DN04_close").call(this); }

	begin(x, y, width, height) { return native("xs_ls013b4dn04_begin").call(this, x, y, width, height); }
	send(pixels, offset, count) { return native("xs_ls013b4dn04_send").call(this, pixels, offset, count); }
	end() { return native("xs_ls013b4dn04_end").call(this); }

	adaptInvalid(r) { return native("xs_ls013b4dn04_adaptInvalid").call(this, r); }
	continue() {}

	pixelsToBytes(count) { return count; }

	get pixelFormat() { return native("xs_ls013b4dn04_get_pixelFormat").call(this); }
	get width() { return native("xs_ls013b4dn04_get_width").call(this); }
	get height() { return native("xs_ls013b4dn04_get_height").call(this); }
	get async() {return false;}

	hold() { return native("xs_ls013b4dn04_hold").call(this); }
	clear() { return native("xs_ls013b4dn04_clear").call(this); }
	get dither() { return native("xs_ls013b4dn04_dither_get").call(this); }
	set dither(value) { native("xs_ls013b4dn04_dither_set").call(this, value); }

	get frameBuffer() { return native("xs_ls013b4dn04_frameBuffer_get").call(this); }
	get c_dispatch() { return native("xs_ls013b4dn04_get_c_dispatch").call(this); }

	get rotation() { return native("xs_ls013b4dn04_get_rotation").call(this); }
	set rotation(value) { native("xs_ls013b4dn04_set_rotation").call(this, value); }
}


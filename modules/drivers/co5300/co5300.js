/*
 * Copyright (c) 2024-2026  Moddable Tech, Inc.
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
	CO5300 QSPI AMOLED display driver
*/

export default class CO5300 extends Native("xs_co5300_destructor") {
	constructor(options) { super(); native("xs_co5300").call(this, options); }

	begin(x, y, width, height) { return native("xs_co5300_begin").call(this, x, y, width, height); }
	send(pixels, offset, count) { return native("xs_co5300_send").call(this, pixels, offset, count); }
	end() { return native("xs_co5300_end").call(this); }

	adaptInvalid() {}
	continue() { return native("xs_co5300_continue").call(this); }

	pixelsToBytes(count) { return native("xs_co5300_pixelsToBytes").call(this, count); }

	get pixelFormat() { return native("xs_co5300_get_pixelFormat").call(this); }
	get width() { return native("xs_co5300_get_width").call(this); }
	get height() { return native("xs_co5300_get_height").call(this); }
	get async() {return true;}
	get rotation() { return native("xs_co5300_get_rotation").call(this); }
	set rotation(value) { native("xs_co5300_set_rotation").call(this, value); }

	get c_dispatch() { return native("xs_co5300_get_c_dispatch").call(this); }

	command(id, data) { return native("xs_co5300_command").call(this, id, data); }
	set brightness(value) { native("xs_co5300_set_brightness").call(this, value); }
	get brightness() { return native("xs_co5300_get_brightness").call(this); }

	close() { return native("xs_co5300_close").call(this); }

	pixels(value = 0) {
		const pixels = this.width << 5;
		return (value > pixels) ? value : pixels;
	}
}

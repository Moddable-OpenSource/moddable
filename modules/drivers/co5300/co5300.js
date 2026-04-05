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

export default class CO5300 @ "xs_co5300_destructor" {
	constructor(options) @ "xs_co5300";

	begin(x, y, width, height) @ "xs_co5300_begin";
	send(pixels, offset, count) @ "xs_co5300_send";
	end() @ "xs_co5300_end";

	adaptInvalid() {}
	continue() @ "xs_co5300_continue";

	pixelsToBytes(count) @ "xs_co5300_pixelsToBytes";

	get pixelFormat() @ "xs_co5300_get_pixelFormat";
	get width() @ "xs_co5300_get_width";
	get height() @ "xs_co5300_get_height";
	get async() {return true;}

	get c_dispatch() @ "xs_co5300_get_c_dispatch";

	command(id, data) @ "xs_co5300_command";
	set brightness(value) @ "xs_co5300_set_brightness";
	get brightness() @ "xs_co5300_get_brightness";

	close() @ "xs_co5300_close";

	pixels(value = 0) {
		const pixels = this.width << 5;
		return (value > pixels) ? value : pixels;
	}
}

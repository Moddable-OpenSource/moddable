/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

import Bitmap from "commodetto/Bitmap";

export default class LS013B4DN04 @ "xs_ls013b4dn04_destructor" {
	constructor(dictionary) @ "xs_LS013B4DN04";
	close() @ "xs_LS013B4DN04_close";

	begin(x, y, width, height) @ "xs_ls013b4dn04_begin";
	send(pixels, offset, count) @  "xs_ls013b4dn04_send";
	end() @  "xs_ls013b4dn04_end";

	adaptInvalid() @ "xs_ls013b4dn04_adaptInvalid";
	continue() {}

	pixelsToBytes(count) { return count; }

	get pixelFormat() @ "xs_ls013b4dn04_get_pixelFormat";
	get width() @ "xs_ls013b4dn04_get_width";
	get height() @ "xs_ls013b4dn04_get_height";
	get async() {return false;}

	hold() @ "xs_ls013b4dn04_hold";
	clear() @ "xs_ls013b4dn04_clear";
	get dither() @ "xs_ls013b4dn04_dither_get";
	set dither(value) @ "xs_ls013b4dn04_dither_set";

	get frameBuffer() @ "xs_ls013b4dn04_frameBuffer_get";
	get c_dispatch() @ "xs_ls013b4dn04_get_c_dispatch";
}

Object.freeze(LS013B4DN04.prototype);

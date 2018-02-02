/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

	begin(x, y, width, height) @ "xs_ls013b4dn04_begin";
	send(pixels, offset, count) @  "xs_ls013b4dn04_send";
	end() @  "xs_ls013b4dn04_end";

	hold() @ "xs_ls013b4dn04_hold";
	clear() @ "xs_ls013b4dn04_clear";

	adaptInvalid() @ "xs_ls013b4dn04_adaptInvalid";

	continue() {
	}

	pixelsToBytes(count) {
		return count;
	}

	get async() {return false;}
	get pixelFormat() @ "xs_ls013b4dn04_get_pixelFormat";
	get width() @ "xs_ls013b4dn04_get_width";
	get height() @ "xs_ls013b4dn04_get_height";
	get c_dispatch() @ "xs_ls013b4dn04_get_c_dispatch";
}

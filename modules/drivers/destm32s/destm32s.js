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
	DESTM32S SPI ePaper display controller
*/

import Bitmap from "commodetto/Bitmap";

export default class DESTM32S @ "xs_destm32s_destructor" {
	constructor(dictionary) @ "xs_destm32s";

	begin(x, y, width, height) @ "xs_destm32s_begin";
	send(pixels, offset, count) @ "xs_destm32s_send";
	end() @  "xs_destm32s_end";

	adaptInvalid() @ "xs_destm32s_adaptInvalid";
	continue() {
		// empty implementation overrides PixelOut.continue which throws
	}

	pixelsToBytes(count) {
		return (count * Bitmap.depth(this.pixelFormat)) >> 3;
	}

	get pixelFormat() @ "xs_destm32s_get_pixelFormat";
	get width() @ "xs_destm32s_get_width";
	get height() @ "xs_destm32s_get_height";
	get async() {return false;}

	get c_dispatch() @ "xs_destm32s_get_c_dispatch";

	refresh() @ "xs_destm32s_refresh";
	configure(options) @ "xs_destm32s_configure";
}

Object.freeze(DESTM32S.prototype);

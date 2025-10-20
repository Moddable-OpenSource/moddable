/*
 * Copyright (c) 2021-2025  Moddable Tech, Inc.
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

class Dither extends Native("xs_dither_destructor") {
	constructor(options) { super(); native("xs_dither").call(this, options); }
	/*
		width,
		pixelFormat,
		algorithm: "atkinson"
	*/
	
	close() { return native("xs_dither_close").call(this); }
	reset() { return native("xs_dither_reset").call(this); }
	send(lines, src, offset, dst, o) { return native("xs_dither_send").call(this, lines, src, offset, dst, o); }
}

export default Dither;

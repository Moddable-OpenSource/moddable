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

class Digital @ "xs_digital_destructor" {
	static read(pin) @ "xs_digital_static_read";
	static write(pin, value) @ "xs_digital_static_write";

	constructor(port, pin, mode) @ "xs_digital";// or (pin, mode) where port is implicitly NULL
	close() @ "xs_digital_close";
	mode(mode) @ "xs_digital_mode";				// change pin mode
	read() @ "xs_digital_read";					// read pin
	write(value) @ "xs_digital_write";			// write pin
}
Digital.Input = 0;
Digital.InputPullUp = 1;
Digital.InputPullDown = 2;
Digital.InputPullUpDown = 3;
Digital.Output = 8;
Digital.OutputOpenDrain = 9;

Object.freeze(Digital.prototype);

export default Digital;

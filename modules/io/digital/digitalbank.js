/*
 * Copyright (c) 2019-2025  Moddable Tech, Inc.
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

class DigitalBank extends Native("xs_digitalbank_destructor") {
	constructor(dictionary) { super(); native("xs_digitalbank_constructor").call(this, dictionary); }
	close() { return native("xs_digitalbank_close").call(this); }
	read() { return native("xs_digitalbank_read").call(this); }
	write(value) { return native("xs_digitalbank_write").call(this, value); }

	get format() {
		return "number";
	}
	set format(value) {
		if ("number" !== value)
			throw new RangeError;
	}
}
DigitalBank.Input = 0;
DigitalBank.InputPullUp = 1;
DigitalBank.InputPullDown = 2;
DigitalBank.InputPullUpDown = 3;

DigitalBank.Output = 8;
DigitalBank.OutputOpenDrain = 9;

DigitalBank.ActiveLow = 16;

export default DigitalBank;

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
	pins
*/

export class Digital {
	static configure(pin, mode) @ "xs_digital_configure";
	static read(pin) @ "xs_digital_read";
	static write(pin, value) @ "xs_digital_write";
};

export class I2C @ "xs_i2c_destructor" {
	constructor(sda, scl, address) @ "xs_i2c";
	read(count, stop) @ "xs_i2c_read";
	write() @ "xs_i2c_write";
}

export class Analog {
	static read(pin) @ "xs_analog_read";
};

export class PWM {
	static write(pin) @ "xs_pwm_write";
};


// xsID_*

let
sda,
clock,
address,
mhz
;

export default {Digital, I2C, Analog, PWM};

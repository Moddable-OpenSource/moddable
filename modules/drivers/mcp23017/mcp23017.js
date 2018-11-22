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
	Microchip MCP23017 GPIO expander
	16 bit sets - portA is low byte, portB is high byte
	constructor dictionary:
		address:	i2c address
		hz:			i2c speed
		inputs:		16 bits: 1=input, 0=output
			(the following are for input channels only)
		pullup:		16 bits: 100k pullup resistor
							 0=disabled
							 1=enabled

	example: create an object "gpio" with all 8 portA bits as input, pulled up, active low (ie: switch wired from gpio to ground)
	let gpio = new MCP23017({address: 32, hz:17000000, inputs:0x00ff, pullup:0x00ff});

*/

export default class MCP23017 @ "xs_mcp23017_destructor" {
	constructor(dictionary) {
		global.sleepers.push(this.delete);
		this.construct(dictionary);
	};
	construct(dictionary) @ "xs_mcp23017";

	delete() {
		delete this;
	}

	configure(pin, mode) @ "xs_mcp23017_configure";
	configure_set(pin_set, mode) @ "xs_mcp23017_configure_set";

	read(pin) @ "xs_mcp23017_read";
	write(pin, value) @ "xs_mcp23017_write";
	read_set(pin_set) @ "xs_mcp23017_read_set";
	write_set(pin_set, value) @ "xs_mcp23017_write_set";
};

let hz, address, inputs, pullup

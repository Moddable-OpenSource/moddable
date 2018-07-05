/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

/*
	pins
*/

export class I2C @ "xs_i2c_destructor" {
	constructor(sda, scl, address) @ "xs_i2c";
	read(count, stop) @ "xs_i2c_read";
	write() @ "xs_i2c_write";
}

// xsID_*

let
sda,
clock,
address,
mhz
;

export default {I2C};

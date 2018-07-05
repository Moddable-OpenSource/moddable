/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

/*
	pins
*/

export class Digital {
	static configure(pin, mode) @ "xs_digital_configure";
	static read(pin) @ "xs_digital_read";
	static write(pin, value) @ "xs_digital_write";
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

export default {Digital, PWM};

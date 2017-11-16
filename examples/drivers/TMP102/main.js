/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import I2C from "pins/i2c";
import Timer from "timer";

let sensor = new I2C({address: 0x48});

Timer.set(id => {
	// SMB readWord - write address, read data: S Addr Wr [A] Comm [A] S Addr Rd [A] [DataLow] A [DataHigh] NA P
	sensor.write(0);			// set address 0
	let value = sensor.read(2);	// read two bytes

	// calculate degrees celsius from two data bytes
	value = (value[0] << 4) | (value[1] >> 4);
	if (value & 0x800) {
	    value -= 1;
	    value = ~value & 0xFFF;
        value = -value;
    }
    value *= 0.0625;

	trace(`Celsius temperature: ${value}\n`);
}, 333, true);

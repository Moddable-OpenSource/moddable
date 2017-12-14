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

/*
	SparkFun Triple Axis Magnetometer Breakout - SEN-10530

	HMC5883L - https://cdn.sparkfun.com/datasheets/Sensors/Magneto/HMC5883L-FDS.pdf
*/

import SMBus from "pins/smbus";
import Timer from "timer";

let sensor = new SMBus({sda: 5, scl: 4, address: 0x1E});

let id = sensor.readBlock(10, 3);			// ID
id = String.fromCharCode(id[0]) + String.fromCharCode(id[1]) + String.fromCharCode(id[2]);
if ("H43" != id)
	throw new Error("unable to verify magnometer id");

sensor.writeByte(2, 0);			// continuous measurement mode

Timer.set(id => {
	let result = sensor.readBlock(3, 6);
	trace(`x: ${toInt16(result[0], result[1])}, y: ${toInt16(result[2], result[3])}, z: ${toInt16(result[4], result[5])}\n`);
}, 250, true);

function toInt16(high, low) {
	let result = (high << 8) | low;
	if (result > 32767)
		result = result - 65536;
	return result;
}

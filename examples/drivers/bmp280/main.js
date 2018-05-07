/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

// Demonstrates reading the bmp280 Temperature/Humidity sensor
// that is built into the Thunderboard Sense.

import BMP280 from "bmp280";


export default function() {
	// fetch sensor data
	let pressTemp = new BMP280();
	let v = {};
	pressTemp.read(v);

	let f = Math.round(v.celcius * 1.8 + 32);
	let p = Math.round(v.millibar);

	trace(f + " degrees Fahrenheit\n");
	trace(p + " millibar\n");

	debugger;
}

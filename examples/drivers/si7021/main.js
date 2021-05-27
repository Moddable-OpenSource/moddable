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

// Demonstrates reading the si7021 Temperature/Humidity sensor
// that is built into many of the SiLabs dev kits.

import SI7021 from "si7021";

export default function() {
	// fetch sensor data
	let humidTemp = new SI7021();
	let v = {};
	humidTemp.read(v);

	let f = Math.round(v.celsius * 1.8 + 32);
	let h = Math.round(v.humidity);

	trace(f + " degrees Fahrenheit\n");
	trace(h + " % humidity\n");

	debugger;
}

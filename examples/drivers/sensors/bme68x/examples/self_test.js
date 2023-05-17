/*
 * Copyright (c) 2023 Moddable Tech, Inc.
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

import {BME68x} from "embedded:sensor/Barometer-Humidity-Temperature/BME68x"; 

export default function() {
	const bme = new BME68x({
		sensor: device.I2C.default
	});

	try {
		trace(`Self test start\n`);
		bme.selftest_check();
		trace(`Self test passed\n`);
	}
	catch (e) {
		trace(`Self test failed: ${e}\n`);
	}

	bme.close();
}

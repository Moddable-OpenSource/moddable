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

import Sensor from "embedded:sensor/Barometer-Humidity-Temperature/BME68x"; 
import Timer from "timer";

export default function() {
	const sensor = new Sensor({
		sensor: device.I2C.default
	});

	Timer.repeat(() => {
		const sample = sensor.sample();
		if (sample)
			trace(JSON.stringify(sample, undefined, 3), "\n");
	}, 250);
}

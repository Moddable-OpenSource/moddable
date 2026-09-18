/*
 * Copyright (c) 2026 Moddable Tech, Inc.
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

import BME280 from "embedded:sensor/Barometer-Humidity-Temperature/BME280";
import Timer from "timer";

const sensor = new BME280({
	sensor: device.I2C.default
});

Timer.repeat(() => {
	const sample = sensor.sample();

	trace(`Temperature: ${sample.thermometer.temperature.toFixed(2)} C\n`);
	trace(`Humidity: ${sample.hygrometer.humidity == null ? "n/a" : (sample.hygrometer.humidity * 100).toFixed(2)} %RH\n`);
	trace(`Pressure: ${sample.barometer.pressure.toFixed(0)} Pa\n`);
}, 2000);

/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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
 * NOTE: The AHT10 sensor uses address 0x38 which conflicts with the FT6206
 *          TouchScreen driver for Moddable One and Moddable Two
 */

import Humidity from "embedded:sensor/Humidity-Temperature/AHT10";
import Timer from "timer";

const humidity = new Humidity({ sensor: device.I2C.default });

Timer.repeat(() => {
	const sample = humidity.sample();

	trace(`Temperature: ${sample.thermometer.temperature.toFixed(2)} C `);
	trace(`Humidity: ${sample.hygrometer.humidity.toFixed(2)} %RH\n`);

}, 2000);


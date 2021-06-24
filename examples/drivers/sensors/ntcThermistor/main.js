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

import device from "embedded:provider/builtin";
import Thermistor from "embedded:sensor/Temperature/NTC_Thermistor";
import Timer from "timer";
import config from "mc/config";

const sensor = new Thermistor({
	sensor: device.Analog.default
});

sensor.configure({
	series_resistance: 10000,
	thermistor_resistance: 10000,
	beta: 3435,
	averaging: 8,
	pullup: true
});

function toF(c) {
	return (c * 9) / 5 + 32;
}

Timer.repeat(id => {
	let sample = sensor.sample().temperature;
	trace(`${sample.toFixed(2)} C -- ${toF(sample).toFixed(2)} F\n`);
}, 100);

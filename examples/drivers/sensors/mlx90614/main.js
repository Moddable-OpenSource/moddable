/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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

import Temperature from "embedded:sensor/Temperature/MLX90614";
import Timer from "timer";

const sensor = new Temperature({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

function toF(c) { return (((c * 9) / 5) + 32).toFixed(2); }

Timer.repeat(id => {
	let value = sensor.sample();
	trace(`temperature: ${toF(value.temperature)} F - ambient temperature: ${toF(value.ambientTemperature)} F\n`);
}, 100);


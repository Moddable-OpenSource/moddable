/*
 * Copyright (c) 2022 Moddable Tech, Inc.
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

import Timer from "timer";
import APDS9301 from "embedded:sensor/AmbientLight/APDS9301";

let sensor = new APDS9301({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

trace(`Sensor configuration is: ${JSON.stringify(sensor.configuration)}\n`);
trace(`Sensor identification is: ${JSON.stringify(sensor.identification)}\n`);

Timer.repeat(() => {
	const sample = sensor.sample();
	trace(`${JSON.stringify(sample)}\n`);
}, 1000);

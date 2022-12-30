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

import APDS9301 from "embedded:sensor/AmbientLight/APDS9301";
import config from "mc/config";

let sensor = new APDS9301({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	},
	alert: {
		pin: config.interrupt_pin,
		io: device.io.Digital
	},
	onAlert: () => {
		const sample = sensor.sample();
		trace(`Alert! ${JSON.stringify(sample)}\n`);
	}
});

sensor.configure({
	thresholdLow: 500,
	thresholdHigh: 15_000,
	thresholdPersistence: 4
});

trace(`Sensor configuration is: ${JSON.stringify(sensor.configuration)}\n`);
trace(`Sensor identification is: ${JSON.stringify(sensor.identification)}\n`);

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

import device from "embedded:provider/builtin";
import Temperature from "embedded:sensor/TMP102";
import Timer from "timer";
import config from "mc/config";

const sensor = new Temperature({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus,
	},
	alert: {
		io: device.io.Digital,
		pin: config.interrupt_pin,
	},
	onAlert() {
		trace(`Trigger: ${this.sample().temperature} C\n`);
	}
});

sensor.configure({
	extendedRange: true,
	conversionRate: 0.25,
	faultQueue: 2,
	alert: {
		thermostatMode: "comparator",
		highTemperature: 33,
		lowTemperature: 29
	}
});

Timer.repeat(() => {
	trace(`Temperature: ${sensor.sample().temperature.toFixed(2)} C\n`);
}, 2000);


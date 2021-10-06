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

import Temperature from "embedded:sensor/Temperature/TMP102";
import Timer from "timer";
import config from "mc/config";

const sensor = new Temperature({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	},
	alert: {
		io: device.io.Digital,
		pin: config.interrupt_pin
	},
	onAlert() {
		let sample = this.sample();
		trace(`Trigger: ${sample.temperature} C, alert: ${sample.alert}\n`);
	}
});

sensor.configure({
	highTemperature: 33,
	lowTemperature: 29,
	polarity: 0,
	shutdownMode: false,
	thermostatMode: "interrupt",
	faultQueue: 2,
	conversionRate: 1,
	extendedRange: true
});

Timer.repeat(() => {
	let sample = sensor.sample();
	trace(`Temperature: ${sample.temperature.toFixed(2)} C, alert: ${sample.alert}\n`);
}, 2000);


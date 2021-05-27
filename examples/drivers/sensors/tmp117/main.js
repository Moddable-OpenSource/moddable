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
import Temperature from "embedded:sensor/TMP117";
import Timer from "timer";
import config from "mc/config";
const Digital = device.io.Digital;

const temp = new Temperature({
	...device.I2C.default,
	alert: {
		pin: config.interrupt_pin,
		mode: Digital.Input,
		edge: Digital.Falling
	},
	onAlert: () => {
		trace(`Trigger: temp ${temp.sample().temperature} C\n`);
	}
});

temp.configure({
	alert: {
		highTemperature: 31,
		lowTemperature: 27
	}
});

let sample;
Timer.repeat(() => {
	sample = temp.sample();
	trace(`Temperature: ${sample.temperature.toFixed(2)} C\n`);
}, 2000);


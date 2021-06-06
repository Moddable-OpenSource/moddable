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

import device from "embedded:provider/builtin";
import Temperature from "embedded:sensor/LM75";
import Timer from "timer";
import config from "mc/config";
const Digital = device.io.Digital;

const temp = new Temperature({
	...device.I2C.default,
	alert: {
		io: device.io.Digital,
		pin: config.interrupt_pin,
		mode: Digital.InputPullUp,
		edge: Digital.Falling
	},
	onAlert() {
		trace(`Trigger: temp ${temp.sample().temperature} C\n`);
	}
});

temp.configure({
	alert: {
		highTemperature: 33,
		lowTemperature: 29
	}
});

Timer.repeat(() => {
	const sample = temp.sample();

	trace(`Temperature: ${sample.temperature.toFixed(2)} C\n`);
}, 2000);


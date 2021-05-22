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
import Timer from "timer";
import Temperature from "embedded:sensor/LM75";
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
	highTemperature: 31,
	lowTemperature: 29
});


let last;
Timer.repeat(id => {
	let value = temp.sample();
	if (value !== last) {
		trace(`Celsius temperature: ${value.temperature}\n`);
		last = value;
	}
}, 1000);


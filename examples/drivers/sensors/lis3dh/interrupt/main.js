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

import LIS3DH from "embedded:sensor/Accelerometer/LIS3DH";
import Timer from "timer";
import config from "mc/config";
const Digital = device.io.Digital;

const sensor = new LIS3DH({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	},
	alert: {
		io: device.io.Digital,
		pin: config.interrupt_pin
	},
	onAlert() {
		const status = sensor.status();		 // clear interrupt
		const sample = sensor.sample();
		trace(`IRQ Motion: [${sample.x.toFixed(2)}, ${sample.y.toFixed(2)}, ${sample.z.toFixed(2)}] - status: ${status}\n`);
	}
});


sensor.configure({
	rate: 2,		// 20 Hz
	range: 0,		// 2 g
	enable: 0b111,	// ENABLE_X ENABLE_Y ENABLE_Z,
	lowPower: false,
	alert: {
		mode: 1,
		threshold:	0x05,
		duration:	0x02
	}
});

function asHex(v) { return `${v.toString(16).padStart(2, "0")}`; }

let status = sensor.status();
trace(`sensor - status: ${asHex(status)}\n`);

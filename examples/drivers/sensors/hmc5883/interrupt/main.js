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
import { HMC5883, Config } from "embedded:sensor/Magnetometer/HMC5883";
import Timer from "timer";
import config from "mc/config";


const sensor = new HMC5883({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	},
	alert: {
		io: device.io.Digital,
		pin: config.interrupt_pin
	},
	onAlert() {
		const sample = sensor.sample();
		trace(`DataReady: [${sample.x}, ${sample.y}, ${sample.z}]\n`);
	}
});

sensor.configure({
	rate: Config.Rate.RATE_15,
	gain: Config.Gain.GAIN_1_3,
	mode: Config.Mode.CONTINUOUS,
});


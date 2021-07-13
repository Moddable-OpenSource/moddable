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
import { LIS3DH, Config } from "embedded:sensor/Accelerometer/LIS3DH";
import Timer from "timer";

const sensor = new LIS3DH({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

sensor.configure({
	rate: Config.DataRate.DATARATE_10_HZ,
	enable: Config.Features.ENABLE_X | Config.Features.ENABLE_Y | Config.Features.ENABLE_Z,
	lowPower: Config.Features.DISABLE
});

Timer.repeat(() => {
	const sample = sensor.sample();

	trace(`Motion: [${sample.x.toFixed(2)}, ${sample.y.toFixed(2)}, ${sample.z.toFixed(2)}]\n`);
}, 2000);


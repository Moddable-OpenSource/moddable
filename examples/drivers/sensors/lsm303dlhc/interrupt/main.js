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
import { LSM303DLHC_Magnetic, LSM303DLHC_Inertial, Config } from "embedded:sensor/LSM303DLHC";
import Timer from "timer";
import config from "mc/config";
const Digital = device.io.Digital;


const magnetic_sensor = new LSM303DLHC_Magnetic({
	...device.I2C.default,
	rate: Config.Rate.RATE_15,
	gain: Config.Gain.GAIN_1_3,
	mode: Config.Mode.CONTINUOUS,
	alert: {
		io: device.io.Digital,
		pin: config.magnetometer_data_ready,
		mode: Digital.Input,		// device is internally pulled up
		edge: Digital.Falling
	},
	onAlert() {
		const sample = magnetic_sensor.sample();
//		trace(`DataReady: [${sample.x}, ${sample.y}, ${sample.z}]\n`);
	}
});

const inertial_sensor = new LSM303DLHC_Inertial({
    ...device.I2C.default,
    range: Config.Range.RANGE_8_G,
    rate: Config.DataRate.DATARATE_25_HZ,
	polarity: Config.Interrupt.ACTIVE_LOW,
    enable: {
        accel: Config.Features.ENABLE_X | Config.Features.ENABLE_Y | Config.Features_ENABLE_Z,
    },
	alert: {
		io: device.io.Digital,
		pin: config.motion_interrupt_pin,
		mode: Digital.Input,		// device is internally pulled up
		edge: Digital.Falling
	},
	onAlert() {
		const sample = inertial_sensor.sample();
		trace(`Motion: [${sample.x}, ${sample.y}, ${sample.z}]\n`);
	}
});

inertial_sensor.configure({
	alert: {
		enable:	0x7F,				// 6-dir detection
		threshold: 0x20			// about 2G with Range_8_G
	}
});



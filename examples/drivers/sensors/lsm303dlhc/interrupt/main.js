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
import HMC5883 from "embedded:sensor/Magnetometer/HMC5883";
import Timer from "timer";
import config from "mc/config";
const Digital = device.io.Digital;


const mag_sensor = new HMC5883({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	},
	alert: {
		io: device.io.Digital,
		pin: config.magnetometer_data_ready,
		mode: Digital.Input,		// device is internally pulled up
		edge: Digital.Falling
	},
	onAlert() {
		const sample = mag_sensor.sample();
		trace(`DataReady: [${sample.x}, ${sample.y}, ${sample.z}]\n`);
	}
});

mag_sensor.configure({
	rate: 0b100,			// RATE_15
	gain: 0b001,			// GAIN_1_3
	mode: 0					// CONTINUOUS
});

const accel_sensor = new LIS3DH({
	sensor: {
	    ...device.I2C.default,
		address: 0x19,
		io: device.io.SMBus
	},
	alert: {
		io: device.io.Digital,
		pin: config.motion_interrupt_pin
	},
	onAlert() {
		const status = accel_sensor.status();		// clear interrupt
		const sample = accel_sensor.sample();
		trace(`Motion: [${sample.x.toFixed(2)}, ${sample.y.toFixed(2)}, ${sample.z.toFixed(2)}] - status: ${status}\n`);
	}
});

accel_sensor.configure({
    enable: 0b111,			// ENABLE_X ENABLE_Y ENABLE_Z
    range: 0,				// RANGE_2_G
    rate: 0b0010,			// DATARATE_10_HZ
	lowPower: false,
	alert: {
		mode:	1,			// 6-dir detection
		threshold: 0x05,
		duration: 0x02
	}
});


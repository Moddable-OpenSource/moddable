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
import { LIS3DH, Config as Accel_Config } from "embedded:sensor/Accelerometer/LIS3DH";
import { HMC5883, Config as Mag_Config } from "embedded:sensor/Magnetometer/HMC5883";
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
	rate: Mag_Config.Rate.RATE_15,
	gain: Mag_Config.Gain.GAIN_1_3,
	mode: Mag_Config.Mode.CONTINUOUS
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
    enable: Accel_Config.Features.ENABLE_X | Accel_Config.Features.ENABLE_Y | Accel_Config.Features_ENABLE_Z,
    range: Accel_Config.Range.RANGE_2_G,
    rate: Accel_Config.DataRate.DATARATE_10_HZ,
	lowPower: false,
	alert: {
		mode:	Accel_Config.Alert.MOVEMENT,			// 6-dir detection
		threshold: 0x05,
		duration: 0x02
	}
});


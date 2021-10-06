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

/* NOTE: The LSM303DLHC is essentially a LIS3DH and HMC5883 */

import device from "embedded:provider/builtin";
import LIS3DH from "embedded:sensor/Accelerometer/LIS3DH";
import HMC5883 from "embedded:sensor/Magnetometer/HMC5883";
import Timer from "timer";


const mag_sensor = new HMC5883({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

mag_sensor.configure({
	rate: 0b100,		// RATE_15,
	gain: 0b001,		// GAIN_1_3,
	mode: 0				// CONTINUOUS
});

const accel_sensor = new LIS3DH({
	sensor: {
		...device.I2C.default,
		address: 0x19,
		io: device.io.SMBus
	}
});

accel_sensor.configure({
	rate: 2,		// DATARATE_10_HZ,
	enable: 0b111	// ENABLE_X ENABLE_Y ENABLE_Z
});

Timer.repeat(() => {
	const mag_sample = mag_sensor.sample();
	const accel_sample = accel_sensor.sample();

	trace(`Magnetic Field: [${mag_sample.x}, ${mag_sample.y}, ${mag_sample.z}]\n`);
	trace(`Motion: [${accel_sample.x.toFixed(2)}, ${accel_sample.y.toFixed(2)}, ${accel_sample.z.toFixed(2)}]\n`);
}, 2000);


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
import { LIS3DH, Config as Accel_Config } from "embedded:sensor/Accelerometer/LIS3DH";
import { HMC5883, Config as Mag_Config } from "embedded:sensor/Magnetometer/HMC5883";
import Timer from "timer";


const mag_sensor = new HMC5883({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
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
	}
});

accel_sensor.configure({
	rate: Accel_Config.DataRate.DATARATE_10_HZ,
	enable: Accel_Config.Features.ENABLE_X | Accel_Config.Features.ENABLE_Y | Accel_Config.Features_ENABLE_Z
});

Timer.repeat(() => {
	const mag_sample = mag_sensor.sample();
	const accel_sample = accel_sensor.sample();

	trace(`Magnetic Field: [${mag_sample.x}, ${mag_sample.y}, ${mag_sample.z}]\n`);
	trace(`Motion: [${accel_sample.x.toFixed(2)}, ${accel_sample.y.toFixed(2)}, ${accel_sample.z.toFixed(2)}]\n`);
}, 2000);


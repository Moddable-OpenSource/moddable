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

import device from "embedded:provider/builtin";
import { LSM303DLHC_Mag, LSM303DLHC_Accel, Config } from "embedded:sensor/Accelerometer-Magnetometer/LSM303DLHC";
import Timer from "timer";


const mag_sensor = new LSM303DLHC_Mag({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

mag_sensor.configure({
	rate: Config.Rate.RATE_15,
	gain: Config.Gain.GAIN_1_3,
	mode: Config.Mode.CONTINUOUS
});

const accel_sensor = new LSM303DLHC_Accel({
	sensor: {
		...device.I2C.default,
		io: device.io.SMBus
	}
});

accel_sensor.configure({
	rate: Config.DataRate.DATARATE_10_HZ,
	enable: {
		accel: Config.Features.ENABLE_X | Config.Features.ENABLE_Y | Config.Features_ENABLE_Z
	}
});

Timer.repeat(() => {
	const mag_sample = mag_sensor.sample();
	const accel_sample = accel_sensor.sample();

	trace(`Magnetic Field: [${mag_sample.x}, ${mag_sample.y}, ${mag_sample.z}]\n`);
	trace(`Motion: [${accel_sample.x.toFixed(2)}, ${accel_sample.y.toFixed(2)}, ${accel_sample.z.toFixed(2)}]\n`);
}, 2000);


/*
 * Copyright (c) 2021  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import Analog from "embedded:io/analog";
import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";
import I2C from "embedded:io/i2c";
import PulseCount from "embedded:io/pulsecount";
import PWM from "embedded:io/pwm";
import Serial from "embedded:io/serial";
import SMBus from "embedded:io/smbus";
import SPI from "embedded:io/spi";
import PulseWidth from "embedded:io/pulsewidth";
import BMI270 from "embedded:sensor/Accelerometer-Gyroscope-Magnetometer/BMI270";
import MPU6886 from "embedded:sensor/Accelerometer-Gyroscope/MPU6886";

const ACCELERATION_SCALER = 1 / 9.80665;
const IMU_ADDRESS = 0x68;
const BMI270_CHIP_ID_ADDR = 0x00;
const BMI270_CHIP_ID = 0x24;
const MPU6886_WHO_AM_I_ADDR = 0x75;
const MPU6886_WHO_AM_I = 0x19;

class Core2BMI270 extends BMI270 {
	sample() {
		const sample = super.sample();

		if (sample.accelerometer) {
			sample.accelerometer.x *= ACCELERATION_SCALER;
			sample.accelerometer.y *= ACCELERATION_SCALER;
			sample.accelerometer.z *= ACCELERATION_SCALER;
		}

		return sample;
	}
}

const device = {
	I2C: {
		default: {
			io: I2C,
			data: 32,
			clock: 33
		},
		internal: {
			io: I2C,
			data: 21,
			clock: 22
		}
	},
	Serial: {
		default: {
			io: Serial,
			port: 1,
			receive: 3,
			transmit: 1
		}
	},
	SPI: {
		default: {
			io: SPI,
			clock: 18,
			in: 38,
			out: 23,
			port: 1
		}
	},
	Analog: {
		default: {
			io: Analog,
			pin: 35
		}
	},
	io: {Analog, Digital, DigitalBank, I2C, PulseCount, PulseWidth, PWM, Serial, SMBus, SPI},
	pin: {
		displayDC: 15,
		displaySelect: 5
	},
	sensor: {
		IMU: class {
			constructor(options) {
				const sensor = {
					...device.I2C.internal,
					address: IMU_ADDRESS,
					io: device.io.SMBus
				};

				if (MPU6886_WHO_AM_I === readIMURegister(MPU6886_WHO_AM_I_ADDR))
					return new MPU6886({
						...options,
						sensor
					});

				if (BMI270_CHIP_ID === readIMURegister(BMI270_CHIP_ID_ADDR))
					return new Core2BMI270({
						...options,
						sensor
					});

				throw new Error("IMU not found");
			}
		}
	}
};

function readIMURegister(register) {
	let io;
	try {
		io = new device.io.SMBus({
			data: device.I2C.internal.data,
			clock: device.I2C.internal.clock,
			hz: 400_000,
			address: IMU_ADDRESS
		});
		return io.readUint8(register);
	}
	catch (e) {
	}
	finally {
		io?.close();
	}
}

export default device;

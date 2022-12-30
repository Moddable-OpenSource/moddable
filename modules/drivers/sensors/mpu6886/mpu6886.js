/*
 * Copyright (c) 2020-2022 Daisuke Sato, Wilberforce, Moddable Tech, Inc.
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

/*
 * MPU6886 Accelerometer + Gyro
 *          Datasheet: 	    https://github.com/m5stack/M5-Schematic/blob/master/datasheet/MPU-6886-000193%2Bv1.1_GHIC.PDF.pdf
 *          Register Map:   https://github.com/m5stack/M5StickC/blob/master/src/utility/MPU6886.h
 *                          https://github.com/m5stack/M5StickC/blob/master/src/utility/MPU6886.cpp
 */

import Timer from "timer";

const REGISTERS = Object.freeze({
    WHO_AM_I: 0x75,
    ACCEL_INTEL_CTRL: 0x69,
    SMPLRT_DIV: 0x19,
    INT_PIN_CFG: 0x37,
    INT_ENABLE: 0x38,
    FIFO_WM_INT_STATUS: 0x39,
    INT_STATUS: 0x3A,
    ACCEL_WOM_X_THR: 0x20,
    ACCEL_WOM_Y_THR: 0x21,
    ACCEL_WOM_Z_THR: 0x22,
    ACCEL_XOUT_H: 0x3B,
    ACCEL_XOUT_L: 0x3C,
    ACCEL_YOUT_H: 0x3D,
    ACCEL_YOUT_L: 0x3E,
    ACCEL_ZOUT_H: 0x3F,
    ACCEL_ZOUT_L: 0x40,
    TEMP_OUT_H: 0x41,
    TEMP_OUT_L: 0x42,
    GYRO_XOUT_H: 0x43,
    GYRO_XOUT_L: 0x44,
    GYRO_YOUT_H: 0x45,
    GYRO_YOUT_L: 0x46,
    GYRO_ZOUT_H: 0x47,
    GYRO_ZOUT_L: 0x48,
    USER_CTRL: 0x6A,
    PWR_MGMT_1: 0x6B,
    PWR_MGMT_2: 0x6C,
    CONFIG: 0x1A,
    GYRO_CONFIG: 0x1B,
    ACCEL_CONFIG: 0x1C,
    ACCEL_CONFIG2: 0x1D,
    FIFO_EN: 0x23
});

const EXPECTED_WHO_AM_I = 0x19;
const GYRO_SCALER = Object.freeze({
    GFS_250DPS: (250.0 / 32768.0),
    GFS_500DPS: (500.0 / 32768.0),
    GFS_1000DPS: (1000.0 / 32768.0),
    GFS_2000DPS: (2000.0 / 32768.0)
});
const ACCEL_SCALER = Object.freeze({
    AFS_2G: (2.0 / 32768.0),
    AFS_4G: (4.0 / 32768.0),
    AFS_8G: (8.0 / 32768.0),
    AFS_16G: (16.0 / 32768.0)
});

class MPU6886  {
	#io;
	#view;
    #gyroScale = GYRO_SCALER.GFS_2000DPS;
    #accelScale = ACCEL_SCALER.AFS_8G;

    constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 400_000,
			address: 0x68,
			...options.sensor
		});

        this.#view = new DataView(new ArrayBuffer(14));
        this.operation = "gyroscope";
        if (io.readUint8(REGISTERS.WHO_AM_I) != EXPECTED_WHO_AM_I)
			throw new Error("unrecognized")

        Timer.delay(1);

        io.writeUint8(REGISTERS.PWR_MGMT_1, 0x00)
        Timer.delay(10);
        io.writeUint8(REGISTERS.PWR_MGMT_1, 0x80)
        Timer.delay(10);
        io.writeUint8(REGISTERS.PWR_MGMT_1, 0x01)
        Timer.delay(10);

        io.writeUint8(REGISTERS.ACCEL_CONFIG, 0x10)
        Timer.delay(1);
        io.writeUint8(REGISTERS.GYRO_CONFIG, 0x18)
        Timer.delay(1);
        io.writeUint8(REGISTERS.CONFIG, 0x01)
        Timer.delay(1);
        io.writeUint8(REGISTERS.SMPLRT_DIV, 0x05)
        Timer.delay(1);
        io.writeUint8(REGISTERS.INT_ENABLE, 0x00)
        Timer.delay(1);
        io.writeUint8(REGISTERS.ACCEL_CONFIG2, 0x00)
        Timer.delay(1);
        io.writeUint8(REGISTERS.USER_CTRL, 0x00)
        Timer.delay(1);
        io.writeUint8(REGISTERS.INT_PIN_CFG, 0x22)
        Timer.delay(1);
        io.writeUint8(REGISTERS.INT_ENABLE, 0x01)
        Timer.delay(1);

        Timer.delay(100);
    }
	close() {
		this.#io?.close();
		this.#io = undefined;
	}
    configure(dictionary) {		//@@ revisit
        for (let property in dictionary) {
            switch (property) {
                case "GYRO_SCALER":
                    this.#gyroScale = dictionary.GYRO_SCALER;
                    break;
                case "ACCEL_SCALER":
                    this.#accelScale = dictionary.ACCEL_SCALER;
                    break;
            }
        }
    }
    sample() {
		const view = this.#view;
        this.#io.readBuffer(REGISTERS.ACCEL_XOUT_H, view);
    
		return {
			accelerometer: {
				x: view.getInt16(0) * this.#accelScale,
				y: view.getInt16(2) * this.#accelScale,
				z: view.getInt16(4) * this.#accelScale
			},
			thermometer: {
				temperature: view.getInt16(6) / 326.8 + 25.0
			},
			gyroscope: {
				x: view.getInt16(8) * this.#gyroScale,
				y: view.getInt16(10) * this.#gyroScale,
				z: view.getInt16(12) * this.#gyroScale
			}
		}
	}
}

export default MPU6886;

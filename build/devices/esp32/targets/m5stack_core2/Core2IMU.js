/*
 * Copyright (c) 2026  Satoshi Tanaka
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

import MPU6886 from "embedded:sensor/Accelerometer-Gyroscope/MPU6886";
import BMI270 from "embedded:sensor/Accelerometer-Gyroscope-Magnetometer/BMI270";

class Core2PMU6886 extends MPU6886 {
    sample() {
        const sample = super.sample();
        sample.accelerometer.x *= -1;
        sample.accelerometer.y *= -1;
        sample.gyroscope.y *= -1;

        return sample;
    }
}

const ACCELERATION_SCALER = 1 / 9.80665;
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

export class Core2IMU {
    constructor(options) {
        const s = new device.io.SMBus({
            ...device.I2C.internal,
            address: 0x68,
            hz: 400_000
        });

        if (0x19 === s.readUint8(0x75)){
            s.close();
            return new Core2PMU6886(options);
        }

        if (0x24 === s.readUint8(0x00)){
            s.close();
            return new Core2BMI270(options);
        }
        s.close();
        throw new Error("IMU not found");
    }
}
export default Core2IMU;
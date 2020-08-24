/*
 * Copyright (c) 2019 Moddable Tech, Inc.
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
	InvenSense MPU-6050 Accelerometer + Gyro
            Datasheet: 			http://43zrtwysvxb2gf29r5o0athu.wpengine.netdna-cdn.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
            Register Map:       https://www.invensense.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
*/

import SMBus from "pins/smbus";
import Timer from "timer";

const REGISTERS = {
    INT_BYPASS: 0x37,
    ACCEL_XOUT: 0x3B, //big endian
    ACCEL_YOUT: 0x3D,
    ACCEL_ZOUT: 0x3F,
    TEMP_OUT: 0x41,
    GYRO_XOUT: 0x43,
    GYRO_YOUT: 0x45,
    GYRO_ZOUT: 0x47,
    PWR_MGMT_1: 0x6B,
    PWR_MGMT_2: 0x6C,
    WHO_AM_I: 0x75
};
Object.freeze(REGISTERS);

const EXPECTED_WHO_AM_I = 0x68;
const GYRO_SCALER = (1 / 131); //Datasheet Section 6.1
const ACCEL_SCALER = (1 / 16384); //Datasheet Section 6.2

class Gyro_Accelerometer extends SMBus {
    constructor(dictionary) {
        super(Object.assign({ address: 0x68 }, dictionary));
        this.xlRaw = new ArrayBuffer(6);
        this.xlView = new DataView(this.xlRaw);
        this.gyroRaw = new ArrayBuffer(6);
        this.gyroView = new DataView(this.gyroRaw);
        this.operation = "gyroscope";
        this.enable();
        this.checkIdentification();
    }

    checkIdentification() {
        let gxlID = this.readByte(REGISTERS.WHO_AM_I);
        if (gxlID != EXPECTED_WHO_AM_I) throw ("bad WHO_AM_I ID for MPU-6050.");
    }

    configure(dictionary) {
        for (let property in dictionary) {
            switch (property) {
                case "operation":
                    this.operation = dictionary.operation;
            }
        }
    }

    enable() {
        this.writeByte(REGISTERS.PWR_MGMT_1, 0b10000000);
        Timer.delay(150);
        this.writeByte(REGISTERS.PWR_MGMT_1, 0);
        this.writeByte(REGISTERS.INT_BYPASS, 0b00000010);
        Timer.delay(150);
    }

    sampleXL() {
        this.readBlock(REGISTERS.ACCEL_XOUT, 6, this.xlRaw);
        return {
            x: this.xlView.getInt16(0) * ACCEL_SCALER, 
            y: this.xlView.getInt16(2) * ACCEL_SCALER,
            z: this.xlView.getInt16(4) * ACCEL_SCALER
        }
    }

    sampleGyro() {
        this.readBlock(REGISTERS.GYRO_XOUT, 6, this.gyroRaw);
        return {
            x: this.gyroView.getInt16(0) * GYRO_SCALER, 
            y: this.gyroView.getInt16(2) * GYRO_SCALER,
            z: this.gyroView.getInt16(4) * GYRO_SCALER
        }
    }

    sample() {
        switch (this.operation) {
            case "gyroscope":
                return this.sampleGyro();
            case "accelerometer":
                return this.sampleXL();
            default:
                trace("Invalid operation for MPU-6050.");
                throw ("Invalid operation for MPU-6050.");
        }
    }
}
Object.freeze(Gyro_Accelerometer.prototype);

export default Gyro_Accelerometer;
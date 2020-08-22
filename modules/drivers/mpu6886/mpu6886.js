/*
 * Copyright (c) 2020 Daisuke Sato
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
 *          Datasheet: 	    https://github.com/m5stack/M5-Schematic/blob/afd7409be8593a0199a0617325dda171f3b2b44c/datasheet/MPU-6886-000193%2Bv1.1_GHIC.PDF.pdf
 *          Register Map:   https://github.com/m5stack/M5StickC/blob/fbabae1d836e497cd50cfae9f7d1e5c05015eb77/src/utility/MPU6886.h
 *                          https://github.com/m5stack/M5StickC/blob/fbabae1d836e497cd50cfae9f7d1e5c05015eb77/src/utility/MPU6886.cpp
 */

import SMBus from "pins/smbus";
import Timer from "timer";

const REGISTERS = {
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
};

Object.freeze(REGISTERS);

const EXPECTED_WHO_AM_I = 0x19;
const GYRO_SCALER = {
    GFS_250DPS: (250.0/32768.0),
    GFS_500DPS: (500.0/32768.0),
    GFS_1000DPS: (1000.0/32768.0),
    GFS_2000DPS: (2000.0/32768.0)
}
const ACCEL_SCALER = {
    AFS_2G: (2.0/32768.0),
    AFS_4G: (4.0/32768.0),
    AFS_8G: (8.0/32768.0),
    AFS_16G: (16.0/32768.0)
}

class SMBHold extends SMBus { //SMBus implementation that holds the i2c bus between the i2c.read and i2c.write on read operations.
    constructor(dictionary) {
        super(dictionary);
    }
    readByte(register) {
        super.write(register, false);
        return super.read(1)[0];
    }
    readWord(register) {
        super.write(register, false);
        let value = super.read(2);
        return value[0] | (value[1] << 8);
    }
    readBlock(register, count, buffer) {
        super.write(register, false);
        return buffer ? super.read(count, buffer) : super.read(count);
    }
}

class Gyro_Accelerometer extends SMBHold {
    constructor(dictionary) {
        super(Object.assign({
            address:0x68
        }, dictionary));
        this.xlRaw = new ArrayBuffer(6);
        this.xlView = new DataView(this.xlRaw);
        this.gyroRaw = new ArrayBuffer(6);
        this.gyroView = new DataView(this.gyroRaw);
        this.tempRaw = new ArrayBuffer(2);
        this.tempView = new DataView(this.tempRaw);
        this.operation = "gyroscope";
        this.gyroScale = 0.0;
        this.accelScale = 0.0;
        this.checkIdentification();
        this.reboot();
    }

    checkIdentification() {
        let gxlID = this.readByte(REGISTERS.WHO_AM_I);
        if (gxlID != EXPECTED_WHO_AM_I) throw ("bad WHO_AM_I ID for MPU6886.");
    }

    configure(dictionary) {
        for (let property in dictionary) {
            switch (property) {
                case "operation":
                    this.operation = dictionary.operation;
            }
        }
    }

    reboot() {

        Timer.delay(1);

        this.writeByte(REGISTERS.PWR_MGMT_1, 0x00)
        Timer.delay(10);
        this.writeByte(REGISTERS.PWR_MGMT_1, 0x80)
        Timer.delay(10);
        this.writeByte(REGISTERS.PWR_MGMT_1, 0x01)
        Timer.delay(10);

        this.writeByte(REGISTERS.ACCEL_CONFIG, 0x10)
        Timer.delay(1);
        this.writeByte(REGISTERS.GYRO_CONFIG, 0x18)
        Timer.delay(1);
        this.writeByte(REGISTERS.CONFIG, 0x01)
        Timer.delay(1);
        this.writeByte(REGISTERS.SMPLRT_DIV, 0x05)
        Timer.delay(1);
        this.writeByte(REGISTERS.INT_ENABLE, 0x00)
        Timer.delay(1);
        this.writeByte(REGISTERS.ACCEL_CONFIG2, 0x00)
        Timer.delay(1);
        this.writeByte(REGISTERS.USER_CTRL, 0x00)
        Timer.delay(1);
        this.writeByte(REGISTERS.INT_PIN_CFG, 0x22)
        Timer.delay(1);
        this.writeByte(REGISTERS.INT_ENABLE, 0x01)
        Timer.delay(1);

        // Set default scalers
        this.gyroScale = GYRO_SCALER.GFS_2000DPS;
        this.accelScale = ACCEL_SCALER.AFS_8G;

        Timer.delay(100);
    }

    sampleXL() {
        this.readBlock(REGISTERS.ACCEL_XOUT_H, 6, this.xlRaw);
        return {
            x: this.xlView.getInt16(0, true) * this.accelScale,
            y: this.xlView.getInt16(2, true) * this.accelScale,
            z: this.xlView.getInt16(4, true) * this.accelScale
        }
    }

    sampleGyro() {
        this.readBlock(REGISTERS.GYRO_XOUT_H, 6, this.gyroRaw);
        return {
            x: this.gyroView.getInt16(0, true) * this.gyroScale,
            y: this.gyroView.getInt16(2, true) * this.gyroScale,
            z: this.gyroView.getInt16(4, true) * this.gyroScale
        }
    }

    sampleTemp() {
        this.readBlock(REGISTERS.TEMP_OUT_H, 2, this.tempRaw);
        return this.tempView.getInt16(0, true) / 326.8 + 25.0
    }

    sample() {
        switch (this.operation) {
            case "gyroscope":
                return this.sampleGyro();
            case "accelerometer":
                return this.sampleXL();
            case "temp":
                return this.sampleTemp();
            default:
                trace("Invalid operation for MPU6886.");
                throw ("Invalid operation for MPU6886.");
        }
    }
}
Object.freeze(Gyro_Accelerometer.prototype);

export default Gyro_Accelerometer;

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
    I2C_MST_CNTRL: 0x24,
    I2C_SLV0_ADDR: 0x25,
    I2C_SLV0_REG: 0x26,
    I2C_SLV0_CTRL: 0x27,
    INT_BYPASS: 0x37,
    ACCEL_XOUT: 0x3B, //big endian
    ACCEL_YOUT: 0x3D,
    ACCEL_ZOUT: 0x3F,
    TEMP_OUT: 0x41,
    GYRO_XOUT: 0x43,
    GYRO_YOUT: 0x45,
    GYRO_ZOUT: 0x47,
    EXT_SENS_DATA_00: 0x49,
    I2C_SLV0_DO: 0x63,
    USER_CNTRL: 0x6A,
    PWR_MGMT_1: 0x6B,
    PWR_MGMT_2: 0x6C,
    WHO_AM_I: 0x75
};
Object.freeze(REGISTERS);

const EXPECTED_WHO_AM_I = 0x68;
const GYRO_SCALER = (1 / 131); //Datasheet Section 6.1
const ACCEL_SCALER = (1 / 16384); //Datasheet Section 6.2
const I2C_MST_EN = 0x20;
const I2C_MST_CLK = 0x0D;
const I2C_SLV0_EN = 0x80;

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
        this.writeByte(REGISTERS.INT_BYPASS, 0);
        this.writeByte(REGISTERS.USER_CNTRL, I2C_MST_EN);
        this.writeByte(REGISTERS.I2C_MST_CNTRL, I2C_MST_CLK);
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

    /**
     * Access to I2C Master Bus for the MPU for accessing connected slave devices
     * Usage is similar to regular SMBus
     * (using bypass will cause problems for devices not implementing SMBus)
     */
    get I2CMasterBus() {
        let that = this;
        return class {
            constructor(dictionary) {
                this.config = dictionary;
            }

            calcLength(value) {
                let len = 0;
                for(let i in value) {
                    switch(typeof(value[i])) {
                        case "number":
                            len++;
                            break;
                        case "string":
                            len += value[i].length;
                            break;
                        default:
                            if(value[i] instanceof(ArrayBuffer))
                                len += value[i].byteLength;
                            else if(value[i].length !== undefined)
                                len += value[i].length;
                            else
                                throw "MPU I2CMasterBus unsupported type";
                    }
                }

                return len;
            }

            readBlock(register, count, buffer) {
                if(count > 24)
                    throw "MPU I2CMasterBus readBlock maximum length is 24 bytes";
                that.writeByte(REGISTERS.I2C_SLV0_ADDR, this.config.address | 0x80);
                that.writeByte(REGISTERS.I2C_SLV0_REG, register);
                that.writeByte(REGISTERS.I2C_SLV0_CTRL, I2C_SLV0_EN | count);
                Timer.delay(1);
                return that.readBlock(REGISTERS.EXT_SENS_DATA_00, count, buffer);
            }

            writeBlock(register, ...value) {
                let count = this.calcLength(value);
                if(count > 4)
                    throw "MPU I2CMasterBus writeBlock maximum length is 4 bytes";
                that.writeByte(REGISTERS.I2C_SLV0_ADDR, this.config.address);
                that.writeByte(REGISTERS.I2C_SLV0_REG, register);
                that.writeBlock(REGISTERS.I2C_SLV0_DO, ...value);
                return that.writeByte(REGISTERS.I2C_SLV0_CTRL, I2C_SLV0_EN | count);
            }

            writeByte(register, value) {
                return this.writeBlock(register, value & 0xFF);
            }

            writeWord(register, value, endian) {
                if (endian)
                    return this.write(register, (value >> 8) & 255, value & 255);
                else
                    return this.write(register, value & 255, (value >> 8) & 255);
            }

            readByte(register) {
                return this.readBlock(register, 1)[0];
            }

            readWord(register, endian) {
                let value = this.readBlock(register, 2);
                return endian ? (value[1] | (value[0] << 8)) : (value[0] | (value[1] << 8));
            }
        };
    }

}
Object.freeze(Gyro_Accelerometer.prototype);

export default Gyro_Accelerometer;
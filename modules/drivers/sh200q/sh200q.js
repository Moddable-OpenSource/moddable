/*
 * Copyright (c) Wilberforce
 
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
	sh200q Accelerometer + Gyro
            Datasheet: 			http://senodia.com/Uploads/Product/5b2b6ef1216e8.pdf
            Register Map:   https://github.com/m5stack/M5StickC/blob/master/src/IMU.h#L6-L21
            https://github.com/m5stack/M5StickC/blob/master/src/IMU.cpp
           
*/

import SMBus from "pins/smbus";
import Timer from "timer";

const REGISTERS = {
    ACCEL_XOUT: 0x00,
    TEMP_OUT: 0x0C,
    GYRO_XOUT: 0x06,
    WHO_AM_I: 0x30,

    ACC_CONFIG: 0x0E,
    GYRO_CONFIG: 0x0F,
    GYRO_DLPF: 0x11,
    FIFO_CONFIG: 0x12,
    ACC_RANGE: 0x16,
    GYRO_RANGE: 0x2B,
    OUTPUT_ACC: 0x00,
    OUTPUT_GYRO: 0x06,
    OUTPUT_TEMP: 0x0C,
    REG_SET1: 0xBA,
    REG_SET2: 0xCA, //ADC reset
    ADC_RESET: 0xC2, //drive reset
    SOFT_RESET: 0x7F,
    RESET: 0x75
};
Object.freeze(REGISTERS);

const EXPECTED_WHO_AM_I = 0x18;
const GYRO_SCALER = (1 / 16.4); // at +/- 2000 degree/sec range, per datasheet section 2.1
const ACCEL_SCALER = (1 / 4096); // at +/- 8g range, per datasheet section 2.2

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
            address:0x6C
        }, dictionary));
        this.xlRaw = new ArrayBuffer(6);
        this.xlView = new DataView(this.xlRaw);
        this.gyroRaw = new ArrayBuffer(6);
        this.gyroView = new DataView(this.gyroRaw);
        this.tempRaw = new ArrayBuffer(2);
        this.tempView = new DataView(this.tempRaw);
        this.operation = "gyroscope";
        this.reboot();
        this.checkIdentification();
    }

    checkIdentification() {
        let gxlID = this.readByte(REGISTERS.WHO_AM_I);
        if (gxlID != EXPECTED_WHO_AM_I) throw ("bad WHO_AM_I ID for sh200Q.");
    }

    configure(dictionary) {
        for (let property in dictionary) {
            switch (property) {
                case "operation":
                    this.operation = dictionary.operation;
            }
        }
    }

    IMUReset() {
        let tempdata = this.readByte(REGISTERS.RESET);

        //REGISTERS.RESET
        tempdata = tempdata | 0x80;
        this.writeByte(REGISTERS.RESET, tempdata);

        Timer.delay(1);

        tempdata = tempdata & 0x7F;
        this.writeByte(REGISTERS.RESET, tempdata);
    }

    AdcReset() {

        let tempdata = this.readByte(REGISTERS.RESET);

        //REGISTERS.RESET
        tempdata = tempdata | 0x04;
        this.writeByte(REGISTERS.RESET, tempdata);

        Timer.delay(1);

        tempdata = tempdata & 0xFB;
        this.writeByte(REGISTERS.RESET, tempdata);
    }

    reboot() {

        //this.IMUReset();

        let tempdata = this.readByte(REGISTERS.RESET);

        //REGISTERS.RESET
        tempdata = tempdata | 0x80;
        this.writeByte(REGISTERS.RESET, tempdata);

        Timer.delay(1);

        tempdata = tempdata & 0x7F;
        this.writeByte(REGISTERS.RESET, tempdata);
        //this.ADCReset();

        this.readByte(REGISTERS.RESET);

        //REGISTERS.RESET
        tempdata = tempdata | 0x04;
        this.writeByte(REGISTERS.RESET, tempdata);

        Timer.delay(1);

        tempdata = tempdata & 0xFB;
        this.writeByte(REGISTERS.RESET, tempdata);

        //let tempdata = this.readByte(0xD8);

        tempdata = this.readByte(0xD8);



        tempdata = tempdata | 0x80;
        this.writeByte(0xD8, tempdata);

        Timer.delay(1);

        tempdata = tempdata & 0x7F;
        this.writeByte(0xD8, tempdata);

        this.writeByte(0x78, 0x61);

        Timer.delay(1);

        this.writeByte(0x78, 1, 0x00);

        //set acc odr 256hz
        //0x81 1024hz   //0x89 512hz    //0x91  256hz 
        this.writeByte(REGISTERS.ACC_CONFIG, 0x91);

        //set gyro odr 500hz
        //0x11 1000hz    //0x13  500hz   //0x15  256hz
        this.writeByte(REGISTERS.GYRO_CONFIG, 0x13);

        //set gyro dlpf 50hz
        //0x00 250hz   //0x01 200hz   0x02 100hz  0x03 50hz  0x04 25hz
        this.writeByte(REGISTERS.GYRO_DLPF, 0x03);

        //set no buffer mode
        this.writeByte(REGISTERS.FIFO_CONFIG, 0x00);

        //set acc range +-8G
        this.writeByte(REGISTERS.ACC_RANGE, 0x01);

        //set gyro range +-2000/s
        this.writeByte(REGISTERS.GYRO_RANGE, 0x00);

        this.writeByte(REGISTERS.REG_SET1, 0xC0);

        tempdata = this.readByte(REGISTERS.REG_SET2);

        //ADC Reset
        tempdata = tempdata | 0x10;
        this.writeByte(REGISTERS.REG_SET2, tempdata);

        Timer.delay(1);

        tempdata = tempdata & 0xEF;
        this.writeByte(REGISTERS.REG_SET2, tempdata);

        Timer.delay(10);
    }

    sampleXL() {
        this.readBlock(REGISTERS.ACCEL_XOUT, 6, this.xlRaw);
        return {
            x: this.xlView.getInt16(0, true) * ACCEL_SCALER,
            y: this.xlView.getInt16(2, true) * ACCEL_SCALER,
            z: this.xlView.getInt16(4, true) * ACCEL_SCALER
        }
    }

    sampleGyro() {
        this.readBlock(REGISTERS.GYRO_XOUT, 6, this.gyroRaw);
        return {
            x: this.gyroView.getInt16(0, true) * GYRO_SCALER,
            y: this.gyroView.getInt16(2, true) * GYRO_SCALER,
            z: this.gyroView.getInt16(4, true) * GYRO_SCALER
        }
    }

    sampleTemp() {
        this.readBlock(REGISTERS.TEMP_OUT, 2, this.gyroRaw);
        return this.tempView.getInt16(0, true) / 333.87 + 21.0
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
                trace("Invalid operation for sh200Q.");
                throw ("Invalid operation for sh200Q.");
        }
    }
}
Object.freeze(Gyro_Accelerometer.prototype);

export default Gyro_Accelerometer;
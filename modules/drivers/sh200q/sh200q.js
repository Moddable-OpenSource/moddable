/*
 * Copyright (c) Wilberforce
 
 * Copyright (c) 2019-2020 Moddable Tech, Inc.
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
            Datasheet: 	http://senodia.com/Uploads/Product/5b2b6ef1216e8.pdf
            Register Map:   https://github.com/m5stack/M5StickC/blob/master/src/utility/SH200Q.h#L7-L22
            https://github.com/m5stack/M5StickC/blob/master/src/utility/SH200Q.cpp
           
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

const GYRO_SCALER = {
    GFS_125DPS: (125.0 / 32768.0),
    GFS_250DPS: (250.0 / 32768.0),
    GFS_500DPS: (500.0 / 32768.0),
    GFS_1000DPS: (1000.0 / 32768.0),
    GFS_2000DPS: (2000.0 / 32768.0)
};
Object.freeze(GYRO_SCALER);

const ACCEL_SCALER = {
    AFS_2G: (2.0 / 32768.0),
    AFS_4G: (4.0 / 32768.0),
    AFS_8G: (8.0 / 32768.0),
    AFS_16G: (16.0 / 32768.0)
};
Object.freeze(ACCEL_SCALER);

class Gyro_Accelerometer extends SMBus {
    #gyroScale = GYRO_SCALER.GFS_2000DPS;
    #accelScale = ACCEL_SCALER.AFS_8G;
    #view = new DataView(new ArrayBuffer(6));
    #operation = this.sampleGyro;

    constructor(dictionary) {
        super({
            address:0x6C,
            ...dictionary
		});
        if (super.readByte(REGISTERS.WHO_AM_I) != EXPECTED_WHO_AM_I)
			throw new Error("unrecognized")
        this.enable();
    }

    configure(dictionary) {
        for (let property in dictionary) {
            switch (property) {
                case "operation":
					switch (dictionary.operation) {
						case "gyroscope":
							this.#operation = this.sampleGyro;
							break;
						case "accelerometer":
							this.#operation = this.sampleXL;
							break;
						case "temp":
							this.#operation = this.sampleTemp;
							break;
						default:
							throw new Error("invalid");
					}
                    break;
                case "GYRO_SCALER":
                    this.#gyroScale = dictionary.GYRO_SCALER;
                    // 
                    //this.writeByte(REGISTERS.GYRO_RANGE, 0x00);
                    break;
                case "ACCEL_SCALER":
                    this.#accelScale = dictionary.ACCEL_SCALER;
                    // 
                    //this.writeByte(REGISTERS.ACC_RANGE, 0x00);
                    break;
            }
        }
    }

    enable() {

        Timer.delay(1);

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

        //ADC Reset
        const temp = this.readByte(REGISTERS.REG_SET2);
        this.writeByte(REGISTERS.REG_SET2, temp | 0x10);

        Timer.delay(1);

        this.writeByte(REGISTERS.REG_SET2, temp & 0xEF);

        Timer.delay(10);
    }

    sampleXL() {
        this.readBlock(REGISTERS.ACCEL_XOUT, 6, this.#view.buffer);
        return {
            x: this.#view.getInt16(0, true) * this.#accelScale,
            y: this.#view.getInt16(2, true) * this.#accelScale,
            z: this.#view.getInt16(4, true) * this.#accelScale
        }
    }

    sampleGyro() {
        this.readBlock(REGISTERS.GYRO_XOUT, 6, this.#view.buffer);
        return {
            x: this.#view.getInt16(0, true) * this.#gyroScale,
            y: this.#view.getInt16(2, true) * this.#gyroScale,
            z: this.#view.getInt16(4, true) * this.#gyroScale
        }
    }

    sampleTemp() {
        this.readBlock(REGISTERS.TEMP_OUT, 2, this.#view.buffer);
        return this.#view.getInt16(0, true) / 333.87 + 21.0
    }

    sample() {
		return this.#operation();
    }
}
Object.freeze(Gyro_Accelerometer.prototype);

export default Gyro_Accelerometer;

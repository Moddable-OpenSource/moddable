/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
    BMP85,BMP180 - temp/pressure
	Datasheet - https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf
    Implementation based on Adafruit https://github.com/adafruit/Adafruit-BMP085-Library
*/

import Timer from "timer";

const Register = Object.freeze({
	BMP180_CONTROL: 0xF4,
	BMP180_RESULT: 0xF6,
	BMP180_RESET: 0xE0,
	BMP180_CHIPID: 0xD0,
	CMD_TEMP: 0x2E,
	CMD_PRES: 0x34,
	CMD_RESET: 0xB6
});

const Config = Object.freeze({
	Mode: {
		ULTRALOWPOWER:	0x00,
		STANDARD:		0x01,
		HIGHRES:		0x02,
		ULTRAHIGHRES:	0x03
	}
}, true);

const READ_DELAY = Object.freeze([5, 8, 14, 26]);

class aHostObject @ "xs_bmp180_host_object_destructor" {
	constructor() @ "xs_bmp180_host_object_constructor";
}

class BMP180 extends aHostObject {
	#io;
	#calib;
	#byteBuffer;
	#wordBuffer;
	#valueBuffer;
	#mode;

	constructor(options) {
		super(options);
		const io = this.#io = new options.sensor.io({
			hz: 400_000,
			address: 0x77,
			...options.sensor
		});

		const bBuf = this.#byteBuffer = new Uint8Array(1);
		const wBuf = this.#wordBuffer = new Uint8Array(2);
		this.#valueBuffer = new Uint8Array(3);

		this.#mode = Config.Mode.ULTRALOWPOWER;

		bBuf[0] = Register.BMP180_CHIPID;
		io.write(bBuf);
		io.read(bBuf);
		if (0x55 !== bBuf[0]) {
			this.close();
			throw new Error("unexpected sensor");
		}

		wBuf[0] = Register.BMP180_RESET;
		wBuf[1] = Register.CMD_RESET;
		io.write(wBuf);

		this.#initialize();
	}
	configure(options) {
		if (undefined !== options.mode)
			this.#mode = options.mode & 0b11;
	}
	#close() @ "xs_bmp180_close";
	close() {
		this.#close();
		this.#io?.close();
		this.#io = undefined;
	}
	sample() {
		const io = this.#io;
		const bBuf = this.#byteBuffer;
		const wBuf = this.#wordBuffer;

		wBuf[0] = Register.BMP180_CONTROL;
		wBuf[1] = Register.CMD_TEMP;
		io.write(wBuf);
		Timer.delay(5);

		let temp = this.#readUInt(Register.BMP180_RESULT);

		wBuf[0] = Register.BMP180_CONTROL;
		wBuf[1] = Register.CMD_PRES + (this.#mode << 6);
		io.write(wBuf);
		Timer.delay(READ_DELAY[this.#mode]);

		bBuf[0] = Register.BMP180_RESULT;
		io.write(bBuf);
		io.read(this.#valueBuffer);

		let pr = (this.#valueBuffer[0] << 16) | (this.#valueBuffer[1] << 8) | this.#valueBuffer[2];
		pr >>= (8 - this.#mode);

		const val = this.#calculate(temp, pr, this.#mode);
		return { barometer: { pressure: val.pressure }, thermometer: { temperature: val.temperature } };
	}
	#initialize() {
		let calib = {};
		calib.AC1 = this.#readInt(0xAA);
		calib.AC2 = this.#readInt(0xAC);
		calib.AC3 = this.#readInt(0xAE);
		calib.AC4 = this.#readUInt(0xB0);
		calib.AC5 = this.#readUInt(0xB2);
		calib.AC6 = this.#readUInt(0xB4);
		calib.B1 = this.#readInt(0xB6);
		calib.B2 = this.#readInt(0xB8);
		calib.MB = this.#readInt(0xBA);
		calib.MC = this.#readInt(0xBC);
		calib.MD = this.#readInt(0xBE);

		this.#setCalibration(calib);
	}
	#calculate(rawTemp, rawPressure, mode) @ "xs_bmp180_calculate";
	#setCalibration(calibrate) @ "xs_bmp180_setCalibration";
	#twoC16(val) {
		return (val > 32767) ? -(65535 - val + 1) : val;
	}
	#readUInt(reg) {
		const io = this.#io;
		const bBuf = this.#byteBuffer;
		const wBuf = this.#wordBuffer;
		bBuf[0] = reg;
		io.write(bBuf);
		io.read(wBuf);
		return (wBuf[0] << 8) | wBuf[1];
	}
	#readInt(reg) {
		return this.#twoC16(this.#readUInt(reg));
	}
}

export { BMP180 as default, BMP180, Config };

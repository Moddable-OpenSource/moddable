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
    BMP280 - temp/pressure
    Implementation based on Adafruit https://github.com/adafruit/Adafruit_BMP280_Library
*/
import Timer from "timer";

const Register = Object.freeze({
	BMP280_SOFTRESET: 0xE0,
	BMP280_STATUS: 0xF3,
	BMP280_CONTROL: 0xF4,
	BMP280_CONFIG: 0xF5,
	BMP280_PRESSUREDATA: 0xF7,
	BMP280_TEMPDATA: 0xFA,
	BMP280_CHIPID: 0xD0
});

const Config = Object.freeze({
	Sampling: {
		NONE: 0x00,
		X1: 0x01,
		X2: 0x02,
		X4: 0x03,
		X8: 0x04,
		X16: 0x05
	},
	Filter: {
		OFF: 0x00,
		X2: 0x01,
		X4: 0x02,
		X8: 0x03,
		X16: 0x04
	},
	Mode: {
		SLEEP: 0x00,
		FORCED: 0x01,
		NORMAL: 0x03,
		SOFT_RESET: 0xB6
	},
	Standby: {
		MS_1:	0x00,
		MS_63:	0x01,
		MS_125:	0x02,
		MS_250:	0x03,
		MS_500: 0x04,
		MS_1000: 0x05,
		MS_2000: 0x06,
		MS_4000: 0x07
	}
}, true);

class aHostObject @ "xs_bmp280_host_object_destructor" {
	constructor() @ "xs_bmp280_host_object_constructor";
}

class BMP280 extends aHostObject {
	#io;
	#byteBuffer = new Uint8Array(1);
	#wordBuffer = new Uint8Array(2);
	#valueBuffer = new Uint8Array(3);

	constructor(options) {
		super(options);
		const io = this.#io = new (options.io)({
			...options,
			hz: 100_000,
			address: 0x76
		});

		this.#byteBuffer[0] = Register.BMP280_CHIPID;
		io.write(this.#byteBuffer);
		if (0x58 !== io.read(this.#byteBuffer)[0])
			throw new Error("unexpected sensor");

		this.initialize();
		this.configure(options);
	}
	configure(options) {
		const io = this.#io;
		const bBuf = this.#byteBuffer;
		const wBuf = this.#wordBuffer;

		bBuf[0] = Register.BMP280_CONFIG;
		io.write(bBuf);
		io.read(bBuf);
		let configReg = bBuf[0];

		bBuf[0] = Register.BMP280_CONTROL;
		io.write(bBuf);
		io.read(bBuf);
		let measureReg = bBuf[0];

		if (undefined != options.mode)
			measureReg = (measureReg & 0b1111_1100) | options.mode;
		if (undefined != options.pressureSampling)
			measureReg = (measureReg & 0b1110_0011) | (options.pressureSampling << 2);
		if (undefined != options.tempSampling)
			measureReg = (measureReg & 0b0001_1111) | (options.tempSampling << 5);
		if (undefined != options.standbyDuration)
			configReg = (configReg & 0b0001_1111) | (options.standbyDuration << 5);
		if (undefined != options.filter)
			configReg = (configReg & 0b1110_0011) | (options.filter << 2);


		wBuf[0] = Register.BMP280_CONFIG;
		wBuf[1] = configReg;
		io.write(wBuf);

		wBuf[0] = Register.BMP280_CONTROL;
		wBuf[1] = measureReg;
		io.write(wBuf);
	}
	calculate(rawTemp, rawPressure) @ "xs_bmp280_calculate";
	setCalibration(calibrate) @ "xs_bmp280_setCalibration";
	close() @ "xs_bmp280_close";
	sample() {
		let rawT = this.read24(Register.BMP280_TEMPDATA);
		let rawP = this.read24(Register.BMP280_PRESSUREDATA);
		return this.calculate(rawT, rawP);
	}
	initialize() {
		let calib = {};
		calib.T1 = this.read16LE(0x88);
		calib.T2 = this.readS16LE(0x8A);
		calib.T3 = this.readS16LE(0x8C);

		calib.P1 = this.read16LE(0x8E);
		calib.P2 = this.readS16LE(0x90);
		calib.P3 = this.readS16LE(0x92);
		calib.P4 = this.readS16LE(0x94);
		calib.P5 = this.readS16LE(0x96);
		calib.P6 = this.readS16LE(0x98);
		calib.P7 = this.readS16LE(0x9A);
		calib.P8 = this.readS16LE(0x9C);
		calib.P9 = this.readS16LE(0x9E);

		this.setCalibration(calib);
	}
	twoC(val) {
		if (val > 32767)
			val = -(65535 - val + 1);
		return val;
	}
	read16(reg) {
		const io = this.#io;
		const bBuf = this.#byteBuffer;
		const wBuf = this.#wordBuffer;
		bBuf[0] = reg;
		io.write(bBuf);
		io.read(wBuf);
		return (wBuf[0] << 8) | wBuf[1];
	}
	readS16(reg) {
		return this.twoC(this.read16(reg));
	}
	read16LE(reg) {
		let v = this.read16(reg);
		return ((v & 0xff00) >> 8) | ((v & 0xff) << 8);
	}
	readS16LE(reg) {
		return this.twoC(this.read16LE(reg));
	}
	read24(reg) {
		const io = this.#io;
		const bBuf = this.#byteBuffer;
		const vBuf = this.#valueBuffer;
		bBuf[0] = reg;
		io.write(bBuf);
		io.read(vBuf);
		return (vBuf[0] << 16) | (vBuf[1] << 8) | vBuf[2];
	}
}
Object.freeze(BMP280.prototype);

export { BMP280 as default, BMP280, Config };

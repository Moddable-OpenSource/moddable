/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Timer from "timer";

import I2C from "pins/i2c";

const BMP180_ADDR = 0x77

const BMP180_REG_CONTROL = 0xF4
const BMP180_REG_RESULT = 0xF6

const BMP180_COMMAND_TEMPERATURE = 0x2E
const BMP180_COMMAND_PRESSURE0 = 0x34
const BMP180_COMMAND_PRESSURE1 = 0x74
const BMP180_COMMAND_PRESSURE2 = 0xB4
const BMP180_COMMAND_PRESSURE3 = 0xF4

class BMP180 extends I2C {
	constructor(it) {
		super(Object.assign({address: BMP180_ADDR }, it));
		this.initialize();
	}
	readInt(address) {
		this.write(address);
		let buffer = this.read(2);
		return this.twoCompl((buffer[0] << 8) | buffer[1]);
	}
	readUInt(address) {
		this.write(address);
		let buffer = this.read(2);
		return (buffer[0] << 8) | buffer[1];
	}
	readBytes(address, count) {
		this.write(address);
		return this.read(count);
	}
	twoCompl(value) {
 		if (value > 32767) {
		 	value = -(65535 - value + 1);
		 } 
 		return value
	}
	initialize() {
		let AC1 = this.readInt(0xAA);
		let AC2 = this.readInt(0xAC);
		let AC3 = this.readInt(0xAE);
		let AC4 = this.readUInt(0xB0);
		let AC5 = this.readUInt(0xB2);
		let AC6 = this.readUInt(0xB4);
		let VB1 = this.readInt(0xB6);
		let VB2 = this.readInt(0xB8);
		let MB = this.readInt(0xBA);
		let MC = this.readInt(0xBC);
		let MD = this.readInt(0xBE);

		this.c3 = 160.0 * Math.pow(2,-15) * AC3;
		this.c4 = Math.pow(10,-3) * Math.pow(2,-15) * AC4;
		this.b1 = Math.pow(160,2) * Math.pow(2,-30) * VB1;
		this.c5 = (Math.pow(2,-15) / 160) * AC5;
		this.c6 = AC6;
		this.mc = (Math.pow(2,11) / Math.pow(160,2)) * MC;
		this.md = MD / 160.0;
		this.x0 = AC1;
		this.x1 = 160.0 * Math.pow(2,-13) * AC2;
		this.x2 = Math.pow(160,2) * Math.pow(2,-25) * VB2;
		this.y0 = this.c4 * Math.pow(2,15);
		this.y1 = this.c4 * this.c3;
		this.y2 = this.c4 * this.b1;
		this.p0 = (3791.0 - 8.0) / 1600.0;
		this.p1 = 1.0 - 7357.0 * Math.pow(2,-20);
		this.p2 = 3038.0 * 100.0 * Math.pow(2,-36);
	}
	sample() {
		this.startTemperature();
		Timer.delay(5);
		let temperature = this.getTemperature();
		this.startPressure();
		Timer.delay(5);
		let pressure = this.getPressure(temperature);
		return {temperature: Math.round(temperature), pressure: Math.round(pressure)};
	}
	startTemperature() {
		return this.write(BMP180_REG_CONTROL, BMP180_COMMAND_TEMPERATURE);
	}
	getTemperature() {
		let data = this.readBytes(BMP180_REG_RESULT, 2);

		let tu = (data[0] * 256.0) + data[1];
		let a = this.c5 * (tu - this.c6);
		return a + (this.mc / (a + this.md));
	}

	startPressure() {
		return this.write(BMP180_REG_CONTROL, BMP180_COMMAND_PRESSURE0);
	}
	getPressure(T) {
		let data = this.readBytes(BMP180_REG_RESULT, 3);

		let pu = (data[0] * 256.0) + data[1] + (data[2]/256.0);

		let s = T - 25.0;
		let x = (this.x2 * Math.pow(s,2)) + (this.x1 * s) + this.x0;
		let y = (this.y2 * Math.pow(s,2)) + (this.y1 * s) + this.y0;
		let z = (pu - x) / y;
		return (this.p2 * Math.pow(z,2)) + (this.p1 * z) + this.p0;
	}
}

Object.freeze(BMP180.prototype);

export default BMP180;


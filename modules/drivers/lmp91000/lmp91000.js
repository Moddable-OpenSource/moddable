/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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


/*	TI LMP91000
	Configurable AFE Potentiostat for Low-Power Chemical Sensing Applications
	http://www.ti.com/product/LMP91000

	If using multiple LMP91000's, each has an enable pin MENB, active low,
	which allows communication with a specific sensor.

	Instantiation:
	
	let sensor = new LMP91000({address: LMP91000_ADDR, enable: { "pin": 6, "port": "gpioPortB"} });


	Object methods: (* indicates default value)
	enable(x);
		// *0/1: enable this sensor (triggers enable pin)
	lock(x);
		// *1/0: locks or unlocks TIACN/REFCN
	setTIACN(tiaGain, rload);
		// tiaGain is one of: *0 (external), 2.75, 3.5, 7, 14
		//  				  35, 120, 350 (kOhms)
		// rload is one of:   10, 33, 50, *100 (Ohms)
	setREFCN(source, int_z, bias_sign, bias);
		// source is: *0 (internal), 1 (external) voltage reference
		// int_z is one of: 20, *50, 67, "bypass"  (% of source ref)
		// bias_sign is: *0 (negative), 1 (positive)
		// bias is one of: *0, 1, 2, 4, 6, 8, 10, 12, 14, 16, 18
		//					20, 22, 24 (% of source ref)
	sleepMode();
		// set lmp91000 mode to Deep Sleep
	standbyMode();
		// set lmp91000 mode to standby
	galvanicMode();
		// set lmp91000 to 2-lead ground referred galvanic cell
	amperometricMode();
		// set lmp91000 to 3-lead amperometric cell
	tiaMode(on);
		// on is:	0 - Temperature measurement (TIA OFF)
		//			1 - Temperature messurement (TIA ON)
	check();
		// trace values of registers
*/

import Digital from "pins/digital";
import I2C from "pins/i2c";

export const LMP91000_ADDR = 0x48

const LMP91000_REG_STATUS = 0x00
const LMP91000_REG_LOCK = 0x01
const LMP91000_REG_TIACN = 0x10
const LMP91000_REG_REFCN = 0x11
const LMP91000_REG_MODECN = 0x12

const LMP91000_MODE_SLEEP = 0x0
const LMP91000_MODE_GALVANIC = 0x1
const LMP91000_MODE_STANDBY = 0x2
const LMP91000_MODE_AMPEROMETRIC = 0x3
const LMP91000_MODE_TIA_OFF = 0x6
const LMP91000_MODE_TIA_ON = 0x7

export default class LMP91000 extends I2C {
	constructor(it) {
		super(it);
		this.enablePin = new Digital({ "pin": it.enable.pin, "port": it.enable.port, mode: Digital.Output});
		this.enablePin.write(0);		// enabled low
		this.initialize();
	}
	readByte(address) {
		this.write(address);
		return this.read(1);
	}
	initialize() {
		while (this.readByte(LMP91000_REG_STATUS) != 1)
			trace("waiting for LMP91000 to come ready\n");
		this.enablePin.write(1);
	}
	enable(x) {
		if (x)
			this.enablePin.write(0);
		else
			this.enablePin.write(1);
	}
	lock(x) {
		this.write(LMP91000_REG_LOCK, x);
	}
	setTIACN(tiaGain, rload) {
		let val = 0;
		switch (tiaGain) {
			case 0:		val = 0; break;		// default
			case 2.75:	val = 1; break;
			case 3.5:	val = 2; break;
			case 7:		val = 3; break;
			case 14:	val = 4; break;
			case 35:	val = 5; break;
			case 120:	val = 6; break;
			case 350:	val = 7; break;
			default:
				throw new Error(`bad tiaGain: ${tiaGain}`);
		}
		val <<= 2;
		switch (rload) {
			case 10:	val += 0; break;
			case 33:	val += 1; break;
			case 50:	val += 2; break;
			case 100:	val += 3; break;	// default
			default:
				throw new Error(`bad rload: ${rload}`);
		}
		this.lock(0);
		this.write(LMP91000_REG_TIACN, val);
		this.lock(1);
	}
	setREFCN(source, int_z, bias_sign, bias) {
		let val = source ? 0x80 : 0;
		let x;
		switch (int_z) {
			case 20:		x = 0; break;
			case 50:		x = 1; break;		// default 
			case 67:		x = 2; break;
			case "bypass":	x = 3; break;
			default:
				throw new Error(`bad int_z: ${int_z}`);
		}
		val += x << 5;
		if (bias_sign) val += 0x10;
		switch (bias) {
			case 0:		x = 0; break;		// default 
			case 1:		x = 1; break;
			case 2:		x = 2; break;
			case 4:		x = 3; break;
			case 6:		x = 4; break;
			case 8:		x = 5; break;
			case 10:	x = 6; break;
			case 12:	x = 7; break;
			case 14:	x = 8; break;
			case 16:	x = 9; break;
			case 18:	x = 10; break;
			case 20:	x = 11; break;
			case 22:	x = 12; break;
			case 24:	x = 13; break;
			default:
				throw new Error(`bad bias: ${bias}`);
		}
		val += x;
		this.lock(0);
		this.write(LMP91000_REG_REFCN, val);
		this.lock(1);
	}
	sleepMode() {
		this.write(LMP91000_REG_MODECN, LMP91000_MODE_SLEEP);
	}
	standbyMode() {
		this.write(LMP91000_REG_MODECN, LMP91000_MODE_STANDBY);
	}
	galvanicMode() {
		this.write(LMP91000_REG_MODECN, LMP91000_MODE_GALVANIC);
	}
	amperometricMode() {
		this.write(LMP91000_REG_MODECN, LMP91000_MODE_AMPEROMETRIC);
	}
	tiaMode(on) {
		if (on)
			this.write(LMP91000_REG_MODECN, LMP91000_MODE_TIA_ON);
		else
			this.write(LMP91000_REG_MODECN, LMP91000_MODE_TIA_OFF);
	}

	check() {
		let stat = this.readByte(LMP91000_REG_STATUS);
		let lock = this.readByte(LMP91000_REG_LOCK);
		let tiacn = this.readByte(LMP91000_REG_TIACN);
		let refcn = this.readByte(LMP91000_REG_REFCN);
		let modecn = this.readByte( LMP91000_REG_MODECN);
		trace(`stat: ${stat} lock:${lock} tiacn:${tiacn} refcn:${refcn} modecn:${modecn}\n`);
	}
}

Object.freeze(LMP91000.prototype);

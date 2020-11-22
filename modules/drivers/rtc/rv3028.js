/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
	RV-3028-C7 - Micro Crystal Extreme Low Power Real-Time Clock Module
*/

import RTC from "rtc";
import I2C from "pins/i2c";

const RV3028_ADDR = 0x52;

const RV3028_STATUS = 0x0E;
const  RV3028_BACKUP_SWICHOVER = 0b100000;
const  RV3028_POWER_ON_RESET =   0b000001;
const RV3028_UNIX = 0x1B;

class RV3028 extends RTC {
	constructor(dictionary) {
		super(Object.assign({address: RV3028_ADDR}, dictionary));
		let foo = super.readByte(RV3028_ADDR, 1);
	}

	_get_seconds() {
		this.block = super.readBlock(RV3028_UNIX, 4);
		this.unixTime = (this.block[3] << 24) | (this.block[2] << 16) | (this.block[1] << 8) | this.block[0];
//trace(`block[3-0]: ${this.block[3]}, ${this.block[2]}, ${this.block[1]}, ${this.block[0]} - unixTime: ${this.unixTime}\n`);
		return this.unixTime;
	}

	set seconds(v) {
		let buf = new ArrayBuffer(4*1);
		let bytes = new Uint8Array(buf);
		bytes[0] = v & 0xff;
		bytes[1] = (v & 0xff00) >> 8;
		bytes[2] = (v & 0xff0000) >> 16;
		bytes[3] = (v & 0xff000000) >> 24;
		super.writeBlock(RV3028_UNIX, buf);
		this.enabled = true;
		return this.unixTime;
	}

	get seconds() {
		let curTime = this._get_seconds();
		let lastCheck;
		do {
			lastCheck = curTime;
			curTime = this._get_seconds();
		} while (lastCheck != curTime);
		return curTime;
	}
/*
	traceBlock(str, bytes) {
		trace(`${str}: sec:${this.bcdStr(bytes[0])}, min:${this.bcdStr(bytes[1])}, hours:${this.bcdStr(bytes[2])}, DOW:${this.bcdStr(bytes[3])}, date:${this.bcdStr(bytes[4])}, month:${this.bcdStr(bytes[5]&0x7f)}, year:${this.bcdStr(bytes[6]+((bytes[5]&0x80)?100:0))}\n`);
	}
*/

	get _enabled() {
		let status = super.readByte(RV3028_STATUS);
		if (status & RV3028_POWER_ON_RESET)
			return 0;
		return 1;
	}

	set _enabled(e) {
		let c = super.readByte(0);
		c = (c & 0xfe) | (0 == e);		// set last bit to 1 to note disabled
		super.writeByte(RV3028_STATUS, c);
	}
	static probe() {
		const i = new I2C({address: RV3028_ADDR, throw: false});
		const result = i.read(1);
		i.close();
		return undefined !== result;
	}
}
Object.freeze(RV3028.prototype);

export default RV3028;

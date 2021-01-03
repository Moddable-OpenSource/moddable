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
	ds1307 realtime clock
*/

import RTC from "rtc";
import I2C from "pins/i2c";

const DS1307_ADDR = 0x68;

class DS1307 extends RTC {
	constructor(dictionary) {
		super(Object.assign({address: DS1307_ADDR}, dictionary));
		let foo = super.readByte(DS1307_ADDR, 1);
	}

	traceBlock(str, bytes) {
		trace(`${str}: sec:${this.bcdStr(bytes[0])}, min:${this.bcdStr(bytes[1])}, hours:${this.bcdStr(bytes[2])}, DOW:${this.bcdStr(bytes[3])}, date:${this.bcdStr(bytes[4])}, month:${this.bcdStr(bytes[5]&0x7f)}, year:${this.bcdStr(bytes[6]+((bytes[5]&0x80)?100:0))}\n`);
	}

	get _enabled() {
		return (super.readByte(0) & 0x80) == 0;
	}

	set _enabled(e) {
		let c = super.readByte(0);
		c = (c & 0x7f) | ((0 == e) << 7);
		super.writeByte(0, c);
	}

	_getDate() {
		let date = {};
		this.block = super.readBlock(0, 7);
		date.year = this.bcdToDec(this.block[6]);			// year
		date.month = this.bcdToDec(this.block[5]);			// month
		date.date = this.bcdToDec(this.block[4]);			// day
		date.year += 1970;
		date.seconds = this.bcdToDec(this.block[0] & 0x7f);		// seconds
		date.minutes = this.bcdToDec(this.block[1]);				// minutes
		date.hours = this.bcdToDec(this.block[2] & 0x3f);	// hours
		return date;
	}

	_setDate(date) {
		let buf = new ArrayBuffer(7*1);
		let bytes = new Uint8Array(buf);

		bytes[0] = this.decToBcd(date.seconds);
		bytes[1] = this.decToBcd(date.minutes);
		bytes[2] = this.decToBcd(date.hours);
		bytes[3] = this.decToBcd(date.dow);
		bytes[4] = this.decToBcd(date.date);
		bytes[5] = this.decToBcd(date.month)
		bytes[6] = this.decToBcd(date.year - 1970);

		super.writeBlock(0, buf);
	}
	static probe() {
		const i = new I2C({address: DS1307_ADDR, throw: false});
		const result = i.read(1);
		i.close();
		return undefined !== result;
	}
}
Object.freeze(DS1307.prototype);

export default DS1307;

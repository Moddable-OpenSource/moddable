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
	DS3231 Extremely Accurate I2C-Integrated RTC/TCXO/Crystal
	https://datasheets.maximintegrated.com/en/ds/DS3231.pdf
*/

import RTC from "rtc";
import I2C from "pins/i2c";

const DS3231_ADDR = 0x68;
const DS3231_TIME_CAL_ADDR = 0x00;
const DS3231_CONTROL_ADDR = 0x0E;
const DS3231_STATUS_ADDR = 0x0F;
const DS3231_TEMP_ADDR = 0x11;

const DS3231_CONTROL_CONV_BIT = 0x20;		// convert temp
const DS3231_CONTROL_EOSC_BIT = 0x80;		// enable (0 == on)
const DS3231_EOSC_BIT_SHIFT = 7;

const DS3231_2412_BIT = 0x40;			// 1 == 12 hour
const DS3231_CENTURY_BIT = 0x80;		// 

class DS3231 extends RTC {
	constructor(dictionary) {
		super(Object.assign({address: DS3231_ADDR}, dictionary));
		let foo = super.readByte(0);
	}

	traceBlock(str, bytes) {
		let y = this.bcdToDec(bytes[6]);
		if (bytes[5]&DS3231_CENTURY_BIT)
			y += 100;
		trace(`${str}: sec:${this.bcdStr(bytes[0])}, min:${this.bcdStr(bytes[1])}, hours:${this.bcdStr(bytes[2])}, DOW:${this.bcdStr(bytes[3])}, date:${this.bcdStr(bytes[4])}, month:${this.bcdStr(bytes[5]&0x7f)}, year:${y}\n`);
	}

	get _enabled() {
		return (super.readByte(DS3231_CONTROL_ADDR) & DS3231_CONTROL_EOSC_BIT) == 0;
	}

	set _enabled(e) {
		let c = super.readByte(DS3231_CONTROL_ADDR);
		c = (c & ~DS3231_CONTROL_EOSC_BIT) | ((0 == e) << DS3231_EOSC_BIT_SHIFT);
		super.writeByte(DS3231_CONTROL_ADDR, c);
	 }

	_getDate() {
		let date = {};
		let m;
		this.block = super.readBlock(DS3231_TIME_CAL_ADDR, 7);
		date.year = this.bcdToDec(this.block[6]) + 1900;	// year
		m = this.block[5];
		if (m & DS3231_CENTURY_BIT) {
			date.year += 100;
		}
		date.month = this.bcdToDec(m & 0x7f);				// month
		date.date = this.bcdToDec(this.block[4]);			// day
		date.seconds = this.bcdToDec(this.block[0]);				// seconds
		date.minutes = this.bcdToDec(this.block[1]);				// minutes
		date.hours = this.bcdToDec(this.block[2] & 0x3F);	// hours
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
		bytes[5] = this.decToBcd(date.month) | ((date.year > 2000) ? DS3231_CENTURY_BIT : 0);
		bytes[6] = this.decToBcd(date.year % 100);
	
		super.writeBlock(DS3231_TIME_CAL_ADDR, buf);
	}
	static probe() {
		const i = new I2C({address: DS3231_ADDR, throw: false});
		const result = i.read(1);
		i.close();
		return undefined !== result;
	}
}
Object.freeze(DS3231.prototype);

export default DS3231;

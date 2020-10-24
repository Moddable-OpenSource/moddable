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
	bm8563 realtime clock
	(c) Wilbeforce 2020
*/

import RTC from "rtc";

const BM8563_ADDR = 0x51;
const BM8563_CONTROL_STATUS1 = 0x00;
const BM8563_CONTROL_STATUS2 = 0x01;
const BM8563_SECONDS = 0x02;
const BM8563_CENTURY_BIT = 0b10000000;
const BM8563_STOP = 0b00100000;

class BM8563 extends RTC {
	constructor(dictionary) {
		super(Object.assign({
			address: BM8563_ADDR
		}, dictionary));
		// Status reset (twice)
		super.writeByte(BM8563_CONTROL_STATUS1, 0x00);
		super.writeByte(BM8563_CONTROL_STATUS1, 0x00);
		// Status2 reset
		super.writeByte(BM8563_CONTROL_STATUS2, 0x00);

	}

	get _enabled() {
		return (super.readByte(BM8563_CONTROL_STATUS1) & BM8563_STOP) != 0;
	}

	set _enabled(e) {
		let c = e ? 0 : BM8563_STOP;
		super.writeByte(BM8563_CONTROL_STATUS1, c);
	}

	_getDate() {
		let date = {};
		this.block = super.readBlock(BM8563_SECONDS, 7);
		let century = (this.block[5] & BM8563_CENTURY_BIT) ? 100 : 0;
		date.year = this.bcdToDec(this.block[6]) + century + 1900; // year
		date.month = this.bcdToDec(this.block[5] & 0b00011111); // month
		date.date = this.bcdToDec(this.block[3]); // day
		date.seconds = this.bcdToDec(this.block[0] & 0x7f); // seconds
		date.minutes = this.bcdToDec(this.block[1]); // minutes
		date.hours = this.bcdToDec(this.block[2] & 0x3f); // hours
		return date;
	}

	_setDate(date) {
		let buf = new ArrayBuffer(7 * 1);
		let bytes = new Uint8Array(buf);

		bytes[0] = this.decToBcd(date.seconds);
		bytes[1] = this.decToBcd(date.minutes);
		bytes[2] = this.decToBcd(date.hours);
		bytes[3] = this.decToBcd(date.date);
		bytes[4] = this.decToBcd(date.dow);
		bytes[5] = this.decToBcd(date.month);
		if (date.year >= 100) {
			bytes[5] |= BM8563_CENTURY_BIT;
		}
		bytes[6] = this.decToBcd(date.year % 100);

		super.writeBlock(BM8563_SECONDS, buf);
	}

}
Object.freeze(BM8563.prototype);

export default BM8563;
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

import SMBus from "pins/smbus";

// dictionary should contain 'address' - DS3231 and DS1307 are 0x68

class RTC extends SMBus {
	constructor(dictionary) {
		super(dictionary);
	}

	get _enabled() {}
	set _enabled(e) {}

	get enabled() {
		return this._enabled;
	}

	set enabled(e) {
		this._enabled = e;
	}

	_getDate() {
		return {
			year: 0,
			month: 1,
			date: 1,
			dow: 1,
			seconds: 0,
			minutes: 0,
			hours: 0
		};
	}
	_setDate(date) {}

	decToBcd(d) {
		let val = d | 0;
		let v = val / 10 | 0;
		v *= 16;
		v += val % 10;
		return v;
	}

	bcdToDec(val) {
		let v = val / 16 | 0;
		v *= 10;
		v += val % 16;
		return v;
	}

	bcdStr(v) {
		let a = (v & 0xf0) >> 4;
		let b = (v & 0x0f);
		return `${a}${b}`;
	}

	get seconds() {
		let date = this._getDate();
		let when = new Date(date.year, date.month, date.date, date.hours, date.minutes, date.seconds, 0);
		return when.getTime() / 1000;
	}

	set seconds(secs) {
		let d = new Date(secs * 1000);
		let date = {
			date: d.getDate(),
			month: d.getMonth(),
			year: d.getFullYear(),
			dow: d.getDay(),
			hours: d.getHours(),
			minutes: d.getMinutes(),
			seconds: d.getSeconds()
		};
		this._setDate(date);
	}

}

Object.freeze(RTC);
export default RTC;
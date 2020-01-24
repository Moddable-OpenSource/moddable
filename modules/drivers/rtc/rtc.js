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
import Timer from "timer";

const DAYS_1_YEAR = 365;
const DAYS_4_YEARS = (4 * DAYS_1_YEAR) + 1;
const DAYS_100_YEARS = (25 * DAYS_4_YEARS) - 1;
const SECONDS_IN_DAY = 86400;
const EPOCH_YEAR = 1970;
const monthsDays = [ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 ];

// dictionary should contain 'address' - DS3231 and DS1307 are 0x68

class RTC extends SMBus {
	constructor(dictionary) {
		super(dictionary);
	}

	get _enabled() { }
	set _enabled(e) { }

	get enabled() {
		return this._enabled;
	}

	set enabled(e) {
		this._enabled = e;
	 }

	_getDate() { return { year: 0, month: 1, date: 1, dow: 1, seconds: 0, minutes: 0, hours: 0}; }
	_setDate(date) { }

	decToBcd(d) {
		let val = d|0;
		let v = val/10|0;
		v *= 16;
		v += val%10;
		return v;
	}

	bcdToDec(val) {
		let v = val/16|0;
		v *= 10;
		v += val%16;
		return v;
	}

	isLeapYear(year) { return ((year & 3) == 0 &&  (year % 100 != 0 || ((year / 100)&3) == (-(YEAR_OFFSET/100)&3))); }

	bcdStr(v) {
		let a = (v&0xf0)>>4;
		let b = (v&0x0f);
		return `${a}${b}`;
	}

	get seconds() {
		let date = this._getDate();

		let year, time, mons, years, leapyears;

		year = date.year - 1900;
		if (date.month < 0) {
			mons = -date.month - 1;
			year -= 1 + (mons/12)|0;
			date.month = 11 - (mons%12);
		}
		else if (date.month > 11) {
			mons = date.month;
			year += (mons/12)|0;
			date.month = mons % 12;
		}

		if (year < 70 || year > 139) {
			throw("fail");
		}

		years = year - 70;
		leapyears = ((years + 1)/4)|0 ;
		time = years * 365 + leapyears;
		time += monthsDays[date.month];

		if (((years + 2) % 4) == 0) {
			if (date.month > 2) {
				time++;
			}
		}

		time += date.date - 1;
		time *= 24;
		time += date.hours;
		time *= 60;
		time += date.minutes;
		time *= 60;
		time += date.seconds;

		return time;
	}

	set seconds(secs) {
		let date = {};
		let day, time, year, month;
		
		day = (secs / SECONDS_IN_DAY)|0;
		time = secs % SECONDS_IN_DAY;
		if (time < 0) {
			day--;
			time += SECONDS_IN_DAY;
		}
		date.dow = (day + 4) % 7;		// day of week

		date.seconds = time%60;
		time = (time / 60) | 0;
		date.minutes = time%60;
		time = (time / 60) | 0;
		date.hours = time;

		year = EPOCH_YEAR;
		year += 100 * ((day / DAYS_100_YEARS)|0);
		day %= DAYS_100_YEARS;
		day++;
		year += 4 * ((day / DAYS_4_YEARS)|0);
		day %= DAYS_4_YEARS;
		day--;
		year += (day / DAYS_1_YEAR)|0;
		day %= DAYS_1_YEAR;
		day += this.isLeapYear(year);
		for (month = 0; month < 11; month++) {
			if (day < monthsDays[month + 1])
				break;
		}
		day -= monthsDays[month];
		date.date = day + 1;		// day of month
		date.month = month;
		date.year = year;
	
		this._setDate(date);
	}

}

Object.freeze(monthsDays);
Object.freeze(RTC);
export default RTC;

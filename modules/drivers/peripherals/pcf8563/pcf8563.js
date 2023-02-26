/*
 * Copyright (c) 2021-2023 Moddable Tech, Inc.
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
	NXP PCF8563 - Real-time clock/calendar
	https://www.nxp.com/docs/en/data-sheet/PCF8563.pdf
*/

const Register = Object.freeze({
	CTRL1:			0x00,
	CTRL2:			0x01,
	TIME:			0x02,
	ALARM_MINUTES:	0x09,
	ALARM_HOURS:	0x0a,
	ALARM_DAY:		0x0b,
	ALARM_WEEKDAY:	0x0c,

	VALID_BIT:		0x80,
	CENTURY_BIT:	0x80
});

const AlarmRange = 60 * 60 * 24 * 31 * 1000;

class PCF8563 {
	#io;
	#onAlarm;
	#blockBuffer = new Uint8Array(7);

	constructor(options) {
		const { clock, interrupt, onAlarm } = options;
		const io = this.#io = new clock.io({
			hz: 400_000,
			address: 0x51,
			...clock
		});

		try {
			io.readUint8(0);
		}
		catch(e) {
			io.close();
			throw e;
		}

		if (interrupt && onAlarm) {
			this.#onAlarm = onAlarm;
			io.interrupt = new interrupt.io({
				mode: interrupt.io.InputPullUp,
				...interrupt,
				edge: interrupt.io.Falling,
				onReadable: () => {
					this.#io.writeUint8(Register.CTRL2, 0);	//  clear alarm, disable interrupt
					this.#onAlarm();
				}
			});
		}
	}
	close() {
		this.#io?.interrupt?.close();
		this.#io?.close();
		this.#io = undefined;
	}
	configure(options) {
		if (options?.alarm !== undefined) {
			const time = options.alarm;
			
			let io = this.#io;
			let now = this.time;
	
			if (!time) {
				io.writeUint8(Register.CTRL2, 0);	//  clear alarm, disable interrupt
				return;
			}

			if (time - now > AlarmRange)
				throw new Error;
	
			let future = new Date(time);
			future.setUTCSeconds(0);
			io.writeUint8(Register.ALARM_MINUTES, decToBcd(future.getUTCMinutes()));
			io.writeUint8(Register.ALARM_HOURS, decToBcd(future.getUTCHours()));
			io.writeUint8(Register.ALARM_DAY, decToBcd(future.getUTCDate()));
			io.writeUint8(Register.ALARM_WEEKDAY, 0x80);		// disable
	
			io.writeUint8(Register.CTRL2, 0b0001_0010);	// pulse interrupt, clear alarm, enable interrupt
		}
	}
	get configuration() {
		let io = this.#io;
		let now = new Date(this.time);

		const alarmStatus = io.readUint8(Register.CTRL2) & 0b0000_0010;
		if (!alarmStatus)
			return {alarm: 0}; // alarm is disabled

		now.setUTCSeconds(0);
		now.setUTCMinutes( bcdToDec(io.readUint8(Register.ALARM_MINUTES) & 0x7f) );
		now.setUTCHours( bcdToDec(io.readUint8(Register.ALARM_HOURS) & 0x3f) );

		let date = bcdToDec(io.readUint8(Register.ALARM_DAY) & 0x3f);
		if (date < now.getUTCDate()) {
			let month = now.getUTCMonth() + 1;
			if (month > 11) {
				month = 0;
				now.setUTCFullYear(now.getUTCFullYear() + 1);
			}
			now.setUTCMonth(month);
		}
		now.setUTCDate( date );

		return {alarm: now.getTime()};
	}
	get time() {
		const io = this.#io;
		const reg = this.#blockBuffer;

		io.readBuffer(Register.TIME, reg);

		if (reg[0] & Register.VALID_BIT) // if high bit of seconds is set, then time is uncertain
			return undefined;

		if (this.#io.readUint8(Register.CTRL1) & 0b0010_0000) // check if disabled
			return undefined;

		// yr, mo, day, hr, min, sec
		return Date.UTC(
			bcdToDec(reg[6]) + ((reg[5] & Register.CENTURY_BIT) ? 2100 : 2000),
			bcdToDec(reg[5] & 0x7f) - 1,
			bcdToDec(reg[3]),
			bcdToDec(reg[2]),
			bcdToDec(reg[1]),
			bcdToDec(reg[0] & 0x7f) );
	}
	set time(v) {
		let io = this.#io;
		let b = this.#blockBuffer;

		let now = new Date(v);
		let year = now.getUTCFullYear();

		if (year < 2000)
			throw new Error;

		b[0] = decToBcd(now.getUTCSeconds());
		b[1] = decToBcd(now.getUTCMinutes());
		b[2] = decToBcd(now.getUTCHours());
		b[3] = decToBcd(now.getUTCDate());
		b[4] = decToBcd(now.getUTCDay());
		b[5] = decToBcd(now.getUTCMonth() + 1) | (year > 2099 ? Register.CENTURY_BIT : 0);
		b[6] = decToBcd(year % 100);

		io.writeBuffer(Register.TIME, b);

		io.writeUint16(Register.CTRL1, 0);			// enable
	}
}

function decToBcd(d) {
	let v = Math.idiv(d, 10);
	v *= 16;
	v += Math.imod(d, 10);
	return v;
}
function bcdToDec(b) {
	let v = Math.idiv(b, 16);
	v *= 10;
	v += Math.imod(b, 16);
	return v;
}

export default PCF8563;


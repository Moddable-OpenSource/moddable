/*
 * Copyright (c) 2021-2023  Moddable Tech, Inc.
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
	NXP PCF85063 - Real-time clock/calendar
	https://www.nxp.com/docs/en/data-sheet/PCF85063A.pdf
*/

const Register = Object.freeze({
	CTRL1:			0x00,
	CTRL2:			0x01,
	TIME:			0x04,
	ALARM_SECONDS:	0x0b,
	ALARM_MINUTES:	0x0c,
	ALARM_HOURS:	0x0d,
	ALARM_DAY:		0x0e,
	ALARM_WEEKDAY:	0x0f,

	VALID_BIT:		0x80
});

const AlarmRange = 60 * 60 * 24 * 31 * 1000;

class PCF85063 {
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
			let io = this.#io;
			let now = this.time;
			const v = options.alarm;

			if (!v) {
				io.writeUint8(Register.CTRL2, 0);	//  clear alarm, disable interrupt
				return;
			}

			if (v - now > AlarmRange)
				throw new Error;

			let future = new Date(v);
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
		const b = this.#blockBuffer;

		io.readBuffer(Register.TIME, b);

		if (b[0] & Register.VALID_BIT) // if high bit of seconds is set, then time is uncertain
			return;

		if (this.#io.readUint8(Register.CTRL1)) // not enabled
			return undefined;

		// yr, mo, day, hr, min, sec
		return Date.UTC(
			bcdToDec(b[6]) + 2000,
			bcdToDec(b[5]) - 1,
			bcdToDec(b[3]),
			bcdToDec(b[2]),
			bcdToDec(b[1]),
			bcdToDec(b[0]));
	}
	set time(v) {
		const io = this.#io;
		const b = this.#blockBuffer;

		const now = new Date(v);
		const year = now.getUTCFullYear();

		if ((year < 2000) || (year >= 2100))
			throw new Error;

		b[0] = decToBcd(now.getUTCSeconds());
		b[1] = decToBcd(now.getUTCMinutes());
		b[2] = decToBcd(now.getUTCHours());
		b[3] = decToBcd(now.getUTCDate());
		b[4] = decToBcd(now.getUTCDay());
		b[5] = decToBcd(now.getUTCMonth() + 1);
		b[6] = decToBcd(year % 100);

		io.writeBuffer(Register.TIME, b);

		io.writeUint8(Register.CTRL1, 0);			// enable
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

export default PCF85063;

/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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
	TI TMP117
	Digital temperature sensor and thermal watchdog
	https://www.ti.com/lit/ds/symlink/tmp117.pdf
*/

import device from "embedded:provider/builtin";
import SMBus from "embedded:io/smbus";
const Digital = device.io.Digital;

import Timer from "timer";

const Register = Object.freeze({
	TMP117_TEMP:	0x00,
	TMP117_CONF:	0x01,
	TMP117_THI:		0x02,
	TMP117_TLO:		0x03,
	TMP117_CHIPID:	0x0F
});

const StatusMask = Object.freeze({
	ALERT_HIGH:		0b1000_0000_0000_0000,
	ALERT_LOW:		0b0100_0000_0000_0000,
	DATA_READY:		0b0010_0000_0000_0000,
	EEPROM_BUSY:	0b0001_0000_0000_0000
});
const ConfigMask = Object.freeze({
	MODE:			0b0000_1100_0000_0000,
	CONV:			0b0000_0011_1000_0000,
	AVE:			0b0000_0000_0110_0000,
	THERM:			0b0000_0000_0001_0000,
	POL:			0b0000_0000_0000_1000,
	DR:				0b0000_0000_0000_0100,
	SOFT_RESET:		0b0000_0000_0000_0010
});

class Temperature {
	#io;
	#onAlert;
	#irqMonitor;
	#status;

	constructor(options) {
		const io = this.#io = new SMBus({
			...options,
			hz: 100_000, 
			address: 0x48
		});

		let conf = io.readWord(Register.TMP117_CHIPID, true);
		if (0x117 != (conf & 0x0fff))
			throw new Error("unexpected sensor");

		conf = io.readWord(Register.TMP117_CONF, true);
		conf |= ConfigMask.SOFT_RESET;		// softreset
		io.writeWord(Register.TMP117_CONF, conf, true);
		
		if (undefined !== options.target)
			this.target = options.target;

		try {
			if (options.alert && options.onAlert) {
				this.#onAlert = options.onAlert;
				this.#irqMonitor = new Digital({
					pin: options.alert.pin,
					mode: Digital.Input,
					edge: Digital.Falling,
					onReadable: this.onAlert});
//					onReadable: options.onAlert});
				this.#irqMonitor.target = this;
			}

			this.configure({
				shutdownMode: false,
				thermostatMode: this.#onAlert ? "interrupt" : "comparator",
				polarity: 0,
				averaging: 8
			});
		}
		catch(e) {
			this.close();
			throw e;
		}
		this.#status = "ready";
	}
	onAlert() {
		this.target.status;	// clear alert
		this.target.callAlert();
	}
	callAlert() {
		this.#onAlert();
	}
	get status() { return this.#io.readWord(Register.TMP117_CONF, true); }

	shutdown() {
		let conf = this.#io.readWord(Register.TMP117_CONF, true);
		conf &= ~ConfigMask.MODE;
		this.#io.writeWord(Register.TMP117_CONF, conf, true);
	}
	close() {
		if ("ready" === this.#status) {
			this.#io.writeByte(Register.TMP117_CONF, 1);		// shut down device
			this.#status = undefined;
		}
		this.#irqMonitor?.close();
		this.#irqMonitor = undefined;
	}
	configure(options) {
		const io = this.#io;
		let conf = io.readWord(Register.TMP117_CONF, true);

		if (undefined !== options.shutdownMode) {
			conf &= ~ConfigMask.MODE;
			if (options.shutdownMode)
				conf |= 0b0000_0100_0000_0000;
		}
		if (undefined !== options.thermostatMode) {
			conf &= ~ConfigMask.THERM;
			if (options.thermostatMode === "interrupt")
				conf |= 0b1_0000;
			else if (options.thermostatMode !== "comparator")
				throw new Error("bad thermostatMode");
		}
		if (undefined !== options.polarity) {
			conf &= ~ConfigMask.POL;
			if (options.polarity)
				conf |= 0b1000;
		}
		if (undefined !== options.averaging) {
			conf &= ~ConfigMask.AVG;
			switch (options.averaging) {
				case 0: break;
				case 8:  conf |= 0b0010_0000; break;
				case 32: conf |= 0b0100_0000; break;
				case 64: conf |= 0b0110_0000; break;
				default: throw new Error("invalid averaging");
			}
		}

		if (undefined !== options.alert?.highTemperature) {
			const temp = options.alert.highTemperature / .0078125;
			io.writeWord(Register.TMP117_THI, temp, true);
		}

		if (undefined !== options.alert?.lowTemperature) {
			const temp = options.alert.lowTemperature / .0078125;
			io.writeWord(Register.TMP117_TLO, temp, true);
		}

		io.writeByte(Register.TMP117_CONF, conf);
	}


	sample() {
		const io = this.#io;

		const value = twoC(io.readWord(Register.TMP117_TEMP, true));

		let sign = 1;
		if (1 === (value & 0b1000_0000_0000_0000))
			sign = -1;

		return { temperature: value * sign * 0.0078125 };
	}
}

function twoC(a) {
	return ((a & 0x8000) ? (a | 0xffff0000) : a);
}

export default Temperature;

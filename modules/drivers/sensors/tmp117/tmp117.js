/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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

import Timer from "timer";

const Register = Object.freeze({
	TMP117_TEMP:	0x00,
	TMP117_CONF:	0x01,
	TMP117_THI:		0x02,
	TMP117_TLO:		0x03,
	TMP117_CHIPID:	0x0F
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

const SHUTDOWN_MODE = 0b0000_0100_0000_0000;		// Shutdown mode
const ONE_SHOT = 0b0000_1100_0000_0000;
const ONE_SHOT_DELAY = Object.freeze([ 16, 15.5*8, 15.5*32, 15.5*64 ]);
const HIGH_ALERT = 0b1000_0000_0000_0000;
const LOW_ALERT = 0b0100_0000_0000_0000;
const DATA_READY = 0b0010_0000_0000_0000;

class TMP117 {
	#io;
	#onAlert;
	#onError;
	#monitor;
	#status;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 400_000, 
			address: 0x48,
			...options.sensor
		});

		this.#onError = options.onError;

		let conf = io.readUint16(Register.TMP117_CHIPID, true);
		if (0x117 != (conf & 0x0fff)) {
			this.#onError?.("unexpected sensor");
			this.#io.close();
			this.#io = undefined;
			return;
		}

		conf = io.readUint16(Register.TMP117_CONF, true);
		conf |= ConfigMask.SOFT_RESET;		// softreset
		io.writeUint16(Register.TMP117_CONF, conf, true);
		
		const {alert, onAlert} = options;
		if (alert && onAlert) {
			this.#onAlert = options.onAlert;
			this.#monitor = new alert.io({
				mode: alert.io.InputPullUp,
				...alert,
				edge: alert.io.Falling,
				onReadable: () => this.#onAlert()
			});
		}

		// Reset to default: DataSheet 7.6.3
		io.writeUint16(Register.TMP117_CONF, 0b0000_0010_0010_0000, true);

		// DataSheet 7.6.4, 7.6.5
		io.writeUint16(Register.TMP117_THI, 0b0110_0000_0000_0000, true);
		io.writeUint16(Register.TMP117_TLO, 0b1000_0000_0000_0000, true);

		this.#status = "ready";
	}
	configure(options) {
		const io = this.#io;
		let conf = io.readUint16(Register.TMP117_CONF, true);

		if (undefined !== options.shutdownMode) {
			conf &= ~ConfigMask.MODE;
			if (options.shutdownMode) {
				conf |= 0b0000_0100_0000_0000;
				this.#status = "shutdown";
			}
			else
				this.#status = "ready";
		}

		if (undefined !== options.highTemperature)
			io.writeUint16(Register.TMP117_THI, (options.highTemperature / .0078125) | 0, true);

		if (undefined !== options.lowTemperature)
			io.writeUint16(Register.TMP117_TLO, (options.lowTemperature / .0078125) | 0, true);

		if (undefined !== options.thermostatMode) {
			conf &= ~ConfigMask.THERM;
			if (options.thermostatMode === "interrupt")
				conf |= 0b1_0000;
			else if (options.thermostatMode !== "comparator")
				this.#onError?.("bad thermostatMode");
		}

		if (undefined !== options.conversionRate) {
			conf &= ~ConfigMask.CONV;
			conf |= (options.conversionRate & 0b111) << 7;
		}

		if (undefined !== options.polarity) {
			conf &= ~ConfigMask.POL;
			if (options.polarity)
				conf |= ConfigMask.POL;
		}
		if (undefined !== options.averaging) {
			conf &= ~ConfigMask.AVG;
			switch (options.averaging) {
				case 0: break;
				case 8:  conf |= 0b0010_0000; break;
				case 32: conf |= 0b0100_0000; break;
				case 64: conf |= 0b0110_0000; break;
				default: this.#onError?.("invalid averaging");
			}
		}

		io.writeUint16(Register.TMP117_CONF, conf, true);
	}
	close() {
		if ("ready" === this.#status) {
			const io = this.#io;
			let conf = io.readUint16(Register.TMP117_CONF, true);
			conf &= ~ConfigMask.MODE;
			conf |= SHUTDOWN_MODE;
			io.writeUint16(Register.TMP117_CONF, conf, true);
			this.#status = undefined;
		}
		this.#monitor?.close();
		this.#monitor = undefined;
		this.#io.close();
		this.#io = undefined;
	}
	#twoC16(val) {
		return (val > 32768) ?  -(65535 - val + 1) : val;
	}
	sample() {
		const io = this.#io;
		let conf = io.readUint16(Register.TMP117_CONF, true);
		if ((conf & ConfigMask.MODE) == SHUTDOWN_MODE) {
			conf |= ONE_SHOT;
			io.writeUint16(Register.TMP117_CONF, conf, true);
			Timer.delay(ONE_SHOT_DELAY[(conf & ConfigMask.AVE) >> 5]);
			conf &= ~ConfigMask.MODE;
			io.writeUint16(Register.TMP117_CONF, conf | SHUTDOWN_MODE, true);
		}

		let value = this.#twoC16(io.readUint16(Register.TMP117_TEMP, true)) * 0.0078125;

		conf = io.readUint16(Register.TMP117_CONF, true);
		let alert = 0;
		if (conf & HIGH_ALERT)
			alert |= 0b01;
		if (conf & LOW_ALERT)
			alert |= 0b10;

		return { temperature: value, alert };
	}
}

export default TMP117;

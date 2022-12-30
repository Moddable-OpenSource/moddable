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
	NXP Semiconductors LM75B
	Digital temperature sensor and thermal watchdog
	https://www.nxp.com/docs/en/data-sheet/LM75B.pdf
*/

import Timer from "timer";

const Register = Object.freeze({
	LM75_TEMP:	0x00,
	LM75_CONF:	0x01,
	LM75_THYST:	0x02,
	LM75_TOS:	0x03
});

class LM75 {
	#io;
	#onAlert;
	#onError;
	#TOS;
	#THYST;
	#monitor;
	#status = "ready";

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 400_000, 
			address: 0x48,
			...options.sensor
		});

		this.#onError = options.onError;

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

		// reset to default values (7.4.2 of datasheet)
		io.writeUint8(Register.LM75_CONF, 0);
		

		// Tth(ots) = +80C Thys = +75C (7.9)
		this.#TOS = 80;
		io.writeUint16(Register.LM75_TOS, 160 << 7, true);	// half degrees
		this.#THYST = 75;
		io.writeUint16(Register.LM75_THYST, 150 << 7, true);	// half degrees
	}

	configure(options) {
		const io = this.#io;

		let conf = io.readUint8(Register.LM75_CONF) & 0b0001_1111;

		if (undefined !== options.shutdownMode) {
			conf &= ~0b1;
			if (options.shutdownMode) {
				conf |= 0b1;
				this.#status = "shutdown";
			}
		}

		const highT = options.highTemperature;
		if (undefined !== highT) {
			const value = (((highT > 125) ? 125 : (highT < -55) ? -55 : highT) * 2) | 0;	// half degrees
			io.writeUint16(Register.LM75_TOS, value << 7, true);
			this.#TOS = value/2;
		}
		const lowT = options.lowTemperature;
		if (undefined !== lowT) {
			const value = (((lowT > 125) ? 125 : (lowT < -55) ? -55 : lowT) * 2) | 0;		// half degrees
			io.writeUint16(Register.LM75_THYST, value << 7, true);
			this.#THYST = value/2;
		}

		const mode = options.thermostatMode;
		if (undefined !== mode) {
			conf &= ~0b10;
			if (mode === "interrupt")
				conf |= 0b10;
			else if (mode !== "comparator")
				this.#onError?.("bad thermostatMode");
		}
			
		if (undefined !== options.polarity) {
			conf &= ~0b100;
			if (options.polarity)
				conf |= 0b100;
		}
		if (undefined !== options.faultQueue) {
			conf &= ~0b1_100;
			switch (options.faultQueue) {
				case 1: break;
				case 2: conf |= 0b0_1000; break;
				case 4: conf |= 0b1_0000; break;
				case 6: conf |= 0b1_1000; break;
				default: this.#onError?.("invalid faultQueue");
			}
		}

		io.writeUint8(Register.LM75_CONF, conf);
	}

	close() {
		if ("ready" === this.#status) {
			this.#io.writeUint8(Register.LM75_CONF, 1);		// shut down device
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
		let ret = {};
		const io = this.#io;
		const conf = io.readUint8(Register.LM75_CONF);
		if (conf & 1) {	// if in shutdown mode, turn it on
			io.writeUint8(Register.LM75_CONF, conf & 0b1111_1110);
			Timer.delay(100);		// wait for new reading to be available
		}
			
		let value = io.readUint16(Register.LM75_TEMP, true);

		if (conf & 1)	// if was in shutdown mode, turn it back off
			io.writeUint8(Register.LM75_CONF, conf);

		value = (this.#twoC16(value) >> 5) * 0.125;
		ret.temperature = value;
		if (this.#onAlert) {
			ret.alert = (value > this.#TOS) || (value < this.#THYST);
		}
		return ret;
	}
}

export default LM75;

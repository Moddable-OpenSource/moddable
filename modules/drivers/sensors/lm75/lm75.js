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
	NXP Semiconductors LM75B
	Digital temperature sensor and thermal watchdog
	https://www.nxp.com/docs/en/data-sheet/LM75B.pdf
*/

import device from "embedded:provider/builtin";
import SMBus from "embedded:io/smbus";
const Digital = device.io.Digital;

import Timer from "timer";

const Register = Object.freeze({
	LM75_TEMP:	0x00,
	LM75_CONF:	0x01,
	LM75_THYST:	0x02,
	LM75_TOS:	0x03
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

		if (undefined !== options.target)
			this.target = options.target;

		try {
			if (options.alert && options.onAlert) {
				this.#onAlert = options.onAlert;
				this.#irqMonitor = new Digital({
					pin: options.alert.pin,
					mode: Digital.Input,
					edge: Digital.Falling,
					onReadable: options.onAlert});
				this.#irqMonitor.target = this;
//				this.#irqMonitor.onChanged = () => this.#onAlert();
			}

			this.configure({
				shutdownMode: false,
				thermostatMode: this.#onAlert ? "interrupt" : "comparator",
				polarity: 0,
				faultQueue: 1
			});
		}
		catch(e) {
			this.close();
			throw e;
		}
		this.#status = "ready";
	}

	close() {
		if ("ready" === this.#status) {
			this.#io.writeByte(Register.LM75_CONF, 1);		// shut down device
			this.#status = undefined;
		}
		this.#irqMonitor?.close();
		this.#irqMonitor = undefined;
	}

	configure(options) {
		const io = this.#io;
		let conf = io.readByte(Register.LM75_CONF);

		if (undefined !== options.shutdownMode) {
			conf &= ~0b1;
			if (options.shutdownMode)
				conf |= 0b1;
		}
		if (undefined !== options.thermostatMode) {
			conf &= ~0b10;
			if (options.thermostatMode === "interrupt")
				conf |= 0b10;
			else if (options.thermostatMode !== "comparator")
				throw new Error("bad thermostatMode");
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
				default: throw new Error("invalid faultQueue");
			}
		}
	
		const hiTemp = (options.highTemperature ?? 80) * 2;
		const loTemp = (options.lowTemperature ?? 75) * 2;

		const hiVal = (hiTemp > 250) ? 250 : (hiTemp < -110) ? -110 : hiTemp | 0;
		const loVal = (loTemp > 250) ? 250 : (loTemp < -110) ? -110 : loTemp | 0;

		io.writeWord(Register.LM75_TOS, hiVal << 7, true);
		io.writeWord(Register.LM75_THYST, loVal << 7, true);

		io.writeByte(Register.LM75_CONF, conf);
	}


	sample() {
		const io = this.#io;
		const conf = io.readByte(Register.LM75_CONF);
		if (conf & 1) {	// if in shutdown mode, turn it on
			io.writeByte(Register.LM75_CONF, conf & 0b1111_1110);
			Timer.delay(250);		// wait for new reading to be available
		}
			
		const value = twoC(io.readWord(Register.LM75_TEMP, true));

		if (conf & 1)	// if was in shutdown mode, turn it back off
			io.writeByte(Register.LM75_CONF, conf);

		let sign = 1;
		if (1 === (value & 0b1000_0000_0000_0000))
			sign = -1;

		return { temperature: (value >> 5) * sign * 0.125 };
	}
}

function twoC(a) {
	return ((a & 0x8000) ? (a | 0xffff0000) : a);
}


export default Temperature;

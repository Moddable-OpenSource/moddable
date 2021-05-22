/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
    TMP102 - temperature
*/

import Timer from "timer";
import SMBus from "embedded:io/smbus";
import device from "embedded:provider/builtin";
const Digital = device.io.Digital;

const Register = Object.freeze({
	TEMP_READ: 0x00,
	CONFIG: 0x01,
	TEMP_LOW: 0x02,
	TEMP_HIGH: 0x03
})

class TMP102  {
	#io;
	#extendedRange;
	#extShift;
	#onAlert;
	#irqMonitor;
	#status;

	constructor(options) {
		const io = this.#io = new SMBus({
			...options,
			hz: 100_000,
			address: 0x48
		});

		this.#extShift = 4;
		this.#extendedRange = false;
		this.configure(options);
	}

	configure(options) {
		const io = this.#io;

//		io.writeWord(Register.TEMP_HIGH, 0x7ff8);
		let config = io.readWord(Register.CONFIG, true);

		if (undefined !== options.extendedRange)
			this.#extendedRange = options.extendedRange;

		config &= 0b1111_1111_1110_0000;

		if (this.#extendedRange) {
			config |= 0b1_0000;
			this.#extShift = 3;
		}

		if (options.alert && options.onAlert) {
			this.#onAlert = options.onAlert;
			this.#irqMonitor = new Digital({
				pin: options.alert.pin,
				mode: Digital.Input,
				edge: Digital.Falling,
				onReadable: options.onAlert});
			this.#irqMonitor.target = this;
		}

		if (options.alert?.highTemperature) {
			const value = (options.alert.highTemperature / 0.0625) << this.#extShift;
			io.writeWord(Register.TEMP_HIGH, value, true);
		}
		else
			io.writeWord(Register.TEMP_HIGH, 0x7ff8, true);

		if (options.alert?.lowTemperature) {
			const value = (options.alert.lowTemperature / 0.0625) << this.#extShift;
			io.writeWord(Register.TEMP_LO, value, true);
		}
		else
			io.writeWord(Register.TEMP_LO, 0x0, true);	// lower?

		if (undefined !== options.hz) {
			const hz = options.hz;
			config &= 0b1111_1111_0011_0000;
			
			if (hz < 0.5)
				; // config |= 0b00;
			else if (hz < 2)
				config |= 0b0100_0000;
			else if (hz < 6)
				config |= 0b1000_0000;
			else
				config |= 0b1100_0000;
		}

		if (undefined !== options.faultQueue) {
			conf &= 0b1110_0111_1111_0000;
			switch (options.faultQueue) {
				case 1: break;
				case 2: conf |= 0b0000_1000_0000_0000; break;
				case 4: conf |= 0b0001_0000_0000_0000; break;
				case 6: conf |= 0b0001_1000_0000_0000; break;
				default: throw new Error("invalid faultQueue");
			}
		}

		io.writeWord(Register.CONFIG, config, true);
	}

	close() {
		this.#irqMonitor?.close();
		this.#irqMonitor = undefined;
	}

	sample() {
		const io = this.#io;

		let value = io.readWord(Register.TEMP_READ, true) >> this.#extShift;
//		value = ((value & 0x00ff) << 5) | ((value & 0xff00) >> 11);
		if (value & 0x1000) {
			value -= 1;
			value = ~value & 0x1fff;
			value = -value;
		}
		value *= 0.0625;

		return { temperature: value };
	}
}

export default TMP102;

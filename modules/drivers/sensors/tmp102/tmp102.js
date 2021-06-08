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
	Datasheet: https://www.ti.com/lit/ds/symlink/tmp102.pdf
*/

import SMBus from "embedded:io/smbus";

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
			hz: 100_000,
			address: 0x48,
			...options
		});

		if (options.alert) {
			const alert = options.alert;

			if (options.onAlert) {
				this.#onAlert = options.onAlert;
				this.#irqMonitor = new alert.io({
					onReadable: () => this.#onAlert(),
					...alert
				 });
			}
		}

		this.#extendedRange = false;

		// reset to default values (7.5.3 of datasheet)
		this.#io.writeWord(Register.Config, 0b0110_0000_1010_0000, true);

		// THigh = +80C TLow = +75C (7.5.4)
		this.#io.writeWord(Register.TEMP_HIGH, 0b0101_0000_0000_0000, true);
		this.#io.writeWord(Register.TEMP_LO, 0b0100_1011_0000_0000, true);
	}

	configure(options) {
		const io = this.#io;

		let config = io.readWord(Register.CONFIG, true) & 0b1111_1111_1110_0000;

		if (undefined !== options.extendedRange)
			this.#extendedRange = options.extendedRange;

		if (this.#extendedRange) {
			config |= 0b1_0000;
			this.#extShift = 3;
		}
		else
			this.#extShift = 4;

		if (options.alert) {
			const alert = options.alert;

			if (undefined !== alert.highTemperature) {
				const value = (alert.highTemperature / 0.0625) << this.#extShift;
				io.writeWord(Register.TEMP_HIGH, value, true);
			}
			else
				io.writeWord(Register.TEMP_HIGH, 0x7ff8, true);

			if (undefined !== alert.lowTemperature) {
				const value = (alert.lowTemperature / 0.0625) << this.#extShift;
				io.writeWord(Register.TEMP_LO, value, true);
			}
			else
				io.writeWord(Register.TEMP_LO, 0xfff8, true);
		}

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
		this.#io.close();
		this.#io = undefined;
	}

	sample() {
		let value = this.#io.readWord(Register.TEMP_READ, true) >> this.#extShift;
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

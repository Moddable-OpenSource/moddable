/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

import Timer from "timer";

const Register = Object.freeze({
	TEMP_READ: 0x00,
	CONFIG: 0x01,
	TEMP_LOW: 0x02,
	TEMP_HIGH: 0x03
});

const SHUTDOWN_MODE = 0b0000_0001_0000_0000;
const ALERT_BIT = 0b0000_0000_0010_0000;
const POLARITY_BIT = 0b0000_0100_0000_0000;
const ONE_SHOT = 0b1000_0000_0000_0000;

class TMP102  {
	#io;
	#extendedRange = false;
	#shift = 4;
	#onAlert;
	#status;

	constructor(options) {
		let io, monitor;
		try {
			io = new options.sensor.io({
				hz: 400_000,
				address: 0x48,
				...options.sensor
			});

			const {alert, onAlert} = options;
			if (alert && onAlert) {
				this.#onAlert = onAlert;
				monitor = new alert.io({
					mode: alert.io.InputPullUp,
					...alert,
					edge: alert.io.Falling,
					onReadable: () => this.#onAlert()
				 });
			}

			// reset to default values (7.5.3 of datasheet)
			io.writeUint16(Register.CONFIG, 0b0110_0000_1010_0000, true);

			// THigh = +80C TLow = +75C (7.5.4)
			io.writeUint16(Register.TEMP_HIGH, 0b0101_0000_0000_0000, true);
			io.writeUint16(Register.TEMP_LOW, 0b0100_1011_0000_0000, true);
			
			this.#io = io;
			io.monitor = monitor;
			this.#status = true;
		}
		catch (e) {
			io?.close();
			monitor?.close();
			throw e;
		}
	}

	configure(options) {
		const io = this.#io;

		let config = io.readUint16(Register.CONFIG, true) & 0b1111_1111_1110_0000;

		if (undefined !== options.extendedRange)
			this.#extendedRange = options.extendedRange;

		if (this.#extendedRange) {
			config |= 0b1_0000;
			this.#shift = 3;
		}
		else
			this.#shift = 4;

		if (undefined !== options.shutdownMode) {
			config &= ~SHUTDOWN_MODE;
			if (options.shutdownMode) {
				config |= SHUTDOWN_MODE;
				this.#status = undefined;
			}
			else
				this.#status = true;
		}

		if (undefined !== options.highTemperature) {
			const value = (options.highTemperature / 0.0625) << this.#shift;
			io.writeUint16(Register.TEMP_HIGH, value, true);
		}
		if (undefined !== options.lowTemperature) {
			const value = (options.lowTemperature / 0.0625) << this.#shift;
			io.writeUint16(Register.TEMP_LOW, value, true);
		}

		if (undefined !== options.thermostatMode) {
			config &= 0b1111_1101_1111_0000;
			if (options.thermostatMode === "interrupt")
				config |= 0b10_0000_0000;
			else if (options.thermostatMode !== "comparator")
				throw new Error("bad thermostatMode");
		}

		if (undefined !== options.conversionRate) {
			config &= 0b1111_1111_0011_0000;
			switch (options.conversionRate)	{
				case 0.25:	break;
				case 1:		config |= 0b0100_0000;	break;
				case 4:		config |= 0b1000_0000;	break;
				case 8:		config |= 0b1100_0000;	break;
				default: throw new Error("invalid conversionRate");
			}
		}

		if (undefined !== options.polarity) {
			config &= ~POLARITY_BIT;
			if (options.polarity)
				config |= POLARITY_BIT;
		}
		if (undefined !== options.faultQueue) {
			config &= 0b1110_0111_1111_0000;
			switch (options.faultQueue) {
				case 1: break;
				case 2: config |= 0b0000_1000_0000_0000; break;
				case 4: config |= 0b0001_0000_0000_0000; break;
				case 6: config |= 0b0001_1000_0000_0000; break;
				default: throw new Error("invalid faultQueue");
			}
		}

		io.writeUint16(Register.CONFIG, config, true);
	}

	close() {
		const io = this.#io;
		if (io) {
			this.#io = undefined;

			if (this.#status)
				io.writeUint16(Register.CONFIG, SHUTDOWN_MODE, true);	// shut down device
			
			io.close();
			io.monitor?.close();
		}
	}

	#twoC16(val) {
		return (val > 32768) ?  -(65535 - val + 1) : val;
	}

	sample() {
		const io = this.#io;
		const conf = io.readUint16(Register.CONFIG, true);
		if (conf & SHUTDOWN_MODE) { // if in shutdown mode, set ONE_SHOT
			io.writeUint8(Register.CONFIG, conf & ONE_SHOT);
			Timer.delay(35);		// wait for new reading to be available
		}

		let value = (this.#twoC16(io.readUint16(Register.TEMP_READ, true)) >> this.#shift) * 0.0625;
		return { temperature: value, alert: (conf & ALERT_BIT) == (conf & POLARITY_BIT) };
	}
}

export default TMP102;

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
    AMG88xx Grid-EYE - Infrared array sensor temperature
	Datasheet: https://api.pim.na.industrial.panasonic.com/file_stream/main/fileversion/244774
*/


/*
	register map from SparkFun_GridEYE_Arduino_Library.cpp
*/

import Timer from "timer";

const Register = Object.freeze({
	POWER_CONTROL: 0x00,
	RESET: 0x01,
	FRAMERATE: 0x02,
	INT_CONTROL: 0x03,
	STATUS: 0x04,
	STATUS_CLEAR: 0x05,
	AVERAGE: 0x07,
	INT_LEVEL_UPPER: 0x08,
	INT_LEVEL_LOWER: 0x0A,
	INT_LEVEL_HYST: 0x0C,
	THERMISTOR: 0x0E,
	INT_TABLE_INT0: 0x10,
	RESERVED_AVERAGE: 0x1F,
	TEMPERATURE_START: 0x80
});

class AMG88  {
	#io;
	#values = new Uint16Array(16);

	constructor(options) {
		const io = new (options.sensor.io)({
			hz: 400_000,	// data sheet says 400 kHz max
			...options.sensor,
			address: 0x69
		});
	
		try {
			io.writeUint8(Register.POWER_CONTROL, 0);		// wake
			Timer.delay(2);
			io.writeUint8(Register.RESET, 0x3F);			// initial reset

//			this.configure({framerate: 10});				// 10 fps is the reset state
		}
		catch (e) {
			io.close();
			throw e;
		}
		
		this.#io = io;
	}
	close() {
		if (!this.#io)
			return;

		this.#io.writeUint8(Register.POWER_CONTROL, 0x10);	// sleep

		this.#io?.close();
		this.#io = undefined;
	}
	configure(options) {
		if (undefined !== options.framerate)
			this.#io.writeUint8(Register.FRAMERATE, (options.framerate < 5) ? 1 : 0);
	}
	sample() {
		const io = this.#io;
		const result = {
			temperature: convert(io.readUint16(Register.THERMISTOR)) * 0.0625,
			temperatures: new Float32Array(64)
		};

		for (let i = 0, temperatures = result.temperatures, values = this.#values; i < 64; i += 16) {
			io.readBuffer(Register.TEMPERATURE_START + (i << 1), values.buffer)
			for (let j = 0; j < 16; j++)
				temperatures[i + j] = convert(values[j]) * 0.25;
		}

		return result;
	}
}

function convert(value) {
	if (value & (1 << 11))
		return -(value & ~(1 << 11));

	return value;
}

export default AMG88;

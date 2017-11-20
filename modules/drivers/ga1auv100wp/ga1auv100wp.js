/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
	Sharp GA1AUV100WP UV/ALS sensor
*/

import SMBus from "pins/smbus";
import Timer from "timer";

const Register = {
	OPERATION: 0x00,	// Operation register
	MODE: 0x01,			// Mode register
	RAN: 0x02,			// Range register
	INT: 0x03,			// Interval register
	D0_LSB: 0x04,		// D0 Least Significant Byte
	D0_MSB: 0x05,		// D0 Most Significant Byte
	D1_LSB: 0x06,		// D1 Least Significant Byte
	D1_MSB: 0x07,		// D1 Most Significant Byte
	ID: 0x08			// Device Identification Register
}
Object.freeze(Register);

const Mode = {
	// Register settings for Basic operation (REG_OPE)
	SHUTDOWN: 0x00,		// set OP3 '0'
	UV: 0x80,			// set OP3 '1' and OP0 '0'
	ALS: 0x90,			// set OP3 '1' and OP0 '1'

	// Photodiode selection
	UVPD: 0x00,			// set PD0 '0'
	ALSPD: 0x40			// set PD0 '1'
}
Object.freeze(Mode);

const Measurable = {
	// Maximum measurable range for UV mode and ALS mode: RANGE[3:0] (ADDRESS:02h)
	RANGEx001: 0x00,
	RANGEx002: 0x01,
	RANGEx008: 0x03,
	RANGEx016: 0x04,
	RANGEx032: 0x05,
	RANGEx064: 0x06,
	RANGEx128: 0x07,
	RANGEx256: 0x0F
};
Object.freeze(Measurable);

const MeasureTime = {
	// Resolution /Measuring time setting for UV mode and ALS mode: RES[2:0] (ADDRESS:02h)
	RES800: 0x00,
	RES400: 0x01,
	RES200: 0x02,
	RES100: 0x03,
	RES050: 0x04,
	RES025: 0x05,
	RES012: 0x06,
	RES006: 0x07
}
Object.freeze(MeasureTime);

const Interval = {
	// Interval time setting for UV mode and ALS mode: INTVAL[2:0] (ADDRESS 03H)
	ms_000: 0x00,
	ms_012: 0x01,
	ms_025: 0x02,
	ms_050: 0x03,
	ms_100: 0x04,
	ms_200: 0x05,
	ms_400: 0x06,
	ms_800: 0x07
}
Object.freeze(Interval);

class GA1AUV100WP extends SMBus {
	constructor(dictionary) {
		super(Object.assign({address: 0x10}, dictionary));

		if (0x81 != this.readByte(Register.ID))
			throw new Error("unrecognized ID");

		this.operation = Mode.SHUTDOWN;
		this.range = Measurable.RANGEx032;
		this.measureTime = MeasureTime.RES025;
		this.interval = Interval.ms_200;

		this.writeByte(Register.OPERATION, 1);	// reset
	}

	close() {
		this.writeByte(Register.OPERATION, Mode.SHUTDOWN);
		super.close();
	}

	configure(dictionary) {
		this.writeByte(Register.OPERATION, Mode.SHUTDOWN);

		for (let property in dictionary) {
			switch (property) {
				case "operation":
					if ("als" === dictionary.operation)
						this.operation = Mode.ALS;
					else if ("uv" === dictionary.operation)
						this.operation = Mode.UV;
					else if ("shutdown" === dictionary.operation)
						this.operation = Mode.SHUTDOWN;
					else
						throw new Error("invalid operation");
					break;
				case "interval":
				case "measureTime":
				case "range":
					this[property] = parseInt(dictionary[property]);
					break;
				default:
					throw new Error(`invalid property "${property}`);
					break;
			}
		}

		this.writeByte(Register.RAN, (this.range << 4) | this.measureTime);
		this.writeByte(Register.INT, this.interval);

		this.writeByte(Register.MODE, (this.operation === Mode.ALS) ? Mode.ALSPD : 0);
		this.writeByte(Register.OPERATION, this.operation);

		Timer.delay(1);   // wait 1 msec.
	}

	sample() {
		let bytes = this.readBlock(Register.D0_LSB, 4);
		let d0 = (bytes[1] << 8) | bytes[0];
		let d1 = (bytes[3] << 8) | bytes[2];

		if (Mode.ALS === this.operation) {
			return {
				clear: d0,
				ir: d1,
				lux: d0 - d1
			}
		}

		if (Mode.UV === this.operation) {
			return {
				uv0: d0,
				uvCut: d1,
				uv: 0.04216 * (d0 - d1)
			}
		}

		throw new Error("can't read when shutdown");
	}
}
Object.freeze(GA1AUV100WP.prototype);

export {GA1AUV100WP as default, GA1AUV100WP, Measurable, MeasureTime, Interval};

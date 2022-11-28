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
	AK8963 Magnetometer -
	embedded with MPU6050 to make MPU9250

	https://www.alldatasheet.com/datasheet-pdf/pdf/535561/AKM/AK8963.html

	sample value in microtesla
*/

import Timer from "timer";

const REGISTERS = Object.freeze({
	MAG_ST1: 0x02,
	MAG_XOUT: 0x03,
	MAG_YOUT: 0x05,
	MAG_ZOUT: 0x07,
	MAG_ST2: 0x09,
	MAG_CNTL1: 0x0A,
	MAG_CNTL2: 0x0B,
	MAG_ASAX: 0x10,
	MAG_ASAY: 0x11,
	MAG_ASAZ: 0x12
});

const Config = Object.freeze({
    Range: {
        RANGE_14bit: 0b0,
        RANGE_16bit: 0b1
    },
    Mode: {
        MODE_POWERDOWN: 0b0000,
        MODE_SINGLE:    0b0001,
        MODE_CONT_1:    0b0010,
        MODE_CONT_2:    0b0110,
        MODE_EXT_TRIG:  0b0100,
        MODE_SELF_TEST: 0b1000,
        MODE_FUSE_ROM:  0b1111
    }
}, true);

class AK8963 {
	#io;
	#values = new ArrayBuffer(7);
	#dataView;
	#range = 0;				// 14 bit
	#res = 4912.0/8190.0	// 14 bit
	#mode = 0b0000;			// powerdown
	#coefX;
	#coefY;
	#coefZ;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 400_000,
			address: 0x0C,
			...options.sensor
		});

		this.#dataView = new DataView(this.#values);

		try {
			if (0x48 !== io.readUint8(0)) {
				this.close();
				return undefined;
			}
		}
		catch(e) {
			this.close();
			throw e;
		}

		// reset
		io.writeUint8(REGISTERS.MAG_CNTL2, 0b0000_0001);

		// powerdown
		io.writeUint8(REGISTERS.MAG_CNTL2, 0b0000_0000);
		// read fuseROM
		io.writeUint8(REGISTERS.MAG_CNTL2, 0b0000_1111);
		const coef = new Uint8Array(3);
		io.readBuffer(REGISTERS.MAG_ASAX, coef);
		this.#coefX = (coef[0] - 128) / 256.0 + 1;
		this.#coefY = (coef[1] - 128) / 256.0 + 1;
		this.#coefZ = (coef[2] - 128) / 256.0 + 1;
		// powerdown
		io.writeUint8(REGISTERS.MAG_CNTL2, 0b0000_0000);
	}

	close() {
		this.#io?.close();
		this.#io = undefined;
	}
	configure(options) {
		const io = this.#io;

		if ((undefined !== options.range) || (undefined !== options.mode)) {
			this.#range = options.range | 0b1;
			if (this.#range)
				this.#res = 4912.0 / 32760.0;
			else
				this.#res = 4912.0 / 8190.0;
			this.#mode = options.mode & 0b1111;
            io.writeUint8(REGISTERS.MAG_CNTL1, this.#mode | (this.#range << 4));
		}
	}
	sample() {
		const io = this.#io;
		let ret = {};

		const rdy = io.readUint8(REGISTERS.MAG_ST1);
		if (rdy & 0b01) {
			io.readBuffer(REGISTERS.MAG_XOUT, this.#values);
			ret.x = this.#dataView.getInt16(0) * this.#coefX * this.#res;	// µT/LSB
			ret.y = this.#dataView.getInt16(2) * this.#coefY * this.#res;
			ret.z = this.#dataView.getInt16(4) * this.#coefZ * this.#res;
		}

		return ret;
	}
}
Object.freeze(AK8963.prototype);

export default AK8963;

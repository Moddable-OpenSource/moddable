/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
	Bosch BME280 — temperature, humidity, pressure

	Datasheet: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf
	Compensation formulas from the datasheet / Bosch BME280 API.

	ECMA-419 compound sensor:
	  sample() → {
	    thermometer: { temperature },   // °C
	    hygrometer:  { humidity },      // 0 to 1
	    barometer:   { pressure }       // Pa
	  }
*/

import Timer from "timer";

const Register = Object.freeze({
	DIG_T1: 0x88,
	DIG_H1: 0xA1,
	DIG_H2: 0xE1,
	CHIPID: 0xD0,
	SOFTRESET: 0xE0,
	CTRL_HUM: 0xF2,
	STATUS: 0xF3,
	CTRL_MEAS: 0xF4,
	CONFIG: 0xF5,
	PRESS_MSB: 0xF7
});

const Config = Object.freeze({
	Sampling: {
		SKIPPED: 0x00,
		X1: 0x01,
		X2: 0x02,
		X4: 0x03,
		X8: 0x04,
		X16: 0x05
	},
	Filter: {
		OFF: 0x00,
		X2: 0x01,
		X4: 0x02,
		X8: 0x03,
		X16: 0x04
	},
	Mode: {
		SLEEP: 0x00,
		FORCED: 0x01,
		NORMAL: 0x03
	},
	Standby: {
		MS_0_5: 0x00,
		MS_62_5: 0x01,
		MS_125: 0x02,
		MS_250: 0x03,
		MS_500: 0x04,
		MS_1000: 0x05,
		MS_10: 0x06,
		MS_20: 0x07
	}
}, true);

const CHIP_ID = 0x60;
const SOFT_RESET_CMD = 0xB6;

class BME280 {
	#io;
	#byteBuffer = new Uint8Array(1);
	#wordBuffer = new Uint8Array(2);
	#valueBuffer = new Uint8Array(8);
	#calib;
	#tFine = 0;
	#mode = Config.Mode.NORMAL;
	#humiditySampling = Config.Sampling.X1;
	#temperatureSampling = Config.Sampling.X1;
	#pressureSampling = Config.Sampling.X1;
	#filter = Config.Filter.OFF;
	#standby = Config.Standby.MS_1000;

	constructor(options) {
		if ("target" in options)
			this.target = options.target;

		try {
			// Defaults first; caller sensor options override (419 IO pattern)
			const io = this.#io = new options.sensor.io({
				hz: 400_000,
				address: 0x76,
				...options.sensor
			});

			const bBuf = this.#byteBuffer;
			const wBuf = this.#wordBuffer;

			bBuf[0] = Register.CHIPID;
			io.write(bBuf);
			io.read(bBuf);
			if (CHIP_ID !== bBuf[0])
				throw new Error("unexpected sensor");

			wBuf[0] = Register.SOFTRESET;
			wBuf[1] = SOFT_RESET_CMD;
			io.write(wBuf);
			Timer.delay(10);

			this.#readCalibration();

			this.configure({
				mode: Config.Mode.NORMAL,
				temperatureSampling: Config.Sampling.X2,
				pressureSampling: Config.Sampling.X16,
				humiditySampling: Config.Sampling.X1,
				filter: Config.Filter.X16,
				standbyDuration: Config.Standby.MS_500
			});
		}
		catch (e) {
			this.close();
			throw e;
		}
	}

	configure(options) {
		if ("mode" in options)
			this.#mode = options.mode;
		if ("temperatureSampling" in options)
			this.#temperatureSampling = options.temperatureSampling;
		if ("pressureSampling" in options)
			this.#pressureSampling = options.pressureSampling;
		if ("humiditySampling" in options)
			this.#humiditySampling = options.humiditySampling;
		if ("filter" in options)
			this.#filter = options.filter;
		if ("standbyDuration" in options)
			this.#standby = options.standbyDuration;

		const io = this.#io;
		const wBuf = this.#wordBuffer;

		// CTRL_HUM must be written before CTRL_MEAS (changes take effect on CTRL_MEAS write)
		wBuf[0] = Register.CTRL_HUM;
		wBuf[1] = this.#humiditySampling & 0x07;
		io.write(wBuf);

		wBuf[0] = Register.CONFIG;
		wBuf[1] = ((this.#standby & 0x07) << 5) | ((this.#filter & 0x07) << 2);
		io.write(wBuf);

		wBuf[0] = Register.CTRL_MEAS;
		wBuf[1] = ((this.#temperatureSampling & 0x07) << 5) |
			((this.#pressureSampling & 0x07) << 2) |
			(this.#mode & 0x03);
		io.write(wBuf);
	}

	sample() {
		const io = this.#io;
		const bBuf = this.#byteBuffer;
		const vBuf = this.#valueBuffer;
		const wBuf = this.#wordBuffer;

		// Forced mode: trigger a measurement and wait for it to complete
		if (this.#mode === Config.Mode.FORCED) {
			wBuf[0] = Register.CTRL_MEAS;
			wBuf[1] = ((this.#temperatureSampling & 0x07) << 5) |
				((this.#pressureSampling & 0x07) << 2) |
				Config.Mode.FORCED;
			io.write(wBuf);

			// Measuring bit (status[3]) or im_update (status[0])
			for (let i = 0; i < 20; i++) {
				Timer.delay(2);
				bBuf[0] = Register.STATUS;
				io.write(bBuf);
				io.read(bBuf);
				if (0 === (bBuf[0] & 0x08))
					break;
			}
		}

		bBuf[0] = Register.PRESS_MSB;
		io.write(bBuf);
		io.read(vBuf);

		const rawP = (vBuf[0] << 16) | (vBuf[1] << 8) | vBuf[2];
		const rawT = (vBuf[3] << 16) | (vBuf[4] << 8) | vBuf[5];
		const rawH = (vBuf[6] << 8) | vBuf[7];

		const temperature = this.#compensateT(rawT);
		const pressure = this.#compensateP(rawP);
		const humidity = this.#compensateH(rawH);

		return {
			thermometer: { temperature },
			hygrometer: { humidity },
			barometer: { pressure }
		};
	}

	close() {
		this.#io?.close();
		this.#io = undefined;
	}

	#readCalibration() {
		const io = this.#io;
		const bBuf = this.#byteBuffer;

		// T1..P9: 24 bytes at 0x88
		const tp = new Uint8Array(24);
		bBuf[0] = Register.DIG_T1;
		io.write(bBuf);
		io.read(tp);
		const tpView = new DataView(tp.buffer);

		// H1 at 0xA1; H2..H6 packed at 0xE1..0xE7
		bBuf[0] = Register.DIG_H1;
		io.write(bBuf);
		io.read(bBuf);
		const H1 = bBuf[0];

		const h = new Uint8Array(7);
		bBuf[0] = Register.DIG_H2;
		io.write(bBuf);
		io.read(h);
		const hView = new DataView(h.buffer);

		// H4/H5 are signed 12-bit; sign-extend with arithmetic shift
		const H4 = (((h[3] << 4) | (h[4] & 0x0F)) << 20) >> 20;
		const H5 = (((h[5] << 4) | (h[4] >> 4)) << 20) >> 20;

		this.#calib = {
			T1: tpView.getUint16(0, true),
			T2: tpView.getInt16(2, true),
			T3: tpView.getInt16(4, true),
			P1: tpView.getUint16(6, true),
			P2: tpView.getInt16(8, true),
			P3: tpView.getInt16(10, true),
			P4: tpView.getInt16(12, true),
			P5: tpView.getInt16(14, true),
			P6: tpView.getInt16(16, true),
			P7: tpView.getInt16(18, true),
			P8: tpView.getInt16(20, true),
			P9: tpView.getInt16(22, true),
			H1,
			H2: hView.getInt16(0, true),
			H3: h[2],
			H4,
			H5,
			H6: hView.getInt8(6)
		};
	}

	// Bosch floating-point temperature compensation; returns °C
	// (datasheet §4.2.3 / BME280 API double-precision path)
	#compensateT(adcT) {
		const c = this.#calib;
		adcT >>= 4;

		const var1 = (adcT / 16384 - c.T1 / 1024) * c.T2;
		const var2 = ((adcT / 131072 - c.T1 / 8192) *
			(adcT / 131072 - c.T1 / 8192)) * c.T3;
		this.#tFine = var1 + var2;

		return this.#tFine / 5120;
	}

	// Bosch floating-point pressure compensation; returns Pa
	#compensateP(adcP) {
		const c = this.#calib;
		adcP >>= 4;

		let var1 = this.#tFine / 2 - 64000;
		let var2 = var1 * var1 * c.P6 / 32768;
		var2 = var2 + var1 * c.P5 * 2;
		var2 = var2 / 4 + c.P4 * 65536;
		var1 = (c.P3 * var1 * var1 / 524288 + c.P2 * var1) / 524288;
		var1 = (1 + var1 / 32768) * c.P1;
		if (0 === var1)
			return 0;

		let p = 1048576 - adcP;
		p = (p - var2 / 4096) * 6250 / var1;
		var1 = c.P9 * p * p / 2147483648;
		var2 = p * c.P8 / 32768;
		p = p + (var1 + var2 + c.P7) / 16;

		return p;
	}

	// Bosch floating-point humidity compensation; returns 0 to 1
	#compensateH(adcH) {
		if (adcH === 0x8000)
			return undefined;

		const c = this.#calib;
		let h = this.#tFine - 76800;
		h = (adcH - (c.H4 * 64 + c.H5 / 16384 * h)) *
			(c.H2 / 65536 * (1 + c.H6 / 67108864 * h *
				(1 + c.H3 / 67108864 * h)));
		h = h * (1 - c.H1 * h / 524288);

		if (h > 100)
			h = 100;
		else if (h < 0)
			h = 0;

		return h / 100;
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export { BME280 as default, BME280, Config };

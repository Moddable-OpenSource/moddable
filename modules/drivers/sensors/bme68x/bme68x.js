/*
 * Copyright (c) 2023 Moddable Tech, Inc.
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

// Bosch BME68x library as JavaScript class
export class BME68x @ "xs_bne68x_destructor" {
	#io;
	#register = Uint8Array.of(0);

	constructor(options) {
		const io = this.#io = new (options.sensor.io)({
			address: 0x77,
			hz: 100_000,
			...options.sensor
		});
		try {
			this.init();
		}
		catch (e) {
			this.close();
			throw e;
		}
	}
	close() {
		this.#io?.close();
		this.#io = undefined;
	}
	read(register, byteLength) {
		this.#register[0] = register;
		return this.#io.writeRead(this.#register, byteLength, false);
	}
	write(bytes) {
		this.#io.write(bytes);
	}

	init() @ "xs_bme68x_init"; 
	set_conf(options) @ "xs_bme68x_set_conf"; 
	get_conf() @ "xs_bme68x_get_conf"; 
	set_heatr_conf(mode, options) @ "xs_bme68x_set_heatr_conf"; 
	get_heatr_conf() @ "xs_bme68x_get_heatr_conf"; 
	set_op_mode(mode) @ "xs_bme68x_set_op_mode"; 
	get_op_mode() @ "xs_bme68x_get_op_mode"; 
	get_meas_dur(mode) @ "xs_bme68x_get_meas_dur";		// returns milliseconds 
	get_data(mode) @ "xs_bme68x_get_data"; 
	selftest_check() @ "xs_bme68x_selftest_check"; 
	soft_reset() @ "xs_bme68x_soft_reset"; 
	set_regs(regs, buffer) @ "xs_bme68x_set_regs"; 
	get_regs(regs) @ "xs_bme68x_get_regs"; 

	static FILTER_OFF = 0
	static FILTER_SIZE_1 = 1;
	static FILTER_SIZE_3 = 2;
	static FILTER_SIZE_7 = 3;
	static FILTER_SIZE_15 = 4;
	static FILTER_SIZE_31 = 5;
	static FILTER_SIZE_63 = 6;
	static FILTER_SIZE_127 = 7;

	static ODR_0_59_MS = 0;
	static ODR_62_5_MS = 1;
	static ODR_125_MS = 2;
	static ODR_250_MS = 3;
	static ODR_500_MS = 4;
	static ODR_1000_MS = 5;
	static ODR_10_MS = 6;
	static ODR_20_MS = 7;
	static ODR_NONE = 8;

	static OS_NONE = 0;
	static OS_1X = 1;
	static OS_2X = 2;
	static OS_4X = 3;
	static OS_8X = 4;
	static OS_16X = 5;

	static ENABLE = 1;
	static DISABLE = 0;

	static SLEEP_MODE = 0;
	static FORCED_MODE = 1;
	static PARALLEL_MODE = 2;
	static SEQUENTIAL_MODE = 3;

	static VALID_DATA = 0x80;
}

// ECMA-419 sensor driver - compound sensor with temperature, humidity, pressure
export default class {
	#bme;

	constructor(options) {
		const bme = this.#bme = new BME68x(options);

		bme.set_conf({
			filter: BME68x.FILTER_OFF,
			odr: BME68x.ODR_NONE,
			os_hum: BME68x.OS_2X,
			os_pres: BME68x.OS_2X,
			os_temp: BME68x.OS_2X,
		});

		bme.set_heatr_conf(BME68x.SEQUENTIAL_MODE, {
			enable: BME68x.ENABLE,
			heatr_temp_prof: [200, 240, 280, 320, 360, 360, 320, 280, 240, 200],
			heatr_dur_prof: [100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
		});

		bme.set_op_mode(BME68x.SEQUENTIAL_MODE);
	}
	close() {
		if (!this.#bme)
			return;

		this.#bme.set_op_mode(BME68x.SLEEP_MODE);
		this.#bme.close();
		this.#bme = undefined;
	}
	configure(options) {
		if (options.conf)
			this.#bme.set_conf(options.conf);
		if (options.heatr_conf)
			this.#bme.set_heatr_conf(BME68x.SEQUENTIAL_MODE, options.heatr_conf);
	}
	sample() {
		const data = this.#bme.get_data(BME68x.SEQUENTIAL_MODE);
		if (!data?.length)
			return;

		const sample = data[0];
		return {
			thermometer: {
				temperature: sample.temperature
			},
			hygrometer: {
				humidity: sample.humidity / 100
			},
			barometer: {
				pressure: sample.pressure
			}
		}
	}
}

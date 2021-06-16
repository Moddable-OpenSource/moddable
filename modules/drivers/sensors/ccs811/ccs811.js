/*
 * Copyright (c) 2019-2021  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

/*
    CCS811 - Indoor Air Quality sensor
	https://www.sciosense.com/products/environmental-sensors/ccs811-gas-sensor-solution/
    Datasheet: https://cdn.sparkfun.com/assets/learn_tutorials/1/4/3/CCS811_Datasheet-DS000459.pdf
*/

// @ToDo: Thresholds
//		: more config
//		: Wake pin (not just tied to low)
//		: shut down firmware on close

import Timer from "timer";
import SMBus from "embedded:io/smbus";

const Register = Object.freeze({
	STATUS: 0x00,
	MEAS_MODE: 0x01,
	ALG_RESULT: 0x02,
	RAW_RESULT: 0x03,
	ENV_DATA: 0x05,
	NTC: 0x06,
	THRESHOLDS: 0x10,
	BASELINE: 0x11,
	HW_ID: 0x20,
	HW_VERS: 0x21,
	FW_BOOT: 0x23,
	FW_APP: 0x24,
	ERROR_ID: 0xE0,
	SW_RESET: 0xFF,

	BOOTLOADER_APP_ERASE: 0xF1,
	BOOTLOADER_APP_DATA: 0xF2,
	BOOTLOADER_APP_VERIFY: 0xF3,
	BOOTLOADER_APP_START: 0xF4
});

const HW_ID_CODE = 0x81;

const INT_THRESH = 1 << 2;
const INT_DATARDY = 1 << 3;

const Config = Object.freeze({
	Drive_Mode: {
		IDLE: 0x00,
		MEAS_1SEC: 0x01 << 4,
		MEAS_10SEC: 0x02 << 4,
		MEAS_60SEC: 0x03 << 4,
		MEAS_250MS: 0x04 << 4
	}
}, true);


class CCS811  {
	#io;
	#valueBuffer = new Uint8Array(8);
	#status = {};
	#mode = { INT_THRESH: 0, INT_DATARDY: 0, DRIVE_MODE: Config.Drive_Mode.IDLE };

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 100_000,
			address: 0x5A,
			sendStop: true,
			...options.sensor
		});

		io.writeBlock(Register.SW_RESET, Uint8Array.of(0x11, 0xE5, 0x72, 0x8A));
		Timer.delay(100);

		if (io.readByte(Register.HW_ID) != HW_ID_CODE)
			throw new Error("unexpected sensor");

		io.sendByte(Register.BOOTLOADER_APP_START);
		Timer.delay(100);

		this.#readStatus();
		if (this.#status.ERROR || !this.#status.FW_MODE)
			throw new Error("sensor error");

		this.#disableInterrupt();
		this.#driveMode = Config.Drive_Mode.MEAS_1SEC;
	}
	configure(options) {
		if (undefined !== options.environmentalData) {
			let humidity = options.environmentalData.humidity ?? 50;
			let temperature = options.environmentalData.temperature ?? 25;

			humidity = (humidity * 512.0 + 0.5) & 0xffff;
			temperature = ((temperature + 25.0) * 512.0 + 0.5) & 0xffff;

			this.#io.writeBlock(Register.ENV_DATA, Uint8Array.of(
					(humidity >> 8) & 0xff, humidity & 0xff,
					(temperature >> 8) & 0xff, temperature & 0xff));
		}
	}
	#enableInterrupt() {
		this.#mode.INT_DATARDY = INT_DATARDY;
		this.#writeMode();
	}
	#disableInterrupt() {
		this.#mode.INT_DATARDY = 0;
		this.#writeMode();
	}
	set #driveMode(mode) {
		this.#mode.DRIVE_MODE = mode;
		this.#writeMode();
	}
	#writeMode() {
		let mode = this.#measMode;
		this.#io.writeByte(Register.MEAS_MODE, mode);
	}
	get #measMode() {
		return (this.#mode.INT_THRESH | this.#mode.INT_DATARDY | this.#mode.DRIVE_MODE);
	}
	#setStatus(stat) {
		this.#status.ERROR		= stat & 0b0000_0001;
		this.#status.DATA_READY	= stat & 0b0000_1000;
		this.#status.APP_VALID	= stat & 0b0001_0000;
		this.#status.FW_MODE	= stat & 0b1000_0000;
		if (this.#status.ERROR)
			this.#status.error_detail = this.#io.readByte(Register.ERROR_ID);
	}
	#readStatus() {
		let stat = this.#io.readByte(Register.STATUS);
		this.#setStatus(stat);
	}
	get available() {
		this.#readStatus();
		return this.#status.DATA_READY ? true: false;
	}
	close() {
				// low power?
		this.#io.close();
		this.#io = undefined;
	}
	sample() {
		let ret = {};
		const io = this.#io;
		const vBuf = this.#valueBuffer;
		if (!this.available)
			return false;
		io.readBlock(Register.ALG_RESULT, vBuf);
		ret.eCO2 = (vBuf[0] << 8) | vBuf[1];
		ret.TVOC = (vBuf[2] << 8) | vBuf[3];
		ret.selected = vBuf[6] >> 2;
		ret.rawADC = ((vBuf[6] & 3) << 8) | vBuf[7];
		return ret;
	}
}

export { CCS811 as default, CCS811, Config };

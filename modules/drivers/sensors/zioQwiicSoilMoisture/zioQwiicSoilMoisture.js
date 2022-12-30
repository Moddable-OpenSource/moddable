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
	Zio Qwiic Soil Moisture Sensor
	QuickStart guide: https://www.smart-prototyping.com/blog/Zio-Soil-Moisture-Sensor-Qwiic-Start-Guide
	https://github.com/sparkfun/Zio-Qwiic-Soil-Moisture-Sensor/blob/master/Firmware/Qwiic%20Soil%20Moisture%20Sensor%20Examples/Example1-Basic_Reading/Example1-Basic_Reading.ino
*/

import Timer from "timer";

const Register = Object.freeze({
	LED_OFF:	0x00,
	LED_ON:		0x01,
	GET_VALUE:	0x05,
	NOTHING_NEW:	0x99
});

class ZIOQWIICMOISTURE {
	#io;
	#averaging = 1;

	constructor(options) {
		const io = this.#io = new options.sensor.io({
			hz: 100_000, 
			address: 0x28,
			...options.sensor
		});

		try {
			io.writeQuick();
		}
		catch (e) {
			trace(`no response from sensor`);
		}

		this.configure({led: 0});
	}
	configure(options) {
		if (undefined !== options.led) {
			if (options.led)
				this.#ledOn();
			else
				this.#ledOff();
		}
		if (undefined !== options.averaging)
			this.#averaging = options.averaging;
	}
	close() {
		this.#io.close();
		this.#io = undefined;
	}
	sample() {
		let accum = 0;
		for (let i=0; i<this.#averaging; i++)
			accum += this.#io.readUint16(Register.GET_VALUE, 0);
		accum /= this.#averaging;
		return { moisture: (1024 - accum) / 1024 };
	}
	#ledOn() {
		this.#io.sendByte(Register.LED_ON);
	}
	#ledOff() {
		this.#io.sendByte(Register.LED_OFF);
	}
}


export default ZIOQWIICMOISTURE;

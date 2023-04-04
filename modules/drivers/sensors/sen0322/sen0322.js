/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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
	SEN0322 - Oxygen Sensors
	https://www.dfrobot.com/product-2052.html
	Datasheet: https://wiki.dfrobot.com/Gravity_I2C_Oxygen_Sensor_SKU_SEN0322
	Reference Driver: https://github.com/DFRobot/DFRobot_OxygenSensor/blob/master/DFRobot_OxygenSensor.cpp
*/


import Timer from "timer";

const Register = Object.freeze({
	OXYGEN_DATA: 0x03, ///< register for oxygen data
	USER_SET: 0x08, ///< register for users to configure key value manually
	AUTUAL_SET: 0x09, ///< register that automatically configure key value
	GET_KEY: 0x0A ///< register for obtaining key value
});

const I2C_ADDR = 0x73;

class SEN0322
{
	#io;
	#key;

	constructor(options) {
		this.#io = new options.sensor.io({
			address: I2C_ADDR,
			hz: 100_000,
			...options.sensor
		});
	}

	configure(options)
	{
		if (undefined !== options.vol)
		{
			const vol = options.vol;
			const mv = options.mv ?? 0;
			if (0 > vol || vol > 25)
				throw new RangeError("invalid O2 concentration (must be 0-25)");

			const io = this.#io;
			let keyValue = vol * 10;

			if (-0.000001 < mv && mv < 0.000001)		
				io.write(Uint8Array.of(Register.USER_SET, keyValue));
			else
			{
				keyValue = vol / mv * 1000;
				io.write(Uint8Array.of(Register.AUTUAL_SET, keyValue));
			}
		}
	}
	#setKey()
	{
		const io = this.#io;
		io.write(Uint8Array.of(Register.GET_KEY));
		const curKey = (new Uint8Array(io.read(1)))[0];
		if (curKey)
			this.#key = curKey / 1000.0;
		else
			this.#key = 20.9 / 120.0;
	}
	close()
	{
		this.#io?.close();
		this.#io = undefined;
	}
	sample()
	{
		const io = this.#io;
		this.#setKey();
		io.write(Uint8Array.of(Register.OXYGEN_DATA));
		Timer.delay(100);
		const rxbuf = new Uint8Array(io.read(3));
		const oxygenPercent = this.#key * (rxbuf[0] + (rxbuf[1] / 10.0) + (rxbuf[2] / 100.0));
		return { O: oxygenPercent * 10_000 }; // O2 concentration in PPM
	}
	
}
export default SEN0322;

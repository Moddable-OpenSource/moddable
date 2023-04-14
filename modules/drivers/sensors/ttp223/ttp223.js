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
	Tontek TTP223 - Capactive Touch Switch Sensor
	http://hiletgo.com/ProductDetail/1915450.html
	Datasheet: https://datasheet.lcsc.com/szlcsc/TTP223-BA6_C80757.pdf
	Reference Driver: https://github.com/ac005sheekar/Flex-Force-sensitivity-Gravity-Sensors-Interfacing-with-Arduino-Microcontrollers-and-Firmware/blob/master/TTP223B%20Digital%20Touch%20Sensor%20Capacitive%20Touch.ino
*/

class TTP223
{
	#io;
	#onAlert;

	constructor(options) {
		const { sensor, onAlert } = options;
		if (sensor && onAlert)
		{
			this.#onAlert = options.onAlert;
			this.#io = new sensor.io({
				mode: sensor.io.InputPullUp,
				...sensor,
				edge: sensor.io.Rising | sensor.io.Falling,
				onReadable: () => this.#onAlert()
			});
		}
		
	}

	configure(options)
	{
	}

	close()
	{
		this.#io?.close();
		this.#io = undefined;
	}
	sample()
	{
		return { position: this.#io.read() };
	}
	
}
export default TTP223;

/*
 * Copyright (c) 2024  Moddable Tech, Inc.
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

import config from "mc/config";
import Timer from "timer";
import Button from "button";
import Digital from "pins/digital";

class Backlight {  // extends PWM
	#pwm;

	constructor(brightness = 100) {
		this.#pwm = new device.io.PWM({ pin: config.backlight });
		this.write(100 - brightness);
	}
	write(value) {
		value = 100 - value;		// reversed
		value /= 100;
		if (value <= 0)
			value = 1023;
		else if (value >= 1)
			value = 0;
		else {
			value *= value;		// linear
			value = 1 - value;	// PWM is inverted from brightness (0 is full power, 1023 is no power)
			value *= 1023;
		}
		this.#pwm.write(value);
	}
}

class Flash {
	constructor(options) {
		return new Button({
			...options,
			pin: 0,
			invert: true
		});
	}
}

globalThis.Host = Object.freeze({
	Backlight,
	Button: {
		Default: Flash,
		Flash
	}
}, true);

export default function (done) {
	done?.();
}

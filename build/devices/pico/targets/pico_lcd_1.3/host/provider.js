/*
 * Copyright (c) 2022-2026  Moddable Tech, Inc.
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

import Backlight from "backlight";
import Button from "button";
import LED from "led/pwm";

class ButtonA {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonA,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonB {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonB,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonX {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonX,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonY {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonY,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonUP {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonUP,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonDOWN {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonDOWN,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonLEFT {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonLEFT,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonRIGHT {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonRIGHT,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonCENTER {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonCENTER,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	I2C: {
		default: {
			io: "I2C",
			data: 4,
			clock: 5,
			port: 0
		}
	},
	SPI: {
		default: {
			io: "SPI",
			clock: 10,
			out: 11,
			port: 1
		}
	},
	Analog: {
		default: {
			io: "Analog",
			pin: 26
		}
	},
	pin: {
		button: 15,
		buttonA: 15,
		buttonB: 17,
		buttonX: 19,
		buttonY: 21,
		buttonUP: 2,
		buttonDOWN: 18,
		buttonLEFT: 16,
		buttonRIGHT: 20,
		buttonCENTER: 3,
		led: 25,
		backlight: 13
	},
	peripheral: {
		button: {
			Default: ButtonA,
			A: ButtonA,
			B: ButtonB,
			X: ButtonX,
			Y: ButtonY,
			UP: ButtonUP,
			DOWN: ButtonDOWN,
			LEFT: ButtonLEFT,
			RIGHT: ButtonRIGHT,
			CENTER: ButtonCENTER
		},
		Backlight: class {
			constructor() {
				return new Backlight({
					io: device.io.PWM,
					pin: device.pin.backlight
				});
			}
		},
		led: {
			Default: class {
				constructor(options) {
					return new LED({
						...options,
						io: device.io.PWM,
						pin: device.pin.led
					});
				}
			}
		}
	}
};

export default device;


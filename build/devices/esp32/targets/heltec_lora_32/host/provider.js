/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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
 * FIXME: Analog port needs updating.
 * NOTES: Not sure how the I2C screen works here.
 * NOTES: Pins LED is updated.  backlight: 18, displayDC: 2, displaySelect: 15 are placeholders.
 * Pinout is referenced below.
 * https://resource.heltec.cn/download/WiFi_Kit_32/WIFI_Kit_32_pinoutDiagram_V2.pdf
 */

import LoRa from "sx127x";

import Button from "button";
import LED from "led/pwm";

class ButtonFlash {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.button,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	i2c: {
		default: {
			io: "I2C",
			data: 21,
			clock: 22
		}
	},
	serial: {
		default: {
			io: "Serial",
			port: 1,
			receive: 3,
			transmit: 1
		}
	},
	spi: {
		default: {
			io: "SPI",
			clock: 5,
			in: 19,
			out: 27,
			port: 1
		}
	},
	analog: {
		default: {
			io: "Analog",
			pin: 35
		}
	},
	pin: {
		button: 0,
		led: 25,
		displayDC: 2,
		displaySelect: 15
	},
	peripheral: {
		button: {
			Default: ButtonFlash,
			Flash: ButtonFlash
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
		},
		lora: {
			Default: class {
				constructor(options) {
					return new LoRa ({
						...options,
						spi: {
							...device.spi.default,
							hz: 9_000_000,
							mode: 0
						},
						reset: {
							io: device.io.Digital,
							pin: 14,
							mode: device.io.Digital.Output,
						},
						interrupt: {
							io: device.io.Digital,
							pin: 26,
							mode: device.io.Digital.Input
						},
						select: {
							io: device.io.Digital,
							pin: 18,
							mode: device.io.Digital.Output
						}
					});
				}
			}
		}
	}
};

export default device;

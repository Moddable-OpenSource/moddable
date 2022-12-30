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

import Analog from "embedded:io/analog";
import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";
import I2C from "embedded:io/i2c";
import PulseCount from "embedded:io/pulsecount";
import PWM from "embedded:io/pwm";
import Serial from "embedded:io/serial";
import SMBus from "embedded:io/smbus";
import SPI from "embedded:io/spi";
import LoRa from "sx127x";

class Button {
	#io;
	#onPush;

	constructor(options) {
		options = {...options};
		if (options.onReadable || options.onWritable || options.onError)
			throw new Error;

		if (options.target)
			this.target = options.target;

		const Digital = options.io;
		if (options.onPush) {
			this.#onPush = options.onPush; 
			options.onReadable = () => this.#onPush();
			options.edge = Digital.Rising | Digital.Falling;
		}

		this.#io = new Digital(options);
		this.#io.pressed = options.invert ? 0 : 1;
	}
	close() {
		this.#io?.close();
		this.#io = undefined;
	}
	get pressed() {
		return (this.#io.read() === this.#io.pressed) ? 1 : 0;
	}
}

const device = {
	I2C: {
		default: {
			io: I2C,
			data: 21,
			clock: 22
		}
	},
	Serial: {
		default: {
			io: Serial,
			port: 1,
			receive: 3,
			transmit: 1
		}
	},
	SPI: {
		default: {
			io: SPI,
			clock: 5,
			in: 19,
			out: 27,
			port: 1
		}
	},
	Analog: {
		default: {
			io: Analog,
			pin: 35
		}
	},
	io: {Analog, Digital, DigitalBank, I2C, PulseCount, PWM, Serial, SMBus, SPI},
	pin: {
		button: 0,
		led: 25,
		backlight: 18,
		displayDC: 2,
		displaySelect: 15
	},
	peripheral: {
		Backlight: class {
			constructor() {
				return new Backlight({pin: device.pin.backlight});
			}
		},
		button: {
			Flash: class {
				constructor(options) {
					return new Button({
						...options,
						io: Digital,
						pin: device.pin.button,
						mode: Digital.InputPullUp,
						invert: true
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
							...device.SPI.default,
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

/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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
			clock: 18,
			in: 19,
			out: 23,
			port: 1
		}
	},
	analog: {
		default: {
			io: "Analog",
			pin: 35
		}
	},
	pins: {
		button: 0,
		led: 25,
		backlight: 18,
		displayDC: 2,
		displaySelect: 15
	}
};

export default device;

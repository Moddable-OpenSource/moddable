/*
 * Copyright (c) 2021-2026  Moddable Tech, Inc.
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

import Touch from "embedded:sensor/Touch/GT911";
import HumidityTemperature from "embedded:sensor/Humidity-Temperature/SHT3x"
import RTC from "embedded:RTC/PCF8563"
import Button from "button";

class M5PaperTouch extends Touch {
	constructor(options) {
		let i2c, address = 0x14;
		
		// I2C address floats: try both
		try {
			i2c = new device.io.I2C({...device.I2C.default, address, hz: 100_000});
			i2c.read(1);
		}
		catch {
			address = 0x5D;
			i2c = new device.io.I2C({...device.I2C.default, address, hz: 100_000});
			i2c.read(1);
		}
		i2c.close();
		
		const o = {
			sensor: {...device.I2C.default, address},
			interrupt: {
				io: device.io.Digital,
				mode: device.io.Digital.Input,
				pin: device.pin.touchInterrupt
			}
		};
		if (options?.onSample)
			o.onSample = options.onSample; 

		super(o);
	}
	sample() {
		const sample = super.sample();
		if (!sample)
			return;
		// adjusts coordinates to match touch / EPD
		for (let i = 0; i < sample.length; i++) {
			const p = sample[i];
			const t = p.x;
			p.x = p.y;
			p.y = 540 - t;
		}
		return sample;
	}
}

class ButtonA {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: 38,
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
			pin: 37,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

class ButtonC {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: 39,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
	I2C: {
		default: {
			io: "I2C",
			data: 21,
			clock: 22
		}
	},
	SPI: {
		default: {
			io: "SPI",
			clock: 14,
			in: 13,
			out: 12,
			port: 2,		// VSPI_HOST
		}
	},
	pin: {
		powerMain: 2,
		powerExternal: 5,
		powerEPD: 23,
		touchInterrupt: 36,
		epdSelect: 15,     
		epdBusy: 27,
		batteryVoltage: 35
	},
	sensor: {
		Touch: M5PaperTouch,
		HumidityTemperature: class {
			constructor(options) {
				return new HumidityTemperature({
					sensor: {
						...device.I2C.default
					}
				});
			}
		}
	},
	peripheral: {
		RTC: class {
			constructor(options) {
				return new RTC({
					...options,
					clock: {
						...device.I2C.default,
						io: device.io.SMBus
					}
				});
			}
		},
		button: {
			Default: ButtonA,
			A: ButtonA,
			B: ButtonB,
			C: ButtonC
		},
		battery: {
			Default: class {
				#analog;
				constructor() {
					this.#analog = new device.io.Analog({pin: device.pin.batteryVoltage});
				}
				close() {
					this.#analog?.close();
				}
				read() {
					let value = 0;
					for (let i = 0; i < 100; i++) {
						let voltage = (this.#analog.read() / (1 << this.#analog.resolution) / 0.5) * 3300;
						value += Math.max(3300, Math.min(4300, voltage));
					}
					value /= 100;
					return ((value - 3300) / (4300 - 3300));
				}
			}
		}
	}
};

export default device;

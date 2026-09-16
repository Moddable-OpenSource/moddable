/*
 * Copyright (c) 2024-2026  Moddable Tech, Inc.
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

import Touch from 'embedded:sensor/Touch/FT3168'
import RTC from 'embedded:RTC/PCF85063'
import QMI8658 from 'embedded:sensor/Accelerometer-Gyroscope/QMI8658'

import Button from "button";
import AXP2101 from 'embedded:peripheral/Power/axp2101'

class Backlight {
	constructor(brightness = 100) {
		this.write(brightness);
	}
	write(value) {
		if (globalThis.screen)
			globalThis.screen.brightness = value;
	}
	close() {}
};

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

const device = {
  I2C: {
    default: {
      io: "I2C",
      data: 15,
      clock: 14
    }
  },
  SPI: {
    default: {
      io: "SPI",
      port: 2,
      clock: 11,
      out: 4
    }
  },
  pin: {
    button: 0,
	buttonA: 0,
    buttonB: 10,
	amplifier: 46
  },
  peripheral: {
	button: {
		Default: ButtonA,
		A: ButtonA,
		B: ButtonB
	},
	Backlight,
    Power: {
      PMIC: class {
        constructor() {
          return new AXP2101({
            sensor: { ...device.I2C.default, io: device.io.SMBus },
            address: 0x34
          });
        }
      },
      Amplifier: class {
        constructor() {
          return new device.io.Digital({
            io: device.io.Digital,
            pin: device.pin.amplifier,
            mode: device.io.Digital.Output,
            initialValue: 1
          });
		}
      }
    },
    RTC: class {
      constructor(options) {
        return new RTC({
          ...options,
          clock: {
            ...device.I2C.default,
            io: device.io.SMBus
          }
        })
      }
	}
  },
  sensor: {
    Touch: class {
      constructor(options) {
        const result = new Touch({
          ...options,
          sensor: {
            ...device.I2C.default,
            io: device.io.SMBus
          },
          reset: {
            io: device.io.Digital,
            mode: device.io.Digital.Output,
            pin: 9
          },
          interrupt: {
            io: device.io.Digital,
            mode: device.io.Digital.Input,
            pin: 38
          }
        })
        result.configure({ length: 1 })
        return result
      }
    },
    IMU: class extends QMI8658 {
      constructor(options) {
        super({
          ...options,
          sensor: {
            ...device.I2C.default,
            io: device.io.SMBus
          }
        });
      }
	  sample() {
		const sample = super.sample();
		[sample.accelerometer.x, sample.accelerometer.y, sample.accelerometer.z] =
		  [sample.accelerometer.y, sample.accelerometer.x, sample.accelerometer.z * -1];
        [sample.gyroscope.x, sample.gyroscope.y, sample.gyroscope.z] =
		  [sample.gyroscope.y, sample.gyroscope.x, sample.gyroscope.z * -1];
		return sample;
	  }
    }
  },
  xperipheral: {
    button: {
      A: class {
        constructor(options) {
          return new device.io.Digital({
            ...options,
            pin: device.pin.button,
            mode: device.io.Digital.InputPullUp
          })
        }
      }
    }
  }
}

export default device

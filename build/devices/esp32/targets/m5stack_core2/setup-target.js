/*
 * Copyright (c) 2020-2022  Moddable Tech, Inc.
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
 
import AXP192 from "axp192";
import MPU6886 from "mpu6886";
import AudioOut from "pins/audioout";
import Resource from "Resource";
import Timer from "timer";
import config from "mc/config";
import I2C from "pins/i2c";

const INTERNAL_I2C = Object.freeze({
	sda: 21,
	scl: 22
});

const state = {
  handleRotation: nop,
};

globalThis.Host = {
  Backlight: class {
    constructor(brightness = 100) {
      this.write(brightness);
    }
    write(value) {
      if (undefined !== globalThis.power)
        globalThis.power.brightness = value;
    }
    close() {}
  }
}

class M5Core2Button {		// M5StackCoreTouch calls write when button changes 
	#value = 0;
	read() {
		return this.#value;
	}
	write(value) {
		if (this.#value === value)
			return;
		this.#value = value;
		this.onChanged?.();
	}
}

export default function (done) {
	// buttons
	globalThis.button = {
		a: new M5Core2Button,
		b: new M5Core2Button,
		c: new M5Core2Button,
	};

	// power
	globalThis.power = new Power();

	// speaker
	power.speaker.enable = true;

	// start-up sound
	if (config.startupSound) {
    const speaker = new AudioOut({streams: 1});
		speaker.callback = function () {
			this.stop();
			this.close();
			this.done();
		};
		speaker.done = done;
		done = undefined;

		speaker.enqueue(0, AudioOut.Samples, new Resource(config.startupSound));
		speaker.enqueue(0, AudioOut.Callback, 0);
		speaker.start();
	}

  // vibration
  globalThis.vibration = {
    read: function () {
      return power.vibration.enable;
    },
    write: function (v) {
      power.vibration.enable = v;
    },
  };

  if (config.startupVibration) {
    vibration.write(true);
    Timer.set(() => {
      vibration.write(false);
    }, config.startupVibration);
  }

  // accelerometer and gyrometer
  const test = new I2C({...INTERNAL_I2C, address: 0x68, throw: false});
  test.write(0x75, false);
  const ok = test.read(1);
  test.close();
  if (undefined !== ok) {
    state.accelerometerGyro = new MPU6886(INTERNAL_I2C);

    globalThis.accelerometer = {
      onreading: nop,
    };

    globalThis.gyro = {
      onreading: nop,
    };

    accelerometer.start = function (frequency) {
      accelerometer.stop();
      state.accelerometerTimerID = Timer.repeat((id) => {
        state.accelerometerGyro.configure({
          operation: "accelerometer",
        });
        const sample = state.accelerometerGyro.sample();
        if (sample) {
          state.handleRotation(sample);
          accelerometer.onreading(sample);
        }
      }, frequency);
    };

    gyro.start = function (frequency) {
      gyro.stop();
      state.gyroTimerID = Timer.repeat((id) => {
        state.accelerometerGyro.configure({
          operation: "gyroscope",
        });
        const sample = state.accelerometerGyro.sample();
        if (sample) {
          let { x, y, z } = sample;
          const temp = x;
          x = y * -1;
          y = temp * -1;
          z *= -1;
          gyro.onreading({
            x,
            y,
            z,
          });
        }
      }, frequency);
    };

    accelerometer.stop = function () {
      if (undefined !== state.accelerometerTimerID)
        Timer.clear(state.accelerometerTimerID);
      delete state.accelerometerTimerID;
    };

    gyro.stop = function () {
      if (undefined !== state.gyroTimerID) Timer.clear(state.gyroTimerID);
      delete state.gyroTimerID;
    };
  }

  // autorotate
  if (config.autorotate && globalThis.Application && globalThis.accelerometer) {
    state.handleRotation = function (reading) {
      if (globalThis.application === undefined) return;

      if (Math.abs(reading.y) > Math.abs(reading.x)) {
        if (reading.y < -0.7 && application.rotation != 180) {
          application.rotation = 180;
        } else if (reading.y > 0.7 && application.rotation != 0) {
          application.rotation = 0;
        }
      } else {
        if (reading.x < -0.7 && application.rotation != 270) {
          application.rotation = 270;
        } else if (reading.x > 0.7 && application.rotation != 90) {
          application.rotation = 90;
        }
      }
    };
    accelerometer.start(300);
  }

  done?.();
}

class Power extends AXP192 {
  constructor() {
    super(INTERNAL_I2C);

    // TODO: encapsulate direct register access by class method
    this.writeByte(0x30, (this.readByte(0x30) & 0x04) | 0x02); //AXP192 30H
    this.writeByte(0x92, this.readByte(0x92) & 0xf8); //AXP192 GPIO1:OD OUTPUT
    this.writeByte(0x93, this.readByte(0x93) & 0xf8); //AXP192 GPIO2:OD OUTPUT
    this.writeByte(0x35, (this.readByte(0x35) & 0x1c) | 0xa3); //AXP192 RTC CHG

    // main power line
    this._dcdc1.voltage = 3350;
    this.chargeCurrent = AXP192.CHARGE_CURRENT.Ch_100mA;

    // LCD
    this.lcd = this._dcdc3;
    this.lcd.voltage = 2800;

    // internal LCD logic
    this._ldo2.voltage = 3300;
    this._ldo2.enable = true;

    // Vibration
    this.vibration = this._ldo3;
    this.vibration.voltage = 2000;

    // Speaker
    this.speaker = this._gpio2;

    // AXP192 GPIO4
    this.writeByte(0x95, (this.readByte(0x95) & 0x72) | 0x84);
    this.writeByte(0x36, 0x4c);
    this.writeByte(0x82, 0xff);
    this.resetLcd();
    this.busPowerMode = 0; //  bus power mode_output
    Timer.delay(200);
  }

  resetLcd() {
    this._gpio4.enable = false
    Timer.delay(20);
    this._gpio4.enable = true
  }

  set busPowerMode(mode) {
    if (mode == 0) {
      this.writeByte(0x91, (this.readByte(0x91) & 0x0f) | 0xf0);
      this.writeByte(0x90, (this.readByte(0x90) & 0xf8) | 0x02); //set GPIO0 to LDO OUTPUT , pullup N_VBUSEN to disable supply from BUS_5V
      this.writeByte(0x12, this.readByte(0x12) | 0x40); //set EXTEN to enable 5v boost
    } else {
      this.writeByte(0x12, this.readByte(0x12) & 0xbf); //set EXTEN to disable 5v boost
      this.writeByte(0x90, (this.readByte(0x90) & 0xf8) | 0x01); //set GPIO0 to float , using enternal pulldown resistor to enable supply from BUS_5VS
    }
  }
  
  // value 0 - 100 %
  set brightness(value) {
    if (value <= 0)
      value = 2500;
    else if (value >= 100)
      value = 3300;
    else
      value = (value / 100) * 800 + 2500;
    this.lcd.voltage = value;
  }
  
  get brightness() {
    return (this.lcd.voltage - 2500) / 800 * 100;
  }
}

function nop() {}

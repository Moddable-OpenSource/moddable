/*
 * Copyright (c) 2020-2026  Moddable Tech, Inc.
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
 
import BMI270 from "embedded:sensor/Accelerometer-Gyroscope-Magnetometer/BMI270";
import EmbeddedSMBus from "embedded:io/smbus";
import MPU6886 from "mpu6886";
import AudioOut from "pins/audioout";
import Resource from "Resource";
import Timer from "timer";
import config from "mc/config";
import I2C from "pins/i2c";
import SMBus from "pins/smbus";

const INTERNAL_I2C = Object.freeze({
	sda: 21,
	scl: 22
});

const INTERNAL_I2C_IO = Object.freeze({
	data: 21,
	clock: 22
});
const ACCELERATION_SCALER = 1 / 9.80665;
const IMU_ADDRESS = 0x68;
const BMI270_CHIP_ID_ADDR = 0x00;
const BMI270_CHIP_ID = 0x24;
const MPU6886_WHO_AM_I_ADDR = 0x75;
const MPU6886_WHO_AM_I = 0x19;

const state = {
  handleRotation: nop,
};

class Core2BMI270 extends BMI270 {
	#operation = "gyroscope";

	constructor() {
		super({
			sensor: {
				...INTERNAL_I2C_IO,
				address: IMU_ADDRESS,
				io: EmbeddedSMBus
			}
		});
	}

	configure(dictionary) {
		if (dictionary?.operation)
			this.#operation = dictionary.operation;
	}

	sample() {
		const sample = super.sample();
		let result;

		switch (this.#operation) {
			case "accelerometer":
				result = sample.accelerometer;
				if (result) {
					result.x *= ACCELERATION_SCALER;
					result.y *= ACCELERATION_SCALER;
					result.z *= ACCELERATION_SCALER;
				}
				return result;
			case "gyroscope":
				return sample.gyroscope;
			case "temp":
				return sample.thermometer?.temperature;
		}
	}
}

function createAccelerometerGyro() {
	if (MPU6886_WHO_AM_I === readIMURegister(MPU6886_WHO_AM_I_ADDR))
		return new MPU6886(INTERNAL_I2C);

	if (BMI270_CHIP_ID === readIMURegister(BMI270_CHIP_ID_ADDR))
		return new Core2BMI270;
}

function getAccelerometerGyro() {
	if (undefined === state.accelerometerGyro)
		state.accelerometerGyro = createAccelerometerGyro();
	return state.accelerometerGyro;
}

function readIMURegister(register) {
	let io;
	try {
		io = new I2C({...INTERNAL_I2C, address: IMU_ADDRESS, throw: false});
		let result = io.write(register, false);
		if (result instanceof Error)
			return;

		result = io.read(1);
		if (result instanceof Error)
			return;

		return result?.[0];
	}
	catch (e) {
	}
	finally {
		io?.close();
	}
}

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

  globalThis.power = new device.peripheral.Power()

  if (config.enablePowerButton) {
    globalThis.button.power = new M5Core2Button();
    // AXP192/AXP2101 PEK reports latched press events, so expose them as a short 1 -> 0 pulse.
    Timer.repeat(() => {
      const state = globalThis.power.getPekState();
      globalThis.button.power.write(state ? 1 : 0);
    }, 100);
  }

	// speaker
	globalThis.power.speaker.enable = true;

	// start-up sound
	if (config.startupSound) {
    const speaker = new AudioOut({streams: 1});
		speaker.callback = function () {
			this.stop();
			this.close();
			Timer.set(this.done);
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
      return globalThis.power.vibration.enable;
    },
    write: function (v) {
      globalThis.power.vibration.enable = v;
    },
  };

  if (config.startupVibration) {
    vibration.write(true);
    Timer.set(() => {
      vibration.write(false);
    }, config.startupVibration);
  }

  // accelerometer and gyrometer
  globalThis.accelerometer = {
    onreading: nop,
  };

  globalThis.gyro = {
    onreading: nop,
  };

  accelerometer.start = function (frequency) {
    accelerometer.stop();
    state.accelerometerTimerID = Timer.repeat((id) => {
      const accelerometerGyro = getAccelerometerGyro();
      if (undefined === accelerometerGyro)
        return;

      accelerometerGyro.configure({
        operation: "accelerometer",
      });
      const sample = accelerometerGyro.sample();
      if (sample) {
        state.handleRotation(sample);
        accelerometer.onreading(sample);
      }
    }, frequency);
  };

  gyro.start = function (frequency) {
    gyro.stop();
    state.gyroTimerID = Timer.repeat((id) => {
      const accelerometerGyro = getAccelerometerGyro();
      if (undefined === accelerometerGyro)
        return;

      accelerometerGyro.configure({
        operation: "gyroscope",
      });
      const sample = accelerometerGyro.sample();
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

function nop() {}

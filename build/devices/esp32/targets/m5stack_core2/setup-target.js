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

import AudioOut from "pins/audioout";
import Resource from "Resource";
import Timer from "timer";
import config from "mc/config";

globalThis.Host = {
  Backlight: class {
    constructor(brightness = 100) {
      this.write(brightness);
    }
    write(value) {
      if (undefined !== globalThis.power)
        globalThis.power.brightness = value;
    }
    close() { }
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
    const speaker = new AudioOut({ streams: 1 });
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
    read: () => {
      return globalThis.power.vibration.enable;
    },
    write: (v) => {
      globalThis.power.vibration.enable = v;
    },
  };

  if (config.startupVibration) {
    globalThis.vibration.write(true);
    Timer.set(() => {
      globalThis.vibration.write(false);
    }, config.startupVibration);
  }

  if (config.autorotate && globalThis.Application) {
    const imu = new device.sensor.IMU();
    Timer.repeat(id => {
      const sample = imu.sample();
      const { x, y } = sample.accelerometer;
      if (Math.abs(y) > Math.abs(x)) {
        if (y < -0.7 && application.rotation !== 270) {
          application.rotation = 270;
        } else if (y > 0.7 && application.rotation !== 90) {
          application.rotation = 90;
        }
      } else {
        if (x < -0.7 && application.rotation !== 180) {
          application.rotation = 180;
        } else if (x > 0.7 && application.rotation !== 0) {
          application.rotation = 0;
        }
      }
    }, 300);
  }

  done?.();
}

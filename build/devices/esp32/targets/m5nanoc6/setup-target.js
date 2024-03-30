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
import NeoPixel from "neopixel";
import Timer from "timer";
import Button from "button";
import LED from "led";
import Digital from "pins/digital";

class NeoPixelLED extends NeoPixel {
  #value = 0;
  constructor(options) {
    // Enable RGB LED Power
    const power = new Digital({
      pin: config.led.rgb.power_pin,
      mode: Digital.Output,
    });
    power.write(1);

    super({
      ...options,
      pin: config.led.rgb.data_pin,
      length: 1,
      order: "GRB",
    });
    super.brightness = config.led.rgb.brightness;
  }
  read() {
    return this.#value;
  }
  write(value) {
    this.#value = value;
    if (value) {
      super.setPixel(0, super.makeRGB(255, 255, 255));
    } else {
      super.setPixel(0, super.makeRGB(0, 0, 0));
    }
    super.update();
  }
  on() {
    this.write(1);
  }
  off() {
    this.write(0);
  }
}

class BlueLED {
  constructor(options) {
    return new LED({
      ...options,
      pin: config.led.blue.pin,
    });
  }
}

class Flash {
  constructor(options) {
    return new Button({
      ...options,
      pin: 9,
      invert: true,
    });
  }
}

globalThis.Host = Object.freeze(
  {
    LED: {
      Default: NeoPixelLED,
      RGB: NeoPixelLED,
      Blue: BlueLED,
    },
    Button: {
      Default: Flash,
      Flash,
    },
  },
  true
);

export default function (done) {
  done?.();
}

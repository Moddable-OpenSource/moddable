/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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

import Analog from "embedded:io/analog";
import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";
import I2C from "embedded:io/i2c";
import PulseCount from "embedded:io/pulsecount";
import PWM from "embedded:io/pwm";
import Serial from "embedded:io/serial";
import SMBus from "embedded:io/smbus";
import SPI from "embedded:io/spi";
import RTC from "embedded:RTC/BM8563";
import Touch from "embedded:sensor/Touch/FT6x06";

import Timer from "timer";

//@@ Move Button class to common module
class Button {
  #io;
  #onPush;

  constructor(options) {
    options = { ...options };
    if (options.onReadable || options.onWritable || options.onError)
      throw new Error();

    if (options.target) this.target = options.target;

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
    return this.#io.read() === this.#io.pressed ? 1 : 0;
  }
}

//@@ Move Tone class to common module
const notes = new Map();
notes.set("C", 4186);
notes.set("Db", 4435);
notes.set("C#", 4435);
notes.set("D", 4699);
notes.set("D#", 4978);
notes.set("Eb", 4978);
notes.set("E", 5274);
notes.set("F", 5588);
notes.set("F#", 5920);
notes.set("Gb", 5920);
notes.set("G", 6272);
notes.set("G#", 6645);
notes.set("Ab", 6645);
notes.set("A", 7040);
notes.set("A#", 7459);
notes.set("Bb", 7459);
notes.set("B", 790);

class Tone {
  #io;
  #timer;

  constructor() {
    this.#io = new PWM({ pin: device.pin.buzzer });
  }
  close() {
    this.#io?.close();
    if (this.#timer) Timer.clear(this.#timer);
    this.#io = this.#timer = undefined;
  }
  tone(hz, duration) {
    const io = (this.#io = new PWM({ from: this.#io, hz }));
    io.write(512);

    if (duration) {
      if (this.#timer) Timer.schedule(this.#timer, duration);
      else
        this.#timer = Timer.set(() => {
          this.#timer = undefined;
          this.mute();
        }, duration);
    } else if (this.#timer) {
      Timer.clear(this.#timer);
      this.#timer = undefined;
    }
  }
  note(note, octave = 4, duration) {
    note = notes.get(note);
    if (!note || octave > 8) throw new Error();
    this.tone(Math.idiv(note, 1 << (8 - octave)), duration);
  }
  mute() {
    this.#io.write(0);
  }
}

const device = {
  I2C: {
    default: {
      io: I2C,
      data: 13,
      clock: 15,
    },
    internal: {
      io: I2C,
      data: 11,
      clock: 12,
    },
  },
  SPI: {
    default: {
      io: SPI,
      port: 3,
      clock: 6,
      out: 5,
    },
  },
  io: {
    Analog,
    Digital,
    DigitalBank,
    I2C,
    PulseCount,
    PWM,
    Serial,
    SMBus,
    SPI,
  },
  pin: {
    button: 42,
    buzzer: 3,
    signal: 41,
    control: 40,
  },
  sensor: {
    Touch: class {
      constructor(options) {
        const result = new Touch({
          ...options,
          sensor: {
            ...device.I2C.internal,
            io: device.io.SMBus,
          },
          interrupt: {
            io: Digital,
            mode: Digital.Input,
            pin: 14,
          },
        });
        result.configure({});
        return result;
      }
    },
  },
  peripheral: {
    button: {
      A: class {
        constructor(options) {
          return new Button({
            ...options,
            io: Digital,
            pin: device.pin.button,
            mode: Digital.InputPullUp,
            invert: true,
          });
        }
      },
    },
    tone: {
      Default: Tone,
    },
    RotaryEncoder: class {
      constructor(options) {
        return new PulseCount({
          ...options,
          signal: device.pin.signal,
          control: device.pin.control,
        });
      }
    },
    RTC: class {
      constructor(options) {
        return new RTC({
          ...options,
          clock: {
            ...device.I2C.internal,
            io: SMBus,
          },
        });
      }
    },
  },
};

export default device;

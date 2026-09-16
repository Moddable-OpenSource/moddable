/*
 * Copyright (c) 2023-2026  Moddable Tech, Inc.
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

import RTC from "embedded:RTC/BM8563";
import Touch from "embedded:sensor/Touch/FT6x06";
import MFRC522 from "embedded:sensor/RFID/MFRC522";

import Timer from "timer";
import Backlight from "backlight";
import Button from "button";

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
    this.#io = new device.io.PWM({ pin: device.pin.buzzer });
  }
  close() {
    this.#io?.close();
    if (this.#timer) Timer.clear(this.#timer);
    this.#io = this.#timer = undefined;
  }
  tone(hz, duration) {
    const io = (this.#io = new device.io.PWM({ from: this.#io, hz }));
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

class ButtonFlash {
	constructor(options) {
		return new Button({
			...options,
			io: device.io.Digital,
			pin: device.pin.buttonFlash,
			mode: device.io.Digital.InputPullUp,
			activeLow: true
		});
	}
}

const device = {
  I2C: {
    default: {
      io: "I2C",
      data: 13,
      clock: 15
    },
    internal: {
      io: "I2C",
      data: 11,
      clock: 12
    }
  },
  SPI: {
    default: {
      io: "SPI",
      port: 3,
      clock: 6,
      out: 5
    }
  },
  pin: {
    button: 42,
    buttonA: 42,
    buttonFlash: 0,
	backlight: 9,
    buzzer: 3,
    signal: 41,
    control: 40
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
            io: device.io.Digital,
            mode: device.io.Digital.Input,
            pin: 14,
          },
        });
        result.configure({});
        return result;
      }
    },
    /*
     * Built-in WS1850S/MFRC522 on internal I2C @ 0x28 (G11 SDA / G12 SCL).
     * Do not pass reset: RST is shared with LCD_RESET (G8).
     */
    RFID: class {
      constructor(options = {}) {
        return new MFRC522({
          ...options,
          sensor: {
            ...device.I2C.internal,
            io: device.io.SMBus,
            address: 0x28
          }
        });
      }
    }
  },
  peripheral: {
	Backlight: class {
		constructor() {
			return new Backlight({
				io: device.io.PWM,
				pin: device.pin.backlight
			});
		}
	},
	button: {
		Default: ButtonFlash,
		A: ButtonA,
		Flash: ButtonFlash
	},
    tone: {
      Default: Tone
    },
    RotaryEncoder: class {
      constructor(options) {
        return new device.io.PulseCount({
          ...options,
          signal: device.pin.signal,
          control: device.pin.control
        });
      }
    },
    RTC: class {
      constructor(options) {
        return new RTC({
          ...options,
          clock: {
            ...device.I2C.internal,
            io: device.io.SMBus
          }
        });
      }
    }
  }
};

export default device;

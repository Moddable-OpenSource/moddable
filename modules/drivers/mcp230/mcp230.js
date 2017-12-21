/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

/*
  Microchip GPIO expander
*/

import SMBus from "pins/smbus";

// Default State: MCP23008
const INPUTS_8   = 0b11111111;
const PULLUPS_8  = 0b00000000;

// Default State: MCP23017
const INPUTS_16  = 0b1111111111111111;
const PULLUPS_16 = 0b0000000000000000;

// Default Config (Both)
const ADDRESS = 0x20;
const HZ = 0x00;
const SDA = 0x04;
const SCL = 0x05;

// Modes (Ref: modules/pins/digital)
const INPUT = 0;
const INPUT_PULLUP = 1;
const OUTPUT = 8;

class Expander extends SMBus {
  constructor(dictionary) {

    const offset = (dictionary.pins >> 3) - 1;
    const {
      address = ADDRESS,
      hz = HZ,
      sda = SDA,
      scl = SCL,

      // User specific state initialization settings
      inputs = offset ? INPUTS_16 : INPUTS_8,
      pullups = offset ? PULLUPS_16 : PULLUPS_8,
      pins,
      reg,
      reg: { IODIR, GPIO, GPPU }
    } = dictionary || {};

    super({ address, hz, sda, scl });

    // trace(`IODIR: 0x${IODIR.toString(16)}\n`);
    // trace(`GPIO: 0x${GPIO.toString(16)}\n`);
    // trace(`GPPU: 0x${GPPU.toString(16)}\n`);

    // If the value of inputs does not match the default,
    // then set the user defined inputs configuration
    if ((inputs !== (offset ? INPUTS_16 : INPUTS_8))) {
      this.writeByte(IODIR, inputs);

      if (offset) {
        this.writeByte(IODIR + offset, inputs >> 8);
      }
    }

    // If the value of pullups does not match the default,
    // then set the user defined pullups configuration
    if ((pullups !== (offset ? PULLUPS_16 : PULLUPS_8))) {
      this.writeByte(GPPU, pullups);

      if (offset) {
        this.writeByte(GPPU + offset, pullups >> 8);
      }
    }

    for (let pin = 0; pin < pins; pin++) {
      this[pin] = new Pin({ pin, expander: this });
    }

    this.length = pins;
    this.offset = offset;
    this.reg = Object.freeze(reg);

    Object.freeze(this);
  }

  * [Symbol.iterator]() {
    let i = 0;
    while (i < this.length) {
      yield this[i];
      i++;
    }
  }

  bankWrite(state) {
    const { IODIR, GPIO } = this.reg;

    if (this.offset) {
      // Read IODIR state
      let iodir = this.readWord(IODIR);

      // Set IODIR state to OUTPUT
      this.writeWord(IODIR, 0x0000);

      // Write GPIO
      this.writeWord(GPIO, state);

      // Restore previous IODIR state
      this.writeWord(IODIR, iodir);
    } else {
      // Read IODIR state
      let iodir = this.readByte(IODIR);

      // Set IODIR state to OUTPUT
      this.writeByte(IODIR, 0x00);

      // Write GPIO
      this.writeByte(GPIO, state & 0xFF);

      // Restore previous IODIR state
      this.writeByte(IODIR, iodir);
    }
  }
  bankRead() {
    const { IODIR, GPIO } = this.reg;

    if (this.offset) {
      // Read IODIR state
      let iodir = this.readWord(IODIR);

      // Set IODIR state to INPUT
      this.writeWord(IODIR, 0xFFFF);

      // Read GPIO
      let gpio = this.readWord(GPIO);

      // Restore previous IODIR state
      this.writeWord(IODIR, iodir);

      return gpio;
    } else {
      // Read IODIR state
      let iodir = this.readByte(IODIR);

      // Set IODIR state to INPUT
      this.writeByte(IODIR, 0xFF);

      // Read GPIO
      let gpio = this.readByte(GPIO);

      // Restore previous IODIR state
      this.writeByte(IODIR, iodir);

      return gpio;
    }
  }
}

class Pin {
  constructor(dictionary) {
    Object.assign(this, dictionary);
    Object.freeze(this);
  }

  mode(mode) {
    const offset = this.pin >> 3;
    const pin = offset ? this.pin - 8 : this.pin;
    const pinMask = 1 << pin;
    const IODIR = this.expander.reg.IODIR + offset;
    const GPPU = this.expander.reg.GPPU + offset;

    let iodir = this.expander.readByte(IODIR);
    let gppu = this.expander.readByte(GPPU);

    if (mode === INPUT ||
        mode === INPUT_PULLUP) {
      // Check and Set Direction
      if (!(iodir & pinMask)) {
        // When the bit set, the corresponding pin is an input
        iodir |= pinMask;
      }

      if (mode === INPUT_PULLUP) {
        gppu |= pinMask;
      }
    } else {
      // OUTPUT
      if (iodir & pinMask) {
        // When the bit is not set, the corresponding pin is an output
        iodir &= ~pinMask;
        gppu &= ~pinMask;
      }
    }

    this.expander.writeByte(IODIR, iodir);
    this.expander.writeByte(GPPU, gppu);
  }

  read() {
    const offset = this.pin >> 3;
    const pin = offset ? this.pin - 8 : this.pin;
    const pinMask = 1 << pin;
    const IODIR = this.expander.reg.IODIR + offset;
    const GPIO = this.expander.reg.GPIO + offset;

    let iodir = this.expander.readByte(IODIR);

    // Check and Set Direction
    if (!(iodir & pinMask)) {
      // When the bit set, the corresponding pin is an input
      iodir |= pinMask;
      this.expander.writeByte(IODIR, iodir);
    }

    return (this.expander.readByte(GPIO) >> pin) & 1;
  }

  write(value) {
    const offset = this.pin >> 3;
    const pin = offset ? this.pin - 8 : this.pin;
    const pinMask = 1 << pin;
    const IODIR = this.expander.reg.IODIR + offset;
    const GPIO = this.expander.reg.GPIO + offset;


    let gpio = this.expander.readByte(GPIO);
    let iodir = this.expander.readByte(IODIR);

    // Check and Set Direction
    if (iodir & pinMask) {
      // When the bit is not set, the corresponding pin is an output
      iodir &= ~pinMask;
      this.expander.writeByte(IODIR, iodir);
    }

    if (value) {
      gpio |= pinMask;
    } else {
      gpio &= ~pinMask;
    }

    this.expander.writeByte(GPIO, gpio);
  }
}

class MCP23008 extends Expander {
  constructor(dictionary = {}) {
    super(
      Object.assign(dictionary, {
        pins: 8,
        reg: {
          IODIR: 0x00,
          GPPU: 0x06,
          GPIO: 0x09,
        }
      })
    );
  }
}

class MCP23017 extends Expander {
  constructor(dictionary = {}) {
    super(
      Object.assign(dictionary, {
        pins: 16,
        reg: {
          IODIR: 0x00,
          GPPU: 0x0C,
          GPIO: 0x12,
        }
      })
    );
  }
}

export { MCP23008, MCP23017 };

/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

// Modes (Ref: modules/pins/digital)
const INPUT = 0;
const INPUT_PULLUP = 1;
const OUTPUT = 8;

class Expander extends SMBus {
  constructor(dictionary = { address: 0x20 }) {
    super(dictionary);

    this.offset = (this.length >> 3) - 1;

    const {
      // User specific state initialization settings
      inputs = this.offset ? INPUTS_16 : INPUTS_8,
      pullups = this.offset ? PULLUPS_16 : PULLUPS_8,
    } = dictionary;

    // If the value of inputs does not match the default,
    // then set the user defined inputs configuration
    if ((inputs !== (this.offset ? INPUTS_16 : INPUTS_8))) {
      this.writeByte(this.IODIR, inputs);

      if (this.offset) {
        this.writeByte(this.IODIR + this.offset, inputs >> 8);
      }
    }

    // If the value of pullups does not match the default,
    // then set the user defined pullups configuration
    if ((pullups !== (this.offset ? PULLUPS_16 : PULLUPS_8))) {
      this.writeByte(this.GPPU, pullups);

      if (this.offset) {
        this.writeByte(this.GPPU + this.offset, pullups >> 8);
      }
    }

    for (let pin = 0; pin < this.length; pin++) {
      this[pin] = new Pin({ pin, expander: this });
    }

    Object.freeze(this);
  }

  write(state) {
    if (this.offset) {
      // Read IODIR state
      let iodir = this.readWord(this.IODIR);

      // Set IODIR state to OUTPUT
      this.writeWord(this.IODIR, 0x0000);

      // Write GPIO
      this.writeWord(this.GPIO, state);

      // Restore previous IODIR state
      this.writeWord(this.IODIR, iodir);
    } else {
      // Read IODIR state
      let iodir = this.readByte(this.IODIR);

      // Set IODIR state to OUTPUT
      this.writeByte(this.IODIR, 0x00);

      // Write GPIO
      this.writeByte(this.GPIO, state & 0xFF);

      // Restore previous IODIR state
      this.writeByte(this.IODIR, iodir);
    }
  }
  read() {
    if (this.offset) {
      // Read IODIR state
      let iodir = this.readWord(this.IODIR);

      // Set IODIR state to INPUT
      this.writeWord(this.IODIR, 0xFFFF);

      // Read GPIO
      let gpio = this.readWord(this.GPIO);

      // Restore previous IODIR state
      this.writeWord(this.IODIR, iodir);

      return gpio;
    } else {
      // Read IODIR state
      let iodir = this.readByte(this.IODIR);

      // Set IODIR state to INPUT
      this.writeByte(this.IODIR, 0xFF);

      // Read GPIO
      let gpio = this.readByte(this.GPIO);

      // Restore previous IODIR state
      this.writeByte(this.IODIR, iodir);

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
    const IODIR = this.expander.IODIR + offset;
    const GPPU = this.expander.GPPU + offset;

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
    const IODIR = this.expander.IODIR + offset;
    const GPIO = this.expander.GPIO + offset;

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
    const IODIR = this.expander.IODIR + offset;
    const GPIO = this.expander.GPIO + offset;

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

class MCP23008 extends Expander {}
MCP23008.prototype.length = 8;
MCP23008.prototype.IODIR = 0x00;
MCP23008.prototype.GPPU = 0x06;
MCP23008.prototype.GPIO = 0x09;
Object.freeze(MCP23008.prototype);

class MCP23017 extends Expander {}
MCP23017.prototype.length = 16;
MCP23017.prototype.IODIR = 0x00;
MCP23017.prototype.GPPU = 0x0C;
MCP23017.prototype.GPIO = 0x12;
Object.freeze(MCP23017.prototype);


export { MCP23008, MCP23017 };

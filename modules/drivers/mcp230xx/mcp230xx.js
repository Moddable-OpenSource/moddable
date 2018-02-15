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

// Modes (Ref: modules/pins/digital)
const INPUT = 0;
const INPUT_PULLUP = 1;
const OUTPUT = 8;

class Expander extends SMBus {
  constructor(dictionary = { address: 0x20 }) {
    super(dictionary);

    const {
      // User specific state initialization settings
      inputs = this.INPUTS,
      pullups = this.PULLUPS,
    } = dictionary;

    // If the value of inputs does not match the default,
    // then set the user defined inputs configuration
    if ((inputs !== this.INPUTS)) {
		this.writeSize(this.IODIR, inputs);
    }

    // If the value of pullups does not match the default,
    // then set the user defined pullups configuration
    if ((pullups !== this.PULLUPS)) {
        this.writeSize(this.GPPU, pullups);
    }

	let prototype = Object.freeze(Object.create(Pin.prototype, {
		expander: {
			writable: false,
			configurable: false,
			value: this
		}
	}));

	let properties = {
		pin: {
			writable: false,
			configurable: false,
		}
	};

	for (properties.pin.value = 0; properties.pin.value < this.length; properties.pin.value++) {
		this[properties.pin.value] = Object.freeze(Object.create(prototype, properties));
	}

    Object.freeze(this);
  }

  write(state) {
    // Read IODIR state
    let iodir = this.readSize(this.IODIR);

    // Set IODIR state to OUTPUT
    this.writeSize(this.IODIR, 0x0000);

    // Write GPIO
    this.writeSize(this.GPIO, state);

    // Restore previous IODIR state
    this.writeSize(this.IODIR, iodir);
  }
  read() {
    // Read IODIR state
    let iodir = this.readSize(this.IODIR);

    // Set IODIR state to INPUT
    this.writeSize(this.IODIR, 0xFFFF);

    // Read GPIO
    let gpio = this.readSize(this.GPIO);

    // Restore previous IODIR state
    this.writeSize(this.IODIR, iodir);

    return gpio;
  }
}

class Pin {
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
MCP23008.prototype.INPUTS = 0b11111111;
MCP23008.prototype.PULLUPS = 0b00000000;
MCP23008.prototype.IODIR = 0x00;
MCP23008.prototype.GPPU = 0x06;
MCP23008.prototype.GPIO = 0x09;
MCP23008.prototype.writeSize = SMBus.prototype.writeByte;
MCP23008.prototype.readSize = SMBus.prototype.readByte;
Object.freeze(MCP23008.prototype);

class MCP23017 extends Expander {}
MCP23017.prototype.length = 16;
MCP23017.prototype.INPUTS = 0b1111111111111111;
MCP23017.prototype.PULLUPS = 0b0000000000000000;
MCP23017.prototype.IODIR = 0x00;
MCP23017.prototype.GPPU = 0x0C;
MCP23017.prototype.GPIO = 0x12;
MCP23017.prototype.writeSize = SMBus.prototype.writeWord;
MCP23017.prototype.readSize = SMBus.prototype.readWord;
Object.freeze(MCP23017.prototype);


export { MCP23008, MCP23017 };

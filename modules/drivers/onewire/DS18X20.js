/*
 * Copyright (c) 2019  Moddable Tech, Inc.
 * Copyright (c) 2019  Wilberforce
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Timer from "timer";
import OneWire from "onewire";

// Family code
const DS18B20 = 0x28;
const DS18S20 = 0x10;

export class DS18X20 {

  constructor(dictionary) {
    if (dictionary.bus) {
      this.bus = dictionary.bus;
    } else {
      throw new Error("Onewire bus expected");
    }
    if (dictionary.index && dictionary.index >= 0 && dictionary.index <= 126) {
      this.rom = this.bus.search()[dictionary.index];
    }
    if (dictionary.id) {
      this.rom = dictionary.id;
    }
    // default to first device
    if (!this.rom) {
      this.rom = this.bus.search()[0];
    }
    if ( dictionary.digits) {
      this.digits=dictionary.digits;
    }
  }

  // return hex version of the ROM
  toString() {
    return OneWire.hex(this.rom);
  }

  get family() {
    return new DataView(this.rom).getInt8();
  }

  // internal use
  get scratchpad() {
    let b = this.bus;
    b.select(this.rom);
    b.write(0xBE); // Read scratchpad
    let buffer = b.read(9);
    return new Uint8Array(buffer);
  }

   // internal use
  set scratchpad(bytes) {
    let b = this.bus;
    b.reset();
    b.select(this.rom);
    b.write(0x4E); // Write scratchpad
    bytes.forEach(function (byte) {
      b.write(byte);
    });
    b.select(this.rom);
    b.write(0x48); // Copy to scratch pad
    b.reset();
  }

  /** Set the sensor resolution in bits 9-12.
      This setting is stored in device's EEPROM so it persists even after the sensor loses power. */
  set resolution(bits) {
    let spad = this.scratchpad;
    if (bits < 9 || bits > 12) {
      throw new Error("resolution 9-12 bits");
    }
    bits = [0x1F, 0x3F, 0x5F, 0x7F][bits - 9];
    this.scratchpad = [spad[2], spad[3], bits];
  }

  /** Return the resolution in bits 9-12 */
  get resolution() {
    return [0x1F, 0x3F, 0x5F, 0x7F].indexOf(this.scratchpad[4]) + 9;
  }

  /** Return true if this device is still connected to the OneWire Bus */
  get present() {
    return this.bus.search().indexOf(this.rom) !== -1;
  }

  get temp() {
    return this.getTemp();
  }

  // Read the conversion back, checking crc for valid result
  get read() {
    let s = this.scratchpad;
    let temp = null;

    if (OneWire.crc(s.buffer, 8) == s[8]) {
      temp = new DataView(s.buffer).getInt16(0, true);
      temp = temp / ((this.family == DS18S20) ? 2 : 16);
      if ( this.digits ) temp=temp.toFixed(this.digits );
    }
    return temp;
  }

  /** trigger device to start temperature conversion. 
   * If a callback is supplied, it is called with the temperature in C.
   * Otherwise the last temperature read is returned. The first value will be invalid.
   * If the CRC fails null is returned.
   */
  getTemp(callback) {
    this.bus.select(this.rom);
    this.bus.write(0x44, true); // Start conversion
    if (!callback) return this.read; // if no callback, read now - we'll get the last temperature 
    let max_conversion_ms = {
      8: 100, // Hack for simulator
      9: 94,
      10: 188,
      11: 375,
      12: 750
    };
    let wait = max_conversion_ms[this.resolution];
    Timer.set(id => {
      callback(this.read);
    }, wait);
  }

}

Object.freeze(DS18X20.prototype);

export default DS18X20;
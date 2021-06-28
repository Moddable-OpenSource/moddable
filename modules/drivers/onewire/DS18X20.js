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


const max_conversion_ms = {
  8: 100, // Hack for simulator
  9: 94,
  10: 188,
  11: 375,
  12: 750
};
Object.freeze(max_conversion_ms);

export class DS18X20 {

  constructor(dictionary) {
    if (!dictionary.bus) {
      throw new Error("OneWire bus required");
    }
    this.bus = dictionary.bus;

    if (dictionary.id) {
      this.id = dictionary.id;
    } else if ((undefined !== dictionary.index) && (dictionary.index >= 0) && (dictionary.index <= 126)) {
      this.id = this.bus.search()[dictionary.index];
    } else {
      this.id = this.bus.search()[0]; // default to first device
    }
    if (!this.id) {
      throw new Error("no device");
    }
  }

  // return hex version of the ROM
  toString() {
    return OneWire.hex(this.id);
  }

  get family() {
    return (new Int8Array(this.id))[0];
  }

  // internal use
  get scratchpad() {
    const bus = this.bus;
    bus.select(this.id);
    bus.write(0xBE); // Read scratchpad
    let buffer = bus.read(9);
    return new Uint8Array(buffer);
  }

  // internal use
  set scratchpad(bytes) {
    const bus = this.bus;
    bus.reset();
    bus.select(this.id);
    bus.write(0x4E); // Write scratchpad
    for (let i = 0, length = bytes.length; i < length; i++) {
      bus.write(bytes[i]);
    }
    bus.select(this.id);
    bus.write(0x48); // Copy to scratch pad
    bus.reset();
  }

  /** Set the sensor resolution in bits 9-12.
      This setting is stored in device's EEPROM so it persists even after the sensor loses power. */
  set resolution(bits) {
    if (bits < 9 || bits > 12) {
      throw new Error("resolution 9-12 bits");
    }
    bits = [0x1F, 0x3F, 0x5F, 0x7F][bits - 9];
    const scratchpad = this.scratchpad;
    this.scratchpad = [scratchpad[2], scratchpad[3], bits];
  }

  /** Return the resolution in bits 9-12 */
  get resolution() {
    return [0x1F, 0x3F, 0x5F, 0x7F].indexOf(this.scratchpad[4]) + 9;
  }

  /** Return true if this device is still connected to the OneWire Bus */
  get present() {
    return this.bus.isPresent(this.id);
  }

  get temperature() {
    return this.getTemperature();
  }

  // Read the conversion back, checking crc for valid result
  get _read() {
    const scratchpad = this.scratchpad;

    if (OneWire.crc(scratchpad.buffer, 8) !== scratchpad[8]) {
      return null;
    }
    let temp = scratchpad[0] | (scratchpad[1] << 8);
    temp = temp / ((this.family === DS18S20) ? 2 : 16);

    return temp;
  }

  /** trigger device to start temperature conversion. 
   * If a callback is supplied, it is called with the temperature in C.
   * Otherwise the last temperature read is returned. The first value will be invalid.
   * If the CRC fails null is returned.
   */
  getTemperature(callback) {
    this.bus.select(this.id);
    this.bus.write(0x44, true); // Start conversion
    if (!callback) return this._read; // if no callback, read now - we'll get the last temperature

    Timer.set(() => {
      callback(this._read);
    }, max_conversion_ms[this.resolution]); //@@jph this always retreives the resolution. necessary? Or can that be cached?
  }

}

Object.freeze(DS18X20.prototype);

export default DS18X20;
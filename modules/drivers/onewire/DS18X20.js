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

/*
  To do:
  Close and destructor -> What if close not called?
  slots seems to increase then decrease - ramp...
  constructor - dictionary?  pass number of decimal places for .toFixed?
 */

/*
connect(new OneWire(pin)) - use the first found DS18X20 device
connect(new OneWire(pin), n) - use the nth DS18X20 device
connect(new OneWire(pin), rom) - use the DS18X20 device with the given rom
*/
import Timer from "timer";

// Family code
const DS18B20=0x28;
const DS18S20=0x10;

export class DS18X20 {
  constructor(oneWire, device) {
    this.bus = oneWire;
    if (device === undefined) {
      this.rom = this.bus.search()[0];
    } else {
      if (parseInt(device).toString() == device && device >= 0 && device <= 126) {
        this.rom = this.bus.search()[device];
      } else {
        // Check is an ArrayBuffer ?
        this.rom = device;
      }
    }
    if (!this.rom) throw new Error("No DS18X20 found");
  }

  // return hex version of the ROM
  toString() {
    return BigInt.fromArrayBuffer(this.rom).toString(16);
  }

  get family() {
    return new DataView(this.rom).getInt8();
  }

  /** For internal use - read the scratchpad region */
  get scratchpad() {
    let b = this.bus;
    b.select(this.rom);
    b.write(0xBE); // Read scratchpd
    let buffer = b.read(9);
    //trace( BigInt.fromArrayBuffer(buffer).toString(16),'\n');
    return new Uint8Array(buffer);
  }

  /** For internal use - write to the scratchpad region */

  // Does not seem to be working? setting res bits to 9
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
    if (bits < 9) bits = 9;
    if (bits > 12) bits = 12;
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
    if (this.bus.crc(s.buffer, 8) == s[8]) {
      temp = new DataView(s.buffer).getInt16(0, true);
      temp = temp / ((this.family == DS18S20) ? 2 : 16);
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
    let wait=max_conversion_ms[this.resolution];
    Timer.set(id => {
      callback(this.read);
    }, wait);
  }

  /** Return a list of all DS18X20 sensors with their alarms set */
  // Untested
  searchAlarm() {
    return this.bus.search(0xEC);
  }

  /** Set alarm low and high values in whole degrees C
    If the temperature goes below lo or above hi the alarm will be set. */
  // Untested
  setAlarm(lo, hi) {
    lo--; // DS18X20 alarms if (temp<=lo || temp>hi), but we want (temp<lo || temp>hi)
    // change to signed byte
    if (lo < 0) lo += 256;
    if (hi < 0) hi += 256;
    let spad = this.scratchpad;
    this.scratchpad = [hi, lo, spad[4]];
  };

}

Object.freeze(DS18X20.prototype);

export default DS18X20;
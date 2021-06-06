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

// Dallas instruments onewire protocol
// DS18B20, DS18S20 - temperature sensors
// OneWire EEPROMs- AT24 AT25 TCS3472x DS2xxx  (DS24B33, DS2431, DS28EC20 etc)

class OneWire @ "xs_onewire_destructor" {
  constructor(dictionary) @ "xs_onewire";

  // Free resources
  close() @ "xs_onewire_close";

  // The byte that was read, or a Uint8Array if count was specified and >=0
  read(count) @ "xs_onewire_read";

  // data - A byte (or array of bytes) to write
  write(data) @ "xs_onewire_write";

  // rom - The device to select (get this using OneWire.search())
  select(device) @ "xs_onewire_select";

  // An array of devices that were found
  search() @ "xs_onewire_search";

  // Checks if id is present on bus
  isPresent(id) @ "xs_onewire_isPresent";

  // Reset the bus - returns True is a device was present
  reset() @ "xs_onewire_reset";

  // Calculate CRC
  static crc(buffer) @ "xs_onewire_crc";

  // hex string version of the rom
  static hex(buffer) {
    let result = "";
    (new Uint8Array(buffer)).forEach(i => result += i.toString(16).padStart(2, 0));
    return result;
  }

}

Object.freeze(OneWire.prototype);

export default OneWire;

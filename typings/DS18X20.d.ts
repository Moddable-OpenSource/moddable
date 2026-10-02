/*
 * Copyright (c) 2023  Moddable Tech, Inc.
 * Copyright (c) 2023  Thorsten von Eicken
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

declare module "DS18X20" {
  import type OneWire from "onewire";

  export default class DS18X20 {
    // the sensor is the one with the given id (its 8 byte ROM code), else the one at index in the bus's search results, else the first
    constructor(dictionary: {
      bus: OneWire;
      id?: ArrayBuffer;
      index?: number;
    });
    toString(): string;
    readonly family: number;
    get scratchpad() : Uint8Array;
    set scratchpad(bytes: Uint8Array);
    get resolution(): number;
    set resolution(bits: number);
    readonly present: boolean;
    // null when the CRC of the reading is wrong
    readonly temperature: number | null;
    // starts a conversion. with a callback, it is called with the temperature when the conversion completes.
    // with no callback, returns the previous reading.
    getTemperature(): number | null;
    getTemperature(callback: (temp: number | null) => void): void;
  }
}

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
  export default class DS18X20 {
    constructor(dictionary: Record<string, any>);
    toString(): string;
    readonly family: number;
    get scratchpad() : Uint8Array;
    set scratchpad(bytes: Uint8Array);
    get resolution(): number;
    set resolution(bits: number);
    readonly present: boolean;
    readonly temperature: number;
    getTemperature(callback: (temp: number) => void): void;
  }
}

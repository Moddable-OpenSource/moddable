/*
 * Copyright (c) 2023  Moddable Tech, Inc.
 * Copyright (c) 2023  Throsten von Eicken
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

declare module "onewire" {
  export default class OneWire {
    constructor(dictionary : Record<string,any>);
    close() : void;
    // the bus identifies devices by their 8 byte ROM code, in an ArrayBuffer
    read() : number;
    read(count : number) : ArrayBuffer;
    write(data : number) : void;
    select(device : ArrayBuffer) : void;
    search() : ArrayBuffer[];
    isPresent(id : ArrayBuffer) : boolean;
    reset() : boolean;
    static crc(buffer : ArrayBuffer, length? : number) : number;
    static hex(buffer : ArrayBuffer | Uint8Array) : string;
  }
}

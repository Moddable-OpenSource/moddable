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
    read(count : number) : number|Uint8Array;
    write(data : number|Uint8Array) : void;
    select(device : Uint8Array) : void;
    search() : Uint8Array[];
    isPresent(id : Uint8Array) : boolean;
    reset() : boolean;
    static crc(buffer : Uint8Array) : number;
    static hex(buffer : Uint8Array) : string;
    static fromHexString(str : string) : Uint8Array;
  }
}

/*
* Copyright (c) 2022-2025 Richard Lea, Satoshi Tanaka
*
*   This file is part of the Moddable SDK Tools.
*
*   The Moddable SDK Tools is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   The Moddable SDK Tools is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
*
*/

type ManufacturerSpec = {
  /**
   * The `identifier` property is a number corresponding to the
   * _Company Identifier Code_.
   */
  identifier: string | number,

  /**
   * The `data` property is an array of numbers corresponding
   * to additional manufacturer specific data.
   */
  data: number[],
};

declare module "btutils" {

  export class Bytes extends ArrayBuffer {
    constructor(bytes: string | ArrayBufferLike, littleEndian?: boolean)
    /**
     * The `equals` function returns `true` if the instance
     * ArrayBuffer data equals the data contained in `buffer`.
     * 
     * @param bytes 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#equalsbuffer
     */
    equals(bytes: Bytes): boolean;

    /**
     * The `toString` function returns a printable hex string of the Bytes contents. The string is formatted in big endian order with separators.
     * https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#tostring
     */
    toString(): string;
  }

  /**
   * Provides Accessor functions to read common advertisement and
   * scan response data types as JavaScript properties.
   * 
   * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#class-advertisement
   */
  export interface Advertisement {

    /**
     * Contains the raw advertisement data bytes.
     */
    buffer: ArrayBuffer;

    /**
     * Number corresponding to the Appearance.
     */
    appearance: number;

    /**
     * The advertised complete local name.
     */
    completeName: string;

    /**
     * The advertised shortened local name.
     */
    shortName: string;

    /**
     * An object containing the advertised manufacturer specific data.
     */
    manufacturerSpecific: ManufacturerSpec;

    /**
     * The advertised flags value.
     */
    flags: number;

    /**
     * The advertised complete 16-bit UUID list.
     */
    completeUUID16List: Array<string | number>;

    /**
     * The advertised incomplete 16-bit UUID list.
     */
    incompleteUUID16List: Array<string | number>;

    findIndex(type: number, index?: number): number;
  }

  /**
   * Convert a Bluetooth UUID expressed as a hex string to a `Bytes` instance.
   * @param strings 
   */
  export function uuid(strings: TemplateStringsArray): Bytes;

  /**
   * Convert a Bluetooth address expressed as a hex string to a `Bytes` instance.
   * @param strings 
   */
  export function address(strings: TemplateStringsArray): Bytes;
}

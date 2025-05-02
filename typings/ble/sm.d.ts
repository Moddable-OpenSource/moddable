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

declare module "sm" {

  import { GAP } from "gap";

  type getAddressType = (typeof GAP.AddressType);

  /**
   * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#class-sm
   */
  export const enum IOCapability {
    NoInputNoOutput = 0,
    DisplayOnly = 1,
    KeyboardOnly = 2,
    KeyboardDisplay = 3,
    DisplayYesNo = 4,
  }

  export const enum Authorization {
    None = 0,
    NoMITM = 1,
    MITM = 2,
    SignedNoMITM = 3,
    SignedMITM = 4
  }

  /**
   * Provides objects used to configure BLE client and server security requirements
   * and device capabilities.
   */
  export class SM {


    /**
     * Delete all bonding information and encryption keys
     * from persistent storage.
     * 
     * https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#deleteallbondings
     */
    static deleteAllBondings(): void;

    /**
     * Delete stored bonding information for the peer device with the provided
     * `address` and `addressType`.
     * 
     * @param address Contains peer device Bluetooth address
     * @param addressType Peer device Bluetooth address type.
     */
    static deleteBonding(address: ArrayBuffer, addressType: getAddressType): void;
  }
}

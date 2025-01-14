/*
* Copyright (c) 2022 Richard Lea
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

declare module "bleclient" {

  import { GAP } from "gap";
  import { Client, Characteristic } from "gatt";
  import { IOCapability } from "sm";

  type gapScanFilterPolicies =
    typeof GAP.ScanFilterPolicy.NONE |
    typeof GAP.ScanFilterPolicy.NOT_RESOLVED_DIRECTED |
    typeof GAP.ScanFilterPolicy.WHITELIST |
    typeof GAP.ScanFilterPolicy.WHITELIST_NOT_RESOLVED_DIRECTED;

  /**
   * The `BLEClient` class provides access to the BLE client features.
   */
  export class BLEClient {

    securityParameters: {
      encryption?: boolean,
      bonding?: boolean,
      mitm?: boolean,
      ioCapability?: IOCapability,
    };

    /**
     * Applications must wait for the `onReady` callback before calling
     * other `BLEClient` functions
     */
    onReady(): void

    /**
     * The `startScanning` function enables scanning for nearby peripherals.
     * 
     * @param params If params is not provided, the default scan properties are used.
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#startscanningparams
     */
    startScanning(params?: {
      active: boolean,
      duplicates: boolean,
      filterPolicy: gapScanFilterPolicies,
      interval: number,
      window: number,
    }): void;

    /**
     * Called when one or more times for each peripheral device discovered.
     * @param device 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#ondiscovereddevice
     */
    onDiscovered(device: Client): void;

    /**
     * Disables scanning for nearby peripherals.
     */
    stopScanning(): void;

    /**
     * Initiates a connection request between the `BLEClient` and a target peripheral `device`.
     * @param device 
     */
    connect(device: Client): void;

    onConnected(device: Client): void;

    onDisconnected(): void;

    onCharacteristicNotification(characteristic: Characteristic, buffer: number[]): void;
  }

  export { BLEClient as default };
}

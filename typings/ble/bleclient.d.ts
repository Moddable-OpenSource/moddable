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

declare module "bleclient" {

  import { GAP } from "gap";
  import { Client, Service, Characteristic, Descriptor } from "gatt";
  import { IOCapability } from "sm";

  type gapScanFilterPolicies =
    typeof GAP.ScanFilterPolicy.NONE |
    typeof GAP.ScanFilterPolicy.NOT_RESOLVED_DIRECTED |
    typeof GAP.ScanFilterPolicy.WHITELIST |
    typeof GAP.ScanFilterPolicy.WHITELIST_NOT_RESOLVED_DIRECTED;

  type securityParameters = {
    encryption?: boolean,
    bonding?: boolean,
    mitm?: boolean,
    ioCapability?: IOCapability,
  }

  /**
   * The `BLEClient` class provides access to the BLE client features.
   */
  export class BLEClient {

    securityParameters: securityParameters

    /**
     * Applications must wait for the `onReady` callback before calling
     * other `BLEClient` functions
     */
    onReady(): void

    onSecurityParameters(params: securityParameters): void

  
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

    /**
     * Called when the client connects to a target peripheral `device`.
     * @param device 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#onconnecteddevice
     */
    onConnected(device: Client): void;

    /**
     * Called when the peripheral signal strength is read.
     * @param device 
     * @param rssi 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#onrssidevice-rssi
     */
    onRSSI(device: Client, rssi: number): void;

    /**
     * Called when the MTU exchange procedure has been completed.
     * @param device 
     * @param mtu
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#onmtuexchangeddevice-mtu
     */
    onMTUExchanged(device: Client, mtu: number): void;

    /**
     * Called when the client connection is closed.
     * @param device 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#ondisconnecteddevice
     */
    onDisconnected(device: Client): void;

    /**
     * Called when service discovery completes.
     * @param services 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#onservicesservices
     */
    onServices(services: Service[]): void;

    /**
     * Called when characteristic discovery completes. 
     * @param characteristics 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#oncharacteristicscharacteristics
     */
    onCharacteristics(characteristics: Characteristic[]): void;

    /**
     * Called when a characteristic is read by the `readValue` function.
     * @param characteristic 
     * @param value 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#oncharacteristicvaluecharacteristic-value
     */
    onCharacteristicValue(characteristic: Characteristic, value: any): void;

    /**
     * Called when notifications are enabled and the peripheral notifies the characteristic value.
     * @param characteristic 
     * @param buffer 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#oncharacteristicnotificationcharacteristic-value
     */
    onCharacteristicNotification(characteristic: Characteristic, buffer: number[]): void;

    /**
     * Called when notifications have been enabled for the `characteristic`.
     * @param characteristic 
     * @param buffer 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#oncharacteristicnotificationenabledcharacteristic
     */
    onCharacteristicNotificationEnabled(characteristic: Characteristic): void;

    /**
     * Called when notifications have been disabled for the `characteristic`.
     * @param characteristic 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#oncharacteristicnotificationdisabledcharacteristic
     */
    onCharacteristicNotificationDisabled(characteristic: Characteristic): void;

    /**
     * Called when a descriptor is read by the `readValue` function.
     * @param descriptor 
     * @param value 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#ondescriptorvaluedescriptor-value
     */
    onDescriptorValue(descriptor: Descriptor, value: any): void;

    close(): void
  }

  export { BLEClient as default };
}

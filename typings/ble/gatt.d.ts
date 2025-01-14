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

declare module "gatt" {

  import { Bytes, Advertisement } from "btutils";
  import { GAP } from "gap";

  type getAddressType = (typeof GAP.AddressType);

  export interface Client {
    connection: number,
    address: Bytes,
    addressType: getAddressType,
    scanResponse: Advertisement,
    rssi: number,

    /**
     * Read the connected peripheral's signal strength.
     * 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#readrssi
     */
    readRSSI(): void;

    /**
     * Discover a single GATT primary service by UUID.
     *
     * @param uuid 
     */
    discoverPrimaryService(uuid: Bytes): void;

    /**
     * Discover all the peripheral's GATT primary services.
     * Discovered services are returned to the `onServices` callback.
     */
    discoverAllPrimaryServices(): void;

    /**
     * Finds and returns the service identified by `uuid`.
     * 
     * @param uuid 
     */
    findServiceByUUID(uuid: Bytes): void;

    /**
     * Called when service discovery completes.
     * If `findServiceByUUID` was called to find a single service,
     * the `services` array contains the single service found.
     * 
     * @param services 
     */
    onServices(services: Service[]): void;

    /**
     * Terminates the peripheral function.
     */
    close(): void;
  }

  /**
   * A single Peripheral GATT service.
   */
  export interface Service {
    connection: number;
    uuid: Bytes;
    start: number;
    end: number;
    characteristics: Characteristic[];

    /**
     * Discover all the service characteristics. Discovered characteristics
     * are returned to the `onCharacteristics` callback.
     */
    discoverAllCharacteristics(): void;

    /**
     * Discover a single service characteristic by UUID.
     * 
     * @param uuid 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#discovercharacteristicuuid
     */
    discoverCharacteristic(uuid: Bytes): void;

    /**
     * Called when characteristic discovery completes.
     * If `findCharacteristicByUUID` was called to find a single characteristic,
     * the `characteristics` array contains the single characteristic found.
     *
     * @param characteristics 
     */
    onCharacteristics(characteristics: Characteristic[]): void;
  }

  /**
   * A single service Characteristic.
   * 
   * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#class-characteristic
   */
  export interface Characteristic {
    connection: number;
    uuid: Bytes;
    service: Service;
    handle: number;
    name: string;
    type: string;
    descriptors: Descriptor[];

    /**
     * Discover all the characteristic's descriptors.
     * Discovered descriptors are returned to the `onDescriptors` callback.
     */
    discoverAllDescriptors(): void;

    /**
     * Called when descriptor discovery completes.
     *
     * @param descriptors 
     */
    onDescriptors(descriptors: Descriptor[]): void;

    /**
     * Enable characteristic value change notifications.
     *
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#enablenotifications
     */
    disableNotifications(): void;
  }

  /**
   * A single Characteristic descriptor.
   * 
   * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#class-descriptor
   */
  export interface Descriptor {
    connection: number;
    uuid: string;
    characteristic: Characteristic;
    handle: number;
    name: string;
    type: string;

    /**
     * Read a descriptor value on demand.
     *
     * @param auth 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#readvalueauth-1
     */
    readValue(auth?: number): void;
  }
}

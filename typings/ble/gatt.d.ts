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

declare module "gatt" {

  import { Bytes, Advertisement } from "btutils";
  import { GAP } from "gap";
  import { Authorization } from "sm";

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
     * Request a higher MTU once the peripheral connection has been established.
     * @param mtu 
     */
    exchangeMTU(mtu: number): void

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
     * finds and returns the characteristic identified by `uuid`. 
     * @param uuid 
     */
    findCharacteristicByUUID(uuid: Bytes): void;
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
    properties: number;

    /**
     * Discover all the characteristic's descriptors.
     * Discovered descriptors are returned to the `onDescriptors` callback.
     */
    discoverAllDescriptors(): void;

    
    /* Enable characteristic value change notifications.
     *
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#enablenotifications
     */
    enableNotifications(): void;

    /**
     * Enable characteristic value change notifications.
     *
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#enablenotifications
     */
    disableNotifications(): void;

    /**
     * Disable characteristic value change notifications.
     * @param auth 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#disablenotifications
     */
    readValue(auth: Authorization): void;

    /**
     * Write a characteristic value on demand.
     * @param value 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#writewithoutresponsevalue
     */
    writeWithoutResponse(value: any): void;
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

    writeWithoutResponse(value: any): void;

    /**
     * Write a descriptor value.
     * @param value 
     * @url https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/network/ble/ble.md#writevaluevalue
     */
    writeValue(value: any): void;
  }
}

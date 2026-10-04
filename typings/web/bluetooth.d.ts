/*
* Copyright (c) 2026 Moddable Tech, Inc.
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

// Web Bluetooth (https://webbluetoothcg.github.io/web-bluetooth/): the subset implemented by modules/io/ble/web-bluetooth
declare module "web-bluetooth" {
  // a 16- or 32-bit alias, a full 128-bit UUID string, or a name from the GATT assigned numbers (or config.webbluetooth)
  export type BluetoothServiceUUID = number | string;
  export type BluetoothCharacteristicUUID = number | string;
  export type BluetoothDescriptorUUID = number | string;

  export class BluetoothUUID {
    static canonicalUUID(alias: number): string;
    static getService(name: BluetoothServiceUUID): string;
    static getCharacteristic(name: BluetoothCharacteristicUUID): string;
    static getDescriptor(name: BluetoothDescriptorUUID): string;
  }

  export interface BluetoothLEScanFilter {
    name?: string;
    services?: BluetoothServiceUUID[];
  }

  export interface RequestDeviceOptions {
    filters?: BluetoothLEScanFilter[];
  }

  export const bluetooth: {
    requestDevice(options?: RequestDeviceOptions): Promise<BluetoothDevice>;
  };

  export class BluetoothDevice {
    constructor(address: string, name?: string);		// Moddable SDK extension: a device discovered by other means, such as GAPClient
    readonly id: string;
    readonly name: string;
    readonly gatt: BluetoothRemoteGATTServer;
  }

  export interface BluetoothRemoteGATTServer {
    readonly device: BluetoothDevice;
    readonly connected: boolean;
    connect(): Promise<BluetoothRemoteGATTServer>;
    disconnect(): void;
    getPrimaryService(service: BluetoothServiceUUID): Promise<BluetoothRemoteGATTService>;
    getPrimaryServices(services?: BluetoothServiceUUID[]): Promise<BluetoothRemoteGATTService[]>;
  }

  export interface BluetoothRemoteGATTService {
    readonly device: BluetoothDevice;
    readonly uuid: string;
    readonly isPrimary: boolean;
    getCharacteristic(characteristic: BluetoothCharacteristicUUID): Promise<BluetoothRemoteGATTCharacteristic>;
    getCharacteristics(characteristics?: BluetoothCharacteristicUUID[]): Promise<BluetoothRemoteGATTCharacteristic[]>;
  }

  export interface BluetoothCharacteristicProperties {
    readonly authenticatedSignedWrites: boolean;
    readonly broadcast: boolean;
    readonly indicate: boolean;
    readonly notify: boolean;
    readonly read: boolean;
    readonly reliableWrite: boolean;
    readonly write: boolean;
    readonly writeWithoutResponse: boolean;
  }

  // the characteristic's value is always present in its characteristicvaluechanged event
  export interface BluetoothCharacteristicValueChangedEvent {
    readonly target: BluetoothRemoteGATTCharacteristic & { readonly value: DataView };
  }

  export interface BluetoothRemoteGATTCharacteristic {
    readonly service: BluetoothRemoteGATTService;
    readonly uuid: string;
    readonly properties: BluetoothCharacteristicProperties;
    readonly value: DataView | undefined;		// the most recently read or notified value
    getDescriptor(descriptor: BluetoothDescriptorUUID): Promise<BluetoothRemoteGATTDescriptor>;
    getDescriptors(descriptors?: BluetoothDescriptorUUID[]): Promise<BluetoothRemoteGATTDescriptor[]>;
    readValue(): Promise<DataView>;
    writeValue(value: ByteBuffer): Promise<void>;		// without response
    writeValueWithResponse(value: ByteBuffer): Promise<void>;
    writeValueWithoutResponse(value: ByteBuffer): Promise<void>;
    startNotifications(): Promise<BluetoothRemoteGATTCharacteristic>;
    stopNotifications(): Promise<BluetoothRemoteGATTCharacteristic>;
    addEventListener(type: "characteristicvaluechanged", listener: (event: BluetoothCharacteristicValueChangedEvent) => void): void;
    removeEventListener(type: "characteristicvaluechanged", listener: (event: BluetoothCharacteristicValueChangedEvent) => void): void;
  }

  export interface BluetoothRemoteGATTDescriptor {
    readonly characteristic: BluetoothRemoteGATTCharacteristic;
    readonly uuid: string;
    readonly value: DataView | undefined;
    readValue(): Promise<DataView>;
    writeValue(value: ByteBuffer): Promise<void>;
  }
}

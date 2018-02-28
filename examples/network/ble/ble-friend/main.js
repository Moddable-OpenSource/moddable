/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
 /*
 	Adafruit Bluefruit LE Friend UART service
 	https://learn.adafruit.com/introducing-adafruit-ble-bluetooth-low-energy-friend/uart-service
 */

import BLE from "ble";
import {Descriptor} from "gatt";

const DEVICE_NAME = "UART";
const UART_SERVICE_UUID = '6E400001-B5A3-F393-E0A9-E50E24DCCA9E';
const UART_CHARACTERISTIC_RX_UUID = '6E400003-B5A3-F393-E0A9-E50E24DCCA9E';

let ble = new BLE();
ble.onReady = () => {
	ble.onDiscovered = device => {
		if (DEVICE_NAME == device.scanResponse.completeName) {
			ble.stopScanning();
			ble.connect(device.address);
		}
	}
	ble.onConnected = connection => {
		connection.onDisconnected = () => {
			ble.startScanning();
		}
		let client = connection.client;
		client.onServices = services => {
			let service = client.findServiceByUUID(UART_SERVICE_UUID);
			if (service) {
				service.onCharacteristics = characteristics => {
					let characteristic = service.findCharacteristicByUUID(UART_CHARACTERISTIC_RX_UUID);
					if (characteristic) {
						characteristic.onDescriptors = descriptors => {
							let descriptor = characteristic.findDescriptorByUUID(Descriptor.CCCD_UUID);
							if (descriptor) {
								trace("bingo!!\n");
							}
						}
						characteristic.discoverAllDescriptors();
					}
				}
				service.discoverAllCharacteristics();
			}
		}
		client.discoverAllPrimaryServices();
	}
	ble.startScanning();
}
	
ble.initialize();


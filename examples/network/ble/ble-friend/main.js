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
	https://learn.adafruit.com/introducing-adafruit-ble-bluetooth-low-energy-friend/terminal-settings
 
 	To use this application, first set the BLEFriend for UART mode and connect with a serial terminal @ 9600,8,N,1.
 	Strings written to the RX characteristic display in the terminal, and strings sent from
 	the terminal arrive as TX notifications and are traced to the console.
 */

import BLE from "ble";
import {UUID} from "btutils";
import Timer from "timer";

const DEVICE_NAME = "UART";
const SERVICE_UUID = '6E400001-B5A3-F393-E0A9-E50E24DCCA9E';
const CHARACTERISTIC_RX_UUID = '6E400002-B5A3-F393-E0A9-E50E24DCCA9E';
const CHARACTERISTIC_TX_UUID = '6E400003-B5A3-F393-E0A9-E50E24DCCA9E';

let count = 1;

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
			let service = client.findServiceByUUID(SERVICE_UUID);
			if (service) {
				service.onCharacteristics = characteristics => {
					let rx_characteristic = service.findCharacteristicByUUID(CHARACTERISTIC_RX_UUID);
					let tx_characteristic = service.findCharacteristicByUUID(CHARACTERISTIC_TX_UUID);
					if (tx_characteristic && rx_characteristic) {
						tx_characteristic.onNotification = buffer => {
							trace(String.fromArrayBuffer(buffer));
						}
						tx_characteristic.onDescriptors = descriptors => {
							let descriptor = tx_characteristic.findDescriptorByUUID(UUID.CCCD);
							if (descriptor) {
								descriptor.writeValue(1);	// enable notifications
								ble.timer = Timer.repeat(id => {
									rx_characteristic.writeWithoutResponse(`Hello UART ${count}\n`);
									++count;
								}, 1000);
							}
						}
						tx_characteristic.discoverDescriptor(UUID.CCCD);
					}
				}
				service.discoverAllCharacteristics();
			}
		}
		client.discoverPrimaryService(SERVICE_UUID);
	}
	ble.startScanning();
}
	
ble.initialize();


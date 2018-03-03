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
 	BLE Colorific Light Bulb
	https://www.walmart.com/ip/Star-Tech-Bc090-Colorific-A19-Bluetooth-D67-Controlled-LED-Bulb/46711393
 */

import BLE from "ble";
import Timer from "timer";

const DEVICE_NAME = "RGBLightOne";
const SERVICE_UUID = '1802';
const CHARACTERISTIC_UUID = '2A06';

let buffer = new ArrayBuffer(9);
let payload = new Uint8Array(buffer);
payload[0] = 0x58;
payload[1] = 0x01;
payload[2] = 0x03;
payload[3] = 0x01;
payload[4] = 0x10; // White brightness
payload[5] = 0x00; // Separator byte

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
					let characteristic = service.findCharacteristicByUUID(CHARACTERISTIC_UUID);
					if (characteristic) {
						ble.timer = Timer.repeat(id => {
							payload[6] = Math.floor(Math.random() * 256);
							payload[7] = Math.floor(Math.random() * 256);
							payload[8] = Math.floor(Math.random() * 256);
							characteristic.writeWithoutResponse(buffer);
						}, 200);
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


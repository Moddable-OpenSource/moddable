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
	This application discovers the device information service (0x180A), traces the
	manufacturer name read from the contained characteristic (0x2A29) and disconnects.
	To use the app, set the DEVICE_NAME string to your device's advertised complete name.
*/

import BLE from "ble";

const DEVICE_NAME = "<YOUR DEVICE NAME>";

let ble = new BLE();
ble.onReady = () => {
	ble.startScanning();
	ble.onDiscovered = device => {
		if (DEVICE_NAME == device.scanResponse.completeName) {
			ble.stopScanning();
			ble.connect(device.address);
		}
	}
	ble.onConnected = connection => {
		let client = connection.client;
		client.onServices = services => {
			let service = services.find(service => '180A' == service.uuid);
			if (service) {
				service.onCharacteristics = characteristics => {
					let characteristic = service.findCharacteristicByUUID('2A29');
					characteristic.onValue = value => {
						let manufacturer = String.fromArrayBuffer(value);
						trace(`Manufacturer: ${manufacturer}\n`);
						connection.disconnect();
					}
					characteristic.readValue();
				}
				service.discoverAllCharacteristics();
			}
		}
		client.discoverAllPrimaryServices();
	}
}
	
ble.initialize();


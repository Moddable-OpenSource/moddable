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
 	Griffin PowerMate Multimedia Control Knob
	https://griffintechnology.com/us/powermate-bluetooth
 */

import BLE from "ble";
import {UUID} from "btutils";

const DEVICE_NAME = 'PowerMate Bluetooth';
const SERVICE_UUID = '25598CF7-4240-40A6-9910-080F19F91EBC';
const CHARACTERISTIC_UUID = '9CF53570-DDD9-47F3-BA63-09ACEFC60415';

let ble = new BLE();
ble.onReady = () => {
	ble.onDiscovered = device => {
		if ("incompleteUUID128List" in device.scanResponse) {
			if (device.scanResponse.incompleteUUID128List.find(uuid => SERVICE_UUID == uuid)) {
				ble.stopScanning();
				ble.connect(device.address);
			}
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
						characteristic.onNotification = buffer => {
							let state, value = new Uint8Array(buffer)[0];
							if (104 == value)
								state = "right";
							else if (103 == value)
								state = "left";
							else if (101 == value)
								state = "press";
							else if (value >= 114)
								state = "hold";
							else
								state = "idle";
							trace(`state: ${state}\n`);
						}
						characteristic.onDescriptors = descriptors => {
							let descriptor = characteristic.findDescriptorByUUID(UUID.CCCD);
							if (descriptor)
								descriptor.writeValue(1);	// enable notifications
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


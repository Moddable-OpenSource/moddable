/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

import BLEClient from "bleclient";
import {uuid} from "btutils";

const SERVICE_UUID = uuid`25598CF7-4240-40A6-9910-080F19F91EBC`;
const CHARACTERISTIC_UUID = uuid`9CF53570-DDD9-47F3-BA63-09ACEFC60415`;

class PowerMate extends BLEClient {
	onReady() {
		this.onDisconnected();
	}
	onDiscovered(device) {
		if ('PowerMate Bluetooth' == device.scanResponse.completeName) {
			this.stopScanning();
			this.connect(device);
		}
	}
	onConnected(device) {
		device.discoverPrimaryService(SERVICE_UUID);
	}
	onServices(services) {
		if (services.length)
			services[0].discoverCharacteristic(CHARACTERISTIC_UUID);
	}
	onCharacteristics(characteristics) {
		if (characteristics.length)
			characteristics[0].enableNotifications();
	}
	onCharacteristicNotification(characteristic, value) {
		let state;
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
		trace(`${characteristic.name} state: ${state}\n`);
	}
	onDisconnected() {
		this.startScanning();
	}
}

let powermate = new PowerMate;

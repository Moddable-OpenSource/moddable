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

import BLEClient from "bleclient";
import {uuid} from "btutils";

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
		device.discoverPrimaryService(uuid`25598CF7-4240-40A6-9910-080F19F91EBC`);
	}
	onServices(services) {
		if (services.length)
			services[0].discoverCharacteristic(uuid`9CF53570-DDD9-47F3-BA63-09ACEFC60415`);
	}
	onCharacteristics(characteristics) {
		if (characteristics.length)
			characteristics[0].enableNotifications();
	}
	onCharacteristicNotification(characteristic, buffer) {
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
	onDisconnected() {
		this.startScanning();
	}
}

let powermate = new PowerMate;

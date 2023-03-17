/*
 * Copyright (c) 2023 Moddable Tech, Inc.
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

import BLEServer from "bleserver";
import {uuid} from "btutils";
import Timer from "timer";

const SERVICE_CHANGE_UUID = uuid`1801`;

class NameChangeServer extends BLEServer {
	onReady() {
		this.deviceName = "Test 1";
		this.onDisconnected();
	}
	onConnected() {
		this.stopAdvertising();
	}
	onDisconnected() {
		this.stop();
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: this.deviceName, completeUUID16List: [SERVICE_CHANGE_UUID]}
		});
	}
	onCharacteristicRead(characteristic) {
		if (characteristic.name === "device_name")
			return this.deviceName;
		
		if (characteristic.name === "service_changed")
			return new Uint8Array([0x00, 0x00, 0xFF, 0xFF]);
	}
	onCharacteristicNotifyEnabled(characteristic) {
		if (characteristic.name === "service_changed")
			this.start(characteristic);

	}
	onCharacteristicNotifyDisabled(characteristic) {
		if (characteristic.name === "service_changed")
			this.stop();
	}
	start(characteristic) {
		this.characteristicTimer = Timer.repeat(id => {
			const name = `Test ${Math.floor(Math.random() * 1000 + 1)}`;
			trace(`Device Name: ${name}\n`);
			this.deviceName = name;
			this.notifyValue(characteristic, new Uint8Array([0x00, 0x00, 0xFF, 0xFF]));
		}, 10_000);
	}
	stop() {
		Timer.clear(this.characteristicTimer);
		delete this.characteristicTimer;
	}
}

let ncs = new NameChangeServer;

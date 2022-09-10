/*
 * Copyright (c) 2016-2022 Moddable Tech, Inc.
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

const HEART_RATE_SERVICE_UUID = uuid`180D`;
const UART_SERVICE_UUID = uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`;

class TestServer extends BLEServer {
	onReady() {
		this.deviceName = "Moddable Server";
		this.onDisconnected();
	}
	onConnected() {
		this.stopAdvertising();
	}
	onDisconnected() {
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: this.deviceName, incompleteUUID16List: [HEART_RATE_SERVICE_UUID], manufacturerSpecific: { identifier: 0x03, data: [1,2,3] }}
		});
	}
	onCharacteristicNotifyEnabled(characteristic) {
		if ('tx' === characteristic.name) 
			this.tx = characteristic;
		if ('bpm' === characteristic.name)
			this.startMeasurements(characteristic);
	}
	onCharacteristicNotifyDisabled(characteristic) {
		if ('tx' === characteristic.name)
			delete this.tx;
		if ('bpm' === characteristic.name)
			this.stopMeasurements();
	}
	startMeasurements(characteristic) {
		this.bpm = [0, 0];
		this.timer = Timer.repeat(id => {
			this.notifyValue(characteristic, this.bpm);
			this.bpm[1] += 1;
		}, 1000);
	}
	stopMeasurements() {
		if (this.timer) {
			Timer.clear(this.timer);
			delete this.timer;
		}
		delete this.bpmChar;
		this.bpm = [0, 0];
	}
	onCharacteristicWritten(characteristic, value) {
		if ('rx' === characteristic.name && this.tx)
			this.notifyValue(this.tx, value);
	}
}

let test = new TestServer;
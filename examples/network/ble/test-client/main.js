/*
 * Copyright (c) 2022 Moddable Tech, Inc.
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


import BLEClient from "bleclient";
import {uuid} from "btutils";
import Tests from "tests";

const TESTMC_SERVICE_UUID = uuid`b1bc973f-14be-41e4-8e44-d261dce243fb`;
const RESULT_CHAR_UUID = uuid`b901519a-f888-491a-af84-50868305caf3`;

class TestClient extends BLEClient {
	onReady() {
		this.onDisconnected();
	}
	onDisconnected() {
		trace('disconnected\n');
		delete this.dispatch;
		delete this.resultChar;
		this.startScanning();
	}
	onDiscovered(device) {
		const completeName = device.scanResponse?.completeName;
		
		if (completeName === "testmc Server" && this.dispatch === undefined) {
			this.stopScanning();
			this.scanResponse = device.scanResponse;
			const data = device.scanResponse.manufacturerSpecific?.data;
			if (!(data instanceof Uint8Array) || data.length < 1) {
				trace(`Invalid manufacturer-specific data in test\n`);
				return;
			}

			trace(`Setting remote test to ${data[0]}\n`);

			this.dispatch = new Tests[data[0]](this.complete.bind(this));
			this.device = device;

			this.connect(device);
		}
	}
	onConnected(device) {
		trace(`connected\n`);
		this.device = device;
		
		device.discoverPrimaryService(TESTMC_SERVICE_UUID);
	}
	onServices(services) {
		trace(`services\n`);
		if (this.resultChar === undefined && services.length && services[0].uuid.equals(TESTMC_SERVICE_UUID)) {
			services[0].discoverCharacteristic(RESULT_CHAR_UUID);
		} else {
			this.dispatch.onServices(services);
		}
	}
	onCharacteristics(characteristics) {
		if (this.resultChar === undefined && characteristics.length && characteristics[0].uuid.equals(RESULT_CHAR_UUID)) {
			this.resultChar = characteristics[0];
			trace(`starting test\n`);
			this.dispatch.startTest(this.device, this.scanResponse);
		} else {
			this.dispatch.onCharacteristics(characteristics);
		}
	}
	onCharacteristicValue(characteristic, value) {
		this.dispatch.onCharacteristicValue(characteristic, value);
	}
	onCharacteristicNotification(characteristic, value) {
		this.dispatch.onCharacteristicNotification(characteristic, value);
	}
    onDescriptors(descriptors) {
		this.dispatch.onDescriptors(descriptors);
	}
    onDescriptorValue(descriptor, value) {
		this.dispatch.onDescriptorValue(descriptor, value);
	}
	complete(result = "okay") {
		trace(`complete\n`);
		this.resultChar.writeWithoutResponse(result);
	}
}

let test = new TestClient;
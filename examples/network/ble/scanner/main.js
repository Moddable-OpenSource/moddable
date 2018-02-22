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

import BLE from "ble";

const DEVICE_NAME = "UART";

let ble = new BLE();
ble.onReady = () => {
	ble.startScanning();
	
	ble.onDiscovered = device => {
		let scanResponse = device.scanResponse;
		if ("completeName" in scanResponse && DEVICE_NAME == scanResponse.completeName) {
			ble.stopScanning();
			ble.connect(device.address);
		}
	}
	ble.onConnected = connection => {
		trace("connected!\n");
		let client = connection.client;
		client.onServices = services => {
			debugger
		}
		client.discoverAllPrimaryServices();
	}
}
	
ble.initialize();


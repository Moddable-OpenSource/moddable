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
import Timer from "timer";

const DEVICE_NAME = "UART";

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
			Timer.clear(ble.timer);
			ble.startScanning();
		}
		connection.onRSSI = rssi => {
			trace(`RSSI: ${rssi}\n`);
		}
		ble.timer = Timer.repeat(() => {
			connection.readRSSI();
		}, 500);
	}
	ble.startScanning();
}
	
ble.initialize();


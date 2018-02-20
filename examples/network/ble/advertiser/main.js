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

let advertisingData = {
	shortName: "Brian",
};

let scanResponseData = {
	completeName: "Brian's device",
	publicAddress: "01:02:03:04:05:06",
	incompleteUUID16List: ['FF00', 'FF01'],
	incompleteUUID128List: ['00000000-0000-1000-8000-00805F9B34FB']
};

let ble = new BLE();
ble.onReady = () => {
	ble.deviceName = "Moddable ESP32";
	ble.startAdvertising({ advertisingData, scanResponseData });
	
	ble.onConnected = connection => {
		ble.stopAdvertising();
	}
}
	
ble.initialize();


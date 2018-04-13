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

import {BLEServer} from "ble";
import WiFi from "wifi";
import Net from "net";

const WIFI_NETWORK_CHARACTERISTIC = "FF01";
const WIFI_PASSWORD_CHARACTERISTIC = "FF02";
const WIFI_CONTROL_CHARACTERISTIC = "FF03";
const DEVICE_NAME = "Moddable Device";

let advertisingData = {
	shortName: "Moddable",
	completeUUID16List: ["FF00"]
};
let scanResponseData = {
	flags: 6,
	completeName: DEVICE_NAME
};
let ssid, password;

function connectToWiFiNetwork() {
	trace(`Conneting to ${ssid}...\n`);
	let monitor = new WiFi({ ssid, password }, msg => {
		switch (msg) {
			case "connect":
				break; // still waiting for IP address
			case "gotIP":
				trace(`IP address ${Net.get("IP")}\n`);
				break;
			case "disconnect":
				break;  // connection lost
		}
	})
}

let server = new BLEServer();
server.onReady = () => {
	server.deviceName = DEVICE_NAME;
	server.startAdvertising({ advertisingData, scanResponseData });
	server.deploy();
}
server.onConnected = () => {
	server.stopAdvertising();
}
server.onDisconnected = () => {
	server.startAdvertising({ advertisingData, scanResponseData });
}
server.onCharacteristicWritten = params => {
	let characteristic = params.characteristic;
	let value = params.value;
	if (WIFI_NETWORK_CHARACTERISTIC == characteristic.uuid)
		ssid = String.fromArrayBuffer(value);
	else if (WIFI_PASSWORD_CHARACTERISTIC == characteristic.uuid)
		password = String.fromArrayBuffer(value);
	else if (WIFI_CONTROL_CHARACTERISTIC == characteristic.uuid) {
		let command = new Uint8Array(value)[0];
		if ((1 == command) && ssid && password) {
			server.close();
			connectToWiFiNetwork();
		}
	}
}
server.initialize();


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

import BLEServer from "bleserver";
import WiFi from "wifi";
import Net from "net";

const WIFI_NETWORK_CHARACTERISTIC = "FF01";
const WIFI_PASSWORD_CHARACTERISTIC = "FF02";
const WIFI_CONTROL_CHARACTERISTIC = "FF03";
const DEVICE_NAME = "Moddable Device";

export default class WiFiServer extends BLEServer {
	onReady() {
		this.deviceName = DEVICE_NAME;
		this.password = "";
		this.onDisconnected();
		this.deploy();
	}
	onConnected() {
		this.stopAdvertising();
	}
	onDisconnected() {
		this.startAdvertising({
			advertisingData: {shortName: "Moddable", completeUUID16List: ["FF00"]},
			scanResponseData: {flags: 6, completeName: DEVICE_NAME}
		});
	}
	onCharacteristicWritten(params) {
		let uuid = params.characteristic.uuid;
		let value = params.value;
		if (WIFI_NETWORK_CHARACTERISTIC == uuid)
			this.ssid = String.fromArrayBuffer(value);
		else if (WIFI_PASSWORD_CHARACTERISTIC == uuid)
			this.password = String.fromArrayBuffer(value);
		else if (WIFI_CONTROL_CHARACTERISTIC == uuid) {
			let command = new Uint8Array(value)[0];
			if ((1 == command) && this.ssid) {
				this.close();
				this.connectToWiFiNetwork();
			}
		}
	}
	connectToWiFiNetwork() {
		trace(`Conneting to ${this.ssid}...\n`);
		let monitor = new WiFi({ssid: this.ssid, password: this.password}, msg => {
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
}
Object.freeze(WiFiServer.prototoype);


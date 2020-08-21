/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import BLEServer from "bleserver";
import {uuid} from "btutils";
import WiFi from "wifi";
import Net from "net";
import Timer from "timer";

export default class WiFiServer extends BLEServer {
	onReady() {
		this.deviceName = "Moddable Device";
		this.onDisconnected();
	}
	onConnected() {
		this.stopAdvertising();
	}
	onDisconnected() {
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: this.deviceName, completeUUID16List: [uuid`FF00`]}
		});
	}
	onCharacteristicNotifyDisabled(characteristic) {
		if ("status" == characteristic.name)
			delete this.notify;
	}
	onCharacteristicNotifyEnabled(characteristic) {
		if ("status" == characteristic.name)
			this.notify = characteristic;
	}
	onCharacteristicWritten(characteristic, value) {
		switch(characteristic.name) {
			case "SSID":
				this.ssid = value;
				this.doStatusNotification(`Setting SSID to <${this.ssid}>`);
				break;
			case "password":
				this.password = value;
				this.doStatusNotification(`Setting password to <${this.password}>`);
				break;
			case "control":
				if ((1 == value) && this.ssid) {
					this.doStatusNotification(`Connecting to <${this.ssid}>...`);
					this.connectToWiFiNetwork();
				}
				break;
		}
	}
	connectToWiFiNetwork() {
		let dictionary = { ssid:this.ssid };
		if (this.password)
			dictionary.password = this.password;
		let monitor = new WiFi(dictionary, msg => {
			switch (msg) {
				case "connect":
					break; // still waiting for IP address
				case "gotIP":
					this.doStatusNotification(`IP address: ${Net.get("IP")}`);
					Timer.set(() => this.close, 500);
					break;
				case "disconnect":
					this.doStatusNotification(`Connection lost`);
					break;  // connection lost
			}
		})
	}
	doStatusNotification(message) {
		trace(message, "\n");
		if (this.notify)
			this.notifyValue(this.notify, message);
	}
}
Object.freeze(WiFiServer.prototoype);


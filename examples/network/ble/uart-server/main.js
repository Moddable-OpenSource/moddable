/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
 	Nordic UART Service Server 
	The Nordic UART Service is a simple GATT-based service with TX and RX characteristics.
	Strings written to the UART Service server are echoed back to the client as characteristic notifications.

	https://devzone.nordicsemi.com/f/nordic-q-a/10567/what-is-nus-nordic-uart-service
 */

import BLEServer from "bleserver";
import {uuid} from "btutils";

const SERVICE_UUID = uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`;
const UART_SERVER = "UART Server";

class UARTServer extends BLEServer {
	onReady() {
		this.deviceName = "UART";
		this.onDisconnected();
	}
	onConnected() {
		this.stopAdvertising();
	}
	onDisconnected() {
		delete this.tx;
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: this.deviceName, completeUUID128List: [SERVICE_UUID]}
		});
	}
	onCharacteristicNotifyEnabled(characteristic) {
		this.tx = characteristic;
	}
	onCharacteristicNotifyDisabled(characteristic) {
		delete this.tx;
	}
	onCharacteristicWritten(characteristic, value) {
		trace.left(value, UART_SERVER);
		if (this.tx)
			this.notifyValue(this.tx, value);
	}
}

let server = new UARTServer;

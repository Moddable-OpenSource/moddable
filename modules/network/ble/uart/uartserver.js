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
 /*
 	This module implements a server class for the Nordic UART service:
 	
 	https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v14.0.0%2Fble_sdk_app_nus_eval.html
 */

import BLEServer from "bleserver";
import {uuid} from "btutils";

const SERVICE_UUID = uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`;
const RX_CHARACTERISTIC_UUID = uuid`6E400002-B5A3-F393-E0A9-E50E24DCCA9E`;
const TX_CHARACTERISTIC_UUID = uuid`6E400003-B5A3-F393-E0A9-E50E24DCCA9E`;

class UARTServer extends BLEServer {
	onReady() {
		this.deviceName = "UART";
		this.onDisconnected();
		this.deploy();
	}
	onConnected() {
		this.stopAdvertising();
	}
	onDisconnected() {
		this.startAdvertising({
			advertisingData: {shortName: this.deviceName},
			scanResponseData: {flags: 6, completeName: this.deviceName, completeUUID128List: [uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`]}
		});
	}
	onCharacteristicWritten(params) {
		this.onRX(params);
	}
	onRX(params) {
	}
}
Object.freeze(UARTServer.prototoype);

export {UARTServer as default, UARTServer, SERVICE_UUID, RX_CHARACTERISTIC_UUID, TX_CHARACTERISTIC_UUID}


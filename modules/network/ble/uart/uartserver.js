/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
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
	constructor(deviceName = "UART") {
		super();
		this.deviceName = deviceName;
	}
	onReady() {
		this.onDisconnected();
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
	onCharacteristicWritten(characteristic, value) {
		this.onRX(value);
	}
	onRX(value) {
	}
}
Object.freeze(UARTServer.prototoype);

export {UARTServer as default, UARTServer, SERVICE_UUID, RX_CHARACTERISTIC_UUID, TX_CHARACTERISTIC_UUID}


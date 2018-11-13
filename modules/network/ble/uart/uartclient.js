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
 	This module implements a client class for the Nordic UART service:
 	
 	https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v14.0.0%2Fble_sdk_app_nus_eval.html
 */

import BLEClient from "bleclient";
import {uuid} from "btutils";

const SERVICE_UUID = uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`;
const RX_CHARACTERISTIC_UUID = uuid`6E400002-B5A3-F393-E0A9-E50E24DCCA9E`;
const TX_CHARACTERISTIC_UUID = uuid`6E400003-B5A3-F393-E0A9-E50E24DCCA9E`;

class UARTClient extends BLEClient {
	onReady() {
		this.onDisconnected();
	}
	onDisconnected() {
		delete this.rx_characteristic;
		delete this.tx_characteristic;
		this.startScanning();
	}
	onDiscovered(device) {
		let found = false;
		let uuids = device.scanResponse.completeUUID128List;
		if (uuids)
			found = uuids.find(uuid => uuid.equals(SERVICE_UUID));
		if (!found) {
			uuids = device.scanResponse.incompleteUUID128List;
			if (uuids)
				found = uuids.find(uuid => uuid.equals(SERVICE_UUID));
		}
		if (!found) {
			if ("UART" == device.scanResponse.completeName)
				found = true;
		}
		if (found) {
			this.stopScanning();
			this.connect(device);
		}
	}
	onConnected(device) {
		device.discoverPrimaryService(SERVICE_UUID);
	}
	onServices(services) {
		if (services.length)
			services[0].discoverAllCharacteristics();
	}
	onCharacteristics(characteristics) {
		characteristics.forEach(characteristic => {
			let uuid = characteristic.uuid;
			if (uuid.equals(RX_CHARACTERISTIC_UUID))
				this.rx_characteristic = characteristic;
			else if (uuid.equals(TX_CHARACTERISTIC_UUID)) {
				this.tx_characteristic = characteristic;
				this.tx_characteristic.enableNotifications();
			}
		});
	}
}
Object.freeze(UARTClient.prototype);

export {UARTClient as default, UARTClient, SERVICE_UUID, RX_CHARACTERISTIC_UUID, TX_CHARACTERISTIC_UUID}


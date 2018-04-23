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

import GAP from "gap";
import {BluetoothAddress, Advertisement, UUID} from "btutils";

export class BLEServer @ "xs_ble_server_destructor" {
	constructor() {
		this.initialize();
	}
	close() @ "xs_ble_server_close"
	set deviceName() @ "xs_ble_server_set_device_name"
	startAdvertising(params) {
		let {fast = true, connectable = true, discoverable = true, scanResponseData = null, advertisingData} = params;
		let flags = GAP.ADFlag.NO_BR_EDR;
		if (discoverable)
			flags |= GAP.ADFlag.LE_GENERAL_DISCOVERABLE_MODE;
		let interval;
		if (connectable) {
			// Undirected Connectable Mode
			interval = fast ?
				(discoverable ? GAP.ADV_FAST_INTERVAL1 : GAP.ADV_FAST_INTERVAL2) :
				GAP.ADV_SLOW_INTERVAL;
		} else {
			// Non-Connectable Mode
			interval = fast ? GAP.ADV_FAST_INTERVAL2 : GAP.ADV_SLOW_INTERVAL;
		}
		advertisingData.flags = flags;
		let advertisingDataBuffer = Advertisement.serialize(advertisingData);
		let scanResponseDataBuffer = scanResponseData ? Advertisement.serialize(scanResponseData) : null;
		this._startAdvertising(interval.min, interval.max, advertisingDataBuffer, scanResponseDataBuffer);
	}
	stopAdvertising() @ "xs_ble_server_stop_advertising"
	deploy() @ "xs_ble_server_deploy"
	initialize() @ "xs_ble_server_initialize"
	
	set passkey() @ "xs_ble_server_set_passkey"
	
	onReady() {}
	onCharacteristicWritten() {}
	onCharacteristicRead() {}
	onConnected() {}
	onDisconnected() {}

	_startAdvertising() @ "xs_ble_server_start_advertising"

	callback(event, params) {
		//trace(`BLE callback ${event}\n`);
		switch(event) {
			case "onReady":
				this.onReady();
				break;
			case "onCharacteristicWritten":
				this.onCharacteristicWritten({
					characteristic:{ uuid:UUID.toString(params.uuid), handle:params.handle },
					value:params.value
				});
				break;
			case "onCharacteristicRead":
				return this.onCharacteristicRead({
					characteristic:{ uuid:UUID.toString(params.uuid), handle:params.handle }
				});
				break;
			case "onConnected":
				this.onConnected({
					address:BluetoothAddress.toString(params.address),
					connection:params.connection
				});
				break;
			case "onDisconnected":
				this.onDisconnected(params);
				break;
		}
	}
};
Object.freeze(BLEServer.prototype);

export default BLEServer;

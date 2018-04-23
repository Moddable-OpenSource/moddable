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
import Connection from "connection";
import {Client} from "gatt";
import {BluetoothAddress, Advertisement} from "btutils";

export class BLEClient @ "xs_ble_client_destructor" {
	constructor() {
		this.initialize();
	}
	
	initialize() @ "xs_ble_client_initialize"
	close() @ "xs_ble_client_close"
	
	set deviceName() @ "xs_ble_client_set_device_name"
	
	connect(address) {
		this._connect(BluetoothAddress.toBuffer(address));
	}
	
	onReady() {}
	onDiscovered() {}
	onConnected() {}
	
	// From Connection object
	onDisconnected() {}
	
	// From Client object
	onServices() {}

	// From Service object
	onCharacteristics() {}
	
	// From Characteristic object
	onCharacteristicValue() {}
	onCharacteristicNotification() {}

	startScanning(params) {
		if (!params) params = {};
		let {active = true, interval = 0x50, window = 0x30} = params;
		this._startScanning(active, interval, window);
	}
	stopScanning() @ "xs_ble_client_stop_scanning"
	
	_connect() @ "xs_ble_client_connect"
	_startScanning() @ "xs_ble_client_start_scanning"
	
	_onReady() {
		this.onReady();
	}
	_onDiscovered(params) {
		let address = BluetoothAddress.toString(params.address);
		let scanResponse = new Advertisement(params.scanResponse);
		this.onDiscovered({ address, scanResponse });
	}
	_onConnected(params) {
		let address = BluetoothAddress.toString(params.address);
		let ble = this;
		let client = new Client({ connection:params.connection, ble });
		let connection = new Connection({ address, client, ble });
		this.onConnected(client);
	}
	callback(event, params) {
		//trace(`BLE callback ${event}\n`);
		this[event](params);
	}
};
Object.freeze(BLEClient.prototype);

export default BLEClient;

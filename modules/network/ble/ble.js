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

import Advertisement from "gapadvdata";
import GAP from "gap";
import Connection from "connection";
import {BluetoothAddress, UUID} from "btutils";
import {Client} from "gatt";

import {BluetoothAddress as BTAddress, UUID as BTUUID, Advertisement as BTAdvertisement} from "btutils2";

class BLE @ "xs_ble_destructor" {
	constructor() {
		this.onReady = function() {};
		this.onDiscovered = function() {};
		this.onConnected = function() {};
	}
	
	initialize(params) @ "xs_ble_initialize"
	close() @ "xs_ble_close"
	
	set deviceName() @ "xs_ble_set_device_name"
	
	connect(address) {
		this._connect(BTAddress.toBuffer(address));
	}
	
	startAdvertising(params) {
		let fast = params.hasOwnProperty("fast") ? params.fast : true;
		let connectable = params.hasOwnProperty("connectable") ? params.connectable : true;
		let discoverable = params.hasOwnProperty("discoverable") ? params.discoverable : true;
		let scanResponseData = params.hasOwnProperty("scanResponseData") ? params.scanResponseData : null;
		let advertisingData = params.advertisingData;
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
		let advertisingDataBuffer = Advertisement.toByteArray(advertisingData).buffer;
		let scanResponseDataBuffer = scanResponseData ? Advertisement.toByteArray(scanResponseData).buffer : null;
		this._startAdvertising(interval.intervalMin, interval.intervalMax, advertisingDataBuffer, scanResponseDataBuffer);
	}
	stopAdvertising() @ "xs_ble_stop_advertising"
	
	startScanning(params) {
		if (!params) params = {};
		let active = params.hasOwnProperty("active") ? params.active : true;
		let interval = params.hasOwnProperty("interval") ? params.interval : 0x50;
		let window = params.hasOwnProperty("window") ? params.window : 0x30;
		this._startScanning(active, interval, window);
	}
	stopScanning() @ "xs_ble_stop_scanning"
	
	_connect() @ "xs_ble_connect"
	_startScanning() @ "xs_ble_start_scanning"
	_startAdvertising() @ "xs_ble_start_advertising"
	
	callback(event, params) {
		if ("onDiscovered" == event) {
			let address = BTAddress.toString(params.address);
			let scanResponse = new BTAdvertisement(params.scanResponse);
			params = { address, scanResponse };
		}
		else if ("onConnected" == event) {
			let address = BluetoothAddress.getByAddress(new Uint8Array(params.address)).toString();
			let client = new Client(params.connection);
			params = new Connection({ address, client });
		}
		this[event](params);
	}
};

export default BLE;

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
import {Advertisement, Bytes} from "btutils";
import {IOCapability} from "sm";

export class BLEClient @ "xs_ble_client_destructor" {
	constructor() {
		this.initialize();
	}
	
	initialize() @ "xs_ble_client_initialize"
	close() @ "xs_ble_client_close"
	
	connect(params) {
		this._connect(params.address, params.addressType);
	}
	
	onReady() {}
	onSecurityParameters() {}
	onDiscovered() {}
	onConnected() {}
	onBondingDeleted() {}
	
	// From Connection object
	onAuthenticated() {}
	onDisconnected() {}
	onPasskeyConfirm() {}
	onPasskeyDisplay() {}
	onPasskeyRequested() {}
	onRSSI() {}
	onMTUExchanged() {}
	
	// From Client object
	onServices() {}

	// From Service object
	onCharacteristics() {}
	
	// From Characteristic object
	onCharacteristicValue() {}
	onCharacteristicNotification() {}
	onCharacteristicNotificationEnabled() {}
	onCharacteristicNotificationDisabled() {}

	// From Descriptor object
	onDescriptorValue() {}
	onDescriptorWritten() {}

	set localPrivacy(how) @ "xs_ble_client_set_local_privacy"
	
	set securityParameters(params) {
		let {encryption = true, bonding = false, mitm = false, ioCapability = IOCapability.NoInputNoOutput} = params;
		this._setSecurityParameters(encryption, bonding, mitm, ioCapability);
	}
	
	startScanning(params = {}) {
		let {active = true, duplicates = true, interval = 0x50, window = 0x30, filterPolicy = GAP.ScanFilterPolicy.NONE } = params;
		this._startScanning(active, duplicates, interval, window, filterPolicy);
	}
	stopScanning() @ "xs_ble_client_stop_scanning"
	
	passkeyReply() @ "xs_ble_client_passkey_reply"

	_connect() @ "xs_ble_client_connect"
	_startScanning() @ "xs_ble_client_start_scanning"
	_setSecurityParameters() @ "xs_ble_client_set_security_parameters"
	
	callback(event, params) {
		//trace(`BLE callback ${event}\n`);
		switch(event) {
			case "onReady":
				this.onReady();
				break;
			case "onSecurityParameters":
				this.onSecurityParameters(params);
				break;
			case "onDiscovered": {
				const address = new Bytes(params.address);
				const addressType = params.addressType;
				const rssi = params.rssi;
				const scanResponse = new Advertisement(params.scanResponse);
				this.onDiscovered({ address, addressType, rssi, scanResponse });
				break;
			}
			case "onConnected": {
				const address = new Bytes(params.address);
				const addressType = params.addressType;
				const ble = this;
				const client = new Client({ address, addressType, connection:params.connection, ble });
				const connection = new Connection({ address, addressType, client, ble });
				this.onConnected(client);
				break;
			}
			case "onDisconnected":
				this.onDisconnected({ address:new Bytes(params.address), addressType:params.addressType, connection:params.connection });
				break;
			case "onPasskeyConfirm":
				this.onPasskeyConfirm({ address:new Bytes(params.address), passkey:params.passkey });
				break;
			case "onPasskeyDisplay":
				this.onPasskeyDisplay({ address:new Bytes(params.address), passkey:params.passkey });
				break;
			case "onPasskeyRequested":
				return this.onPasskeyRequested({ address:new Bytes(params.address) });
				break;
			case "onAuthenticated":
				this.onAuthenticated({ bonded:params.bonded });
				break;
			case "onBondingDeleted": {
				this.onBondingDeleted({ address:new Bytes(params.address), addressType:params.addressType });
				break;
			}
		}
	}
};
Object.freeze(BLEClient.prototype);

export default BLEClient;

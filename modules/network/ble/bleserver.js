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
import {Advertisement, Bytes} from "btutils";
import {IOCapability} from "sm";

export class BLEServer @ "xs_ble_server_destructor" {
	constructor() {
		this.initialize();
		this.device_name = "";
	}
	close() @ "xs_ble_server_close"
	set deviceName(it) {
		this.device_name = it;
		this._setDeviceName(it);
	}
	get deviceName() {
		return this.device_name;
	}
	set securityParameters(params) {
		let {encryption = true, bonding = false, mitm = false, ioCapability = IOCapability.NoInputNoOutput} = params;
		this._setSecurityParameters(encryption, bonding, mitm, ioCapability);
	}
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
	
	get localAddress() {
		return new Bytes(this._getLocalAddress());
	}
	
	notifyValue(characteristic, value) {
		value = this._typedValueToBuffer(characteristic.type, value);
		this._notifyValue(characteristic.handle, characteristic.notify, value);
	}
	
	passkeyReply() @ "xs_ble_server_passkey_reply"
	
	disconnect() @ "xs_ble_server_disconnect"

	onReady() {}
	onCharacteristicWritten() {}
	onCharacteristicRead() {}
	onCharacteristicNotifyEnabled() {}
	onCharacteristicNotifyDisabled() {}
	onConnected() {}
	onDisconnected() {}
	onPasskeyConfirm() {}
	onPasskeyDisplay() {}
	onPasskeyRequested() {}
	onAuthenticated() {}

	_setDeviceName() @ "xs_ble_server_set_device_name"
	_startAdvertising() @ "xs_ble_server_start_advertising"
	_notifyValue() @ "xs_ble_server_characteristic_notify_value"

	_getLocalAddress() @ "xs_ble_server_get_local_address"
	
	_setSecurityParameters() @ "xs_ble_server_set_security_parameters"

	_typedValueToBuffer(type, value) {
		let buffer;
		switch(type) {
			case "Array":
				buffer = new Uint8Array(value).buffer;
				break;
			case "String":
				buffer = ArrayBuffer.fromString(value);
				break;
			case "Uint8":
				buffer = Uint8Array.of(value & 0xFF).buffer;
				break;
			case "Int16":
			case "Uint16":
				buffer = Uint8Array.of(value & 0xFF, (value >> 8) & 0xFF).buffer;
				break;
			case "Uint32":
				buffer = Uint8Array.of(value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF).buffer;
				break;
			default:
				buffer = value;
				break;
		}
		return buffer;
	}
	_typedBufferToValue(type, buffer) {
		let value;
		switch(type) {
			case "Array":
				value = new Uint8Array(buffer);
				break;
			case "String":
				value = String.fromArrayBuffer(buffer);
				break;
			case "Uint8":
				value = new Uint8Array(buffer)[0] & 0xFF;
				break;
			case "Int16":
				value = (new DataView(buffer)).getInt16(0, true);
				break;
			case "Uint16":
				value = (new DataView(buffer)).getUint16(0, true);
				break;
			case "Uint32":
				value = (new DataView(buffer)).getUint32(0, true);
				break;
			default:
				value = buffer;
				break;
		}
		return value;
	}
	
	callback(event, params) {
		//trace(`BLE callback ${event}\n`);
		switch(event) {
			case "onReady":
				this.onReady();
				break;
			case "onCharacteristicWritten":
				params.value = this._typedBufferToValue(params.type, params.value);
				this.onCharacteristicWritten(params);
				break;
			case "onCharacteristicRead": {
				let value = this.onCharacteristicRead({ uuid:new Bytes(params.uuid), handle:params.handle, name:params.name });
				value = this._typedValueToBuffer(params.type, value);
				return value;
			}
			case "onCharacteristicNotifyEnabled":
				this.onCharacteristicNotifyEnabled(params);
				break;
			case "onCharacteristicNotifyDisabled":
				this.onCharacteristicNotifyDisabled(params);
				break;
			case "onConnected":
				this.onConnected({ address:new Bytes(params.address), connection:params.connection });
				break;
			case "onDisconnected":
				this.onDisconnected({ address:new Bytes(params.address), connection:params.connection });
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
				return this.onAuthenticated();
				break;
		}
	}
};
Object.freeze(BLEServer.prototype);

export default BLEServer;

/*
 * Copyright (c) 2016-2024  Moddable Tech, Inc.
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
import {Advertisement, Bytes, typedValueToBuffer, typedBufferToValue} from "btutils";
import {IOCapability} from "sm";

export class BLEServer @ "xs_ble_server_destructor" {
	#deviceName = "";

	constructor(options = {}) {
		initialize.call(this, options);
	}
	close() @ "xs_ble_server_close"
	set deviceName(name) {
		this.#deviceName = name;
		deviceName.call(this, name);
	}
	get deviceName() {
		return this.#deviceName;
	}
	set securityParameters(options) {
		let {encryption = true, bonding = false, mitm = false, ioCapability = IOCapability.NoInputNoOutput} = options;
		securityParameters.call(this, encryption, bonding, mitm, ioCapability);
	}
	startAdvertising(options) {
		let {fast = true, scanResponseData = null, filterPolicy = GAP.AdvFilterPolicy.NONE, advertisingData, notify = false, interval} = options;
		let flags = advertisingData.flags ?? GAP.ADFlag.NO_BR_EDR;
		if (undefined === interval) {
			if (flags & (GAP.ADFlag.LE_LIMITED_DISCOVERABLE_MODE | GAP.ADFlag.LE_GENERAL_DISCOVERABLE_MODE)) {
				// Connectable mode
				interval = fast ? GAP.ADV_FAST_INTERVAL1 : GAP.ADV_SLOW_INTERVAL;
			} else {
				// Non-Connectable Mode
				interval = fast ? GAP.ADV_FAST_INTERVAL2 : GAP.ADV_SLOW_INTERVAL;
			}
		}
		startAdvertising.call(this, flags, interval.min, interval.max, filterPolicy,
			Advertisement.serialize(advertisingData), scanResponseData ? Advertisement.serialize(scanResponseData) : null, notify);
	}
	stopAdvertising() @ "xs_ble_server_stop_advertising"
	
	get localAddress() {
		return new Bytes(localAddress.call(this));
	}
	
	notifyValue(characteristic, value) {
		value = typedValueToBuffer(characteristic.type, value);
		notifyValue.call(this, characteristic.handle, characteristic.notify, value);
	}
	
	passkeyInput() @ "xs_ble_server_passkey_input"
	passkeyReply() @ "xs_ble_server_passkey_reply"
	
	disconnect() @ "xs_ble_server_disconnect"

	getServiceAttributes(uuid) {
		let atts = getServiceAttributes.call(this, uuid);
		if (undefined === atts)
			return [];
		atts.forEach(att => {
			if (undefined !== att.type && undefined !== att.value) {
				att.value = typedBufferToValue(att.type, att.value);
			}
			att.uuid = new Bytes(att.uuid);
		});
		return atts;
	}

	callback(event, options) {
		//trace(`BLE callback ${event}\n`);
		switch(event) {
			case "onReady":
				this.onReady?.();
				deploy.call(this);
				break;
			case "onCharacteristicWritten":
				this.onCharacteristicWritten?.({ uuid:new Bytes(options.uuid), handle:options.handle, name:options.name, type:options.type },
						typedBufferToValue(options.type, options.value));
				break;
			case "onCharacteristicRead": {
				const value = this.onCharacteristicRead?.({ uuid:new Bytes(options.uuid), handle:options.handle, name:options.name, type:options.type });
				if (undefined !== value)
					return typedValueToBuffer(options.type, value);
				}
				break;
			case "onCharacteristicNotifyEnabled":
				this.onCharacteristicNotifyEnabled?.(options);
				break;
			case "onCharacteristicNotifyDisabled":
				this.onCharacteristicNotifyDisabled?.(options);
				break;
			case "onConnected":
				this.onConnected?.({ address:new Bytes(options.address), addressType:options.addressType, connection:options.connection });
				break;
			case "onDisconnected":
				this.onDisconnected?.({ address:new Bytes(options.address), addressType:options.addressType, connection:options.connection });
				break;
			case "onPasskeyConfirm":
				this.onPasskeyConfirm?.({ address:new Bytes(options.address), passkey:options.passkey });
				break;
			case "onPasskeyDisplay":
				this.onPasskeyDisplay?.({ address:new Bytes(options.address), passkey:options.passkey });
				break;
			case "onPasskeyInput":
				this.onPasskeyInput?.({ address:new Bytes(options.address) });
				break;
			case "onPasskeyRequested":
				return this.onPasskeyRequested?.({ address:new Bytes(options.address) });
				break;
			case "onAuthenticated":
				this.onAuthenticated?.({ bonded:options.bonded });
				break;
			case "onMTUExchanged":
				this.onMTUExchanged?.(options);
				break;
			case "onAdvertisementSent":
				this.onAdvertisementSent?.();
				break;
			case "onBondingDeleted": {
				this.onBondingDeleted?.({ address:new Bytes(options.address), addressType:options.addressType });
				break;
			}
		}
	}
};

function initialize(options) @ "xs_ble_server_initialize";
function deploy() @ "xs_ble_server_deploy";
function deviceName() @ "xs_ble_server_set_device_name";
function startAdvertising() @ "xs_ble_server_start_advertising";
function notifyValue() @ "xs_ble_server_characteristic_notify_value";
function localAddress() @ "xs_ble_server_get_local_address";
function securityParameters() @ "xs_ble_server_set_security_parameters";
function getServiceAttributes() @ "xs_ble_server_get_service_attributes";

export default BLEServer;

/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
	https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Services/org.bluetooth.service.health_thermometer.xml
	https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.temperature_measurement.xml
 */

import BLEClient from "bleclient";
import {uuid} from "btutils";
import {SM, IOCapability} from "sm";

const HTM_SERVICE_UUID = uuid`1809`;
const TEMPERATURE_CHARACTERISTIC_UUID = uuid`2A1C`;

class SecureHealthThermometerClient extends BLEClient {
	onReady() {
		this.securityParameters = { mitm:true, ioCapability:IOCapability.DisplayOnly };
		//this.securityParameters = { mitm:true, bonding:true, ioCapability:IOCapability.DisplayOnly };
		//this.securityParameters = { mitm:true, ioCapability:IOCapability.KeyboardDisplay };
		//this.securityParameters = { mitm:true, ioCapability:IOCapability.KeyboardOnly };
		//this.securityParameters = { mitm:true, ioCapability:IOCapability.NoInputNoOutput };
		this.onDisconnected();
	}
	onDiscovered(device) {
		let found = false;
		let uuids = device.scanResponse.completeUUID16List;
		if (uuids)
			found = uuids.find(uuid => uuid.equals(HTM_SERVICE_UUID));
		if (!found) {
			uuids = device.scanResponse.incompleteUUID16List;
			if (uuids)
				found = uuids.find(uuid => uuid.equals(HTM_SERVICE_UUID));
		}
		if (found) {
			this.stopScanning();
			this.connect(device);
		}
	}
	onAuthenticated(params) {
		this.authenticated = true;
		this.bonded = params.bonded;
		this.characteristic?.enableNotifications();
	}
	onConnected(device) {
		trace(`connected to device ${device.address}\n`);
		device.discoverPrimaryService(HTM_SERVICE_UUID);
	}
	onDisconnected(device) {
		if (device) {
			trace(`disconnected from device ${device.address}\n`);
			if (this.bonded) {
				SM.deleteBonding(device.address, device.addressType);
				delete this.bonded;
			}
		}
		delete this.characteristic;
		delete this.authenticated;
		this.startScanning();
	}
	onServices(services) {
		if (services.length)
			services[0].discoverCharacteristic(TEMPERATURE_CHARACTERISTIC_UUID);
	}
	onCharacteristics(characteristics) {
		if (characteristics.length) {
			this.characteristic = characteristics[0];
			if (this.authenticated)
				this.characteristic.enableNotifications();
		}
	}
	onCharacteristicNotification(characteristic, value) {
		let units = value[0];
		let temp = value[1] | (value[2] << 8) | (value[3] << 16) | (value[4] << 24);
		let exponent = (temp >> 24) & 0xFF;
		exponent = exponent & 0x80 ? -(~exponent + 257): exponent;
		let mantissa = temp & 0xFFFFFF;
		let temperature = (mantissa * Math.pow(10, exponent)).toFixed(2);
		trace(`${temperature}\n`);
	}
	onPasskeyConfirm(params) {
		let passkey = this.passkeyToString(params.passkey);
		trace(`client confirm passkey: ${passkey}\n`);
		this.passkeyReply(params.address, true);
	}
	onPasskeyDisplay(params) {
		let passkey = this.passkeyToString(params.passkey);
		trace(`client display passkey: ${passkey}\n`);
	}
	onPasskeyInput(params) {
		trace(`client input passkey displayed by peer\n`);
		//let passkey = 0;
		//this.passkeyInput(params.address, passkey);
	}
	onPasskeyRequested(params) {
		let passkey = Math.round(Math.random() * 999999);
		trace(`client requested passkey: ${this.passkeyToString(passkey)}\n`);
		return passkey;
	}
	onBondingDeleted(params) {
		trace(`device ${params.address} bond deleted\n`);
	}
	passkeyToString(passkey) {
		return passkey.toString().padStart(6, "0");
	}
}

let htm = new SecureHealthThermometerClient;

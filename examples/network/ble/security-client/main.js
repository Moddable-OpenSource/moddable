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
	https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Services/org.bluetooth.service.health_thermometer.xml
	https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.temperature_measurement.xml
 */

import BLEClient from "bleclient";
import {uuid} from "btutils";
import {IOCapability} from "sm";

const HTM_SERVICE_UUID = uuid`1809`;
const TEMPERATURE_CHARACTERISTIC_UUID = uuid`2A1C`;

class SecureHealthThermometerClient extends BLEClient {
	onReady() {
		this.securityParameters = { mitm:true, ioCapability:IOCapability.DisplayOnly };
		//this.securityParameters = { mitm:true, ioCapability:IOCapability.KeyboardDisplay };
		//this.securityParameters = { mitm:true, ioCapability:IOCapability.KeyboardOnly };
		//this.securityParameters = { mitm:true, ioCapability:IOCapability.NoInputNoOutput };
		this.onDisconnected();
	}
	onDiscovered(device) {
		if ('Moddable HTM' == device.scanResponse.completeName) {
			this.stopScanning();
			this.connect(device);
		}
	}
	onConnected(device) {
		device.discoverPrimaryService(HTM_SERVICE_UUID);
	}
	onDisconnected() {
		this.startScanning();
	}
	onServices(services) {
		if (services.length)
			services[0].discoverCharacteristic(TEMPERATURE_CHARACTERISTIC_UUID);
	}
	onCharacteristics(characteristics) {
		if (characteristics.length)
			characteristics[0].enableNotifications();
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
	passkeyToString(passkey) {
		return passkey.toString().padStart(6, "0");
	}
}

let htm = new SecureHealthThermometerClient;

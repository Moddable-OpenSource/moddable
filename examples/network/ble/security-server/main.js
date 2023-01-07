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

import BLEServer from "bleserver";
import {uuid} from "btutils";
import {SM, IOCapability} from "sm";
import Timer from "timer";

const HTM_SERVICE_UUID = uuid`1809`;
const BATTERY_SERVICE_UUID = uuid`180F`;

class SecureHealthThermometerServer extends BLEServer {
	onReady() {
		this.deviceName = "Moddable HTM";
		this.securityParameters = { mitm:true, ioCapability:IOCapability.DisplayOnly };
		//this.securityParameters = { mitm:true, bonding:true, ioCapability:IOCapability.DisplayOnly };
		//this.securityParameters = { mitm:true, ioCapability:IOCapability.KeyboardDisplay };
		//this.securityParameters = { mitm:true, ioCapability:IOCapability.KeyboardOnly };
		//this.securityParameters = { mitm:true, ioCapability:IOCapability.NoInputNoOutput };
		//this.securityParameters = { ioCapability:IOCapability.NoInputNoOutput };
		this.onDisconnected();
	}
	onAuthenticated(params) {
		this.authenticated = true;
		this.bonded = params.bonded;
		if (this.characteristic)
			this.startMeasurements();
	}
	onConnected(device) {
		trace(`connected to device ${device.address}\n`);
		this.stopAdvertising();
	}
	onDisconnected(device) {
		if (device) {
			trace(`disconnected from device ${device.address}\n`);
			if (this.bonded) {
				SM.deleteBonding(device.address, device.addressType);
				delete this.bonded;
			}
		}
		this.stopMeasurements();
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: this.deviceName, completeUUID16List: [HTM_SERVICE_UUID, BATTERY_SERVICE_UUID]}
		});
	}
	onCharacteristicNotifyEnabled(characteristic) {
		this.characteristic = characteristic;
		if (this.authenticated)
			this.startMeasurements();
	}
	onCharacteristicNotifyDisabled(characteristic) {
		this.stopMeasurements();
	}
	onPasskeyConfirm(params) {
		let passkey = this.passkeyToString(params.passkey);
		trace(`server confirm passkey: ${passkey}\n`);
		this.passkeyReply(params.address, true);
	}
	onPasskeyDisplay(params) {
		let passkey = this.passkeyToString(params.passkey);
		trace(`server display passkey: ${passkey}\n`);
	}
	onPasskeyInput(params) {
		trace(`server input passkey displayed by peer\n`);
		//let passkey = 0;
		//this.passkeyInput(params.address, passkey);
	}
	onPasskeyRequested(params) {
		let passkey = Math.round(Math.random() * 999999);
		trace(`server requested passkey: ${this.passkeyToString(passkey)}\n`);
		return passkey;
	}
	onBondingDeleted(params) {
		trace(`device ${params.address} bond deleted\n`);
	}
	get temperature() {
		if (98.5 > this.temp)
			this.temp += 0.1;
		let flags = 0x01;		// fahrenheit
		let exponent = 0xFD;	// -1
		let mantissa = Math.round(this.temp * 1000);
		let temp = (exponent << 24) | mantissa;		// IEEE-11073 32-bit float
		let result = [flags, temp & 0xFF, (temp >> 8) & 0xFF, (temp >> 16) & 0xFF, (temp >> 24) & 0xFF];
		return result;
	}
	startMeasurements() {
		this.timer = Timer.repeat(id => {
			if (this.characteristic)
				this.notifyValue(this.characteristic, this.temperature);
		}, 250);
	}
	stopMeasurements() {
		if (this.timer) {
			Timer.clear(this.timer);
			delete this.timer;
		}
		delete this.characteristic;
		this.temp = 95.0;
		this.authenticated = false;
	}
	passkeyToString(passkey) {
		return passkey.toString().padStart(6, "0");
	}
}

let htm = new SecureHealthThermometerServer;

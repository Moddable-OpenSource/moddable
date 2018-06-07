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
	https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.health_thermometer.xml
	https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.temperature_measurement.xml
 */

import BLEServer from "bleserver";
import {uuid} from "btutils";
import {SM, IOCapability} from "sm";
import Timer from "timer";

class SecureHealthThermometerServer extends BLEServer {
	onReady() {
		this.timer = null;
		this.deviceName = "Moddable HTM";
		SM.securityParameters = { mitm:true, ioCapability:IOCapability.DisplayOnly };
		//SM.securityParameters = { mitm:true, ioCapability:IOCapability.KeyboardDisplay };
		//SM.securityParameters = { mitm:true, ioCapability:IOCapability.KeyboardOnly };
		//SM.securityParameters = { mitm:true, ioCapability:IOCapability.NoInputNoOutput };
		//SM.securityParameters = { ioCapability:IOCapability.NoInputNoOutput };
		this.onDisconnected();
		this.deploy();
	}
	onAuthenticated() {
		this.authenticated = true;
		if (this.characteristic)
			this.startMeasurements();
	}
	onConnected() {
		this.stopAdvertising();
	}
	onDisconnected() {
		this.stopMeasurements();
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: this.deviceName, completeUUID16List: [uuid`1809`, uuid`180F`]}
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
		return true;
	}
	onPasskeyDisplay(params) {
		let passkey = this.passkeyToString(params.passkey);
		trace(`server display passkey: ${passkey}\n`);
	}
	onPasskeyRequested(params) {
		let passkey = Math.round(Math.random() * 999999);
		trace(`server requested passkey: ${this.passkeyToString(passkey)}\n`);
		return passkey;
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
			this.timer = null;
		}
		this.temp = 95.0;
		this.authenticated = false;
		this.characteristic = null;
	}
	passkeyToString(passkey) {
		return passkey.toString().padStart(6, "0");
	}
}

let htm = new SecureHealthThermometerServer;

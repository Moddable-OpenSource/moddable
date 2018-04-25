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
import Timer from "timer";

class HealthThermometerService extends BLEServer {
	onReady() {
		this.timer = 0;
		this.battery = 85;	// battery level percent
		this.deviceName = "Moddable HTM";
		this.onDisconnected();
		this.deploy();
	}
	onConnected() {
		this.stopAdvertising();
	}
	onDisconnected() {
		this.temp = 95.0;
		if (this.timer) {
			Timer.clear(this.timer);
			this.timer = 0;
		}
		this.startAdvertising({
			advertisingData: {shortName: "Thermometer", completeUUID16List: ["1809","180F"]},
			scanResponseData: {flags: 6, completeName: "HTM Example"}
		});
	}
	onCharacteristicRead(params) {
		return this[params.name];
	}
	onCharacteristicNotifyEnabled(params) {
		this.timer = Timer.repeat(id => {
			this.notifyValue(params, this.temperature);
		}, 250);
	}
	onCharacteristicNotifyDisabled(params) {
		Timer.clear(this.timer);
		this.timer = 0;
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
}

let htm = new HealthThermometerService;

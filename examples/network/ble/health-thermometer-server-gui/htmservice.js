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

import BLEServer from "bleserver";
import {uuid} from "btutils";
import Timer from "timer";

export default class HealthThermometerService extends BLEServer {
	onReady() {
		this.deviceName = "Moddable HTM";
		this.onDisconnected();
	}
	onConnected() {
		this.stopAdvertising();
		application.distribute("onConnected");
	}
	onDisconnected() {
		this.stopMeasurements();
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: this.deviceName, completeUUID16List: [uuid`1809`, uuid`180F`]}
		});
		application.distribute("onDisconnected");
	}
	onCharacteristicNotifyEnabled(characteristic) {
		this.startMeasurements(characteristic);
	}
	onCharacteristicNotifyDisabled(characteristic) {
		this.stopMeasurements();
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
	startMeasurements(characteristic) {
		this.timer = Timer.set(id => {
			this.notifyValue(characteristic, this.temperature);
			application.distribute("onValue", this.temp);
		}, 1000, 100);
	}
	stopMeasurements() {
		if (this.timer) {
			Timer.clear(this.timer);
			delete this.timer;
		}
		this.temp = 95.9;
	}
}
Object.freeze(HealthThermometerService.prototoype);


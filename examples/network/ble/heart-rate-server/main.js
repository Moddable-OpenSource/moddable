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
	https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.heart_rate.xml
 	https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.generic_access.xml
	https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.battery_service.xml
 	https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.heart_rate_measurement.xml
	https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.body_sensor_location.xml
 */

import BLEServer from "bleserver";
import Timer from "timer";

class HeartRateService extends BLEServer {
	onReady() {
		this.timer = 0;
		this.bpm = [0, 60];		// flags, beats per minute
		this.location = 1;		// chest location
		this.battery = 85;		// battery level percent
		this.device_name = "Moddable HRM";
		this.appearance = 832;	// generic heart rate sensor
		
		this.deviceName = this.device_name;
		this.onDisconnected();
		this.deploy();
	}
	onConnected() {
		this.stopAdvertising();
	}
	onDisconnected() {
		if (this.timer) {
			Timer.clear(this.timer);
			this.timer = 0;
		}
		this.startAdvertising({
			advertisingData: {shortName: "HRS", completeUUID16List: ["180D","180F"]},
			scanResponseData: {flags: 6, completeName: "HRS Example"}
		});
	}
	onCharacteristicRead(params) {
		return this[params.name];
	}
	onCharacteristicNotifyEnabled(params) {
		this.timer = Timer.repeat(id => {
			this.notifyValue(params, this.bpm);
		}, 1000);
	}
	onCharacteristicNotifyDisabled(params) {
		Timer.clear(this.timer);
		this.timer = 0;
	}
}

let hrs = new HeartRateService;




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
		this.timer = null;
		this.deviceName = "Moddable HRM";
		this.onDisconnected();
		this.deploy();
	}
	onConnected() {
		this.stopAdvertising();
	}
	onDisconnected() {
		this.stopMeasurements();
		this.startAdvertising({
			advertisingData: {shortName: "HRS"},
			scanResponseData: {flags: 6, completeName: this.deviceName, completeUUID16List: ["180D","180F"]}
		});
	}
	onCharacteristicNotifyEnabled(characteristic) {
		this.startMeasurements(characteristic);
	}
	onCharacteristicNotifyDisabled(characteristic) {
		this.stopMeasurements();
	}
	startMeasurements(characteristic) {
		this.bump = +1;
		this.timer = Timer.repeat(id => {
			this.notifyValue(characteristic, this.bpm);
			this.bpm[1] += this.bump;
			if (this.bpm[1] == 65) {
				this.bump = -1;
				this.bpm[1] == 64;
			}
			else if (this.bpm[1] == 55) {
				this.bump = +1;
				this.bpm[1] == 56;
			}
		}, 1000);
	}
	stopMeasurements() {
		if (this.timer) {
			Timer.clear(this.timer);
			this.timer = null;
		}
		this.bpm = [0, 60]; // flags, beats per minute
	}
}

let hrs = new HeartRateService;




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
 	https://www.bluetooth.org/docman/handlers/downloaddoc.ashx?doc_id=239866&_ga=2.113570511.973536733.1524505559-1286694985.1517851833
 	https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.heart_rate_measurement.xml
	https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.body_sensor_location.xml
 */

import BLEServer from "bleserver";
import Timer from "timer";

const DEVICE_NAME = "Moddable HRM";

class HeartRateService extends BLEServer {
	onReady() {
		this.timer = 0;
		this.deviceName = DEVICE_NAME;
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
			advertisingData: {shortName: "HRS", completeUUID16List: ["180D"]},
			scanResponseData: {flags: 6, completeName: "HRS Example"}
		});
	}
	onCharacteristicRead(params) {
		if ("location" == params.name)
			return 1;		// chest
		else if ("bpm" == params.name)
			return [0, 60];	// flags, beats per minute
	}
	onCharacteristicNotifyEnabled(params) {
		this.timer = Timer.repeat(id => {
			this.notifyValue(params, [0, 60]);
		}, 1000);
	}
	onCharacteristicNotifyDisabled(params) {
		Timer.clear(this.timer);
		this.timer = 0;
	}
}

let hrs = new HeartRateService;




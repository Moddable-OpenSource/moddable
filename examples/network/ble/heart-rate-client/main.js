/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
	https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Services/org.bluetooth.service.heart_rate.xml
 */

import BLEClient from "bleclient";
import {uuid} from "btutils";

const HR_SERVICE_UUID = uuid`180D`;
const HRM_CHARACTERISTIC_UUID = uuid`2A37`;

class HeartRateClient extends BLEClient {
	onReady() {
		this.onDisconnected();
	}
	onDiscovered(device) {
		let found = false;
		let uuids = device.scanResponse.completeUUID16List;
		if (uuids)
			found = uuids.find(uuid => uuid.equals(HR_SERVICE_UUID));
		if (!found) {
			uuids = device.scanResponse.incompleteUUID16List;
			if (uuids)
				found = uuids.find(uuid => uuid.equals(HR_SERVICE_UUID));
		}
		if (found) {
			this.stopScanning();
			this.connect(device);
		}
	}
	onConnected(device) {
		device.discoverPrimaryService(HR_SERVICE_UUID);
	}
	onDisconnected() {
		this.startScanning();
	}
	onServices(services) {
		if (services.length)
			services[0].discoverCharacteristic(HRM_CHARACTERISTIC_UUID);
	}
	onCharacteristics(characteristics) {
		if (characteristics.length)
			characteristics[0].enableNotifications();
	}
	onCharacteristicNotification(characteristic, value) {
		trace(`Beats per minute: ${value[1]}\n`);
	}
}

let hrc = new HeartRateClient;




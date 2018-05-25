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
	This application discovers the device information service (0x180A), traces the
	manufacturer name read from the contained characteristic (0x2A29) and disconnects.
	To use the app, set the DEVICE_NAME string to your device's advertised complete name.
*/

import BLEClient from "bleclient";

const DEVICE_NAME = "<YOUR DEVICE NAME>";
const DEVICE_INFORMATION_SERVICE_UUID = 0x180A;
const MANUFACTURER_NAME_UUID = 0x2A29;

class Discovery extends BLEClient {
	onReady() {
		this.startScanning();
	}
	onDiscovered(device) {
		if (DEVICE_NAME == device.scanResponse.completeName) {
			this.stopScanning();
			this.connect(device);
		}
	}
	onConnected(device) {
		this.device = device;
		device.discoverPrimaryService(DEVICE_INFORMATION_SERVICE_UUID);
	}
	onServices(services) {
		if (services.length)
			services[0].discoverCharacteristic(MANUFACTURER_NAME_UUID);
	}
	onCharacteristics(characteristics) {
		if (characteristics.length)
			characteristics[0].readValue();
	}
	onCharacteristicValue(characteristic, value) {
		let manufacturer = String.fromArrayBuffer(value);
		trace(`Manufacturer: ${manufacturer}\n`);
		this.device.close();
	}
	onDisconnected() {
		trace("done!\n");
	}
}

let discovery = new Discovery;

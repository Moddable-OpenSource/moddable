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
 	BLE Colorific Light Bulb
	https://www.walmart.com/ip/Star-Tech-Bc090-Colorific-A19-Bluetooth-D67-Controlled-LED-Bulb/46711393
 */

import BLEClient from "bleclient";
import Timer from "timer";

const DEVICE_NAME = "RGBLightOne";
const SERVICE_UUID = '1802';
const CHARACTERISTIC_UUID = '2A06';

class Colorific extends BLEClient {
	onReady() {
		let payload = this.payload = new Uint8Array(9);
		payload[0] = 0x58;
		payload[1] = 0x01;
		payload[2] = 0x03;
		payload[3] = 0x01;
		payload[4] = 0x10; // White brightness
		payload[5] = 0x00; // Separator byte
		this.startScanning();
	}
	onDiscovered(device) {
		if (DEVICE_NAME == device.scanResponse.completeName) {
			this.stopScanning();
			this.connect(device.address);
		}
	}
	onConnected(device) {
		device.discoverPrimaryService(SERVICE_UUID);
	}
	onServices(services) {
		let service = services.find(service => SERVICE_UUID == service.uuid);
		if (service)
			service.discoverCharacteristic(CHARACTERISTIC_UUID);
	}
	onCharacteristics(characteristics) {
		let characteristic = characteristics.find(characteristic => CHARACTERISTIC_UUID == characteristic.uuid);
		if (characteristic) {
			this.timer = Timer.repeat(() => {
				let payload = this.payload;
				payload[6] = Math.floor(Math.random() * 256);
				payload[7] = Math.floor(Math.random() * 256);
				payload[8] = Math.floor(Math.random() * 256);
				characteristic.writeWithoutResponse(payload.buffer);
			}, 100);
		}
	}
	onDisconnected() {
		Timer.clear(this.timer);
		this.startScanning();
	}
}

let colorific = new Colorific;

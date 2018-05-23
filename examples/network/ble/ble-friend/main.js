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
 	Adafruit Bluefruit LE Friend UART service
 	https://learn.adafruit.com/introducing-adafruit-ble-bluetooth-low-energy-friend/uart-service
	https://learn.adafruit.com/introducing-adafruit-ble-bluetooth-low-energy-friend/terminal-settings
 
 	To use this application, first set the BLEFriend for UART mode and connect with a serial terminal @ 9600,8,N,1 and line feed output.
 	Strings written to the RX characteristic display in the terminal, and strings sent from
 	the terminal arrive as TX notifications and are traced to the console.
 */

import BLEClient from "bleclient";
import Timer from "timer";

const DEVICE_NAME = "UART";
const SERVICE_UUID = '6E400001-B5A3-F393-E0A9-E50E24DCCA9E';
const CHARACTERISTIC_RX_UUID = '6E400002-B5A3-F393-E0A9-E50E24DCCA9E';
const CHARACTERISTIC_TX_UUID = '6E400003-B5A3-F393-E0A9-E50E24DCCA9E';

class BLEFriend extends BLEClient {
	onReady() {
		this.count = 1;
		this.startScanning();
	}
	onDiscovered(device) {
		let completeName = device.scanResponse.completeName;
		if (completeName && ((completeName == "UART") || completeName.includes("BLE Friend"))) {
			this.stopScanning();
			this.connect(device);
		}
	}
	onConnected(device) {
		device.discoverPrimaryService(SERVICE_UUID);
	}
	onServices(services) {
		let service = services.find(service => SERVICE_UUID == service.uuid);
		if (service)
			service.discoverAllCharacteristics();
	}
	onCharacteristics(characteristics) {
		let rx_characteristic, tx_characteristic;
		for (let i = 0; i < characteristics.length; ++i) {
			let characteristic = characteristics[i];
			if (CHARACTERISTIC_RX_UUID == characteristic.uuid)
				rx_characteristic = characteristic;
			if (CHARACTERISTIC_TX_UUID == characteristic.uuid)
				tx_characteristic = characteristic;
		}
		if (tx_characteristic && rx_characteristic) {
			tx_characteristic.enableNotifications();
			this.timer = Timer.repeat(() => {
				rx_characteristic.writeWithoutResponse(`Hello UART ${this.count}\n`);
				++this.count;
			}, 1000);
		}
	}
	onCharacteristicNotification(characteristic, value) {
		trace(String.fromArrayBuffer(value));
	}
}

let bleFriend = new BLEFriend;

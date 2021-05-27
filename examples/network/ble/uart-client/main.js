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
 	Nordic UART Service Client 
	The Nordic UART Service is a simple GATT-based service with TX and RX characteristics.
	Strings written to the UART Service server are echoed back to the client as characteristic notifications.

	https://devzone.nordicsemi.com/f/nordic-q-a/10567/what-is-nus-nordic-uart-service
 */

import BLEClient from "bleclient";
import Timer from "timer";
import {uuid} from "btutils";

const SERVICE_UUID = uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`;
const CHARACTERISTIC_RX_UUID = uuid`6E400002-B5A3-F393-E0A9-E50E24DCCA9E`;
const CHARACTERISTIC_TX_UUID = uuid`6E400003-B5A3-F393-E0A9-E50E24DCCA9E`;

const UART_CLIENT = "UART Client";

class UARTClient extends BLEClient {
	onReady() {
		this.onDisconnected();
	}
	onDiscovered(device) {
		if ("UART" == device.scanResponse.completeName) {
			this.stopScanning();
			this.connect(device);
		}
	}
	onDisconnected() {
		if (this.timer) {
			Timer.clear(this.timer);
			delete this.timer;
		}
		this.count = 1;
		this.startScanning();
	}
	onConnected(device) {
		device.discoverPrimaryService(SERVICE_UUID);
	}
	onServices(services) {
		if (services.length)
			services[0].discoverAllCharacteristics();
	}
	onCharacteristics(characteristics) {
		let rx_characteristic, tx_characteristic;
		for (let i = 0; i < characteristics.length; ++i) {
			let characteristic = characteristics[i];
			if (characteristic.uuid.equals(CHARACTERISTIC_RX_UUID))
				rx_characteristic = characteristic;
			if (characteristic.uuid.equals(CHARACTERISTIC_TX_UUID))
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
		trace.right(value, UART_CLIENT);
	}
}

let client = new UARTClient;

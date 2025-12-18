/*
* Copyright (c) 2025  Moddable Tech, Inc.
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

import {BluetoothDevice} from "web-bluetooth"
import {GAPClient} from "embedded:io/bluetoothle/central"
import Timer from "timer";

//
// Web Bluetooth does not provide fine grain control over BLE scanning.
// Here we use the ECMA-419 GAPClient to scan. The GAPClient allows complete
// control over the scanning process, so the script can select a device based
// on whatever rules it has. The script may also cancel the scan after some timeout
// interval, which is not supported byh Web Bluetooth's requestDevice.
// Once the device is discovered, Web Bluetooth is used as usual to iteract with
// the GATT device.
//
function scanForHeartRateMonitor(timeout) {
	return new Promise((resolve, reject) => {
		let  timer;
		const scan = new GAPClient({
			services: [
				"180d"		// heart rate monitor
			],
			onReadable() {
				const advertisement = this.read();
				this.close();
				
				Timer.clear(timer);
				resolve(new BluetoothDevice(advertisement.address, advertisement.name));
			},
			onError(error) {
				reject(error ?? new Error("scan failed"));
			}
		});

		if (timeout) {
			timer = Timer.set(() => {
				scan.close();
				reject(new Error("scan time out"));
			}, timeout);
		}
	});
}

const device = await scanForHeartRateMonitor(2000);
trace(`Discovered "${device.name}"\n`);

const server = await device.gatt.connect();
trace("gatt.connect succeeded\n");

const service = await server.getPrimaryService('heart_rate');
trace("getPrimaryService success\n");

const characteristic = await service.getCharacteristic('heart_rate_measurement');
trace("get getCharacteristic success\n");

await characteristic.startNotifications();
trace("startedNotifications\n");

characteristic.addEventListener('characteristicvaluechanged', function(event) {
	const value = event.target.value;
	let offset = 1;
	const flags = value.getUint8(0);
	const rate16Bits = flags & 0x1;

	const heartRate = rate16Bits
	? value.getUint16(offset, true)
	: value.getUint8(offset);

	trace(`Heart Rate: ${heartRate} bpm\n`);
});

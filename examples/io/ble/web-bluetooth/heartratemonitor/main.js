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

import {bluetooth} from "web-bluetooth"

const device = await bluetooth.requestDevice({
	filters: [{ services: ['heart_rate'] }]
});

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

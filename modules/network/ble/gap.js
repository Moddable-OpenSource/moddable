/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

/**
 * Kinoma LowPAN Framework: Kinoma Bluetooth Stack
 * Bluetooth v4.2 - Generic Access Profile (LE Only)
 */

import ByteBuffer from "buffers";

function toAdvertisingDataArray(structures) {
	let buffer = ByteBuffer.allocateUint8Array(MAX_AD_LENGTH, true);
	writeAdvertisingData(buffer, structures);
	buffer.flip();
	return buffer.getByteArray();
}

function readAdvertisingData(buffer) {
	let structures = [];
	while (buffer.remaining() > 0) {
		let length = buffer.getInt8();
		if (length == 0) {
			/* Early termination of data */
			break;
		}
		let structure = {
			type: buffer.getInt8(),
			data: buffer.getByteArray(length - 1)
		};
		structures.push(structure);
	}
	return structures;
}

function writeAdvertisingData(buffer, structures) {
	for (let i = 0; i < structures.length; i++) {
		let structure = structures[i]
		if (structure.data == null) {
			break;
		}
		buffer.putInt8(structure.data.length + 1);
		buffer.putInt8(structure.type);
		buffer.putByteArray(structure.data);
	}
}

class GAP {
};
GAP.SCAN_FAST_INTERVAL = 0x0030;		// TGAP(scan_fast_interval)		30ms to 60ms
GAP.SCAN_FAST_WINDOW = 0x0030;			// TGAP(scan_fast_window)		30ms
GAP.SCAN_SLOW_INTERVAL1 = 0x0800;		// TGAP(scan_slow_interval1)	1.28s
GAP.SCAN_SLOW_WINDOW1 = 0x0012;			// TGAP(scan_slow_window1)		11.25ms
GAP.SCAN_SLOW_INTERVAL2 = 0x1000;		// TGAP(scan_slow_interval2)	2.56s
GAP.SCAN_SLOW_WINDOW2 = 0x0024;			// TGAP(scan_slow_window2)		22.5ms
GAP.ADV_FAST_INTERVAL1 = {				// TGAP(adv_fast_interval1)		30ms to 60ms
	intervalMin: 0x0030,
	intervalMax: 0x0060
};
GAP.ADV_FAST_INTERVAL2 = {				// TGAP(adv_fast_interval2)		100ms to 150ms
	intervalMin: 0x00A0,
	intervalMax: 0x00F0
};
GAP.ADV_SLOW_INTERVAL = {				// TGAP(adv_slow_interval)		1s to 1.2s
	intervalMin: 0x0640,
	intervalMax: 0x0780
};
GAP.ADType = {
	/* Service UUID */
	INCOMPLETE_UUID16_LIST: 0x02,
	COMPLETE_UUID16_LIST: 0x03,
	INCOMPLETE_UUID128_LIST: 0x06,
	COMPLETE_UUID128_LIST: 0x07,
	/* Local Name */
	SHORTENED_LOCAL_NAME: 0x08,
	COMPLETE_LOCAL_NAME: 0x09,
	/* Flags */
	FLAGS: 0x01,
	/* Manufacturer Specific Data */
	MANUFACTURER_SPECIFIC_DATA: 0xFF,
	/* TX Power Level */
	TX_POWER_LEVEL: 0x0A,
	SLAVE_CONNECTION_INTERVAL_RANGE: 0x12,
	/* Service Solicitation */
	SOLICITATION_UUID16_LIST: 0x14,
	SOLICITATION_UUID128_LIST: 0x15,
	/* Service Data */
	SERVICE_DATA_UUID16: 0x16,
	SERVICE_DATA_UUID128: 0x21,
	/* Appearance */
	APPEARANCE: 0x19,
	/* Public Target Address */
	PUBLIC_TARGET_ADDRESS: 0x17,
	/* Random Target Address */
	RANDOM_TARGET_ADDRESS: 0x18,
	/* Advertising Interval */
	ADVERTISING_INTERVAL: 0x1A,
	/* LE Bluetooth Device Address */
	LE_BLUETOOTH_DEVICE_ADDRESS: 0x1B,
	/* LE Role */
	LE_ROLE: 0x1C,
	/* URI */
	URI: 0x24
};
GAP.MAX_AD_LENGTH = 31;

Object.freeze(GAP.prototype);

let MIN_INITIAL_CONN_INTERVAL = 0x18;	// TGAP(initial_conn_interval)	30ms to 50ms
let MAX_INITIAL_CONN_INTERVAL = 0x28;	// TGAP(initial_conn_interval)	30ms to 50ms

export default GAP;

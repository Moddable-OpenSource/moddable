/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
	https://en.wikipedia.org/wiki/IBeacon
	https://os.mbed.com/blog/entry/BLE-Beacons-URIBeacon-AltBeacons-iBeacon/
 */

import BLEServer from "bleserver";
import GAP from "gap";
import {uuid} from "btutils";

//const MANUFACTURER_ID = 0x4c00;		// Apple
const MANUFACTURER_ID = 0x0059;			// Nordic Semiconductor ASA
const MAJOR_VERSION_NUMBER = 1234;
const MINOR_VERSION_NUMBER = 5678;
const PROXIMITY_UUID = uuid`01020304-0506-0708-09A0-112233445566`;
const RSSI = 0xC3;

class IBeacon extends BLEServer {
	onReady() {
		let data = new Uint8Array(23);
		data[0] = 0x02;									// iBeacon subtype
		data[1] = 0x15;									// iBeacon subtype length
		data.set(new Uint8Array(PROXIMITY_UUID), 2);	// proximity uuid
		data[18] = (MAJOR_VERSION_NUMBER >> 8) & 0xFF;
		data[19] = MAJOR_VERSION_NUMBER & 0xFF;
		data[20] = (MINOR_VERSION_NUMBER >> 8) & 0xFF;
 		data[21] = MINOR_VERSION_NUMBER & 0xFF;
		data[22] = RSSI;
		
		this.startAdvertising({
			advertisingData: {
				flags: GAP.ADFlag.NO_BR_EDR,
				manufacturerSpecific: { identifier: MANUFACTURER_ID, data }
			}
		});
	}
}
	
let iBeacon = new IBeacon;


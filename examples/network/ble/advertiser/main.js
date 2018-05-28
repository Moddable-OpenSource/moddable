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

import BLEServer from "bleserver";
import {uuid} from "btutils";

class Advertiser extends BLEServer {
	onReady() {
		this.deviceName = "Moddable Device";
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: "Thermometer Example", completeUUID16List: [uuid`1809`]}
		});
	}
	onConnected(connection) {
		this.stopAdvertising();
	}
}
	
let advertiser = new Advertiser;


/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import BLEServer from "bleserver";
import {uuid} from "btutils";
import GAP from "gap";

class AdvertiserNotify extends BLEServer {
	onReady() {
		this.deviceName = "Advertiser Notify";
		this.startAdvertising({
			fast: false,
			notify: true,
			advertisingData: {flags: GAP.ADFlag.NO_BR_EDR, completeName: this.deviceName}
		});
	}
	onAdvertisementSent() {
		trace("advertisement sent\n");
	}
}
	
let advertiser = new AdvertiserNotify;


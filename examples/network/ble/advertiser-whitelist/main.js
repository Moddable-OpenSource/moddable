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

import BLEServer from "bleserver";
import GAP from "gap";
import GAPWhitelist from "gapwhitelist";
import Timer from "timer";
import {uuid} from "btutils";

const ADDRESS = "XX:XX:XX:XX:XX:XX";

class AdvertiserWhitelist extends BLEServer {
	onReady() {
		if ("XX:XX:XX:XX:XX:XX" === ADDRESS)
			throw new Error("Set ADDRESS to your peer's Bluetooth address");
		this.deviceName = "Advertiser Example";
		GAPWhitelist.add(ADDRESS);
		this.onDisconnected();
	}
	onConnected(connection) {
		this.stopAdvertising();
		this.timer = Timer.set(() => {
			delete this.timer;
			this.disconnect();
		}, 8000);
	}
	onDisconnected(connection) {
		if (this.timer) {
			Timer.clear(this.timer);
			delete this.timer;
		}
		this.startAdvertising({
			filterPolicy: GAP.AdvFilterPolicy.WHITELIST_SCANS_CONNECTIONS,
			advertisingData: {flags: 6, completeName: this.deviceName, completeUUID16List: [uuid`1809`]}
		});
	}
}
	
let advertiser = new AdvertiserWhitelist;


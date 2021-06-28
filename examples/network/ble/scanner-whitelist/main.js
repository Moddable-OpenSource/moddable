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

import BLEClient from "bleclient";
import GAPWhitelist from "gapwhitelist";
import GAP from "gap";

const ADDRESS = "XX:XX:XX:XX:XX:XX";

class ScannerWhitelist extends BLEClient {
	onReady() {
		if ("XX:XX:XX:XX:XX:XX" === ADDRESS)
			throw new Error("Set ADDRESS to your peer's Bluetooth address");
		GAPWhitelist.add(ADDRESS);
		this.startScanning({ filterPolicy:GAP.ScanFilterPolicy.WHITELIST });
	}
	onDiscovered(device) {
		const completeName = device.scanResponse.completeName;
		if (completeName)
			trace(`${completeName} - ${device.address}\n`);
	}
}

let scanner = new ScannerWhitelist;


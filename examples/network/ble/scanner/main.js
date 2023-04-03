/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

class Scanner extends BLEClient {
	addresses = new Set;

	onReady() {
		this.startScanning({});
	}
	onDiscovered(device) {
		const address = device.address.toString();
		if (this.addresses.has(address))
			return;
		this.addresses.add(address);

		trace(`${address} ${device.scanResponse.completeName ?? device.scanResponse.shortName ?? ""}\n`);
	}
}

let scanner = new Scanner;

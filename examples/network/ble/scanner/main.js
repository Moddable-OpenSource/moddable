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

class Scanner extends BLEClient {
	onReady() {
		this.startScanning({ duplicates:false });
	}
	onDiscovered(device) {
		let scanResponse = device.scanResponse;
		let completeName = scanResponse.completeName;
		if (completeName)
			trace(`${completeName} - ${device.address}\n`);
	}
}

let scanner = new Scanner;


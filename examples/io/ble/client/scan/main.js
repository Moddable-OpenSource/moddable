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

import {GAPClient} from "embedded:io/bluetoothle/central"

const scan = new GAPClient({
	onReadable(count) {
		while (count--) {
			const advertisement = this.read();
			const name = advertisement.name;
			const services = advertisement.services;
			const {manufacturer /*, data */} = advertisement.manufacturerData ?? {};
			
			trace(`#${++this.advertisements}: ${advertisement.address}\n`);
			if (name)
				trace(`  name: ${name}\n`);
			if (services)
				trace(`  services: ${services.join(", ")}\n`);
			if (undefined !== manufacturer)
				trace(`  manufacturer: ${manufacturer.toString(16).padStart(4, "0")}\n`);
		}
	},
	onError() {
		trace("scan failed\n");
	}
});
scan.advertisements = 0;

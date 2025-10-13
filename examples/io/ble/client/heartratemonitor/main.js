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

import {GAPClient, GATTClient} from "embedded:io/bluetoothle/central"

new GAPClient({
	services: [
		"180d"		// heart rate monitor
	],
	onReadable() {
		const advertisement = this.read();
		this.close();

		instantiateHeartRateMonitor(advertisement.address);
	},
	onError() {
		trace("scan failed\n");
	}
});

function instantiateHeartRateMonitor(address) {
	trace(`Instantiate Heart Rate Monitor ${address}\n`);

	new GATTClient({
		address,
		onReady() {
			trace("Connected.\n");
			this.getPrimaryServices([ "180d"], (error, services) => {
				this.getCharacteristics(services[0], ["2a37"], (error, characteristics) => {
					this.heartRate = characteristics[0];
					trace(` heartRate.handle ${this.heartRate.handle}\n`);
					this.subscribe(characteristics[0]);
				});
			});
		},
		onError() {
			trace("connection error\n");
		},
		onReadable(count) {
			while (count--) { 
				const value = this.read();
				if (!value || (value.handle !== this.heartRate.handle))
					continue;

				const view = new DataView(value);
				let offset = 1;
				const flags = view.getUint8(0);
				const rate16Bits = flags & 0x1;
				const heartRate = rate16Bits ? view.getUint16(offset, true) : view.getUint8(offset);
				trace(`  Heart Rate: ${heartRate} bpm\n`);
			}
		}
	});
}

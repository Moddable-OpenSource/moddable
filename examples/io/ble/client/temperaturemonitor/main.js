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

/*

	Health Thermometer Profile
		https://www.bluetooth.com/specifications/specs/html/?src=htp-v1-0_1753144152/HTP_v1.0/out/en/index-en.html
 */

import {GAPClient, GATTClient} from "embedded:io/bluetoothle/central"

new GAPClient({
	services: [
		"1809"		// temperature monitor
	],
	onReadable() {
		const advertisement = this.read();
		this.close();

		instantiateTemperatureMonitor(advertisement.address);
	},
	onError() {
		trace("scan failed\n");
	}
});

function instantiateTemperatureMonitor(address) {
	trace(`Instantiate Temperature Monitor ${address}\n`);

	new GATTClient({
		address,
		security: {		// "just works" security configuration
			authenticate: false,		/* default – false */
			bond: false,				/* default – false */
			ioCapabilities: "none",	/* default – none */
			immediate: false,			/* default – false */
		},
		onError() {
			trace("connection error\n");
		},
		onPasskey(action, data) {
			trace("** implement onPasskey as required by your security settings **\n");
			this.replyToPasskey(action, data);
		},
		onSecured(state) {
			trace(`onSecured: encrypted ${state.encrypted}, authenticated ${state.authenticated}, keySize ${state.keySize}, bonded ${state.bonded}\n`);

			this.getPrimaryServices([ "1809"], (error, services) => {
				this.getCharacteristics(services[0], ["2a1c"], (error, characteristics) => {
					this.temperature = characteristics[0];
					trace(` temperature.handle ${this.temperature.handle}\n`);
					this.subscribe(characteristics[0]);
				});
			});
		},
		onReadable(count) {
			while (count--) { 
				const value = this.read();
				if (!value || (value.handle !== this.temperature.handle))
					continue;

				const view = new DataView(value);
				const units = view.getUint8(0);
				if (1 !== units)
					return trace(`expected units of 1, got ${units}\n`);
				const exponent = view.getUint8(4);
				const temperature = (view.getUint32(1, true) & 0x00FFFFFF) * Math.pow(10, (exponent & 0x80) ? -(~exponent + 257) : exponent);
				trace(`${temperature.toFixed(2)}\n`);
			}
		}
	});
}

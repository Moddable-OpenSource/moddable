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

import { GAPClient, GATTClient } from "embedded:io/bluetoothle/central";

function scan() {
	new GAPClient({
		services:[
			"6e400001-b5a3-f393-e0a9-e50e24dcca9e",		// Nordic UART service
		],
		onReadable() {
			const advertisement = this.read();
			this.close();
			testUART(advertisement.address);
		}
	});
}
scan();

function testUART(address) {
	new GATTClient({ 
		address,
		onError(error) {
			trace(`onError: ${ error }\n`);
			scan();
		},
		onReadable(count) {
			while (count--) { 
				const value = this.read();
				if (!value || (value.handle !== this.input.handle))
					continue;
				const bytes = new Uint8Array(value);
				trace(`UART: ${bytes.toHex()}\n`);
			}
		},
		onReady() {
			trace(`onReady: maximumWrite ${ this.maximumWrite }\n`);
			this.getPrimaryServices([ "6e400001-b5a3-f393-e0a9-e50e24dcca9e"], (error, services) => {
				this.getCharacteristics(services[0], ["6e400002-b5a3-f393-e0a9-e50e24dcca9e", "6e400003-b5a3-f393-e0a9-e50e24dcca9e"], (error, characteristics) => {
					characteristics.forEach(characteristic => {
						trace(`characteristic: ${ characteristic.uuid } ${ characteristic.handle }\n`);
						if (characteristic.properties & GATTClient.properties.read)
							this.input = characteristic;
						if (characteristic.properties & GATTClient.properties.write)
							this.output = characteristic;
					});
					this.subscribe(this.input, error => {
						trace(`notification: ${ this.input.uuid } ${ error ? "error " + error : "enabled"}\n`);
					});
					this.write(this.output, Uint8Array.of(0, 1, 2, 3), error => {
						trace(`write 1: ${ error ?? "no error" }\n`);
					});
					this.write(this.output, ArrayBuffer.fromString("Hello, BLE."), error => {
						trace(`write 2: ${ error ?? "no error" }\n`);
					});
				});
			});
		},
	});
}

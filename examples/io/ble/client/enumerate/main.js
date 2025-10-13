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
		"180d"		// heart-rate monitor
	],
	onReadable(/* count */) {
		const advertisement = this.read();
		this.close();

		trace(`Discovered "${advertisement.name}"\n`);
		enumeratePeripheral(advertisement.address);
	},
	onError() {
		trace("scan failed\n");
	}
});
trace("Scanning for a Heart Rate Monitor...\n");

function enumeratePeripheral(address) {
	new GATTClient({
		address,
		onReady() {
			this.getPrimaryServices((error, services) => {
				sortByUUID(services);
				services.forEach(service => {
					this.getCharacteristics(service, (error, characteristics) => {
						sortByUUID(characteristics);
						service.characteristics = characteristics;
						characteristics.forEach(characteristic => {
							this.getDescriptors(characteristic, (error, descriptors) => {
								sortByUUID(descriptors);
								characteristic.descriptors = descriptors;
								if ((service === services.at(-1)) && (characteristic === characteristics.at(-1))) {
									services.forEach(service => {
										trace(`Service ${service.uuid.toUpperCase()}\n`);
										service.characteristics.forEach(characteristic => {
											trace(`  Characteristic ${characteristic.uuid.toUpperCase()}: ${mapProperties(characteristic)}\n`);
											characteristic.descriptors.forEach(descriptor => {
												trace(`    Descriptor ${descriptor.uuid.toUpperCase()}\n`);
											});
										});
									});
								}
							});
						});
					});
				});
			});
		}
	});
}

function sortByUUID(list) {
	list.sort((a, b) => a.uuid > b.uuid);
}

function mapProperties(characteristic)
{
	const properties = characteristic.properties;
	const result = [];
	for (let name in GATTClient.properties) {
		if (GATTClient.properties[name] & properties)
			result.push(name);
	}
	return result.join(", ");
}

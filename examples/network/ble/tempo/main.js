/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
 	Blue Maestro environment monitor: https://www.bluemaestro.com/product/tempo-environment-monitor/
 */

import BLEClient from "bleclient";

const TempoManufacturerID = 307;

class Tempo extends BLEClient {
	onReady() {
		this.startScanning();
	}
	onDiscovered(device) {
		let manufacturerSpecific = device.scanResponse.manufacturerSpecific;
		
		// If this is a Tempo device...
		if (manufacturerSpecific && (TempoManufacturerID == manufacturerSpecific.identifier)) {
			let data = manufacturerSpecific.data;
			if (data[0] == 0 || data[0] == 1) {	// ...and product model T30 or THP
				let temperature, humidity, pressure;
				temperature = (data[3] | (data[4] << 8)) / 10;
				if (data.length > 7) {
					humidity = data[7];
					pressure = data[8] | (data[9] << 8);
				}
				trace(`Temperature: ${temperature} ˚C\n`);
				if (humidity)
					trace(`Relative humidity: ${humidity} %\n`);
				if (pressure)
					trace(`Barometric pressure: ${pressure} hPa\n`);
				trace('\n');
			}
		}
	}
}

let tempo = new Tempo;

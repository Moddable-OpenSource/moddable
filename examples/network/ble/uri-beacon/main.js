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
 	https://github.com/google/uribeacon/tree/uribeacon-final/specification
 	https://itunes.apple.com/us/app/physical-web/id927653608?mt=8
 */

import BLEServer from "bleserver";

const URI = "http://www.moddable.com";

class URIBeacon extends BLEServer {
	onReady() {
		this.startAdvertising({
			advertisingData: {completeUUID16List: ["FED8"], serviceDataUUID16: {uuid: 0xFED8, data: this.encodeData(URI)}}
		});
	}
	encodeData(uri) {
		let flags = 0;
		let txPower = 0xEE;
		let prefix, suffix;
		
		if (uri.startsWith('http://www.')) {
			uri = uri.slice(11);
			prefix = 0;
		}
		else if (uri.startsWith('https://www.')) {
			uri = uri.slice(12);
			prefix = 1;
		}
		else if (uri.startsWith('http://')) {
			uri = uri.slice(7);
			prefix = 2;
		}
		else if (uri.startsWith('https://')) {
			uri = uri.slice(8);
			prefix = 3;
		}
		else if (uri.startsWith('urn:uuid:')) {
			uri = uri.slice(9);
			prefix = 4;
		}
		else
			throw new Error('invalid uri prefix');

		if (uri.endsWith('.com/')) {
			uri = uri.slice(0, -5);
			suffix = 0;
		}
		else if (uri.endsWith('.org/')) {
			uri = uri.slice(0, -5);
			suffix = 1;
		}
		else if (uri.endsWith('.edu/')) {
			uri = uri.slice(0, -5);
			suffix = 2;
		}
		else if (uri.endsWith('.net/')) {
			uri = uri.slice(0, -5);
			suffix = 3;
		}
		else if (uri.endsWith('.info/')) {
			uri = uri.slice(0, -6);
			suffix = 4;
		}
		else if (uri.endsWith('.biz/')) {
			uri = uri.slice(0, -5);
			suffix = 5;
		}
		else if (uri.endsWith('.gov/')) {
			uri = uri.slice(0, -5);
			suffix = 6;
		}
		else if (uri.endsWith('.com')) {
			uri = uri.slice(0, -4);
			suffix = 7;
		}
		else if (uri.endsWith('.org')) {
			uri = uri.slice(0, -4);
			suffix = 8;
		}
		else if (uri.endsWith('.edu')) {
			uri = uri.slice(0, -4);
			suffix = 9;
		}
		else if (uri.endsWith('.net')) {
			uri = uri.slice(0, -4);
			suffix = 10;
		}
		else if (uri.endsWith('.info')) {
			uri = uri.slice(0, -5);
			suffix = 11;
		}
		else if (uri.endsWith('.biz')) {
			uri = uri.slice(0, -4);
			suffix = 12;
		}
		else if (uri.endsWith('.gov')) {
			uri = uri.slice(0, -4);
			suffix = 13;
		}
		else
			throw new Error('invalid uri suffix');
			
		let data = new Array(flags, txPower, prefix);
		data = data.concat(Array.from(new Uint8Array(ArrayBuffer.fromString(uri))));
		data.push(suffix);
		return data;
	}
}
	
let advertiser = new URIBeacon;


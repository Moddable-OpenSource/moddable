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

const prefixes = [ 'http://www.', 'https://www.', 'http://', 'https://', 'urn:uuid:' ];
const suffixes = [ '.com/', '.org/', '.edu/', '.net/', '.info/', '.biz/', '.gov/', '.com', '.org', '.edu', '.net', '.info', '.biz', '.gov' ];

class URIBeacon extends BLEServer {
	onReady() {
		this.startAdvertising({
			advertisingData: {completeUUID16List: ["FED8"], serviceDataUUID16: {uuid: 0xFED8, data: this.encodeData(URI)}}
		});
	}
	encodeData(uri) {
		let flags = 0;
		let txPower = 0xEE;
		let prefix = prefixes.findIndex(item => uri.startsWith(item));
		if (-1 == prefix)
			throw new Error('invalid uri prefix');
		uri = uri.slice(prefixes[prefix].length);
		let suffix = suffixes.findIndex(item => uri.endsWith(item));
		if (-1 == suffix)
			throw new Error('invalid uri suffix');
		uri = uri.slice(0, -suffixes[suffix].length);

		let data = new Array(flags, txPower, prefix);
		data = data.concat(Array.from(new Uint8Array(ArrayBuffer.fromString(uri))));
		data.push(suffix);
		return data;
	}
}
	
let advertiser = new URIBeacon;


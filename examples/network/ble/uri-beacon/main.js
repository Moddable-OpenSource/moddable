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

const schemes = [ 'http://www.', 'https://www.', 'http://', 'https://', 'urn:uuid:' ];
const domains = [ '.com/', '.org/', '.edu/', '.net/', '.info/', '.biz/', '.gov/', '.com', '.org', '.edu', '.net', '.info', '.biz', '.gov' ];

class URIBeacon extends BLEServer {
	onReady() {
		this.startAdvertising({
			advertisingData: {completeUUID16List: ["FED8"], serviceDataUUID16: {uuid: 0xFED8, data: this.encodeData(URI)}}
		});
	}
	encodeData(uri) {
		let flags = 0;
		let txPower = 0xEE;
		let scheme = schemes.findIndex(item => uri.startsWith(item));
		if (-1 == scheme)
			throw new Error('invalid uri scheme');
		uri = uri.slice(schemes[scheme].length);
		let data = new Array(flags, txPower, scheme);
		domains.some((domain, index) => {
			let idx = uri.indexOf(domain);
			if (-1 != idx) {
				data = data.concat(Array.from(new Uint8Array(ArrayBuffer.fromString(uri.substr(0, idx)))));
				data.push(index);
				uri = uri.slice(idx + domains[index].length);
				return true;
			}
		});
		if (uri.length)
			data = data.concat(Array.from(new Uint8Array(ArrayBuffer.fromString(uri))));
		return data;
	}
}
	
let advertiser = new URIBeacon;


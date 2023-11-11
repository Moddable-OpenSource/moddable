/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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

import TextDecoder from "text/decoder"
import TLSSocket from "embedded:io/socket/tlssocket";

const http = new device.network.http.io({ 
	...device.network.http,
	host: "www.google.com",
	port: 443,
	socket: {
		io: TLSSocket,
		TCP: device.network.http.socket
	}
});
for (let i = 0; i < 3; i++) {
	http.request({
		path: `/?${i}`,
		headers: new Map([
			["date", Date()],
			["user-agent", "ecma-419 test"]
		]),
		onHeaders(status, headers) {
			trace(`Status ${status}\n`);
			headers.forEach((value, key) => {
				trace(`${key}: ${value}\n`);
			});
			this.decoder = new TextDecoder;
		},
		onReadable(count) {
			const buffer = this.read(count);
			trace(this.decoder.decode(buffer, {stream: true})); 
		},
		onDone(error) {
			if (error)
				trace(error, `\n\n **ERROR ${i} **\n\n`);
			else
				trace(this.decoder.decode(), `\n\n **DONE ${i} **\n\n`);
		}
	});
}

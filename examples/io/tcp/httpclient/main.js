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

import HTTPClient from "httpclient";

const client = new HTTPClient({
	host: "example.com"
});
for (let i = 0; i < 3; i++) {
	client.request({
		path: `/?${i}`,
		headers: new Map([
			["Date", Date()],
			["user-agent", "ecma-419 test"]
		]),
		onHeaders(status, headers) {
			trace(`Status ${status}, Content-Type ${headers.get("content-type")}\n`);
		},
		onReadable(count) {
			let data = this.read(count);
			try {
				data = String.fromArrayBuffer(data); 
			}
			catch {
				data = " ** ENCODING ERROR ** ";
			}
			trace(data);
		},
		onDone() {
			trace(`\n\n **DONE ${i} **\n\n`);
		}
	});
}

/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

const http = new device.network.https.client.io({
	...device.network.https.client,
	host: "www.moddable.com"
});
http.request({
	path: `/example/firmware/xs_esp32.bin`,
	onHeaders(status, headers /* , statusText */) {
		if (200 !== status) {
			trace(`HTTP GET failed with status ${status}.\n`);
			return void http.close();
		}

		this.options = {
			partition: device.flash.open({
				path: "nextota"
			}),
			mode: "a"
		};
		const byteLength = headers.get("content-length");
		if (byteLength)
			this.options.byteLength = parseInt(byteLength);
		this.update = device.update.open(this.options);
	},
	onReadable(count) {
		trace(`${count} bytes received.\n`);
		this.update.write(this.read(count));
	},
	onDone(error) {
		trace(`Update complete${error ? ` with error (${error})` : ""}.\n`);
		if (!error)
			this.update.complete();
		this.update?.close();
		this.options?.partition.close();
		http.close();
	}
});

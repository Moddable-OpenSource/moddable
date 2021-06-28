/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

import { Request } from "http";
import OTA from "esp32/ota";
import Time from "time";

class OTARequest extends Request {
	constructor(url, response) {
		let slash = url.indexOf("://");
		url = url.substring(slash + 3);
		slash = url.indexOf("/");

		const dict = { host: url.substring(0, slash), path: url.substring(slash), }
		if (response) dict.response = response;
		super(dict);
		
		this.length = undefined;
		this.received = 0;
		this.error = 0;
this.startms = Time.ticks;
	}

	callback(message, value, v2) {
		switch (message) {
			case Request.status:
				if (200 !== value) {
					trace(`Error, unexpected HTTP response ${value}\n`);
					this.close();
					if (this.ota)
						this.ota.cancel();
					this.error = value;
				}
				break;
			case Request.header:
				if ("content-length" === value) {
					this.length = parseInt(v2);
					trace(`Content-Length ${this.length} now: ${Time.ticks}\n`);
					this.ota = new OTA;
					trace(`after new OTA: ${Time.ticks}\n`);
				}
				break;
			case Request.headerComplete:
				if (!this.length) {
					trace(`Error: no Content-Length\n`);
					this.close();
				}
				break;
			case Request.responseFragment:
				let bytes = this.read(ArrayBuffer);

				this.received += bytes.byteLength;
				this.ota.write(bytes);
				trace(`received ${bytes.byteLength}:${this.received} of ${this.length}\n`);
				break;
			case Request.responseComplete:
				trace("received complete\n");
trace(`before complete: ${Time.ticks}\n`);
				this.ota.complete();
trace(`after complete: ${Time.ticks} - total ${Time.ticks - this.startms}\n`);
				if (undefined !== this.onFinished)
					this.onFinished();
				break
			default:
				if (message < 0) {
					trace(`Error, ${message}\n`);
					this.close();
					if (this.ota)
						this.ota.cancel();
					this.error = message;
				}
				break;
		}
	}
}

Object.freeze(OTARequest.prototype);
export default (OTARequest);



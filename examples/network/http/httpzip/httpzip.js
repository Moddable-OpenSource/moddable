/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
 * Copyright (c) Wilberforce
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

import {Server} from "http"
import {ZIP} from "zip"

const mime = new Map;
mime.set("js", "application/javascript");
mime.set("css", "text/css");
mime.set("ico", "image/vnd.microsoft.icon");
mime.set("txt", "text/plain");
mime.set("htm", "text/html");
mime.set("html", "text/html");
mime.set("svg", "image/svg+xml");
mime.set("png", "image/png");
mime.set("gif", "image/gif");
mime.set("webp", "image/webp");
mime.set("jpg", "image/jpeg");
mime.set("jpeg", "image/jpeg");

class WebServer extends Server {
	#onRoute;

	constructor(options, archive) {
		super(options);

		this.#onRoute = options.onRoute;
		this.archive = new ZIP(archive);
	}
	
	callback(message, value, etc) {
		switch (message) {
			case Server.status:
				value = this.server.#onRoute?.(value) ?? value;

				this.path = value.slice(1);		// drop leading /
				this.match = false;

				// Look for file in archive
				try {
					this.data = this.server.archive.file(this.path);
					this.etag = "mod-" + this.data.crc.toString(16);
				}
				catch {
					delete this.data;
				}
				break;

			case Server.header:
				this.match ||= ("if-none-match" === value) && (this.etag === etc);
				break;

			case Server.prepareResponse:
				if (this.match) {
					return {
						status: 304,
						headers: [
							"ETag", this.etag,
						]
					};
				}
				if (!this.data) {
					return {
						status: 404
					};
				}

				this.position = 0;
				const result = {
					headers: [
						"Content-type", mime.get(this.path.split('.').pop()) ?? "text/plain",
						"Content-length", this.data.length,
						"ETag", this.etag,
						"Cache-Control", "max-age=60"
					],
					body: true
				}
				if (8 === this.data.method) // Compression Method
					result.headers.push("Content-Encoding", "deflate");
				return result;

			case Server.responseFragment:
				if (this.position >= this.data.length)
					return;

				const chunk = this.data.read(ArrayBuffer, (value > 1536) ? 1536 : value);
				this.position += chunk.byteLength;
				return chunk;
				break;

			case Server.requestComplete: // request body received
				break;
		}
	}
}

Object.freeze(mime);

export {WebServer as Server};

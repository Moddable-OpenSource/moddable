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

import { Bridge, HTTPServer } from "bridge/webserver";
import Resource from "Resource";
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

export class BridgeHttpZip extends Bridge {
	constructor(resource) {
		super();

		this.archive = new ZIP(new Resource(resource));
	}
	
	handler(req, message, value, etc) {
		switch (message) {
			case HTTPServer.status:
				// redirect home page
				if (value === '/') value='/index.html';
				req.path = value;		
				try {
					req.data = this.archive.file(req.path.slice(1)); // drop leading / to match zip content
					req.etag = "mod-" + req.data.crc.toString(16);
				}
				catch {
					delete req.data;
					delete req.etag;
					return this.next?.handler(req, message, value, etc);
				}
				break;

			case HTTPServer.header:
				req.match ||= ("if-none-match" === value) && (req.etag === etc);
				return this.next?.handler(req, message, value, etc);

			case HTTPServer.prepareResponse:
				if (req.match) {
					return {
						status: 304,
						headers: [
							"ETag", req.etag,
						]
					};
				}
				if (!req.data) {
					trace(`prepareResponse: missing file ${req.path}\n`);

					return this.next?.handler(req, message, value, etc);
				}

				req.data.current = 0;
				const result = {
					headers: [
						"Content-type", mime.get(req.path.split('.').pop()) ?? "text/plain",
						"Content-length", req.data.length,
						"ETag", req.etag,
						"Cache-Control", "max-age=60"
					],
					body: true
				}
				if (8 === req.data.method) // Compression Method
					result.headers.push("Content-Encoding", "deflate");
				return result;

			case HTTPServer.responseFragment:
				if (req.data.current >= req.data.length)
					return;

				const chunk = req.data.read(ArrayBuffer, (value > 1536) ? 1536 : value);
				req.data.current += chunk.byteLength;
				return chunk;			
		}	
	}
}

Object.freeze(mime);
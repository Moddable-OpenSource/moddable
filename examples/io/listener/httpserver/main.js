/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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

import HTTPServer from "embedded:network/http/server"
import Listener from "embedded:io/socket/listener";

const reply = ArrayBuffer.fromString("1 2 3 4 5 6 7 8\n");

new HTTPServer({ 
	io: Listener,
	port: 80,
	onConnect(connection) {
		connection.accept({
			onRequest(request) {
				trace(`${request.method} ${request.path}\n`);
				for (const [header, value] of request.headers)
					trace(`${header}: ${value}\n`);				
			},
			onReadable(count) {
				trace(`${String.fromArrayBuffer(this.read(count))}\n`);
			},
			onResponse(response) {
				response.headers.set("content-length", reply.byteLength);
				this.respond(response);
			},
			onWritable(count) {
				this.write(reply);
			},
			onDone() {
				trace("done\n");
			},
			onError() {
				trace("error\n");
			}
		});
	}
});

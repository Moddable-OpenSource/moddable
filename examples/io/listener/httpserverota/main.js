/*
 * Copyright (c) 2022-2026  Moddable Tech, Inc.
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
import OTA from "embedded:update";
import flash from "embedded:storage/flash";
import Timer from "timer";
import FFI from "mc/ffi";
const Natives = new FFI;

import WebPage from "embedded:network/http/server/options/webpage";

const port = 80;
const router = new Map;
const notFound = {
	...WebPage,		// STATIC ROUTE
	data: ArrayBuffer.fromString("Not found\n"),
};

new HTTPServer({
	io: Listener,
	port,
	onConnect(connection) {
		connection.accept({
			onRequest(request) {
				this.route = router.get(request.path) ?? notFound;
			},
		})
	}
});

router.set("/ota", {
	onRequest(request) {
		this.status = 200;
		if ("PUT" !== request.method) {
			this.status = 405;
			return;
		}
		try {
			this.bytesReceived = 0;
			this.byteLength = parseInt(request.headers.get("content-length") ?? 0)
			this.updater = OTA.open({partition: flash.open({path: "nextota"})});
		}
		catch {
			this.status = 500;
		}
	},
	onReadable(count) {
		try {
			const bytes = this.read();
			this.bytesReceived += bytes.byteLength;
			this.updater?.write(bytes);
		}
		catch {
			this.status = 500;
		}
	},
	onResponse(response) {
		try {
			this.updater?.complete();
			Timer.set(Natives.esp_restart, 1000);
		}
		catch {
			this.status = 500;
		}
		finally {
			this.updater?.close();
		}
		response.status = this.status;
		response.headers.set("content-length", 0);
		this.respond(response);
	},
	onError() {
		this.updater?.close();
	}
});

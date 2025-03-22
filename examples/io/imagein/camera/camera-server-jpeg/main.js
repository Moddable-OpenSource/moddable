/*
 * Copyright (c) 2024  Moddable Tech, Inc.
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
import Camera from "embedded:x-io/imagein/camera";
import Net from "net";

let width = 176, height = 144;
let frame;

const camera = new Camera({
	width,
	height,
	imageType: "jpeg",
	format: "buffer/disposable",
	onReadable: () => {
		if (frame?.inUse) {
			const f = camera.read();		// ignore frame
			f?.close();
			return;
		}
		frame?.close();
		frame = camera.read();
		frame.inUse = 0;
	}
});
width = camera.width;
height = camera.height;
camera.start();

const router = new Map;

const port = 8080;
const server = new HTTPServer({
	io: Listener,
	port,
	onConnect(connection) {
		connection.accept({
			onRequest(request) {
				if (frame && router.get(request.path))
					this.route = router.get(request.path);
				else
					this.close();
			},
		})
	}
})

router.set("/", {
	onRequest(request) {
		this.frame = frame;
		frame.bytes ??= new Uint8Array(frame);
		frame.inUse += 1;
		this.position = 0;
	},
	onReadable(count) {
		this.read();
	},
	onResponse(response) {
		response.headers.set("content-length", this.frame.byteLength);
		response.headers.set("content-type", "image/jpeg");
		this.respond(response);
	},
	onWritable(count) {
		const use = Math.min(count, frame.byteLength - this.position);
		this.write(frame.bytes.subarray(this.position, this.position + use));
		this.position += use;
	},
	onDone() {
		if (frame)
			frame.inUse -= 1;
	},
	onError() {
		if (frame)
			frame.inUse -= 1;
	}
});

trace(`JPEG Camera Server ready at: http://${Net.get("IP")}:${port}\n`);

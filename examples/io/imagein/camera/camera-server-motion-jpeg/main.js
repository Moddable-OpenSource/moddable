/*
 * Copyright (c) 2024-2025  Moddable Tech, Inc.
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
import WebPage from "embedded:network/http/server/options/webpage";
import Listener from "embedded:io/socket/listener";
import Camera from "embedded:x-io/imagein/camera";
import Net from "net";

let frame;

const camera = new Camera({
	width: 320,
	height: 240,
	imageType: "jpeg",
	format: "buffer/disposable",
	onReadable: () => {
		if (frame?.inUse) {
			const f = camera.read();		// ignore frame
			f?.close();
		}
		else {
			frame?.close();
			frame = camera.read();
			if (frame)
				frame.inUse = 0;
		}

		connections.forEach(connection => {
			if (connection.writable)
				connection.route.onWritable.call(connection, connection.writable);
		});
	}
});
const width = camera.width;
const height = camera.height;
camera.start();

const router = new Map;
const connections = new Set;
const boundary = "419cameraboundary";

const server = new HTTPServer({
	io: Listener,
	port: 8080,
	onConnect(connection) {
		connection.accept({
			onRequest(request) {
				const route = router.get(request.path);
				if (frame && route)
					this.route = route;
				else
					this.close();
			},
		})
	}
});

router.set("/camera", {
	onReadable() {
		this.read();
	},
	onResponse(response) {
		connections.add(this);

		response.headers.set("content-type", `multipart/x-mixed-replace; boundary="${boundary}"`);
		this.respond(response);
	},
	onWritable(count) {
		if (!this.frame) {
			this.frame = frame;
			frame.bytes ??= new Uint8Array(frame);
			frame.inUse += 1;
			this.position = 0;

			this.preamble = new Uint8Array(ArrayBuffer.fromString(`\r\n--${boundary}\r\ncontent-length: ${this.frame.byteLength}\r\ncontent-type: image/jpeg\r\n\r\n`));
		}

		if (this.preamble) {
			if (count < this.preamble.length) {
				this.write(this.preamble.subarray(0, count));
				this.preamble = this.preamble.slice(count);
				this.writable = 0;
				return;
			}

			this.write(this.preamble);
			count -= this.preamble.length;
			delete this.preamble;
			if (!count) {
				this.writable = 0;
				return;
			}
		}

		const use = Math.min(count, this.frame.byteLength - this.position);
		this.write(this.frame.bytes.subarray(this.position, this.position + use));
		this.position += use;
		this.writable = count - use;
		
		if (this.position === this.frame.byteLength) {
			this.frame.inUse -= 1;
			delete this.frame;
		}
	},
	onDone() {
		if (this.frame) {
			this.frame.inUse -= 1;
			delete this.frame;
		}
		connections.delete(this);
	},
	onError() {
		if (this.frame) {
			this.frame.inUse -= 1;
			delete this.frame;
		}
		connections.delete(this);
	}
});

//@@ update to use StaticRoute when available
router.set("/", { 
	...WebPage,
	msg: ArrayBuffer.fromString(`<img src="./camera">`)
});

const host = `${Net.get("IP")}:${server.port}`;
trace(`Camera ready.\nWeb page: http://${host}\nCamera Motion JPEG camera: http://${host}/camera\n`);

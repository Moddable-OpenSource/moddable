/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

import ArduCAM from "arducam"
import {Server} from "http"

let camera;

let server = new Server({});
server.callback = function(message, value)
{
	if (2 == message) {
		let width = 320, height = 240;

		let parts = value.split("?");
		if (parts[0] != "/stream") {
			this.status = 404;
			this.phase = -1;
			return;
		}

		this.status = 200;
		this.phase = 1;

		if (2 == parts.length) {
			parts = parts[1].split("&");
			for (let i = 0; i < parts.length; i++) {
				let part = parts[i].split("=");
				if ("w" == part[0])
					width = parseInt(part[1]);
				else if ("h" == part[0])
					height = parseInt(part[1]);
			}
		}
		if (!camera || ((width != camera.width) || (height != camera.height))) {
			if (camera) {
				camera.close();
				camera = undefined;
			}
			try {
				camera = new ArduCAM({width, height, format: "jpeg"});
				camera.width = width;
				camera.height = height;
			}
			catch (e) {
				this.status = 400;
				this.phase = -1;
			}
		}
	}

	if (8 == message) {
		if (200 != this.status)
			return {status: this.status, headers: ["Content-Type", "text/html"], body: 'Try <a href="/stream?w=320&h=240">/stream?w=320&h=240</a>'};

		return {headers: ["Content-Type", "multipart/x-mixed-replace;boundary=MODDABLE", "Content-Length", 0x7FFFFFFF], body: true};
	}

	if (9 == message) {
		switch (this.phase) {
			case 1:
				this.phase = 2;
				return "--MODDABLE\r\n";

			case 2:
				this.byteLength = camera.capture();
				this.phase = 3;
				return `Content-Type: image/jpeg\r\nContent-Length: ${this.byteLength}\r\n\r\n`;

			case 3:
				let byteLength = Math.min(this.byteLength, value);

				if (!camera.pixels || (byteLength != camera.pixels.byteLength)) {
					camera.pixels = undefined;
					camera.pixels = new ArrayBuffer(byteLength);
				}

				this.byteLength -= byteLength;
				if (!this.byteLength)
					this.phase = 4;

				return camera.read(camera.pixels) ? camera.pixels : undefined;

			case 4:
				this.phase = 2;
				return "\r\n--MODDABLE\r\n";

			case -1:
				return;
		}
	}
}

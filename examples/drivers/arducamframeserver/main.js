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

let camera = new ArduCAM({width: 320, height: 240, format: "jpeg"});

let server = new Server({});
server.callback = function(message, value)
{
	if (2 == message)
		this.path = value;

	if (8 == message) {
		this.byteLength = camera.capture();
		return {headers: ["Content-Type", "image/jpeg", "Content-Length", this.byteLength], body: true};
	}

	if (9 == message) {
		let byteLength = Math.min(this.byteLength, value);
		if (!byteLength)
			return;

		if (!camera.pixels || (byteLength != camera.pixels.byteLength)) {
			camera.pixels = undefined;
			camera.pixels = new ArrayBuffer(byteLength);
		}

		this.byteLength -= byteLength;
		return camera.read(camera.pixels) ? camera.pixels : undefined;
	}
}

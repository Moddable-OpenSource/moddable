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

import ArduCAM from "arducam";
import {Request} from "http";
import Timer from "timer";

let index = 300;
let camera = new ArduCAM({width: 160, height: 120, format: "jpeg"});

function captureOne() {
	let byteLength = camera.capture();
	trace(`captured ${byteLength} bytes\n`);

	let request = new Request({	host: "alistairarducam.s3.amazonaws.com",
								path: "/arduCamPhoto" + index++ + ".jpg",
								method: "PUT",
								body: true,
								response: String,
								headers: ["Cache-Control", "no-cache", "content-length", byteLength.toString()]
							});

	request.callback = function(message, value, value2)
	{
		if (2 == message)
			trace(`${value}: ${value2}\n`);

		if (1 == message)
			trace(`HTTP Status ${value}\n`);

		if (0 == message) {
			let count = Math.min(1024, byteLength, value);
			if (0 == count) {
				trace("all jpeg bytes sent\n");
				return;
			}
			if (!camera.buffer || (camera.buffer.byteLength != count)) {
				camera.buffer = undefined
				camera.buffer = new ArrayBuffer(count);
			}
			trace(`transmit ${count} jpeg bytes\n`);
			byteLength -= camera.read(camera.buffer);
			return camera.buffer;
		}

		if (5 == message) {
			trace(`PUT complete, index ${index}\n`);

			Timer.set(id => captureOne(), 1);
		}
	}
}

Timer.set(id => captureOne());

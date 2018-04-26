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

import config from "mc/config";
import Timer from "timer";
import {Request} from "http";
import SecureSocket from "securesocket";

// Adafruit IO REST API Documentation
// https://io.adafruit.com/api/docs/

function streamData(data) {
	let request = new Request({ 
		host: "io.adafruit.com",
		path: `/api/v2/${config.username}/feeds/${config.feedKey}/data`,
		method: "POST",
		headers: [ "X-AIO-Key", config.AIOKey, "Content-Type", "application/json" ],
		body: JSON.stringify({ value: data }),
		response: String,
		Socket: SecureSocket,
		port: 443,
		secure: {protocolVersion: 0x302}
	});
	request.callback = function(message, value, etc) {
		if ((message == 2) && (value == "status")) {
			if (etc == "200 OK") {
				trace(`Sent data "${data}"\n`);
			} else {
				trace(`Error sending data "${data}". Error code: ${etc}\n`);
			}
		}
	}
}

Timer.repeat(() => {
	streamData(Math.floor(Math.random() * 100) + 1);
}, 5000);
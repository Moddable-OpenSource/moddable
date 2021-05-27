/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

import {Request} from "http"

const kBodyPart = "Fragment...\n";
const kRepeat = 128;

let request = new Request({
	host: "httpbin.org",
	path: "/post",
	method: "POST",
	body: true,		// callback is provided by Request.requestFragment callbacks
	headers: ["Content-Length", kBodyPart.length * kRepeat, "Content-Type", "text/plain"],
	response: String
});

request.bytesToSend = kBodyPart.length * kRepeat;

request.callback = function(message, value) {
	if (Request.requestFragment === message) {
		if (0 === this.bytesToSend)
			return;		// no more body
		this.bytesToSend -= kBodyPart.length;
		return kBodyPart;	// fragment of body
	}
	else if (Request.responseComplete === message) {
		value = JSON.parse(value);
		trace(`Posted body: ${value.data}\n`);
	}
}

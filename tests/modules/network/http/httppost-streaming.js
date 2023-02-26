/*---
description: 
flags: [async, module]
---*/

import {Request} from "http";

await $NETWORK.connected;

const bodyPart = "[bodyPart]";
const totalParts = 10;
let remainingParts = totalParts;

const request = new Request({host: "httpbin.org", path: "/post", method: "POST",
	headers: [
		'Content-Type', 'text/plain',
		'Content-Length', totalParts * bodyPart.length,
		"Date", Date(),
	],
	body: true,
	response: String,
});
request.callback = function(message, value, etc) {
	if (Request.requestFragment === message) {
		if (0 === remainingParts--)
			return;
		return bodyPart;
	}
	else if (Request.responseFragment === message)
		$DONE("unexpected fragment");
	else if (Request.status === message) {
		if (200 !== value)
			$DONE("unexpected http result code " + value)
	}
	else if (Request.responseComplete == message) {
		if ("string" !== typeof value)
			$DONE("unexpected response type " + (typeof value))
		else {
			value = JSON.parse(value);
			if (value.data !== bodyPart.repeat(totalParts))
				$DONE("bad response");
			else
				$DONE();
		}
	}
	else if (message < 0)
		$DONE(message)
}

$TESTMC.timeout(10_000);

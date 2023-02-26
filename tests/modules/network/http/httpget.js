/*---
description: 
flags: [async, module]
---*/

import {Request} from "http";

await $NETWORK.connected;

const request = new Request({host: "www.example.com", path: "/", response: String});
request.callback = function(message, value, etc) {
	if (Request.responseFragment === message)
		$DONE("unexpected fragment");
	else if (Request.status === message) {
		if (200 !== value)
			$DONE("unexpected http result code " + value)
	}
	else if (Request.responseComplete == message) {
		if ("string" !== typeof value)
			$DONE("unexpected response type " + (typeof value))
		else
			$DONE();
	}
	else if (message < 0)
		$DONE(message)
}

$TESTMC.timeout(5_000);
